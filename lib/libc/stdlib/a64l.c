/*
 * Written by J.T. Conklin <jtc@netbsd.org>.
 * Public domain.
 */

#include <sys/cdefs.h>
#if defined(LIBC_SCCS) && !defined(lint)
__RCSID("$NetBSD: a64l.c,v 1.5 1997/07/21 14:08:48 jtc Exp $");
#endif

#include "namespace.h"
#include <stdlib.h>

#ifdef __weak_alias
__weak_alias(a64l,_a64l);
#endif

long
a64l(s)
	const char *s;
{
	long value, digit, shift;
	int i;

	value = 0;
	shift = 0;
	for (i = 0; *s && i < 6; i++, s++) {
		if (*s <= '/')
			digit = *s - '.';
		else if (*s <= '9')
			digit = *s - '0' + 2;
		else if (*s <= 'Z')
			digit = *s - 'A' + 12;
		else
			digit = *s - 'a' + 38; 

		value |= digit << shift;
		shift += 6;
	}

	return (long) value;
}
