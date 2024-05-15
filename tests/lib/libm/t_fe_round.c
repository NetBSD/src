/*	$NetBSD: t_fe_round.c,v 1.20 2024/05/15 00:02:57 riastradh Exp $	*/

/*
 * Written by Maya Rashish <maya@NetBSD.org>
 * Public domain.
 *
 * Testing IEEE-754 rounding modes (and lrint)
 */

#include <sys/cdefs.h>
__RCSID("$NetBSD: t_fe_round.c,v 1.20 2024/05/15 00:02:57 riastradh Exp $");

#include <atf-c.h>
#include <fenv.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#ifdef __HAVE_FENV

/*#pragma STDC FENV_ACCESS ON gcc?? */

#define INT 9223L

static const char *
rmname(int rm)
{
	switch (rm) {
	case FE_TOWARDZERO:
		return "FE_TOWARDZERO";
	case FE_DOWNWARD:
		return "FE_DOWNWARD";
	case FE_UPWARD:
		return "FE_UPWARD";
	case FE_TONEAREST:
		return "FE_TONEAREST";
	default:
		return "unknown";
	}
}

/*
 * Examples are chosen to fit within the smallest single-precision
 * format any NetBSD port uses, so that we can write the examples once
 * in type double, and convert to single without raising inexact-result
 * exceptions when we're trying to test whether the integer-rounding
 * functions raise them.
 */
static const struct {
	int round_mode;
	double input;
	long int expected;
} values[] = {
	{ FE_DOWNWARD,		3.75,		3},
	{ FE_DOWNWARD,		-3.75,		-4},
	{ FE_DOWNWARD,		+0.,		0},
	{ FE_DOWNWARD,		-0.,		0},
	{ FE_DOWNWARD,		-INT-0.0625,	-INT-1},
	{ FE_DOWNWARD,		+INT-0.0625,	INT-1},
	{ FE_DOWNWARD,		-INT+0.0625,	-INT},
	{ FE_DOWNWARD,		+INT+0.0625,	INT},

	{ FE_UPWARD,		+0.,		0},
	{ FE_UPWARD,		-0.,		0},
	{ FE_UPWARD,		-123.75,	-123},
	{ FE_UPWARD,		123.75,		124},
	{ FE_UPWARD,		-INT-0.0625,	-INT},
	{ FE_UPWARD,		+INT-0.0625,	INT},
	{ FE_UPWARD,		-INT+0.0625,	-INT+1},
	{ FE_UPWARD,		+INT+0.0625,	INT+1},

	{ FE_TOWARDZERO,	1.9375,		1},
	{ FE_TOWARDZERO,	-1.9375,	-1},
	{ FE_TOWARDZERO,	0.25,		0},
	{ FE_TOWARDZERO,	INT+0.0625,	INT},
	{ FE_TOWARDZERO,	INT-0.0625,	INT - 1},
	{ FE_TOWARDZERO,	-INT+0.0625,	-INT + 1},
	{ FE_TOWARDZERO,	+0.,		0},
	{ FE_TOWARDZERO,	-0.,		0},

	{ FE_TONEAREST,		-INT-0.0625,	-INT},
	{ FE_TONEAREST,		+INT-0.0625,	INT},
	{ FE_TONEAREST,		-INT+0.0625,	-INT},
	{ FE_TONEAREST,		+INT+0.0625,	INT},
	{ FE_TONEAREST,		-INT-0.53125,	-INT-1},
	{ FE_TONEAREST,		+INT-0.53125,	INT-1},
	{ FE_TONEAREST,		-INT+0.53125,	-INT+1},
	{ FE_TONEAREST,		+INT+0.53125,	INT+1},
	{ FE_TONEAREST,		+0.,		0},
	{ FE_TONEAREST,		-0.,		0},
};

ATF_TC(fe_lrint);
ATF_TC_HEAD(fe_lrint, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Checking IEEE 754 rounding modes using lrint(3)");
}

ATF_TC_BODY(fe_lrint, tc)
{
	enum {
		LLRINT,
		LLRINTF,
		LRINT,
		LRINTF,
		N_FN,
	} fn;
	static const char *const fnname[] = {
		[LLRINT] = "llrint",
		[LLRINTF] = "llrintf",
		[LRINT] = "lrint",
		[LRINTF] = "lrintf",
	};
	long int received;
	unsigned i;

	for (i = 0; i < __arraycount(values); i++) {
		for (fn = 0; fn < N_FN; fn++) {
			/*
			 * Set the requested rounding mode.
			 */
			fesetround(values[i].round_mode);

			/*
			 * Call the lrint(3)-family function.
			 */
			switch (fn) {
			case LLRINT:
				received = llrint(values[i].input);
				break;
			case LLRINTF:
				received = llrintf(values[i].input);
				break;
			case LRINT:
				received = lrint(values[i].input);
				break;
			case LRINTF:
				received = lrintf(values[i].input);
				break;
			default:
				atf_tc_fail("impossible");
			}

			/*
			 * Assuming the result we got has zero
			 * fractional part, casting to long int should
			 * have no rounding.  Verify it matches the
			 * integer we expect.
			 */
			ATF_CHECK_MSG((long int)received == values[i].expected,
			    "[%u] %s %s(%f): got %ld, expected %ld",
			    i, rmname(values[i].round_mode), fnname[fn],
			    values[i].input,
			    (long int)received, values[i].expected);

			/* Do we get the same rounding mode out? */
			ATF_CHECK_MSG(fegetround() == values[i].round_mode,
			    "[%u] %s: set %d (%s), got %d (%s)",
			    i, fnname[fn],
			    values[i].round_mode, rmname(values[i].round_mode),
			    fegetround(), rmname(fegetround()));
		}
	}
}

ATF_TC(fe_nearbyint_rint);
ATF_TC_HEAD(fe_nearbyint_rint, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Checking IEEE 754 rounding modes using nearbyint/rint");
}

ATF_TC_BODY(fe_nearbyint_rint, tc)
{
	enum {
		NEARBYINT,
		NEARBYINTF,
		NEARBYINTL,
		RINT,
		RINTF,
		RINTL,
		N_FN,
	} fn;
	static const char *const fnname[] = {
		[NEARBYINT] = "nearbyint",
		[NEARBYINTF] = "nearbyintf",
		[NEARBYINTL] = "nearbyintl",
		[RINT] = "rint",
		[RINTF] = "rintf",
		[RINTL] = "rintl",
	};
	double received, ipart, fpart;
	unsigned i;

	for (i = 0; i < __arraycount(values); i++) {
		for (fn = 0; fn < N_FN; fn++) {
			bool expect_except =
			    values[i].input != (double)values[i].expected;

			/*
			 * Set the requested rounding mode.
			 */
			fesetround(values[i].round_mode);

#ifdef __ia64__
/*
 * Without this barrier, we get:
 *
 * /tmp//ccJayu9g.s:2793: Warning: Use of 'mov.m' violates RAW dependency 'AR[FPSR].sf0.flags' (impliedf)
 * /tmp//ccJayu9g.s:2793: Warning: Only the first path encountering the conflict is reported
 * /tmp//ccJayu9g.s:2757: Warning: This is the location of the conflicting usage
 *
 * (If you fix this, remove the entry from doc/HACKS.)
 */
			__insn_barrier();
#endif

			/*
			 * Clear sticky floating-point exception bits
			 * so we can verify whether the FE_INEXACT
			 * exception is raised.
			 */
			feclearexcept(FE_ALL_EXCEPT);

			/*
			 * Call the rint(3)-family function.
			 */
			switch (fn) {
			case NEARBYINT:
				received = nearbyint(values[i].input);
				expect_except = false;
				break;
			case NEARBYINTF:
				received = nearbyintf(values[i].input);
				expect_except = false;
				break;
			case NEARBYINTL:
				received = nearbyintl(values[i].input);
				expect_except = false;
				break;
			case RINT:
				received = rint(values[i].input);
				break;
			case RINTF:
				received = rintf(values[i].input);
				break;
			case RINTL:
				received = rintl(values[i].input);
				break;
			default:
				atf_tc_fail("impossible");
			}

			/*
			 * Verify FE_INEXACT was raised or not,
			 * depending on whether there was rounding and
			 * whether the function is supposed to raise
			 * exceptions.
			 */
			if (expect_except) {
				ATF_CHECK_MSG(fetestexcept(FE_INEXACT) != 0,
				    "[%u] %s %s(%f)"
				    " failed to raise FE_INEXACT",
				    i, rmname(values[i].round_mode),
				    fnname[fn], values[i].input);
			} else {
				ATF_CHECK_MSG(fetestexcept(FE_INEXACT) == 0,
				    "[%u] %s %s(%f)"
				    " spuriously raised FE_INEXACT",
				    i, rmname(values[i].round_mode),
				    fnname[fn], values[i].input);
			}

			/*
			 * Verify the fractional part of the result is
			 * zero -- the result of rounding to an integer
			 * is supposed to be an integer.
			 */
			fpart = modf(received, &ipart);
			ATF_CHECK_MSG(fpart == 0,
			    "[%u] %s %s(%f)=%f has fractional part %f"
			    " (integer part %f)",
			    i, rmname(values[i].round_mode), fnname[fn],
			    values[i].input, received, fpart, ipart);

			/*
			 * Assuming the result we got has zero
			 * fractional part, casting to long int should
			 * have no rounding.  Verify it matches the
			 * integer we expect.
			 */
			ATF_CHECK_MSG((long int)received == values[i].expected,
			    "[%u] %s %s(%f): got %f, expected %ld",
			    i, rmname(values[i].round_mode), fnname[fn],
			    values[i].input, received, values[i].expected);

			/* Do we get the same rounding mode out? */
			ATF_CHECK_MSG(fegetround() == values[i].round_mode,
			    "[%u] %s: set %d (%s), got %d (%s)",
			    i, fnname[fn],
			    values[i].round_mode, rmname(values[i].round_mode),
			    fegetround(), rmname(fegetround()));
		}
	}
}

#ifdef __HAVE_LONG_DOUBLE

/*
 * Use one bit more than fits in IEEE 754 binary64.
 */
static const struct {
	int round_mode;
	long double input;
	int64_t expected;
} valuesl[] = {
	{ FE_TOWARDZERO,	0x2.00000000000008p+52L, 0x20000000000000 },
	{ FE_DOWNWARD,		0x2.00000000000008p+52L, 0x20000000000000 },
	{ FE_UPWARD,		0x2.00000000000008p+52L, 0x20000000000001 },
	{ FE_TONEAREST,		0x2.00000000000008p+52L, 0x20000000000000 },
	{ FE_TOWARDZERO,	0x2.00000000000018p+52L, 0x20000000000001 },
	{ FE_DOWNWARD,		0x2.00000000000018p+52L, 0x20000000000001 },
	{ FE_UPWARD,		0x2.00000000000018p+52L, 0x20000000000002 },
	{ FE_TONEAREST,		0x2.00000000000018p+52L, 0x20000000000002 },
};

ATF_TC(fe_nearbyintl_rintl);
ATF_TC_HEAD(fe_nearbyintl_rintl, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Checking IEEE 754 rounding modes using nearbyintl/rintl");
}

ATF_TC_BODY(fe_nearbyintl_rintl, tc)
{
	enum {
		RINTL,
		NEARBYINTL,
		N_FN,
	} fn;
	static const char *const fnname[] = {
		[RINTL] = "rintl",
		[NEARBYINTL] = "nearbyintl",
	};
	long double received, ipart, fpart;
	unsigned i;

	for (i = 0; i < __arraycount(valuesl); i++) {
		for (fn = 0; fn < N_FN; fn++) {
			bool expect_except =
			    (valuesl[i].input !=
				(long double)valuesl[i].expected);

			/*
			 * Set the requested rounding mode.
			 */
			fesetround(valuesl[i].round_mode);

			/*
			 * Clear sticky floating-point exception bits
			 * so we can verify whether the FE_INEXACT
			 * exception is raised.
			 */
			feclearexcept(FE_ALL_EXCEPT);

			/*
			 * Call the rint(3)-family function.
			 */
			switch (fn) {
			case NEARBYINTL:
				received = nearbyintl(valuesl[i].input);
				expect_except = false;
				break;
			case RINTL:
				received = rintl(valuesl[i].input);
				break;
			default:
				atf_tc_fail("impossible");
			}

			/*
			 * Verify FE_INEXACT was raised or not,
			 * depending on whether there was rounding and
			 * whether the function is supposed to raise
			 * exceptions.
			 */
			if (expect_except) {
				ATF_CHECK_MSG(fetestexcept(FE_INEXACT) != 0,
				    "[%u] %s %s(%Lf)"
				    " failed to raise FE_INEXACT",
				    i, rmname(valuesl[i].round_mode),
				    fnname[fn], valuesl[i].input);
			} else {
				ATF_CHECK_MSG(fetestexcept(FE_INEXACT) == 0,
				    "[%u] %s %s(%Lf)"
				    " spuriously raised FE_INEXACT",
				    i, rmname(valuesl[i].round_mode),
				    fnname[fn], valuesl[i].input);
			}

			/*
			 * Verify the fractional part of the result is
			 * zero -- the result of rounding to an integer
			 * is supposed to be an integer.
			 */
			fpart = modfl(received, &ipart);
			ATF_CHECK_MSG(fpart == 0,
			    "[%u] %s %s(%Lf)=%Lf has fractional part %Lf"
			    " (integer part %Lf)",
			    i, rmname(valuesl[i].round_mode), fnname[fn],
			    valuesl[i].input, received, fpart, ipart);

			/*
			 * Assuming the result we got has zero
			 * fractional part, casting to int64_t should
			 * have no rounding.  Verify it matches the
			 * integer we expect.
			 */
			ATF_CHECK_MSG(((int64_t)received ==
				valuesl[i].expected),
			    "[%u] %s %s(%Lf): got %Lf, expected %jd",
			    i, rmname(valuesl[i].round_mode), fnname[fn],
			    valuesl[i].input, received, valuesl[i].expected);

			/* Do we get the same rounding mode out? */
			ATF_CHECK_MSG(fegetround() == valuesl[i].round_mode,
			    "[%u] %s: set %d (%s), got %d (%s)",
			    i, fnname[fn],
			    valuesl[i].round_mode,
			    rmname(valuesl[i].round_mode),
			    fegetround(), rmname(fegetround()));
		}
	}
}

#endif	/* __HAVE_LONG_DOUBLE */

ATF_TP_ADD_TCS(tp)
{

	ATF_TP_ADD_TC(tp, fe_lrint);
	ATF_TP_ADD_TC(tp, fe_nearbyint_rint);
#ifdef __HAVE_LONG_DOUBLE
	ATF_TP_ADD_TC(tp, fe_nearbyintl_rintl);
#endif

	return atf_no_error();
}

#else  /* !__HAVE_FENV */

ATF_TP_ADD_TCS(tp)
{

	/*
	 * No fenv, no fesetround to test.
	 */
	return atf_no_error();
}

#endif	/* __HAVE_FENV */
