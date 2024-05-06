/*	$NetBSD: t_bit.c,v 1.2 2024/05/06 18:41:23 riastradh Exp $	*/

/*
 * Written by Maya Rashish <maya@NetBSD.org>
 * Public domain.
 *
 * Testing signbit{,f,l} function correctly
 */

#include <sys/cdefs.h>
__RCSID("$NetBSD: t_bit.c,v 1.2 2024/05/06 18:41:23 riastradh Exp $");

#include <atf-c.h>
#include <float.h>
#include <math.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

static const struct {
	double input;
	bool is_negative;
} values[] = {
	{ -1,		true },
	{ -123,		true },
	{ -123E6,	true },
	{ -INFINITY,	true },
	{ INFINITY,	false },
	{ 123E6,	false },
	{ 0,		false },
	{ -0.,		true },
	{ -FLT_MIN,	true },
	{ FLT_MIN,	false },
	/*
	 * Cannot be accurately represented as float,
	 * but sign should be preserved
	 */
	{ DBL_MAX,	false },
	{ -DBL_MAX,	true },
};

static const struct {
	long double input;
	bool is_negative;
} ldbl_values[] = {
	{ -LDBL_MIN,	true },
	{ LDBL_MIN,	false },
	{ LDBL_MAX,	false },
	{ -LDBL_MAX,	true },
};

ATF_TC(signbit);
ATF_TC_HEAD(signbit, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Check that signbit functions correctly");
}

ATF_TC_BODY(signbit, tc)
{
	unsigned i;

	for (i = 0; i < __arraycount(values); i++) {
		const float iterator_f = values[i].input;
		const double iterator_d = values[i].input;
		const long double iterator_l = values[i].input;

		if (signbit(iterator_f) != values[i].is_negative) {
			atf_tc_fail("%s:%d iteration %d signbitf is wrong"
			    " about the sign of %f", __func__, __LINE__, i,
			    iterator_f);
		}
		if (signbit(iterator_d) != values[i].is_negative) {
			atf_tc_fail("%s:%d iteration %d signbit is wrong"
			    "about the sign of %f", __func__, __LINE__, i,
			    iterator_d);
		}
		if (signbit(iterator_l) != values[i].is_negative) {
			atf_tc_fail("%s:%d iteration %d signbitl is wrong"
			    " about the sign of %Lf", __func__, __LINE__, i,
			    iterator_l);
		}
	}

	for (i = 0; i < __arraycount(ldbl_values); i++) {
		if (signbit(ldbl_values[i].input) !=
		    ldbl_values[i].is_negative) {
			atf_tc_fail("%s:%d iteration %d signbitl is"
			    "wrong about the sign of %Lf",
			    __func__, __LINE__, i,
			    ldbl_values[i].input);
		}
	}

#ifdef NAN
	ATF_CHECK_EQ(signbit(copysignf(NAN, -1)), true);
	ATF_CHECK_EQ(signbit(copysignf(NAN, +1)), false);
	ATF_CHECK_EQ(signbit(copysign(NAN, -1)), true);
	ATF_CHECK_EQ(signbit(copysign(NAN, +1)), false);
	ATF_CHECK_EQ(signbit(copysignl(NAN, -1)), true);
	ATF_CHECK_EQ(signbit(copysignl(NAN, +1)), false);
#endif
}

ATF_TP_ADD_TCS(tp)
{

	ATF_TP_ADD_TC(tp, signbit);

	return atf_no_error();
}
