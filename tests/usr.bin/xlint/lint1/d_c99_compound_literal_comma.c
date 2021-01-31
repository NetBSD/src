/*	$NetBSD: d_c99_compound_literal_comma.c,v 1.2 2021/01/31 14:39:31 rillig Exp $	*/
# 3 "d_c99_compound_literal_comma.c"

struct bintime {
	unsigned long long sec;
	unsigned long long frac;
};

struct bintime
us2bintime(unsigned long long us)
{

	return (struct bintime) {
		.sec = us / 1000000U,
		.frac = (((us % 1000000U) >> 32)/1000000U) >> 32,
	};
}
