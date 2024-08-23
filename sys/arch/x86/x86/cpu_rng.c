/* $NetBSD: cpu_rng.c,v 1.20.4.1 2024/08/23 18:15:31 martin Exp $ */

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
 * For reference on VIA XSTORERNG, see the VIA PadLock Programming
 * Guide (`VIA PPG'), August 4, 2005.
 * http://linux.via.com.tw/support/beginDownload.action?eleid=181&fid=261
 *
 * For reference on Intel RDRAND/RDSEED, see the Intel Digital Random
 * Number Generator Software Implementation Guide (`Intel DRNG SIG'),
 * Revision 2.1, October 17, 2018.
 * https://software.intel.com/sites/default/files/managed/98/4a/DRNG_Software_Implementation_Guide_2.1.pdf
 *
 * For reference on AMD RDRAND/RDSEED, which are designed to be
 * compatible with Intel RDRAND/RDSEED, see the somewhat less detailed
 * AMD Random Number Generator documentation, 2017-06-27.
 * https://www.amd.com/system/files/TechDocs/amd-random-number-generator.pdf
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/cpu.h>
#include <sys/rndsource.h>
#include <sys/sha2.h>

#include <x86/specialreg.h>

#include <machine/cpufunc.h>
#include <machine/cpuvar.h>
#include <machine/cpu_rng.h>
#include <machine/limits.h>

static enum cpu_rng_mode {
	CPU_RNG_NONE = 0,
	CPU_RNG_RDRAND,
	CPU_RNG_RDSEED,
	CPU_RNG_RDSEED_RDRAND,
	CPU_RNG_VIA
} cpu_rng_mode __read_mostly = CPU_RNG_NONE;

static const char *const cpu_rng_name[] = {
	[CPU_RNG_RDRAND] = "rdrand",
	[CPU_RNG_RDSEED] = "rdseed",
	[CPU_RNG_RDSEED_RDRAND] = "rdrand/rdseed",
	[CPU_RNG_VIA] = "via",
};

static struct krndsource cpu_rng_source __read_mostly;

static enum cpu_rng_mode
cpu_rng_detect(void)
{
	bool has_rdseed = (cpu_feature[5] & CPUID_SEF_RDSEED);
	bool has_rdrand = (cpu_feature[1] & CPUID2_RDRAND);
	bool has_viarng = (cpu_feature[4] & CPUID_VIA_HAS_RNG);

	if (has_rdseed && has_rdrand)
		return CPU_RNG_RDSEED_RDRAND;
	else if (has_rdseed)
		return CPU_RNG_RDSEED;
	else if (has_rdrand)
		return CPU_RNG_RDRAND;
	else if (has_viarng)
		return CPU_RNG_VIA;
	else
		return CPU_RNG_NONE;
}

static size_t
cpu_rng_rdrand(uint64_t *out)
{
	uint8_t rndsts;

	/*
	 * XXX The Intel DRNG SIG recommends (Sec. 5.2.1 `Retry
	 * recommendations', p. 22) that we retry up to ten times
	 * before giving up and panicking because something must be
	 * seriously awry with the CPU.
	 *
	 * XXX The Intel DRNG SIG also recommends (Sec. 5.2.6
	 * `Generating Seeds from RDRAND', p. 28) drawing 1024 64-bit
	 * samples (or, 512 128-bit samples) in order to guarantee that
	 * the CPU has drawn an independent sample from the physical
	 * entropy source, since the AES CTR_DRBG behind RDRAND will be
	 * used to generate at most 511 128-bit samples before it is
	 * reseeded from the physical entropy source.  It is unclear
	 * whether the same considerations about RDSEED starvation
	 * apply to this advice.
	 */

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
cpu_rng_rdseed(uint64_t *out)
{
	uint8_t rndsts;

	/*
	 * XXX The Intel DRNG SIG recommends (Sec. 5.3.1 `Retry
	 * recommendations', p. 22) that we consider retrying up to 100
	 * times, separated by PAUSE, but offers no guarantees about
	 * success after that many retries.  In particular, userland
	 * threads could starve the kernel by issuing RDSEED.
	 */

#ifdef __i386__
	uint32_t lo, hi;

	__asm __volatile("rdseed %0; setc %1" : "=r"(lo), "=qm"(rndsts));
	if (rndsts != 1)
		return 0;
	__asm __volatile("rdseed %0; setc %1" : "=r"(hi), "=qm"(rndsts));
	if (rndsts != 1)
		return 0;

	*out = (uint64_t)lo | ((uint64_t)hi << 32);
	explicit_memset(&lo, 0, sizeof(lo));
	explicit_memset(&hi, 0, sizeof(hi));
#else
	__asm __volatile("rdseed %0; setc %1" : "=r"(*out), "=qm"(rndsts));
#endif
	if (rndsts != 1)
		return 0;

	return sizeof(*out) * NBBY;
}

static size_t
cpu_rng_rdseed_rdrand(uint64_t *out)
{
	size_t n = cpu_rng_rdseed(out);

	if (n == 0)
		n = cpu_rng_rdrand(out);

	return n;
}

/*
 * VIA PPG says EAX[4:0] is nbytes, but the only documented numbers of
 * bytes are 0,1,2,4,8 -- and there's only 8 bytes of output buffer
 * anyway, so let's ignore bit 4 and treat it like EAX[3:0] instead.
 */
#define	VIA_RNG_STATUS_NBYTES	__BITS(3,0)
#define	VIA_RNG_STATUS_MSR110B	__BITS(31,5)

static size_t
cpu_rng_via(uint64_t *out)
{
	u_long psl;
	uint32_t cr0, status, nbytes;

	/*
	 * The XSTORE instruction is handled by the SSE unit, which
	 * requires the CR0 TS and CR0 EM bits to be clear.  We disable
	 * all processor interrupts so there is no danger of any
	 * interrupt handler changing CR0 while we work -- although
	 * really, software splvm or fpu_kern_enter/leave should be
	 * enough (but we'll do that in a separate change for the
	 * benefit of bisection in case I'm wrong).
	 */
	psl = x86_read_psl();
	x86_disable_intr();
	cr0 = rcr0();
	lcr0(cr0 & ~(CR0_EM|CR0_TS));

	/* Read up to eight bytes out of the buffer.  */
	asm volatile("xstorerng"
	    : "=a"(status)
	    : "D"(out), "d"(0) /* EDX[1:0]=00 -> wait for 8 bytes or fail */
	    : "memory");

	/* Restore CR0 and interrupts.  */
	lcr0(cr0);
	x86_write_psl(psl);

	/* Get the number of bytes stored.  (Should always be 8 or 0.)  */
	nbytes = __SHIFTOUT(status, VIA_RNG_STATUS_NBYTES);

	/*
	 * The Cryptography Research paper on the VIA RNG estimates
	 * 0.75 bits of entropy per output bit and advises users to
	 * be "even more conservative".
	 *
	 *	`Evaluation of VIA C3 Nehemiah Random Number
	 *	Generator', Cryptography Research, Inc., February 27,
	 *	2003.
	 *	https://www.rambus.com/wp-content/uploads/2015/08/VIA_rng.pdf
	 */
	return nbytes * NBBY/2;
}

static size_t
cpu_rng(enum cpu_rng_mode mode, uint64_t *out)
{

	switch (mode) {
	case CPU_RNG_NONE:
		return 0;
	case CPU_RNG_RDSEED:
		return cpu_rng_rdseed(out);
	case CPU_RNG_RDRAND:
		return cpu_rng_rdrand(out);
	case CPU_RNG_RDSEED_RDRAND:
		return cpu_rng_rdseed_rdrand(out);
	case CPU_RNG_VIA:
		return cpu_rng_via(out);
	default:
		panic("cpu_rng: unknown mode %d", (int)mode);
	}
}

static void
cpu_rng_get(size_t nbytes, void *cookie)
{
	enum {
		NBITS = 256,
		NBYTES = howmany(NBITS, 8),
		NWORDS = howmany(NBITS, 64),
	};
	uint64_t buf[2*NWORDS];
	unsigned i, nbits = 0;

	while (nbytes) {
		/*
		 * The fraction of outputs this rejects in correct
		 * operation is 1/2^256, which is close enough to zero
		 * that we round it to having no effect on the number
		 * of bits of entropy.
		 */
		for (i = 0; i < __arraycount(buf); i++)
			nbits += cpu_rng(cpu_rng_mode, &buf[i]);
		if (consttime_memequal(buf, buf + NWORDS, NBYTES)) {
			printf("cpu_rng %s: failed repetition test\n",
			    cpu_rng_name[cpu_rng_mode]);
			nbits = 0;
		}
		rnd_add_data_sync(&cpu_rng_source, buf, sizeof buf, nbits);
		nbytes -= MIN(MIN(nbytes, sizeof buf), MAX(1, 8*nbits));
	}
}

void
cpu_rng_init(void)
{

	cpu_rng_mode = cpu_rng_detect();
	if (cpu_rng_mode == CPU_RNG_NONE)
		return;
	aprint_normal("cpu_rng: %s\n", cpu_rng_name[cpu_rng_mode]);
	rndsource_setcb(&cpu_rng_source, cpu_rng_get, NULL);
	rnd_attach_source(&cpu_rng_source, cpu_rng_name[cpu_rng_mode],
	    RND_TYPE_RNG, RND_FLAG_COLLECT_VALUE|RND_FLAG_HASCB);
}

/* -------------------------------------------------------------------------- */

void
cpu_rng_early_sample(uint64_t *sample)
{
	static bool has_rdseed = false;
	static bool has_rdrand = false;
	static bool inited = false;
	u_int descs[4];
	size_t n;

	if (!inited) {
		if (cpuid_level >= 7) {
			x86_cpuid(0x07, descs);
			has_rdseed = (descs[1] & CPUID_SEF_RDSEED) != 0;
		}
		if (cpuid_level >= 1) {
			x86_cpuid(0x01, descs);
			has_rdrand = (descs[2] & CPUID2_RDRAND) != 0;
		}
		inited = true;
	}

	n = 0;
	if (has_rdseed && has_rdrand)
		n = cpu_rng_rdseed_rdrand(sample);
	else if (has_rdseed)
		n = cpu_rng_rdseed(sample);
	else if (has_rdrand)
		n = cpu_rng_rdrand(sample);
	if (n == 0)
		*sample = rdtsc();
}
