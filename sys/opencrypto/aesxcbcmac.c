/* $NetBSD: aesxcbcmac.c,v 1.3 2020/06/29 23:34:48 riastradh Exp $ */

/*
 * Copyright (C) 1995, 1996, 1997, 1998 and 2003 WIDE Project.
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: aesxcbcmac.c,v 1.3 2020/06/29 23:34:48 riastradh Exp $");

#include <sys/param.h>
#include <sys/systm.h>

#include <crypto/aes/aes.h>

#include <opencrypto/aesxcbcmac.h>

int
aes_xcbc_mac_init(void *vctx, const uint8_t *key, u_int16_t keylen)
{
	static const uint8_t k1seed[AES_BLOCKSIZE] =
	    { 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1 };
	static const uint8_t k2seed[AES_BLOCKSIZE] =
	    { 2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2 };
	static const uint8_t k3seed[AES_BLOCKSIZE] =
	    { 3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3 };
	struct aesenc r_ks;
	aesxcbc_ctx *ctx;
	uint8_t k1[AES_BLOCKSIZE];

	ctx = vctx;
	memset(ctx, 0, sizeof(*ctx));

	switch (keylen) {
	case 16:
		ctx->r_nr = aes_setenckey128(&r_ks, key);
		break;
	case 24:
		ctx->r_nr = aes_setenckey192(&r_ks, key);
		break;
	case 32:
		ctx->r_nr = aes_setenckey256(&r_ks, key);
		break;
	}
	aes_enc(&r_ks, k1seed, k1, ctx->r_nr);
	aes_enc(&r_ks, k2seed, ctx->k2, ctx->r_nr);
	aes_enc(&r_ks, k3seed, ctx->k3, ctx->r_nr);
	aes_setenckey128(&ctx->r_k1s, k1);

	explicit_memset(&r_ks, 0, sizeof(r_ks));
	explicit_memset(k1, 0, sizeof(k1));

	return 0;
}

int
aes_xcbc_mac_loop(void *vctx, const uint8_t *addr, u_int16_t len)
{
	uint8_t buf[AES_BLOCKSIZE];
	aesxcbc_ctx *ctx;
	const uint8_t *ep;
	int i;

	ctx = vctx;
	ep = addr + len;

	if (ctx->buflen == sizeof(ctx->buf)) {
		for (i = 0; i < sizeof(ctx->e); i++)
			ctx->buf[i] ^= ctx->e[i];
		aes_enc(&ctx->r_k1s, ctx->buf, ctx->e, ctx->r_nr);
		ctx->buflen = 0;
	}
	if (ctx->buflen + len < sizeof(ctx->buf)) {
		memcpy(ctx->buf + ctx->buflen, addr, len);
		ctx->buflen += len;
		return 0;
	}
	if (ctx->buflen && ctx->buflen + len > sizeof(ctx->buf)) {
		memcpy(ctx->buf + ctx->buflen, addr,
		    sizeof(ctx->buf) - ctx->buflen);
		for (i = 0; i < sizeof(ctx->e); i++)
			ctx->buf[i] ^= ctx->e[i];
		aes_enc(&ctx->r_k1s, ctx->buf, ctx->e, ctx->r_nr);
		addr += sizeof(ctx->buf) - ctx->buflen;
		ctx->buflen = 0;
	}
	/* due to the special processing for M[n], "=" case is not included */
	while (ep - addr > AES_BLOCKSIZE) {
		memcpy(buf, addr, AES_BLOCKSIZE);
		for (i = 0; i < sizeof(buf); i++)
			buf[i] ^= ctx->e[i];
		aes_enc(&ctx->r_k1s, buf, ctx->e, ctx->r_nr);
		addr += AES_BLOCKSIZE;
	}
	if (addr < ep) {
		memcpy(ctx->buf + ctx->buflen, addr, ep - addr);
		ctx->buflen += ep - addr;
	}
	return 0;
}

void
aes_xcbc_mac_result(uint8_t *addr, void *vctx)
{
	uint8_t digest[AES_BLOCKSIZE];
	aesxcbc_ctx *ctx;
	int i;

	ctx = vctx;

	if (ctx->buflen == sizeof(ctx->buf)) {
		for (i = 0; i < sizeof(ctx->buf); i++) {
			ctx->buf[i] ^= ctx->e[i];
			ctx->buf[i] ^= ctx->k2[i];
		}
		aes_enc(&ctx->r_k1s, ctx->buf, digest, ctx->r_nr);
	} else {
		for (i = ctx->buflen; i < sizeof(ctx->buf); i++)
			ctx->buf[i] = (i == ctx->buflen) ? 0x80 : 0x00;
		for (i = 0; i < sizeof(ctx->buf); i++) {
			ctx->buf[i] ^= ctx->e[i];
			ctx->buf[i] ^= ctx->k3[i];
		}
		aes_enc(&ctx->r_k1s, ctx->buf, digest, ctx->r_nr);
	}

	memcpy(addr, digest, sizeof(digest));
}
