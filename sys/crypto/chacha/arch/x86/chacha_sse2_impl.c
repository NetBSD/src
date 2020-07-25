/*	$NetBSD: chacha_sse2_impl.c,v 1.1 2020/07/25 22:49:20 riastradh Exp $	*/

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
__KERNEL_RCSID(1, "$NetBSD: chacha_sse2_impl.c,v 1.1 2020/07/25 22:49:20 riastradh Exp $");

#include "chacha_sse2.h"

#ifdef _KERNEL
#include <x86/cpu.h>
#include <x86/fpu.h>
#else
#include <sys/sysctl.h>
#include <cpuid.h>
#include <stddef.h>
#define	fpu_kern_enter()	((void)0)
#define	fpu_kern_leave()	((void)0)
#endif

static void
chacha_core_sse2_impl(uint8_t out[restrict static 64],
    const uint8_t in[static 16],
    const uint8_t k[static 32],
    const uint8_t c[static 16],
    unsigned nr)
{

	fpu_kern_enter();
	chacha_core_sse2(out, in, k, c, nr);
	fpu_kern_leave();
}

static void
hchacha_sse2_impl(uint8_t out[restrict static 32],
    const uint8_t in[static 16],
    const uint8_t k[static 32],
    const uint8_t c[static 16],
    unsigned nr)
{

	fpu_kern_enter();
	hchacha_sse2(out, in, k, c, nr);
	fpu_kern_leave();
}

static void
chacha_stream_sse2_impl(uint8_t *restrict s, size_t nbytes, uint32_t blkno,
    const uint8_t nonce[static 12],
    const uint8_t key[static 32],
    unsigned nr)
{

	fpu_kern_enter();
	chacha_stream_sse2(s, nbytes, blkno, nonce, key, nr);
	fpu_kern_leave();
}

static void
chacha_stream_xor_sse2_impl(uint8_t *c, const uint8_t *p, size_t nbytes,
    uint32_t blkno,
    const uint8_t nonce[static 12],
    const uint8_t key[static 32],
    unsigned nr)
{

	fpu_kern_enter();
	chacha_stream_xor_sse2(c, p, nbytes, blkno, nonce, key, nr);
	fpu_kern_leave();
}

static void
xchacha_stream_sse2_impl(uint8_t *restrict s, size_t nbytes, uint32_t blkno,
    const uint8_t nonce[static 24],
    const uint8_t key[static 32],
    unsigned nr)
{

	fpu_kern_enter();
	xchacha_stream_sse2(s, nbytes, blkno, nonce, key, nr);
	fpu_kern_leave();
}

static void
xchacha_stream_xor_sse2_impl(uint8_t *c, const uint8_t *p, size_t nbytes,
    uint32_t blkno,
    const uint8_t nonce[static 24],
    const uint8_t key[static 32],
    unsigned nr)
{

	fpu_kern_enter();
	xchacha_stream_xor_sse2(c, p, nbytes, blkno, nonce, key, nr);
	fpu_kern_leave();
}

static int
chacha_probe_sse2(void)
{

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

	return 0;
}

const struct chacha_impl chacha_sse2_impl = {
	.ci_name = "x86 SSE2 ChaCha",
	.ci_probe = chacha_probe_sse2,
	.ci_chacha_core = chacha_core_sse2_impl,
	.ci_hchacha = hchacha_sse2_impl,
	.ci_chacha_stream = chacha_stream_sse2_impl,
	.ci_chacha_stream_xor = chacha_stream_xor_sse2_impl,
	.ci_xchacha_stream = xchacha_stream_sse2_impl,
	.ci_xchacha_stream_xor = xchacha_stream_xor_sse2_impl,
};
