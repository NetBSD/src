/* ppp-crypto.h - Generic API for access to crypto/digest functions.
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

#ifndef PPP_CRYPTO_H
#define PPP_CRYPTO_H

#ifdef __cplusplus
extern "C" {
#endif

#ifndef MD5_DIGEST_LENGTH
#define MD5_DIGEST_LENGTH 16
#endif

#ifndef MD4_DIGEST_LENGTH
#define MD4_DIGEST_LENGTH 16
#endif

#ifndef SHA_DIGEST_LENGTH
#define SHA_DIGEST_LENGTH 20
#endif

struct _PPP_MD_CTX;
struct _PPP_MD;

typedef struct _PPP_MD_CTX PPP_MD_CTX;
typedef struct _PPP_MD PPP_MD;

/*
 * Create a new Message Digest context object
 */
PPP_MD_CTX *PPP_MD_CTX_new(void);

/*
 * Free the Message Digest context
 */
void PPP_MD_CTX_free(PPP_MD_CTX*);

/*
 * Fetch the MD4 algorithm
 */
const PPP_MD *PPP_md4(void);

/*
 * Fetch the MD5 algorithm
 */
const PPP_MD *PPP_md5(void);

/*
 * Fetch the SHA1 algorithm
 */
const PPP_MD *PPP_sha1(void);

/*
 * Initializes a context object
 */
int PPP_DigestInit(PPP_MD_CTX *ctx,
        const PPP_MD *type);

/*
 * For each iteration update the context with more input
 */
int PPP_DigestUpdate(PPP_MD_CTX *ctx,
        const void *data, size_t cnt);

/*
 * Perform the final operation, and output the digest
 */
int PPP_DigestFinal(PPP_MD_CTX *ctx,
        unsigned char *out, unsigned int *outlen);


struct _PPP_CIPHER_CTX;
struct _PPP_CIPHER;

typedef struct _PPP_CIPHER_CTX PPP_CIPHER_CTX;
typedef struct _PPP_CIPHER PPP_CIPHER;


/*
 * Create a new Cipher Context
 */
PPP_CIPHER_CTX *PPP_CIPHER_CTX_new(void);

/*
 * Release the Cipher Context
 */
void PPP_CIPHER_CTX_free(PPP_CIPHER_CTX *ctx);

/*
 * Fetch the DES in ECB mode cipher algorithm
 */
const PPP_CIPHER *PPP_des_ecb(void);

/*
 * Set the particular data directly
 */
void PPP_CIPHER_CTX_set_cipher_data(PPP_CIPHER_CTX *ctx,
        const unsigned char *key);

/*
 * Initialize the crypto operation
 */
int PPP_CipherInit(PPP_CIPHER_CTX *ctx,
        const PPP_CIPHER *cipher,
        const unsigned char *key,
        const unsigned char *iv,
        int encr);

/*
 * Encrypt input data, and store it in the output buffer
 */
int PPP_CipherUpdate(PPP_CIPHER_CTX *ctx,
        unsigned char *out, int *outl,
        const unsigned char *in, int inl);

/*
 * Finish the crypto operation, and fetch any outstanding bytes
 */
int PPP_CipherFinal(PPP_CIPHER_CTX *ctx,
        unsigned char *out, int *outl);

/*
 * Log an error message to the log and append the crypto error
 */
void PPP_crypto_error(char *fmt, ...);

/*
 * Global initialization, must be called once per process
 */
int PPP_crypto_init(void);

/*
 * Global deinitialization
 */
int PPP_crypto_deinit(void);

#ifdef __cplusplus
}
#endif

#endif // PPP_CRYPTO_H
