/*	$NetBSD: d_type_conv1.c,v 1.2 2021/01/31 14:39:31 rillig Exp $	*/
# 3 "d_type_conv1.c"

/* Flag information-losing type conversion in argument lists */

int f(unsigned int);

void
should_fail()
{
	long long x = 20;

	f(x);
}
