/*  $NetBSD: bswap16.c,v 1.1 1999/01/15 13:31:22 bouyer Exp $    */

/*
 * Written by Manuel Bouyer <bouyer@netbsd.org>.
 * Public domain.
 */

#include <sys/cdefs.h>
#if defined(LIBC_SCCS) && !defined(lint)
__RCSID("$NetBSD: bswap16.c,v 1.1 1999/01/15 13:31:22 bouyer Exp $");
#endif /* LIBC_SCCS and not lint */

#include <sys/types.h>
#include <machine/bswap.h>

#undef bswap16

u_int16_t
bswap16(x)
	u_int16_t x;
{
	return ((x << 8) & 0xff00) | ((x >> 8) & 0x00ff);
}
