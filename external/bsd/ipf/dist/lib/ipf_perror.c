/*	$NetBSD: ipf_perror.c,v 1.1.1.1 2012/03/23 21:20:08 christos Exp $	*/

#include "ipf.h"

void
ipf_perror(err, string)
	int err;
	char *string;
{
	if (err == 0)
		fprintf(stderr, "%s\n", string);
	else
		fprintf(stderr, "%s %s\n", string, ipf_strerror(err));
}
