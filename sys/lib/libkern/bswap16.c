/*	$NetBSD: bswap16.c,v 1.1.2.2 1997/10/14 10:26:37 thorpej Exp $	*/

/*
 * Written by Manuel Bouyer <bouyer@netbsd.org>.
 * Public domain.
 */

#if defined(LIBC_SCCS) && !defined(lint)
static char *rcsid = "$NetBSD: bswap16.c,v 1.1.2.2 1997/10/14 10:26:37 thorpej Exp $";
#endif

#include <sys/types.h>

#undef bswap16

u_int16_t
bswap16(x)
    u_int16_t x;
{
	return ((x << 8) & 0xff00) | ((x >> 8) & 0x00ff);
}
