/*	$NetBSD: sntp.c,v 1.5.8.1 2025/08/02 05:22:48 perseant Exp $	*/

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
