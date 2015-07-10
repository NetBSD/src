/*	$NetBSD: longsize.c,v 1.1.1.3 2015/07/10 13:11:14 christos Exp $	*/

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
