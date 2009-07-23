/*	$NetBSD: ip_output.c,v 1.200.8.2 2009/07/23 23:32:48 jym Exp $	*/

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

/*-
 * Copyright (c) 1998 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Public Access Networks Corporation ("Panix").  It was developed under
 * contract to Panix by Eric Haszlakiewicz and Thor Lancelot Simon.
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
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * Copyright (c) 1982, 1986, 1988, 1990, 1993
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
 *	@(#)ip_output.c	8.3 (Berkeley) 1/21/94
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: ip_output.c,v 1.200.8.2 2009/07/23 23:32:48 jym Exp $");

#include "opt_pfil_hooks.h"
#include "opt_inet.h"
#include "opt_ipsec.h"
#include "opt_mrouting.h"

#include <sys/param.h>
#include <sys/malloc.h>
#include <sys/mbuf.h>
#include <sys/errno.h>
#include <sys/protosw.h>
#include <sys/socket.h>
#include <sys/socketvar.h>
#include <sys/kauth.h>
#ifdef FAST_IPSEC
#include <sys/domain.h>
#endif
#include <sys/systm.h>
#include <sys/proc.h>

#include <net/if.h>
#include <net/route.h>
#include <net/pfil.h>

#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include <netinet/in_pcb.h>
#include <netinet/in_var.h>
#include <netinet/ip_var.h>
#include <netinet/ip_private.h>
#include <netinet/in_offload.h>

#ifdef MROUTING
#include <netinet/ip_mroute.h>
#endif

#include <machine/stdarg.h>

#ifdef IPSEC
#include <netinet6/ipsec.h>
#include <netinet6/ipsec_private.h>
#include <netkey/key.h>
#include <netkey/key_debug.h>
#endif /*IPSEC*/

#ifdef FAST_IPSEC
#include <netipsec/ipsec.h>
#include <netipsec/key.h>
#include <netipsec/xform.h>
#endif	/* FAST_IPSEC*/

#ifdef IPSEC_NAT_T
#include <netinet/udp.h>
#endif

static struct mbuf *ip_insertoptions(struct mbuf *, struct mbuf *, int *);
static struct ifnet *ip_multicast_if(struct in_addr *, int *);
static void ip_mloopback(struct ifnet *, struct mbuf *,
    const struct sockaddr_in *);

#ifdef PFIL_HOOKS
extern struct pfil_head inet_pfil_hook;			/* XXX */
#endif

int	ip_do_loopback_cksum = 0;

/*
 * IP output.  The packet in mbuf chain m contains a skeletal IP
 * header (with len, off, ttl, proto, tos, src, dst).
 * The mbuf chain containing the packet will be freed.
 * The mbuf opt, if present, will not be freed.
 */
int
ip_output(struct mbuf *m0, ...)
{
	struct rtentry *rt;
	struct ip *ip;
	struct ifnet *ifp;
	struct mbuf *m = m0;
	int hlen = sizeof (struct ip);
	int len, error = 0;
	struct route iproute;
	const struct sockaddr_in *dst;
	struct in_ifaddr *ia;
	struct ifaddr *xifa;
	struct mbuf *opt;
	struct route *ro;
	int flags, sw_csum;
	int *mtu_p;
	u_long mtu;
	struct ip_moptions *imo;
	struct socket *so;
	va_list ap;
#ifdef IPSEC_NAT_T
	int natt_frag = 0;
#endif
#ifdef IPSEC
	struct secpolicy *sp = NULL;
#endif /*IPSEC*/
#ifdef FAST_IPSEC
	struct inpcb *inp;
	struct secpolicy *sp = NULL;
	int s;
#endif
	u_int16_t ip_len;
	union {
		struct sockaddr		dst;
		struct sockaddr_in	dst4;
	} u;
	struct sockaddr *rdst = &u.dst;	/* real IP destination, as opposed
					 * to the nexthop
					 */

	len = 0;
	va_start(ap, m0);
	opt = va_arg(ap, struct mbuf *);
	ro = va_arg(ap, struct route *);
	flags = va_arg(ap, int);
	imo = va_arg(ap, struct ip_moptions *);
	so = va_arg(ap, struct socket *);
	if (flags & IP_RETURNMTU)
		mtu_p = va_arg(ap, int *);
	else
		mtu_p = NULL;
	va_end(ap);

	MCLAIM(m, &ip_tx_mowner);
#ifdef FAST_IPSEC
	if (so != NULL && so->so_proto->pr_domain->dom_family == AF_INET)
		inp = (struct inpcb *)so->so_pcb;
	else
		inp = NULL;
#endif /* FAST_IPSEC */

#ifdef	DIAGNOSTIC
	if ((m->m_flags & M_PKTHDR) == 0)
		panic("ip_output: no HDR");

	if ((m->m_pkthdr.csum_flags & (M_CSUM_TCPv6|M_CSUM_UDPv6)) != 0) {
		panic("ip_output: IPv6 checksum offload flags: %d",
		    m->m_pkthdr.csum_flags);
	}

	if ((m->m_pkthdr.csum_flags & (M_CSUM_TCPv4|M_CSUM_UDPv4)) ==
	    (M_CSUM_TCPv4|M_CSUM_UDPv4)) {
		panic("ip_output: conflicting checksum offload flags: %d",
		    m->m_pkthdr.csum_flags);
	}
#endif
	if (opt) {
		m = ip_insertoptions(m, opt, &len);
		if (len >= sizeof(struct ip))
			hlen = len;
	}
	ip = mtod(m, struct ip *);
	/*
	 * Fill in IP header.
	 */
	if ((flags & (IP_FORWARDING|IP_RAWOUTPUT)) == 0) {
		ip->ip_v = IPVERSION;
		ip->ip_off = htons(0);
		/* ip->ip_id filled in after we find out source ia */
		ip->ip_hl = hlen >> 2;
		IP_STATINC(IP_STAT_LOCALOUT);
	} else {
		hlen = ip->ip_hl << 2;
	}
	/*
	 * Route packet.
	 */
	memset(&iproute, 0, sizeof(iproute));
	if (ro == NULL)
		ro = &iproute;
	sockaddr_in_init(&u.dst4, &ip->ip_dst, 0);
	dst = satocsin(rtcache_getdst(ro));
	/*
	 * If there is a cached route,
	 * check that it is to the same destination
	 * and is still up.  If not, free it and try again.
	 * The address family should also be checked in case of sharing the
	 * cache with IPv6.
	 */
	if (dst == NULL)
		;
	else if (dst->sin_family != AF_INET ||
		 !in_hosteq(dst->sin_addr, ip->ip_dst))
		rtcache_free(ro);

	if ((rt = rtcache_validate(ro)) == NULL &&
	    (rt = rtcache_update(ro, 1)) == NULL) {
		dst = &u.dst4;
		rtcache_setdst(ro, &u.dst);
	}
	/*
	 * If routing to interface only,
	 * short circuit routing lookup.
	 */
	if (flags & IP_ROUTETOIF) {
		if ((ia = ifatoia(ifa_ifwithladdr(sintocsa(dst)))) == NULL) {
			IP_STATINC(IP_STAT_NOROUTE);
			error = ENETUNREACH;
			goto bad;
		}
		ifp = ia->ia_ifp;
		mtu = ifp->if_mtu;
		ip->ip_ttl = 1;
	} else if ((IN_MULTICAST(ip->ip_dst.s_addr) ||
	    ip->ip_dst.s_addr == INADDR_BROADCAST) &&
	    imo != NULL && imo->imo_multicast_ifp != NULL) {
		ifp = imo->imo_multicast_ifp;
		mtu = ifp->if_mtu;
		IFP_TO_IA(ifp, ia);
	} else {
		if (rt == NULL)
			rt = rtcache_init(ro);
		if (rt == NULL) {
			IP_STATINC(IP_STAT_NOROUTE);
			error = EHOSTUNREACH;
			goto bad;
		}
		ia = ifatoia(rt->rt_ifa);
		ifp = rt->rt_ifp;
		if ((mtu = rt->rt_rmx.rmx_mtu) == 0)
			mtu = ifp->if_mtu;
		rt->rt_use++;
		if (rt->rt_flags & RTF_GATEWAY)
			dst = satosin(rt->rt_gateway);
	}
	if (IN_MULTICAST(ip->ip_dst.s_addr) ||
	    (ip->ip_dst.s_addr == INADDR_BROADCAST)) {
		struct in_multi *inm;

		m->m_flags |= (ip->ip_dst.s_addr == INADDR_BROADCAST) ?
			M_BCAST : M_MCAST;
		/*
		 * See if the caller provided any multicast options
		 */
		if (imo != NULL)
			ip->ip_ttl = imo->imo_multicast_ttl;
		else
			ip->ip_ttl = IP_DEFAULT_MULTICAST_TTL;

		/*
		 * if we don't know the outgoing ifp yet, we can't generate
		 * output
		 */
		if (!ifp) {
			IP_STATINC(IP_STAT_NOROUTE);
			error = ENETUNREACH;
			goto bad;
		}

		/*
		 * If the packet is multicast or broadcast, confirm that
		 * the outgoing interface can transmit it.
		 */
		if (((m->m_flags & M_MCAST) &&
		     (ifp->if_flags & IFF_MULTICAST) == 0) ||
		    ((m->m_flags & M_BCAST) &&
		     (ifp->if_flags & (IFF_BROADCAST|IFF_POINTOPOINT)) == 0))  {
			IP_STATINC(IP_STAT_NOROUTE);
			error = ENETUNREACH;
			goto bad;
		}
		/*
		 * If source address not specified yet, use an address
		 * of outgoing interface.
		 */
		if (in_nullhost(ip->ip_src)) {
			struct in_ifaddr *xia;

			IFP_TO_IA(ifp, xia);
			if (!xia) {
				error = EADDRNOTAVAIL;
				goto bad;
			}
			xifa = &xia->ia_ifa;
			if (xifa->ifa_getifa != NULL) {
				xia = ifatoia((*xifa->ifa_getifa)(xifa, rdst));
			}
			ip->ip_src = xia->ia_addr.sin_addr;
		}

		IN_LOOKUP_MULTI(ip->ip_dst, ifp, inm);
		if (inm != NULL &&
		   (imo == NULL || imo->imo_multicast_loop)) {
			/*
			 * If we belong to the destination multicast group
			 * on the outgoing interface, and the caller did not
			 * forbid loopback, loop back a copy.
			 */
			ip_mloopback(ifp, m, &u.dst4);
		}
#ifdef MROUTING
		else {
			/*
			 * If we are acting as a multicast router, perform
			 * multicast forwarding as if the packet had just
			 * arrived on the interface to which we are about
			 * to send.  The multicast forwarding function
			 * recursively calls this function, using the
			 * IP_FORWARDING flag to prevent infinite recursion.
			 *
			 * Multicasts that are looped back by ip_mloopback(),
			 * above, will be forwarded by the ip_input() routine,
			 * if necessary.
			 */
			extern struct socket *ip_mrouter;

			if (ip_mrouter && (flags & IP_FORWARDING) == 0) {
				if (ip_mforward(m, ifp) != 0) {
					m_freem(m);
					goto done;
				}
			}
		}
#endif
		/*
		 * Multicasts with a time-to-live of zero may be looped-
		 * back, above, but must not be transmitted on a network.
		 * Also, multicasts addressed to the loopback interface
		 * are not sent -- the above call to ip_mloopback() will
		 * loop back a copy if this host actually belongs to the
		 * destination group on the loopback interface.
		 */
		if (ip->ip_ttl == 0 || (ifp->if_flags & IFF_LOOPBACK) != 0) {
			m_freem(m);
			goto done;
		}

		goto sendit;
	}
	/*
	 * If source address not specified yet, use address
	 * of outgoing interface.
	 */
	if (in_nullhost(ip->ip_src)) {
		xifa = &ia->ia_ifa;
		if (xifa->ifa_getifa != NULL)
			ia = ifatoia((*xifa->ifa_getifa)(xifa, rdst));
		ip->ip_src = ia->ia_addr.sin_addr;
	}

	/*
	 * packets with Class-D address as source are not valid per
	 * RFC 1112
	 */
	if (IN_MULTICAST(ip->ip_src.s_addr)) {
		IP_STATINC(IP_STAT_ODROPPED);
		error = EADDRNOTAVAIL;
		goto bad;
	}

	/*
	 * Look for broadcast address and
	 * and verify user is allowed to send
	 * such a packet.
	 */
	if (in_broadcast(dst->sin_addr, ifp)) {
		if ((ifp->if_flags & IFF_BROADCAST) == 0) {
			error = EADDRNOTAVAIL;
			goto bad;
		}
		if ((flags & IP_ALLOWBROADCAST) == 0) {
			error = EACCES;
			goto bad;
		}
		/* don't allow broadcast messages to be fragmented */
		if (ntohs(ip->ip_len) > ifp->if_mtu) {
			error = EMSGSIZE;
			goto bad;
		}
		m->m_flags |= M_BCAST;
	} else
		m->m_flags &= ~M_BCAST;

sendit:
	if ((flags & (IP_FORWARDING|IP_NOIPNEWID)) == 0) {
		if (m->m_pkthdr.len < IP_MINFRAGSIZE) {
			ip->ip_id = 0;
		} else if ((m->m_pkthdr.csum_flags & M_CSUM_TSOv4) == 0) {
			ip->ip_id = ip_newid(ia);
		} else {

			/*
			 * TSO capable interfaces (typically?) increment
			 * ip_id for each segment.
			 * "allocate" enough ids here to increase the chance
			 * for them to be unique.
			 *
			 * note that the following calculation is not
			 * needed to be precise.  wasting some ip_id is fine.
			 */

			unsigned int segsz = m->m_pkthdr.segsz;
			unsigned int datasz = ntohs(ip->ip_len) - hlen;
			unsigned int num = howmany(datasz, segsz);

			ip->ip_id = ip_newid_range(ia, num);
		}
	}
	/*
	 * If we're doing Path MTU Discovery, we need to set DF unless
	 * the route's MTU is locked.
	 */
	if ((flags & IP_MTUDISC) != 0 && rt != NULL &&
	    (rt->rt_rmx.rmx_locks & RTV_MTU) == 0)
		ip->ip_off |= htons(IP_DF);

	/* Remember the current ip_len */
	ip_len = ntohs(ip->ip_len);

#ifdef IPSEC
	/* get SP for this packet */
	if (so == NULL)
		sp = ipsec4_getpolicybyaddr(m, IPSEC_DIR_OUTBOUND,
		    flags, &error);
	else {
		if (IPSEC_PCB_SKIP_IPSEC(sotoinpcb_hdr(so)->inph_sp,
					 IPSEC_DIR_OUTBOUND))
			goto skip_ipsec;
		sp = ipsec4_getpolicybysock(m, IPSEC_DIR_OUTBOUND, so, &error);
	}

	if (sp == NULL) {
		IPSEC_STATINC(IPSEC_STAT_IN_INVAL);
		goto bad;
	}

	error = 0;

	/* check policy */
	switch (sp->policy) {
	case IPSEC_POLICY_DISCARD:
		/*
		 * This packet is just discarded.
		 */
		IPSEC_STATINC(IPSEC_STAT_OUT_POLVIO);
		goto bad;

	case IPSEC_POLICY_BYPASS:
	case IPSEC_POLICY_NONE:
		/* no need to do IPsec. */
		goto skip_ipsec;

	case IPSEC_POLICY_IPSEC:
		if (sp->req == NULL) {
			/* XXX should be panic ? */
			printf("ip_output: No IPsec request specified.\n");
			error = EINVAL;
			goto bad;
		}
		break;

	case IPSEC_POLICY_ENTRUST:
	default:
		printf("ip_output: Invalid policy found. %d\n", sp->policy);
	}

#ifdef IPSEC_NAT_T
	/*
	 * NAT-T ESP fragmentation: don't do IPSec processing now,
	 * we'll do it on each fragmented packet.
	 */
	if (sp->req->sav &&
	    ((sp->req->sav->natt_type & UDP_ENCAP_ESPINUDP) ||
	     (sp->req->sav->natt_type & UDP_ENCAP_ESPINUDP_NON_IKE))) {
		if (ntohs(ip->ip_len) > sp->req->sav->esp_frag) {
			natt_frag = 1;
			mtu = sp->req->sav->esp_frag;
			goto skip_ipsec;
		}
	}
#endif /* IPSEC_NAT_T */

	/*
	 * ipsec4_output() expects ip_len and ip_off in network
	 * order.  They have been set to network order above.
	 */

    {
	struct ipsec_output_state state;
	memset(&state, 0, sizeof(state));
	state.m = m;
	if (flags & IP_ROUTETOIF) {
		state.ro = &iproute;
		memset(&iproute, 0, sizeof(iproute));
	} else
		state.ro = ro;
	state.dst = sintocsa(dst);

	/*
	 * We can't defer the checksum of payload data if
	 * we're about to encrypt/authenticate it.
	 *
	 * XXX When we support crypto offloading functions of
	 * XXX network interfaces, we need to reconsider this,
	 * XXX since it's likely that they'll support checksumming,
	 * XXX as well.
	 */
	if (m->m_pkthdr.csum_flags & (M_CSUM_TCPv4|M_CSUM_UDPv4)) {
		in_delayed_cksum(m);
		m->m_pkthdr.csum_flags &= ~(M_CSUM_TCPv4|M_CSUM_UDPv4);
	}

	error = ipsec4_output(&state, sp, flags);

	m = state.m;
	if (flags & IP_ROUTETOIF) {
		/*
		 * if we have tunnel mode SA, we may need to ignore
		 * IP_ROUTETOIF.
		 */
		if (state.ro != &iproute ||
		    rtcache_validate(state.ro) != NULL) {
			flags &= ~IP_ROUTETOIF;
			ro = state.ro;
		}
	} else
		ro = state.ro;
	dst = satocsin(state.dst);
	if (error) {
		/* mbuf is already reclaimed in ipsec4_output. */
		m0 = NULL;
		switch (error) {
		case EHOSTUNREACH:
		case ENETUNREACH:
		case EMSGSIZE:
		case ENOBUFS:
		case ENOMEM:
			break;
		default:
			printf("ip4_output (ipsec): error code %d\n", error);
			/*fall through*/
		case ENOENT:
			/* don't show these error codes to the user */
			error = 0;
			break;
		}
		goto bad;
	}

	/* be sure to update variables that are affected by ipsec4_output() */
	ip = mtod(m, struct ip *);
	hlen = ip->ip_hl << 2;
	ip_len = ntohs(ip->ip_len);

	if ((rt = rtcache_validate(ro)) == NULL) {
		if ((flags & IP_ROUTETOIF) == 0) {
			printf("ip_output: "
				"can't update route after IPsec processing\n");
			error = EHOSTUNREACH;	/*XXX*/
			goto bad;
		}
	} else {
		/* nobody uses ia beyond here */
		if (state.encap) {
			ifp = rt->rt_ifp;
			if ((mtu = rt->rt_rmx.rmx_mtu) == 0)
				mtu = ifp->if_mtu;
		}
	}
    }
skip_ipsec:
#endif /*IPSEC*/
#ifdef FAST_IPSEC
	/*
	 * Check the security policy (SP) for the packet and, if
	 * required, do IPsec-related processing.  There are two
	 * cases here; the first time a packet is sent through
	 * it will be untagged and handled by ipsec4_checkpolicy.
	 * If the packet is resubmitted to ip_output (e.g. after
	 * AH, ESP, etc. processing), there will be a tag to bypass
	 * the lookup and related policy checking.
	 */
	if (!ipsec_outdone(m)) {
		s = splsoftnet();
		if (inp != NULL &&
		    IPSEC_PCB_SKIP_IPSEC(inp->inp_sp, IPSEC_DIR_OUTBOUND)) {
			splx(s);
			goto spd_done;
		}
		sp = ipsec4_checkpolicy(m, IPSEC_DIR_OUTBOUND, flags,
				&error, inp);
		/*
		 * There are four return cases:
		 *    sp != NULL	 	    apply IPsec policy
		 *    sp == NULL, error == 0	    no IPsec handling needed
		 *    sp == NULL, error == -EINVAL  discard packet w/o error
		 *    sp == NULL, error != 0	    discard packet, report error
		 */
		if (sp != NULL) {
#ifdef IPSEC_NAT_T
			/*
			 * NAT-T ESP fragmentation: don't do IPSec processing now,
			 * we'll do it on each fragmented packet.
			 */
			if (sp->req->sav &&
					((sp->req->sav->natt_type & UDP_ENCAP_ESPINUDP) ||
					 (sp->req->sav->natt_type & UDP_ENCAP_ESPINUDP_NON_IKE))) {
				if (ntohs(ip->ip_len) > sp->req->sav->esp_frag) {
					natt_frag = 1;
					mtu = sp->req->sav->esp_frag;
					splx(s);
					goto spd_done;
				}
			}
#endif /* IPSEC_NAT_T */

			/*
			 * Do delayed checksums now because we send before
			 * this is done in the normal processing path.
			 */
			if (m->m_pkthdr.csum_flags & (M_CSUM_TCPv4|M_CSUM_UDPv4)) {
				in_delayed_cksum(m);
				m->m_pkthdr.csum_flags &= ~(M_CSUM_TCPv4|M_CSUM_UDPv4);
			}

#ifdef __FreeBSD__
			ip->ip_len = htons(ip->ip_len);
			ip->ip_off = htons(ip->ip_off);
#endif

			/* NB: callee frees mbuf */
			error = ipsec4_process_packet(m, sp->req, flags, 0);
			/*
			 * Preserve KAME behaviour: ENOENT can be returned
			 * when an SA acquire is in progress.  Don't propagate
			 * this to user-level; it confuses applications.
			 *
			 * XXX this will go away when the SADB is redone.
			 */
			if (error == ENOENT)
				error = 0;
			splx(s);
			goto done;
		} else {
			splx(s);

			if (error != 0) {
				/*
				 * Hack: -EINVAL is used to signal that a packet
				 * should be silently discarded.  This is typically
				 * because we asked key management for an SA and
				 * it was delayed (e.g. kicked up to IKE).
				 */
				if (error == -EINVAL)
					error = 0;
				goto bad;
			} else {
				/* No IPsec processing for this packet. */
			}
		}
	}
spd_done:
#endif /* FAST_IPSEC */

#ifdef PFIL_HOOKS
	/*
	 * Run through list of hooks for output packets.
	 */
	if ((error = pfil_run_hooks(&inet_pfil_hook, &m, ifp, PFIL_OUT)) != 0)
		goto done;
	if (m == NULL)
		goto done;

	ip = mtod(m, struct ip *);
	hlen = ip->ip_hl << 2;
	ip_len = ntohs(ip->ip_len);
#endif /* PFIL_HOOKS */

	m->m_pkthdr.csum_data |= hlen << 16;

#if IFA_STATS
	/*
	 * search for the source address structure to
	 * maintain output statistics.
	 */
	INADDR_TO_IA(ip->ip_src, ia);
#endif

	/* Maybe skip checksums on loopback interfaces. */
	if (IN_NEED_CHECKSUM(ifp, M_CSUM_IPv4)) {
		m->m_pkthdr.csum_flags |= M_CSUM_IPv4;
	}
	sw_csum = m->m_pkthdr.csum_flags & ~ifp->if_csum_flags_tx;
	/*
	 * If small enough for mtu of path, or if using TCP segmentation
	 * offload, can just send directly.
	 */
	if (ip_len <= mtu ||
	    (m->m_pkthdr.csum_flags & M_CSUM_TSOv4) != 0) {
#if IFA_STATS
		if (ia)
			ia->ia_ifa.ifa_data.ifad_outbytes += ip_len;
#endif
		/*
		 * Always initialize the sum to 0!  Some HW assisted
		 * checksumming requires this.
		 */
		ip->ip_sum = 0;

		if ((m->m_pkthdr.csum_flags & M_CSUM_TSOv4) == 0) {
			/*
			 * Perform any checksums that the hardware can't do
			 * for us.
			 *
			 * XXX Does any hardware require the {th,uh}_sum
			 * XXX fields to be 0?
			 */
			if (sw_csum & M_CSUM_IPv4) {
				KASSERT(IN_NEED_CHECKSUM(ifp, M_CSUM_IPv4));
				ip->ip_sum = in_cksum(m, hlen);
				m->m_pkthdr.csum_flags &= ~M_CSUM_IPv4;
			}
			if (sw_csum & (M_CSUM_TCPv4|M_CSUM_UDPv4)) {
				if (IN_NEED_CHECKSUM(ifp,
				    sw_csum & (M_CSUM_TCPv4|M_CSUM_UDPv4))) {
					in_delayed_cksum(m);
				}
				m->m_pkthdr.csum_flags &=
				    ~(M_CSUM_TCPv4|M_CSUM_UDPv4);
			}
		}

#ifdef IPSEC
		/* clean ipsec history once it goes out of the node */
		ipsec_delaux(m);
#endif

		if (__predict_true(
		    (m->m_pkthdr.csum_flags & M_CSUM_TSOv4) == 0 ||
		    (ifp->if_capenable & IFCAP_TSOv4) != 0)) {
			error =
			    (*ifp->if_output)(ifp, m,
				(m->m_flags & M_MCAST) ?
				    sintocsa(rdst) : sintocsa(dst),
				rt);
		} else {
			error =
			    ip_tso_output(ifp, m,
				(m->m_flags & M_MCAST) ?
				    sintocsa(rdst) : sintocsa(dst),
				rt);
		}
		goto done;
	}

	/*
	 * We can't use HW checksumming if we're about to
	 * to fragment the packet.
	 *
	 * XXX Some hardware can do this.
	 */
	if (m->m_pkthdr.csum_flags & (M_CSUM_TCPv4|M_CSUM_UDPv4)) {
		if (IN_NEED_CHECKSUM(ifp,
		    m->m_pkthdr.csum_flags & (M_CSUM_TCPv4|M_CSUM_UDPv4))) {
			in_delayed_cksum(m);
		}
		m->m_pkthdr.csum_flags &= ~(M_CSUM_TCPv4|M_CSUM_UDPv4);
	}

	/*
	 * Too large for interface; fragment if possible.
	 * Must be able to put at least 8 bytes per fragment.
	 */
	if (ntohs(ip->ip_off) & IP_DF) {
		if (flags & IP_RETURNMTU)
			*mtu_p = mtu;
		error = EMSGSIZE;
		IP_STATINC(IP_STAT_CANTFRAG);
		goto bad;
	}

	error = ip_fragment(m, ifp, mtu);
	if (error) {
		m = NULL;
		goto bad;
	}

	for (; m; m = m0) {
		m0 = m->m_nextpkt;
		m->m_nextpkt = 0;
		if (error == 0) {
#if IFA_STATS
			if (ia)
				ia->ia_ifa.ifa_data.ifad_outbytes +=
				    ntohs(ip->ip_len);
#endif
#ifdef IPSEC
			/* clean ipsec history once it goes out of the node */
			ipsec_delaux(m);
#endif /* IPSEC */

#ifdef IPSEC_NAT_T
			/*
			 * If we get there, the packet has not been handeld by
			 * IPSec whereas it should have. Now that it has been
			 * fragmented, re-inject it in ip_output so that IPsec
			 * processing can occur.
			 */
			if (natt_frag) {
				error = ip_output(m, opt,
				    ro, flags, imo, so, mtu_p);
			} else
#endif /* IPSEC_NAT_T */
			{
				KASSERT((m->m_pkthdr.csum_flags &
				    (M_CSUM_UDPv4 | M_CSUM_TCPv4)) == 0);
				error = (*ifp->if_output)(ifp, m,
				    (m->m_flags & M_MCAST) ?
					sintocsa(rdst) : sintocsa(dst),
				    rt);
			}
		} else
			m_freem(m);
	}

	if (error == 0)
		IP_STATINC(IP_STAT_FRAGMENTED);
done:
	rtcache_free(&iproute);

#ifdef IPSEC
	if (sp != NULL) {
		KEYDEBUG(KEYDEBUG_IPSEC_STAMP,
			printf("DP ip_output call free SP:%p\n", sp));
		key_freesp(sp);
	}
#endif /* IPSEC */
#ifdef FAST_IPSEC
	if (sp != NULL)
		KEY_FREESP(&sp);
#endif /* FAST_IPSEC */

	return (error);
bad:
	m_freem(m);
	goto done;
}

int
ip_fragment(struct mbuf *m, struct ifnet *ifp, u_long mtu)
{
	struct ip *ip, *mhip;
	struct mbuf *m0;
	int len, hlen, off;
	int mhlen, firstlen;
	struct mbuf **mnext;
	int sw_csum = m->m_pkthdr.csum_flags;
	int fragments = 0;
	int s;
	int error = 0;

	ip = mtod(m, struct ip *);
	hlen = ip->ip_hl << 2;
	if (ifp != NULL)
		sw_csum &= ~ifp->if_csum_flags_tx;

	len = (mtu - hlen) &~ 7;
	if (len < 8) {
		m_freem(m);
		return (EMSGSIZE);
	}

	firstlen = len;
	mnext = &m->m_nextpkt;

	/*
	 * Loop through length of segment after first fragment,
	 * make new header and copy data of each part and link onto chain.
	 */
	m0 = m;
	mhlen = sizeof (struct ip);
	for (off = hlen + len; off < ntohs(ip->ip_len); off += len) {
		MGETHDR(m, M_DONTWAIT, MT_HEADER);
		if (m == 0) {
			error = ENOBUFS;
			IP_STATINC(IP_STAT_ODROPPED);
			goto sendorfree;
		}
		MCLAIM(m, m0->m_owner);
		*mnext = m;
		mnext = &m->m_nextpkt;
		m->m_data += max_linkhdr;
		mhip = mtod(m, struct ip *);
		*mhip = *ip;
		/* we must inherit MCAST and BCAST flags */
		m->m_flags |= m0->m_flags & (M_MCAST|M_BCAST);
		if (hlen > sizeof (struct ip)) {
			mhlen = ip_optcopy(ip, mhip) + sizeof (struct ip);
			mhip->ip_hl = mhlen >> 2;
		}
		m->m_len = mhlen;
		mhip->ip_off = ((off - hlen) >> 3) +
		    (ntohs(ip->ip_off) & ~IP_MF);
		if (ip->ip_off & htons(IP_MF))
			mhip->ip_off |= IP_MF;
		if (off + len >= ntohs(ip->ip_len))
			len = ntohs(ip->ip_len) - off;
		else
			mhip->ip_off |= IP_MF;
		HTONS(mhip->ip_off);
		mhip->ip_len = htons((u_int16_t)(len + mhlen));
		m->m_next = m_copym(m0, off, len, M_DONTWAIT);
		if (m->m_next == 0) {
			error = ENOBUFS;	/* ??? */
			IP_STATINC(IP_STAT_ODROPPED);
			goto sendorfree;
		}
		m->m_pkthdr.len = mhlen + len;
		m->m_pkthdr.rcvif = (struct ifnet *)0;
		mhip->ip_sum = 0;
		if (sw_csum & M_CSUM_IPv4) {
			mhip->ip_sum = in_cksum(m, mhlen);
			KASSERT((m->m_pkthdr.csum_flags & M_CSUM_IPv4) == 0);
		} else {
			m->m_pkthdr.csum_flags |= M_CSUM_IPv4;
			m->m_pkthdr.csum_data |= mhlen << 16;
		}
		IP_STATINC(IP_STAT_OFRAGMENTS);
		fragments++;
	}
	/*
	 * Update first fragment by trimming what's been copied out
	 * and updating header, then send each fragment (in order).
	 */
	m = m0;
	m_adj(m, hlen + firstlen - ntohs(ip->ip_len));
	m->m_pkthdr.len = hlen + firstlen;
	ip->ip_len = htons((u_int16_t)m->m_pkthdr.len);
	ip->ip_off |= htons(IP_MF);
	ip->ip_sum = 0;
	if (sw_csum & M_CSUM_IPv4) {
		ip->ip_sum = in_cksum(m, hlen);
		m->m_pkthdr.csum_flags &= ~M_CSUM_IPv4;
	} else {
		KASSERT(m->m_pkthdr.csum_flags & M_CSUM_IPv4);
		KASSERT(M_CSUM_DATA_IPv4_IPHL(m->m_pkthdr.csum_data) >=
			sizeof(struct ip));
	}
sendorfree:
	/*
	 * If there is no room for all the fragments, don't queue
	 * any of them.
	 */
	if (ifp != NULL) {
		s = splnet();
		if (ifp->if_snd.ifq_maxlen - ifp->if_snd.ifq_len < fragments &&
		    error == 0) {
			error = ENOBUFS;
			IP_STATINC(IP_STAT_ODROPPED);
			IFQ_INC_DROPS(&ifp->if_snd);
		}
		splx(s);
	}
	if (error) {
		for (m = m0; m; m = m0) {
			m0 = m->m_nextpkt;
			m->m_nextpkt = NULL;
			m_freem(m);
		}
	}
	return (error);
}

/*
 * Process a delayed payload checksum calculation.
 */
void
in_delayed_cksum(struct mbuf *m)
{
	struct ip *ip;
	u_int16_t csum, offset;

	ip = mtod(m, struct ip *);
	offset = ip->ip_hl << 2;
	csum = in4_cksum(m, 0, offset, ntohs(ip->ip_len) - offset);
	if (csum == 0 && (m->m_pkthdr.csum_flags & M_CSUM_UDPv4) != 0)
		csum = 0xffff;

	offset += M_CSUM_DATA_IPv4_OFFSET(m->m_pkthdr.csum_data);

	if ((offset + sizeof(u_int16_t)) > m->m_len) {
		/* This happen when ip options were inserted
		printf("in_delayed_cksum: pullup len %d off %d proto %d\n",
		    m->m_len, offset, ip->ip_p);
		 */
		m_copyback(m, offset, sizeof(csum), (void *) &csum);
	} else
		*(u_int16_t *)(mtod(m, char *) + offset) = csum;
}

/*
 * Determine the maximum length of the options to be inserted;
 * we would far rather allocate too much space rather than too little.
 */

u_int
ip_optlen(struct inpcb *inp)
{
	struct mbuf *m = inp->inp_options;

	if (m && m->m_len > offsetof(struct ipoption, ipopt_dst))
		return (m->m_len - offsetof(struct ipoption, ipopt_dst));
	else
		return 0;
}


/*
 * Insert IP options into preformed packet.
 * Adjust IP destination as required for IP source routing,
 * as indicated by a non-zero in_addr at the start of the options.
 */
static struct mbuf *
ip_insertoptions(struct mbuf *m, struct mbuf *opt, int *phlen)
{
	struct ipoption *p = mtod(opt, struct ipoption *);
	struct mbuf *n;
	struct ip *ip = mtod(m, struct ip *);
	unsigned optlen;

	optlen = opt->m_len - sizeof(p->ipopt_dst);
	if (optlen + ntohs(ip->ip_len) > IP_MAXPACKET)
		return (m);		/* XXX should fail */
	if (!in_nullhost(p->ipopt_dst))
		ip->ip_dst = p->ipopt_dst;
	if (M_READONLY(m) || M_LEADINGSPACE(m) < optlen) {
		MGETHDR(n, M_DONTWAIT, MT_HEADER);
		if (n == 0)
			return (m);
		MCLAIM(n, m->m_owner);
		M_MOVE_PKTHDR(n, m);
		m->m_len -= sizeof(struct ip);
		m->m_data += sizeof(struct ip);
		n->m_next = m;
		m = n;
		m->m_len = optlen + sizeof(struct ip);
		m->m_data += max_linkhdr;
		bcopy((void *)ip, mtod(m, void *), sizeof(struct ip));
	} else {
		m->m_data -= optlen;
		m->m_len += optlen;
		memmove(mtod(m, void *), ip, sizeof(struct ip));
	}
	m->m_pkthdr.len += optlen;
	ip = mtod(m, struct ip *);
	bcopy((void *)p->ipopt_list, (void *)(ip + 1), (unsigned)optlen);
	*phlen = sizeof(struct ip) + optlen;
	ip->ip_len = htons(ntohs(ip->ip_len) + optlen);
	return (m);
}

/*
 * Copy options from ip to jp,
 * omitting those not copied during fragmentation.
 */
int
ip_optcopy(struct ip *ip, struct ip *jp)
{
	u_char *cp, *dp;
	int opt, optlen, cnt;

	cp = (u_char *)(ip + 1);
	dp = (u_char *)(jp + 1);
	cnt = (ip->ip_hl << 2) - sizeof (struct ip);
	for (; cnt > 0; cnt -= optlen, cp += optlen) {
		opt = cp[0];
		if (opt == IPOPT_EOL)
			break;
		if (opt == IPOPT_NOP) {
			/* Preserve for IP mcast tunnel's LSRR alignment. */
			*dp++ = IPOPT_NOP;
			optlen = 1;
			continue;
		}
#ifdef DIAGNOSTIC
		if (cnt < IPOPT_OLEN + sizeof(*cp))
			panic("malformed IPv4 option passed to ip_optcopy");
#endif
		optlen = cp[IPOPT_OLEN];
#ifdef DIAGNOSTIC
		if (optlen < IPOPT_OLEN + sizeof(*cp) || optlen > cnt)
			panic("malformed IPv4 option passed to ip_optcopy");
#endif
		/* bogus lengths should have been caught by ip_dooptions */
		if (optlen > cnt)
			optlen = cnt;
		if (IPOPT_COPIED(opt)) {
			bcopy((void *)cp, (void *)dp, (unsigned)optlen);
			dp += optlen;
		}
	}
	for (optlen = dp - (u_char *)(jp+1); optlen & 0x3; optlen++)
		*dp++ = IPOPT_EOL;
	return (optlen);
}

/*
 * IP socket option processing.
 */
int
ip_ctloutput(int op, struct socket *so, struct sockopt *sopt)
{
	struct inpcb *inp = sotoinpcb(so);
	int optval = 0;
	int error = 0;
#if defined(IPSEC) || defined(FAST_IPSEC)
	struct lwp *l = curlwp;	/*XXX*/
#endif

	if (sopt->sopt_level != IPPROTO_IP) {
		if (sopt->sopt_level == SOL_SOCKET && sopt->sopt_name == SO_NOHEADER)
			return 0;
		return ENOPROTOOPT;
	}

	switch (op) {
	case PRCO_SETOPT:
		switch (sopt->sopt_name) {
		case IP_OPTIONS:
#ifdef notyet
		case IP_RETOPTS:
#endif
			error = ip_pcbopts(&inp->inp_options, sopt);
			break;

		case IP_TOS:
		case IP_TTL:
		case IP_MINTTL:
		case IP_RECVOPTS:
		case IP_RECVRETOPTS:
		case IP_RECVDSTADDR:
		case IP_RECVIF:
		case IP_RECVTTL:
			error = sockopt_getint(sopt, &optval);
			if (error)
				break;

			switch (sopt->sopt_name) {
			case IP_TOS:
				inp->inp_ip.ip_tos = optval;
				break;

			case IP_TTL:
				inp->inp_ip.ip_ttl = optval;
				break;

			case IP_MINTTL:
				if (optval > 0 && optval <= MAXTTL)
					inp->inp_ip_minttl = optval;
				else
					error = EINVAL;
				break;
#define	OPTSET(bit) \
	if (optval) \
		inp->inp_flags |= bit; \
	else \
		inp->inp_flags &= ~bit;

			case IP_RECVOPTS:
				OPTSET(INP_RECVOPTS);
				break;

			case IP_RECVRETOPTS:
				OPTSET(INP_RECVRETOPTS);
				break;

			case IP_RECVDSTADDR:
				OPTSET(INP_RECVDSTADDR);
				break;

			case IP_RECVIF:
				OPTSET(INP_RECVIF);
				break;

			case IP_RECVTTL:
				OPTSET(INP_RECVTTL);
				break;
			}
		break;
#undef OPTSET

		case IP_MULTICAST_IF:
		case IP_MULTICAST_TTL:
		case IP_MULTICAST_LOOP:
		case IP_ADD_MEMBERSHIP:
		case IP_DROP_MEMBERSHIP:
			error = ip_setmoptions(&inp->inp_moptions, sopt);
			break;

		case IP_PORTRANGE:
			error = sockopt_getint(sopt, &optval);
			if (error)
				break;

			/* INP_LOCK(inp); */
			switch (optval) {
			case IP_PORTRANGE_DEFAULT:
			case IP_PORTRANGE_HIGH:
				inp->inp_flags &= ~(INP_LOWPORT);
				break;

			case IP_PORTRANGE_LOW:
				inp->inp_flags |= INP_LOWPORT;
				break;

			default:
				error = EINVAL;
				break;
			}
			/* INP_UNLOCK(inp); */
			break;

#if defined(IPSEC) || defined(FAST_IPSEC)
		case IP_IPSEC_POLICY:
		    {
			error = ipsec4_set_policy(inp, sopt->sopt_name,
			    sopt->sopt_data, sopt->sopt_size, l->l_cred);
			break;
		    }
#endif /*IPSEC*/

		default:
			error = ENOPROTOOPT;
			break;
		}
		break;

	case PRCO_GETOPT:
		switch (sopt->sopt_name) {
		case IP_OPTIONS:
		case IP_RETOPTS:
			if (inp->inp_options) {
				struct mbuf *m;

				m = m_copym(inp->inp_options, 0, M_COPYALL,
				    M_DONTWAIT);
				if (m == NULL) {
					error = ENOBUFS;
					break;
				}

				error = sockopt_setmbuf(sopt, m);
			}
			break;

		case IP_TOS:
		case IP_TTL:
		case IP_MINTTL:
		case IP_RECVOPTS:
		case IP_RECVRETOPTS:
		case IP_RECVDSTADDR:
		case IP_RECVIF:
		case IP_RECVTTL:
		case IP_ERRORMTU:
			switch (sopt->sopt_name) {
			case IP_TOS:
				optval = inp->inp_ip.ip_tos;
				break;

			case IP_TTL:
				optval = inp->inp_ip.ip_ttl;
				break;

			case IP_MINTTL:
				optval = inp->inp_ip_minttl;
				break;

			case IP_ERRORMTU:
				optval = inp->inp_errormtu;
				break;

#define	OPTBIT(bit)	(inp->inp_flags & bit ? 1 : 0)

			case IP_RECVOPTS:
				optval = OPTBIT(INP_RECVOPTS);
				break;

			case IP_RECVRETOPTS:
				optval = OPTBIT(INP_RECVRETOPTS);
				break;

			case IP_RECVDSTADDR:
				optval = OPTBIT(INP_RECVDSTADDR);
				break;

			case IP_RECVIF:
				optval = OPTBIT(INP_RECVIF);
				break;

			case IP_RECVTTL:
				optval = OPTBIT(INP_RECVTTL);
				break;
			}
			error = sockopt_setint(sopt, optval);
			break;

#if 0	/* defined(IPSEC) || defined(FAST_IPSEC) */
		case IP_IPSEC_POLICY:
		{
			struct mbuf *m = NULL;

			/* XXX this will return EINVAL as sopt is empty */
			error = ipsec4_get_policy(inp, sopt->sopt_data,
			    sopt->sopt_size, &m);
			if (error == 0)
				error = sockopt_setmbuf(sopt, m);
			break;
		}
#endif /*IPSEC*/

		case IP_MULTICAST_IF:
		case IP_MULTICAST_TTL:
		case IP_MULTICAST_LOOP:
		case IP_ADD_MEMBERSHIP:
		case IP_DROP_MEMBERSHIP:
			error = ip_getmoptions(inp->inp_moptions, sopt);
			break;

		case IP_PORTRANGE:
			if (inp->inp_flags & INP_LOWPORT)
				optval = IP_PORTRANGE_LOW;
			else
				optval = IP_PORTRANGE_DEFAULT;

			error = sockopt_setint(sopt, optval);

			break;

		default:
			error = ENOPROTOOPT;
			break;
		}
		break;
	}
	return (error);
}

/*
 * Set up IP options in pcb for insertion in output packets.
 * Store in mbuf with pointer in pcbopt, adding pseudo-option
 * with destination address if source routed.
 */
int
ip_pcbopts(struct mbuf **pcbopt, const struct sockopt *sopt)
{
	struct mbuf *m;
	const u_char *cp;
	u_char *dp;
	int cnt;
	uint8_t optval, olen, offset;

	/* turn off any old options */
	if (*pcbopt)
		(void)m_free(*pcbopt);
	*pcbopt = NULL;

	cp = sopt->sopt_data;
	cnt = sopt->sopt_size;

	if (cnt == 0)
		return (0);	/* Only turning off any previous options */

#ifndef	__vax__
	if (cnt % sizeof(int32_t))
		return (EINVAL);
#endif

	m = m_get(M_DONTWAIT, MT_SOOPTS);
	if (m == NULL)
		return (ENOBUFS);

	dp = mtod(m, u_char *);
	memset(dp, 0, sizeof(struct in_addr));
	dp += sizeof(struct in_addr);
	m->m_len = sizeof(struct in_addr);

	/*
	 * IP option list according to RFC791. Each option is of the form
	 *
	 *	[optval] [olen] [(olen - 2) data bytes]
	 *
	 * we validate the list and copy options to an mbuf for prepending
	 * to data packets. The IP first-hop destination address will be
	 * stored before actual options and is zero if unset.
	 */
	while (cnt > 0) {
		optval = cp[IPOPT_OPTVAL];

		if (optval == IPOPT_EOL || optval == IPOPT_NOP) {
			olen = 1;
		} else {
			if (cnt < IPOPT_OLEN + 1)
				goto bad;

			olen = cp[IPOPT_OLEN];
			if (olen < IPOPT_OLEN + 1 || olen > cnt)
				goto bad;
		}

		if (optval == IPOPT_LSRR || optval == IPOPT_SSRR) {
			/*
			 * user process specifies route as:
			 *	->A->B->C->D
			 * D must be our final destination (but we can't
			 * check that since we may not have connected yet).
			 * A is first hop destination, which doesn't appear in
			 * actual IP option, but is stored before the options.
			 */
			if (olen < IPOPT_OFFSET + 1 + sizeof(struct in_addr))
				goto bad;

			offset = cp[IPOPT_OFFSET];
			memcpy(mtod(m, u_char *), cp + IPOPT_OFFSET + 1,
			    sizeof(struct in_addr));

			cp += sizeof(struct in_addr);
			cnt -= sizeof(struct in_addr);
			olen -= sizeof(struct in_addr);

			if (m->m_len + olen > MAX_IPOPTLEN + sizeof(struct in_addr))
				goto bad;

			memcpy(dp, cp, olen);
			dp[IPOPT_OPTVAL] = optval;
			dp[IPOPT_OLEN] = olen;
			dp[IPOPT_OFFSET] = offset;
			break;
		} else {
			if (m->m_len + olen > MAX_IPOPTLEN + sizeof(struct in_addr))
				goto bad;

			memcpy(dp, cp, olen);
			break;
		}

		dp += olen;
		m->m_len += olen;

		if (optval == IPOPT_EOL)
			break;

		cp += olen;
		cnt -= olen;
	}

	*pcbopt = m;
	return (0);

bad:
	(void)m_free(m);
	return (EINVAL);
}

/*
 * following RFC1724 section 3.3, 0.0.0.0/8 is interpreted as interface index.
 */
static struct ifnet *
ip_multicast_if(struct in_addr *a, int *ifindexp)
{
	int ifindex;
	struct ifnet *ifp = NULL;
	struct in_ifaddr *ia;

	if (ifindexp)
		*ifindexp = 0;
	if (ntohl(a->s_addr) >> 24 == 0) {
		ifindex = ntohl(a->s_addr) & 0xffffff;
		if (ifindex < 0 || if_indexlim <= ifindex)
			return NULL;
		ifp = ifindex2ifnet[ifindex];
		if (!ifp)
			return NULL;
		if (ifindexp)
			*ifindexp = ifindex;
	} else {
		LIST_FOREACH(ia, &IN_IFADDR_HASH(a->s_addr), ia_hash) {
			if (in_hosteq(ia->ia_addr.sin_addr, *a) &&
			    (ia->ia_ifp->if_flags & IFF_MULTICAST) != 0) {
				ifp = ia->ia_ifp;
				break;
			}
		}
	}
	return ifp;
}

static int
ip_getoptval(const struct sockopt *sopt, u_int8_t *val, u_int maxval)
{
	u_int tval;
	u_char cval;
	int error;

	if (sopt == NULL)
		return EINVAL;

	switch (sopt->sopt_size) {
	case sizeof(u_char):
		error = sockopt_get(sopt, &cval, sizeof(u_char));
		tval = cval;
		break;

	case sizeof(u_int):
		error = sockopt_get(sopt, &tval, sizeof(u_int));
		break;

	default:
		error = EINVAL;
	}

	if (error)
		return error;

	if (tval > maxval)
		return EINVAL;

	*val = tval;
	return 0;
}

/*
 * Set the IP multicast options in response to user setsockopt().
 */
int
ip_setmoptions(struct ip_moptions **imop, const struct sockopt *sopt)
{
	int error = 0;
	int i;
	struct in_addr addr;
	struct ip_mreq lmreq, *mreq;
	struct ifnet *ifp;
	struct ip_moptions *imo = *imop;
	int ifindex;

	if (imo == NULL) {
		/*
		 * No multicast option buffer attached to the pcb;
		 * allocate one and initialize to default values.
		 */
		imo = malloc(sizeof(*imo), M_IPMOPTS, M_NOWAIT);
		if (imo == NULL)
			return (ENOBUFS);

		*imop = imo;
		imo->imo_multicast_ifp = NULL;
		imo->imo_multicast_addr.s_addr = INADDR_ANY;
		imo->imo_multicast_ttl = IP_DEFAULT_MULTICAST_TTL;
		imo->imo_multicast_loop = IP_DEFAULT_MULTICAST_LOOP;
		imo->imo_num_memberships = 0;
	}

	switch (sopt->sopt_name) {
	case IP_MULTICAST_IF:
		/*
		 * Select the interface for outgoing multicast packets.
		 */
		error = sockopt_get(sopt, &addr, sizeof(addr));
		if (error)
			break;

		/*
		 * INADDR_ANY is used to remove a previous selection.
		 * When no interface is selected, a default one is
		 * chosen every time a multicast packet is sent.
		 */
		if (in_nullhost(addr)) {
			imo->imo_multicast_ifp = NULL;
			break;
		}
		/*
		 * The selected interface is identified by its local
		 * IP address.  Find the interface and confirm that
		 * it supports multicasting.
		 */
		ifp = ip_multicast_if(&addr, &ifindex);
		if (ifp == NULL || (ifp->if_flags & IFF_MULTICAST) == 0) {
			error = EADDRNOTAVAIL;
			break;
		}
		imo->imo_multicast_ifp = ifp;
		if (ifindex)
			imo->imo_multicast_addr = addr;
		else
			imo->imo_multicast_addr.s_addr = INADDR_ANY;
		break;

	case IP_MULTICAST_TTL:
		/*
		 * Set the IP time-to-live for outgoing multicast packets.
		 */
		error = ip_getoptval(sopt, &imo->imo_multicast_ttl, MAXTTL);
		break;

	case IP_MULTICAST_LOOP:
		/*
		 * Set the loopback flag for outgoing multicast packets.
		 * Must be zero or one.
		 */
		error = ip_getoptval(sopt, &imo->imo_multicast_loop, 1);
		break;

	case IP_ADD_MEMBERSHIP:
		/*
		 * Add a multicast group membership.
		 * Group must be a valid IP multicast address.
		 */
		error = sockopt_get(sopt, &lmreq, sizeof(lmreq));
		if (error)
			break;

		mreq = &lmreq;

		if (!IN_MULTICAST(mreq->imr_multiaddr.s_addr)) {
			error = EINVAL;
			break;
		}
		/*
		 * If no interface address was provided, use the interface of
		 * the route to the given multicast address.
		 */
		if (in_nullhost(mreq->imr_interface)) {
			struct rtentry *rt;
			union {
				struct sockaddr		dst;
				struct sockaddr_in	dst4;
			} u;
			struct route ro;

			memset(&ro, 0, sizeof(ro));

			sockaddr_in_init(&u.dst4, &mreq->imr_multiaddr, 0);
			rtcache_setdst(&ro, &u.dst);
			ifp = (rt = rtcache_init(&ro)) != NULL ? rt->rt_ifp
			                                        : NULL;
			rtcache_free(&ro);
		} else {
			ifp = ip_multicast_if(&mreq->imr_interface, NULL);
		}
		/*
		 * See if we found an interface, and confirm that it
		 * supports multicast.
		 */
		if (ifp == NULL || (ifp->if_flags & IFF_MULTICAST) == 0) {
			error = EADDRNOTAVAIL;
			break;
		}
		/*
		 * See if the membership already exists or if all the
		 * membership slots are full.
		 */
		for (i = 0; i < imo->imo_num_memberships; ++i) {
			if (imo->imo_membership[i]->inm_ifp == ifp &&
			    in_hosteq(imo->imo_membership[i]->inm_addr,
				      mreq->imr_multiaddr))
				break;
		}
		if (i < imo->imo_num_memberships) {
			error = EADDRINUSE;
			break;
		}
		if (i == IP_MAX_MEMBERSHIPS) {
			error = ETOOMANYREFS;
			break;
		}
		/*
		 * Everything looks good; add a new record to the multicast
		 * address list for the given interface.
		 */
		if ((imo->imo_membership[i] =
		    in_addmulti(&mreq->imr_multiaddr, ifp)) == NULL) {
			error = ENOBUFS;
			break;
		}
		++imo->imo_num_memberships;
		break;

	case IP_DROP_MEMBERSHIP:
		/*
		 * Drop a multicast group membership.
		 * Group must be a valid IP multicast address.
		 */
		error = sockopt_get(sopt, &lmreq, sizeof(lmreq));
		if (error)
			break;

		mreq = &lmreq;

		if (!IN_MULTICAST(mreq->imr_multiaddr.s_addr)) {
			error = EINVAL;
			break;
		}
		/*
		 * If an interface address was specified, get a pointer
		 * to its ifnet structure.
		 */
		if (in_nullhost(mreq->imr_interface))
			ifp = NULL;
		else {
			ifp = ip_multicast_if(&mreq->imr_interface, NULL);
			if (ifp == NULL) {
				error = EADDRNOTAVAIL;
				break;
			}
		}
		/*
		 * Find the membership in the membership array.
		 */
		for (i = 0; i < imo->imo_num_memberships; ++i) {
			if ((ifp == NULL ||
			     imo->imo_membership[i]->inm_ifp == ifp) &&
			     in_hosteq(imo->imo_membership[i]->inm_addr,
				       mreq->imr_multiaddr))
				break;
		}
		if (i == imo->imo_num_memberships) {
			error = EADDRNOTAVAIL;
			break;
		}
		/*
		 * Give up the multicast address record to which the
		 * membership points.
		 */
		in_delmulti(imo->imo_membership[i]);
		/*
		 * Remove the gap in the membership array.
		 */
		for (++i; i < imo->imo_num_memberships; ++i)
			imo->imo_membership[i-1] = imo->imo_membership[i];
		--imo->imo_num_memberships;
		break;

	default:
		error = EOPNOTSUPP;
		break;
	}

	/*
	 * If all options have default values, no need to keep the mbuf.
	 */
	if (imo->imo_multicast_ifp == NULL &&
	    imo->imo_multicast_ttl == IP_DEFAULT_MULTICAST_TTL &&
	    imo->imo_multicast_loop == IP_DEFAULT_MULTICAST_LOOP &&
	    imo->imo_num_memberships == 0) {
		free(*imop, M_IPMOPTS);
		*imop = NULL;
	}

	return (error);
}

/*
 * Return the IP multicast options in response to user getsockopt().
 */
int
ip_getmoptions(struct ip_moptions *imo, struct sockopt *sopt)
{
	struct in_addr addr;
	struct in_ifaddr *ia;
	int error;
	uint8_t optval;

	error = 0;

	switch (sopt->sopt_name) {
	case IP_MULTICAST_IF:
		if (imo == NULL || imo->imo_multicast_ifp == NULL)
			addr = zeroin_addr;
		else if (imo->imo_multicast_addr.s_addr) {
			/* return the value user has set */
			addr = imo->imo_multicast_addr;
		} else {
			IFP_TO_IA(imo->imo_multicast_ifp, ia);
			addr = ia ? ia->ia_addr.sin_addr : zeroin_addr;
		}
		error = sockopt_set(sopt, &addr, sizeof(addr));
		break;

	case IP_MULTICAST_TTL:
		optval = imo ? imo->imo_multicast_ttl
			     : IP_DEFAULT_MULTICAST_TTL;

		error = sockopt_set(sopt, &optval, sizeof(optval));
		break;

	case IP_MULTICAST_LOOP:
		optval = imo ? imo->imo_multicast_loop
			     : IP_DEFAULT_MULTICAST_LOOP;

		error = sockopt_set(sopt, &optval, sizeof(optval));
		break;

	default:
		error = EOPNOTSUPP;
	}

	return (error);
}

/*
 * Discard the IP multicast options.
 */
void
ip_freemoptions(struct ip_moptions *imo)
{
	int i;

	if (imo != NULL) {
		for (i = 0; i < imo->imo_num_memberships; ++i)
			in_delmulti(imo->imo_membership[i]);
		free(imo, M_IPMOPTS);
	}
}

/*
 * Routine called from ip_output() to loop back a copy of an IP multicast
 * packet to the input queue of a specified interface.  Note that this
 * calls the output routine of the loopback "driver", but with an interface
 * pointer that might NOT be lo0ifp -- easier than replicating that code here.
 */
static void
ip_mloopback(struct ifnet *ifp, struct mbuf *m, const struct sockaddr_in *dst)
{
	struct ip *ip;
	struct mbuf *copym;

	copym = m_copypacket(m, M_DONTWAIT);
	if (copym != NULL
	 && (copym->m_flags & M_EXT || copym->m_len < sizeof(struct ip)))
		copym = m_pullup(copym, sizeof(struct ip));
	if (copym == NULL)
		return;
	/*
	 * We don't bother to fragment if the IP length is greater
	 * than the interface's MTU.  Can this possibly matter?
	 */
	ip = mtod(copym, struct ip *);

	if (copym->m_pkthdr.csum_flags & (M_CSUM_TCPv4|M_CSUM_UDPv4)) {
		in_delayed_cksum(copym);
		copym->m_pkthdr.csum_flags &=
		    ~(M_CSUM_TCPv4|M_CSUM_UDPv4);
	}

	ip->ip_sum = 0;
	ip->ip_sum = in_cksum(copym, ip->ip_hl << 2);
	(void)looutput(ifp, copym, sintocsa(dst), NULL);
}
