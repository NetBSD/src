/*	$NetBSD: puts.c,v 1.4 1998/10/15 00:56:56 ross Exp $	*/

#include <lib/libsa/stand.h>

/* XXX non-standard */

void
puts(s)
	const char *s;
{

	while (*s)
		putchar(*s++);
}
