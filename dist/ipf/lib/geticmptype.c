/*	$NetBSD$	*/

/*
 * Copyright (C) 2009 by Darren Reed.
 *
 * See the IPFILTER.LICENCE file for details on licencing.
 *
 * Id: geticmptype.c,v 1.4.2.1 2012/01/26 05:29:15 darrenr Exp
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
