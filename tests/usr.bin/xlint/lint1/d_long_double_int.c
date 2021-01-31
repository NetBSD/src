/*	$NetBSD: d_long_double_int.c,v 1.3 2021/01/31 14:39:31 rillig Exp $	*/
# 3 "d_long_double_int.c"

/* PR bin/39639: writing "long double" gave "long int" */

int
fail(long double *a, long int *b)
{
	return a == b;
}
