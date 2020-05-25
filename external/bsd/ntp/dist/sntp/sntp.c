/*	$NetBSD: sntp.c,v 1.5 2020/05/25 20:47:32 christos Exp $	*/

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
