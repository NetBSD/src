/*	$NetBSD: icmptypename.c,v 1.1.1.1.2.3 2012/10/30 18:55:05 yamt Exp $	*/

/*
 * Copyright (C) 2012 by Darren Reed.
 *
 * See the IPFILTER.LICENCE file for details on licencing.
 *
 * Id: icmptypename.c,v 1.1.1.2 2012/07/22 13:44:38 darrenr Exp $
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
