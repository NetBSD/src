/*	$NetBSD: if_vlan.c,v 1.40 2004/04/23 02:58:29 simonb Exp $	*/

/*-
 * Copyright (c) 2000, 2001 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Andrew Doran, and by Jason R. Thorpe of Zembu Labs, Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the NetBSD
 *	Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * Copyright 1998 Massachusetts Institute of Technology
 *
 * Permission to use, copy, modify, and distribute this software and
 * its documentation for any purpose and without fee is hereby
 * granted, provided that both the above copyright notice and this
 * permission notice appear in all copies, that both the above
 * copyright notice and this permission notice appear in all
 * supporting documentation, and that the name of M.I.T. not be used
 * in advertising or publicity pertaining to distribution of the
 * software without specific, written prior permission.  M.I.T. makes
 * no representations about the suitability of this software for any
 * purpose.  It is provided "as is" without express or implied
 * warranty.
 * 
 * THIS SOFTWARE IS PROVIDED BY M.I.T. ``AS IS''.  M.I.T. DISCLAIMS
 * ALL EXPRESS OR IMPLIED WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE. IN NO EVENT
 * SHALL M.I.T. BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF
 * USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * from FreeBSD: if_vlan.c,v 1.16 2000/03/26 15:21:40 charnier Exp
 * via OpenBSD: if_vlan.c,v 1.4 2000/05/15 19:15:00 chris Exp
 */

/*
 * if_vlan.c - pseudo-device driver for IEEE 802.1Q virtual LANs.  Might be
 * extended some day to also handle IEEE 802.1P priority tagging.  This is
 * sort of sneaky in the implementation, since we need to pretend to be
 * enough of an Ethernet implementation to make ARP work.  The way we do
 * this is by telling everyone that we are an Ethernet interface, and then
 * catch the packets that ether_output() left on our output queue when it
 * calls if_start(), rewrite them for use by the real outgoing interface,
 * and ask it to send them.
 *
 * TODO:
 *
 *	- Need some way to notify vlan interfaces when the parent
 *	  interface changes MTU.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: if_vlan.c,v 1.40 2004/04/23 02:58:29 simonb Exp $");

#include "opt_inet.h"
#include "bpfilter.h"

#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/mbuf.h>
#include <sys/queue.h>
#include <sys/socket.h>
#include <sys/sockio.h>
#include <sys/systm.h>
#include <sys/proc.h>

#if NBPFILTER > 0
#include <net/bpf.h>
#endif
#include <net/if.h>
#include <net/if_dl.h>
#include <net/if_types.h>
#include <net/if_ether.h>
#include <net/if_vlanvar.h>

#ifdef INET
#include <netinet/in.h>
#include <netinet/if_inarp.h>
#endif

struct vlan_mc_entry {
	LIST_ENTRY(vlan_mc_entry)	mc_entries;
	/*
	 * A key to identify this entry.  The mc_addr below can't be
	 * used since multiple sockaddr may mapped into the same
	 * ether_multi (e.g., AF_UNSPEC).
	 */
	union {
		struct ether_multi	*mcu_enm;
	} mc_u;
	struct sockaddr_storage		mc_addr;
};

#define	mc_enm		mc_u.mcu_enm

struct ifvlan {
	union {
		struct ethercom ifvu_ec;
	} ifv_u;
	struct ifnet *ifv_p;	/* parent interface of this vlan */
	struct ifv_linkmib {
		const struct vlan_multisw *ifvm_msw;
		int	ifvm_encaplen;	/* encapsulation length */
		int	ifvm_mtufudge;	/* MTU fudged by this much */
		int	ifvm_mintu;	/* min transmission unit */
		u_int16_t ifvm_proto;	/* encapsulation ethertype */
		u_int16_t ifvm_tag;	/* tag to apply on packets */
	} ifv_mib;
	LIST_HEAD(__vlan_mchead, vlan_mc_entry) ifv_mc_listhead;
	LIST_ENTRY(ifvlan) ifv_list;
	int ifv_flags;
};

#define	IFVF_PROMISC	0x01		/* promiscuous mode enabled */

#define	ifv_ec		ifv_u.ifvu_ec

#define	ifv_if		ifv_ec.ec_if

#define	ifv_msw		ifv_mib.ifvm_msw
#define	ifv_encaplen	ifv_mib.ifvm_encaplen
#define	ifv_mtufudge	ifv_mib.ifvm_mtufudge
#define	ifv_mintu	ifv_mib.ifvm_mintu
#define	ifv_tag		ifv_mib.ifvm_tag

struct vlan_multisw {
	int	(*vmsw_addmulti)(struct ifvlan *, struct ifreq *);
	int	(*vmsw_delmulti)(struct ifvlan *, struct ifreq *);
	void	(*vmsw_purgemulti)(struct ifvlan *);
};

static int	vlan_ether_addmulti(struct ifvlan *, struct ifreq *);
static int	vlan_ether_delmulti(struct ifvlan *, struct ifreq *);
static void	vlan_ether_purgemulti(struct ifvlan *);

const struct vlan_multisw vlan_ether_multisw = {
	vlan_ether_addmulti,
	vlan_ether_delmulti,
	vlan_ether_purgemulti,
};

static int	vlan_clone_create(struct if_clone *, int);
static void	vlan_clone_destroy(struct ifnet *);
static int	vlan_config(struct ifvlan *, struct ifnet *);
static int	vlan_ioctl(struct ifnet *, u_long, caddr_t);
static void	vlan_start(struct ifnet *);
static void	vlan_unconfig(struct ifnet *);

void		vlanattach(int);

/* XXX This should be a hash table with the tag as the basis of the key. */
static LIST_HEAD(, ifvlan) ifv_list;

struct if_clone vlan_cloner =
    IF_CLONE_INITIALIZER("vlan", vlan_clone_create, vlan_clone_destroy);

/* Used to pad ethernet frames with < ETHER_MIN_LEN bytes */
static char vlan_zero_pad_buff[ETHER_MIN_LEN];

void
vlanattach(int n)
{

	LIST_INIT(&ifv_list);
	if_clone_attach(&vlan_cloner);
}

static void
vlan_reset_linkname(struct ifnet *ifp)
{

	/*
	 * We start out with a "802.1Q VLAN" type and zero-length
	 * addresses.  When we attach to a parent interface, we
	 * inherit its type, address length, address, and data link
	 * type.
	 */

	ifp->if_type = IFT_L2VLAN;
	ifp->if_addrlen = 0;
	ifp->if_dlt = DLT_NULL;
	if_alloc_sadl(ifp);
}

static int
vlan_clone_create(struct if_clone *ifc, int unit)
{
	struct ifvlan *ifv;
	struct ifnet *ifp;
	int s;

	ifv = malloc(sizeof(struct ifvlan), M_DEVBUF, M_WAITOK);
	memset(ifv, 0, sizeof(struct ifvlan));
	ifp = &ifv->ifv_if;
	LIST_INIT(&ifv->ifv_mc_listhead);

	s = splnet();
	LIST_INSERT_HEAD(&ifv_list, ifv, ifv_list);
	splx(s);

	snprintf(ifp->if_xname, sizeof(ifp->if_xname), "%s%d", ifc->ifc_name,
	    unit);
	ifp->if_softc = ifv;
	ifp->if_flags = IFF_BROADCAST | IFF_SIMPLEX | IFF_MULTICAST;
	ifp->if_start = vlan_start;
	ifp->if_ioctl = vlan_ioctl;
	IFQ_SET_READY(&ifp->if_snd);

	if_attach(ifp);
	vlan_reset_linkname(ifp);

	return (0);
}

static void
vlan_clone_destroy(struct ifnet *ifp)
{
	struct ifvlan *ifv = ifp->if_softc;
	int s;

	s = splnet();
	LIST_REMOVE(ifv, ifv_list);
	vlan_unconfig(ifp);
	splx(s);

	if_detach(ifp);
	free(ifv, M_DEVBUF);
}

/*
 * Configure a VLAN interface.  Must be called at splnet().
 */
static int
vlan_config(struct ifvlan *ifv, struct ifnet *p)
{
	struct ifnet *ifp = &ifv->ifv_if;
	int error;

	if (ifv->ifv_p != NULL)
		return (EBUSY);

	switch (p->if_type) {
	case IFT_ETHER:
	    {
		struct ethercom *ec = (void *) p;

		ifv->ifv_msw = &vlan_ether_multisw;
		ifv->ifv_encaplen = ETHER_VLAN_ENCAP_LEN;
		ifv->ifv_mintu = ETHERMIN;

		/*
		 * If the parent supports the VLAN_MTU capability,
		 * i.e. can Tx/Rx larger than ETHER_MAX_LEN frames,
		 * enable it.
		 */
		if (ec->ec_nvlans++ == 0 &&
		    (ec->ec_capabilities & ETHERCAP_VLAN_MTU) != 0) {
			/*
			 * Enable Tx/Rx of VLAN-sized frames.
			 */
			ec->ec_capenable |= ETHERCAP_VLAN_MTU;
			if (p->if_flags & IFF_UP) {
				struct ifreq ifr;

				ifr.ifr_flags = p->if_flags;
				error = (*p->if_ioctl)(p, SIOCSIFFLAGS,
				    (caddr_t) &ifr);
				if (error) {
					if (ec->ec_nvlans-- == 1)
						ec->ec_capenable &=
						    ~ETHERCAP_VLAN_MTU;
					return (error);
				}
			}
			ifv->ifv_mtufudge = 0;
		} else if ((ec->ec_capabilities & ETHERCAP_VLAN_MTU) == 0) {
			/*
			 * Fudge the MTU by the encapsulation size.  This
			 * makes us incompatible with strictly compliant
			 * 802.1Q implementations, but allows us to use
			 * the feature with other NetBSD implementations,
			 * which might still be useful.
			 */
			ifv->ifv_mtufudge = ifv->ifv_encaplen;
		}

		/*
		 * If the parent interface can do hardware-assisted
		 * VLAN encapsulation, then propagate its hardware-
		 * assisted checksumming flags.
		 */
		if (ec->ec_capabilities & ETHERCAP_VLAN_HWTAGGING)
			ifp->if_capabilities = p->if_capabilities &
			    (IFCAP_CSUM_IPv4|IFCAP_CSUM_TCPv4|
			     IFCAP_CSUM_UDPv4|IFCAP_CSUM_TCPv6|
			     IFCAP_CSUM_UDPv6);

		/*
		 * We inherit the parent's Ethernet address.
		 */
		ether_ifattach(ifp, LLADDR(p->if_sadl));
		ifp->if_hdrlen = sizeof(struct ether_vlan_header); /* XXX? */
		break;
	    }

	default:
		return (EPROTONOSUPPORT);
	}

	ifv->ifv_p = p;
	ifv->ifv_if.if_mtu = p->if_mtu - ifv->ifv_mtufudge;
	ifv->ifv_if.if_flags = p->if_flags &
	    (IFF_UP | IFF_BROADCAST | IFF_SIMPLEX | IFF_MULTICAST);

	/*
	 * Inherit the if_type from the parent.  This allows us
	 * to participate in bridges of that type.
	 */
	ifv->ifv_if.if_type = p->if_type;

	return (0);
}

/*
 * Unconfigure a VLAN interface.  Must be called at splnet().
 */
static void
vlan_unconfig(struct ifnet *ifp)
{
	struct ifvlan *ifv = ifp->if_softc;

	if (ifv->ifv_p == NULL)
		return;

	/*
 	 * Since the interface is being unconfigured, we need to empty the
	 * list of multicast groups that we may have joined while we were
	 * alive and remove them from the parent's list also.
	 */
	(*ifv->ifv_msw->vmsw_purgemulti)(ifv);

	/* Disconnect from parent. */
	switch (ifv->ifv_p->if_type) {
	case IFT_ETHER:
	    {
		struct ethercom *ec = (void *) ifv->ifv_p;

		if (ec->ec_nvlans-- == 1) {
			/*
			 * Disable Tx/Rx of VLAN-sized frames.
			 */
			ec->ec_capenable &= ~ETHERCAP_VLAN_MTU;
			if (ifv->ifv_p->if_flags & IFF_UP) {
				struct ifreq ifr;

				ifr.ifr_flags = ifv->ifv_p->if_flags;
				(void) (*ifv->ifv_p->if_ioctl)(ifv->ifv_p,
				    SIOCSIFFLAGS, (caddr_t) &ifr);
			}
		}

		ether_ifdetach(ifp);
		vlan_reset_linkname(ifp);
		break;
	    }

#ifdef DIAGNOSTIC
	default:
		panic("vlan_unconfig: impossible");
#endif
	}

	ifv->ifv_p = NULL;
	ifv->ifv_if.if_mtu = 0;
	ifv->ifv_flags = 0;

	if_down(ifp);
	ifp->if_flags &= ~(IFF_UP|IFF_RUNNING);
	ifp->if_capabilities = 0;
}

/*
 * Called when a parent interface is detaching; destroy any VLAN
 * configuration for the parent interface.
 */
void
vlan_ifdetach(struct ifnet *p)
{
	struct ifvlan *ifv;
	int s;

	s = splnet();

	for (ifv = LIST_FIRST(&ifv_list); ifv != NULL;
	     ifv = LIST_NEXT(ifv, ifv_list)) {
		if (ifv->ifv_p == p)
			vlan_unconfig(&ifv->ifv_if);
	}

	splx(s);
}

static int
vlan_set_promisc(struct ifnet *ifp)
{
	struct ifvlan *ifv = ifp->if_softc;
	int error = 0;

	if ((ifp->if_flags & IFF_PROMISC) != 0) {
		if ((ifv->ifv_flags & IFVF_PROMISC) == 0) {
			error = ifpromisc(ifv->ifv_p, 1);
			if (error == 0)
				ifv->ifv_flags |= IFVF_PROMISC;
		}
	} else {
		if ((ifv->ifv_flags & IFVF_PROMISC) != 0) {
			error = ifpromisc(ifv->ifv_p, 0);
			if (error == 0)
				ifv->ifv_flags &= ~IFVF_PROMISC;
		}
	}

	return (error);
}

static int
vlan_ioctl(struct ifnet *ifp, u_long cmd, caddr_t data)
{
	struct proc *p = curproc;	/* XXX */
	struct ifvlan *ifv = ifp->if_softc;
	struct ifaddr *ifa = (struct ifaddr *) data;
	struct ifreq *ifr = (struct ifreq *) data;
	struct ifnet *pr;
	struct vlanreq vlr;
	struct sockaddr *sa;
	int s, error = 0;

	s = splnet();

	switch (cmd) {
	case SIOCSIFADDR:
		if (ifv->ifv_p != NULL) {
			ifp->if_flags |= IFF_UP;

			switch (ifa->ifa_addr->sa_family) {
#ifdef INET
			case AF_INET:
				arp_ifinit(ifp, ifa);
				break;
#endif
			default:
				break;
			}
		} else {
			error = EINVAL;
		}
		break;

	case SIOCGIFADDR:
		sa = (struct sockaddr *)&ifr->ifr_data;
		memcpy(sa->sa_data, LLADDR(ifp->if_sadl), ifp->if_addrlen);
		break;

	case SIOCSIFMTU:
		if (ifv->ifv_p != NULL) {
			if (ifr->ifr_mtu >
			     (ifv->ifv_p->if_mtu - ifv->ifv_mtufudge) ||
			    ifr->ifr_mtu <
			     (ifv->ifv_mintu - ifv->ifv_mtufudge))
				error = EINVAL;
			else
				ifp->if_mtu = ifr->ifr_mtu;
		} else
			error = EINVAL;
		break;

	case SIOCSETVLAN:
		if ((error = suser(p->p_ucred, &p->p_acflag)) != 0)
			break;
		if ((error = copyin(ifr->ifr_data, &vlr, sizeof(vlr))) != 0)
			break;
		if (vlr.vlr_parent[0] == '\0') {
			vlan_unconfig(ifp);
			break;
		}
		if (vlr.vlr_tag != EVL_VLANOFTAG(vlr.vlr_tag)) {
			error = EINVAL;		 /* check for valid tag */
			break;
		}
		if ((pr = ifunit(vlr.vlr_parent)) == 0) {
			error = ENOENT;
			break;
		}
		if ((error = vlan_config(ifv, pr)) != 0)
			break;
		ifv->ifv_tag = vlr.vlr_tag;
		ifp->if_flags |= IFF_RUNNING;

		/* Update promiscuous mode, if necessary. */
		vlan_set_promisc(ifp);
		break;

	case SIOCGETVLAN:
		memset(&vlr, 0, sizeof(vlr));
		if (ifv->ifv_p != NULL) {
			snprintf(vlr.vlr_parent, sizeof(vlr.vlr_parent), "%s",
			    ifv->ifv_p->if_xname);
			vlr.vlr_tag = ifv->ifv_tag;
		}
		error = copyout(&vlr, ifr->ifr_data, sizeof(vlr));
		break;

	case SIOCSIFFLAGS:
		/*
		 * For promiscuous mode, we enable promiscuous mode on
		 * the parent if we need promiscuous on the VLAN interface.
		 */
		if (ifv->ifv_p != NULL)
			error = vlan_set_promisc(ifp);
		break;

	case SIOCADDMULTI:
		error = (ifv->ifv_p != NULL) ?
		    (*ifv->ifv_msw->vmsw_addmulti)(ifv, ifr) : EINVAL;
		break;

	case SIOCDELMULTI:
		error = (ifv->ifv_p != NULL) ?
		    (*ifv->ifv_msw->vmsw_delmulti)(ifv, ifr) : EINVAL;
		break;

	default:
		error = EINVAL;
	}

	splx(s);

	return (error);
}

static int
vlan_ether_addmulti(struct ifvlan *ifv, struct ifreq *ifr)
{
	struct vlan_mc_entry *mc;
	u_int8_t addrlo[ETHER_ADDR_LEN], addrhi[ETHER_ADDR_LEN];
	int error;

	if (ifr->ifr_addr.sa_len > sizeof(struct sockaddr_storage))
		return (EINVAL);

	error = ether_addmulti(ifr, &ifv->ifv_ec);
	if (error != ENETRESET)
		return (error);

	/*
	 * This is new multicast address.  We have to tell parent
	 * about it.  Also, remember this multicast address so that
	 * we can delete them on unconfigure.
	 */
	MALLOC(mc, struct vlan_mc_entry *, sizeof(struct vlan_mc_entry),
	    M_DEVBUF, M_NOWAIT);
	if (mc == NULL) {
		error = ENOMEM;
		goto alloc_failed;
	}

	/*
	 * As ether_addmulti() returns ENETRESET, following two
	 * statement shouldn't fail.
	 */
	(void)ether_multiaddr(&ifr->ifr_addr, addrlo, addrhi);
	ETHER_LOOKUP_MULTI(addrlo, addrhi, &ifv->ifv_ec, mc->mc_enm);
	memcpy(&mc->mc_addr, &ifr->ifr_addr, ifr->ifr_addr.sa_len);
	LIST_INSERT_HEAD(&ifv->ifv_mc_listhead, mc, mc_entries);

	error = (*ifv->ifv_p->if_ioctl)(ifv->ifv_p, SIOCADDMULTI,
	    (caddr_t)ifr);
	if (error != 0)
		goto ioctl_failed;
	return (error);

 ioctl_failed:
	LIST_REMOVE(mc, mc_entries);
	FREE(mc, M_DEVBUF);
 alloc_failed:
	(void)ether_delmulti(ifr, &ifv->ifv_ec);
	return (error);
}

static int
vlan_ether_delmulti(struct ifvlan *ifv, struct ifreq *ifr)
{
	struct ether_multi *enm;
	struct vlan_mc_entry *mc;
	u_int8_t addrlo[ETHER_ADDR_LEN], addrhi[ETHER_ADDR_LEN];
	int error;

	/*
	 * Find a key to lookup vlan_mc_entry.  We have to do this
	 * before calling ether_delmulti for obvious reason.
	 */
	if ((error = ether_multiaddr(&ifr->ifr_addr, addrlo, addrhi)) != 0)
		return (error);
	ETHER_LOOKUP_MULTI(addrlo, addrhi, &ifv->ifv_ec, enm);

	error = ether_delmulti(ifr, &ifv->ifv_ec);
	if (error != ENETRESET)
		return (error);

	/* We no longer use this multicast address.  Tell parent so. */
	error = (*ifv->ifv_p->if_ioctl)(ifv->ifv_p, SIOCDELMULTI,
	    (caddr_t)ifr);
	if (error == 0) {
		/* And forget about this address. */
		for (mc = LIST_FIRST(&ifv->ifv_mc_listhead); mc != NULL;
		    mc = LIST_NEXT(mc, mc_entries)) {
			if (mc->mc_enm == enm) {
				LIST_REMOVE(mc, mc_entries);
				FREE(mc, M_DEVBUF);
				break;
			}
		}
		KASSERT(mc != NULL);
	} else
		(void)ether_addmulti(ifr, &ifv->ifv_ec);
	return (error);
}

/*
 * Delete any multicast address we have asked to add from parent
 * interface.  Called when the vlan is being unconfigured.
 */
static void
vlan_ether_purgemulti(struct ifvlan *ifv)
{
	struct ifnet *ifp = ifv->ifv_p;		/* Parent. */
	struct vlan_mc_entry *mc;
	union {
		struct ifreq ifreq;
		struct {
			char ifr_name[IFNAMSIZ];
			struct sockaddr_storage ifr_ss;
		} ifreq_storage;
	} ifreq;
	struct ifreq *ifr = &ifreq.ifreq;

	memcpy(ifr->ifr_name, ifp->if_xname, IFNAMSIZ);
	while ((mc = LIST_FIRST(&ifv->ifv_mc_listhead)) != NULL) {
		memcpy(&ifr->ifr_addr, &mc->mc_addr, mc->mc_addr.ss_len);
		(void)(*ifp->if_ioctl)(ifp, SIOCDELMULTI, (caddr_t)ifr);
		LIST_REMOVE(mc, mc_entries);
		FREE(mc, M_DEVBUF);
	}
}

static void
vlan_start(struct ifnet *ifp)
{
	struct ifvlan *ifv = ifp->if_softc;
	struct ifnet *p = ifv->ifv_p;
	struct ethercom *ec = (void *) ifv->ifv_p;
	struct mbuf *m;
	int error;
	ALTQ_DECL(struct altq_pktattr pktattr;)

	ifp->if_flags |= IFF_OACTIVE;

	for (;;) {
		IFQ_DEQUEUE(&ifp->if_snd, m);
		if (m == NULL)
			break;

#ifdef ALTQ
		/*
		 * If ALTQ is enabled on the parent interface, do
		 * classification; the queueing discipline might
		 * not require classification, but might require
		 * the address family/header pointer in the pktattr.
		 */
		if (ALTQ_IS_ENABLED(&p->if_snd)) {
			switch (p->if_type) {
			case IFT_ETHER:
				altq_etherclassify(&p->if_snd, m, &pktattr);
				break;
#ifdef DIAGNOSTIC
			default:
				panic("vlan_start: impossible (altq)");
#endif
			}
		}
#endif /* ALTQ */

#if NBPFILTER > 0
		if (ifp->if_bpf)
			bpf_mtap(ifp->if_bpf, m);
#endif
		/*
		 * If the parent can insert the tag itself, just mark
		 * the tag in the mbuf header.
		 */
		if (ec->ec_capabilities & ETHERCAP_VLAN_HWTAGGING) {
			struct m_tag *mtag;

			mtag = m_tag_get(PACKET_TAG_VLAN, sizeof(u_int),
			    M_NOWAIT);
			if (mtag == NULL) {
				ifp->if_oerrors++;
				m_freem(m);
				continue;
			}
			*(u_int *)(mtag + 1) = ifv->ifv_tag;
			m_tag_prepend(m, mtag);
		} else {
			/*
			 * insert the tag ourselves
			 */
			M_PREPEND(m, ifv->ifv_encaplen, M_DONTWAIT);
			if (m == NULL) {
				printf("%s: unable to prepend encap header",
				    ifv->ifv_p->if_xname);
				ifp->if_oerrors++;
				continue;
			}

			switch (p->if_type) {
			case IFT_ETHER:
			    {
				struct ether_vlan_header *evl;

				if (m->m_len < sizeof(struct ether_vlan_header))
					m = m_pullup(m,
					    sizeof(struct ether_vlan_header));
				if (m == NULL) {
					printf("%s: unable to pullup encap "
					    "header", ifv->ifv_p->if_xname);
					ifp->if_oerrors++;
					continue;
				}

				/*
				 * Transform the Ethernet header into an
				 * Ethernet header with 802.1Q encapsulation.
				 */
				memmove(mtod(m, caddr_t),
				    mtod(m, caddr_t) + ifv->ifv_encaplen, 
				    sizeof(struct ether_header));
				evl = mtod(m, struct ether_vlan_header *);
				evl->evl_proto = evl->evl_encap_proto;
				evl->evl_encap_proto = htons(ETHERTYPE_VLAN);
				evl->evl_tag = htons(ifv->ifv_tag);

				/*
				 * To cater for VLAN-aware layer 2 ethernet
				 * switches which may need to strip the tag
				 * before forwarding the packet, make sure
				 * the packet+tag is at least 68 bytes long.
				 * This is necessary because our parent will
				 * only pad to 64 bytes (ETHER_MIN_LEN) and
				 * some switches will not pad by themselves
				 * after deleting a tag.
				 */
				if (m->m_pkthdr.len <
				    (ETHER_MIN_LEN + ETHER_VLAN_ENCAP_LEN)) {
					m_copyback(m, m->m_pkthdr.len,
					    (ETHER_MIN_LEN +
					     ETHER_VLAN_ENCAP_LEN) -
					     m->m_pkthdr.len,
					    vlan_zero_pad_buff);
				}
				break;
			    }

#ifdef DIAGNOSTIC
			default:
				panic("vlan_start: impossible");
#endif
			}
		}

		/*
		 * Send it, precisely as the parent's output routine
		 * would have.  We are already running at splnet.
		 */
		IFQ_ENQUEUE(&p->if_snd, m, &pktattr, error);
		if (error) {
			/* mbuf is already freed */
			ifp->if_oerrors++;
			continue;
		}

		ifp->if_opackets++;
		if ((p->if_flags & IFF_OACTIVE) == 0)
			(*p->if_start)(p);
	}

	ifp->if_flags &= ~IFF_OACTIVE;
}

/*
 * Given an Ethernet frame, find a valid vlan interface corresponding to the
 * given source interface and tag, then run the real packet through the 
 * parent's input routine.
 */
void
vlan_input(struct ifnet *ifp, struct mbuf *m)
{
	struct ifvlan *ifv;
	u_int tag;
	struct m_tag *mtag;

	mtag = m_tag_find(m, PACKET_TAG_VLAN, NULL);
	if (mtag != NULL) {
		/* m contains a normal ethernet frame, the tag is in mtag */
		tag = *(u_int *)(mtag + 1);
		m_tag_delete(m, mtag);
		for (ifv = LIST_FIRST(&ifv_list); ifv != NULL;
		    ifv = LIST_NEXT(ifv, ifv_list))
			if (ifp == ifv->ifv_p && tag == ifv->ifv_tag)
				break;
	} else {
		switch (ifp->if_type) {
		case IFT_ETHER:
		    {
			struct ether_vlan_header *evl;

			if (m->m_len < sizeof(struct ether_vlan_header) &&
			    (m = m_pullup(m,
			     sizeof(struct ether_vlan_header))) == NULL) {
				printf("%s: no memory for VLAN header, "
				    "dropping packet.\n", ifp->if_xname);
				return;
			}
			evl = mtod(m, struct ether_vlan_header *);
			KASSERT(ntohs(evl->evl_encap_proto) == ETHERTYPE_VLAN);

			tag = EVL_VLANOFTAG(ntohs(evl->evl_tag));

			/*
			 * Restore the original ethertype.  We'll remove
			 * the encapsulation after we've found the vlan
			 * interface corresponding to the tag.
			 */
			evl->evl_encap_proto = evl->evl_proto;
			break;
		    }

		default:
			tag = (u_int) -1;	/* XXX GCC */
#ifdef DIAGNOSTIC
			panic("vlan_input: impossible");
#endif
		}

		for (ifv = LIST_FIRST(&ifv_list); ifv != NULL;
		     ifv = LIST_NEXT(ifv, ifv_list))
			if (ifp == ifv->ifv_p && tag == ifv->ifv_tag)
				break;


		/*
		 * Now, remove the encapsulation header.  The original
		 * header has already been fixed up above.
		 */
		if (ifv) {
			memmove(mtod(m, caddr_t) + ifv->ifv_encaplen,
			    mtod(m, caddr_t), sizeof(struct ether_header));
			m_adj(m, ifv->ifv_encaplen);
		}
	}

	if (ifv == NULL ||
	    (ifv->ifv_if.if_flags & (IFF_UP|IFF_RUNNING)) !=
	     (IFF_UP|IFF_RUNNING)) {
		m_freem(m);
		ifp->if_noproto++;
		return;
	}
	m->m_pkthdr.rcvif = &ifv->ifv_if;
	ifv->ifv_if.if_ipackets++;

#if NBPFILTER > 0
	if (ifv->ifv_if.if_bpf)
		bpf_mtap(ifv->ifv_if.if_bpf, m);
#endif

	/* Pass it back through the parent's input routine. */
	(*ifp->if_input)(&ifv->ifv_if, m);
}
