/*	$NetBSD: icmptypename.c,v 1.1.1.1.2.2 2012/04/17 00:03:17 yamt Exp $	*/

/*
 * Copyright (C) 2009 by Darren Reed.
 *
 * See the IPFILTER.LICENCE file for details on licencing.
 *
 * Id
 */
#include "ipf.h"

char *icmptypename(family, type)
	int family, type;
{
	icmptype_t *i;

	if ((type < 0) || (type > 255))
		return NULL;

	for (i = icmptypelist; i->it_name != NULL; i++) {
		if ((family == AF_INET) && (i->it_v4 == type))
			return i->it_name;
#ifdef USE_INET6
		if ((family == AF_INET6) && (i->it_v6 == type))
			return i->it_name;
#endif
	}

	return NULL;
}
