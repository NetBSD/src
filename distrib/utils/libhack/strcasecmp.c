/*	$NetBSD: strcasecmp.c,v 1.2 2019/08/12 09:34:53 martin Exp $	*/

/*
 * Written by Martin Husemann <martin@NetBSD.org>
 * Public domain.
 */

#include <strings.h>

/*
 * Simple strcasecmp, try to avoid pulling in real locales
 */
int
strcasecmp(const char *s1, const char *s2)
{
	unsigned int c1, c2;

	do {
		c1 = *s1++;
		c2 = *s2++;
		if (c1 >= 'A' && c1 <= 'Z')
			c1 += 'a' - 'A';
		if (c2 >= 'A' && c2 <= 'Z')
			c2 += 'a' - 'A';
	} while (c1 == c2 && c1 != 0 && c2 != 0);

	return ((c1 == c2) ? 0 : ((c1 > c2) ? 1 : -1));
}
