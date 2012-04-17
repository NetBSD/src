/*	$NetBSD: mlo_rule.c,v 1.1.1.1.56.1 2012/04/17 00:02:24 yamt Exp $	*/

/*
 * Copyright (C) 1993-2001 by Darren Reed.
 *
 * See the IPFILTER.LICENCE file for details on licencing.
 *
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/conf.h>
#include <sys/proc.h>
#include <sys/ioctl.h>
#include <sys/kernel.h>
#include <sys/mbuf.h>
#include <sys/exec.h>
#include <sys/socket.h>
#include <net/if.h>
#include <netinet/in_systm.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <net/route.h>
#include <netinet/ip_var.h>
#include <netinet/tcp.h>
#include <netinet/tcpip.h>
#include <sys/lkm.h>
#include "ip_compat.h"
#include "ip_fil.h"
#include "ip_rules.h"


#ifdef	IPFILTER_LKM

static int ipfruleaction(struct lkm_table *, int);

int	ipfrule(struct lkm_table *, int, int);


MOD_MISC("IPFilter Rules");

int
ipfrule(struct lkm_table *lkmtp, int cmd, int ver)
{
	DISPATCH(lkmtp, cmd, ver, ipfruleaction, ipfruleaction, ipfruleaction);
}

int lkmexists(struct lkm_table *); /* defined in /sys/kern/kern_lkm.c */

static int
ipfruleaction(struct lkm_table *lkmtp, int cmd)
{
	int err = 0;

	switch (cmd)
	{
	case LKM_E_LOAD :
		if (lkmexists(lkmtp))
			return EEXIST;

		err = ipfrule_add();
		if (!err)
			fr_refcnt++;
		break;
	case LKM_E_UNLOAD :
		err = ipfrule_remove();
		if (!err)
			fr_refcnt--;
		break;
	case LKM_E_STAT :
		break;
	default:
		err = EIO;
		break;
	}
	return err;
}
#endif	/* IPFILTER_LKM */
