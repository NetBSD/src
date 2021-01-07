/*	$NetBSD: msg_097.c,v 1.2 2021/01/07 00:38:46 rillig Exp $	*/
# 3 "msg_097.c"

/* Test for message: suffix U is illegal in traditional C [97] */

/* lint1-flags: -gtw */

void
example()
{
	int i = 1234567;
	unsigned u = 1234567;
	unsigned u_upper = 1234567U;
	unsigned u_lower = 1234567u;

	long l = 1234567L;
	unsigned long ul = 1234567UL;

	long long ll = 1234567LL;
	unsigned long long ull = 1234567ULL;
}
