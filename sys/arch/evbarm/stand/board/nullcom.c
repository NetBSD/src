/*	$NetBSD: nullcom.c,v 1.1 2008/11/10 20:30:12 cliff Exp $	*/

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
