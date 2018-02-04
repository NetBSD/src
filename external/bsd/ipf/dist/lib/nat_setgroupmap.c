/*	$NetBSD: nat_setgroupmap.c,v 1.3 2018/02/04 08:19:42 mrg Exp $	*/

/*
 * Copyright (C) 2012 by Darren Reed.
 *
 * See the IPFILTER.LICENCE file for details on licencing.
 */
#if !defined(lint)
static __attribute__((__used__)) const char rcsid[] = "@(#)Id: nat_setgroupmap.c,v 1.1.1.2 2012/07/22 13:44:39 darrenr Exp $";
#endif

#include "ipf.h"

void nat_setgroupmap(n)
	ipnat_t *n;
{
	if (n->in_nsrcmsk == n->in_osrcmsk)
		n->in_ippip = 1;
	else if (n->in_flags & IPN_AUTOPORTMAP) {
		n->in_ippip = ~ntohl(n->in_osrcmsk);
		if (n->in_nsrcmsk != 0xffffffff)
			n->in_ippip /= (~ntohl(n->in_nsrcmsk) + 1);
		n->in_ippip++;
		if (n->in_ippip == 0)
			n->in_ippip = 1;
		n->in_ppip = USABLE_PORTS / n->in_ippip;
	} else {
		n->in_space = USABLE_PORTS * ~ntohl(n->in_nsrcmsk);
		n->in_snip = 0;
		if (!(n->in_ppip = n->in_spmin))
			n->in_ppip = 1;
		n->in_ippip = USABLE_PORTS / n->in_ppip;
	}
}
