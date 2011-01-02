/*	$NetBSD: t_nan_inf.c,v 1.1 2011/01/02 03:51:21 pgoyette Exp $	*/

/*
 * This file is in the Public Domain.
 *
 * The nan test is blatently copied by Simon Burge from the infinity
 * test by Ben Harris.
 */

#include <atf-c.h>
#include <atf-c/config.h>

#include <math.h>
#include <string.h>

ATF_TC(isnan);

ATF_TC_HEAD(isnan, tc)
{

	atf_tc_set_md_var(tc, "descr", "Verify that isnan(3) works");
}

ATF_TC_BODY(isnan, tc)
{

	/* NAN is meant to be a (float) NaN. */
	ATF_CHECK(isnan(NAN));
	ATF_CHECK(isnan((double)NAN));
}

ATF_TC(isinf);

ATF_TC_HEAD(isinf, tc)
{

	atf_tc_set_md_var(tc, "descr", "Verify that isinf(3) works");
}

ATF_TC_BODY(isinf, tc)
{

	/* HUGE_VAL is meant to be an infinity. */
	ATF_CHECK(isinf(HUGE_VAL));

	/* HUGE_VALF is the float analog of HUGE_VAL. */
	ATF_CHECK(isinf(HUGE_VALF));

	/* HUGE_VALL is the long double analog of HUGE_VAL. */
	ATF_CHECK(isinf(HUGE_VALL));
}

ATF_TP_ADD_TCS(tp)
{
	const char *arch;

	arch = atf_config_get("atf_machine");
	if (strcmp("vax", arch) == 0 || strcmp("m68000", arch) == 0)
		atf_tc_skip("Test not applicable on %s", arch);
	else {
		ATF_TP_ADD_TC(tp, isnan);
		ATF_TP_ADD_TC(tp, isinf);
	}

	return atf_no_error();
}
