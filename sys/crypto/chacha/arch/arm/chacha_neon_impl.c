/*	$NetBSD: chacha_neon_impl.c,v 1.1 2020/07/25 22:51:57 riastradh Exp $	*/

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
__KERNEL_RCSID(1, "$NetBSD: chacha_neon_impl.c,v 1.1 2020/07/25 22:51:57 riastradh Exp $");

#include "chacha_neon.h"

#ifdef __aarch64__
#include <aarch64/armreg.h>
#endif

#ifdef _KERNEL
#include <sys/proc.h>
#ifndef __aarch64__
#include <arm/locore.h>
#endif
#include <arm/fpu.h>
#else
#include <sys/sysctl.h>
#include <stddef.h>
#define	fpu_kern_enter()	((void)0)
#define	fpu_kern_leave()	((void)0)
#endif

static void
chacha_core_neon_impl(uint8_t out[restrict static 64],
    const uint8_t in[static 16],
    const uint8_t k[static 32],
    const uint8_t c[static 16],
    unsigned nr)
{

	fpu_kern_enter();
	chacha_core_neon(out, in, k, c, nr);
	fpu_kern_leave();
}

static void
hchacha_neon_impl(uint8_t out[restrict static 32],
    const uint8_t in[static 16],
    const uint8_t k[static 32],
    const uint8_t c[static 16],
    unsigned nr)
{

	fpu_kern_enter();
	hchacha_neon(out, in, k, c, nr);
	fpu_kern_leave();
}

static void
chacha_stream_neon_impl(uint8_t *restrict s, size_t nbytes, uint32_t blkno,
    const uint8_t nonce[static 12],
    const uint8_t key[static 32],
    unsigned nr)
{

	fpu_kern_enter();
	chacha_stream_neon(s, nbytes, blkno, nonce, key, nr);
	fpu_kern_leave();
}

static void
chacha_stream_xor_neon_impl(uint8_t *c, const uint8_t *p, size_t nbytes,
    uint32_t blkno,
    const uint8_t nonce[static 12],
    const uint8_t key[static 32],
    unsigned nr)
{

	fpu_kern_enter();
	chacha_stream_xor_neon(c, p, nbytes, blkno, nonce, key, nr);
	fpu_kern_leave();
}

static void
xchacha_stream_neon_impl(uint8_t *restrict s, size_t nbytes, uint32_t blkno,
    const uint8_t nonce[static 24],
    const uint8_t key[static 32],
    unsigned nr)
{

	fpu_kern_enter();
	xchacha_stream_neon(s, nbytes, blkno, nonce, key, nr);
	fpu_kern_leave();
}

static void
xchacha_stream_xor_neon_impl(uint8_t *c, const uint8_t *p, size_t nbytes,
    uint32_t blkno,
    const uint8_t nonce[static 24],
    const uint8_t key[static 32],
    unsigned nr)
{

	fpu_kern_enter();
	xchacha_stream_xor_neon(c, p, nbytes, blkno, nonce, key, nr);
	fpu_kern_leave();
}

static int
chacha_probe_neon(void)
{
#ifdef __aarch64__
	struct aarch64_sysctl_cpu_id *id;
#endif
	int result = 0;

	/* Verify that the CPU supports NEON.  */
#ifdef __aarch64__
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
	switch (__SHIFTOUT(id->ac_aa64pfr0, ID_AA64PFR0_EL1_ADVSIMD)) {
	case ID_AA64PFR0_EL1_ADV_SIMD_IMPL:
		break;
	default:
		return -1;
	}
#else
#ifdef _KERNEL
	if (!cpu_neon_present)
		return -1;
#else
	int neon;
	size_t neonlen = sizeof neon;
	if (sysctlbyname("machdep.neon_present", &neon, &neonlen, NULL, 0))
		return -1;
	if (!neon)
		return -1;
#endif
#endif

	return result;
}

const struct chacha_impl chacha_neon_impl = {
	.ci_name = "ARM NEON ChaCha",
	.ci_probe = chacha_probe_neon,
	.ci_chacha_core = chacha_core_neon_impl,
	.ci_hchacha = hchacha_neon_impl,
	.ci_chacha_stream = chacha_stream_neon_impl,
	.ci_chacha_stream_xor = chacha_stream_xor_neon_impl,
	.ci_xchacha_stream = xchacha_stream_neon_impl,
	.ci_xchacha_stream_xor = xchacha_stream_xor_neon_impl,
};
