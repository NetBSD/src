/* $NetBSD: opt_c.c,v 1.1 2021/10/22 20:54:36 rillig Exp $ */
/* $FreeBSD$ */

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
