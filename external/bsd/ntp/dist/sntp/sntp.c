/*	$NetBSD: sntp.c,v 1.2.4.2 2015/01/07 10:10:21 msaitoh Exp $	*/

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
