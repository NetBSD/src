/*	$NetBSD: d_type_conv2.c,v 1.3 2021/02/21 09:07:58 rillig Exp $	*/
# 3 "d_type_conv2.c"

/* Flag information-losing type conversion in argument lists */

int f(float);

void
should_fail()
{
	double x = 2.0;

	f(x);			/* expect: 259 */
}
