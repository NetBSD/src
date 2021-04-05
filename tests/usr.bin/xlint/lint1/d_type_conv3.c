/*	$NetBSD: d_type_conv3.c,v 1.4 2021/04/05 01:35:34 rillig Exp $	*/
# 3 "d_type_conv3.c"

/* Flag information-losing type conversion in argument lists */

int f(unsigned int);

void
should_fail()
{

	f(0x7fffffffffffffffLL);	/* expect: 259 *//* expect: 295 */
}
