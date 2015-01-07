/*	$NetBSD: sntp.c,v 1.2.2.2 2015/01/07 04:45:35 msaitoh Exp $	*/

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
