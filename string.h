#ifndef CHATNET_STRING_H_
#define CHATNET_STRING_H_

#include "autofree.h"
#include <stdlib.h>

typedef struct {
	char *str;
	size_t len;
} string;

char *strinit(size_t len);

void strrealloc(char **dest, char *src);

void strappend(char **dest, char *src);

#endif // CHATNET_STRING_H_
