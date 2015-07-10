/*	$NetBSD: sntp.c,v 1.1.1.4 2015/07/10 13:11:09 christos Exp $	*/

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
