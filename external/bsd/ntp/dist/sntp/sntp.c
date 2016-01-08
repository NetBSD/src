/*	$NetBSD: sntp.c,v 1.4 2016/01/08 21:35:40 christos Exp $	*/

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
