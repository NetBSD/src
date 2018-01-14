/*	$NetBSD: xform_ipip.c,v 1.56 2018/01/14 16:36:04 maxv Exp $	*/
/*	$FreeBSD: src/sys/netipsec/xform_ipip.c,v 1.3.2.1 2003/01/24 05:11:36 sam Exp $	*/
/*	$OpenBSD: ip_ipip.c,v 1.25 2002/06/10 18:04:55 itojun Exp $ */

/*
 * The authors of this code are John Ioannidis (ji@tla.org),
 * Angelos D. Keromytis (kermit@csd.uch.gr) and
 * Niels Provos (provos@physnet.uni-hamburg.de).
 *
 * The original version of this code was written by John Ioannidis
 * for BSD/OS in Athens, Greece, in November 1995.
 *
 * Ported to OpenBSD and NetBSD, with additional transforms, in December 1996,
 * by Angelos D. Keromytis.
 *
 * Additional transforms and features in 1997 and 1998 by Angelos D. Keromytis
 * and Niels Provos.
 *
 * Additional features in 1999 by Angelos D. Keromytis.
 *
 * Copyright (C) 1995, 1996, 1997, 1998, 1999 by John Ioannidis,
 * Angelos D. Keromytis and Niels Provos.
 * Copyright (c) 2001, Angelos D. Keromytis.
 *
 * Permission to use, copy, and modify this software with or without fee
 * is hereby granted, provided that this entire notice is included in
 * all copies of any software which is or includes a copy or
 * modification of this software.
 * You may use this code under the GNU public license if you so wish. Please
 * contribute changes back to the authors under this freer than GPL license
 * so that we may further the use of strong encryption without limitations to
 * all.
 *
 * THIS SOFTWARE IS BEING PROVIDED "AS IS", WITHOUT ANY EXPRESS OR
 * IMPLIED WARRANTY. IN PARTICULAR, NONE OF THE AUTHORS MAKES ANY
 * REPRESENTATION OR WARRANTY OF ANY KIND CONCERNING THE
 * MERCHANTABILITY OF THIS SOFTWARE OR ITS FITNESS FOR ANY PARTICULAR
 * PURPOSE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: xform_ipip.c,v 1.56 2018/01/14 16:36:04 maxv Exp $");

/*
 * IP-inside-IP processing
 */
#if defined(_KERNEL_OPT)
#include "opt_inet.h"
#endif

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/mbuf.h>
#include <sys/socket.h>
#include <sys/kernel.h>
#include <sys/protosw.h>
#include <sys/sysctl.h>

#include <net/if.h>
#include <net/route.h>
#include <net/netisr.h>

#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/in_var.h>
#include <netinet/ip.h>
#include <netinet/ip_ecn.h>
#include <netinet/ip_var.h>
#include <netinet/ip_encap.h>

#include <netipsec/ipsec.h>
#include <netipsec/ipsec_private.h>
#include <netipsec/xform.h>

#include <netipsec/ipip_var.h>

#ifdef MROUTING
#include <netinet/ip_mroute.h>
#endif

#ifdef INET6
#include <netinet/ip6.h>
#include <netipsec/ipsec6.h>
#include <netinet6/in6_var.h>
#include <netinet6/ip6protosw.h>
#endif

#include <netipsec/key.h>
#include <netipsec/key_debug.h>

typedef void	pr_in_input_t (struct mbuf *m, ...);

/*
 * We can control the acceptance of IP4 packets by altering the sysctl
 * net.inet.ipip.allow value.  Zero means drop them, all else is acceptance.
 */
int	ipip_allow = 0;

percpu_t *ipipstat_percpu;

#ifdef SYSCTL_DECL
SYSCTL_DECL(_net_inet_ipip);

SYSCTL_INT(_net_inet_ipip, OID_AUTO,
	ipip_allow,	CTLFLAG_RW,	&ipip_allow,	0, "");
SYSCTL_STRUCT(_net_inet_ipip, IPSECCTL_STATS,
	stats,		CTLFLAG_RD,	&ipipstat,	ipipstat, "");

#endif

void ipe4_attach(void);


/* XXX IPCOMP */
#define	M_IPSEC	(M_AUTHIPHDR|M_AUTHIPDGM|M_DECRYPTED)

static void _ipip_input(struct mbuf *m, int iphlen, struct ifnet *gifp);

#ifdef INET6
/*
 * Really only a wrapper for ipip_input(), for use with IPv6.
 */
int
ip4_input6(struct mbuf **m, int *offp, int proto, void *eparg __unused)
{
#if 0
	/* If we do not accept IP-in-IP explicitly, drop.  */
	if (!ipip_allow && ((*m)->m_flags & M_IPSEC) == 0) {
		DPRINTF(("%s: dropped due to policy\n", __func__));
		IPIP_STATINC(IPIP_STAT_PDROPS);
		m_freem(*m);
		return IPPROTO_DONE;
	}
#endif
	_ipip_input(*m, *offp, NULL);
	return IPPROTO_DONE;
}
#endif /* INET6 */

#ifdef INET
/*
 * Really only a wrapper for ipip_input(), for use with IPv4.
 */
void
ip4_input(struct mbuf *m, int off, int proto, void *eparg __unused)
{

#if 0
	/* If we do not accept IP-in-IP explicitly, drop.  */
	if (!ipip_allow && (m->m_flags & M_IPSEC) == 0) {
		DPRINTF(("%s: dropped due to policy\n", __func__));
		IPIP_STATINC(IPIP_STAT_PDROPS);
		m_freem(m);
		return;
	}
#endif

	_ipip_input(m, off, NULL);
}
#endif /* INET */

/*
 * ipip_input gets called when we receive an IP{46} encapsulated packet,
 * either because we got it at a real interface, or because AH or ESP
 * were being used in tunnel mode (in which case the rcvif element will
 * contain the address of the encX interface associated with the tunnel.
 */

static void
_ipip_input(struct mbuf *m, int iphlen, struct ifnet *gifp)
{
	register struct sockaddr_in *sin;
	register struct ifnet *ifp;
	register struct ifaddr *ifa;
	pktqueue_t *pktq = NULL;
	struct ip *ipo;
#ifdef INET6
	register struct sockaddr_in6 *sin6;
	struct ip6_hdr *ip6 = NULL;
	uint8_t itos;
#endif
	uint8_t otos;
	uint8_t v;
	int hlen;

	IPIP_STATINC(IPIP_STAT_IPACKETS);

	m_copydata(m, 0, 1, &v);

	switch (v >> 4) {
#ifdef INET
        case 4:
		hlen = sizeof(struct ip);
		break;
#endif /* INET */
#ifdef INET6
        case 6:
		hlen = sizeof(struct ip6_hdr);
		break;
#endif
        default:
		DPRINTF(("%s: bad protocol version 0x%x (%u) "
		    "for outer header\n", __func__, v, v>>4));
		IPIP_STATINC(IPIP_STAT_FAMILY);
		m_freem(m);
		return /* EAFNOSUPPORT */;
	}

	/* Bring the IP header in the first mbuf, if not there already */
	if (m->m_len < hlen) {
		if ((m = m_pullup(m, hlen)) == NULL) {
			DPRINTF(("%s: m_pullup (1) failed\n", __func__));
			IPIP_STATINC(IPIP_STAT_HDROPS);
			return;
		}
	}

	ipo = mtod(m, struct ip *);

#ifdef MROUTING
	if (ipo->ip_v == IPVERSION && ipo->ip_p == IPPROTO_IPV4) {
		if (IN_MULTICAST(((struct ip *)((char *) ipo + iphlen))->ip_dst.s_addr)) {
			ipip_mroute_input (m, iphlen);
			return;
		}
	}
#endif /* MROUTING */

	/* Keep outer ecn field. */
	switch (v >> 4) {
#ifdef INET
	case 4:
		otos = ipo->ip_tos;
		break;
#endif /* INET */
#ifdef INET6
	case 6:
		otos = (ntohl(mtod(m, struct ip6_hdr *)->ip6_flow) >> 20) & 0xff;
		break;
#endif
	default:
		panic("%s: unknown ip version %u (outer)", __func__, v >> 4);
	}

	/* Remove outer IP header */
	m_adj(m, iphlen);

	/* Sanity check */
	if (m->m_pkthdr.len < sizeof(struct ip))  {
		IPIP_STATINC(IPIP_STAT_HDROPS);
		m_freem(m);
		return;
	}

	m_copydata(m, 0, 1, &v);

	switch (v >> 4) {
#ifdef INET
        case 4:
		hlen = sizeof(struct ip);
		break;
#endif /* INET */

#ifdef INET6
        case 6:
		hlen = sizeof(struct ip6_hdr);
		break;
#endif
	default:
		DPRINTF(("%s: bad protocol version %#x (%u) "
		    "for inner header\n", __func__, v, v >> 4));
		IPIP_STATINC(IPIP_STAT_FAMILY);
		m_freem(m);
		return; /* EAFNOSUPPORT */
	}

	/*
	 * Bring the inner IP header in the first mbuf, if not there already.
	 */
	if (m->m_len < hlen) {
		if ((m = m_pullup(m, hlen)) == NULL) {
			DPRINTF(("%s: m_pullup (2) failed\n", __func__));
			IPIP_STATINC(IPIP_STAT_HDROPS);
			return;
		}
	}

	/*
	 * RFC 1853 specifies that the inner TTL should not be touched on
	 * decapsulation. There's no reason this comment should be here, but
	 * this is as good as any a position.
	 */

	/* Some sanity checks in the inner IP header */
	switch (v >> 4) {
#ifdef INET
    	case 4:
                ipo = mtod(m, struct ip *);
		ip_ecn_egress(ip4_ipsec_ecn, &otos, &ipo->ip_tos);
                break;
#endif /* INET */
#ifdef INET6
    	case 6:
		ipo = mtod(m, struct ip *);
		ip6 = (struct ip6_hdr *)ipo;
		itos = (ntohl(ip6->ip6_flow) >> 20) & 0xff;
		ip_ecn_egress(ip6_ipsec_ecn, &otos, &itos);
		ip6->ip6_flow &= ~htonl(0xff << 20);
		ip6->ip6_flow |= htonl((uint32_t) itos << 20);
                break;
#endif
	default:
		panic("%s: unknown ip version %u (inner)", __func__, v>>4);
	}

	/* Check for local address spoofing. */
	if ((m_get_rcvif_NOMPSAFE(m) == NULL ||
	    !(m_get_rcvif_NOMPSAFE(m)->if_flags & IFF_LOOPBACK)) &&
	    ipip_allow != 2) {
		int s = pserialize_read_enter();
		IFNET_READER_FOREACH(ifp) {
			IFADDR_READER_FOREACH(ifa, ifp) {
#ifdef INET
				if (ipo) {
					if (ifa->ifa_addr->sa_family !=
					    AF_INET)
						continue;

					sin = (struct sockaddr_in *) ifa->ifa_addr;

					if (sin->sin_addr.s_addr ==
					    ipo->ip_src.s_addr)	{
						pserialize_read_exit(s);
						IPIP_STATINC(IPIP_STAT_SPOOF);
						m_freem(m);
						return;
					}
				}
#endif /* INET */

#ifdef INET6
				if (ip6) {
					if (ifa->ifa_addr->sa_family !=
					    AF_INET6)
						continue;

					sin6 = (struct sockaddr_in6 *) ifa->ifa_addr;

					if (IN6_ARE_ADDR_EQUAL(&sin6->sin6_addr, &ip6->ip6_src)) {
						pserialize_read_exit(s);
						IPIP_STATINC(IPIP_STAT_SPOOF);
						m_freem(m);
						return;
					}

				}
#endif /* INET6 */
			}
		}
		pserialize_read_exit(s);
	}

	/* Statistics */
	IPIP_STATADD(IPIP_STAT_IBYTES, m->m_pkthdr.len - iphlen);

	/*
	 * Interface pointer stays the same; if no IPsec processing has
	 * been done (or will be done), this will point to a normal
	 * interface. Otherwise, it'll point to an enc interface, which
	 * will allow a packet filter to distinguish between secure and
	 * untrusted packets.
	 */

	switch (v >> 4) {
#ifdef INET
	case 4:
		pktq = ip_pktq;
		break;
#endif
#ifdef INET6
	case 6:
		pktq = ip6_pktq;
		break;
#endif
	default:
		panic("%s: should never reach here", __func__);
	}

	int s = splnet();
	if (__predict_false(!pktq_enqueue(pktq, m, 0))) {
		IPIP_STATINC(IPIP_STAT_QFULL);
		m_freem(m);
	}
	splx(s);
}

int
ipip_output(
    struct mbuf *m,
    const struct ipsecrequest *isr,
    struct secasvar *sav,
    struct mbuf **mp,
    int skip,
    int protoff
)
{
	char buf[IPSEC_ADDRSTRLEN];
	uint8_t tp, otos;
	struct secasindex *saidx;
	int error;
#ifdef INET
	uint8_t itos;
	struct ip *ipo;
#endif /* INET */
#ifdef INET6
	struct ip6_hdr *ip6, *ip6o;
#endif /* INET6 */

	IPSEC_SPLASSERT_SOFTNET(__func__);
	KASSERT(sav != NULL);

	/* XXX Deal with empty TDB source/destination addresses. */

	m_copydata(m, 0, 1, &tp);
	tp = (tp >> 4) & 0xff;  /* Get the IP version number. */

	saidx = &sav->sah->saidx;
	switch (saidx->dst.sa.sa_family) {
#ifdef INET
	case AF_INET:
		if (saidx->src.sa.sa_family != AF_INET ||
		    saidx->src.sin.sin_addr.s_addr == INADDR_ANY ||
		    saidx->dst.sin.sin_addr.s_addr == INADDR_ANY) {
			DPRINTF(("%s: unspecified tunnel endpoint "
			    "address in SA %s/%08lx\n", __func__,
			    ipsec_address(&saidx->dst, buf, sizeof(buf)),
			    (u_long) ntohl(sav->spi)));
			IPIP_STATINC(IPIP_STAT_UNSPEC);
			error = EINVAL;
			goto bad;
		}

		M_PREPEND(m, sizeof(struct ip), M_DONTWAIT);
		if (m == 0) {
			DPRINTF(("%s: M_PREPEND failed\n", __func__));
			IPIP_STATINC(IPIP_STAT_HDROPS);
			error = ENOBUFS;
			goto bad;
		}

		ipo = mtod(m, struct ip *);

		ipo->ip_v = IPVERSION;
		ipo->ip_hl = 5;
		ipo->ip_len = htons(m->m_pkthdr.len);
		ipo->ip_ttl = ip_defttl;
		ipo->ip_sum = 0;
		ipo->ip_src = saidx->src.sin.sin_addr;
		ipo->ip_dst = saidx->dst.sin.sin_addr;
		ipo->ip_id = ip_newid(NULL);

		/* If the inner protocol is IP... */
		if (tp == IPVERSION) {
			/* Save ECN notification */
			m_copydata(m, sizeof(struct ip) +
			    offsetof(struct ip, ip_tos),
			    sizeof(uint8_t), &itos);

			ipo->ip_p = IPPROTO_IPIP;

			/*
			 * We should be keeping tunnel soft-state and
			 * send back ICMPs if needed.
			 */
			m_copydata(m, sizeof(struct ip) +
			    offsetof(struct ip, ip_off),
			    sizeof(uint16_t), &ipo->ip_off);
			ipo->ip_off &= ~ htons(IP_DF | IP_MF | IP_OFFMASK);
		}
#ifdef INET6
		else if (tp == (IPV6_VERSION >> 4)) {
			uint32_t itos32;

			/* Save ECN notification. */
			m_copydata(m, sizeof(struct ip) +
			    offsetof(struct ip6_hdr, ip6_flow),
			    sizeof(uint32_t), &itos32);
			itos = ntohl(itos32) >> 20;
			ipo->ip_p = IPPROTO_IPV6;
			ipo->ip_off = 0;
		}
#endif /* INET6 */
		else {
			goto nofamily;
		}

		otos = 0;
		ip_ecn_ingress(ECN_ALLOWED, &otos, &itos);
		ipo->ip_tos = otos;
		break;
#endif /* INET */

#ifdef INET6
	case AF_INET6:
		if (IN6_IS_ADDR_UNSPECIFIED(&saidx->dst.sin6.sin6_addr) ||
		    saidx->src.sa.sa_family != AF_INET6 ||
		    IN6_IS_ADDR_UNSPECIFIED(&saidx->src.sin6.sin6_addr)) {
			DPRINTF(("%s: unspecified tunnel endpoint "
			    "address in SA %s/%08lx\n", __func__,
			    ipsec_address(&saidx->dst, buf, sizeof(buf)),
			    (u_long) ntohl(sav->spi)));
			IPIP_STATINC(IPIP_STAT_UNSPEC);
			error = ENOBUFS;
			goto bad;
		}

		if (tp == (IPV6_VERSION >> 4)) {
			/* scoped address handling */
			ip6 = mtod(m, struct ip6_hdr *);
			if (IN6_IS_SCOPE_LINKLOCAL(&ip6->ip6_src))
				ip6->ip6_src.s6_addr16[1] = 0;
			if (IN6_IS_SCOPE_LINKLOCAL(&ip6->ip6_dst))
				ip6->ip6_dst.s6_addr16[1] = 0;
		}

		M_PREPEND(m, sizeof(struct ip6_hdr), M_DONTWAIT);
		if (m == 0) {
			DPRINTF(("%s: M_PREPEND failed\n", __func__));
			IPIP_STATINC(IPIP_STAT_HDROPS);
			error = ENOBUFS;
			goto bad;
		}

		/* Initialize IPv6 header */
		ip6o = mtod(m, struct ip6_hdr *);
		ip6o->ip6_flow = 0;
		ip6o->ip6_vfc &= ~IPV6_VERSION_MASK;
		ip6o->ip6_vfc |= IPV6_VERSION;
		ip6o->ip6_plen = htons(m->m_pkthdr.len - sizeof(*ip6o));
		ip6o->ip6_hlim = ip_defttl;
		ip6o->ip6_dst = saidx->dst.sin6.sin6_addr;
		ip6o->ip6_src = saidx->src.sin6.sin6_addr;
		if (IN6_IS_SCOPE_LINKLOCAL(&ip6o->ip6_dst))
			ip6o->ip6_dst.s6_addr16[1] = htons(saidx->dst.sin6.sin6_scope_id);
		if (IN6_IS_SCOPE_LINKLOCAL(&ip6o->ip6_src))
			ip6o->ip6_src.s6_addr16[1] = htons(saidx->src.sin6.sin6_scope_id);

#ifdef INET
		if (tp == IPVERSION) {
			/* Save ECN notification */
			m_copydata(m, sizeof(struct ip6_hdr) +
			    offsetof(struct ip, ip_tos), sizeof(uint8_t),
			    &itos);

			/* This is really IPVERSION. */
			ip6o->ip6_nxt = IPPROTO_IPIP;
		} else
#endif /* INET */
			if (tp == (IPV6_VERSION >> 4)) {
				uint32_t itos32;

				/* Save ECN notification. */
				m_copydata(m, sizeof(struct ip6_hdr) +
				    offsetof(struct ip6_hdr, ip6_flow),
				    sizeof(uint32_t), &itos32);
				itos = ntohl(itos32) >> 20;

				ip6o->ip6_nxt = IPPROTO_IPV6;
			} else {
				goto nofamily;
			}

		otos = 0;
		ip_ecn_ingress(ECN_ALLOWED, &otos, &itos);
		ip6o->ip6_flow |= htonl((uint32_t) otos << 20);
		break;
#endif /* INET6 */

	default:
nofamily:
		DPRINTF(("%s: unsupported protocol family %u\n", __func__,
		    saidx->dst.sa.sa_family));
		IPIP_STATINC(IPIP_STAT_FAMILY);
		error = EAFNOSUPPORT;		/* XXX diffs from openbsd */
		goto bad;
	}

	IPIP_STATINC(IPIP_STAT_OPACKETS);
	*mp = m;

#ifdef INET
	if (saidx->dst.sa.sa_family == AF_INET) {
#if 0
		if (sav->tdb_xform->xf_type == XF_IP4)
			tdb->tdb_cur_bytes +=
			    m->m_pkthdr.len - sizeof(struct ip);
#endif
		IPIP_STATADD(IPIP_STAT_OBYTES,
			     m->m_pkthdr.len - sizeof(struct ip));
	}
#endif /* INET */

#ifdef INET6
	if (saidx->dst.sa.sa_family == AF_INET6) {
#if 0
		if (sav->tdb_xform->xf_type == XF_IP4)
			tdb->tdb_cur_bytes +=
			    m->m_pkthdr.len - sizeof(struct ip6_hdr);
#endif
		IPIP_STATADD(IPIP_STAT_OBYTES,
		    m->m_pkthdr.len - sizeof(struct ip6_hdr));
	}
#endif /* INET6 */

	return 0;
bad:
	if (m)
		m_freem(m);
	*mp = NULL;
	return (error);
}

static int
ipe4_init(struct secasvar *sav, const struct xformsw *xsp)
{
	sav->tdb_xform = xsp;
	return 0;
}

static int
ipe4_zeroize(struct secasvar *sav)
{
	sav->tdb_xform = NULL;
	return 0;
}

static int
ipe4_input(
    struct mbuf *m,
    struct secasvar *sav,
    int skip,
    int protoff
)
{
	/* This is a rather serious mistake, so no conditional printing. */
	printf("%s: should never be called\n", __func__);
	if (m)
		m_freem(m);
	return EOPNOTSUPP;
}

static struct xformsw ipe4_xformsw = {
	.xf_type	= XF_IP4,
	.xf_flags	= 0,
	.xf_name	= "IPv4 Simple Encapsulation",
	.xf_init	= ipe4_init,
	.xf_zeroize	= ipe4_zeroize,
	.xf_input	= ipe4_input,
	.xf_output	= ipip_output,
	.xf_next	= NULL,
};

#ifdef INET
static struct encapsw ipe4_encapsw = {
	.encapsw4 = {
		.pr_input = ip4_input,
		.pr_ctlinput = NULL,
	}
};
#endif
#ifdef INET6
static struct encapsw ipe4_encapsw6 = {
	.encapsw6 = {
		.pr_input = ip4_input6,
		.pr_ctlinput = NULL,
	}
};
#endif

/*
 * Check the encapsulated packet to see if we want it
 */
static int
ipe4_encapcheck(struct mbuf *m,
    int off,
    int proto,
    void *arg
)
{
	/*
	 * Only take packets coming from IPSEC tunnels; the rest
	 * must be handled by the gif tunnel code.  Note that we
	 * also return a minimum priority when we want the packet
	 * so any explicit gif tunnels take precedence.
	 */
	return ((m->m_flags & M_IPSEC) != 0 ? 1 : 0);
}

void
ipe4_attach(void)
{

	ipipstat_percpu = percpu_alloc(sizeof(uint64_t) * IPIP_NSTATS);

	xform_register(&ipe4_xformsw);
	/* attach to encapsulation framework */
	/* XXX save return cookie for detach on module remove */

	encapinit();
	/* This function is called before ifinit(). Who else gets lock? */
	(void)encap_lock_enter();
	/* ipe4_encapsw and ipe4_encapsw must be added atomically */
#ifdef INET
	(void) encap_attach_func(AF_INET, -1,
		ipe4_encapcheck, &ipe4_encapsw, NULL);
#endif
#ifdef INET6
	(void) encap_attach_func(AF_INET6, -1,
		ipe4_encapcheck, &ipe4_encapsw6, NULL);
#endif
	encap_lock_exit();
}
