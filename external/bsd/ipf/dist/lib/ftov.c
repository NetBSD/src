/*	$NetBSD: ftov.c,v 1.1.1.1.2.2 2012/04/17 00:03:16 yamt Exp $	*/

#include "ipf.h"

int
ftov(version)
	int version;
{
#ifdef USE_INET6
	if (version == AF_INET6)
		return 6;
#endif
	if (version == AF_INET)
		return 4;
	if (version == AF_UNSPEC)
		return 0;
	return -1;
}
