/*	$NetBSD: getport.c,v 1.1.1.1 2004/03/28 08:56:18 martti Exp $	*/

#include "ipf.h"

int getport(name)
char *name;
{
	struct servent *s;

	s = getservbyname(name, NULL);
	if (s != NULL)
		return s->s_port;
	return 0;
}
