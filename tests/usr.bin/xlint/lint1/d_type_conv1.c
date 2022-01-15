/*	$NetBSD: d_type_conv1.c,v 1.4 2022/01/15 14:22:03 rillig Exp $	*/
# 3 "d_type_conv1.c"

/* Flag information-losing type conversion in argument lists */

int f(unsigned int);

void
should_fail()
{
	long long x = 20;

	/* expect+1: warning: argument #1 is converted from 'long long' to 'unsigned int' due to prototype [259] */
	f(x);
}
