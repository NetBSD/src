/*	$NetBSD: binprint.c,v 1.1.1.1.2.3 2012/10/30 18:55:03 yamt Exp $	*/

/*
 * Copyright (C) 2012 by Darren Reed.
 *
 * See the IPFILTER.LICENCE file for details on licencing.
 *
 * Id: binprint.c,v 1.1.1.2 2012/07/22 13:44:38 darrenr Exp $
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
