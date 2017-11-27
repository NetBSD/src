/*	$NetBSD: if_gif.c,v 1.134 2017/11/27 05:05:50 knakahara Exp $	*/
/*	$KAME: if_gif.c,v 1.76 2001/08/20 02:01:02 kjc Exp $	*/

/*
 * Copyright (C) 1995, 1996, 1997, and 1998 WIDE Project.
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
 * 3. Neither the name of the project nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE PROJECT AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE PROJECT OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: if_gif.c,v 1.134 2017/11/27 05:05:50 knakahara Exp $");

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
#include <sys/socketvar.h>
#include <sys/syslog.h>
#include <sys/proc.h>
#include <sys/cpu.h>
#include <sys/intr.h>
#include <sys/kmem.h>
#include <sys/sysctl.h>
#include <sys/xcall.h>
#include <sys/device.h>
#include <sys/module.h>
#include <sys/mutex.h>
#include <sys/pserialize.h>
#include <sys/psref.h>

#include <net/if.h>
#include <net/if_types.h>
#include <net/netisr.h>
#include <net/route.h>
#include <net/bpf.h>

#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#ifdef	INET
#include <netinet/in_var.h>
#endif	/* INET */
#include <netinet/in_gif.h>

#ifdef INET6
#ifndef INET
#include <netinet/in.h>
#endif
#include <netinet6/in6_var.h>
#include <netinet/ip6.h>
#include <netinet6/ip6_var.h>
#include <netinet6/in6_gif.h>
#endif /* INET6 */

#include <netinet/ip_encap.h>
#include <net/if_gif.h>

#include <net/net_osdep.h>

#include "ioconf.h"

#ifdef NET_MPSAFE
#define GIF_MPSAFE	1
#endif

/*
 * gif global variable definitions
 */
LIST_HEAD(gif_sclist, gif_softc);
static struct {
	struct gif_sclist list;
	kmutex_t lock;
} gif_softcs __cacheline_aligned;

pserialize_t gif_psz __read_mostly;
struct psref_class *gv_psref_class __read_mostly;

static void	gif_ro_init_pc(void *, void *, struct cpu_info *);
static void	gif_ro_fini_pc(void *, void *, struct cpu_info *);

static int	gifattach0(struct gif_softc *);
static int	gif_output(struct ifnet *, struct mbuf *,
			   const struct sockaddr *, const struct rtentry *);
static void	gif_start(struct ifnet *);
static int	gif_transmit(struct ifnet *, struct mbuf *);
static int	gif_transmit_direct(struct gif_variant *, struct mbuf *);
static int	gif_ioctl(struct ifnet *, u_long, void *);
static int	gif_set_tunnel(struct ifnet *, struct sockaddr *,
			       struct sockaddr *);
static void	gif_delete_tunnel(struct ifnet *);

static int	gif_clone_create(struct if_clone *, int);
static int	gif_clone_destroy(struct ifnet *);
static int	gif_check_nesting(struct ifnet *, struct mbuf *);

static int	gif_encap_attach(struct gif_variant *);
static int	gif_encap_detach(struct gif_variant *);

static void	gif_update_variant(struct gif_softc *, struct gif_variant *);

static struct if_clone gif_cloner =
    IF_CLONE_INITIALIZER("gif", gif_clone_create, gif_clone_destroy);

#ifndef MAX_GIF_NEST
/*
 * This macro controls the upper limitation on nesting of gif tunnels.
 * Since, setting a large value to this macro with a careless configuration
 * may introduce system crash, we don't allow any nestings by default.
 * If you need to configure nested gif tunnels, you can define this macro
 * in your kernel configuration file.  However, if you do so, please be
 * careful to configure the tunnels so that it won't make a loop.
 */
#define MAX_GIF_NEST 1
#endif
static int max_gif_nesting = MAX_GIF_NEST;

static struct sysctllog *gif_sysctl;

static void
gif_sysctl_setup(void)
{
	gif_sysctl = NULL;

#ifdef INET
	/*
	 * Previously create "net.inet.ip" entry to avoid sysctl_createv error.
	 */
	sysctl_createv(NULL, 0, NULL, NULL,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_NODE, "inet",
		       SYSCTL_DESCR("PF_INET related settings"),
		       NULL, 0, NULL, 0,
		       CTL_NET, PF_INET, CTL_EOL);
	sysctl_createv(NULL, 0, NULL, NULL,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_NODE, "ip",
		       SYSCTL_DESCR("IPv4 related settings"),
		       NULL, 0, NULL, 0,
		       CTL_NET, PF_INET, IPPROTO_IP, CTL_EOL);

	sysctl_createv(&gif_sysctl, 0, NULL, NULL,
		       CTLFLAG_PERMANENT|CTLFLAG_READWRITE,
		       CTLTYPE_INT, "gifttl",
		       SYSCTL_DESCR("Default TTL for a gif tunnel datagram"),
		       NULL, 0, &ip_gif_ttl, 0,
		       CTL_NET, PF_INET, IPPROTO_IP,
		       IPCTL_GIF_TTL, CTL_EOL);
#endif
#ifdef INET6
	/*
	 * Previously create "net.inet6.ip6" entry to avoid sysctl_createv error.
	 */
	sysctl_createv(NULL, 0, NULL, NULL,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_NODE, "inet6",
		       SYSCTL_DESCR("PF_INET6 related settings"),
		       NULL, 0, NULL, 0,
		       CTL_NET, PF_INET6, CTL_EOL);
	sysctl_createv(NULL, 0, NULL, NULL,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_NODE, "ip6",
		       SYSCTL_DESCR("IPv6 related settings"),
		       NULL, 0, NULL, 0,
		       CTL_NET, PF_INET6, IPPROTO_IPV6, CTL_EOL);

	sysctl_createv(&gif_sysctl, 0, NULL, NULL,
		       CTLFLAG_PERMANENT|CTLFLAG_READWRITE,
		       CTLTYPE_INT, "gifhlim",
		       SYSCTL_DESCR("Default hop limit for a gif tunnel datagram"),
		       NULL, 0, &ip6_gif_hlim, 0,
		       CTL_NET, PF_INET6, IPPROTO_IPV6,
		       IPV6CTL_GIF_HLIM, CTL_EOL);
#endif
}

/* ARGSUSED */
void
gifattach(int count)
{
	/*
	 * Nothing to do here, initialization is handled by the
	 * module initialization code in gifinit() below).
	 */
}

static void
gifinit(void)
{

	mutex_init(&gif_softcs.lock, MUTEX_DEFAULT, IPL_NONE);
	LIST_INIT(&gif_softcs.list);
	if_clone_attach(&gif_cloner);

	gif_psz = pserialize_create();
	gv_psref_class = psref_class_create("gifvar", IPL_SOFTNET);

	gif_sysctl_setup();
}

static int
gifdetach(void)
{
	int error = 0;

	mutex_enter(&gif_softcs.lock);
	if (!LIST_EMPTY(&gif_softcs.list)) {
		mutex_exit(&gif_softcs.lock);
		error = EBUSY;
	}

	if (error == 0) {
		psref_class_destroy(gv_psref_class);
		pserialize_destroy(gif_psz);

		if_clone_detach(&gif_cloner);
		sysctl_teardown(&gif_sysctl);
	}

	return error;
}

static int
gif_clone_create(struct if_clone *ifc, int unit)
{
	struct gif_softc *sc;
	struct gif_variant *var;
	int rv;

	sc = kmem_zalloc(sizeof(struct gif_softc), KM_SLEEP);

	if_initname(&sc->gif_if, ifc->ifc_name, unit);

	rv = gifattach0(sc);
	if (rv != 0) {
		kmem_free(sc, sizeof(struct gif_softc));
		return rv;
	}

	var = kmem_zalloc(sizeof(*var), KM_SLEEP);
	var->gv_softc = sc;
	psref_target_init(&var->gv_psref, gv_psref_class);

	sc->gif_var = var;
	mutex_init(&sc->gif_lock, MUTEX_DEFAULT, IPL_NONE);
	sc->gif_ro_percpu = percpu_alloc(sizeof(struct gif_ro));
	percpu_foreach(sc->gif_ro_percpu, gif_ro_init_pc, NULL);

	mutex_enter(&gif_softcs.lock);
	LIST_INSERT_HEAD(&gif_softcs.list, sc, gif_list);
	mutex_exit(&gif_softcs.lock);
	return 0;
}

static int
gifattach0(struct gif_softc *sc)
{
	int rv;

	sc->gif_if.if_addrlen = 0;
	sc->gif_if.if_mtu    = GIF_MTU;
	sc->gif_if.if_flags  = IFF_POINTOPOINT | IFF_MULTICAST;
	sc->gif_if.if_extflags  = IFEF_NO_LINK_STATE_CHANGE;
#ifdef GIF_MPSAFE
	sc->gif_if.if_extflags  |= IFEF_MPSAFE;
#endif
	sc->gif_if.if_ioctl  = gif_ioctl;
	sc->gif_if.if_output = gif_output;
	sc->gif_if.if_start = gif_start;
	sc->gif_if.if_transmit = gif_transmit;
	sc->gif_if.if_type   = IFT_GIF;
	sc->gif_if.if_dlt    = DLT_NULL;
	sc->gif_if.if_softc  = sc;
	IFQ_SET_READY(&sc->gif_if.if_snd);
	rv = if_initialize(&sc->gif_if);
	if (rv != 0)
		return rv;

	if_register(&sc->gif_if);
	if_alloc_sadl(&sc->gif_if);
	bpf_attach(&sc->gif_if, DLT_NULL, sizeof(u_int));
	return 0;
}

static void
gif_ro_init_pc(void *p, void *arg __unused, struct cpu_info *ci __unused)
{
	struct gif_ro *gro = p;

	mutex_init(&gro->gr_lock, MUTEX_DEFAULT, IPL_NONE);
}

static void
gif_ro_fini_pc(void *p, void *arg __unused, struct cpu_info *ci __unused)
{
	struct gif_ro *gro = p;

	rtcache_free(&gro->gr_ro);

	mutex_destroy(&gro->gr_lock);
}

void
gif_rtcache_free_pc(void *p, void *arg __unused, struct cpu_info *ci __unused)
{
	struct gif_ro *gro = p;

	rtcache_free(&gro->gr_ro);
}

static int
gif_clone_destroy(struct ifnet *ifp)
{
	struct gif_softc *sc = (void *) ifp;
	struct gif_variant *var;

	LIST_REMOVE(sc, gif_list);

	gif_delete_tunnel(&sc->gif_if);
	bpf_detach(ifp);
	if_detach(ifp);

	percpu_foreach(sc->gif_ro_percpu, gif_ro_fini_pc, NULL);
	percpu_free(sc->gif_ro_percpu, sizeof(struct gif_ro));

	mutex_destroy(&sc->gif_lock);

	var = sc->gif_var;
	kmem_free(var, sizeof(*var));
	kmem_free(sc, sizeof(struct gif_softc));

	return 0;
}

#ifdef GIF_ENCAPCHECK
int
gif_encapcheck(struct mbuf *m, int off, int proto, void *arg)
{
	struct ip ip;
	struct gif_softc *sc;
	struct gif_variant *var;
	struct psref psref;
	int ret = 0;

	sc = arg;
	if (sc == NULL)
		return 0;

	if ((sc->gif_if.if_flags & IFF_UP) == 0)
		return 0;

	var = gif_getref_variant(sc, &psref);
	if (var->gv_psrc == NULL || var->gv_pdst == NULL)
		goto out;

	/* no physical address */
	if (!var->gv_psrc || !var->gv_pdst)
		goto out;

	switch (proto) {
#ifdef INET
	case IPPROTO_IPV4:
		break;
#endif
#ifdef INET6
	case IPPROTO_IPV6:
		break;
#endif
	default:
		goto out;
	}

	/* Bail on short packets */
	KASSERT(m->m_flags & M_PKTHDR);
	if (m->m_pkthdr.len < sizeof(ip))
		goto  out;

	m_copydata(m, 0, sizeof(ip), &ip);

	switch (ip.ip_v) {
#ifdef INET
	case 4:
		if (var->gv_psrc->sa_family != AF_INET ||
		    var->gv_pdst->sa_family != AF_INET)
			goto out;
		ret = gif_encapcheck4(m, off, proto, var);
		break;
#endif
#ifdef INET6
	case 6:
		if (m->m_pkthdr.len < sizeof(struct ip6_hdr))
			goto out;
		if (var->gv_psrc->sa_family != AF_INET6 ||
		    var->gv_pdst->sa_family != AF_INET6)
			goto out;
		ret = gif_encapcheck6(m, off, proto, var);
		break;
#endif
	default:
		goto out;
	}

out:
	gif_putref_variant(var, &psref);
	return ret;
}
#endif

/*
 * gif may cause infinite recursion calls when misconfigured.
 * We'll prevent this by introducing upper limit.
 */
static int
gif_check_nesting(struct ifnet *ifp, struct mbuf *m)
{
	struct m_tag *mtag;
	int *count;

	mtag = m_tag_find(m, PACKET_TAG_TUNNEL_INFO, NULL);
	if (mtag != NULL) {
		count = (int *)(mtag + 1);
		if (++(*count) > max_gif_nesting) {
			log(LOG_NOTICE,
			    "%s: recursively called too many times(%d)\n",
			    if_name(ifp),
			    *count);
			return EIO;
		}
	} else {
		mtag = m_tag_get(PACKET_TAG_TUNNEL_INFO, sizeof(*count),
		    M_NOWAIT);
		if (mtag != NULL) {
			m_tag_prepend(m, mtag);
			count = (int *)(mtag + 1);
			*count = 0;
		} else {
			log(LOG_DEBUG,
			    "%s: m_tag_get() failed, recursion calls are not prevented.\n",
			    if_name(ifp));
		}
	}

	return 0;
}

static int
gif_output(struct ifnet *ifp, struct mbuf *m, const struct sockaddr *dst,
    const struct rtentry *rt)
{
	struct gif_softc *sc = ifp->if_softc;
	struct gif_variant *var = NULL;
	struct psref psref;
	int error = 0;

	IFQ_CLASSIFY(&ifp->if_snd, m, dst->sa_family);

	if ((error = gif_check_nesting(ifp, m)) != 0) {
		m_free(m);
		goto end;
	}

	if ((ifp->if_flags & IFF_UP) == 0) {
		m_freem(m);
		error = ENETDOWN;
		goto end;
	}

	var = gif_getref_variant(sc, &psref);
	if (var->gv_psrc == NULL || var->gv_pdst == NULL) {
		m_freem(m);
		error = ENETDOWN;
		goto end;
	}
	/* XXX should we check if our outer source is legal? */

	m->m_flags &= ~(M_BCAST|M_MCAST);

	/* use DLT_NULL encapsulation here to pass inner af type */
	M_PREPEND(m, sizeof(int), M_DONTWAIT);
	if (!m) {
		error = ENOBUFS;
		goto end;
	}
	*mtod(m, int *) = dst->sa_family;

	/* Clear checksum-offload flags. */
	m->m_pkthdr.csum_flags = 0;
	m->m_pkthdr.csum_data = 0;

	error = gif_transmit_direct(var, m);
end:
	if (var != NULL)
		gif_putref_variant(var, &psref);
	if (error)
		ifp->if_oerrors++;
	return error;
}

static void
gif_start(struct ifnet *ifp)
{
	struct gif_softc *sc;
	struct gif_variant *var;
	struct mbuf *m;
	struct psref psref;
	int family;
	int len;
	int error;

	sc = ifp->if_softc;
	var = gif_getref_variant(sc, &psref);

	KASSERT(var->gv_output != NULL);

	/* output processing */
	while (1) {
		IFQ_DEQUEUE(&sc->gif_if.if_snd, m);
		if (m == NULL)
			break;

		/* grab and chop off inner af type */
		if (sizeof(int) > m->m_len) {
			m = m_pullup(m, sizeof(int));
			if (!m) {
				ifp->if_oerrors++;
				continue;
			}
		}
		family = *mtod(m, int *);
		bpf_mtap(ifp, m);
		m_adj(m, sizeof(int));

		len = m->m_pkthdr.len;

		error = var->gv_output(var, family, m);
		if (error)
			ifp->if_oerrors++;
		else {
			ifp->if_opackets++;
			ifp->if_obytes += len;
		}
	}

	gif_putref_variant(var, &psref);
}

static int
gif_transmit(struct ifnet *ifp, struct mbuf *m)
{
	struct gif_softc *sc;
	struct gif_variant *var;
	struct psref psref;
	int error;

	sc = ifp->if_softc;

	/* output processing */
	if (m == NULL)
		return EINVAL;

	var = gif_getref_variant(sc, &psref);
	error = gif_transmit_direct(var, m);
	gif_putref_variant(var, &psref);

	return error;
}

static int
gif_transmit_direct(struct gif_variant *var, struct mbuf *m)
{
	struct ifnet *ifp = &var->gv_softc->gif_if;
	int error;
	int family;
	int len;

	KASSERT(gif_heldref_variant(var));
	KASSERT(var->gv_output != NULL);

	/* grab and chop off inner af type */
	if (sizeof(int) > m->m_len) {
		m = m_pullup(m, sizeof(int));
		if (!m) {
			ifp->if_oerrors++;
			return ENOBUFS;
		}
	}
	family = *mtod(m, int *);
	bpf_mtap(ifp, m);
	m_adj(m, sizeof(int));

	len = m->m_pkthdr.len;

	error = var->gv_output(var, family, m);
	if (error)
		ifp->if_oerrors++;
	else {
		ifp->if_opackets++;
		ifp->if_obytes += len;
	}

	return error;
}

void
gif_input(struct mbuf *m, int af, struct ifnet *ifp)
{
	pktqueue_t *pktq;
	size_t pktlen;

	if (ifp == NULL) {
		/* just in case */
		m_freem(m);
		return;
	}

	m_set_rcvif(m, ifp);
	pktlen = m->m_pkthdr.len;

	bpf_mtap_af(ifp, af, m);

	/*
	 * Put the packet to the network layer input queue according to the
	 * specified address family.  Note: we avoid direct call to the
	 * input function of the network layer in order to avoid recursion.
	 * This may be revisited in the future.
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
		m_freem(m);
		return;
	}

#ifdef GIF_MPSAFE
	const u_int h = curcpu()->ci_index;
#else
	const uint32_t h = pktq_rps_hash(m);
#endif
	if (__predict_true(pktq_enqueue(pktq, m, h))) {
		ifp->if_ibytes += pktlen;
		ifp->if_ipackets++;
	} else {
		m_freem(m);
	}
}

/* XXX how should we handle IPv6 scope on SIOC[GS]IFPHYADDR? */
static int
gif_ioctl(struct ifnet *ifp, u_long cmd, void *data)
{
	struct gif_softc *sc  = ifp->if_softc;
	struct ifreq     *ifr = (struct ifreq*)data;
	struct ifaddr    *ifa = (struct ifaddr*)data;
	int error = 0, size, bound;
	struct sockaddr *dst, *src;
	struct gif_variant *var;
	struct psref psref;

	switch (cmd) {
	case SIOCINITIFADDR:
		ifp->if_flags |= IFF_UP;
		ifa->ifa_rtrequest = p2p_rtrequest;
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
		if (ifr->ifr_mtu < GIF_MTU_MIN || ifr->ifr_mtu > GIF_MTU_MAX)
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
#endif
#ifdef INET6
		case SIOCSIFPHYADDR_IN6:
			src = (struct sockaddr *)
				&(((struct in6_aliasreq *)data)->ifra_addr);
			dst = (struct sockaddr *)
				&(((struct in6_aliasreq *)data)->ifra_dstaddr);
			break;
#endif
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

		/* validate sa_len */
		switch (src->sa_family) {
#ifdef INET
		case AF_INET:
			if (src->sa_len != sizeof(struct sockaddr_in))
				return EINVAL;
			break;
#endif
#ifdef INET6
		case AF_INET6:
			if (src->sa_len != sizeof(struct sockaddr_in6))
				return EINVAL;
			break;
#endif
		default:
			return EAFNOSUPPORT;
		}
		switch (dst->sa_family) {
#ifdef INET
		case AF_INET:
			if (dst->sa_len != sizeof(struct sockaddr_in))
				return EINVAL;
			break;
#endif
#ifdef INET6
		case AF_INET6:
			if (dst->sa_len != sizeof(struct sockaddr_in6))
				return EINVAL;
			break;
#endif
		default:
			return EAFNOSUPPORT;
		}

		/* check sa_family looks sane for the cmd */
		switch (cmd) {
		case SIOCSIFPHYADDR:
			if (src->sa_family == AF_INET)
				break;
			return EAFNOSUPPORT;
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
		 * calls gif_getref_variant() for other softcs to check
		 * address pair duplicattion
		 */
		bound = curlwp_bind();
		error = gif_set_tunnel(&sc->gif_if, src, dst);
		curlwp_bindx(bound);
		break;

#ifdef SIOCDIFPHYADDR
	case SIOCDIFPHYADDR:
		bound = curlwp_bind();
		gif_delete_tunnel(&sc->gif_if);
		curlwp_bindx(bound);
		break;
#endif

	case SIOCGIFPSRCADDR:
#ifdef INET6
	case SIOCGIFPSRCADDR_IN6:
#endif /* INET6 */
		bound = curlwp_bind();
		var = gif_getref_variant(sc, &psref);
		if (var->gv_psrc == NULL) {
			gif_putref_variant(var, &psref);
			curlwp_bindx(bound);
			error = EADDRNOTAVAIL;
			goto bad;
		}
		src = var->gv_psrc;
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
			gif_putref_variant(var, &psref);
			curlwp_bindx(bound);
			error = EADDRNOTAVAIL;
			goto bad;
		}
		if (src->sa_len > size) {
			gif_putref_variant(var, &psref);
			curlwp_bindx(bound);
			return EINVAL;
		}
		memcpy(dst, src, src->sa_len);
		gif_putref_variant(var, &psref);
		curlwp_bindx(bound);
		break;

	case SIOCGIFPDSTADDR:
#ifdef INET6
	case SIOCGIFPDSTADDR_IN6:
#endif /* INET6 */
		bound = curlwp_bind();
		var = gif_getref_variant(sc, &psref);
		if (var->gv_pdst == NULL) {
			gif_putref_variant(var, &psref);
			curlwp_bindx(bound);
			error = EADDRNOTAVAIL;
			goto bad;
		}
		src = var->gv_pdst;
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
			gif_putref_variant(var, &psref);
			curlwp_bindx(bound);
			error = EADDRNOTAVAIL;
			goto bad;
		}
		if (src->sa_len > size) {
			gif_putref_variant(var, &psref);
			curlwp_bindx(bound);
			return EINVAL;
		}
		memcpy(dst, src, src->sa_len);
		gif_putref_variant(var, &psref);
		curlwp_bindx(bound);
		break;

	case SIOCGLIFPHYADDR:
		bound = curlwp_bind();
		var = gif_getref_variant(sc, &psref);
		if (var->gv_psrc == NULL || var->gv_pdst == NULL) {
			gif_putref_variant(var, &psref);
			curlwp_bindx(bound);
			error = EADDRNOTAVAIL;
			goto bad;
		}

		/* copy src */
		src = var->gv_psrc;
		dst = (struct sockaddr *)
			&(((struct if_laddrreq *)data)->addr);
		size = sizeof(((struct if_laddrreq *)data)->addr);
		if (src->sa_len > size) {
			gif_putref_variant(var, &psref);
			curlwp_bindx(bound);
			return EINVAL;
		}
		memcpy(dst, src, src->sa_len);

		/* copy dst */
		src = var->gv_pdst;
		dst = (struct sockaddr *)
			&(((struct if_laddrreq *)data)->dstaddr);
		size = sizeof(((struct if_laddrreq *)data)->dstaddr);
		if (src->sa_len > size) {
			gif_putref_variant(var, &psref);
			curlwp_bindx(bound);
			return EINVAL;
		}
		memcpy(dst, src, src->sa_len);
		gif_putref_variant(var, &psref);
		curlwp_bindx(bound);
		break;

	default:
		return ifioctl_common(ifp, cmd, data);
	}
 bad:
	return error;
}

static int
gif_encap_attach(struct gif_variant *var)
{
	int error;

	if (var == NULL || var->gv_psrc == NULL)
		return EINVAL;

	switch (var->gv_psrc->sa_family) {
#ifdef INET
	case AF_INET:
		error = in_gif_attach(var);
		break;
#endif
#ifdef INET6
	case AF_INET6:
		error = in6_gif_attach(var);
		break;
#endif
	default:
		error = EINVAL;
		break;
	}

	return error;
}

static int
gif_encap_detach(struct gif_variant *var)
{
	int error;

	if (var == NULL || var->gv_psrc == NULL)
		return EINVAL;

	switch (var->gv_psrc->sa_family) {
#ifdef INET
	case AF_INET:
		error = in_gif_detach(var);
		break;
#endif
#ifdef INET6
	case AF_INET6:
		error = in6_gif_detach(var);
		break;
#endif
	default:
		error = EINVAL;
		break;
	}

	return error;
}

static int
gif_set_tunnel(struct ifnet *ifp, struct sockaddr *src, struct sockaddr *dst)
{
	struct gif_softc *sc = ifp->if_softc;
	struct gif_softc *sc2;
	struct gif_variant *ovar, *nvar;
	struct sockaddr *osrc, *odst;
	struct sockaddr *nsrc, *ndst;
	int error;
#ifndef GIF_MPSAFE
	int s;

	s = splsoftnet();
#endif
	error = encap_lock_enter();
	if (error) {
#ifndef GIF_MPSAFE
		splx(s);
#endif
		return error;
	}

	nsrc = sockaddr_dup(src, M_WAITOK);
	ndst = sockaddr_dup(dst, M_WAITOK);
	nvar = kmem_alloc(sizeof(*nvar), KM_SLEEP);

	mutex_enter(&sc->gif_lock);

	ovar = sc->gif_var;

	if ((ovar->gv_pdst && sockaddr_cmp(ovar->gv_pdst, dst) == 0) &&
	    (ovar->gv_psrc && sockaddr_cmp(ovar->gv_psrc, src) == 0)) {
		/* address and port pair not changed. */
		error = 0;
		goto out;
	}

	mutex_enter(&gif_softcs.lock);
	LIST_FOREACH(sc2, &gif_softcs.list, gif_list) {
		struct gif_variant *var2;
		struct psref psref;

		if (sc2 == sc)
			continue;
		var2 = gif_getref_variant(sc, &psref);
		if (!var2->gv_pdst || !var2->gv_psrc) {
			gif_putref_variant(var2, &psref);
			continue;
		}
		/* can't configure same pair of address onto two gifs */
		if (sockaddr_cmp(var2->gv_pdst, dst) == 0 &&
		    sockaddr_cmp(var2->gv_psrc, src) == 0) {
			/* continue to use the old configureation. */
			gif_putref_variant(var2, &psref);
			mutex_exit(&gif_softcs.lock);
			error =  EADDRNOTAVAIL;
			goto out;
		}
		gif_putref_variant(var2, &psref);
		/* XXX both end must be valid? (I mean, not 0.0.0.0) */
	}
	mutex_exit(&gif_softcs.lock);

	osrc = ovar->gv_psrc;
	odst = ovar->gv_pdst;

	*nvar = *ovar;
	nvar->gv_psrc = nsrc;
	nvar->gv_pdst = ndst;
	nvar->gv_encap_cookie4 = NULL;
	nvar->gv_encap_cookie6 = NULL;
	error = gif_encap_attach(nvar);
	if (error)
		goto out;
	psref_target_init(&nvar->gv_psref, gv_psref_class);
	membar_producer();
	gif_update_variant(sc, nvar);

	mutex_exit(&sc->gif_lock);

	(void)gif_encap_detach(ovar);
	encap_lock_exit();

	if (osrc)
		sockaddr_free(osrc);
	if (odst)
		sockaddr_free(odst);
	kmem_free(ovar, sizeof(*ovar));

#ifndef GIF_MPSAFE
	splx(s);
#endif
	return 0;

 out:
	sockaddr_free(nsrc);
	sockaddr_free(ndst);
	kmem_free(nvar, sizeof(*nvar));

	mutex_exit(&sc->gif_lock);
	encap_lock_exit();
#ifndef GIF_MPSAFE
	splx(s);
#endif
	return error;
}

static void
gif_delete_tunnel(struct ifnet *ifp)
{
	struct gif_softc *sc = ifp->if_softc;
	struct gif_variant *ovar, *nvar;
	struct sockaddr *osrc, *odst;
	int error;
#ifndef GIF_MPSAFE
	int s;

	s = splsoftnet();
#endif
	error = encap_lock_enter();
	if (error) {
#ifndef GIF_MPSAFE
		splx(s);
#endif
		return;
	}

	nvar = kmem_alloc(sizeof(*nvar), KM_SLEEP);

	mutex_enter(&sc->gif_lock);

	ovar = sc->gif_var;
	osrc = ovar->gv_psrc;
	odst = ovar->gv_pdst;
	if (osrc == NULL || odst == NULL) {
		/* address pair not changed. */
		mutex_exit(&sc->gif_lock);
		encap_lock_exit();
		kmem_free(nvar, sizeof(*nvar));
		return;
	}

	*nvar = *ovar;
	nvar->gv_psrc = NULL;
	nvar->gv_pdst = NULL;
	nvar->gv_encap_cookie4 = NULL;
	nvar->gv_encap_cookie6 = NULL;
	nvar->gv_output = NULL;
	psref_target_init(&nvar->gv_psref, gv_psref_class);
	membar_producer();
	gif_update_variant(sc, nvar);

	mutex_exit(&sc->gif_lock);

	gif_encap_detach(ovar);
	encap_lock_exit();

	sockaddr_free(osrc);
	sockaddr_free(odst);
	kmem_free(ovar, sizeof(*ovar));

#ifndef GIF_MPSAFE
	splx(s);
#endif
}

/*
 * gif_variant update API.
 *
 * Assumption:
 * reader side dereferences sc->gif_var in reader critical section only,
 * that is, all of reader sides do not reader the sc->gif_var after
 * pserialize_perform().
 */
static void
gif_update_variant(struct gif_softc *sc, struct gif_variant *nvar)
{
	struct ifnet *ifp = &sc->gif_if;
	struct gif_variant *ovar = sc->gif_var;

	KASSERT(mutex_owned(&sc->gif_lock));

	sc->gif_var = nvar;
	pserialize_perform(gif_psz);
	psref_target_destroy(&ovar->gv_psref, gv_psref_class);

	if (nvar->gv_psrc != NULL && nvar->gv_pdst != NULL)
		ifp->if_flags |= IFF_RUNNING;
	else
		ifp->if_flags &= ~IFF_RUNNING;
}

/*
 * Module infrastructure
 */
#include "if_module.h"

IF_MODULE(MODULE_CLASS_DRIVER, gif, "")
