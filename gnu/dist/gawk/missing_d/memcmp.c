/*	$NetBSD: memcmp.c,v 1.1.1.1 2003/10/06 16:44:06 wiz Exp $	*/

/*
 * memcmp --- compare strings.
 *
 * We use our own routine since it has to act like strcmp() for return
 * value, and the BSD manual says bcmp() only returns zero/non-zero.
 */

int
memcmp (s1, s2, l)
register char *s1, *s2;
register int l;
{
	for (; l-- > 0; s1++, s2++) {
		if (*s1 != *s2)
			return (*s1 - *s2);
	}
	return (0);
}
