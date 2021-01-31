/*	$NetBSD: d_type_conv3.c,v 1.2 2021/01/31 14:39:31 rillig Exp $	*/
# 3 "d_type_conv3.c"

/* Flag information-losing type conversion in argument lists */

int f(unsigned int);

void
should_fail()
{

	f(0x7fffffffffffffffLL);
}
