/*	$NetBSD: bswap16.c,v 1.1 1997/10/09 15:42:32 bouyer Exp $	*/

/*
 * Written by Manuel Bouyer <bouyer@netbsd.org>.
 * Public domain.
 */

#if defined(LIBC_SCCS) && !defined(lint)
static char *rcsid = "$NetBSD: bswap16.c,v 1.1 1997/10/09 15:42:32 bouyer Exp $";
#endif

#include <sys/types.h>

#undef bswap16

u_int16_t
bswap16(x)
    u_int16_t x;
{
	return ((x << 8) & 0xff00) | ((x >> 8) & 0x00ff);
}
