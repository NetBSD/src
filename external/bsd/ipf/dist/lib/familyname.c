/*	$NetBSD: familyname.c,v 1.1.1.1.2.2 2012/04/17 00:03:16 yamt Exp $	*/

#include "ipf.h"

const char *familyname(int family)
{
	if (family == AF_INET)
		return "inet";
#ifdef AF_INET6
	if (family == AF_INET6)
		return "inet6";
#endif
	return "unknown";
}
