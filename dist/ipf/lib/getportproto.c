/*	$NetBSD: getportproto.c,v 1.4 2004/11/13 19:16:10 he Exp $	*/

#include <ctype.h>
#include "ipf.h"

int getportproto(name, proto)
char *name;
int proto;
{
	struct servent *s;
	struct protoent *p;

	if (ISDIGIT(*name) && atoi(name) > 0)
		return htons(atoi(name) & 65535);

	p = getprotobynumber(proto);
	s = getservbyname(name, p ? p->p_name : NULL);
	if (s != NULL)
		return s->s_port;
	return 0;
}
