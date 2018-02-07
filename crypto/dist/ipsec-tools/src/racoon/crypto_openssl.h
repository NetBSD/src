/*	$NetBSD: crypto_openssl.h,v 1.9 2018/02/07 03:59:03 christos Exp $	*/

/* Id: crypto_openssl.h,v 1.11 2004/11/13 11:28:01 manubsd Exp */

/*
 * Copyright (C) 1995, 1996, 1997, and 1998 WIDE Project.
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
 * 3. Neither the name of the project nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE PROJECT AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE PROJECT OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef _CRYPTO_OPENSSL_H
#define _CRYPTO_OPENSSL_H

#include <openssl/x509v3.h>
#include <openssl/rsa.h>

#define GENT_OTHERNAME	GEN_OTHERNAME
#define GENT_EMAIL	GEN_EMAIL
#define GENT_DNS	GEN_DNS
#define GENT_X400	GEN_X400
#define GENT_DIRNAME	GEN_DIRNAME
#define GENT_EDIPARTY	GEN_EDIPARTY
#define GENT_URI	GEN_URI
#define GENT_IPADD	GEN_IPADD
#define GENT_RID	GEN_RID

extern vchar_t *eay_str2asn1dn(const char *, int);
extern vchar_t *eay_hex2asn1dn(const char *, int);
extern int eay_cmp_asn1dn(vchar_t *, vchar_t *);
extern int eay_check_x509cert(vchar_t *, char *, char *, int);
extern vchar_t *eay_get_x509asn1subjectname(vchar_t *);
extern int eay_get_x509subjectaltname(vchar_t *, char **, int *, int);
extern vchar_t * eay_get_x509asn1issuername(vchar_t *);
extern char *eay_get_x509text(vchar_t *);
extern vchar_t *eay_get_x509cert(char *);
extern vchar_t *eay_get_x509sign(vchar_t *, vchar_t *);
extern int eay_check_x509sign(vchar_t *, vchar_t *, vchar_t *);

extern int eay_check_rsasign(vchar_t *, vchar_t *, RSA *);
extern vchar_t *eay_get_rsasign(vchar_t *, RSA *);

/* RSA */
extern vchar_t *eay_rsa_sign(vchar_t *, RSA *);
extern int eay_rsa_verify(vchar_t *, vchar_t *, RSA *);

/* ASN.1 */
extern vchar_t *eay_get_pkcs1privkey(char *);
extern vchar_t *eay_get_pkcs1pubkey(char *);

/* string error */
extern char *eay_strerror(void);

/* OpenSSL initialization */
extern void eay_init(void);

/* Generic EVP */
extern vchar_t *evp_crypt(vchar_t *data, vchar_t *key, vchar_t *iv,
    const EVP_CIPHER *e, int enc);
extern int evp_weakkey(vchar_t *key, const EVP_CIPHER *e);
extern int evp_keylen(int len, const EVP_CIPHER *e);

/* DES */
extern vchar_t *eay_des_encrypt(vchar_t *, vchar_t *, vchar_t *);
extern vchar_t *eay_des_decrypt(vchar_t *, vchar_t *, vchar_t *);
extern int eay_des_weakkey(vchar_t *);
extern int eay_des_keylen(int);

/* IDEA */
extern vchar_t *eay_idea_encrypt(vchar_t *, vchar_t *, vchar_t *);
extern vchar_t *eay_idea_decrypt(vchar_t *, vchar_t *, vchar_t *);
extern int eay_idea_weakkey(vchar_t *);
extern int eay_idea_keylen(int);

/* blowfish */
extern vchar_t *eay_bf_encrypt(vchar_t *, vchar_t *, vchar_t *);
extern vchar_t *eay_bf_decrypt(vchar_t *, vchar_t *, vchar_t *);
extern int eay_bf_weakkey(vchar_t *);
extern int eay_bf_keylen(int);

/* RC5 */
extern vchar_t *eay_rc5_encrypt(vchar_t *, vchar_t *, vchar_t *);
extern vchar_t *eay_rc5_decrypt(vchar_t *, vchar_t *, vchar_t *);
extern int eay_rc5_weakkey(vchar_t *);
extern int eay_rc5_keylen(int);

/* 3DES */
extern vchar_t *eay_3des_encrypt(vchar_t *, vchar_t *, vchar_t *);
extern vchar_t *eay_3des_decrypt(vchar_t *, vchar_t *, vchar_t *);
extern int eay_3des_weakkey(vchar_t *);
extern int eay_3des_keylen(int);

/* CAST */
extern vchar_t *eay_cast_encrypt(vchar_t *, vchar_t *, vchar_t *);
extern vchar_t *eay_cast_decrypt(vchar_t *, vchar_t *, vchar_t *);
extern int eay_cast_weakkey(vchar_t *);
extern int eay_cast_keylen(int);

/* AES(RIJNDAEL) */
extern vchar_t *eay_aes_encrypt(vchar_t *, vchar_t *, vchar_t *);
extern vchar_t *eay_aes_decrypt(vchar_t *, vchar_t *, vchar_t *);
extern int eay_aes_weakkey(vchar_t *);
extern int eay_aes_keylen(int);

/* AES GCM 16*/
extern int eay_aesgcm_keylen(int);

#if defined(HAVE_OPENSSL_CAMELLIA_H)
/* Camellia */
extern vchar_t *eay_camellia_encrypt(vchar_t *, vchar_t *, vchar_t *);
extern vchar_t *eay_camellia_decrypt(vchar_t *, vchar_t *, vchar_t *);
extern int eay_camellia_weakkey(vchar_t *);
extern int eay_camellia_keylen(int);
#endif

/* misc */
extern int eay_null_keylen(int);
extern int eay_null_hashlen(void);
extern int eay_kpdk_hashlen(void);
extern int eay_twofish_keylen(int);

/* hash */
#if defined(WITH_SHA2)
/* HMAC SHA2 */
extern vchar_t *eay_hmacsha2_512_one(vchar_t *, vchar_t *);
extern caddr_t eay_hmacsha2_512_init(vchar_t *);
extern void eay_hmacsha2_512_update(caddr_t, vchar_t *);
extern vchar_t *eay_hmacsha2_512_final(caddr_t);
extern vchar_t *eay_hmacsha2_384_one(vchar_t *, vchar_t *);
extern caddr_t eay_hmacsha2_384_init(vchar_t *);
extern void eay_hmacsha2_384_update(caddr_t, vchar_t *);
extern vchar_t *eay_hmacsha2_384_final(caddr_t);
extern vchar_t *eay_hmacsha2_256_one(vchar_t *, vchar_t *);
extern caddr_t eay_hmacsha2_256_init(vchar_t *);
extern void eay_hmacsha2_256_update(caddr_t, vchar_t *);
extern vchar_t *eay_hmacsha2_256_final(caddr_t);
#endif
/* HMAC SHA1 */
extern vchar_t *eay_hmacsha1_one(vchar_t *, vchar_t *);
extern caddr_t eay_hmacsha1_init(vchar_t *);
extern void eay_hmacsha1_update(caddr_t, vchar_t *);
extern vchar_t *eay_hmacsha1_final(caddr_t);
/* HMAC MD5 */
extern vchar_t *eay_hmacmd5_one(vchar_t *, vchar_t *);
extern caddr_t eay_hmacmd5_init(vchar_t *);
extern void eay_hmacmd5_update(caddr_t, vchar_t *);
extern vchar_t *eay_hmacmd5_final(caddr_t);

#if defined(WITH_SHA2)
/* SHA2 functions */
extern caddr_t eay_sha2_512_init(void);
extern void eay_sha2_512_update(caddr_t, vchar_t *);
extern vchar_t *eay_sha2_512_final(caddr_t);
extern vchar_t *eay_sha2_512_one(vchar_t *);
#endif
extern int eay_sha2_512_hashlen(void);

#if defined(WITH_SHA2)
extern caddr_t eay_sha2_384_init(void);
extern void eay_sha2_384_update(caddr_t, vchar_t *);
extern vchar_t *eay_sha2_384_final(caddr_t);
extern vchar_t *eay_sha2_384_one(vchar_t *);
#endif
extern int eay_sha2_384_hashlen(void);

#if defined(WITH_SHA2)
extern caddr_t eay_sha2_256_init(void);
extern void eay_sha2_256_update(caddr_t, vchar_t *);
extern vchar_t *eay_sha2_256_final(caddr_t);
extern vchar_t *eay_sha2_256_one(vchar_t *);
#endif
extern int eay_sha2_256_hashlen(void);

/* SHA functions */
extern caddr_t eay_sha1_init(void);
extern void eay_sha1_update(caddr_t, vchar_t *);
extern vchar_t *eay_sha1_final(caddr_t);
extern vchar_t *eay_sha1_one(vchar_t *);
extern int eay_sha1_hashlen(void);

/* MD5 functions */
extern caddr_t eay_md5_init(void);
extern void eay_md5_update(caddr_t, vchar_t *);
extern vchar_t *eay_md5_final(caddr_t);
extern vchar_t *eay_md5_one(vchar_t *);
extern int eay_md5_hashlen(void);

/* RNG */
extern vchar_t *eay_set_random(u_int32_t);
extern u_int32_t eay_random(void);

/* DH */
extern int eay_dh_generate(vchar_t *, u_int32_t, u_int, vchar_t **, vchar_t **);
extern int eay_dh_compute(vchar_t *, u_int32_t, vchar_t *, vchar_t *,
    vchar_t *, vchar_t **);

/* Base 64 */
vchar_t *base64_encode(char *, long);
vchar_t *base64_decode(char *, long);

RSA *base64_pubkey2rsa(char *);
RSA *bignum_pubkey2rsa(BIGNUM *);

/* misc */
extern int eay_revbnl(vchar_t *);
#include <openssl/bn.h>
extern int eay_v2bn(BIGNUM **, vchar_t *);
extern int eay_bn2v(vchar_t **, BIGNUM *);

extern const char *eay_version(void);

#define CBC_BLOCKLEN 8
#define IPSEC_ENCRYPTKEYLEN 8

#endif /* _CRYPTO_OPENSSL_H */
