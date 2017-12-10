/*	$NetBSD: in6_l2tp.c,v 1.5.8.2 2017/12/10 09:41:32 snj Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: in6_l2tp.c,v 1.5.8.2 2017/12/10 09:41:32 snj Exp $");

#ifdef _KERNEL_OPT
#include "opt_l2tp.h"
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
#include <net/if_ether.h>

#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include <netinet/ip_var.h>
#include <netinet/ip_private.h>
#include <netinet/in_l2tp.h>
#include <netinet/in_var.h>
#include <netinet/ip_encap.h>

#include <netinet/ip6.h>
#include <netinet6/ip6_var.h>
#include <netinet6/in6_l2tp.h>

#ifdef ALTQ
#include <altq/altq.h>
#endif

/* TODO: IP_TCPMSS support */
#undef IP_TCPMSS
#ifdef IP_TCPMSS
#include <netinet/ip_tcpmss.h>
#endif

#include <net/if_l2tp.h>

#define L2TP_HLIM6		64
int ip6_l2tp_hlim = L2TP_HLIM6;

static int in6_l2tp_input(struct mbuf **, int *, int, void *);

static const struct encapsw in6_l2tp_encapsw = {
	.encapsw6 = {
		.pr_input	= in6_l2tp_input,
		.pr_ctlinput	= NULL,
	}
};

static int in6_l2tp_match(struct mbuf *, int, int, void *);

int
in6_l2tp_output(struct l2tp_variant *var, struct mbuf *m)
{
	struct rtentry *rt;
	struct l2tp_ro *lro;
	struct l2tp_softc *sc;
	struct ifnet *ifp;
	struct sockaddr_in6 *sin6_src = satosin6(var->lv_psrc);
	struct sockaddr_in6 *sin6_dst = satosin6(var->lv_pdst);
	struct ip6_hdr ip6hdr;	/* capsule IP header, host byte ordered */
	int error;
	uint32_t sess_id;

	KASSERT(var != NULL);
	KASSERT(l2tp_heldref_variant(var));
	KASSERT(sin6_src != NULL && sin6_dst != NULL);
	KASSERT(sin6_src->sin6_family == AF_INET6
	    && sin6_dst->sin6_family == AF_INET6);

	sc = var->lv_softc;
	ifp = &sc->l2tp_ec.ec_if;
	error = l2tp_check_nesting(ifp, m);
	if (error) {
		m_freem(m);
		goto looped;
	}

#ifdef NOTYET
/* TODO: support ALTQ for innner frame */
#ifdef ALTQ
	ALTQ_SAVE_PAYLOAD(m, AF_ETHER);
#endif
#endif

	memset(&ip6hdr, 0, sizeof(ip6hdr));
	ip6hdr.ip6_src = sin6_src->sin6_addr;
	/* bidirectional configured tunnel mode */
	if (!IN6_IS_ADDR_UNSPECIFIED(&sin6_dst->sin6_addr))
		ip6hdr.ip6_dst = sin6_dst->sin6_addr;
	else {
		m_freem(m);
		if ((ifp->if_flags & IFF_DEBUG) != 0)
			log(LOG_DEBUG, "%s: ENETUNREACH\n", __func__);
		return ENETUNREACH;
	}
	/* unlike IPv4, IP version must be filled by caller of ip6_output() */
	ip6hdr.ip6_vfc  = 0x60;
	ip6hdr.ip6_nxt  = IPPROTO_L2TP;
	ip6hdr.ip6_hlim = ip6_l2tp_hlim;
	/* outer IP payload length */
	ip6hdr.ip6_plen = 0;
	/* session-id length */
	ip6hdr.ip6_plen += sizeof(uint32_t);
	if (var->lv_use_cookie == L2TP_COOKIE_ON) {
		/* cookie length */
		ip6hdr.ip6_plen += var->lv_peer_cookie_len;
	}

/* TODO: IP_TCPMSS support */
#ifdef IP_TCPMSS
	m = l2tp_tcpmss_clamp(ifp, m);
	if (m == NULL)
		return EINVAL;
#endif

	/*
	 * payload length
	 *  NOTE: Payload length may be changed in ip_tcpmss().
	 *        Typical case is missing of TCP mss option in original
	 *        TCP header.
	 */
	ip6hdr.ip6_plen += m->m_pkthdr.len;
	HTONS(ip6hdr.ip6_plen);

	if (var->lv_use_cookie == L2TP_COOKIE_ON) {
		/* prepend session cookie */
		uint32_t cookie_32;
		uint64_t cookie_64;
		M_PREPEND(m, var->lv_peer_cookie_len, M_DONTWAIT);
		if (m && m->m_len < var->lv_peer_cookie_len)
			m = m_pullup(m, var->lv_peer_cookie_len);
		if (m == NULL)
			return ENOBUFS;
		if (var->lv_peer_cookie_len == 4) {
			cookie_32 = htonl((uint32_t)var->lv_peer_cookie);
			memcpy(mtod(m, void *), &cookie_32,
			    sizeof(uint32_t));
		} else {
			cookie_64 = htobe64(var->lv_peer_cookie);
			memcpy(mtod(m, void *), &cookie_64,
			    sizeof(uint64_t));
		}
	}

	/* prepend session-ID */
	sess_id = htonl(var->lv_peer_sess_id);
	M_PREPEND(m, sizeof(uint32_t), M_DONTWAIT);
	if (m && m->m_len < sizeof(uint32_t))
		m = m_pullup(m, sizeof(uint32_t));
	if (m == NULL)
		return ENOBUFS;
	memcpy(mtod(m, uint32_t *), &sess_id, sizeof(uint32_t));

	/* prepend new IP header */
	M_PREPEND(m, sizeof(struct ip6_hdr), M_DONTWAIT);
	if (IP_HDR_ALIGNED_P(mtod(m, void *)) == 0) {
		if (m)
			m = m_copyup(m, sizeof(struct ip), 0);
	} else {
		if (m && m->m_len < sizeof(struct ip6_hdr))
			m = m_pullup(m, sizeof(struct ip6_hdr));
	}
	if (m == NULL)
		return ENOBUFS;
	memcpy(mtod(m, struct ip6_hdr *), &ip6hdr, sizeof(struct ip6_hdr));

	lro = percpu_getref(sc->l2tp_ro_percpu);
	mutex_enter(&lro->lr_lock);
	if ((rt = rtcache_lookup(&lro->lr_ro, var->lv_pdst)) == NULL) {
		mutex_exit(&lro->lr_lock);
		percpu_putref(sc->l2tp_ro_percpu);
		m_freem(m);
		return ENETUNREACH;
	}

	/* If the route constitutes infinite encapsulation, punt. */
	if (rt->rt_ifp == ifp) {
		rtcache_unref(rt, &lro->lr_ro);
		rtcache_free(&lro->lr_ro);
		mutex_exit(&lro->lr_lock);
		percpu_putref(sc->l2tp_ro_percpu);
		m_freem(m);
		return ENETUNREACH;	/* XXX */
	}
	rtcache_unref(rt, &lro->lr_ro);

	/*
	 * To avoid inappropriate rewrite of checksum,
	 * clear csum flags.
	 */
	m->m_pkthdr.csum_flags  = 0;

	error = ip6_output(m, 0, &lro->lr_ro, 0, NULL, NULL, NULL);
	mutex_exit(&lro->lr_lock);
	percpu_putref(sc->l2tp_ro_percpu);
	return(error);

looped:
	if (error)
		ifp->if_oerrors++;

	return error;
}

static int
in6_l2tp_input(struct mbuf **mp, int *offp, int proto, void *eparg __unused)
{
	struct mbuf *m = *mp;
	int off = *offp;

	struct ifnet *l2tpp = NULL;
	struct l2tp_softc *sc;
	struct l2tp_variant *var;
	uint32_t sess_id;
	uint32_t cookie_32;
	uint64_t cookie_64;
	struct psref psref;

	if (m->m_len < off + sizeof(uint32_t)) {
		m = m_pullup(m, off + sizeof(uint32_t));
		if (!m) {
			/* if payload length < 4 octets */
			return IPPROTO_DONE;
		}
		*mp = m;
        }

	/* get L2TP session ID */
	m_copydata(m, off, sizeof(uint32_t), (void *)&sess_id);
	NTOHL(sess_id);
#ifdef L2TP_DEBUG
	log(LOG_DEBUG, "%s: sess_id = %" PRIu32 "\n", __func__, sess_id);
#endif
	if (sess_id == 0) {
		/*
		 * L2TPv3 control packet received.
		 * userland daemon(l2tpd?) should process.
		 */
		return rip6_input(mp, offp, proto);
	}

	var = l2tp_lookup_session_ref(sess_id, &psref);
	if (var == NULL) {
		m_freem(m);
		IP_STATINC(IP_STAT_NOL2TP);
		return IPPROTO_DONE;
	} else {
		sc = var->lv_softc;
		l2tpp = &(sc->l2tp_ec.ec_if);

		if (l2tpp == NULL || (l2tpp->if_flags & IFF_UP) == 0) {
#ifdef L2TP_DEBUG
			if (l2tpp == NULL)
				log(LOG_DEBUG, "%s: l2tpp is NULL\n", __func__);
			else
				log(LOG_DEBUG, "%s: l2tpp is down\n", __func__);
#endif
			m_freem(m);
			IP_STATINC(IP_STAT_NOL2TP);
			goto out;
		}
		/* other CPU do l2tp_delete_tunnel */
		if (var->lv_psrc == NULL || var->lv_pdst == NULL) {
			m_freem(m);
			ip_statinc(IP_STAT_NOL2TP);
			goto out;
		}
	}

	if (var->lv_state != L2TP_STATE_UP) {
		m_freem(m);
		goto out;
	}
	m_adj(m, off + sizeof(uint32_t));

	if (var->lv_use_cookie == L2TP_COOKIE_ON) {
		if (var->lv_my_cookie_len == 4) {
			m_copydata(m, 0, sizeof(uint32_t), (void *)&cookie_32);
			NTOHL(cookie_32);
			if (cookie_32 != var->lv_my_cookie) {
				m_freem(m);
				goto out;
			}
			m_adj(m, sizeof(uint32_t));
		} else {
			m_copydata(m, 0, sizeof(uint64_t), (void *)&cookie_64);
			BE64TOH(cookie_64);
			if (cookie_64 != var->lv_my_cookie) {
				m_freem(m);
				goto out;
			}
			m_adj(m, sizeof(uint64_t));
		}
	}

/* TODO: IP_TCPMSS support */
#ifdef IP_TCPMSS
	m = l2tp_tcpmss_clamp(l2tpp, m);
	if (m == NULL)
		goto out;
#endif
	l2tp_input(m, l2tpp);

out:
	l2tp_putref_variant(var, &psref);
	return IPPROTO_DONE;
}

/*
 * This function is used by encap6_lookup() to decide priority of the encaptab.
 * This priority is compared to the match length between mbuf's source/destination
 * IPv6 address pair and encaptab's one.
 * l2tp(4) does not use address pairs to search matched encaptab, so this
 * function must return the length bigger than or equals to IPv6 address pair to
 * avoid wrong encaptab.
 */
static int
in6_l2tp_match(struct mbuf *m, int off, int proto, void *arg)
{
	struct l2tp_variant *var = arg;
	uint32_t sess_id;

	KASSERT(proto == IPPROTO_L2TP);

	if (m->m_len < off + sizeof(uint32_t)) {
		m = m_pullup(m, off + sizeof(uint32_t));
		if (!m) {
			/* if payload length < 4 octets */
			return 0;
		}
        }

	/* get L2TP session ID */
	m_copydata(m, off, sizeof(uint32_t), (void *)&sess_id);
	NTOHL(sess_id);
	if (sess_id == 0) {
		/*
		 * L2TPv3 control packet received.
		 * userland daemon(l2tpd?) should process.
		 */
		return 128 * 2;
	} else if (sess_id == var->lv_my_sess_id)
		return 128 * 2;
	else
		return 0;
}

int
in6_l2tp_attach(struct l2tp_variant *var)
{

	var->lv_encap_cookie = encap_attach_func(AF_INET6, IPPROTO_L2TP,
	    in6_l2tp_match, &in6_l2tp_encapsw, var);
	if (var->lv_encap_cookie == NULL)
		return EEXIST;

	return 0;
}

int
in6_l2tp_detach(struct l2tp_variant *var)
{
	int error;

	error = encap_detach(var->lv_encap_cookie);
	if (error == 0)
		var->lv_encap_cookie = NULL;

	return error;
}
