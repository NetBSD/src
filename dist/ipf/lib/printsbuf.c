/*	$NetBSD: printsbuf.c,v 1.5 2012/01/30 16:12:04 darrenr Exp $	*/

/*
 * Copyright (C) 2009 by Darren Reed.
 *
 * See the IPFILTER.LICENCE file for details on licencing.
 *
 * Id: printsbuf.c,v 1.12.2.1 2012/01/26 05:29:17 darrenr Exp
 */

#ifdef	IPFILTER_SCAN

#include <ctype.h>
#include <stdio.h>
#include "ipf.h"
#include "netinet/ip_scan.h"

void
printsbuf(buf)
	char *buf;
{
	u_char *s;
	int i;

	for (s = (u_char *)buf, i = ISC_TLEN; i; i--, s++) {
		if (ISPRINT(*s))
			putchar(*s);
		else
			PRINTF("\\%o", *s);
	}
}
#else
void printsbuf(char *buf);

void printsbuf(buf)
	char *buf;
{
}
#endif
