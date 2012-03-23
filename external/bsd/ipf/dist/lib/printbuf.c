/*	$NetBSD: printbuf.c,v 1.1.1.1 2012/03/23 21:20:09 christos Exp $	*/

/*
 * Copyright (C) 2009 by Darren Reed.
 *
 * See the IPFILTER.LICENCE file for details on licencing.
 *
 * Id
 */

#include <ctype.h>

#include "ipf.h"


void
printbuf(buf, len, zend)
	char *buf;
	int len, zend;
{
	char *s, c;
	int i;

	for (s = buf, i = len; i; i--) {
		c = *s++;
		if (ISPRINT(c))
			putchar(c);
		else
			PRINTF("\\%03o", c);
		if ((c == '\0') && zend)
			break;
	}
}
