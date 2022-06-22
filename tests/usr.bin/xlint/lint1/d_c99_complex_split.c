/*	$NetBSD: d_c99_complex_split.c,v 1.11 2022/06/22 19:23:18 rillig Exp $	*/
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

/*
 * Before tree.c 1.275 from 2021-04-09, lint wrongly warned that when
 * '__real__ c' was assigned, 'c may be used before set'.
 *
 * As of 2021-04-09, support for _Complex is still very incomplete, see
 * build_real_imag for details.  For example, lint does not know that after
 * the assignment to '__real__ c', the variable is partially initialized.
 */
void
set_complex_complete(double re, double im)
{
	double _Complex c;

	__real__ c = re;
	__imag__ c = im;
	sink(c);
}

/*
 * Before tree.c 1.275 from 2021-04-09, lint wrongly warned that when
 * '__real__ c' was assigned, 'c may be used before set [158]'.
 *
 * As of 2021-04-09, support for _Complex is still very incomplete, see
 * build_real_imag for details.
 */
void
set_complex_only_real(double re)
{
	double _Complex c;

	__real__ c = re;
	/* __imag__ c is left uninitialized */
	sink(c);		/* XXX: may be used before set */
}

/*
 * Before tree.c 1.275 from 2021-04-09, lint wrongly warned that when
 * '__imag__ c' was assigned, 'c may be used before set [158]'.
 *
 * As of 2021-04-09, support for _Complex is still very incomplete, see
 * build_real_imag for details.
 */
void
set_complex_only_imag(double im)
{
	double _Complex c;

	/* __real__ c is left uninitialized */
	__imag__ c = im;
	sink(c);		/* XXX: may be used before set */
}

/* Just to keep the .exp file alive. */
void
trigger_warning(double _Complex c)
{
	c += 1.0;
	/* expect+1: error: operands of '|' have incompatible types 'double _Complex' and 'double _Complex' [107] */
	return c | c;
}

void
precedence_cast_expression(void)
{
	double _Complex z = 0;
	if (b(__real__(double _Complex)z) && b(__imag__(double _Complex)z))
		return;
	if (b(__real__(z)) && b(__imag__(z)))
		return;
}
