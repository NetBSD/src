/*	$NetBSD: msg_098.c,v 1.5 2022/04/03 09:34:45 rillig Exp $	*/
# 3 "msg_098.c"

/* Test for message: suffixes F and L are illegal in traditional C [98] */

/* lint1-flags: -gtw */

void
example()
{
	float f = 1234.5;
	/* expect+1: warning: suffixes F and L are illegal in traditional C [98] */
	float f_F = 1234.5F;
	/* expect+1: warning: suffixes F and L are illegal in traditional C [98] */
	float f_f = 1234.5f;

	double d = 1234.5;
	/* expect+1: error: syntax error 'U' [249] */
	double d_U = 1234.5U;

	/* expect+1: warning: 'long double' is illegal in traditional C [266] */
	long double ld = 1234.5;
	/* expect+2: warning: 'long double' is illegal in traditional C [266] */
	/* expect+1: warning: suffixes F and L are illegal in traditional C [98] */
	long double ld_L = 1234.5L;
}
