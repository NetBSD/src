/*	$NetBSD: d_long_double_int.c,v 1.5 2022/01/15 14:22:03 rillig Exp $	*/
# 3 "d_long_double_int.c"

/* PR bin/39639: writing "long double" gave "long int" */

int
fail(long double *a, long int *b)
{
	/* expect+1: warning: illegal combination of 'pointer to long double' and 'pointer to long', op '==' [124] */
	return a == b;
}
