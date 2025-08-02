/* ppp-crypo-priv.h - Crypto private data structures
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
 */
#ifndef PPP_CRYPTO_PRIV_H
#define PPP_CRYPTO_PRIV_H

#include "crypto.h"

#define MAX_KEY_SIZE 32
#define MAX_IV_SIZE 32

struct _PPP_MD
{
    int  (*init_fn)(PPP_MD_CTX *ctx);
    int  (*update_fn)(PPP_MD_CTX *ctx, const void *data, size_t cnt);
    int  (*final_fn)(PPP_MD_CTX *ctx, unsigned char *out, unsigned int *outlen);
    void (*clean_fn)(PPP_MD_CTX *ctx);
};

struct _PPP_MD_CTX
{
    PPP_MD md;
    void *priv;
};

struct _PPP_CIPHER
{
    int  (*init_fn)(PPP_CIPHER_CTX *ctx, const unsigned char *key, const unsigned char *iv);
    int  (*update_fn)(PPP_CIPHER_CTX *ctx, unsigned char *out, int *outl, const unsigned char *in, int inl);
    int  (*final_fn)(PPP_CIPHER_CTX *ctx, unsigned char *out, int *outl);
    void (*clean_fn)(PPP_CIPHER_CTX *ctx);
};

struct _PPP_CIPHER_CTX
{
    PPP_CIPHER cipher;
    unsigned char key[MAX_KEY_SIZE];
    unsigned char iv[MAX_IV_SIZE];
    int is_encr;
    void *priv;
};


#endif
