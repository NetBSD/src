/*-
 * Copyright (c) 2025 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Emmanuel Nyarko.
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
 * NPF route extension.
 */

 #ifdef _KERNEL
#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: npf_ext_route.c,v 1.1 2026/04/08 00:33:07 joe Exp $");

#include <sys/param.h>
#include <sys/types.h>
#include <sys/module.h>

#include <sys/conf.h>
#include <sys/kmem.h>
#include <sys/mbuf.h>
#include <sys/mutex.h>
#include <sys/queue.h>
#include <sys/syslog.h>
#include <sys/proc.h>

#include <net/if.h>
#include <net/if_types.h>
#include <net/bpf.h>
#include <net/route.h>
#include <netinet/in.h>
#include <netinet/in_var.h>
#include <netinet/ip.h>
#include <netinet/ip_var.h>
#include <netinet/in_offload.h>
#include <netinet/ip6.h>
#include <netinet6/ip6_var.h>
#include <netinet6/scope6_var.h>
#include <netinet6/in6_offload.h>
#endif

#include "npf_impl.h"

NPF_EXT_MODULE(npf_ext_route, "");

#define        NPFEXT_ROUTE_VER                1

#define NPF_LOGADDR(buf, a) \
    snprintf(buf, sizeof(buf), "%u.%u.%u.%u", \
        ((a)->s_addr >> 24) & 0xFF, \
        ((a)->s_addr >> 16) & 0xFF, \
        ((a)->s_addr >> 8) & 0xFF, \
        (a)->s_addr & 0xFF)

static void *          npf_ext_route_id;

typedef struct {
    char            ifname[IFNAMSIZ];
} npf_ext_route_t;

static int
npf_route_ctor(npf_rproc_t *rp, const nvlist_t* params)
{
	npf_ext_route_t *meta;
	const char *ifname;

	meta = kmem_zalloc(sizeof(*meta), KM_SLEEP);
	ifname = nvlist_get_string(params, "route-interface");

	if (!ifname)
		return EINVAL;

	/* XXX use something like npf_ifmap */
	strlcpy(meta->ifname, ifname, IFNAMSIZ);
	npf_rproc_assign(rp, meta);
	return 0;
}

static void
npf_route_dtor(npf_rproc_t *rp, void *meta)
{
	kmem_free(meta, sizeof(npf_ext_route_t));
}

static void
npf_chcksum(struct ifnet *ifp1, struct mbuf *m0, struct ip *ip1, int *sw_csum)
{
	int hlen;
	struct mbuf *m = m0;
	struct ip *ip = ip1;
	struct ifnet *ifp = ifp1;
	int csum;

	/* NB: This code is copied from ip_output */
	hlen = ip->ip_hl << 2;
	ip->ip_sum = 0;
	m->m_pkthdr.csum_data |= hlen << 16;

	/* Maybe skip checksums on loopback interfaces. */
	if (IN_NEED_CHECKSUM(ifp, M_CSUM_IPv4)) {
		m->m_pkthdr.csum_flags |= M_CSUM_IPv4;
	}

	*sw_csum = m->m_pkthdr.csum_flags & ~ifp->if_csum_flags_tx;
	csum = *sw_csum;

	if ((m->m_pkthdr.csum_flags & M_CSUM_TSOv4) == 0) {
		/*
		 * Perform any checksums that the hardware can't do
		 * for us.
		 *
		 * XXX Does any hardware require the {th,uh}_sum
		 * XXX fields to be 0?
		 */
		if (csum & M_CSUM_IPv4) {
			KASSERT(IN_NEED_CHECKSUM(ifp, M_CSUM_IPv4));
			ip->ip_sum = in_cksum(m, hlen);
			m->m_pkthdr.csum_flags &= ~M_CSUM_IPv4;
		}
		if (csum & (M_CSUM_TCPv4|M_CSUM_UDPv4)) {

			if (IN_NEED_CHECKSUM(ifp,
				csum & (M_CSUM_TCPv4|M_CSUM_UDPv4))) {
				in_undefer_cksum_tcpudp(m);
			}
			m->m_pkthdr.csum_flags &=
			~(M_CSUM_TCPv4|M_CSUM_UDPv4);

		}
	}
}

static int
npf_fragment(npf_t *npf, struct ifnet *ifp, struct ip *ip1,
    struct mbuf **m0, struct sockaddr_in *dst)
{
	int error;
	struct ip *ip = ip1;
	struct mbuf *m = *m0;

	/*
	 * Too large for interface; fragment if possible.
	 * Must be able to put at least 8 bytes per fragment.
	 */
	if (ntohs(ip->ip_off) & IP_DF) {
		npf_stats_inc(npf, NPF_STAT_NOFRAGMENT);
		icmp_error(m, ICMP_UNREACH, ICMP_UNREACH_NEEDFRAG, 0,
			ifp->if_mtu);
		return EINVAL;
	}
	/*
		* We can't use HW checksumming if we're about to fragment the packet.
		*
		* XXX Some hardware can do this.
		*/
	if (m->m_pkthdr.csum_flags & (M_CSUM_TCPv4|M_CSUM_UDPv4)) {
		if (IN_NEED_CHECKSUM(ifp,
			m->m_pkthdr.csum_flags & (M_CSUM_TCPv4|M_CSUM_UDPv4))) {
			in_undefer_cksum_tcpudp(m);
		}
		m->m_pkthdr.csum_flags &= ~(M_CSUM_TCPv4|M_CSUM_UDPv4);
	}

	error = ip_fragment(m, ifp, ifp->if_mtu);
	if (error) {
		m = NULL;
		return error;
	}

	for (; m; m = *m0) {
		*m0 = m->m_nextpkt;
		m->m_nextpkt = NULL;
		if (error) {
			m_freem(m);
			continue;
		}

		KASSERT((m->m_pkthdr.csum_flags &
			(M_CSUM_UDPv4 | M_CSUM_TCPv4)) == 0);
		error = ip_if_output(ifp, m, sintocsa(dst), NULL);
	}

	if (error == 0) {
		npf_stats_inc(npf, NPF_STAT_FRAGMENTS);
	}

	return error;
}

/*
 * Ensure sending address is valid.
 * if the packet could be dropped without error (protocol dependent).
 */
static int
ip_ifaddrvalid(const struct in_ifaddr *ia)
{

	if (ia->ia_addr.sin_addr.s_addr == INADDR_ANY)
		return 0;

	if (ia->ia4_flags & IN_IFF_DUPLICATED)
		return EADDRINUSE;
	else if (ia->ia4_flags & (IN_IFF_TENTATIVE | IN_IFF_DETACHED))
		return EADDRNOTAVAIL;

	return 0;
}

/*
 * search for the source address structure to
 * maintain output statistics, and verify address
 * validity
 */
static int
npf_validate_saddr(npf_t * npf, struct ip *ip, struct ifnet* ifp)
{
	int error = 0;
	struct in_ifaddr *ia = NULL;
	union {
		struct sockaddr		sa;
		struct sockaddr_in	sin;
	} usrc;

	struct psref psref_ia;

	KASSERT(ia == NULL);
	sockaddr_in_init(&usrc.sin, &ip->ip_src, 0);
	ia = ifatoia(ifaof_ifpforaddr_psref(&usrc.sa, ifp, &psref_ia));

	/*
	 * Ensure we only send from a valid address.
	 * A NULL address is valid because the packet could be
	 * generated from a packet filter.
	 */
	if (ia != NULL &&
	    (error = ip_ifaddrvalid(ia)) != 0)
	{
		char buf[32];
		NPF_LOGADDR(buf, &ip->ip_src);
		log(LOG_ERR,
		    "refusing to send from invalid address %s (pid %d)\n",
		    buf, curproc->p_pid);

		npf_stats_inc(npf, NPF_STAT_NOREROUTE);
	}
	ia4_release(ia, &psref_ia);
	return error;
}

#if defined(INET6)
/* this code is copied from ip6_output*/
static void
npf_validate_s6addr(struct mbuf *m0, struct ifnet *ifp1, int *sw_csum)
{
	struct in6_ifaddr *ia6;
	struct mbuf *m = m0;
	struct ifnet *ifp = ifp1;
	struct ip6_hdr *ip6;
	int csum;
	int s;

	ip6 = mtod(m, struct ip6_hdr *);
	s = pserialize_read_enter();
	ia6 = in6_ifawithifp(ifp, &ip6->ip6_src);
	if (ia6) {
		/* Record statistics for this interface address. */
		ia6->ia_ifa.ifa_data.ifad_outbytes += m->m_pkthdr.len;
	}
	pserialize_read_exit(s);

	/* check sum */
	*sw_csum = m->m_pkthdr.csum_flags & ~ifp->if_csum_flags_tx;
	csum = *sw_csum;

	if ((csum & (M_CSUM_UDPv6|M_CSUM_TCPv6)) != 0) {
		if (IN6_NEED_CHECKSUM(ifp,
			csum & (M_CSUM_UDPv6|M_CSUM_TCPv6))) {
			in6_undefer_cksum_tcpudp(m);
		}
		m->m_pkthdr.csum_flags &= ~(M_CSUM_UDPv6|M_CSUM_TCPv6);
	}
}
#endif

/* main routing function for kernel module */
static bool
npf_route(npf_cache_t *npc, void *meta, const npf_match_info_t __unused *mi, int *decision)
{
	struct mbuf *m0 = nbuf_head_mbuf(npc->npc_nbuf);
	const npf_ext_route_t *route = meta;
	npf_t *npf = npf_getkernctx();
	struct ifnet *ifp;
	int error;
	int sw_csum;

	union {
		struct sockaddr_in v4;
		struct sockaddr_in6 v6;
	} dst;

	/* Skip, if already blocking.
	 * also when routing is applied to a stateful rule, incoming packets
	 * are routed since the procedure becomes attached to the connection
	 * and hence will not be desirable
	 */
	if (*decision == NPF_DECISION_BLOCK ||
    (mi->mi_di == PFIL_IN)) {
			return true;
	}

	/* global lock for interface lookup */
	KERNEL_LOCK(1, NULL);
	ifp = ifunit(route->ifname);
	if (ifp == NULL) {
			/* XXX: oops */
			goto bad;
	}

	if (npf_iscached(npc, NPC_IP6)) {
#if defined(INET6)
		struct ip6_hdr *ip6 = npc->npc_ip.v6;
		sockaddr_in6_init(&dst.v6, &ip6->ip6_dst, 0, 0, 0);

		if (IN6_IS_SCOPE_EMBEDDABLE(&dst.v6.sin6_addr)) {
			error = in6_setscope(&dst.v6.sin6_addr, ifp, NULL);
			if (error) {
				goto bad;
			}
		}

		npf_validate_s6addr(m0, ifp, &sw_csum);

		if (m0->m_pkthdr.len <= ifp->if_mtu) {
			if (__predict_false(sw_csum & M_CSUM_TSOv6)) {
				/*
				 * TSO6 is required by a packet, but disabled for
				 * the interface.
				 */
				error = ip6_tso_output(ifp, ifp, m0, &dst.v6, NULL);
			} else
				error = ip6_if_output(ifp, ifp, m0, &dst.v6, NULL);

			if (error) {
				goto bad;
			}

		} else {
			/* router not allowed to fragmenrt */
			npf_stats_inc(npf, NPF_STAT_NOFRAGMENT);
			icmp6_error(m0, ICMP6_PACKET_TOO_BIG, 0, ifp->if_mtu);
		}
#endif
	} else if (npf_iscached(npc, NPC_IP4)) {
		struct ip *ip = npc->npc_ip.v4;
		struct mbuf *m = m0;

		KASSERT(ip != NULL);
		KASSERT(m != NULL);

		/*
		 * NB: This code is copied from ip_output and re-arranged
		 * checks fragmentation, checksum and source address validity
		 */
		sockaddr_in_init(&dst.v4, &ip->ip_dst, 0);

		error = npf_validate_saddr(npf, ip, ifp);
		if (error)
			goto bad;

		if (ntohs(ip->ip_len) > ifp->if_mtu)
			goto fragment;

		npf_chcksum(ifp, m, ip, &sw_csum);

		/* Send it */
		if (__predict_false(sw_csum & M_CSUM_TSOv4)) {
			/*
			 * TSO4 is required by a packet, but disabled for
			 * the interface.
			 */
			error = ip_tso_output(ifp, m, sintocsa(&dst.v4), NULL);
		} else
			error = ip_if_output(ifp, m, sintocsa(&dst.v4), NULL);

		if (error) {
			goto bad;
		}
		goto done;

fragment:
		error = npf_fragment(npf, ifp, ip, &m0, &dst.v4);
		if (error) {
			goto bad;
		}
	}

/*
 * for routing procedures, we reverse the returns
 * because we need the kernel to stop processing the mbuf
 * after we leave the filtering context
 */
done:
	npf_stats_inc(npf, NPF_STAT_REROUTE);
	m0 = NULL;
	KERNEL_UNLOCK_ONE(NULL);
	return false;

bad:
	npf_stats_inc(npf, NPF_STAT_NOREROUTE);
	m_freem(m0);
	m0 = NULL;
	KERNEL_UNLOCK_ONE(NULL);
	return true;
}

__dso_public int
npf_ext_route_init(npf_t *npf)
{
	static const npf_ext_ops_t npf_route_ops = {
		.version        = NPFEXT_ROUTE_VER,
		.ctx            = NULL,
		.ctor           = npf_route_ctor,
		.dtor           = npf_route_dtor,
		.proc           = npf_route
	};
	npf_ext_route_id = npf_ext_register(npf, "route", &npf_route_ops);
	return npf_ext_route_id ? 0 : EEXIST;
}

__dso_public int
npf_ext_route_fini(npf_t *npf)
{
	return npf_ext_unregister(npf, npf_ext_route_id);
}

#ifdef _KERNEL
static int
npf_ext_route_modcmd(modcmd_t cmd, void *arg)
{
	npf_t *npf = npf_getkernctx();

	switch (cmd) {
	case MODULE_CMD_INIT:
		return npf_ext_route_init(npf);
	case MODULE_CMD_FINI:
		return npf_ext_route_fini(npf);
	case MODULE_CMD_AUTOUNLOAD:
		/* Allow auto-unload only if NPF permits it. */
		return npf_autounload_p() ? 0 : EBUSY;
	default:
		return ENOTTY;
	}
	return 0;
}
#endif
