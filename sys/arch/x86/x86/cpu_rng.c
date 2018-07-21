/* $NetBSD: cpu_rng.c,v 1.7 2018/07/21 14:46:41 kre Exp $ */

/*-
 * Copyright (c) 2015 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Thor Lancelot Simon.
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
 * The VIA RNG code in this file is inspired by Jason Wright and
 * Theo de Raadt's OpenBSD version but has been rewritten in light of
 * comments from Henric Jungheim on the tech@openbsd.org mailing list.
 */

#include "opt_xen.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/cpu.h>
#include <sys/sha2.h>

#include <x86/specialreg.h>

#include <machine/cpufunc.h>
#include <machine/cpuvar.h>
#include <machine/cpu_rng.h>

static enum {
	CPU_RNG_NONE = 0,
	CPU_RNG_RDRAND,
	CPU_RNG_RDSEED,
	CPU_RNG_VIA
} cpu_rng_mode __read_mostly = CPU_RNG_NONE;

bool
cpu_rng_init(void)
{

	if (cpu_feature[5] & CPUID_SEF_RDSEED) {
		cpu_rng_mode = CPU_RNG_RDSEED;
		aprint_normal("cpu_rng: RDSEED\n");
		return true;
	} else if (cpu_feature[1] & CPUID2_RDRAND) {
		cpu_rng_mode = CPU_RNG_RDRAND;
		aprint_normal("cpu_rng: RDRAND\n");
		return true;
	} else if (cpu_feature[4] & CPUID_VIA_HAS_RNG) {
		cpu_rng_mode = CPU_RNG_VIA;
		aprint_normal("cpu_rng: VIA\n");
		return true;
	}
	return false;
}

static size_t
cpu_rng_rdrand(cpu_rng_t *out)
{
	uint8_t rndsts;

#ifdef __i386__
	uint32_t lo, hi;

	__asm __volatile("rdrand %0; setc %1" : "=r"(lo), "=qm"(rndsts));
	if (rndsts != 1)
		return 0;
	__asm __volatile("rdrand %0; setc %1" : "=r"(hi), "=qm"(rndsts));

	*out = (uint64_t)lo | ((uint64_t)hi << 32);
	explicit_memset(&lo, 0, sizeof(lo));
	explicit_memset(&hi, 0, sizeof(hi));
	if (rndsts != 1)
		return sizeof(lo) * NBBY;
#else
	__asm __volatile("rdrand %0; setc %1" : "=r"(*out), "=qm"(rndsts));
	if (rndsts != 1)
		return 0;
#endif
	return sizeof(*out) * NBBY;
}

static size_t
cpu_rng_rdseed(cpu_rng_t *out)
{
	uint8_t rndsts;

#ifdef __i386__
	uint32_t lo, hi;

	__asm __volatile("rdseed %0; setc %1" : "=r"(lo), "=qm"(rndsts));
        if (rndsts != 1)
		goto exhausted;
	__asm __volatile("rdseed %0; setc %1" : "=r"(hi), "=qm"(rndsts));
	if (rndsts != 1)
		goto exhausted;

	*out = (uint64_t)lo | ((uint64_t)hi << 32);
	explicit_memset(&lo, 0, sizeof(lo));
	explicit_memset(&hi, 0, sizeof(hi));
#else
	__asm __volatile("rdseed %0; setc %1" : "=r"(*out), "=qm"(rndsts));
#endif
	if (rndsts != 1)
		goto exhausted;

	return sizeof(*out) * NBBY;

	/*
	 * Userspace could have exhausted RDSEED, but the
	 * CPU-internal generator feeding RDRAND is guaranteed
	 * to be seeded even in this case.
	 */
exhausted:
	return cpu_rng_rdrand(out);
}

static size_t
cpu_rng_via(cpu_rng_t *out)
{
	uint32_t creg0, rndsts;

	/*
	 * Sadly, we have to monkey with the coprocessor enable and fault
	 * registers, which are really for the FPU, in order to read
	 * from the RNG.
	 *
	 * Don't remove CR0_TS from the call below -- comments in the Linux
	 * driver indicate that the xstorerng instruction can generate
	 * spurious DNA faults though no FPU or SIMD state is changed
	 * even if such a fault is generated.
	 *
	 * XXX can this really happen if we don't use "rep xstorrng"?
	 *
	 */
	kpreempt_disable();
	x86_disable_intr();
	creg0 = rcr0();
	lcr0(creg0 & ~(CR0_EM|CR0_TS)); /* Permit access to SIMD/FPU path */
	/*
	 * The VIA RNG has an output queue of 8-byte values.  Read one.
	 * This is atomic, so if the FPU were already enabled, we could skip
	 * all the preemption and interrupt frobbing.  If we had bread,
	 * we could have a ham sandwich, if we had any ham.
	 */
	__asm __volatile("xstorerng"
	    : "=a" (rndsts), "+D" (out) : "d" (0) : "memory");
	/* Put CR0 back how it was */
	lcr0(creg0);
	x86_enable_intr();
	kpreempt_enable();

	/*
	 * The Cryptography Research paper on the VIA RNG estimates
	 * 0.75 bits of entropy per output bit and advises users to
	 * be "even more conservative".
	 */
	return rndsts & 0xf ? 0 : sizeof(cpu_rng_t) * NBBY / 2;
}

size_t
cpu_rng(cpu_rng_t *out)
{

	switch (cpu_rng_mode) {
	case CPU_RNG_NONE:
		return 0;
	case CPU_RNG_RDSEED:
		return cpu_rng_rdseed(out);
	case CPU_RNG_RDRAND:
		return cpu_rng_rdrand(out);
	case CPU_RNG_VIA:
		return cpu_rng_via(out);
	default:
		panic("cpu_rng: unknown mode %d", (int)cpu_rng_mode);
	}
}

/* -------------------------------------------------------------------------- */

static uint64_t earlyrng_state;

/*
 * Small PRNG, that can be used very early. The only requirement is that
 * cpu_probe got called before.
 */
void
cpu_earlyrng(void *out, size_t sz)
{
	uint8_t digest[SHA512_DIGEST_LENGTH];
	SHA512_CTX ctx;
	cpu_rng_t buf[8];
	uint64_t val;
	int i;

	bool has_rdseed = (cpu_feature[5] & CPUID_SEF_RDSEED) != 0;
	bool has_rdrand = (cpu_feature[1] & CPUID2_RDRAND) != 0;

	KASSERT(sz + sizeof(uint64_t) <= SHA512_DIGEST_LENGTH);

	SHA512_Init(&ctx);

	SHA512_Update(&ctx, (uint8_t *)&earlyrng_state, sizeof(earlyrng_state));
	if (has_rdseed) {
		for (i = 0; i < 8; i++) {
			if (cpu_rng_rdseed(&buf[i]) == 0) {
				break;
			}
		}
		SHA512_Update(&ctx, (uint8_t *)buf, i * sizeof(cpu_rng_t));
	} else if (has_rdrand) {
		for (i = 0; i < 8; i++) {
			if (cpu_rng_rdrand(&buf[i]) == 0) {
				break;
			}
		}
		SHA512_Update(&ctx, (uint8_t *)buf, i * sizeof(cpu_rng_t));
	}
#ifndef XEN
	val = rdtsc();
#else
	val = 0;		/* XXX */
#endif
	SHA512_Update(&ctx, (uint8_t *)&val, sizeof(val));

	SHA512_Final(digest, &ctx);

	memcpy(out, digest, sz);
	memcpy(&earlyrng_state, &digest[sz], sizeof(earlyrng_state));
}
