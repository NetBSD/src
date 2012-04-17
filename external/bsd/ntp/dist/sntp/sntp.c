/*	$NetBSD: sntp.c,v 1.1.1.1.4.2 2012/04/17 00:03:51 yamt Exp $	*/

#include <config.h>

#include "main.h"

volatile int debug;

int 
main (
	int argc,
	char **argv
	) 
{
	return sntp_main(argc, argv);
}
