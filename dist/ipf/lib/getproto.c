/*	$NetBSD: getproto.c,v 1.8 2012/02/15 17:55:06 riz Exp $	*/

/*
 * Copyright (C) 2002-2005 by Darren Reed.
 *
 * See the IPFILTER.LICENCE file for details on licencing.
 *
 * Id: getproto.c,v 1.2.2.5 2009/12/27 06:58:06 darrenr Exp
 */

#include "ipf.h"

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
