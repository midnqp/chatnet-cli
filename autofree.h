#ifndef CHATNET_AUTOFREE_H
#define CHATNET_AUTOFREE_H

#include <stddef.h>
#include <gc.h>

#define alloc GC_malloc
#define autofree_realloc GC_realloc
#define autofree_free GC_free
//void *alloc(size_t len);
//void dealloc(); // autofree_free_all();
//void* autofree_realloc(void* ptr, size_t sz);
//void autofree_free(void* ptr);

#endif // ends CHATNET_AUTOFREE_H
