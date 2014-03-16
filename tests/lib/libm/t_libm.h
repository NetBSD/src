/* $NetBSD: t_libm.h,v 1.4 2014/03/16 18:42:21 dsl Exp $ */

/*
 * Check result of fn(arg) is correct within the bounds.
 * Should be ok to do the checks using 'double' for 'float' functions.
 */
#define T_LIBM_CHECK(subtest, fn, arg, expect, epsilon) do { \
	double r = fn(arg); \
	double e = fabs(r - expect); \
	if (r != expect && e > epsilon) \
		atf_tc_fail_nonfatal( \
		    "subtest %u: " #fn "(%g) is %g (%.13a) not %g (%.13a), error %g (%.6a) > %g", \
		    subtest, arg, r, r, expect, expect, e, e, epsilon); \
    } while (0)

/* Check that the result of fn(arg) is NaN */
#ifndef __vax__
#define T_LIBM_CHECK_NAN(subtest, fn, arg) do { \
	double r = fn(arg); \
	if (!isnan(r)) \
		atf_tc_fail_nonfatal("subtest %u: " #fn "(%g) is %g not NaN", \
		    subtest, arg, r); \
    } while (0)
#else
/* vax doesn't support NaN */
#define T_LIBM_CHECK_NAN(subtest, fn, arg) (void)(arg)
#endif

/* Check that the result of fn(arg) is +0.0 */
#define T_LIBM_CHECK_PLUS_ZERO(subtest, fn, arg) do { \
	double r = fn(arg); \
	if (fabs(r) > 0.0 || signbit(r) != 0) \
		atf_tc_fail_nonfatal("subtest %u: " #fn "(%g) is %g not +0.0", \
		    subtest, arg, r); \
    } while (0)

/* Check that the result of fn(arg) is -0.0 */
#define T_LIBM_CHECK_MINUS_ZERO(subtest, fn, arg) do { \
	double r = fn(arg); \
	if (fabs(r) > 0.0 || signbit(r) == 0) \
		atf_tc_fail_nonfatal("subtest %u: " #fn "(%g) is %g not -0.0", \
		    subtest, arg, r); \
    } while (0)

/* Some useful constants (for test vectors) */
#ifndef __vax__	/* no NAN nor +/- INF on vax */
#define T_LIBM_NAN	(0.0 / 0.0)
#define T_LIBM_PLUS_INF	(+1.0 / 0.0)
#define T_LIBM_MINUS_INF (-1.0 / 0.0)
#endif

/* One line definition of a simple test */
#define ATF_LIBM_TEST(name, description) \
ATF_TC(name); \
ATF_TC_HEAD(name, tc) { atf_tc_set_md_var(tc, "descr", description); } \
ATF_TC_BODY(name, tc)
