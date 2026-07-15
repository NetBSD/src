/*	$NetBSD: sig_fpu.c,v 1.3 2026/07/15 02:08:18 riastradh Exp $	*/

/*
 * Copyright (c) 2026 The NetBSD Foundation, Inc.
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
__RCSID("$NetBSD: sig_fpu.c,v 1.3 2026/07/15 02:08:18 riastradh Exp $");

#include "sig_fpu.h"

#include <sys/types.h>

#include <x86/cpu_extended_state.h>

#include <cpuid.h>
#include <inttypes.h>
#include <sched.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>

#include "h_macros.h"

#ifdef __x86_64__
#  define	NXMMREGS	16
#  define	NYMMREGS	16
#  define	NZMMREGS	32
#else  /* 32-bit */
#  define	NXMMREGS	8
#  define	NYMMREGS	8
#  define	NZMMREGS	8
#endif

#define	X87_CW_RC	__BITS(11,10) /* rounding control (mode) */
#define	X87_CW_PC	__BITS(9,8)   /* precision control */
#define	X87_CW_SW_EXC	__BITS(5,0)   /* exception bits */

struct cpuid {
	uint32_t edx;
	uint32_t ecx;
	uint32_t ebx;
	uint32_t eax;
};

static struct cpuid
cpuid(uint32_t eax_fn, uint32_t ecx_idx)
{
	struct cpuid c;

	if (!__get_cpuid_count(eax_fn, ecx_idx,
		&c.eax, &c.ebx, &c.ecx, &c.edx))
		memset(&c, 0, sizeof(c));

	return c;
}

__printflike(4,5)
__noinline
static void
hexdump(FILE *fp, const void *buf, size_t len, const char *title, ...)
{
	va_list va;
	const uint8_t *p = buf;
	size_t i;

	fprintf(fp, "# ");
	va_start(va, title);
	vfprintf(fp, title, va);
	va_end(va);
	fprintf(fp, "\n");
	for (i = 0; i < len; i++) {
		if ((i % 8) == 0)
			fprintf(fp, " ");
		fprintf(fp, " %02hhx", p[i]);
		if ((i % 16) == 15)
			fprintf(fp, "\n");
	}
	if (i % 16)
		fprintf(fp, "\n");
}

bool
x87_supported(void)
{
#ifdef __x86_64__
	return true;
#else  /* 32-bit */
	return cpuid(0x00000001, 0).edx & __BIT(0);
#endif
}

int
test_x87(volatile bool *ready, const volatile bool *done)
{
	struct save87 s, s1;
	uint32_t rcpc_exc;
	int error = 0;

	/*
	 * Gather the current FPU state so we have some reasonable
	 * content for the weird stuff when we restore from it.
	 */
	asm("wait\n"
	    "	fnsave	%0"
	    : /*out*/ "=m"(s));

	/*
	 * Randomize the floating-point stack entries ST(0),...,ST(7).
	 */
	arc4random_buf(s.s87_ac, sizeof(s.s87_ac));

	/*
	 * Randomize rounding control, precision control, and a set of
	 * exception bits.  We don't want to raise any unmasked
	 * exceptions, so we'll raise _and_ mask them.
	 *
	 * XXX Consider testing the condition code, exception status,
	 * and top of stack bits in the status word too.
	 */
	rcpc_exc = arc4random() & (X87_CW_RC | X87_CW_PC | X87_CW_SW_EXC);
	if (__SHIFTOUT(rcpc_exc, X87_CW_PC) == 1) /* reserved */
		rcpc_exc |= __SHIFTIN(3, X87_CW_PC); /* extended */
	s.s87_cw &= ~(X87_CW_RC | X87_CW_PC | X87_CW_SW_EXC);
	s.s87_cw |= rcpc_exc;
	s.s87_sw &= ~X87_CW_SW_EXC;
	s.s87_sw |= rcpc_exc & X87_CW_SW_EXC;

	/*
	 * Load up the registers, report that we're ready, busy-wait
	 * with the registers still live -- so no subroutine calls --
	 * until we're done, and then store the registers back to
	 * memory so we can check them.
	 */
	asm("\n"
	    "	frstor	%[s]\n"
	    "	lock\n"
	    "	orb	$1,%[ready]\n"
	    "0:	pause\n"
	    "	cmpb	$0,%[done]\n"
	    "	lfence\n"
	    "	je	0b\n"
	    "	wait\n"
	    "	fnsave	%[s1]\n"
	    : /*out*/ [ready]"=m"(*ready), [s1]"=m"(s1)
	    : /*in*/ [done]"m"(*done), [s]"m"(s));

	/*
	 * If there are any mismatches between the before and after
	 * ST(i) registers, or the control word, or the status word,
	 * print them and fail.
	 */
	if (memcmp(s.s87_ac, s1.s87_ac, sizeof(s.s87_ac)) != 0) {
		unsigned i;

		for (i = 0; i < 8; i++) {
			if (memcmp(&s.s87_ac[i], &s1.s87_ac[i],
				sizeof(s.s87_ac[i])) == 0)
				continue;
			fprintf(stderr, "ST(%u)=0x%08x%016"PRIx64","
			    " expected 0x%08x%016"PRIx64"\n",
			    i,
			    s1.s87_ac[i].f87_exp_sign,
			    s1.s87_ac[i].f87_mantissa,
			    s.s87_ac[i].f87_exp_sign,
			    s.s87_ac[i].f87_mantissa);
			error |= 1 << i;
		}
	}
	if ((s1.s87_cw & (X87_CW_RC | X87_CW_PC | X87_CW_SW_EXC)) !=
	    (s.s87_cw & (X87_CW_RC | X87_CW_PC | X87_CW_SW_EXC))) {
		fprintf(stderr, "cw=0x%04x, expected 0x%04x,"
		    " diff 0x%04x\n",
		    s1.s87_cw, s.s87_cw,
		    (uint32_t)((s1.s87_cw ^ s.s87_cw) &
			(X87_CW_RC | X87_CW_PC | X87_CW_SW_EXC)));
		error |= 1 << 8;
	}
	if ((s1.s87_sw & X87_CW_SW_EXC) !=
	    (s.s87_cw & X87_CW_SW_EXC)) {
		fprintf(stderr, "sw=0x%04x, expected 0x%04x,"
		    " diff 0x%04x\n",
		    s1.s87_sw, s.s87_sw,
		    (uint32_t)((s1.s87_sw ^ s.s87_sw) &
			X87_CW_SW_EXC));
		error |= 1 << 9;
	}

	return error;
}

void
trash_x87(void)
{
	struct save87 s;
	uint32_t rcpc_exc;

	/*
	 * Gather the current FPU state so we have some reasonable
	 * content for the weird stuff when we restore from it.
	 */
	asm("wait\n"
	    "	fnsave	%0"
	    : /*out*/ "=m"(s));

	/*
	 * Randomize the floating-point stack entries ST(0),...,ST(7).
	 */
	arc4random_buf(s.s87_ac, sizeof(s.s87_ac));

	/*
	 * Randomize rounding control, precision control, and a set of
	 * exception bits.  We don't want to raise any unmasked
	 * exceptions, so we'll raise _and_ mask them.
	 *
	 * XXX Consider trashing the condition code, exception status,
	 * and top of stack bits in the status word too.
	 */
	rcpc_exc = arc4random() & (X87_CW_RC | X87_CW_PC | X87_CW_SW_EXC);
	s.s87_cw &= ~(X87_CW_RC | X87_CW_PC | X87_CW_SW_EXC);
	s.s87_cw |= rcpc_exc;
	s.s87_sw &= ~X87_CW_SW_EXC;
	s.s87_sw |= rcpc_exc & X87_CW_SW_EXC;

	/*
	 * Load the state into the FPU.
	 */
	asm volatile("frstor %0" :: /*in*/ "m"(s));
}

#define	MXCSR_MISALIGNEDEXC	__BIT(17)	/* misaligned exc fault */
#define	MXCSR_FTZ		__BIT(15)	/* flush to zero */
#define	MXCSR_RC		__BITS(14,13)	/* rounding control */
#define	MXCSR_EXCMASK		__BITS(12,7)	/* exceptions masked */
#define	MXCSR_DAZ		__BIT(6)	/* denormals are zero */
#define	MXCSR_EXCFLAG		__BITS(5,0)	/* exceptions raised */

struct xmmregs {
	struct {
		uint8_t b[16];
	} __aligned(16)	xmm[NXMMREGS];
	uint32_t mxcsr;
};

bool
xmm_supported(void)
{
#ifdef __x86_64__
	return true;
#else  /* 32-bit */
	return cpuid(0x00000001, 0).edx & __BIT(25);
#endif
}

__attribute__((target("sse")))
int
test_xmm(volatile bool *ready, const volatile bool *done)
{
	struct xmmregs before, after;
	uint32_t exc;
	unsigned i;
	int error = 0;

	/*
	 * Randomize the xmm register content.  Randomize plausible
	 * bits for the mxcsr.
	 */
	arc4random_buf(before.xmm, sizeof(before.xmm));
	before.mxcsr = arc4random() & (MXCSR_FTZ | MXCSR_RC | MXCSR_DAZ);
	exc = arc4random() & 0x3f;
	before.mxcsr |= __SHIFTIN(exc, MXCSR_EXCMASK);
	before.mxcsr |= __SHIFTIN(exc, MXCSR_EXCFLAG);

	/*
	 * Load up the registers, report that we're ready, busy-wait
	 * with the registers still live -- so no subroutine calls --
	 * until we're done, and then store the registers back to
	 * memory so we can check them.
	 */
	asm("\n"
	    "	ldmxcsr	%[mxcsr_before]\n"
	    "	movdqa	0*16(%[xmm_before]),%%xmm0\n"
	    "	movdqa	1*16(%[xmm_before]),%%xmm1\n"
	    "	movdqa	2*16(%[xmm_before]),%%xmm2\n"
	    "	movdqa	3*16(%[xmm_before]),%%xmm3\n"
	    "	movdqa	4*16(%[xmm_before]),%%xmm4\n"
	    "	movdqa	5*16(%[xmm_before]),%%xmm5\n"
	    "	movdqa	6*16(%[xmm_before]),%%xmm6\n"
	    "	movdqa	7*16(%[xmm_before]),%%xmm7\n"
#ifdef __x86_64__
	    "	movdqa	8*16(%[xmm_before]),%%xmm8\n"
	    "	movdqa	9*16(%[xmm_before]),%%xmm9\n"
	    "	movdqa	10*16(%[xmm_before]),%%xmm10\n"
	    "	movdqa	11*16(%[xmm_before]),%%xmm11\n"
	    "	movdqa	12*16(%[xmm_before]),%%xmm12\n"
	    "	movdqa	13*16(%[xmm_before]),%%xmm13\n"
	    "	movdqa	14*16(%[xmm_before]),%%xmm14\n"
	    "	movdqa	15*16(%[xmm_before]),%%xmm15\n"
#endif
	    "	lock\n"
	    "	orb	$1,%[ready]\n"
	    "0:	pause\n"
	    "	cmpb	$0,%[done]\n"
	    "	lfence\n"
	    "	je	0b\n"
	    "	movdqa	%%xmm0,0*16(%[xmm_after])\n"
	    "	movdqa	%%xmm1,1*16(%[xmm_after])\n"
	    "	movdqa	%%xmm2,2*16(%[xmm_after])\n"
	    "	movdqa	%%xmm3,3*16(%[xmm_after])\n"
	    "	movdqa	%%xmm4,4*16(%[xmm_after])\n"
	    "	movdqa	%%xmm5,5*16(%[xmm_after])\n"
	    "	movdqa	%%xmm6,6*16(%[xmm_after])\n"
	    "	movdqa	%%xmm7,7*16(%[xmm_after])\n"
#ifdef __x86_64__
	    "	movdqa	%%xmm8,8*16(%[xmm_after])\n"
	    "	movdqa	%%xmm9,9*16(%[xmm_after])\n"
	    "	movdqa	%%xmm10,10*16(%[xmm_after])\n"
	    "	movdqa	%%xmm11,11*16(%[xmm_after])\n"
	    "	movdqa	%%xmm12,12*16(%[xmm_after])\n"
	    "	movdqa	%%xmm13,13*16(%[xmm_after])\n"
	    "	movdqa	%%xmm14,14*16(%[xmm_after])\n"
	    "	movdqa	%%xmm15,15*16(%[xmm_after])\n"
#endif
	    "	stmxcsr	%[mxcsr_after]\n"
	    : /*out*/ [ready]"=m"(*ready), "=m"(after.xmm),
	      [mxcsr_after]"=m"(after.mxcsr)
	    : /*in*/ [done]"m"(*done), "m"(before.xmm),
	      [mxcsr_before]"m"(before.mxcsr),
	      [xmm_before]"r"(&before.xmm), [xmm_after]"r"(&after.xmm)
	    : /*clobber*/ "xmm0", "xmm1", "xmm2", "xmm3", "xmm4", "xmm5",
	      "xmm6", "xmm7"
#ifdef __x86_64__
	    , "xmm8", "xmm9", "xmm10", "xmm11", "xmm12",
	      "xmm13", "xmm14", "xmm15"
#endif
	);

	/*
	 * If there are any mismatches between the before and after xmm
	 * registers, or the mxcsr, print them and fail.
	 */
	for (i = 0; i < __arraycount(before.xmm); i++) {
		if (memcmp(&before.xmm[i], &after.xmm[i],
			sizeof(before.xmm[i]))) {
			fprintf(stderr, "xmm%u clobbered\n", i);
			hexdump(stderr, &before.xmm[i], sizeof(before.xmm[i]),
			    "before");
			hexdump(stderr, &after.xmm[i], sizeof(after.xmm[i]),
			    "after");
			error |= 1 << i;
		}
	}

	if (before.mxcsr != after.mxcsr) {
		fprintf(stderr, "mxcsr clobbered:"
		    " before=0x%08"PRIx32", after=0x%08"PRIx32"\n",
		    before.mxcsr, after.mxcsr);
		error |= 1 << 16;
	}

	return error;
}

__attribute__((target("sse")))
void
trash_xmm(void)
{
	struct xmmregs regs;

	arc4random_buf(&regs, sizeof(regs));

	asm("\n"
	    "	movdqa	0*16(%[xmm]),%%xmm0\n"
	    "	movdqa	1*16(%[xmm]),%%xmm1\n"
	    "	movdqa	2*16(%[xmm]),%%xmm2\n"
	    "	movdqa	3*16(%[xmm]),%%xmm3\n"
	    "	movdqa	4*16(%[xmm]),%%xmm4\n"
	    "	movdqa	5*16(%[xmm]),%%xmm5\n"
	    "	movdqa	6*16(%[xmm]),%%xmm6\n"
	    "	movdqa	7*16(%[xmm]),%%xmm7\n"
#ifdef __x86_64__
	    "	movdqa	8*16(%[xmm]),%%xmm8\n"
	    "	movdqa	9*16(%[xmm]),%%xmm9\n"
	    "	movdqa	10*16(%[xmm]),%%xmm10\n"
	    "	movdqa	11*16(%[xmm]),%%xmm11\n"
	    "	movdqa	12*16(%[xmm]),%%xmm12\n"
	    "	movdqa	13*16(%[xmm]),%%xmm13\n"
	    "	movdqa	14*16(%[xmm]),%%xmm14\n"
	    "	movdqa	15*16(%[xmm]),%%xmm15\n"
#endif
	    : /*out*/
	    : /*in*/ [xmm]"r"(regs.xmm), "m"(regs.xmm)
	    : /*clobber*/ "xmm0", "xmm1", "xmm2", "xmm3", "xmm4", "xmm5",
	      "xmm6", "xmm7"
#ifdef __x86_64__
	    , "xmm8", "xmm9", "xmm10", "xmm11", "xmm12",
	      "xmm13", "xmm14", "xmm15"
#endif
	);
}

bool
ymm_supported(void)
{
	return cpuid(0x00000001, 0).ecx & __BIT(28);
}

struct ymmregs {
	struct {
		uint8_t b[32];
	} __aligned(32)	ymm[NYMMREGS];
};

__attribute__((target("avx")))
int
test_ymm(volatile bool *ready, const volatile bool *done)
{
	struct ymmregs before, after;
	unsigned i;
	int error = 0;

	/*
	 * Randomize the ymm register content.
	 */
	arc4random_buf(&before, sizeof(before));

	/*
	 * Load up the registers, report that we're ready, busy-wait
	 * with the registers still live -- so no subroutine calls --
	 * until we're done, and then store the registers back to
	 * memory so we can check them.
	 */
	asm("\n"
	    "	vmovdqa	0*32(%[before_ymm]),%%ymm0\n"
	    "	vmovdqa	1*32(%[before_ymm]),%%ymm1\n"
	    "	vmovdqa	2*32(%[before_ymm]),%%ymm2\n"
	    "	vmovdqa	3*32(%[before_ymm]),%%ymm3\n"
	    "	vmovdqa	4*32(%[before_ymm]),%%ymm4\n"
	    "	vmovdqa	5*32(%[before_ymm]),%%ymm5\n"
	    "	vmovdqa	6*32(%[before_ymm]),%%ymm6\n"
	    "	vmovdqa	7*32(%[before_ymm]),%%ymm7\n"
#ifdef __x86_64__
	    "	vmovdqa	8*32(%[before_ymm]),%%ymm8\n"
	    "	vmovdqa	9*32(%[before_ymm]),%%ymm9\n"
	    "	vmovdqa	10*32(%[before_ymm]),%%ymm10\n"
	    "	vmovdqa	11*32(%[before_ymm]),%%ymm11\n"
	    "	vmovdqa	12*32(%[before_ymm]),%%ymm12\n"
	    "	vmovdqa	13*32(%[before_ymm]),%%ymm13\n"
	    "	vmovdqa	14*32(%[before_ymm]),%%ymm14\n"
	    "	vmovdqa	15*32(%[before_ymm]),%%ymm15\n"
#endif
	    "	lock\n"
	    "	orb	$1,%[ready]\n"
	    "0:	pause\n"
	    "	cmpb	$0,%[done]\n"
	    "	lfence\n"
	    "	je	0b\n"
	    "	vmovdqa	%%ymm0,0*32(%[after_ymm])\n"
	    "	vmovdqa	%%ymm1,1*32(%[after_ymm])\n"
	    "	vmovdqa	%%ymm2,2*32(%[after_ymm])\n"
	    "	vmovdqa	%%ymm3,3*32(%[after_ymm])\n"
	    "	vmovdqa	%%ymm4,4*32(%[after_ymm])\n"
	    "	vmovdqa	%%ymm5,5*32(%[after_ymm])\n"
	    "	vmovdqa	%%ymm6,6*32(%[after_ymm])\n"
	    "	vmovdqa	%%ymm7,7*32(%[after_ymm])\n"
#ifdef __x86_64__
	    "	vmovdqa	%%ymm8,8*32(%[after_ymm])\n"
	    "	vmovdqa	%%ymm9,9*32(%[after_ymm])\n"
	    "	vmovdqa	%%ymm10,10*32(%[after_ymm])\n"
	    "	vmovdqa	%%ymm11,11*32(%[after_ymm])\n"
	    "	vmovdqa	%%ymm12,12*32(%[after_ymm])\n"
	    "	vmovdqa	%%ymm13,13*32(%[after_ymm])\n"
	    "	vmovdqa	%%ymm14,14*32(%[after_ymm])\n"
	    "	vmovdqa	%%ymm15,15*32(%[after_ymm])\n"
#endif
	    : /*out*/ [ready]"=m"(*ready), "=m"(after.ymm)
	    : /*in*/ [done]"m"(*done), "m"(before.ymm),
	      [before_ymm]"r"(before.ymm), [after_ymm]"r"(after.ymm)
	    : /*clobber*/ "ymm0", "ymm1", "ymm2", "ymm3", "ymm4", "ymm5",
	      "ymm6", "ymm7"
#ifdef __x86_64__
	    , "ymm8", "ymm9", "ymm10", "ymm11", "ymm12",
	      "ymm13", "ymm14", "ymm15"
#endif
	);

	for (i = 0; i < __arraycount(before.ymm); i++) {
		if (memcmp(&before.ymm[i], &after.ymm[i],
			sizeof(before.ymm[i]))) {
			fprintf(stderr, "ymm%u clobbered\n", i);
			hexdump(stderr, &before.ymm[i], sizeof(before.ymm[i]),
			    "before");
			hexdump(stderr, &after.ymm[i], sizeof(after.ymm[i]),
			    "after");
			error |= 1 << i;
		}
	}

	return error;
}

__attribute__((target("avx")))
void
trash_ymm(void)
{
	struct ymmregs regs;

	arc4random_buf(&regs, sizeof(regs));

	asm("\n"
	    "	vmovdqa	0*32(%[ymm]),%%ymm0\n"
	    "	vmovdqa	1*32(%[ymm]),%%ymm1\n"
	    "	vmovdqa	2*32(%[ymm]),%%ymm2\n"
	    "	vmovdqa	3*32(%[ymm]),%%ymm3\n"
	    "	vmovdqa	4*32(%[ymm]),%%ymm4\n"
	    "	vmovdqa	5*32(%[ymm]),%%ymm5\n"
	    "	vmovdqa	6*32(%[ymm]),%%ymm6\n"
	    "	vmovdqa	7*32(%[ymm]),%%ymm7\n"
#ifdef __x86_64__
	    "	vmovdqa	8*32(%[ymm]),%%ymm8\n"
	    "	vmovdqa	9*32(%[ymm]),%%ymm9\n"
	    "	vmovdqa	10*32(%[ymm]),%%ymm10\n"
	    "	vmovdqa	11*32(%[ymm]),%%ymm11\n"
	    "	vmovdqa	12*32(%[ymm]),%%ymm12\n"
	    "	vmovdqa	13*32(%[ymm]),%%ymm13\n"
	    "	vmovdqa	14*32(%[ymm]),%%ymm14\n"
	    "	vmovdqa	15*32(%[ymm]),%%ymm15\n"
#endif
	    : /*out*/
	    : /*in*/ "m"(regs.ymm), [ymm]"r"(regs.ymm)
	    : /*clobber*/ "ymm0", "ymm1", "ymm2", "ymm3", "ymm4", "ymm5",
	      "ymm6", "ymm7"
#ifdef __x86_64__
	    , "ymm8", "ymm9", "ymm10", "ymm11", "ymm12",
	      "ymm13", "ymm14", "ymm15"
#endif
	);
}

bool
zmm_supported(void)
{
	return cpuid(0x00000007, 0).ebx & __BIT(16);
}

struct zmmregs {
	struct {
		uint8_t b[64];
	} __aligned(64)	zmm[NZMMREGS];
};

__attribute__((target("avx512f")))
int
test_zmm(volatile bool *ready, const volatile bool *done)
{
	struct zmmregs before, after;
	unsigned i;
	int error = 0;

	/*
	 * Randomize the zmm register content.
	 */
	arc4random_buf(&before, sizeof(before));

	/*
	 * Load up the registers, report that we're ready, busy-wait
	 * with the registers still live -- so no subroutine calls --
	 * until we're done, and then store the registers back to
	 * memory so we can check them.
	 */
	asm("\n"
	    "	vmovdqa64	0*64(%[before_zmm]),%%zmm0\n"
	    "	vmovdqa64	1*64(%[before_zmm]),%%zmm1\n"
	    "	vmovdqa64	2*64(%[before_zmm]),%%zmm2\n"
	    "	vmovdqa64	3*64(%[before_zmm]),%%zmm3\n"
	    "	vmovdqa64	4*64(%[before_zmm]),%%zmm4\n"
	    "	vmovdqa64	5*64(%[before_zmm]),%%zmm5\n"
	    "	vmovdqa64	6*64(%[before_zmm]),%%zmm6\n"
	    "	vmovdqa64	7*64(%[before_zmm]),%%zmm7\n"
#ifdef __x86_64__
	    "	vmovdqa64	8*64(%[before_zmm]),%%zmm8\n"
	    "	vmovdqa64	9*64(%[before_zmm]),%%zmm9\n"
	    "	vmovdqa64	10*64(%[before_zmm]),%%zmm10\n"
	    "	vmovdqa64	11*64(%[before_zmm]),%%zmm11\n"
	    "	vmovdqa64	12*64(%[before_zmm]),%%zmm12\n"
	    "	vmovdqa64	13*64(%[before_zmm]),%%zmm13\n"
	    "	vmovdqa64	14*64(%[before_zmm]),%%zmm14\n"
	    "	vmovdqa64	15*64(%[before_zmm]),%%zmm15\n"
	    "	vmovdqa64	16*64(%[before_zmm]),%%zmm16\n"
	    "	vmovdqa64	17*64(%[before_zmm]),%%zmm17\n"
	    "	vmovdqa64	18*64(%[before_zmm]),%%zmm18\n"
	    "	vmovdqa64	19*64(%[before_zmm]),%%zmm19\n"
	    "	vmovdqa64	20*64(%[before_zmm]),%%zmm20\n"
	    "	vmovdqa64	21*64(%[before_zmm]),%%zmm21\n"
	    "	vmovdqa64	22*64(%[before_zmm]),%%zmm22\n"
	    "	vmovdqa64	23*64(%[before_zmm]),%%zmm23\n"
	    "	vmovdqa64	24*64(%[before_zmm]),%%zmm24\n"
	    "	vmovdqa64	25*64(%[before_zmm]),%%zmm25\n"
	    "	vmovdqa64	26*64(%[before_zmm]),%%zmm26\n"
	    "	vmovdqa64	27*64(%[before_zmm]),%%zmm27\n"
	    "	vmovdqa64	28*64(%[before_zmm]),%%zmm28\n"
	    "	vmovdqa64	29*64(%[before_zmm]),%%zmm29\n"
	    "	vmovdqa64	30*64(%[before_zmm]),%%zmm30\n"
	    "	vmovdqa64	31*64(%[before_zmm]),%%zmm31\n"
#endif
	    "	lock\n"
	    "	orb	$1,%[ready]\n"
	    "0:	pause\n"
	    "	cmpb	$0,%[done]\n"
	    "	lfence\n"
	    "	je	0b\n"
	    "	vmovdqa64	%%zmm0,0*64(%[after_zmm])\n"
	    "	vmovdqa64	%%zmm1,1*64(%[after_zmm])\n"
	    "	vmovdqa64	%%zmm2,2*64(%[after_zmm])\n"
	    "	vmovdqa64	%%zmm3,3*64(%[after_zmm])\n"
	    "	vmovdqa64	%%zmm4,4*64(%[after_zmm])\n"
	    "	vmovdqa64	%%zmm5,5*64(%[after_zmm])\n"
	    "	vmovdqa64	%%zmm6,6*64(%[after_zmm])\n"
	    "	vmovdqa64	%%zmm7,7*64(%[after_zmm])\n"
#ifdef __x86_64__
	    "	vmovdqa64	%%zmm8,8*64(%[after_zmm])\n"
	    "	vmovdqa64	%%zmm9,9*64(%[after_zmm])\n"
	    "	vmovdqa64	%%zmm10,10*64(%[after_zmm])\n"
	    "	vmovdqa64	%%zmm11,11*64(%[after_zmm])\n"
	    "	vmovdqa64	%%zmm12,12*64(%[after_zmm])\n"
	    "	vmovdqa64	%%zmm13,13*64(%[after_zmm])\n"
	    "	vmovdqa64	%%zmm14,14*64(%[after_zmm])\n"
	    "	vmovdqa64	%%zmm15,15*64(%[after_zmm])\n"
	    "	vmovdqa64	%%zmm16,16*64(%[after_zmm])\n"
	    "	vmovdqa64	%%zmm17,17*64(%[after_zmm])\n"
	    "	vmovdqa64	%%zmm18,18*64(%[after_zmm])\n"
	    "	vmovdqa64	%%zmm19,19*64(%[after_zmm])\n"
	    "	vmovdqa64	%%zmm20,20*64(%[after_zmm])\n"
	    "	vmovdqa64	%%zmm21,21*64(%[after_zmm])\n"
	    "	vmovdqa64	%%zmm22,22*64(%[after_zmm])\n"
	    "	vmovdqa64	%%zmm23,23*64(%[after_zmm])\n"
	    "	vmovdqa64	%%zmm24,24*64(%[after_zmm])\n"
	    "	vmovdqa64	%%zmm25,25*64(%[after_zmm])\n"
	    "	vmovdqa64	%%zmm26,26*64(%[after_zmm])\n"
	    "	vmovdqa64	%%zmm27,27*64(%[after_zmm])\n"
	    "	vmovdqa64	%%zmm28,28*64(%[after_zmm])\n"
	    "	vmovdqa64	%%zmm29,29*64(%[after_zmm])\n"
	    "	vmovdqa64	%%zmm30,30*64(%[after_zmm])\n"
	    "	vmovdqa64	%%zmm31,31*64(%[after_zmm])\n"
#endif
	    : /*out*/ [ready]"=m"(*ready), "=m"(after.zmm)
	    : /*in*/ [done]"m"(*done), "m"(before.zmm),
	      [before_zmm]"r"(before.zmm), [after_zmm]"r"(after.zmm)
	    : /*clobber*/ "zmm0", "zmm1", "zmm2", "zmm3", "zmm4", "zmm5",
	      "zmm6", "zmm7"
#ifdef __x86_64__
	    , "zmm8", "zmm9", "zmm10", "zmm11", "zmm12",
	      "zmm13", "zmm14", "zmm15", "zmm16", "zmm17",
	      "zmm18", "zmm19", "zmm20", "zmm21", "zmm22",
	      "zmm23", "zmm24", "zmm25", "zmm26", "zmm27",
	      "zmm28", "zmm29", "zmm30", "zmm31"
#endif
	);

	for (i = 0; i < __arraycount(before.zmm); i++) {
		if (memcmp(&before.zmm[i], &after.zmm[i],
			sizeof(before.zmm[i]))) {
			fprintf(stderr, "zmm%u clobbered\n", i);
			hexdump(stderr, &before.zmm[i], sizeof(before.zmm[i]),
			    "before");
			hexdump(stderr, &after.zmm[i], sizeof(after.zmm[i]),
			    "after");
			error |= 1 << i;
		}
	}

	return error;
}

__attribute__((target("avx512f")))
void
trash_zmm(void)
{
	struct zmmregs regs;

	arc4random_buf(&regs, sizeof(regs));

	asm("\n"
	    "	vmovdqa64	0*64(%[zmm]),%%zmm0\n"
	    "	vmovdqa64	1*64(%[zmm]),%%zmm1\n"
	    "	vmovdqa64	2*64(%[zmm]),%%zmm2\n"
	    "	vmovdqa64	3*64(%[zmm]),%%zmm3\n"
	    "	vmovdqa64	4*64(%[zmm]),%%zmm4\n"
	    "	vmovdqa64	5*64(%[zmm]),%%zmm5\n"
	    "	vmovdqa64	6*64(%[zmm]),%%zmm6\n"
	    "	vmovdqa64	7*64(%[zmm]),%%zmm7\n"
#ifdef __x86_64__
	    "	vmovdqa64	8*64(%[zmm]),%%zmm8\n"
	    "	vmovdqa64	9*64(%[zmm]),%%zmm9\n"
	    "	vmovdqa64	10*64(%[zmm]),%%zmm10\n"
	    "	vmovdqa64	11*64(%[zmm]),%%zmm11\n"
	    "	vmovdqa64	12*64(%[zmm]),%%zmm12\n"
	    "	vmovdqa64	13*64(%[zmm]),%%zmm13\n"
	    "	vmovdqa64	14*64(%[zmm]),%%zmm14\n"
	    "	vmovdqa64	15*64(%[zmm]),%%zmm15\n"
	    "	vmovdqa64	16*64(%[zmm]),%%zmm16\n"
	    "	vmovdqa64	17*64(%[zmm]),%%zmm17\n"
	    "	vmovdqa64	18*64(%[zmm]),%%zmm18\n"
	    "	vmovdqa64	19*64(%[zmm]),%%zmm19\n"
	    "	vmovdqa64	20*64(%[zmm]),%%zmm20\n"
	    "	vmovdqa64	21*64(%[zmm]),%%zmm21\n"
	    "	vmovdqa64	22*64(%[zmm]),%%zmm22\n"
	    "	vmovdqa64	23*64(%[zmm]),%%zmm23\n"
	    "	vmovdqa64	24*64(%[zmm]),%%zmm24\n"
	    "	vmovdqa64	25*64(%[zmm]),%%zmm25\n"
	    "	vmovdqa64	26*64(%[zmm]),%%zmm26\n"
	    "	vmovdqa64	27*64(%[zmm]),%%zmm27\n"
	    "	vmovdqa64	28*64(%[zmm]),%%zmm28\n"
	    "	vmovdqa64	29*64(%[zmm]),%%zmm29\n"
	    "	vmovdqa64	30*64(%[zmm]),%%zmm30\n"
	    "	vmovdqa64	31*64(%[zmm]),%%zmm31\n"
#endif
	    : /*out*/
	    : /*in*/ "m"(regs.zmm), [zmm]"r"(regs.zmm)
	    : /*clobber*/ "zmm0", "zmm1", "zmm2", "zmm3", "zmm4", "zmm5",
	      "zmm6", "zmm7"
#ifdef __x86_64__
	    , "zmm8", "zmm9", "zmm10", "zmm11", "zmm12",
	      "zmm13", "zmm14", "zmm15", "zmm16", "zmm17",
	      "zmm18", "zmm19", "zmm20", "zmm21", "zmm22",
	      "zmm23", "zmm24", "zmm25", "zmm26", "zmm27",
	      "zmm28", "zmm29", "zmm30", "zmm31"
#endif
	);
}
