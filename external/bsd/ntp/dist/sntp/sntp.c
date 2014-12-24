/*	$NetBSD: sntp.c,v 1.1.1.2.4.1 2014/12/24 00:05:24 riz Exp $	*/

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
