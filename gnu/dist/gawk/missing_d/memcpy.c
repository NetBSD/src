/*	$NetBSD: memcpy.c,v 1.1.1.1 2003/10/06 16:44:06 wiz Exp $	*/

/*
 * memcpy --- copy strings.
 *
 * We supply this routine for those systems that aren't standard yet.
 */

char *
memcpy (dest, src, l)
register char *dest, *src;
register int l;
{
	register char *ret = dest;

	while (l--)
		*dest++ = *src++;

	return ret;
}
