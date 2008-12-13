/*	$NetBSD: nullcom.c,v 1.1.4.2 2008/12/13 01:13:08 haad Exp $	*/

/* 
 * nulldev-like console stubs for quiet gzboot
 */

void
cons_init(void)
{
}

int
getchar(void)
{
	return 0;	/* XXX */
}

void
putchar(int c)
{
}
