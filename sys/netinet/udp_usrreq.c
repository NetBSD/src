/*	$NetBSD: udp_usrreq.c,v 1.190.2.2 2013/08/28 15:21:48 rmind Exp $	*/

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

/*
 * UDP protocol implementation.
 * Per RFC 768, August, 1980.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: udp_usrreq.c,v 1.190.2.2 2013/08/28 15:21:48 rmind Exp $");

#include "opt_inet.h"
#include "opt_compat_netbsd.h"
#include "opt_ipsec.h"
#include "opt_inet_csum.h"
#include "opt_ipkdb.h"
#include "opt_mbuftrace.h"

#include <sys/param.h>
#include <sys/mbuf.h>
#include <sys/protosw.h>
#include <sys/socket.h>
#include <sys/socketvar.h>
#include <sys/systm.h>
#include <sys/kmem.h>
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
#include <netinet/udp_private.h>

#ifdef INET6
#include <netinet/ip6.h>
#include <netinet/icmp6.h>
#include <netinet6/ip6_var.h>
#include <netinet6/ip6_private.h>
#include <netinet6/in6_pcb.h>
#include <netinet6/udp6_var.h>
#include <netinet6/udp6_private.h>
#endif

#ifndef INET6
/* always need ip6.h for IP6_EXTHDR_GET */
#include <netinet/ip6.h>
#endif

#ifdef IPSEC
#include <netipsec/ipsec.h>
#include <netipsec/ipsec_var.h>
#include <netipsec/ipsec_private.h>
#include <netipsec/esp.h>
#ifdef INET6
#include <netipsec/ipsec6.h>
#endif
#endif	/* IPSEC */

#ifdef COMPAT_50
#include <compat/sys/socket.h>
#endif

#ifdef IPKDB
#include <ipkdb/ipkdb.h>
#endif

int	udpcksum = 1;
int	udp_do_loopback_cksum = 0;

inpcbtable_t *		udbtable __read_mostly;
percpu_t *		udpstat_percpu;

#ifdef INET
#ifdef IPSEC
static int udp4_espinudp (struct mbuf **, int, struct sockaddr *,
	struct socket *);
#endif
static void udp4_sendup (struct mbuf *, int, struct sockaddr *,
	struct socket *);
static int udp4_realinput (struct sockaddr_in *, struct sockaddr_in *,
	struct mbuf **, int);
static int udp4_input_checksum(struct mbuf *, const struct udphdr *, int, int);
#endif

#ifndef UDBHASHSIZE
#define	UDBHASHSIZE	128
#endif
int	udbhashsize = UDBHASHSIZE;

static int	udp_sendspace = 9216;
static int	udp_recvspace = 40 * (1024 + sizeof(struct sockaddr_in));

#ifdef MBUFTRACE
struct mowner udp_mowner = MOWNER_INIT("udp", "");
struct mowner udp_rx_mowner = MOWNER_INIT("udp", "rx");
struct mowner udp_tx_mowner = MOWNER_INIT("udp", "tx");
#endif

#ifdef UDP_CSUM_COUNTERS
#include <sys/device.h>

#if defined(INET)
struct evcnt udp_hwcsum_bad = EVCNT_INITIALIZER(EVCNT_TYPE_MISC,
    NULL, "udp", "hwcsum bad");
struct evcnt udp_hwcsum_ok = EVCNT_INITIALIZER(EVCNT_TYPE_MISC,
    NULL, "udp", "hwcsum ok");
struct evcnt udp_hwcsum_data = EVCNT_INITIALIZER(EVCNT_TYPE_MISC,
    NULL, "udp", "hwcsum data");
struct evcnt udp_swcsum = EVCNT_INITIALIZER(EVCNT_TYPE_MISC,
    NULL, "udp", "swcsum");

EVCNT_ATTACH_STATIC(udp_hwcsum_bad);
EVCNT_ATTACH_STATIC(udp_hwcsum_ok);
EVCNT_ATTACH_STATIC(udp_hwcsum_data);
EVCNT_ATTACH_STATIC(udp_swcsum);
#endif /* defined(INET) */

#define	UDP_CSUM_COUNTER_INCR(ev)	(ev)->ev_count++
#else
#define	UDP_CSUM_COUNTER_INCR(ev)	/* nothing */
#endif /* UDP_CSUM_COUNTERS */

static void sysctl_net_inet_udp_setup(struct sysctllog **);

void
udp_init(void)
{
	udbtable = inpcb_init(udbhashsize, udbhashsize, 0);
	sysctl_net_inet_udp_setup(NULL);

	MOWNER_ATTACH(&udp_tx_mowner);
	MOWNER_ATTACH(&udp_rx_mowner);
	MOWNER_ATTACH(&udp_mowner);

#ifdef INET
	udpstat_percpu = percpu_alloc(sizeof(uint64_t) * UDP_NSTATS);
#endif
}

/*
 * Checksum extended UDP header and data.
 */

int
udp_input_checksum(int af, struct mbuf *m, const struct udphdr *uh,
    int iphlen, int len)
{
	switch (af) {
#ifdef INET
	case AF_INET:
		return udp4_input_checksum(m, uh, iphlen, len);
#endif
#ifdef INET6
	case AF_INET6:
		return udp6_input_checksum(m, uh, iphlen, len);
#endif
	default:
		KASSERT(false);
	}
	return -1;
}

#ifdef INET

/*
 * Checksum extended UDP header and data.
 */

static int
udp4_input_checksum(struct mbuf *m, const struct udphdr *uh,
    int iphlen, int len)
{

	/*
	 * XXX it's better to record and check if this mbuf is
	 * already checked.
	 */

	if (uh->uh_sum == 0)
		return 0;

	switch (m->m_pkthdr.csum_flags &
	    ((m->m_pkthdr.rcvif->if_csum_flags_rx & M_CSUM_UDPv4) |
	    M_CSUM_TCP_UDP_BAD | M_CSUM_DATA)) {
	case M_CSUM_UDPv4|M_CSUM_TCP_UDP_BAD:
		UDP_CSUM_COUNTER_INCR(&udp_hwcsum_bad);
		goto badcsum;

	case M_CSUM_UDPv4|M_CSUM_DATA: {
		u_int32_t hw_csum = m->m_pkthdr.csum_data;

		UDP_CSUM_COUNTER_INCR(&udp_hwcsum_data);
		if (m->m_pkthdr.csum_flags & M_CSUM_NO_PSEUDOHDR) {
			const struct ip *ip =
			    mtod(m, const struct ip *);

			hw_csum = in_cksum_phdr(ip->ip_src.s_addr,
			    ip->ip_dst.s_addr,
			    htons(hw_csum + len + IPPROTO_UDP));
		}
		if ((hw_csum ^ 0xffff) != 0)
			goto badcsum;
		break;
	}

	case M_CSUM_UDPv4:
		/* Checksum was okay. */
		UDP_CSUM_COUNTER_INCR(&udp_hwcsum_ok);
		break;

	default:
		/*
		 * Need to compute it ourselves.  Maybe skip checksum
		 * on loopback interfaces.
		 */
		if (__predict_true(!(m->m_pkthdr.rcvif->if_flags &
				     IFF_LOOPBACK) ||
				   udp_do_loopback_cksum)) {
			UDP_CSUM_COUNTER_INCR(&udp_swcsum);
			if (in4_cksum(m, IPPROTO_UDP, iphlen, len) != 0)
				goto badcsum;
		}
		break;
	}

	return 0;

badcsum:
	UDP_STATINC(UDP_STAT_BADSUM);
	return -1;
}

void
udp_input(struct mbuf *m, ...)
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
	UDP_STATINC(UDP_STAT_IPACKETS);

	/*
	 * Get IP and UDP header together in first mbuf.
	 */
	ip = mtod(m, struct ip *);
	IP6_EXTHDR_GET(uh, struct udphdr *, m, iphlen, sizeof(struct udphdr));
	if (uh == NULL) {
		UDP_STATINC(UDP_STAT_HDROPS);
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
			UDP_STATINC(UDP_STAT_BADLEN);
			goto bad;
		}
		m_adj(m, iphlen + len - ip_len);
	}

	/*
	 * Checksum extended UDP header and data.
	 */
	if (udp4_input_checksum(m, uh, iphlen, len))
		goto badcsum;

	/* construct source and dst sockaddrs. */
	sockaddr_in_init(&src, &ip->ip_src, uh->uh_sport);
	sockaddr_in_init(&dst, &ip->ip_dst, uh->uh_dport);

	if ((n = udp4_realinput(&src, &dst, &m, iphlen)) == -1) {
		UDP_STATINC(UDP_STAT_HDROPS);
		return;
	}
	if (m == NULL) {
		/*
		 * packet has been processed by ESP stuff -
		 * e.g. dropped NAT-T-keep-alive-packet ...
		 */
		return;
	}
	ip = mtod(m, struct ip *);
#ifdef INET6
	if (IN_MULTICAST(ip->ip_dst.s_addr) || n == 0) {
		struct sockaddr_in6 src6, dst6;

		memset(&src6, 0, sizeof(src6));
		src6.sin6_family = AF_INET6;
		src6.sin6_len = sizeof(struct sockaddr_in6);
		src6.sin6_addr.s6_addr[10] = src6.sin6_addr.s6_addr[11] = 0xff;
		memcpy(&src6.sin6_addr.s6_addr[12], &ip->ip_src,
			sizeof(ip->ip_src));
		src6.sin6_port = uh->uh_sport;
		memset(&dst6, 0, sizeof(dst6));
		dst6.sin6_family = AF_INET6;
		dst6.sin6_len = sizeof(struct sockaddr_in6);
		dst6.sin6_addr.s6_addr[10] = dst6.sin6_addr.s6_addr[11] = 0xff;
		memcpy(&dst6.sin6_addr.s6_addr[12], &ip->ip_dst,
			sizeof(ip->ip_dst));
		dst6.sin6_port = uh->uh_dport;

		n += udp6_realinput(AF_INET, &src6, &dst6, m, iphlen);
	}
#endif

	if (n == 0) {
		if (m->m_flags & (M_BCAST | M_MCAST)) {
			UDP_STATINC(UDP_STAT_NOPORTBCAST);
			goto bad;
		}
		UDP_STATINC(UDP_STAT_NOPORT);
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
}

static void
udp4_sendup(struct mbuf *m, int off /* offset of data portion */,
	struct sockaddr *src, struct socket *so)
{
	struct mbuf *opts = NULL;
	struct mbuf *n;
	inpcb_t *inp = NULL;

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

#if defined(IPSEC)
	/* check AH/ESP integrity. */
	if (so != NULL && ipsec4_in_reject_so(m, so)) {
		IPSEC_STATINC(IPSEC_STAT_IN_POLVIO);
		if ((n = m_copypacket(m, M_DONTWAIT)) != NULL)
			icmp_error(n, ICMP_UNREACH, ICMP_UNREACH_ADMIN_PROHIBIT,
			    0, 0);
		return;
	}
#endif /*IPSEC*/

	if ((n = m_copypacket(m, M_DONTWAIT)) != NULL) {
		if (inp && ((inpcb_get_flags(inp) & INP_CONTROLOPTS) != 0
#ifdef SO_OTIMESTAMP
			 || so->so_options & SO_OTIMESTAMP
#endif
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
			so->so_rcv.sb_overflowed++;
			UDP_STATINC(UDP_STAT_FULLSOCK);
		} else
			sorwakeup(so);
	}
}

struct udp_pcb_ctx {
	struct mbuf *		mbuf;
	struct sockaddr_in *	src;
	struct sockaddr_in *	dst;
	int			off;
	int			rcvcnt;
};

static int
udp4_pcb_process(inpcb_t *inp, void *arg)
{
	struct udp_pcb_ctx *uctx = arg;
	struct in_addr dst4 = uctx->dst->sin_addr;
	in_port_t dport = uctx->dst->sin_port;
	struct in_addr laddr, faddr;
	in_port_t lport, fport;
	struct socket *so;

	inpcb_get_ports(inp, &lport, &fport);
	if (lport != dport) {
		return 0;
	}
	inpcb_get_addrs(inp, &laddr, &faddr);
	if (!in_nullhost(laddr) && !in_hosteq(laddr, dst4)) {
		return 0;
	}
	if (!in_nullhost(faddr)) {
		struct in_addr src4 = uctx->src->sin_addr;
		in_port_t sport = uctx->src->sin_port;

		if (!in_hosteq(faddr, src4) || fport != sport) {
			return 0;
		}
	}

	so = inpcb_get_socket(inp);
	udp4_sendup(uctx->mbuf, uctx->off, (struct sockaddr *)uctx->src, so);
	uctx->rcvcnt++;

	/*
	 * Do not look for additional matches if this one does not have
	 * either the SO_REUSEPORT or SO_REUSEADDR socket options set.
	 * This heuristic avoids searching through all PCBs in the common
	 * case of a non-shared port.  It assumes that an application will
	 * never clear these options after setting them.
	 */
	if ((so->so_options & (SO_REUSEPORT|SO_REUSEADDR)) == 0) {
		return EJUSTRETURN;
	}
	return 0;
}

static int
udp4_realinput(struct sockaddr_in *src, struct sockaddr_in *dst,
    struct mbuf **mp, int off /* offset of udphdr */)
{
	in_port_t *sport, *dport;
	struct in_addr *src4, *dst4;
	inpcb_t *inp;
	struct mbuf *m = *mp;
	int rcvcnt;

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
		struct udp_pcb_ctx uctx = {
			.mbuf = m, .src = src, .dst = dst,
			.off = off, .rcvcnt = 0
		};
		int error;

		/*
		 * Deliver a multicast or broadcast datagram to *all* sockets
		 * for which the local and remote addresses and ports match
		 * those of the incoming datagram.  This allows more than
		 * one process to receive multi/broadcasts on the same port.
		 */
		error = inpcb_foreach(udbtable, AF_INET,
		    udp4_pcb_process, &uctx);
		KASSERT(error == 0 || error == EJUSTRETURN);
		rcvcnt = uctx.rcvcnt;
	} else {
		/*
		 * Locate PCB for datagram.
		 */
		struct socket *so;

		inp = inpcb_lookup_connect(udbtable, *src4, *sport, *dst4,
		    *dport, 0);
		if (inp == NULL) {
			UDP_STATINC(UDP_STAT_PCBHASHMISS);
			inp = inpcb_lookup_bind(udbtable, *dst4, *dport);
			if (inp == NULL)
				return rcvcnt;
		}
		so = inpcb_get_socket(inp);

#ifdef IPSEC
		/* Handle ESP over UDP */
		if (inpcb_get_flags(inp) & INP_ESPINUDP_ALL) {
			struct sockaddr *sa = (struct sockaddr *)src;

			switch (udp4_espinudp(mp, off, sa, so)) {
			case -1: 	/* Error, m was freeed */
				rcvcnt = -1;
				goto bad;
				break;

			case 1:		/* ESP over UDP */
				rcvcnt++;
				goto bad;
				break;

			case 0: 	/* plain UDP */
			default: 	/* Unexpected */
				/* 
				 * Normal UDP processing will take place 
				 * m may have changed.
				 */
				m = *mp;
				break;
			}
		}
#endif

		/*
		 * Check the minimum TTL for socket.
		 */
		if (mtod(m, struct ip *)->ip_ttl < inpcb_get_minttl(inp)) {
			goto bad;
		}
		udp4_sendup(m, off, (struct sockaddr *)src, so);
		rcvcnt++;
	}

bad:
	return rcvcnt;
}
#endif

#ifdef INET
/*
 * Notify a UDP user of an asynchronous error;
 * just wake up so that he can collect error status.
 */
static void
udp_notify(inpcb_t *inp, int errno)
{
	struct socket *so = inpcb_get_socket(inp);

	so->so_error = errno;
	sorwakeup(so);
	sowwakeup(so);
}

void *
udp_ctlinput(int cmd, const struct sockaddr *sa, void *v)
{
	struct ip *ip = v;
	struct udphdr *uh;
	int errno;
	bool rdr;

	if (sa->sa_family != AF_INET ||
	    sa->sa_len != sizeof(struct sockaddr_in))
		return NULL;
	if ((unsigned)cmd >= PRC_NCMDS)
		return NULL;
	errno = inetctlerrmap[cmd];

	rdr = PRC_IS_REDIRECT(cmd);
	if (rdr || cmd == PRC_HOSTDEAD || ip == NULL) {
		inpcb_notifyall(udbtable, satocsin(sa)->sin_addr,
		    errno, rdr ? inpcb_rtchange : udp_notify);
		return NULL;
	} else if (errno == 0) {
		return NULL;
	}

	/* Note: mapped address case */
	uh = (struct udphdr *)((char *)ip + (ip->ip_hl << 2));
	inpcb_notify(udbtable, satocsin(sa)->sin_addr, uh->uh_dport,
	    ip->ip_src, uh->uh_sport, errno, udp_notify);
	return NULL;
}

int
udp_ctloutput(int op, struct socket *so, struct sockopt *sopt)
{
	int s, family, optval, inpflags, error = 0;
	inpcb_t *inp;

	family = so->so_proto->pr_domain->dom_family;

	s = splsoftnet();
	switch (family) {
#ifdef INET
	case PF_INET:
		if (sopt->sopt_level != IPPROTO_UDP) {
			error = ip_ctloutput(op, so, sopt);
			goto end;
		}
		break;
#endif
#ifdef INET6
	case PF_INET6:
		if (sopt->sopt_level != IPPROTO_UDP) {
			error = ip6_ctloutput(op, so, sopt);
			goto end;
		}
		break;
#endif
	default:
		error = EAFNOSUPPORT;
		goto end;
	}

	switch (op) {
	case PRCO_SETOPT:
		inp = sotoinpcb(so);

		switch (sopt->sopt_name) {
		case UDP_ENCAP:
			error = sockopt_getint(sopt, &optval);
			if (error)
				break;

			inpflags = inpcb_get_flags(inp);
			switch(optval) {
			case 0:
				inpflags &= ~INP_ESPINUDP_ALL;
				break;

			case UDP_ENCAP_ESPINUDP:
				inpflags &= ~INP_ESPINUDP_ALL;
				inpflags |= INP_ESPINUDP;
				break;

			case UDP_ENCAP_ESPINUDP_NON_IKE:
				inpflags &= ~INP_ESPINUDP_ALL;
				inpflags |= INP_ESPINUDP_NON_IKE;
				break;
			default:
				error = EINVAL;
				break;
			}
			inpcb_set_flags(inp, inpflags);
			break;

		default:
			error = ENOPROTOOPT;
			break;
		}
		break;

	default:
		error = EINVAL;
		break;
	}

end:
	splx(s);
	return error;
}


int
udp_output(struct mbuf *m, ...)
{
	inpcb_t *inp;
	struct socket *so;
	struct udpiphdr *ui;
	struct route *ro;
	int len = m->m_pkthdr.len;
	int error = 0;
	va_list ap;

	MCLAIM(m, &udp_tx_mowner);
	va_start(ap, m);
	inp = va_arg(ap, inpcb_t *);
	va_end(ap);

	so = inpcb_get_socket(inp);
	KASSERT(solocked(so));

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

	inpcb_get_addrs(inp, &ui->ui_src, &ui->ui_dst);
	inpcb_get_ports(inp, &ui->ui_sport, &ui->ui_dport);
	ui->ui_ulen = htons((u_int16_t)len + sizeof(struct udphdr));

	ro = inpcb_get_route(inp);

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

	struct ip *inp_ip = in_getiphdr(inp);
	((struct ip *)ui)->ip_ttl = inp_ip->ip_ttl;	/* XXX */
	((struct ip *)ui)->ip_tos = inp_ip->ip_tos;	/* XXX */
	UDP_STATINC(UDP_STAT_OPACKETS);

	return (ip_output(m, inpcb_get_options(inp), ro,
	    so->so_options & (SO_DONTROUTE | SO_BROADCAST),
	    inpcb_get_moptions(inp), so));

release:
	m_freem(m);
	return error;
}

static int
udp_attach(struct socket *so, int proto)
{
	inpcb_t *inp;
	struct ip *ip;
	int s, error;

	KASSERT(sotoinpcb(so) == NULL);

	s = splsoftnet();
	sosetlock(so);

#ifdef MBUFTRACE
	so->so_mowner = &udp_mowner;
	so->so_rcv.sb_mowner = &udp_rx_mowner;
	so->so_snd.sb_mowner = &udp_tx_mowner;
#endif
	if (so->so_snd.sb_hiwat == 0 || so->so_rcv.sb_hiwat == 0) {
		error = soreserve(so, udp_sendspace, udp_recvspace);
		if (error) {
			goto out;
		}
	}

	error = inpcb_create(so, udbtable);
	if (error) {
		goto out;
	}
	inp = sotoinpcb(so);
	ip = in_getiphdr(inp);
	ip->ip_ttl = ip_defttl;
out:
	splx(s);
	return error;
}

static void
udp_detach(struct socket *so)
{
	inpcb_t *inp;
	int s;

	KASSERT(solocked(so));

	s = splsoftnet();
	inp = sotoinpcb(so);
	KASSERT(inp != NULL);
	inpcb_destroy(inp);
	splx(s);
}

static int
udp_usrreq(struct socket *so, int req, struct mbuf *m, struct mbuf *nam,
    struct mbuf *control, struct lwp *l)
{
	inpcb_t *inp;
	int s, error = 0;

	KASSERT(req != PRU_ATTACH);
	KASSERT(req != PRU_DETACH);

	if (req == PRU_CONTROL) {
		return in_control(so, (long)m, nam, (ifnet_t *)control, l);
	}
	s = splsoftnet();
	if (req == PRU_PURGEIF) {
		mutex_enter(softnet_lock);
		inpcb_purgeif0(udbtable, (ifnet_t *)control);
		in_purgeif((ifnet_t *)control);
		inpcb_purgeif(udbtable, (ifnet_t *)control);
		mutex_exit(softnet_lock);
		splx(s);
		return (0);
	}

	KASSERT(solocked(so));
	inp = sotoinpcb(so);

	KASSERT(!control || (req == PRU_SEND || req == PRU_SENDOOB));
	if (inp == NULL) {
		error = EINVAL;
		goto release;
	}

	/*
	 * Note: need to block udp_input while changing
	 * the udp pcb queue and/or pcb addresses.
	 */
	switch (req) {
	case PRU_BIND:
		error = inpcb_bind(inp, nam, l);
		break;

	case PRU_LISTEN:
		error = EOPNOTSUPP;
		break;

	case PRU_CONNECT:
		error = inpcb_connect(inp, nam, l);
		if (error)
			break;
		soisconnected(so);
		break;

	case PRU_CONNECT2:
		error = EOPNOTSUPP;
		break;

	case PRU_DISCONNECT:
		/*soisdisconnected(so);*/
		so->so_state &= ~SS_ISCONNECTED;		/* XXX */
		inpcb_disconnect(inp);
		inpcb_set_addrs(inp, &zeroin_addr, NULL);	/* XXX */
		inpcb_set_state(inp, INP_BOUND);		/* XXX */
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
		/*
		 * Note: sendto case - temporarily connect the socket
		 * to the destination, send and then disconnect.
		 * XXX: save the local address, restore after.
		 */
		struct in_addr laddr;

		memset(&laddr, 0, sizeof laddr);
		if (nam) {
			inpcb_get_addrs(inp, &laddr, NULL);
			if ((so->so_state & SS_ISCONNECTED) != 0) {
				error = EISCONN;
				goto die;
			}
			error = inpcb_connect(inp, nam, l);
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
			inpcb_disconnect(inp);
			inpcb_set_addrs(inp, &laddr, NULL);
			inpcb_set_state(inp, INP_BOUND);
		}
die:
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
		inpcb_fetch_sockaddr(inp, nam);
		break;

	case PRU_PEERADDR:
		inpcb_fetch_peeraddr(inp, nam);
		break;

	default:
		panic("udp_usrreq");
	}

release:
	splx(s);
	return (error);
}

static int
sysctl_net_inet_udp_stats(SYSCTLFN_ARGS)
{

	return (NETSTAT_SYSCTL(udpstat_percpu, UDP_NSTATS));
}

/*
 * Sysctl for udp variables.
 */
static void
sysctl_net_inet_udp_setup(struct sysctllog **clog)
{
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_NODE, "net", NULL,
		       NULL, 0, NULL, 0,
		       CTL_NET, CTL_EOL);
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_NODE, "inet", NULL,
		       NULL, 0, NULL, 0,
		       CTL_NET, PF_INET, CTL_EOL);
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_NODE, "udp",
		       SYSCTL_DESCR("UDPv4 related settings"),
		       NULL, 0, NULL, 0,
		       CTL_NET, PF_INET, IPPROTO_UDP, CTL_EOL);

	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT|CTLFLAG_READWRITE,
		       CTLTYPE_INT, "checksum",
		       SYSCTL_DESCR("Compute UDP checksums"),
		       NULL, 0, &udpcksum, 0,
		       CTL_NET, PF_INET, IPPROTO_UDP, UDPCTL_CHECKSUM,
		       CTL_EOL);
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT|CTLFLAG_READWRITE,
		       CTLTYPE_INT, "sendspace",
		       SYSCTL_DESCR("Default UDP send buffer size"),
		       NULL, 0, &udp_sendspace, 0,
		       CTL_NET, PF_INET, IPPROTO_UDP, UDPCTL_SENDSPACE,
		       CTL_EOL);
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT|CTLFLAG_READWRITE,
		       CTLTYPE_INT, "recvspace",
		       SYSCTL_DESCR("Default UDP receive buffer size"),
		       NULL, 0, &udp_recvspace, 0,
		       CTL_NET, PF_INET, IPPROTO_UDP, UDPCTL_RECVSPACE,
		       CTL_EOL);
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT|CTLFLAG_READWRITE,
		       CTLTYPE_INT, "do_loopback_cksum",
		       SYSCTL_DESCR("Perform UDP checksum on loopback"),
		       NULL, 0, &udp_do_loopback_cksum, 0,
		       CTL_NET, PF_INET, IPPROTO_UDP, UDPCTL_LOOPBACKCKSUM,
		       CTL_EOL);
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_STRUCT, "pcblist",
		       SYSCTL_DESCR("UDP protocol control block list"),
		       sysctl_inpcblist, 0, udbtable, 0,
		       CTL_NET, PF_INET, IPPROTO_UDP, CTL_CREATE,
		       CTL_EOL);
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_STRUCT, "stats",
		       SYSCTL_DESCR("UDP statistics"),
		       sysctl_net_inet_udp_stats, 0, NULL, 0,
		       CTL_NET, PF_INET, IPPROTO_UDP, UDPCTL_STATS,
		       CTL_EOL);
}
#endif

void
udp_statinc(u_int stat)
{

	KASSERT(stat < UDP_NSTATS);
	UDP_STATINC(stat);
}

#if defined(INET) && defined(IPSEC)
/*
 * Returns:
 * 1 if the packet was processed
 * 0 if normal UDP processing should take place
 * -1 if an error occurent and m was freed
 */
static int
udp4_espinudp(struct mbuf **mp, int off, struct sockaddr *src,
    struct socket *so)
{
	size_t len;
	void *data;
	inpcb_t *inp;
	size_t skip = 0;
	size_t minlen;
	size_t iphdrlen;
	struct ip *ip;
	struct m_tag *tag;
	struct udphdr *udphdr;
	in_port_t sport, dport;
	struct mbuf *m = *mp;
	int inpflags;

	/*
	 * Collapse the mbuf chain if the first mbuf is too short
	 * The longest case is: UDP + non ESP marker + ESP
	 */
	minlen = off + sizeof(u_int64_t) + sizeof(struct esp);
	if (minlen > m->m_pkthdr.len)
		minlen = m->m_pkthdr.len;

	if (m->m_len < minlen) {
		if ((*mp = m_pullup(m, minlen)) == NULL) {
			printf("udp4_espinudp: m_pullup failed\n");
			return -1;
		}
		m = *mp;
	}

	len = m->m_len - off;
	data = mtod(m, char *) + off;
	inp = sotoinpcb(so);

	/* Ignore keepalive packets */
	if ((len == 1) && (*(unsigned char *)data == 0xff)) {
		m_free(m);
		*mp = NULL; /* avoid any further processiong by caller ... */
		return 1;
	}
	inpflags = inpcb_get_flags(inp);

	/*
	 * Check that the payload is long enough to hold
	 * an ESP header and compute the length of encapsulation
	 * header to remove
	 */
	if (inpflags & INP_ESPINUDP) {
		u_int32_t *st = (u_int32_t *)data;

		if ((len <= sizeof(struct esp)) || (*st == 0))
			return 0; /* Normal UDP processing */

		skip = sizeof(struct udphdr);
	}

	if (inpflags & INP_ESPINUDP_NON_IKE) {
		u_int32_t *st = (u_int32_t *)data;

		if ((len <= sizeof(u_int64_t) + sizeof(struct esp))
		    || ((st[0] | st[1]) != 0))
			return 0; /* Normal UDP processing */

		skip = sizeof(struct udphdr) + sizeof(u_int64_t);
	}

	/*
	 * Get the UDP ports. They are handled in network 
	 * order everywhere in IPSEC_NAT_T code.
	 */
	udphdr = (struct udphdr *)((char *)data - skip);
	sport = udphdr->uh_sport;
	dport = udphdr->uh_dport;

	/*
	 * Remove the UDP header (and possibly the non ESP marker)
	 * IP header lendth is iphdrlen
	 * Before:
	 *   <--- off --->
	 *   +----+------+-----+
	 *   | IP |  UDP | ESP |
	 *   +----+------+-----+
	 *        <-skip->
	 * After:
	 *          +----+-----+
	 *          | IP | ESP |
	 *          +----+-----+
	 *   <-skip->
	 */
	iphdrlen = off - sizeof(struct udphdr);
	memmove(mtod(m, char *) + skip, mtod(m, void *), iphdrlen);
	m_adj(m, skip);

	ip = mtod(m, struct ip *);
	ip->ip_len = htons(ntohs(ip->ip_len) - skip);
	ip->ip_p = IPPROTO_ESP;

	/*
	 * We have modified the packet - it is now ESP, so we should not
	 * return to UDP processing ... 
	 *
	 * Add a PACKET_TAG_IPSEC_NAT_T_PORT tag to remember
	 * the source UDP port. This is required if we want
	 * to select the right SPD for multiple hosts behind 
	 * same NAT 
	 */
	if ((tag = m_tag_get(PACKET_TAG_IPSEC_NAT_T_PORTS,
	    sizeof(sport) + sizeof(dport), M_DONTWAIT)) == NULL) {
		printf("udp4_espinudp: m_tag_get failed\n");
		m_freem(m);
		return -1;
	}
	((u_int16_t *)(tag + 1))[0] = sport;
	((u_int16_t *)(tag + 1))[1] = dport;
	m_tag_prepend(m, tag);

#ifdef IPSEC
	ipsec4_common_input(m, iphdrlen, IPPROTO_ESP);
#else
	esp4_input(m, iphdrlen);
#endif

	/* We handled it, it shouldn't be handled by UDP */
	*mp = NULL; /* avoid free by caller ... */
	return 1;
}
#endif

PR_WRAP_USRREQ(udp_usrreq)

#define	udp_usrreq	udp_usrreq_wrapper

const struct pr_usrreqs udp_usrreqs = {
	.pr_attach	= udp_attach,
	.pr_detach	= udp_detach,
	.pr_generic	= udp_usrreq,
};
