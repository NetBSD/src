/*	$NetBSD: sntp.c,v 1.1.1.1.10.1 2014/12/25 02:28:14 snj Exp $	*/

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
