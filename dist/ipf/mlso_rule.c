/*	$NetBSD: mlso_rule.c,v 1.1.1.1.56.1 2012/04/17 00:02:24 yamt Exp $	*/

/*
 * Copyright (C) 2000 by Darren Reed.
 *
 * See the IPFILTER.LICENCE file for details on licencing.
 */
#pragma ident "@(#)Id: mlso_rule.c,v 2.4 2002/08/10 14:00:15 darrenr Exp"

#include <sys/systm.h>
#include <sys/types.h>
#include <sys/param.h>
#include <sys/errno.h>
#include <sys/uio.h>
#include <sys/buf.h>
#include <sys/modctl.h>
#include <sys/open.h>
#include <sys/kmem.h>
#include <sys/conf.h>
#include <sys/cmn_err.h>
#include <sys/stat.h>
#include <sys/cred.h>
#include <sys/dditypes.h>
#include <sys/stream.h>
#include <sys/poll.h>
#include <sys/autoconf.h>
#include <sys/byteorder.h>
#include <sys/socket.h>
#include <sys/dlpi.h>
#include <sys/stropts.h>
#include <sys/sockio.h>
#include <net/if.h>
#if SOLARIS2 >= 6
# include <net/if_types.h>
#endif
#include <net/af.h>
#include <net/route.h>
#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/if_ether.h>
#include <netinet/ip.h>
#include <netinet/ip_var.h>
#include <netinet/tcp.h>
#include <netinet/udp.h>
#include <netinet/tcpip.h>
#include <netinet/ip_icmp.h>
#include <sys/ddi.h>
#include <sys/sunddi.h>
#include "ip_compat.h"
#include "ip_fil.h"
#include "ip_rules.h"

char	_depends_on[] = "drv/ipf";


extern	struct mod_ops		mod_miscops;
static	struct modlmisc		ipfrulemod = {
		&mod_miscops,
		"IP Filter rules"
};

static	struct modlinkage	modlink1 = {
		MODREV_1,
		&ipfrulemod,
		NULL
};



{
	int ipfruleinst;

	ipfruleinst = mod_install(&modlink1);
#ifdef	IPFRULEDEBUG
	cmn_err(CE_NOTE, "IP Filter Rules: _init() = %d", ipfruleinst);
#endif

	if (ipfruleinst == 0) {
		if (fr_running >= 0) {
			ipfruleinst = ipfrule_add();
			if (!ipfruleinst)
				fr_refcnt++;
			else {
				cmn_err(CE_NOTE,
					"IP Filter Rules: ipfrule_add failed");
				ipfruleinst = -1;
			}
		} else
			ipfruleinst = -1;
	}
	if (ipfruleinst == 0)
		cmn_err(CE_CONT, "IP Filter Rules: loaded\n");
	return ipfruleinst;
}



{
	int ipfruleinst;

	ipfruleinst = mod_remove(&modlink1);
#ifdef	IPFRULEDEBUG
	cmn_err(CE_NOTE, "IP Filter Rules: _fini() = %d", ipfruleinst);
#endif
	if (ipfruleinst == 0) {
		ipfruleinst = ipfrule_remove();
		if (!ipfruleinst)
			fr_refcnt--;
		else
			ipfruleinst = -1;
	}
	if (ipfruleinst == 0)
		cmn_err(CE_CONT, "IP Filter Rules: unloaded\n");
	return ipfruleinst;
}



struct modinfo *modinfop;
{
	int ipfruleinst;

	ipfruleinst = mod_info(&modlink1, modinfop);
#ifdef	IPFRULEDEBUG
	cmn_err(CE_NOTE, "IP Filter Rules: _info(%x) = %x",
		modinfop, ipfruleinst);
#endif
	return ipfruleinst;
}
