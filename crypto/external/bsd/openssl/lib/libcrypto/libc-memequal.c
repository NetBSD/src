#include <string.h>
/*
 * Special version of CRYPTO_memcmp for platforms with no assembly versions
 */

#include <string.h>
#include <openssl/crypto.h>

int
CRYPTO_memcmp(
    const volatile void * volatile in_a,
    const volatile void * volatile in_b,
    size_t len)
{
	return consttime_memequal(__UNVOLATILE(in_a), __UNVOLATILE(in_b), len);
}
