/* $NetBSD: opt_c.c,v 1.3 2022/04/22 21:21:20 rillig Exp $ */

/*
 * Tests for the option '-c', which specifies the column in which the comments
 * to the right of the code start.
 */

#indent input
bool
is_prime(int n)
{
	if (n <= 3)
		return n >= 2; /* special case */
	if (n % 2 == 0)
		return false;				/* even numbers */
	return true;
}
#indent end

#indent run -c49
bool
is_prime(int n)
{
	if (n <= 3)
		return n >= 2;			/* special case */
	if (n % 2 == 0)
		return false;			/* even numbers */
	return true;
}
#indent end

/*
 * If the code is too wide to allow the comment in its preferred column, it is
 * nevertheless indented with a single tab, to keep multiple comments
 * vertically aligned.
 */
#indent run -c9
bool
is_prime(int n)
{
	if (n <= 3)
		return n >= 2;	/* special case */
	if (n % 2 == 0)
		return false;	/* even numbers */
	return true;
}
#indent end

/*
 * Usually, comments are aligned at a tabstop, but indent can also align them
 * at any other column.
 */
#indent run -c37
bool
is_prime(int n)
{
	if (n <= 3)
		return n >= 2;	    /* special case */
	if (n % 2 == 0)
		return false;	    /* even numbers */
	return true;
}
#indent end
