/*	$NetBSD: ipsec_input.c,v 1.76 2022/05/19 19:18:03 christos Exp $	*/
/*	$FreeBSD: ipsec_input.c,v 1.2.4.2 2003/03/28 20:32:53 sam Exp $	*/
/*	$OpenBSD: ipsec_input.c,v 1.63 2003/02/20 18:35:43 deraadt Exp $	*/

/*
 * The authors of this code are John Ioannidis (ji@tla.org),
 * Angelos D. Keromytis (kermit@csd.uch.gr) and
 * Niels Provos (provos@physnet.uni-hamburg.de).
 *
 * This code was written by John Ioannidis for BSD/OS in Athens, Greece,
 * in November 1995.
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
__KERNEL_RCSID(0, "$NetBSD: ipsec_input.c,v 1.76 2022/05/19 19:18:03 christos Exp $");

/*
 * IPsec input processing.
 */

#if defined(_KERNEL_OPT)
#include "opt_inet.h"
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
#include <netinet/in_proto.h>
#include <netinet/udp.h>
#include <netinet/tcp.h>

#include <netinet/ip6.h>
#ifdef INET6
#include <netinet6/in6.h>
#include <netinet6/ip6_var.h>
#include <netinet6/ip6_private.h>
#include <netinet6/scope6_var.h>
#endif
#include <netinet/in_pcb.h>

#include <netipsec/ipsec.h>
#include <netipsec/ipsec_private.h>
#ifdef INET6
#include <netipsec/ipsec6.h>
#endif
#include <netipsec/ah_var.h>
#include <netipsec/esp.h>
#include <netipsec/esp_var.h>
#include <netipsec/ipcomp_var.h>

#include <netipsec/key.h>
#include <netipsec/keydb.h>

#include <netipsec/xform.h>
#include <netinet6/ip6protosw.h>

#define	IPSEC_ISTAT(p, x, y, z)						\
do {									\
	switch (p) {							\
	case IPPROTO_ESP:						\
		ESP_STATINC(x);						\
		break;							\
	case IPPROTO_AH:						\
		AH_STATINC(y);						\
		break;							\
	default:							\
		IPCOMP_STATINC(z);					\
		break;							\
	}								\
} while (/*CONSTCOND*/0)

/*
 * fixup TCP/UDP checksum
 *
 * XXX: if we have NAT-OA payload from IKE server,
 *      we must do the differential update of checksum.
 *
 * XXX: NAT-OAi/NAT-OAr drived from IKE initiator/responder.
 *      how to know the IKE side from kernel?
 */
static struct mbuf *
ipsec4_fixup_checksum(struct mbuf *m)
{
	struct ip *ip;
	struct tcphdr *th;
	struct udphdr *uh;
	int poff, off;
	int plen;

	if (m->m_len < sizeof(*ip)) {
		m = m_pullup(m, sizeof(*ip));
		if (m == NULL)
			return NULL;
	}
	ip = mtod(m, struct ip *);
	poff = ip->ip_hl << 2;
	plen = ntohs(ip->ip_len) - poff;

	switch (ip->ip_p) {
	case IPPROTO_TCP:
		M_REGION_GET(th, struct tcphdr *, m, poff, sizeof(*th));
		if (th == NULL)
			return NULL;
		off = th->th_off << 2;
		if (off < sizeof(*th) || off > plen) {
			m_freem(m);
			return NULL;
		}
		th->th_sum = 0;
		th->th_sum = in4_cksum(m, IPPROTO_TCP, poff, plen);
		break;
	case IPPROTO_UDP:
		M_REGION_GET(uh, struct udphdr *, m, poff, sizeof(*uh));
		if (uh == NULL)
			return NULL;
		off = sizeof(*uh);
		if (off > plen) {
			m_freem(m);
			return NULL;
		}
		uh->uh_sum = 0;
		uh->uh_sum = in4_cksum(m, IPPROTO_UDP, poff, plen);
		break;
	default:
		/* no checksum */
		return m;
	}

	return m;
}

static void
nat_t_ports_get(struct mbuf *m, uint16_t *dport, uint16_t *sport)
{
	struct m_tag *tag;

	if ((tag = m_tag_find(m, PACKET_TAG_IPSEC_NAT_T_PORTS))) {
		*sport = ((uint16_t *)(tag + 1))[0];
		*dport = ((uint16_t *)(tag + 1))[1];
	} else
		*sport = *dport = 0;
}

static uint32_t
spi_get(struct mbuf *m, int sproto, int skip)
{
	uint32_t spi;
	uint16_t cpi;

	switch (sproto) {
	case IPPROTO_ESP:
		m_copydata(m, skip, sizeof(spi), &spi);
		return spi;
	case IPPROTO_AH:
		m_copydata(m, skip + sizeof(spi), sizeof(spi), &spi);
		return spi;
	case IPPROTO_IPCOMP:
		m_copydata(m, skip + sizeof(cpi), sizeof(cpi), &cpi);
		return htonl(ntohs(cpi));
	default:
		panic("%s called with bad protocol number: %d\n", __func__,
		    sproto);
	}
}


/*
 * ipsec_common_input gets called when an IPsec-protected packet
 * is received by IPv4 or IPv6.  Its job is to find the right SA
 * and call the appropriate transform.  The transform callback
 * takes care of further processing (like ingress filtering).
 */
static int
ipsec_common_input(struct mbuf *m, int skip, int protoff, int af, int sproto)
{
	char buf[IPSEC_ADDRSTRLEN];
	union sockaddr_union dst_address;
	struct secasvar *sav;
	u_int32_t spi;
	u_int16_t sport;
	u_int16_t dport;
	int s, error;

	IPSEC_ISTAT(sproto, ESP_STAT_INPUT, AH_STAT_INPUT,
		IPCOMP_STAT_INPUT);

	KASSERT(m != NULL);

	if ((sproto == IPPROTO_ESP && !esp_enable) ||
	    (sproto == IPPROTO_AH && !ah_enable) ||
	    (sproto == IPPROTO_IPCOMP && !ipcomp_enable)) {
		m_freem(m);
		IPSEC_ISTAT(sproto, ESP_STAT_PDROPS, AH_STAT_PDROPS,
		    IPCOMP_STAT_PDROPS);
		return EOPNOTSUPP;
	}

	if (m->m_pkthdr.len - skip < 2 * sizeof(u_int32_t)) {
		m_freem(m);
		IPSEC_ISTAT(sproto, ESP_STAT_HDROPS, AH_STAT_HDROPS,
		    IPCOMP_STAT_HDROPS);
		IPSECLOG(LOG_DEBUG, "packet too small\n");
		return EINVAL;
	}

	/* Retrieve the SPI from the relevant IPsec header */
	spi = spi_get(m, sproto, skip);

	/* find the source port for NAT-T */
	nat_t_ports_get(m, &dport, &sport);

	/*
	 * Find the SA and (indirectly) call the appropriate
	 * kernel crypto routine. The resulting mbuf chain is a valid
	 * IP packet ready to go through input processing.
	 */
	memset(&dst_address, 0, sizeof(dst_address));
	dst_address.sa.sa_family = af;
	switch (af) {
#ifdef INET
	case AF_INET:
		dst_address.sin.sin_len = sizeof(struct sockaddr_in);
		m_copydata(m, offsetof(struct ip, ip_dst),
		    sizeof(struct in_addr),
		    &dst_address.sin.sin_addr);
		break;
#endif
#ifdef INET6
	case AF_INET6:
		dst_address.sin6.sin6_len = sizeof(struct sockaddr_in6);
		m_copydata(m, offsetof(struct ip6_hdr, ip6_dst),
		    sizeof(struct in6_addr),
		    &dst_address.sin6.sin6_addr);
		if (sa6_recoverscope(&dst_address.sin6)) {
			m_freem(m);
			return EINVAL;
		}
		break;
#endif
	default:
		IPSECLOG(LOG_DEBUG, "unsupported protocol family %u\n", af);
		m_freem(m);
		IPSEC_ISTAT(sproto, ESP_STAT_NOPF, AH_STAT_NOPF,
		    IPCOMP_STAT_NOPF);
		return EPFNOSUPPORT;
	}

	s = splsoftnet();

	/* NB: only pass dst since key_lookup_sa follows RFC2401 */
	sav = KEY_LOOKUP_SA(&dst_address, sproto, spi, sport, dport);
	if (sav == NULL) {
		IPSECLOG(LOG_DEBUG,
		    "no key association found for SA %s/%08lx/%u/%u\n",
		    ipsec_address(&dst_address, buf, sizeof(buf)),
		    (u_long) ntohl(spi), sproto, ntohs(dport));
		IPSEC_ISTAT(sproto, ESP_STAT_NOTDB, AH_STAT_NOTDB,
		    IPCOMP_STAT_NOTDB);
		splx(s);
		m_freem(m);
		return ENOENT;
	}

	KASSERT(sav->tdb_xform != NULL);

	/*
	 * Call appropriate transform and return -- callback takes care of
	 * everything else.
	 */
	error = (*sav->tdb_xform->xf_input)(m, sav, skip, protoff);
	KEY_SA_UNREF(&sav);
	splx(s);
	return error;
}

#ifdef INET
/*
 * Common input handler for IPv4 AH, ESP, and IPCOMP.
 */
void
ipsec4_common_input(struct mbuf *m, int off, int proto)
{
	(void)ipsec_common_input(m, off, offsetof(struct ip, ip_p),
	    AF_INET, proto);
}

/*
 * IPsec input callback for INET protocols.
 * This routine is called as the transform callback.
 * Takes care of filtering and other sanity checks on
 * the processed packet.
 */
int
ipsec4_common_input_cb(struct mbuf *m, struct secasvar *sav,
    int skip, int protoff)
{
	int prot, af __diagused, sproto;
	struct ip *ip;
	struct secasindex *saidx;
	int error;

	if (__predict_false(m == NULL)) {
		panic("%s: NULL mbuf", __func__);
	}
	if (__predict_false(skip < sizeof(struct ip))) {
		panic("%s: short skip", __func__);
	}

	KASSERT(sav != NULL);
	saidx = &sav->sah->saidx;
	af = saidx->dst.sa.sa_family;
	KASSERTMSG(af == AF_INET, "unexpected af %u", af);
	sproto = saidx->proto;
	KASSERTMSG(sproto == IPPROTO_ESP || sproto == IPPROTO_AH ||
	    sproto == IPPROTO_IPCOMP,
	    "unexpected security protocol %u", sproto);

	/*
	 * Update the IPv4 header. The length of the packet may have changed,
	 * so fix it, and recompute the checksum.
	 */
	if (m->m_len < skip && (m = m_pullup(m, skip)) == NULL) {
		char buf[IPSEC_ADDRSTRLEN];
cantpull:
		IPSECLOG(LOG_DEBUG,
		    "processing failed for SA %s/%08lx\n",
		    ipsec_address(&sav->sah->saidx.dst, buf,
		    sizeof(buf)), (u_long) ntohl(sav->spi));
		IPSEC_ISTAT(sproto, ESP_STAT_HDROPS, AH_STAT_HDROPS,
		    IPCOMP_STAT_HDROPS);
		error = ENOBUFS;
		goto bad;
	}
	ip = mtod(m, struct ip *);
	ip->ip_len = htons(m->m_pkthdr.len);
	ip->ip_sum = 0;
	ip->ip_sum = in_cksum(m, ip->ip_hl << 2);

	/*
	 * Update TCP/UDP checksum
	 * XXX: should only do it in NAT-T case
	 * XXX: should do it incrementally, see FreeBSD code.
	 */
	m = ipsec4_fixup_checksum(m);
	if (m == NULL)
		goto cantpull;
	ip = mtod(m, struct ip *);

	prot = ip->ip_p;

	M_VERIFY_PACKET(m);

	key_sa_recordxfer(sav, m);

	if ((inetsw[ip_protox[prot]].pr_flags & PR_LASTHDR) != 0 &&
	    ipsec_in_reject(m, NULL)) {
		error = EINVAL;
		goto bad;
	}

	/*
	 * There is no struct ifnet for tunnel mode IP-IP tunnel connecttion,
	 * so we cannot write filtering rule to the inner packet.
	 */
	if (saidx->mode == IPSEC_MODE_TUNNEL)
		m->m_pkthdr.pkthdr_flags |= PKTHDR_FLAG_IPSEC_SKIP_PFIL;

	(*inetsw[ip_protox[prot]].pr_input)(m, skip, prot);
	return 0;

bad:
	m_freem(m);
	return error;
}
#endif /* INET */

#ifdef INET6
int
ipsec6_common_input(struct mbuf **mp, int *offp, int proto)
{
	int l = 0;
	int protoff, nxt;
	struct ip6_ext ip6e;

	if (*offp < sizeof(struct ip6_hdr)) {
		IPSECLOG(LOG_DEBUG, "bad offset %u\n", *offp);
		IPSEC_ISTAT(proto, ESP_STAT_HDROPS, AH_STAT_HDROPS,
			    IPCOMP_STAT_HDROPS);
		m_freem(*mp);
		return IPPROTO_DONE;
	} else if (*offp == sizeof(struct ip6_hdr)) {
		protoff = offsetof(struct ip6_hdr, ip6_nxt);
	} else {
		/* Chase down the header chain... */
		protoff = sizeof(struct ip6_hdr);
		nxt = (mtod(*mp, struct ip6_hdr *))->ip6_nxt;

		do {
			protoff += l;
			m_copydata(*mp, protoff, sizeof(ip6e), &ip6e);

			if (nxt == IPPROTO_AH)
				l = (ip6e.ip6e_len + 2) << 2;
			else if (nxt == IPPROTO_FRAGMENT)
				l = sizeof(struct ip6_frag);
			else
				l = (ip6e.ip6e_len + 1) << 3;
			KASSERT(l > 0);

			nxt = ip6e.ip6e_nxt;
		} while (protoff + l < *offp);

		/* Malformed packet check */
		if (protoff + l != *offp) {
			IPSECLOG(LOG_DEBUG, "bad packet header chain, "
			    "protoff %u, l %u, off %u\n", protoff, l, *offp);
			IPSEC_ISTAT(proto, ESP_STAT_HDROPS,
				    AH_STAT_HDROPS,
				    IPCOMP_STAT_HDROPS);
			m_freem(*mp);
			*mp = NULL;
			return IPPROTO_DONE;
		}
		protoff += offsetof(struct ip6_ext, ip6e_nxt);
	}
	(void) ipsec_common_input(*mp, *offp, protoff, AF_INET6, proto);
	return IPPROTO_DONE;
}

/*
 * IPsec input callback, called by the transform callback. Takes care of
 * filtering and other sanity checks on the processed packet.
 */
int
ipsec6_common_input_cb(struct mbuf *m, struct secasvar *sav, int skip,
    int protoff)
{
	int af __diagused, sproto;
	struct ip6_hdr *ip6;
	struct secasindex *saidx;
	int nxt;
	u_int8_t prot;
	int error, nest;

	if (__predict_false(m == NULL)) {
		panic("%s: NULL mbuf", __func__);
	}

	KASSERT(sav != NULL);
	saidx = &sav->sah->saidx;
	af = saidx->dst.sa.sa_family;
	KASSERTMSG(af == AF_INET6, "unexpected af %u", af);
	sproto = saidx->proto;
	KASSERTMSG(sproto == IPPROTO_ESP || sproto == IPPROTO_AH ||
	    sproto == IPPROTO_IPCOMP,
	    "unexpected security protocol %u", sproto);

	/* Fix IPv6 header */
	if (m->m_len < sizeof(struct ip6_hdr) &&
	    (m = m_pullup(m, sizeof(struct ip6_hdr))) == NULL) {
		char buf[IPSEC_ADDRSTRLEN];
		IPSECLOG(LOG_DEBUG, "processing failed for SA %s/%08lx\n",
		    ipsec_address(&sav->sah->saidx.dst,
		    buf, sizeof(buf)), (u_long) ntohl(sav->spi));
		IPSEC_ISTAT(sproto, ESP_STAT_HDROPS, AH_STAT_HDROPS,
		    IPCOMP_STAT_HDROPS);
		error = EACCES;
		goto bad;
	}

	ip6 = mtod(m, struct ip6_hdr *);
	ip6->ip6_plen = htons(m->m_pkthdr.len - sizeof(struct ip6_hdr));

	m_copydata(m, protoff, sizeof(prot), &prot);

	key_sa_recordxfer(sav, m);

	/*
	 * See the end of ip6_input for this logic.
	 * IPPROTO_IPV[46] case will be processed just like other ones
	 */
	nest = 0;
	nxt = prot;
	while (nxt != IPPROTO_DONE) {
		if (ip6_hdrnestlimit && (++nest > ip6_hdrnestlimit)) {
			IP6_STATINC(IP6_STAT_TOOMANYHDR);
			error = EINVAL;
			goto bad;
		}

		M_VERIFY_PACKET(m);

		/*
		 * Protection against faulty packet - there should be
		 * more sanity checks in header chain processing.
		 */
		if (m->m_pkthdr.len < skip) {
			IP6_STATINC(IP6_STAT_TOOSHORT);
			in6_ifstat_inc(m_get_rcvif_NOMPSAFE(m),
			    ifs6_in_truncated);
			error = EINVAL;
			goto bad;
		}

		/*
		 * Enforce IPsec policy checking if we are seeing last header.
		 * Note that we do not visit this with protocols with pcb layer
		 * code - like udp/tcp/raw ip.
		 */
		if ((inet6sw[ip6_protox[nxt]].pr_flags & PR_LASTHDR) != 0 &&
		    ipsec_in_reject(m, NULL)) {
			error = EINVAL;
			goto bad;
		}

		/*
		 * There is no struct ifnet for tunnel mode IP-IP tunnel connecttion,
		 * so we cannot write filtering rule to the inner packet.
		 */
		if (saidx->mode == IPSEC_MODE_TUNNEL)
			m->m_pkthdr.pkthdr_flags |= PKTHDR_FLAG_IPSEC_SKIP_PFIL;

		nxt = (*inet6sw[ip6_protox[nxt]].pr_input)(&m, &skip, nxt);
	}
	return 0;

bad:
	if (m)
		m_freem(m);
	return error;
}
#endif /* INET6 */
