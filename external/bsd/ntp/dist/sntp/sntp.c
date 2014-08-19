/*	$NetBSD: sntp.c,v 1.1.1.1.8.1 2014/08/19 23:51:45 tls Exp $	*/

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
