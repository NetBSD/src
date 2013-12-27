/*	$NetBSD: sntp.c,v 1.1.1.2 2013/12/27 23:31:10 christos Exp $	*/

#include <config.h>

#include "main.h"

int 
main (
	int	argc,
	char **	argv
	) 
{
	return sntp_main(argc, argv, Version);
}
