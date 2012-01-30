/*	$NetBSD: getproto.c,v 1.7 2012/01/30 16:12:04 darrenr Exp $	*/

/*
 * Copyright (C) 2009 by Darren Reed.
 *
 * See the IPFILTER.LICENCE file for details on licencing.
 *
 * Id: getproto.c,v 1.10.2.1 2012/01/26 05:29:15 darrenr Exp
 */

#include "ipf.h"
#include <ctype.h>

int getproto(name)
	char *name;
{
	struct protoent *p;
	char *s;

	for (s = name; *s != '\0'; s++)
		if (!ISDIGIT(*s))
			break;
	if (*s == '\0')
		return atoi(name);

#ifdef _AIX51
	/*
	 * For some bogus reason, "ip" is 252 in /etc/protocols on AIX 5
	 * The IANA has doubled up on the definition of 0 - it is now also
	 * used for IPv6 hop-opts, so we can no longer rely on /etc/protocols
	 * providing the correct name->number mapping
	 */
#endif
	if (!strcasecmp(name, "ip"))
		return 0;

	p = getprotobyname(name);
	if (p != NULL)
		return p->p_proto;
	return -1;
}
