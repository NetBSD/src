/*	$NetBSD: if_l2tp.c,v 1.24 2018/04/27 09:55:27 knakahara Exp $	*/

/*
 * Copyright (c) 2017 Internet Initiative Japan Inc.
 * All rights reserved.
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
 * L2TPv3 kernel interface
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: if_l2tp.c,v 1.24 2018/04/27 09:55:27 knakahara Exp $");

#ifdef _KERNEL_OPT
#include "opt_inet.h"
#include "opt_net_mpsafe.h"
#endif

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/mbuf.h>
#include <sys/socket.h>
#include <sys/sockio.h>
#include <sys/errno.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <sys/syslog.h>
#include <sys/proc.h>
#include <sys/conf.h>
#include <sys/kauth.h>
#include <sys/cpu.h>
#include <sys/cprng.h>
#include <sys/intr.h>
#include <sys/kmem.h>
#include <sys/mutex.h>
#include <sys/atomic.h>
#include <sys/pserialize.h>
#include <sys/device.h>
#include <sys/module.h>

#include <net/if.h>
#include <net/if_dl.h>
#include <net/if_ether.h>
#include <net/if_types.h>
#include <net/netisr.h>
#include <net/route.h>
#include <net/bpf.h>
#include <net/if_vlanvar.h>

#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include <netinet/ip_encap.h>
#ifdef	INET
#include <netinet/in_var.h>
#include <netinet/in_l2tp.h>
#endif	/* INET */
#ifdef INET6
#include <netinet6/in6_l2tp.h>
#endif

#include <net/if_l2tp.h>

#include <net/if_vlanvar.h>

/* TODO: IP_TCPMSS support */
#undef IP_TCPMSS
#ifdef IP_TCPMSS
#include <netinet/ip_tcpmss.h>
#endif

#include <net/bpf.h>
#include <net/net_osdep.h>

/*
 * l2tp global variable definitions
 */
LIST_HEAD(l2tp_sclist, l2tp_softc);
static struct {
	struct l2tp_sclist list;
	kmutex_t lock;
} l2tp_softcs __cacheline_aligned;


#if !defined(L2TP_ID_HASH_SIZE)
#define L2TP_ID_HASH_SIZE 64
#endif
static struct {
	kmutex_t lock;
	struct pslist_head *lists;
	u_long mask;
} l2tp_hash __cacheline_aligned = {
	.lists = NULL,
};

pserialize_t l2tp_psz __read_mostly;
struct psref_class *lv_psref_class __read_mostly;

static void	l2tp_ro_init_pc(void *, void *, struct cpu_info *);
static void	l2tp_ro_fini_pc(void *, void *, struct cpu_info *);

static int	l2tp_clone_create(struct if_clone *, int);
static int	l2tp_clone_destroy(struct ifnet *);

struct if_clone l2tp_cloner =
    IF_CLONE_INITIALIZER("l2tp", l2tp_clone_create, l2tp_clone_destroy);

static int	l2tp_output(struct ifnet *, struct mbuf *,
		    const struct sockaddr *, const struct rtentry *);
static void	l2tpintr(struct l2tp_variant *);

static void	l2tp_hash_init(void);
static int	l2tp_hash_fini(void);

static void	l2tp_start(struct ifnet *);
static int	l2tp_transmit(struct ifnet *, struct mbuf *);

static int	l2tp_set_tunnel(struct ifnet *, struct sockaddr *,
		    struct sockaddr *);
static void	l2tp_delete_tunnel(struct ifnet *);

static int	id_hash_func(uint32_t, u_long);

static void	l2tp_variant_update(struct l2tp_softc *, struct l2tp_variant *);
static int	l2tp_set_session(struct l2tp_softc *, uint32_t, uint32_t);
static int	l2tp_clear_session(struct l2tp_softc *);
static int	l2tp_set_cookie(struct l2tp_softc *, uint64_t, u_int, uint64_t, u_int);
static void	l2tp_clear_cookie(struct l2tp_softc *);
static void	l2tp_set_state(struct l2tp_softc *, int);
static int	l2tp_encap_attach(struct l2tp_variant *);
static int	l2tp_encap_detach(struct l2tp_variant *);

#ifndef MAX_L2TP_NEST
/*
 * This macro controls the upper limitation on nesting of l2tp tunnels.
 * Since, setting a large value to this macro with a careless configuration
 * may introduce system crash, we don't allow any nestings by default.
 * If you need to configure nested l2tp tunnels, you can define this macro
 * in your kernel configuration file.  However, if you do so, please be
 * careful to configure the tunnels so that it won't make a loop.
 */
/*
 * XXX
 * Currently, if in_l2tp_output recursively calls, it causes locking against
 * myself of struct l2tp_ro->lr_lock. So, nested l2tp tunnels is prohibited.
 */
#define MAX_L2TP_NEST 0
#endif

static int max_l2tp_nesting = MAX_L2TP_NEST;

/* ARGSUSED */
void
l2tpattach(int count)
{
	/*
	 * Nothing to do here, initialization is handled by the
	 * module initialization code in l2tpinit() below).
	 */
}

static void
l2tpinit(void)
{

	mutex_init(&l2tp_softcs.lock, MUTEX_DEFAULT, IPL_NONE);
	LIST_INIT(&l2tp_softcs.list);

	mutex_init(&l2tp_hash.lock, MUTEX_DEFAULT, IPL_NONE);
	l2tp_psz = pserialize_create();
	lv_psref_class = psref_class_create("l2tpvar", IPL_SOFTNET);
	if_clone_attach(&l2tp_cloner);

	l2tp_hash_init();
}

static int
l2tpdetach(void)
{
	int error;

	mutex_enter(&l2tp_softcs.lock);
	if (!LIST_EMPTY(&l2tp_softcs.list)) {
		mutex_exit(&l2tp_softcs.lock);
		return EBUSY;
	}
	mutex_exit(&l2tp_softcs.lock);

	error = l2tp_hash_fini();
	if (error)
		return error;

	if_clone_detach(&l2tp_cloner);
	psref_class_destroy(lv_psref_class);
	pserialize_destroy(l2tp_psz);
	mutex_destroy(&l2tp_hash.lock);

	mutex_destroy(&l2tp_softcs.lock);

	return error;
}

static int
l2tp_clone_create(struct if_clone *ifc, int unit)
{
	struct l2tp_softc *sc;
	struct l2tp_variant *var;
	int rv;

	sc = kmem_zalloc(sizeof(struct l2tp_softc), KM_SLEEP);
	if_initname(&sc->l2tp_ec.ec_if, ifc->ifc_name, unit);
	rv = l2tpattach0(sc);
	if (rv != 0) {
		kmem_free(sc, sizeof(struct l2tp_softc));
		return rv;
	}

	var = kmem_zalloc(sizeof(struct l2tp_variant), KM_SLEEP);
	var->lv_softc = sc;
	var->lv_state = L2TP_STATE_DOWN;
	var->lv_use_cookie = L2TP_COOKIE_OFF;
	psref_target_init(&var->lv_psref, lv_psref_class);

	sc->l2tp_var = var;
	mutex_init(&sc->l2tp_lock, MUTEX_DEFAULT, IPL_NONE);
	PSLIST_ENTRY_INIT(sc, l2tp_hash);

	sc->l2tp_ro_percpu = percpu_alloc(sizeof(struct l2tp_ro));
	percpu_foreach(sc->l2tp_ro_percpu, l2tp_ro_init_pc, NULL);

	mutex_enter(&l2tp_softcs.lock);
	LIST_INSERT_HEAD(&l2tp_softcs.list, sc, l2tp_list);
	mutex_exit(&l2tp_softcs.lock);

	return (0);
}

int
l2tpattach0(struct l2tp_softc *sc)
{
	int rv;

	sc->l2tp_ec.ec_if.if_addrlen = 0;
	sc->l2tp_ec.ec_if.if_mtu    = L2TP_MTU;
	sc->l2tp_ec.ec_if.if_flags  = IFF_POINTOPOINT|IFF_MULTICAST|IFF_SIMPLEX;
	sc->l2tp_ec.ec_if.if_extflags = IFEF_NO_LINK_STATE_CHANGE;
#ifdef NET_MPSAFE
	sc->l2tp_ec.ec_if.if_extflags |= IFEF_MPSAFE;
#endif
	sc->l2tp_ec.ec_if.if_ioctl  = l2tp_ioctl;
	sc->l2tp_ec.ec_if.if_output = l2tp_output;
	sc->l2tp_ec.ec_if.if_type   = IFT_L2TP;
	sc->l2tp_ec.ec_if.if_dlt    = DLT_NULL;
	sc->l2tp_ec.ec_if.if_start  = l2tp_start;
	sc->l2tp_ec.ec_if.if_transmit = l2tp_transmit;
	sc->l2tp_ec.ec_if._if_input = ether_input;
	IFQ_SET_READY(&sc->l2tp_ec.ec_if.if_snd);
	/* XXX
	 * It may improve performance to use if_initialize()/if_register()
	 * so that l2tp_input() calls if_input() instead of
	 * if_percpuq_enqueue(). However, that causes recursive softnet_lock
	 * when NET_MPSAFE is not set.
	 */
	rv = if_attach(&sc->l2tp_ec.ec_if);
	if (rv != 0)
		return rv;
	if_alloc_sadl(&sc->l2tp_ec.ec_if);
	bpf_attach(&sc->l2tp_ec.ec_if, DLT_EN10MB, sizeof(struct ether_header));

	return 0;
}

void
l2tp_ro_init_pc(void *p, void *arg __unused, struct cpu_info *ci __unused)
{
	struct l2tp_ro *lro = p;

	lro->lr_lock = mutex_obj_alloc(MUTEX_DEFAULT, IPL_NONE);
}

void
l2tp_ro_fini_pc(void *p, void *arg __unused, struct cpu_info *ci __unused)
{
	struct l2tp_ro *lro = p;

	rtcache_free(&lro->lr_ro);

	mutex_obj_free(lro->lr_lock);
}

static int
l2tp_clone_destroy(struct ifnet *ifp)
{
	struct l2tp_variant *var;
	struct l2tp_softc *sc = container_of(ifp, struct l2tp_softc,
	    l2tp_ec.ec_if);

	l2tp_clear_session(sc);
	l2tp_delete_tunnel(&sc->l2tp_ec.ec_if);
	/*
	 * To avoid for l2tp_transmit() to access sc->l2tp_var after free it.
	 */
	mutex_enter(&sc->l2tp_lock);
	var = sc->l2tp_var;
	l2tp_variant_update(sc, NULL);
	mutex_exit(&sc->l2tp_lock);

	mutex_enter(&l2tp_softcs.lock);
	LIST_REMOVE(sc, l2tp_list);
	mutex_exit(&l2tp_softcs.lock);

	bpf_detach(ifp);

	if_detach(ifp);

	percpu_foreach(sc->l2tp_ro_percpu, l2tp_ro_fini_pc, NULL);
	percpu_free(sc->l2tp_ro_percpu, sizeof(struct l2tp_ro));

	kmem_free(var, sizeof(struct l2tp_variant));
	mutex_destroy(&sc->l2tp_lock);
	kmem_free(sc, sizeof(struct l2tp_softc));

	return 0;
}

static int
l2tp_output(struct ifnet *ifp, struct mbuf *m, const struct sockaddr *dst,
    const struct rtentry *rt)
{
	struct l2tp_softc *sc = container_of(ifp, struct l2tp_softc,
	    l2tp_ec.ec_if);
	struct l2tp_variant *var;
	struct psref psref;
	int error = 0;

	var = l2tp_getref_variant(sc, &psref);
	if (var == NULL) {
		m_freem(m);
		return ENETDOWN;
	}

	IFQ_CLASSIFY(&ifp->if_snd, m, dst->sa_family);

	m->m_flags &= ~(M_BCAST|M_MCAST);

	if ((ifp->if_flags & IFF_UP) == 0) {
		m_freem(m);
		error = ENETDOWN;
		goto end;
	}

	if (var->lv_psrc == NULL || var->lv_pdst == NULL) {
		m_freem(m);
		error = ENETDOWN;
		goto end;
	}

	/* XXX should we check if our outer source is legal? */

	/* use DLT_NULL encapsulation here to pass inner af type */
	M_PREPEND(m, sizeof(int), M_DONTWAIT);
	if (!m) {
		error = ENOBUFS;
		goto end;
	}
	*mtod(m, int *) = dst->sa_family;

	IFQ_ENQUEUE(&ifp->if_snd, m, error);
	if (error)
		goto end;

	/*
	 * direct call to avoid infinite loop at l2tpintr()
	 */
	l2tpintr(var);

	error = 0;

end:
	l2tp_putref_variant(var, &psref);
	if (error)
		ifp->if_oerrors++;

	return error;
}

static void
l2tpintr(struct l2tp_variant *var)
{
	struct l2tp_softc *sc;
	struct ifnet *ifp;
	struct mbuf *m;
	int error;

	KASSERT(psref_held(&var->lv_psref, lv_psref_class));

	sc = var->lv_softc;
	ifp = &sc->l2tp_ec.ec_if;

	/* output processing */
	if (var->lv_my_sess_id == 0 || var->lv_peer_sess_id == 0) {
		IFQ_PURGE(&ifp->if_snd);
		return;
	}

	for (;;) {
		IFQ_DEQUEUE(&ifp->if_snd, m);
		if (m == NULL)
			break;
		m->m_flags &= ~(M_BCAST|M_MCAST);
		bpf_mtap(ifp, m);
		switch (var->lv_psrc->sa_family) {
#ifdef INET
		case AF_INET:
			error = in_l2tp_output(var, m);
			break;
#endif
#ifdef INET6
		case AF_INET6:
			error = in6_l2tp_output(var, m);
			break;
#endif
		default:
			m_freem(m);
			error = ENETDOWN;
			break;
		}

		if (error)
			ifp->if_oerrors++;
		else {
			ifp->if_opackets++;
			/*
			 * obytes is incremented at ether_output() or
			 * bridge_enqueue().
			 */
		}
	}

}

void
l2tp_input(struct mbuf *m, struct ifnet *ifp)
{
	vaddr_t addr;

	KASSERT(ifp != NULL);

	/*
	 * Currently, l2tp(4) supports only ethernet as inner protocol.
	 */
	if (m->m_pkthdr.len < sizeof(struct ether_header)) {
		m_freem(m);
		return;
	}

	/*
	 * If the head of the payload is not aligned, align it.
	 */
	addr = mtod(m, vaddr_t);
	if ((addr & 0x03) != 0x2) {
		/* copy and align head of payload */
		struct mbuf *m_head;
		int copy_length;
		u_int pad = roundup(sizeof(struct ether_header), 4)
			- sizeof(struct ether_header);

#define L2TP_COPY_LENGTH		60

		if (m->m_pkthdr.len < L2TP_COPY_LENGTH) {
			copy_length = m->m_pkthdr.len;
		} else {
			copy_length = L2TP_COPY_LENGTH;
		}

		if (m->m_len < copy_length) {
			m = m_pullup(m, copy_length);
			if (m == NULL)
				return;
		}

		MGETHDR(m_head, M_DONTWAIT, MT_HEADER);
		if (m_head == NULL) {
			m_freem(m);
			return;
		}
		M_COPY_PKTHDR(m_head, m);

		/*
		 * m_head should be:
		 *                             L2TP_COPY_LENGTH
		 *                          <-  + roundup(pad, 4) - pad ->
		 *   +-------+--------+-----+--------------+-------------+
		 *   | m_hdr | pkthdr | ... | ether header |   payload   |
		 *   +-------+--------+-----+--------------+-------------+
		 *                          ^              ^
		 *                          m_data         4 byte aligned
		 */
		MH_ALIGN(m_head, L2TP_COPY_LENGTH + roundup(pad, 4));
		m_head->m_data += pad;

		memcpy(mtod(m_head, void *), mtod(m, void *), copy_length);
		m_head->m_len = copy_length;
		m->m_data += copy_length;
		m->m_len -= copy_length;

		/* construct chain */
		if (m->m_len == 0) {
			m_head->m_next = m_free(m);
		} else {
			/*
			 * Already copied mtag with M_COPY_PKTHDR.
			 * but don't delete mtag in case cut off M_PKTHDR flag
			 */
			m_tag_delete_chain(m, NULL);
			m->m_flags &= ~M_PKTHDR;
			m_head->m_next = m;
		}

		/* override m */
		m = m_head;
	}

	m_set_rcvif(m, ifp);

	/*
	 * bpf_mtap() and ifp->if_ipackets++ is done in if_input()
	 *
	 * obytes is incremented at ether_output() or bridge_enqueue().
	 */
	if_percpuq_enqueue(ifp->if_percpuq, m);
}

void
l2tp_start(struct ifnet *ifp)
{
	struct psref psref;
	struct l2tp_variant *var;
	struct l2tp_softc *sc = container_of(ifp, struct l2tp_softc,
	    l2tp_ec.ec_if);

	var = l2tp_getref_variant(sc, &psref);
	if (var == NULL)
		return;

	if (var->lv_psrc == NULL || var->lv_pdst == NULL)
		return;

	l2tpintr(var);
	l2tp_putref_variant(var, &psref);
}

int
l2tp_transmit(struct ifnet *ifp, struct mbuf *m)
{
	int error;
	struct psref psref;
	struct l2tp_variant *var;
	struct l2tp_softc *sc = container_of(ifp, struct l2tp_softc,
	    l2tp_ec.ec_if);

	var = l2tp_getref_variant(sc, &psref);
	if (var == NULL) {
		m_freem(m);
		return ENETDOWN;
	}

	if (var->lv_psrc == NULL || var->lv_pdst == NULL) {
		m_freem(m);
		error = ENETDOWN;
		goto out;
	}

	m->m_flags &= ~(M_BCAST|M_MCAST);
	bpf_mtap(ifp, m);
	switch (var->lv_psrc->sa_family) {
#ifdef INET
	case AF_INET:
		error = in_l2tp_output(var, m);
		break;
#endif
#ifdef INET6
	case AF_INET6:
		error = in6_l2tp_output(var, m);
		break;
#endif
	default:
		m_freem(m);
		error = ENETDOWN;
		break;
	}

	if (error)
		ifp->if_oerrors++;
	else {
		ifp->if_opackets++;
		/*
		 * obytes is incremented at ether_output() or bridge_enqueue().
		 */
	}

out:
	l2tp_putref_variant(var, &psref);
	return error;
}

/* XXX how should we handle IPv6 scope on SIOC[GS]IFPHYADDR? */
int
l2tp_ioctl(struct ifnet *ifp, u_long cmd, void *data)
{
	struct l2tp_softc *sc = container_of(ifp, struct l2tp_softc,
	    l2tp_ec.ec_if);
	struct l2tp_variant *var, *var_tmp;
	struct ifreq     *ifr = data;
	int error = 0, size;
	struct sockaddr *dst, *src;
	struct l2tp_req l2tpr;
	u_long mtu;
	int bound;
	struct psref psref;

	switch (cmd) {
	case SIOCSIFADDR:
		ifp->if_flags |= IFF_UP;
		break;

	case SIOCSIFDSTADDR:
		break;

	case SIOCADDMULTI:
	case SIOCDELMULTI:
		switch (ifr->ifr_addr.sa_family) {
#ifdef INET
		case AF_INET:	/* IP supports Multicast */
			break;
#endif /* INET */
#ifdef INET6
		case AF_INET6:	/* IP6 supports Multicast */
			break;
#endif /* INET6 */
		default:  /* Other protocols doesn't support Multicast */
			error = EAFNOSUPPORT;
			break;
		}
		break;

	case SIOCSIFMTU:
		mtu = ifr->ifr_mtu;
		if (mtu < L2TP_MTU_MIN || mtu > L2TP_MTU_MAX)
			return (EINVAL);
		ifp->if_mtu = mtu;
		break;

#ifdef INET
	case SIOCSIFPHYADDR:
		src = (struct sockaddr *)
			&(((struct in_aliasreq *)data)->ifra_addr);
		dst = (struct sockaddr *)
			&(((struct in_aliasreq *)data)->ifra_dstaddr);
		if (src->sa_family != AF_INET || dst->sa_family != AF_INET)
			return EAFNOSUPPORT;
		else if (src->sa_len != sizeof(struct sockaddr_in)
		    || dst->sa_len != sizeof(struct sockaddr_in))
			return EINVAL;

		error = l2tp_set_tunnel(&sc->l2tp_ec.ec_if, src, dst);
		break;

#endif /* INET */
#ifdef INET6
	case SIOCSIFPHYADDR_IN6:
		src = (struct sockaddr *)
			&(((struct in6_aliasreq *)data)->ifra_addr);
		dst = (struct sockaddr *)
			&(((struct in6_aliasreq *)data)->ifra_dstaddr);
		if (src->sa_family != AF_INET6 || dst->sa_family != AF_INET6)
			return EAFNOSUPPORT;
		else if (src->sa_len != sizeof(struct sockaddr_in6)
		    || dst->sa_len != sizeof(struct sockaddr_in6))
			return EINVAL;

		error = l2tp_set_tunnel(&sc->l2tp_ec.ec_if, src, dst);
		break;

#endif /* INET6 */
	case SIOCSLIFPHYADDR:
		src = (struct sockaddr *)
			&(((struct if_laddrreq *)data)->addr);
		dst = (struct sockaddr *)
			&(((struct if_laddrreq *)data)->dstaddr);
		if (src->sa_family != dst->sa_family)
			return EINVAL;
		else if (src->sa_family == AF_INET
		    && src->sa_len != sizeof(struct sockaddr_in))
			return EINVAL;
		else if (src->sa_family == AF_INET6
		    && src->sa_len != sizeof(struct sockaddr_in6))
			return EINVAL;
		else if (dst->sa_family == AF_INET
		    && dst->sa_len != sizeof(struct sockaddr_in))
			return EINVAL;
		else if (dst->sa_family == AF_INET6
		    && dst->sa_len != sizeof(struct sockaddr_in6))
			return EINVAL;

		error = l2tp_set_tunnel(&sc->l2tp_ec.ec_if, src, dst);
		break;

	case SIOCDIFPHYADDR:
		l2tp_delete_tunnel(&sc->l2tp_ec.ec_if);
		break;

	case SIOCGIFPSRCADDR:
#ifdef INET6
	case SIOCGIFPSRCADDR_IN6:
#endif /* INET6 */
		bound = curlwp_bind();
		var = l2tp_getref_variant(sc, &psref);
		if (var == NULL) {
			curlwp_bindx(bound);
			error = EADDRNOTAVAIL;
			goto bad;
		}
		if (var->lv_psrc == NULL) {
			l2tp_putref_variant(var, &psref);
			curlwp_bindx(bound);
			error = EADDRNOTAVAIL;
			goto bad;
		}
		src = var->lv_psrc;
		switch (cmd) {
#ifdef INET
		case SIOCGIFPSRCADDR:
			dst = &ifr->ifr_addr;
			size = sizeof(ifr->ifr_addr);
			break;
#endif /* INET */
#ifdef INET6
		case SIOCGIFPSRCADDR_IN6:
			dst = (struct sockaddr *)
				&(((struct in6_ifreq *)data)->ifr_addr);
			size = sizeof(((struct in6_ifreq *)data)->ifr_addr);
			break;
#endif /* INET6 */
		default:
			l2tp_putref_variant(var, &psref);
			curlwp_bindx(bound);
			error = EADDRNOTAVAIL;
			goto bad;
		}
		if (src->sa_len > size) {
			l2tp_putref_variant(var, &psref);
			curlwp_bindx(bound);
			return EINVAL;
		}
		sockaddr_copy(dst, src->sa_len, src);
		l2tp_putref_variant(var, &psref);
		curlwp_bindx(bound);
		break;

	case SIOCGIFPDSTADDR:
#ifdef INET6
	case SIOCGIFPDSTADDR_IN6:
#endif /* INET6 */
		bound = curlwp_bind();
		var = l2tp_getref_variant(sc, &psref);
		if (var == NULL) {
			curlwp_bindx(bound);
			error = EADDRNOTAVAIL;
			goto bad;
		}
		if (var->lv_pdst == NULL) {
			l2tp_putref_variant(var, &psref);
			curlwp_bindx(bound);
			error = EADDRNOTAVAIL;
			goto bad;
		}
		src = var->lv_pdst;
		switch (cmd) {
#ifdef INET
		case SIOCGIFPDSTADDR:
			dst = &ifr->ifr_addr;
			size = sizeof(ifr->ifr_addr);
			break;
#endif /* INET */
#ifdef INET6
		case SIOCGIFPDSTADDR_IN6:
			dst = (struct sockaddr *)
				&(((struct in6_ifreq *)data)->ifr_addr);
			size = sizeof(((struct in6_ifreq *)data)->ifr_addr);
			break;
#endif /* INET6 */
		default:
			l2tp_putref_variant(var, &psref);
			curlwp_bindx(bound);
			error = EADDRNOTAVAIL;
			goto bad;
		}
		if (src->sa_len > size) {
			l2tp_putref_variant(var, &psref);
			curlwp_bindx(bound);
			return EINVAL;
		}
		sockaddr_copy(dst, src->sa_len, src);
		l2tp_putref_variant(var, &psref);
		curlwp_bindx(bound);
		break;

	case SIOCGLIFPHYADDR:
		bound = curlwp_bind();
		var = l2tp_getref_variant(sc, &psref);
		if (var == NULL) {
			curlwp_bindx(bound);
			error = EADDRNOTAVAIL;
			goto bad;
		}
		if (var->lv_psrc == NULL || var->lv_pdst == NULL) {
			l2tp_putref_variant(var, &psref);
			curlwp_bindx(bound);
			error = EADDRNOTAVAIL;
			goto bad;
		}

		/* copy src */
		src = var->lv_psrc;
		dst = (struct sockaddr *)
			&(((struct if_laddrreq *)data)->addr);
		size = sizeof(((struct if_laddrreq *)data)->addr);
		if (src->sa_len > size) {
			l2tp_putref_variant(var, &psref);
			curlwp_bindx(bound);
			return EINVAL;
                }
		sockaddr_copy(dst, src->sa_len, src);

		/* copy dst */
		src = var->lv_pdst;
		dst = (struct sockaddr *)
			&(((struct if_laddrreq *)data)->dstaddr);
		size = sizeof(((struct if_laddrreq *)data)->dstaddr);
		if (src->sa_len > size) {
			l2tp_putref_variant(var, &psref);
			curlwp_bindx(bound);
			return EINVAL;
                }
		sockaddr_copy(dst, src->sa_len, src);
		l2tp_putref_variant(var, &psref);
		curlwp_bindx(bound);
		break;

	case SIOCSL2TPSESSION:
		if ((error = copyin(ifr->ifr_data, &l2tpr, sizeof(l2tpr))) != 0)
			break;

		/* session id must not zero */
		if (l2tpr.my_sess_id == 0 || l2tpr.peer_sess_id == 0)
			return EINVAL;

		bound = curlwp_bind();
		var_tmp = l2tp_lookup_session_ref(l2tpr.my_sess_id, &psref);
		if (var_tmp != NULL) {
			/* duplicate session id */
			log(LOG_WARNING, "%s: duplicate session id %" PRIu32 " of %s\n",
				sc->l2tp_ec.ec_if.if_xname, l2tpr.my_sess_id,
				var_tmp->lv_softc->l2tp_ec.ec_if.if_xname);
			psref_release(&psref, &var_tmp->lv_psref,
			    lv_psref_class);
			curlwp_bindx(bound);
			return EINVAL;
		}
		curlwp_bindx(bound);

		error = l2tp_set_session(sc, l2tpr.my_sess_id, l2tpr.peer_sess_id);
		break;
	case SIOCDL2TPSESSION:
		l2tp_clear_session(sc);
		break;
	case SIOCSL2TPCOOKIE:
		if ((error = copyin(ifr->ifr_data, &l2tpr, sizeof(l2tpr))) != 0)
			break;

		error = l2tp_set_cookie(sc, l2tpr.my_cookie, l2tpr.my_cookie_len,
		    l2tpr.peer_cookie, l2tpr.peer_cookie_len);
		break;
	case SIOCDL2TPCOOKIE:
		l2tp_clear_cookie(sc);
		break;
	case SIOCSL2TPSTATE:
		if ((error = copyin(ifr->ifr_data, &l2tpr, sizeof(l2tpr))) != 0)
			break;

		l2tp_set_state(sc, l2tpr.state);
		break;
	case SIOCGL2TP:
		/* get L2TPV3 session info */
		memset(&l2tpr, 0, sizeof(l2tpr));

		bound = curlwp_bind();
		var = l2tp_getref_variant(sc, &psref);
		if (var == NULL) {
			curlwp_bindx(bound);
			error = EADDRNOTAVAIL;
			goto bad;
		}

		l2tpr.state = var->lv_state;
		l2tpr.my_sess_id = var->lv_my_sess_id;
		l2tpr.peer_sess_id = var->lv_peer_sess_id;
		l2tpr.my_cookie = var->lv_my_cookie;
		l2tpr.my_cookie_len = var->lv_my_cookie_len;
		l2tpr.peer_cookie = var->lv_peer_cookie;
		l2tpr.peer_cookie_len = var->lv_peer_cookie_len;
		l2tp_putref_variant(var, &psref);
		curlwp_bindx(bound);

		error = copyout(&l2tpr, ifr->ifr_data, sizeof(l2tpr));
		break;

	default:
		error =	ifioctl_common(ifp, cmd, data);
		break;
	}
 bad:
	return error;
}

static int
l2tp_set_tunnel(struct ifnet *ifp, struct sockaddr *src, struct sockaddr *dst)
{
	struct l2tp_softc *sc = container_of(ifp, struct l2tp_softc,
	    l2tp_ec.ec_if);
	struct sockaddr *osrc, *odst;
	struct sockaddr *nsrc, *ndst;
	struct l2tp_variant *ovar, *nvar;
	int error;

	nsrc = sockaddr_dup(src, M_WAITOK);
	ndst = sockaddr_dup(dst, M_WAITOK);

	nvar = kmem_alloc(sizeof(*nvar), KM_SLEEP);

	error = encap_lock_enter();
	if (error)
		goto error;

	mutex_enter(&sc->l2tp_lock);

	ovar = sc->l2tp_var;
	osrc = ovar->lv_psrc;
	odst = ovar->lv_pdst;
	*nvar = *ovar;
	psref_target_init(&nvar->lv_psref, lv_psref_class);
	nvar->lv_psrc = nsrc;
	nvar->lv_pdst = ndst;
	error = l2tp_encap_attach(nvar);
	if (error) {
		mutex_exit(&sc->l2tp_lock);
		encap_lock_exit();
		goto error;
	}
	membar_producer();
	l2tp_variant_update(sc, nvar);

	mutex_exit(&sc->l2tp_lock);

	(void)l2tp_encap_detach(ovar);
	encap_lock_exit();

	if (osrc)
		sockaddr_free(osrc);
	if (odst)
		sockaddr_free(odst);
	kmem_free(ovar, sizeof(*ovar));

	return 0;

error:
	sockaddr_free(nsrc);
	sockaddr_free(ndst);
	kmem_free(nvar, sizeof(*nvar));

	return error;
}

static void
l2tp_delete_tunnel(struct ifnet *ifp)
{
	struct l2tp_softc *sc = container_of(ifp, struct l2tp_softc,
	    l2tp_ec.ec_if);
	struct sockaddr *osrc, *odst;
	struct l2tp_variant *ovar, *nvar;
	int error;

	nvar = kmem_alloc(sizeof(*nvar), KM_SLEEP);

	error = encap_lock_enter();
	if (error) {
		kmem_free(nvar, sizeof(*nvar));
		return;
	}
	mutex_enter(&sc->l2tp_lock);

	ovar = sc->l2tp_var;
	osrc = ovar->lv_psrc;
	odst = ovar->lv_pdst;
	*nvar = *ovar;
	psref_target_init(&nvar->lv_psref, lv_psref_class);
	nvar->lv_psrc = NULL;
	nvar->lv_pdst = NULL;
	membar_producer();
	l2tp_variant_update(sc, nvar);

	mutex_exit(&sc->l2tp_lock);

	(void)l2tp_encap_detach(ovar);
	encap_lock_exit();

	if (osrc)
		sockaddr_free(osrc);
	if (odst)
		sockaddr_free(odst);
	kmem_free(ovar, sizeof(*ovar));
}

static int
id_hash_func(uint32_t id, u_long mask)
{
	uint32_t hash;

	hash = (id >> 16) ^ id;
	hash = (hash >> 4) ^ hash;

	return hash & mask;
}

static void
l2tp_hash_init(void)
{

	l2tp_hash.lists = hashinit(L2TP_ID_HASH_SIZE, HASH_PSLIST, true,
	    &l2tp_hash.mask);
}

static int
l2tp_hash_fini(void)
{
	int i;

	mutex_enter(&l2tp_hash.lock);

	for (i = 0; i < l2tp_hash.mask + 1; i++) {
		if (PSLIST_WRITER_FIRST(&l2tp_hash.lists[i], struct l2tp_softc,
			l2tp_hash) != NULL) {
			mutex_exit(&l2tp_hash.lock);
			return EBUSY;
		}
	}
	for (i = 0; i < l2tp_hash.mask + 1; i++)
		PSLIST_DESTROY(&l2tp_hash.lists[i]);

	mutex_exit(&l2tp_hash.lock);

	hashdone(l2tp_hash.lists, HASH_PSLIST, l2tp_hash.mask);

	return 0;
}

static int
l2tp_set_session(struct l2tp_softc *sc, uint32_t my_sess_id,
    uint32_t peer_sess_id)
{
	uint32_t idx;
	struct l2tp_variant *nvar;
	struct l2tp_variant *ovar;
	struct ifnet *ifp = &sc->l2tp_ec.ec_if;

	nvar = kmem_alloc(sizeof(*nvar), KM_SLEEP);

	mutex_enter(&sc->l2tp_lock);
	ovar = sc->l2tp_var;
	*nvar = *ovar;
	psref_target_init(&nvar->lv_psref, lv_psref_class);
	nvar->lv_my_sess_id = my_sess_id;
	nvar->lv_peer_sess_id = peer_sess_id;
	membar_producer();

	mutex_enter(&l2tp_hash.lock);
	if (ovar->lv_my_sess_id > 0 && ovar->lv_peer_sess_id > 0) {
		PSLIST_WRITER_REMOVE(sc, l2tp_hash);
		pserialize_perform(l2tp_psz);
	}
	mutex_exit(&l2tp_hash.lock);
	PSLIST_ENTRY_DESTROY(sc, l2tp_hash);

	l2tp_variant_update(sc, nvar);
	mutex_exit(&sc->l2tp_lock);

	idx = id_hash_func(nvar->lv_my_sess_id, l2tp_hash.mask);
	if ((ifp->if_flags & IFF_DEBUG) != 0)
		log(LOG_DEBUG, "%s: add hash entry: sess_id=%" PRIu32 ", idx=%" PRIu32 "\n",
		    sc->l2tp_ec.ec_if.if_xname, nvar->lv_my_sess_id, idx);

	PSLIST_ENTRY_INIT(sc, l2tp_hash);
	mutex_enter(&l2tp_hash.lock);
	PSLIST_WRITER_INSERT_HEAD(&l2tp_hash.lists[idx], sc, l2tp_hash);
	mutex_exit(&l2tp_hash.lock);

	kmem_free(ovar, sizeof(*ovar));
	return 0;
}

static int
l2tp_clear_session(struct l2tp_softc *sc)
{
	struct l2tp_variant *nvar;
	struct l2tp_variant *ovar;

	nvar = kmem_alloc(sizeof(*nvar), KM_SLEEP);

	mutex_enter(&sc->l2tp_lock);
	ovar = sc->l2tp_var;
	*nvar = *ovar;
	psref_target_init(&nvar->lv_psref, lv_psref_class);
	nvar->lv_my_sess_id = 0;
	nvar->lv_peer_sess_id = 0;
	membar_producer();

	mutex_enter(&l2tp_hash.lock);
	if (ovar->lv_my_sess_id > 0 && ovar->lv_peer_sess_id > 0) {
		PSLIST_WRITER_REMOVE(sc, l2tp_hash);
		pserialize_perform(l2tp_psz);
	}
	mutex_exit(&l2tp_hash.lock);

	l2tp_variant_update(sc, nvar);
	mutex_exit(&sc->l2tp_lock);
	kmem_free(ovar, sizeof(*ovar));
	return 0;
}

struct l2tp_variant *
l2tp_lookup_session_ref(uint32_t id, struct psref *psref)
{
	int idx;
	int s;
	struct l2tp_softc *sc;

	idx = id_hash_func(id, l2tp_hash.mask);

	s = pserialize_read_enter();
	PSLIST_READER_FOREACH(sc, &l2tp_hash.lists[idx], struct l2tp_softc,
	    l2tp_hash) {
		struct l2tp_variant *var = sc->l2tp_var;
		if (var == NULL)
			continue;
		if (var->lv_my_sess_id != id)
			continue;
		psref_acquire(psref, &var->lv_psref, lv_psref_class);
		pserialize_read_exit(s);
		return var;
	}
	pserialize_read_exit(s);
	return NULL;
}

/*
 * l2tp_variant update API.
 *
 * Assumption:
 * reader side dereferences sc->l2tp_var in reader critical section only,
 * that is, all of reader sides do not reader the sc->l2tp_var after
 * pserialize_perform().
 */
static void
l2tp_variant_update(struct l2tp_softc *sc, struct l2tp_variant *nvar)
{
	struct ifnet *ifp = &sc->l2tp_ec.ec_if;
	struct l2tp_variant *ovar = sc->l2tp_var;

	KASSERT(mutex_owned(&sc->l2tp_lock));

	sc->l2tp_var = nvar;
	pserialize_perform(l2tp_psz);
	psref_target_destroy(&ovar->lv_psref, lv_psref_class);

	/*
	 * In the manual of atomic_swap_ptr(3), there is no mention if 2nd
	 * argument is rewrite or not. So, use sc->l2tp_var instead of nvar.
	 */
	if (sc->l2tp_var != NULL) {
		if (sc->l2tp_var->lv_psrc != NULL
		    && sc->l2tp_var->lv_pdst != NULL)
			ifp->if_flags |= IFF_RUNNING;
		else
			ifp->if_flags &= ~IFF_RUNNING;
	}
}

static int
l2tp_set_cookie(struct l2tp_softc *sc, uint64_t my_cookie, u_int my_cookie_len,
    uint64_t peer_cookie, u_int peer_cookie_len)
{
	struct l2tp_variant *nvar;

	if (my_cookie == 0 || peer_cookie == 0)
		return EINVAL;

	if (my_cookie_len != 4 && my_cookie_len != 8
	    && peer_cookie_len != 4 && peer_cookie_len != 8)
		return EINVAL;

	nvar = kmem_alloc(sizeof(*nvar), KM_SLEEP);

	mutex_enter(&sc->l2tp_lock);

	*nvar = *sc->l2tp_var;
	psref_target_init(&nvar->lv_psref, lv_psref_class);
	nvar->lv_my_cookie = my_cookie;
	nvar->lv_my_cookie_len = my_cookie_len;
	nvar->lv_peer_cookie = peer_cookie;
	nvar->lv_peer_cookie_len = peer_cookie_len;
	nvar->lv_use_cookie = L2TP_COOKIE_ON;
	membar_producer();
	l2tp_variant_update(sc, nvar);

	mutex_exit(&sc->l2tp_lock);

	struct ifnet *ifp = &sc->l2tp_ec.ec_if;
	if ((ifp->if_flags & IFF_DEBUG) != 0) {
		log(LOG_DEBUG,
		    "%s: set cookie: "
		    "local cookie_len=%u local cookie=%" PRIu64 ", "
		    "remote cookie_len=%u remote cookie=%" PRIu64 "\n",
		    ifp->if_xname, my_cookie_len, my_cookie,
		    peer_cookie_len, peer_cookie);
	}

	return 0;
}

static void
l2tp_clear_cookie(struct l2tp_softc *sc)
{
	struct l2tp_variant *nvar;

	nvar = kmem_alloc(sizeof(*nvar), KM_SLEEP);

	mutex_enter(&sc->l2tp_lock);

	*nvar = *sc->l2tp_var;
	psref_target_init(&nvar->lv_psref, lv_psref_class);
	nvar->lv_my_cookie = 0;
	nvar->lv_my_cookie_len = 0;
	nvar->lv_peer_cookie = 0;
	nvar->lv_peer_cookie_len = 0;
	nvar->lv_use_cookie = L2TP_COOKIE_OFF;
	membar_producer();
	l2tp_variant_update(sc, nvar);

	mutex_exit(&sc->l2tp_lock);
}

static void
l2tp_set_state(struct l2tp_softc *sc, int state)
{
	struct ifnet *ifp = &sc->l2tp_ec.ec_if;
	struct l2tp_variant *nvar;

	nvar = kmem_alloc(sizeof(*nvar), KM_SLEEP);

	mutex_enter(&sc->l2tp_lock);

	*nvar = *sc->l2tp_var;
	psref_target_init(&nvar->lv_psref, lv_psref_class);
	nvar->lv_state = state;
	membar_producer();
	l2tp_variant_update(sc, nvar);

	if (nvar->lv_state == L2TP_STATE_UP) {
		ifp->if_link_state = LINK_STATE_UP;
	} else {
		ifp->if_link_state = LINK_STATE_DOWN;
	}

	mutex_exit(&sc->l2tp_lock);

#ifdef NOTYET
	vlan_linkstate_notify(ifp, ifp->if_link_state);
#endif
}

static int
l2tp_encap_attach(struct l2tp_variant *var)
{
	int error;

	if (var == NULL || var->lv_psrc == NULL)
		return EINVAL;

	switch (var->lv_psrc->sa_family) {
#ifdef INET
	case AF_INET:
		error = in_l2tp_attach(var);
		break;
#endif
#ifdef INET6
	case AF_INET6:
		error = in6_l2tp_attach(var);
		break;
#endif
	default:
		error = EINVAL;
		break;
	}

	return error;
}

static int
l2tp_encap_detach(struct l2tp_variant *var)
{
	int error;

	if (var == NULL || var->lv_psrc == NULL)
		return EINVAL;

	switch (var->lv_psrc->sa_family) {
#ifdef INET
	case AF_INET:
		error = in_l2tp_detach(var);
		break;
#endif
#ifdef INET6
	case AF_INET6:
		error = in6_l2tp_detach(var);
		break;
#endif
	default:
		error = EINVAL;
		break;
	}

	return error;
}

int
l2tp_check_nesting(struct ifnet *ifp, struct mbuf *m)
{

	return if_tunnel_check_nesting(ifp, m, max_l2tp_nesting);
}

/*
 * Module infrastructure
 */
#include "if_module.h"

IF_MODULE(MODULE_CLASS_DRIVER, l2tp, "")


/* TODO: IP_TCPMSS support */
#ifdef IP_TCPMSS
static int l2tp_need_tcpmss_clamp(struct ifnet *);
#ifdef INET
static struct mbuf *l2tp_tcpmss4_clamp(struct ifnet *, struct mbuf *);
#endif
#ifdef INET6
static struct mbuf *l2tp_tcpmss6_clamp(struct ifnet *, struct mbuf *);
#endif

struct mbuf *
l2tp_tcpmss_clamp(struct ifnet *ifp, struct mbuf *m)
{
	struct ether_header *eh;
	struct ether_vlan_header evh;

	if (!l2tp_need_tcpmss_clamp(ifp)) {
		return m;
	}

	if (m->m_pkthdr.len < sizeof(evh)) {
		m_freem(m);
		return NULL;
	}

	/* save ether header */
	m_copydata(m, 0, sizeof(evh), (void *)&evh);
	eh = (struct ether_header *)&evh;

	switch (ntohs(eh->ether_type)) {
	case ETHERTYPE_VLAN: /* Ether + VLAN */
		if (m->m_pkthdr.len <= sizeof(struct ether_vlan_header))
			break;
		m_adj(m, sizeof(struct ether_vlan_header));
		switch (ntohs(evh.evl_proto)) {
#ifdef INET
		case ETHERTYPE_IP: /* Ether + VLAN + IPv4 */
			m = l2tp_tcpmss4_clamp(ifp, m);
			if (m == NULL)
				return NULL;
			break;
#endif /* INET */
#ifdef INET6
		case ETHERTYPE_IPV6: /* Ether + VLAN + IPv6 */
			m = l2tp_tcpmss6_clamp(ifp, m);
			if (m == NULL)
				return NULL;
			break;
#endif /* INET6 */
		default:
			break;
		}

		/* restore ether header */
		M_PREPEND(m, sizeof(struct ether_vlan_header),
		    M_DONTWAIT);
		if (m == NULL)
			return NULL;
		*mtod(m, struct ether_vlan_header *) = evh;
		break;

#ifdef INET
	case ETHERTYPE_IP: /* Ether + IPv4 */
		if (m->m_pkthdr.len <= sizeof(struct ether_header))
			break;
		m_adj(m, sizeof(struct ether_header));
		m = l2tp_tcpmss4_clamp(ifp, m);
		if (m == NULL)
			return NULL;
		/* restore ether header */
		M_PREPEND(m, sizeof(struct ether_header), M_DONTWAIT);
		if (m == NULL)
			return NULL;
		*mtod(m, struct ether_header *) = *eh;
		break;
#endif /* INET */

#ifdef INET6
	case ETHERTYPE_IPV6: /* Ether + IPv6 */
		if (m->m_pkthdr.len <= sizeof(struct ether_header))
			break;
		m_adj(m, sizeof(struct ether_header));
		m = l2tp_tcpmss6_clamp(ifp, m);
		if (m == NULL)
			return NULL;
		/* restore ether header */
		M_PREPEND(m, sizeof(struct ether_header), M_DONTWAIT);
		if (m == NULL)
			return NULL;
		*mtod(m, struct ether_header *) = *eh;
		break;
#endif /* INET6 */

	default:
		break;
	}

	return m;
}

static int
l2tp_need_tcpmss_clamp(struct ifnet *ifp)
{
	int ret = 0;

#ifdef INET
	if (ifp->if_tcpmss != 0)
		ret = 1;
#endif

#ifdef INET6
	if (ifp->if_tcpmss6 != 0)
		ret = 1;
#endif

	return ret;
}

#ifdef INET
static struct mbuf *
l2tp_tcpmss4_clamp(struct ifnet *ifp, struct mbuf *m)
{

	if (ifp->if_tcpmss != 0) {
		return ip_tcpmss(m, (ifp->if_tcpmss < 0) ?
			ifp->if_mtu - IP_TCPMSS_EXTLEN :
			ifp->if_tcpmss);
	}
	return m;
}
#endif /* INET */

#ifdef INET6
static struct mbuf *
l2tp_tcpmss6_clamp(struct ifnet *ifp, struct mbuf *m)
{
	int ip6hdrlen;

	if (ifp->if_tcpmss6 != 0 &&
	    ip6_tcpmss_applicable(m, &ip6hdrlen)) {
		return ip6_tcpmss(m, ip6hdrlen,
			(ifp->if_tcpmss6 < 0) ?
			ifp->if_mtu - IP6_TCPMSS_EXTLEN :
			ifp->if_tcpmss6);
	}
	return m;
}
#endif /* INET6 */

#endif /* IP_TCPMSS */
