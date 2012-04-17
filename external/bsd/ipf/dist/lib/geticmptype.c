/*	$NetBSD: geticmptype.c,v 1.1.1.1.2.2 2012/04/17 00:03:16 yamt Exp $	*/

/*
 * Copyright (C) 2009 by Darren Reed.
 *
 * See the IPFILTER.LICENCE file for details on licencing.
 *
 * Id
 */
#include "ipf.h"

int geticmptype(family, name)
	int family;
	char *name;
{
	icmptype_t *i;

	for (i = icmptypelist; i->it_name != NULL; i++) {
		if (!strcmp(name, i->it_name)) {
			if (family == AF_INET)
				return i->it_v4;
#ifdef USE_INET6
			if (family == AF_INET6)
				return i->it_v6;
#endif
			return -1;
		}
	}

	return -1;
}
