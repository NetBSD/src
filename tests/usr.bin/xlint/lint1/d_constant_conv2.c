/*	$NetBSD: d_constant_conv2.c,v 1.7 2023/07/09 10:42:07 rillig Exp $	*/
# 3 "d_constant_conv2.c"

/* Flag information-losing constant conversion in argument lists */

/*
 * Before tree.c 1.427 from 2022-04-15, lint warned about conversions due to
 * prototype even in C99 mode, which is far away from traditional C to make
 * non-prototype functions an issue.
 */
/* lint1-flags: -h -w */
/* lint1-extra-flags: -X 351 */

int f(unsigned int);

void
should_fail(void)
{
	/* expect+1: warning: argument 1 is converted from 'double' to 'unsigned int' due to prototype [259] */
	f(2.1);
}
