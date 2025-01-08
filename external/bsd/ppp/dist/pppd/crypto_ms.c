/*
 * pppcrypt.c - PPP/DES linkage for MS-CHAP and EAP SRP-SHA1
 *
 * Extracted from chap_ms.c by James Carlson.
 *
 * Copyright (c) 1995 Eric Rosenquist.  All rights reserved.
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

#include <stddef.h>

#include "crypto.h"
#include "crypto_ms.h"


/*
 * DES_set_odd_parity function are imported from openssl 3.0 project with the 
 * follwoing license:
 *
 * Copyright 1995-2020 The OpenSSL Project Authors. All Rights Reserved.
 *
 * Licensed under the Apache License 2.0 (the "License").  You may not use
 * this file except in compliance with the License.  You can obtain a copy
 * in the file LICENSE in the source distribution or at
 * https://www.openssl.org/source/license.html
 */
typedef unsigned char DES_cblock[8];
#define DES_KEY_SZ      (sizeof(DES_cblock))

static const unsigned char odd_parity[256] = {
    1, 1, 2, 2, 4, 4, 7, 7, 8, 8, 11, 11, 13, 13, 14, 14,
    16, 16, 19, 19, 21, 21, 22, 22, 25, 25, 26, 26, 28, 28, 31, 31,
    32, 32, 35, 35, 37, 37, 38, 38, 41, 41, 42, 42, 44, 44, 47, 47,
    49, 49, 50, 50, 52, 52, 55, 55, 56, 56, 59, 59, 61, 61, 62, 62,
    64, 64, 67, 67, 69, 69, 70, 70, 73, 73, 74, 74, 76, 76, 79, 79,
    81, 81, 82, 82, 84, 84, 87, 87, 88, 88, 91, 91, 93, 93, 94, 94,
    97, 97, 98, 98, 100, 100, 103, 103, 104, 104, 107, 107, 109, 109, 110,
    110,
    112, 112, 115, 115, 117, 117, 118, 118, 121, 121, 122, 122, 124, 124, 127,
    127,
    128, 128, 131, 131, 133, 133, 134, 134, 137, 137, 138, 138, 140, 140, 143,
    143,
    145, 145, 146, 146, 148, 148, 151, 151, 152, 152, 155, 155, 157, 157, 158,
    158,
    161, 161, 162, 162, 164, 164, 167, 167, 168, 168, 171, 171, 173, 173, 174,
    174,
    176, 176, 179, 179, 181, 181, 182, 182, 185, 185, 186, 186, 188, 188, 191,
    191,
    193, 193, 194, 194, 196, 196, 199, 199, 200, 200, 203, 203, 205, 205, 206,
    206,
    208, 208, 211, 211, 213, 213, 214, 214, 217, 217, 218, 218, 220, 220, 223,
    223,
    224, 224, 227, 227, 229, 229, 230, 230, 233, 233, 234, 234, 236, 236, 239,
    239,
    241, 241, 242, 242, 244, 244, 247, 247, 248, 248, 251, 251, 253, 253, 254,
    254
};

static void DES_set_odd_parity(DES_cblock *key)
{
    unsigned int i;
    for (i = 0; i < DES_KEY_SZ; i++)
        (*key)[i] = odd_parity[(*key)[i]];
}

static unsigned char
Get7Bits(const unsigned char *input, int startBit)
{
	unsigned int word;

	word  = (unsigned)input[startBit / 8] << 8;
	word |= (unsigned)input[startBit / 8 + 1];

	word >>= 15 - (startBit % 8 + 7);

	return word & 0xFE;
}

static void
MakeKey(const unsigned char *key, unsigned char *des_key)
{
	/* key     IN  56 bit DES key missing parity bits */
	/* des_key OUT 64 bit DES key with parity bits added */
	des_key[0] = Get7Bits(key,  0);
	des_key[1] = Get7Bits(key,  7);
	des_key[2] = Get7Bits(key, 14);
	des_key[3] = Get7Bits(key, 21);
	des_key[4] = Get7Bits(key, 28);
	des_key[5] = Get7Bits(key, 35);
	des_key[6] = Get7Bits(key, 42);
	des_key[7] = Get7Bits(key, 49);

	DES_set_odd_parity((DES_cblock *)des_key);
}

int
DesEncrypt(const unsigned char *clear, const unsigned char *key, unsigned char *cipher)
{
    int retval = 0;
    unsigned int clen = 0;
    unsigned char des_key[8];

    PPP_CIPHER_CTX *ctx = PPP_CIPHER_CTX_new();
    if (ctx) {

        MakeKey(key, des_key);
        
        if (PPP_CipherInit(ctx, PPP_des_ecb(), des_key, NULL, 1)) {

            if (PPP_CipherUpdate(ctx, cipher, &clen, clear, 8)) {

                if (PPP_CipherFinal(ctx, cipher + clen, &clen)) {

                    retval = 1;
                }
            }
        }
        
        PPP_CIPHER_CTX_free(ctx);
    }

	return (retval);
}

int
DesDecrypt(const unsigned char *cipher, const unsigned char *key, unsigned char *clear)
{
    int retval = 0;
    unsigned int clen = 0;
    unsigned char des_key[8];

    PPP_CIPHER_CTX *ctx = PPP_CIPHER_CTX_new();
    if (ctx) {

        MakeKey(key, des_key);
        
        if (PPP_CipherInit(ctx, PPP_des_ecb(), des_key, NULL, 0)) {

            if (PPP_CipherUpdate(ctx, clear, &clen, cipher, 8)) {

                if (PPP_CipherFinal(ctx, clear + clen, &clen)) {

                    retval = 1;
                }
            }
        }
        
        PPP_CIPHER_CTX_free(ctx);
    }

	return (retval);
}

#ifdef UNIT_TEST_MSCRYPTO

#include <string.h>
#include <stdio.h>

/**
 * The test-vectors are taken from RFC2759.
 */
int test_encrypt()
{
    unsigned char Challenge[8] = { 
        0xD0, 0x2E, 0x43, 0x86, 0xBC, 0xE9, 0x12, 0x26
    };  

    unsigned char ZPasswordHash[24] = {
        0x44, 0xEB, 0xBA, 0x8D, 0x53, 0x12, 0xB8, 0xD6,
        0x11, 0x47, 0x44, 0x11, 0xF5, 0x69, 0x89, 0xAE
    };

    unsigned char expected[24] = { 
        0x82, 0x30, 0x9E, 0xCD, 0x8D, 0x70, 0x8B, 0x5E, 
        0xA0, 0x8F, 0xAA, 0x39, 0x81, 0xCD, 0x83, 0x54, 
        0x42, 0x33, 0x11, 0x4A, 0x3D, 0x85, 0xD6, 0xDF
    };  
    unsigned char response[24] = {}; 
    unsigned int retval = 0;

    DesEncrypt(Challenge, ZPasswordHash + 0,  response + 0);
    DesEncrypt(Challenge, ZPasswordHash + 7,  response + 8);
    DesEncrypt(Challenge, ZPasswordHash + 14, response + 16);

    return memcmp(response, expected, sizeof(response)) == 0;
}

int test_decrypt()
{
    unsigned char Challenge[8] = {
        0xD0, 0x2E, 0x43, 0x86, 0xBC, 0xE9, 0x12, 0x26
    };

    unsigned char ZPasswordHash[24] = {
        0x44, 0xEB, 0xBA, 0x8D, 0x53, 0x12, 0xB8, 0xD6,
        0x11, 0x47, 0x44, 0x11, 0xF5, 0x69, 0x89, 0xAE
    };

    unsigned char Response[24] = {
        0x82, 0x30, 0x9E, 0xCD, 0x8D, 0x70, 0x8B, 0x5E,
        0xA0, 0x8F, 0xAA, 0x39, 0x81, 0xCD, 0x83, 0x54,
        0x42, 0x33, 0x11, 0x4A, 0x3D, 0x85, 0xD6, 0xDF
    };
    unsigned char Output[8];
    unsigned int failure = 0;

    if (DesDecrypt(Response + 0, ZPasswordHash + 0, Output)) {
        failure += memcmp(Challenge, Output, sizeof(Challenge));
    }

    if (DesDecrypt(Response + 8, ZPasswordHash + 7, Output)) {
        failure += memcmp(Challenge, Output, sizeof(Challenge));
    }

    if (DesDecrypt(Response +16, ZPasswordHash +14, Output)) {
        failure += memcmp(Challenge, Output, sizeof(Challenge));
    }

    return failure == 0;
}

int main(int argc, char *argv[])
{
    int failure = 0;

    if (!PPP_crypto_init()) {
        printf("Couldn't initialize crypto test\n");
        return -1;
    }

    if (!test_encrypt()) {
        printf("CHAP DES encryption test failed\n");
        failure++;
    }

    if (!test_decrypt()) {
        printf("CHAP DES decryption test failed\n");
        failure++;
    }

    if (!PPP_crypto_deinit()) {
        printf("Couldn't deinitialize crypto test\n");
        return -1;
    }

    return failure;
}

#endif
