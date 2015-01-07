/*	$NetBSD: sntp.c,v 1.2.6.2 2015/01/07 12:13:33 msaitoh Exp $	*/

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
