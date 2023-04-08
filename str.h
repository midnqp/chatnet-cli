#ifndef CHATNET_STRING_H_
#define CHATNET_STRING_H_

#include <stddef.h>

typedef struct {
	char *str;
	size_t len;
} string;

char *strinit(size_t len);
void strrealloc(char **dest, const char *src);
void strappend(char **dest, const char *src);

#endif // CHATNET_STRING_H_
