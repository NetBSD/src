/*	$NetBSD: _inet_aton.c,v 1.1 1999/09/15 14:21:03 kleink Exp $	*/

/*
 * Written by Klaus Klein, September 14, 1999.
 * Public domain.
 */

#include <sys/cdefs.h>

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
