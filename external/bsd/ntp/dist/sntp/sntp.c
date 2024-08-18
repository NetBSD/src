/*	$NetBSD: sntp.c,v 1.1.1.9 2024/08/18 20:37:40 christos Exp $	*/

#include <config.h>

#include "main.h"

const char * progname;

int 
main (
	int	argc,
	char **	argv
	) 
{
	return sntp_main(argc, argv, Version);
}
