/*	$NetBSD: d_type_conv2.c,v 1.2 2021/01/31 14:39:31 rillig Exp $	*/
# 3 "d_type_conv2.c"

/* Flag information-losing type conversion in argument lists */

int f(float);

void
should_fail()
{
	double x = 2.0;

	f(x);
}
