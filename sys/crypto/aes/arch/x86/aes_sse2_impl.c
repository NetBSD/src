/*	$NetBSD: aes_sse2_impl.c,v 1.5 2020/07/25 22:29:56 riastradh Exp $	*/

/*-
 * Copyright (c) 2020 The NetBSD Foundation, Inc.
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

#include <sys/cdefs.h>
__KERNEL_RCSID(1, "$NetBSD: aes_sse2_impl.c,v 1.5 2020/07/25 22:29:56 riastradh Exp $");

#include <sys/types.h>
#include <sys/endian.h>

#include <crypto/aes/aes.h>
#include <crypto/aes/aes_impl.h>
#include <crypto/aes/arch/x86/aes_sse2.h>

#ifdef _KERNEL
#include <x86/cpu.h>
#include <x86/cpuvar.h>
#include <x86/fpu.h>
#include <x86/specialreg.h>
#else
#include <cpuid.h>
#define	fpu_kern_enter()	((void)0)
#define	fpu_kern_leave()	((void)0)
#endif

static void
aes_sse2_setenckey_impl(struct aesenc *enc, const uint8_t *key,
    uint32_t nrounds)
{

	fpu_kern_enter();
	aes_sse2_setkey(enc->aese_aes.aes_rk64, key, nrounds);
	fpu_kern_leave();
}

static void
aes_sse2_setdeckey_impl(struct aesdec *dec, const uint8_t *key,
    uint32_t nrounds)
{

	fpu_kern_enter();
	/*
	 * BearSSL computes InvMixColumns on the fly -- no need for
	 * distinct decryption round keys.
	 */
	aes_sse2_setkey(dec->aesd_aes.aes_rk64, key, nrounds);
	fpu_kern_leave();
}

static void
aes_sse2_enc_impl(const struct aesenc *enc, const uint8_t in[static 16],
    uint8_t out[static 16], uint32_t nrounds)
{

	fpu_kern_enter();
	aes_sse2_enc(enc, in, out, nrounds);
	fpu_kern_leave();
}

static void
aes_sse2_dec_impl(const struct aesdec *dec, const uint8_t in[static 16],
    uint8_t out[static 16], uint32_t nrounds)
{

	fpu_kern_enter();
	aes_sse2_dec(dec, in, out, nrounds);
	fpu_kern_leave();
}

static void
aes_sse2_cbc_enc_impl(const struct aesenc *enc, const uint8_t in[static 16],
    uint8_t out[static 16], size_t nbytes, uint8_t iv[static 16],
    uint32_t nrounds)
{

	if (nbytes == 0)
		return;
	fpu_kern_enter();
	aes_sse2_cbc_enc(enc, in, out, nbytes, iv, nrounds);
	fpu_kern_leave();
}

static void
aes_sse2_cbc_dec_impl(const struct aesdec *dec, const uint8_t in[static 16],
    uint8_t out[static 16], size_t nbytes, uint8_t iv[static 16],
    uint32_t nrounds)
{

	if (nbytes == 0)
		return;
	fpu_kern_enter();
	aes_sse2_cbc_dec(dec, in, out, nbytes, iv, nrounds);
	fpu_kern_leave();
}

static void
aes_sse2_xts_enc_impl(const struct aesenc *enc, const uint8_t in[static 16],
    uint8_t out[static 16], size_t nbytes, uint8_t tweak[static 16],
    uint32_t nrounds)
{

	if (nbytes == 0)
		return;
	fpu_kern_enter();
	aes_sse2_xts_enc(enc, in, out, nbytes, tweak, nrounds);
	fpu_kern_leave();
}

static void
aes_sse2_xts_dec_impl(const struct aesdec *dec, const uint8_t in[static 16],
    uint8_t out[static 16], size_t nbytes, uint8_t tweak[static 16],
    uint32_t nrounds)
{

	if (nbytes == 0)
		return;
	fpu_kern_enter();
	aes_sse2_xts_dec(dec, in, out, nbytes, tweak, nrounds);
	fpu_kern_leave();
}

static void
aes_sse2_cbcmac_update1_impl(const struct aesenc *enc,
    const uint8_t in[static 16], size_t nbytes, uint8_t auth[static 16],
    uint32_t nrounds)
{

	fpu_kern_enter();
	aes_sse2_cbcmac_update1(enc, in, nbytes, auth, nrounds);
	fpu_kern_leave();
}

static void
aes_sse2_ccm_enc1_impl(const struct aesenc *enc, const uint8_t in[static 16],
    uint8_t out[static 16], size_t nbytes, uint8_t authctr[static 32],
    uint32_t nrounds)
{

	fpu_kern_enter();
	aes_sse2_ccm_enc1(enc, in, out, nbytes, authctr, nrounds);
	fpu_kern_leave();
}

static void
aes_sse2_ccm_dec1_impl(const struct aesenc *enc, const uint8_t in[static 16],
    uint8_t out[static 16], size_t nbytes, uint8_t authctr[static 32],
    uint32_t nrounds)
{

	fpu_kern_enter();
	aes_sse2_ccm_dec1(enc, in, out, nbytes, authctr, nrounds);
	fpu_kern_leave();
}

static int
aes_sse2_probe(void)
{
	int result = 0;

	/* Verify that the CPU supports SSE and SSE2.  */
#ifdef _KERNEL
	if (!i386_has_sse)
		return -1;
	if (!i386_has_sse2)
		return -1;
#else
	unsigned eax, ebx, ecx, edx;
	if (!__get_cpuid(1, &eax, &ebx, &ecx, &edx))
		return -1;
	if ((edx & bit_SSE) == 0)
		return -1;
	if ((edx & bit_SSE2) == 0)
		return -1;
#endif

	fpu_kern_enter();
	result = aes_sse2_selftest();
	fpu_kern_leave();

	return result;
}

struct aes_impl aes_sse2_impl = {
	.ai_name = "Intel SSE2 bitsliced",
	.ai_probe = aes_sse2_probe,
	.ai_setenckey = aes_sse2_setenckey_impl,
	.ai_setdeckey = aes_sse2_setdeckey_impl,
	.ai_enc = aes_sse2_enc_impl,
	.ai_dec = aes_sse2_dec_impl,
	.ai_cbc_enc = aes_sse2_cbc_enc_impl,
	.ai_cbc_dec = aes_sse2_cbc_dec_impl,
	.ai_xts_enc = aes_sse2_xts_enc_impl,
	.ai_xts_dec = aes_sse2_xts_dec_impl,
	.ai_cbcmac_update1 = aes_sse2_cbcmac_update1_impl,
	.ai_ccm_enc1 = aes_sse2_ccm_enc1_impl,
	.ai_ccm_dec1 = aes_sse2_ccm_dec1_impl,
};
