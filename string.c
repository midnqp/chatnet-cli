#include <stdlib.h>
#include <string.h>

#include "autofree.h"
#include "string.h"

char *strinit(size_t len) {
	char *s = alloc(len);
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

		*dest = autofree_realloc(*dest, total);
	}
}

/* appending strings that just works! */
void strappend(char **dest, const char *src) {
	strrealloc(dest, src);
	strcat(*dest, src);
}
