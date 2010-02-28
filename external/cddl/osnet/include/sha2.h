#ifndef _SHA256_H_
#define _SHA256_H_

#include <string.h>

#include_next <sha2.h>

#define SHA256Init(b)	SHA256_Init(b)
#define SHA256Update	SHA256_Update

static void
SHA256Final(void *digest, SHA256_CTX *ctx)
{
	uint8_t tmp[SHA256_DIGEST_LENGTH];

	SHA256_Final(tmp, ctx);
	memcpy(digest, &tmp, sizeof(tmp));
}


#endif
