/*  $NetBSD: bswap64.c,v 1.3 2003/07/26 19:24:42 salo Exp $    */

/*
 * Written by Manuel Bouyer <bouyer@NetBSD.org>.
 * Public domain.
 */

#include <sys/cdefs.h>
#if defined(LIBC_SCCS) && !defined(lint)
__RCSID("$NetBSD: bswap64.c,v 1.3 2003/07/26 19:24:42 salo Exp $");
#endif /* LIBC_SCCS and not lint */

#include <sys/types.h>
#include <machine/bswap.h>

#undef bswap64

u_int64_t
bswap64(x)
	u_int64_t x;
{
	u_int32_t *p = (u_int32_t*)(void *)&x;
	u_int32_t t;
	t = bswap32(p[0]);
	p[0] = bswap32(p[1]);
	p[1] = t;
	return x;
}
