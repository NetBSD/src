/*	$NetBSD: getportproto.c,v 1.1.1.1 2004/03/28 08:56:18 martti Exp $	*/

#include <ctype.h>
#include "ipf.h"

int getportproto(name, proto)
char *name;
int proto;
{
	struct servent *s;
	struct protoent *p;

	if (isdigit(*name) && atoi(name) > 0)
		return htons(atoi(name) & 65535);

	p = getprotobynumber(proto);
	if (p != NULL) {
		s = getservbyname(name, p->p_name);
		if (s != NULL)
			return s->s_port;
	}
	return 0;
}
