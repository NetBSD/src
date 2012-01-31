/*	$NetBSD: sntp.c,v 1.1.1.1 2012/01/31 21:27:34 kardel Exp $	*/

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
