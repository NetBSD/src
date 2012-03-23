/*	$NetBSD: vtof.c,v 1.1.1.1 2012/03/23 21:20:10 christos Exp $	*/

#include "ipf.h"

int
vtof(version)
	int version;
{
#ifdef USE_INET6
	if (version == 6)
		return AF_INET6;
#endif
	if (version == 4)
		return AF_INET;
	if (version == 0)
		return AF_UNSPEC;
	return -1;
}
