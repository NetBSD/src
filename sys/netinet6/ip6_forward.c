/*	$NetBSD: ip6_forward.c,v 1.3 1999/07/03 21:30:18 thorpej Exp $	*/

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

#include <netinet/in.h>
#include <netinet/in_var.h>
#include <netinet6/in6_systm.h>
#include <netinet6/ip6.h>
#if !defined(__FreeBSD__) || __FreeBSD__ < 3
#include <netinet6/in6_pcb.h>
#endif
#include <netinet6/ip6_var.h>
#include <netinet6/icmp6.h>

struct	route_in6 ip6_forward_rt;

/*
 * Forward a packet.  If some error occurs return the sender
 * an icmp packet.  Note we can't always generate a meaningful
 * icmp message because icmp doesn't have a large enough repertoire
 * of codes and types.
 *
 * If not forwarding, just drop the packet.  This could be confusing
 * if ipforwarding was zero but some routing protocol was advancing
 * us as a gateway to somewhere.  However, we must let the routing
 * protocol deal with that.
 *
 */

void
ip6_forward(m, srcrt)
	struct mbuf *m;
	int srcrt;
{
	register struct ip6_hdr *ip6 = mtod(m, struct ip6_hdr *);
	register struct sockaddr_in6 *dst;
	register struct rtentry *rt;
	int error, type = 0, code = 0;
	struct mbuf *mcopy;

	if (m->m_flags & (M_BCAST|M_MCAST) ||
	   in6_canforward(&ip6->ip6_src, &ip6->ip6_dst) == 0) {
		ip6stat.ip6s_cantforward++;
		ip6stat.ip6s_badscope++;
#if !defined(__FreeBSD__) || __FreeBSD__ < 3
		if (ip6_log_time + ip6_log_interval < time.tv_sec) {
			ip6_log_time = time.tv_sec;
#else
		if (ip6_log_time + ip6_log_interval < time_second) {
			ip6_log_time = time_second;
#endif
			log(LOG_DEBUG,
			    "cannot forward "
			    "from %s to %s nxt %d received on %s\n",
			    ip6_sprintf(&ip6->ip6_src), /* XXX meaningless */
			    ip6_sprintf(&ip6->ip6_dst),
			    ip6->ip6_nxt,
			    m->m_pkthdr.rcvif->if_xname);
		}
		m_freem(m);
		return;
	}

	if (ip6->ip6_hlim <= IPV6_HLIMDEC) {
		icmp6_error(m, ICMP6_TIME_EXCEEDED,
				ICMP6_TIME_EXCEED_TRANSIT, 0);
		return;
	}
	ip6->ip6_hlim -= IPV6_HLIMDEC;

	dst = &ip6_forward_rt.ro_dst;
	if (!srcrt) {
		/*
		 * ip6_forward_rt.ro_dst.sin6_addr is equal to ip6->ip6_dst
		 */
		if (ip6_forward_rt.ro_rt == 0 ||
		    (ip6_forward_rt.ro_rt->rt_flags & RTF_UP) == 0) {
			if (ip6_forward_rt.ro_rt) {
				RTFREE(ip6_forward_rt.ro_rt);
				ip6_forward_rt.ro_rt = 0;
			}
			/* this probably fails but give it a try again */
#ifdef __NetBSD__
			rtalloc((struct route *)&ip6_forward_rt);
#else
			rtalloc_ign((struct route *)&ip6_forward_rt,
				    RTF_PRCLONING);
#endif
		}
		
		if (ip6_forward_rt.ro_rt == 0) {
			ip6stat.ip6s_noroute++;
			icmp6_error(m, ICMP6_DST_UNREACH,
				    ICMP6_DST_UNREACH_NOROUTE, 0);
			return;
		}
	} else if ((rt = ip6_forward_rt.ro_rt) == 0 ||
		 !IN6_ARE_ADDR_EQUAL(&ip6->ip6_dst, &dst->sin6_addr)) {
		if (ip6_forward_rt.ro_rt) {
			RTFREE(ip6_forward_rt.ro_rt);
			ip6_forward_rt.ro_rt = 0;
		}
		bzero(dst, sizeof(*dst));
		dst->sin6_len = sizeof(struct sockaddr_in6);
		dst->sin6_family = AF_INET6;
		dst->sin6_addr = ip6->ip6_dst;

#ifdef __NetBSD__
		rtalloc((struct route *)&ip6_forward_rt);
#else
  		rtalloc_ign((struct route *)&ip6_forward_rt, RTF_PRCLONING);
#endif
		if (ip6_forward_rt.ro_rt == 0) {
			ip6stat.ip6s_noroute++;
			icmp6_error(m, ICMP6_DST_UNREACH,
				    ICMP6_DST_UNREACH_NOROUTE, 0);
			return;
		}
	}
	rt = ip6_forward_rt.ro_rt;
	if (m->m_pkthdr.len > rt->rt_ifp->if_mtu){
 		icmp6_error(m, ICMP6_PACKET_TOO_BIG, 0, rt->rt_ifp->if_mtu);
		return;
 	}

	if (rt->rt_flags & RTF_GATEWAY)
		dst = (struct sockaddr_in6 *)rt->rt_gateway;
	/*
	 * Save at most 528 bytes of the packet in case
	 * we need to generate an ICMP6 message to the src.
	 * Thanks to M_EXT, in most cases copy will not occur.
	 */
	mcopy = m_copy(m, 0, imin(m->m_pkthdr.len, ICMPV6_PLD_MAXLEN));

	/*
	 * If we are to forward the packet using the same interface
	 * as one we got the packet from, perhaps we should send a redirect
	 * to sender to shortcut a hop.
	 * Only send redirect if source is sending directly to us,
	 * and if packet was not source routed (or has any options).
	 * Also, don't send redirect if forwarding using a route
	 * modified by a redirect.
	 */
	if (rt->rt_ifp == m->m_pkthdr.rcvif && !srcrt &&
	    (rt->rt_flags & (RTF_DYNAMIC|RTF_MODIFIED)) == 0)
		type = ND_REDIRECT;

	error = (*rt->rt_ifp->if_output)(rt->rt_ifp, m,
					 (struct sockaddr *)dst,
					 ip6_forward_rt.ro_rt);
	if (error)
		ip6stat.ip6s_cantforward++;
	else {
		ip6stat.ip6s_forward++;
		if (type)
			ip6stat.ip6s_redirectsent++;
		else {
			if (mcopy)
				goto freecopy;
		}
	}
	if (mcopy == NULL)
		return;

	switch (error) {
	case 0:
#if 1
		if (type == ND_REDIRECT) {
			icmp6_redirect_output(mcopy, rt);
			return;
		}
#endif
		goto freecopy;

	case EMSGSIZE:
		/* xxx MTU is constant in PPP? */
		goto freecopy;

	case ENOBUFS:
		/* Tell source to slow down like source quench in IP? */
		goto freecopy;

	case ENETUNREACH:	/* shouldn't happen, checked above */
	case EHOSTUNREACH:
	case ENETDOWN:
	case EHOSTDOWN:
	default:
		type = ICMP6_DST_UNREACH;
		code = ICMP6_DST_UNREACH_ADDR;
		break;
	}
	icmp6_error(mcopy, type, code, 0);
	return;

 freecopy:
	m_freem(mcopy);
}
