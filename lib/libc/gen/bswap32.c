/*  $NetBSD: bswap32.c,v 1.1 1999/01/15 13:31:22 bouyer Exp $    */

/*
 * Written by Manuel Bouyer <bouyer@netbsd.org>.
 * Public domain.
 */

#include <sys/cdefs.h>
#if defined(LIBC_SCCS) && !defined(lint)
__RCSID("$NetBSD: bswap32.c,v 1.1 1999/01/15 13:31:22 bouyer Exp $");
#endif /* LIBC_SCCS and not lint */

#include <sys/types.h>
#include <machine/bswap.h>

#undef bswap32

u_int32_t
bswap32(x)
	u_int32_t x;
{
	return	((x << 24) & 0xff000000 ) |
		((x <<  8) & 0x00ff0000 ) |
		((x >>  8) & 0x0000ff00 ) |
		((x >> 24) & 0x000000ff );
}
