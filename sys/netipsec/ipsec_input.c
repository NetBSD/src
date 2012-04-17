/*	$NetBSD: ipsec_input.c,v 1.28.2.1 2012/04/17 00:08:46 yamt Exp $	*/
/*	$FreeBSD: /usr/local/www/cvsroot/FreeBSD/src/sys/netipsec/ipsec_input.c,v 1.2.4.2 2003/03/28 20:32:53 sam Exp $	*/
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
__KERNEL_RCSID(0, "$NetBSD: ipsec_input.c,v 1.28.2.1 2012/04/17 00:08:46 yamt Exp $");

/*
 * IPsec input processing.
 */

#include "opt_inet.h"
#ifdef __FreeBSD__
#include "opt_inet6.h"
#endif
#include "opt_ipsec.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/malloc.h>
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

#include <netinet/ip6.h>
#ifdef INET6
#include <netinet6/ip6_var.h>
#include <netinet6/ip6_private.h>
#include <netinet6/scope6_var.h>
#endif
#include <netinet/in_pcb.h>
#ifdef INET6
#include <netinet/icmp6.h>
#endif

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

#include <netipsec/ipsec_osdep.h>

#include <net/net_osdep.h>

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
 * ipsec_common_input gets called when an IPsec-protected packet
 * is received by IPv4 or IPv6.  It's job is to find the right SA
 # and call the appropriate transform.  The transform callback
 * takes care of further processing (like ingress filtering).
 */
static int
ipsec_common_input(struct mbuf *m, int skip, int protoff, int af, int sproto)
{
	union sockaddr_union dst_address;
	struct secasvar *sav;
	u_int32_t spi;
	u_int16_t sport = 0;
	u_int16_t dport = 0;
	int s, error;
#ifdef IPSEC_NAT_T
	struct m_tag * tag = NULL;
#endif

	IPSEC_ISTAT(sproto, ESP_STAT_INPUT, AH_STAT_INPUT,
		IPCOMP_STAT_INPUT);

	IPSEC_ASSERT(m != NULL, ("ipsec_common_input: null packet"));

	if ((sproto == IPPROTO_ESP && !esp_enable) ||
	    (sproto == IPPROTO_AH && !ah_enable) ||
	    (sproto == IPPROTO_IPCOMP && !ipcomp_enable)) {
		m_freem(m);
		IPSEC_ISTAT(sproto, ESP_STAT_PDROPS, AH_STAT_PDROPS,
		    IPCOMP_STAT_PDROPS);
		return EOPNOTSUPP;
	}

	if (m->m_pkthdr.len - skip < 2 * sizeof (u_int32_t)) {
		m_freem(m);
		IPSEC_ISTAT(sproto, ESP_STAT_HDROPS, AH_STAT_HDROPS,
		    IPCOMP_STAT_HDROPS);
		DPRINTF(("ipsec_common_input: packet too small\n"));
		return EINVAL;
	}

	/* Retrieve the SPI from the relevant IPsec header */
	if (sproto == IPPROTO_ESP)
		m_copydata(m, skip, sizeof(u_int32_t), &spi);
	else if (sproto == IPPROTO_AH)
		m_copydata(m, skip + sizeof(u_int32_t), sizeof(u_int32_t), &spi);
	else if (sproto == IPPROTO_IPCOMP) {
		u_int16_t cpi;
		m_copydata(m, skip + sizeof(u_int16_t), sizeof(u_int16_t), &cpi);
		spi = ntohl(htons(cpi));
	} else {
		panic("ipsec_common_input called with bad protocol number :"
		      "%d\n", sproto);
	}
		

#ifdef IPSEC_NAT_T
	/* find the source port for NAT-T */
	if ((tag = m_tag_find(m, PACKET_TAG_IPSEC_NAT_T_PORTS, NULL))) {
		sport = ((u_int16_t *)(tag + 1))[0];
		dport = ((u_int16_t *)(tag + 1))[1];
	}
#endif

	/*
	 * Find the SA and (indirectly) call the appropriate
	 * kernel crypto routine. The resulting mbuf chain is a valid
	 * IP packet ready to go through input processing.
	 */
	memset(&dst_address, 0, sizeof (dst_address));
	dst_address.sa.sa_family = af;
	switch (af) {
#ifdef INET
	case AF_INET:
		dst_address.sin.sin_len = sizeof(struct sockaddr_in);
		m_copydata(m, offsetof(struct ip, ip_dst),
		    sizeof(struct in_addr),
		    &dst_address.sin.sin_addr);
		break;
#endif /* INET */
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
#endif /* INET6 */
	default:
		DPRINTF(("ipsec_common_input: unsupported protocol "
			"family %u\n", af));
		m_freem(m);
		IPSEC_ISTAT(sproto, ESP_STAT_NOPF, AH_STAT_NOPF,
		    IPCOMP_STAT_NOPF);
		return EPFNOSUPPORT;
	}

	s = splsoftnet();

	/* NB: only pass dst since key_allocsa follows RFC2401 */
	sav = KEY_ALLOCSA(&dst_address, sproto, spi, sport, dport);
	if (sav == NULL) {
		DPRINTF(("ipsec_common_input: no key association found for"
			  " SA %s/%08lx/%u/%u\n",
			  ipsec_address(&dst_address),
			  (u_long) ntohl(spi), sproto, ntohs(dport)));
		IPSEC_ISTAT(sproto, ESP_STAT_NOTDB, AH_STAT_NOTDB,
		    IPCOMP_STAT_NOTDB);
		splx(s);
		m_freem(m);
		return ENOENT;
	}

	if (sav->tdb_xform == NULL) {
		DPRINTF(("ipsec_common_input: attempted to use uninitialized"
			 " SA %s/%08lx/%u\n",
			 ipsec_address(&dst_address),
			 (u_long) ntohl(spi), sproto));
		IPSEC_ISTAT(sproto, ESP_STAT_NOXFORM, AH_STAT_NOXFORM,
		    IPCOMP_STAT_NOXFORM);
		KEY_FREESAV(&sav);
		splx(s);
		m_freem(m);
		return ENXIO;
	}

	/*
	 * Call appropriate transform and return -- callback takes care of
	 * everything else.
	 */
	error = (*sav->tdb_xform->xf_input)(m, sav, skip, protoff);
	KEY_FREESAV(&sav);
	splx(s);
	return error;
}

#ifdef INET
/*
 * Common input handler for IPv4 AH, ESP, and IPCOMP.
 */
void
ipsec4_common_input(struct mbuf *m, ...)
{
	va_list ap;
	int off, nxt;

	va_start(ap, m);
	off = va_arg(ap, int);
	nxt = va_arg(ap, int);
	va_end(ap);

	(void) ipsec_common_input(m, off, offsetof(struct ip, ip_p),
				  AF_INET, nxt);
}

/*
 * IPsec input callback for INET protocols.
 * This routine is called as the transform callback.
 * Takes care of filtering and other sanity checks on
 * the processed packet.
 */
int
ipsec4_common_input_cb(struct mbuf *m, struct secasvar *sav,
    int skip, int protoff, struct m_tag *mt)
{
	int prot, af, sproto;
	struct ip *ip;
	struct m_tag *mtag;
	struct tdb_ident *tdbi;
	struct secasindex *saidx;
	int error;

	IPSEC_SPLASSERT_SOFTNET("ipsec4_common_input_cb");

	IPSEC_ASSERT(m != NULL, ("ipsec4_common_input_cb: null mbuf"));
	IPSEC_ASSERT(sav != NULL, ("ipsec4_common_input_cb: null SA"));
	IPSEC_ASSERT(sav->sah != NULL, ("ipsec4_common_input_cb: null SAH"));
	saidx = &sav->sah->saidx;
	af = saidx->dst.sa.sa_family;
	IPSEC_ASSERT(af == AF_INET, ("ipsec4_common_input_cb: unexpected af %u",af));
	sproto = saidx->proto;
	IPSEC_ASSERT(sproto == IPPROTO_ESP || sproto == IPPROTO_AH ||
		sproto == IPPROTO_IPCOMP,
		("ipsec4_common_input_cb: unexpected security protocol %u",
		sproto));

	/* Sanity check */
	if (m == NULL) {
		DPRINTF(("ipsec4_common_input_cb: null mbuf"));
		IPSEC_ISTAT(sproto, ESP_STAT_BADKCR, AH_STAT_BADKCR,
		    IPCOMP_STAT_BADKCR);
		KEY_FREESAV(&sav);
		return EINVAL;
	}

	/* Fix IPv4 header */
	if (m->m_len < skip && (m = m_pullup(m, skip)) == NULL) {
		DPRINTF(("ipsec4_common_input_cb: processing failed "
		    "for SA %s/%08lx\n",
		    ipsec_address(&sav->sah->saidx.dst),
		    (u_long) ntohl(sav->spi)));
		IPSEC_ISTAT(sproto, ESP_STAT_HDROPS, AH_STAT_HDROPS,
		    IPCOMP_STAT_HDROPS);
		error = ENOBUFS;
		goto bad;
	}

	ip = mtod(m, struct ip *);
	ip->ip_len = htons(m->m_pkthdr.len);
	prot = ip->ip_p;

	/* IP-in-IP encapsulation */
	if (prot == IPPROTO_IPIP) {
		struct ip ipn;

		/* ipn will now contain the inner IPv4 header */
		m_copydata(m, ip->ip_hl << 2, sizeof(struct ip), &ipn);

#ifdef notyet
		/* XXX PROXY address isn't recorded in SAH */
		/*
		 * Check that the inner source address is the same as
		 * the proxy address, if available.
		 */
		if ((saidx->proxy.sa.sa_family == AF_INET &&
		    saidx->proxy.sin.sin_addr.s_addr !=
		    INADDR_ANY &&
		    ipn.ip_src.s_addr !=
		    saidx->proxy.sin.sin_addr.s_addr) ||
		    (saidx->proxy.sa.sa_family != AF_INET &&
			saidx->proxy.sa.sa_family != 0)) {

			DPRINTF(("ipsec4_common_input_cb: inner "
			    "source address %s doesn't correspond to "
			    "expected proxy source %s, SA %s/%08lx\n",
			    inet_ntoa4(ipn.ip_src),
			    ipsp_address(saidx->proxy),
			    ipsp_address(saidx->dst),
			    (u_long) ntohl(sav->spi)));

			IPSEC_ISTAT(sproto, ESP_STAT_PDROPS,
			    AH_STAT_PDROPS,
			    IPCOMP_STAT_PDROPS);
			error = EACCES;
			goto bad;
		}
#endif /*XXX*/
	}
#if INET6
	/* IPv6-in-IP encapsulation. */
	if (prot == IPPROTO_IPV6) {
		struct ip6_hdr ip6n;

		/* ip6n will now contain the inner IPv6 header. */
		m_copydata(m, ip->ip_hl << 2, sizeof(struct ip6_hdr), &ip6n);

#ifdef notyet
		/*
		 * Check that the inner source address is the same as
		 * the proxy address, if available.
		 */
		if ((saidx->proxy.sa.sa_family == AF_INET6 &&
		    !IN6_IS_ADDR_UNSPECIFIED(&saidx->proxy.sin6.sin6_addr) &&
		    !IN6_ARE_ADDR_EQUAL(&ip6n.ip6_src,
			&saidx->proxy.sin6.sin6_addr)) ||
		    (saidx->proxy.sa.sa_family != AF_INET6 &&
			saidx->proxy.sa.sa_family != 0)) {

			DPRINTF(("ipsec4_common_input_cb: inner "
			    "source address %s doesn't correspond to "
			    "expected proxy source %s, SA %s/%08lx\n",
			    ip6_sprintf(&ip6n.ip6_src),
			    ipsec_address(&saidx->proxy),
			    ipsec_address(&saidx->dst),
			    (u_long) ntohl(sav->spi)));

			IPSEC_ISTAT(sproto, ESP_STAT_PDROPS,
			    AH_STAT_PDROPS,
			    IPCOMP_STAT_PDROPS);
			error = EACCES;
			goto bad;
		}
#endif /*XXX*/
	}
#endif /* INET6 */

	/*
	 * Record what we've done to the packet (under what SA it was
	 * processed). If we've been passed an mtag, it means the packet
	 * was already processed by an ethernet/crypto combo card and
	 * thus has a tag attached with all the right information, but
	 * with a PACKET_TAG_IPSEC_IN_CRYPTO_DONE as opposed to
	 * PACKET_TAG_IPSEC_IN_DONE type; in that case, just change the type.
	 */
	if (mt == NULL && sproto != IPPROTO_IPCOMP) {
		mtag = m_tag_get(PACKET_TAG_IPSEC_IN_DONE,
		    sizeof(struct tdb_ident), M_NOWAIT);
		if (mtag == NULL) {
			DPRINTF(("ipsec4_common_input_cb: failed to get tag\n"));
			IPSEC_ISTAT(sproto, ESP_STAT_HDROPS,
			    AH_STAT_HDROPS, IPCOMP_STAT_HDROPS);
			error = ENOMEM;
			goto bad;
		}

		tdbi = (struct tdb_ident *)(mtag + 1);
		memcpy(&tdbi->dst, &saidx->dst, saidx->dst.sa.sa_len);
		tdbi->proto = sproto;
		tdbi->spi = sav->spi;

		m_tag_prepend(m, mtag);
	} else {
		if (mt != NULL)
			mt->m_tag_id = PACKET_TAG_IPSEC_IN_DONE;
			/* XXX do we need to mark m_flags??? */
	}

	key_sa_recordxfer(sav, m);		/* record data transfer */

	if ((inetsw[ip_protox[prot]].pr_flags & PR_LASTHDR) != 0 &&
				ipsec4_in_reject(m, NULL)) {
		error = EINVAL;
		goto bad;
	}
	(*inetsw[ip_protox[prot]].pr_input)(m, skip, prot);
	return 0;
bad:
	m_freem(m);
	return error;
}
#endif /* INET */

#ifdef INET6
/* IPv6 AH wrapper. */
int
ipsec6_common_input(struct mbuf **mp, int *offp, int proto)
{
	int l = 0;
	int protoff, nxt;
	struct ip6_ext ip6e;

	if (*offp < sizeof(struct ip6_hdr)) {
		DPRINTF(("ipsec6_common_input: bad offset %u\n", *offp));
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
			else
				l = (ip6e.ip6e_len + 1) << 3;
			IPSEC_ASSERT(l > 0,
			  ("ipsec6_common_input: l went zero or negative"));

			nxt = ip6e.ip6e_nxt;
		} while (protoff + l < *offp);

		/* Malformed packet check */
		if (protoff + l != *offp) {
			DPRINTF(("ipsec6_common_input: bad packet header chain, "
				"protoff %u, l %u, off %u\n", protoff, l, *offp));
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
 * NB: ipsec_netbsd.c has a duplicate definition of esp6_ctlinput(),
 * with slightly ore recent multicast tests. These should be merged.
 * For now, ifdef accordingly.
 */
#ifdef __FreeBSD__
void
esp6_ctlinput(int cmd, struct sockaddr *sa, void *d)
{
	if (sa->sa_family != AF_INET6 ||
	    sa->sa_len != sizeof(struct sockaddr_in6))
		return;
	if ((unsigned)cmd >= PRC_NCMDS)
		return;

	/* if the parameter is from icmp6, decode it. */
	if (d !=  NULL) {
		struct ip6ctlparam *ip6cp = (struct ip6ctlparam *)d;
		struct mbuf *m = ip6cp->ip6c_m;
		int off = ip6cp->ip6c_off;

		struct ip6ctlparam ip6cp1;

		/*
		 * Notify the error to all possible sockets via pfctlinput2.
		 * Since the upper layer information (such as protocol type,
		 * source and destination ports) is embedded in the encrypted
		 * data and might have been cut, we can't directly call
		 * an upper layer ctlinput function. However, the pcbnotify
		 * function will consider source and destination addresses
		 * as well as the flow info value, and may be able to find
		 * some PCB that should be notified.
		 * Although pfctlinput2 will call esp6_ctlinput(), there is
		 * no possibility of an infinite loop of function calls,
		 * because we don't pass the inner IPv6 header.
		 */
		memset(&ip6cp1, 0, sizeof(ip6cp1));
		ip6cp1.ip6c_src = ip6cp->ip6c_src;
		pfctlinput2(cmd, sa, &ip6cp1);

		/*
		 * Then go to special cases that need ESP header information.
		 * XXX: We assume that when ip6 is non NULL,
		 * M and OFF are valid.
		 */

		if (cmd == PRC_MSGSIZE) {
			struct secasvar *sav;
			u_int32_t spi;
			int valid;

			/* check header length before using m_copydata */
			if (m->m_pkthdr.len < off + sizeof (struct esp))
				return;
			m_copydata(m, off + offsetof(struct esp, esp_spi),
				sizeof(u_int32_t), &spi);
			/*
			 * Check to see if we have a valid SA corresponding to
			 * the address in the ICMP message payload.
			 */
			sav = KEY_ALLOCSA((union sockaddr_union *)sa,
					IPPROTO_ESP, spi);
			valid = (sav != NULL);
			if (sav)
				KEY_FREESAV(&sav);

			/* XXX Further validation? */

			/*
			 * Depending on whether the SA is "valid" and
			 * routing table size (mtudisc_{hi,lo}wat), we will:
			 * - recalcurate the new MTU and create the
			 *   corresponding routing entry, or
			 * - ignore the MTU change notification.
			 */
			icmp6_mtudisc_update(ip6cp, valid);
		}
	} else {
		/* we normally notify any pcb here */
	}
}
#endif /* __FreeBSD__ */

extern	const struct ip6protosw inet6sw[];
extern	u_char ip6_protox[];

/*
 * IPsec input callback, called by the transform callback. Takes care of
 * filtering and other sanity checks on the processed packet.
 */
int
ipsec6_common_input_cb(struct mbuf *m, struct secasvar *sav, int skip, int protoff,
    struct m_tag *mt)
{
	int af, sproto;
	struct ip6_hdr *ip6;
	struct m_tag *mtag;
	struct tdb_ident *tdbi;
	struct secasindex *saidx;
	int nxt;
	u_int8_t prot, nxt8;
	int error, nest;

	IPSEC_ASSERT(m != NULL, ("ipsec6_common_input_cb: null mbuf"));
	IPSEC_ASSERT(sav != NULL, ("ipsec6_common_input_cb: null SA"));
	IPSEC_ASSERT(sav->sah != NULL, ("ipsec6_common_input_cb: null SAH"));
	saidx = &sav->sah->saidx;
	af = saidx->dst.sa.sa_family;
	IPSEC_ASSERT(af == AF_INET6,
		("ipsec6_common_input_cb: unexpected af %u", af));
	sproto = saidx->proto;
	IPSEC_ASSERT(sproto == IPPROTO_ESP || sproto == IPPROTO_AH ||
		sproto == IPPROTO_IPCOMP,
		("ipsec6_common_input_cb: unexpected security protocol %u",
		sproto));

	/* Sanity check */
	if (m == NULL) {
		DPRINTF(("ipsec6_common_input_cb: null mbuf"));
		IPSEC_ISTAT(sproto, ESP_STAT_BADKCR, AH_STAT_BADKCR,
		    IPCOMP_STAT_BADKCR);
		error = EINVAL;
		goto bad;
	}

	/* Fix IPv6 header */
	if (m->m_len < sizeof(struct ip6_hdr) &&
	    (m = m_pullup(m, sizeof(struct ip6_hdr))) == NULL) {

		DPRINTF(("ipsec6_common_input_cb: processing failed "
		    "for SA %s/%08lx\n", ipsec_address(&sav->sah->saidx.dst),
		    (u_long) ntohl(sav->spi)));

		IPSEC_ISTAT(sproto, ESP_STAT_HDROPS, AH_STAT_HDROPS,
		    IPCOMP_STAT_HDROPS);
		error = EACCES;
		goto bad;
	}

	ip6 = mtod(m, struct ip6_hdr *);
	ip6->ip6_plen = htons(m->m_pkthdr.len - sizeof(struct ip6_hdr));

	/* Save protocol */
	m_copydata(m, protoff, 1, &prot);

#ifdef INET
	/* IP-in-IP encapsulation */
	if (prot == IPPROTO_IPIP) {
		struct ip ipn;

		/* ipn will now contain the inner IPv4 header */
		m_copydata(m, skip, sizeof(struct ip), &ipn);

#ifdef notyet
		/*
		 * Check that the inner source address is the same as
		 * the proxy address, if available.
		 */
		if ((saidx->proxy.sa.sa_family == AF_INET &&
		    saidx->proxy.sin.sin_addr.s_addr != INADDR_ANY &&
		    ipn.ip_src.s_addr != saidx->proxy.sin.sin_addr.s_addr) ||
		    (saidx->proxy.sa.sa_family != AF_INET &&
			saidx->proxy.sa.sa_family != 0)) {

			DPRINTF(("ipsec6_common_input_cb: inner "
			    "source address %s doesn't correspond to "
			    "expected proxy source %s, SA %s/%08lx\n",
			    inet_ntoa4(ipn.ip_src),
			    ipsec_address(&saidx->proxy),
			    ipsec_address(&saidx->dst),
			    (u_long) ntohl(sav->spi)));

			IPSEC_ISTAT(sproto, ESP_STAT_PDROPS,
			    AH_STAT_PDROPS, IPCOMP_STAT_PDROPS);
			error = EACCES;
			goto bad;
		}
#endif /*XXX*/
	}
#endif /* INET */

	/* IPv6-in-IP encapsulation */
	if (prot == IPPROTO_IPV6) {
		struct ip6_hdr ip6n;

		/* ip6n will now contain the inner IPv6 header. */
		m_copydata(m, skip, sizeof(struct ip6_hdr), &ip6n);

#ifdef notyet
		/*
		 * Check that the inner source address is the same as
		 * the proxy address, if available.
		 */
		if ((saidx->proxy.sa.sa_family == AF_INET6 &&
		    !IN6_IS_ADDR_UNSPECIFIED(&saidx->proxy.sin6.sin6_addr) &&
		    !IN6_ARE_ADDR_EQUAL(&ip6n.ip6_src,
			&saidx->proxy.sin6.sin6_addr)) ||
		    (saidx->proxy.sa.sa_family != AF_INET6 &&
			saidx->proxy.sa.sa_family != 0)) {

			DPRINTF(("ipsec6_common_input_cb: inner "
			    "source address %s doesn't correspond to "
			    "expected proxy source %s, SA %s/%08lx\n",
			    ip6_sprintf(&ip6n.ip6_src),
			    ipsec_address(&saidx->proxy),
			    ipsec_address(&saidx->dst),
			    (u_long) ntohl(sav->spi)));

			IPSEC_ISTAT(sproto, ESP_STAT_PDROPS,
			    AH_STAT_PDROPS, IPCOMP_STAT_PDROPS);
			error = EACCES;
			goto bad;
		}
#endif /*XXX*/
	}

	/*
	 * Record what we've done to the packet (under what SA it was
	 * processed). If we've been passed an mtag, it means the packet
	 * was already processed by an ethernet/crypto combo card and
	 * thus has a tag attached with all the right information, but
	 * with a PACKET_TAG_IPSEC_IN_CRYPTO_DONE as opposed to
	 * PACKET_TAG_IPSEC_IN_DONE type; in that case, just change the type.
	 */
	if (mt == NULL && sproto != IPPROTO_IPCOMP) {
		mtag = m_tag_get(PACKET_TAG_IPSEC_IN_DONE,
		    sizeof(struct tdb_ident), M_NOWAIT);
		if (mtag == NULL) {
			DPRINTF(("ipsec_common_input_cb: failed to "
			    "get tag\n"));
			IPSEC_ISTAT(sproto, ESP_STAT_HDROPS,
			    AH_STAT_HDROPS, IPCOMP_STAT_HDROPS);
			error = ENOMEM;
			goto bad;
		}

		tdbi = (struct tdb_ident *)(mtag + 1);
		memcpy(&tdbi->dst, &saidx->dst, sizeof(union sockaddr_union));
		tdbi->proto = sproto;
		tdbi->spi = sav->spi;

		m_tag_prepend(m, mtag);
	} else {
		if (mt != NULL)
			mt->m_tag_id = PACKET_TAG_IPSEC_IN_DONE;
			/* XXX do we need to mark m_flags??? */
	}

	key_sa_recordxfer(sav, m);

	/* Retrieve new protocol */
	m_copydata(m, protoff, sizeof(u_int8_t), &nxt8);

	/*
	 * See the end of ip6_input for this logic.
	 * IPPROTO_IPV[46] case will be processed just like other ones
	 */
	nest = 0;
	nxt = nxt8;
	while (nxt != IPPROTO_DONE) {
		if (ip6_hdrnestlimit && (++nest > ip6_hdrnestlimit)) {
			IP6_STATINC(IP6_STAT_TOOMANYHDR);
			error = EINVAL;
			goto bad;
		}

		/*
		 * Protection against faulty packet - there should be
		 * more sanity checks in header chain processing.
		 */
		if (m->m_pkthdr.len < skip) {
			IP6_STATINC(IP6_STAT_TOOSHORT);
			in6_ifstat_inc(m->m_pkthdr.rcvif, ifs6_in_truncated);
			error = EINVAL;
			goto bad;
		}
		/*
		 * Enforce IPsec policy checking if we are seeing last header.
		 * note that we do not visit this with protocols with pcb layer
		 * code - like udp/tcp/raw ip.
		 */
		if ((inet6sw[ip6_protox[nxt]].pr_flags & PR_LASTHDR) != 0 &&
		    ipsec6_in_reject(m, NULL)) {
			error = EINVAL;
			goto bad;
		}
		nxt = (*inet6sw[ip6_protox[nxt]].pr_input)(&m, &skip, nxt);
	}
	return 0;
bad:
	if (m)
		m_freem(m);
	return error;
}
#endif /* INET6 */
