/*	$NetBSD: if_vlan.c,v 1.118 2017/12/08 04:03:51 ozaki-r Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: if_vlan.c,v 1.118 2017/12/08 04:03:51 ozaki-r Exp $");

#ifdef _KERNEL_OPT
#include "opt_inet.h"
#endif

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/mbuf.h>
#include <sys/queue.h>
#include <sys/socket.h>
#include <sys/sockio.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/kauth.h>
#include <sys/mutex.h>
#include <sys/kmem.h>
#include <sys/cpu.h>
#include <sys/pserialize.h>
#include <sys/psref.h>
#include <sys/pslist.h>
#include <sys/atomic.h>
#include <sys/device.h>
#include <sys/module.h>

#include <net/bpf.h>
#include <net/if.h>
#include <net/if_dl.h>
#include <net/if_types.h>
#include <net/if_ether.h>
#include <net/if_vlanvar.h>

#ifdef INET
#include <netinet/in.h>
#include <netinet/if_inarp.h>
#endif
#ifdef INET6
#include <netinet6/in6_ifattach.h>
#include <netinet6/in6_var.h>
#endif

#include "ioconf.h"

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


struct ifvlan_linkmib {
	struct ifvlan *ifvm_ifvlan;
	const struct vlan_multisw *ifvm_msw;
	int	ifvm_encaplen;	/* encapsulation length */
	int	ifvm_mtufudge;	/* MTU fudged by this much */
	int	ifvm_mintu;	/* min transmission unit */
	uint16_t ifvm_proto;	/* encapsulation ethertype */
	uint16_t ifvm_tag;	/* tag to apply on packets */
	struct ifnet *ifvm_p;		/* parent interface of this vlan */

	struct psref_target ifvm_psref;
};

struct ifvlan {
	union {
		struct ethercom ifvu_ec;
	} ifv_u;
	struct ifvlan_linkmib *ifv_mib;	/*
					 * reader must use vlan_getref_linkmib()
					 * instead of direct dereference
					 */
	kmutex_t ifv_lock;		/* writer lock for ifv_mib */

	LIST_HEAD(__vlan_mchead, vlan_mc_entry) ifv_mc_listhead;
	LIST_ENTRY(ifvlan) ifv_list;
	struct pslist_entry ifv_hash;
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
static int	vlan_clone_destroy(struct ifnet *);
static int	vlan_config(struct ifvlan *, struct ifnet *,
    uint16_t);
static int	vlan_ioctl(struct ifnet *, u_long, void *);
static void	vlan_start(struct ifnet *);
static int	vlan_transmit(struct ifnet *, struct mbuf *);
static void	vlan_unconfig(struct ifnet *);
static int	vlan_unconfig_locked(struct ifvlan *,
    struct ifvlan_linkmib *);
static void	vlan_hash_init(void);
static int	vlan_hash_fini(void);
static int	vlan_tag_hash(uint16_t, u_long);
static struct ifvlan_linkmib*	vlan_getref_linkmib(struct ifvlan *,
    struct psref *);
static void	vlan_putref_linkmib(struct ifvlan_linkmib *,
    struct psref *);
static void	vlan_linkmib_update(struct ifvlan *,
    struct ifvlan_linkmib *);
static struct ifvlan_linkmib*	vlan_lookup_tag_psref(struct ifnet *,
    uint16_t, struct psref *);

LIST_HEAD(vlan_ifvlist, ifvlan);
static struct {
	kmutex_t lock;
	struct vlan_ifvlist list;
} ifv_list __cacheline_aligned;


#if !defined(VLAN_TAG_HASH_SIZE)
#define VLAN_TAG_HASH_SIZE 32
#endif
static struct {
	kmutex_t lock;
	struct pslist_head *lists;
	u_long mask;
} ifv_hash __cacheline_aligned = {
	.lists = NULL,
	.mask = 0,
};

pserialize_t vlan_psz __read_mostly;
static struct psref_class *ifvm_psref_class __read_mostly;

struct if_clone vlan_cloner =
    IF_CLONE_INITIALIZER("vlan", vlan_clone_create, vlan_clone_destroy);

/* Used to pad ethernet frames with < ETHER_MIN_LEN bytes */
static char vlan_zero_pad_buff[ETHER_MIN_LEN];

static inline int
vlan_safe_ifpromisc(struct ifnet *ifp, int pswitch)
{
	int e;
	KERNEL_LOCK_UNLESS_NET_MPSAFE();
	e = ifpromisc(ifp, pswitch);
	KERNEL_UNLOCK_UNLESS_NET_MPSAFE();
	return e;
}

static inline int
vlan_safe_ifpromisc_locked(struct ifnet *ifp, int pswitch)
{
	int e;
	KERNEL_LOCK_UNLESS_NET_MPSAFE();
	e = ifpromisc_locked(ifp, pswitch);
	KERNEL_UNLOCK_UNLESS_NET_MPSAFE();
	return e;
}

void
vlanattach(int n)
{

	/*
	 * Nothing to do here, initialization is handled by the
	 * module initialization code in vlaninit() below).
	 */
}

static void
vlaninit(void)
{
	mutex_init(&ifv_list.lock, MUTEX_DEFAULT, IPL_NONE);
	LIST_INIT(&ifv_list.list);

	mutex_init(&ifv_hash.lock, MUTEX_DEFAULT, IPL_NONE);
	vlan_psz = pserialize_create();
	ifvm_psref_class = psref_class_create("vlanlinkmib", IPL_SOFTNET);
	if_clone_attach(&vlan_cloner);

	vlan_hash_init();
}

static int
vlandetach(void)
{
	int error = 0;

	mutex_enter(&ifv_list.lock);
	if (!LIST_EMPTY(&ifv_list.list)) {
		mutex_exit(&ifv_list.lock);
		return EBUSY;
	}
	mutex_exit(&ifv_list.lock);

	error = vlan_hash_fini();
	if (error != 0)
		return error;

	if_clone_detach(&vlan_cloner);
	psref_class_destroy(ifvm_psref_class);
	pserialize_destroy(vlan_psz);
	mutex_destroy(&ifv_hash.lock);
	mutex_destroy(&ifv_list.lock);

	return 0;
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
	struct ifvlan_linkmib *mib;
	int rv;

	ifv = malloc(sizeof(struct ifvlan), M_DEVBUF, M_WAITOK|M_ZERO);
	mib = kmem_zalloc(sizeof(struct ifvlan_linkmib), KM_SLEEP);
	ifp = &ifv->ifv_if;
	LIST_INIT(&ifv->ifv_mc_listhead);

	mib->ifvm_ifvlan = ifv;
	mib->ifvm_p = NULL;
	psref_target_init(&mib->ifvm_psref, ifvm_psref_class);

	mutex_init(&ifv->ifv_lock, MUTEX_DEFAULT, IPL_NONE);
	ifv->ifv_mib = mib;

	mutex_enter(&ifv_list.lock);
	LIST_INSERT_HEAD(&ifv_list.list, ifv, ifv_list);
	mutex_exit(&ifv_list.lock);

	if_initname(ifp, ifc->ifc_name, unit);
	ifp->if_softc = ifv;
	ifp->if_flags = IFF_BROADCAST | IFF_SIMPLEX | IFF_MULTICAST;
	ifp->if_extflags = IFEF_MPSAFE | IFEF_NO_LINK_STATE_CHANGE;
	ifp->if_start = vlan_start;
	ifp->if_transmit = vlan_transmit;
	ifp->if_ioctl = vlan_ioctl;
	IFQ_SET_READY(&ifp->if_snd);

	rv = if_initialize(ifp);
	if (rv != 0) {
		aprint_error("%s: if_initialize failed(%d)\n", ifp->if_xname,
		    rv);
		goto fail;
	}

	vlan_reset_linkname(ifp);
	if_register(ifp);
	return 0;

fail:
	mutex_enter(&ifv_list.lock);
	LIST_REMOVE(ifv, ifv_list);
	mutex_exit(&ifv_list.lock);

	mutex_destroy(&ifv->ifv_lock);
	psref_target_destroy(&ifv->ifv_mib->ifvm_psref, ifvm_psref_class);
	kmem_free(ifv->ifv_mib, sizeof(struct ifvlan_linkmib));
	free(ifv, M_DEVBUF);

	return rv;
}

static int
vlan_clone_destroy(struct ifnet *ifp)
{
	struct ifvlan *ifv = ifp->if_softc;

	mutex_enter(&ifv_list.lock);
	LIST_REMOVE(ifv, ifv_list);
	mutex_exit(&ifv_list.lock);

	mutex_enter(ifp->if_ioctl_lock);
	vlan_unconfig(ifp);
	mutex_exit(ifp->if_ioctl_lock);
	if_detach(ifp);

	psref_target_destroy(&ifv->ifv_mib->ifvm_psref, ifvm_psref_class);
	kmem_free(ifv->ifv_mib, sizeof(struct ifvlan_linkmib));
	mutex_destroy(&ifv->ifv_lock);
	free(ifv, M_DEVBUF);

	return (0);
}

/*
 * Configure a VLAN interface.
 */
static int
vlan_config(struct ifvlan *ifv, struct ifnet *p, uint16_t tag)
{
	struct ifnet *ifp = &ifv->ifv_if;
	struct ifvlan_linkmib *nmib = NULL;
	struct ifvlan_linkmib *omib = NULL;
	struct ifvlan_linkmib *checkmib = NULL;
	struct psref_target *nmib_psref = NULL;
	uint16_t vid = EVL_VLANOFTAG(tag);
	int error = 0;
	int idx;
	bool omib_cleanup = false;
	struct psref psref;

	/* VLAN ID 0 and 4095 are reserved in the spec */
	if ((vid == 0) || (vid == 0xfff))
		return EINVAL;

	nmib = kmem_alloc(sizeof(*nmib), KM_SLEEP);

	mutex_enter(&ifv->ifv_lock);
	omib = ifv->ifv_mib;

	if (omib->ifvm_p != NULL) {
		error = EBUSY;
		goto done;
	}

	/* Duplicate check */
	checkmib = vlan_lookup_tag_psref(p, vid, &psref);
	if (checkmib != NULL) {
		vlan_putref_linkmib(checkmib, &psref);
		error = EEXIST;
		goto done;
	}

	*nmib = *omib;
	nmib_psref = &nmib->ifvm_psref;

	psref_target_init(nmib_psref, ifvm_psref_class);

	switch (p->if_type) {
	case IFT_ETHER:
	    {
		struct ethercom *ec = (void *) p;
		nmib->ifvm_msw = &vlan_ether_multisw;
		nmib->ifvm_encaplen = ETHER_VLAN_ENCAP_LEN;
		nmib->ifvm_mintu = ETHERMIN;

		if (ec->ec_nvlans++ == 0) {
			mutex_enter(p->if_ioctl_lock);
			error = ether_enable_vlan_mtu(p);
			mutex_exit(p->if_ioctl_lock);
			if (error >= 0) {
				if (error) {
					ec->ec_nvlans--;
					goto done;
				}
				nmib->ifvm_mtufudge = 0;
			} else {
				/*
				 * Fudge the MTU by the encapsulation size. This
				 * makes us incompatible with strictly compliant
				 * 802.1Q implementations, but allows us to use
				 * the feature with other NetBSD
				 * implementations, which might still be useful.
				 */
				nmib->ifvm_mtufudge = nmib->ifvm_encaplen;
			}
			error = 0;
		}

		/*
		 * If the parent interface can do hardware-assisted
		 * VLAN encapsulation, then propagate its hardware-
		 * assisted checksumming flags and tcp segmentation
		 * offload.
		 */
		if (ec->ec_capabilities & ETHERCAP_VLAN_HWTAGGING) {
		        ec->ec_capenable |= ETHERCAP_VLAN_HWTAGGING;
			ifp->if_capabilities = p->if_capabilities &
			    (IFCAP_TSOv4 | IFCAP_TSOv6 |
			     IFCAP_CSUM_IPv4_Tx|IFCAP_CSUM_IPv4_Rx|
			     IFCAP_CSUM_TCPv4_Tx|IFCAP_CSUM_TCPv4_Rx|
			     IFCAP_CSUM_UDPv4_Tx|IFCAP_CSUM_UDPv4_Rx|
			     IFCAP_CSUM_TCPv6_Tx|IFCAP_CSUM_TCPv6_Rx|
			     IFCAP_CSUM_UDPv6_Tx|IFCAP_CSUM_UDPv6_Rx);
                }
		/*
		 * We inherit the parent's Ethernet address.
		 */
		ether_ifattach(ifp, CLLADDR(p->if_sadl));
		ifp->if_hdrlen = sizeof(struct ether_vlan_header); /* XXX? */
		break;
	    }

	default:
		error = EPROTONOSUPPORT;
		goto done;
	}

	nmib->ifvm_p = p;
	nmib->ifvm_tag = vid;
	ifv->ifv_if.if_mtu = p->if_mtu - nmib->ifvm_mtufudge;
	ifv->ifv_if.if_flags = p->if_flags &
	    (IFF_UP | IFF_BROADCAST | IFF_SIMPLEX | IFF_MULTICAST);

	/*
	 * Inherit the if_type from the parent.  This allows us
	 * to participate in bridges of that type.
	 */
	ifv->ifv_if.if_type = p->if_type;

	PSLIST_ENTRY_INIT(ifv, ifv_hash);
	idx = vlan_tag_hash(vid, ifv_hash.mask);

	mutex_enter(&ifv_hash.lock);
	PSLIST_WRITER_INSERT_HEAD(&ifv_hash.lists[idx], ifv, ifv_hash);
	mutex_exit(&ifv_hash.lock);

	vlan_linkmib_update(ifv, nmib);
	nmib = NULL;
	nmib_psref = NULL;
	omib_cleanup = true;

done:
	mutex_exit(&ifv->ifv_lock);

	if (nmib_psref)
		psref_target_destroy(nmib_psref, ifvm_psref_class);

	if (nmib)
		kmem_free(nmib, sizeof(*nmib));

	if (omib_cleanup)
		kmem_free(omib, sizeof(*omib));

	return error;
}

/*
 * Unconfigure a VLAN interface.
 */
static void
vlan_unconfig(struct ifnet *ifp)
{
	struct ifvlan *ifv = ifp->if_softc;
	struct ifvlan_linkmib *nmib = NULL;
	int error;

	KASSERT(mutex_owned(ifp->if_ioctl_lock));

	nmib = kmem_alloc(sizeof(*nmib), KM_SLEEP);

	mutex_enter(&ifv->ifv_lock);
	error = vlan_unconfig_locked(ifv, nmib);
	mutex_exit(&ifv->ifv_lock);

	if (error)
		kmem_free(nmib, sizeof(*nmib));
}
static int
vlan_unconfig_locked(struct ifvlan *ifv, struct ifvlan_linkmib *nmib)
{
	struct ifnet *p;
	struct ifnet *ifp = &ifv->ifv_if;
	struct psref_target *nmib_psref = NULL;
	struct ifvlan_linkmib *omib;
	int error = 0;

	KASSERT(mutex_owned(ifp->if_ioctl_lock));
	KASSERT(mutex_owned(&ifv->ifv_lock));

	ifp->if_flags &= ~(IFF_UP|IFF_RUNNING);

	omib = ifv->ifv_mib;
	p = omib->ifvm_p;

	if (p == NULL) {
		error = -1;
		goto done;
	}

	*nmib = *omib;
	nmib_psref = &nmib->ifvm_psref;
	psref_target_init(nmib_psref, ifvm_psref_class);

	/*
 	 * Since the interface is being unconfigured, we need to empty the
	 * list of multicast groups that we may have joined while we were
	 * alive and remove them from the parent's list also.
	 */
	(*nmib->ifvm_msw->vmsw_purgemulti)(ifv);

	/* Disconnect from parent. */
	switch (p->if_type) {
	case IFT_ETHER:
	    {
		struct ethercom *ec = (void *)p;
		if (--ec->ec_nvlans == 0) {
			mutex_enter(p->if_ioctl_lock);
			(void) ether_disable_vlan_mtu(p);
			mutex_exit(p->if_ioctl_lock);
		}

		ether_ifdetach(ifp);
		/* Restore vlan_ioctl overwritten by ether_ifdetach */
		ifp->if_ioctl = vlan_ioctl;
		vlan_reset_linkname(ifp);
		break;
	    }

#ifdef DIAGNOSTIC
	default:
		panic("vlan_unconfig: impossible");
#endif
	}

	nmib->ifvm_p = NULL;
	ifv->ifv_if.if_mtu = 0;
	ifv->ifv_flags = 0;

	mutex_enter(&ifv_hash.lock);
	PSLIST_WRITER_REMOVE(ifv, ifv_hash);
	pserialize_perform(vlan_psz);
	mutex_exit(&ifv_hash.lock);
	PSLIST_ENTRY_DESTROY(ifv, ifv_hash);

	vlan_linkmib_update(ifv, nmib);

	mutex_exit(&ifv->ifv_lock);

	nmib_psref = NULL;
	kmem_free(omib, sizeof(*omib));

#ifdef INET6
	KERNEL_LOCK_UNLESS_NET_MPSAFE();
	/* To delete v6 link local addresses */
	if (in6_present)
		in6_ifdetach(ifp);
	KERNEL_UNLOCK_UNLESS_NET_MPSAFE();
#endif

	if ((ifp->if_flags & IFF_PROMISC) != 0)
		vlan_safe_ifpromisc_locked(ifp, 0);
	if_down_locked(ifp);
	ifp->if_capabilities = 0;
	mutex_enter(&ifv->ifv_lock);
done:

	if (nmib_psref)
		psref_target_destroy(nmib_psref, ifvm_psref_class);

	return error;
}

static void
vlan_hash_init(void)
{

	ifv_hash.lists = hashinit(VLAN_TAG_HASH_SIZE, HASH_PSLIST, true,
	    &ifv_hash.mask);
}

static int
vlan_hash_fini(void)
{
	int i;

	mutex_enter(&ifv_hash.lock);

	for (i = 0; i < ifv_hash.mask + 1; i++) {
		if (PSLIST_WRITER_FIRST(&ifv_hash.lists[i], struct ifvlan,
		    ifv_hash) != NULL) {
			mutex_exit(&ifv_hash.lock);
			return EBUSY;
		}
	}

	for (i = 0; i < ifv_hash.mask + 1; i++)
		PSLIST_DESTROY(&ifv_hash.lists[i]);

	mutex_exit(&ifv_hash.lock);

	hashdone(ifv_hash.lists, HASH_PSLIST, ifv_hash.mask);

	ifv_hash.lists = NULL;
	ifv_hash.mask = 0;

	return 0;
}

static int
vlan_tag_hash(uint16_t tag, u_long mask)
{
	uint32_t hash;

	hash = (tag >> 8) ^ tag;
	hash = (hash >> 2) ^ hash;

	return hash & mask;
}

static struct ifvlan_linkmib *
vlan_getref_linkmib(struct ifvlan *sc, struct psref *psref)
{
	struct ifvlan_linkmib *mib;
	int s;

	s = pserialize_read_enter();
	mib = sc->ifv_mib;
	if (mib == NULL) {
		pserialize_read_exit(s);
		return NULL;
	}
	membar_datadep_consumer();
	psref_acquire(psref, &mib->ifvm_psref, ifvm_psref_class);
	pserialize_read_exit(s);

	return mib;
}

static void
vlan_putref_linkmib(struct ifvlan_linkmib *mib, struct psref *psref)
{
	if (mib == NULL)
		return;
	psref_release(psref, &mib->ifvm_psref, ifvm_psref_class);
}

static struct ifvlan_linkmib *
vlan_lookup_tag_psref(struct ifnet *ifp, uint16_t tag, struct psref *psref)
{
	int idx;
	int s;
	struct ifvlan *sc;

	idx = vlan_tag_hash(tag, ifv_hash.mask);

	s = pserialize_read_enter();
	PSLIST_READER_FOREACH(sc, &ifv_hash.lists[idx], struct ifvlan,
	    ifv_hash) {
		struct ifvlan_linkmib *mib = sc->ifv_mib;
		if (mib == NULL)
			continue;
		if (mib->ifvm_tag != tag)
			continue;
		if (mib->ifvm_p != ifp)
			continue;

		psref_acquire(psref, &mib->ifvm_psref, ifvm_psref_class);
		pserialize_read_exit(s);
		return mib;
	}
	pserialize_read_exit(s);
	return NULL;
}

static void
vlan_linkmib_update(struct ifvlan *ifv, struct ifvlan_linkmib *nmib)
{
	struct ifvlan_linkmib *omib = ifv->ifv_mib;

	KASSERT(mutex_owned(&ifv->ifv_lock));

	membar_producer();
	ifv->ifv_mib = nmib;

	pserialize_perform(vlan_psz);
	psref_target_destroy(&omib->ifvm_psref, ifvm_psref_class);
}

/*
 * Called when a parent interface is detaching; destroy any VLAN
 * configuration for the parent interface.
 */
void
vlan_ifdetach(struct ifnet *p)
{
	struct ifvlan *ifv;
	struct ifvlan_linkmib *mib, **nmibs;
	struct psref psref;
	int error;
	int bound;
	int i, cnt = 0;

	bound = curlwp_bind();
	mutex_enter(&ifv_list.lock);
	LIST_FOREACH(ifv, &ifv_list.list, ifv_list) {
		mib = vlan_getref_linkmib(ifv, &psref);
		if (mib == NULL)
			continue;

		if (mib->ifvm_p == p)
			cnt++;

		vlan_putref_linkmib(mib, &psref);
	}
	mutex_exit(&ifv_list.lock);

	/*
	 * The value of "cnt" does not increase while ifv_list.lock
	 * and ifv->ifv_lock are released here, because the parent
	 * interface is detaching.
	 */
	nmibs = kmem_alloc(sizeof(*nmibs) * cnt, KM_SLEEP);
	for (i=0; i < cnt; i++) {
		nmibs[i] = kmem_alloc(sizeof(*nmibs[i]), KM_SLEEP);
	}

	mutex_enter(&ifv_list.lock);

	i = 0;
	LIST_FOREACH(ifv, &ifv_list.list, ifv_list) {
		struct ifnet *ifp = &ifv->ifv_if;

		/* Need if_ioctl_lock that must be held before ifv_lock. */
		mutex_enter(ifp->if_ioctl_lock);
		mutex_enter(&ifv->ifv_lock);
		if (ifv->ifv_mib->ifvm_p == p) {
			KASSERTMSG(i < cnt, "no memory for unconfig, parent=%s",
			    p->if_xname);
			error = vlan_unconfig_locked(ifv, nmibs[i]);
			if (!error) {
				nmibs[i] = NULL;
				i++;
			}

		}
		mutex_exit(&ifv->ifv_lock);
		mutex_exit(ifp->if_ioctl_lock);
	}

	mutex_exit(&ifv_list.lock);
	curlwp_bindx(bound);

	for (i=0; i < cnt; i++) {
		if (nmibs[i])
			kmem_free(nmibs[i], sizeof(*nmibs[i]));
	}

	kmem_free(nmibs, sizeof(*nmibs) * cnt);

	return;
}

static int
vlan_set_promisc(struct ifnet *ifp)
{
	struct ifvlan *ifv = ifp->if_softc;
	struct ifvlan_linkmib *mib;
	struct psref psref;
	int error = 0;
	int bound;

	bound = curlwp_bind();
	mib = vlan_getref_linkmib(ifv, &psref);
	if (mib == NULL) {
		curlwp_bindx(bound);
		return EBUSY;
	}

	if ((ifp->if_flags & IFF_PROMISC) != 0) {
		if ((ifv->ifv_flags & IFVF_PROMISC) == 0) {
			error = vlan_safe_ifpromisc(mib->ifvm_p, 1);
			if (error == 0)
				ifv->ifv_flags |= IFVF_PROMISC;
		}
	} else {
		if ((ifv->ifv_flags & IFVF_PROMISC) != 0) {
			error = vlan_safe_ifpromisc(mib->ifvm_p, 0);
			if (error == 0)
				ifv->ifv_flags &= ~IFVF_PROMISC;
		}
	}
	vlan_putref_linkmib(mib, &psref);
	curlwp_bindx(bound);

	return (error);
}

static int
vlan_ioctl(struct ifnet *ifp, u_long cmd, void *data)
{
	struct lwp *l = curlwp;	/* XXX */
	struct ifvlan *ifv = ifp->if_softc;
	struct ifaddr *ifa = (struct ifaddr *) data;
	struct ifreq *ifr = (struct ifreq *) data;
	struct ifnet *pr;
	struct ifcapreq *ifcr;
	struct vlanreq vlr;
	struct ifvlan_linkmib *mib;
	struct psref psref;
	int error = 0;
	int bound;

	switch (cmd) {
	case SIOCSIFMTU:
		bound = curlwp_bind();
		mib = vlan_getref_linkmib(ifv, &psref);
		if (mib == NULL) {
			curlwp_bindx(bound);
			error = EBUSY;
			break;
		}

		if (mib->ifvm_p == NULL) {
			vlan_putref_linkmib(mib, &psref);
			curlwp_bindx(bound);
			error = EINVAL;
		} else if (
		    ifr->ifr_mtu > (mib->ifvm_p->if_mtu - mib->ifvm_mtufudge) ||
		    ifr->ifr_mtu < (mib->ifvm_mintu - mib->ifvm_mtufudge)) {
			vlan_putref_linkmib(mib, &psref);
			curlwp_bindx(bound);
			error = EINVAL;
		} else {
			vlan_putref_linkmib(mib, &psref);
			curlwp_bindx(bound);

			error = ifioctl_common(ifp, cmd, data);
			if (error == ENETRESET)
					error = 0;
		}

		break;

	case SIOCSETVLAN:
		if ((error = kauth_authorize_network(l->l_cred,
		    KAUTH_NETWORK_INTERFACE,
		    KAUTH_REQ_NETWORK_INTERFACE_SETPRIV, ifp, (void *)cmd,
		    NULL)) != 0)
			break;
		if ((error = copyin(ifr->ifr_data, &vlr, sizeof(vlr))) != 0)
			break;

		if (vlr.vlr_parent[0] == '\0') {
			bound = curlwp_bind();
			mib = vlan_getref_linkmib(ifv, &psref);
			if (mib == NULL) {
				curlwp_bindx(bound);
				error = EBUSY;
				break;
			}

			if (mib->ifvm_p != NULL &&
			    (ifp->if_flags & IFF_PROMISC) != 0)
				error = vlan_safe_ifpromisc(mib->ifvm_p, 0);

			vlan_putref_linkmib(mib, &psref);
			curlwp_bindx(bound);

			vlan_unconfig(ifp);
			break;
		}
		if (vlr.vlr_tag != EVL_VLANOFTAG(vlr.vlr_tag)) {
			error = EINVAL;		 /* check for valid tag */
			break;
		}
		if ((pr = ifunit(vlr.vlr_parent)) == NULL) {
			error = ENOENT;
			break;
		}
		error = vlan_config(ifv, pr, vlr.vlr_tag);
		if (error != 0) {
			break;
		}

		/* Update promiscuous mode, if necessary. */
		vlan_set_promisc(ifp);

		ifp->if_flags |= IFF_RUNNING;
		break;

	case SIOCGETVLAN:
		memset(&vlr, 0, sizeof(vlr));
		bound = curlwp_bind();
		mib = vlan_getref_linkmib(ifv, &psref);
		if (mib == NULL) {
			curlwp_bindx(bound);
			error = EBUSY;
			break;
		}
		if (mib->ifvm_p != NULL) {
			snprintf(vlr.vlr_parent, sizeof(vlr.vlr_parent), "%s",
			    mib->ifvm_p->if_xname);
			vlr.vlr_tag = mib->ifvm_tag;
		}
		vlan_putref_linkmib(mib, &psref);
		curlwp_bindx(bound);
		error = copyout(&vlr, ifr->ifr_data, sizeof(vlr));
		break;

	case SIOCSIFFLAGS:
		if ((error = ifioctl_common(ifp, cmd, data)) != 0)
			break;
		/*
		 * For promiscuous mode, we enable promiscuous mode on
		 * the parent if we need promiscuous on the VLAN interface.
		 */
		bound = curlwp_bind();
		mib = vlan_getref_linkmib(ifv, &psref);
		if (mib == NULL) {
			curlwp_bindx(bound);
			error = EBUSY;
			break;
		}

		if (mib->ifvm_p != NULL)
			error = vlan_set_promisc(ifp);
		vlan_putref_linkmib(mib, &psref);
		curlwp_bindx(bound);
		break;

	case SIOCADDMULTI:
		mutex_enter(&ifv->ifv_lock);
		mib = ifv->ifv_mib;
		if (mib == NULL) {
			error = EBUSY;
			mutex_exit(&ifv->ifv_lock);
			break;
		}

		error = (mib->ifvm_p != NULL) ?
		    (*mib->ifvm_msw->vmsw_addmulti)(ifv, ifr) : EINVAL;
		mib = NULL;
		mutex_exit(&ifv->ifv_lock);
		break;

	case SIOCDELMULTI:
		mutex_enter(&ifv->ifv_lock);
		mib = ifv->ifv_mib;
		if (mib == NULL) {
			error = EBUSY;
			mutex_exit(&ifv->ifv_lock);
			break;
		}
		error = (mib->ifvm_p != NULL) ?
		    (*mib->ifvm_msw->vmsw_delmulti)(ifv, ifr) : EINVAL;
		mib = NULL;
		mutex_exit(&ifv->ifv_lock);
		break;

	case SIOCSIFCAP:
		ifcr = data;
		/* make sure caps are enabled on parent */
		bound = curlwp_bind();
		mib = vlan_getref_linkmib(ifv, &psref);
		if (mib == NULL) {
			curlwp_bindx(bound);
			error = EBUSY;
			break;
		}

		if (mib->ifvm_p == NULL) {
			vlan_putref_linkmib(mib, &psref);
			curlwp_bindx(bound);
			error = EINVAL;
			break;
		}
		if ((mib->ifvm_p->if_capenable & ifcr->ifcr_capenable) !=
		    ifcr->ifcr_capenable) {
			vlan_putref_linkmib(mib, &psref);
			curlwp_bindx(bound);
			error = EINVAL;
			break;
		}

		vlan_putref_linkmib(mib, &psref);
		curlwp_bindx(bound);

		if ((error = ifioctl_common(ifp, cmd, data)) == ENETRESET)
			error = 0;
		break;
	case SIOCINITIFADDR:
		bound = curlwp_bind();
		mib = vlan_getref_linkmib(ifv, &psref);
		if (mib == NULL) {
			curlwp_bindx(bound);
			error = EBUSY;
			break;
		}

		if (mib->ifvm_p == NULL) {
			error = EINVAL;
			vlan_putref_linkmib(mib, &psref);
			curlwp_bindx(bound);
			break;
		}
		vlan_putref_linkmib(mib, &psref);
		curlwp_bindx(bound);

		ifp->if_flags |= IFF_UP;
#ifdef INET
		if (ifa->ifa_addr->sa_family == AF_INET)
			arp_ifinit(ifp, ifa);
#endif
		break;

	default:
		error = ether_ioctl(ifp, cmd, data);
	}

	return (error);
}

static int
vlan_ether_addmulti(struct ifvlan *ifv, struct ifreq *ifr)
{
	const struct sockaddr *sa = ifreq_getaddr(SIOCADDMULTI, ifr);
	struct vlan_mc_entry *mc;
	uint8_t addrlo[ETHER_ADDR_LEN], addrhi[ETHER_ADDR_LEN];
	struct ifvlan_linkmib *mib;
	int error;

	KASSERT(mutex_owned(&ifv->ifv_lock));

	if (sa->sa_len > sizeof(struct sockaddr_storage))
		return (EINVAL);

	error = ether_addmulti(sa, &ifv->ifv_ec);
	if (error != ENETRESET)
		return (error);

	/*
	 * This is new multicast address.  We have to tell parent
	 * about it.  Also, remember this multicast address so that
	 * we can delete them on unconfigure.
	 */
	mc = malloc(sizeof(struct vlan_mc_entry), M_DEVBUF, M_NOWAIT);
	if (mc == NULL) {
		error = ENOMEM;
		goto alloc_failed;
	}

	/*
	 * As ether_addmulti() returns ENETRESET, following two
	 * statement shouldn't fail.
	 */
	(void)ether_multiaddr(sa, addrlo, addrhi);
	ETHER_LOOKUP_MULTI(addrlo, addrhi, &ifv->ifv_ec, mc->mc_enm);
	memcpy(&mc->mc_addr, sa, sa->sa_len);
	LIST_INSERT_HEAD(&ifv->ifv_mc_listhead, mc, mc_entries);

	mib = ifv->ifv_mib;

	KERNEL_LOCK_UNLESS_IFP_MPSAFE(mib->ifvm_p);
	error = if_mcast_op(mib->ifvm_p, SIOCADDMULTI, sa);
	KERNEL_UNLOCK_UNLESS_IFP_MPSAFE(mib->ifvm_p);

	if (error != 0)
		goto ioctl_failed;
	return (error);

 ioctl_failed:
	LIST_REMOVE(mc, mc_entries);
	free(mc, M_DEVBUF);
 alloc_failed:
	(void)ether_delmulti(sa, &ifv->ifv_ec);
	return (error);
}

static int
vlan_ether_delmulti(struct ifvlan *ifv, struct ifreq *ifr)
{
	const struct sockaddr *sa = ifreq_getaddr(SIOCDELMULTI, ifr);
	struct ether_multi *enm;
	struct vlan_mc_entry *mc;
	struct ifvlan_linkmib *mib;
	uint8_t addrlo[ETHER_ADDR_LEN], addrhi[ETHER_ADDR_LEN];
	int error;

	KASSERT(mutex_owned(&ifv->ifv_lock));

	/*
	 * Find a key to lookup vlan_mc_entry.  We have to do this
	 * before calling ether_delmulti for obvious reason.
	 */
	if ((error = ether_multiaddr(sa, addrlo, addrhi)) != 0)
		return (error);
	ETHER_LOOKUP_MULTI(addrlo, addrhi, &ifv->ifv_ec, enm);

	error = ether_delmulti(sa, &ifv->ifv_ec);
	if (error != ENETRESET)
		return (error);

	/* We no longer use this multicast address.  Tell parent so. */
	mib = ifv->ifv_mib;
	error = if_mcast_op(mib->ifvm_p, SIOCDELMULTI, sa);

	if (error == 0) {
		/* And forget about this address. */
		for (mc = LIST_FIRST(&ifv->ifv_mc_listhead); mc != NULL;
		    mc = LIST_NEXT(mc, mc_entries)) {
			if (mc->mc_enm == enm) {
				LIST_REMOVE(mc, mc_entries);
				free(mc, M_DEVBUF);
				break;
			}
		}
		KASSERT(mc != NULL);
	} else
		(void)ether_addmulti(sa, &ifv->ifv_ec);
	return (error);
}

/*
 * Delete any multicast address we have asked to add from parent
 * interface.  Called when the vlan is being unconfigured.
 */
static void
vlan_ether_purgemulti(struct ifvlan *ifv)
{
	struct vlan_mc_entry *mc;
	struct ifvlan_linkmib *mib;

	KASSERT(mutex_owned(&ifv->ifv_lock));
	mib = ifv->ifv_mib;
	if (mib == NULL) {
		return;
	}

	while ((mc = LIST_FIRST(&ifv->ifv_mc_listhead)) != NULL) {
		(void)if_mcast_op(mib->ifvm_p, SIOCDELMULTI,
		    (const struct sockaddr *)&mc->mc_addr);
		LIST_REMOVE(mc, mc_entries);
		free(mc, M_DEVBUF);
	}
}

static void
vlan_start(struct ifnet *ifp)
{
	struct ifvlan *ifv = ifp->if_softc;
	struct ifnet *p;
	struct ethercom *ec;
	struct mbuf *m;
	struct ifvlan_linkmib *mib;
	struct psref psref;
	int error;

	mib = vlan_getref_linkmib(ifv, &psref);
	if (mib == NULL)
		return;
	p = mib->ifvm_p;
	ec = (void *)mib->ifvm_p;

	ifp->if_flags |= IFF_OACTIVE;

	for (;;) {
		IFQ_DEQUEUE(&ifp->if_snd, m);
		if (m == NULL)
			break;

#ifdef ALTQ
		/*
		 * KERNEL_LOCK is required for ALTQ even if NET_MPSAFE is defined.
		 */
		KERNEL_LOCK(1, NULL);
		/*
		 * If ALTQ is enabled on the parent interface, do
		 * classification; the queueing discipline might
		 * not require classification, but might require
		 * the address family/header pointer in the pktattr.
		 */
		if (ALTQ_IS_ENABLED(&p->if_snd)) {
			switch (p->if_type) {
			case IFT_ETHER:
				altq_etherclassify(&p->if_snd, m);
				break;
#ifdef DIAGNOSTIC
			default:
				panic("vlan_start: impossible (altq)");
#endif
			}
		}
		KERNEL_UNLOCK_ONE(NULL);
#endif /* ALTQ */

		bpf_mtap(ifp, m);
		/*
		 * If the parent can insert the tag itself, just mark
		 * the tag in the mbuf header.
		 */
		if (ec->ec_capabilities & ETHERCAP_VLAN_HWTAGGING) {
			vlan_set_tag(m, mib->ifvm_tag);
		} else {
			/*
			 * insert the tag ourselves
			 */
			M_PREPEND(m, mib->ifvm_encaplen, M_DONTWAIT);
			if (m == NULL) {
				printf("%s: unable to prepend encap header",
				    p->if_xname);
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
					    "header", p->if_xname);
					ifp->if_oerrors++;
					continue;
				}

				/*
				 * Transform the Ethernet header into an
				 * Ethernet header with 802.1Q encapsulation.
				 */
				memmove(mtod(m, void *),
				    mtod(m, char *) + mib->ifvm_encaplen,
				    sizeof(struct ether_header));
				evl = mtod(m, struct ether_vlan_header *);
				evl->evl_proto = evl->evl_encap_proto;
				evl->evl_encap_proto = htons(ETHERTYPE_VLAN);
				evl->evl_tag = htons(mib->ifvm_tag);

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
				    (ETHER_MIN_LEN - ETHER_CRC_LEN +
				     ETHER_VLAN_ENCAP_LEN)) {
					m_copyback(m, m->m_pkthdr.len,
					    (ETHER_MIN_LEN - ETHER_CRC_LEN +
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

		if ((p->if_flags & IFF_RUNNING) == 0) {
			m_freem(m);
			continue;
		}

		error = if_transmit_lock(p, m);
		if (error) {
			/* mbuf is already freed */
			ifp->if_oerrors++;
			continue;
		}
		ifp->if_opackets++;
	}

	ifp->if_flags &= ~IFF_OACTIVE;

	/* Remove reference to mib before release */
	p = NULL;
	ec = NULL;

	vlan_putref_linkmib(mib, &psref);
}

static int
vlan_transmit(struct ifnet *ifp, struct mbuf *m)
{
	struct ifvlan *ifv = ifp->if_softc;
	struct ifnet *p;
	struct ethercom *ec;
	struct ifvlan_linkmib *mib;
	struct psref psref;
	int error;
	size_t pktlen = m->m_pkthdr.len;
	bool mcast = (m->m_flags & M_MCAST) != 0;

	mib = vlan_getref_linkmib(ifv, &psref);
	if (mib == NULL) {
		m_freem(m);
		return ENETDOWN;
	}

	p = mib->ifvm_p;
	ec = (void *)mib->ifvm_p;

	bpf_mtap(ifp, m);

	if (pfil_run_hooks(ifp->if_pfil, &m, ifp, PFIL_OUT) != 0) {
		if (m != NULL)
			m_freem(m);
		error = 0;
		goto out;
	}

	/*
	 * If the parent can insert the tag itself, just mark
	 * the tag in the mbuf header.
	 */
	if (ec->ec_capabilities & ETHERCAP_VLAN_HWTAGGING) {
		vlan_set_tag(m, mib->ifvm_tag);
	} else {
		/*
		 * insert the tag ourselves
		 */
		M_PREPEND(m, mib->ifvm_encaplen, M_DONTWAIT);
		if (m == NULL) {
			printf("%s: unable to prepend encap header",
			    p->if_xname);
			ifp->if_oerrors++;
			error = ENOBUFS;
			goto out;
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
				    "header", p->if_xname);
				ifp->if_oerrors++;
				error = ENOBUFS;
				goto out;
			}

			/*
			 * Transform the Ethernet header into an
			 * Ethernet header with 802.1Q encapsulation.
			 */
			memmove(mtod(m, void *),
			    mtod(m, char *) + mib->ifvm_encaplen,
			    sizeof(struct ether_header));
			evl = mtod(m, struct ether_vlan_header *);
			evl->evl_proto = evl->evl_encap_proto;
			evl->evl_encap_proto = htons(ETHERTYPE_VLAN);
			evl->evl_tag = htons(mib->ifvm_tag);

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
			    (ETHER_MIN_LEN - ETHER_CRC_LEN +
			     ETHER_VLAN_ENCAP_LEN)) {
				m_copyback(m, m->m_pkthdr.len,
				    (ETHER_MIN_LEN - ETHER_CRC_LEN +
				     ETHER_VLAN_ENCAP_LEN) -
				     m->m_pkthdr.len,
				    vlan_zero_pad_buff);
			}
			break;
		    }

#ifdef DIAGNOSTIC
		default:
			panic("vlan_transmit: impossible");
#endif
		}
	}

	if ((p->if_flags & IFF_RUNNING) == 0) {
		m_freem(m);
		error = ENETDOWN;
		goto out;
	}

	error = if_transmit_lock(p, m);
	if (error) {
		/* mbuf is already freed */
		ifp->if_oerrors++;
	} else {

		ifp->if_opackets++;
		ifp->if_obytes += pktlen;
		if (mcast)
			ifp->if_omcasts++;
	}

out:
	/* Remove reference to mib before release */
	p = NULL;
	ec = NULL;

	vlan_putref_linkmib(mib, &psref);
	return error;
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
	uint16_t vid;
	struct ifvlan_linkmib *mib;
	struct psref psref;
	bool have_vtag;

	have_vtag = vlan_has_tag(m);
	if (have_vtag) {
		vid = EVL_VLANOFTAG(vlan_get_tag(m));
		m->m_flags &= ~M_VLANTAG;
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

			vid = EVL_VLANOFTAG(ntohs(evl->evl_tag));

			/*
			 * Restore the original ethertype.  We'll remove
			 * the encapsulation after we've found the vlan
			 * interface corresponding to the tag.
			 */
			evl->evl_encap_proto = evl->evl_proto;
			break;
		    }

		default:
			vid = (uint16_t) -1;	/* XXX GCC */
#ifdef DIAGNOSTIC
			panic("vlan_input: impossible");
#endif
		}
	}

	mib = vlan_lookup_tag_psref(ifp, vid, &psref);
	if (mib == NULL) {
		m_freem(m);
		ifp->if_noproto++;
		return;
	}

	ifv = mib->ifvm_ifvlan;
	if ((ifv->ifv_if.if_flags & (IFF_UP|IFF_RUNNING)) !=
	    (IFF_UP|IFF_RUNNING)) {
		m_freem(m);
		ifp->if_noproto++;
		goto out;
	}

	/*
	 * Now, remove the encapsulation header.  The original
	 * header has already been fixed up above.
	 */
	if (!have_vtag) {
		memmove(mtod(m, char *) + mib->ifvm_encaplen,
		    mtod(m, void *), sizeof(struct ether_header));
		m_adj(m, mib->ifvm_encaplen);
	}

	m_set_rcvif(m, &ifv->ifv_if);
	ifv->ifv_if.if_ipackets++;

	if (pfil_run_hooks(ifp->if_pfil, &m, ifp, PFIL_IN) != 0) {
		if (m != NULL)
			m_freem(m);
		goto out;
	}

	m->m_flags &= ~M_PROMISC;
	if_input(&ifv->ifv_if, m);
out:
	vlan_putref_linkmib(mib, &psref);
}

/*
 * Module infrastructure
 */
#include "if_module.h"

IF_MODULE(MODULE_CLASS_DRIVER, vlan, "")
