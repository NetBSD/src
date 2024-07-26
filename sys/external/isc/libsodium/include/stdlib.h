#include <sys/cdefs.h>
#include <sys/malloc.h>
#undef malloc
#undef free
#define	malloc(size)	__malloc_should_not_be_used
#define	free(addr)	__free_should_not_be_used

#define abort()		panic("libsodium internal error")
