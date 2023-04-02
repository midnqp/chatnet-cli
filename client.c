#include <libgen.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <leveldb/c.h>

#include "./autofree.c"

#ifndef DEBUG
#define DEBUG 0
#endif
#define _add(a, b) a b
#define debuglog(label, ...)                                                   \
  do {                                                                         \
    if (DEBUG)                                                                 \
      printf(_add(_add("\033[104m", label), "\033[0m") __VA_ARGS__);           \
  } while (0);

char *strinit(size_t len) {
  char *s = alloc(len);
  strcpy(s, ""); // adds \0
  return s;
}

/* auto reallocs target ptr based on src string! */
void strrealloc(char **dest, char *src) {
  int ptr_len = strlen(*dest);
  int src_len = strlen(src);
  if (src_len > 0) {
    int total = ptr_len + src_len;
    if (src[src_len - 1] != '\0')
      total++;
    else if (*dest[ptr_len - 1] != '\0')
      total++;

    freeable_remove(*dest);
    *dest = realloc(*dest, total);
    // in modern sysetms, that never fails. 0%.
    // that was a major error, since realloc freed the pointer
    // which was noted by libautofree.
    // that's why, adding back this pointer to libautofree.
    freeable_add(*dest);
  }
}

/* appending strings that just works! */
void strappend(char **dest, char *src) {
  strrealloc(dest, src);
  strcat(*dest, src);
}

/* check if file or folder exists */
bool entexists(char *filename) { return access(filename, F_OK) != -1; }

char *getdbpath() {
  char *result = strinit(1);
  strappend(&result, getenv("HOME"));
  strappend(&result, "/.config/chatnet-client");
  return result;
}

typedef struct {
  /* ok = 0
   * not found = 1
   * corruption = 2
   * not supported = 3
   * invalid = 4
   * io error = 5
   *
   * @see https://github.com/google/leveldb
   */
  int code;
  char message[1024];
} Status;

typedef struct {
  char *str;
  size_t len;
} StrictStr;

const char* sioc_name = "chatnet-sio-client";

Status initsioclient(char *execname) {
  Status err;
  char *dir = dirname(execname);

  char *sioclientpath = strinit(1);
  strappend(&sioclientpath, dir);
  strappend(&sioclientpath, "/");
  strappend(&sioclientpath, sioc_name);
  if (!entexists(sioclientpath)) {
    err.code = 1;
    sprintf(err.message,
            "binary \"%s\" not found in the same folder.", sioc_name);
    dealloc();
    return err;
  }

  char *prebuildspath = strinit(1);
  strappend(&prebuildspath, dir);
  strappend(&prebuildspath, "/prebuilds");
  if (!entexists(prebuildspath)) {
    err.code = 1;
    sprintf(err.message, "folder \"prebuilds\" not found in the same folder.");
    dealloc();
    return err;
  }

  char* cmd = strinit(1);
  strappend(&cmd, sioclientpath);
  strappend(&cmd, " &");
  int opened = system(cmd);
  if (opened == -1) {
    err.code = 5;
    sprintf(err.message, "launch of \"%s\" failed.", sioc_name);
    return err;
  }

  int sleepc = 0;
  while (1) {
    if (sleepc == 10) {
      err.code = 2;
      sprintf(err.message, "database not initiated.");
      return err;
    }

    if (entexists(getdbpath()))
      break;
    sleepc++;
    sleep(1);
  }

  err.code = 0;
  return err;
}

StrictStr leveldbget(leveldb_t *db, const char *key, char **errptr) {
  leveldb_readoptions_t *ropts = leveldb_readoptions_create();
  size_t len = -1;
  char *data = leveldb_get(db, ropts, key, strlen(key), &len, errptr);
  leveldb_readoptions_destroy(ropts);

  StrictStr result;
  result.len = len;
  result.str = strinit(len+1);
  memcpy(result.str, data, len);
  result.str[len] = '\0';
  leveldb_free(data);
  return result;
}

void leveldbput(leveldb_t *db, const char *key, const char *val,
                char **errptr) {
  leveldb_writeoptions_t *wopts = leveldb_writeoptions_create();
  leveldb_put(db, wopts, key, strlen(key), val, strlen(val), errptr);
  leveldb_writeoptions_destroy(wopts);
}

int main(int argc, char *argv[]) {
  int exitcode = 0;

  Status siocerr = initsioclient(argv[0]);
  if (siocerr.code != 0) {
    fprintf(stderr, "%s\n", siocerr.message);
    return siocerr.code;
  }

  leveldb_t *db;
  char *leveldberr = NULL;
  char *leveldbkey;
  size_t leveldblen = -1;
  leveldb_options_t *options = leveldb_options_create();
  leveldb_options_set_create_if_missing(options, 1);
  leveldb_readoptions_t *ropts = leveldb_readoptions_create();
  leveldb_writeoptions_t *wopts = leveldb_writeoptions_create();

  db = leveldb_open(options, getdbpath(), &leveldberr);
  leveldb_options_destroy(options);
  if (leveldberr != NULL) {
    fprintf(stderr, "database connection failed\n%s\n", leveldberr);
    exitcode = 2; // corruption
    goto cleanup;
  }

  leveldberr = NULL;
  leveldbkey = "userstate";
  leveldbput(db, leveldbkey, "true", &leveldberr);
  /*leveldb_put(db, wopts, leveldbkey, strlen(leveldbkey), "true", 4,*/
              /*&leveldberr);*/
  if (leveldberr != NULL) {
    fprintf(stderr, "database save failed\n%s\n", leveldberr);
    exitcode = 2;
    goto cleanup;
  }

  leveldberr = NULL;
  leveldbkey = "userstate";
  StrictStr userstate = leveldbget(db, leveldbkey, &leveldberr);
  /*char *userstate = leveldb_get(db, ropts, leveldbkey, strlen(leveldbkey),
   * &leveldblen, &leveldberr);*/
  if (leveldberr != NULL) {
    fprintf(stderr, "database query failed\n%s\n", leveldberr);
    exitcode = 2;
    goto cleanup;
  }
  printf("userstate found: %zu %s\n", userstate.len, userstate.str);

cleanup:
  if (leveldberr != NULL)
    leveldb_free(leveldberr);
  leveldb_readoptions_destroy(ropts);
  leveldb_writeoptions_destroy(wopts);
  leveldb_close(db);

  return exitcode;
}
