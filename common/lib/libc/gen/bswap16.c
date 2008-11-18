/*  $NetBSD: bswap16.c,v 1.1.6.1 2008/11/18 19:10:02 snj Exp $    */

/*
 * Written by Manuel Bouyer <bouyer@NetBSD.org>.
 * Public domain.
 */

#include <sys/cdefs.h>
#if defined(LIBC_SCCS) && !defined(lint)
__RCSID("$NetBSD: bswap16.c,v 1.1.6.1 2008/11/18 19:10:02 snj Exp $");
#endif /* LIBC_SCCS and not lint */

#include <sys/types.h>
#include <machine/bswap.h>

#undef bswap16

uint16_t
bswap16(x)
	uint16_t x;
{
	return ((x << 8) & 0xff00) | ((x >> 8) & 0x00ff);
}
