#ifndef CHATNET_DB_H_
#define CHATNET_DB_H_

/* get value by key, fatal exit on error */
char *ipc_get(const char *key);

/* put value by key, fatal exit on error */
void ipc_put(const char *key, const char *val);

#endif // ends CHATNET_DB_H_
