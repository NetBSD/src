/*	$NetBSD: vtof.c,v 1.1.1.1.2.2 2012/04/17 00:03:22 yamt Exp $	*/

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
