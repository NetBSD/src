/*	$NetBSD: in_l2tp.c,v 1.2.8.5 2018/05/17 14:07:03 martin Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: in_l2tp.c,v 1.2.8.5 2018/05/17 14:07:03 martin Exp $");

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

#ifdef ALTQ
#include <altq/altq.h>
#endif

/* TODO: IP_TCPMSS support */
#undef IP_TCPMSS
#ifdef IP_TCPMSS
#include <netinet/ip_tcpmss.h>
#endif

#include <net/if_l2tp.h>

#include <net/net_osdep.h>

int ip_l2tp_ttl = L2TP_TTL;

static void in_l2tp_input(struct mbuf *, int, int, void *);

static const struct encapsw in_l2tp_encapsw = {
	.encapsw4 = {
		.pr_input	= in_l2tp_input,
		.pr_ctlinput	= NULL,
	}
};

static int in_l2tp_match(struct mbuf *, int, int, void *);

int
in_l2tp_output(struct l2tp_variant *var, struct mbuf *m)
{
	struct l2tp_softc *sc;
	struct ifnet *ifp;
	struct sockaddr_in *sin_src = satosin(var->lv_psrc);
	struct sockaddr_in *sin_dst = satosin(var->lv_pdst);
	struct ip iphdr;	/* capsule IP header, host byte ordered */
	struct rtentry *rt;
	struct l2tp_ro *lro;
	int error;
	uint32_t sess_id;

	KASSERT(var != NULL);
	KASSERT(l2tp_heldref_variant(var));
	KASSERT(sin_src != NULL && sin_dst != NULL);
	KASSERT(sin_src->sin_family == AF_INET
	    && sin_dst->sin_family == AF_INET);

	sc = var->lv_softc;
	ifp = &sc->l2tp_ec.ec_if;
	error = l2tp_check_nesting(ifp, m);
	if (error) {
		m_freem(m);
		goto looped;
	}

	/* bidirectional configured tunnel mode */
	if (sin_dst->sin_addr.s_addr == INADDR_ANY) {
		m_freem(m);
		if ((ifp->if_flags & IFF_DEBUG) != 0)
			log(LOG_DEBUG, "%s: ENETUNREACH\n", __func__);
		error = ENETUNREACH;
		goto out;
	}

#ifdef NOTYET
/* TODO: support ALTQ for innner frame */
#ifdef ALTQ
	ALTQ_SAVE_PAYLOAD(m, AF_ETHER);
#endif
#endif

	memset(&iphdr, 0, sizeof(iphdr));
	iphdr.ip_src = sin_src->sin_addr;
	iphdr.ip_dst = sin_dst->sin_addr;
	iphdr.ip_p = IPPROTO_L2TP;
	/* version will be set in ip_output() */
	iphdr.ip_ttl = ip_l2tp_ttl;
	/* outer IP header length */
	iphdr.ip_len = sizeof(struct ip);
	/* session-id length */
	iphdr.ip_len += sizeof(uint32_t);
	if (var->lv_use_cookie == L2TP_COOKIE_ON) {
		/* cookie length */
		iphdr.ip_len += var->lv_peer_cookie_len;
	}

/* TODO: IP_TCPMSS support */
#ifdef IP_TCPMSS
	m = l2tp_tcpmss_clamp(ifp, m);
	if (m == NULL) {
		error = EINVAL;
		goto out;
	}
#endif

	/*
	 * Payload length.
	 *
	 * NOTE: payload length may be changed in ip_tcpmss(). Typical case
	 * is missing of TCP mss option in original TCP header.
	 */
	iphdr.ip_len += m->m_pkthdr.len;
	HTONS(iphdr.ip_len);

	if (var->lv_use_cookie == L2TP_COOKIE_ON) {
		/* prepend session cookie */
		uint32_t cookie_32;
		uint64_t cookie_64;
		M_PREPEND(m, var->lv_peer_cookie_len, M_DONTWAIT);
		if (m && m->m_len < var->lv_peer_cookie_len)
			m = m_pullup(m, var->lv_peer_cookie_len);
		if (m == NULL) {
			error = ENOBUFS;
			goto out;
		}
		if (var->lv_peer_cookie_len == 4) {
			cookie_32 = htonl((uint32_t)var->lv_peer_cookie);
			memcpy(mtod(m, void *), &cookie_32, sizeof(uint32_t));
		} else {
			cookie_64 = htobe64(var->lv_peer_cookie);
			memcpy(mtod(m, void *), &cookie_64, sizeof(uint64_t));
		}
	}

	/* prepend session-ID */
	sess_id = htonl(var->lv_peer_sess_id);
	M_PREPEND(m, sizeof(uint32_t), M_DONTWAIT);
	if (m && m->m_len < sizeof(uint32_t))
		m = m_pullup(m, sizeof(uint32_t));
	if (m == NULL) {
		error = ENOBUFS;
		goto out;
	}
	memcpy(mtod(m, uint32_t *), &sess_id, sizeof(uint32_t));

	/* prepend new IP header */
	M_PREPEND(m, sizeof(struct ip), M_DONTWAIT);
	if (m == NULL) {
		error = ENOBUFS;
		goto out;
	}
	if (IP_HDR_ALIGNED_P(mtod(m, void *)) == 0) {
		m = m_copyup(m, sizeof(struct ip), 0);
	} else {
		if (m->m_len < sizeof(struct ip))
			m = m_pullup(m, sizeof(struct ip));
	}
	if (m == NULL) {
		error = ENOBUFS;
		goto out;
	}
	memcpy(mtod(m, struct ip *), &iphdr, sizeof(struct ip));

	lro = percpu_getref(sc->l2tp_ro_percpu);
	mutex_enter(lro->lr_lock);
	if ((rt = rtcache_lookup(&lro->lr_ro, var->lv_pdst)) == NULL) {
		mutex_exit(lro->lr_lock);
		percpu_putref(sc->l2tp_ro_percpu);
		m_freem(m);
		error = ENETUNREACH;
		goto out;
	}

	if (rt->rt_ifp == ifp) {
		rtcache_unref(rt, &lro->lr_ro);
		rtcache_free(&lro->lr_ro);
		mutex_exit(lro->lr_lock);
		percpu_putref(sc->l2tp_ro_percpu);
		m_freem(m);
		error = ENETUNREACH;	/*XXX*/
		goto out;
	}
	rtcache_unref(rt, &lro->lr_ro);

	/*
	 * To avoid inappropriate rewrite of checksum,
	 * clear csum flags.
	 */
	m->m_pkthdr.csum_flags  = 0;

	error = ip_output(m, NULL, &lro->lr_ro, 0, NULL, NULL);
	mutex_exit(lro->lr_lock);
	percpu_putref(sc->l2tp_ro_percpu);
	return error;

looped:
	if (error)
		ifp->if_oerrors++;

out:
	return error;
}

static void
in_l2tp_input(struct mbuf *m, int off, int proto, void *eparg __unused)
{
	struct ifnet *l2tpp = NULL;
	struct l2tp_softc *sc;
	uint32_t sess_id;
	uint32_t cookie_32;
	uint64_t cookie_64;
	struct psref psref;
	struct l2tp_variant *var;

	KASSERT((m->m_flags & M_PKTHDR) != 0);

	if (m->m_pkthdr.len < off + sizeof(uint32_t)) {
		m_freem(m);
		return;
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
		rip_input(m, off, proto);
		return;
	}

	var = l2tp_lookup_session_ref(sess_id, &psref);
	if (var == NULL) {
		m_freem(m);
		ip_statinc(IP_STAT_NOL2TP);
		return;
	}

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
		ip_statinc(IP_STAT_NOL2TP);
		goto out;
	}

	/* other CPU did l2tp_delete_tunnel */
	if (var->lv_psrc == NULL || var->lv_pdst == NULL) {
		m_freem(m);
		ip_statinc(IP_STAT_NOL2TP);
		goto out;
	}

	if (var->lv_state != L2TP_STATE_UP) {
		m_freem(m);
		goto out;
	}

	m_adj(m, off + sizeof(uint32_t));

	if (var->lv_use_cookie == L2TP_COOKIE_ON) {
		if (m->m_pkthdr.len < var->lv_my_cookie_len) {
			m_freem(m);
			goto out;
		}
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
	return;
}

/*
 * This function is used by encap4_lookup() to decide priority of the encaptab.
 * This priority is compared to the match length between mbuf's source/destination
 * IPv4 address pair and encaptab's one.
 * l2tp(4) does not use address pairs to search matched encaptab, so this
 * function must return the length bigger than or equals to IPv4 address pair to
 * avoid wrong encaptab.
 */
static int
in_l2tp_match(struct mbuf *m, int off, int proto, void *arg)
{
	struct l2tp_variant *var = arg;
	uint32_t sess_id;

	KASSERT(proto == IPPROTO_L2TP);

	/*
	 * If the packet contains no session ID it cannot match
	 */
	if (m_length(m) < off + sizeof(uint32_t))
		return 0;

	/* get L2TP session ID */
	m_copydata(m, off, sizeof(uint32_t), (void *)&sess_id);
	NTOHL(sess_id);
	if (sess_id == 0) {
		/*
		 * L2TPv3 control packet received.
		 * userland daemon(l2tpd?) should process.
		 */
		return 32 * 2;
	} else if (sess_id == var->lv_my_sess_id)
		return 32 * 2;
	else
		return 0;
}

int
in_l2tp_attach(struct l2tp_variant *var)
{

	var->lv_encap_cookie = encap_attach_func(AF_INET, IPPROTO_L2TP,
	    in_l2tp_match, &in_l2tp_encapsw, var);
	if (var->lv_encap_cookie == NULL)
		return EEXIST;

	return 0;
}

int
in_l2tp_detach(struct l2tp_variant *var)
{
	int error;

	error = encap_detach(var->lv_encap_cookie);
	if (error == 0)
		var->lv_encap_cookie = NULL;

	return error;
}
