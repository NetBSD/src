/*	$NetBSD: _inet_aton.c,v 1.2 2005/06/12 05:21:27 lukem Exp $	*/

/*
 * Written by Klaus Klein, September 14, 1999.
 * Public domain.
 */

#include <sys/cdefs.h>
#if defined(LIBC_SCCS) && !defined(lint)
__RCSID("$NetBSD: _inet_aton.c,v 1.2 2005/06/12 05:21:27 lukem Exp $");
#endif /* LIBC_SCCS and not lint */

#ifdef __indr_reference
__indr_reference(_inet_aton,inet_aton)
#else

#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>

extern int	_inet_aton __P((const char *, struct in_addr *));

int
inet_aton(cp, addr)
	const char *cp;
	struct in_addr *addr;
{

	return _inet_aton(cp, addr);
}
#endif
