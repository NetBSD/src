/*	$NetBSD: if_vlan.c,v 1.6 2000/09/28 07:35:36 enami Exp $	*/

/*-
 * Copyright (c) 2000 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Andrew Doran and Jason R. Thorpe.
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
 *
 *	- Need to make promiscuous mode work.
 */

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

extern struct	ifaddr **ifnet_addrs;	/* XXX if.c */

static int	vlan_clone_create(struct if_clone *, int);
static void	vlan_clone_destroy(struct ifnet *);
static int	vlan_config(struct ifvlan *, struct ifnet *);
static int	vlan_ioctl(struct ifnet *, u_long, caddr_t);
static int	vlan_addmulti(struct ifvlan *, struct ifreq *);
static int	vlan_delmulti(struct ifvlan *, struct ifreq *);
static void	vlan_purgemulti(struct ifvlan *);
static void	vlan_start(struct ifnet *);
static int	vlan_unconfig(struct ifnet *);
void	vlanattach(int);

/* XXX This should be a hash table with the tag as the basis of the key. */
static LIST_HEAD(, ifvlan) ifv_list;

struct if_clone vlan_cloner =
    IF_CLONE_INITIALIZER("vlan", vlan_clone_create, vlan_clone_destroy);

void
vlanattach(int n)
{

	LIST_INIT(&ifv_list);
	if_clone_attach(&vlan_cloner);
}

static int
vlan_clone_create(struct if_clone *ifc, int unit)
{
	struct ifvlan *ifv;
	struct ifnet *ifp;
	u_int8_t eaddr[ETHER_ADDR_LEN];

	ifv = malloc(sizeof(struct ifvlan), M_DEVBUF, M_WAIT);
	memset(ifv, 0, sizeof(struct ifvlan));
	ifp = &ifv->ifv_ec.ec_if;
	LIST_INIT(&ifv->ifv_mc_listhead);
	LIST_INSERT_HEAD(&ifv_list, ifv, ifv_list);

	sprintf(ifp->if_xname, "%s%d", ifc->ifc_name, unit);
	ifp->if_softc = ifv;
	ifp->if_flags = IFF_BROADCAST | IFF_SIMPLEX | IFF_MULTICAST;
	ifp->if_start = vlan_start;
	ifp->if_ioctl = vlan_ioctl;

	if_attach(ifp);
	memset(eaddr, 0, sizeof(eaddr));
	ether_ifattach(ifp, eaddr);

	ifp->if_hdrlen = sizeof(struct ether_vlan_header);
	ifp->if_mtu = ETHERMTU - EVL_ENCAPLEN;

#if NBPFILTER > 0
	bpfattach(&ifp->if_bpf, ifp, DLT_EN10MB, 
	    sizeof(struct ether_header));
#endif

	return (0);
}

static void
vlan_clone_destroy(struct ifnet *ifp)
{
	struct ifvlan *ifv;
	int s;

	ifv = (struct ifvlan *)ifp->if_softc;
	s = splsoftnet();

	LIST_REMOVE(ifv, ifv_list);
	vlan_unconfig(ifp);

#if NBPFILTER > 0
	bpfdetach(ifp);
#endif
	ether_ifdetach(ifp);
	if_detach(ifp);
	free(ifv, M_DEVBUF);

	splx(s);
}

static int
vlan_config(struct ifvlan *ifv, struct ifnet *p)
{
	struct ifaddr *ifa1, *ifa2;
	struct sockaddr_dl *sdl1, *sdl2;

	if (p->if_data.ifi_type != IFT_ETHER)
		return (EPROTONOSUPPORT);
	if (ifv->ifv_p != NULL)
		return (EBUSY);
	ifv->ifv_p = p;
	ifv->ifv_if.if_mtu = p->if_data.ifi_mtu - EVL_ENCAPLEN;
	ifv->ifv_if.if_flags = p->if_flags;

	/*
	 * Set up our ``Ethernet address'' to match the underlying
	 * physical interface's.
	 */
	ifa1 = ifnet_addrs[ifv->ifv_if.if_index];
	ifa2 = ifnet_addrs[p->if_index];
	sdl1 = (struct sockaddr_dl *)ifa1->ifa_addr;
	sdl2 = (struct sockaddr_dl *)ifa2->ifa_addr;
	sdl1->sdl_type = IFT_ETHER;
	sdl1->sdl_alen = ETHER_ADDR_LEN;
	memcpy(LLADDR(sdl1), LLADDR(sdl2), ETHER_ADDR_LEN);
	memcpy(LLADDR(ifv->ifv_ec.ec_if.if_sadl), LLADDR(sdl2), ETHER_ADDR_LEN);
	return (0);
}

static int
vlan_unconfig(struct ifnet *ifp)
{
	struct ifaddr *ifa;
	struct sockaddr_dl *sdl;
	struct ifvlan *ifv;
	int s;

	s = splsoftnet();

	ifv = ifp->if_softc;

	/*
 	 * Since the interface is being unconfigured, we need to empty the
	 * list of multicast groups that we may have joined while we were
	 * alive and remove them from the parent's list also.
	 */
	vlan_purgemulti(ifv);

	/* Disconnect from parent. */
	ifv->ifv_p = NULL;
	ifv->ifv_if.if_mtu = ETHERMTU - EVL_ENCAPLEN;

	/* Clear our MAC address. */
	ifa = ifnet_addrs[ifv->ifv_if.if_index];
	sdl = (struct sockaddr_dl *)ifa->ifa_addr;
	sdl->sdl_type = IFT_ETHER;
	sdl->sdl_alen = ETHER_ADDR_LEN;
	memset(LLADDR(sdl), 0, ETHER_ADDR_LEN);
	memset(LLADDR(ifv->ifv_ec.ec_if.if_sadl), 0, ETHER_ADDR_LEN);

	splx(s);
	return (0);
}

static int
vlan_ioctl(struct ifnet *ifp, u_long cmd, caddr_t data)
{
	struct proc *p = curproc;	/* XXX */
	struct ifaddr *ifa;
	struct ifnet *pr;
	struct ifreq *ifr;
	struct ifvlan *ifv;
	struct vlanreq vlr;
	struct sockaddr *sa;
	int error;

	error = 0;
	ifr = (struct ifreq *)data;
	ifa = (struct ifaddr *)data;
	ifv = ifp->if_softc;

	switch (cmd) {
	case SIOCSIFADDR:
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
		break;

	case SIOCGIFADDR:
		sa = (struct sockaddr *)&ifr->ifr_data;
		memcpy(sa->sa_data, LLADDR(ifp->if_sadl), ETHER_ADDR_LEN);
		break;

	case SIOCSIFMTU:
		if (ifv->ifv_p != NULL) {
			if (ifr->ifr_mtu > ifv->ifv_p->if_mtu - EVL_ENCAPLEN ||
			    ifr->ifr_mtu < ETHERMIN + EVL_ENCAPLEN)
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
			if_down(ifp);
			ifp->if_flags &= ~(IFF_UP|IFF_RUNNING);
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
		 * XXX We don't support promiscuous mode right now because
		 * it would require help from the underlying drivers, which
		 * hasn't been implemented.
		 */
		if ((ifr->ifr_flags & IFF_PROMISC) != 0) {
			ifp->if_flags &= ~(IFF_PROMISC);
			error = EINVAL;
		}
		break;

	case SIOCADDMULTI:
		error = vlan_addmulti(ifv, ifr);
		break;

	case SIOCDELMULTI:
		error = vlan_delmulti(ifv, ifr);
		break;

	default:
		error = EINVAL;
	}

	return (error);
}

static int
vlan_addmulti(struct ifvlan *ifv, struct ifreq *ifr)
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
vlan_delmulti(struct ifvlan *ifv, struct ifreq *ifr)
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
 * Delete any multicast address we have asked to add form parent
 * interface.  Called when the vlan is being unconfigured.
 */
static void
vlan_purgemulti(struct ifvlan *ifv)
{
	struct ifnet *ifp = ifv->ifv_p;		/* Parent. */
	struct vlan_mc_entry *mc;
	union {
		struct ifreq ifreq;
		struct {
			char ifr_name[IFNAMSIZ];
			struct sockaddr_storage;
		} ifreq_storage;
	} ifreq;
	struct ifreq *ifr = &ifreq.ifreq;

	memcpy(ifr->ifr_name, ifp->if_xname, IFNAMSIZ);
	while ((mc = LIST_FIRST(&ifv->ifv_mc_listhead)) != NULL) {
		memcpy(&ifr->ifr_addr, &mc->mc_addr, mc->mc_addr.ss_len);
		(void)(*ifp->if_ioctl)(ifp, SIOCDELMULTI, (caddr_t)ifr);
		LIST_REMOVE(mc->mc_enm, enm_list);
		free(mc->mc_enm, M_IFMADDR);
		LIST_REMOVE(mc, mc_entries);
		FREE(mc, M_DEVBUF);
	}

	KASSERT(LIST_FIRST(&ifv->ifv_ec.ec_multiaddrs) == NULL);
}

static void
vlan_start(struct ifnet *ifp)
{
	struct ifvlan *ifv;
	struct ifnet *p;
	struct ether_vlan_header *evl;
	struct mbuf *m;

	ifv = ifp->if_softc;
	p = ifv->ifv_p;
	ifp->if_flags |= IFF_OACTIVE;

	for (;;) {
		IF_DEQUEUE(&ifp->if_snd, m);
		if (m == NULL)
			break;

#if NBPFILTER > 0
		if (ifp->if_bpf)
			bpf_mtap(ifp->if_bpf, m);
#endif

		/*
		 * XXX Should handle the case where the underlying hardware
		 * interface can do VLAN tag insertion itself.
		 */
		M_PREPEND(m, EVL_ENCAPLEN, M_DONTWAIT);
		if (m == NULL) {
			printf("%s: M_PREPEND failed", ifv->ifv_p->if_xname);
			ifp->if_ierrors++;
			continue;
		}

		if (m->m_len < sizeof(struct ether_vlan_header) &&
		    (m = m_pullup(m,
		     sizeof(struct ether_vlan_header))) == NULL) {
			printf("%s: m_pullup failed", ifv->ifv_p->if_xname);
			ifp->if_ierrors++;
			continue;
		}

		/*
		 * Transform the Ethernet header into an Ethernet header
		 * with 802.1Q encapsulation.
		 */
		memmove(mtod(m, caddr_t), mtod(m, caddr_t) + EVL_ENCAPLEN, 
		    sizeof(struct ether_header));
		evl = mtod(m, struct ether_vlan_header *);
		evl->evl_proto = evl->evl_encap_proto;
		evl->evl_encap_proto = htons(ETHERTYPE_VLAN);
		evl->evl_tag = htons(ifv->ifv_tag);

		/*
		 * Send it, precisely as ether_output() would have.  We are
		 * already running at splimp.
		 */
		if (IF_QFULL(&p->if_snd)) {
			IF_DROP(&p->if_snd);
			/* XXX stats */
			ifp->if_oerrors++;
			m_freem(m);
			continue;
		}
	
		IF_ENQUEUE(&p->if_snd, m);
		if ((p->if_flags & IFF_OACTIVE) == 0) {
			p->if_start(p);
			ifp->if_opackets++;
		}
	}

	ifp->if_flags &= ~IFF_OACTIVE;
}

/*
 * Given an Ethernet frame, find a valid vlan interface corresponding to the
 * given source interface and tag, then run the the real packet through
 * the parent's input routine.
 */
void
vlan_input(struct ifnet *ifp, struct mbuf *m)
{
	struct ether_vlan_header *evl;
	struct ifvlan *ifv;
	u_int tag;

	if (m->m_len < sizeof(struct ether_vlan_header) &&
	    (m = m_pullup(m, sizeof(struct ether_vlan_header))) == NULL) {
		printf("%s: no memory for VLAN header, dropping packet.\n",
		    ifp->if_xname);
		return;
	}
	evl = mtod(m, struct ether_vlan_header *);
	KASSERT(htons(evl->evl_encap_proto) == ETHERTYPE_VLAN);

	tag = EVL_VLANOFTAG(ntohs(evl->evl_tag));

	for (ifv = LIST_FIRST(&ifv_list); ifv != NULL;
	    ifv = LIST_NEXT(ifv, ifv_list))
		if (ifp == ifv->ifv_p && tag == ifv->ifv_tag)
			break;

	if (ifv == NULL || (ifv->ifv_if.if_flags & (IFF_UP|IFF_RUNNING)) !=
	    (IFF_UP|IFF_RUNNING)) {
		m_free(m);
		ifp->if_data.ifi_noproto++;
		return;
	}

	/*
	 * Having found a valid vlan interface corresponding to the given
	 * source interface and vlan tag, remove the encapsulation.
	 */
	evl->evl_encap_proto = evl->evl_proto;
	memmove(mtod(m, caddr_t) + EVL_ENCAPLEN, mtod(m, caddr_t),
	    EVL_ENCAPLEN);
	m_adj(m, EVL_ENCAPLEN);

	m->m_pkthdr.rcvif = &ifv->ifv_if;
	ifv->ifv_if.if_ipackets++;

#if NBPFILTER > 0
	if (ifv->ifv_if.if_bpf)
		bpf_mtap(ifv->ifv_if.if_bpf, m);
#endif

	/* Pass it back through the parent's input routine. */
	(*ifp->if_input)(&ifv->ifv_if, m);
}
