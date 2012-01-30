/*	$NetBSD: getportproto.c,v 1.8 2012/01/30 16:12:04 darrenr Exp $	*/

/*
 * Copyright (C) 2009 by Darren Reed.
 *
 * See the IPFILTER.LICENCE file for details on licencing.
 *
 * Id: getportproto.c,v 1.9.2.1 2012/01/26 05:29:15 darrenr Exp
 */

#include <ctype.h>
#include "ipf.h"

int getportproto(name, proto)
	char *name;
	int proto;
{
	struct servent *s;
	struct protoent *p;

	if (ISDIGIT(*name)) {
		int number;
		char *s;

		for (s = name; *s != '\0'; s++)
			if (!ISDIGIT(*s))
				return -1;

		number = atoi(name);
		if (number < 0 || number > 65535)
			return -1;
		return htons(number);
	}

	p = getprotobynumber(proto);
	s = getservbyname(name, p ? p->p_name : NULL);
	if (s != NULL)
		return s->s_port;
	return -1;
}
