/*  bswap16.c,v 1.1 2005/12/20 19:28:51 christos Exp    */

/*
 * Written by Manuel Bouyer <bouyer@NetBSD.org>.
 * Public domain.
 */

#include <sys/cdefs.h>
#if defined(LIBC_SCCS) && !defined(lint)
__RCSID("bswap16.c,v 1.1 2005/12/20 19:28:51 christos Exp");
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
