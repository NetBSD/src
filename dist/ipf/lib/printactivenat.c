/*	$NetBSD: printactivenat.c,v 1.1.1.1 2004/03/28 08:56:19 martti Exp $	*/

/*
 * Copyright (C) 1993-2001 by Darren Reed.
 *
 * See the IPFILTER.LICENCE file for details on licencing.
 *
 * Added redirect stuff and a variety of bug fixes. (mcn@EnGarde.com)
 */

#include "ipf.h"


#if !defined(lint)
static const char rcsid[] = "@(#)Id: printactivenat.c,v 1.3 2004/01/17 17:31:20 darrenr Exp";
#endif


void printactivenat(nat, opts)
nat_t *nat;
int opts;
{

	printf("%s", getnattype(nat->nat_ptr));

	if (nat->nat_flags & SI_CLONE)
		printf(" CLONE");

	printf(" %-15s", inet_ntoa(nat->nat_inip));

	if ((nat->nat_flags & IPN_TCPUDP) != 0)
		printf(" %-5hu", ntohs(nat->nat_inport));

	printf(" <- -> %-15s",inet_ntoa(nat->nat_outip));

	if ((nat->nat_flags & IPN_TCPUDP) != 0)
		printf(" %-5hu", ntohs(nat->nat_outport));

	printf(" [%s", inet_ntoa(nat->nat_oip));
	if ((nat->nat_flags & IPN_TCPUDP) != 0)
		printf(" %hu", ntohs(nat->nat_oport));
	printf("]");

	if (opts & OPT_VERBOSE) {
		printf("\n\tage %lu use %hu sumd %s/",
			nat->nat_age, nat->nat_use, getsumd(nat->nat_sumd[0]));
		printf("%s pr %u bkt %d/%d flags %x\n",
			getsumd(nat->nat_sumd[1]), nat->nat_p,
			nat->nat_hv[0], nat->nat_hv[1], nat->nat_flags);
		printf("\tifp %s", getifname(nat->nat_ifps[0]));
		printf(",%s ", getifname(nat->nat_ifps[1]));
#ifdef	USE_QUAD_T
		printf("bytes %qu/%qu pkts %qu/%qu",
			(unsigned long long)nat->nat_bytes[0],
			(unsigned long long)nat->nat_bytes[1],
			(unsigned long long)nat->nat_pkts[0],
			(unsigned long long)nat->nat_pkts[1]);
#else
		printf("bytes %lu/%lu pkts %lu/%lu", nat->nat_bytes[0],
			nat->nat_bytes[1], nat->nat_pkts[0], nat->nat_pkts[1]);
#endif
#if SOLARIS
		printf(" %lx", nat->nat_ipsumd);
#endif
	}

	putchar('\n');
	if (nat->nat_aps)
		printaps(nat->nat_aps, opts);
}
