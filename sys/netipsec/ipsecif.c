/*	$NetBSD: ipsecif.c,v 1.5 2018/03/13 03:05:12 knakahara Exp $  */

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
__KERNEL_RCSID(0, "$NetBSD: ipsecif.c,v 1.5 2018/03/13 03:05:12 knakahara Exp $");

#ifdef _KERNEL_OPT
#include "opt_inet.h"
#include "opt_ipsec.h"
#endif

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/socket.h>
#include <sys/sockio.h>
#include <sys/mbuf.h>
#include <sys/errno.h>
#include <sys/ioctl.h>
#include <sys/syslog.h>
#include <sys/kernel.h>

#include <net/if.h>
#include <net/route.h>

#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include <netinet/ip_var.h>
#include <netinet/in_var.h>
#include <netinet/ip_encap.h>
#include <netinet/ip_ecn.h>
#include <netinet/ip_private.h>
#include <netinet/udp.h>

#ifdef INET6
#include <netinet/ip6.h>
#include <netinet6/ip6_var.h>
#include <netinet6/ip6_private.h>
#include <netinet6/in6_var.h>
#include <netinet6/ip6protosw.h> /* for struct ip6ctlparam */
#include <netinet/ip_ecn.h>
#endif

#include <netipsec/key.h>
#include <netipsec/ipsecif.h>

#include <net/if_ipsec.h>

static void ipsecif4_input(struct mbuf *, int, int, void *);
static int ipsecif4_output(struct ipsec_variant *, int, struct mbuf *);
static int ipsecif4_filter4(const struct ip *, struct ipsec_variant *,
	struct ifnet *);

#ifdef INET6
static int ipsecif6_input(struct mbuf **, int *, int, void *);
static int ipsecif6_output(struct ipsec_variant *, int, struct mbuf *);
static int ipsecif6_filter6(const struct ip6_hdr *, struct ipsec_variant *,
	struct ifnet *);
#endif

static int ip_ipsec_ttl = IPSEC_TTL;
static int ip_ipsec_copy_tos = 0;
#ifdef INET6
static int ip6_ipsec_hlim = IPSEC_HLIM;
static int ip6_ipsec_pmtu = 0; /* XXX: per interface configuration?? */
static int ip6_ipsec_copy_tos = 0;
#endif

struct encapsw ipsecif4_encapsw = {
	.encapsw4 = {
		.pr_input = ipsecif4_input,
		.pr_ctlinput = NULL,
	}
};

#ifdef INET6
static const struct encapsw ipsecif6_encapsw;
#endif

static struct mbuf *
ipsecif4_prepend_hdr(struct ipsec_variant *var, struct mbuf *m,
    uint8_t proto, uint8_t tos)
{
	struct ip *ip;
	struct sockaddr_in *src, *dst;

	src = satosin(var->iv_psrc);
	dst = satosin(var->iv_pdst);

	if (in_nullhost(src->sin_addr) || in_nullhost(src->sin_addr) ||
	    src->sin_addr.s_addr == INADDR_BROADCAST ||
	    dst->sin_addr.s_addr == INADDR_BROADCAST) {
		m_freem(m);
		return NULL;
	}
	m->m_flags &= ~M_BCAST;

	if (IN_MULTICAST(src->sin_addr.s_addr) ||
	    IN_MULTICAST(dst->sin_addr.s_addr)) {
		m_freem(m);
		return NULL;
	}

	M_PREPEND(m, sizeof(struct ip), M_DONTWAIT);
	if (m && M_UNWRITABLE(m, sizeof(struct ip)))
		m = m_pullup(m, sizeof(struct ip));
	if (m == NULL)
		return NULL;

	ip = mtod(m, struct ip *);
	ip->ip_v = IPVERSION;
	ip->ip_off = htons(0);
	ip->ip_id = 0;
	ip->ip_hl = sizeof(*ip) >> 2;
	if (ip_ipsec_copy_tos)
		ip->ip_tos = tos;
	else
		ip->ip_tos = 0;
	ip->ip_sum = 0;
	ip->ip_src = src->sin_addr;
	ip->ip_dst = dst->sin_addr;
	ip->ip_p = proto;
	ip->ip_ttl = ip_ipsec_ttl;
	ip->ip_len = htons(m->m_pkthdr.len);
#ifndef IPSEC_TX_TOS_CLEAR
	struct ifnet *ifp = &var->iv_softc->ipsec_if;
	if (ifp->if_flags & IFF_ECN)
		ip_ecn_ingress(ECN_ALLOWED, &ip->ip_tos, &tos);
	else
		ip_ecn_ingress(ECN_NOCARE, &ip->ip_tos, &tos);
#endif

	return m;
}

static int
ipsecif4_needfrag(struct mbuf *m, struct ipsecrequest *isr)
{
	struct ip ip0;
	struct ip *ip;
	int mtu;
	struct secasvar *sav;

	sav = key_lookup_sa_bysaidx(&isr->saidx);
	if (sav == NULL)
		return 0;

	if (!(sav->natt_type & UDP_ENCAP_ESPINUDP) &&
	    !(sav->natt_type & UDP_ENCAP_ESPINUDP_NON_IKE)) {
		mtu = 0;
		goto out;
	}

	if (m->m_len < sizeof(struct ip)) {
		m_copydata(m, 0, sizeof(ip0), &ip0);
		ip = &ip0;
	} else {
		ip = mtod(m, struct ip *);
	}
	mtu = sav->esp_frag;
	if (ntohs(ip->ip_len) <= mtu)
		mtu = 0;

out:
	KEY_SA_UNREF(&sav);
	return mtu;
}

static struct mbuf *
ipsecif4_flowinfo(struct mbuf *m, int family, int *proto0, u_int8_t *tos0)
{
	const struct ip *ip;
	int proto;
	int tos;

	KASSERT(proto0 != NULL);
	KASSERT(tos0 != NULL);

	switch (family) {
	case AF_INET:
		proto = IPPROTO_IPV4;
		if (m->m_len < sizeof(*ip)) {
			m = m_pullup(m, sizeof(*ip));
			if (m == NULL) {
				*tos0 = 0;
				*proto0 = 0;
				return NULL;
			}
		}
		ip = mtod(m, const struct ip *);
		tos = ip->ip_tos;
		/* TODO: support ALTQ for innner packet */
		break;
#ifdef INET6
	case AF_INET6: {
		const struct ip6_hdr *ip6;
		proto = IPPROTO_IPV6;
		if (m->m_len < sizeof(*ip6)) {
			m = m_pullup(m, sizeof(*ip6));
			if (m == NULL) {
				*tos0 = 0;
				*proto0 = 0;
				return NULL;
			}
		}
		ip6 = mtod(m, const struct ip6_hdr *);
		tos = (ntohl(ip6->ip6_flow) >> 20) & 0xff;
		/* TODO: support ALTQ for innner packet */
		break;
	}
#endif /* INET6 */
	default:
		*tos0 = 0;
		*proto0 = 0;
		return NULL;
	}

	*proto0 = proto;
	*tos0 = tos;
	return m;
}

static int
ipsecif4_fragout(struct ipsec_variant *var, int family, struct mbuf *m, int mtu)
{
	struct ifnet *ifp = &var->iv_softc->ipsec_if;
	struct mbuf *next;
	struct m_tag *mtag;
	int error;

	KASSERT(if_ipsec_heldref_variant(var));

	mtag = m_tag_find(m, PACKET_TAG_IPSEC_NAT_T_PORTS, NULL);
	if (mtag)
		m_tag_delete(m, mtag);

	/* consider new IP header prepended in ipsecif4_output() */
	if (mtu <= sizeof(struct ip)) {
		m_freem(m);
		return ENETUNREACH;
	}
	m->m_pkthdr.csum_flags |= M_CSUM_IPv4;
	error = ip_fragment(m, ifp, mtu - sizeof(struct ip));
	if (error)
		return error;

	for (error = 0; m; m = next) {
		next = m->m_nextpkt;
		m->m_nextpkt = NULL;
		if (error) {
			m_freem(m);
			continue;
		}

		error = ipsecif4_output(var, family, m);
	}
	if (error == 0)
		IP_STATINC(IP_STAT_FRAGMENTED);

	return error;
}

int
ipsecif4_encap_func(struct mbuf *m, struct ip *ip, struct ipsec_variant *var)
{
	struct m_tag *mtag;
	struct sockaddr_in *src, *dst;
	u_int16_t src_port = 0;
	u_int16_t dst_port = 0;

	KASSERT(var != NULL);

	src = satosin(var->iv_psrc);
	dst = satosin(var->iv_pdst);
	mtag = m_tag_find(m, PACKET_TAG_IPSEC_NAT_T_PORTS, NULL);
	if (mtag) {
		u_int16_t *ports;

		ports = (u_int16_t *)(mtag + 1);
		src_port = ports[0];
		dst_port = ports[1];
	}

	/* address match */
	if (src->sin_addr.s_addr != ip->ip_dst.s_addr ||
	    dst->sin_addr.s_addr != ip->ip_src.s_addr)
		return 0;

	/* UDP encap? */
	if (mtag == NULL && var->iv_sport == 0 && var->iv_dport == 0)
		goto match;

	/* port match */
	if (src_port != var->iv_dport ||
	    dst_port != var->iv_sport) {
#ifdef DEBUG
		printf("%s: port mismatch: pkt(%u, %u), if(%u, %u)\n",
		    __func__, ntohs(src_port), ntohs(dst_port),
		    ntohs(var->iv_sport), ntohs(var->iv_dport));
#endif
		return 0;
	}

match:
	/*
	 * hide NAT-T information from encapsulated traffics.
	 * they don't know about IPsec.
	 */
	if (mtag)
		m_tag_delete(m, mtag);
	return sizeof(src->sin_addr) + sizeof(dst->sin_addr);
}

static int
ipsecif4_output(struct ipsec_variant *var, int family, struct mbuf *m)
{
	struct secpolicy *sp = NULL;
	u_int8_t tos;
	int proto;
	int error;
	int mtu;
	u_long sa_mtu = 0;

	KASSERT(if_ipsec_heldref_variant(var));
	KASSERT(if_ipsec_variant_is_configured(var));
	KASSERT(var->iv_psrc->sa_family == AF_INET);
	KASSERT(var->iv_pdst->sa_family == AF_INET);

	sp = IV_SP_OUT(var);
	KASSERT(sp != NULL);
	/*
	 * The SPs in ipsec_variant are prevented from freed by
	 * ipsec_variant->iv_psref. So, KEY_SP_REF() is unnecessary here.
	 */

	KASSERT(sp->policy != IPSEC_POLICY_NONE);
	KASSERT(sp->policy != IPSEC_POLICY_ENTRUST);
	KASSERT(sp->policy != IPSEC_POLICY_BYPASS);
	if (sp->policy != IPSEC_POLICY_IPSEC) {
		struct ifnet *ifp = &var->iv_softc->ipsec_if;
		m_freem(m);
		IF_DROP(&ifp->if_snd);
		return 0;
	}

	/* get flowinfo */
	m = ipsecif4_flowinfo(m, family, &proto, &tos);
	if (m == NULL) {
		error = ENETUNREACH;
		goto done;
	}

	/* prepend new IP header */
	m = ipsecif4_prepend_hdr(var, m, proto, tos);
	if (m == NULL) {
		error = ENETUNREACH;
		goto done;
	}

	/*
	 * Normal netipsec's NAT-T fragmentation is done in ip_output().
	 * See "natt_frag" processing.
	 * However, ipsec(4) interface's one is not done in the same way,
	 * so we must do NAT-T fragmentation by own code.
	 */
	/* NAT-T ESP fragmentation */
	mtu = ipsecif4_needfrag(m, sp->req);
	if (mtu > 0)
		return ipsecif4_fragout(var, family, m, mtu);

	/* IPsec output */
	IP_STATINC(IP_STAT_LOCALOUT);
	error = ipsec4_process_packet(m, sp->req, &sa_mtu);
	if (error == ENOENT)
		error = 0;
	/*
	 * frangmentation is already done in ipsecif4_fragout(),
	 * so ipsec4_process_packet() must not do fragmentation here.
	 */
	KASSERT(sa_mtu == 0);

done:
	return error;
}

#ifdef INET6
static int
ipsecif6_output(struct ipsec_variant *var, int family, struct mbuf *m)
{
	struct ifnet *ifp = &var->iv_softc->ipsec_if;
	struct ipsec_softc *sc = ifp->if_softc;
	struct ipsec_ro *iro;
	struct rtentry *rt;
	struct sockaddr_in6 *sin6_src;
	struct sockaddr_in6 *sin6_dst;
	struct ip6_hdr *ip6;
	int proto, error;
	u_int8_t itos, otos;
	union {
		struct sockaddr		dst;
		struct sockaddr_in6	dst6;
	} u;

	KASSERT(if_ipsec_heldref_variant(var));
	KASSERT(if_ipsec_variant_is_configured(var));

	sin6_src = satosin6(var->iv_psrc);
	sin6_dst = satosin6(var->iv_pdst);

	KASSERT(sin6_src->sin6_family == AF_INET6);
	KASSERT(sin6_dst->sin6_family == AF_INET6);

	switch (family) {
#ifdef INET
	case AF_INET:
	    {
		struct ip *ip;

		proto = IPPROTO_IPV4;
		if (m->m_len < sizeof(*ip)) {
			m = m_pullup(m, sizeof(*ip));
			if (m == NULL)
				return ENOBUFS;
		}
		ip = mtod(m, struct ip *);
		itos = ip->ip_tos;
		/* TODO: support ALTQ for innner packet */
		break;
	    }
#endif /* INET */
	case AF_INET6:
	    {
		struct ip6_hdr *xip6;
		proto = IPPROTO_IPV6;
		if (m->m_len < sizeof(*xip6)) {
			m = m_pullup(m, sizeof(*xip6));
			if (m == NULL)
				return ENOBUFS;
		}
		xip6 = mtod(m, struct ip6_hdr *);
		itos = (ntohl(xip6->ip6_flow) >> 20) & 0xff;
		/* TODO: support ALTQ for innner packet */
		break;
	    }
	default:
		m_freem(m);
		return EAFNOSUPPORT;
	}

	/* prepend new IP header */
	M_PREPEND(m, sizeof(struct ip6_hdr), M_DONTWAIT);
	if (m && M_UNWRITABLE(m, sizeof(struct ip6_hdr)))
		m = m_pullup(m, sizeof(struct ip6_hdr));
	if (m == NULL)
		return ENOBUFS;

	ip6 = mtod(m, struct ip6_hdr *);
	ip6->ip6_flow	= 0;
	ip6->ip6_vfc	&= ~IPV6_VERSION_MASK;
	ip6->ip6_vfc	|= IPV6_VERSION;
#if 0	/* ip6->ip6_plen will be filled by ip6_output */
	ip6->ip6_plen	= htons((u_short)m->m_pkthdr.len - sizeof(*ip6));
#endif
	ip6->ip6_nxt	= proto;
	ip6->ip6_hlim	= ip6_ipsec_hlim;
	ip6->ip6_src	= sin6_src->sin6_addr;
	/* bidirectional configured tunnel mode */
	if (!IN6_IS_ADDR_UNSPECIFIED(&sin6_dst->sin6_addr)) {
		ip6->ip6_dst = sin6_dst->sin6_addr;
	} else  {
		m_freem(m);
		return ENETUNREACH;
	}
#ifndef IPSEC_TX_TOS_CLEAR
	if (ifp->if_flags & IFF_ECN)
		ip_ecn_ingress(ECN_ALLOWED, &otos, &itos);
	else
		ip_ecn_ingress(ECN_NOCARE, &otos, &itos);

	if (!ip6_ipsec_copy_tos)
		otos = 0;
#else
	if (ip6_ipsec_copy_tos)
		otos = itos;
	else
		otos = 0;
#endif
	ip6->ip6_flow &= ~ntohl(0xff00000);
	ip6->ip6_flow |= htonl((u_int32_t)otos << 20);

	sockaddr_in6_init(&u.dst6, &sin6_dst->sin6_addr, 0, 0, 0);

	iro = percpu_getref(sc->ipsec_ro_percpu);
	mutex_enter(&iro->ir_lock);
	if ((rt = rtcache_lookup(&iro->ir_ro, &u.dst)) == NULL) {
		mutex_exit(&iro->ir_lock);
		percpu_putref(sc->ipsec_ro_percpu);
		m_freem(m);
		return ENETUNREACH;
	}

	if (rt->rt_ifp == ifp) {
		rtcache_unref(rt, &iro->ir_ro);
		rtcache_free(&iro->ir_ro);
		mutex_exit(&iro->ir_lock);
		percpu_putref(sc->ipsec_ro_percpu);
		m_freem(m);
		return ENETUNREACH;
	}
	rtcache_unref(rt, &iro->ir_ro);

	/*
	 * force fragmentation to minimum MTU, to avoid path MTU discovery.
	 * it is too painful to ask for resend of inner packet, to achieve
	 * path MTU discovery for encapsulated packets.
	 */
	error = ip6_output(m, 0, &iro->ir_ro,
	    ip6_ipsec_pmtu ? 0 : IPV6_MINMTU, 0, NULL, NULL);
	if (error)
		rtcache_free(&iro->ir_ro);

	mutex_exit(&iro->ir_lock);
	percpu_putref(sc->ipsec_ro_percpu);

	return error;
}
#endif /* INET6 */

static void
ipsecif4_input(struct mbuf *m, int off, int proto, void *eparg)
{
	struct ifnet *ipsecp;
	struct ipsec_softc *sc = eparg;
	struct ipsec_variant *var;
	const struct ip *ip;
	int af;
#ifndef IPSEC_TX_TOS_CLEAR
	u_int8_t otos;
#endif
	struct psref psref_rcvif;
	struct psref psref_var;
	struct ifnet *rcvif;

	KASSERT(sc != NULL);

	ipsecp = &sc->ipsec_if;
	if ((ipsecp->if_flags & IFF_UP) == 0) {
		m_freem(m);
		ip_statinc(IP_STAT_NOIPSEC);
		return;
	}

	var = if_ipsec_getref_variant(sc, &psref_var);
	if (if_ipsec_variant_is_unconfigured(var)) {
		if_ipsec_putref_variant(var, &psref_var);
		m_freem(m);
		ip_statinc(IP_STAT_NOIPSEC);
		return;
	}

	ip = mtod(m, const struct ip *);

	rcvif = m_get_rcvif_psref(m, &psref_rcvif);
	if (rcvif == NULL || !ipsecif4_filter4(ip, var, rcvif)) {
		m_put_rcvif_psref(rcvif, &psref_rcvif);
		if_ipsec_putref_variant(var, &psref_var);
		m_freem(m);
		ip_statinc(IP_STAT_NOIPSEC);
		return;
	}
	m_put_rcvif_psref(rcvif, &psref_rcvif);
	if_ipsec_putref_variant(var, &psref_var);
#ifndef IPSEC_TX_TOS_CLEAR
	otos = ip->ip_tos;
#endif
	m_adj(m, off);

	switch (proto) {
	case IPPROTO_IPV4:
	    {
		struct ip *xip;
		af = AF_INET;
		if (M_UNWRITABLE(m, sizeof(*xip))) {
			m = m_pullup(m, sizeof(*xip));
			if (m == NULL)
				return;
		}
		xip = mtod(m, struct ip *);
#ifndef IPSEC_TX_TOS_CLEAR
		if (ipsecp->if_flags & IFF_ECN)
			ip_ecn_egress(ECN_ALLOWED, &otos, &xip->ip_tos);
		else
			ip_ecn_egress(ECN_NOCARE, &otos, &xip->ip_tos);
#endif
		break;
	    }
#ifdef INET6
	case IPPROTO_IPV6:
	    {
		struct ip6_hdr *ip6;
		u_int8_t itos;
		af = AF_INET6;
		if (M_UNWRITABLE(m, sizeof(*ip6))) {
			m = m_pullup(m, sizeof(*ip6));
			if (m == NULL)
				return;
		}
		ip6 = mtod(m, struct ip6_hdr *);
		itos = (ntohl(ip6->ip6_flow) >> 20) & 0xff;
#ifndef IPSEC_TX_TOS_CLEAR
		if (ipsecp->if_flags & IFF_ECN)
			ip_ecn_egress(ECN_ALLOWED, &otos, &itos);
		else
			ip_ecn_egress(ECN_NOCARE, &otos, &itos);
#endif
		ip6->ip6_flow &= ~htonl(0xff << 20);
		ip6->ip6_flow |= htonl((u_int32_t)itos << 20);
		break;
	    }
#endif /* INET6 */
	default:
		ip_statinc(IP_STAT_NOIPSEC);
		m_freem(m);
		return;
	}
	if_ipsec_input(m, af, ipsecp);

	return;
}

/*
 * validate and filter the pakcet
 */
static int
ipsecif4_filter4(const struct ip *ip, struct ipsec_variant *var,
    struct ifnet *ifp)
{
	struct sockaddr_in *src, *dst;

	src = satosin(var->iv_psrc);
	dst = satosin(var->iv_pdst);

	return in_tunnel_validate(ip, src->sin_addr, dst->sin_addr);
}

#ifdef INET6
static int
ipsecif6_input(struct mbuf **mp, int *offp, int proto, void *eparg)
{
	struct mbuf *m = *mp;
	struct ifnet *ipsecp;
	struct ipsec_softc *sc = eparg;
	struct ipsec_variant *var;
	struct ip6_hdr *ip6;
	int af = 0;
#ifndef IPSEC_TX_TOS_CLEAR
	u_int32_t otos;
#endif
	struct psref psref_rcvif;
	struct psref psref_var;
	struct ifnet *rcvif;

	KASSERT(eparg != NULL);

	ipsecp = &sc->ipsec_if;
	if ((ipsecp->if_flags & IFF_UP) == 0) {
		m_freem(m);
		IP6_STATINC(IP6_STAT_NOIPSEC);
		return IPPROTO_DONE;
	}

	var = if_ipsec_getref_variant(sc, &psref_var);
	if (if_ipsec_variant_is_unconfigured(var)) {
		if_ipsec_putref_variant(var, &psref_var);
		m_freem(m);
		IP6_STATINC(IP6_STAT_NOIPSEC);
		return IPPROTO_DONE;
	}

	ip6 = mtod(m, struct ip6_hdr *);

	rcvif = m_get_rcvif_psref(m, &psref_rcvif);
	if (rcvif == NULL || !ipsecif6_filter6(ip6, var, rcvif)) {
		m_put_rcvif_psref(rcvif, &psref_rcvif);
		if_ipsec_putref_variant(var, &psref_var);
		m_freem(m);
		IP6_STATINC(IP6_STAT_NOIPSEC);
		return IPPROTO_DONE;
	}
	m_put_rcvif_psref(rcvif, &psref_rcvif);
	if_ipsec_putref_variant(var, &psref_var);

#ifndef IPSEC_TX_TOS_CLEAR
	otos = ip6->ip6_flow;
#endif
	m_adj(m, *offp);

	switch (proto) {
#ifdef INET
	case IPPROTO_IPV4:
	    {
		af = AF_INET;
#ifndef IPSEC_TX_TOS_CLEAR
		struct ip *ip;
		u_int8_t otos8;
		otos8 = (ntohl(otos) >> 20) & 0xff;

		if (M_UNWRITABLE(m, sizeof(*ip))) {
			m = m_pullup(m, sizeof(*ip));
			if (m == NULL)
				return IPPROTO_DONE;
		}
		ip = mtod(m, struct ip *);
		if (ipsecp->if_flags & IFF_ECN)
			ip_ecn_egress(ECN_ALLOWED, &otos8, &ip->ip_tos);
		else
			ip_ecn_egress(ECN_NOCARE, &otos8, &ip->ip_tos);
#endif
		break;
	    }
#endif /* INET */
	case IPPROTO_IPV6:
	    {
		af = AF_INET6;
#ifndef IPSEC_TX_TOS_CLEAR
		struct ip6_hdr *xip6;

		if (M_UNWRITABLE(m, sizeof(*xip6))) {
			m = m_pullup(m, sizeof(*xip6));
			if (m == NULL)
				return IPPROTO_DONE;
		}
		xip6 = mtod(m, struct ip6_hdr *);
		if (ipsecp->if_flags & IFF_ECN)
			ip6_ecn_egress(ECN_ALLOWED, &otos, &xip6->ip6_flow);
		else
			ip6_ecn_egress(ECN_NOCARE, &otos, &xip6->ip6_flow);
		break;
#endif
	    }
	default:
		IP6_STATINC(IP6_STAT_NOIPSEC);
		m_freem(m);
		return IPPROTO_DONE;
	}

	if_ipsec_input(m, af, ipsecp);
	return IPPROTO_DONE;
}

/*
 * validate and filter the packet.
 */
static int
ipsecif6_filter6(const struct ip6_hdr *ip6, struct ipsec_variant *var,
    struct ifnet *ifp)
{
	struct sockaddr_in6 *src, *dst;

	src = satosin6(var->iv_psrc);
	dst = satosin6(var->iv_pdst);

	return in6_tunnel_validate(ip6, &src->sin6_addr, &dst->sin6_addr);
}
#endif /* INET6 */

int
ipsecif4_attach(struct ipsec_variant *var)
{
	struct ipsec_softc *sc = var->iv_softc;

	KASSERT(if_ipsec_variant_is_configured(var));

	if (var->iv_encap_cookie4 != NULL)
		return EALREADY;
	var->iv_encap_cookie4 = encap_attach_func(AF_INET, -1, if_ipsec_encap_func,
	    &ipsecif4_encapsw, sc);
	if (var->iv_encap_cookie4 == NULL)
		return EEXIST;

	var->iv_output = ipsecif4_output;
	return 0;
}

int
ipsecif4_detach(struct ipsec_variant *var)
{
	int error;

	if (var->iv_encap_cookie4 == NULL)
		return 0;

	var->iv_output = NULL;
	error = encap_detach(var->iv_encap_cookie4);
	if (error == 0)
		var->iv_encap_cookie4 = NULL;

	return error;
}

#ifdef INET6
int
ipsecif6_attach(struct ipsec_variant *var)
{
	struct sockaddr_in6 mask6;
	struct ipsec_softc *sc = var->iv_softc;

	KASSERT(if_ipsec_variant_is_configured(var));
	KASSERT(var->iv_encap_cookie6 == NULL);

	memset(&mask6, 0, sizeof(mask6));
	mask6.sin6_len = sizeof(struct sockaddr_in6);
	mask6.sin6_addr.s6_addr32[0] = mask6.sin6_addr.s6_addr32[1] =
	mask6.sin6_addr.s6_addr32[2] = mask6.sin6_addr.s6_addr32[3] = ~0;

	var->iv_encap_cookie6 = encap_attach(AF_INET6, -1,
	    var->iv_psrc, (struct sockaddr *)&mask6,
	    var->iv_pdst, (struct sockaddr *)&mask6,
	    &ipsecif6_encapsw, sc);
	if (var->iv_encap_cookie6 == NULL)
		return EEXIST;

	var->iv_output = ipsecif6_output;
	return 0;
}

static void
ipsecif6_rtcache_free_pc(void *p, void *arg __unused, struct cpu_info *ci __unused)
{
	struct ipsec_ro *iro = p;

	mutex_enter(&iro->ir_lock);
	rtcache_free(&iro->ir_ro);
	mutex_exit(&iro->ir_lock);
}

int
ipsecif6_detach(struct ipsec_variant *var)
{
	struct ipsec_softc *sc = var->iv_softc;
	int error;

	KASSERT(var->iv_encap_cookie6 != NULL);

	percpu_foreach(sc->ipsec_ro_percpu, ipsecif6_rtcache_free_pc, NULL);

	var->iv_output = NULL;
	error = encap_detach(var->iv_encap_cookie6);
	if (error == 0)
		var->iv_encap_cookie6 = NULL;
	return error;
}

void *
ipsecif6_ctlinput(int cmd, const struct sockaddr *sa, void *d, void *eparg)
{
	struct ipsec_softc *sc = eparg;
	struct ip6ctlparam *ip6cp = NULL;
	struct ip6_hdr *ip6;
	const struct sockaddr_in6 *dst6;
	struct ipsec_ro *iro;

	if (sa->sa_family != AF_INET6 ||
	    sa->sa_len != sizeof(struct sockaddr_in6))
		return NULL;

	if ((unsigned)cmd >= PRC_NCMDS)
		return NULL;
	if (cmd == PRC_HOSTDEAD)
		d = NULL;
	else if (inet6ctlerrmap[cmd] == 0)
		return NULL;

	/* if the parameter is from icmp6, decode it. */
	if (d != NULL) {
		ip6cp = (struct ip6ctlparam *)d;
		ip6 = ip6cp->ip6c_ip6;
	} else {
		ip6 = NULL;
	}

	if (!ip6)
		return NULL;

	iro = percpu_getref(sc->ipsec_ro_percpu);
	mutex_enter(&iro->ir_lock);
	dst6 = satocsin6(rtcache_getdst(&iro->ir_ro));
	/* XXX scope */
	if (dst6 == NULL)
		;
	else if (IN6_ARE_ADDR_EQUAL(&ip6->ip6_dst, &dst6->sin6_addr))
		/* flush route cache */
		rtcache_free(&iro->ir_ro);

	mutex_exit(&iro->ir_lock);
	percpu_putref(sc->ipsec_ro_percpu);

	return NULL;
}

ENCAP_PR_WRAP_CTLINPUT(ipsecif6_ctlinput)
#define	ipsecif6_ctlinput	ipsecif6_ctlinput_wrapper

static const struct encapsw ipsecif6_encapsw = {
	.encapsw6 = {
		.pr_input = ipsecif6_input,
		.pr_ctlinput = ipsecif6_ctlinput,
	}
};
#endif /* INET6 */
