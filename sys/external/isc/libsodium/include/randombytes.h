/* This overwrites dist/src/libsodium/include/sodium/randombytes.h */

static inline void
randombytes_buf(void * const buf, const size_t size)
{

	extern size_t cprng_fast(void *, size_t);
	cprng_fast(buf, size);
}

static inline void
randombytes_stir(void)
{
}
