/*	$NetBSD: getproto.c,v 1.5 2008/05/20 07:08:07 darrenr Exp $	*/

/*
 * Copyright (C) 2002-2005 by Darren Reed.
 * 
 * See the IPFILTER.LICENCE file for details on licencing.  
 *   
 * Id: getproto.c,v 1.2.2.4 2007/10/27 16:03:38 darrenr Exp 
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
