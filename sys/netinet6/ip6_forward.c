/*	$NetBSD: ip6_forward.c,v 1.69.6.2 2018/04/01 09:20:22 martin Exp $	*/
/*	$KAME: ip6_forward.c,v 1.109 2002/09/11 08:10:17 sakane Exp $	*/

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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: ip6_forward.c,v 1.69.6.2 2018/04/01 09:20:22 martin Exp $");

#include "opt_gateway.h"
#include "opt_ipsec.h"
#include "opt_pfil_hooks.h"

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
#include <netinet/ip_var.h>
#include <netinet/ip6.h>
#include <netinet6/ip6_var.h>
#include <netinet6/ip6_private.h>
#include <netinet6/scope6_var.h>
#include <netinet/icmp6.h>
#include <netinet6/nd6.h>

#ifdef KAME_IPSEC
#include <netinet6/ipsec.h>
#include <netinet6/ipsec_private.h>
#include <netkey/key.h>
#endif /* KAME_IPSEC */

#ifdef FAST_IPSEC
#include <netipsec/ipsec.h>
#include <netipsec/ipsec6.h>
#include <netipsec/key.h>
#include <netipsec/xform.h>
#endif /* FAST_IPSEC */

#ifdef PFIL_HOOKS
#include <net/pfil.h>
#endif

#include <net/net_osdep.h>

struct	route ip6_forward_rt;

#ifdef PFIL_HOOKS
extern struct pfil_head inet6_pfil_hook;	/* XXX */
#endif

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
ip6_forward(struct mbuf *m, int srcrt)
{
	struct ip6_hdr *ip6 = mtod(m, struct ip6_hdr *);
	const struct sockaddr_in6 *dst;
	struct rtentry *rt;
	int error = 0, type = 0, code = 0;
	struct mbuf *mcopy = NULL;
	struct ifnet *origifp;	/* maybe unnecessary */
	u_int32_t inzone, outzone;
	struct in6_addr src_in6, dst_in6;
#ifdef KAME_IPSEC
	struct secpolicy *sp = NULL;
	int ipsecrt = 0;
#endif
#ifdef FAST_IPSEC
    struct secpolicy *sp = NULL;
    int needipsec = 0;
    int s;
#endif

	/*
	 * Clear any in-bound checksum flags for this packet.
	 */
	m->m_pkthdr.csum_flags = 0;

#ifdef KAME_IPSEC
	/*
	 * Check AH/ESP integrity.
	 */
	/*
	 * Don't increment ip6s_cantforward because this is the check
	 * before forwarding packet actually.
	 */
	if (ipsec6_in_reject(m, NULL)) {
		IPSEC6_STATINC(IPSEC_STAT_IN_POLVIO);
		m_freem(m);
		return;
	}
#endif /* KAME_IPSEC */

	/*
	 * Do not forward packets to multicast destination (should be handled
	 * by ip6_mforward().
	 * Do not forward packets with unspecified source.  It was discussed
	 * in July 2000, on ipngwg mailing list.
	 */
	if ((m->m_flags & (M_BCAST|M_MCAST)) != 0 ||
	    IN6_IS_ADDR_MULTICAST(&ip6->ip6_dst) ||
	    IN6_IS_ADDR_UNSPECIFIED(&ip6->ip6_src)) {
		IP6_STATINC(IP6_STAT_CANTFORWARD);
		/* XXX in6_ifstat_inc(rt->rt_ifp, ifs6_in_discard) */
		if (ip6_log_time + ip6_log_interval < time_second) {
			ip6_log_time = time_second;
			log(LOG_DEBUG,
			    "cannot forward "
			    "from %s to %s nxt %d received on %s\n",
			    ip6_sprintf(&ip6->ip6_src),
			    ip6_sprintf(&ip6->ip6_dst),
			    ip6->ip6_nxt,
			    if_name(m->m_pkthdr.rcvif));
		}
		m_freem(m);
		return;
	}

	if (ip6->ip6_hlim <= IPV6_HLIMDEC) {
		/* XXX in6_ifstat_inc(rt->rt_ifp, ifs6_in_discard) */
		icmp6_error(m, ICMP6_TIME_EXCEEDED,
				ICMP6_TIME_EXCEED_TRANSIT, 0);
		return;
	}
	ip6->ip6_hlim -= IPV6_HLIMDEC;

	/*
	 * Save at most ICMPV6_PLD_MAXLEN (= the min IPv6 MTU -
	 * size of IPv6 + ICMPv6 headers) bytes of the packet in case
	 * we need to generate an ICMP6 message to the src.
	 * Thanks to M_EXT, in most cases copy will not occur.
	 *
	 * It is important to save it before IPsec processing as IPsec
	 * processing may modify the mbuf.
	 */
	mcopy = m_copy(m, 0, imin(m->m_pkthdr.len, ICMPV6_PLD_MAXLEN));

#ifdef KAME_IPSEC
	/* get a security policy for this packet */
	sp = ipsec6_getpolicybyaddr(m, IPSEC_DIR_OUTBOUND,
	    IP_FORWARDING, &error);
	if (sp == NULL) {
		IPSEC6_STATINC(IPSEC_STAT_OUT_INVAL);
		IP6_STATINC(IP6_STAT_CANTFORWARD);
		if (mcopy) {
#if 0
			/* XXX: what icmp ? */
#else
			m_freem(mcopy);
#endif
		}
		m_freem(m);
		return;
	}

	error = 0;

	/* check policy */
	switch (sp->policy) {
	case IPSEC_POLICY_DISCARD:
		/*
		 * This packet is just discarded.
		 */
		IPSEC6_STATINC(IPSEC_STAT_OUT_POLVIO);
		IP6_STATINC(IP6_STAT_CANTFORWARD);
		key_freesp(sp);
		if (mcopy) {
#if 0
			/* XXX: what icmp ? */
#else
			m_freem(mcopy);
#endif
		}
		m_freem(m);
		return;

	case IPSEC_POLICY_BYPASS:
	case IPSEC_POLICY_NONE:
		/* no need to do IPsec. */
		key_freesp(sp);
		goto skip_ipsec;

	case IPSEC_POLICY_IPSEC:
		if (sp->req == NULL) {
			/* XXX should be panic ? */
			printf("ip6_forward: No IPsec request specified.\n");
			IP6_STATINC(IP6_STAT_CANTFORWARD);
			key_freesp(sp);
			if (mcopy) {
#if 0
				/* XXX: what icmp ? */
#else
				m_freem(mcopy);
#endif
			}
			m_freem(m);
			return;
		}
		/* do IPsec */
		break;

	case IPSEC_POLICY_ENTRUST:
	default:
		/* should be panic ?? */
		printf("ip6_forward: Invalid policy found. %d\n", sp->policy);
		key_freesp(sp);
		goto skip_ipsec;
	}

    {
	struct ipsecrequest *isr = NULL;
	struct ipsec_output_state state;

	/*
	 * when the kernel forwards a packet, it is not proper to apply
	 * IPsec transport mode to the packet is not proper.  this check
	 * avoid from this.
	 * at present, if there is even a transport mode SA request in the
	 * security policy, the kernel does not apply IPsec to the packet.
	 * this check is not enough because the following case is valid.
	 *      ipsec esp/tunnel/xxx-xxx/require esp/transport//require;
	 */
	for (isr = sp->req; isr; isr = isr->next) {
		if (isr->saidx.mode == IPSEC_MODE_ANY)
			goto doipsectunnel;
		if (isr->saidx.mode == IPSEC_MODE_TUNNEL)
			goto doipsectunnel;
	}

	/*
	 * if there's no need for tunnel mode IPsec, skip.
	 */
	if (!isr)
		goto skip_ipsec;

    doipsectunnel:
	/*
	 * All the extension headers will become inaccessible
	 * (since they can be encrypted).
	 * Don't panic, we need no more updates to extension headers
	 * on inner IPv6 packet (since they are now encapsulated).
	 *
	 * IPv6 [ESP|AH] IPv6 [extension headers] payload
	 */
	memset(&state, 0, sizeof(state));
	state.m = m;
	state.ro = NULL;	/* update at ipsec6_output_tunnel() */
	state.dst = NULL;	/* update at ipsec6_output_tunnel() */

	error = ipsec6_output_tunnel(&state, sp, 0);

	m = state.m;
	key_freesp(sp);

	if (error) {
		/* mbuf is already reclaimed in ipsec6_output_tunnel. */
		switch (error) {
		case EHOSTUNREACH:
		case ENETUNREACH:
		case EMSGSIZE:
		case ENOBUFS:
		case ENOMEM:
			break;
		default:
			printf("ip6_forward (ipsec): error code %d\n", error);
			/* FALLTHROUGH */
		case ENOENT:
			/* don't show these error codes to the user */
			break;
		}
		IP6_STATINC(IP6_STAT_CANTFORWARD);
		if (mcopy) {
#if 0
			/* XXX: what icmp ? */
#else
			m_freem(mcopy);
#endif
		}
		m_freem(m);
		return;
	}

	if (ip6 != mtod(m, struct ip6_hdr *)) {
		/*
		 * now tunnel mode headers are added.  we are originating
		 * packet instead of forwarding the packet.
		 */
		ip6_output(m, NULL, NULL, IPV6_FORWARDING/*XXX*/, NULL, NULL,
		    NULL);
		goto freecopy;
	}

	/* adjust pointer */
	rt = state.ro ? rtcache_validate(state.ro) : NULL;
	dst = (const struct sockaddr_in6 *)state.dst;
	if (dst != NULL && rt != NULL) {
		ipsecrt = 1;
		goto skip_routing;
	}
    }
    skip_ipsec:
#endif /* KAME_IPSEC */
#ifdef FAST_IPSEC
	/* Check the security policy (SP) for the packet */

	sp = ipsec6_check_policy(m,NULL,0,&needipsec,&error);
	if (error != 0) {
		/*
		 * Hack: -EINVAL is used to signal that a packet
		 * should be silently discarded.  This is typically
		 * because we asked key management for an SA and
		 * it was delayed (e.g. kicked up to IKE).
		 */
		if (error == -EINVAL)
			error = 0;
		m_freem(m);
		goto freecopy;
	}
#endif /* FAST_IPSEC */

	if (srcrt) {
		union {
			struct sockaddr		dst;
			struct sockaddr_in6	dst6;
		} u;

		sockaddr_in6_init(&u.dst6, &ip6->ip6_dst, 0, 0, 0);
		if ((rt = rtcache_lookup(&ip6_forward_rt, &u.dst)) == NULL) {
			IP6_STATINC(IP6_STAT_NOROUTE);
			/* XXX in6_ifstat_inc(rt->rt_ifp, ifs6_in_noroute) */
			if (mcopy) {
				icmp6_error(mcopy, ICMP6_DST_UNREACH,
					    ICMP6_DST_UNREACH_NOROUTE, 0);
			}
			m_freem(m);
			return;
		}
	} else if ((rt = rtcache_validate(&ip6_forward_rt)) == NULL &&
	           (rt = rtcache_update(&ip6_forward_rt, 1)) == NULL) {
		/*
		 * rtcache_getdst(ip6_forward_rt)->sin6_addr was equal to
		 * ip6->ip6_dst
		 */
		IP6_STATINC(IP6_STAT_NOROUTE);
		/* XXX in6_ifstat_inc(rt->rt_ifp, ifs6_in_noroute) */
		if (mcopy) {
			icmp6_error(mcopy, ICMP6_DST_UNREACH,
			    ICMP6_DST_UNREACH_NOROUTE, 0);
		}
		m_freem(m);
		return;
	}
	dst = satocsin6(rtcache_getdst(&ip6_forward_rt));
#ifdef KAME_IPSEC
    skip_routing:;
#endif /* KAME_IPSEC */

	/*
	 * Source scope check: if a packet can't be delivered to its
	 * destination for the reason that the destination is beyond the scope
	 * of the source address, discard the packet and return an icmp6
	 * destination unreachable error with Code 2 (beyond scope of source
	 * address).  We use a local copy of ip6_src, since in6_setscope()
	 * will possibly modify its first argument.
	 * [draft-ietf-ipngwg-icmp-v3-07, Section 3.1]
	 */
	src_in6 = ip6->ip6_src;
	if (in6_setscope(&src_in6, rt->rt_ifp, &outzone)) {
		/* XXX: this should not happen */
		uint64_t *ip6s = IP6_STAT_GETREF();
		ip6s[IP6_STAT_CANTFORWARD]++;
		ip6s[IP6_STAT_BADSCOPE]++;
		IP6_STAT_PUTREF();
		m_freem(m);
		return;
	}
	if (in6_setscope(&src_in6, m->m_pkthdr.rcvif, &inzone)) {
		uint64_t *ip6s = IP6_STAT_GETREF();
		ip6s[IP6_STAT_CANTFORWARD]++;
		ip6s[IP6_STAT_BADSCOPE]++;
		IP6_STAT_PUTREF();
		m_freem(m);
		return;
	}
	if (inzone != outzone
#ifdef KAME_IPSEC
	    && !ipsecrt
#endif
	    ) {
		uint64_t *ip6s = IP6_STAT_GETREF();
		ip6s[IP6_STAT_CANTFORWARD]++;
		ip6s[IP6_STAT_BADSCOPE]++;
		IP6_STAT_PUTREF();
		in6_ifstat_inc(rt->rt_ifp, ifs6_in_discard);

		if (ip6_log_time + ip6_log_interval < time_second) {
			ip6_log_time = time_second;
			log(LOG_DEBUG,
			    "cannot forward "
			    "src %s, dst %s, nxt %d, rcvif %s, outif %s\n",
			    ip6_sprintf(&ip6->ip6_src),
			    ip6_sprintf(&ip6->ip6_dst),
			    ip6->ip6_nxt,
			    if_name(m->m_pkthdr.rcvif), if_name(rt->rt_ifp));
		}
		if (mcopy)
			icmp6_error(mcopy, ICMP6_DST_UNREACH,
				    ICMP6_DST_UNREACH_BEYONDSCOPE, 0);
		m_freem(m);
		return;
	}
#ifdef FAST_IPSEC
    /*
     * If we need to encapsulate the packet, do it here
     * ipsec6_proces_packet will send the packet using ip6_output 
     */
	if (needipsec) {
		s = splsoftnet();
		error = ipsec6_process_packet(m,sp->req);
		splx(s);
		/* m is freed */
		if (mcopy)
			goto freecopy;
		return;
    }
#endif   



	/*
	 * Destination scope check: if a packet is going to break the scope
	 * zone of packet's destination address, discard it.  This case should
	 * usually be prevented by appropriately-configured routing table, but
	 * we need an explicit check because we may mistakenly forward the
	 * packet to a different zone by (e.g.) a default route.
	 */
	dst_in6 = ip6->ip6_dst;
	if (in6_setscope(&dst_in6, m->m_pkthdr.rcvif, &inzone) != 0 ||
	    in6_setscope(&dst_in6, rt->rt_ifp, &outzone) != 0 ||
	    inzone != outzone) {
		uint64_t *ip6s = IP6_STAT_GETREF();
		ip6s[IP6_STAT_CANTFORWARD]++;
		ip6s[IP6_STAT_BADSCOPE]++;
		IP6_STAT_PUTREF();
		m_freem(m);
		return;
	}

	if (m->m_pkthdr.len > IN6_LINKMTU(rt->rt_ifp)) {
		in6_ifstat_inc(rt->rt_ifp, ifs6_in_toobig);
		if (mcopy) {
			u_long mtu;
#ifdef KAME_IPSEC
			struct secpolicy *xsp;
			int ipsecerror;
			size_t ipsechdrsiz;
#endif

			mtu = IN6_LINKMTU(rt->rt_ifp);
#ifdef KAME_IPSEC
			/*
			 * When we do IPsec tunnel ingress, we need to play
			 * with the link value (decrement IPsec header size
			 * from mtu value).  The code is much simpler than v4
			 * case, as we have the outgoing interface for
			 * encapsulated packet as "rt->rt_ifp".
			 */
			xsp = ipsec6_getpolicybyaddr(mcopy, IPSEC_DIR_OUTBOUND,
				IP_FORWARDING, &ipsecerror);
			if (xsp) {
				ipsechdrsiz = ipsec6_hdrsiz(mcopy,
					IPSEC_DIR_OUTBOUND, NULL);
				if (ipsechdrsiz < mtu)
					mtu -= ipsechdrsiz;
			}

			/*
			 * if mtu becomes less than minimum MTU,
			 * tell minimum MTU (and I'll need to fragment it).
			 */
			if (mtu < IPV6_MMTU)
				mtu = IPV6_MMTU;
#endif
			icmp6_error(mcopy, ICMP6_PACKET_TOO_BIG, 0, mtu);
		}
		m_freem(m);
		return;
	}

	if (rt->rt_flags & RTF_GATEWAY)
		dst = (struct sockaddr_in6 *)rt->rt_gateway;

	/*
	 * If we are to forward the packet using the same interface
	 * as one we got the packet from, perhaps we should send a redirect
	 * to sender to shortcut a hop.
	 * Only send redirect if source is sending directly to us,
	 * and if packet was not source routed (or has any options).
	 * Also, don't send redirect if forwarding using a route
	 * modified by a redirect.
	 */
	if (rt->rt_ifp == m->m_pkthdr.rcvif && !srcrt && ip6_sendredirects &&
#ifdef KAME_IPSEC
	    !ipsecrt &&
#endif
	    (rt->rt_flags & (RTF_DYNAMIC|RTF_MODIFIED)) == 0) {
		if ((rt->rt_ifp->if_flags & IFF_POINTOPOINT) &&
		    nd6_is_addr_neighbor(
		        satocsin6(rtcache_getdst(&ip6_forward_rt)),
			rt->rt_ifp)) {
			/*
			 * If the incoming interface is equal to the outgoing
			 * one, the link attached to the interface is
			 * point-to-point, and the IPv6 destination is
			 * regarded as on-link on the link, then it will be
			 * highly probable that the destination address does
			 * not exist on the link and that the packet is going
			 * to loop.  Thus, we immediately drop the packet and
			 * send an ICMPv6 error message.
			 * For other routing loops, we dare to let the packet
			 * go to the loop, so that a remote diagnosing host
			 * can detect the loop by traceroute.
			 * type/code is based on suggestion by Rich Draves.
			 * not sure if it is the best pick.
			 */
			icmp6_error(mcopy, ICMP6_DST_UNREACH,
				    ICMP6_DST_UNREACH_ADDR, 0);
			m_freem(m);
			return;
		}
		type = ND_REDIRECT;
	}

	/*
	 * Fake scoped addresses. Note that even link-local source or
	 * destinaion can appear, if the originating node just sends the
	 * packet to us (without address resolution for the destination).
	 * Since both icmp6_error and icmp6_redirect_output fill the embedded
	 * link identifiers, we can do this stuff after making a copy for
	 * returning an error.
	 */
	if ((rt->rt_ifp->if_flags & IFF_LOOPBACK) != 0) {
		/*
		 * See corresponding comments in ip6_output.
		 * XXX: but is it possible that ip6_forward() sends a packet
		 *      to a loopback interface? I don't think so, and thus
		 *      I bark here. (jinmei@kame.net)
		 * XXX: it is common to route invalid packets to loopback.
		 *	also, the codepath will be visited on use of ::1 in
		 *	rthdr. (itojun)
		 */
#if 1
		if (0)
#else
		if ((rt->rt_flags & (RTF_BLACKHOLE|RTF_REJECT)) == 0)
#endif
		{
			printf("ip6_forward: outgoing interface is loopback. "
			       "src %s, dst %s, nxt %d, rcvif %s, outif %s\n",
			       ip6_sprintf(&ip6->ip6_src),
			       ip6_sprintf(&ip6->ip6_dst),
			       ip6->ip6_nxt, if_name(m->m_pkthdr.rcvif),
			       if_name(rt->rt_ifp));
		}

		/* we can just use rcvif in forwarding. */
		origifp = m->m_pkthdr.rcvif;
	}
	else
		origifp = rt->rt_ifp;
	/*
	 * clear embedded scope identifiers if necessary.
	 * in6_clearscope will touch the addresses only when necessary.
	 */
	in6_clearscope(&ip6->ip6_src);
	in6_clearscope(&ip6->ip6_dst);

#ifdef PFIL_HOOKS
	/*
	 * Run through list of hooks for output packets.
	 */
	if ((error = pfil_run_hooks(&inet6_pfil_hook, &m, rt->rt_ifp,
	    PFIL_OUT)) != 0)
		goto senderr;
	if (m == NULL)
		goto freecopy;
	ip6 = mtod(m, struct ip6_hdr *);
#endif /* PFIL_HOOKS */

	error = nd6_output(rt->rt_ifp, origifp, m, dst, rt);
	if (error) {
		in6_ifstat_inc(rt->rt_ifp, ifs6_out_discard);
		IP6_STATINC(IP6_STAT_CANTFORWARD);
	} else {
		IP6_STATINC(IP6_STAT_FORWARD);
		in6_ifstat_inc(rt->rt_ifp, ifs6_out_forward);
		if (type)
			IP6_STATINC(IP6_STAT_REDIRECTSENT);
		else {
#ifdef GATEWAY
			if (mcopy->m_flags & M_CANFASTFWD)
				ip6flow_create(&ip6_forward_rt, mcopy);
#endif
			if (mcopy)
				goto freecopy;
		}
	}

#ifdef PFIL_HOOKS
 senderr:
#endif
	if (mcopy == NULL)
		return;
	switch (error) {
	case 0:
		if (type == ND_REDIRECT) {
			icmp6_redirect_output(mcopy, rt);
			return;
		}
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
	return;
}
