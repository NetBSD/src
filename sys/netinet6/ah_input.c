/*	$NetBSD: ah_input.c,v 1.3 1999/07/03 21:30:17 thorpej Exp $	*/

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
 * RFC1826/2402 authentication header.
 */

#if (defined(__FreeBSD__) && __FreeBSD__ >= 3) || defined(__NetBSD__)
#include "opt_inet.h"
#endif

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
#include <machine/cpu.h>

#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/in_var.h>
#include <netinet/ip.h>
#include <netinet/ip_var.h>
#include <netinet/ip_ecn.h>

#ifdef INET6
#include <netinet6/in6_systm.h>
#include <netinet6/ip6.h>
#if !defined(__FreeBSD__) || __FreeBSD__ < 3
#include <netinet6/in6_pcb.h>
#endif
#include <netinet6/ip6_var.h>
#include <netinet6/icmp6.h>
#endif

#include <netinet6/ipsec.h>
#include <netinet6/ah.h>
#include <netkey/key.h>
#include <netkey/keydb.h>
#include <netkey/key_debug.h>

#include <machine/stdarg.h>

#ifdef __NetBSD__
#define ovbcopy	bcopy
#endif

#ifdef INET
extern struct protosw inetsw[];
#if defined(__bsdi__) || defined(__NetBSD__)
extern u_char ip_protox[];
#endif

void
#if __STDC__
ah4_input(struct mbuf *m, ...)
#else
ah4_input(m, va_alist)
	struct mbuf *m;
	va_dcl
#endif
{
	struct ip *ip;
	struct ah *ah;
	u_int32_t spi;
	struct ah_algorithm *algo;
	size_t siz;
	size_t siz1;
	u_char *cksum;
	struct secas *sa = NULL;
	u_int16_t nxt;
	size_t hlen;
	int s;
	int off, proto;
	va_list ap;

	va_start(ap, m);
	off = va_arg(ap, int);
	proto = va_arg(ap, int);
	va_end(ap);

	if (m->m_len < off + sizeof(struct newah)) {
		m = m_pullup(m, off + sizeof(struct newah));
		if (!m) {
			printf("IPv4 AH input: can't pullup;"
				"dropping the packet for simplicity\n");
			ipsecstat.in_inval++;
			goto fail;
		}
	}

	ip = mtod(m, struct ip *);
	ah = (struct ah *)(((caddr_t)ip) + off);
	nxt = ah->ah_nxt;
#ifdef _IP_VHL
	hlen = IP_VHL_HL(ip->ip_vhl) << 2;
#else
	hlen = ip->ip_hl << 2;
#endif

	/* find the sassoc. */
	spi = ah->ah_spi;

	if ((sa = key_allocsa(AF_INET,
	                      (caddr_t)&ip->ip_src, (caddr_t)&ip->ip_dst,
	                      IPPROTO_AH, spi)) == 0) {
		printf("IPv4 AH input: no key association found for spi %u;"
			"dropping the packet for simplicity\n",
			(u_int32_t)ntohl(spi));
		ipsecstat.in_nosa++;
		goto fail;
	}
	KEYDEBUG(KEYDEBUG_IPSEC_STAMP,
		printf("DP ah4_input called to allocate SA:%p\n", sa));
	if (sa->state != SADB_SASTATE_MATURE
	 && sa->state != SADB_SASTATE_DYING) {
		printf("IPv4 AH input: non-mature/dying SA found for spi %u; "
			"dropping the packet for simplicity\n",
			(u_int32_t)ntohl(spi));
		ipsecstat.in_badspi++;
		goto fail;
	}
	if (sa->alg_auth == SADB_AALG_NONE) {
		printf("IPv4 AH input: unspecified authentication algorithm "
			"for spi %u;"
			"dropping the packet for simplicity\n",
			(u_int32_t)ntohl(spi));
		ipsecstat.in_badspi++;
		goto fail;
	}

	algo = &ah_algorithms[sa->alg_auth];

	siz = (*algo->sumsiz)(sa);
	siz1 = ((siz + 3) & ~(4 - 1));

	/*
	 * sanity checks for header, 1.
	 */
    {
	int sizoff;

	sizoff = (sa->flags & SADB_X_EXT_OLD) ? 0 : 4;

	if ((ah->ah_len << 2) - sizoff != siz1) {
		log(LOG_NOTICE, "sum length mismatch in IPv4 AH input "
			"(%d should be %d): %s\n",
			(ah->ah_len << 2) - sizoff, siz1,
			ipsec4_logpacketstr(ip, spi));
		ipsecstat.in_inval++;
		goto fail;
	}

	if (m->m_len < off + sizeof(struct ah) + sizoff + siz1) {
		m = m_pullup(m, off + sizeof(struct ah) + sizoff + siz1);
		if (!m) {
			printf("IPv4 AH input: can't pullup;"
				"dropping the packet for simplicity\n");
			ipsecstat.in_inval++;
			goto fail;
		}

		ip = mtod(m, struct ip *);
		ah = (struct ah *)(((caddr_t)ip) + off);
	}
    }

	/*
	 * check for sequence number.
	 */
	if ((sa->flags & SADB_X_EXT_OLD) == 0 && sa->replay) {
		if (ipsec_chkreplay(ntohl(((struct newah *)ah)->ah_seq), sa))
			; /*okey*/
		else {
			ipsecstat.in_ahreplay++;
			log(LOG_AUTH, "replay packet in IPv4 AH input: %s %s\n",
				ipsec4_logpacketstr(ip, spi),
				ipsec_logsastr(sa));
			goto fail;
		}
	}

	/*
	 * alright, it seems sane.  now we are going to check the
	 * cryptographic checksum.
	 */
	cksum = malloc(siz1, M_TEMP, M_NOWAIT);
	if (!cksum) {
		printf("IPv4 AH input: couldn't alloc temporary region for cksum\n");
		ipsecstat.in_inval++;
		goto fail;
	}
	
    {
#if 1
	/*
	 * some of IP header fields are flipped to the host endian.
	 * convert them back to network endian.  VERY stupid.
	 */
#ifndef __NetBSD__
	ip->ip_len = htons(ip->ip_len + hlen);
	ip->ip_id = htons(ip->ip_id);
#else
	ip->ip_len = htons(ip->ip_len);
#endif
	ip->ip_off = htons(ip->ip_off);
#endif
	if (ah4_calccksum(m, (caddr_t)cksum, algo, sa)) {
		free(cksum, M_TEMP);
		ipsecstat.in_inval++;
		goto fail;
	}
	ipsecstat.in_ahhist[sa->alg_auth]++;
#if 1
	/*
	 * flip them back.
	 */
#ifndef __NetBSD__
	ip->ip_len = ntohs(ip->ip_len) - hlen;
	ip->ip_id = ntohs(ip->ip_id);
#else
	ip->ip_len = ntohs(ip->ip_len);
#endif
	ip->ip_off = ntohs(ip->ip_off);
#endif
    }

    {
	caddr_t sumpos = NULL;

	if (sa->flags & SADB_X_EXT_OLD) {
		/* RFC 1826 */
		sumpos = (caddr_t)(ah + 1);
	} else {
		/* RFC 2402 */
		sumpos = (caddr_t)(((struct newah *)ah) + 1);
	}

	if (bcmp(sumpos, cksum, siz) != 0) {
		log(LOG_AUTH, "checksum mismatch in IPv4 AH input: %s %s\n",
			ipsec4_logpacketstr(ip, spi),
			ipsec_logsastr(sa));
		free(cksum, M_TEMP);
		ipsecstat.in_ahauthfail++;
		goto fail;
	}
    }

	free(cksum, M_TEMP);

	m->m_flags |= M_AUTHIPHDR;
	m->m_flags |= M_AUTHIPDGM;

#if 0
	/*
	 * looks okey, but we need more sanity check.
	 * XXX should elaborate.
	 */
	if (ah->ah_nxt == IPPROTO_IPIP || ah->ah_nxt == IPPROTO_IP) {
		struct ip *nip;
		size_t sizoff;

		sizoff = (sa->flags & SADB_X_EXT_OLD) ? 0 : 4;

		if (m->m_len < off + sizeof(struct ah) + sizoff + siz1 + hlen) {
			m = m_pullup(m, off + sizeof(struct ah)
					+ sizoff + siz1 + hlen);
			if (!m) {
				printf("IPv4 AH input: can't pullup;"
					"dropping the packet for simplicity\n");
				ipsecstat.in_inval++;
				goto fail;
			}
		}

		nip = (struct ip *)((u_char *)(ah + 1) + sizoff + siz1);
		if (nip->ip_src.s_addr != ip->ip_src.s_addr
		 || nip->ip_dst.s_addr != ip->ip_dst.s_addr) {
			m->m_flags &= ~M_AUTHIPHDR;
			m->m_flags &= ~M_AUTHIPDGM;
		}
	}
#ifdef INET6
	else if (ah->ah_nxt == IPPROTO_IPV6) {
		m->m_flags &= ~M_AUTHIPHDR;
		m->m_flags &= ~M_AUTHIPDGM;
	}
#endif /*INET6*/
#endif /*0*/

	if (m->m_flags & M_AUTHIPHDR
	 && m->m_flags & M_AUTHIPDGM) {
#if 0
		printf("IPv4 AH input: authentication succeess\n");
#else
		;
#endif
		ipsecstat.in_ahauthsucc++;
	} else {
		log(LOG_AUTH, "authentication failed in IPv4 AH input: %s %s\n",
			ipsec4_logpacketstr(ip, spi),
			ipsec_logsastr(sa));
		ipsecstat.in_ahauthfail++;
	}

	/*
	 * update sequence number.
	 */
	if ((sa->flags & SADB_X_EXT_OLD) == 0 && sa->replay) {
		(void)ipsec_updatereplay(ntohl(((struct newah *)ah)->ah_seq), sa);
	}

	/* was it transmitted over the IPsec tunnel SA? */
	if (ipsec4_tunnel_validate(ip, nxt, sa) && nxt == IPPROTO_IPV4) {
		/*
		 * strip off all the headers that precedes AH.
		 *	IP xx AH IP' payload -> IP' payload
		 *
		 * XXX more sanity checks
		 * XXX relationship with gif?
		 */
		size_t stripsiz = 0;
		u_int8_t tos;

		tos = ip->ip_tos;
		if (sa->flags & SADB_X_EXT_OLD) {
			/* RFC 1826 */
			stripsiz = sizeof(struct ah) + siz1;
		} else {
			/* RFC 2402 */
			stripsiz = sizeof(struct newah) + siz1;
		}
		m_adj(m, off + stripsiz);
		if (m->m_len < sizeof(*ip)) {
			m = m_pullup(m, sizeof(*ip));
			if (!m) {
				ipsecstat.in_inval++;
				goto fail;
			}
		}
		ip = mtod(m, struct ip *);
		/* ECN consideration. */
		ip_ecn_egress(ip4_ipsec_ecn, &tos, &ip->ip_tos);
		if (!key_checktunnelsanity(sa, AF_INET,
			    (caddr_t)&ip->ip_src, (caddr_t)&ip->ip_dst)) {
			log(LOG_NOTICE, "ipsec tunnel address mismatch in IPv4 AH input: %s %s\n",
				ipsec4_logpacketstr(ip, spi),
				ipsec_logsastr(sa));
			ipsecstat.in_inval++;
			goto fail;
		}

#if 0 /* XXX should call ipfw rather than ipsec_inn_reject, shouldn't it ? */
		/* drop it if it does not match the default policy */
		if (ipsec4_in_reject(m, NULL)) {
			ipsecstat.in_polvio++;
			goto fail;
		}
#endif

#if 1
		/*
		 * Should the inner packet be considered authentic?
		 * My current answer is: NO.
		 *
		 * host1 -- gw1 === gw2 -- host2
		 *	In this case, gw2 can trust the	authenticity of the
		 *	outer packet, but NOT inner.  Packet may be altered
		 *	between host1 and gw1.
		 *
		 * host1 -- gw1 === host2
		 *	This case falls into the same scenario as above.
		 *
		 * host1 === host2
		 *	This case is the only case when we may be able to leave
		 *	M_AUTHIPHDR and M_AUTHIPDGM set.
		 *	However, if host1 is wrongly configured, and allows
		 *	attacker to inject some packet with src=host1 and
		 *	dst=host2, you are in risk.
		 */
		m->m_flags &= ~M_AUTHIPHDR;
		m->m_flags &= ~M_AUTHIPDGM;
#endif

		key_sa_recordxfer(sa, m);

		s = splimp();
		if (IF_QFULL(&ipintrq)) {
			ipsecstat.in_inval++;
			goto fail;
		}
		IF_ENQUEUE(&ipintrq, m);
		m = NULL;
		schednetisr(NETISR_IP);	/*can be skipped but to make sure*/
		splx(s);
		nxt = IPPROTO_DONE;
	} else {
		/*
		 * strip off AH.
		 * We do deep-copy since KAME requires that
		 * the packet is placed in a single external mbuf.
		 */
		size_t stripsiz = 0;

		if (sa->flags & SADB_X_EXT_OLD) {
			/* RFC 1826 */
			stripsiz = sizeof(struct ah) + siz1;
		} else {
			/* RFC 2402 */
			stripsiz = sizeof(struct newah) + siz1;
		}

		ip = mtod(m, struct ip *);
		ovbcopy((caddr_t)ip, (caddr_t)(((u_char *)ip) + stripsiz), off);
		m->m_data += stripsiz;
		m->m_len -= stripsiz;
		m->m_pkthdr.len -= stripsiz;

		ip = mtod(m, struct ip *);
#if 1
		/*ip_len is in host endian*/
		ip->ip_len = ip->ip_len - stripsiz;
#else
		/*ip_len is in net endian*/
		ip->ip_len = htons(ntohs(ip->ip_len) - stripsiz);
#endif
		ip->ip_p = nxt;
		/* forget about IP hdr checksum, the check has already been passed */

		if (nxt != IPPROTO_DONE)
			(*inetsw[ip_protox[nxt]].pr_input)(m, off, nxt);
		else
			m_freem(m);
		m = NULL;
	}

	if (sa) {
		KEYDEBUG(KEYDEBUG_IPSEC_STAMP,
			printf("DP ah4_input call free SA:%p\n", sa));
		key_freesa(sa);
	}
	ipsecstat.in_success++;
	return;

fail:
	if (sa) {
		KEYDEBUG(KEYDEBUG_IPSEC_STAMP,
			printf("DP ah4_input call free SA:%p\n", sa));
		key_freesa(sa);
	}
	if (m)
		m_freem(m);
	return;
}
#endif /* INET */

#ifdef INET6
int
ah6_input(mp, offp, proto)
	struct mbuf **mp;
	int *offp, proto;
{
	struct mbuf *m = *mp;
	int off = *offp;
	struct ip6_hdr *ip6;
	struct ah *ah;
	u_int32_t spi;
	struct ah_algorithm *algo;
	size_t siz;
	size_t siz1;
	u_char *cksum;
	struct secas *sa = NULL;
	u_int16_t nxt;
	int s;

	IP6_EXTHDR_CHECK(m, off, sizeof(struct ah), IPPROTO_DONE);

	ip6 = mtod(m, struct ip6_hdr *);
	ah = (struct ah *)(((caddr_t)ip6) + off);

	nxt = ah->ah_nxt;

	/* find the sassoc.  */
	spi = ah->ah_spi;

	if (ntohs(ip6->ip6_plen) == 0) {
		printf("IPv6 AH input: AH with IPv6 jumbogram is not supported.\n");
		ipsec6stat.in_inval++;
		goto fail;
	}

	if ((sa = key_allocsa(AF_INET6,
	                      (caddr_t)&ip6->ip6_src, (caddr_t)&ip6->ip6_dst,
	                      IPPROTO_AH, spi)) == 0) {
		printf("IPv6 AH input: no key association found for spi %u;"
			"dropping the packet for simplicity\n",
			(u_int32_t)ntohl(spi));
		ipsec6stat.in_nosa++;
		goto fail;
	}
	KEYDEBUG(KEYDEBUG_IPSEC_STAMP,
		printf("DP ah6_input called to allocate SA:%p\n", sa));
	if (sa->state != SADB_SASTATE_MATURE
	 && sa->state != SADB_SASTATE_DYING) {
		printf("IPv6 AH input: non-mature/dying SA found for spi %u; "
			"dropping the packet for simplicity\n",
			(u_int32_t)ntohl(spi));
		ipsec6stat.in_badspi++;
		goto fail;
	}
	if (sa->alg_auth == SADB_AALG_NONE) {
		printf("IPv6 AH input: unspecified authentication algorithm "
			"for spi %u;"
			"dropping the packet for simplicity\n",
			(u_int32_t)ntohl(spi));
		ipsec6stat.in_badspi++;
		goto fail;
	}

	algo = &ah_algorithms[sa->alg_auth];

	siz = (*algo->sumsiz)(sa);
	siz1 = ((siz + 3) & ~(4 - 1));

	/*
	 * sanity checks for header, 1.
	 */
    {
	int sizoff;

	sizoff = (sa->flags & SADB_X_EXT_OLD) ? 0 : 4;

	if ((ah->ah_len << 2) - sizoff != siz1) {
		log(LOG_NOTICE, "sum length mismatch in IPv6 AH input "
			"(%d should be %d): %s\n",
			(ah->ah_len << 2) - sizoff, siz1,
			ipsec6_logpacketstr(ip6, spi));
		ipsec6stat.in_inval++;
		goto fail;
	}
	IP6_EXTHDR_CHECK(m, off, sizeof(struct ah) + sizoff + siz1, IPPROTO_DONE);
    }

	/*
	 * check for sequence number.
	 */
	if ((sa->flags & SADB_X_EXT_OLD) == 0 && sa->replay) {
		if (ipsec_chkreplay(ntohl(((struct newah *)ah)->ah_seq), sa))
			; /*okey*/
		else {
			ipsec6stat.in_ahreplay++;
			log(LOG_AUTH, "replay packet in IPv6 AH input: %s %s\n",
				ipsec6_logpacketstr(ip6, spi),
				ipsec_logsastr(sa));
			goto fail;
		}
	}

	/*
	 * alright, it seems sane.  now we are going to check the
	 * cryptographic checksum.
	 */
	cksum = malloc(siz1, M_TEMP, M_NOWAIT);
	if (!cksum) {
		printf("IPv6 AH input: couldn't alloc temporary region for cksum\n");
		ipsec6stat.in_inval++;
		goto fail;
	}
	
	if (ah6_calccksum(m, (caddr_t)cksum, algo, sa)) {
		free(cksum, M_TEMP);
		ipsec6stat.in_inval++;
		goto fail;
	}
	ipsec6stat.in_ahhist[sa->alg_auth]++;

    {
	caddr_t sumpos = NULL;

	if (sa->flags & SADB_X_EXT_OLD) {
		/* RFC 1826 */
		sumpos = (caddr_t)(ah + 1);
	} else {
		/* RFC 2402 */
		sumpos = (caddr_t)(((struct newah *)ah) + 1);
	}

	if (bcmp(sumpos, cksum, siz) != 0) {
		log(LOG_AUTH, "checksum mismatch in IPv6 AH input: %s %s\n",
			ipsec6_logpacketstr(ip6, spi),
			ipsec_logsastr(sa));
		free(cksum, M_TEMP);
		ipsec6stat.in_ahauthfail++;
		goto fail;
	}
    }

	free(cksum, M_TEMP);

	m->m_flags |= M_AUTHIPHDR;
	m->m_flags |= M_AUTHIPDGM;

#if 0
	/*
	 * looks okey, but we need more sanity check.
	 * XXX should elaborate.
	 */
	if (ah->ah_nxt == IPPROTO_IPV6) {
		struct ip6_hdr *nip6;
		size_t sizoff;

		sizoff = (sa->flags & SADB_X_EXT_OLD) ? 0 : 4;

		IP6_EXTHDR_CHECK(m, off, sizeof(struct ah) + sizoff + siz1
				+ sizeof(struct ip6_hdr), IPPROTO_DONE);

		nip6 = (struct ip6_hdr *)((u_char *)(ah + 1) + sizoff + siz1);
		if (!IN6_ARE_ADDR_EQUAL(&nip6->ip6_src, &ip6->ip6_src)
		 || !IN6_ARE_ADDR_EQUAL(&nip6->ip6_dst, &ip6->ip6_dst)) {
			m->m_flags &= ~M_AUTHIPHDR;
			m->m_flags &= ~M_AUTHIPDGM;
		}
	} else if (ah->ah_nxt == IPPROTO_IPIP) {
		m->m_flags &= ~M_AUTHIPHDR;
		m->m_flags &= ~M_AUTHIPDGM;
	} else if (ah->ah_nxt == IPPROTO_IP) {
		m->m_flags &= ~M_AUTHIPHDR;
		m->m_flags &= ~M_AUTHIPDGM;
	}
#endif

	if (m->m_flags & M_AUTHIPHDR
	 && m->m_flags & M_AUTHIPDGM) {
#if 0
		printf("IPv6 AH input: authentication succeess\n");
#else
		;
#endif
		ipsec6stat.in_ahauthsucc++;
	} else {
		log(LOG_AUTH, "authentication failed in IPv6 AH input: %s %s\n",
			ipsec6_logpacketstr(ip6, spi),
			ipsec_logsastr(sa));
		ipsec6stat.in_ahauthfail++;
	}

	/*
	 * update sequence number.
	 */
	if ((sa->flags & SADB_X_EXT_OLD) == 0 && sa->replay) {
		(void)ipsec_updatereplay(ntohl(((struct newah *)ah)->ah_seq), sa);
	}

	/* was it transmitted over the IPsec tunnel SA? */
	if (ipsec6_tunnel_validate(ip6, nxt, sa) && nxt == IPPROTO_IPV6) {
		/*
		 * strip off all the headers that precedes AH.
		 *	IP6 xx AH IP6' payload -> IP6' payload
		 *
		 * XXX more sanity checks
		 * XXX relationship with gif?
		 */
		size_t stripsiz = 0;
		u_int32_t flowinfo;	/*net endian*/

		flowinfo = ip6->ip6_flow;
		if (sa->flags & SADB_X_EXT_OLD) {
			/* RFC 1826 */
			stripsiz = sizeof(struct ah) + siz1;
		} else {
			/* RFC 2402 */
			stripsiz = sizeof(struct newah) + siz1;
		}
		m_adj(m, off + stripsiz);
		if (m->m_len < sizeof(*ip6)) {
			/*
			 * m_pullup is prohibited in KAME IPv6 input processing
			 * but there's no other way!
			 */
			m = m_pullup(m, sizeof(*ip6));
			if (!m) {
				ipsec6stat.in_inval++;
				goto fail;
			}
		}
		ip6 = mtod(m, struct ip6_hdr *);
		/* ECN consideration. */
		ip6_ecn_egress(ip6_ipsec_ecn, &flowinfo, &ip6->ip6_flow);
		if (!key_checktunnelsanity(sa, AF_INET6,
			    (caddr_t)&ip6->ip6_src, (caddr_t)&ip6->ip6_dst)) {
			log(LOG_NOTICE, "ipsec tunnel address mismatch in IPv6 AH input: %s %s\n",
				ipsec6_logpacketstr(ip6, spi),
				ipsec_logsastr(sa));
			ipsec6stat.in_inval++;
			goto fail;
		}

#if 0 /* XXX should call ipfw rather than ipsec_inn_reject, shouldn't it ? */
		/* drop it if it does not match the default policy */
		if (ipsec6_in_reject(m, NULL)) {
			ipsec6stat.in_polvio++;
			goto fail;
		}
#endif

#if 1
		/*
		 * should the inner packet be considered authentic?
		 * see comment in ah4_input().
		 */
		m->m_flags &= ~M_AUTHIPHDR;
		m->m_flags &= ~M_AUTHIPDGM;
#endif

		key_sa_recordxfer(sa, m);

		s = splimp();
		if (IF_QFULL(&ip6intrq)) {
			ipsec6stat.in_inval++;
			goto fail;
		}
		IF_ENQUEUE(&ip6intrq, m);
		m = NULL;
		schednetisr(NETISR_IPV6); /*can be skipped but to make sure*/
		splx(s);
		nxt = IPPROTO_DONE;
	} else {
		/*
		 * strip off AH.
		 * We do deep-copy since KAME requires that
		 * the packet is placed in a single mbuf.
		 */
		size_t stripsiz = 0;
		char *prvnxtp;

		/*
		 * Copy the value of the next header field of AH to the
		 * next header field of the previous header.
		 * This is necessary because AH will be stripped off below.
		 */
		prvnxtp = ip6_get_prevhdr(m, off); /* XXX */
		*prvnxtp = nxt;

		if (sa->flags & SADB_X_EXT_OLD) {
			/* RFC 1826 */
			stripsiz = sizeof(struct ah) + siz1;
		} else {
			/* RFC 2402 */
			stripsiz = sizeof(struct newah) + siz1;
		}

		ip6 = mtod(m, struct ip6_hdr *);
		ovbcopy((caddr_t)ip6, (caddr_t)(((u_char *)ip6) + stripsiz),
			off);
		m->m_data += stripsiz;
		m->m_len -= stripsiz;
		m->m_pkthdr.len -= stripsiz;

		ip6 = mtod(m, struct ip6_hdr *);
		ip6->ip6_plen = htons(ntohs(ip6->ip6_plen) - stripsiz);

		key_sa_recordxfer(sa, m);
	}

	*offp = off;
	*mp = m;

	if (sa) {
		KEYDEBUG(KEYDEBUG_IPSEC_STAMP,
			printf("DP ah6_input call free SA:%p\n", sa));
		key_freesa(sa);
	}
	ipsec6stat.in_success++;
	return nxt;

fail:
	if (sa) {
		KEYDEBUG(KEYDEBUG_IPSEC_STAMP,
			printf("DP ah6_input call free SA:%p\n", sa));
		key_freesa(sa);
	}
	if (m)
		m_freem(m);
	return IPPROTO_DONE;
}
#endif /* INET6 */
