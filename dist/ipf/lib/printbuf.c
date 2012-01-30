/*	$NetBSD: printbuf.c,v 1.1.1.4 2012/01/30 16:03:24 darrenr Exp $	*/

/*
 * Copyright (C) 2009 by Darren Reed.
 *
 * See the IPFILTER.LICENCE file for details on licencing.
 *
 * Id: printbuf.c,v 1.10.2.1 2012/01/26 05:29:16 darrenr Exp
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
