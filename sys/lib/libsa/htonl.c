/*	$NetBSD: htonl.c,v 1.1 1999/11/11 20:21:59 thorpej Exp $	*/

/*
 * Written by J.T. Conklin <jtc@netbsd.org>.
 * Public domain.
 */

#include "stand.h"

#undef htonl

u_int32_t 
htonl(x)
	u_int32_t x;
{
#if BYTE_ORDER == LITTLE_ENDIAN
	u_char *s = (u_char *)&x;
	return (u_int32_t)(s[0] << 24 | s[1] << 16 | s[2] << 8 | s[3]);
#else
	return x;
#endif
}
