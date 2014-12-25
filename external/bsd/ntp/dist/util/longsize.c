/*	$NetBSD: longsize.c,v 1.1.1.1.8.1 2014/12/25 02:34:48 snj Exp $	*/

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
