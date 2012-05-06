/*-
 * Copyright (c) 2012 Alistair Crooks <agc@NetBSD.org>
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
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#include <sys/types.h>
#include <sys/syslog.h>

#ifdef _KERNEL
# include <sys/kmem.h>
# define logmessage	log
#else
# include <stdio.h>
# include <stdlib.h>
# include <string.h>
# include <unistd.h>
#endif

#include "misc.h"
#include "rsa.h"
#include "verify.h"

#ifndef USE_ARG
#define USE_ARG(x)	/*LINTED*/(void)&(x)
#endif

#define RSA_MAX_MODULUS_BITS	16384
#define RSA_SMALL_MODULUS_BITS	3072
#define RSA_MAX_PUBEXP_BITS	64 /* exponent limit enforced for "large" modulus only */

static int
rsa_padding_check_none(uint8_t *to, int tlen, const uint8_t *from, int flen, int num)
{
	USE_ARG(num);
	if (flen > tlen) {
		printf("r too large\n");
		return -1;
	}
	(void) memset(to, 0x0, tlen - flen);
	(void) memcpy(to + tlen - flen, from, flen);
	return tlen;
}

static int
lowlevel_rsa_public_decrypt(const uint8_t *encbuf, int enclen, uint8_t *dec, const rsa_pubkey_t *rsa)
{
	uint8_t		*decbuf;
	BIGNUM		*decbn;
	BIGNUM		*encbn;
	int		 decbytes;
	int		 nbytes;
	int		 r;

	nbytes = 0;
	r = -1;
	decbuf = NULL;
	decbn = encbn = NULL;
	if (BN_num_bits(rsa->n) > RSA_MAX_MODULUS_BITS) {
		printf("rsa r modulus too large\n");
		goto err;
	}
	if (BN_cmp(rsa->n, rsa->e) <= 0) {
		printf("rsa r bad n value\n");
		goto err;
	}
	if (BN_num_bits(rsa->n) > RSA_SMALL_MODULUS_BITS &&
	    BN_num_bits(rsa->e) > RSA_MAX_PUBEXP_BITS) {
		printf("rsa r bad exponent limit\n");
		goto err;
	}
	if ((encbn = BN_new()) == NULL ||
	    (decbn = BN_new()) == NULL ||
	    (decbuf = netpgp_allocate(1, nbytes = BN_num_bytes(rsa->n))) == NULL) {
		printf("allocation failure\n");
		goto err;
	}
	if (enclen > nbytes) {
		printf("rsa r > mod len\n");
		goto err;
	}
	if (BN_bin2bn(encbuf, enclen, encbn) == NULL) {
		printf("null encrypted BN\n");
		goto err;
	}
	if (BN_cmp(encbn, rsa->n) >= 0) {
		printf("rsa r data too large for modulus\n");
		goto err;
	}
	if (BN_mod_exp(decbn, encbn, rsa->e, rsa->n, NULL) < 0) {
		printf("BN_mod_exp < 0\n");
		goto err;
	}
	decbytes = BN_num_bytes(decbn);
	(void) BN_bn2bin(decbn, decbuf);
	if ((r = rsa_padding_check_none(dec, nbytes, decbuf, decbytes, 0)) < 0) {
		printf("rsa r padding check failed\n");
	}
err:
	BN_free(encbn);
	BN_free(decbn);
	if (decbuf != NULL) {
		(void) memset(decbuf, 0x0, nbytes);
		netpgp_deallocate(decbuf, nbytes);
	}
	return r;
}

/* verify */
int
RSA_public_decrypt(int enclen, const unsigned char *enc, unsigned char *dec, RSA *rsa, int padding)
{
	rsa_pubkey_t	pub;
	int		ret;

	if (enc == NULL || dec == NULL || rsa == NULL) {
		return 0;
	}
	USE_ARG(padding);
	(void) memset(&pub, 0x0, sizeof(pub));
	pub.n = BN_dup(rsa->n);
	pub.e = BN_dup(rsa->e);
	ret = lowlevel_rsa_public_decrypt(enc, enclen, dec, &pub);
	BN_free(pub.n);
	BN_free(pub.e);
	return ret;
}
