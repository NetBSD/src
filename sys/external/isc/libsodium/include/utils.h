#ifndef __SODIUM_UTILS_H__
#define __SODIUM_UTILS_H__

/* This overwrites dist/src/libsodium/include/sodium/utils.h */

#define SODIUM_C99(X) X

static inline void
sodium_memzero(void *const pnt, const size_t len)
{

	explicit_memset(pnt, 0, len);
}

/* Just copied from dist/src/libsodium/sodium/utils.c */
static inline int
sodium_is_zero(const unsigned char *n, const size_t nlen)
{
    size_t                 i;
    volatile unsigned char d = 0U;

    for (i = 0U; i < nlen; i++) {
        d |= n[i];
    }
    return 1 & ((d - 1) >> 8);
}
#endif /* __SODIUM_UTILS_H__ */
