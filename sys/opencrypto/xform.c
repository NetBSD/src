/*	$NetBSD: xform.c,v 1.28.14.1 2017/12/03 11:39:06 jdolecek Exp $ */
/*	$FreeBSD: src/sys/opencrypto/xform.c,v 1.1.2.1 2002/11/21 23:34:23 sam Exp $	*/
/*	$OpenBSD: xform.c,v 1.19 2002/08/16 22:47:25 dhartmei Exp $	*/

/*
 * The authors of this code are John Ioannidis (ji@tla.org),
 * Angelos D. Keromytis (kermit@csd.uch.gr) and
 * Niels Provos (provos@physnet.uni-hamburg.de).
 *
 * This code was written by John Ioannidis for BSD/OS in Athens, Greece,
 * in November 1995.
 *
 * Ported to OpenBSD and NetBSD, with additional transforms, in December 1996,
 * by Angelos D. Keromytis.
 *
 * Additional transforms and features in 1997 and 1998 by Angelos D. Keromytis
 * and Niels Provos.
 *
 * Additional features in 1999 by Angelos D. Keromytis.
 *
 * Copyright (C) 1995, 1996, 1997, 1998, 1999 by John Ioannidis,
 * Angelos D. Keromytis and Niels Provos.
 *
 * Copyright (C) 2001, Angelos D. Keromytis.
 *
 * Permission to use, copy, and modify this software with or without fee
 * is hereby granted, provided that this entire notice is included in
 * all copies of any software which is or includes a copy or
 * modification of this software.
 * You may use this code under the GNU public license if you so wish. Please
 * contribute changes back to the authors under this freer than GPL license
 * so that we may further the use of strong encryption without limitations to
 * all.
 *
 * THIS SOFTWARE IS BEING PROVIDED "AS IS", WITHOUT ANY EXPRESS OR
 * IMPLIED WARRANTY. IN PARTICULAR, NONE OF THE AUTHORS MAKES ANY
 * REPRESENTATION OR WARRANTY OF ANY KIND CONCERNING THE
 * MERCHANTABILITY OF THIS SOFTWARE OR ITS FITNESS FOR ANY PARTICULAR
 * PURPOSE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: xform.c,v 1.28.14.1 2017/12/03 11:39:06 jdolecek Exp $");

#include <sys/param.h>
#include <sys/malloc.h>

#include <opencrypto/cryptodev.h>
#include <opencrypto/xform.h>

MALLOC_DEFINE(M_XDATA, "xform", "xform data buffers");

const u_int8_t hmac_ipad_buffer[128] = {
	0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36,
	0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36,
	0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36,
	0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36,
	0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36,
	0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36,
	0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36,
	0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36,
	0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36,
	0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36,
	0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36,
	0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36,
	0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36,
	0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36,
	0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36,
	0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36
};

const u_int8_t hmac_opad_buffer[128] = {
	0x5C, 0x5C, 0x5C, 0x5C, 0x5C, 0x5C, 0x5C, 0x5C,
	0x5C, 0x5C, 0x5C, 0x5C, 0x5C, 0x5C, 0x5C, 0x5C,
	0x5C, 0x5C, 0x5C, 0x5C, 0x5C, 0x5C, 0x5C, 0x5C,
	0x5C, 0x5C, 0x5C, 0x5C, 0x5C, 0x5C, 0x5C, 0x5C,
	0x5C, 0x5C, 0x5C, 0x5C, 0x5C, 0x5C, 0x5C, 0x5C,
	0x5C, 0x5C, 0x5C, 0x5C, 0x5C, 0x5C, 0x5C, 0x5C,
	0x5C, 0x5C, 0x5C, 0x5C, 0x5C, 0x5C, 0x5C, 0x5C,
	0x5C, 0x5C, 0x5C, 0x5C, 0x5C, 0x5C, 0x5C, 0x5C,
	0x5C, 0x5C, 0x5C, 0x5C, 0x5C, 0x5C, 0x5C, 0x5C,
	0x5C, 0x5C, 0x5C, 0x5C, 0x5C, 0x5C, 0x5C, 0x5C,
	0x5C, 0x5C, 0x5C, 0x5C, 0x5C, 0x5C, 0x5C, 0x5C,
	0x5C, 0x5C, 0x5C, 0x5C, 0x5C, 0x5C, 0x5C, 0x5C,
	0x5C, 0x5C, 0x5C, 0x5C, 0x5C, 0x5C, 0x5C, 0x5C,
	0x5C, 0x5C, 0x5C, 0x5C, 0x5C, 0x5C, 0x5C, 0x5C,
	0x5C, 0x5C, 0x5C, 0x5C, 0x5C, 0x5C, 0x5C, 0x5C,
	0x5C, 0x5C, 0x5C, 0x5C, 0x5C, 0x5C, 0x5C, 0x5C
};

/* Encryption instances */
const struct enc_xform enc_xform_null = {
	.type		= CRYPTO_NULL_CBC,
	.name		= "NULL",
	/* NB: blocksize of 4 is to generate a properly aligned ESP header */
	.blocksize	= 4,
	.ivsize		= 0,
	.minkey		= 0,
	.maxkey		= 256, /* 2048 bits, max key */
};

const struct enc_xform enc_xform_des = {
	.type		= CRYPTO_DES_CBC,
	.name		= "DES",
	.blocksize	= 8,
	.ivsize		= 8,
	.minkey		= 8,
	.maxkey		= 8,
};

const struct enc_xform enc_xform_3des = {
	.type		= CRYPTO_3DES_CBC,
	.name		= "3DES",
	.blocksize	= 8,
	.ivsize		= 8,
	.minkey		= 24,
	.maxkey		= 24,
};

const struct enc_xform enc_xform_blf = {
	.type		= CRYPTO_BLF_CBC,
	.name		= "Blowfish",
	.blocksize	= 8,
	.ivsize		= 8,
	.minkey		= 5,
	.maxkey		= 56, /* 448 bits, max key */
};

const struct enc_xform enc_xform_cast5 = {
	.type		= CRYPTO_CAST_CBC,
	.name		= "CAST-128",
	.blocksize	= 8,
	.ivsize		= 8,
	.minkey		= 5,
	.maxkey		= 16,
};

const struct enc_xform enc_xform_skipjack = {
	.type		= CRYPTO_SKIPJACK_CBC,
	.name		= "Skipjack",
	.blocksize	= 8,
	.ivsize		= 8,
	.minkey		= 10,
	.maxkey		= 10,
};

const struct enc_xform enc_xform_rijndael128 = {
	.type		= CRYPTO_RIJNDAEL128_CBC,
	.name		= "Rijndael-128/AES",
	.blocksize	= 16,
	.ivsize		= 16,
	.minkey		= 16,
	.maxkey		= 32,
};

const struct enc_xform enc_xform_arc4 = {
	.type		= CRYPTO_ARC4,
	.name		= "ARC4",
	.blocksize	= 1,
	.ivsize		= 0,
	.minkey		= 1,
	.maxkey		= 32,
};

const struct enc_xform enc_xform_camellia = {
	.type		= CRYPTO_CAMELLIA_CBC,
	.name		= "Camellia",
	.blocksize	= 16,
	.ivsize		= 16,
	.minkey		= 8,
	.maxkey		= 32,
};

const struct enc_xform enc_xform_aes_ctr = {
	.type		= CRYPTO_AES_CTR,
	.name		= "AES-CTR",
	.blocksize	= 16,
	.ivsize		= 8,
	.minkey		= 16 + 4,
	.maxkey		= 32 + 4,
};

const struct enc_xform enc_xform_aes_gcm = {
	.type		= CRYPTO_AES_GCM_16,
	.name		= "AES-GCM",
	.blocksize	= 4, /* ??? */
	.ivsize		= 8,
	.minkey		= 16 + 4,
	.maxkey		= 32 + 4,
};

const struct enc_xform enc_xform_aes_gmac = {
	.type		= CRYPTO_AES_GMAC,
	.name		= "AES-GMAC",
	.blocksize	= 4, /* ??? */
	.ivsize		= 8,
	.minkey		= 16 + 4,
	.maxkey		= 32 + 4,
};

/* Authentication instances */
const struct auth_hash auth_hash_null = {
	.type		= CRYPTO_NULL_HMAC,
	.name		= "NULL-HMAC",
	.keysize	= 0,
	.hashsize	= 0,
	.authsize	= 12,
	.blocksize	= 64,
};

const struct auth_hash auth_hash_hmac_md5 = {
	.type		= CRYPTO_MD5_HMAC,
	.name		= "HMAC-MD5",
	.keysize	= 16,
	.hashsize	= 16,
	.authsize	= 16,
	.blocksize	= 64,
};

const struct auth_hash auth_hash_hmac_sha1 = {
	.type		= CRYPTO_SHA1_HMAC,
	.name		= "HMAC-SHA1",
	.keysize	= 20,
	.hashsize	= 20,
	.authsize	= 20,
	.blocksize	= 64,
};

const struct auth_hash auth_hash_hmac_ripemd_160 = {
	.type		= CRYPTO_RIPEMD160_HMAC,
	.name		= "HMAC-RIPEMD-160",
	.keysize	= 20,
	.hashsize	= 20,
	.authsize	= 20,
	.blocksize	= 64,
};

const struct auth_hash auth_hash_hmac_md5_96 = {
	.type		= CRYPTO_MD5_HMAC_96,
	.name		= "HMAC-MD5-96",
	.keysize	= 16,
	.hashsize	= 16,
	.authsize	= 12,
	.blocksize	= 64,
};

const struct auth_hash auth_hash_hmac_sha1_96 = {
	.type		= CRYPTO_SHA1_HMAC_96,
	.name		= "HMAC-SHA1-96",
	.keysize	= 20,
	.hashsize	= 20,
	.authsize	= 12,
	.blocksize	= 64,
};

const struct auth_hash auth_hash_hmac_ripemd_160_96 = {
	.type		= CRYPTO_RIPEMD160_HMAC_96,
	.name		= "HMAC-RIPEMD-160",
	.keysize	= 20,
	.hashsize	= 20,
	.authsize	= 12,
	.blocksize	= 64,
};

const struct auth_hash auth_hash_key_md5 = {
	.type		= CRYPTO_MD5_KPDK,
	.name		= "Keyed MD5",
	.keysize	= 0,
	.hashsize	= 16,
	.authsize	= 16,
	.blocksize	= 0,
};

const struct auth_hash auth_hash_key_sha1 = {
	.type		= CRYPTO_SHA1_KPDK,
	.name		= "Keyed SHA1",
	.keysize	= 0,
	.hashsize	= 20,
	.authsize	= 20,
	.blocksize	= 0,
};

const struct auth_hash auth_hash_md5 = {
	.type		= CRYPTO_MD5,
	.name		= "MD5",
	.keysize	= 0,
	.hashsize	= 16,
	.authsize	= 16,
	.blocksize	= 0,
};

const struct auth_hash auth_hash_sha1 = {
	.type		= CRYPTO_SHA1,
	.name		= "SHA1",
	.keysize	= 0,
	.hashsize	= 20,
	.authsize	= 20,
	.blocksize	= 0,
};

const struct auth_hash auth_hash_hmac_sha2_256 = {
	.type		= CRYPTO_SHA2_256_HMAC,
	.name		= "HMAC-SHA2",
	.keysize	= 32,
	.hashsize	= 32,
	.authsize	= 16,
	.blocksize	= 64,
};

const struct auth_hash auth_hash_hmac_sha2_384 = {
	.type		= CRYPTO_SHA2_384_HMAC,
	.name		= "HMAC-SHA2-384",
	.keysize	= 48,
	.hashsize	= 48,
	.authsize	= 24,
	.blocksize	= 128,
};

const struct auth_hash auth_hash_hmac_sha2_512 = {
	.type		= CRYPTO_SHA2_512_HMAC,
	.name		= "HMAC-SHA2-512",
	.keysize	= 64,
	.hashsize	= 64,
	.authsize	= 32,
	.blocksize	= 128,
};

const struct auth_hash auth_hash_aes_xcbc_mac_96 = {
	.type		= CRYPTO_AES_XCBC_MAC_96,
	.name		= "AES-XCBC-MAC-96",
	.keysize	= 16,
	.hashsize	= 16,
	.authsize	= 12,
	.blocksize	= 0,
};

const struct auth_hash auth_hash_gmac_aes_128 = {
	.type		= CRYPTO_AES_128_GMAC,
	.name		= "GMAC-AES-128",
	.keysize	= 16 + 4,
	.hashsize	= 16,
	.authsize	= 16,
	.blocksize	= 16, /* ??? */
};

const struct auth_hash auth_hash_gmac_aes_192 = {
	.type		= CRYPTO_AES_192_GMAC,
	.name		= "GMAC-AES-192",
	.keysize	= 24 + 4,
	.hashsize	= 16,
	.authsize	= 16,
	.blocksize	= 16, /* ??? */
};

const struct auth_hash auth_hash_gmac_aes_256 = {
	.type		= CRYPTO_AES_256_GMAC,
	.name		= "GMAC-AES-256",
	.keysize	= 32 + 4,
	.hashsize	= 16,
	.authsize	= 16,
	.blocksize	= 16, /* ??? */
};

/* Compression instance */
const struct comp_algo comp_algo_deflate = {
	.type	= CRYPTO_DEFLATE_COMP,
	.name	= "Deflate",
	.minlen	= 90,
};

const struct comp_algo comp_algo_deflate_nogrow = {
	.type	= CRYPTO_DEFLATE_COMP_NOGROW,
	.name	= "Deflate",
	.minlen	= 90,
};

const struct comp_algo comp_algo_gzip = {
	.type	= CRYPTO_GZIP_COMP,
	.name	= "GZIP",
	.minlen	= 90,
};
