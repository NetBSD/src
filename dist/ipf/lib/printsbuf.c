/*	$NetBSD: printsbuf.c,v 1.1.1.1 2004/03/28 08:56:20 martti Exp $	*/

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
		if (isprint(*s))
			putchar(*s);
		else
			printf("\\%o", *s);
	}
}

#endif
