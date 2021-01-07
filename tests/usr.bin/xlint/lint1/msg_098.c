/*	$NetBSD: msg_098.c,v 1.2 2021/01/07 00:38:46 rillig Exp $	*/
# 3 "msg_098.c"

/* Test for message: suffixes F and L are illegal in traditional C [98] */

/* lint1-flags: -gtw */

void
example()
{
	float f = 1234.5;
	float f_F = 1234.5F;
	float f_f = 1234.5F;

	double d = 1234.5;
	double d_U = 1234.5U;

	long double ld = 1234.5;
	long double ld_L = 1234.5L;
}
