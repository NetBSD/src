/*	$NetBSD: memset.c,v 1.1.1.1 2003/10/06 16:44:06 wiz Exp $	*/

/*
 * memset --- initialize memory
 *
 * We supply this routine for those systems that aren't standard yet.
 */

void *
memset(dest, val, l)
void *dest;
register int val;
register size_t l;
{
	register char *ret = dest;
	register char *d = dest;

	while (l--)
		*d++ = val;

	return ((void *) ret);
}
