/*	$NetBSD: htonl.c,v 1.7 1996/10/13 04:08:35 christos Exp $	*/

/*
 * Written by J.T. Conklin <jtc@netbsd.org>.
 * Public domain.
 */

#if defined(LIBC_SCCS) && !defined(lint)
static char *rcsid = "$NetBSD: htonl.c,v 1.7 1996/10/13 04:08:35 christos Exp $";
#endif

#include <sys/types.h>

#undef htonl

in_addr_t 
htonl(x)
	in_addr_t x;
{
#if BYTE_ORDER == LITTLE_ENDIAN
	u_char *s = (u_char *)x;
	return (in_addr_t)(s[0] << 24 | s[1] << 16 | s[2] << 8 | s[3]);
#else
	return x;
#endif
}
