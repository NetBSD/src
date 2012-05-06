/*-
 * Copyright (c) 2012 Alistair Crooks <agc@NetBSD.org>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/param.h>
#include <sys/syslog.h>

#ifdef _KERNEL
# include <sys/md5.h>
# include <sys/sha1.h>
# include <sys/sha2.h>
# include <sys/rmd160.h>
# include <sys/kmem.h>
#else
# include <arpa/inet.h>
# include <ctype.h>
# include <inttypes.h>
# include <md5.h>
# include <rmd160.h>
# include <sha1.h>
# include <sha2.h>
# include <stdarg.h>
# include <stdio.h>
# include <stdlib.h>
# include <string.h>
# include <time.h>
# include <unistd.h>
#endif

#include "digest.h"

static uint8_t prefix_md5[] = {
	0x30, 0x20, 0x30, 0x0C, 0x06, 0x08, 0x2A, 0x86, 0x48, 0x86,
	0xF7, 0x0D, 0x02, 0x05, 0x05, 0x00, 0x04, 0x10
};

static uint8_t prefix_sha1[] = {
	0x30, 0x21, 0x30, 0x09, 0x06, 0x05, 0x2b, 0x0E, 0x03, 0x02,
	0x1A, 0x05, 0x00, 0x04, 0x14
};

static uint8_t prefix_sha256[] = {
	0x30, 0x31, 0x30, 0x0d, 0x06, 0x09, 0x60, 0x86, 0x48, 0x01,
	0x65, 0x03, 0x04, 0x02, 0x01, 0x05, 0x00, 0x04, 0x20
};

#define V4_SIGNATURE		4

/*************************************************************************/

void                                                                                                                 
MD5_Init(MD5_CTX *context)
{
	MD5Init(context);
}

void                                                                                                                 
MD5_Update(MD5_CTX *context, const unsigned char *data, unsigned int len)
{
	MD5Update(context, data, len);
}

void                                                                                                                 
MD5_Final(unsigned char digest[16], MD5_CTX *context)
{
	MD5Final(digest, context);
}

void                                                                                                                 
SHA1_Init(SHA1_CTX *context)
{
	SHA1Init(context);
}

void                                                                                                                 
SHA1_Update(SHA1_CTX *context, const unsigned char *data, unsigned int len)
{
	SHA1Update(context, data, len);
}

void                                                                                                                 
SHA1_Final(unsigned char digest[20], SHA1_CTX *context)
{
	SHA1Final(digest, context);
}


/* initialise the hash structure */
int 
digest_alg_size(unsigned alg)
{
	switch(alg) {
	case MD5_HASH_ALG:
		return 16;
	case SHA1_HASH_ALG:
		return 20;
	case SHA256_HASH_ALG:
		return 32;
	default:
		printf("hash_any: bad algorithm\n");
		return 0;
	}
}

/* initialise the hash structure */
int 
digest_init(digest_t *hash, const uint32_t hashalg)
{
	switch(hash->alg = hashalg) {
	case MD5_HASH_ALG:
		MD5Init(&hash->u.md5ctx);
		hash->size = 16;
		hash->prefix = prefix_md5;
		hash->len = sizeof(prefix_md5);
		hash->ctx = &hash->u.md5ctx;
		return 1;
	case SHA1_HASH_ALG:
		SHA1Init(&hash->u.sha1ctx);
		hash->size = 20;
		hash->prefix = prefix_sha1;
		hash->len = sizeof(prefix_sha1);
		hash->ctx = &hash->u.sha1ctx;
		return 1;
	case SHA256_HASH_ALG:
		SHA256_Init(&hash->u.sha256ctx);
		hash->size = 32;
		hash->prefix = prefix_sha256;
		hash->len = sizeof(prefix_sha256);
		hash->ctx = &hash->u.sha256ctx;
		return 1;
	default:
		printf("hash_any: bad algorithm\n");
		return 0;
	}
}

int 
digest_update(digest_t *hash, const uint8_t *data, size_t length)
{
	switch(hash->alg) {
	case MD5_HASH_ALG:
		MD5Update(hash->ctx, data, (unsigned)length);
		return 1;
	case SHA1_HASH_ALG:
		SHA1Update(hash->ctx, data, (unsigned)length);
		return 1;
	case SHA256_HASH_ALG:
		SHA256_Update(hash->ctx, data, length);
		return 1;
	default:
		printf("hash_any: bad algorithm\n");
		return 0;
	}
}

unsigned 
digest_final(uint8_t *out, digest_t *hash)
{
	switch(hash->alg) {
	case MD5_HASH_ALG:
		MD5Final(out, hash->ctx);
		break;
	case SHA1_HASH_ALG:
		SHA1Final(out, hash->ctx);
		break;
	case SHA256_HASH_ALG:
		SHA256_Final(out, hash->ctx);
		break;
	default:
		printf("hash_any: bad algorithm\n");
		return 0;
	}
	(void) memset(hash->ctx, 0x0, hash->size);
	return (unsigned)hash->size;
}

int
digest_length(digest_t *hash, unsigned hashedlen)
{
	uint8_t		 trailer[6];

	trailer[0] = V4_SIGNATURE;
	trailer[1] = 0xFF;
	trailer[2] = (uint8_t)((hashedlen >> 24) & 0xff);
	trailer[3] = (uint8_t)((hashedlen >> 16) & 0xff);
	trailer[4] = (uint8_t)((hashedlen >> 8) & 0xff);
	trailer[5] = (uint8_t)(hashedlen & 0xff);
	digest_update(hash, trailer, sizeof(trailer));
	return 1;
}
