/*	$NetBSD: sntp.c,v 1.3 2015/07/10 14:20:33 christos Exp $	*/

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
