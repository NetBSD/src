/*	$NetBSD: printsbuf.c,v 1.6 2012/02/15 17:55:07 riz Exp $	*/

/*
 * Copyright (C) 2002-2004 by Darren Reed.
 *
 * See the IPFILTER.LICENCE file for details on licencing.
 *
 * Id: printsbuf.c,v 1.2.4.3 2009/12/27 06:58:07 darrenr Exp
 */

#ifdef	IPFILTER_SCAN

#include <ctype.h>
#include <stdio.h>
#include "ipf.h"
#include "netinet/ip_scan.h"

void printsbuf(buf)
char *buf;
{
	u_char *s;
	int i;

	for (s = (u_char *)buf, i = ISC_TLEN; i; i--, s++) {
		if (ISPRINT(*s))
			putchar(*s);
		else
			printf("\\%o", *s);
	}
}

#endif
