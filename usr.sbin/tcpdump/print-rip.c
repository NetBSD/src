/*	$NetBSD: print-rip.c,v 1.5 1996/11/04 21:33:02 christos Exp $	*/

/*
 * Copyright (c) 1989, 1990, 1991, 1993, 1994
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
static char rcsid[] =
    "@(#) Header: print-rip.c,v 1.20 94/06/14 20:18:47 leres Exp (LBL)";
#endif

#include <sys/param.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>

#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include <netinet/ip_var.h>
#include <netinet/udp.h>
#include <netinet/udp_var.h>

#include <protocols/routed.h>

#include <errno.h>
#include <stdio.h>

#include "interface.h"
#include "addrtoname.h"

static void
rip_entry_print(register const struct netinfo *ni)
{
	if (ntohs(ni->rip_family) != AF_INET) {
		register int i;

		printf(" [family %d:", ntohs(ni->rip_family));
		printf(" %04x", ni->rip_dst);
		printf("]");
	} else {
		struct sockaddr_in sin;
		sin.sin_addr.s_addr = ni->rip_dst;
		printf(" %s", ipaddr_string(&sin.sin_addr));

		/* 
		 * In RIP V1 the dst_mask and next hop fields are
		 * supposed to be 0. If they are not, we assume that we are
		 * dealing with a V2 packet. Some routers (eg. annexes),
		 * in compatibility mode, advertize V1 packets with the V2
		 * fields filled.
		 */
		if (ni->rip_dst_mask != 0) {
			u_int32_t mask = ni->rip_dst_mask;
			int bits = 32;

			while (mask)
				bits--, mask >>= 1;
			printf("/%d", bits);
		}

		if (ni->rip_router != 0) {
			sin.sin_addr.s_addr = ni->rip_router;
			printf(" -> %s", ipaddr_string(&sin.sin_addr));
		}

		if (ni->rip_tag)
			printf(" [port %d]", ni->rip_tag);
	}
	printf("(%d)", ntohl(ni->rip_metric));
}

void
rip_print(const u_char *dat, int length)
{
	register const struct rip *rp = (struct rip *)dat;
	register const struct netinfo *ni;
	register int amt = snapend - dat;
	register int i = min(length, amt) -
			 (sizeof(struct rip) - sizeof(struct netinfo));
	int j;
	int trunc;

	if (i < 0)
		return;

	switch (rp->rip_cmd) {

	case RIPCMD_REQUEST:
		printf(" rip-req %d", length);
		break;
	case RIPCMD_RESPONSE:
		j = length / sizeof(*ni);
		if (j * sizeof(*ni) != length - 4)
			printf(" rip-resp %d[%d]:", j, length);
		else
			printf(" rip-resp %d:", j);
		trunc = ((i / sizeof(*ni)) * sizeof(*ni) != i);
		for (ni = rp->rip_nets; (i -= sizeof(*ni)) >= 0; ++ni)
			rip_entry_print(ni);
		if (trunc)
			printf("[|rip]");
		break;
	case RIPCMD_TRACEON:
		printf(" rip-traceon %d: \"%s\"", length, rp->rip_tracefile);
		break;
	case RIPCMD_TRACEOFF:
		printf(" rip-traceoff %d", length);
		break;
	case RIPCMD_POLL:
		printf(" rip-poll %d", length);
		break;
	case RIPCMD_POLLENTRY:
		printf(" rip-pollentry %d", length);
		break;
	default:
		printf(" rip-%d ?? %d", rp->rip_cmd, length);
		break;
	}
	if (rp->rip_vers != RIP_VERSION_1)
		printf(" [vers %d]", rp->rip_vers);
}
