/*	$NetBSD: puts.c,v 1.3 1998/01/05 07:02:46 perry Exp $	*/

#include <lib/libsa/stand.h>

/* XXX non-standard */

void
puts(s)
	char *s;
{

	while (*s)
		putchar(*s++);
}
