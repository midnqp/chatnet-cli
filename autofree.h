#ifndef CHATNET_AUTOFREE_H
#define CHATNET_AUTOFREE_H

#include <malloc.h>
#include <stdio.h>
#include <string.h>

void *alloc(size_t len);
void dealloc(); // autofree_free_all();
void* autofree_realloc(void* ptr, size_t sz);
void autofree_free(void* ptr);

#endif // ends CHATNET_AUTOFREE_H
