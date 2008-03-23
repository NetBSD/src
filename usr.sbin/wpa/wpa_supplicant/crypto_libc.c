#include <sys/types.h>
#include <string.h>
#include <sys/md5.h>
#include <sys/sha1.h>

#include "common.h"

void md5_vector(size_t num_elem, const u8 *addr[], const size_t *len, u8 *mac)
{
	MD5_CTX ctx;
	size_t i;

	MD5Init(&ctx);
	for (i = 0; i < num_elem; i++)
		MD5Update(&ctx, addr[i], len[i]);
	MD5Final(mac, &ctx);
}


void sha1_vector(size_t num_elem, const u8 *addr[], const size_t *len, u8 *mac)
{
	SHA1_CTX ctx;
	size_t i;

	SHA1Init(&ctx);
	for (i = 0; i < num_elem; i++)
		SHA1Update(&ctx, addr[i], len[i]);
	SHA1Final(mac, &ctx);
}

int aes_wrap(const u8 *kek, int n, const u8 *plain, u8 *cipher)
{
	return -1;
}

int aes_unwrap(const u8 *kek, int n, const u8 *cipher, u8 *plain)
{
	return -1;
}
