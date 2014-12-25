/*	$NetBSD: sntp.c,v 1.1.1.1.2.1 2014/12/25 02:34:42 snj Exp $	*/

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
