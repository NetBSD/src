/*	$NetBSD: ipf_perror.c,v 1.1.1.1.2.2 2012/04/17 00:03:17 yamt Exp $	*/

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
