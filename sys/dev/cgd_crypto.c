/* $NetBSD: cgd_crypto.c,v 1.27 2020/07/25 22:14:35 riastradh Exp $ */

/*-
 * Copyright (c) 2002 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Roland C. Dowdeswell.
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
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
 *  Crypto Framework For cgd.c
 *
 *	This framework is temporary and awaits a more complete
 *	kernel wide crypto implementation.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: cgd_crypto.c,v 1.27 2020/07/25 22:14:35 riastradh Exp $");

#include <sys/param.h>
#include <sys/kmem.h>
#include <sys/systm.h>

#include <dev/cgd_crypto.h>

#include <crypto/adiantum/adiantum.h>
#include <crypto/aes/aes.h>
#include <crypto/aes/aes_cbc.h>
#include <crypto/aes/aes_xts.h>
#include <crypto/blowfish/blowfish.h>
#include <crypto/des/des.h>

/*
 * The general framework provides only one generic function.
 * It takes the name of an algorithm and returns a struct cryptfuncs *
 * for it.  It is up to the initialisation routines of the algorithm
 * to check key size and block size.
 */

static cfunc_init		cgd_cipher_aes_cbc_init;
static cfunc_destroy		cgd_cipher_aes_cbc_destroy;
static cfunc_cipher		cgd_cipher_aes_cbc;

static cfunc_init		cgd_cipher_aes_xts_init;
static cfunc_destroy		cgd_cipher_aes_xts_destroy;
static cfunc_cipher		cgd_cipher_aes_xts;

static cfunc_init		cgd_cipher_3des_init;
static cfunc_destroy		cgd_cipher_3des_destroy;
static cfunc_cipher		cgd_cipher_3des_cbc;

static cfunc_init		cgd_cipher_bf_init;
static cfunc_destroy		cgd_cipher_bf_destroy;
static cfunc_cipher		cgd_cipher_bf_cbc;

static cfunc_init		cgd_cipher_adiantum_init;
static cfunc_destroy		cgd_cipher_adiantum_destroy;
static cfunc_cipher		cgd_cipher_adiantum_crypt;

static const struct cryptfuncs cf[] = {
	{
		.cf_name	= "aes-xts",
		.cf_init	= cgd_cipher_aes_xts_init,
		.cf_destroy	= cgd_cipher_aes_xts_destroy,
		.cf_cipher	= cgd_cipher_aes_xts,
	},
	{
		.cf_name	= "aes-cbc",
		.cf_init	= cgd_cipher_aes_cbc_init,
		.cf_destroy	= cgd_cipher_aes_cbc_destroy,
		.cf_cipher	= cgd_cipher_aes_cbc,
	},
	{
		.cf_name	= "3des-cbc",
		.cf_init	= cgd_cipher_3des_init,
		.cf_destroy	= cgd_cipher_3des_destroy,
		.cf_cipher	= cgd_cipher_3des_cbc,
	},
	{
		.cf_name	= "blowfish-cbc",
		.cf_init	= cgd_cipher_bf_init,
		.cf_destroy	= cgd_cipher_bf_destroy,
		.cf_cipher	= cgd_cipher_bf_cbc,
	},
	{
		.cf_name	= "adiantum",
		.cf_init	= cgd_cipher_adiantum_init,
		.cf_destroy	= cgd_cipher_adiantum_destroy,
		.cf_cipher	= cgd_cipher_adiantum_crypt,
	},
};
const struct cryptfuncs *
cryptfuncs_find(const char *alg)
{

	for (size_t i = 0; i < __arraycount(cf); i++)
		if (strcmp(cf[i].cf_name, alg) == 0)
			return &cf[i];

	return NULL;
}

/*
 *  AES Framework
 */

struct aes_privdata {
	struct aesenc	ap_enckey;
	struct aesdec	ap_deckey;
	uint32_t	ap_nrounds;
};

static void *
cgd_cipher_aes_cbc_init(size_t keylen, const void *key, size_t *blocksize)
{
	struct	aes_privdata *ap;

	if (!blocksize)
		return NULL;
	if (keylen != 128 && keylen != 192 && keylen != 256)
		return NULL;
	if (*blocksize == (size_t)-1)
		*blocksize = 128;
	if (*blocksize != 128)
		return NULL;
	ap = kmem_zalloc(sizeof(*ap), KM_SLEEP);
	switch (keylen) {
	case 128:
		aes_setenckey128(&ap->ap_enckey, key);
		aes_setdeckey128(&ap->ap_deckey, key);
		ap->ap_nrounds = AES_128_NROUNDS;
		break;
	case 192:
		aes_setenckey192(&ap->ap_enckey, key);
		aes_setdeckey192(&ap->ap_deckey, key);
		ap->ap_nrounds = AES_192_NROUNDS;
		break;
	case 256:
		aes_setenckey256(&ap->ap_enckey, key);
		aes_setdeckey256(&ap->ap_deckey, key);
		ap->ap_nrounds = AES_256_NROUNDS;
		break;
	}
	return ap;
}

static void
cgd_cipher_aes_cbc_destroy(void *data)
{
	struct aes_privdata *apd = data;

	explicit_memset(apd, 0, sizeof(*apd));
	kmem_free(apd, sizeof(*apd));
}

static void
cgd_cipher_aes_cbc(void *privdata, void *dst, const void *src, size_t nbytes,
    const void *blkno, int dir)
{
	struct aes_privdata	*apd = privdata;
	uint8_t iv[CGD_AES_BLOCK_SIZE] __aligned(CGD_AES_BLOCK_SIZE) = {0};

	/* Compute the CBC IV as AES_k(blkno).  */
	aes_enc(&apd->ap_enckey, blkno, iv, apd->ap_nrounds);

	switch (dir) {
	case CGD_CIPHER_ENCRYPT:
		aes_cbc_enc(&apd->ap_enckey, src, dst, nbytes, iv,
		    apd->ap_nrounds);
		break;
	case CGD_CIPHER_DECRYPT:
		aes_cbc_dec(&apd->ap_deckey, src, dst, nbytes, iv,
		    apd->ap_nrounds);
		break;
	default:
		panic("%s: unrecognised direction %d", __func__, dir);
	}
}

/*
 * AES-XTS
 */

struct aesxts {
	struct aesenc	ax_enckey;
	struct aesdec	ax_deckey;
	struct aesenc	ax_tweakkey;
	uint32_t	ax_nrounds;
};

static void *
cgd_cipher_aes_xts_init(size_t keylen, const void *xtskey, size_t *blocksize)
{
	struct aesxts *ax;
	const char *key, *key2; /* XTS key is made of two AES keys. */

	if (!blocksize)
		return NULL;
	if (keylen != 256 && keylen != 512)
		return NULL;
	if (*blocksize == (size_t)-1)
		*blocksize = 128;
	if (*blocksize != 128)
		return NULL;

	ax = kmem_zalloc(sizeof(*ax), KM_SLEEP);
	keylen /= 2;
	key = xtskey;
	key2 = key + keylen / CHAR_BIT;

	switch (keylen) {
	case 128:
		aes_setenckey128(&ax->ax_enckey, key);
		aes_setdeckey128(&ax->ax_deckey, key);
		aes_setenckey128(&ax->ax_tweakkey, key2);
		ax->ax_nrounds = AES_128_NROUNDS;
		break;
	case 256:
		aes_setenckey256(&ax->ax_enckey, key);
		aes_setdeckey256(&ax->ax_deckey, key);
		aes_setenckey256(&ax->ax_tweakkey, key2);
		ax->ax_nrounds = AES_256_NROUNDS;
		break;
	}

	return ax;
}

static void
cgd_cipher_aes_xts_destroy(void *cookie)
{
	struct aesxts *ax = cookie;

	explicit_memset(ax, 0, sizeof(*ax));
	kmem_free(ax, sizeof(*ax));
}

static void
cgd_cipher_aes_xts(void *cookie, void *dst, const void *src, size_t nbytes,
    const void *blkno, int dir)
{
	struct aesxts *ax = cookie;
	uint8_t tweak[CGD_AES_BLOCK_SIZE];

	/* Compute the initial tweak as AES_k(blkno).  */
	aes_enc(&ax->ax_tweakkey, blkno, tweak, ax->ax_nrounds);

	switch (dir) {
	case CGD_CIPHER_ENCRYPT:
		aes_xts_enc(&ax->ax_enckey, src, dst, nbytes, tweak,
		    ax->ax_nrounds);
		break;
	case CGD_CIPHER_DECRYPT:
		aes_xts_dec(&ax->ax_deckey, src, dst, nbytes, tweak,
		    ax->ax_nrounds);
		break;
	default:
		panic("%s: unrecognised direction %d", __func__, dir);
	}
}

/*
 * 3DES Framework
 */

struct c3des_privdata {
	des_key_schedule	cp_key1;
	des_key_schedule	cp_key2;
	des_key_schedule	cp_key3;
};

static void *
cgd_cipher_3des_init(size_t keylen, const void *key, size_t *blocksize)
{
	struct	c3des_privdata *cp;
	int	error = 0;
	des_cblock *block;

	if (!blocksize)
		return NULL;
	if (*blocksize == (size_t)-1)
		*blocksize = 64;
	if (keylen != (DES_KEY_SZ * 3 * 8) || *blocksize != 64)
		return NULL;
	cp = kmem_zalloc(sizeof(*cp), KM_SLEEP);
	block = __UNCONST(key);
	error  = des_key_sched(block, cp->cp_key1);
	error |= des_key_sched(block + 1, cp->cp_key2);
	error |= des_key_sched(block + 2, cp->cp_key3);
	if (error) {
		explicit_memset(cp, 0, sizeof(*cp));
		kmem_free(cp, sizeof(*cp));
		return NULL;
	}
	return cp;
}

static void
cgd_cipher_3des_destroy(void *data)
{
	struct c3des_privdata *cp = data;

	explicit_memset(cp, 0, sizeof(*cp));
	kmem_free(cp, sizeof(*cp));
}

static void
cgd_cipher_3des_cbc(void *privdata, void *dst, const void *src, size_t nbytes,
    const void *blkno, int dir)
{
	struct	c3des_privdata *cp = privdata;
	des_cblock zero;
	uint8_t iv[CGD_3DES_BLOCK_SIZE];

	/* Compute the CBC IV as 3DES_k(blkno) = 3DES-CBC_k(iv=blkno, 0).  */
	memset(&zero, 0, sizeof(zero));
	des_ede3_cbc_encrypt(blkno, iv, CGD_3DES_BLOCK_SIZE,
	    cp->cp_key1, cp->cp_key2, cp->cp_key3, &zero, /*encrypt*/1);

	switch (dir) {
	case CGD_CIPHER_ENCRYPT:
		des_ede3_cbc_encrypt(src, dst, nbytes,
		    cp->cp_key1, cp->cp_key2, cp->cp_key3,
		    (des_cblock *)iv, /*encrypt*/1);
		break;
	case CGD_CIPHER_DECRYPT:
		des_ede3_cbc_encrypt(src, dst, nbytes,
		    cp->cp_key1, cp->cp_key2, cp->cp_key3,
		    (des_cblock *)iv, /*encrypt*/0);
		break;
	default:
		panic("%s: unrecognised direction %d", __func__, dir);
	}
}

/*
 * Blowfish Framework
 */

struct bf_privdata {
	BF_KEY	bp_key;
};

struct bf_encdata {
	BF_KEY		*be_key;
	uint8_t		 be_iv[CGD_BF_BLOCK_SIZE];
};

static void *
cgd_cipher_bf_init(size_t keylen, const void *key, size_t *blocksize)
{
	struct	bf_privdata *bp;

	if (!blocksize)
		return NULL;
	if (keylen < 40 || keylen > 448 || (keylen % 8 != 0))
		return NULL;
	if (*blocksize == (size_t)-1)
		*blocksize = 64;
	if (*blocksize != 64)
		return NULL;
	bp = kmem_zalloc(sizeof(*bp), KM_SLEEP);
	if (!bp)
		return NULL;
	BF_set_key(&bp->bp_key, keylen / 8, key);
	return bp;
}

static void
cgd_cipher_bf_destroy(void *data)
{
	struct	bf_privdata *bp = data;

	explicit_memset(bp, 0, sizeof(*bp));
	kmem_free(bp, sizeof(*bp));
}

static void
cgd_cipher_bf_cbc(void *privdata, void *dst, const void *src, size_t nbytes,
    const void *blkno, int dir)
{
	struct	bf_privdata *bp = privdata;
	uint8_t zero[CGD_BF_BLOCK_SIZE], iv[CGD_BF_BLOCK_SIZE];

	/* Compute the CBC IV as Blowfish_k(blkno) = BF_CBC_k(blkno, 0).  */
	memset(zero, 0, sizeof(zero));
	BF_cbc_encrypt(blkno, iv, CGD_BF_BLOCK_SIZE, &bp->bp_key, zero,
	    /*encrypt*/1);

	switch (dir) {
	case CGD_CIPHER_ENCRYPT:
		BF_cbc_encrypt(src, dst, nbytes, &bp->bp_key, iv,
		    /*encrypt*/1);
		break;
	case CGD_CIPHER_DECRYPT:
		BF_cbc_encrypt(src, dst, nbytes, &bp->bp_key, iv,
		    /*encrypt*/0);
		break;
	default:
		panic("%s: unrecognised direction %d", __func__, dir);
	}
}

/*
 * Adiantum
 */

static void *
cgd_cipher_adiantum_init(size_t keylen, const void *key, size_t *blocksize)
{
	struct adiantum *A;

	if (!blocksize)
		return NULL;
	if (keylen != 256)
		return NULL;
	if (*blocksize == (size_t)-1)
		*blocksize = 128;
	if (*blocksize != 128)
		return NULL;

	A = kmem_zalloc(sizeof(*A), KM_SLEEP);
	adiantum_init(A, key);

	return A;
}

static void
cgd_cipher_adiantum_destroy(void *cookie)
{
	struct adiantum *A = cookie;

	explicit_memset(A, 0, sizeof(*A));
	kmem_free(A, sizeof(*A));
}

static void
cgd_cipher_adiantum_crypt(void *cookie, void *dst, const void *src,
    size_t nbytes, const void *blkno, int dir)
{
	/*
	 * Treat the block number as a 128-bit block.  This is more
	 * than twice as big as the largest number of reasonable
	 * blocks, but it doesn't hurt (it would be rounded up to a
	 * 128-bit input anyway).
	 */
	const unsigned tweaklen = 16;
	struct adiantum *A = cookie;

	switch (dir) {
	case CGD_CIPHER_ENCRYPT:
		adiantum_enc(dst, src, nbytes, blkno, tweaklen, A);
		break;
	case CGD_CIPHER_DECRYPT:
		adiantum_dec(dst, src, nbytes, blkno, tweaklen, A);
		break;
	default:
		panic("%s: unrecognised direction %d", __func__, dir);
	}
}
