/*	$NetBSD: mln_rule.c,v 1.1.1.1.56.1 2012/04/17 00:02:24 yamt Exp $	*/

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


static int ipfruleaction(struct lkm_table *, int);

#ifdef IPFILTER_LKM
# if NetBSD >= 199706
int	ipfrule_lkmentry(struct lkm_table *, int, int);
# else
int	xxxinit(struct lkm_table *, int, int);
# endif


MOD_MISC("IPFilter Rules");

# if NetBSD >= 199706
int
ipfrule_lkmentry(lkmtp, cmd, ver)
# else
int
xxxinit(lkmtp, cmd, ver)
# endif
struct lkm_table *lkmtp;
int cmd, ver;
{
	DISPATCH(lkmtp, cmd, ver, ipfruleaction, ipfruleaction, ipfruleaction);
}

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
#endif /* IPFILTER_LKM */
