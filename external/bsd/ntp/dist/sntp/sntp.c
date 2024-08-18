/*	$NetBSD: sntp.c,v 1.6 2024/08/18 20:47:20 christos Exp $	*/

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
