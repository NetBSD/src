/*	$NetBSD: aes_neon_impl.c,v 1.1 2020/06/29 23:56:31 riastradh Exp $	*/

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
__KERNEL_RCSID(1, "$NetBSD: aes_neon_impl.c,v 1.1 2020/06/29 23:56:31 riastradh Exp $");

#include <sys/types.h>
#include <sys/proc.h>

#include <crypto/aes/aes.h>
#include <crypto/aes/arch/arm/aes_neon.h>

#include <arm/fpu.h>

#ifdef __aarch64__
#include <aarch64/armreg.h>
#else
#include <arm/locore.h>
#endif

static void
aes_neon_setenckey_impl(struct aesenc *enc, const uint8_t *key,
    uint32_t nrounds)
{

	fpu_kern_enter();
	aes_neon_setenckey(enc, key, nrounds);
	fpu_kern_leave();
}

static void
aes_neon_setdeckey_impl(struct aesdec *dec, const uint8_t *key,
    uint32_t nrounds)
{

	fpu_kern_enter();
	aes_neon_setdeckey(dec, key, nrounds);
	fpu_kern_leave();
}

static void
aes_neon_enc_impl(const struct aesenc *enc, const uint8_t in[static 16],
    uint8_t out[static 16], uint32_t nrounds)
{

	fpu_kern_enter();
	aes_neon_enc(enc, in, out, nrounds);
	fpu_kern_leave();
}

static void
aes_neon_dec_impl(const struct aesdec *dec, const uint8_t in[static 16],
    uint8_t out[static 16], uint32_t nrounds)
{

	fpu_kern_enter();
	aes_neon_dec(dec, in, out, nrounds);
	fpu_kern_leave();
}

static void
aes_neon_cbc_enc_impl(const struct aesenc *enc, const uint8_t in[static 16],
    uint8_t out[static 16], size_t nbytes, uint8_t iv[static 16],
    uint32_t nrounds)
{

	if (nbytes == 0)
		return;
	fpu_kern_enter();
	aes_neon_cbc_enc(enc, in, out, nbytes, iv, nrounds);
	fpu_kern_leave();
}

static void
aes_neon_cbc_dec_impl(const struct aesdec *dec, const uint8_t in[static 16],
    uint8_t out[static 16], size_t nbytes, uint8_t iv[static 16],
    uint32_t nrounds)
{

	if (nbytes == 0)
		return;
	fpu_kern_enter();
	aes_neon_cbc_dec(dec, in, out, nbytes, iv, nrounds);
	fpu_kern_leave();
}

static void
aes_neon_xts_enc_impl(const struct aesenc *enc, const uint8_t in[static 16],
    uint8_t out[static 16], size_t nbytes, uint8_t iv[static 16],
    uint32_t nrounds)
{

	if (nbytes == 0)
		return;
	fpu_kern_enter();
	aes_neon_xts_enc(enc, in, out, nbytes, iv, nrounds);
	fpu_kern_leave();
}

static void
aes_neon_xts_dec_impl(const struct aesdec *dec, const uint8_t in[static 16],
    uint8_t out[static 16], size_t nbytes, uint8_t iv[static 16],
    uint32_t nrounds)
{

	if (nbytes == 0)
		return;
	fpu_kern_enter();
	aes_neon_xts_dec(dec, in, out, nbytes, iv, nrounds);
	fpu_kern_leave();
}

static int
aes_neon_probe(void)
{
#ifdef __aarch64__
	struct aarch64_sysctl_cpu_id *id;
#endif
	int result = 0;

	/* Verify that the CPU supports NEON.  */
#ifdef __aarch64__
	id = &curcpu()->ci_id;
	switch (__SHIFTOUT(id->ac_aa64pfr0, ID_AA64PFR0_EL1_ADVSIMD)) {
	case ID_AA64PFR0_EL1_ADV_SIMD_IMPL:
		break;
	default:
		return -1;
	}
#else
	if (!cpu_neon_present)
		return -1;
#endif

	fpu_kern_enter();
	result = aes_neon_selftest();
	fpu_kern_leave();

	return result;
}

struct aes_impl aes_neon_impl = {
	.ai_name = "ARM NEON vpaes",
	.ai_probe = aes_neon_probe,
	.ai_setenckey = aes_neon_setenckey_impl,
	.ai_setdeckey = aes_neon_setdeckey_impl,
	.ai_enc = aes_neon_enc_impl,
	.ai_dec = aes_neon_dec_impl,
	.ai_cbc_enc = aes_neon_cbc_enc_impl,
	.ai_cbc_dec = aes_neon_cbc_dec_impl,
	.ai_xts_enc = aes_neon_xts_enc_impl,
	.ai_xts_dec = aes_neon_xts_dec_impl,
};
