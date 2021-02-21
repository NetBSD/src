/*	$NetBSD: d_long_double_int.c,v 1.4 2021/02/21 09:07:58 rillig Exp $	*/
# 3 "d_long_double_int.c"

/* PR bin/39639: writing "long double" gave "long int" */

int
fail(long double *a, long int *b)
{
	return a == b;		/* expect: 124 */
}
