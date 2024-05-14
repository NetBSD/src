/* $NetBSD: t_fenv.c,v 1.18 2024/05/14 14:55:44 riastradh Exp $ */

/*-
 * Copyright (c) 2014 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Martin Husemann.
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
__RCSID("$NetBSD: t_fenv.c,v 1.18 2024/05/14 14:55:44 riastradh Exp $");

#include <atf-c.h>

#include <fenv.h>
#ifdef __HAVE_FENV

/* XXXGCC gcc lacks #pragma STDC FENV_ACCESS */
/* XXXclang clang lacks #pragma STDC FENV_ACCESS on some ports */
#if !defined(__GNUC__)
#pragma STDC FENV_ACCESS ON
#endif

#include <float.h>
#include <ieeefp.h>
#include <stdlib.h>

#if FLT_RADIX != 2
#error This test assumes binary floating-point arithmetic.
#endif

#if (__arm__ && !__SOFTFP__) || __aarch64__
	/*
	 * Some NEON fpus do not trap on IEEE 754 FP exceptions.
	 * Skip these tests if running on them and compiled for
	 * hard float.
	 */
#define	FPU_EXC_PREREQ() do						      \
{									      \
	if (0 == fpsetmask(fpsetmask(FP_X_INV)))			      \
		atf_tc_skip("FPU does not implement traps on FP exceptions"); \
} while (0)

	/*
	 * Same as above: some don't allow configuring the rounding mode.
	 */
#define	FPU_RND_PREREQ() do						      \
{									      \
	if (0 == fpsetround(fpsetround(FP_RZ)))				      \
		atf_tc_skip("FPU does not implement configurable "	      \
		    "rounding modes");					      \
} while (0)
#endif

#ifdef __riscv__
#define	FPU_EXC_PREREQ()						      \
	atf_tc_skip("RISC-V does not support floating-point exception traps")
#endif

#ifndef FPU_EXC_PREREQ
#define	FPU_EXC_PREREQ()	__nothing
#endif
#ifndef FPU_RND_PREREQ
#define	FPU_RND_PREREQ()	__nothing
#endif


static int
feround_to_fltrounds(int feround)
{

	/*
	 * C99, Sec. 5.2.4.2.2 Characteristics of floating types
	 * <float.h>, p. 24, clause 7
	 */
	switch (feround) {
	case FE_TOWARDZERO:
		return 0;
	case FE_TONEAREST:
		return 1;
	case FE_UPWARD:
		return 2;
	case FE_DOWNWARD:
		return 3;
	default:
		return -1;
	}
}

static void
checkfltrounds(void)
{
	int feround = fegetround();
	int expected = feround_to_fltrounds(feround);

	ATF_CHECK_EQ_MSG(FLT_ROUNDS, expected,
	    "FLT_ROUNDS=%d expected=%d fegetround()=%d",
	    FLT_ROUNDS, expected, feround);
}

static void
checkrounding(int feround, const char *name)
{
	volatile double ulp1 = DBL_EPSILON;

	/*
	 * XXX These must be volatile to force rounding to double when
	 * the intermediate quantities are evaluated in long double
	 * precision, e.g. on 32-bit x86 with x87 long double.  Under
	 * the C standard (C99, C11, C17, &c.), cast and assignment
	 * operators are required to remove all extra range and
	 * precision, i.e., round double to long double.  But we build
	 * this code with -std=gnu99, which diverges from this
	 * requirement -- unless you add a volatile qualifier.
	 */
	volatile double y1 = -1 + ulp1/4;
	volatile double y2 = 1 + 3*(ulp1/2);

	double z1, z2;

	switch (feround) {
	case FE_TONEAREST:
		z1 = -1;
		z2 = 1 + 2*ulp1;
		break;
	case FE_TOWARDZERO:
		z1 = -1 + ulp1/2;
		z2 = 1 + ulp1;
		break;
	case FE_UPWARD:
		z1 = -1 + ulp1/2;
		z2 = 1 + 2*ulp1;
		break;
	case FE_DOWNWARD:
		z1 = -1;
		z2 = 1 + ulp1;
		break;
	default:
		atf_tc_fail("unknown rounding mode %d (%s)", feround, name);
	}

	ATF_CHECK_EQ_MSG(y1, z1, "%s[-1 + ulp(1)/4] expected=%a actual=%a",
	    name, y1, z1);
	ATF_CHECK_EQ_MSG(y2, z2, "%s[1 + 3*(ulp(1)/2)] expected=%a actual=%a",
	    name, y2, z2);
}

ATF_TC(fegetround);

ATF_TC_HEAD(fegetround, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "verify the fegetround() function agrees with the legacy "
	    "fpsetround");
}

ATF_TC_BODY(fegetround, tc)
{
	FPU_RND_PREREQ();

	checkrounding(FE_TONEAREST, "FE_TONEAREST");

	fpsetround(FP_RZ);
	ATF_CHECK_EQ_MSG(fegetround(), FE_TOWARDZERO,
	    "fegetround()=%d FE_TOWARDZERO=%d",
	    fegetround(), FE_TOWARDZERO);
	checkfltrounds();
	checkrounding(FE_TOWARDZERO, "FE_TOWARDZERO");

	fpsetround(FP_RM);
	ATF_CHECK_EQ_MSG(fegetround(), FE_DOWNWARD,
	    "fegetround()=%d FE_DOWNWARD=%d",
	    fegetround(), FE_DOWNWARD);
	checkfltrounds();
	checkrounding(FE_DOWNWARD, "FE_DOWNWARD");

	fpsetround(FP_RN);
	ATF_CHECK_EQ_MSG(fegetround(), FE_TONEAREST,
	    "fegetround()=%d FE_TONEAREST=%d",
	    fegetround(), FE_TONEAREST);
	checkfltrounds();
	checkrounding(FE_TONEAREST, "FE_TONEAREST");

	fpsetround(FP_RP);
	ATF_CHECK_EQ_MSG(fegetround(), FE_UPWARD,
	    "fegetround()=%d FE_UPWARD=%d",
	    fegetround(), FE_UPWARD);
	checkfltrounds();
	checkrounding(FE_UPWARD, "FE_UPWARD");
}

ATF_TC(fesetround);

ATF_TC_HEAD(fesetround, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "verify the fesetround() function agrees with the legacy "
	    "fpgetround");
}

ATF_TC_BODY(fesetround, tc)
{
	FPU_RND_PREREQ();

	checkrounding(FE_TONEAREST, "FE_TONEAREST");

	fesetround(FE_TOWARDZERO);
	ATF_CHECK_EQ_MSG(fpgetround(), FP_RZ,
	    "fpgetround()=%d FP_RZ=%d",
	    (int)fpgetround(), (int)FP_RZ);
	checkfltrounds();
	checkrounding(FE_TOWARDZERO, "FE_TOWARDZERO");

	fesetround(FE_DOWNWARD);
	ATF_CHECK_EQ_MSG(fpgetround(), FP_RM,
	    "fpgetround()=%d FP_RM=%d",
	    (int)fpgetround(), (int)FP_RM);
	checkfltrounds();
	checkrounding(FE_DOWNWARD, "FE_DOWNWARD");

	fesetround(FE_TONEAREST);
	ATF_CHECK_EQ_MSG(fpgetround(), FP_RN,
	    "fpgetround()=%d FP_RN=%d",
	    (int)fpgetround(), (int)FP_RN);
	checkfltrounds();
	checkrounding(FE_TONEAREST, "FE_TONEAREST");

	fesetround(FE_UPWARD);
	ATF_CHECK_EQ_MSG(fpgetround(), FP_RP,
	    "fpgetround()=%d FP_RP=%d",
	    (int)fpgetround(), (int)FP_RP);
	checkfltrounds();
	checkrounding(FE_UPWARD, "FE_UPWARD");
}

ATF_TC(fegetexcept);

ATF_TC_HEAD(fegetexcept, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "verify the fegetexcept() function agrees with the legacy "
	    "fpsetmask()");
}

ATF_TC_BODY(fegetexcept, tc)
{
	FPU_EXC_PREREQ();

	fpsetmask(0);
	ATF_CHECK_EQ_MSG(fegetexcept(), 0,
	    "fegetexcept()=%d",
	    fegetexcept());

	fpsetmask(FP_X_INV|FP_X_DZ|FP_X_OFL|FP_X_UFL|FP_X_IMP);
	ATF_CHECK(fegetexcept() == (FE_INVALID|FE_DIVBYZERO|FE_OVERFLOW
	    |FE_UNDERFLOW|FE_INEXACT));

	fpsetmask(FP_X_INV);
	ATF_CHECK_EQ_MSG(fegetexcept(), FE_INVALID,
	    "fegetexcept()=%d FE_INVALID=%d",
	    fegetexcept(), FE_INVALID);

	fpsetmask(FP_X_DZ);
	ATF_CHECK_EQ_MSG(fegetexcept(), FE_DIVBYZERO,
	    "fegetexcept()=%d FE_DIVBYZERO=%d",
	    fegetexcept(), FE_DIVBYZERO);

	fpsetmask(FP_X_OFL);
	ATF_CHECK_EQ_MSG(fegetexcept(), FE_OVERFLOW,
	    "fegetexcept()=%d FE_OVERFLOW=%d",
	    fegetexcept(), FE_OVERFLOW);

	fpsetmask(FP_X_UFL);
	ATF_CHECK_EQ_MSG(fegetexcept(), FE_UNDERFLOW,
	    "fegetexcept()=%d FE_UNDERFLOW=%d",
	    fegetexcept(), FE_UNDERFLOW);

	fpsetmask(FP_X_IMP);
	ATF_CHECK_EQ_MSG(fegetexcept(), FE_INEXACT,
	    "fegetexcept()=%d FE_INEXACT=%d",
	    fegetexcept(), FE_INEXACT);
}

ATF_TC(feenableexcept);

ATF_TC_HEAD(feenableexcept, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "verify the feenableexcept() function agrees with the legacy "
	    "fpgetmask()");
}

ATF_TC_BODY(feenableexcept, tc)
{
	FPU_EXC_PREREQ();

	fedisableexcept(FE_ALL_EXCEPT);
	ATF_CHECK_EQ_MSG(fpgetmask(), 0,
	    "fpgetmask()=%d",
	    (int)fpgetmask());

	feenableexcept(FE_UNDERFLOW);
	ATF_CHECK_EQ_MSG(fpgetmask(), FP_X_UFL,
	    "fpgetmask()=%d FP_X_UFL=%d",
	    (int)fpgetmask(), (int)FP_X_UFL);

	fedisableexcept(FE_ALL_EXCEPT);
	feenableexcept(FE_OVERFLOW);
	ATF_CHECK_EQ_MSG(fpgetmask(), FP_X_OFL,
	    "fpgetmask()=%d FP_X_OFL=%d",
	    (int)fpgetmask(), (int)FP_X_OFL);

	fedisableexcept(FE_ALL_EXCEPT);
	feenableexcept(FE_DIVBYZERO);
	ATF_CHECK_EQ_MSG(fpgetmask(), FP_X_DZ,
	    "fpgetmask()=%d FP_X_DZ=%d",
	    (int)fpgetmask(), (int)FP_X_DZ);

	fedisableexcept(FE_ALL_EXCEPT);
	feenableexcept(FE_INEXACT);
	ATF_CHECK_EQ_MSG(fpgetmask(), FP_X_IMP,
	    "fpgetmask()=%d FP_X_IMP=%d",
	    (int)fpgetmask(), (int)FP_X_IMP);

	fedisableexcept(FE_ALL_EXCEPT);
	feenableexcept(FE_INVALID);
	ATF_CHECK_EQ_MSG(fpgetmask(), FP_X_INV,
	    "fpgetmask()=%d FP_X_INV=%d",
	    (int)fpgetmask(), (int)FP_X_INV);
}

/*
 * Temporary workaround for PR 58253: powerpc has more fenv exception
 * bits than it can handle.
 */
#if defined __powerpc__
#define	FE_TRAP_EXCEPT							      \
	(FE_DIVBYZERO|FE_INEXACT|FE_INVALID|FE_OVERFLOW|FE_UNDERFLOW)
#else
#define	FE_TRAP_EXCEPT	FE_ALL_EXCEPT
#endif

ATF_TC(fetestexcept_trap);
ATF_TC_HEAD(fetestexcept_trap, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Verify fetestexcept doesn't affect the trapped excpetions");
}
ATF_TC_BODY(fetestexcept_trap, tc)
{
	int except;

	FPU_EXC_PREREQ();

	fedisableexcept(FE_TRAP_EXCEPT);
	ATF_CHECK_EQ_MSG((except = fegetexcept()), 0,
	    "fegetexcept()=0x%x", except);

	(void)fetestexcept(FE_TRAP_EXCEPT);
	ATF_CHECK_EQ_MSG((except = fegetexcept()), 0,
	    "fegetexcept()=0x%x", except);

	feenableexcept(FE_TRAP_EXCEPT);
	ATF_CHECK_EQ_MSG((except = fegetexcept()), FE_TRAP_EXCEPT,
	    "fegetexcept()=0x%x FE_TRAP_EXCEPT=0x%x", except, FE_TRAP_EXCEPT);

	(void)fetestexcept(FE_TRAP_EXCEPT);
	ATF_CHECK_EQ_MSG((except = fegetexcept()), FE_TRAP_EXCEPT,
	    "fegetexcept()=0x%x FE_ALL_EXCEPT=0x%x", except, FE_TRAP_EXCEPT);
}

ATF_TP_ADD_TCS(tp)
{
	ATF_TP_ADD_TC(tp, fegetround);
	ATF_TP_ADD_TC(tp, fesetround);
	ATF_TP_ADD_TC(tp, fegetexcept);
	ATF_TP_ADD_TC(tp, feenableexcept);
	ATF_TP_ADD_TC(tp, fetestexcept_trap);

	return atf_no_error();
}

#else	/* no fenv.h support */

ATF_TC(t_nofenv);

ATF_TC_HEAD(t_nofenv, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "dummy test case - no fenv.h support");
}


ATF_TC_BODY(t_nofenv, tc)
{
	atf_tc_skip("no fenv.h support on this architecture");
}

ATF_TP_ADD_TCS(tp)
{
	ATF_TP_ADD_TC(tp, t_nofenv);
	return atf_no_error();
}

#endif
