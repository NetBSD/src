/*	$NetBSD: printfraginfo.c,v 1.2.2.1 2014/08/19 23:46:48 tls Exp $	*/

/*
 * Copyright (C) 2012 by Darren Reed.
 *
 * See the IPFILTER.LICENCE file for details on licencing.
 *
 * Id: printfraginfo.c,v 1.1.1.2 2012/07/22 13:44:40 darrenr Exp $
 */
#include "ipf.h"
#include "kmem.h"


void
printfraginfo(prefix, ifr)
	char *prefix;
	struct ipfr *ifr;
{
	int family;

	PRINTF("%s", prefix);
	if (ifr->ipfr_v == 6) {
		PRINTF("inet6");
		family = AF_INET6;
	} else {
		PRINTF("inet");
		family = AF_INET;
	}

	PRINTF(" %s -> ", hostname(family, &ifr->ipfr_src));
/*
	frentry_t fr;
	fr.fr_flags = 0xffffffff;
	if (kmemcpy((char *)&fr, (u_long)ifr->ipfr_rule,
		    sizeof(fr)) == -1)
		return;
 */
	PRINTF("%s id %x ttl %lu pr %d pkts %u bytes %u seen0 %d ref %d\n",
		hostname(family, &ifr->ipfr_dst), ifr->ipfr_id,
		ifr->ipfr_ttl, ifr->ipfr_p, ifr->ipfr_pkts, ifr->ipfr_bytes,
		ifr->ipfr_seen0, ifr->ipfr_ref);
}
