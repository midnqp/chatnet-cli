#ifndef CHATNET_GCMALLOC_H_
#define CHATNET_GCMALLOC_H_
// Make sure this is of the last included files,
// to ensure that the following remaps don't get cleared.

#include <gc/gc.h>

#define malloc GC_malloc
#define realloc GC_realloc
#define free GC_free

#endif