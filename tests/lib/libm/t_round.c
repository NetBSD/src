/* $NetBSD: t_round.c,v 1.1 2011/04/08 06:49:21 jruoho Exp $ */

#include <atf-c.h>
#include <math.h>

/*
 * This tests for a bug in the initial implementation where
 * precision was lost in an internal substraction, leading to
 * rounding into the wrong direction.
 */

/* 0.5 - EPSILON */
#define VAL	0x0.7ffffffffffffcp0
#define VALF	0x0.7fffff8p0

ATF_TC(round_dir);
ATF_TC_HEAD(round_dir, tc)
{
	atf_tc_set_md_var(tc, "descr","Check for rounding in wrong direction");
}

ATF_TC_BODY(round_dir, tc)
{
	double a = VAL, b, c;
	float af = VALF, bf, cf;

	b = round(a);
	bf = roundf(af);

	ATF_REQUIRE(fabs(b) < 1.0e-40);
	ATF_REQUIRE(fabsf(bf) < 1.0e-40);

	c = round(-a);
	cf = roundf(-af);

	ATF_REQUIRE(fabs(c) < 1.0e-40);
	ATF_REQUIRE(fabsf(cf) < 1.0e-40);
}

ATF_TP_ADD_TCS(tp)
{

	ATF_TP_ADD_TC(tp, round_dir);

	return atf_no_error();
}
