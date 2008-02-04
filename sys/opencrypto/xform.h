/*	$NetBSD: xform.h,v 1.6.2.3 2008/02/04 09:24:48 yamt Exp $ */
/*	$FreeBSD: src/sys/opencrypto/xform.h,v 1.1.2.1 2002/11/21 23:34:23 sam Exp $	*/
/*	$OpenBSD: xform.h,v 1.10 2002/04/22 23:10:09 deraadt Exp $	*/

/*
 * The author of this code is Angelos D. Keromytis (angelos@cis.upenn.edu)
 *
 * This code was written by Angelos D. Keromytis in Athens, Greece, in
 * February 2000. Network Security Technologies Inc. (NSTI) kindly
 * supported the development of this code.
 *
 * Copyright (c) 2000 Angelos D. Keromytis
 *
 * Permission to use, copy, and modify this software with or without fee
 * is hereby granted, provided that this entire notice is included in
 * all source code copies of any software which is or includes a copy or
 * modification of this software.
 *
 * THIS SOFTWARE IS BEING PROVIDED "AS IS", WITHOUT ANY EXPRESS OR
 * IMPLIED WARRANTY. IN PARTICULAR, NONE OF THE AUTHORS MAKES ANY
 * REPRESENTATION OR WARRANTY OF ANY KIND CONCERNING THE
 * MERCHANTABILITY OF THIS SOFTWARE OR ITS FITNESS FOR ANY PARTICULAR
 * PURPOSE.
 */

#ifndef _CRYPTO_XFORM_H_
#define _CRYPTO_XFORM_H_

#include <sys/md5.h>
#include <sys/sha1.h>
#include <sys/sha2.h>
#include <sys/rmd160.h>

/* Declarations */
struct auth_hash {
	int type;
	const char *name;
	u_int16_t keysize;
	u_int16_t hashsize;
	u_int16_t authsize;
	u_int16_t ctxsize;
};

/* Provide array-limit for clients (e.g., netipsec) */
#define	AH_ALEN_MAX	20	/* max authenticator hash length */

struct enc_xform {
	int type;
	const char *name;
	u_int16_t blocksize;
	u_int16_t minkey, maxkey;
};

struct comp_algo {
	int type;
	const char *name;
	size_t minlen;
};

extern const u_int8_t hmac_ipad_buffer[64];
extern const u_int8_t hmac_opad_buffer[64];

extern struct enc_xform enc_xform_null;
extern struct enc_xform enc_xform_des;
extern struct enc_xform enc_xform_3des;
extern struct enc_xform enc_xform_blf;
extern struct enc_xform enc_xform_cast5;
extern struct enc_xform enc_xform_skipjack;
extern struct enc_xform enc_xform_rijndael128;
extern struct enc_xform enc_xform_arc4;

extern struct auth_hash auth_hash_null;
extern struct auth_hash auth_hash_md5;
extern struct auth_hash auth_hash_sha1;
extern struct auth_hash auth_hash_key_md5;
extern struct auth_hash auth_hash_key_sha1;
extern struct auth_hash auth_hash_hmac_md5;
extern struct auth_hash auth_hash_hmac_sha1;
extern struct auth_hash auth_hash_hmac_ripemd_160;
extern struct auth_hash auth_hash_hmac_md5_96;
extern struct auth_hash auth_hash_hmac_sha1_96;
extern struct auth_hash auth_hash_hmac_ripemd_160_96;
extern struct auth_hash auth_hash_hmac_sha2_256;
extern struct auth_hash auth_hash_hmac_sha2_384;
extern struct auth_hash auth_hash_hmac_sha2_512;

extern struct comp_algo comp_algo_deflate;

#ifdef _KERNEL
#include <sys/malloc.h>
MALLOC_DECLARE(M_XDATA);
#endif
#endif /* _CRYPTO_XFORM_H_ */
