/* ppp-sha1.c - SHA1 Digest implementation
 *
 * Copyright (c) 2022 Eivind Næss. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *
 * 3. The name(s) of the authors of this software must not be used to
 *    endorse or promote products derived from this software without
 *    prior written permission.
 *
 * THE AUTHORS OF THIS SOFTWARE DISCLAIM ALL WARRANTIES WITH REGARD TO
 * THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS, IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY
 * SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN
 * AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING
 * OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * Sections of this code holds different copyright information.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#include <stddef.h>

#include "crypto-priv.h"


/* #define SHA1HANDSOFF * Copies data before messing with it. */
#ifdef OPENSSL_HAVE_SHA
#include <openssl/evp.h>

#if OPENSSL_VERSION_NUMBER < 0x10100000L
#define EVP_MD_CTX_free EVP_MD_CTX_destroy
#define EVP_MD_CTX_new EVP_MD_CTX_create
#endif

static int sha1_init(PPP_MD_CTX *ctx)
{
    if (ctx) {
        EVP_MD_CTX *mctx = EVP_MD_CTX_new();
        if (mctx) {
            if (EVP_DigestInit(mctx, EVP_sha1())) {
                ctx->priv = mctx;
                return 1;
            }
            EVP_MD_CTX_free(mctx);
        }
    }
    return 0;
}

static int sha1_update(PPP_MD_CTX *ctx, const void *data, size_t len)
{
    if (EVP_DigestUpdate((EVP_MD_CTX*) ctx->priv, data, len)) {
        return 1;
    }
    return 0;
}

static int sha1_final(PPP_MD_CTX *ctx, unsigned char *out, unsigned int *len)
{
    if (EVP_DigestFinal((EVP_MD_CTX*) ctx->priv, out, len)) {
        return 1;
    }
    return 0;
}

static void sha1_clean(PPP_MD_CTX *ctx)
{
    if (ctx->priv) {
        EVP_MD_CTX_free((EVP_MD_CTX*) ctx->priv);
        ctx->priv = NULL;
    }
}


#else // !OPENSSL_HAVE_SHA

/*
 * ftp://ftp.funet.fi/pub/crypt/hash/sha/sha1.c
 *
 * SHA-1 in C
 * By Steve Reid <steve@edmweb.com>
 * 100% Public Domain
 *
 * Test Vectors (from FIPS PUB 180-1)
 * "abc"
 * A9993E36 4706816A BA3E2571 7850C26C 9CD0D89D
 * "abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq"
 * 84983E44 1C3BD26E BAAE4AA1 F95129E5 E54670F1
 * A million repetitions of "a"
 * 34AA973C D4C4DAA4 F61EEB2B DBAD2731 6534016F
 */

#include <string.h>
#include <netinet/in.h>	/* htonl() */

typedef struct {
    uint32_t state[5];
    uint32_t count[2];
    unsigned char buffer[64];
} SHA1_CTX;


static void
SHA1_Transform(uint32_t[5], const unsigned char[64]);

#define rol(value, bits) (((value) << (bits)) | ((value) >> (32 - (bits))))

/* blk0() and blk() perform the initial expand. */
/* I got the idea of expanding during the round function from SSLeay */
#define blk0(i) (block->l[i] = htonl(block->l[i]))
#define blk(i) (block->l[i&15] = rol(block->l[(i+13)&15]^block->l[(i+8)&15] \
    ^block->l[(i+2)&15]^block->l[i&15],1))

/* (R0+R1), R2, R3, R4 are the different operations used in SHA1 */
#define R0(v,w,x,y,z,i) z+=((w&(x^y))^y)+blk0(i)+0x5A827999+rol(v,5);w=rol(w,30);
#define R1(v,w,x,y,z,i) z+=((w&(x^y))^y)+blk(i)+0x5A827999+rol(v,5);w=rol(w,30);
#define R2(v,w,x,y,z,i) z+=(w^x^y)+blk(i)+0x6ED9EBA1+rol(v,5);w=rol(w,30);
#define R3(v,w,x,y,z,i) z+=(((w|x)&y)|(w&x))+blk(i)+0x8F1BBCDC+rol(v,5);w=rol(w,30);
#define R4(v,w,x,y,z,i) z+=(w^x^y)+blk(i)+0xCA62C1D6+rol(v,5);w=rol(w,30);


/* Hash a single 512-bit block. This is the core of the algorithm. */

static void
SHA1_Transform(uint32_t state[5], const unsigned char buffer[64])
{
    uint32_t a, b, c, d, e;
    typedef union {
	unsigned char c[64];
	uint32_t l[16];
    } CHAR64LONG16;
    CHAR64LONG16 *block;

#ifdef SHA1HANDSOFF
    static unsigned char workspace[64];
    block = (CHAR64LONG16 *) workspace;
    memcpy(block, buffer, 64);
#else
    block = (CHAR64LONG16 *) buffer;
#endif
    /* Copy context->state[] to working vars */
    a = state[0];
    b = state[1];
    c = state[2];
    d = state[3];
    e = state[4];
    /* 4 rounds of 20 operations each. Loop unrolled. */
    R0(a,b,c,d,e, 0); R0(e,a,b,c,d, 1); R0(d,e,a,b,c, 2); R0(c,d,e,a,b, 3);
    R0(b,c,d,e,a, 4); R0(a,b,c,d,e, 5); R0(e,a,b,c,d, 6); R0(d,e,a,b,c, 7);
    R0(c,d,e,a,b, 8); R0(b,c,d,e,a, 9); R0(a,b,c,d,e,10); R0(e,a,b,c,d,11);
    R0(d,e,a,b,c,12); R0(c,d,e,a,b,13); R0(b,c,d,e,a,14); R0(a,b,c,d,e,15);
    R1(e,a,b,c,d,16); R1(d,e,a,b,c,17); R1(c,d,e,a,b,18); R1(b,c,d,e,a,19);
    R2(a,b,c,d,e,20); R2(e,a,b,c,d,21); R2(d,e,a,b,c,22); R2(c,d,e,a,b,23);
    R2(b,c,d,e,a,24); R2(a,b,c,d,e,25); R2(e,a,b,c,d,26); R2(d,e,a,b,c,27);
    R2(c,d,e,a,b,28); R2(b,c,d,e,a,29); R2(a,b,c,d,e,30); R2(e,a,b,c,d,31);
    R2(d,e,a,b,c,32); R2(c,d,e,a,b,33); R2(b,c,d,e,a,34); R2(a,b,c,d,e,35);
    R2(e,a,b,c,d,36); R2(d,e,a,b,c,37); R2(c,d,e,a,b,38); R2(b,c,d,e,a,39);
    R3(a,b,c,d,e,40); R3(e,a,b,c,d,41); R3(d,e,a,b,c,42); R3(c,d,e,a,b,43);
    R3(b,c,d,e,a,44); R3(a,b,c,d,e,45); R3(e,a,b,c,d,46); R3(d,e,a,b,c,47);
    R3(c,d,e,a,b,48); R3(b,c,d,e,a,49); R3(a,b,c,d,e,50); R3(e,a,b,c,d,51);
    R3(d,e,a,b,c,52); R3(c,d,e,a,b,53); R3(b,c,d,e,a,54); R3(a,b,c,d,e,55);
    R3(e,a,b,c,d,56); R3(d,e,a,b,c,57); R3(c,d,e,a,b,58); R3(b,c,d,e,a,59);
    R4(a,b,c,d,e,60); R4(e,a,b,c,d,61); R4(d,e,a,b,c,62); R4(c,d,e,a,b,63);
    R4(b,c,d,e,a,64); R4(a,b,c,d,e,65); R4(e,a,b,c,d,66); R4(d,e,a,b,c,67);
    R4(c,d,e,a,b,68); R4(b,c,d,e,a,69); R4(a,b,c,d,e,70); R4(e,a,b,c,d,71);
    R4(d,e,a,b,c,72); R4(c,d,e,a,b,73); R4(b,c,d,e,a,74); R4(a,b,c,d,e,75);
    R4(e,a,b,c,d,76); R4(d,e,a,b,c,77); R4(c,d,e,a,b,78); R4(b,c,d,e,a,79);
    /* Add the working vars back into context.state[] */
    state[0] += a;
    state[1] += b;
    state[2] += c;
    state[3] += d;
    state[4] += e;
    /* Wipe variables */
    a = b = c = d = e = 0;
}


/* SHA1Init - Initialize new context */

static void
SHA1_Init(SHA1_CTX *context)
{
    /* SHA1 initialization constants */
    context->state[0] = 0x67452301;
    context->state[1] = 0xEFCDAB89;
    context->state[2] = 0x98BADCFE;
    context->state[3] = 0x10325476;
    context->state[4] = 0xC3D2E1F0;
    context->count[0] = context->count[1] = 0;
}


/* Run your data through this. */

static void
SHA1_Update(SHA1_CTX *context, const unsigned char *data, unsigned int len)
{
    unsigned int i, j;

    j = (context->count[0] >> 3) & 63;
    if ((context->count[0] += len << 3) < (len << 3)) context->count[1]++;
    context->count[1] += (len >> 29);
    i = 64 - j;
    while (len >= i) {
	memcpy(&context->buffer[j], data, i);
	SHA1_Transform(context->state, context->buffer);
	data += i;
	len -= i;
	i = 64;
	j = 0;
    }

    memcpy(&context->buffer[j], data, len);
}


/* Add padding and return the message digest. */

static void
SHA1_Final(unsigned char digest[20], SHA1_CTX *context)
{
    uint32_t i, j;
    unsigned char finalcount[8];

    for (i = 0; i < 8; i++) {
        finalcount[i] = (unsigned char)((context->count[(i >= 4 ? 0 : 1)]
         >> ((3-(i & 3)) * 8) ) & 255);  /* Endian independent */
    }
    SHA1_Update(context, (unsigned char *) "\200", 1);
    while ((context->count[0] & 504) != 448) {
	SHA1_Update(context, (unsigned char *) "\0", 1);
    }
    SHA1_Update(context, finalcount, 8);  /* Should cause a SHA1Transform() */
    for (i = 0; i < 20; i++) {
	digest[i] = (unsigned char)
		     ((context->state[i>>2] >> ((3-(i & 3)) * 8) ) & 255);
    }
    /* Wipe variables */
    i = j = 0;
    memset(context->buffer, 0, 64);
    memset(context->state, 0, 20);
    memset(context->count, 0, 8);
    memset(&finalcount, 0, 8);
#ifdef SHA1HANDSOFF  /* make SHA1Transform overwrite it's own static vars */
    SHA1Transform(context->state, context->buffer);
#endif
}

static int sha1_init(PPP_MD_CTX *ctx)
{
    if (ctx) {
        SHA1_CTX *mctx = calloc(1, sizeof(SHA1_CTX));
        if (mctx) {
            SHA1_Init(mctx);
            ctx->priv = mctx;
            return 1;
        }
    }
    return 0;
}

static int sha1_update(PPP_MD_CTX* ctx, const void *data, size_t len)
{
    SHA1_Update((SHA1_CTX*) ctx->priv, (void*) data, len);
    return 1;
}

static int sha1_final(PPP_MD_CTX *ctx, unsigned char *out, unsigned int *len)
{
    SHA1_Final(out, (SHA1_CTX*) ctx->priv);
    return 1;
}

static void sha1_clean(PPP_MD_CTX *ctx)
{
    if (ctx->priv) {
        free(ctx->priv);
        ctx->priv = NULL;
    }
}

#endif

static PPP_MD ppp_sha1 = {
    .init_fn = sha1_init,
    .update_fn = sha1_update,
    .final_fn = sha1_final,
    .clean_fn = sha1_clean,
};

const PPP_MD *PPP_sha1(void)
{
    return &ppp_sha1;
}

