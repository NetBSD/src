/*
 * Special version of sha512.c that uses the libc SHA512 implementation
 * of libc.
 */


#include <string.h>
#include <sys/sha2.h>

static const uint64_t sha512_224_initial_hash_value[] = {
	0x8c3d37c819544da2ULL,
	0x73e1996689dcd4d6ULL,
	0x1dfab7ae32ff9c82ULL,
	0x679dd514582f9fcfULL,
	0x0f6d2b697bd44da8ULL,
	0x77e36f7304c48942ULL,
	0x3f9d85a86a1d36c8ULL,
	0x1112e6ad91d692a1ULL,
};

static const uint64_t sha512_256_initial_hash_value[] = {
	0x22312194fc2bf72cULL,
	0x9f555fa3c84c64c2ULL,
	0x2393b86b6f53b151ULL,
	0x963877195940eabdULL,
	0x96283ee2a88effe3ULL,
	0xbe5e1e2553863992ULL,
	0x2b0199fc2c85b8aaULL,
	0x0eb72ddc81c52ca2ULL,
};

extern int
sha512_224_init(SHA512_CTX *context);
int
sha512_224_init(SHA512_CTX *context)
{
	if (context == NULL)
		return 1;

	memcpy(context->state, sha512_224_initial_hash_value,
	    (size_t)(SHA512_DIGEST_LENGTH));
	memset(context->buffer, 0, (size_t)(SHA512_BLOCK_LENGTH));
	context->bitcount[0] = context->bitcount[1] =  0;

	return 1;

}

extern int
sha512_224_final(unsigned char *md, SHA512_CTX *context);
int
sha512_224_final(unsigned char *md, SHA512_CTX *context)
{
	unsigned char tmp[64];

	SHA512_Final(tmp, context);
	memcpy(md, tmp, 28);
	explicit_memset(tmp, 0, sizeof(tmp));
	return 1;

}

extern int
sha512_256_init(SHA512_CTX *context);
int
sha512_256_init(SHA512_CTX *context)
{
	if (context == NULL)
		return 1;

	memcpy(context->state, sha512_256_initial_hash_value,
	    (size_t)(SHA512_DIGEST_LENGTH));
	memset(context->buffer, 0, (size_t)(SHA512_BLOCK_LENGTH));
	context->bitcount[0] = context->bitcount[1] =  0;

	return 1;
}

extern int
sha512_256_final(unsigned char *md, SHA512_CTX *context);
int
sha512_256_final(unsigned char *md, SHA512_CTX *context)
{
	unsigned char tmp[64];

	SHA512_Final(tmp, context);
	memcpy(md, tmp, 32);
	explicit_memset(tmp, 0, sizeof(tmp));
	return 1;
}
