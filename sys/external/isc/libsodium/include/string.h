#include <sys/cdefs.h>
#include <sys/types.h>

extern void *memset(void *, int, size_t);
extern void *memcpy(void * restrict, const void * restrict, size_t);
extern void *memmove(void *, const void *, size_t);
