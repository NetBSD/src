/*	$NetBSD: d_type_conv2.c,v 1.4 2022/01/15 14:22:03 rillig Exp $	*/
# 3 "d_type_conv2.c"

/* Flag information-losing type conversion in argument lists */

int f(float);

void
should_fail()
{
	double x = 2.0;

	/* expect+1: warning: argument #1 is converted from 'double' to 'float' due to prototype [259] */
	f(x);
}
