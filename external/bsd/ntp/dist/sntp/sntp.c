/*	$NetBSD: sntp.c,v 1.2 2014/12/19 20:43:18 christos Exp $	*/

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
