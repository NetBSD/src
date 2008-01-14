/*	$NetBSD: ddp_usrreq.c,v 1.29 2008/01/14 04:12:40 dyoung Exp $	 */

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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: ddp_usrreq.c,v 1.29 2008/01/14 04:12:40 dyoung Exp $");

#include "opt_mbuftrace.h"

#include <sys/param.h>
#include <sys/errno.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/mbuf.h>
#include <sys/ioctl.h>
#include <sys/queue.h>
#include <sys/socket.h>
#include <sys/socketvar.h>
#include <sys/protosw.h>
#include <sys/kauth.h>
#include <net/if.h>
#include <net/route.h>
#include <net/if_ether.h>
#include <netinet/in.h>

#include <netatalk/at.h>
#include <netatalk/at_var.h>
#include <netatalk/ddp_var.h>
#include <netatalk/aarp.h>
#include <netatalk/at_extern.h>

static void at_pcbdisconnect __P((struct ddpcb *));
static void at_sockaddr __P((struct ddpcb *, struct mbuf *));
static int at_pcbsetaddr __P((struct ddpcb *, struct mbuf *, struct lwp *));
static int at_pcbconnect __P((struct ddpcb *, struct mbuf *, struct lwp *));
static void at_pcbdetach __P((struct socket *, struct ddpcb *));
static int at_pcballoc __P((struct socket *));

struct ifqueue atintrq1, atintrq2;
struct ddpcb   *ddp_ports[ATPORT_LAST];
struct ddpcb   *ddpcb = NULL;
struct ddpstat	ddpstat;
struct at_ifaddrhead at_ifaddr;		/* Here as inited in this file */
u_long ddp_sendspace = DDP_MAXSZ;	/* Max ddp size + 1 (ddp_type) */
u_long ddp_recvspace = 25 * (587 + sizeof(struct sockaddr_at));

#ifdef MBUFTRACE
struct mowner atalk_rx_mowner = MOWNER_INIT("atalk", "rx");
struct mowner atalk_tx_mowner = MOWNER_INIT("atalk", "tx");
#endif

/* ARGSUSED */
int
ddp_usrreq(so, req, m, addr, rights, l)
	struct socket  *so;
	int             req;
	struct mbuf    *m;
	struct mbuf    *addr;
	struct mbuf    *rights;
	struct lwp *l;
{
	struct ddpcb   *ddp;
	int             error = 0;

	ddp = sotoddpcb(so);

	if (req == PRU_CONTROL) {
		return (at_control((long) m, (void *) addr,
		    (struct ifnet *) rights, l));
	}
	if (req == PRU_PURGEIF) {
		at_purgeif((struct ifnet *) rights);
		return (0);
	}
	if (rights && rights->m_len) {
		error = EINVAL;
		goto release;
	}
	if (ddp == NULL && req != PRU_ATTACH) {
		error = EINVAL;
		goto release;
	}
	switch (req) {
	case PRU_ATTACH:
		if (ddp != NULL) {
			error = EINVAL;
			break;
		}
		if ((error = at_pcballoc(so)) != 0) {
			break;
		}
		error = soreserve(so, ddp_sendspace, ddp_recvspace);
		break;

	case PRU_DETACH:
		at_pcbdetach(so, ddp);
		break;

	case PRU_BIND:
		error = at_pcbsetaddr(ddp, addr, l);
		break;

	case PRU_SOCKADDR:
		at_sockaddr(ddp, addr);
		break;

	case PRU_CONNECT:
		if (ddp->ddp_fsat.sat_port != ATADDR_ANYPORT) {
			error = EISCONN;
			break;
		}
		error = at_pcbconnect(ddp, addr, l);
		if (error == 0)
			soisconnected(so);
		break;

	case PRU_DISCONNECT:
		if (ddp->ddp_fsat.sat_addr.s_node == ATADDR_ANYNODE) {
			error = ENOTCONN;
			break;
		}
		at_pcbdisconnect(ddp);
		soisdisconnected(so);
		break;

	case PRU_SHUTDOWN:
		socantsendmore(so);
		break;

	case PRU_SEND:{
			int s = 0;

			if (addr) {
				if (ddp->ddp_fsat.sat_port != ATADDR_ANYPORT) {
					error = EISCONN;
					break;
				}
				s = splnet();
				error = at_pcbconnect(ddp, addr, l);
				if (error) {
					splx(s);
					break;
				}
			} else {
				if (ddp->ddp_fsat.sat_port == ATADDR_ANYPORT) {
					error = ENOTCONN;
					break;
				}
			}

			error = ddp_output(m, ddp);
			m = NULL;
			if (addr) {
				at_pcbdisconnect(ddp);
				splx(s);
			}
		}
		break;

	case PRU_ABORT:
		soisdisconnected(so);
		at_pcbdetach(so, ddp);
		break;

	case PRU_LISTEN:
	case PRU_CONNECT2:
	case PRU_ACCEPT:
	case PRU_SENDOOB:
	case PRU_FASTTIMO:
	case PRU_SLOWTIMO:
	case PRU_PROTORCV:
	case PRU_PROTOSEND:
		error = EOPNOTSUPP;
		break;

	case PRU_RCVD:
	case PRU_RCVOOB:
		/*
		 * Don't mfree. Good architecture...
		 */
		return (EOPNOTSUPP);

	case PRU_SENSE:
		/*
		 * 1. Don't return block size.
		 * 2. Don't mfree.
		 */
		return (0);

	default:
		error = EOPNOTSUPP;
	}

release:
	if (m != NULL) {
		m_freem(m);
	}
	return (error);
}

static void
at_sockaddr(ddp, addr)
	struct ddpcb   *ddp;
	struct mbuf    *addr;
{
	struct sockaddr_at *sat;

	addr->m_len = sizeof(struct sockaddr_at);
	sat = mtod(addr, struct sockaddr_at *);
	*sat = ddp->ddp_lsat;
}

static int
at_pcbsetaddr(ddp, addr, l)
	struct ddpcb   *ddp;
	struct mbuf    *addr;
	struct lwp	*l;
{
	struct sockaddr_at lsat, *sat;
	struct at_ifaddr *aa;
	struct ddpcb   *ddpp;

	if (ddp->ddp_lsat.sat_port != ATADDR_ANYPORT) {	/* shouldn't be bound */
		return (EINVAL);
	}
	if (addr != 0) {	/* validate passed address */
		sat = mtod(addr, struct sockaddr_at *);
		if (addr->m_len != sizeof(*sat))
			return (EINVAL);

		if (sat->sat_family != AF_APPLETALK)
			return (EAFNOSUPPORT);

		if (sat->sat_addr.s_node != ATADDR_ANYNODE ||
		    sat->sat_addr.s_net != ATADDR_ANYNET) {
			TAILQ_FOREACH(aa, &at_ifaddr, aa_list) {
				if ((sat->sat_addr.s_net ==
				    AA_SAT(aa)->sat_addr.s_net) &&
				    (sat->sat_addr.s_node ==
				    AA_SAT(aa)->sat_addr.s_node))
					break;
			}
			if (!aa)
				return (EADDRNOTAVAIL);
		}
		if (sat->sat_port != ATADDR_ANYPORT) {
			if (sat->sat_port < ATPORT_FIRST ||
			    sat->sat_port >= ATPORT_LAST)
				return (EINVAL);

			if (sat->sat_port < ATPORT_RESERVED && l &&
			    kauth_authorize_generic(l->l_cred,
			    KAUTH_GENERIC_ISSUSER, NULL))
				return (EACCES);
		}
	} else {
		bzero((void *) & lsat, sizeof(struct sockaddr_at));
		lsat.sat_len = sizeof(struct sockaddr_at);
		lsat.sat_addr.s_node = ATADDR_ANYNODE;
		lsat.sat_addr.s_net = ATADDR_ANYNET;
		lsat.sat_family = AF_APPLETALK;
		sat = &lsat;
	}

	if (sat->sat_addr.s_node == ATADDR_ANYNODE &&
	    sat->sat_addr.s_net == ATADDR_ANYNET) {
		if (TAILQ_EMPTY(&at_ifaddr))
			return EADDRNOTAVAIL;
		sat->sat_addr = AA_SAT(TAILQ_FIRST(&at_ifaddr))->sat_addr;
	}
	ddp->ddp_lsat = *sat;

	/*
         * Choose port.
         */
	if (sat->sat_port == ATADDR_ANYPORT) {
		for (sat->sat_port = ATPORT_RESERVED;
		     sat->sat_port < ATPORT_LAST; sat->sat_port++) {
			if (ddp_ports[sat->sat_port - 1] == 0)
				break;
		}
		if (sat->sat_port == ATPORT_LAST) {
			return (EADDRNOTAVAIL);
		}
		ddp->ddp_lsat.sat_port = sat->sat_port;
		ddp_ports[sat->sat_port - 1] = ddp;
	} else {
		for (ddpp = ddp_ports[sat->sat_port - 1]; ddpp;
		     ddpp = ddpp->ddp_pnext) {
			if (ddpp->ddp_lsat.sat_addr.s_net ==
			    sat->sat_addr.s_net &&
			    ddpp->ddp_lsat.sat_addr.s_node ==
			    sat->sat_addr.s_node)
				break;
		}
		if (ddpp != NULL)
			return (EADDRINUSE);

		ddp->ddp_pnext = ddp_ports[sat->sat_port - 1];
		ddp_ports[sat->sat_port - 1] = ddp;
		if (ddp->ddp_pnext)
			ddp->ddp_pnext->ddp_pprev = ddp;
	}

	return 0;
}

static int
at_pcbconnect(ddp, addr, l)
	struct ddpcb   *ddp;
	struct mbuf    *addr;
	struct lwp     *l;
{
	struct rtentry *rt;
	const struct sockaddr_at *cdst;
	struct sockaddr_at *sat = mtod(addr, struct sockaddr_at *);
	struct route *ro;
	struct at_ifaddr *aa;
	struct ifnet   *ifp;
	u_short         hintnet = 0, net;

	if (addr->m_len != sizeof(*sat))
		return EINVAL;
	if (sat->sat_family != AF_APPLETALK) {
		return EAFNOSUPPORT;
	}
	/*
         * Under phase 2, network 0 means "the network".  We take "the
         * network" to mean the network the control block is bound to.
         * If the control block is not bound, there is an error.
         */
	if (sat->sat_addr.s_net == ATADDR_ANYNET
	    && sat->sat_addr.s_node != ATADDR_ANYNODE) {
		if (ddp->ddp_lsat.sat_port == ATADDR_ANYPORT) {
			return EADDRNOTAVAIL;
		}
		hintnet = ddp->ddp_lsat.sat_addr.s_net;
	}
	ro = &ddp->ddp_route;
	/*
         * If we've got an old route for this pcb, check that it is valid.
         * If we've changed our address, we may have an old "good looking"
         * route here.  Attempt to detect it.
         */
	if ((rt = rtcache_validate(ro)) != NULL ||
	    (rt = rtcache_update(ro, 1)) != NULL) {
		if (hintnet) {
			net = hintnet;
		} else {
			net = sat->sat_addr.s_net;
		}
		if ((ifp = rt->rt_ifp) != NULL) {
			TAILQ_FOREACH(aa, &at_ifaddr, aa_list) {
				if (aa->aa_ifp == ifp &&
				    ntohs(net) >= ntohs(aa->aa_firstnet) &&
				    ntohs(net) <= ntohs(aa->aa_lastnet)) {
					break;
				}
			}
		} else
			aa = NULL;
		cdst = satocsat(rtcache_getdst(ro));
		if (aa == NULL || (cdst->sat_addr.s_net !=
		    (hintnet ? hintnet : sat->sat_addr.s_net) ||
		    cdst->sat_addr.s_node != sat->sat_addr.s_node)) {
			rtcache_free(ro);
			rt = NULL;
		}
	}
	/*
         * If we've got no route for this interface, try to find one.
         */
	if (rt == NULL) {
		union {
			struct sockaddr		dst;
			struct sockaddr_at	dsta;
		} u;

		sockaddr_at_init(&u.dsta, &sat->sat_addr, 0);
		if (hintnet)
			u.dsta.sat_addr.s_net = hintnet;
		rtcache_setdst(ro, &u.dst);

		rt = rtcache_init(ro);
	}
	/*
         * Make sure any route that we have has a valid interface.
         */
	if (rt != NULL && (ifp = rt->rt_ifp) != NULL) {
		TAILQ_FOREACH(aa, &at_ifaddr, aa_list) {
			if (aa->aa_ifp == ifp)
				break;
		}
	} else
		aa = NULL;
	if (aa == NULL)
		return ENETUNREACH;
	ddp->ddp_fsat = *sat;
	if (ddp->ddp_lsat.sat_port == ATADDR_ANYPORT)
		return at_pcbsetaddr(ddp, NULL, l);
	return 0;
}

static void
at_pcbdisconnect(ddp)
	struct ddpcb   *ddp;
{
	ddp->ddp_fsat.sat_addr.s_net = ATADDR_ANYNET;
	ddp->ddp_fsat.sat_addr.s_node = ATADDR_ANYNODE;
	ddp->ddp_fsat.sat_port = ATADDR_ANYPORT;
}

static int
at_pcballoc(so)
	struct socket  *so;
{
	struct ddpcb   *ddp;

	MALLOC(ddp, struct ddpcb *, sizeof(*ddp), M_PCB, M_WAITOK|M_ZERO);
	if (!ddp)
		panic("at_pcballoc");
	ddp->ddp_lsat.sat_port = ATADDR_ANYPORT;

	ddp->ddp_next = ddpcb;
	ddp->ddp_prev = NULL;
	ddp->ddp_pprev = NULL;
	ddp->ddp_pnext = NULL;
	if (ddpcb) {
		ddpcb->ddp_prev = ddp;
	}
	ddpcb = ddp;

	ddp->ddp_socket = so;
	so->so_pcb = (void *) ddp;
#ifdef MBUFTRACE
	so->so_rcv.sb_mowner = &atalk_rx_mowner;
	so->so_snd.sb_mowner = &atalk_tx_mowner;
#endif
	return 0;
}

static void
at_pcbdetach(so, ddp)
	struct socket  *so;
	struct ddpcb   *ddp;
{
	soisdisconnected(so);
	so->so_pcb = 0;
	sofree(so);

	/* remove ddp from ddp_ports list */
	if (ddp->ddp_lsat.sat_port != ATADDR_ANYPORT &&
	    ddp_ports[ddp->ddp_lsat.sat_port - 1] != NULL) {
		if (ddp->ddp_pprev != NULL) {
			ddp->ddp_pprev->ddp_pnext = ddp->ddp_pnext;
		} else {
			ddp_ports[ddp->ddp_lsat.sat_port - 1] = ddp->ddp_pnext;
		}
		if (ddp->ddp_pnext != NULL) {
			ddp->ddp_pnext->ddp_pprev = ddp->ddp_pprev;
		}
	}
	rtcache_free(&ddp->ddp_route);
	if (ddp->ddp_prev) {
		ddp->ddp_prev->ddp_next = ddp->ddp_next;
	} else {
		ddpcb = ddp->ddp_next;
	}
	if (ddp->ddp_next) {
		ddp->ddp_next->ddp_prev = ddp->ddp_prev;
	}
	free(ddp, M_PCB);
}

/*
 * For the moment, this just find the pcb with the correct local address.
 * In the future, this will actually do some real searching, so we can use
 * the sender's address to do de-multiplexing on a single port to many
 * sockets (pcbs).
 */
struct ddpcb   *
ddp_search(
    struct sockaddr_at *from,
    struct sockaddr_at *to,
    struct at_ifaddr *aa)
{
	struct ddpcb   *ddp;

	/*
         * Check for bad ports.
         */
	if (to->sat_port < ATPORT_FIRST || to->sat_port >= ATPORT_LAST)
		return NULL;

	/*
         * Make sure the local address matches the sent address.  What about
         * the interface?
         */
	for (ddp = ddp_ports[to->sat_port - 1]; ddp; ddp = ddp->ddp_pnext) {
		/* XXX should we handle 0.YY? */

		/* XXXX.YY to socket on destination interface */
		if (to->sat_addr.s_net == ddp->ddp_lsat.sat_addr.s_net &&
		    to->sat_addr.s_node == ddp->ddp_lsat.sat_addr.s_node) {
			break;
		}
		/* 0.255 to socket on receiving interface */
		if (to->sat_addr.s_node == ATADDR_BCAST &&
		    (to->sat_addr.s_net == 0 ||
		    to->sat_addr.s_net == ddp->ddp_lsat.sat_addr.s_net) &&
		ddp->ddp_lsat.sat_addr.s_net == AA_SAT(aa)->sat_addr.s_net) {
			break;
		}
		/* XXXX.0 to socket on destination interface */
		if (to->sat_addr.s_net == aa->aa_firstnet &&
		    to->sat_addr.s_node == 0 &&
		    ntohs(ddp->ddp_lsat.sat_addr.s_net) >=
		    ntohs(aa->aa_firstnet) &&
		    ntohs(ddp->ddp_lsat.sat_addr.s_net) <=
		    ntohs(aa->aa_lastnet)) {
			break;
		}
	}
	return (ddp);
}

/*
 * Initialize all the ddp & appletalk stuff
 */
void
ddp_init()
{
	TAILQ_INIT(&at_ifaddr);
	atintrq1.ifq_maxlen = IFQ_MAXLEN;
	atintrq2.ifq_maxlen = IFQ_MAXLEN;

	MOWNER_ATTACH(&atalk_tx_mowner);
	MOWNER_ATTACH(&atalk_rx_mowner);
}

#if 0
static void
ddp_clean()
{
	struct ddpcb   *ddp;

	for (ddp = ddpcb; ddp; ddp = ddp->ddp_next)
		at_pcbdetach(ddp->ddp_socket, ddp);
}
#endif
