/*	$NetBSD: print-ah.c,v 1.2 1999/07/04 02:57:51 itojun Exp $	*/

/*
 * Copyright (c) 1988, 1989, 1990, 1991, 1993, 1994
 *	The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that: (1) source code distributions
 * retain the above copyright notice and this paragraph in its entirety, (2)
 * distributions including binary code include the above copyright notice and
 * this paragraph in its entirety in the documentation or other materials
 * provided with the distribution, and (3) all advertising materials mentioning
 * features or use of this software display the following acknowledgement:
 * ``This product includes software developed by the University of California,
 * Lawrence Berkeley Laboratory and its contributors.'' Neither the name of
 * the University nor the names of its contributors may be used to endorse
 * or promote products derived from this software without specific prior
 * written permission.
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 */

#ifndef lint
static const char rcsid[] =
    "@(#) /master/usr.sbin/tcpdump/tcpdump/print-icmp.c,v 2.1 1995/02/03 18:14:42 polk Exp (LBL)";
#endif

#include <sys/param.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>

#include <net/route.h>
#include <net/if.h>

#include <netinet/in.h>
#include <netinet/if_ether.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <netinet/ip_var.h>
#include <netinet/udp.h>
#include <netinet/udp_var.h>
#include <netinet/tcp.h>

#include <stdio.h>

#include <netinet6/ah.h>

#include "interface.h"
#include "addrtoname.h"

int
ah_print(register const u_char *bp, register const u_char *bp2)
{
	register const struct ah *ah;
	register const u_char *ep;
	int sumlen;
	u_int32_t spi;

	ah = (struct ah *)bp;
	ep = snapend;		/* 'ep' points to the end of avaible data. */

	if ((u_char *)(ah + 1) >= ep - sizeof(struct ah))
		goto trunc;

	sumlen = ah->ah_len << 2;
	spi = (u_int32_t)ntohl(ah->ah_spi);

	printf("AH(spi=%u", spi);
	if (vflag)
		printf(",sumlen=%d", sumlen);
	if (Rflag)
		printf(",seq=0x%x", (u_int32_t)ntohl(*(u_int32_t *)(ah + 1)));
	if (bp + sizeof(struct ah) + sumlen > ep)
		fputs("[truncated]", stdout);
	fputs("): ", stdout);
	
	return sizeof(struct ah) + sumlen;
 trunc:
	fputs("[|AH]", stdout);
	return 65535;
}
