/*	$NetBSD: ntohs.c,v 1.1 1999/11/11 20:22:00 thorpej Exp $	*/

/*
 * Written by J.T. Conklin <jtc@netbsd.org>.
 * Public domain.
 */

#include "stand.h"

#undef ntohs

u_int16_t
ntohs(x)
	u_int16_t x;
{
#if BYTE_ORDER == LITTLE_ENDIAN
	u_char *s = (u_char *) &x;
	return (u_int16_t)(s[0] << 8 | s[1]);
#else
	return x;
#endif
}
