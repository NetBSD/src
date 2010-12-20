/* $NetBSD: t_libm.c,v 1.1 2010/12/20 23:47:23 pgoyette Exp $ */

#include <atf-c.h>
#include <math.h>

/*
 * This tests for a bug in the initial implementation where precision
 * was lost in an internal substraction, leading to rounding
 * into the wrong direction.
 */

ATF_TC(round);

/* 0.5 - EPSILON */
#define VAL 0x0.7ffffffffffffcp0
#define VALF 0x0.7fffff8p0

ATF_TC_HEAD(round, tc)
{
	atf_tc_set_md_var(tc, "descr", "Check for rounding in wrong direction");
}

ATF_TC_BODY(round, tc)
{
	double a = VAL, b, c;
	float af = VALF, bf, cf;

	b = round(a);
	bf = roundf(af);
	c = round(-a);
	cf = roundf(-af);
	ATF_REQUIRE(b == 0);
	ATF_REQUIRE(bf == 0);
	ATF_REQUIRE(c == 0);
	ATF_REQUIRE(cf == 0);
}

ATF_TP_ADD_TCS(tp)
{

	ATF_TP_ADD_TC(tp, round);

	return atf_no_error();
}
