/*	$NetBSD: at_proto.c,v 1.2 1999/01/14 01:19:29 thorpej Exp $	*/

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

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/protosw.h>
#include <sys/domain.h>
#include <sys/types.h>
#include <sys/socket.h>

#include <sys/kernel.h>
#include <net/if.h>
#include <net/radix.h>
#include <net/if_ether.h>
#include <netinet/in.h>
#include <net/route.h>

#include <netatalk/at.h>
#include <netatalk/ddp.h>
#include <netatalk/at_var.h>
#include <netatalk/ddp_var.h>
#include <netatalk/at_extern.h>


struct protosw		atalksw[] = {
    {
	/* Identifiers */
	SOCK_DGRAM,	&atalkdomain,	ATPROTO_DDP,	PR_ATOMIC|PR_ADDR,
	/*
	 * protocol-protocol interface.
	 * fields are pr_input, pr_output, pr_ctlinput, and pr_ctloutput.
	 * pr_input can be called from the udp protocol stack for iptalk
	 * packets bound for a local socket.
	 * pr_output can be used by higher level appletalk protocols, should
	 * they be included in the kernel.
	 */
	0,		ddp_output,	0,		0,
	/* socket-protocol interface. */
	ddp_usrreq,
	/* utility routines. */
	ddp_init,	0,		0,		0,
    },
};

struct domain		atalkdomain = {
    PF_APPLETALK,	"appletalk",	0,	0,	0,
    atalksw, &atalksw[sizeof(atalksw)/sizeof(atalksw[0])],
    0, rn_inithead,
    32,
    sizeof(struct sockaddr_at)
};


