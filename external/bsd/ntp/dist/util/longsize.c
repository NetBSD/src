/*	$NetBSD: longsize.c,v 1.1.1.5 2016/01/08 21:21:33 christos Exp $	*/

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
