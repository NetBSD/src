/*	$NetBSD: d_c99_complex_split.c,v 1.5 2021/03/27 13:59:18 rillig Exp $	*/
# 3 "d_c99_complex_split.c"

/*
 * Checks that the real and imaginary parts of a complex number can be
 * accessed (since C99).
 */

int
b(double a)
{
	return a == 0;
}

void
a(void)
{
	double _Complex z = 0;
	if (b(__real__ z) && b(__imag__ z))
		return;
}
