/*	$NetBSD: sntp.c,v 1.1.1.8 2020/05/25 20:40:11 christos Exp $	*/

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
