/*	$NetBSD: udp6_usrreq.c,v 1.40.2.2 2001/08/24 00:12:45 nathanw Exp $	*/
/*	$KAME: udp6_usrreq.c,v 1.86 2001/05/27 17:33:00 itojun Exp $	*/

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
 * Copyright (c) 1982, 1986, 1989, 1993
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
 *	@(#)udp_var.h	8.1 (Berkeley) 6/10/93
 */

#include "opt_ipsec.h"

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
#include <sys/syslog.h>

#include <net/if.h>
#include <net/route.h>
#include <net/if_types.h>

#include <netinet/in.h>
#include <netinet/in_var.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include <netinet/ip_var.h>
#include <netinet/in_pcb.h>
#include <netinet/udp.h>
#include <netinet/udp_var.h>
#include <netinet/ip6.h>
#include <netinet6/ip6_var.h>
#include <netinet6/in6_pcb.h>
#include <netinet/icmp6.h>
#include <netinet6/udp6_var.h>
#include <netinet6/ip6protosw.h>

#ifdef IPSEC
#include <netinet6/ipsec.h>
#endif /*IPSEC*/

#include "faith.h"
#if defined(NFAITH) && NFAITH > 0
#include <net/if_faith.h>
#endif

/*
 * UDP protocol inplementation.
 * Per RFC 768, August, 1980.
 */

struct	in6pcb *udp6_last_in6pcb = &udb6;

#ifdef UDP6
static	int in6_mcmatch __P((struct in6pcb *, struct in6_addr *, struct ifnet *));
#endif
static	void udp6_detach __P((struct in6pcb *));
static	void udp6_notify __P((struct in6pcb *, int));

void
udp6_init()
{
	udb6.in6p_next = udb6.in6p_prev = &udb6;
}

#ifdef UDP6
static int
in6_mcmatch(in6p, ia6, ifp)
	struct in6pcb *in6p;
	struct in6_addr *ia6;
	struct ifnet *ifp;
{
	struct ip6_moptions *im6o = in6p->in6p_moptions;
	struct in6_multi_mship *imm;

	if (im6o == NULL)
		return 0;

	for (imm = im6o->im6o_memberships.lh_first; imm != NULL;
	     imm = imm->i6mm_chain.le_next) {
		if ((ifp == NULL ||
		     imm->i6mm_maddr->in6m_ifp == ifp) &&
		    IN6_ARE_ADDR_EQUAL(&imm->i6mm_maddr->in6m_addr,
				       ia6))
			return 1;
	}
	return 0;
}

int
udp6_input(mp, offp, proto)
	struct mbuf **mp;
	int *offp, proto;
{
	struct mbuf *m = *mp;
	struct ip6_hdr *ip6;
	struct udphdr *uh;
	struct in6pcb *in6p;
	struct	mbuf *opts = 0;
	int off = *offp;
	u_int32_t plen, ulen;
	struct sockaddr_in6 udp_in6;

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
#ifndef PULLDOWN_TEST
	IP6_EXTHDR_CHECK(m, off, sizeof(struct udphdr), IPPROTO_DONE);
	uh = (struct udphdr *)((caddr_t)ip6 + off);
#else
	IP6_EXTHDR_GET(uh, struct udphdr *, m, off, sizeof(struct udphdr));
	if (uh == NULL) {
		udp6stat.udp6s_hdrops++;
		return IPPROTO_DONE;
	}
#endif
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

	if (IN6_IS_ADDR_MULTICAST(&ip6->ip6_dst)) {
		struct	in6pcb *last;

		/*
		 * Deliver a multicast datagram to all sockets
		 * for which the local and remote addresses and ports match
		 * those of the incoming datagram.  This allows more than
		 * one process to receive multicasts on the same port.
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
		 * In a case that laddr should be set to the link-local
		 * address (this happens in RIPng), the multicast address
		 * specified in the received packet does not match with
		 * laddr. To cure this situation, the matching is relaxed
		 * if the receiving interface is the same as one specified
		 * in the socket and if the destination multicast address
		 * matches one of the multicast groups specified in the socket.
		 */

		/*
		 * Construct sockaddr format source address.
		 */
		bzero(&udp_in6, sizeof(udp_in6));
		udp_in6.sin6_len = sizeof(struct sockaddr_in6);
		udp_in6.sin6_family = AF_INET6;
		udp_in6.sin6_port = uh->uh_sport;
#if 0 /*XXX inbound flowinfo */
		udp_in6.sin6_flowinfo = ip6->ip6_flow & IPV6_FLOWINFO_MASK;
#endif
		/* KAME hack: recover scopeid */
		(void)in6_recoverscope(&udp_in6, &ip6->ip6_src,
		    m->m_pkthdr.rcvif);

		/*
		 * KAME note: usually we drop udphdr from mbuf here.
		 * We need udphdr for IPsec processing so we do that later.
		 */

		/*
		 * Locate pcb(s) for datagram.
		 * (Algorithm copied from raw_intr().)
		 */
		last = NULL;
		for (in6p = udb6.in6p_next;
		     in6p != &udb6;
		     in6p = in6p->in6p_next) {
			if (in6p->in6p_lport != uh->uh_dport)
				continue;
			if (!IN6_IS_ADDR_UNSPECIFIED(&in6p->in6p_laddr)) {
				if (!IN6_ARE_ADDR_EQUAL(&in6p->in6p_laddr,
							&ip6->ip6_dst) &&
				    !in6_mcmatch(in6p, &ip6->ip6_dst,
						 m->m_pkthdr.rcvif))
					continue;
			}
			if (!IN6_IS_ADDR_UNSPECIFIED(&in6p->in6p_faddr)) {
				if (!IN6_ARE_ADDR_EQUAL(&in6p->in6p_faddr,
							&ip6->ip6_src) ||
				   in6p->in6p_fport != uh->uh_sport)
					continue;
			}

			if (last != NULL) {
				struct	mbuf *n;

#ifdef IPSEC
				/*
				 * Check AH/ESP integrity.
				 */
				if (ipsec6_in_reject(m, last)) {
					ipsec6stat.in_polvio++;
					/* do not inject data into pcb */
				} else
#endif /*IPSEC*/
				if ((n = m_copy(m, 0, M_COPYALL)) != NULL) {
					/*
					 * KAME NOTE: do not
					 * m_copy(m, offset, ...) above.
					 * sbappendaddr() expects M_PKTHDR,
					 * and m_copy() will copy M_PKTHDR
					 * only if offset is 0.
					 */
					if (last->in6p_flags & IN6P_CONTROLOPTS
					 || last->in6p_socket->so_options & SO_TIMESTAMP) {
						ip6_savecontrol(last, &opts,
								ip6, n);
					}

					m_adj(n, off + sizeof(struct udphdr));
					if (sbappendaddr(&last->in6p_socket->so_rcv,
							(struct sockaddr *)&udp_in6,
							n, opts) == 0) {
						m_freem(n);
						if (opts)
							m_freem(opts);
						udp6stat.udp6s_fullsock++;
					} else
						sorwakeup(last->in6p_socket);
					opts = 0;
				}
			}
			last = in6p;
			/*
			 * Don't look for additional matches if this one does
			 * not have either the SO_REUSEPORT or SO_REUSEADDR
			 * socket options set.  This heuristic avoids searching
			 * through all pcbs in the common case of a non-shared
			 * port.  It assumes that an application will never
			 * clear these options after setting them.
			 */
			if ((last->in6p_socket->so_options &
			     (SO_REUSEPORT|SO_REUSEADDR)) == 0)
				break;
		}

		if (last == NULL) {
			/*
			 * No matching pcb found; discard datagram.
			 * (No need to send an ICMP Port Unreachable
			 * for a broadcast or multicast datgram.)
			 */
			udp6stat.udp6s_noport++;
			udp6stat.udp6s_noportmcast++;
			goto bad;
		}
#ifdef IPSEC
		/*
		 * Check AH/ESP integrity.
		 */
		if (last != NULL && ipsec6_in_reject(m, last)) {
			ipsec6stat.in_polvio++;
			goto bad;
		}
#endif /*IPSEC*/
		if (last->in6p_flags & IN6P_CONTROLOPTS
		 || last->in6p_socket->so_options & SO_TIMESTAMP) {
			ip6_savecontrol(last, &opts, ip6, m);
		}

		m_adj(m, off + sizeof(struct udphdr));
		if (sbappendaddr(&last->in6p_socket->so_rcv,
				(struct sockaddr *)&udp_in6,
				m, opts) == 0) {
			udp6stat.udp6s_fullsock++;
			goto bad;
		}
		sorwakeup(last->in6p_socket);
		return IPPROTO_DONE;
	}
	/*
	 * Locate pcb for datagram.
	 */
	in6p = udp6_last_in6pcb;
	if (in6p->in6p_lport != uh->uh_dport ||
	   in6p->in6p_fport != uh->uh_sport ||
	   !IN6_ARE_ADDR_EQUAL(&in6p->in6p_faddr, &ip6->ip6_src) ||
	   !IN6_ARE_ADDR_EQUAL(&in6p->in6p_laddr, &ip6->ip6_dst)) {
		in6p = in6_pcblookup(&udb6,
				     &ip6->ip6_src, uh->uh_sport,
				     &ip6->ip6_dst, uh->uh_dport,
				     IN6PLOOKUP_WILDCARD);
		if (in6p)
			udp6_last_in6pcb = in6p;
		udp6stat.udp6ps_pcbcachemiss++;
	}
	if (in6p == 0) {
		udp6stat.udp6s_noport++;
		if (m->m_flags & M_MCAST) {
			udp6stat.udp6s_noportmcast++;
			goto bad;
		}
		icmp6_error(m, ICMP6_DST_UNREACH, ICMP6_DST_UNREACH_NOPORT, 0);
		return IPPROTO_DONE;
	}
#ifdef IPSEC
	/*
	 * Check AH/ESP integrity.
	 */
	if (in6p != NULL && ipsec6_in_reject(m, in6p)) {
		ipsec6stat.in_polvio++;
		goto bad;
	}
#endif /*IPSEC*/

	/*
	 * Construct sockaddr format source address.
	 * Stuff source address and datagram in user buffer.
	 */
	bzero(&udp_in6, sizeof(udp_in6));
	udp_in6.sin6_len = sizeof(struct sockaddr_in6);
	udp_in6.sin6_family = AF_INET6;
	udp_in6.sin6_port = uh->uh_sport;
	/* KAME hack: recover scopeid */
	(void)in6_recoverscope(&udp_in6, &ip6->ip6_src, m->m_pkthdr.rcvif);
	if (in6p->in6p_flags & IN6P_CONTROLOPTS
	 || in6p->in6p_socket->so_options & SO_TIMESTAMP) {
		ip6_savecontrol(in6p, &opts, ip6, m);
	}

	m_adj(m, off + sizeof(struct udphdr));
	if (sbappendaddr(&in6p->in6p_socket->so_rcv,
			(struct sockaddr *)&udp_in6,
			m, opts) == 0) {
		udp6stat.udp6s_fullsock++;
		goto bad;
	}
	sorwakeup(in6p->in6p_socket);
	return IPPROTO_DONE;
bad:
	if (m)
		m_freem(m);
	if (opts)
		m_freem(opts);
	return IPPROTO_DONE;
}
#endif

/*
 * Notify a udp user of an asynchronous error;
 * just wake up so that he can collect error status.
 */
static	void
udp6_notify(in6p, errno)
	struct in6pcb *in6p;
	int errno;
{
	in6p->in6p_socket->so_error = errno;
	sorwakeup(in6p->in6p_socket);
	sowwakeup(in6p->in6p_socket);
}

void
udp6_ctlinput(cmd, sa, d)
	int cmd;
	struct sockaddr *sa;
	void *d;
{
	struct udphdr uh;
	struct ip6_hdr *ip6;
	struct sockaddr_in6 *sa6 = (struct sockaddr_in6 *)sa;
	struct mbuf *m;
	int off;
	void *cmdarg;
	struct ip6ctlparam *ip6cp = NULL;
	const struct sockaddr_in6 *sa6_src = NULL;
	void (*notify) __P((struct in6pcb *, int)) = udp6_notify;
	struct udp_portonly {
		u_int16_t uh_sport;
		u_int16_t uh_dport;
	} *uhp;

	if (sa->sa_family != AF_INET6 ||
	    sa->sa_len != sizeof(struct sockaddr_in6))
		return;

	if ((unsigned)cmd >= PRC_NCMDS)
		return;
	if (PRC_IS_REDIRECT(cmd))
		notify = in6_rtchange, d = NULL;
	else if (cmd == PRC_HOSTDEAD)
		d = NULL;
	else if (cmd == PRC_MSGSIZE)
		; /* special code is present, see below */
	else if (inet6ctlerrmap[cmd] == 0)
		return;

	/* if the parameter is from icmp6, decode it. */
	if (d != NULL) {
		ip6cp = (struct ip6ctlparam *)d;
		m = ip6cp->ip6c_m;
		ip6 = ip6cp->ip6c_ip6;
		off = ip6cp->ip6c_off;
		cmdarg = ip6cp->ip6c_cmdarg;
		sa6_src = ip6cp->ip6c_src;
	} else {
		m = NULL;
		ip6 = NULL;
		cmdarg = NULL;
		sa6_src = &sa6_any;
	}

	if (ip6) {
		/*
		 * XXX: We assume that when IPV6 is non NULL,
		 * M and OFF are valid.
		 */

		/* check if we can safely examine src and dst ports */
		if (m->m_pkthdr.len < off + sizeof(*uhp)) {
			if (cmd == PRC_MSGSIZE)
				icmp6_mtudisc_update((struct ip6ctlparam *)d, 0);
			return;
		}

		bzero(&uh, sizeof(uh));
		m_copydata(m, off, sizeof(*uhp), (caddr_t)&uh);

		if (cmd == PRC_MSGSIZE) {
			int valid = 0;

			/*
			 * Check to see if we have a valid UDP socket
			 * corresponding to the address in the ICMPv6 message
			 * payload.
			 */
			if (in6_pcblookup_connect(&udb6, &sa6->sin6_addr,
			    uh.uh_dport, (struct in6_addr *)&sa6_src->sin6_addr,
			    uh.uh_sport, 0))
				valid++;
#if 0
			/*
			 * As the use of sendto(2) is fairly popular,
			 * we may want to allow non-connected pcb too.
			 * But it could be too weak against attacks...
			 * We should at least check if the local address (= s)
			 * is really ours.
			 */
			else if (in6_pcblookup_bind(&udb6, &sa6->sin6_addr,
						    uh.uh_dport, 0))
				valid++;
#endif

			/*
			 * Depending on the value of "valid" and routing table
			 * size (mtudisc_{hi,lo}wat), we will:
			 * - recalcurate the new MTU and create the
			 *   corresponding routing entry, or
			 * - ignore the MTU change notification.
			 */
			icmp6_mtudisc_update((struct ip6ctlparam *)d, valid);

			/*
			 * regardless of if we called icmp6_mtudisc_update(),
			 * we need to call in6_pcbnotify(), to notify path
			 * MTU change to the userland (2292bis-02), because
			 * some unconnected sockets may share the same
			 * destination and want to know the path MTU.
			 */
		}

		(void) in6_pcbnotify(&udb6, sa, uh.uh_dport,
		    (struct sockaddr *)sa6_src, uh.uh_sport, cmd, cmdarg,
		    notify);
	} else {
		(void) in6_pcbnotify(&udb6, sa, 0, (struct sockaddr *)sa6_src,
		    0, cmd, cmdarg, notify);
	}
}

extern	int udp6_sendspace;
extern	int udp6_recvspace;

int
udp6_usrreq(so, req, m, addr6, control, p)
	struct socket *so;
	int req;
	struct mbuf *m, *addr6, *control;
	struct proc *p;
{
	struct	in6pcb *in6p = sotoin6pcb(so);
	int	error = 0;
	int	s;

	/*
	 * MAPPED_ADDR implementation info:
	 *  Mapped addr support for PRU_CONTROL is not necessary.
	 *  Because typical user of PRU_CONTROL is such as ifconfig,
	 *  and they don't associate any addr to their socket.  Then
	 *  socket family is only hint about the PRU_CONTROL'ed address
	 *  family, especially when getting addrs from kernel.
	 *  So AF_INET socket need to be used to control AF_INET addrs,
	 *  and AF_INET6 socket for AF_INET6 addrs.
	 */
	if (req == PRU_CONTROL)
		return(in6_control(so, (u_long)m, (caddr_t)addr6,
				   (struct ifnet *)control, p));

	if (req == PRU_PURGEIF) {
		in6_pcbpurgeif0(&udb6, (struct ifnet *)control);
		in6_purgeif((struct ifnet *)control);
		in6_pcbpurgeif(&udb6, (struct ifnet *)control);
		return (0);
	}

	if (in6p == NULL && req != PRU_ATTACH) {
		error = EINVAL;
		goto release;
	}

	switch (req) {
	case PRU_ATTACH:
		/*
		 * MAPPED_ADDR implementation spec:
		 *  Always attach for IPv6,
		 *  and only when necessary for IPv4.
		 */
		if (in6p != NULL) {
			error = EINVAL;
			break;
		}
		s = splsoftnet();
		error = in6_pcballoc(so, &udb6);
		splx(s);
		if (error)
			break;
		error = soreserve(so, udp6_sendspace, udp6_recvspace);
		if (error)
			break;
		in6p = sotoin6pcb(so);
		in6p->in6p_cksum = -1;	/* just to be sure */
		break;

	case PRU_DETACH:
		udp6_detach(in6p);
		break;

	case PRU_BIND:
		s = splsoftnet();
		error = in6_pcbbind(in6p, addr6, p);
		splx(s);
		break;

	case PRU_LISTEN:
		error = EOPNOTSUPP;
		break;

	case PRU_CONNECT:
		if (!IN6_IS_ADDR_UNSPECIFIED(&in6p->in6p_faddr)) {
			error = EISCONN;
			break;
		}
		s = splsoftnet();
		error = in6_pcbconnect(in6p, addr6);
		if (ip6_auto_flowlabel) {
			in6p->in6p_flowinfo &= ~IPV6_FLOWLABEL_MASK;
			in6p->in6p_flowinfo |=
				(htonl(ip6_flow_seq++) & IPV6_FLOWLABEL_MASK);
		}
		splx(s);
		if (error == 0)
			soisconnected(so);
		break;

	case PRU_CONNECT2:
		error = EOPNOTSUPP;
		break;

	case PRU_ACCEPT:
		error = EOPNOTSUPP;
		break;

	case PRU_DISCONNECT:
		if (IN6_IS_ADDR_UNSPECIFIED(&in6p->in6p_faddr)) {
			error = ENOTCONN;
			break;
		}
		s = splsoftnet();
		in6_pcbdisconnect(in6p);
		bzero((caddr_t)&in6p->in6p_laddr, sizeof(in6p->in6p_laddr));
		splx(s);
		so->so_state &= ~SS_ISCONNECTED;		/* XXX */
		break;

	case PRU_SHUTDOWN:
		socantsendmore(so);
		break;

	case PRU_SEND:
		return(udp6_output(in6p, m, addr6, control, p));

	case PRU_ABORT:
		soisdisconnected(so);
		udp6_detach(in6p);
		break;

	case PRU_SOCKADDR:
		in6_setsockaddr(in6p, addr6);
		break;

	case PRU_PEERADDR:
		in6_setpeeraddr(in6p, addr6);
		break;

	case PRU_SENSE:
		/*
		 * stat: don't bother with a blocksize
		 */
		return(0);

	case PRU_SENDOOB:
	case PRU_FASTTIMO:
	case PRU_SLOWTIMO:
	case PRU_PROTORCV:
	case PRU_PROTOSEND:
		error = EOPNOTSUPP;
		break;

	case PRU_RCVD:
	case PRU_RCVOOB:
		return(EOPNOTSUPP);	/* do not free mbuf's */

	default:
		panic("udp6_usrreq");
	}

release:
	if (control)
		m_freem(control);
	if (m)
		m_freem(m);
	return(error);
}

static void
udp6_detach(in6p)
	struct in6pcb *in6p;
{
	int s = splsoftnet();

	if (in6p == udp6_last_in6pcb)
		udp6_last_in6pcb = &udb6;
	in6_pcbdetach(in6p);
	splx(s);
}

#include <uvm/uvm_extern.h>
#include <sys/sysctl.h>

int
udp6_sysctl(name, namelen, oldp, oldlenp, newp, newlen)
	int *name;
	u_int namelen;
	void *oldp;
	size_t *oldlenp;
	void *newp;
	size_t newlen;
{
	/* All sysctl names at this level are terminal. */
	if (namelen != 1)
		return ENOTDIR;

	switch (name[0]) {

	case UDP6CTL_SENDSPACE:
		return sysctl_int(oldp, oldlenp, newp, newlen,
		    &udp6_sendspace);
	case UDP6CTL_RECVSPACE:
		return sysctl_int(oldp, oldlenp, newp, newlen,
		    &udp6_recvspace);
	default:
		return ENOPROTOOPT;
	}
	/* NOTREACHED */
}
