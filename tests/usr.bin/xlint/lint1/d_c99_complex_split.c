/*	$NetBSD: d_c99_complex_split.c,v 1.6 2021/04/09 21:07:39 rillig Exp $	*/
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

void sink(double _Complex);

void
set_complex_complete(double re, double im)
{
	double _Complex c;

	__real__ c = re; /* FIXME *//* expect: may be used before set */
	__imag__ c = im;
	sink(c);
}

void
set_complex_only_real(double re)
{
	double _Complex c;

	__real__ c = re; /* FIXME *//* expect: may be used before set */
	/* __imag__ c is left uninitialized */
	sink(c);		/* XXX: may be used before set */
}

void
set_complex_only_imag(double im)
{
	double _Complex c;

	/* __real__ c is left uninitialized */
	__imag__ c = im; /* FIXME *//* expect: may be used before set */
	sink(c);		/* XXX: may be used before set */
}
