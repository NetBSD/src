/*	$NetBSD: ipsec_output.c,v 1.66 2018/02/08 20:57:41 maxv Exp $	*/

/*-
 * Copyright (c) 2002, 2003 Sam Leffler, Errno Consulting
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * $FreeBSD: /repoman/r/ncvs/src/sys/netipsec/ipsec_output.c,v 1.3.2.2 2003/03/28 20:32:53 sam Exp $
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: ipsec_output.c,v 1.66 2018/02/08 20:57:41 maxv Exp $");

/*
 * IPsec output processing.
 */
#if defined(_KERNEL_OPT)
#include "opt_inet.h"
#include "opt_net_mpsafe.h"
#endif

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/mbuf.h>
#include <sys/domain.h>
#include <sys/protosw.h>
#include <sys/socket.h>
#include <sys/errno.h>
#include <sys/syslog.h>

#include <net/if.h>
#include <net/route.h>

#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include <netinet/ip_var.h>
#include <netinet/in_var.h>
#include <netinet/ip_ecn.h>

#include <netinet/ip6.h>
#ifdef INET6
#include <netinet6/ip6_var.h>
#endif
#include <netinet/in_pcb.h>
#ifdef INET6
#include <netinet/icmp6.h>
#endif
#include <netinet/udp.h>

#include <netipsec/ipsec.h>
#include <netipsec/ipsec_var.h>
#include <netipsec/ipsec_private.h>
#ifdef INET6
#include <netipsec/ipsec6.h>
#endif
#include <netipsec/ah_var.h>
#include <netipsec/esp_var.h>
#include <netipsec/ipcomp_var.h>

#include <netipsec/xform.h>

#include <netipsec/key.h>
#include <netipsec/keydb.h>
#include <netipsec/key_debug.h>

static percpu_t *ipsec_rtcache_percpu __cacheline_aligned;

/*
 * Add a IPSEC_OUT_DONE tag to mark that we have finished the ipsec processing
 * It will be used by ip{,6}_output to check if we have already or not 
 * processed this packet.
 */
static int
ipsec_register_done(struct mbuf *m, int * error)
{
	struct m_tag *mtag;

	mtag = m_tag_get(PACKET_TAG_IPSEC_OUT_DONE, 0, M_NOWAIT);
	if (mtag == NULL) {
		IPSECLOG(LOG_DEBUG, "could not get packet tag\n");
		*error = ENOMEM;
		return -1;
	}

	m_tag_prepend(m, mtag);
	return 0;
}

static int
ipsec_reinject_ipstack(struct mbuf *m, int af)
{
	int rv = -1;
	struct route *ro;

	KASSERT(af == AF_INET || af == AF_INET6);

	KERNEL_LOCK_UNLESS_NET_MPSAFE();
	ro = percpu_getref(ipsec_rtcache_percpu);
	switch (af) {
#ifdef INET
	case AF_INET:
		rv = ip_output(m, NULL, ro, IP_RAWOUTPUT|IP_NOIPNEWID,
		    NULL, NULL);
		break;
#endif
#ifdef INET6
	case AF_INET6:
		/*
		 * We don't need massage, IPv6 header fields are always in
		 * net endian.
		 */
		rv = ip6_output(m, NULL, ro, 0, NULL, NULL, NULL);
		break;
#endif
	}
	percpu_putref(ipsec_rtcache_percpu);
	KERNEL_UNLOCK_UNLESS_NET_MPSAFE();

	return rv;
}

int
ipsec_process_done(struct mbuf *m, const struct ipsecrequest *isr,
    struct secasvar *sav)
{
	struct secasindex *saidx;
	int error;
#ifdef INET
	struct ip * ip;
#endif /* INET */
#ifdef INET6
	struct ip6_hdr * ip6;
#endif /* INET6 */
	struct mbuf * mo;
	struct udphdr *udp = NULL;
	uint64_t * data = NULL;
	int hlen, roff;

	IPSEC_SPLASSERT_SOFTNET("ipsec_process_done");

	KASSERT(m != NULL);
	KASSERT(isr != NULL);
	KASSERT(sav != NULL);

	saidx = &sav->sah->saidx;

	if(sav->natt_type != 0) {
		ip = mtod(m, struct ip *);

		hlen = sizeof(struct udphdr);
		if (sav->natt_type == UDP_ENCAP_ESPINUDP_NON_IKE) 
			hlen += sizeof(uint64_t);

		mo = m_makespace(m, sizeof(struct ip), hlen, &roff);
		if (mo == NULL) {
			char buf[IPSEC_ADDRSTRLEN];
			IPSECLOG(LOG_DEBUG,
			    "failed to inject %u byte UDP for SA %s/%08lx\n",
			    hlen, ipsec_address(&saidx->dst, buf, sizeof(buf)),
			    (u_long) ntohl(sav->spi));
			error = ENOBUFS;
			goto bad;
		}
		
		udp = (struct udphdr*) (mtod(mo, char*) + roff);
		data = (uint64_t*) (udp + 1);

		if (sav->natt_type == UDP_ENCAP_ESPINUDP_NON_IKE)
			*data = 0; /* NON-IKE Marker */

		if (sav->natt_type == UDP_ENCAP_ESPINUDP_NON_IKE)
			udp->uh_sport = htons(UDP_ENCAP_ESPINUDP_PORT);
		else
			udp->uh_sport = key_portfromsaddr(&saidx->src);
		
		udp->uh_dport = key_portfromsaddr(&saidx->dst);
		udp->uh_sum = 0;
		udp->uh_ulen = htons(m->m_pkthdr.len - (ip->ip_hl << 2));
	}
	
	switch (saidx->dst.sa.sa_family) {
#ifdef INET
	case AF_INET:
		/* Fix the header length, for AH processing. */
		ip = mtod(m, struct ip *);
		ip->ip_len = htons(m->m_pkthdr.len);
		if (sav->natt_type != 0)
			ip->ip_p = IPPROTO_UDP;
		break;
#endif /* INET */
#ifdef INET6
	case AF_INET6:
		/* Fix the header length, for AH processing. */
		if (m->m_pkthdr.len < sizeof (struct ip6_hdr)) {
			error = ENXIO;
			goto bad;
		}
		if (m->m_pkthdr.len - sizeof (struct ip6_hdr) > IPV6_MAXPACKET) {
			/* No jumbogram support. */
			error = ENXIO;	/*?*/
			goto bad;
		}
		ip6 = mtod(m, struct ip6_hdr *);
		ip6->ip6_plen = htons(m->m_pkthdr.len - sizeof(struct ip6_hdr));
		if (sav->natt_type != 0)
			ip6->ip6_nxt = IPPROTO_UDP;
		break;
#endif /* INET6 */
	default:
		IPSECLOG(LOG_DEBUG, "unknown protocol family %u\n",
		    saidx->dst.sa.sa_family);
		error = ENXIO;
		goto bad;
	}

	key_sa_recordxfer(sav, m);

	/*
	 * If there's another (bundled) SA to apply, do so.
	 * Note that this puts a burden on the kernel stack size.
	 * If this is a problem we'll need to introduce a queue
	 * to set the packet on so we can unwind the stack before
	 * doing further processing.
	 */
	if (isr->next) {
		IPSEC_STATINC(IPSEC_STAT_OUT_BUNDLESA);
		switch ( saidx->dst.sa.sa_family ) {
#ifdef INET
		case AF_INET:
			return ipsec4_process_packet(m, isr->next, NULL);
#endif /* INET */
#ifdef INET6
		case AF_INET6:
			return ipsec6_process_packet(m,isr->next);
#endif /* INET6 */
		default :
			IPSECLOG(LOG_DEBUG, "unknown protocol family %u\n",
			    saidx->dst.sa.sa_family);
			error = ENXIO;
			goto bad;
		}
	}

	/*
	 * We're done with IPsec processing, 
	 * mark that we have already processed the packet
	 * transmit it packet using the appropriate network protocol (IP or IPv6). 
	 */

	if (ipsec_register_done(m, &error) < 0)
		goto bad;

	return ipsec_reinject_ipstack(m, saidx->dst.sa.sa_family);
bad:
	m_freem(m);
	return (error);
}

static void
ipsec_fill_saidx_bymbuf(struct secasindex *saidx, const struct mbuf *m,
    const int af)
{

	if (af == AF_INET) {
		struct sockaddr_in *sin;
		struct ip *ip = mtod(m, struct ip *);

		if (saidx->src.sa.sa_len == 0) {
			sin = &saidx->src.sin;
			sin->sin_len = sizeof(*sin);
			sin->sin_family = AF_INET;
			sin->sin_port = IPSEC_PORT_ANY;
			sin->sin_addr = ip->ip_src;
		}
		if (saidx->dst.sa.sa_len == 0) {
			sin = &saidx->dst.sin;
			sin->sin_len = sizeof(*sin);
			sin->sin_family = AF_INET;
			sin->sin_port = IPSEC_PORT_ANY;
			sin->sin_addr = ip->ip_dst;
		}
	} else {
		struct sockaddr_in6 *sin6;
		struct ip6_hdr *ip6 = mtod(m, struct ip6_hdr *);

		if (saidx->src.sin6.sin6_len == 0) {
			sin6 = (struct sockaddr_in6 *)&saidx->src;
			sin6->sin6_len = sizeof(*sin6);
			sin6->sin6_family = AF_INET6;
			sin6->sin6_port = IPSEC_PORT_ANY;
			sin6->sin6_addr = ip6->ip6_src;
			if (IN6_IS_SCOPE_LINKLOCAL(&ip6->ip6_src)) {
				/* fix scope id for comparing SPD */
				sin6->sin6_addr.s6_addr16[1] = 0;
				sin6->sin6_scope_id =
				    ntohs(ip6->ip6_src.s6_addr16[1]);
			}
		}
		if (saidx->dst.sin6.sin6_len == 0) {
			sin6 = (struct sockaddr_in6 *)&saidx->dst;
			sin6->sin6_len = sizeof(*sin6);
			sin6->sin6_family = AF_INET6;
			sin6->sin6_port = IPSEC_PORT_ANY;
			sin6->sin6_addr = ip6->ip6_dst;
			if (IN6_IS_SCOPE_LINKLOCAL(&ip6->ip6_dst)) {
				/* fix scope id for comparing SPD */
				sin6->sin6_addr.s6_addr16[1] = 0;
				sin6->sin6_scope_id =
				    ntohs(ip6->ip6_dst.s6_addr16[1]);
			}
		}
	}
}

struct secasvar *
ipsec_lookup_sa(const struct ipsecrequest *isr, const struct mbuf *m)
{
	struct secasindex saidx;

	saidx = isr->saidx;
	if (isr->saidx.mode == IPSEC_MODE_TRANSPORT) {
		/* Fillin unspecified SA peers only for transport mode */
		ipsec_fill_saidx_bymbuf(&saidx, m, isr->saidx.dst.sa.sa_family);
	}

	return key_lookup_sa_bysaidx(&saidx);
}

/*
 * ipsec_nextisr can return :
 * - isr == NULL and error != 0 => something is bad : the packet must be
 *   discarded
 * - isr == NULL and error == 0 => no more rules to apply, ipsec processing 
 *   is done, reinject it in ip stack
 * - isr != NULL (error == 0) => we need to apply one rule to the packet
 */
static const struct ipsecrequest *
ipsec_nextisr(
	struct mbuf *m,
	const struct ipsecrequest *isr,
	int af,
	int *error,
	struct secasvar **ret
)
{
#define	IPSEC_OSTAT(type)						\
do {									\
	switch (isr->saidx.proto) {					\
	case IPPROTO_ESP:						\
		ESP_STATINC(ESP_STAT_ ## type);				\
		break;							\
	case IPPROTO_AH:						\
		AH_STATINC(AH_STAT_ ## type);				\
		break;							\
	default:							\
		IPCOMP_STATINC(IPCOMP_STAT_ ## type);			\
		break;							\
	}								\
} while (/*CONSTCOND*/0)

	struct secasvar *sav = NULL;
	struct secasindex saidx;

	IPSEC_SPLASSERT_SOFTNET("ipsec_nextisr");
	KASSERTMSG(af == AF_INET || af == AF_INET6,
	    "invalid address family %u", af);
again:
	/*
	 * Craft SA index to search for proper SA.  Note that
	 * we only fillin unspecified SA peers for transport
	 * mode; for tunnel mode they must already be filled in.
	 */
	saidx = isr->saidx;
	if (isr->saidx.mode == IPSEC_MODE_TRANSPORT) {
		/* Fillin unspecified SA peers only for transport mode */
		ipsec_fill_saidx_bymbuf(&saidx, m, af);
	}

	/*
	 * Lookup SA and validate it.
	 */
	*error = key_checkrequest(isr, &saidx, &sav);
	if (*error != 0) {
		/*
		 * IPsec processing is required, but no SA found.
		 * I assume that key_acquire() had been called
		 * to get/establish the SA. Here I discard
		 * this packet because it is responsibility for
		 * upper layer to retransmit the packet.
		 */
		IPSEC_STATINC(IPSEC_STAT_OUT_NOSA);
		goto bad;
	}
	/* sav may be NULL here if we have an USE rule */
	if (sav == NULL) {		
		KASSERTMSG(ipsec_get_reqlevel(isr) == IPSEC_LEVEL_USE,
		    "no SA found, but required; level %u",
		    ipsec_get_reqlevel(isr));
		isr = isr->next;
		/* 
		 * No more rules to apply, return NULL isr and no error 
		 * It can happen when the last rules are USE rules
		 * */
		if (isr == NULL) {
			*ret = NULL;
			*error = 0;		
			return isr;
		}
		goto again;
	}

	/*
	 * Check system global policy controls.
	 */
	if ((isr->saidx.proto == IPPROTO_ESP && !esp_enable) ||
	    (isr->saidx.proto == IPPROTO_AH && !ah_enable) ||
	    (isr->saidx.proto == IPPROTO_IPCOMP && !ipcomp_enable)) {
		IPSECLOG(LOG_DEBUG, "IPsec outbound packet dropped due"
		    " to policy (check your sysctls)\n");
		IPSEC_OSTAT(PDROPS);
		*error = EHOSTUNREACH;
		KEY_SA_UNREF(&sav);
		goto bad;
	}

	/*
	 * Sanity check the SA contents for the caller
	 * before they invoke the xform output method.
	 */
	KASSERT(sav->tdb_xform != NULL);
	*ret = sav;
	return isr;
bad:
	KASSERTMSG(*error != 0, "error return w/ no error code");
	return NULL;
#undef IPSEC_OSTAT
}

#ifdef INET
/*
 * IPsec output logic for IPv4.
 */
int
ipsec4_process_packet(struct mbuf *m, const struct ipsecrequest *isr,
    u_long *mtu)
{
	struct secasvar *sav = NULL;
	struct ip *ip;
	int s, error, i, off;
	union sockaddr_union *dst;
	int setdf;

	KASSERT(m != NULL);
	KASSERT(isr != NULL);

	s = splsoftnet();			/* insure SA contents don't change */

	isr = ipsec_nextisr(m, isr, AF_INET, &error, &sav);
	if (isr == NULL) {
		if (error != 0) {
			goto bad;
		} else {
			if (ipsec_register_done(m, &error) < 0)
				goto bad;

			splx(s);
			return ipsec_reinject_ipstack(m, AF_INET);
		}
	}

	KASSERT(sav != NULL);
	/*
	 * Check if we need to handle NAT-T fragmentation.
	 */
	if (isr == isr->sp->req) { /* Check only if called from ipsec4_output */
		KASSERT(mtu != NULL);
		ip = mtod(m, struct ip *);
		if (!(sav->natt_type &
		    (UDP_ENCAP_ESPINUDP|UDP_ENCAP_ESPINUDP_NON_IKE))) {
			goto noneed;
		}
		if (ntohs(ip->ip_len) <= sav->esp_frag)
			goto noneed;
		*mtu = sav->esp_frag;
		KEY_SA_UNREF(&sav);
		splx(s);
		return 0;
	}
noneed:
	dst = &sav->sah->saidx.dst;

	/*
	 * Collect IP_DF state from the outer header.
	 */
	if (dst->sa.sa_family == AF_INET) {
		if (m->m_len < sizeof (struct ip) &&
		    (m = m_pullup(m, sizeof (struct ip))) == NULL) {
			error = ENOBUFS;
			goto unrefsav;
		}
		ip = mtod(m, struct ip *);
		/* Honor system-wide control of how to handle IP_DF */
		switch (ip4_ipsec_dfbit) {
		case 0:			/* clear in outer header */
		case 1:			/* set in outer header */
			setdf = ip4_ipsec_dfbit;
			break;
		default:		/* propagate to outer header */
			setdf = ip->ip_off;
			setdf = ntohs(setdf);
			setdf = htons(setdf & IP_DF);
			break;
		}
	} else {
		ip = NULL;		/* keep compiler happy */
		setdf = 0;
	}
	/* Do the appropriate encapsulation, if necessary */
	if (isr->saidx.mode == IPSEC_MODE_TUNNEL || /* Tunnel requ'd */
	    dst->sa.sa_family != AF_INET ||	    /* PF mismatch */
#if 0
	    (sav->flags & SADB_X_SAFLAGS_TUNNEL) || /* Tunnel requ'd */
	    sav->tdb_xform->xf_type == XF_IP4 ||    /* ditto */
#endif
	    (dst->sa.sa_family == AF_INET &&	    /* Proxy */
	     dst->sin.sin_addr.s_addr != INADDR_ANY &&
	     dst->sin.sin_addr.s_addr != ip->ip_dst.s_addr)) {
		struct mbuf *mp;

		/* Fix IPv4 header checksum and length */
		if (m->m_len < sizeof (struct ip) &&
		    (m = m_pullup(m, sizeof (struct ip))) == NULL) {
			error = ENOBUFS;
			goto unrefsav;
		}
		ip = mtod(m, struct ip *);
		ip->ip_len = htons(m->m_pkthdr.len);
		ip->ip_sum = 0;
		ip->ip_sum = in_cksum(m, ip->ip_hl << 2);

		/* Encapsulate the packet */
		error = ipip_output(m, isr, sav, &mp, 0, 0);
		if (mp == NULL && !error) {
			/* Should never happen. */
			IPSECLOG(LOG_DEBUG,
			    "ipip_output returns no mbuf and no error!");
			error = EFAULT;
		}
		if (error) {
			if (mp) {
				/* XXX: Should never happen! */
				m_freem(mp);
			}
			m = NULL; /* ipip_output() already freed it */
			goto unrefsav;
		}
		m = mp, mp = NULL;
		/*
		 * ipip_output clears IP_DF in the new header.  If
		 * we need to propagate IP_DF from the outer header,
		 * then we have to do it here.
		 *
		 * XXX shouldn't assume what ipip_output does.
		 */
		if (dst->sa.sa_family == AF_INET && setdf) {
			if (m->m_len < sizeof (struct ip) &&
			    (m = m_pullup(m, sizeof (struct ip))) == NULL) {
				error = ENOBUFS;
				goto unrefsav;
			}
			ip = mtod(m, struct ip *);
			ip->ip_off |= htons(IP_DF);
		}
	}

	/*
	 * Dispatch to the appropriate IPsec transform logic.  The
	 * packet will be returned for transmission after crypto
	 * processing, etc. are completed.  For encapsulation we
	 * bypass this call because of the explicit call done above
	 * (necessary to deal with IP_DF handling for IPv4).
	 *
	 * NB: m & sav are ``passed to caller'' who's reponsible for
	 *     for reclaiming their resources.
	 */
	if (sav->tdb_xform->xf_type != XF_IP4) {
		if (dst->sa.sa_family == AF_INET) {
			ip = mtod(m, struct ip *);
			i = ip->ip_hl << 2;
			off = offsetof(struct ip, ip_p);
		} else {
			i = sizeof(struct ip6_hdr);
			off = offsetof(struct ip6_hdr, ip6_nxt);
		}
		error = (*sav->tdb_xform->xf_output)(m, isr, sav, NULL, i, off);
	} else {
		error = ipsec_process_done(m, isr, sav);
	}
	KEY_SA_UNREF(&sav);
	splx(s);
	return error;
unrefsav:
	KEY_SA_UNREF(&sav);
bad:
	splx(s);
	if (m)
		m_freem(m);
	return error;
}
#endif

#ifdef INET6
static void
compute_ipsec_pos(struct mbuf *m, int *i, int *off)
{
	int nxt;
	struct ip6_hdr *ip6 = mtod(m, struct ip6_hdr*);
	struct ip6_ext ip6e;
	int dstopt = 0;

	*i = sizeof(struct ip6_hdr);
	*off = offsetof(struct ip6_hdr, ip6_nxt);
	nxt = ip6->ip6_nxt;

	/*
	 * chase mbuf chain to find the appropriate place to
	 * put AH/ESP/IPcomp header.
	 *  IPv6 hbh dest1 rthdr ah* [esp* dest2 payload]
	 */
	do {
		switch (nxt) {
		case IPPROTO_AH:
		case IPPROTO_ESP:
		case IPPROTO_IPCOMP:
		/*
		 * we should not skip security header added
		 * beforehand.
		 */
			return;

		case IPPROTO_HOPOPTS:
		case IPPROTO_DSTOPTS:
		case IPPROTO_ROUTING:
		/*
		 * if we see 2nd destination option header,
		 * we should stop there.
		 */
			if (nxt == IPPROTO_DSTOPTS && dstopt)
				return;

			if (nxt == IPPROTO_DSTOPTS) {
				/*
				 * seen 1st or 2nd destination option.
				 * next time we see one, it must be 2nd.
				 */
				dstopt = 1;
			} else if (nxt == IPPROTO_ROUTING) {
				/*
				 * if we see destionation option next
				 * time, it must be dest2.
				 */
				dstopt = 2;
			}

			/* skip this header */
			m_copydata(m, *i, sizeof(ip6e), &ip6e);
			nxt = ip6e.ip6e_nxt;
			*off = *i + offsetof(struct ip6_ext, ip6e_nxt);
			/*
			 * we will never see nxt == IPPROTO_AH
			 * so it is safe to omit AH case.
			 */
			*i += (ip6e.ip6e_len + 1) << 3;
			break;
		default:
			return;
		}
	} while (*i < m->m_pkthdr.len);
}

static int
in6_sa_equal_addrwithscope(const struct sockaddr_in6 *sa, const struct in6_addr *ia)
{
	struct in6_addr ia2;

	memcpy(&ia2, &sa->sin6_addr, sizeof(ia2));
	if (IN6_IS_SCOPE_LINKLOCAL(&sa->sin6_addr))
		ia2.s6_addr16[1] = htons(sa->sin6_scope_id);

	return IN6_ARE_ADDR_EQUAL(ia, &ia2);
}

int
ipsec6_process_packet(
	struct mbuf *m,
 	const struct ipsecrequest *isr
    )
{
	struct secasvar *sav = NULL;
	struct ip6_hdr *ip6;
	int s, error, i, off;
	union sockaddr_union *dst;

	KASSERT(m != NULL);
	KASSERT(isr != NULL);

	s = splsoftnet();   /* insure SA contents don't change */

	isr = ipsec_nextisr(m, isr, AF_INET6, &error, &sav);
	if (isr == NULL) {
		if (error != 0) {
			/* XXX Should we send a notification ? */
			goto bad;
		} else {
			if (ipsec_register_done(m, &error) < 0)
				goto bad;

			splx(s);
			return ipsec_reinject_ipstack(m, AF_INET6);
		}
	}

	KASSERT(sav != NULL);
	dst = &sav->sah->saidx.dst;

	ip6 = mtod(m, struct ip6_hdr *); /* XXX */

	/* Do the appropriate encapsulation, if necessary */
	if (isr->saidx.mode == IPSEC_MODE_TUNNEL || /* Tunnel requ'd */
	    dst->sa.sa_family != AF_INET6 ||        /* PF mismatch */
	    ((dst->sa.sa_family == AF_INET6) &&
	     (!IN6_IS_ADDR_UNSPECIFIED(&dst->sin6.sin6_addr)) &&
	     (!in6_sa_equal_addrwithscope(&dst->sin6,
				  &ip6->ip6_dst)))) {
		struct mbuf *mp;

		/* Fix IPv6 header payload length. */
		if (m->m_len < sizeof(struct ip6_hdr)) {
			if ((m = m_pullup(m,sizeof(struct ip6_hdr))) == NULL) {
				error = ENOBUFS;
				goto unrefsav;
			}
		}

		if (m->m_pkthdr.len - sizeof(*ip6) > IPV6_MAXPACKET) {
			/* No jumbogram support. */
			error = ENXIO;   /*XXX*/
			goto unrefsav;
		}

		ip6 = mtod(m, struct ip6_hdr *);
		ip6->ip6_plen = htons(m->m_pkthdr.len - sizeof(*ip6));

		/* Encapsulate the packet */
		error = ipip_output(m, isr, sav, &mp, 0, 0);
		if (mp == NULL && !error) {
			/* Should never happen. */
			IPSECLOG(LOG_DEBUG,
			    "ipip_output returns no mbuf and no error!");
			error = EFAULT;
		}

		if (error) {
			if (mp) {
				/* XXX: Should never happen! */
				m_freem(mp);
			}
			m = NULL; /* ipip_output() already freed it */
			goto unrefsav;
		}

		m = mp;
		mp = NULL;
	}

	if (dst->sa.sa_family == AF_INET) {
		struct ip *ip;
		ip = mtod(m, struct ip *);
		i = ip->ip_hl << 2;
		off = offsetof(struct ip, ip_p);
	} else {	
		compute_ipsec_pos(m, &i, &off);
	}
	error = (*sav->tdb_xform->xf_output)(m, isr, sav, NULL, i, off);
	KEY_SA_UNREF(&sav);
	splx(s);
	return error;
unrefsav:
	KEY_SA_UNREF(&sav);
bad:
	splx(s);
	if (m)
		m_freem(m);
	return error;
}
#endif /*INET6*/

void
ipsec_output_init(void)
{

	ipsec_rtcache_percpu = percpu_alloc(sizeof(struct route));
}
