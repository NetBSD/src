/*	$KAME: crypto_openssl.h,v 1.23 2001/08/14 12:26:06 sakane Exp $	*/

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

#ifdef HAVE_SIGNING_C
/* X509 Certificate */
#define GENT_OTHERNAME	0
#define GENT_EMAIL	1
#define GENT_DNS	2
#define GENT_X400	3
#define GENT_DIRNAME	4
#define GENT_EDIPARTY	5
#define GENT_URI	6
#define GENT_IPADD	7
#define GENT_RID	8

extern vchar_t *eay_str2asn1dn __P((char *, int));
extern int eay_cmp_asn1dn __P((vchar_t *, vchar_t *));
extern int eay_check_x509cert __P((vchar_t *, char *));
extern vchar_t *eay_get_x509asn1subjectname __P((vchar_t *));
extern int eay_get_x509subjectaltname __P((vchar_t *, char **, int *, int));
extern char *eay_get_x509text __P((vchar_t *));
extern vchar_t *eay_get_x509cert __P((char *));
extern vchar_t *eay_get_x509sign __P((vchar_t *, vchar_t *, vchar_t *));
extern int eay_check_x509sign __P((vchar_t *, vchar_t *, vchar_t *));
extern int eay_check_pkcs7sign __P((vchar_t *, vchar_t *, vchar_t *));

/* ASN.1 */
extern vchar_t *eay_get_pkcs1privkey __P((char *));
extern vchar_t *eay_get_pkcs1pubkey __P((char *));
#endif

/* string error */
extern char *eay_strerror __P((void));
extern void eay_init_error __P((void));

/* DES */
extern vchar_t *eay_des_encrypt __P((vchar_t *, vchar_t *, vchar_t *));
extern vchar_t *eay_des_decrypt __P((vchar_t *, vchar_t *, vchar_t *));
extern int eay_des_weakkey __P((vchar_t *));
extern int eay_des_keylen __P((int));

/* IDEA */
extern vchar_t *eay_idea_encrypt __P((vchar_t *, vchar_t *, vchar_t *));
extern vchar_t *eay_idea_decrypt __P((vchar_t *, vchar_t *, vchar_t *));
extern int eay_idea_weakkey __P((vchar_t *));
extern int eay_idea_keylen __P((int));

/* blowfish */
extern vchar_t *eay_bf_encrypt __P((vchar_t *, vchar_t *, vchar_t *));
extern vchar_t *eay_bf_decrypt __P((vchar_t *, vchar_t *, vchar_t *));
extern int eay_bf_weakkey __P((vchar_t *));
extern int eay_bf_keylen __P((int));

/* RC5 */
extern vchar_t *eay_rc5_encrypt __P((vchar_t *, vchar_t *, vchar_t *));
extern vchar_t *eay_rc5_decrypt __P((vchar_t *, vchar_t *, vchar_t *));
extern int eay_rc5_weakkey __P((vchar_t *));
extern int eay_rc5_keylen __P((int));

/* 3DES */
extern vchar_t *eay_3des_encrypt __P((vchar_t *, vchar_t *, vchar_t *));
extern vchar_t *eay_3des_decrypt __P((vchar_t *, vchar_t *, vchar_t *));
extern int eay_3des_weakkey __P((vchar_t *));
extern int eay_3des_keylen __P((int));

/* CAST */
extern vchar_t *eay_cast_encrypt __P((vchar_t *, vchar_t *, vchar_t *));
extern vchar_t *eay_cast_decrypt __P((vchar_t *, vchar_t *, vchar_t *));
extern int eay_cast_weakkey __P((vchar_t *));
extern int eay_cast_keylen __P((int));

/* AES(RIJNDAEL) */
extern vchar_t *eay_aes_encrypt __P((vchar_t *, vchar_t *, vchar_t *));
extern vchar_t *eay_aes_decrypt __P((vchar_t *, vchar_t *, vchar_t *));
extern int eay_aes_weakkey __P((vchar_t *));
extern int eay_aes_keylen __P((int));

/* misc */
extern int eay_null_hashlen __P((void));
extern int eay_kpdk_hashlen __P((void));
extern int eay_twofish_keylen __P((int));

/* hash */
/* HMAC SHA2 */
extern vchar_t *eay_hmacsha2_512_one __P((vchar_t *, vchar_t *));
extern caddr_t eay_hmacsha2_512_init __P((vchar_t *));
extern void eay_hmacsha2_512_update __P((caddr_t, vchar_t *));
extern vchar_t *eay_hmacsha2_512_final __P((caddr_t));
extern vchar_t *eay_hmacsha2_384_one __P((vchar_t *, vchar_t *));
extern caddr_t eay_hmacsha2_384_init __P((vchar_t *));
extern void eay_hmacsha2_384_update __P((caddr_t, vchar_t *));
extern vchar_t *eay_hmacsha2_384_final __P((caddr_t));
extern vchar_t *eay_hmacsha2_256_one __P((vchar_t *, vchar_t *));
extern caddr_t eay_hmacsha2_256_init __P((vchar_t *));
extern void eay_hmacsha2_256_update __P((caddr_t, vchar_t *));
extern vchar_t *eay_hmacsha2_256_final __P((caddr_t));
/* HMAC SHA1 */
extern vchar_t *eay_hmacsha1_one __P((vchar_t *, vchar_t *));
extern caddr_t eay_hmacsha1_init __P((vchar_t *));
extern void eay_hmacsha1_update __P((caddr_t, vchar_t *));
extern vchar_t *eay_hmacsha1_final __P((caddr_t));
/* HMAC MD5 */
extern vchar_t *eay_hmacmd5_one __P((vchar_t *, vchar_t *));
extern caddr_t eay_hmacmd5_init __P((vchar_t *));
extern void eay_hmacmd5_update __P((caddr_t, vchar_t *));
extern vchar_t *eay_hmacmd5_final __P((caddr_t));

/* SHA2 functions */
extern caddr_t eay_sha2_512_init __P((void));
extern void eay_sha2_512_update __P((caddr_t, vchar_t *));
extern vchar_t *eay_sha2_512_final __P((caddr_t));
extern vchar_t *eay_sha2_512_one __P((vchar_t *));
extern int eay_sha2_512_hashlen __P((void));

extern caddr_t eay_sha2_384_init __P((void));
extern void eay_sha2_384_update __P((caddr_t, vchar_t *));
extern vchar_t *eay_sha2_384_final __P((caddr_t));
extern vchar_t *eay_sha2_384_one __P((vchar_t *));
extern int eay_sha2_384_hashlen __P((void));

extern caddr_t eay_sha2_256_init __P((void));
extern void eay_sha2_256_update __P((caddr_t, vchar_t *));
extern vchar_t *eay_sha2_256_final __P((caddr_t));
extern vchar_t *eay_sha2_256_one __P((vchar_t *));
extern int eay_sha2_256_hashlen __P((void));

/* SHA functions */
extern caddr_t eay_sha1_init __P((void));
extern void eay_sha1_update __P((caddr_t, vchar_t *));
extern vchar_t *eay_sha1_final __P((caddr_t));
extern vchar_t *eay_sha1_one __P((vchar_t *));
extern int eay_sha1_hashlen __P((void));

/* MD5 functions */
extern caddr_t eay_md5_init __P((void));
extern void eay_md5_update __P((caddr_t, vchar_t *));
extern vchar_t *eay_md5_final __P((caddr_t));
extern vchar_t *eay_md5_one __P((vchar_t *));
extern int eay_md5_hashlen __P((void));

/* eay_set_random */
extern vchar_t *eay_set_random __P((u_int32_t));

/* DH */
extern int eay_dh_generate __P((vchar_t *, u_int32_t, u_int, vchar_t **, vchar_t **));
extern int eay_dh_compute __P((vchar_t *, u_int32_t, vchar_t *, vchar_t *, vchar_t *, vchar_t **));

/* misc */
extern int eay_revbnl __P((vchar_t *));
#include <openssl/bn.h>
extern int eay_v2bn __P((BIGNUM **, vchar_t *));
extern int eay_bn2v __P((vchar_t **, BIGNUM *));

extern const char *eay_version __P((void));

#define CBC_BLOCKLEN 8
#define IPSEC_ENCRYPTKEYLEN 8
