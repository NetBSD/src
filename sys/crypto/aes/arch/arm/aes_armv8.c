/*	$NetBSD: aes_armv8.c,v 1.5 2020/07/25 22:33:04 riastradh Exp $	*/

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
__KERNEL_RCSID(1, "$NetBSD: aes_armv8.c,v 1.5 2020/07/25 22:33:04 riastradh Exp $");

#ifdef _KERNEL
#include <sys/types.h>
#include <sys/proc.h>
#include <sys/systm.h>
#else
#include <assert.h>
#include <err.h>
#include <stdint.h>
#include <string.h>
#define	KASSERT			assert
#define	panic(fmt, args...)	err(1, fmt, args)
#endif

#include <crypto/aes/aes.h>
#include <crypto/aes/aes_impl.h>
#include <crypto/aes/arch/arm/aes_armv8.h>

#include <aarch64/armreg.h>

#ifdef _KERNEL
#include <arm/fpu.h>
#else
#include <sys/sysctl.h>
#include <stddef.h>
#define	fpu_kern_enter()	((void)0)
#define	fpu_kern_leave()	((void)0)
#endif

static void
aesarmv8_setenckey(struct aesenc *enc, const uint8_t key[static 16],
    uint32_t nrounds)
{

	switch (nrounds) {
	case 10:
		aesarmv8_setenckey128(enc, key);
		break;
	case 12:
		aesarmv8_setenckey192(enc, key);
		break;
	case 14:
		aesarmv8_setenckey256(enc, key);
		break;
	default:
		panic("invalid AES rounds: %u", nrounds);
	}
}

static void
aesarmv8_setenckey_impl(struct aesenc *enc, const uint8_t key[static 16],
    uint32_t nrounds)
{

	fpu_kern_enter();
	aesarmv8_setenckey(enc, key, nrounds);
	fpu_kern_leave();
}

static void
aesarmv8_setdeckey_impl(struct aesdec *dec, const uint8_t key[static 16],
    uint32_t nrounds)
{
	struct aesenc enc;

	fpu_kern_enter();
	aesarmv8_setenckey(&enc, key, nrounds);
	aesarmv8_enctodec(&enc, dec, nrounds);
	fpu_kern_leave();

	explicit_memset(&enc, 0, sizeof enc);
}

static void
aesarmv8_enc_impl(const struct aesenc *enc, const uint8_t in[static 16],
    uint8_t out[static 16], uint32_t nrounds)
{

	fpu_kern_enter();
	aesarmv8_enc(enc, in, out, nrounds);
	fpu_kern_leave();
}

static void
aesarmv8_dec_impl(const struct aesdec *dec, const uint8_t in[static 16],
    uint8_t out[static 16], uint32_t nrounds)
{

	fpu_kern_enter();
	aesarmv8_dec(dec, in, out, nrounds);
	fpu_kern_leave();
}

static void
aesarmv8_cbc_enc_impl(const struct aesenc *enc, const uint8_t in[static 16],
    uint8_t out[static 16], size_t nbytes, uint8_t iv[static 16],
    uint32_t nrounds)
{

	KASSERT(nbytes % 16 == 0);

	fpu_kern_enter();
	aesarmv8_cbc_enc(enc, in, out, nbytes, iv, nrounds);
	fpu_kern_leave();
}

static void
aesarmv8_cbc_dec_impl(const struct aesdec *dec, const uint8_t in[static 16],
    uint8_t out[static 16], size_t nbytes, uint8_t iv[static 16],
    uint32_t nrounds)
{

	KASSERT(nbytes % 16 == 0);

	fpu_kern_enter();

	if (nbytes % 128) {
		aesarmv8_cbc_dec1(dec, in, out, nbytes % 128, iv, nrounds);
		in += nbytes % 128;
		out += nbytes % 128;
		nbytes -= nbytes % 128;
	}

	KASSERT(nbytes % 128 == 0);
	if (nbytes)
		aesarmv8_cbc_dec8(dec, in, out, nbytes, iv, nrounds);

	fpu_kern_leave();
}

static void
aesarmv8_xts_enc_impl(const struct aesenc *enc, const uint8_t in[static 16],
    uint8_t out[static 16], size_t nbytes, uint8_t tweak[static 16],
    uint32_t nrounds)
{

	KASSERT(nbytes % 16 == 0);

	fpu_kern_enter();

	if (nbytes % 128) {
		aesarmv8_xts_enc1(enc, in, out, nbytes % 128, tweak, nrounds);
		in += nbytes % 128;
		out += nbytes % 128;
		nbytes -= nbytes % 128;
	}

	KASSERT(nbytes % 128 == 0);
	if (nbytes)
		aesarmv8_xts_enc8(enc, in, out, nbytes, tweak, nrounds);

	fpu_kern_leave();
}

static void
aesarmv8_xts_dec_impl(const struct aesdec *dec, const uint8_t in[static 16],
    uint8_t out[static 16], size_t nbytes, uint8_t tweak[static 16],
    uint32_t nrounds)
{

	KASSERT(nbytes % 16 == 0);

	fpu_kern_enter();

	if (nbytes % 128) {
		aesarmv8_xts_dec1(dec, in, out, nbytes % 128, tweak, nrounds);
		in += nbytes % 128;
		out += nbytes % 128;
		nbytes -= nbytes % 128;
	}

	KASSERT(nbytes % 128 == 0);
	if (nbytes)
		aesarmv8_xts_dec8(dec, in, out, nbytes, tweak, nrounds);

	fpu_kern_leave();
}

static void
aesarmv8_cbcmac_update1_impl(const struct aesenc *enc,
    const uint8_t in[static 16], size_t nbytes, uint8_t auth[static 16],
    uint32_t nrounds)
{

	KASSERT(nbytes);
	KASSERT(nbytes % 16 == 0);

	fpu_kern_enter();
	aesarmv8_cbcmac_update1(enc, in, nbytes, auth, nrounds);
	fpu_kern_leave();
}

static void
aesarmv8_ccm_enc1_impl(const struct aesenc *enc, const uint8_t in[static 16],
    uint8_t out[static 16], size_t nbytes, uint8_t authctr[static 32],
    uint32_t nrounds)
{

	KASSERT(nbytes);
	KASSERT(nbytes % 16 == 0);

	fpu_kern_enter();
	aesarmv8_ccm_enc1(enc, in, out, nbytes, authctr, nrounds);
	fpu_kern_leave();
}

static void
aesarmv8_ccm_dec1_impl(const struct aesenc *enc, const uint8_t in[static 16],
    uint8_t out[static 16], size_t nbytes, uint8_t authctr[static 32],
    uint32_t nrounds)
{

	KASSERT(nbytes);
	KASSERT(nbytes % 16 == 0);

	fpu_kern_enter();
	aesarmv8_ccm_dec1(enc, in, out, nbytes, authctr, nrounds);
	fpu_kern_leave();
}

static int
aesarmv8_xts_update_selftest(void)
{
	static const struct {
		uint8_t	in[16], out[16];
	} cases[] = {
		{{1}, {2}},
		{{0,0,0,0x80}, {0,0,0,0,1}},
		{{0,0,0,0,0,0,0,0x80}, {0,0,0,0,0,0,0,0,1}},
		{{0,0,0,0x80,0,0,0,0x80}, {0,0,0,0,1,0,0,0,1}},
		{{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0x80}, {0x87}},
		{{0,0,0,0,0,0,0,0x80,0,0,0,0,0,0,0,0x80},
		 {0x87,0,0,0,0,0,0,0,1}},
		{{0,0,0,0x80,0,0,0,0,0,0,0,0,0,0,0,0x80}, {0x87,0,0,0,1}},
		{{0,0,0,0x80,0,0,0,0x80,0,0,0,0,0,0,0,0x80},
		 {0x87,0,0,0,1,0,0,0,1}},
	};
	unsigned i;
	uint8_t tweak[16];

	for (i = 0; i < sizeof(cases)/sizeof(cases[0]); i++) {
		aesarmv8_xts_update(cases[i].in, tweak);
		if (memcmp(tweak, cases[i].out, 16))
			return -1;
	}

	/* Success!  */
	return 0;
}

static int
aesarmv8_probe(void)
{
	struct aarch64_sysctl_cpu_id *id;
	int result = 0;

	/* Verify that the CPU supports AES.  */
#ifdef _KERNEL
	id = &curcpu()->ci_id;
#else
	struct aarch64_sysctl_cpu_id ids;
	size_t idlen;
	id = &ids;
	idlen = sizeof ids;
	if (sysctlbyname("machdep.cpu0.cpu_id", id, &idlen, NULL, 0))
		return -1;
	if (idlen != sizeof ids)
		return -1;
#endif
	switch (__SHIFTOUT(id->ac_aa64isar0, ID_AA64ISAR0_EL1_AES)) {
	case ID_AA64ISAR0_EL1_AES_AES:
	case ID_AA64ISAR0_EL1_AES_PMUL:
		break;
	default:
		return -1;
	}

	fpu_kern_enter();

	/* Verify that our XTS tweak update logic works.  */
	if (aesarmv8_xts_update_selftest())
		result = -1;

	fpu_kern_leave();

	return result;
}

struct aes_impl aes_armv8_impl = {
	.ai_name = "ARMv8.0-AES",
	.ai_probe = aesarmv8_probe,
	.ai_setenckey = aesarmv8_setenckey_impl,
	.ai_setdeckey = aesarmv8_setdeckey_impl,
	.ai_enc = aesarmv8_enc_impl,
	.ai_dec = aesarmv8_dec_impl,
	.ai_cbc_enc = aesarmv8_cbc_enc_impl,
	.ai_cbc_dec = aesarmv8_cbc_dec_impl,
	.ai_xts_enc = aesarmv8_xts_enc_impl,
	.ai_xts_dec = aesarmv8_xts_dec_impl,
	.ai_cbcmac_update1 = aesarmv8_cbcmac_update1_impl,
	.ai_ccm_enc1 = aesarmv8_ccm_enc1_impl,
	.ai_ccm_dec1 = aesarmv8_ccm_dec1_impl,
};
