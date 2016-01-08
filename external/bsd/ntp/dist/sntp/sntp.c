/*	$NetBSD: sntp.c,v 1.1.1.6 2016/01/08 21:21:29 christos Exp $	*/

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
