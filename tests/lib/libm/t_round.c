/* $NetBSD: t_round.c,v 1.2 2011/07/04 22:33:29 mrg Exp $ */

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

#ifdef __vax__
#define SMALL_NUM	1.0e-38
#else
#define SMALL_NUM	1.0e-40
#endif

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

	ATF_REQUIRE(fabs(b) < SMALL_NUM);
	ATF_REQUIRE(fabsf(bf) < SMALL_NUM);

	c = round(-a);
	cf = roundf(-af);

	ATF_REQUIRE(fabs(c) < SMALL_NUM);
	ATF_REQUIRE(fabsf(cf) < SMALL_NUM);
}

ATF_TP_ADD_TCS(tp)
{

	ATF_TP_ADD_TC(tp, round_dir);

	return atf_no_error();
}
