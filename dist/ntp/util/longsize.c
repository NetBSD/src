/*	$NetBSD: longsize.c,v 1.1.1.1 2000/03/29 12:38:59 simonb Exp $	*/

#include <stdio.h>

main()
{
	if (sizeof(long) == 8) { 
		printf("-DLONG8\n");
	} else if (sizeof(long) == 4) {
		printf("-DLONG4\n");
	}
	exit(0);
}
