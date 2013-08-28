/*	$NetBSD: in_pcb.c,v 1.145.2.2 2013/08/28 15:21:48 rmind Exp $	*/

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
 * Copyright (c) 1998-2013 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Coyote Point Systems, Inc.
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
 * Copyright (c) 1982, 1986, 1991, 1993, 1995
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
 *	@(#)in_pcb.c	8.4 (Berkeley) 5/24/95
 */

/*
 * Internet Protocol Control Block (PCB) module.
 *
 *	Each PCB (inpcb_t) is associated with a socket during PCB creation.
 *	Its members are protected by the socket lock.  Creation is done on
 *	PRU_ATTACH protocol command and destruction on PRU_DETACH.
 *
 * Synchronisation
 *
 *	PCBs are inserted into a PCB table (inpcbtable_t).  The hash and
 *	the lists of the table are protected by the inpcbtable_t::inpt_lock.
 *	There are two main PCB lookup points, which can occur either from
 *	the top or the bottom of the stack:
 *
 *	- Process performs a protocol operation (e.g. PRU_SEND) and gets
 *	  PCB from the socket, i.e. sotoinpcb(9).
 *	- When a packet arrives (e.g. UDP datagram), the protocol layer
 *	  performs 4-tuple a PCB lookup to find an associated socket.
 *
 *	In addition to this, there are cases when multiple PCBs are matched
 *	and processed (e.g. raw IP or UDP multicast).
 *
 * Lock order, XXXrmind: NOT YET
 *
 *	inpcbtable_t::inpt_lock ->
 *		struct socket::so_lock
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: in_pcb.c,v 1.145.2.2 2013/08/28 15:21:48 rmind Exp $");

#include "opt_inet.h"
#include "opt_ipsec.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/mbuf.h>
#include <sys/protosw.h>
#include <sys/socket.h>
#include <sys/socketvar.h>
#include <sys/ioctl.h>
#include <sys/once.h>
#include <sys/pool.h>
#include <sys/kmem.h>
#include <sys/kauth.h>
#include <sys/uidinfo.h>
#include <sys/domain.h>
#include <sys/sysctl.h>

#include <net/if.h>
#include <net/route.h>

#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#define __INPCB_PRIVATE
#include <netinet/in_pcb.h>
#include <netinet/in_var.h>
#include <netinet/ip_var.h>
#include <netinet/portalgo.h>

#ifdef INET6
#include <netinet/ip6.h>
#include <netinet6/ip6_var.h>
#include <netinet6/in6_pcb.h>
#endif

#ifdef IPSEC
#include <netipsec/ipsec.h>
#include <netipsec/key.h>
#endif

#include <netinet/tcp_vtw.h>

static pool_cache_t	inpcb_cache __read_mostly;

struct in_addr		zeroin_addr;

int	anonportmin = IPPORT_ANONMIN;
int	anonportmax = IPPORT_ANONMAX;
int	lowportmin  = IPPORT_RESERVEDMIN;
int	lowportmax  = IPPORT_RESERVEDMAX;

#define	INPCBHASH_PORT(table, lport) \
	&(table)->inpt_porthashtbl[ntohs(lport) & (table)->inpt_porthash]
#define	INPCBHASH_BIND(table, laddr, lport) \
	&(table)->inpt_bindhashtbl[ \
	    ((ntohl((laddr).s_addr) + ntohs(lport))) & (table)->inpt_bindhash]
#define	INPCBHASH_CONNECT(table, faddr, fport, laddr, lport) \
	&(table)->inpt_connecthashtbl[ \
	    ((ntohl((faddr).s_addr) + ntohs(fport)) + \
	     (ntohl((laddr).s_addr) + ntohs(lport))) & (table)->inpt_connecthash]

static int
inpcb_poolinit(void)
{
	inpcb_cache = pool_cache_init(sizeof(inpcb_t), coherency_unit,
	    0, 0, "inpcbpl", NULL, IPL_NET, NULL, NULL, NULL);
	return 0;
}

inpcbtable_t *
inpcb_init(size_t bindhashsize, size_t connecthashsize, int flags)
{
	static ONCE_DECL(control);
	inpcbtable_t *inpt;

	RUN_ONCE(&control, inpcb_poolinit);

	inpt = kmem_zalloc(sizeof(inpcbtable_t), KM_SLEEP);

	mutex_init(&inpt->inpt_lock, MUTEX_DEFAULT, IPL_SOFTNET);
	CIRCLEQ_INIT(&inpt->inpt_queue);
	inpt->inpt_flags = flags;

	inpt->inpt_porthashtbl = hashinit(bindhashsize, HASH_LIST, true,
	    &inpt->inpt_porthash);
	inpt->inpt_bindhashtbl = hashinit(bindhashsize, HASH_LIST, true,
	    &inpt->inpt_bindhash);
	inpt->inpt_connecthashtbl = hashinit(connecthashsize, HASH_LIST, true,
	    &inpt->inpt_connecthash);
	inpt->inpt_lastlow = IPPORT_RESERVEDMAX;
	inpt->inpt_lastport = (in_port_t)anonportmax;

	return inpt;
}

/*
 * inpcb_create: construct a new PCB and associated with a given socket.
 * Sets the PCB state to INP_ATTACHED and makes PCB globally visible.
 */
int
inpcb_create(struct socket *so, inpcbtable_t *inpt)
{
	inpcb_t *inp;
	inpcbhead_t *head;

	inp = pool_cache_get(inpcb_cache, PR_NOWAIT);
	if (inp == NULL) {
		return ENOBUFS;
	}
	memset(inp, 0, sizeof(*inp));

	inp->inp_af = AF_INET;
	inp->inp_table = inpt;
	inp->inp_socket = so;
	inp->inp_errormtu = -1;
	inp->inp_portalgo = PORTALGO_DEFAULT;
	inp->inp_bindportonsend = false;
#if defined(IPSEC)
	int error = ipsec_init_pcbpolicy(so, &inp->inp_sp);
	if (error != 0) {
		pool_cache_put(inpcb_cache, inp);
		return error;
	}
#endif
	so->so_pcb = inp;
	inpcb_set_state(inp, INP_ATTACHED);

	head = INPCBHASH_PORT(inpt, inp->inp_lport);
	CIRCLEQ_INSERT_HEAD(&inpt->inpt_queue, &inp->inp_head, inph_queue);
	LIST_INSERT_HEAD(head, &inp->inp_head, inph_lhash);

	return 0;
}

static int
in_pcbsetport(struct sockaddr_in *sin, inpcb_t *inp, kauth_cred_t cred)
{
	inpcbtable_t *inpt = inp->inp_table;
	struct socket *so = inp->inp_socket;
	in_port_t *lastport, lport = 0;
	enum kauth_network_req req;
	int error;

	if (inp->inp_flags & INP_LOWPORT) {
#ifndef IPNOPRIVPORTS
		req = KAUTH_REQ_NETWORK_BIND_PRIVPORT;
#else
		req = KAUTH_REQ_NETWORK_BIND_PORT;
#endif
		lastport = &inpt->inpt_lastlow;
	} else {
		req = KAUTH_REQ_NETWORK_BIND_PORT;
		lastport = &inpt->inpt_lastport;
	}

	/* XXX-kauth: KAUTH_REQ_NETWORK_BIND_AUTOASSIGN_{,PRIV}PORT */
	error = kauth_authorize_network(cred, KAUTH_NETWORK_BIND, req, so, sin,
	    NULL);
	if (error)
		return EACCES;

	/*
	 * Use RFC 6056 randomized port selection.
	 */
	error = portalgo_randport(&lport, &inp->inp_head, cred);
	if (error)
		return error;

	inp->inp_flags |= INP_ANONPORT;
	*lastport = lport;
	lport = htons(lport);
	inp->inp_lport = lport;

	return 0;
}

static int
inpcb_bind_addr(inpcb_t *inp, struct sockaddr_in *sin, kauth_cred_t cred)
{
	if (sin->sin_family != AF_INET) {
		return EAFNOSUPPORT;
	}

	if (IN_MULTICAST(sin->sin_addr.s_addr)) {
		/*
		 * Always succeed; port reuse handled in inpcb_bind_port().
		 */
	} else if (!in_nullhost(sin->sin_addr)) {
		struct in_ifaddr *ia = NULL;

		INADDR_TO_IA(sin->sin_addr, ia);
		/* Check for broadcast addresses. */
		if (ia == NULL) {
			ia = ifatoia(ifa_ifwithaddr(sintosa(sin)));
		}
		if (ia == NULL) {
			return EADDRNOTAVAIL;
		}
	}

	inp->inp_laddr = sin->sin_addr;
	return 0;
}

static int
inpcb_bind_port(inpcb_t *inp, struct sockaddr_in *sin, kauth_cred_t cred)
{
	inpcbtable_t *inpt = inp->inp_table;
	struct socket *so = inp->inp_socket;
	int reuseport = (so->so_options & SO_REUSEPORT);
	int wild = 0, error;

	if (IN_MULTICAST(sin->sin_addr.s_addr)) {
		/*
		 * Treat SO_REUSEADDR as SO_REUSEPORT for multicast;
		 * allow complete duplication of binding if
		 * SO_REUSEPORT is set, or if SO_REUSEADDR is set
		 * and a multicast address is bound on both
		 * new and duplicated sockets.
		 */
		if (so->so_options & SO_REUSEADDR)
			reuseport = SO_REUSEADDR|SO_REUSEPORT;
	}

	if (sin->sin_port == 0) {
		error = in_pcbsetport(sin, inp, cred);
		if (error)
			return error;
	} else {
		inpcb_t *t;
		vestigial_inpcb_t vestige;
#ifdef INET6
		struct in6pcb *t6;
		struct in6_addr mapped;
#endif
		enum kauth_network_req req;

		if ((so->so_options & (SO_REUSEADDR|SO_REUSEPORT)) == 0)
			wild = 1;

#ifndef IPNOPRIVPORTS
		if (ntohs(sin->sin_port) < IPPORT_RESERVED)
			req = KAUTH_REQ_NETWORK_BIND_PRIVPORT;
		else
#endif /* !IPNOPRIVPORTS */
			req = KAUTH_REQ_NETWORK_BIND_PORT;

		error = kauth_authorize_network(cred, KAUTH_NETWORK_BIND, req,
		    so, sin, NULL);
		if (error)
			return (EACCES);

#ifdef INET6
		memset(&mapped, 0, sizeof(mapped));
		mapped.s6_addr16[5] = 0xffff;
		memcpy(&mapped.s6_addr32[3], &sin->sin_addr,
		    sizeof(mapped.s6_addr32[3]));
		t6 = in6_pcblookup_port(inpt, &mapped, sin->sin_port, wild, &vestige);
		if (t6 && (reuseport & t6->in6p_socket->so_options) == 0)
			return (EADDRINUSE);
		if (!t6 && vestige.valid) {
		    if (!!reuseport != !!vestige.reuse_port) {
			return EADDRINUSE;
		    }
		}
#endif

		/* XXX-kauth */
		if (so->so_uidinfo->ui_uid && !IN_MULTICAST(sin->sin_addr.s_addr)) {
			t = inpcb_lookup_port(inpt, sin->sin_addr, sin->sin_port, 1, &vestige);
			/*
			 * XXX:	investigate ramifications of loosening this
			 *	restriction so that as long as both ports have
			 *	SO_REUSEPORT allow the bind
			 */
			if (t &&
			    (!in_nullhost(sin->sin_addr) ||
			     !in_nullhost(t->inp_laddr) ||
			     (t->inp_socket->so_options & SO_REUSEPORT) == 0)
			    && (so->so_uidinfo->ui_uid != t->inp_socket->so_uidinfo->ui_uid)) {
				return (EADDRINUSE);
			}
			if (!t && vestige.valid) {
				if ((!in_nullhost(sin->sin_addr)
				     || !in_nullhost(vestige.laddr.v4)
				     || !vestige.reuse_port)
				    && so->so_uidinfo->ui_uid != vestige.uid) {
					return EADDRINUSE;
				}
			}
		}
		t = inpcb_lookup_port(inpt, sin->sin_addr, sin->sin_port, wild, &vestige);
		if (t && (reuseport & t->inp_socket->so_options) == 0)
			return (EADDRINUSE);
		if (!t
		    && vestige.valid
		    && !(reuseport && vestige.reuse_port))
			return EADDRINUSE;

		inp->inp_lport = sin->sin_port;
	}

	inpcb_set_state(inp, INP_BOUND);
	LIST_REMOVE(&inp->inp_head, inph_lhash);
	LIST_INSERT_HEAD(INPCBHASH_PORT(inpt, inp->inp_lport), &inp->inp_head,
	    inph_lhash);

	return 0;
}

/*
 * inpcb_bind: assign a local IP address and port number to the PCB.
 *
 * If the address is not a wildcard, verify that it corresponds to a
 * local interface.  If a port is specified and it is privileged, then
 * check the permission.  Check whether the address or port is in use,
 * and if so, whether we can re-use them.
 */
int
inpcb_bind(inpcb_t *inp, struct mbuf *nam, struct lwp *l)
{
	struct sockaddr_in *sin;
	struct sockaddr_in lsin;
	int error;

	if (inp->inp_af != AF_INET)
		return EINVAL;

	if (TAILQ_EMPTY(&in_ifaddrhead))
		return EADDRNOTAVAIL;
	if (inp->inp_lport || !in_nullhost(inp->inp_laddr))
		return EINVAL;

	if (nam != NULL) {
		sin = mtod(nam, struct sockaddr_in *);
		if (nam->m_len != sizeof (*sin))
			return EINVAL;
	} else {
		lsin = *((const struct sockaddr_in *)
		    inp->inp_socket->so_proto->pr_domain->dom_sa_any);
		sin = &lsin;
	}

	/* Bind address. */
	error = inpcb_bind_addr(inp, sin, l->l_cred);
	if (error)
		return error;

	/* Bind port. */
	error = inpcb_bind_port(inp, sin, l->l_cred);
	if (error) {
		inp->inp_laddr.s_addr = INADDR_ANY;
		return error;
	}

	return 0;
}

/*
 * inpcb_connect: connect from a socket to a specified address i.e.
 * assign a foreign IP address and port number to the PCB.
 *
 * Both address and port must be specified in the name argument.
 * If there is no local address for this socket yet, then pick one.
 */
int
inpcb_connect(inpcb_t *inp, struct mbuf *nam, struct lwp *l)
{
	struct in_ifaddr *ia = NULL;
	struct sockaddr_in *ifaddr = NULL;
	struct sockaddr_in *sin = mtod(nam, struct sockaddr_in *);
	struct socket *so = inp->inp_socket;
	vestigial_inpcb_t vestige;
	int error = 0;

	if (nam->m_len != sizeof(*sin))
		return EINVAL;

	if (sin->sin_family != AF_INET)
		return EAFNOSUPPORT;

	if (sin->sin_port == 0)
		return EADDRNOTAVAIL;

	if (inp->inp_af != AF_INET) {
		error = EINVAL;
		goto out;
	}

	if (IN_MULTICAST(sin->sin_addr.s_addr) && so->so_type == SOCK_STREAM) {
		error = EADDRNOTAVAIL;
		goto out;
	}

	if (!TAILQ_EMPTY(&in_ifaddrhead)) {
		/*
		 * If the destination address is INADDR_ANY,
		 * use any local address (likely loopback).
		 * If the supplied address is INADDR_BROADCAST,
		 * use the broadcast address of an interface
		 * which supports broadcast. (loopback does not)
		 */

		if (in_nullhost(sin->sin_addr)) {
			sin->sin_addr =
			    TAILQ_FIRST(&in_ifaddrhead)->ia_addr.sin_addr;
		} else if (sin->sin_addr.s_addr == INADDR_BROADCAST) {
			TAILQ_FOREACH(ia, &in_ifaddrhead, ia_list) {
				if (ia->ia_ifp->if_flags & IFF_BROADCAST) {
					sin->sin_addr =
					    ia->ia_broadaddr.sin_addr;
					break;
				}
			}
		}
	}

	/*
	 * If we have not bound which network number to use as ours, we
	 * will use the number of the outgoing interface.  This depends
	 * on having done a routing lookup, which we will probably have
	 * to do anyway, so we might as well do it now.  On the other
	 * hand if we are sending to multiple destinations we may have
	 * already done the lookup, so see if we can use the route from
	 * before.  In any case, we only chose a port number once, even
	 * if sending to multiple destinations.
	 */
	if (in_nullhost(inp->inp_laddr)) {
		int xerror;
		ifaddr = in_selectsrc(sin, &inp->inp_route,
		    so->so_options, inp->inp_moptions, &xerror);
		if (ifaddr == NULL) {
			if (xerror == 0)
				xerror = EADDRNOTAVAIL;
			return xerror;
		}
		INADDR_TO_IA(ifaddr->sin_addr, ia);
		if (ia == NULL)
			return (EADDRNOTAVAIL);
	}
	if (inpcb_lookup_connect(inp->inp_table, sin->sin_addr, sin->sin_port,
	    !in_nullhost(inp->inp_laddr) ? inp->inp_laddr : ifaddr->sin_addr,
	    inp->inp_lport, &vestige) != 0 || vestige.valid)
		return (EADDRINUSE);
	if (in_nullhost(inp->inp_laddr)) {
		if (inp->inp_lport == 0) {
			error = inpcb_bind(inp, NULL, l);
			/*
			 * This used to ignore the return value completely,
			 * but we need to check for ephemeral port shortage.
			 * And attempts to request low ports if not root.
			 */
			if (error != 0)
				return (error);
		}
		inp->inp_laddr = ifaddr->sin_addr;
	}
	inp->inp_faddr = sin->sin_addr;
	inp->inp_fport = sin->sin_port;

	/* Late bind, if needed. */
	if (inp->inp_bindportonsend) {
		struct sockaddr_in lsin = *((const struct sockaddr_in *)
		    so->so_proto->pr_domain->dom_sa_any);
		lsin.sin_addr = inp->inp_laddr;
		lsin.sin_port = 0;

		if ((error = inpcb_bind_port(inp, &lsin, l->l_cred)) != 0)
			return error;
	}

	inpcb_set_state(inp, INP_CONNECTED);
#if defined(IPSEC)
	if (so->so_type == SOCK_STREAM)
		ipsec_pcbconn(inp->inp_sp);
#endif
out:
	return error;
}

/*
 * inpcb_disconnect: remove any foreign IP/port association.
 *
 * Note: destroys the PCB if socket was closed.
 */
void
inpcb_disconnect(inpcb_t *inp)
{
	if (inp->inp_af != AF_INET)
		return;

	inp->inp_faddr = zeroin_addr;
	inp->inp_fport = 0;
	inpcb_set_state(inp, INP_BOUND);

#if defined(IPSEC)
	ipsec_pcbdisconn(inp->inp_sp);
#endif
	if (inp->inp_socket->so_state & SS_NOFDREF)
		inpcb_destroy(inp);
}

/*
 * inpcb_destroy: destroy PCB as well as the associated socket.
 */
void
inpcb_destroy(inpcb_t *inp)
{
	struct socket *so = inp->inp_socket;
	inpcbtable_t *inpt = inp->inp_table;

	if (inp->inp_af != AF_INET)
		return;

	inpcb_set_state(inp, INP_ATTACHED);
	LIST_REMOVE(&inp->inp_head, inph_lhash);
	CIRCLEQ_REMOVE(&inpt->inpt_queue, &inp->inp_head, inph_queue);

#if defined(IPSEC)
	ipsec4_delete_pcbpolicy(inp);
#endif
	so->so_pcb = NULL;
	if (inp->inp_options) {
		(void)m_free(inp->inp_options);
	}
	rtcache_free(&inp->inp_route);
	ip_freemoptions(inp->inp_moptions);

	/* Note: drops the socket lock. */
	sofree(so);
	pool_cache_put(inpcb_cache, inp);

	if ((inpt->inpt_flags & INPT_MPSAFE) == 0) {
		/* Reacquire the softnet_lock */
		mutex_enter(softnet_lock);
	}
}

/*
 * inpcb_fetch_sockaddr: fetch the local IP address and port number.
 */
void
inpcb_fetch_sockaddr(inpcb_t *inp, struct mbuf *nam)
{
	struct sockaddr_in *sin;

	if (inp->inp_af != AF_INET)
		return;

	sin = mtod(nam, struct sockaddr_in *);
	sockaddr_in_init(sin, &inp->inp_laddr, inp->inp_lport);
	nam->m_len = sin->sin_len;
}

/*
 * inpcb_fetch_peeraddr: fetch the foreign IP address and port number.
 */
void
inpcb_fetch_peeraddr(inpcb_t *inp, struct mbuf *nam)
{
	struct sockaddr_in *sin;

	if (inp->inp_af != AF_INET)
		return;

	sin = mtod(nam, struct sockaddr_in *);
	sockaddr_in_init(sin, &inp->inp_faddr, inp->inp_fport);
	nam->m_len = sin->sin_len;
}

/*
 * inpcb_notify: pass some notification to all connections of a protocol
 * associated with destination address.  The local address and/or port
 * numbers may be specified to limit the search.  The "usual action" will
 * be taken, depending on the command.
 *
 * The caller must filter any commands that are not interesting (e.g.,
 * no error in the map).  Call the protocol specific routine (if any) to
 * report any errors for each matching socket.
 */
int
inpcb_notify(inpcbtable_t *inpt, struct in_addr faddr, u_int fport_arg,
    struct in_addr laddr, u_int lport_arg, int errno,
    void (*notify)(inpcb_t *, int))
{
	inpcbhead_t *head;
	struct inpcb_hdr *inph;
	in_port_t fport = fport_arg, lport = lport_arg;
	int nmatch;

	if (in_nullhost(faddr) || notify == NULL)
		return 0;

	nmatch = 0;
	head = INPCBHASH_CONNECT(inpt, faddr, fport, laddr, lport);
	LIST_FOREACH(inph, head, inph_hash) {
		inpcb_t *inp = (inpcb_t *)inph;

		if (inp->inp_af != AF_INET) {
			continue;
		}
		if (in_hosteq(inp->inp_faddr, faddr) &&
		    inp->inp_fport == fport &&
		    inp->inp_lport == lport &&
		    in_hosteq(inp->inp_laddr, laddr)) {
			(*notify)(inp, errno);
			nmatch++;
		}
	}
	return nmatch;
}

void
inpcb_notifyall(inpcbtable_t *inpt, struct in_addr faddr, int errno,
    void (*notify)(inpcb_t *, int))
{
	struct inpcb_hdr *inph;

	if (in_nullhost(faddr) || notify == NULL)
		return;

	CIRCLEQ_FOREACH(inph, &inpt->inpt_queue, inph_queue) {
		inpcb_t *inp = (inpcb_t *)inph;

		if (inp->inp_af != AF_INET)
			continue;
		if (in_hosteq(inp->inp_faddr, faddr))
			(*notify)(inp, errno);
	}
}

void
inpcb_purgeif0(inpcbtable_t *inpt, ifnet_t *ifp)
{
	struct inpcb_hdr *inph;

	CIRCLEQ_FOREACH(inph, &inpt->inpt_queue, inph_queue) {
		inpcb_t *inp = (inpcb_t *)inph;
		struct ip_moptions *imo;
		int i, gap;

		if (inp->inp_af != AF_INET)
			continue;
		if ((imo = inp->inp_moptions) == NULL)
			continue;

		/*
		 * Unselect the outgoing interface if it is being detached.
		 */
		if (imo->imo_multicast_ifp == ifp)
			imo->imo_multicast_ifp = NULL;

		/*
		 * Drop multicast group membership if we joined
		 * through the interface being detached.
		 */
		for (i = 0, gap = 0; i < imo->imo_num_memberships; i++) {
			if (imo->imo_membership[i]->inm_ifp == ifp) {
				in_delmulti(imo->imo_membership[i]);
				gap++;
			} else if (gap != 0)
				imo->imo_membership[i - gap] =
				    imo->imo_membership[i];
		}
		imo->imo_num_memberships -= gap;
	}
}

void
inpcb_purgeif(inpcbtable_t *inpt, ifnet_t *ifp)
{
	struct inpcb_hdr *inph;

	CIRCLEQ_FOREACH(inph, &inpt->inpt_queue, inph_queue) {
		inpcb_t *inp = (inpcb_t *)inph;
		struct rtentry *rt;

		if (inp->inp_af != AF_INET)
			continue;
		rt = rtcache_validate(&inp->inp_route);
		if (rt && rt->rt_ifp == ifp) {
			inpcb_rtchange(inp, 0);
		}
	}
}

int
inpcb_foreach(inpcbtable_t *inpt, int af, inpcb_func_t func, void *arg)
{
	struct inpcb_hdr *inph;
	int error = 0;

	KASSERT(func != NULL);

	CIRCLEQ_FOREACH(inph, &inpt->inpt_queue, inph_queue) {
		inpcb_t *inp = (inpcb_t *)inph;

		if (inp->inp_af != af)
			continue;
		if ((error = func(inp, arg)) != 0)
			break;
	}
	return error;
}

/*
 * inpcb_losing: check for alternatives when higher level complains about
 * service problems.  For now, invalidate cached routing information.
 * If the route was created dynamically (by a redirect), time to try a
 * default gateway again.
 */
void
inpcb_losing(inpcb_t *inp)
{
	struct rtentry *rt;
	struct rt_addrinfo info;

	if (inp->inp_af != AF_INET)
		return;

	if ((rt = rtcache_validate(&inp->inp_route)) == NULL)
		return;

	memset(&info, 0, sizeof(info));
	info.rti_info[RTAX_DST] = rtcache_getdst(&inp->inp_route);
	info.rti_info[RTAX_GATEWAY] = rt->rt_gateway;
	info.rti_info[RTAX_NETMASK] = rt_mask(rt);
	rt_missmsg(RTM_LOSING, &info, rt->rt_flags, 0);
	if (rt->rt_flags & RTF_DYNAMIC) {
		(void) rtrequest(RTM_DELETE, rt_getkey(rt), rt->rt_gateway,
		    rt_mask(rt), rt->rt_flags, NULL);
	}

	/* A new route can be allocated the next time output is attempted. */
	rtcache_free(&inp->inp_route);
}

/*
 * inpcb_rtchange: after a routing change, flush old routing.
 * A new route can be allocated the next time output is attempted.
 */
void
inpcb_rtchange(inpcb_t *inp, int errno)
{
	if (inp->inp_af != AF_INET)
		return;

	rtcache_free(&inp->inp_route);

	/* XXX SHOULD NOTIFY HIGHER-LEVEL PROTOCOLS */
}

inpcb_t *
inpcb_lookup_port(inpcbtable_t *inpt, struct in_addr laddr,
    u_int lport_arg, int lookup_wildcard, vestigial_inpcb_t *vp)
{
	inpcbhead_t *head;
	struct inpcb_hdr *inph;
	inpcb_t *match = NULL;
	struct vestigial_hooks *vestige;
	in_port_t lport = lport_arg;
	int wildcard, matchwild = 3;

	if (vp)
		vp->valid = 0;

	head = INPCBHASH_PORT(inpt, lport);
	LIST_FOREACH(inph, head, inph_lhash) {
		inpcb_t * const inp = (inpcb_t *)inph;

		if (inp->inp_af != AF_INET)
			continue;
		if (inp->inp_lport != lport)
			continue;
		/*
		 * Check if PCB's foreign and local addresses match with ours.
		 * Our foreign address is considered NULL.
		 * Count the number of wildcard matches. (0 - 2)
		 *
		 *	null	null	match
		 *	A	null	wildcard match
		 *	null	B	wildcard match
		 *	A	B	non match
		 *	A	A	match
		 */
		wildcard = 0;
		if (!in_nullhost(inp->inp_faddr))
			wildcard++;
		if (in_nullhost(inp->inp_laddr)) {
			if (!in_nullhost(laddr))
				wildcard++;
		} else {
			if (in_nullhost(laddr))
				wildcard++;
			else {
				if (!in_hosteq(inp->inp_laddr, laddr))
					continue;
			}
		}
		if (wildcard && !lookup_wildcard)
			continue;
		/*
		 * Prefer an address with less wildcards.
		 */
		if (wildcard < matchwild) {
			match = inp;
			matchwild = wildcard;
			if (matchwild == 0)
				break;
		}
	}
	if (match && matchwild == 0)
		return match;

	if (vp && (vestige = inpt->inpt_vestige) != NULL) {
		void *state = (*vestige->init_ports4)(laddr, lport_arg,
		    lookup_wildcard);
		vestigial_inpcb_t better;

		while ((*vestige->next_port4)(state, vp)) {

			if (vp->lport != lport)
				continue;
			wildcard = 0;
			if (!in_nullhost(vp->faddr.v4))
				wildcard++;
			if (in_nullhost(vp->laddr.v4)) {
				if (!in_nullhost(laddr))
					wildcard++;
			} else {
				if (in_nullhost(laddr))
					wildcard++;
				else {
					if (!in_hosteq(vp->laddr.v4, laddr))
						continue;
				}
			}
			if (wildcard && !lookup_wildcard)
				continue;
			if (wildcard < matchwild) {
				better = *vp;
				match  = (void*)&better;

				matchwild = wildcard;
				if (matchwild == 0)
					break;
			}
		}

		if (match) {
			if (match != (void*)&better)
				return match;
			else {
				*vp = better;
				return 0;
			}
		}
	}

	return match;
}

#ifdef DIAGNOSTIC
int	inpcb_notifymiss = 0;
#endif

inpcb_t *
inpcb_lookup_connect(inpcbtable_t *inpt,
    struct in_addr faddr, u_int fport_arg,
    struct in_addr laddr, u_int lport_arg,
    vestigial_inpcb_t *vp)
{
	inpcbhead_t *head;
	struct inpcb_hdr *inph;
	inpcb_t *inp;
	in_port_t fport = fport_arg, lport = lport_arg;
	struct vestigial_hooks *vestige;

	if (vp)
		vp->valid = 0;

	head = INPCBHASH_CONNECT(inpt, faddr, fport, laddr, lport);
	LIST_FOREACH(inph, head, inph_hash) {
		inp = (inpcb_t *)inph;
		if (inp->inp_af != AF_INET)
			continue;

		if (in_hosteq(inp->inp_faddr, faddr) &&
		    inp->inp_fport == fport &&
		    inp->inp_lport == lport &&
		    in_hosteq(inp->inp_laddr, laddr))
			goto out;
	}
	if (vp && (vestige = inpt->inpt_vestige) != NULL) {
		if ((*vestige->lookup4)(faddr, fport_arg, laddr, lport_arg, vp))
			return 0;
	}

#ifdef DIAGNOSTIC
	if (inpcb_notifymiss) {
		printf("inpcb_lookup_connect: faddr=%08x fport=%d laddr=%08x lport=%d\n",
		    ntohl(faddr.s_addr), ntohs(fport),
		    ntohl(laddr.s_addr), ntohs(lport));
	}
#endif
	return 0;

out:
	/* Move this PCB to the head of hash chain. */
	inph = &inp->inp_head;
	if (inph != LIST_FIRST(head)) {
		LIST_REMOVE(inph, inph_hash);
		LIST_INSERT_HEAD(head, inph, inph_hash);
	}
	return inp;
}

inpcb_t *
inpcb_lookup_bind(inpcbtable_t *inpt, struct in_addr laddr, u_int lport_arg)
{
	inpcbhead_t *head;
	struct inpcb_hdr *inph;
	inpcb_t *inp;
	in_port_t lport = lport_arg;

	head = INPCBHASH_BIND(inpt, laddr, lport);
	LIST_FOREACH(inph, head, inph_hash) {
		inp = (inpcb_t *)inph;
		if (inp->inp_af != AF_INET)
			continue;

		if (inp->inp_lport == lport &&
		    in_hosteq(inp->inp_laddr, laddr))
			goto out;
	}
	head = INPCBHASH_BIND(inpt, zeroin_addr, lport);
	LIST_FOREACH(inph, head, inph_hash) {
		inp = (inpcb_t *)inph;
		if (inp->inp_af != AF_INET)
			continue;

		if (inp->inp_lport == lport &&
		    in_hosteq(inp->inp_laddr, zeroin_addr))
			goto out;
	}
#ifdef DIAGNOSTIC
	if (inpcb_notifymiss) {
		printf("inpcb_lookup_bind: laddr=%08x lport=%d\n",
		    ntohl(laddr.s_addr), ntohs(lport));
	}
#endif
	return 0;

out:
	/* Move this PCB to the head of hash chain. */
	inph = &inp->inp_head;
	if (inph != LIST_FIRST(head)) {
		LIST_REMOVE(inph, inph_hash);
		LIST_INSERT_HEAD(head, inph, inph_hash);
	}
	return inp;
}

void
inpcb_set_state(inpcb_t *inp, int state)
{

	if (inp->inp_af != AF_INET)
		return;

	if (inp->inp_state > INP_ATTACHED) {
		LIST_REMOVE(&inp->inp_head, inph_hash);
	}
	switch (state) {
	case INP_BOUND:
		LIST_INSERT_HEAD(INPCBHASH_BIND(inp->inp_table,
		    inp->inp_laddr, inp->inp_lport), &inp->inp_head,
		    inph_hash);
		break;
	case INP_CONNECTED:
		LIST_INSERT_HEAD(INPCBHASH_CONNECT(inp->inp_table,
		    inp->inp_faddr, inp->inp_fport, inp->inp_laddr,
		    inp->inp_lport), &inp->inp_head, inph_hash);
		break;
	}

	inp->inp_state = state;
}

struct rtentry *
inpcb_rtentry(inpcb_t *inp)
{
	struct route *ro;
	union {
		struct sockaddr		dst;
		struct sockaddr_in	dst4;
	} u;

	if (inp->inp_af != AF_INET)
		return NULL;

	ro = &inp->inp_route;
	sockaddr_in_init(&u.dst4, &inp->inp_faddr, 0);
	return rtcache_lookup(ro, &u.dst);
}

struct socket *
inpcb_get_socket(inpcb_t *inp)
{
	struct socket *so = inp->inp_socket;

	KASSERT(so != NULL);
	return so;
}

struct route *
inpcb_get_route(inpcb_t *inp)
{
	return &inp->inp_route;
}

void *
inpcb_get_protopcb(inpcb_t *inp)
{
	return inp->inp_ppcb;
}

void
inpcb_set_protopcb(inpcb_t *inp, void *ppcb)
{
	inp->inp_ppcb = ppcb;
}

int
inpcb_get_flags(inpcb_t *inp)
{
	return inp->inp_flags;
}

void
inpcb_set_flags(inpcb_t *inp, int flags)
{
	inp->inp_flags = flags;
}

void
inpcb_get_addrs(inpcb_t *inp, struct in_addr *laddr, struct in_addr *faddr)
{
	if (laddr)
		*laddr = inp->inp_laddr;
	if (faddr)
		*faddr = inp->inp_faddr;
}

void
inpcb_set_addrs(inpcb_t *inp, struct in_addr *laddr, struct in_addr *faddr)
{
	if (laddr)
		inp->inp_laddr = *laddr;
	if (faddr)
		inp->inp_faddr = *faddr;
}

void
inpcb_get_ports(inpcb_t *inp, in_port_t *lport, in_port_t *fport)
{
	if (lport)
		*lport = inp->inp_lport;
	if (fport)
		*fport = inp->inp_fport;
}

void
inpcb_set_ports(inpcb_t *inp, in_port_t lport, in_port_t fport)
{
	if (lport)
		inp->inp_lport = lport;
	if (fport)
		inp->inp_fport = fport;
}

void
inpcb_set_vestige(inpcbtable_t *inpt, void *v)
{
	inpt->inpt_vestige = v;
}

void *
inpcb_get_vestige(inpcbtable_t *inpt)
{
	return inpt->inpt_vestige;
}

struct inpcbpolicy *
inpcb_get_sp(inpcb_t *inp)
{
	return inp->inp_sp;
}

/*
 * XXXrmind TODO: abstract the following?
 */

struct ip *
in_getiphdr(inpcb_t *inp)
{
	return &inp->inp_ip;
}

struct mbuf *
inpcb_get_options(inpcb_t *inp)
{
	return inp->inp_options;
}

void
inpcb_set_options(inpcb_t *inp, struct mbuf *opts)
{
	if (inp->inp_options) {
		m_free(inp->inp_options);
	}
	inp->inp_options = opts;
}

struct ip_moptions *
inpcb_get_moptions(inpcb_t *inp)
{
	return inp->inp_moptions;
}

void
inpcb_set_moptions(inpcb_t *inp, struct ip_moptions *mopts)
{
	inp->inp_moptions = mopts;
}

uint8_t
inpcb_get_minttl(inpcb_t *inp)
{
	return inp->inp_ip_minttl;
}

void
inpcb_set_minttl(inpcb_t *inp, uint8_t minttl)
{
	inp->inp_ip_minttl = minttl;
}

int
inpcb_get_errormtu(inpcb_t *inp)
{
	return inp->inp_errormtu;
}

void
inpcb_set_errormtu(inpcb_t *inp, int errmtu)
{
	inp->inp_errormtu = errmtu;
}

int
inpcb_get_portalgo(inpcb_t *inp)
{
	return inp->inp_portalgo;
}

/*
 * sysctl(9) helper for the INET and INET6 PCB lists.  Handles TCP/UDP
 * and INET/INET6, as well as raw PCBs for each.
 */
int
sysctl_inpcblist(SYSCTLFN_ARGS)
{
	inpcbtable_t *pcbtbl = __UNCONST(rnode->sysctl_data);
	struct inpcb_hdr *inph;
	struct kinfo_pcb pcb;
	char *dp;
	u_int op, arg;
	size_t len, needed, elem_size, out_size;
	int error, elem_count, pf, proto, pf2;

	if (namelen != 4) {
		return EINVAL;
	}
	if (oldp != NULL) {
		len = *oldlenp;
		elem_size = name[2];
		elem_count = name[3];
		if (elem_size != sizeof(pcb))
			return EINVAL;
	} else {
		len = 0;
		elem_count = INT_MAX;
		elem_size = sizeof(pcb);
	}
	error = 0;
	dp = oldp;
	op = name[0];
	arg = name[1];
	out_size = elem_size;
	needed = 0;

	if (namelen == 1 && name[0] == CTL_QUERY) {
		return sysctl_query(SYSCTLFN_CALL(rnode));
	}
	if (name - oname != 4) {
		return EINVAL;
	}

	pf = oname[1];
	proto = oname[2];
	pf2 = (oldp != NULL) ? pf : 0;

	if ((pcbtbl->inpt_flags & INPT_MPSAFE) == 0) {
		mutex_enter(softnet_lock);
	}
	CIRCLEQ_FOREACH(inph, &pcbtbl->inpt_queue, inph_queue) {
#ifdef INET
		inpcb_t *inp = (inpcb_t *)inph;
		struct sockaddr_in *in;
#endif
#ifdef INET6
		struct in6pcb *in6p = (struct in6pcb *)inph;
		struct sockaddr_in6 *in6;
#endif
		const struct socket *so = inp->inp_socket;
		const struct tcpcb *tp;

		if (inph->inph_af != pf)
			continue;

		if (kauth_authorize_network(l->l_cred, KAUTH_NETWORK_SOCKET,
		    KAUTH_REQ_NETWORK_SOCKET_CANSEE, inph->inph_socket, NULL,
		    NULL) != 0)
			continue;

		memset(&pcb, 0, sizeof(pcb));

		pcb.ki_family = pf;
		pcb.ki_type = proto;

		switch (pf2) {
		case 0:
			/* just probing for size */
			break;
#ifdef INET
		case PF_INET:
			pcb.ki_family = so->so_proto->pr_domain->dom_family;
			pcb.ki_type = so->so_proto->pr_type;
			pcb.ki_protocol = so->so_proto->pr_protocol;
			pcb.ki_pflags = inp->inp_flags;

			pcb.ki_sostate = so->so_state;
			pcb.ki_prstate = inp->inp_state;
			if (proto == IPPROTO_TCP) {
				tp = intotcpcb(inp);
				pcb.ki_tstate = tp->t_state;
				pcb.ki_tflags = tp->t_flags;
			}

			pcb.ki_pcbaddr = PTRTOUINT64(inp);
			pcb.ki_ppcbaddr = PTRTOUINT64(inp->inp_ppcb);
			pcb.ki_sockaddr = PTRTOUINT64(so);

			pcb.ki_rcvq = so->so_rcv.sb_cc;
			pcb.ki_sndq = so->so_snd.sb_cc;

			in = satosin(&pcb.ki_src);
			in->sin_len = sizeof(*in);
			in->sin_family = pf;
			in->sin_port = inp->inp_lport;
			in->sin_addr = inp->inp_laddr;
			if (pcb.ki_prstate >= INP_CONNECTED) {
				in = satosin(&pcb.ki_dst);
				in->sin_len = sizeof(*in);
				in->sin_family = pf;
				in->sin_port = inp->inp_fport;
				in->sin_addr = inp->inp_faddr;
			}
			break;
#endif
#ifdef INET6
		case PF_INET6:
			pcb.ki_family = in6p->in6p_socket->so_proto->
			    pr_domain->dom_family;
			pcb.ki_type = in6p->in6p_socket->so_proto->pr_type;
			pcb.ki_protocol = in6p->in6p_socket->so_proto->
			    pr_protocol;
			pcb.ki_pflags = in6p->in6p_flags;

			pcb.ki_sostate = in6p->in6p_socket->so_state;
			pcb.ki_prstate = in6p->in6p_state;
			if (proto == IPPROTO_TCP) {
				tp = in6totcpcb(in6p);
				pcb.ki_tstate = tp->t_state;
				pcb.ki_tflags = tp->t_flags;
			}

			pcb.ki_pcbaddr = PTRTOUINT64(in6p);
			pcb.ki_ppcbaddr = PTRTOUINT64(in6p->in6p_ppcb);
			pcb.ki_sockaddr = PTRTOUINT64(in6p->in6p_socket);

			pcb.ki_rcvq = in6p->in6p_socket->so_rcv.sb_cc;
			pcb.ki_sndq = in6p->in6p_socket->so_snd.sb_cc;

			in6 = satosin6(&pcb.ki_src);
			in6->sin6_len = sizeof(*in6);
			in6->sin6_family = pf;
			in6->sin6_port = in6p->in6p_lport;
			in6->sin6_flowinfo = in6p->in6p_flowinfo;
			in6->sin6_addr = in6p->in6p_laddr;
			in6->sin6_scope_id = 0; /* XXX? */

			if (pcb.ki_prstate >= IN6P_CONNECTED) {
				in6 = satosin6(&pcb.ki_dst);
				in6->sin6_len = sizeof(*in6);
				in6->sin6_family = pf;
				in6->sin6_port = in6p->in6p_fport;
				in6->sin6_flowinfo = in6p->in6p_flowinfo;
				in6->sin6_addr = in6p->in6p_faddr;
				in6->sin6_scope_id = 0; /* XXX? */
			}
			break;
#endif
		}
		if (len >= elem_size && elem_count > 0) {
			error = copyout(&pcb, dp, out_size);
			if (error) {
				goto out;
			}
			dp += elem_size;
			len -= elem_size;
		}
		needed += elem_size;
		if (elem_count > 0 && elem_count != INT_MAX) {
			elem_count--;
		}
	}

	*oldlenp = needed;
	if (oldp == NULL) {
		*oldlenp += PCB_SLOP * sizeof(struct kinfo_pcb);
	}
	if ((pcbtbl->inpt_flags & INPT_MPSAFE) == 0) {
		mutex_exit(softnet_lock);
	}
out:
	return error;
}
