/*	$NetBSD: d_c99_complex_split.c,v 1.4 2021/01/31 14:57:28 rillig Exp $	*/
# 3 "d_c99_complex_split.c"

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
