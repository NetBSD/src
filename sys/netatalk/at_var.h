/*	$NetBSD: at_var.h,v 1.2.6.1 2002/06/20 03:48:25 nathanw Exp $	 */

/*
 * Copyright (c) 1990,1991 Regents of The University of Michigan.
 * All Rights Reserved.
 *
 * Permission to use, copy, modify, and distribute this software and
 * its documentation for any purpose and without fee is hereby granted,
 * provided that the above copyright notice appears in all copies and
 * that both that copyright notice and this permission notice appear
 * in supporting documentation, and that the name of The University
 * of Michigan not be used in advertising or publicity pertaining to
 * distribution of the software without specific, written prior
 * permission. This software is supplied as is without expressed or
 * implied warranties of any kind.
 *
 * This product includes software developed by the University of
 * California, Berkeley and its contributors.
 *
 *	Research Systems Unix Group
 *	The University of Michigan
 *	c/o Wesley Craig
 *	535 W. William Street
 *	Ann Arbor, Michigan
 *	+1-313-764-2278
 *	netatalk@umich.edu
 */

#ifndef _NETATALK_AT_VAR_H_
#define _NETATALK_AT_VAR_H_ 1

#include <sys/callout.h>

/*
 * For phase2, we need to keep not only our address on an interface,
 * but also the legal networks on the interface.
 */
struct at_ifaddr {
	struct ifaddr   aa_ifa;
#define aa_ifp		aa_ifa.ifa_ifp
	struct sockaddr_at aa_addr;
	struct sockaddr_at aa_broadaddr;
#define aa_dstaddr	aa_broadaddr;
	struct sockaddr_at aa_netmask;
	int             aa_flags;
	u_short         aa_firstnet, aa_lastnet;
	int             aa_probcnt;
	TAILQ_ENTRY(at_ifaddr) aa_list;	/* list of appletalk addresses */
	struct callout	aa_probe_ch;	/* for aarpprobe() */
};

struct at_aliasreq {
	char			ifra_name[IFNAMSIZ];
	struct sockaddr_at	ifra_addr;
	struct sockaddr_at	ifra_broadaddr;
#define ifra_dstaddr		ifra_broadaddr
	struct sockaddr_at	ifra_mask;
};

#define AA_SAT(aa) \
    (&(aa->aa_addr))
#define satosat(sa)	((struct sockaddr_at *)(sa))

#define AFA_ROUTE	0x0001
#define AFA_PROBING	0x0002
#define AFA_PHASE2	0x0004

#ifdef _KERNEL
TAILQ_HEAD(at_ifaddrhead, at_ifaddr);
extern struct at_ifaddrhead at_ifaddr;
extern struct ifqueue atintrq1, atintrq2;
#endif

#endif /* _NETATALK_AT_VAR_H_ */
