/*	$NetBSD: sntp.c,v 1.1.1.1.4.3 2014/05/22 15:50:12 yamt Exp $	*/

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
