/*	$NetBSD: d_constant_conv1.c,v 1.3 2021/02/21 09:07:58 rillig Exp $	*/
# 3 "d_constant_conv1.c"

/* Flag information-losing constant conversion in argument lists */

int f(unsigned int);

void
should_fail()
{
	f(-1);			/* expect: 296 */
}
