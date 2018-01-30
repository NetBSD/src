/*	$NetBSD: esp_input.c,v 1.50.14.1 2018/01/30 22:10:56 martin Exp $	*/
/*	$KAME: esp_input.c,v 1.60 2001/09/04 08:43:19 itojun Exp $	*/

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

/*
 * RFC1827/2406 Encapsulated Security Payload.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: esp_input.c,v 1.50.14.1 2018/01/30 22:10:56 martin Exp $");

#include "opt_inet.h"
#include "opt_ipsec.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/malloc.h>
#include <sys/mbuf.h>
#include <sys/domain.h>
#include <sys/protosw.h>
#include <sys/socket.h>
#include <sys/errno.h>
#include <sys/time.h>
#include <sys/kernel.h>
#include <sys/syslog.h>

#include <net/if.h>
#include <net/route.h>
#include <net/netisr.h>
#include <sys/cpu.h>

#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include <netinet/ip_var.h>
#include <netinet/in_var.h>
#include <netinet/in_proto.h>
#include <netinet/ip_ecn.h>
#include <netinet/ip_icmp.h>

#ifdef INET6
#include <netinet/ip6.h>
#include <netinet6/ip6_var.h>
#include <netinet/icmp6.h>
#include <netinet6/ip6protosw.h>
#endif

#include <netinet6/ipsec.h>
#include <netinet6/ipsec_private.h>
#include <netinet6/ah.h>
#include <netinet6/esp.h>
#include <netkey/key.h>
#include <netkey/keydb.h>
#include <netkey/key_debug.h>

#include <net/net_osdep.h>

/*#define IPLEN_FLIPPED*/

#define ESPMAXLEN \
	(sizeof(struct esp) < sizeof(struct newesp) \
		? sizeof(struct newesp) : sizeof(struct esp))

#ifdef INET
void
esp4_init(void)
{

	ipsec4_init();
}

void
#if __STDC__
esp4_input(struct mbuf *m, ...)
#else
esp4_input(m, va_alist)
	struct mbuf *m;
	va_dcl
#endif
{
	struct ip *ip;
	struct esp *esp;
	struct esptail esptail;
	u_int32_t spi;
	struct secasvar *sav = NULL;
	size_t taillen;
	u_int16_t nxt;
	const struct esp_algorithm *algo;
	int ivlen;
	size_t hlen;
	size_t esplen;
	int s;
	va_list ap;
	int off;
	u_int16_t sport = 0;
	u_int16_t dport = 0;
#ifdef IPSEC_NAT_T
	struct m_tag *tag = NULL;
#endif

	va_start(ap, m);
	off = va_arg(ap, int);
	(void)va_arg(ap, int);		/* ignore value, advance ap */
	va_end(ap);

	/* sanity check for alignment. */
	if (off % 4 != 0 || m->m_pkthdr.len % 4 != 0) {
		ipseclog((LOG_ERR, "IPv4 ESP input: packet alignment problem "
			"(off=%d, pktlen=%d)\n", off, m->m_pkthdr.len));
		IPSEC_STATINC(IPSEC_STAT_IN_INVAL);
		goto bad;
	}

	if (m->m_len < off + ESPMAXLEN) {
		m = m_pullup(m, off + ESPMAXLEN);
		if (!m) {
			ipseclog((LOG_DEBUG,
			    "IPv4 ESP input: can't pullup in esp4_input\n"));
			IPSEC_STATINC(IPSEC_STAT_IN_INVAL);
			goto bad;
		}
	}

#ifdef IPSEC_NAT_T
	/* find the source port for NAT_T */
	if ((tag = m_tag_find(m, PACKET_TAG_IPSEC_NAT_T_PORTS, NULL)) != NULL) {
		sport = ((u_int16_t *)(tag + 1))[0];
		dport = ((u_int16_t *)(tag + 1))[1];
	}
#endif

	ip = mtod(m, struct ip *);
	esp = (struct esp *)(((u_int8_t *)ip) + off);
	hlen = ip->ip_hl << 2;

	/* find the sassoc. */
	spi = esp->esp_spi;

	if ((sav = key_allocsa(AF_INET,
	                      (void *)&ip->ip_src, (void *)&ip->ip_dst,
	                      IPPROTO_ESP, spi, sport, dport)) == 0) {
		ipseclog((LOG_WARNING,
		    "IPv4 ESP input: no key association found for spi %u\n",
		    (u_int32_t)ntohl(spi)));
		IPSEC_STATINC(IPSEC_STAT_IN_NOSA);
		goto bad;
	}
	KEYDEBUG(KEYDEBUG_IPSEC_STAMP,
		printf("DP esp4_input called to allocate SA:%p\n", sav));
	if (sav->state != SADB_SASTATE_MATURE &&
	    sav->state != SADB_SASTATE_DYING) {
		ipseclog((LOG_DEBUG,
		    "IPv4 ESP input: non-mature/dying SA found for spi %u\n",
		    (u_int32_t)ntohl(spi)));
		IPSEC_STATINC(IPSEC_STAT_IN_BADSPI);
		goto bad;
	}
	algo = esp_algorithm_lookup(sav->alg_enc);
	if (!algo) {
		ipseclog((LOG_DEBUG, "IPv4 ESP input: "
		    "unsupported encryption algorithm for spi %u\n",
		    (u_int32_t)ntohl(spi)));
		IPSEC_STATINC(IPSEC_STAT_IN_BADSPI);
		goto bad;
	}

	/* check if we have proper ivlen information */
	ivlen = sav->ivlen;
	if (ivlen < 0) {
		ipseclog((LOG_ERR, "inproper ivlen in IPv4 ESP input: %s %s\n",
		    ipsec4_logpacketstr(ip, spi), ipsec_logsastr(sav)));
		IPSEC_STATINC(IPSEC_STAT_IN_INVAL);
		goto bad;
	}

	if (!((sav->flags & SADB_X_EXT_OLD) == 0 && sav->replay &&
	    sav->alg_auth && sav->key_auth))
		goto noreplaycheck;

	if (sav->alg_auth == SADB_X_AALG_NULL ||
	    sav->alg_auth == SADB_AALG_NONE)
		goto noreplaycheck;

	/*
	 * check for sequence number.
	 */
	if (ipsec_chkreplay(ntohl(((struct newesp *)esp)->esp_seq), sav))
		; /* okey */
	else {
		IPSEC_STATINC(IPSEC_STAT_IN_ESPREPLAY);
		ipseclog((LOG_WARNING,
		    "replay packet in IPv4 ESP input: %s %s\n",
		    ipsec4_logpacketstr(ip, spi), ipsec_logsastr(sav)));
		goto bad;
	}

	/* check ICV */
    {
	u_int8_t sum0[AH_MAXSUMSIZE];
	u_int8_t sum[AH_MAXSUMSIZE];
	const struct ah_algorithm *sumalgo;
	size_t siz;

	sumalgo = ah_algorithm_lookup(sav->alg_auth);
	if (!sumalgo)
		goto noreplaycheck;
	siz = (((*sumalgo->sumsiz)(sav) + 3) & ~(4 - 1));
	if (m->m_pkthdr.len < off + ESPMAXLEN + siz) {
		IPSEC_STATINC(IPSEC_STAT_IN_INVAL);
		goto bad;
	}
	if (AH_MAXSUMSIZE < siz) {
		ipseclog((LOG_DEBUG,
		    "internal error: AH_MAXSUMSIZE must be larger than %lu\n",
		    (u_long)siz));
		IPSEC_STATINC(IPSEC_STAT_IN_INVAL);
		goto bad;
	}

	m_copydata(m, m->m_pkthdr.len - siz, siz, (void *)&sum0[0]);

	if (esp_auth(m, off, m->m_pkthdr.len - off - siz, sav, sum)) {
		ipseclog((LOG_WARNING, "auth fail in IPv4 ESP input: %s %s\n",
		    ipsec4_logpacketstr(ip, spi), ipsec_logsastr(sav)));
		IPSEC_STATINC(IPSEC_STAT_IN_ESPAUTHFAIL);
		goto bad;
	}

	if (memcmp(sum0, sum, siz) != 0) {
		ipseclog((LOG_WARNING, "auth fail in IPv4 ESP input: %s %s\n",
		    ipsec4_logpacketstr(ip, spi), ipsec_logsastr(sav)));
		IPSEC_STATINC(IPSEC_STAT_IN_ESPAUTHFAIL);
		goto bad;
	}

	/* strip off the authentication data */
	m_adj(m, -siz);
	ip = mtod(m, struct ip *);
#ifdef IPLEN_FLIPPED
	ip->ip_len = ip->ip_len - siz;
#else
	ip->ip_len = htons(ntohs(ip->ip_len) - siz);
#endif
	m->m_flags |= M_AUTHIPDGM;
	IPSEC_STATINC(IPSEC_STAT_IN_ESPAUTHSUCC);
    }

	/*
	 * update sequence number.
	 */
	if ((sav->flags & SADB_X_EXT_OLD) == 0 && sav->replay) {
		if (ipsec_updatereplay(ntohl(((struct newesp *)esp)->esp_seq), sav)) {
			IPSEC_STATINC(IPSEC_STAT_IN_ESPREPLAY);
			goto bad;
		}
	}

noreplaycheck:

	/* process main esp header. */
	if (sav->flags & SADB_X_EXT_OLD) {
		/* RFC 1827 */
		esplen = sizeof(struct esp);
	} else {
		/* RFC 2406 */
		if (sav->flags & SADB_X_EXT_DERIV)
			esplen = sizeof(struct esp);
		else
			esplen = sizeof(struct newesp);
	}

	if (m->m_pkthdr.len < off + esplen + ivlen + sizeof(esptail)) {
		ipseclog((LOG_WARNING,
		    "IPv4 ESP input: packet too short\n"));
		IPSEC_STATINC(IPSEC_STAT_IN_INVAL);
		goto bad;
	}

	if (m->m_len < off + esplen + ivlen) {
		m = m_pullup(m, off + esplen + ivlen);
		if (!m) {
			ipseclog((LOG_DEBUG,
			    "IPv4 ESP input: can't pullup in esp4_input\n"));
			IPSEC_STATINC(IPSEC_STAT_IN_INVAL);
			goto bad;
		}
	}

	/*
	 * pre-compute and cache intermediate key
	 */
	if (esp_schedule(algo, sav) != 0) {
		IPSEC_STATINC(IPSEC_STAT_IN_INVAL);
		goto bad;
	}

	/*
	 * decrypt the packet.
	 */
	if (!algo->decrypt)
		panic("internal error: no decrypt function");
	if ((*algo->decrypt)(m, off, sav, algo, ivlen)) {
		/* m is already freed */
		m = NULL;
		ipseclog((LOG_ERR, "decrypt fail in IPv4 ESP input: %s\n",
		    ipsec_logsastr(sav)));
		IPSEC_STATINC(IPSEC_STAT_IN_INVAL);
		goto bad;
	}
	IPSEC_STATINC(IPSEC_STAT_IN_ESPHIST + sav->alg_enc);

	m->m_flags |= M_DECRYPTED;

	/*
	 * find the trailer of the ESP.
	 */
	m_copydata(m, m->m_pkthdr.len - sizeof(esptail), sizeof(esptail),
	     (void *)&esptail);
	nxt = esptail.esp_nxt;
	taillen = esptail.esp_padlen + sizeof(esptail);

	if (m->m_pkthdr.len < taillen ||
	    m->m_pkthdr.len - taillen < off + esplen + ivlen + sizeof(esptail)) {
		ipseclog((LOG_WARNING,
		    "bad pad length in IPv4 ESP input: %s %s\n",
		    ipsec4_logpacketstr(ip, spi), ipsec_logsastr(sav)));
		IPSEC_STATINC(IPSEC_STAT_IN_INVAL);
		goto bad;
	}

	/* strip off the trailing pad area. */
	m_adj(m, -taillen);

#ifdef IPLEN_FLIPPED
	ip->ip_len = ip->ip_len - taillen;
#else
	ip->ip_len = htons(ntohs(ip->ip_len) - taillen);
#endif

	/* was it transmitted over the IPsec tunnel SA? */
	if (ipsec4_tunnel_validate(ip, nxt, sav)) {
		/*
		 * strip off all the headers that precedes ESP header.
		 *	IP4 xx ESP IP4' payload -> IP4' payload
		 *
		 * XXX more sanity checks
		 * XXX relationship with gif?
		 */
		u_int8_t tos;

		tos = ip->ip_tos;
		m_adj(m, off + esplen + ivlen);
		if (m->m_len < sizeof(*ip)) {
			m = m_pullup(m, sizeof(*ip));
			if (!m) {
				IPSEC_STATINC(IPSEC_STAT_IN_INVAL);
				goto bad;
			}
		}
		ip = mtod(m, struct ip *);
		/* ECN consideration. */
		ip_ecn_egress(ip4_ipsec_ecn, &tos, &ip->ip_tos);
		if (!key_checktunnelsanity(sav, AF_INET,
			    (void *)&ip->ip_src, (void *)&ip->ip_dst)) {
			ipseclog((LOG_ERR, "ipsec tunnel address mismatch "
			    "in IPv4 ESP input: %s %s\n",
			    ipsec4_logpacketstr(ip, spi), ipsec_logsastr(sav)));
			IPSEC_STATINC(IPSEC_STAT_IN_INVAL);
			goto bad;
		}

		key_sa_recordxfer(sav, m);
		if (ipsec_addhist(m, IPPROTO_ESP, spi) != 0 ||
		    ipsec_addhist(m, IPPROTO_IPV4, 0) != 0) {
			IPSEC_STATINC(IPSEC_STAT_IN_NOMEM);
			goto bad;
		}

		s = splnet();
		if (IF_QFULL(&ipintrq)) {
			IPSEC_STATINC(IPSEC_STAT_IN_INVAL);
			splx(s);
			goto bad;
		}
		IF_ENQUEUE(&ipintrq, m);
		m = NULL;
		schednetisr(NETISR_IP); /* can be skipped but to make sure */
		splx(s);
		nxt = IPPROTO_DONE;
	} else {
		/*
		 * strip off ESP header and IV.
		 * even in m_pulldown case, we need to strip off ESP so that
		 * we can always compute checksum for AH correctly.
		 */
		size_t stripsiz;

		stripsiz = esplen + ivlen;

		ip = mtod(m, struct ip *);
		ovbcopy((void *)ip, (void *)(((u_char *)ip) + stripsiz), off);
		m->m_data += stripsiz;
		m->m_len -= stripsiz;
		m->m_pkthdr.len -= stripsiz;

		ip = mtod(m, struct ip *);
#ifdef IPLEN_FLIPPED
		ip->ip_len = ip->ip_len - stripsiz;
#else
		ip->ip_len = htons(ntohs(ip->ip_len) - stripsiz);
#endif
		ip->ip_p = nxt;

		key_sa_recordxfer(sav, m);
		if (ipsec_addhist(m, IPPROTO_ESP, spi) != 0) {
			IPSEC_STATINC(IPSEC_STAT_IN_NOMEM);
			goto bad;
		}

		if (nxt != IPPROTO_DONE) {
			if ((inetsw[ip_protox[nxt]].pr_flags & PR_LASTHDR) != 0 &&
			    ipsec4_in_reject(m, NULL)) {
				IPSEC_STATINC(IPSEC_STAT_IN_POLVIO);
				goto bad;
			}
			(*inetsw[ip_protox[nxt]].pr_input)(m, off, nxt);
		} else
			m_freem(m);
		m = NULL;
	}

	if (sav) {
		KEYDEBUG(KEYDEBUG_IPSEC_STAMP,
			printf("DP esp4_input call free SA:%p\n", sav));
		key_freesav(sav);
	}
	IPSEC_STATINC(IPSEC_STAT_IN_SUCCESS);
	return;

bad:
	if (sav) {
		KEYDEBUG(KEYDEBUG_IPSEC_STAMP,
			printf("DP esp4_input call free SA:%p\n", sav));
		key_freesav(sav);
	}
	if (m)
		m_freem(m);
	return;
}

/* assumes that ip header and esp header are contiguous on mbuf */
void *
esp4_ctlinput(int cmd, const struct sockaddr *sa, void *v)
{
	struct ip *ip = v;
	struct esp *esp;
	struct icmp *icp;
	struct secasvar *sav;

	if (sa->sa_family != AF_INET ||
	    sa->sa_len != sizeof(struct sockaddr_in))
		return NULL;
	if ((unsigned)cmd >= PRC_NCMDS)
		return NULL;
	if (cmd == PRC_MSGSIZE && ip_mtudisc && ip && ip->ip_v == 4) {
		/*
		 * Check to see if we have a valid SA corresponding to
		 * the address in the ICMP message payload.
		 */
		esp = (struct esp *)((char *)ip + (ip->ip_hl << 2));
		if ((sav = key_allocsa(AF_INET,
				       (void *) &ip->ip_src,
				       (void *) &ip->ip_dst,
				       IPPROTO_ESP, esp->esp_spi,
				       0, 0)) == NULL)
			return NULL;
		if (sav->state != SADB_SASTATE_MATURE &&
		    sav->state != SADB_SASTATE_DYING) {
			key_freesav(sav);
			return NULL;
		}

		/* XXX Further validation? */

		key_freesav(sav);

		/*
		 * Now that we've validated that we are actually communicating
		 * with the host indicated in the ICMP message, locate the
		 * ICMP header, recalculate the new MTU, and create the
		 * corresponding routing entry.
		 */
		icp = (struct icmp *)((char *)ip -
		    offsetof(struct icmp, icmp_ip));
		icmp_mtudisc(icp, ip->ip_dst);

		return NULL;
	}

	return NULL;
}

#endif /* INET */

#ifdef INET6
void
esp6_init(void)
{

	ipsec6_init();
}

int
esp6_input(struct mbuf **mp, int *offp, int proto)
{
	struct mbuf *m = *mp;
	int off = *offp;
	struct ip6_hdr *ip6;
	struct esp *esp;
	struct esptail esptail;
	u_int32_t spi;
	struct secasvar *sav = NULL;
	size_t taillen;
	u_int16_t nxt;
	const struct esp_algorithm *algo;
	int ivlen;
	size_t esplen;
	int s;

	/* sanity check for alignment. */
	if (off % 4 != 0 || m->m_pkthdr.len % 4 != 0) {
		ipseclog((LOG_ERR, "IPv6 ESP input: packet alignment problem "
			"(off=%d, pktlen=%d)\n", off, m->m_pkthdr.len));
		IPSEC6_STATINC(IPSEC_STAT_IN_INVAL);
		goto bad;
	}

	IP6_EXTHDR_GET(esp, struct esp *, m, off, ESPMAXLEN);
	if (esp == NULL) {
		IPSEC6_STATINC(IPSEC_STAT_IN_INVAL);
		return IPPROTO_DONE;
	}
	ip6 = mtod(m, struct ip6_hdr *);

	if (ntohs(ip6->ip6_plen) == 0) {
		ipseclog((LOG_ERR, "IPv6 ESP input: "
		    "ESP with IPv6 jumbogram is not supported.\n"));
		IPSEC6_STATINC(IPSEC_STAT_IN_INVAL);
		goto bad;
	}

	/* find the sassoc. */
	spi = esp->esp_spi;

	if ((sav = key_allocsa(AF_INET6,
	                      (void *)&ip6->ip6_src, (void *)&ip6->ip6_dst,
	                      IPPROTO_ESP, spi, 0, 0)) == 0) {
		ipseclog((LOG_WARNING,
		    "IPv6 ESP input: no key association found for spi %u\n",
		    (u_int32_t)ntohl(spi)));
		IPSEC6_STATINC(IPSEC_STAT_IN_NOSA);
		goto bad;
	}
	KEYDEBUG(KEYDEBUG_IPSEC_STAMP,
		printf("DP esp6_input called to allocate SA:%p\n", sav));
	if (sav->state != SADB_SASTATE_MATURE &&
	    sav->state != SADB_SASTATE_DYING) {
		ipseclog((LOG_DEBUG,
		    "IPv6 ESP input: non-mature/dying SA found for spi %u\n",
		    (u_int32_t)ntohl(spi)));
		IPSEC6_STATINC(IPSEC_STAT_IN_BADSPI);
		goto bad;
	}
	algo = esp_algorithm_lookup(sav->alg_enc);
	if (!algo) {
		ipseclog((LOG_DEBUG, "IPv6 ESP input: "
		    "unsupported encryption algorithm for spi %u\n",
		    (u_int32_t)ntohl(spi)));
		IPSEC6_STATINC(IPSEC_STAT_IN_BADSPI);
		goto bad;
	}

	/* check if we have proper ivlen information */
	ivlen = sav->ivlen;
	if (ivlen < 0) {
		ipseclog((LOG_ERR, "inproper ivlen in IPv6 ESP input: %s %s\n",
		    ipsec6_logpacketstr(ip6, spi), ipsec_logsastr(sav)));
		IPSEC6_STATINC(IPSEC_STAT_IN_BADSPI);
		goto bad;
	}

	if (!((sav->flags & SADB_X_EXT_OLD) == 0 && sav->replay &&
	    sav->alg_auth && sav->key_auth))
		goto noreplaycheck;

	if (sav->alg_auth == SADB_X_AALG_NULL ||
	    sav->alg_auth == SADB_AALG_NONE)
		goto noreplaycheck;

	/*
	 * check for sequence number.
	 */
	if (ipsec_chkreplay(ntohl(((struct newesp *)esp)->esp_seq), sav))
		; /* okey */
	else {
		IPSEC6_STATINC(IPSEC_STAT_IN_ESPREPLAY);
		ipseclog((LOG_WARNING,
		    "replay packet in IPv6 ESP input: %s %s\n",
		    ipsec6_logpacketstr(ip6, spi), ipsec_logsastr(sav)));
		goto bad;
	}

	/* check ICV */
    {
	u_char sum0[AH_MAXSUMSIZE];
	u_char sum[AH_MAXSUMSIZE];
	const struct ah_algorithm *sumalgo;
	size_t siz;

	sumalgo = ah_algorithm_lookup(sav->alg_auth);
	if (!sumalgo)
		goto noreplaycheck;
	siz = (((*sumalgo->sumsiz)(sav) + 3) & ~(4 - 1));
	if (m->m_pkthdr.len < off + ESPMAXLEN + siz) {
		IPSEC6_STATINC(IPSEC_STAT_IN_INVAL);
		goto bad;
	}
	if (AH_MAXSUMSIZE < siz) {
		ipseclog((LOG_DEBUG,
		    "internal error: AH_MAXSUMSIZE must be larger than %lu\n",
		    (u_long)siz));
		IPSEC6_STATINC(IPSEC_STAT_IN_INVAL);
		goto bad;
	}

	m_copydata(m, m->m_pkthdr.len - siz, siz, (void *)&sum0[0]);

	if (esp_auth(m, off, m->m_pkthdr.len - off - siz, sav, sum)) {
		ipseclog((LOG_WARNING, "auth fail in IPv6 ESP input: %s %s\n",
		    ipsec6_logpacketstr(ip6, spi), ipsec_logsastr(sav)));
		IPSEC6_STATINC(IPSEC_STAT_IN_ESPAUTHFAIL);
		goto bad;
	}

	if (memcmp(sum0, sum, siz) != 0) {
		ipseclog((LOG_WARNING, "auth fail in IPv6 ESP input: %s %s\n",
		    ipsec6_logpacketstr(ip6, spi), ipsec_logsastr(sav)));
		IPSEC6_STATINC(IPSEC_STAT_IN_ESPAUTHFAIL);
		goto bad;
	}

	/* strip off the authentication data */
	m_adj(m, -siz);
	ip6 = mtod(m, struct ip6_hdr *);
	ip6->ip6_plen = htons(ntohs(ip6->ip6_plen) - siz);

	m->m_flags |= M_AUTHIPDGM;
	IPSEC6_STATINC(IPSEC_STAT_IN_ESPAUTHSUCC);
    }

	/*
	 * update sequence number.
	 */
	if ((sav->flags & SADB_X_EXT_OLD) == 0 && sav->replay) {
		if (ipsec_updatereplay(ntohl(((struct newesp *)esp)->esp_seq), sav)) {
			IPSEC6_STATINC(IPSEC_STAT_IN_ESPREPLAY);
			goto bad;
		}
	}

noreplaycheck:

	/* process main esp header. */
	if (sav->flags & SADB_X_EXT_OLD) {
		/* RFC 1827 */
		esplen = sizeof(struct esp);
	} else {
		/* RFC 2406 */
		if (sav->flags & SADB_X_EXT_DERIV)
			esplen = sizeof(struct esp);
		else
			esplen = sizeof(struct newesp);
	}

	if (m->m_pkthdr.len < off + esplen + ivlen + sizeof(esptail)) {
		ipseclog((LOG_WARNING,
		    "IPv6 ESP input: packet too short\n"));
		IPSEC6_STATINC(IPSEC_STAT_IN_INVAL);
		goto bad;
	}

	IP6_EXTHDR_GET(esp, struct esp *, m, off, esplen + ivlen);
	if (esp == NULL) {
		IPSEC6_STATINC(IPSEC_STAT_IN_INVAL);
		m = NULL;
		goto bad;
	}
	ip6 = mtod(m, struct ip6_hdr *);	/* set it again just in case */

	/*
	 * pre-compute and cache intermediate key
	 */
	if (esp_schedule(algo, sav) != 0) {
		IPSEC6_STATINC(IPSEC_STAT_IN_INVAL);
		goto bad;
	}

	/*
	 * decrypt the packet.
	 */
	if (!algo->decrypt)
		panic("internal error: no decrypt function");
	if ((*algo->decrypt)(m, off, sav, algo, ivlen)) {
		/* m is already freed */
		m = NULL;
		ipseclog((LOG_ERR, "decrypt fail in IPv6 ESP input: %s\n",
		    ipsec_logsastr(sav)));
		IPSEC6_STATINC(IPSEC_STAT_IN_INVAL);
		goto bad;
	}
	IPSEC6_STATINC(IPSEC_STAT_IN_ESPHIST + sav->alg_enc);

	m->m_flags |= M_DECRYPTED;

	/*
	 * find the trailer of the ESP.
	 */
	m_copydata(m, m->m_pkthdr.len - sizeof(esptail), sizeof(esptail),
	     (void *)&esptail);
	nxt = esptail.esp_nxt;
	taillen = esptail.esp_padlen + sizeof(esptail);

	if (m->m_pkthdr.len < taillen
	 || m->m_pkthdr.len - taillen < sizeof(struct ip6_hdr)) {	/* ? */
		ipseclog((LOG_WARNING,
		    "bad pad length in IPv6 ESP input: %s %s\n",
		    ipsec6_logpacketstr(ip6, spi), ipsec_logsastr(sav)));
		IPSEC6_STATINC(IPSEC_STAT_IN_INVAL);
		goto bad;
	}

	/* strip off the trailing pad area. */
	m_adj(m, -taillen);

	ip6->ip6_plen = htons(ntohs(ip6->ip6_plen) - taillen);

	/* was it transmitted over the IPsec tunnel SA? */
	if (ipsec6_tunnel_validate(ip6, nxt, sav)) {
		/*
		 * strip off all the headers that precedes ESP header.
		 *	IP6 xx ESP IP6' payload -> IP6' payload
		 *
		 * XXX more sanity checks
		 * XXX relationship with gif?
		 */
		u_int32_t flowinfo;	/* net endian */
		flowinfo = ip6->ip6_flow;
		m_adj(m, off + esplen + ivlen);
		if (m->m_len < sizeof(*ip6)) {
			m = m_pullup(m, sizeof(*ip6));
			if (!m) {
				IPSEC6_STATINC(IPSEC_STAT_IN_INVAL);
				goto bad;
			}
		}
		ip6 = mtod(m, struct ip6_hdr *);
		/* ECN consideration. */
		ip6_ecn_egress(ip6_ipsec_ecn, &flowinfo, &ip6->ip6_flow);
		if (!key_checktunnelsanity(sav, AF_INET6,
			    (void *)&ip6->ip6_src, (void *)&ip6->ip6_dst)) {
			ipseclog((LOG_ERR, "ipsec tunnel address mismatch "
			    "in IPv6 ESP input: %s %s\n",
			    ipsec6_logpacketstr(ip6, spi),
			    ipsec_logsastr(sav)));
			IPSEC6_STATINC(IPSEC_STAT_IN_INVAL);
			goto bad;
		}

		key_sa_recordxfer(sav, m);
		if (ipsec_addhist(m, IPPROTO_ESP, spi) != 0 ||
		    ipsec_addhist(m, IPPROTO_IPV6, 0) != 0) {
			IPSEC6_STATINC(IPSEC_STAT_IN_NOMEM);
			goto bad;
		}

		s = splnet();
		if (IF_QFULL(&ip6intrq)) {
			IPSEC6_STATINC(IPSEC_STAT_IN_INVAL);
			splx(s);
			goto bad;
		}
		IF_ENQUEUE(&ip6intrq, m);
		m = NULL;
		schednetisr(NETISR_IPV6); /* can be skipped but to make sure */
		splx(s);
		nxt = IPPROTO_DONE;
	} else {
		/*
		 * strip off ESP header and IV.
		 * even in m_pulldown case, we need to strip off ESP so that
		 * we can always compute checksum for AH correctly.
		 */
		size_t stripsiz;
		u_int8_t *prvnxtp;

		/*
		 * Set the next header field of the previous header correctly.
		 */
		const int prvnxt = ip6_get_prevhdr(m, off);
		prvnxtp = (mtod(m, u_int8_t *) + prvnxt); /* XXX */
		*prvnxtp = nxt;

		stripsiz = esplen + ivlen;

		ip6 = mtod(m, struct ip6_hdr *);
		if (m->m_len >= stripsiz + off) {
			(void)memmove((char *)ip6 + stripsiz, ip6, off);
			m->m_data += stripsiz;
			m->m_len -= stripsiz;
			m->m_pkthdr.len -= stripsiz;
		} else {
			/*
			 * this comes with no copy if the boundary is on
			 * cluster
			 */
			struct mbuf *n;

			n = m_split(m, off, M_DONTWAIT);
			if (n == NULL) {
				/* m is retained by m_split */
				goto bad;
			}
			m_adj(n, stripsiz);
			/* m_cat does not update m_pkthdr.len */
			m->m_pkthdr.len += n->m_pkthdr.len;
			m_cat(m, n);
		}

		ip6 = mtod(m, struct ip6_hdr *);
		ip6->ip6_plen = htons(ntohs(ip6->ip6_plen) - stripsiz);

		key_sa_recordxfer(sav, m);
		if (ipsec_addhist(m, IPPROTO_ESP, spi) != 0) {
			IPSEC6_STATINC(IPSEC_STAT_IN_NOMEM);
			goto bad;
		}
	}

	*offp = off;
	*mp = m;

	if (sav) {
		KEYDEBUG(KEYDEBUG_IPSEC_STAMP,
			printf("DP esp6_input call free SA:%p\n", sav));
		key_freesav(sav);
	}
	IPSEC6_STATINC(IPSEC_STAT_IN_SUCCESS);
	return nxt;

bad:
	if (sav) {
		KEYDEBUG(KEYDEBUG_IPSEC_STAMP,
			printf("DP esp6_input call free SA:%p\n", sav));
		key_freesav(sav);
	}
	if (m)
		m_freem(m);
	return IPPROTO_DONE;
}

void *
esp6_ctlinput(int cmd, const struct sockaddr *sa, void *d)
{
	const struct newesp *espp;
	struct newesp esp;
	struct ip6ctlparam *ip6cp = NULL, ip6cp1;
	struct secasvar *sav;
	struct ip6_hdr *ip6;
	struct mbuf *m;
	int off;
	const struct sockaddr_in6 *sa6_src, *sa6_dst;

	if (sa->sa_family != AF_INET6 ||
	    sa->sa_len != sizeof(struct sockaddr_in6))
		return NULL;
	if ((unsigned)cmd >= PRC_NCMDS)
		return NULL;

	/* if the parameter is from icmp6, decode it. */
	if (d != NULL) {
		ip6cp = (struct ip6ctlparam *)d;
		m = ip6cp->ip6c_m;
		ip6 = ip6cp->ip6c_ip6;
		off = ip6cp->ip6c_off;
	} else {
		m = NULL;
		ip6 = NULL;
		off = 0;
	}

	if (ip6) {
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
		pfctlinput2(cmd, sa, (void *)&ip6cp1);

		/*
		 * Then go to special cases that need ESP header information.
		 * XXX: We assume that when ip6 is non NULL,
		 * M and OFF are valid.
		 */

		/* check if we can safely examine src and dst ports */
		if (m->m_pkthdr.len < off + sizeof(esp))
			return NULL;

		if (m->m_len < off + sizeof(esp)) {
			/*
			 * this should be rare case,
			 * so we compromise on this copy...
			 */
			m_copydata(m, off, sizeof(esp), (void *)&esp);
			espp = &esp;
		} else
			espp = (struct newesp*)(mtod(m, char *) + off);

		if (cmd == PRC_MSGSIZE) {
			int valid = 0;

			/*
			 * Check to see if we have a valid SA corresponding to
			 * the address in the ICMP message payload.
			 */
			sa6_src = ip6cp->ip6c_src;
			sa6_dst = (const struct sockaddr_in6 *)sa;
			sav = key_allocsa(AF_INET6,
					  (const void *)&sa6_src->sin6_addr,
					  (const void *)&sa6_dst->sin6_addr,
					  IPPROTO_ESP, espp->esp_spi, 0, 0);
			if (sav) {
				if (sav->state == SADB_SASTATE_MATURE ||
				    sav->state == SADB_SASTATE_DYING)
					valid++;
				key_freesav(sav);
			}

			/* XXX Further validation? */

			/*
			 * Depending on the value of "valid" and routing table
			 * size (mtudisc_{hi,lo}wat), we will:
			 * - recalcurate the new MTU and create the
			 *   corresponding routing entry, or
			 * - ignore the MTU change notification.
			 */
			icmp6_mtudisc_update((struct ip6ctlparam *)d, valid);
		}
	} else {
		/* we normally notify any pcb here */
	}

	return NULL;
}
#endif /* INET6 */
