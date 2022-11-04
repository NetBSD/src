/*	$NetBSD: in6_pcb.c,v 1.177 2022/11/04 09:04:27 ozaki-r Exp $	*/
/*	$KAME: in6_pcb.c,v 1.84 2001/02/08 18:02:08 itojun Exp $	*/

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
 *	@(#)in_pcb.c	8.2 (Berkeley) 1/4/94
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: in6_pcb.c,v 1.177 2022/11/04 09:04:27 ozaki-r Exp $");

#ifdef _KERNEL_OPT
#include "opt_inet.h"
#include "opt_ipsec.h"
#endif

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/mbuf.h>
#include <sys/protosw.h>
#include <sys/socket.h>
#include <sys/socketvar.h>
#include <sys/ioctl.h>
#include <sys/errno.h>
#include <sys/time.h>
#include <sys/proc.h>
#include <sys/kauth.h>
#include <sys/domain.h>
#include <sys/once.h>

#include <net/if.h>
#include <net/route.h>

#include <netinet/in.h>
#include <netinet/in_var.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include <netinet/in_pcb.h>
#include <netinet/ip6.h>
#include <netinet/portalgo.h>
#include <netinet6/ip6_var.h>
#include <netinet6/in6_pcb.h>
#include <netinet6/scope6_var.h>

#include "faith.h"

#ifdef IPSEC
#include <netipsec/ipsec.h>
#include <netipsec/ipsec6.h>
#include <netipsec/key.h>
#endif /* IPSEC */

#include <netinet/tcp_vtw.h>

const struct in6_addr zeroin6_addr;

#define	IN6PCBHASH_PORT(table, lport) \
	&(table)->inpt_porthashtbl[ntohs(lport) & (table)->inpt_porthash]
#define IN6PCBHASH_BIND(table, laddr, lport) \
	&(table)->inpt_bindhashtbl[ \
	    (((laddr)->s6_addr32[0] ^ (laddr)->s6_addr32[1] ^ \
	      (laddr)->s6_addr32[2] ^ (laddr)->s6_addr32[3]) + ntohs(lport)) & \
	    (table)->inpt_bindhash]
#define IN6PCBHASH_CONNECT(table, faddr, fport, laddr, lport) \
	&(table)->inpt_bindhashtbl[ \
	    ((((faddr)->s6_addr32[0] ^ (faddr)->s6_addr32[1] ^ \
	      (faddr)->s6_addr32[2] ^ (faddr)->s6_addr32[3]) + ntohs(fport)) + \
	     (((laddr)->s6_addr32[0] ^ (laddr)->s6_addr32[1] ^ \
	      (laddr)->s6_addr32[2] ^ (laddr)->s6_addr32[3]) + \
	      ntohs(lport))) & (table)->inpt_bindhash]

int ip6_anonportmin = IPV6PORT_ANONMIN;
int ip6_anonportmax = IPV6PORT_ANONMAX;
int ip6_lowportmin  = IPV6PORT_RESERVEDMIN;
int ip6_lowportmax  = IPV6PORT_RESERVEDMAX;

void
in6pcb_init(struct inpcbtable *table, int bindhashsize, int connecthashsize)
{

	inpcb_init(table, bindhashsize, connecthashsize);
	table->inpt_lastport = (in_port_t)ip6_anonportmax;
}

/*
 * Bind address from sin6 to inp.
 */
static int
in6pcb_bind_addr(struct inpcb *inp, struct sockaddr_in6 *sin6, struct lwp *l)
{
	int error;
	int s;

	/*
	 * We should check the family, but old programs
	 * incorrectly fail to initialize it.
	 */
	if (sin6->sin6_family != AF_INET6)
		return EAFNOSUPPORT;

#ifndef INET
	if (IN6_IS_ADDR_V4MAPPED(&sin6->sin6_addr))
		return EADDRNOTAVAIL;
#endif

	if ((error = sa6_embedscope(sin6, ip6_use_defzone)) != 0)
		return error;

	s = pserialize_read_enter();
	if (IN6_IS_ADDR_V4MAPPED(&sin6->sin6_addr)) {
		if ((inp->inp_flags & IN6P_IPV6_V6ONLY) != 0) {
			error = EINVAL;
			goto out;
		}
		if (sin6->sin6_addr.s6_addr32[3]) {
			struct sockaddr_in sin;

			memset(&sin, 0, sizeof(sin));
			sin.sin_len = sizeof(sin);
			sin.sin_family = AF_INET;
			bcopy(&sin6->sin6_addr.s6_addr32[3],
			    &sin.sin_addr, sizeof(sin.sin_addr));
			if (!IN_MULTICAST(sin.sin_addr.s_addr)) {
				struct ifaddr *ifa;
				ifa = ifa_ifwithaddr((struct sockaddr *)&sin);
				if (ifa == NULL &&
				    (inp->inp_flags & IN6P_BINDANY) == 0) {
					error = EADDRNOTAVAIL;
					goto out;
				}
			}
		}
	} else if (IN6_IS_ADDR_MULTICAST(&sin6->sin6_addr)) {
		// succeed
	} else if (!IN6_IS_ADDR_UNSPECIFIED(&sin6->sin6_addr)) {
		struct ifaddr *ifa = NULL;

		if ((inp->inp_flags & IN6P_FAITH) == 0) {
			ifa = ifa_ifwithaddr(sin6tosa(sin6));
			if (ifa == NULL &&
			    (inp->inp_flags & IN6P_BINDANY) == 0) {
				error = EADDRNOTAVAIL;
				goto out;
			}
		}

		/*
		 * bind to an anycast address might accidentally
		 * cause sending a packet with an anycast source
		 * address, so we forbid it.
		 *
		 * We should allow to bind to a deprecated address,
		 * since the application dare to use it.
		 * But, can we assume that they are careful enough
		 * to check if the address is deprecated or not?
		 * Maybe, as a safeguard, we should have a setsockopt
		 * flag to control the bind(2) behavior against
		 * deprecated addresses (default: forbid bind(2)).
		 */
		if (ifa &&
		    ifatoia6(ifa)->ia6_flags &
		    (IN6_IFF_ANYCAST | IN6_IFF_DUPLICATED)) {
			error = EADDRNOTAVAIL;
			goto out;
		}
	}
	in6p_laddr(inp) = sin6->sin6_addr;
	error = 0;
out:
	pserialize_read_exit(s);
	return error;
}

/*
 * Bind port from sin6 to inp.
 */
static int
in6pcb_bind_port(struct inpcb *inp, struct sockaddr_in6 *sin6, struct lwp *l)
{
	struct inpcbtable *table = inp->inp_table;
	struct socket *so = inp->inp_socket;
	int wild = 0, reuseport = (so->so_options & SO_REUSEPORT);
	int error;

	if ((so->so_options & (SO_REUSEADDR|SO_REUSEPORT)) == 0 &&
	   ((so->so_proto->pr_flags & PR_CONNREQUIRED) == 0 ||
	    (so->so_options & SO_ACCEPTCONN) == 0))
		wild = 1;

	if (sin6->sin6_port != 0) {
		enum kauth_network_req req;

#ifndef IPNOPRIVPORTS
		if (ntohs(sin6->sin6_port) < IPV6PORT_RESERVED)
			req = KAUTH_REQ_NETWORK_BIND_PRIVPORT;
		else
#endif /* IPNOPRIVPORTS */
			req = KAUTH_REQ_NETWORK_BIND_PORT;

		error = kauth_authorize_network(l->l_cred, KAUTH_NETWORK_BIND,
		    req, so, sin6, NULL);
		if (error)
			return EACCES;
	}

	if (IN6_IS_ADDR_MULTICAST(&sin6->sin6_addr)) {
		/*
		 * Treat SO_REUSEADDR as SO_REUSEPORT for multicast;
		 * allow compepte duplication of binding if
		 * SO_REUSEPORT is set, or if SO_REUSEADDR is set
		 * and a multicast address is bound on both
		 * new and duplicated sockets.
		 */
		if (so->so_options & (SO_REUSEADDR | SO_REUSEPORT))
			reuseport = SO_REUSEADDR|SO_REUSEPORT;
	}

	if (sin6->sin6_port != 0) {
		if (IN6_IS_ADDR_V4MAPPED(&sin6->sin6_addr)) {
#ifdef INET
			struct inpcb *t;
			struct vestigial_inpcb vestige;

			t = inpcb_lookup_local(table,
			    *(struct in_addr *)&sin6->sin6_addr.s6_addr32[3],
			    sin6->sin6_port, wild, &vestige);
			if (t && (reuseport & t->inp_socket->so_options) == 0)
				return EADDRINUSE;
			if (!t
			    && vestige.valid
			    && !(reuseport && vestige.reuse_port))
			    return EADDRINUSE;
#else
			return EADDRNOTAVAIL;
#endif
		}

		{
			struct inpcb *t;
			struct vestigial_inpcb vestige;

			t = in6pcb_lookup_local(table, &sin6->sin6_addr,
			    sin6->sin6_port, wild, &vestige);
			if (t && (reuseport & t->inp_socket->so_options) == 0)
				return EADDRINUSE;
			if (!t
			    && vestige.valid
			    && !(reuseport && vestige.reuse_port))
			    return EADDRINUSE;
		}
	}

	if (sin6->sin6_port == 0) {
		int e;
		e = in6pcb_set_port(sin6, inp, l);
		if (e != 0)
			return e;
	} else {
		inp->inp_lport = sin6->sin6_port;
		inpcb_set_state(inp, INP_BOUND);
	}

	LIST_REMOVE(inp, inp_lhash);
	LIST_INSERT_HEAD(IN6PCBHASH_PORT(table, inp->inp_lport),
	    inp, inp_lhash);

	return 0;
}

int
in6pcb_bind(void *v, struct sockaddr_in6 *sin6, struct lwp *l)
{
	struct inpcb *inp = v;
	struct sockaddr_in6 lsin6;
	int error;

	if (inp->inp_af != AF_INET6)
		return EINVAL;

	/*
	 * If we already have a local port or a local address it means we're
	 * bounded.
	 */
	if (inp->inp_lport || !(IN6_IS_ADDR_UNSPECIFIED(&in6p_laddr(inp)) ||
	    (IN6_IS_ADDR_V4MAPPED(&in6p_laddr(inp)) &&
	      in6p_laddr(inp).s6_addr32[3] == 0)))
		return EINVAL;

	if (NULL != sin6) {
		/* We were provided a sockaddr_in6 to use. */
		if (sin6->sin6_len != sizeof(*sin6))
			return EINVAL;
	} else {
		/* We always bind to *something*, even if it's "anything". */
		lsin6 = *((const struct sockaddr_in6 *)
		    inp->inp_socket->so_proto->pr_domain->dom_sa_any);
		sin6 = &lsin6;
	}

	/* Bind address. */
	error = in6pcb_bind_addr(inp, sin6, l);
	if (error)
		return error;

	/* Bind port. */
	error = in6pcb_bind_port(inp, sin6, l);
	if (error) {
		/*
		 * Reset the address here to "any" so we don't "leak" the
		 * inpcb.
		 */
		in6p_laddr(inp) = in6addr_any;

		return error;
	}


#if 0
	in6p_flowinfo(inp) = 0;	/* XXX */
#endif
	return 0;
}

/*
 * Connect from a socket to a specified address.
 * Both address and port must be specified in argument sin6.
 * If don't have a local address for this socket yet,
 * then pick one.
 */
int
in6pcb_connect(void *v, struct sockaddr_in6 *sin6, struct lwp *l)
{
	struct inpcb *inp = v;
	struct in6_addr *in6a = NULL;
	struct in6_addr ia6;
	struct ifnet *ifp = NULL;	/* outgoing interface */
	int error = 0;
	int scope_ambiguous = 0;
#ifdef INET
	struct in6_addr mapped;
#endif
	struct sockaddr_in6 tmp;
	struct vestigial_inpcb vestige;
	struct psref psref;
	int bound;

	(void)&in6a;				/* XXX fool gcc */

	if (inp->inp_af != AF_INET6)
		return EINVAL;

	if (sin6->sin6_len != sizeof(*sin6))
		return EINVAL;
	if (sin6->sin6_family != AF_INET6)
		return EAFNOSUPPORT;
	if (sin6->sin6_port == 0)
		return EADDRNOTAVAIL;

	if (IN6_IS_ADDR_MULTICAST(&sin6->sin6_addr) &&
	    inp->inp_socket->so_type == SOCK_STREAM)
		return EADDRNOTAVAIL;

	if (sin6->sin6_scope_id == 0 && !ip6_use_defzone)
		scope_ambiguous = 1;
	if ((error = sa6_embedscope(sin6, ip6_use_defzone)) != 0)
		return error;

	/* sanity check for mapped address case */
	if (IN6_IS_ADDR_V4MAPPED(&sin6->sin6_addr)) {
		if ((inp->inp_flags & IN6P_IPV6_V6ONLY) != 0)
			return EINVAL;
		if (IN6_IS_ADDR_UNSPECIFIED(&in6p_laddr(inp)))
			in6p_laddr(inp).s6_addr16[5] = htons(0xffff);
		if (!IN6_IS_ADDR_V4MAPPED(&in6p_laddr(inp)))
			return EINVAL;
	} else
	{
		if (IN6_IS_ADDR_V4MAPPED(&in6p_laddr(inp)))
			return EINVAL;
	}

	/* protect *sin6 from overwrites */
	tmp = *sin6;
	sin6 = &tmp;

	bound = curlwp_bind();
	/* Source address selection. */
	if (IN6_IS_ADDR_V4MAPPED(&in6p_laddr(inp)) &&
	    in6p_laddr(inp).s6_addr32[3] == 0) {
#ifdef INET
		struct sockaddr_in sin;
		struct in_ifaddr *ia4;
		struct psref _psref;

		memset(&sin, 0, sizeof(sin));
		sin.sin_len = sizeof(sin);
		sin.sin_family = AF_INET;
		memcpy(&sin.sin_addr, &sin6->sin6_addr.s6_addr32[3],
			sizeof(sin.sin_addr));
		ia4 = in_selectsrc(&sin, &inp->inp_route,
			inp->inp_socket->so_options, NULL, &error, &_psref);
		if (ia4 == NULL) {
			if (error == 0)
				error = EADDRNOTAVAIL;
			curlwp_bindx(bound);
			return error;
		}
		memset(&mapped, 0, sizeof(mapped));
		mapped.s6_addr16[5] = htons(0xffff);
		memcpy(&mapped.s6_addr32[3], &IA_SIN(ia4)->sin_addr,
		    sizeof(IA_SIN(ia4)->sin_addr));
		ia4_release(ia4, &_psref);
		in6a = &mapped;
#else
		curlwp_bindx(bound);
		return EADDRNOTAVAIL;
#endif
	} else {
		/*
		 * XXX: in6_selectsrc might replace the bound local address
		 * with the address specified by setsockopt(IPV6_PKTINFO).
		 * Is it the intended behavior?
		 */
		error = in6_selectsrc(sin6, in6p_outputopts(inp),
		    in6p_moptions(inp), &inp->inp_route, &in6p_laddr(inp),
		    &ifp, &psref, &ia6);
		if (error == 0)
			in6a = &ia6;
		if (ifp && scope_ambiguous &&
		    (error = in6_setscope(&sin6->sin6_addr, ifp, NULL)) != 0) {
			if_put(ifp, &psref);
			curlwp_bindx(bound);
			return error;
		}

		if (in6a == NULL) {
			if_put(ifp, &psref);
			curlwp_bindx(bound);
			if (error == 0)
				error = EADDRNOTAVAIL;
			return error;
		}
	}

	if (ifp != NULL) {
		in6p_ip6(inp).ip6_hlim = (u_int8_t)in6pcb_selecthlim(inp, ifp);
		if_put(ifp, &psref);
	} else
		in6p_ip6(inp).ip6_hlim = (u_int8_t)in6pcb_selecthlim_rt(inp);
	curlwp_bindx(bound);

	if (in6pcb_lookup(inp->inp_table, &sin6->sin6_addr,
	    sin6->sin6_port,
	    IN6_IS_ADDR_UNSPECIFIED(&in6p_laddr(inp)) ? in6a : &in6p_laddr(inp),
				  inp->inp_lport, 0, &vestige)
		|| vestige.valid)
		return EADDRINUSE;
	if (IN6_IS_ADDR_UNSPECIFIED(&in6p_laddr(inp)) ||
	    (IN6_IS_ADDR_V4MAPPED(&in6p_laddr(inp)) &&
	     in6p_laddr(inp).s6_addr32[3] == 0))
	{
		if (inp->inp_lport == 0) {
			error = in6pcb_bind(inp, NULL, l);
			if (error != 0)
				return error;
		}
		in6p_laddr(inp) = *in6a;
	}
	in6p_faddr(inp) = sin6->sin6_addr;
	inp->inp_fport = sin6->sin6_port;

        /* Late bind, if needed */
	if (inp->inp_bindportonsend) {
               struct sockaddr_in6 lsin = *((const struct sockaddr_in6 *)
		    inp->inp_socket->so_proto->pr_domain->dom_sa_any);
		lsin.sin6_addr = in6p_laddr(inp);
		lsin.sin6_port = 0;

               if ((error = in6pcb_bind_port(inp, &lsin, l)) != 0)
                       return error;
	}
	
	inpcb_set_state(inp, INP_CONNECTED);
	in6p_flowinfo(inp) &= ~IPV6_FLOWLABEL_MASK;
	if (ip6_auto_flowlabel)
		in6p_flowinfo(inp) |=
		    (htonl(ip6_randomflowlabel()) & IPV6_FLOWLABEL_MASK);
#if defined(IPSEC)
	if (ipsec_enabled && inp->inp_socket->so_type == SOCK_STREAM)
		ipsec_pcbconn(inp->inp_sp);
#endif
	return 0;
}

void
in6pcb_disconnect(struct inpcb *inp)
{
	memset((void *)&in6p_faddr(inp), 0, sizeof(in6p_faddr(inp)));
	inp->inp_fport = 0;
	inpcb_set_state(inp, INP_BOUND);
	in6p_flowinfo(inp) &= ~IPV6_FLOWLABEL_MASK;
#if defined(IPSEC)
	if (ipsec_enabled)
		ipsec_pcbdisconn(inp->inp_sp);
#endif
	if (inp->inp_socket->so_state & SS_NOFDREF)
		inpcb_destroy(inp);
}

void
in6pcb_fetch_sockaddr(struct inpcb *inp, struct sockaddr_in6 *sin6)
{

	if (inp->inp_af != AF_INET6)
		return;

	sockaddr_in6_init(sin6, &in6p_laddr(inp), inp->inp_lport, 0, 0);
	(void)sa6_recoverscope(sin6); /* XXX: should catch errors */
}

void
in6pcb_fetch_peeraddr(struct inpcb *inp, struct sockaddr_in6 *sin6)
{

	if (inp->inp_af != AF_INET6)
		return;

	sockaddr_in6_init(sin6, &in6p_faddr(inp), inp->inp_fport, 0, 0);
	(void)sa6_recoverscope(sin6); /* XXX: should catch errors */
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
 * Must be called at splsoftnet.
 *
 * Note: src (4th arg) carries the flowlabel value on the original IPv6
 * header, in sin6_flowinfo member.
 */
int
in6pcb_notify(struct inpcbtable *table, const struct sockaddr *dst,
    u_int fport_arg, const struct sockaddr *src, u_int lport_arg, int cmd,
    void *cmdarg, void (*notify)(struct inpcb *, int))
{
	struct inpcb *inp;
	struct sockaddr_in6 sa6_src;
	const struct sockaddr_in6 *sa6_dst;
	in_port_t fport = fport_arg, lport = lport_arg;
	int errno;
	int nmatch = 0;
	u_int32_t flowinfo;

	if ((unsigned)cmd >= PRC_NCMDS || dst->sa_family != AF_INET6)
		return 0;

	sa6_dst = (const struct sockaddr_in6 *)dst;
	if (IN6_IS_ADDR_UNSPECIFIED(&sa6_dst->sin6_addr))
		return 0;

	/*
	 * note that src can be NULL when we get notify by local fragmentation.
	 */
	sa6_src = (src == NULL) ? sa6_any : *(const struct sockaddr_in6 *)src;
	flowinfo = sa6_src.sin6_flowinfo;

	/*
	 * Redirects go to all references to the destination,
	 * and use in6pcb_rtchange to invalidate the route cache.
	 * Dead host indications: also use in6pcb_rtchange to invalidate
	 * the cache, and deliver the error to all the sockets.
	 * Otherwise, if we have knowledge of the local port and address,
	 * deliver only to that socket.
	 */
	if (PRC_IS_REDIRECT(cmd) || cmd == PRC_HOSTDEAD) {
		fport = 0;
		lport = 0;
		memset((void *)&sa6_src.sin6_addr, 0, sizeof(sa6_src.sin6_addr));

		if (cmd != PRC_HOSTDEAD)
			notify = in6pcb_rtchange;
	}

	errno = inet6ctlerrmap[cmd];
	TAILQ_FOREACH(inp, &table->inpt_queue, inp_queue) {
		struct rtentry *rt = NULL;

		if (inp->inp_af != AF_INET6)
			continue;

		/*
		 * Under the following condition, notify of redirects
		 * to the pcb, without making address matches against inpcb.
		 * - redirect notification is arrived.
		 * - the inpcb is unconnected.
		 * - the inpcb is caching !RTF_HOST routing entry.
		 * - the ICMPv6 notification is from the gateway cached in the
		 *   inpcb.  i.e. ICMPv6 notification is from nexthop gateway
		 *   the inpcb used very recently.
		 *
		 * This is to improve interaction between netbsd/openbsd
		 * redirect handling code, and inpcb route cache code.
		 * without the clause, !RTF_HOST routing entry (which carries
		 * gateway used by inpcb right before the ICMPv6 redirect)
		 * will be cached forever in unconnected inpcb.
		 *
		 * There still is a question regarding to what is TRT:
		 * - On bsdi/freebsd, RTF_HOST (cloned) routing entry will be
		 *   generated on packet output.  inpcb will always cache
		 *   RTF_HOST routing entry so there's no need for the clause
		 *   (ICMPv6 redirect will update RTF_HOST routing entry,
		 *   and inpcb is caching it already).
		 *   However, bsdi/freebsd are vulnerable to local DoS attacks
		 *   due to the cloned routing entries.
		 * - Specwise, "destination cache" is mentioned in RFC2461.
		 *   Jinmei says that it implies bsdi/freebsd behavior, itojun
		 *   is not really convinced.
		 * - Having hiwat/lowat on # of cloned host route (redirect/
		 *   pmtud) may be a good idea.  netbsd/openbsd has it.  see
		 *   icmp6_mtudisc_update().
		 */
		if ((PRC_IS_REDIRECT(cmd) || cmd == PRC_HOSTDEAD) &&
		    IN6_IS_ADDR_UNSPECIFIED(&in6p_laddr(inp)) &&
		    (rt = rtcache_validate(&inp->inp_route)) != NULL &&
		    !(rt->rt_flags & RTF_HOST)) {
			const struct sockaddr_in6 *dst6;

			dst6 = (const struct sockaddr_in6 *)
			    rtcache_getdst(&inp->inp_route);
			if (dst6 == NULL)
				;
			else if (IN6_ARE_ADDR_EQUAL(&dst6->sin6_addr,
			    &sa6_dst->sin6_addr)) {
				rtcache_unref(rt, &inp->inp_route);
				goto do_notify;
			}
		}
		rtcache_unref(rt, &inp->inp_route);

		/*
		 * If the error designates a new path MTU for a destination
		 * and the application (associated with this socket) wanted to
		 * know the value, notify. Note that we notify for all
		 * disconnected sockets if the corresponding application
		 * wanted. This is because some UDP applications keep sending
		 * sockets disconnected.
		 * XXX: should we avoid to notify the value to TCP sockets?
		 */
		if (cmd == PRC_MSGSIZE && (inp->inp_flags & IN6P_MTU) != 0 &&
		    (IN6_IS_ADDR_UNSPECIFIED(&in6p_faddr(inp)) ||
		     IN6_ARE_ADDR_EQUAL(&in6p_faddr(inp), &sa6_dst->sin6_addr))) {
			ip6_notify_pmtu(inp, (const struct sockaddr_in6 *)dst,
					(u_int32_t *)cmdarg);
		}

		/*
		 * Detect if we should notify the error. If no source and
		 * destination ports are specified, but non-zero flowinfo and
		 * local address match, notify the error. This is the case
		 * when the error is delivered with an encrypted buffer
		 * by ESP. Otherwise, just compare addresses and ports
		 * as usual.
		 */
		if (lport == 0 && fport == 0 && flowinfo &&
		    inp->inp_socket != NULL &&
		    flowinfo == (in6p_flowinfo(inp) & IPV6_FLOWLABEL_MASK) &&
		    IN6_ARE_ADDR_EQUAL(&in6p_laddr(inp), &sa6_src.sin6_addr))
			goto do_notify;
		else if (!IN6_ARE_ADDR_EQUAL(&in6p_faddr(inp),
					     &sa6_dst->sin6_addr) ||
		    inp->inp_socket == NULL ||
		    (lport && inp->inp_lport != lport) ||
		    (!IN6_IS_ADDR_UNSPECIFIED(&sa6_src.sin6_addr) &&
		     !IN6_ARE_ADDR_EQUAL(&in6p_laddr(inp),
					 &sa6_src.sin6_addr)) ||
		    (fport && inp->inp_fport != fport))
			continue;

	  do_notify:
		if (notify)
			(*notify)(inp, errno);
		nmatch++;
	}
	return nmatch;
}

void
in6pcb_purgeif0(struct inpcbtable *table, struct ifnet *ifp)
{
	struct inpcb *inp;
	struct ip6_moptions *im6o;
	struct in6_multi_mship *imm, *nimm;

	KASSERT(ifp != NULL);

	TAILQ_FOREACH(inp, &table->inpt_queue, inp_queue) {
		bool need_unlock = false;
		if (inp->inp_af != AF_INET6)
			continue;

		/* The caller holds either one of inps' lock */
		if (!inp_locked(inp)) {
			inp_lock(inp);
			need_unlock = true;
		}
		im6o = in6p_moptions(inp);
		if (im6o) {
			/*
			 * Unselect the outgoing interface if it is being
			 * detached.
			 */
			if (im6o->im6o_multicast_if_index == ifp->if_index)
				im6o->im6o_multicast_if_index = 0;

			/*
			 * Drop multicast group membership if we joined
			 * through the interface being detached.
			 * XXX controversial - is it really legal for kernel
			 * to force this?
			 */
			LIST_FOREACH_SAFE(imm, &im6o->im6o_memberships,
			    i6mm_chain, nimm) {
				if (imm->i6mm_maddr->in6m_ifp == ifp) {
					LIST_REMOVE(imm, i6mm_chain);
					in6_leavegroup(imm);
				}
			}
		}

		in_purgeifmcast(inp->inp_moptions, ifp);

		if (need_unlock)
			inp_unlock(inp);
	}
}

void
in6pcb_purgeif(struct inpcbtable *table, struct ifnet *ifp)
{
	struct rtentry *rt;
	struct inpcb *inp;

	TAILQ_FOREACH(inp, &table->inpt_queue, inp_queue) {
		if (inp->inp_af != AF_INET6)
			continue;
		if ((rt = rtcache_validate(&inp->inp_route)) != NULL &&
		    rt->rt_ifp == ifp) {
			rtcache_unref(rt, &inp->inp_route);
			in6pcb_rtchange(inp, 0);
		} else
			rtcache_unref(rt, &inp->inp_route);
	}
}

/*
 * After a routing change, flush old routing.  A new route can be
 * allocated the next time output is attempted.
 */
void
in6pcb_rtchange(struct inpcb *inp, int errno)
{
	if (inp->inp_af != AF_INET6)
		return;

	rtcache_free(&inp->inp_route);
	/*
	 * A new route can be allocated the next time
	 * output is attempted.
	 */
}

struct inpcb *
in6pcb_lookup_local(struct inpcbtable *table, struct in6_addr *laddr6, 
		   u_int lport_arg, int lookup_wildcard, struct vestigial_inpcb *vp)
{
	struct inpcbhead *head;
	struct inpcb *inp, *match = NULL;
	int matchwild = 3, wildcard;
	in_port_t lport = lport_arg;

	if (vp)
		vp->valid = 0;

	head = IN6PCBHASH_PORT(table, lport);
	LIST_FOREACH(inp, head, inp_lhash) {
		if (inp->inp_af != AF_INET6)
			continue;

		if (inp->inp_lport != lport)
			continue;
		wildcard = 0;
		if (IN6_IS_ADDR_V4MAPPED(&in6p_faddr(inp))) {
			if ((inp->inp_flags & IN6P_IPV6_V6ONLY) != 0)
				continue;
		}
		if (!IN6_IS_ADDR_UNSPECIFIED(&in6p_faddr(inp)))
			wildcard++;
		if (IN6_IS_ADDR_V4MAPPED(&in6p_laddr(inp))) {
			if ((inp->inp_flags & IN6P_IPV6_V6ONLY) != 0)
				continue;
			if (!IN6_IS_ADDR_V4MAPPED(laddr6))
				continue;

			/* duplicate of IPv4 logic */
			wildcard = 0;
			if (IN6_IS_ADDR_V4MAPPED(&in6p_faddr(inp)) &&
			    in6p_faddr(inp).s6_addr32[3])
				wildcard++;
			if (!in6p_laddr(inp).s6_addr32[3]) {
				if (laddr6->s6_addr32[3])
					wildcard++;
			} else {
				if (!laddr6->s6_addr32[3])
					wildcard++;
				else {
					if (in6p_laddr(inp).s6_addr32[3] !=
					    laddr6->s6_addr32[3])
						continue;
				}
			}
		} else if (IN6_IS_ADDR_UNSPECIFIED(&in6p_laddr(inp))) {
			if (IN6_IS_ADDR_V4MAPPED(laddr6)) {
				if ((inp->inp_flags & IN6P_IPV6_V6ONLY) != 0)
					continue;
			}
			if (!IN6_IS_ADDR_UNSPECIFIED(laddr6))
				wildcard++;
		} else {
			if (IN6_IS_ADDR_V4MAPPED(laddr6)) {
				if ((inp->inp_flags & IN6P_IPV6_V6ONLY) != 0)
					continue;
			}
			if (IN6_IS_ADDR_UNSPECIFIED(laddr6))
				wildcard++;
			else {
				if (!IN6_ARE_ADDR_EQUAL(&in6p_laddr(inp),
				    laddr6))
					continue;
			}
		}
		if (wildcard && !lookup_wildcard)
			continue;
		if (wildcard < matchwild) {
			match = inp;
			matchwild = wildcard;
			if (matchwild == 0)
				break;
		}
	}
	if (match && matchwild == 0)
		return match;

	if (vp && table->vestige && table->vestige->init_ports6) {
		struct vestigial_inpcb better;
		bool has_better = false;
		void *state;

		state = (*table->vestige->init_ports6)(laddr6,
						       lport_arg,
						       lookup_wildcard);
		while (table->vestige
		       && (*table->vestige->next_port6)(state, vp)) {

			if (vp->lport != lport)
				continue;
			wildcard = 0;
			if (!IN6_IS_ADDR_UNSPECIFIED(&vp->faddr.v6))
				wildcard++;
			if (IN6_IS_ADDR_UNSPECIFIED(&vp->laddr.v6)) {
				if (!IN6_IS_ADDR_UNSPECIFIED(laddr6))
					wildcard++;
			} else {
				if (IN6_IS_ADDR_V4MAPPED(laddr6)) {
					if (vp->v6only)
						continue;
				}
				if (IN6_IS_ADDR_UNSPECIFIED(laddr6))
					wildcard++;
				else {
					if (!IN6_ARE_ADDR_EQUAL(&vp->laddr.v6, laddr6))
						continue;
				}
			}
			if (wildcard && !lookup_wildcard)
				continue;
			if (wildcard < matchwild) {
				better = *vp;
				has_better = true;

				matchwild = wildcard;
				if (matchwild == 0)
					break;
			}
		}

		if (has_better) {
			*vp = better;
			return 0;
		}
	}
	return match;
}

/*
 * WARNING: return value (rtentry) could be IPv4 one if inpcb is connected to
 * IPv4 mapped address.
 */
struct rtentry *
in6pcb_rtentry(struct inpcb *inp)
{
	struct rtentry *rt;
	struct route *ro;
	union {
		const struct sockaddr *sa;
		const struct sockaddr_in6 *sa6;
#ifdef INET
		const struct sockaddr_in *sa4;
#endif
	} cdst;

	ro = &inp->inp_route;

	if (inp->inp_af != AF_INET6)
		return NULL;

	cdst.sa = rtcache_getdst(ro);
	if (cdst.sa == NULL)
		;
#ifdef INET
	else if (cdst.sa->sa_family == AF_INET) {
		KASSERT(IN6_IS_ADDR_V4MAPPED(&in6p_faddr(inp)));
		if (cdst.sa4->sin_addr.s_addr != in6p_faddr(inp).s6_addr32[3])
			rtcache_free(ro);
	}
#endif
	else {
		if (!IN6_ARE_ADDR_EQUAL(&cdst.sa6->sin6_addr,
					&in6p_faddr(inp)))
			rtcache_free(ro);
	}
	if ((rt = rtcache_validate(ro)) == NULL)
		rt = rtcache_update(ro, 1);
#ifdef INET
	if (rt == NULL && IN6_IS_ADDR_V4MAPPED(&in6p_faddr(inp))) {
		union {
			struct sockaddr		dst;
			struct sockaddr_in	dst4;
		} u;
		struct in_addr addr;

		addr.s_addr = in6p_faddr(inp).s6_addr32[3];

		sockaddr_in_init(&u.dst4, &addr, 0);
		if (rtcache_setdst(ro, &u.dst) != 0)
			return NULL;

		rt = rtcache_init(ro);
	} else
#endif
	if (rt == NULL && !IN6_IS_ADDR_UNSPECIFIED(&in6p_faddr(inp))) {
		union {
			struct sockaddr		dst;
			struct sockaddr_in6	dst6;
		} u;

		sockaddr_in6_init(&u.dst6, &in6p_faddr(inp), 0, 0, 0);
		if (rtcache_setdst(ro, &u.dst) != 0)
			return NULL;

		rt = rtcache_init(ro);
	}
	return rt;
}

void
in6pcb_rtentry_unref(struct rtentry *rt, struct inpcb *inp)
{

	rtcache_unref(rt, &inp->inp_route);
}

struct inpcb *
in6pcb_lookup(struct inpcbtable *table, const struct in6_addr *faddr6,
		      u_int fport_arg, const struct in6_addr *laddr6, u_int lport_arg,
		      int faith,
		      struct vestigial_inpcb *vp)
{
	struct inpcbhead *head;
	struct inpcb *inp;
	in_port_t fport = fport_arg, lport = lport_arg;

	if (vp)
		vp->valid = 0;

	head = IN6PCBHASH_CONNECT(table, faddr6, fport, laddr6, lport);
	LIST_FOREACH(inp, head, inp_hash) {
		if (inp->inp_af != AF_INET6)
			continue;

		/* find exact match on both source and dest */
		if (inp->inp_fport != fport)
			continue;
		if (inp->inp_lport != lport)
			continue;
		if (IN6_IS_ADDR_UNSPECIFIED(&in6p_faddr(inp)))
			continue;
		if (!IN6_ARE_ADDR_EQUAL(&in6p_faddr(inp), faddr6))
			continue;
		if (IN6_IS_ADDR_UNSPECIFIED(&in6p_laddr(inp)))
			continue;
		if (!IN6_ARE_ADDR_EQUAL(&in6p_laddr(inp), laddr6))
			continue;
		if ((IN6_IS_ADDR_V4MAPPED(laddr6) ||
		     IN6_IS_ADDR_V4MAPPED(faddr6)) &&
		    (inp->inp_flags & IN6P_IPV6_V6ONLY))
			continue;
		return inp;
	}
	if (vp && table->vestige) {
		if ((*table->vestige->lookup6)(faddr6, fport_arg,
					       laddr6, lport_arg, vp))
			return NULL;
	}

	return NULL;
}

struct inpcb *
in6pcb_lookup_bound(struct inpcbtable *table, const struct in6_addr *laddr6, 
	u_int lport_arg, int faith)
{
	struct inpcbhead *head;
	struct inpcb *inp;
	in_port_t lport = lport_arg;
#ifdef INET
	struct in6_addr zero_mapped;
#endif

	head = IN6PCBHASH_BIND(table, laddr6, lport);
	LIST_FOREACH(inp, head, inp_hash) {
		if (inp->inp_af != AF_INET6)
			continue;

		if (faith && (inp->inp_flags & IN6P_FAITH) == 0)
			continue;
		if (inp->inp_fport != 0)
			continue;
		if (inp->inp_lport != lport)
			continue;
		if (IN6_IS_ADDR_V4MAPPED(laddr6) &&
		    (inp->inp_flags & IN6P_IPV6_V6ONLY) != 0)
			continue;
		if (IN6_ARE_ADDR_EQUAL(&in6p_laddr(inp), laddr6))
			goto out;
	}
#ifdef INET
	if (IN6_IS_ADDR_V4MAPPED(laddr6)) {
		memset(&zero_mapped, 0, sizeof(zero_mapped));
		zero_mapped.s6_addr16[5] = 0xffff;
		head = IN6PCBHASH_BIND(table, &zero_mapped, lport);
		LIST_FOREACH(inp, head, inp_hash) {
			if (inp->inp_af != AF_INET6)
				continue;

			if (faith && (inp->inp_flags & IN6P_FAITH) == 0)
				continue;
			if (inp->inp_fport != 0)
				continue;
			if (inp->inp_lport != lport)
				continue;
			if ((inp->inp_flags & IN6P_IPV6_V6ONLY) != 0)
				continue;
			if (IN6_ARE_ADDR_EQUAL(&in6p_laddr(inp), &zero_mapped))
				goto out;
		}
	}
#endif
	head = IN6PCBHASH_BIND(table, &zeroin6_addr, lport);
	LIST_FOREACH(inp, head, inp_hash) {
		if (inp->inp_af != AF_INET6)
			continue;

		if (faith && (inp->inp_flags & IN6P_FAITH) == 0)
			continue;
		if (inp->inp_fport != 0)
			continue;
		if (inp->inp_lport != lport)
			continue;
		if (IN6_IS_ADDR_V4MAPPED(laddr6) &&
		    (inp->inp_flags & IN6P_IPV6_V6ONLY) != 0)
			continue;
		if (IN6_ARE_ADDR_EQUAL(&in6p_laddr(inp), &zeroin6_addr))
			goto out;
	}
	return NULL;

out:
	if (inp != LIST_FIRST(head)) {
		LIST_REMOVE(inp, inp_hash);
		LIST_INSERT_HEAD(head, inp, inp_hash);
	}
	return inp;
}

void
in6pcb_set_state(struct inpcb *inp, int state)
{

	if (inp->inp_af != AF_INET6)
		return;

	if (inp->inp_state > INP_ATTACHED)
		LIST_REMOVE(inp, inp_hash);

	switch (state) {
	case INP_BOUND:
		LIST_INSERT_HEAD(IN6PCBHASH_BIND(inp->inp_table,
		    &in6p_laddr(inp), inp->inp_lport), inp,
		    inp_hash);
		break;
	case INP_CONNECTED:
		LIST_INSERT_HEAD(IN6PCBHASH_CONNECT(inp->inp_table,
		    &in6p_faddr(inp), inp->inp_fport,
		    &in6p_laddr(inp), inp->inp_lport), inp,
		    inp_hash);
		break;
	}

	inp->inp_state = state;
}
