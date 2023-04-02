#include "string.h"

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

typedef struct {
	char *str;
	size_t len;
} string;
