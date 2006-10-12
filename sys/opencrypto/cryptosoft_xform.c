/*	$NetBSD: cryptosoft_xform.c,v 1.2 2006/10/12 01:32:47 christos Exp $ */
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
__KERNEL_RCSID(1, "$NetBSD: cryptosoft_xform.c,v 1.2 2006/10/12 01:32:47 christos Exp $");

#include <crypto/blowfish/blowfish.h>
#include <crypto/cast128/cast128.h>
#include <crypto/des/des.h>
#include <crypto/rijndael/rijndael.h>
#include <crypto/ripemd160/rmd160.h>
#include <crypto/skipjack/skipjack.h>

#include <opencrypto/deflate.h>

#include <sys/md5.h>
#include <sys/sha1.h>

struct swcr_auth_hash {
	struct auth_hash *auth_hash;
	void (*Init)(void *);
	int  (*Update)(void *, const uint8_t *, uint16_t);
	void (*Final)(uint8_t *, void *);
};

struct swcr_enc_xform {
	struct enc_xform *enc_xform;
	void (*encrypt)(caddr_t, uint8_t *);
	void (*decrypt)(caddr_t, uint8_t *);
	int  (*setkey)(uint8_t **, const uint8_t *, int len);
	void (*zerokey)(uint8_t **);
};

struct swcr_comp_algo {
	struct comp_algo *comp_algo;
	uint32_t (*compress)(uint8_t *, uint32_t, uint8_t **);
	uint32_t (*decompress)(uint8_t *, uint32_t, uint8_t **);
};

static void null_encrypt(caddr_t, u_int8_t *);
static void null_decrypt(caddr_t, u_int8_t *);
static int null_setkey(u_int8_t **, const u_int8_t *, int);
static void null_zerokey(u_int8_t **);

static	int des1_setkey(u_int8_t **, const u_int8_t *, int);
static	int des3_setkey(u_int8_t **, const u_int8_t *, int);
static	int blf_setkey(u_int8_t **, const u_int8_t *, int);
static	int cast5_setkey(u_int8_t **, const u_int8_t *, int);
static  int skipjack_setkey(u_int8_t **, const u_int8_t *, int);
static  int rijndael128_setkey(u_int8_t **, const u_int8_t *, int);
static	void des1_encrypt(caddr_t, u_int8_t *);
static	void des3_encrypt(caddr_t, u_int8_t *);
static	void blf_encrypt(caddr_t, u_int8_t *);
static	void cast5_encrypt(caddr_t, u_int8_t *);
static	void skipjack_encrypt(caddr_t, u_int8_t *);
static	void rijndael128_encrypt(caddr_t, u_int8_t *);
static	void des1_decrypt(caddr_t, u_int8_t *);
static	void des3_decrypt(caddr_t, u_int8_t *);
static	void blf_decrypt(caddr_t, u_int8_t *);
static	void cast5_decrypt(caddr_t, u_int8_t *);
static	void skipjack_decrypt(caddr_t, u_int8_t *);
static	void rijndael128_decrypt(caddr_t, u_int8_t *);
static	void des1_zerokey(u_int8_t **);
static	void des3_zerokey(u_int8_t **);
static	void blf_zerokey(u_int8_t **);
static	void cast5_zerokey(u_int8_t **);
static	void skipjack_zerokey(u_int8_t **);
static	void rijndael128_zerokey(u_int8_t **);

static	void null_init(void *);
static	int null_update(void *, const u_int8_t *, u_int16_t);
static	void null_final(u_int8_t *, void *);

static int	MD5Update_int(void *, const u_int8_t *, u_int16_t);
static void	SHA1Init_int(void *);
static	int SHA1Update_int(void *, const u_int8_t *, u_int16_t);
static	void SHA1Final_int(u_int8_t *, void *);


static int RMD160Update_int(void *, const u_int8_t *, u_int16_t);
static	int SHA1Update_int(void *, const u_int8_t *, u_int16_t);
static	void SHA1Final_int(u_int8_t *, void *);
static	int RMD160Update_int(void *, const u_int8_t *, u_int16_t);
static	int SHA256Update_int(void *, const u_int8_t *, u_int16_t);
static	int SHA384Update_int(void *, const u_int8_t *, u_int16_t);
static	int SHA512Update_int(void *, const u_int8_t *, u_int16_t);

static u_int32_t deflate_compress(u_int8_t *, u_int32_t, u_int8_t **);
static u_int32_t deflate_decompress(u_int8_t *, u_int32_t, u_int8_t **);

/* Encryption instances */
static const struct swcr_enc_xform swcr_enc_xform_null = {
	&enc_xform_null,
	null_encrypt,
	null_decrypt,
	null_setkey,
	null_zerokey,
};

static const struct swcr_enc_xform swcr_enc_xform_des = {
	&enc_xform_des,
	des1_encrypt,
	des1_decrypt,
	des1_setkey,
	des1_zerokey,
};

static const struct swcr_enc_xform swcr_enc_xform_3des = {
	&enc_xform_3des,
	des3_encrypt,
	des3_decrypt,
	des3_setkey,
	des3_zerokey
};

static const struct swcr_enc_xform swcr_enc_xform_blf = {
	&enc_xform_blf,
	blf_encrypt,
	blf_decrypt,
	blf_setkey,
	blf_zerokey
};

static const struct swcr_enc_xform swcr_enc_xform_cast5 = {
	&enc_xform_cast5,
	cast5_encrypt,
	cast5_decrypt,
	cast5_setkey,
	cast5_zerokey
};

static const struct swcr_enc_xform swcr_enc_xform_skipjack = {
	&enc_xform_skipjack,
	skipjack_encrypt,
	skipjack_decrypt,
	skipjack_setkey,
	skipjack_zerokey
};

static const struct swcr_enc_xform swcr_enc_xform_rijndael128 = {
	&enc_xform_rijndael128,
	rijndael128_encrypt,
	rijndael128_decrypt,
	rijndael128_setkey,
	rijndael128_zerokey,
};

static const struct swcr_enc_xform swcr_enc_xform_arc4 = {
	&enc_xform_arc4,
	NULL,
	NULL,
	NULL,
	NULL,
};

/* Authentication instances */
static const struct swcr_auth_hash swcr_auth_hash_null = {
	&auth_hash_null,
	null_init, null_update, null_final
};

static const struct swcr_auth_hash swcr_auth_hash_hmac_md5_96 = {
	&auth_hash_hmac_md5_96,
	(void (*) (void *)) MD5Init, MD5Update_int,
	(void (*) (u_int8_t *, void *)) MD5Final
};

static const struct swcr_auth_hash swcr_auth_hash_hmac_sha1_96 = {
	&auth_hash_hmac_sha1_96,
	SHA1Init_int, SHA1Update_int, SHA1Final_int
};

static const struct swcr_auth_hash swcr_auth_hash_hmac_ripemd_160_96 = {
	&auth_hash_hmac_ripemd_160_96,
	(void (*)(void *)) RMD160Init, RMD160Update_int,
	(void (*)(u_int8_t *, void *)) RMD160Final
};

static const struct swcr_auth_hash swcr_auth_hash_key_md5 = {
	&auth_hash_key_md5,
	(void (*)(void *)) MD5Init, MD5Update_int,
	(void (*)(u_int8_t *, void *)) MD5Final
};

static const struct swcr_auth_hash swcr_auth_hash_key_sha1 = {
	&auth_hash_key_sha1,
	SHA1Init_int, SHA1Update_int, SHA1Final_int
};

static const struct swcr_auth_hash swcr_auth_hash_md5 = {
	&auth_hash_md5,
	(void (*) (void *)) MD5Init, MD5Update_int,
	(void (*) (u_int8_t *, void *)) MD5Final
};

static const struct swcr_auth_hash swcr_auth_hash_sha1 = {
	&auth_hash_sha1,
	(void (*)(void *)) SHA1Init, SHA1Update_int,
	(void (*)(u_int8_t *, void *)) SHA1Final
};

static const struct swcr_auth_hash swcr_auth_hash_hmac_sha2_256 = {
	&auth_hash_hmac_sha2_256,
	(void (*)(void *)) SHA256_Init, SHA256Update_int,
	(void (*)(u_int8_t *, void *)) SHA256_Final
};

static const struct swcr_auth_hash swcr_auth_hash_hmac_sha2_384 = {
	&auth_hash_hmac_sha2_384,
	(void (*)(void *)) SHA384_Init, SHA384Update_int,
	(void (*)(u_int8_t *, void *)) SHA384_Final
};

static const struct swcr_auth_hash swcr_auth_hash_hmac_sha2_512 = {
	&auth_hash_hmac_sha2_384,
	(void (*)(void *)) SHA512_Init, SHA512Update_int,
	(void (*)(u_int8_t *, void *)) SHA512_Final
};

/* Compression instance */
static const struct swcr_comp_algo swcr_comp_algo_deflate = {
	&comp_algo_deflate,
	deflate_compress,
	deflate_decompress
};

/*
 * Encryption wrapper routines.
 */
static void
null_encrypt(caddr_t key __unused, u_int8_t *blk __unused)
{
}
static void
null_decrypt(caddr_t key __unused, u_int8_t *blk __unused)
{
}
static int
null_setkey(u_int8_t **sched, const u_int8_t *key __unused, int len __unused)
{
	*sched = NULL;
	return 0;
}
static void
null_zerokey(u_int8_t **sched)
{
	*sched = NULL;
}

static void
des1_encrypt(caddr_t key, u_int8_t *blk)
{
	des_cblock *cb = (des_cblock *) blk;
	des_key_schedule *p = (des_key_schedule *) key;

	des_ecb_encrypt(cb, cb, p[0], DES_ENCRYPT);
}

static void
des1_decrypt(caddr_t key, u_int8_t *blk)
{
	des_cblock *cb = (des_cblock *) blk;
	des_key_schedule *p = (des_key_schedule *) key;

	des_ecb_encrypt(cb, cb, p[0], DES_DECRYPT);
}

static int
des1_setkey(u_int8_t **sched, const u_int8_t *key, int len __unused)
{
	des_key_schedule *p;
	int err;

	MALLOC(p, des_key_schedule *, sizeof (des_key_schedule),
		M_CRYPTO_DATA, M_NOWAIT);
	if (p != NULL) {
		bzero(p, sizeof(des_key_schedule));
		des_set_key((des_cblock *)__UNCONST(key), p[0]);
		err = 0;
	} else
		err = ENOMEM;
	*sched = (u_int8_t *) p;
	return err;
}

static void
des1_zerokey(u_int8_t **sched)
{
	bzero(*sched, sizeof (des_key_schedule));
	FREE(*sched, M_CRYPTO_DATA);
	*sched = NULL;
}

static void
des3_encrypt(caddr_t key, u_int8_t *blk)
{
	des_cblock *cb = (des_cblock *) blk;
	des_key_schedule *p = (des_key_schedule *) key;

	des_ecb3_encrypt(cb, cb, p[0], p[1], p[2], DES_ENCRYPT);
}

static void
des3_decrypt(caddr_t key, u_int8_t *blk)
{
	des_cblock *cb = (des_cblock *) blk;
	des_key_schedule *p = (des_key_schedule *) key;

	des_ecb3_encrypt(cb, cb, p[0], p[1], p[2], DES_DECRYPT);
}

static int
des3_setkey(u_int8_t **sched, const u_int8_t *key, int len __unused)
{
	des_key_schedule *p;
	int err;

	MALLOC(p, des_key_schedule *, 3*sizeof (des_key_schedule),
		M_CRYPTO_DATA, M_NOWAIT);
	if (p != NULL) {
		bzero(p, 3*sizeof(des_key_schedule));
		des_set_key((des_cblock *)__UNCONST(key +  0), p[0]);
		des_set_key((des_cblock *)__UNCONST(key +  8), p[1]);
		des_set_key((des_cblock *)__UNCONST(key + 16), p[2]);
		err = 0;
	} else
		err = ENOMEM;
	*sched = (u_int8_t *) p;
	return err;
}

static void
des3_zerokey(u_int8_t **sched)
{
	bzero(*sched, 3*sizeof (des_key_schedule));
	FREE(*sched, M_CRYPTO_DATA);
	*sched = NULL;
}

static void
blf_encrypt(caddr_t key, u_int8_t *blk)
{

#if defined(__NetBSD__)
	BF_ecb_encrypt(blk, blk, (BF_KEY *)key, 1);
#else
	blf_ecb_encrypt((blf_ctx *) key, blk, 8);
#endif
}

static void
blf_decrypt(caddr_t key, u_int8_t *blk)
{

#if defined(__NetBSD__)
	BF_ecb_encrypt(blk, blk, (BF_KEY *)key, 0);
#else
	blf_ecb_decrypt((blf_ctx *) key, blk, 8);
#endif
}

static int
blf_setkey(u_int8_t **sched, const u_int8_t *key, int len)
{
	int err;

#if defined(__FreeBSD__) || defined(__NetBSD__)
#define	BLF_SIZ	sizeof(BF_KEY)
#else
#define	BLF_SIZ	sizeof(blf_ctx)
#endif

	MALLOC(*sched, u_int8_t *, BLF_SIZ,
		M_CRYPTO_DATA, M_NOWAIT);
	if (*sched != NULL) {
		bzero(*sched, BLF_SIZ);
#if defined(__FreeBSD__) || defined(__NetBSD__)
		BF_set_key((BF_KEY *) *sched, len, key);
#else
		blf_key((blf_ctx *)*sched, key, len);
#endif
		err = 0;
	} else
		err = ENOMEM;
	return err;
}

static void
blf_zerokey(u_int8_t **sched)
{
	bzero(*sched, BLF_SIZ);
	FREE(*sched, M_CRYPTO_DATA);
	*sched = NULL;
}

static void
cast5_encrypt(caddr_t key, u_int8_t *blk)
{
	cast128_encrypt((cast128_key *) key, blk, blk);
}

static void
cast5_decrypt(caddr_t key, u_int8_t *blk)
{
	cast128_decrypt((cast128_key *) key, blk, blk);
}

static int
cast5_setkey(u_int8_t **sched, const u_int8_t *key, int len)
{
	int err;

	MALLOC(*sched, u_int8_t *, sizeof(cast128_key), M_CRYPTO_DATA,
	       M_NOWAIT);
	if (*sched != NULL) {
		bzero(*sched, sizeof(cast128_key));
		cast128_setkey((cast128_key *)*sched, key, len);
		err = 0;
	} else
		err = ENOMEM;
	return err;
}

static void
cast5_zerokey(u_int8_t **sched)
{
	bzero(*sched, sizeof(cast128_key));
	FREE(*sched, M_CRYPTO_DATA);
	*sched = NULL;
}

static void
skipjack_encrypt(caddr_t key, u_int8_t *blk)
{
	skipjack_forwards(blk, blk, (u_int8_t **) key);
}

static void
skipjack_decrypt(caddr_t key, u_int8_t *blk)
{
	skipjack_backwards(blk, blk, (u_int8_t **) key);
}

static int
skipjack_setkey(u_int8_t **sched, const u_int8_t *key, int len __unused)
{
	int err;

	/* NB: allocate all the memory that's needed at once */
	/* XXX assumes bytes are aligned on sizeof(u_char) == 1 boundaries.
	 * Will this break a pdp-10, Cray-1, or GE-645 port?
	 */
	MALLOC(*sched, u_int8_t *, 10 * (sizeof(u_int8_t *) + 0x100),
		M_CRYPTO_DATA, M_NOWAIT);

	if (*sched != NULL) {

		u_int8_t** key_tables = (u_int8_t**) *sched;
		u_int8_t* table = (u_int8_t*) &key_tables[10];
		int k;

		bzero(*sched, 10 * sizeof(u_int8_t *)+0x100);

		for (k = 0; k < 10; k++) {
			key_tables[k] = table;
			table += 0x100;
		}
		subkey_table_gen(key, (u_int8_t **) *sched);
		err = 0;
	} else
		err = ENOMEM;
	return err;
}

static void
skipjack_zerokey(u_int8_t **sched)
{
	bzero(*sched, 10 * (sizeof(u_int8_t *) + 0x100));
	FREE(*sched, M_CRYPTO_DATA);
	*sched = NULL;
}

static void
rijndael128_encrypt(caddr_t key, u_int8_t *blk)
{
	rijndael_encrypt((rijndael_ctx *) key, (u_char *) blk, (u_char *) blk);
}

static void
rijndael128_decrypt(caddr_t key, u_int8_t *blk)
{
	rijndael_decrypt((rijndael_ctx *) key, (u_char *) blk,
	    (u_char *) blk);
}

static int
rijndael128_setkey(u_int8_t **sched, const u_int8_t *key, int len)
{
	int err;

	MALLOC(*sched, u_int8_t *, sizeof(rijndael_ctx), M_CRYPTO_DATA,
	    M_WAITOK);
	if (*sched != NULL) {
		bzero(*sched, sizeof(rijndael_ctx));
		rijndael_set_key((rijndael_ctx *) *sched, key, len * 8);
		err = 0;
	} else
		err = ENOMEM;
	return err;
}

static void
rijndael128_zerokey(u_int8_t **sched)
{
	bzero(*sched, sizeof(rijndael_ctx));
	FREE(*sched, M_CRYPTO_DATA);
	*sched = NULL;
}

/*
 * And now for auth.
 */

static void
null_init(void *ctx __unused)
{
}

static int
null_update(void *ctx __unused, const u_int8_t *buf __unused,
    u_int16_t len __unused)
{
	return 0;
}

static void
null_final(u_int8_t *buf, void *ctx __unused)
{
	if (buf != (u_int8_t *) 0)
		bzero(buf, 12);
}

static int
RMD160Update_int(void *ctx, const u_int8_t *buf, u_int16_t len)
{
	RMD160Update(ctx, buf, len);
	return 0;
}

static int
MD5Update_int(void *ctx, const u_int8_t *buf, u_int16_t len)
{
	MD5Update(ctx, buf, len);
	return 0;
}

static void
SHA1Init_int(void *ctx)
{
	SHA1Init(ctx);
}

static int
SHA1Update_int(void *ctx, const u_int8_t *buf, u_int16_t len)
{
	SHA1Update(ctx, buf, len);
	return 0;
}

static void
SHA1Final_int(u_int8_t *blk, void *ctx)
{
	SHA1Final(blk, ctx);
}

static int
SHA256Update_int(void *ctx, const u_int8_t *buf, u_int16_t len)
{
	SHA256_Update(ctx, buf, len);
	return 0;
}

static int
SHA384Update_int(void *ctx, const u_int8_t *buf, u_int16_t len)
{
	SHA384_Update(ctx, buf, len);
	return 0;
}

static int
SHA512Update_int(void *ctx, const u_int8_t *buf, u_int16_t len)
{
	SHA512_Update(ctx, buf, len);
	return 0;
}

/*
 * And compression
 */

static u_int32_t
deflate_compress(data, size, out)
	u_int8_t *data;
	u_int32_t size;
	u_int8_t **out;
{
	return deflate_global(data, size, 0, out);
}

static u_int32_t
deflate_decompress(data, size, out)
	u_int8_t *data;
	u_int32_t size;
	u_int8_t **out;
{
	return deflate_global(data, size, 1, out);
}
