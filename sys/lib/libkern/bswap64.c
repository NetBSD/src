/*	$NetBSD: bswap64.c,v 1.1 1997/10/09 15:42:33 bouyer Exp $	*/

/*
 * Written by Manuel Bouyer <bouyer@netbsd.org>.
 * Public domain.
 */

#if defined(LIBC_SCCS) && !defined(lint)
static char *rcsid = "$NetBSD: bswap64.c,v 1.1 1997/10/09 15:42:33 bouyer Exp $";
#endif

#include <sys/types.h>

#undef bswap64

u_int64_t
bswap64(x)
    u_int64_t x;
{  
	u_int32_t *p = (u_int32_t*)&x;
	u_int32_t t;
	t = bswap32(p[0]);
	p[0] = bswap32(p[1]);
	p[1] = t;
	return x;
}   

