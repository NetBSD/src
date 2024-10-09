/* This overwrites dist/src/libsodium/include/sodium/crypto_verify_16.h */

#include <lib/libkern/libkern.h>

static inline int
crypto_verify_16(const unsigned char *x, const unsigned char *y)
{

	/*
	 * crypto_verify_16 must return 0 if equal, -1 if not.
	 *
	 * consttime_memequal returns 1 if equal, 0 if not.
	 *
	 * Hence we simply subtract one.
	 */
	return consttime_memequal(x, y, 16) - 1;
}
