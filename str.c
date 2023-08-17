#include <stdlib.h>
#include <string.h>
#include <gc.h>

#include "str.h"

char *strinit(size_t len) {
	char *s = GC_malloc(len);
	strcpy(s, ""); // adds \0
	return s;
}

/* auto reallocs target ptr based on src string! */
void strrealloc(char **dest, const char *src) {
	int ptr_len = strlen(*dest);
	int src_len = strlen(src);
	if (src_len > 0) {
		int total = ptr_len + src_len;
		if (src[src_len - 1] != '\0')
			total++;
		else if (*dest[ptr_len - 1] != '\0')
			total++;

		*dest = GC_realloc(*dest, total);
	}
}

/* appending strings that just works! */
void strappend(char **dest, const char *src) {
	strrealloc(dest, src);
	strcat(*dest, src);
}

char *strtrim(char *string) {
  // Find the first non-whitespace character.
  int start = 0;
  while (string[start] == ' ' || string[start] == '\n' || string[start] == '\r') {
    start++;
  }

  // Find the last non-whitespace character.
  int end = strlen(string) - 1;
  while (end >= 0 && (string[end] == ' ' || string[end] == '\n' || string[end] == '\r')) {
    end--;
  }

  // Truncate the string to the non-whitespace characters.
  if (start > 0 || end < strlen(string) - 1) {
    memmove(string, string + start, end - start + 1);
    string[end - start + 1] = '\0';
  }

  return string;
}

char* streq(const char* a, const char* b) {
	return strcmp(a,b)==0;
}