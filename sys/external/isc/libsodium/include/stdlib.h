#include <sys/cdefs.h>
#include <sys/malloc.h>
#undef malloc
#undef free
#define	malloc(size)	kern_malloc(size, 0)
#define	free(addr)	kern_free(addr)

#define abort()		panic("abort")
