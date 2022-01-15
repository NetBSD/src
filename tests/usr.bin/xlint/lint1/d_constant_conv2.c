/*	$NetBSD: d_constant_conv2.c,v 1.4 2022/01/15 14:22:03 rillig Exp $	*/
# 3 "d_constant_conv2.c"

/* Flag information-losing constant conversion in argument lists */

int f(unsigned int);

void
should_fail()
{
	/* expect+1: warning: argument #1 is converted from 'double' to 'unsigned int' due to prototype [259] */
	f(2.1);
}
