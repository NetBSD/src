/*	$NetBSD: _inet_pton.c,v 1.3 2005/07/30 15:21:20 christos Exp $	*/

/*
 * Written by Klaus Klein, September 14, 1999.
 * Public domain.
 */

#include <sys/cdefs.h>
#if defined(LIBC_SCCS) && !defined(lint)
__RCSID("$NetBSD: _inet_pton.c,v 1.3 2005/07/30 15:21:20 christos Exp $");
#endif /* LIBC_SCCS and not lint */

#if defined(__indr_reference) && !defined(__lint__)
__indr_reference(_inet_pton,inet_pton)
#else

#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>

extern int	_inet_pton __P((int, const char *, void *));

int
inet_pton(af, src, dst)
	int af;
	const char *src;
	void *dst;
{

	return _inet_pton(af, src, dst);
}
#endif
