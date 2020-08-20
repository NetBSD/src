/* This overwrites dist/src/libsodium/include/sodium/randombytes.h */

#include <sys/cprng.h>

static inline void
randombytes_buf(void * const buf, const size_t size)
{

	cprng_strong(kern_cprng, buf, size, 0);
}

static inline void
randombytes_stir(void)
{
}
