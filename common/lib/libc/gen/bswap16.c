/*  $NetBSD: bswap16.c,v 1.4.34.2 2020/04/21 19:37:49 martin Exp $    */

/*
 * Written by Manuel Bouyer <bouyer@NetBSD.org>.
 * Public domain.
 */

#include <sys/cdefs.h>
#if defined(LIBC_SCCS) && !defined(lint)
__RCSID("$NetBSD: bswap16.c,v 1.4.34.2 2020/04/21 19:37:49 martin Exp $");
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
