/*
 * Written by J.T. Conklin <jtc@netbsd.org>.
 * Public domain.
 */

#include <sys/cdefs.h>
#if defined(LIBC_SCCS) && !defined(lint)
__RCSID("$NetBSD: l64a.c,v 1.5 1997/07/13 20:16:44 christos Exp $");
#endif

#include <stdlib.h>

char *
l64a (value)
	long value;
{
	static char buf[8];
	char *s = buf;
	int digit;
	int i;

	if (!value) 
		return NULL;

	for (i = 0; value != 0 && i < 6; i++) {
		digit = value & 0x3f;

		if (digit < 2) 
			*s = digit + '.';
		else if (digit < 12)
			*s = digit + '0' - 2;
		else if (digit < 38)
			*s = digit + 'A' - 12;
		else
			*s = digit + 'a' - 38;

		value >>= 6;
		s++;
	}

	*s = '\0';

	return buf;
}
