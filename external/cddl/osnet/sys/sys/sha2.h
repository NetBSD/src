#ifndef _SHA2_H_
#define _SHA2_H_

#include_next <sys/sha2.h>

#define SHA2_CTX	SHA256_CTX
#define SHA2Init(a, b)	SHA256_Init(b)
#define SHA2Update 	SHA256_Update

static void
SHA2Final(void *digest, SHA2_CTX *ctx)
{
	uint8_t tmp[SHA256_DIGEST_LENGTH];

	SHA256_Final(tmp, ctx);
	memcpy(digest, &tmp, sizeof(tmp));
}


#endif
