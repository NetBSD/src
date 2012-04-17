/*  $NetBSD: bswap16.c,v 1.3.2.1 2012/04/17 00:01:39 yamt Exp $    */

/*
 * Written by Manuel Bouyer <bouyer@NetBSD.org>.
 * Public domain.
 */

#include <sys/cdefs.h>
#if defined(LIBC_SCCS) && !defined(lint)
__RCSID("$NetBSD: bswap16.c,v 1.3.2.1 2012/04/17 00:01:39 yamt Exp $");
#endif /* LIBC_SCCS and not lint */

#include <sys/types.h>
#include <machine/bswap.h>

#undef bswap16

uint16_t
bswap16(uint16_t x)
{
	/*LINTED*/
	return ((x << 8) & 0xff00) | ((x >> 8) & 0x00ff);
}
