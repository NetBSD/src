/*	$NetBSD: d_type_conv1.c,v 1.3 2021/02/21 09:07:58 rillig Exp $	*/
# 3 "d_type_conv1.c"

/* Flag information-losing type conversion in argument lists */

int f(unsigned int);

void
should_fail()
{
	long long x = 20;

	f(x);			/* expect: 259 */
}
