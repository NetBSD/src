/*	$NetBSD: _inet_pton.c,v 1.1 1999/09/15 14:21:03 kleink Exp $	*/

/*
 * Written by Klaus Klein, September 14, 1999.
 * Public domain.
 */

#include <sys/cdefs.h>

#ifdef __indr_reference
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
