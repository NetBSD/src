/*	$NetBSD: in6_pcb.c,v 1.5 1999/07/03 21:30:18 thorpej Exp $	*/

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
 * Copyright (c) 1982, 1986, 1991, 1993
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
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
 *	@(#)in_pcb.c	8.2 (Berkeley) 1/4/94
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/malloc.h>
#include <sys/mbuf.h>
#include <sys/protosw.h>
#include <sys/socket.h>
#include <sys/socketvar.h>
#include <sys/ioctl.h>
#include <sys/errno.h>
#include <sys/time.h>
#include <sys/proc.h>

#include <net/if.h>
#include <net/route.h>

#include <netinet/in.h>
#include <netinet/in_var.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include <netinet/in_pcb.h>
#include <netinet6/ip6.h>
#include <netinet6/in6_pcb.h>
#include <netinet6/ip6_var.h>
#include <netinet6/nd6.h>

#include "loop.h"
#ifdef __NetBSD__
extern struct ifnet loif[NLOOP];
#endif

#ifdef IPSEC
#include <netinet6/ipsec.h>
#include <netkey/key.h>
#include <netkey/key_debug.h>
#endif /* IPSEC */

struct in6_addr zeroin6_addr;

int
in6_pcballoc(so, head)
	struct socket *so;
	struct in6pcb *head;
{
	struct in6pcb *in6p;

	MALLOC(in6p, struct in6pcb *, sizeof(*in6p), M_PCB, M_NOWAIT);
	if (in6p == NULL)
		return(ENOBUFS);
	bzero((caddr_t)in6p, sizeof(*in6p));
	in6p->in6p_head = head;
	in6p->in6p_socket = so;
	in6p->in6p_hops = -1;	/* use kernel default */
	in6p->in6p_icmp6filt = NULL;
#if 0
	insque(in6p, head);
#else
	in6p->in6p_next = head->in6p_next;
	head->in6p_next = in6p;
	in6p->in6p_prev = head;
	in6p->in6p_next->in6p_prev = in6p;
#endif
	so->so_pcb = (caddr_t)in6p;
	return(0);
}

int
in6_pcbbind(in6p, nam)
	register struct in6pcb *in6p;
	struct mbuf *nam;
{
	struct socket *so = in6p->in6p_socket;
	struct in6pcb *head = in6p->in6p_head;
	struct sockaddr_in6 *sin6 = (struct sockaddr_in6 *)NULL;
	struct proc *p = curproc;		/* XXX */
	u_short	lport = 0;
	int wild = 0, reuseport = (so->so_options & SO_REUSEPORT);
	int error;

	if (in6p->in6p_lport || !IN6_IS_ADDR_ANY(&in6p->in6p_laddr))
		return(EINVAL);
	if ((so->so_options & (SO_REUSEADDR|SO_REUSEPORT)) == 0 &&
	   ((so->so_proto->pr_flags & PR_CONNREQUIRED) == 0 ||
	    (so->so_options & SO_ACCEPTCONN) == 0))
		wild = IN6PLOOKUP_WILDCARD;
	if (nam) {
		sin6 = mtod(nam, struct sockaddr_in6 *);
		if (nam->m_len != sizeof(*sin6))
			return(EINVAL);
		/*
		 * We should check the family, but old programs
		 * incorrectly fail to intialize it.
		 */
		if (sin6->sin6_family != AF_INET6)
			return(EAFNOSUPPORT);

		/*
		 * If the scope of the destination is link-local, embed the
		 * interface index in the address.
		 */
		if (IN6_IS_SCOPE_LINKLOCAL(&sin6->sin6_addr)) {
			/* XXX boundary check is assumed to be already done. */
			/* XXX sin6_scope_id is weaker than advanced-api. */
			struct in6_pktinfo *pi;
			if (in6p->in6p_outputopts &&
			    (pi = in6p->in6p_outputopts->ip6po_pktinfo) &&
			    pi->ipi6_ifindex) {
				sin6->sin6_addr.s6_addr16[1]
					= htons(pi->ipi6_ifindex);
			} else if (IN6_IS_ADDR_MULTICAST(&sin6->sin6_addr)
				&& in6p->in6p_moptions
				&& in6p->in6p_moptions->im6o_multicast_ifp) {
				sin6->sin6_addr.s6_addr16[1] =
					htons(in6p->in6p_moptions->im6o_multicast_ifp->if_index);
			} else if (sin6->sin6_scope_id) {
				/* boundary check */
				if (sin6->sin6_scope_id < 0 
				 || if_index < sin6->sin6_scope_id) {
					return ENXIO;  /* XXX EINVAL? */
				}
				sin6->sin6_addr.s6_addr16[1]
					= htons(sin6->sin6_scope_id & 0xffff);/*XXX*/
				/* this must be cleared for ifa_ifwithaddr() */
				sin6->sin6_scope_id = 0;
			}
		}

		lport = sin6->sin6_port;
		if (IN6_IS_ADDR_MULTICAST(&sin6->sin6_addr)) {
			/*
			 * Treat SO_REUSEADDR as SO_REUSEPORT for multicast;
			 * allow compepte duplication of binding if
			 * SO_REUSEPORT is set, or if SO_REUSEADDR is set
			 * and a multicast address is bound on both
			 * new and duplicated sockets.
			 */
			if (so->so_options & SO_REUSEADDR)
				reuseport = SO_REUSEADDR|SO_REUSEPORT;
		} else if (IN6_IS_ADDR_V4MAPPED(&sin6->sin6_addr)) {
			struct sockaddr_in sin;

			bzero(&sin, sizeof(sin));
			sin.sin_len = sizeof(sin);
			sin.sin_family = AF_INET;
			bcopy(&sin6->sin6_addr.s6_addr32[3], &sin.sin_addr,
				sizeof(sin.sin_addr));
			if (ifa_ifwithaddr((struct sockaddr *)&sin) == 0)
				return EADDRNOTAVAIL;
		} else if (!IN6_IS_ADDR_ANY(&sin6->sin6_addr)) {
			struct ifaddr *ia = NULL;

			sin6->sin6_port = 0;		/* yech... */
			if ((ia = ifa_ifwithaddr((struct sockaddr *)sin6)) == 0)
				return(EADDRNOTAVAIL);

			/*
			 * XXX: bind to an anycast address might accidentally
			 * cause sending a packet with anycast source address.
			 */
			if (ia &&
			    ((struct in6_ifaddr *)ia)->ia6_flags &
			    (IN6_IFF_ANYCAST|IN6_IFF_NOTREADY|
			     IN6_IFF_DETACHED|IN6_IFF_DEPRECATED)) {
				return(EADDRNOTAVAIL);
			}
		}
		if (lport) {
			/* GROSS */
			if (ntohs(lport) < IPV6PORT_RESERVED &&
			   (error = suser(p->p_ucred, &p->p_acflag)))
				return(EACCES);

			if (IN6_IS_ADDR_V4MAPPED(&sin6->sin6_addr)) {
				/* should check this but we can't ... */
#if 0
				struct inpcb *t;

				t = in_pcblookup_bind(&tcbtable,
					(struct in_addr *)&sin6->sin6_addr.s6_addr32[3],
					lport);
				if (t && (reuseport & t->inp_socket->so_options) == 0)
					return EADDRINUSE;
#endif
			} else {
				struct in6pcb *t;

				t = in6_pcblookup(head, &zeroin6_addr, 0,
						  &sin6->sin6_addr, lport, wild);
				if (t && (reuseport & t->in6p_socket->so_options) == 0)
					return(EADDRINUSE);
			}
		}
		in6p->in6p_laddr = sin6->sin6_addr;
	}
	if (lport == 0) {
		u_short last_port;
		void *t;

		/* XXX IN6P_LOWPORT */

		/* value out of range */
		if (head->in6p_lport < IPV6PORT_ANONMIN)
			head->in6p_lport = IPV6PORT_ANONMIN;
		else if (head->in6p_lport > IPV6PORT_ANONMAX)
			head->in6p_lport = IPV6PORT_ANONMIN;
		last_port = head->in6p_lport;
		goto startover;	/*to randomize*/
		for (;;) {
			lport = htons(head->in6p_lport);
			if (IN6_IS_ADDR_V4MAPPED(&in6p->in6p_laddr)) {
#if 0
				t = in_pcblookup_bind(&tcbtable,
					(struct in_addr *)&in6p->in6p_laddr.s6_addr32[3],
					lport);
#else
				t = NULL;
#endif
			} else {
				t = in6_pcblookup(head, &zeroin6_addr, 0,
					  &in6p->in6p_laddr, lport, wild);
			}
			if (t == 0)
				break;
startover:
			if (head->in6p_lport >= IPV6PORT_ANONMAX)
				head->in6p_lport = IPV6PORT_ANONMIN;
			else
				head->in6p_lport++;
			if (head->in6p_lport == last_port)
				return (EADDRINUSE);
		}
	}
	in6p->in6p_lport = lport;
	in6p->in6p_flowinfo = sin6 ? sin6->sin6_flowinfo : 0;	/*XXX*/
	return(0);
}

/*
 * Connect from a socket to a specified address.
 * Both address and port must be specified in argument sin6.
 * If don't have a local address for this socket yet,
 * then pick one.
 */
int
in6_pcbconnect(in6p, nam)
	struct in6pcb *in6p;
	struct mbuf *nam;
{
	struct in6_addr *in6a = NULL;
	struct sockaddr_in6 *sin6 = mtod(nam, struct sockaddr_in6 *);
	struct in6_pktinfo *pi;
	struct ifnet *ifp = NULL;	/* outgoing interface */
	int error = 0;
	struct in6_addr mapped;

	(void)&in6a;				/* XXX fool gcc */

	if (nam->m_len != sizeof(*sin6))
		return(EINVAL);
	if (sin6->sin6_family != AF_INET6)
		return(EAFNOSUPPORT);
	if (sin6->sin6_port == 0)
		return(EADDRNOTAVAIL);

	/* sanity check for mapped address case */
	if (IN6_IS_ADDR_V4MAPPED(&sin6->sin6_addr)) {
		if (IN6_IS_ADDR_UNSPECIFIED(&in6p->in6p_laddr))
			in6p->in6p_laddr.s6_addr16[5] = htons(0xffff);
		if (!IN6_IS_ADDR_V4MAPPED(&in6p->in6p_laddr))
			return EINVAL;
	} else {
		if (IN6_IS_ADDR_V4MAPPED(&in6p->in6p_laddr))
			return EINVAL;
	}

	/*
	 * If the scope of the destination is link-local, embed the interface
	 * index in the address.
	 */
	if (IN6_IS_SCOPE_LINKLOCAL(&sin6->sin6_addr)) {
		/* XXX boundary check is assumed to be already done. */
		/* XXX sin6_scope_id is weaker than advanced-api. */
		if (in6p->in6p_outputopts &&
		    (pi = in6p->in6p_outputopts->ip6po_pktinfo) &&
		    pi->ipi6_ifindex) {
			sin6->sin6_addr.s6_addr16[1] = htons(pi->ipi6_ifindex);
			ifp = ifindex2ifnet[pi->ipi6_ifindex];
		}
		else if (IN6_IS_ADDR_MULTICAST(&sin6->sin6_addr) &&
			 in6p->in6p_moptions &&
			 in6p->in6p_moptions->im6o_multicast_ifp) {
			sin6->sin6_addr.s6_addr16[1] =
				htons(in6p->in6p_moptions->im6o_multicast_ifp->if_index);
			ifp = ifindex2ifnet[in6p->in6p_moptions->im6o_multicast_ifp->if_index];
		} else if (sin6->sin6_scope_id) {
			/* boundary check */
			if (sin6->sin6_scope_id < 0 
			 || if_index < sin6->sin6_scope_id) {
				return ENXIO;  /* XXX EINVAL? */
			}
			sin6->sin6_addr.s6_addr16[1]
				= htons(sin6->sin6_scope_id & 0xffff);/*XXX*/
			ifp = ifindex2ifnet[sin6->sin6_scope_id];
		}
	}

	/* Source address selection. */
	if (IN6_IS_ADDR_V4MAPPED(&in6p->in6p_laddr)
	 && in6p->in6p_laddr.s6_addr32[3] == 0) {
		struct sockaddr_in sin, *sinp;

		bzero(&sin, sizeof(sin));
		sin.sin_len = sizeof(sin);
		sin.sin_family = AF_INET;
		bcopy(&sin6->sin6_addr.s6_addr32[3], &sin.sin_addr,
			sizeof(sin.sin_addr));
		sinp = in_selectsrc(&sin, (struct route *)&in6p->in6p_route,
			in6p->in6p_socket->so_options, NULL, &error);
		if (sinp == 0) {
			if (error == 0)
				error = EADDRNOTAVAIL;
			return(error);
		}
		bzero(&mapped, sizeof(mapped));
		mapped.s6_addr16[5] = htons(0xffff);
		bcopy(&sinp->sin_addr, &mapped.s6_addr32[3], sizeof(sinp->sin_addr));
		in6a = &mapped;
	} else if (IN6_IS_ADDR_ANY(&in6p->in6p_laddr)) {
		in6a = in6_selectsrc(sin6, in6p->in6p_outputopts,
				  in6p->in6p_moptions, &in6p->in6p_route,
				  &error);
		if (in6a == 0) {
			if (error == 0)
				error = EADDRNOTAVAIL;
			return(error);
		}
	}
	if (in6p->in6p_route.ro_rt)
		ifp = in6p->in6p_route.ro_rt->rt_ifp;

	/*
	 * Default hop limit selection. If a hoplimit was specified via ioctl,
	 * use it. Else if the outgoing interface is detected and the current
	 * hop limit of the interface was specified by router advertisement,
	 * use the value.
	 * Otherwise, use the system default hoplimit.
	 */
	if (in6p->in6p_hops >= 0)
		in6p->in6p_ip6.ip6_hlim = (u_int8_t)in6p->in6p_hops;
	else if (ifp)
		in6p->in6p_ip6.ip6_hlim = nd_ifinfo[ifp->if_index].chlim;
	else
		in6p->in6p_ip6.ip6_hlim = ip6_defhlim;

	if (in6_pcblookup(in6p->in6p_head,
			 &sin6->sin6_addr,
			 sin6->sin6_port,
			 IN6_IS_ADDR_ANY(&in6p->in6p_laddr) ?
			  in6a : &in6p->in6p_laddr,
			 in6p->in6p_lport,
			 0))
		return(EADDRINUSE);
	if (IN6_IS_ADDR_ANY(&in6p->in6p_laddr)
	 || (IN6_IS_ADDR_V4MAPPED(&in6p->in6p_laddr)
	  && in6p->in6p_laddr.s6_addr32[3] == 0)) {
		if (in6p->in6p_lport == 0)
			(void)in6_pcbbind(in6p, (struct mbuf *)0);
		in6p->in6p_laddr = *in6a;
	}
	in6p->in6p_faddr = sin6->sin6_addr;
	in6p->in6p_fport = sin6->sin6_port;
	/*
	 * xxx kazu flowlabel is necessary for connect?
	 * but if this line is missing, the garbage value remains.
	 */
	in6p->in6p_flowinfo = sin6->sin6_flowinfo;
	return(0);
}

/*
 * Return an IPv6 address, which is the most appropriate for given
 * destination and user specified options.
 * If necessary, this function lookups the routing table and return
 * an entry to the caller for later use.
 */
struct in6_addr *
in6_selectsrc(dstsock, opts, mopts, ro, errorp)
	struct sockaddr_in6 *dstsock;
	struct ip6_pktopts *opts;
	struct ip6_moptions *mopts;
	struct route_in6 *ro;
	int *errorp;
{
	struct in6_addr *dst;
	struct in6_ifaddr *ia6 = 0;
	struct in6_pktinfo *pi;

	dst = &dstsock->sin6_addr;
	*errorp = 0;

	/*
	 * If the source address is explicitly specified by the caller,
	 * use it.
	 * If the caller doesn't specify the source address but
	 * the outgoing interface, use an address associated with
	 * the interface.
	 */
	if (opts && (pi = opts->ip6po_pktinfo)) {
		if (!IN6_IS_ADDR_ANY(&pi->ipi6_addr))
			return(&pi->ipi6_addr);
		else if (pi->ipi6_ifindex) {
			/* XXX boundary check is assumed to be already done. */
			ia6 = in6_ifawithscope(ifindex2ifnet[pi->ipi6_ifindex],
					       dst);
			if (ia6 == 0) {
				*errorp = EADDRNOTAVAIL;
				return(0);
			}
			return(&satosin6(&ia6->ia_addr)->sin6_addr);
		}
	}

	/*
	 * If the destination address is a multicast address and
	 * the outgoing interface for the address is specified
	 * by the caller, use an address associated with the interface.
	 * There is a sanity check here; if the destination has node-local
	 * scope, the outgoing interfacde should be a loopback address.
	 * Even if the outgoing interface is not specified, we also
	 * choose a loopback interface as the outgoing interface.
	 */
	if (IN6_IS_ADDR_MULTICAST(dst)) {
		struct ifnet *ifp = mopts ? mopts->im6o_multicast_ifp : NULL;
#ifdef __bsdi__
		extern struct ifnet loif;
#endif

		if (ifp == NULL && IN6_IS_ADDR_MC_NODELOCAL(dst)) {
#ifdef __bsdi__
			ifp = &loif;
#else
			ifp = &loif[0];
#endif
		}

		if (ifp) {
			ia6 = in6_ifawithscope(ifp, dst);
			if (ia6 == 0) {
				*errorp = EADDRNOTAVAIL;
				return(0);
			}
			return(&satosin6(&ia6->ia_addr)->sin6_addr);
		}
	}

	/*
	 * XXX How should we use sin6_scope_id???
	 */

	/*
	 * If the next hop address for the packet is specified
	 * by caller, use an address associated with the route
	 * to the next hop.
	 */
	{
		struct sockaddr_in6 *sin6_next;
		struct rtentry *rt;

		if (opts && opts->ip6po_nexthop) {
			sin6_next = satosin6(opts->ip6po_nexthop);
			rt = nd6_lookup(&sin6_next->sin6_addr, 1, NULL);
			if (rt) {
				ia6 = in6_ifawithscope(rt->rt_ifp, dst);
				if (ia6 == 0)
					ia6 = ifatoia6(rt->rt_ifa);
			}
			if (ia6 == 0) {
				*errorp = EADDRNOTAVAIL;
				return(0);
			}
			return(&satosin6(&ia6->ia_addr)->sin6_addr);
		}
	}

	/*
	 * If route is known or can be allocated now,
	 * our src addr is taken from the i/f, else punt.
	 */
	if (ro) {
		if (ro->ro_rt &&
		    !IN6_ARE_ADDR_EQUAL(&satosin6(&ro->ro_dst)->sin6_addr, dst)) {
			RTFREE(ro->ro_rt);
			ro->ro_rt = (struct rtentry *)0;
		}
		if (ro->ro_rt == (struct rtentry *)0 ||
		    ro->ro_rt->rt_ifp == (struct ifnet *)0) {
			/* No route yet, so try to acquire one */
			bzero(&ro->ro_dst, sizeof(struct sockaddr_in6));
			ro->ro_dst.sin6_family = AF_INET6;
			ro->ro_dst.sin6_len = sizeof(struct sockaddr_in6);
			ro->ro_dst.sin6_addr = *dst;
			if (IN6_IS_ADDR_MULTICAST(dst)) {
#ifdef __FreeBSD__
				ro->ro_rt = rtalloc1(&((struct route *)ro)
						     ->ro_dst, 0, 0UL);
#endif /*__FreeBSD__*/
#if defined(__bsdi__) || defined(__NetBSD__)
				ro->ro_rt = rtalloc1(&((struct route *)ro)
						     ->ro_dst, 0);
#endif /*__bsdi__*/
			} else {
#if 0 /* XXX Is this correct? */
				rtcalloc((struct route *)ro);
#else
				rtalloc((struct route *)ro);
#endif
			}
		}

		/*
		 * in_pcbconnect() checks out IFF_LOOPBACK to skip using
		 * the address. But we don't know why it does so.
		 * It is necessary to ensure the scope even for lo0
		 * so doesn't check out IFF_LOOPBACK.
		 */

		if (ro->ro_rt) {
			ia6 = in6_ifawithscope(ro->ro_rt->rt_ifa->ifa_ifp, dst);
			if (ia6 == 0) /* xxx scope error ?*/
				ia6 = ifatoia6(ro->ro_rt->rt_ifa);
		}
#if 0
		/*
		 * xxx The followings are necessary? (kazu)
		 * I don't think so.
		 * It's for SO_DONTROUTE option in IPv4.(jinmei)
		 */
		if (ia6 == 0) {
			struct sockaddr_in6 sin6 = {sizeof(sin6), AF_INET6, 0};

			sin6->sin6_addr = *dst;

			ia6 = ifatoia6(ifa_ifwithdstaddr(sin6tosa(&sin6)));
			if (ia6 == 0)
				ia6 = ifatoia6(ifa_ifwithnet(sin6tosa(&sin6)));
			if (ia6 == 0)
				return(0);
			return(&satosin6(&ia6->ia_addr)->sin6_addr);
		}
#endif /* 0 */
		if (ia6 == 0) {
			*errorp = EHOSTUNREACH;	/* no route */
			return(0);
		}
		return(&satosin6(&ia6->ia_addr)->sin6_addr);
	}

	*errorp = EADDRNOTAVAIL;
	return(0);
}

void
in6_pcbdisconnect(in6p)
	struct in6pcb *in6p;
{
	bzero((caddr_t)&in6p->in6p_faddr, sizeof(in6p->in6p_faddr));
	in6p->in6p_fport = 0;
	if (in6p->in6p_socket->so_state & SS_NOFDREF)
		in6_pcbdetach(in6p);
}

void
in6_pcbdetach(in6p)
	struct in6pcb *in6p;
{
	struct socket *so = in6p->in6p_socket;

#ifdef IPSEC
	if (sotoin6pcb(so) != 0)
		key_freeso(so);
	ipsec6_delete_pcbpolicy(in6p);
#endif /* IPSEC */
	sotoin6pcb(so) = 0;
	sofree(so);
	if (in6p->in6p_options)
		m_freem(in6p->in6p_options);
	if (in6p->in6p_outputopts) {
		if (in6p->in6p_outputopts->ip6po_rthdr &&
		    in6p->in6p_outputopts->ip6po_route.ro_rt)
			RTFREE(in6p->in6p_outputopts->ip6po_route.ro_rt);
		if (in6p->in6p_outputopts->ip6po_m)
			(void)m_free(in6p->in6p_outputopts->ip6po_m);
		free(in6p->in6p_outputopts, M_IP6OPT);
	}
	if (in6p->in6p_route.ro_rt)
		rtfree(in6p->in6p_route.ro_rt);
	ip6_freemoptions(in6p->in6p_moptions);
#if 0
	remque(in6p);
#else
	in6p->in6p_next->in6p_prev = in6p->in6p_prev;
	in6p->in6p_prev->in6p_next = in6p->in6p_next;
	in6p->in6p_prev = NULL;
#endif
	FREE(in6p, M_PCB);
}

void
in6_setsockaddr(in6p, nam)
	struct in6pcb *in6p;
	struct mbuf *nam;
{
	struct sockaddr_in6 *sin6;

	nam->m_len = sizeof(*sin6);
	sin6 = mtod(nam, struct sockaddr_in6 *);
	bzero((caddr_t)sin6, sizeof(*sin6));
	sin6->sin6_family = AF_INET6;
	sin6->sin6_len = sizeof(struct sockaddr_in6);
	sin6->sin6_port = in6p->in6p_lport;
	sin6->sin6_addr = in6p->in6p_laddr;
	if (IN6_IS_SCOPE_LINKLOCAL(&sin6->sin6_addr))
		sin6->sin6_scope_id = ntohs(sin6->sin6_addr.s6_addr16[1]);
	else
		sin6->sin6_scope_id = 0;	/*XXX*/
	if (IN6_IS_SCOPE_LINKLOCAL(&sin6->sin6_addr))
		sin6->sin6_addr.s6_addr16[1] = 0;
}

void
in6_setpeeraddr(in6p, nam)
	struct in6pcb *in6p;
	struct mbuf *nam;
{
	struct sockaddr_in6 *sin6;

	nam->m_len = sizeof(*sin6);
	sin6 = mtod(nam, struct sockaddr_in6 *);
	bzero((caddr_t)sin6, sizeof(*sin6));
	sin6->sin6_family = AF_INET6;
	sin6->sin6_len = sizeof(struct sockaddr_in6);
	sin6->sin6_port = in6p->in6p_fport;
	sin6->sin6_addr = in6p->in6p_faddr;
	if (IN6_IS_SCOPE_LINKLOCAL(&sin6->sin6_addr))
		sin6->sin6_scope_id = ntohs(sin6->sin6_addr.s6_addr16[1]);
	else
		sin6->sin6_scope_id = 0;	/*XXX*/
	if (IN6_IS_SCOPE_LINKLOCAL(&sin6->sin6_addr))
		sin6->sin6_addr.s6_addr16[1] = 0;
}

/*
 * Pass some notification to all connections of a protocol
 * associated with address dst.  The local address and/or port numbers
 * may be specified to limit the search.  The "usual action" will be
 * taken, depending on the ctlinput cmd.  The caller must filter any
 * cmds that are uninteresting (e.g., no error in the map).
 * Call the protocol specific routine (if any) to report
 * any errors for each matching socket.
 *
 * Must be called at splnet.
 */
int
in6_pcbnotify(head, dst, fport_arg, laddr6, lport_arg, cmd, notify)
	struct in6pcb *head;
	struct sockaddr *dst;
	u_int fport_arg, lport_arg;
	struct in6_addr *laddr6;
	int cmd;
	void (*notify) __P((struct in6pcb *, int));
{
	struct in6pcb *in6p, *oin6p;
	struct in6_addr faddr6;
	u_short	fport = fport_arg, lport = lport_arg;
	int errno;
	int nmatch = 0;

	if ((unsigned)cmd > PRC_NCMDS || dst->sa_family != AF_INET6)
		return 0;
	faddr6 = ((struct sockaddr_in6 *)dst)->sin6_addr;
	if (IN6_IS_ADDR_ANY(&faddr6))
		return 0;

	/*
	 * Redirects go to all references to the destination,
	 * and use in_rtchange to invalidate the route cache.
	 * Dead host indications: notify all references to the destination.
	 * Otherwise, if we have knowledge of the local port and address,
	 * deliver only to that socket.
	 */
	if (PRC_IS_REDIRECT(cmd) || cmd == PRC_HOSTDEAD) {
		fport = 0;
		lport = 0;
		bzero((caddr_t)laddr6, sizeof(*laddr6));
		if (cmd != PRC_HOSTDEAD)
			notify = in6_rtchange;
	}
	if (notify == NULL)
		return 0;
	errno = inet6ctlerrmap[cmd];
	for (in6p = head->in6p_next; in6p != head;) {
		if (!IN6_ARE_ADDR_EQUAL(&in6p->in6p_faddr,&faddr6) ||
		   in6p->in6p_socket == 0 ||
		   (lport && in6p->in6p_lport != lport) ||
		   (!IN6_IS_ADDR_ANY(laddr6) &&
		    !IN6_ARE_ADDR_EQUAL(&in6p->in6p_laddr, laddr6)) ||
		   (fport && in6p->in6p_fport != fport)) {
			in6p = in6p->in6p_next;
			continue;
		}
		oin6p = in6p;
		in6p = in6p->in6p_next;
		(*notify)(oin6p, errno);
		nmatch++;
	}
	return nmatch;
}

/*
 * Check for alternatives when higher level complains
 * about service problems.  For now, invalidate cached
 * routing information.  If the route was created dynamically
 * (by a redirect), time to try a default gateway again.
 */
void
in6_losing(in6p)
	struct in6pcb *in6p;
{
	struct rtentry *rt;
	struct rt_addrinfo info;

	if ((rt = in6p->in6p_route.ro_rt) != NULL) {
		in6p->in6p_route.ro_rt = 0;
		bzero((caddr_t)&info, sizeof(info));
		info.rti_info[RTAX_DST] =
			(struct sockaddr *)&in6p->in6p_route.ro_dst;
		info.rti_info[RTAX_GATEWAY] = rt->rt_gateway;
		info.rti_info[RTAX_NETMASK] = rt_mask(rt);
		rt_missmsg(RTM_LOSING, &info, rt->rt_flags, 0);
		if (rt->rt_flags & RTF_DYNAMIC)
			(void)rtrequest(RTM_DELETE, rt_key(rt),
					rt->rt_gateway, rt_mask(rt), rt->rt_flags,
					(struct rtentry **)0);
		else
		/*
		 * A new route can be allocated
		 * the next time output is attempted.
		 */
			rtfree(rt);
	}
}

/*
 * After a routing change, flush old routing
 * and allocate a (hopefully) better one.
 */
void
in6_rtchange(in6p, errno)
	struct in6pcb *in6p;
	int errno;
{
	if (in6p->in6p_route.ro_rt) {
		rtfree(in6p->in6p_route.ro_rt);
		in6p->in6p_route.ro_rt = 0;
		/*
		 * A new route can be allocated the next time
		 * output is attempted.
		 */
	}
}

struct in6pcb *
in6_pcblookup(head, faddr6, fport_arg, laddr6, lport_arg, flags)
	struct in6pcb *head;
	struct in6_addr *faddr6, *laddr6;
	u_int fport_arg, lport_arg;
	int flags;
{
	struct in6pcb *in6p, *match = 0;
	int matchwild = 3, wildcard;
	u_short	fport = fport_arg, lport = lport_arg;

	for (in6p = head->in6p_next; in6p != head; in6p = in6p->in6p_next) {
		if (in6p->in6p_lport != lport)
			continue;
		wildcard = 0;
		if (!IN6_IS_ADDR_ANY(&in6p->in6p_laddr)) {
			if (IN6_IS_ADDR_ANY(laddr6))
				wildcard++;
			else if (!IN6_ARE_ADDR_EQUAL(&in6p->in6p_laddr, laddr6))
				continue;
		} else {
			if (!IN6_IS_ADDR_ANY(laddr6))
				wildcard++;
		}
		if (!IN6_IS_ADDR_ANY(&in6p->in6p_faddr)) {
			if (IN6_IS_ADDR_ANY(faddr6))
				wildcard++;
			else if (!IN6_ARE_ADDR_EQUAL(&in6p->in6p_faddr, faddr6)
			      || in6p->in6p_fport != fport)
				continue;
		} else {
			if (!IN6_IS_ADDR_ANY(faddr6))
				wildcard++;
		}
		if (wildcard && (flags & IN6PLOOKUP_WILDCARD) == 0)
			continue;
		if (wildcard < matchwild) {
			match = in6p;
			matchwild = wildcard;
			if (matchwild == 0)
				break;
		}
	}
	return(match);
}

#ifndef TCP6
struct rtentry *
in6_pcbrtentry(in6p)
	struct in6pcb *in6p;
{
	struct route_in6 *ro;

	ro = &in6p->in6p_route;

	if (ro->ro_rt == NULL) {
		/*
		 * No route yet, so try to acquire one.
		 */
		if (!IN6_IS_ADDR_UNSPECIFIED(&in6p->in6p_faddr)) {
			bzero(&ro->ro_dst, sizeof(ro->ro_dst));
			ro->ro_dst.sin6_family = AF_INET6;
			ro->ro_dst.sin6_len = sizeof(struct sockaddr_in6);
			satosin6(&ro->ro_dst)->sin6_addr = in6p->in6p_faddr;
			rtalloc((struct route *)ro);
		}
	}
	return (ro->ro_rt);
}

struct in6pcb *
in6_pcblookup_connect(head, faddr6, fport_arg, laddr6, lport_arg)
	struct in6pcb *head;
	struct in6_addr *faddr6, *laddr6;
	u_int fport_arg, lport_arg;
{
	struct in6pcb *in6p;
	u_short	fport = fport_arg, lport = lport_arg;

	for (in6p = head->in6p_next; in6p != head; in6p = in6p->in6p_next) {
		/* find exact match on both source and dest */
		if (in6p->in6p_fport != fport)
			continue;
		if (in6p->in6p_lport != lport)
			continue;
		if (IN6_IS_ADDR_UNSPECIFIED(&in6p->in6p_faddr))
			continue;
		if (!IN6_ARE_ADDR_EQUAL(&in6p->in6p_faddr, faddr6))
			continue;
		if (IN6_IS_ADDR_UNSPECIFIED(&in6p->in6p_laddr))
			continue;
		if (!IN6_ARE_ADDR_EQUAL(&in6p->in6p_laddr, laddr6))
			continue;
		return in6p;
	}
	return NULL;
}

struct in6pcb *
in6_pcblookup_bind(head, laddr6, lport_arg)
	struct in6pcb *head;
	struct in6_addr *laddr6;
	u_int lport_arg;
{
	struct in6pcb *in6p, *match;
	u_short	lport = lport_arg;

	match = NULL;
	for (in6p = head->in6p_next; in6p != head; in6p = in6p->in6p_next) {
		/*
	 	 * find destination match.  exact match is preferred
		 * against wildcard match.
		 */
		if (in6p->in6p_fport != 0)
			continue;
		if (in6p->in6p_lport != lport)
			continue;
		if (IN6_IS_ADDR_UNSPECIFIED(&in6p->in6p_laddr))
			match = in6p;
		else if (IN6_ARE_ADDR_EQUAL(&in6p->in6p_laddr, laddr6))
			return in6p;
	}
	return match;
}
#endif
