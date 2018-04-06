/*	$NetBSD: if_ipsec.c,v 1.11 2018/04/06 10:38:53 knakahara Exp $  */

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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: if_ipsec.c,v 1.11 2018/04/06 10:38:53 knakahara Exp $");

#ifdef _KERNEL_OPT
#include "opt_inet.h"
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
#include <sys/cpu.h>
#include <sys/kmem.h>
#include <sys/mutex.h>
#include <sys/pserialize.h>
#include <sys/psref.h>

#include <net/if.h>
#include <net/if_types.h>
#include <net/route.h>
#include <net/bpf.h>
#include <net/pfkeyv2.h>

#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#ifdef	INET
#include <netinet/in_var.h>
#endif	/* INET */

#ifdef INET6
#include <netinet6/in6_var.h>
#include <netinet/ip6.h>
#include <netinet6/ip6_var.h>
#endif /* INET6 */

#include <netinet/ip_encap.h>

#include <net/if_ipsec.h>

#include <net/raw_cb.h>
#include <net/pfkeyv2.h>

#include <netipsec/key.h>
#include <netipsec/keydb.h> /* for union sockaddr_union */
#include <netipsec/ipsec.h>
#include <netipsec/ipsecif.h>

static void if_ipsec_ro_init_pc(void *, void *, struct cpu_info *);
static void if_ipsec_ro_fini_pc(void *, void *, struct cpu_info *);

static int if_ipsec_clone_create(struct if_clone *, int);
static int if_ipsec_clone_destroy(struct ifnet *);

static inline int if_ipsec_out_direct(struct ipsec_variant *, struct mbuf *, int);
static inline void if_ipsec_in_enqueue(struct mbuf *, int, struct ifnet *);

static int if_ipsec_encap_attach(struct ipsec_variant *);
static int if_ipsec_encap_detach(struct ipsec_variant *);
static int if_ipsec_set_tunnel(struct ifnet *,
    struct sockaddr *, struct sockaddr *);
static void if_ipsec_delete_tunnel(struct ifnet *);
static int if_ipsec_ensure_flags(struct ifnet *, short);
static void if_ipsec_attach0(struct ipsec_softc *);

static int if_ipsec_update_variant(struct ipsec_softc *,
    struct ipsec_variant *, struct ipsec_variant *);

/* sadb_msg */
static inline void if_ipsec_add_mbuf(struct mbuf *, void *, size_t);
static inline void if_ipsec_add_pad(struct mbuf *, size_t);
static inline size_t if_ipsec_set_sadb_addr(struct sadb_address *,
    struct sockaddr *, int, uint16_t);
static inline size_t if_ipsec_set_sadb_src(struct sadb_address *,
    struct sockaddr *, int);
static inline size_t if_ipsec_set_sadb_dst(struct sadb_address *,
    struct sockaddr *, int);
static inline size_t if_ipsec_set_sadb_x_policy(struct sadb_x_policy *,
    struct sadb_x_ipsecrequest *, uint16_t, uint8_t, uint32_t, uint8_t,
    struct sockaddr *, struct sockaddr *);
static inline void if_ipsec_set_sadb_msg(struct sadb_msg *, uint16_t, uint8_t);
static inline void if_ipsec_set_sadb_msg_add(struct sadb_msg *, uint16_t);
static inline void if_ipsec_set_sadb_msg_del(struct sadb_msg *, uint16_t);
/* SPD */
static int if_ipsec_share_sp(struct ipsec_variant *);
static int if_ipsec_unshare_sp(struct ipsec_variant *);
static inline struct secpolicy *if_ipsec_add_sp0(struct sockaddr *,
    in_port_t, struct sockaddr *, in_port_t, int, int, int, u_int);
static inline int if_ipsec_del_sp0(struct secpolicy *);
static int if_ipsec_add_sp(struct ipsec_variant *,
    struct sockaddr *, in_port_t, struct sockaddr *, in_port_t);
static void if_ipsec_del_sp(struct ipsec_variant *);
static int if_ipsec_replace_sp(struct ipsec_softc *, struct ipsec_variant *,
    struct ipsec_variant *);

static int if_ipsec_set_addr_port(struct sockaddr *, struct sockaddr *,
    in_port_t);
#define IF_IPSEC_GATHER_PSRC_ADDR_PORT(var, target)			\
	if_ipsec_set_addr_port(target, (var)->iv_psrc, (var)->iv_sport)
#define IF_IPSEC_GATHER_PDST_ADDR_PORT(var, target)			\
	if_ipsec_set_addr_port(target, (var)->iv_pdst, (var)->iv_dport)

/*
 * ipsec global variable definitions
 */

/* This list is used in ioctl context only. */
LIST_HEAD(ipsec_sclist, ipsec_softc);
static struct {
	struct ipsec_sclist list;
	kmutex_t lock;
} ipsec_softcs __cacheline_aligned;

pserialize_t ipsec_psz __read_mostly;
struct psref_class *iv_psref_class __read_mostly;

struct if_clone ipsec_cloner =
    IF_CLONE_INITIALIZER("ipsec", if_ipsec_clone_create, if_ipsec_clone_destroy);
static int max_ipsec_nesting = MAX_IPSEC_NEST;

/* ARGSUSED */
void
ipsecifattach(int count)
{

	mutex_init(&ipsec_softcs.lock, MUTEX_DEFAULT, IPL_NONE);
	LIST_INIT(&ipsec_softcs.list);

	ipsec_psz = pserialize_create();
	iv_psref_class = psref_class_create("ipsecvar", IPL_SOFTNET);

	if_clone_attach(&ipsec_cloner);
}

static int
if_ipsec_clone_create(struct if_clone *ifc, int unit)
{
	struct ipsec_softc *sc;
	struct ipsec_variant *var;

	sc = kmem_zalloc(sizeof(*sc), KM_SLEEP);

	if_initname(&sc->ipsec_if, ifc->ifc_name, unit);

	if_ipsec_attach0(sc);

	var = kmem_zalloc(sizeof(*var), KM_SLEEP);
	var->iv_softc = sc;
	psref_target_init(&var->iv_psref, iv_psref_class);

	sc->ipsec_var = var;
	mutex_init(&sc->ipsec_lock, MUTEX_DEFAULT, IPL_NONE);
	sc->ipsec_ro_percpu = percpu_alloc(sizeof(struct ipsec_ro));
	percpu_foreach(sc->ipsec_ro_percpu, if_ipsec_ro_init_pc, NULL);

	mutex_enter(&ipsec_softcs.lock);
	LIST_INSERT_HEAD(&ipsec_softcs.list, sc, ipsec_list);
	mutex_exit(&ipsec_softcs.lock);
	return 0;
}

static void
if_ipsec_attach0(struct ipsec_softc *sc)
{

	sc->ipsec_if.if_addrlen = 0;
	sc->ipsec_if.if_mtu    = IPSEC_MTU;
	sc->ipsec_if.if_flags  = IFF_POINTOPOINT | IFF_MULTICAST;
	/* set ipsec(4) specific default flags. */
	sc->ipsec_if.if_flags  |= IFF_FWD_IPV6;
	sc->ipsec_if.if_extflags = IFEF_NO_LINK_STATE_CHANGE | IFEF_MPSAFE;
	sc->ipsec_if.if_ioctl  = if_ipsec_ioctl;
	sc->ipsec_if.if_output = if_ipsec_output;
	sc->ipsec_if.if_type   = IFT_IPSEC;
	sc->ipsec_if.if_dlt    = DLT_NULL;
	sc->ipsec_if.if_softc  = sc;
	IFQ_SET_READY(&sc->ipsec_if.if_snd);
	if_initialize(&sc->ipsec_if);
	if_alloc_sadl(&sc->ipsec_if);
	bpf_attach(&sc->ipsec_if, DLT_NULL, sizeof(u_int));
	if_register(&sc->ipsec_if);
}

static void
if_ipsec_ro_init_pc(void *p, void *arg __unused, struct cpu_info *ci __unused)
{
	struct ipsec_ro *iro = p;

	mutex_init(&iro->ir_lock, MUTEX_DEFAULT, IPL_NONE);
}

static void
if_ipsec_ro_fini_pc(void *p, void *arg __unused, struct cpu_info *ci __unused)
{
	struct ipsec_ro *iro = p;

	rtcache_free(&iro->ir_ro);

	mutex_destroy(&iro->ir_lock);
}

static int
if_ipsec_clone_destroy(struct ifnet *ifp)
{
	struct ipsec_softc *sc = ifp->if_softc;
	struct ipsec_variant *var;
	int bound;

	mutex_enter(&ipsec_softcs.lock);
	LIST_REMOVE(sc, ipsec_list);
	mutex_exit(&ipsec_softcs.lock);

	bound = curlwp_bind();
	if_ipsec_delete_tunnel(&sc->ipsec_if);
	curlwp_bindx(bound);

	bpf_detach(ifp);
	if_detach(ifp);

	percpu_foreach(sc->ipsec_ro_percpu, if_ipsec_ro_fini_pc, NULL);
	percpu_free(sc->ipsec_ro_percpu, sizeof(struct ipsec_ro));

	mutex_destroy(&sc->ipsec_lock);

	var = sc->ipsec_var;
	kmem_free(var, sizeof(*var));
	kmem_free(sc, sizeof(*sc));

	return 0;
}

static inline bool
if_ipsec_nat_t(struct ipsec_softc *sc)
{

	return (sc->ipsec_if.if_flags & IFF_NAT_T) != 0;
}

static inline bool
if_ipsec_fwd_ipv6(struct ipsec_softc *sc)
{

	return (sc->ipsec_if.if_flags & IFF_FWD_IPV6) != 0;
}

int
if_ipsec_encap_func(struct mbuf *m, int off, int proto, void *arg)
{
	uint8_t v;
	struct ipsec_softc *sc;
	struct ipsec_variant *var = NULL;
	struct psref psref;
	int ret = 0;

	sc = arg;
	KASSERT(sc != NULL);

	if ((sc->ipsec_if.if_flags & IFF_UP) == 0)
		goto out;

	var = if_ipsec_getref_variant(sc, &psref);
	if (if_ipsec_variant_is_unconfigured(var))
		goto out;

	switch (proto) {
	case IPPROTO_IPV4:
	case IPPROTO_IPV6:
		break;
	default:
		goto out;
	}

	m_copydata(m, 0, sizeof(v), &v);
	v = (v >> 4) & 0xff;  /* Get the IP version number. */

	switch (v) {
#ifdef INET
	case IPVERSION: {
		struct ip ip;

		if (m->m_pkthdr.len < sizeof(ip))
			goto out;

		m_copydata(m, 0, sizeof(ip), &ip);
		if (var->iv_psrc->sa_family != AF_INET ||
		    var->iv_pdst->sa_family != AF_INET)
			goto out;
		ret = ipsecif4_encap_func(m, &ip, var);
		break;
	}
#endif
#ifdef INET6
	case (IPV6_VERSION >> 4): {
		struct ip6_hdr ip6;

		if (m->m_pkthdr.len < sizeof(ip6))
			goto out;

		m_copydata(m, 0, sizeof(ip6), &ip6);
		if (var->iv_psrc->sa_family != AF_INET6 ||
		    var->iv_pdst->sa_family != AF_INET6)
			goto out;
		ret = ipsecif6_encap_func(m, &ip6, var);
		break;
	}
#endif
	default:
		goto out;
	}

out:
	if (var != NULL)
		if_ipsec_putref_variant(var, &psref);
	return ret;
}

/*
 * ipsec(4) I/F may cause infinite recursion calls when misconfigured.
 * We'll prevent this by introducing upper limit.
 */
static int
if_ipsec_check_nesting(struct ifnet *ifp, struct mbuf *m)
{

	return if_tunnel_check_nesting(ifp, m, max_ipsec_nesting);
}

int
if_ipsec_output(struct ifnet *ifp, struct mbuf *m, const struct sockaddr *dst,
    const struct rtentry *rt)
{
	struct ipsec_softc *sc = ifp->if_softc;
	struct ipsec_variant *var;
	struct psref psref;
	int error;
	int bound;

	IFQ_CLASSIFY(&ifp->if_snd, m, dst->sa_family);

	error = if_ipsec_check_nesting(ifp, m);
	if (error) {
		m_freem(m);
		goto noref_end;
	}

	if ((ifp->if_flags & IFF_UP) == 0) {
		m_freem(m);
		error = ENETDOWN;
		goto noref_end;
	}


	bound = curlwp_bind();
	var = if_ipsec_getref_variant(sc, &psref);
	if (if_ipsec_variant_is_unconfigured(var)) {
		m_freem(m);
		error = ENETDOWN;
		goto end;
	}

	m->m_flags &= ~(M_BCAST|M_MCAST);

	/* use DLT_NULL encapsulation here to pass inner af type */
	M_PREPEND(m, sizeof(int), M_DONTWAIT);
	if (!m) {
		error = ENOBUFS;
		goto end;
	}
	*mtod(m, int *) = dst->sa_family;

#if INET6
	/* drop IPv6 packet if IFF_FWD_IPV6 is not set */
	if (dst->sa_family == AF_INET6 &&
	    !if_ipsec_fwd_ipv6(sc)) {
		/*
		 * IPv6 packet is not allowed to forward,that is not error.
		 */
		error = 0;
		IF_DROP(&ifp->if_snd);
		m_freem(m);
		goto end;
	}
#endif

	error = if_ipsec_out_direct(var, m, dst->sa_family);

end:
	if_ipsec_putref_variant(var, &psref);
	curlwp_bindx(bound);
noref_end:
	if (error)
		ifp->if_oerrors++;

	return error;
}

static inline int
if_ipsec_out_direct(struct ipsec_variant *var, struct mbuf *m, int family)
{
	struct ifnet *ifp = &var->iv_softc->ipsec_if;
	int error;
	int len;

	KASSERT(if_ipsec_heldref_variant(var));
	KASSERT(var->iv_output != NULL);

	len = m->m_pkthdr.len;

	/* input DLT_NULL frame to BPF */
	bpf_mtap(ifp, m);

	/* grab and chop off inner af type */
	/* XXX need pullup? */
	m_adj(m, sizeof(int));

	error = var->iv_output(var, family, m);
	if (error)
		return error;

	ifp->if_opackets++;
	ifp->if_obytes += len;

	return 0;
}

void
if_ipsec_input(struct mbuf *m, int af, struct ifnet *ifp)
{

	KASSERT(ifp != NULL);

	m_set_rcvif(m, ifp);

	bpf_mtap_af(ifp, af, m);

	if_ipsec_in_enqueue(m, af, ifp);

	return;
}

static inline void
if_ipsec_in_enqueue(struct mbuf *m, int af, struct ifnet *ifp)
{
	pktqueue_t *pktq;
	int pktlen;

	/*
	 * Put the packet to the network layer input queue according to the
	 * specified address family.
	 */
	switch (af) {
#ifdef INET
	case AF_INET:
		pktq = ip_pktq;
		break;
#endif
#ifdef INET6
	case AF_INET6:
		pktq = ip6_pktq;
		break;
#endif
	default:
		ifp->if_ierrors++;
		m_freem(m);
		return;
	}

#if 1
	const u_int h = curcpu()->ci_index;
#else
	const uint32_t h = pktq_rps_hash(m);
#endif
	pktlen = m->m_pkthdr.len;
	if (__predict_true(pktq_enqueue(pktq, m, h))) {
		ifp->if_ibytes += pktlen;
		ifp->if_ipackets++;
	} else {
		m_freem(m);
	}

	return;
}

static inline int
if_ipsec_check_salen(struct sockaddr *addr)
{

	switch (addr->sa_family) {
#ifdef INET
	case AF_INET:
		if (addr->sa_len != sizeof(struct sockaddr_in))
			return EINVAL;
		break;
#endif /* INET */
#ifdef INET6
	case AF_INET6:
		if (addr->sa_len != sizeof(struct sockaddr_in6))
			return EINVAL;
		break;
#endif /* INET6 */
	default:
		return EAFNOSUPPORT;
	}

	return 0;
}

/* XXX how should we handle IPv6 scope on SIOC[GS]IFPHYADDR? */
int
if_ipsec_ioctl(struct ifnet *ifp, u_long cmd, void *data)
{
	struct ipsec_softc *sc  = ifp->if_softc;
	struct ipsec_variant *var = NULL;
	struct ifreq     *ifr = (struct ifreq*)data;
	struct ifaddr    *ifa = (struct ifaddr*)data;
	int error = 0, size;
	struct sockaddr *dst, *src;
	u_long mtu;
	short oflags = ifp->if_flags;
	int bound;
	struct psref psref;

	switch (cmd) {
	case SIOCINITIFADDR:
		ifp->if_flags |= IFF_UP;
		ifa->ifa_rtrequest = p2p_rtrequest;
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
		if (mtu < IPSEC_MTU_MIN || mtu > IPSEC_MTU_MAX)
			return EINVAL;
		else if ((error = ifioctl_common(ifp, cmd, data)) == ENETRESET)
			error = 0;
		break;

#ifdef INET
	case SIOCSIFPHYADDR:
#endif
#ifdef INET6
	case SIOCSIFPHYADDR_IN6:
#endif /* INET6 */
	case SIOCSLIFPHYADDR:
		switch (cmd) {
#ifdef INET
		case SIOCSIFPHYADDR:
			src = (struct sockaddr *)
				&(((struct in_aliasreq *)data)->ifra_addr);
			dst = (struct sockaddr *)
				&(((struct in_aliasreq *)data)->ifra_dstaddr);
			break;
#endif /* INET */
#ifdef INET6
		case SIOCSIFPHYADDR_IN6:
			src = (struct sockaddr *)
				&(((struct in6_aliasreq *)data)->ifra_addr);
			dst = (struct sockaddr *)
				&(((struct in6_aliasreq *)data)->ifra_dstaddr);
			break;
#endif /* INET6 */
		case SIOCSLIFPHYADDR:
			src = (struct sockaddr *)
				&(((struct if_laddrreq *)data)->addr);
			dst = (struct sockaddr *)
				&(((struct if_laddrreq *)data)->dstaddr);
			break;
		default:
			return EINVAL;
		}

		/* sa_family must be equal */
		if (src->sa_family != dst->sa_family)
			return EINVAL;

		error = if_ipsec_check_salen(src);
		if (error)
			return error;
		error = if_ipsec_check_salen(dst);
		if (error)
			return error;

		/* check sa_family looks sane for the cmd */
		switch (cmd) {
#ifdef INET
		case SIOCSIFPHYADDR:
			if (src->sa_family == AF_INET)
				break;
			return EAFNOSUPPORT;
#endif /* INET */
#ifdef INET6
		case SIOCSIFPHYADDR_IN6:
			if (src->sa_family == AF_INET6)
				break;
			return EAFNOSUPPORT;
#endif /* INET6 */
		case SIOCSLIFPHYADDR:
			/* checks done in the above */
			break;
		}
		/*
		 * calls if_ipsec_getref_variant() for other softcs to check
		 * address pair duplicattion
		 */
		bound = curlwp_bind();
		error = if_ipsec_set_tunnel(&sc->ipsec_if, src, dst);
		if (error)
			goto bad;
		curlwp_bindx(bound);
		break;

	case SIOCDIFPHYADDR:
		bound = curlwp_bind();
		if_ipsec_delete_tunnel(&sc->ipsec_if);
		curlwp_bindx(bound);
		break;

	case SIOCGIFPSRCADDR:
#ifdef INET6
	case SIOCGIFPSRCADDR_IN6:
#endif /* INET6 */
		bound = curlwp_bind();
		var = if_ipsec_getref_variant(sc, &psref);
		if (var->iv_psrc == NULL) {
			error = EADDRNOTAVAIL;
			goto bad;
		}
		src = var->iv_psrc;
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
			error = EADDRNOTAVAIL;
			goto bad;
		}
		if (src->sa_len > size) {
			error = EINVAL;
			goto bad;
		}
		error = IF_IPSEC_GATHER_PSRC_ADDR_PORT(var, dst);
		if (error)
			goto bad;
		if_ipsec_putref_variant(var, &psref);
		curlwp_bindx(bound);
		break;

	case SIOCGIFPDSTADDR:
#ifdef INET6
	case SIOCGIFPDSTADDR_IN6:
#endif /* INET6 */
		bound = curlwp_bind();
		var = if_ipsec_getref_variant(sc, &psref);
		if (var->iv_pdst == NULL) {
			error = EADDRNOTAVAIL;
			goto bad;
		}
		src = var->iv_pdst;
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
			error = EADDRNOTAVAIL;
			goto bad;
		}
		if (src->sa_len > size) {
			error = EINVAL;
			goto bad;
		}
		error = IF_IPSEC_GATHER_PDST_ADDR_PORT(var, dst);
		if (error)
			goto bad;
		if_ipsec_putref_variant(var, &psref);
		curlwp_bindx(bound);
		break;

	case SIOCGLIFPHYADDR:
		bound = curlwp_bind();
		var = if_ipsec_getref_variant(sc, &psref);
		if (if_ipsec_variant_is_unconfigured(var)) {
			error = EADDRNOTAVAIL;
			goto bad;
		}

		/* copy src */
		src = var->iv_psrc;
		dst = (struct sockaddr *)
			&(((struct if_laddrreq *)data)->addr);
		size = sizeof(((struct if_laddrreq *)data)->addr);
		if (src->sa_len > size) {
			error = EINVAL;
			goto bad;
		}
		error = IF_IPSEC_GATHER_PSRC_ADDR_PORT(var, dst);
		if (error)
			goto bad;

		/* copy dst */
		src = var->iv_pdst;
		dst = (struct sockaddr *)
			&(((struct if_laddrreq *)data)->dstaddr);
		size = sizeof(((struct if_laddrreq *)data)->dstaddr);
		if (src->sa_len > size) {
			error = EINVAL;
			goto bad;
		}
		error = IF_IPSEC_GATHER_PDST_ADDR_PORT(var, dst);
		if (error)
			goto bad;
		if_ipsec_putref_variant(var, &psref);
		curlwp_bindx(bound);
		break;

	default:
		error = ifioctl_common(ifp, cmd, data);
		if (!error) {
			bound = curlwp_bind();
			error = if_ipsec_ensure_flags(&sc->ipsec_if, oflags);
			if (error)
				goto bad;
			curlwp_bindx(bound);
		}
		break;
	}
	return error;

bad:
	if (var != NULL)
		if_ipsec_putref_variant(var, &psref);
	curlwp_bindx(bound);

	return error;
}

struct encap_funcs {
#ifdef INET
	int (*ef_inet)(struct ipsec_variant *);
#endif
#ifdef INET6
	int (*ef_inet6)(struct ipsec_variant *);
#endif
};

static struct encap_funcs ipsec_encap_attach = {
#ifdef INET
	.ef_inet = ipsecif4_attach,
#endif
#ifdef INET6
	.ef_inet6 = &ipsecif6_attach,
#endif
};

static struct encap_funcs ipsec_encap_detach = {
#ifdef INET
	.ef_inet = ipsecif4_detach,
#endif
#ifdef INET6
	.ef_inet6 = &ipsecif6_detach,
#endif
};

static int
if_ipsec_encap_common(struct ipsec_variant *var, struct encap_funcs *funcs)
{
	int error;

	KASSERT(var != NULL);
	KASSERT(if_ipsec_variant_is_configured(var));

	switch (var->iv_psrc->sa_family) {
#ifdef INET
	case AF_INET:
		error = (funcs->ef_inet)(var);
		break;
#endif /* INET */
#ifdef INET6
	case AF_INET6:
		error = (funcs->ef_inet6)(var);
		break;
#endif /* INET6 */
	default:
		error = EINVAL;
		break;
	}

	return error;
}

static int
if_ipsec_encap_attach(struct ipsec_variant *var)
{

	return if_ipsec_encap_common(var, &ipsec_encap_attach);
}

static int
if_ipsec_encap_detach(struct ipsec_variant *var)
{

	return if_ipsec_encap_common(var, &ipsec_encap_detach);
}

/*
 * Validate and set ipsec(4) I/F configurations.
 *     (1) validate
 *         (1-1) Check the argument src and dst address pair will change
 *               configuration from current src and dst address pair.
 *         (1-2) Check any ipsec(4) I/F uses duplicated src and dst address pair
 *               with argument src and dst address pair, except for NAT-T shared
 *               tunnels.
 *     (2) set
 *         (2-1) Create variant for new configuration.
 *         (2-2) Create temporary "null" variant used to avoid to access
 *               dangling variant while SPs are deleted and added.
 *         (2-3) Swap variant include its SPs.
 *         (2-4) Cleanup last configurations.
 */
static int
if_ipsec_set_tunnel(struct ifnet *ifp,
    struct sockaddr *src, struct sockaddr *dst)
{
	struct ipsec_softc *sc = ifp->if_softc;
	struct ipsec_softc *sc2;
	struct ipsec_variant *ovar, *nvar, *nullvar;
	struct sockaddr *osrc, *odst;
	struct sockaddr *nsrc, *ndst;
	in_port_t nsport = 0, ndport = 0;
	int error;

	error = encap_lock_enter();
	if (error)
		return error;

	nsrc = sockaddr_dup(src, M_WAITOK);
	ndst = sockaddr_dup(dst, M_WAITOK);
	nvar = kmem_zalloc(sizeof(*nvar), KM_SLEEP);
	nullvar = kmem_zalloc(sizeof(*nullvar), KM_SLEEP);

	mutex_enter(&sc->ipsec_lock);

	ovar = sc->ipsec_var;

	switch(nsrc->sa_family) {
#ifdef INET
	case AF_INET:
		nsport = satosin(src)->sin_port;
		/*
		 * avoid confuse SP when NAT-T disabled,
		 * e.g.
		 *     expected: 10.0.1.2[any] 10.0.1.1[any] 4(ipv4)
		 *     confuse : 10.0.1.2[600] 10.0.1.1[600] 4(ipv4)
		 */
		satosin(nsrc)->sin_port = 0;
		ndport = satosin(dst)->sin_port;
		satosin(ndst)->sin_port = 0;
		break;
#endif /* INET */
#ifdef INET6
	case AF_INET6:
		nsport = satosin6(src)->sin6_port;
		satosin6(nsrc)->sin6_port = 0;
		ndport = satosin6(dst)->sin6_port;
		satosin6(ndst)->sin6_port = 0;
		break;
#endif /* INET6 */
	default:
		log(LOG_DEBUG,
		    "%s: Invalid address family: %d.\n",
		    __func__, src->sa_family);
		error = EINVAL;
		goto out;
	}

	/*
	 * (1-1) Check the argument src and dst address pair will change
	 *       configuration from current src and dst address pair.
	 */
	if ((ovar->iv_pdst && sockaddr_cmp(ovar->iv_pdst, dst) == 0) &&
	    (ovar->iv_psrc && sockaddr_cmp(ovar->iv_psrc, src) == 0) &&
	    (ovar->iv_sport == nsport && ovar->iv_dport == ndport)) {
		/* address and port pair not changed. */
		error = 0;
		goto out;
	}

	/*
	 * (1-2) Check any ipsec(4) I/F uses duplicated src and dst address pair
	 *       with argument src and dst address pair, except for NAT-T shared
	 *       tunnels.
	 */
	mutex_enter(&ipsec_softcs.lock);
	LIST_FOREACH(sc2, &ipsec_softcs.list, ipsec_list) {
		struct ipsec_variant *var2;
		struct psref psref;

		if (sc2 == sc)
			continue;
		var2 = if_ipsec_getref_variant(sc2, &psref);
		if (if_ipsec_variant_is_unconfigured(var2)) {
			if_ipsec_putref_variant(var2, &psref);
			continue;
		}
		if (if_ipsec_nat_t(sc) || if_ipsec_nat_t(sc2)) {
			if_ipsec_putref_variant(var2, &psref);
			continue; /* NAT-T shared tunnel */
		}
		if (sockaddr_cmp(var2->iv_pdst, dst) == 0 &&
		    sockaddr_cmp(var2->iv_psrc, src) == 0) {
			if_ipsec_putref_variant(var2, &psref);
			mutex_exit(&ipsec_softcs.lock);
			error = EADDRNOTAVAIL;
			goto out;
		}

		if_ipsec_putref_variant(var2, &psref);
		/* XXX both end must be valid? (I mean, not 0.0.0.0) */
	}
	mutex_exit(&ipsec_softcs.lock);


	osrc = ovar->iv_psrc;
	odst = ovar->iv_pdst;

	/*
	 * (2-1) Create ipsec_variant for new configuration.
	 */
	if_ipsec_copy_variant(nvar, ovar);
	nvar->iv_psrc = nsrc;
	nvar->iv_pdst = ndst;
	nvar->iv_sport = nsport;
	nvar->iv_dport = ndport;
	nvar->iv_encap_cookie4 = NULL;
	nvar->iv_encap_cookie6 = NULL;
	psref_target_init(&nvar->iv_psref, iv_psref_class);
	error = if_ipsec_encap_attach(nvar);
	if (error)
		goto out;

	/*
	 * (2-2) Create temporary "null" variant.
	 */
	if_ipsec_copy_variant(nullvar, ovar);
	if_ipsec_clear_config(nullvar);
	psref_target_init(&nullvar->iv_psref, iv_psref_class);
	membar_producer();
	/*
	 * (2-3) Swap variant include its SPs.
	 */
	error = if_ipsec_update_variant(sc, nvar, nullvar);
	if (error) {
		if_ipsec_encap_detach(nvar);
		goto out;
	}

	mutex_exit(&sc->ipsec_lock);

	/*
	 * (2-4) Cleanup last configurations.
	 */
	if (if_ipsec_variant_is_configured(ovar))
		if_ipsec_encap_detach(ovar);
	encap_lock_exit();

	if (osrc != NULL)
		sockaddr_free(osrc);
	if (odst != NULL)
		sockaddr_free(odst);
	kmem_free(ovar, sizeof(*ovar));
	kmem_free(nullvar, sizeof(*nullvar));

	return 0;

out:
	mutex_exit(&sc->ipsec_lock);
	encap_lock_exit();

	sockaddr_free(nsrc);
	sockaddr_free(ndst);
	kmem_free(nvar, sizeof(*nvar));
	kmem_free(nullvar, sizeof(*nullvar));

	return error;
}

/*
 * Validate and delete ipsec(4) I/F configurations.
 *     (1) validate
 *         (1-1) Check current src and dst address pair are null,
 *               which means the ipsec(4) I/F is already done deletetunnel.
 *     (2) delete
 *         (2-1) Create variant for deleted status.
 *         (2-2) Create temporary "null" variant used to avoid to access
 *               dangling variant while SPs are deleted and added.
 *               NOTE:
 *               The contents of temporary "null" variant equal to the variant
 *               of (2-1), however two psref_target_destroy() synchronization
 *               points are necessary to avoid to access dangling variant
 *               while SPs are deleted and added. To implement that simply,
 *               we use the same manner as if_ipsec_set_tunnel(), that is,
 *               create extra "null" variant and use it temporarily.
 *         (2-3) Swap variant include its SPs.
 *         (2-4) Cleanup last configurations.
 */
static void
if_ipsec_delete_tunnel(struct ifnet *ifp)
{
	struct ipsec_softc *sc = ifp->if_softc;
	struct ipsec_variant *ovar, *nvar, *nullvar;
	struct sockaddr *osrc, *odst;
	int error;

	error = encap_lock_enter();
	if (error)
		return;

	nvar = kmem_zalloc(sizeof(*nvar), KM_SLEEP);
	nullvar = kmem_zalloc(sizeof(*nullvar), KM_SLEEP);

	mutex_enter(&sc->ipsec_lock);

	ovar = sc->ipsec_var;
	osrc = ovar->iv_psrc;
	odst = ovar->iv_pdst;
	/*
	 * (1-1) Check current src and dst address pair are null,
	 *       which means the ipsec(4) I/F is already done deletetunnel.
	 */
	if (osrc == NULL || odst == NULL) {
		/* address pair not changed. */
		mutex_exit(&sc->ipsec_lock);
		encap_lock_exit();
		kmem_free(nvar, sizeof(*nvar));
		return;
	}

	/*
	 * (2-1) Create variant for deleted status.
	 */
	if_ipsec_copy_variant(nvar, ovar);
	if_ipsec_clear_config(nvar);
	psref_target_init(&nvar->iv_psref, iv_psref_class);

	/*
	 * (2-2) Create temporary "null" variant used to avoid to access
	 *       dangling variant while SPs are deleted and added.
	 */
	if_ipsec_copy_variant(nullvar, ovar);
	if_ipsec_clear_config(nullvar);
	psref_target_init(&nullvar->iv_psref, iv_psref_class);
	membar_producer();
	/*
	 * (2-3) Swap variant include its SPs.
	 */
	/* if_ipsec_update_variant() does not fail when delete SP only. */
	(void)if_ipsec_update_variant(sc, nvar, nullvar);

	mutex_exit(&sc->ipsec_lock);

	/*
	 * (2-4) Cleanup last configurations.
	 */
	if (if_ipsec_variant_is_configured(ovar))
		if_ipsec_encap_detach(ovar);
	encap_lock_exit();

	sockaddr_free(osrc);
	sockaddr_free(odst);
	kmem_free(ovar, sizeof(*ovar));
	kmem_free(nullvar, sizeof(*nullvar));
}

/*
 * Check IFF_NAT_T and IFF_FWD_IPV6 flags, therefore update SPs if needed.
 *     (1) check
 *         (1-1) Check flags are changed.
 *         (1-2) Check current src and dst address pair. If they are null,
 *               that means the ipsec(4) I/F is deletetunnel'ed, so it is
 *               not needed to update.
 *     (2) update
 *         (2-1) Create variant for new SPs.
 *         (2-2) Create temporary "null" variant used to avoid to access
 *               dangling variant while SPs are deleted and added.
 *               NOTE:
 *               There is the same problem as if_ipsec_delete_tunnel().
 *         (2-3) Swap variant include its SPs.
 *         (2-4) Cleanup unused configurations.
 *               NOTE: use the same encap_cookies.
 */
static int
if_ipsec_ensure_flags(struct ifnet *ifp, short oflags)
{
	struct ipsec_softc *sc = ifp->if_softc;
	struct ipsec_variant *ovar, *nvar, *nullvar;
	int error;

	/*
	 * (1) Check flags are changed.
	 */
	if ((oflags & (IFF_NAT_T|IFF_FWD_IPV6)) ==
	    (ifp->if_flags & (IFF_NAT_T|IFF_FWD_IPV6)))
		return 0; /* flags not changed. */

	error = encap_lock_enter();
	if (error)
		return error;

	nvar = kmem_zalloc(sizeof(*nvar), KM_SLEEP);
	nullvar = kmem_zalloc(sizeof(*nullvar), KM_SLEEP);

	mutex_enter(&sc->ipsec_lock);

	ovar = sc->ipsec_var;
	/*
	 * (1-2) Check current src and dst address pair.
	 */
	if (if_ipsec_variant_is_unconfigured(ovar)) {
		/* nothing to do */
		mutex_exit(&sc->ipsec_lock);
		encap_lock_exit();
		return 0;
	}

	/*
	 * (2-1) Create variant for new SPs.
	 */
	if_ipsec_copy_variant(nvar, ovar);
	psref_target_init(&nvar->iv_psref, iv_psref_class);
	/*
	 * (2-2) Create temporary "null" variant used to avoid to access
	 *       dangling variant while SPs are deleted and added.
	 */
	if_ipsec_copy_variant(nullvar, ovar);
	if_ipsec_clear_config(nullvar);
	psref_target_init(&nullvar->iv_psref, iv_psref_class);
	membar_producer();
	/*
	 * (2-3) Swap variant include its SPs.
	 */
	error = if_ipsec_update_variant(sc, nvar, nullvar);

	mutex_exit(&sc->ipsec_lock);
	encap_lock_exit();

	/*
	 * (2-4) Cleanup unused configurations.
	 */
	if (!error)
		kmem_free(ovar, sizeof(*ovar));
	else
		kmem_free(nvar, sizeof(*ovar));
	kmem_free(nullvar, sizeof(*nullvar));

	return error;
}

/*
 * SPD management
 */

/*
 * Share SP set with other NAT-T ipsec(4) I/F(s).
 *     Return 1, when "var" shares SP set.
 *     Return 0, when "var" cannot share SP set.
 *
 * NOTE:
 * if_ipsec_share_sp() and if_ipsec_unshare_sp() would require global lock
 * to exclude other ipsec(4) I/Fs set_tunnel/delete_tunnel. E.g. when ipsec0
 * and ipsec1 can share SP set, running ipsec0's set_tunnel and ipsec1's
 * set_tunnel causes race.
 * Currently, (fortunately) encap_lock works as this global lock.
 */
static int
if_ipsec_share_sp(struct ipsec_variant *var)
{
	struct ipsec_softc *sc = var->iv_softc;
	struct ipsec_softc *sc2;
	struct ipsec_variant *var2;
	struct psref psref;

	KASSERT(encap_lock_held());
	KASSERT(var->iv_psrc != NULL && var->iv_pdst != NULL);

	mutex_enter(&ipsec_softcs.lock);
	LIST_FOREACH(sc2, &ipsec_softcs.list, ipsec_list) {
		if (sc2 == sc)
			continue;
		var2 = if_ipsec_getref_variant(sc2, &psref);
		if (if_ipsec_variant_is_unconfigured(var2)) {
			if_ipsec_putref_variant(var2, &psref);
			continue;
		}
		if (sockaddr_cmp(var2->iv_pdst, var->iv_pdst) != 0 ||
		    sockaddr_cmp(var2->iv_psrc, var->iv_psrc) != 0) {
			if_ipsec_putref_variant(var2, &psref);
			continue;
		}

		break;
	}
	mutex_exit(&ipsec_softcs.lock);
	if (sc2 == NULL)
		return 0; /* not shared */

	IV_SP_IN(var) = IV_SP_IN(var2);
	IV_SP_IN6(var) = IV_SP_IN6(var2);
	IV_SP_OUT(var) = IV_SP_OUT(var2);
	IV_SP_OUT6(var) = IV_SP_OUT6(var2);

	if_ipsec_putref_variant(var2, &psref);
	return 1; /* shared */
}

/*
 * Unshare SP set with other NAT-T ipsec(4) I/F(s).
 *     Return 1, when "var" shared SP set, and then unshare them.
 *     Return 0, when "var" did not share SP set.
 *
 * NOTE:
 * See if_ipsec_share_sp()'s note.
 */
static int
if_ipsec_unshare_sp(struct ipsec_variant *var)
{
	struct ipsec_softc *sc = var->iv_softc;
	struct ipsec_softc *sc2;
	struct ipsec_variant *var2;
	struct psref psref;

	KASSERT(encap_lock_held());

	if (!var->iv_pdst || !var->iv_psrc)
		return 0;

	mutex_enter(&ipsec_softcs.lock);
	LIST_FOREACH(sc2, &ipsec_softcs.list, ipsec_list) {
		if (sc2 == sc)
			continue;
		var2 = if_ipsec_getref_variant(sc2, &psref);
		if (!var2->iv_pdst || !var2->iv_psrc) {
			if_ipsec_putref_variant(var2, &psref);
			continue;
		}
		if (sockaddr_cmp(var2->iv_pdst, var->iv_pdst) != 0 ||
		    sockaddr_cmp(var2->iv_psrc, var->iv_psrc) != 0) {
			if_ipsec_putref_variant(var2, &psref);
			continue;
		}

		break;
	}
	mutex_exit(&ipsec_softcs.lock);
	if (sc2 == NULL)
		return 0; /* not shared */

	IV_SP_IN(var) = NULL;
	IV_SP_IN6(var) = NULL;
	IV_SP_OUT(var) = NULL;
	IV_SP_OUT6(var) = NULL;
	if_ipsec_putref_variant(var2, &psref);
	return 1; /* shared */
}

static inline void
if_ipsec_add_mbuf_optalign(struct mbuf *m0, void *data, size_t len, bool align)
{
	struct mbuf *m;

	MGET(m, M_WAITOK | M_ZERO, MT_DATA);
	if (align)
		m->m_len = PFKEY_ALIGN8(len);
	else
		m->m_len = len;
	m_copyback(m, 0, len, data);
	m_cat(m0, m);
}

static inline void
if_ipsec_add_mbuf(struct mbuf *m0, void *data, size_t len)
{

	if_ipsec_add_mbuf_optalign(m0, data, len, true);
}

static inline void
if_ipsec_add_mbuf_addr_port(struct mbuf *m0, struct sockaddr *addr, in_port_t port, bool align)
{

	if (port == 0) {
		if_ipsec_add_mbuf_optalign(m0, addr, addr->sa_len, align);
	} else {
		union sockaddr_union addrport_u;
		struct sockaddr *addrport = &addrport_u.sa;

		if_ipsec_set_addr_port(addrport, addr, port);
		if_ipsec_add_mbuf_optalign(m0, addrport, addrport->sa_len, align);
	}
}

static inline void
if_ipsec_add_pad(struct mbuf *m0, size_t len)
{
	struct mbuf *m;

	if (len == 0)
		return;

	MGET(m, M_WAITOK | M_ZERO, MT_DATA);
	m->m_len = len;
	m_cat(m0, m);
}

static inline size_t
if_ipsec_set_sadb_addr(struct sadb_address *saaddr, struct sockaddr *addr,
    int proto, uint16_t exttype)
{
	size_t size;

	KASSERT(saaddr != NULL);
	KASSERT(addr != NULL);

	size = sizeof(*saaddr) + PFKEY_ALIGN8(addr->sa_len);
	saaddr->sadb_address_len = PFKEY_UNIT64(size);
	saaddr->sadb_address_exttype = exttype;
	saaddr->sadb_address_proto = proto;
	switch (addr->sa_family) {
#ifdef INET
	case AF_INET:
		saaddr->sadb_address_prefixlen = sizeof(struct in_addr) << 3;
		break;
#endif /* INET */
#ifdef INET6
	case AF_INET6:
		saaddr->sadb_address_prefixlen = sizeof(struct in6_addr) << 3;
		break;
#endif /* INET6 */
	default:
		log(LOG_DEBUG,
		    "%s: Invalid address family: %d.\n",
		    __func__, addr->sa_family);
		break;
	}
	saaddr->sadb_address_reserved = 0;

	return size;
}

static inline size_t
if_ipsec_set_sadb_src(struct sadb_address *sasrc, struct sockaddr *src,
    int proto)
{

	return if_ipsec_set_sadb_addr(sasrc, src, proto,
	    SADB_EXT_ADDRESS_SRC);
}

static inline size_t
if_ipsec_set_sadb_dst(struct sadb_address *sadst, struct sockaddr *dst,
    int proto)
{

	return if_ipsec_set_sadb_addr(sadst, dst, proto,
	    SADB_EXT_ADDRESS_DST);
}

static inline size_t
if_ipsec_set_sadb_x_policy(struct sadb_x_policy *xpl,
    struct sadb_x_ipsecrequest *xisr, uint16_t policy, uint8_t dir, uint32_t id,
    uint8_t level, struct sockaddr *src, struct sockaddr *dst)
{
	size_t size;

	KASSERT(policy != IPSEC_POLICY_IPSEC || xisr != NULL);

	size = sizeof(*xpl);
	if (policy == IPSEC_POLICY_IPSEC) {
		size += PFKEY_ALIGN8(sizeof(*xisr));
		if (src != NULL && dst != NULL)
			size += PFKEY_ALIGN8(src->sa_len + dst->sa_len);
	}
	xpl->sadb_x_policy_len = PFKEY_UNIT64(size);
	xpl->sadb_x_policy_exttype = SADB_X_EXT_POLICY;
	xpl->sadb_x_policy_type = policy;
	xpl->sadb_x_policy_dir = dir;
	xpl->sadb_x_policy_reserved = 0;
	xpl->sadb_x_policy_id = id;
	xpl->sadb_x_policy_reserved2 = 0;

	if (policy == IPSEC_POLICY_IPSEC) {
		xisr->sadb_x_ipsecrequest_len = PFKEY_ALIGN8(sizeof(*xisr));
		if (src != NULL && dst != NULL)
			xisr->sadb_x_ipsecrequest_len +=
				PFKEY_ALIGN8(src->sa_len + dst->sa_len);
		xisr->sadb_x_ipsecrequest_proto = IPPROTO_ESP;
		xisr->sadb_x_ipsecrequest_mode = IPSEC_MODE_TRANSPORT;
		xisr->sadb_x_ipsecrequest_level = level;
		xisr->sadb_x_ipsecrequest_reqid = key_newreqid();
	}

	return size;
}

static inline void
if_ipsec_set_sadb_msg(struct sadb_msg *msg, uint16_t extlen, uint8_t msgtype)
{

	KASSERT(msg != NULL);

	msg->sadb_msg_version = PF_KEY_V2;
	msg->sadb_msg_type = msgtype;
	msg->sadb_msg_errno = 0;
	msg->sadb_msg_satype = SADB_SATYPE_UNSPEC;
	msg->sadb_msg_len = PFKEY_UNIT64(sizeof(*msg)) + extlen;
	msg->sadb_msg_reserved = 0;
	msg->sadb_msg_seq = 0; /* XXXX */
	msg->sadb_msg_pid = 0; /* XXXX */
}

static inline void
if_ipsec_set_sadb_msg_add(struct sadb_msg *msg, uint16_t extlen)
{

	if_ipsec_set_sadb_msg(msg, extlen, SADB_X_SPDADD);
}

static inline void
if_ipsec_set_sadb_msg_del(struct sadb_msg *msg, uint16_t extlen)
{

	if_ipsec_set_sadb_msg(msg, extlen, SADB_X_SPDDELETE2);
}

static int
if_ipsec_set_addr_port(struct sockaddr *addrport, struct sockaddr *addr,
    in_port_t port)
{
	int error = 0;

	sockaddr_copy(addrport, addr->sa_len, addr);

	switch (addr->sa_family) {
#ifdef INET
	case AF_INET: {
		struct sockaddr_in *sin = satosin(addrport);
		sin->sin_port = port;
		break;
	}
#endif /* INET */
#ifdef INET6
	case AF_INET6: {
		struct sockaddr_in6 *sin6 = satosin6(addrport);
		sin6->sin6_port = port;
		break;
	}
#endif /* INET6 */
	default:
		log(LOG_DEBUG,
		    "%s: Invalid address family: %d.\n",
		    __func__, addr->sa_family);
		error = EINVAL;
	}

	return error;
}

static struct secpolicy *
if_ipsec_add_sp0(struct sockaddr *src, in_port_t sport,
    struct sockaddr *dst, in_port_t dport,
    int dir, int proto, int level, u_int policy)
{
	struct sadb_msg msg;
	struct sadb_address xsrc, xdst;
	struct sadb_x_policy xpl;
	struct sadb_x_ipsecrequest xisr;
	size_t size;
	size_t padlen;
	uint16_t ext_msg_len = 0;
	struct mbuf *m;

	memset(&msg, 0, sizeof(msg));
	memset(&xsrc, 0, sizeof(xsrc));
	memset(&xdst, 0, sizeof(xdst));
	memset(&xpl, 0, sizeof(xpl));
	memset(&xisr, 0, sizeof(xisr));

	MGETHDR(m, M_WAITOK, MT_DATA);

	size = if_ipsec_set_sadb_src(&xsrc, src, proto);
	ext_msg_len += PFKEY_UNIT64(size);
	size = if_ipsec_set_sadb_dst(&xdst, dst, proto);
	ext_msg_len += PFKEY_UNIT64(size);
	size = if_ipsec_set_sadb_x_policy(&xpl, &xisr, policy, dir, 0, level, src, dst);
	ext_msg_len += PFKEY_UNIT64(size);
	if_ipsec_set_sadb_msg_add(&msg, ext_msg_len);

	/* build PF_KEY message */

	m->m_len = sizeof(msg);
	m_copyback(m, 0, sizeof(msg), &msg);

	if_ipsec_add_mbuf(m, &xsrc, sizeof(xsrc));
	if_ipsec_add_mbuf_addr_port(m, src, sport, true);
	padlen = PFKEY_UNUNIT64(xsrc.sadb_address_len)
		- (sizeof(xsrc) + PFKEY_ALIGN8(src->sa_len));
	if_ipsec_add_pad(m, padlen);

	if_ipsec_add_mbuf(m, &xdst, sizeof(xdst));
	if_ipsec_add_mbuf_addr_port(m, dst, dport, true);
	padlen = PFKEY_UNUNIT64(xdst.sadb_address_len)
		- (sizeof(xdst) + PFKEY_ALIGN8(dst->sa_len));
	if_ipsec_add_pad(m, padlen);

	if_ipsec_add_mbuf(m, &xpl, sizeof(xpl));
	if (policy == IPSEC_POLICY_IPSEC) {
		if_ipsec_add_mbuf(m, &xisr, sizeof(xisr));
		if_ipsec_add_mbuf_addr_port(m, src, sport, false);
		if_ipsec_add_mbuf_addr_port(m, dst, dport, false);
	}
	padlen = PFKEY_UNUNIT64(xpl.sadb_x_policy_len) - sizeof(xpl);
	if (src != NULL && dst != NULL)
		padlen -= PFKEY_ALIGN8(src->sa_len + dst->sa_len);
	if_ipsec_add_pad(m, padlen);

	/* key_kpi_spdadd() has already done KEY_SP_REF(). */
	return key_kpi_spdadd(m);
}

static int
if_ipsec_add_sp(struct ipsec_variant *var,
    struct sockaddr *src, in_port_t sport,
    struct sockaddr *dst, in_port_t dport)
{
	struct ipsec_softc *sc = var->iv_softc;
	int level;
	u_int v6policy;

	/*
	 * must delete sp before add it.
	 */
	KASSERT(IV_SP_IN(var) == NULL);
	KASSERT(IV_SP_OUT(var) == NULL);
	KASSERT(IV_SP_IN6(var) == NULL);
	KASSERT(IV_SP_OUT6(var) == NULL);

	/*
	 * can be shared?
	 */
	if (if_ipsec_share_sp(var))
		return 0;

	if (if_ipsec_nat_t(sc))
		level = IPSEC_LEVEL_REQUIRE;
	else
		level = IPSEC_LEVEL_UNIQUE;

	if (if_ipsec_fwd_ipv6(sc))
		v6policy = IPSEC_POLICY_IPSEC;
	else
		v6policy = IPSEC_POLICY_DISCARD;

	IV_SP_IN(var) = if_ipsec_add_sp0(dst, dport, src, sport,
	    IPSEC_DIR_INBOUND, IPPROTO_IPIP, level, IPSEC_POLICY_IPSEC);
	if (IV_SP_IN(var) == NULL)
		goto fail;
	IV_SP_OUT(var) = if_ipsec_add_sp0(src, sport, dst, dport,
	    IPSEC_DIR_OUTBOUND, IPPROTO_IPIP, level, IPSEC_POLICY_IPSEC);
	if (IV_SP_OUT(var) == NULL)
		goto fail;
	IV_SP_IN6(var) = if_ipsec_add_sp0(dst, dport, src, sport,
	    IPSEC_DIR_INBOUND, IPPROTO_IPV6, level, v6policy);
	if (IV_SP_IN6(var) == NULL)
		goto fail;
	IV_SP_OUT6(var) = if_ipsec_add_sp0(src, sport, dst, dport,
	    IPSEC_DIR_OUTBOUND, IPPROTO_IPV6, level, v6policy);
	if (IV_SP_OUT6(var) == NULL)
		goto fail;

	return 0;

fail:
	if (IV_SP_IN6(var) != NULL) {
		if_ipsec_del_sp0(IV_SP_IN6(var));
		IV_SP_IN6(var) = NULL;
	}
	if (IV_SP_OUT(var) != NULL) {
		if_ipsec_del_sp0(IV_SP_OUT(var));
		IV_SP_OUT(var) = NULL;
	}
	if (IV_SP_IN(var) != NULL) {
		if_ipsec_del_sp0(IV_SP_IN(var));
		IV_SP_IN(var) = NULL;
	}

	return EEXIST;
}

static int
if_ipsec_del_sp0(struct secpolicy *sp)
{
	struct sadb_msg msg;
	struct sadb_x_policy xpl;
	size_t size;
	uint16_t ext_msg_len = 0;
	int error;
	struct mbuf *m;

	if (sp == NULL)
		return 0;

	memset(&msg, 0, sizeof(msg));
	memset(&xpl, 0, sizeof(xpl));

	MGETHDR(m, M_WAITOK, MT_DATA);

	size = if_ipsec_set_sadb_x_policy(&xpl, NULL, 0, 0, sp->id, 0, NULL, NULL);
	ext_msg_len += PFKEY_UNIT64(size);

	if_ipsec_set_sadb_msg_del(&msg, ext_msg_len);

	m->m_len = sizeof(msg);
	m_copyback(m, 0, sizeof(msg), &msg);

	if_ipsec_add_mbuf(m, &xpl, sizeof(xpl));

	/*  unreference correspond to key_kpi_spdadd(). */
	KEY_SP_UNREF(&sp);
	error = key_kpi_spddelete2(m);
	if (error != 0) {
		log(LOG_ERR, "%s: cannot delete SP(ID=%u) (error=%d).\n",
		    __func__, sp->id, error);
	}
	return error;
}

static void
if_ipsec_del_sp(struct ipsec_variant *var)
{

	/* are the SPs shared? */
	if (if_ipsec_unshare_sp(var))
		return;

	(void)if_ipsec_del_sp0(IV_SP_OUT(var));
	(void)if_ipsec_del_sp0(IV_SP_IN(var));
	(void)if_ipsec_del_sp0(IV_SP_OUT6(var));
	(void)if_ipsec_del_sp0(IV_SP_IN6(var));
	IV_SP_IN(var) = NULL;
	IV_SP_IN6(var) = NULL;
	IV_SP_OUT(var) = NULL;
	IV_SP_OUT6(var) = NULL;
}

static int
if_ipsec_replace_sp(struct ipsec_softc *sc, struct ipsec_variant *ovar,
    struct ipsec_variant *nvar)
{
	in_port_t src_port = 0;
	in_port_t dst_port = 0;
	struct sockaddr *src;
	struct sockaddr *dst;
	int error = 0;

	KASSERT(mutex_owned(&sc->ipsec_lock));

	if_ipsec_del_sp(ovar);

	src = nvar->iv_psrc;
	dst = nvar->iv_pdst;
	if (if_ipsec_nat_t(sc)) {
		/* NAT-T enabled */
		src_port = nvar->iv_sport;
		dst_port = nvar->iv_dport;
	}
	if (src && dst)
		error = if_ipsec_add_sp(nvar, src, src_port, dst, dst_port);

	return error;
}

/*
 * ipsec_variant and its SPs update API.
 *
 * Assumption:
 * reader side dereferences sc->ipsec_var in reader critical section only,
 * that is, all of reader sides do not reader the sc->ipsec_var after
 * pserialize_perform().
 */
static int
if_ipsec_update_variant(struct ipsec_softc *sc, struct ipsec_variant *nvar,
    struct ipsec_variant *nullvar)
{
	struct ifnet *ifp = &sc->ipsec_if;
	struct ipsec_variant *ovar = sc->ipsec_var;
	int error;

	KASSERT(mutex_owned(&sc->ipsec_lock));

	/*
	 * To keep consistency between ipsec(4) I/F settings and SPs,
	 * we stop packet processing while replacing SPs, that is, we set
	 * "null" config variant to sc->ipsec_var.
	 */
	sc->ipsec_var = nullvar;
	pserialize_perform(ipsec_psz);
	psref_target_destroy(&ovar->iv_psref, iv_psref_class);

	error = if_ipsec_replace_sp(sc, ovar, nvar);
	if (!error)
		sc->ipsec_var = nvar;
	else {
		sc->ipsec_var = ovar; /* rollback */
		psref_target_init(&ovar->iv_psref, iv_psref_class);
	}

	pserialize_perform(ipsec_psz);
	psref_target_destroy(&nullvar->iv_psref, iv_psref_class);

	if (if_ipsec_variant_is_configured(sc->ipsec_var))
		ifp->if_flags |= IFF_RUNNING;
	else
		ifp->if_flags &= ~IFF_RUNNING;

	return error;
}
