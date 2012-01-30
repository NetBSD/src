/*	$NetBSD: binprint.c,v 1.1.1.3 2012/01/30 16:03:25 darrenr Exp $	*/

/*
 * Copyright (C) 2009 by Darren Reed.
 *
 * See the IPFILTER.LICENCE file for details on licencing.
 *
 * Id: binprint.c,v 1.11.2.1 2012/01/26 05:29:15 darrenr Exp
 */

#include "ipf.h"


void binprint(ptr, size)
	void *ptr;
	size_t size;
{
	u_char *s;
	int i, j;

	for (i = size, j = 0, s = (u_char *)ptr; i; i--, s++) {
		j++;
		printf("%02x ", *s);
		if (j == 16) {
			printf("\n");
			j = 0;
		}
	}
	putchar('\n');
	(void)fflush(stdout);
}
