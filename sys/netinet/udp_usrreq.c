/*	$NetBSD: udp_usrreq.c,v 1.110 2003/09/12 09:55:22 itojun Exp $	*/

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
 * Copyright (c) 1982, 1986, 1988, 1990, 1993, 1995
 *	The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	@(#)udp_usrreq.c	8.6 (Berkeley) 5/23/95
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: udp_usrreq.c,v 1.110 2003/09/12 09:55:22 itojun Exp $");

#include "opt_inet.h"
#include "opt_ipsec.h"
#include "opt_inet_csum.h"
#include "opt_ipkdb.h"
#include "opt_mbuftrace.h"

#include <sys/param.h>
#include <sys/malloc.h>
#include <sys/mbuf.h>
#include <sys/protosw.h>
#include <sys/socket.h>
#include <sys/socketvar.h>
#include <sys/errno.h>
#include <sys/stat.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/domain.h>
#include <sys/sysctl.h>

#include <net/if.h>
#include <net/route.h>

#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/in_var.h>
#include <netinet/ip.h>
#include <netinet/in_pcb.h>
#include <netinet/ip_var.h>
#include <netinet/ip_icmp.h>
#include <netinet/udp.h>
#include <netinet/udp_var.h>

#ifdef INET6
#include <netinet/ip6.h>
#include <netinet/icmp6.h>
#include <netinet6/ip6_var.h>
#include <netinet6/in6_pcb.h>
#include <netinet6/udp6_var.h>
#endif

#ifndef INET6
/* always need ip6.h for IP6_EXTHDR_GET */
#include <netinet/ip6.h>
#endif

#include "faith.h"
#if defined(NFAITH) && NFAITH > 0
#include <net/if_faith.h>
#endif

#include <machine/stdarg.h>

#ifdef FAST_IPSEC
#include <netipsec/ipsec.h>
#ifdef INET6
#include <netipsec/ipsec6.h>
#endif
#endif	/* FAST_IPSEC*/

#ifdef IPSEC
#include <netinet6/ipsec.h>
#include <netkey/key.h>
#endif /*IPSEC*/

#ifdef IPKDB
#include <ipkdb/ipkdb.h>
#endif

/*
 * UDP protocol implementation.
 * Per RFC 768, August, 1980.
 */
#ifndef	COMPAT_42
int	udpcksum = 1;
#else
int	udpcksum = 0;		/* XXX */
#endif

struct	inpcbtable udbtable;
struct	udpstat udpstat;

#ifdef INET
static void udp4_sendup __P((struct mbuf *, int, struct sockaddr *,
	struct socket *));
static int udp4_realinput __P((struct sockaddr_in *, struct sockaddr_in *,
	struct mbuf *, int));
#endif
#ifdef INET6
static void udp6_sendup __P((struct mbuf *, int, struct sockaddr *,
	struct socket *));
static int udp6_realinput __P((int, struct sockaddr_in6 *,
	struct sockaddr_in6 *, struct mbuf *, int));
#endif
#ifdef INET
static	void udp_notify __P((struct inpcb *, int));
#endif

#ifndef UDBHASHSIZE
#define	UDBHASHSIZE	128
#endif
int	udbhashsize = UDBHASHSIZE;

#ifdef MBUFTRACE
struct mowner udp_mowner = { "udp" };
struct mowner udp_rx_mowner = { "udp", "rx" };
struct mowner udp_tx_mowner = { "udp", "tx" };
#endif

#ifdef UDP_CSUM_COUNTERS
#include <sys/device.h>

struct evcnt udp_hwcsum_bad = EVCNT_INITIALIZER(EVCNT_TYPE_MISC,
    NULL, "udp", "hwcsum bad");
struct evcnt udp_hwcsum_ok = EVCNT_INITIALIZER(EVCNT_TYPE_MISC,
    NULL, "udp", "hwcsum ok");
struct evcnt udp_hwcsum_data = EVCNT_INITIALIZER(EVCNT_TYPE_MISC,
    NULL, "udp", "hwcsum data");
struct evcnt udp_swcsum = EVCNT_INITIALIZER(EVCNT_TYPE_MISC,
    NULL, "udp", "swcsum");

#define	UDP_CSUM_COUNTER_INCR(ev)	(ev)->ev_count++

#else

#define	UDP_CSUM_COUNTER_INCR(ev)	/* nothing */

#endif /* UDP_CSUM_COUNTERS */

void
udp_init()
{

	in_pcbinit(&udbtable, udbhashsize, udbhashsize);

#ifdef UDP_CSUM_COUNTERS
	evcnt_attach_static(&udp_hwcsum_bad);
	evcnt_attach_static(&udp_hwcsum_ok);
	evcnt_attach_static(&udp_hwcsum_data);
	evcnt_attach_static(&udp_swcsum);
#endif /* UDP_CSUM_COUNTERS */

	MOWNER_ATTACH(&udp_tx_mowner);
	MOWNER_ATTACH(&udp_rx_mowner);
	MOWNER_ATTACH(&udp_mowner);
}

#ifdef INET
void
#if __STDC__
udp_input(struct mbuf *m, ...)
#else
udp_input(m, va_alist)
	struct mbuf *m;
	va_dcl
#endif
{
	va_list ap;
	struct sockaddr_in src, dst;
	struct ip *ip;
	struct udphdr *uh;
	int iphlen;
	int len;
	int n;
	u_int16_t ip_len;

	va_start(ap, m);
	iphlen = va_arg(ap, int);
	(void)va_arg(ap, int);		/* ignore value, advance ap */
	va_end(ap);

	MCLAIM(m, &udp_rx_mowner);
	udpstat.udps_ipackets++;

	/*
	 * Get IP and UDP header together in first mbuf.
	 */
	ip = mtod(m, struct ip *);
	IP6_EXTHDR_GET(uh, struct udphdr *, m, iphlen, sizeof(struct udphdr));
	if (uh == NULL) {
		udpstat.udps_hdrops++;
		return;
	}
	KASSERT(UDP_HDR_ALIGNED_P(uh));

	/* destination port of 0 is illegal, based on RFC768. */
	if (uh->uh_dport == 0)
		goto bad;

	/*
	 * Make mbuf data length reflect UDP length.
	 * If not enough data to reflect UDP length, drop.
	 */
	ip_len = ntohs(ip->ip_len);
	len = ntohs((u_int16_t)uh->uh_ulen);
	if (ip_len != iphlen + len) {
		if (ip_len < iphlen + len || len < sizeof(struct udphdr)) {
			udpstat.udps_badlen++;
			goto bad;
		}
		m_adj(m, iphlen + len - ip_len);
	}

	/*
	 * Checksum extended UDP header and data.
	 */
	if (uh->uh_sum) {
		switch (m->m_pkthdr.csum_flags &
			((m->m_pkthdr.rcvif->if_csum_flags_rx & M_CSUM_UDPv4) |
			 M_CSUM_TCP_UDP_BAD | M_CSUM_DATA)) {
		case M_CSUM_UDPv4|M_CSUM_TCP_UDP_BAD:
			UDP_CSUM_COUNTER_INCR(&udp_hwcsum_bad);
			goto badcsum;

		case M_CSUM_UDPv4|M_CSUM_DATA: {
			u_int32_t hw_csum = m->m_pkthdr.csum_data;
			UDP_CSUM_COUNTER_INCR(&udp_hwcsum_data);
			if (m->m_pkthdr.csum_flags & M_CSUM_NO_PSEUDOHDR)
			  hw_csum = in_cksum_phdr(ip->ip_src.s_addr, ip->ip_dst.s_addr,
				htonl(hw_csum + ntohs(ip->ip_len) + IPPROTO_UDP));
			if ((hw_csum ^ 0xffff) != 0)
				goto badcsum;
			break;
		}

		case M_CSUM_UDPv4:
			/* Checksum was okay. */
			UDP_CSUM_COUNTER_INCR(&udp_hwcsum_ok);
			break;

		default:
			/* Need to compute it ourselves. */
			UDP_CSUM_COUNTER_INCR(&udp_swcsum);
			if (in4_cksum(m, IPPROTO_UDP, iphlen, len) != 0)
				goto badcsum;
			break;
		}
	}

	/* construct source and dst sockaddrs. */
	bzero(&src, sizeof(src));
	src.sin_family = AF_INET;
	src.sin_len = sizeof(struct sockaddr_in);
	bcopy(&ip->ip_src, &src.sin_addr, sizeof(src.sin_addr));
	src.sin_port = uh->uh_sport;
	bzero(&dst, sizeof(dst));
	dst.sin_family = AF_INET;
	dst.sin_len = sizeof(struct sockaddr_in);
	bcopy(&ip->ip_dst, &dst.sin_addr, sizeof(dst.sin_addr));
	dst.sin_port = uh->uh_dport;

	n = udp4_realinput(&src, &dst, m, iphlen);
#ifdef INET6
	if (IN_MULTICAST(ip->ip_dst.s_addr) || n == 0) {
		struct sockaddr_in6 src6, dst6;

		bzero(&src6, sizeof(src6));
		src6.sin6_family = AF_INET6;
		src6.sin6_len = sizeof(struct sockaddr_in6);
		src6.sin6_addr.s6_addr[10] = src6.sin6_addr.s6_addr[11] = 0xff;
		bcopy(&ip->ip_src, &src6.sin6_addr.s6_addr[12],
			sizeof(ip->ip_src));
		src6.sin6_port = uh->uh_sport;
		bzero(&dst6, sizeof(dst6));
		dst6.sin6_family = AF_INET6;
		dst6.sin6_len = sizeof(struct sockaddr_in6);
		dst6.sin6_addr.s6_addr[10] = dst6.sin6_addr.s6_addr[11] = 0xff;
		bcopy(&ip->ip_dst, &dst6.sin6_addr.s6_addr[12],
			sizeof(ip->ip_dst));
		dst6.sin6_port = uh->uh_dport;

		n += udp6_realinput(AF_INET, &src6, &dst6, m, iphlen);
	}
#endif

	if (n == 0) {
		if (m->m_flags & (M_BCAST | M_MCAST)) {
			udpstat.udps_noportbcast++;
			goto bad;
		}
		udpstat.udps_noport++;
#ifdef IPKDB
		if (checkipkdb(&ip->ip_src, uh->uh_sport, uh->uh_dport,
				m, iphlen + sizeof(struct udphdr),
				m->m_pkthdr.len - iphlen - sizeof(struct udphdr))) {
			/*
			 * It was a debugger connect packet,
			 * just drop it now
			 */
			goto bad;
		}
#endif
		icmp_error(m, ICMP_UNREACH, ICMP_UNREACH_PORT, 0, 0);
		m = NULL;
	}

bad:
	if (m)
		m_freem(m);
	return;

badcsum:
	m_freem(m);
	udpstat.udps_badsum++;
}
#endif

#ifdef INET6
int
udp6_input(mp, offp, proto)
	struct mbuf **mp;
	int *offp, proto;
{
	struct mbuf *m = *mp;
	int off = *offp;
	struct sockaddr_in6 src, dst;
	struct ip6_hdr *ip6;
	struct udphdr *uh;
	u_int32_t plen, ulen;

	ip6 = mtod(m, struct ip6_hdr *);

#if defined(NFAITH) && 0 < NFAITH
	if (faithprefix(&ip6->ip6_dst)) {
		/* send icmp6 host unreach? */
		m_freem(m);
		return IPPROTO_DONE;
	}
#endif

	udp6stat.udp6s_ipackets++;

	/* check for jumbogram is done in ip6_input.  we can trust pkthdr.len */
	plen = m->m_pkthdr.len - off;
	IP6_EXTHDR_GET(uh, struct udphdr *, m, off, sizeof(struct udphdr));
	if (uh == NULL) {
		ip6stat.ip6s_tooshort++;
		return IPPROTO_DONE;
	}
	KASSERT(UDP_HDR_ALIGNED_P(uh));
	ulen = ntohs((u_short)uh->uh_ulen);
	/*
	 * RFC2675 section 4: jumbograms will have 0 in the UDP header field,
	 * iff payload length > 0xffff.
	 */
	if (ulen == 0 && plen > 0xffff)
		ulen = plen;

	if (plen != ulen) {
		udp6stat.udp6s_badlen++;
		goto bad;
	}

	/* destination port of 0 is illegal, based on RFC768. */
	if (uh->uh_dport == 0)
		goto bad;

	/* Be proactive about malicious use of IPv4 mapped address */
	if (IN6_IS_ADDR_V4MAPPED(&ip6->ip6_src) ||
	    IN6_IS_ADDR_V4MAPPED(&ip6->ip6_dst)) {
		/* XXX stat */
		goto bad;
	}

	/*
	 * Checksum extended UDP header and data.
	 */
	if (uh->uh_sum == 0)
		udp6stat.udp6s_nosum++;
	else if (in6_cksum(m, IPPROTO_UDP, off, ulen) != 0) {
		udp6stat.udp6s_badsum++;
		goto bad;
	}

	/*
	 * Construct source and dst sockaddrs.
	 * Note that ifindex (s6_addr16[1]) is already filled.
	 */
	bzero(&src, sizeof(src));
	src.sin6_family = AF_INET6;
	src.sin6_len = sizeof(struct sockaddr_in6);
	/* KAME hack: recover scopeid */
	(void)in6_recoverscope(&src, &ip6->ip6_src, m->m_pkthdr.rcvif);
	src.sin6_port = uh->uh_sport;
	bzero(&dst, sizeof(dst));
	dst.sin6_family = AF_INET6;
	dst.sin6_len = sizeof(struct sockaddr_in6);
	/* KAME hack: recover scopeid */
	(void)in6_recoverscope(&dst, &ip6->ip6_dst, m->m_pkthdr.rcvif);
	dst.sin6_port = uh->uh_dport;

	if (udp6_realinput(AF_INET6, &src, &dst, m, off) == 0) {
		if (m->m_flags & M_MCAST) {
			udp6stat.udp6s_noportmcast++;
			goto bad;
		}
		udp6stat.udp6s_noport++;
		icmp6_error(m, ICMP6_DST_UNREACH, ICMP6_DST_UNREACH_NOPORT, 0);
		m = NULL;
	}

bad:
	if (m)
		m_freem(m);
	return IPPROTO_DONE;
}
#endif

#ifdef INET
static void
udp4_sendup(m, off, src, so)
	struct mbuf *m;
	int off;	/* offset of data portion */
	struct sockaddr *src;
	struct socket *so;
{
	struct mbuf *opts = NULL;
	struct mbuf *n;
	struct inpcb *inp = NULL;

	if (!so)
		return;
	switch (so->so_proto->pr_domain->dom_family) {
	case AF_INET:
		inp = sotoinpcb(so);
		break;
#ifdef INET6
	case AF_INET6:
		break;
#endif
	default:
		return;
	}

#if defined(IPSEC) || defined(FAST_IPSEC)
	/* check AH/ESP integrity. */
	if (so != NULL && ipsec4_in_reject_so(m, so)) {
		ipsecstat.in_polvio++;
		if ((n = m_copy(m, 0, M_COPYALL)) != NULL)
			icmp_error(n, ICMP_UNREACH, ICMP_UNREACH_ADMIN_PROHIBIT,
			    0, 0);
		return;
	}
#endif /*IPSEC*/

	if ((n = m_copy(m, 0, M_COPYALL)) != NULL) {
		if (inp && (inp->inp_flags & INP_CONTROLOPTS
			 || so->so_options & SO_TIMESTAMP)) {
			struct ip *ip = mtod(n, struct ip *);
			ip_savecontrol(inp, &opts, ip, n);
		}

		m_adj(n, off);
		if (sbappendaddr(&so->so_rcv, src, n,
				opts) == 0) {
			m_freem(n);
			if (opts)
				m_freem(opts);
			udpstat.udps_fullsock++;
		} else
			sorwakeup(so);
	}
}
#endif

#ifdef INET6
static void
udp6_sendup(m, off, src, so)
	struct mbuf *m;
	int off;	/* offset of data portion */
	struct sockaddr *src;
	struct socket *so;
{
	struct mbuf *opts = NULL;
	struct mbuf *n;
	struct in6pcb *in6p = NULL;

	if (!so)
		return;
	if (so->so_proto->pr_domain->dom_family != AF_INET6)
		return;
	in6p = sotoin6pcb(so);

#if defined(IPSEC) || defined(FAST_IPSEC)
	/* check AH/ESP integrity. */
	if (so != NULL && ipsec6_in_reject_so(m, so)) {
		ipsec6stat.in_polvio++;
		if ((n = m_copy(n, 0, M_COPYALL)) != NULL)
			icmp6_error(n, ICMP6_DST_UNREACH,
			    ICMP6_DST_UNREACH_ADMIN, 0);
		return;
	}
#endif /*IPSEC*/

	if ((n = m_copy(m, 0, M_COPYALL)) != NULL) {
		if (in6p && (in6p->in6p_flags & IN6P_CONTROLOPTS
			  || in6p->in6p_socket->so_options & SO_TIMESTAMP)) {
			struct ip6_hdr *ip6 = mtod(n, struct ip6_hdr *);
			ip6_savecontrol(in6p, &opts, ip6, n);
		}

		m_adj(n, off);
		if (sbappendaddr(&so->so_rcv, src, n, opts) == 0) {
			m_freem(n);
			if (opts)
				m_freem(opts);
			udp6stat.udp6s_fullsock++;
		} else
			sorwakeup(so);
	}
}
#endif

#ifdef INET
static int
udp4_realinput(src, dst, m, off)
	struct sockaddr_in *src;
	struct sockaddr_in *dst;
	struct mbuf *m;
	int off;	/* offset of udphdr */
{
	u_int16_t *sport, *dport;
	int rcvcnt;
	struct in_addr *src4, *dst4;
	struct inpcb_hdr *inph;
	struct inpcb *inp;

	rcvcnt = 0;
	off += sizeof(struct udphdr);	/* now, offset of payload */

	if (src->sin_family != AF_INET || dst->sin_family != AF_INET)
		goto bad;

	src4 = &src->sin_addr;
	sport = &src->sin_port;
	dst4 = &dst->sin_addr;
	dport = &dst->sin_port;

	if (IN_MULTICAST(dst4->s_addr) ||
	    in_broadcast(*dst4, m->m_pkthdr.rcvif)) {
		/*
		 * Deliver a multicast or broadcast datagram to *all* sockets
		 * for which the local and remote addresses and ports match
		 * those of the incoming datagram.  This allows more than
		 * one process to receive multi/broadcasts on the same port.
		 * (This really ought to be done for unicast datagrams as
		 * well, but that would cause problems with existing
		 * applications that open both address-specific sockets and
		 * a wildcard socket listening to the same port -- they would
		 * end up receiving duplicates of every unicast datagram.
		 * Those applications open the multiple sockets to overcome an
		 * inadequacy of the UDP socket interface, but for backwards
		 * compatibility we avoid the problem here rather than
		 * fixing the interface.  Maybe 4.5BSD will remedy this?)
		 */

		/*
		 * KAME note: traditionally we dropped udpiphdr from mbuf here.
		 * we need udpiphdr for IPsec processing so we do that later.
		 */
		/*
		 * Locate pcb(s) for datagram.
		 */
		CIRCLEQ_FOREACH(inph, &udbtable.inpt_queue, inph_queue) {
			inp = (struct inpcb *)inph;
			if (inp->inp_af != AF_INET)
				continue;

			if (inp->inp_lport != *dport)
				continue;
			if (!in_nullhost(inp->inp_laddr)) {
				if (!in_hosteq(inp->inp_laddr, *dst4))
					continue;
			}
			if (!in_nullhost(inp->inp_faddr)) {
				if (!in_hosteq(inp->inp_faddr, *src4) ||
				    inp->inp_fport != *sport)
					continue;
			}

			udp4_sendup(m, off, (struct sockaddr *)src,
				inp->inp_socket);
			rcvcnt++;

			/*
			 * Don't look for additional matches if this one does
			 * not have either the SO_REUSEPORT or SO_REUSEADDR
			 * socket options set.  This heuristic avoids searching
			 * through all pcbs in the common case of a non-shared
			 * port.  It assumes that an application will never
			 * clear these options after setting them.
			 */
			if ((inp->inp_socket->so_options &
			    (SO_REUSEPORT|SO_REUSEADDR)) == 0)
				break;
		}
	} else {
		/*
		 * Locate pcb for datagram.
		 */
		inp = in_pcblookup_connect(&udbtable, *src4, *sport, *dst4, *dport);
		if (inp == 0) {
			++udpstat.udps_pcbhashmiss;
			inp = in_pcblookup_bind(&udbtable, *dst4, *dport);
			if (inp == 0)
				return rcvcnt;
		}

		udp4_sendup(m, off, (struct sockaddr *)src, inp->inp_socket);
		rcvcnt++;
	}

bad:
	return rcvcnt;
}
#endif

#ifdef INET6
static int
udp6_realinput(af, src, dst, m, off)
	int af;		/* af on packet */
	struct sockaddr_in6 *src;
	struct sockaddr_in6 *dst;
	struct mbuf *m;
	int off;	/* offset of udphdr */
{
	u_int16_t sport, dport;
	int rcvcnt;
	struct in6_addr src6, dst6;
	const struct in_addr *dst4;
	struct inpcb_hdr *inph;
	struct in6pcb *in6p;

	rcvcnt = 0;
	off += sizeof(struct udphdr);	/* now, offset of payload */

	if (af != AF_INET && af != AF_INET6)
		goto bad;
	if (src->sin6_family != AF_INET6 || dst->sin6_family != AF_INET6)
		goto bad;

	in6_embedscope(&src6, src, NULL, NULL);
	sport = src->sin6_port;
	in6_embedscope(&dst6, dst, NULL, NULL);
	dport = dst->sin6_port;
	dst4 = (struct in_addr *)&dst->sin6_addr.s6_addr[12];

	if (IN6_IS_ADDR_MULTICAST(&dst6) ||
	    (af == AF_INET && IN_MULTICAST(dst4->s_addr))) {
		/*
		 * Deliver a multicast or broadcast datagram to *all* sockets
		 * for which the local and remote addresses and ports match
		 * those of the incoming datagram.  This allows more than
		 * one process to receive multi/broadcasts on the same port.
		 * (This really ought to be done for unicast datagrams as
		 * well, but that would cause problems with existing
		 * applications that open both address-specific sockets and
		 * a wildcard socket listening to the same port -- they would
		 * end up receiving duplicates of every unicast datagram.
		 * Those applications open the multiple sockets to overcome an
		 * inadequacy of the UDP socket interface, but for backwards
		 * compatibility we avoid the problem here rather than
		 * fixing the interface.  Maybe 4.5BSD will remedy this?)
		 */

		/*
		 * KAME note: traditionally we dropped udpiphdr from mbuf here.
		 * we need udpiphdr for IPsec processing so we do that later.
		 */
		/*
		 * Locate pcb(s) for datagram.
		 */
		CIRCLEQ_FOREACH(inph, &udbtable.inpt_queue, inph_queue) {
			in6p = (struct in6pcb *)inph;
			if (in6p->in6p_af != AF_INET6)
				continue;

			if (in6p->in6p_lport != dport)
				continue;
			if (!IN6_IS_ADDR_UNSPECIFIED(&in6p->in6p_laddr)) {
				if (!IN6_ARE_ADDR_EQUAL(&in6p->in6p_laddr, &dst6))
					continue;
			} else {
				if (IN6_IS_ADDR_V4MAPPED(&dst6) &&
				    (in6p->in6p_flags & IN6P_IPV6_V6ONLY))
					continue;
			}
			if (!IN6_IS_ADDR_UNSPECIFIED(&in6p->in6p_faddr)) {
				if (!IN6_ARE_ADDR_EQUAL(&in6p->in6p_faddr,
				    &src6) || in6p->in6p_fport != sport)
					continue;
			} else {
				if (IN6_IS_ADDR_V4MAPPED(&src6) &&
				    (in6p->in6p_flags & IN6P_IPV6_V6ONLY))
					continue;
			}

			udp6_sendup(m, off, (struct sockaddr *)src,
				in6p->in6p_socket);
			rcvcnt++;

			/*
			 * Don't look for additional matches if this one does
			 * not have either the SO_REUSEPORT or SO_REUSEADDR
			 * socket options set.  This heuristic avoids searching
			 * through all pcbs in the common case of a non-shared
			 * port.  It assumes that an application will never
			 * clear these options after setting them.
			 */
			if ((in6p->in6p_socket->so_options &
			    (SO_REUSEPORT|SO_REUSEADDR)) == 0)
				break;
		}
	} else {
		/*
		 * Locate pcb for datagram.
		 */
		in6p = in6_pcblookup_connect(&udbtable, &src6, sport,
		    &dst6, dport, 0);
		if (in6p == 0) {
			++udpstat.udps_pcbhashmiss;
			in6p = in6_pcblookup_bind(&udbtable, &dst6, dport, 0);
			if (in6p == 0)
				return rcvcnt;
		}

		udp6_sendup(m, off, (struct sockaddr *)src, in6p->in6p_socket);
		rcvcnt++;
	}

bad:
	return rcvcnt;
}
#endif

#ifdef INET
/*
 * Notify a udp user of an asynchronous error;
 * just wake up so that he can collect error status.
 */
static void
udp_notify(inp, errno)
	struct inpcb *inp;
	int errno;
{

	inp->inp_socket->so_error = errno;
	sorwakeup(inp->inp_socket);
	sowwakeup(inp->inp_socket);
}

void *
udp_ctlinput(cmd, sa, v)
	int cmd;
	struct sockaddr *sa;
	void *v;
{
	struct ip *ip = v;
	struct udphdr *uh;
	void (*notify) __P((struct inpcb *, int)) = udp_notify;
	int errno;

	if (sa->sa_family != AF_INET
	 || sa->sa_len != sizeof(struct sockaddr_in))
		return NULL;
	if ((unsigned)cmd >= PRC_NCMDS)
		return NULL;
	errno = inetctlerrmap[cmd];
	if (PRC_IS_REDIRECT(cmd))
		notify = in_rtchange, ip = 0;
	else if (cmd == PRC_HOSTDEAD)
		ip = 0;
	else if (errno == 0)
		return NULL;
	if (ip) {
		uh = (struct udphdr *)((caddr_t)ip + (ip->ip_hl << 2));
		in_pcbnotify(&udbtable, satosin(sa)->sin_addr, uh->uh_dport,
		    ip->ip_src, uh->uh_sport, errno, notify);

		/* XXX mapped address case */
	} else
		in_pcbnotifyall(&udbtable, satosin(sa)->sin_addr, errno,
		    notify);
	return NULL;
}

int
#if __STDC__
udp_output(struct mbuf *m, ...)
#else
udp_output(m, va_alist)
	struct mbuf *m;
	va_dcl
#endif
{
	struct inpcb *inp;
	struct udpiphdr *ui;
	int len = m->m_pkthdr.len;
	int error = 0;
	va_list ap;

	MCLAIM(m, &udp_tx_mowner);
	va_start(ap, m);
	inp = va_arg(ap, struct inpcb *);
	va_end(ap);

	/*
	 * Calculate data length and get a mbuf
	 * for UDP and IP headers.
	 */
	M_PREPEND(m, sizeof(struct udpiphdr), M_DONTWAIT);
	if (m == 0) {
		error = ENOBUFS;
		goto release;
	}

	/*
	 * Compute the packet length of the IP header, and
	 * punt if the length looks bogus.
	 */
	if (len + sizeof(struct udpiphdr) > IP_MAXPACKET) {
		error = EMSGSIZE;
		goto release;
	}

	/*
	 * Fill in mbuf with extended UDP header
	 * and addresses and length put into network format.
	 */
	ui = mtod(m, struct udpiphdr *);
	ui->ui_pr = IPPROTO_UDP;
	ui->ui_src = inp->inp_laddr;
	ui->ui_dst = inp->inp_faddr;
	ui->ui_sport = inp->inp_lport;
	ui->ui_dport = inp->inp_fport;
	ui->ui_ulen = htons((u_int16_t)len + sizeof(struct udphdr));

	/*
	 * Set up checksum and output datagram.
	 */
	if (udpcksum) {
		/*
		 * XXX Cache pseudo-header checksum part for
		 * XXX "connected" UDP sockets.
		 */
		ui->ui_sum = in_cksum_phdr(ui->ui_src.s_addr,
		    ui->ui_dst.s_addr, htons((u_int16_t)len +
		    sizeof(struct udphdr) + IPPROTO_UDP));
		m->m_pkthdr.csum_flags = M_CSUM_UDPv4;
		m->m_pkthdr.csum_data = offsetof(struct udphdr, uh_sum);
	} else
		ui->ui_sum = 0;
	((struct ip *)ui)->ip_len = htons(sizeof (struct udpiphdr) + len);
	((struct ip *)ui)->ip_ttl = inp->inp_ip.ip_ttl;	/* XXX */
	((struct ip *)ui)->ip_tos = inp->inp_ip.ip_tos;	/* XXX */
	udpstat.udps_opackets++;

	return (ip_output(m, inp->inp_options, &inp->inp_route,
	    inp->inp_socket->so_options & (SO_DONTROUTE | SO_BROADCAST),
	    inp->inp_moptions, inp->inp_socket));

release:
	m_freem(m);
	return (error);
}

int	udp_sendspace = 9216;		/* really max datagram size */
int	udp_recvspace = 40 * (1024 + sizeof(struct sockaddr_in));
					/* 40 1K datagrams */

/*ARGSUSED*/
int
udp_usrreq(so, req, m, nam, control, p)
	struct socket *so;
	int req;
	struct mbuf *m, *nam, *control;
	struct proc *p;
{
	struct inpcb *inp;
	int s;
	int error = 0;

	if (req == PRU_CONTROL)
		return (in_control(so, (long)m, (caddr_t)nam,
		    (struct ifnet *)control, p));

	if (req == PRU_PURGEIF) {
		in_pcbpurgeif0(&udbtable, (struct ifnet *)control);
		in_purgeif((struct ifnet *)control);
		in_pcbpurgeif(&udbtable, (struct ifnet *)control);
		return (0);
	}

	s = splsoftnet();
	inp = sotoinpcb(so);
#ifdef DIAGNOSTIC
	if (req != PRU_SEND && req != PRU_SENDOOB && control)
		panic("udp_usrreq: unexpected control mbuf");
#endif
	if (inp == 0 && req != PRU_ATTACH) {
		error = EINVAL;
		goto release;
	}

	/*
	 * Note: need to block udp_input while changing
	 * the udp pcb queue and/or pcb addresses.
	 */
	switch (req) {

	case PRU_ATTACH:
		if (inp != 0) {
			error = EISCONN;
			break;
		}
#ifdef MBUFTRACE
		so->so_mowner = &udp_mowner;
		so->so_rcv.sb_mowner = &udp_rx_mowner;
		so->so_snd.sb_mowner = &udp_tx_mowner;
#endif
		if (so->so_snd.sb_hiwat == 0 || so->so_rcv.sb_hiwat == 0) {
			error = soreserve(so, udp_sendspace, udp_recvspace);
			if (error)
				break;
		}
		error = in_pcballoc(so, &udbtable);
		if (error)
			break;
		inp = sotoinpcb(so);
		inp->inp_ip.ip_ttl = ip_defttl;
		break;

	case PRU_DETACH:
		in_pcbdetach(inp);
		break;

	case PRU_BIND:
		error = in_pcbbind(inp, nam, p);
		break;

	case PRU_LISTEN:
		error = EOPNOTSUPP;
		break;

	case PRU_CONNECT:
		error = in_pcbconnect(inp, nam);
		if (error)
			break;
		soisconnected(so);
		break;

	case PRU_CONNECT2:
		error = EOPNOTSUPP;
		break;

	case PRU_DISCONNECT:
		/*soisdisconnected(so);*/
		so->so_state &= ~SS_ISCONNECTED;	/* XXX */
		in_pcbdisconnect(inp);
		inp->inp_laddr = zeroin_addr;		/* XXX */
		if (inp->inp_ia != NULL) {
			LIST_REMOVE(inp, inp_ialink);
			IFAFREE(&inp->inp_ia->ia_ifa);
			inp->inp_ia = NULL;
		}
		in_pcbstate(inp, INP_BOUND);		/* XXX */
		break;

	case PRU_SHUTDOWN:
		socantsendmore(so);
		break;

	case PRU_RCVD:
		error = EOPNOTSUPP;
		break;

	case PRU_SEND:
		if (control && control->m_len) {
			m_freem(control);
			m_freem(m);
			error = EINVAL;
			break;
		}
	{
		struct in_addr laddr;			/* XXX */

		if (nam) {
			laddr = inp->inp_laddr;		/* XXX */
			if ((so->so_state & SS_ISCONNECTED) != 0) {
				error = EISCONN;
				goto die;
			}
			error = in_pcbconnect(inp, nam);
			if (error)
				goto die;
		} else {
			if ((so->so_state & SS_ISCONNECTED) == 0) {
				error = ENOTCONN;
				goto die;
			}
		}
		error = udp_output(m, inp);
		m = NULL;
		if (nam) {
			in_pcbdisconnect(inp);
			inp->inp_laddr = laddr;		/* XXX */
			in_pcbstate(inp, INP_BOUND);	/* XXX */
		}
	  die:
		if (inp->inp_ia != NULL && in_nullhost(inp->inp_laddr)) {
			LIST_REMOVE(inp, inp_ialink);
			IFAFREE(&inp->inp_ia->ia_ifa);
			inp->inp_ia = NULL;
		}
		if (m)
			m_freem(m);
	}
		break;

	case PRU_SENSE:
		/*
		 * stat: don't bother with a blocksize.
		 */
		splx(s);
		return (0);

	case PRU_RCVOOB:
		error =  EOPNOTSUPP;
		break;

	case PRU_SENDOOB:
		m_freem(control);
		m_freem(m);
		error =  EOPNOTSUPP;
		break;

	case PRU_SOCKADDR:
		in_setsockaddr(inp, nam);
		break;

	case PRU_PEERADDR:
		in_setpeeraddr(inp, nam);
		break;

	default:
		panic("udp_usrreq");
	}

release:
	splx(s);
	return (error);
}

/*
 * Sysctl for udp variables.
 */
int
udp_sysctl(name, namelen, oldp, oldlenp, newp, newlen)
	int *name;
	u_int namelen;
	void *oldp;
	size_t *oldlenp;
	void *newp;
	size_t newlen;
{
	/* All sysctl names at this level are terminal. */
	if (namelen != 1)
		return (ENOTDIR);

	switch (name[0]) {
	case UDPCTL_CHECKSUM:
		return (sysctl_int(oldp, oldlenp, newp, newlen, &udpcksum));
	case UDPCTL_SENDSPACE:
		return (sysctl_int(oldp, oldlenp, newp, newlen,
		    &udp_sendspace));
	case UDPCTL_RECVSPACE:
		return (sysctl_int(oldp, oldlenp, newp, newlen,
		    &udp_recvspace));
	default:
		return (ENOPROTOOPT);
	}
	/* NOTREACHED */
}
#endif
