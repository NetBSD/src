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

#include <sys/param.h>
#include <sys/malloc.h>
#include <sys/mbuf.h>
#include <sys/protosw.h>
#include <sys/socket.h>
#include <sys/socketvar.h>
#include <sys/errno.h>
#include <sys/stat.h>
#include <sys/systm.h>
#ifdef __NetBSD__
#include <sys/proc.h>
#endif

#include <net/if.h>
#include <net/route.h>
#include <net/if_types.h>

#include <netinet/in.h>
#include <netinet/in_var.h>
#include <netinet6/in6_systm.h>
#include <netinet6/ip6.h>
#ifdef MAPPED_ADDR_ENABLED
#include <netinet/in_pcb.h>
#endif /* MAPPED_ADDR_ENABLED */
#include <netinet6/in6_pcb.h>
#include <netinet6/ip6_var.h>
#include <netinet6/icmp6.h>
#include <netinet6/udp6.h>
#include <netinet6/udp6_var.h>

#ifdef IPSEC
#include <netinet6/ipsec.h>
#endif /*IPSEC*/

#include "faith.h"

/*
 * UDP protocol inplementation.
 * Per RFC 768, August, 1980.
 */

struct	in6pcb *udp6_last_in6pcb = &udb6;

static	int in6_mcmatch __P((struct in6pcb *, struct in6_addr *, struct ifnet *));
static	void udp6_detach __P((struct in6pcb *));
static	void udp6_notify __P((struct in6pcb *, int));

void
udp6_init()
{
	udb6.in6p_next = udb6.in6p_prev = &udb6;
}

static int
in6_mcmatch(in6p, ia6, ifp)
	struct in6pcb *in6p;
	register struct in6_addr *ia6;
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
	register struct ip6_hdr *ip6;
	register struct udphdr *uh;
	register struct in6pcb *in6p;
	struct	mbuf *opts = 0;
	int off = *offp;
	int plen, ulen;
	struct sockaddr_in6 udp_in6;

#if defined(NFAITH) && 0 < NFAITH
	if (m->m_pkthdr.rcvif) {
		if (m->m_pkthdr.rcvif->if_type == IFT_FAITH) {
			/* send icmp6 host unreach? */
			m_freem(m);
			return IPPROTO_DONE;
		}
	}
#endif
	udp6stat.udp6s_ipackets++;

	IP6_EXTHDR_CHECK(m, off, sizeof(struct udphdr), IPPROTO_DONE);
	
	ip6 = mtod(m, struct ip6_hdr *);
	plen = ntohs(ip6->ip6_plen) - off + sizeof(*ip6);
	uh = (struct udphdr *)((caddr_t)ip6 + off);
	ulen = ntohs((u_short)uh->uh_ulen);
	
	if (plen != ulen) {
		udp6stat.udp6s_badlen++;
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
		udp_in6.sin6_addr = ip6->ip6_src;
		if (IN6_IS_SCOPE_LINKLOCAL(&udp_in6.sin6_addr))
			udp_in6.sin6_addr.s6_addr16[1] = 0;
		if (m->m_pkthdr.rcvif) {
			if (IN6_IS_SCOPE_LINKLOCAL(&udp_in6.sin6_addr)) {
				udp_in6.sin6_scope_id =
					m->m_pkthdr.rcvif->if_index;
			} else
				udp_in6.sin6_scope_id = 0;
		} else
			udp_in6.sin6_scope_id = 0;
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
			if (!IN6_IS_ADDR_ANY(&in6p->in6p_laddr)) {
				if (!IN6_ARE_ADDR_EQUAL(&in6p->in6p_laddr,
							&ip6->ip6_dst) &&
				    !in6_mcmatch(in6p, &ip6->ip6_dst,
						 m->m_pkthdr.rcvif))
					continue;
			}
			if (!IN6_IS_ADDR_ANY(&in6p->in6p_faddr)) {
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
				if (last != NULL && ipsec6_in_reject(m, last)) {
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
					if (last->in6p_flags & IN6P_CONTROLOPTS) {
						ip6_savecontrol(last, &opts,
								ip6, n);
					}

					m_adj(m, off + sizeof(struct udphdr));
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
		if (last->in6p_flags & IN6P_CONTROLOPTS)
			ip6_savecontrol(last, &opts, ip6, m);

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
			printf("UDP6: M_MCAST is set in a unicast packet.\n");
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
	udp_in6.sin6_addr = ip6->ip6_src;
	if (IN6_IS_SCOPE_LINKLOCAL(&udp_in6.sin6_addr))
		udp_in6.sin6_addr.s6_addr16[1] = 0;
	if (m->m_pkthdr.rcvif) {
		if (IN6_IS_SCOPE_LINKLOCAL(&udp_in6.sin6_addr))
			udp_in6.sin6_scope_id = m->m_pkthdr.rcvif->if_index;
		else
			udp_in6.sin6_scope_id = 0;
	} else
		udp_in6.sin6_scope_id = 0;
	if (in6p->in6p_flags & IN6P_CONTROLOPTS)
		ip6_savecontrol(in6p, &opts, ip6, m);

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

/*
 * Notify a udp user of an asynchronous error;
 * just wake up so tat he can collect error status.
 */
static	void
udp6_notify(in6p, errno)
	register struct in6pcb *in6p;
	int errno;
{
	in6p->in6p_socket->so_error = errno;
	sorwakeup(in6p->in6p_socket);
	sowwakeup(in6p->in6p_socket);
}

void
udp6_ctlinput(cmd, sa, ip6, m, off)
	int cmd;
	struct sockaddr *sa;
	register struct ip6_hdr *ip6;
	struct mbuf *m;
	int off;
{
	register struct udphdr *uhp;
	struct udphdr uh;

#if 0
	if (cmd == PRC_IFNEWADDR)
		in6_mrejoin(&udb6);
	else
#endif
	if (!PRC_IS_REDIRECT(cmd) &&
	    ((unsigned)cmd >= PRC_NCMDS || inet6ctlerrmap[cmd] == 0))
		return;
	if (ip6) {
		/*
		 * XXX: We assume that when IPV6 is non NULL,
		 * M and OFF are valid.
		 */
		if (m->m_len < off + sizeof(uh)) {
			/*
			 * this should be rare case,
			 * so we compromise on this copy...
			 */
			m_copydata(m, off, sizeof(uh), (caddr_t)&uh);
			uhp = &uh;
		} else
			uhp = (struct udphdr *)(mtod(m, caddr_t) + off);
		(void) in6_pcbnotify(&udb6, sa, uhp->uh_dport, &ip6->ip6_src,
					uhp->uh_sport, cmd, udp6_notify);
	} else {
		(void) in6_pcbnotify(&udb6, sa, 0, &zeroin6_addr, 0, cmd,
					udp6_notify);
	}
}

int
udp6_output(in6p, m, addr6, control)
	register struct in6pcb *in6p;
	register struct mbuf *m;
	struct mbuf *addr6, *control;
{
	register int ulen = m->m_pkthdr.len;
	int plen = sizeof(struct udphdr) + ulen;
	struct ip6_hdr *ip6;
	struct udphdr *udp6;
	struct	in6_addr laddr6;
	int s = 0, error = 0;
	struct ip6_pktopts opt, *stickyopt = in6p->in6p_outputopts;
	int priv = 0;
	struct proc *p = curproc;	/* XXX */

	if (p && !suser(p->p_ucred, &p->p_acflag))
		priv = 1;
	if (control) {
		if ((error = ip6_setpktoptions(control, &opt, priv)) != 0)
			goto release;
		in6p->in6p_outputopts = &opt;
	}

	if (addr6) {
		laddr6 = in6p->in6p_laddr;
		if (!IN6_IS_ADDR_ANY(&in6p->in6p_faddr)) {
			error = EISCONN;
			goto release;
		}
		/*
		 * Must block input while temporarily connected.
		 */
		s = splnet();
		error = in6_pcbconnect(in6p, addr6);
		if (error) {
			splx(s);
			goto release;
		}
	} else {
		if (IN6_IS_ADDR_ANY(&in6p->in6p_faddr)) {
			error = ENOTCONN;
			goto release;
		}
	}
	/*
	 * Calculate data length and get a mbuf
	 * for UDP and IP6 headers.
	 */
	M_PREPEND(m, sizeof(struct ip6_hdr) + sizeof(struct udphdr), M_DONTWAIT);
	if (m == 0) {
		error = ENOBUFS;
		goto release;
	}

	/*
	 * Stuff checksum and output datagram.
	 */
	ip6 = mtod(m, struct ip6_hdr *);
	ip6->ip6_flow	= in6p->in6p_flowinfo & IPV6_FLOWINFO_MASK;
	ip6->ip6_vfc 	= IPV6_VERSION;
#if 0				/* ip6_plen will be filled in ip6_output. */
	ip6->ip6_plen	= htons((u_short)plen);
#endif
	ip6->ip6_nxt	= IPPROTO_UDP;
	ip6->ip6_hlim	= in6p->in6p_ip6.ip6_hlim; /* XXX */
	ip6->ip6_src	= in6p->in6p_laddr;
	ip6->ip6_dst	= in6p->in6p_faddr;

	udp6 = (struct udphdr *)(ip6 + 1);
	udp6->uh_sport = in6p->in6p_lport;
	udp6->uh_dport = in6p->in6p_fport;
	udp6->uh_ulen  = htons((u_short)plen);
	udp6->uh_sum   = 0;

	if ((udp6->uh_sum = in6_cksum(m, IPPROTO_UDP,
					sizeof(struct ip6_hdr), plen)) == 0) {
		udp6->uh_sum = 0xffff;
	}

	udp6stat.udp6s_opackets++;

#ifdef IPSEC
	m->m_pkthdr.rcvif = (struct ifnet *)in6p->in6p_socket;
#endif /*IPSEC*/
	error = ip6_output(m, in6p->in6p_outputopts, &in6p->in6p_route,
			    0, in6p->in6p_moptions);

	if (addr6) {
		in6_pcbdisconnect(in6p);
		in6p->in6p_laddr = laddr6;
		splx(s);
	}
	goto releaseopt;

release:
	m_freem(m);

releaseopt:
	if (control) {
		in6p->in6p_outputopts = stickyopt;
		m_freem(control);
	}
	return(error);
}

#ifdef MAPPED_ADDR_ENABLED
void	udp_detach __P((struct inpcb *inp));
int	udp_usrreq __P((struct socket *so, int req, struct mbuf *m,
			struct mbuf *addr, struct mbuf *control));
int	udp_output __P((struct inpcb *, struct mbuf *, struct mbuf *,
			struct mbuf *));

/*
 * Do V4 mapped addr related binding work.
 * Return 1 when V6 bind is not necessary.
 * Return 0 when V6 bind may be necessary.
 */
static int
udp6_mapped_bind(struct socket *so, struct mbuf *addr6, int *error_p)
{
	struct	inpcb *inp = sotoinpcb(so);
	struct sockaddr_in6 *sin6_p;
	int s, unspec;

	sin6_p = mtod(addr6, struct sockaddr_in6 *);
	unspec = (IN6_IS_ADDR_UNSPECIFIED(&sin6_p->sin6_addr)) ? 1 : 0;
	if (IN6_IS_ADDR_V4MAPPED(&sin6_p->sin6_addr) || unspec) {
		
		/* Attach v4 if notyet */
		if (inp == 0) {
			*error_p = udp_usrreq(so, PRU_ATTACH, 0, 0, 0);
			if (*error_p)
			    return 1;
			inp = sotoinpcb(so);
		}
		in6_sin6_2_sin_in_m(addr6);
		s = splnet();
		*error_p = in_pcbbind(inp, addr6);
		splx(s);
		if (*error_p)
		    return 1;
		in6_sin_2_v4mapsin6_in_m(addr6);
		if (unspec) {
			sin6_p = mtod(addr6, struct sockaddr_in6 *);
			sin6_p->sin6_addr.s6_addr32[2] = 0;
			return 0;
		} else
			return 1;
	}
	return 0;
}

/*
 * Do V4 mapped addr related connect work.
 * Return 1 when V6 connect is not necessary.
 * Return 0 when V6 connect is necessary.
 */
static int
udp6_mapped_connect(struct socket *so, struct mbuf *addr6, int *error_p)
{
	struct	inpcb *inp = sotoinpcb(so);
	struct sockaddr_in6 *sin6_p;
	int s;

	sin6_p = mtod(addr6, struct sockaddr_in6 *);
	if (IN6_IS_ADDR_V4MAPPED(&sin6_p->sin6_addr)) {

		/* Attach v4 if notyet */
		if (inp == 0) {
			*error_p = udp_usrreq(so, PRU_ATTACH, 0, 0, 0);
			if (*error_p)
			    return 1;
			inp = sotoinpcb(so);
		}
		if (inp->inp_faddr.s_addr != INADDR_ANY) {
			*error_p = EISCONN;
			return 1;
		}
		in6_sin6_2_sin_in_m(addr6);
		s = splnet();
		*error_p = in_pcbconnect(inp, addr6);
		splx(s);
		if (*error_p)
		    return 1;
		else
		    soisconnected(so);
		in6_sin_2_v4mapsin6_in_m(addr6);
		return 1;
	}
	return 0;
}

/*
 * Do V4 mapped addr related disconnect work.
 * Return 1 when v4 is connected.
 * Return 0 when V4 is not connected.
 */
static int
udp6_mapped_disconnect(struct socket *so, struct mbuf *addr6, int *error_p)
{
	struct	inpcb *inp = sotoinpcb(so);
	int s;

	if (inp) {
		if (inp->inp_faddr.s_addr == INADDR_ANY)
		    return 0;
		else {
			s = splnet();
			in_pcbdisconnect(inp);
			inp->inp_laddr.s_addr = INADDR_ANY;
			splx(s);
			return 1;
		}
	} else
	    return 0;
}

/*
 * Do V4 mapped addr related sending work.
 * Return 2 when v4 attach is failed.
 * Return 1 when v6 send is not necessary.
 * Return 0 when v6 send is necessary.
 */
static int
udp6_mapped_send(struct socket *so,
		 struct mbuf *m,
		 struct mbuf *addr6,
		 struct mbuf *control,
		 int *error_p)
{
	struct	inpcb *inp = sotoinpcb(so);
	struct	in6pcb *in6p = sotoin6pcb(so);
	struct sockaddr_in6 *sin6_p = 0;
	int	hasv4addr;

	if (addr6 == 0) {
		if (!IN6_IS_ADDR_ANY(&in6p->in6p_faddr))
		    hasv4addr = 0;
		else
		    hasv4addr = (inp && 
				 inp->inp_faddr.s_addr != INADDR_ANY) ? 1 : 0;
	} else {
		sin6_p = mtod(addr6, struct sockaddr_in6 *);
		hasv4addr = IN6_IS_ADDR_V4MAPPED(&sin6_p->sin6_addr) ? 1 : 0;
	}
	if (hasv4addr) {

		/* Attach v4 if notyet */
		if (inp == 0) {
			*error_p = udp_usrreq(so, PRU_ATTACH, 0, 0, 0);
			if (*error_p)
			    return 2;
			inp = sotoinpcb(so);
		}
		if (sin6_p)
		    in6_sin6_2_sin_in_m(addr6);
		*error_p = udp_output(inp, m, addr6, control);
		if (sin6_p)
		    in6_sin_2_v4mapsin6_in_m(addr6);
		return 1;
	}
	return 0;
}

/*
 * Do V4 mapped addr related get sockaddr work.
 * Return 1 when v6 get sockaddr is not necessary.
 * Return 0 when v6 get sockaddr is necessary.
 */
int
udp6_mapped_sockaddr(struct socket *so, struct mbuf *addr6, int *error_p)
{
	struct	inpcb *inp = sotoinpcb(so);
	struct	in6pcb *in6p = sotoin6pcb(so);
	int	hasv4addr;
	
	if (inp == 0)
		return 0;
	hasv4addr = (inp->inp_laddr.s_addr == INADDR_ANY) ? 0 : 1;
	if (hasv4addr && IN6_IS_ADDR_ANY(&in6p->in6p_laddr)) {

		in_setsockaddr(inp, addr6);
		in6_sin_2_v4mapsin6_in_m(addr6);
		return 1;
	}
	return 0;
}

/*
 * Do V4 mapped addr related get peeraddr work.
 * Return 1 when v6 get peeraddr is not necessary.
 * Return 0 when v6 get peeraddr is necessary.
 */
int
udp6_mapped_peeraddr(struct socket *so, struct mbuf *addr6, int *error_p)
{
	struct	inpcb *inp = sotoinpcb(so);
	struct	in6pcb *in6p = sotoin6pcb(so);
	int	hasv4addr;
	
	if (inp == 0)
		return 0;
	hasv4addr = (inp->inp_faddr.s_addr == INADDR_ANY) ? 0 : 1;
	if (hasv4addr && IN6_IS_ADDR_ANY(&in6p->in6p_faddr)) {

		in_setpeeraddr(inp, addr6);
		in6_sin_2_v4mapsin6_in_m(addr6);
		return 1;
	}
	return 0;
}
#endif /* MAPPED_ADDR_ENABLED */

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
#ifdef MAPPED_ADDR_ENABLED
	struct	inpcb *inp = sotoinpcb(so);
	int	hasv4addr;
#endif /* MAPPED_ADDR_ENABLED */

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
		if (in6p != NULL
#ifdef MAPPED_ADDR_ENABLED
		    /* 
		     * MAPPED_ADDR implementation spec:
		     *  When PRU_ATTACH, each of in6p and inp must be NULL.
		     */
		    || inp != NULL
#endif /* MAPPED_ADDR_ENABLED */
		    ) {
			error = EINVAL;
			break;
		}
		s = splnet();
		error = in6_pcballoc(so, &udb6);
		splx(s);
		if (error)
			break;
		error = soreserve(so, udp6_sendspace, udp6_recvspace);
		if (error)
			break;
		in6p = sotoin6pcb(so);
		in6p->in6p_ip6.ip6_hlim = ip6_defhlim;
		in6p->in6p_cksum = -1;	/* just to be sure */
#ifdef IPSEC
		error = ipsec_init_policy(&in6p->in6p_sp);
#endif /*IPSEC*/
		break;

	case PRU_DETACH:
		udp6_detach(in6p);
#ifdef MAPPED_ADDR_ENABLED
		if (inp)
			udp_detach(inp);
#endif /* MAPPED_ADDR_ENABLED */
		break;

	case PRU_BIND:
#ifdef MAPPED_ADDR_ENABLED
		if (ip6_mapped_addr_on)
			if (udp6_mapped_bind(so, addr6, &error))
				break;
#endif /* MAPPED_ADDR_ENABLED */
		s = splnet();
		error = in6_pcbbind(in6p, addr6);
		splx(s);
		break;

	case PRU_LISTEN:
		error = EOPNOTSUPP;
		break;

	case PRU_CONNECT:
#ifdef MAPPED_ADDR_ENABLED
		if (ip6_mapped_addr_on)
			if (udp6_mapped_connect(so, addr6, &error))
				break;
#endif /* MAPPED_ADDR_ENABLED */
		if (!IN6_IS_ADDR_ANY(&in6p->in6p_faddr)) {
			error = EISCONN;
			break;
		}
		s = splnet();
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
#ifdef MAPPED_ADDR_ENABLED
		/* 
		 * Should only one of IPv4 or IPv6 is connected at most,
		 * but we check either and (if connected)disconnect them.
		 */
		hasv4addr = udp6_mapped_disconnect(so, addr6, &error);
#endif /* MAPPED_ADDR_ENABLED */
		if (IN6_IS_ADDR_UNSPECIFIED(&in6p->in6p_faddr)) {
#ifdef MAPPED_ADDR_ENABLED
			if (hasv4addr == 0)
				error = ENOTCONN;
			else
				so->so_state &= ~SS_ISCONNECTED; /* XXX */
#else /* MAPPED_ADDR_ENABLED */
			error = ENOTCONN;
#endif /* MAPPED_ADDR_ENABLED */
			break;
		}
		s = splnet();
		in6_pcbdisconnect(in6p);
		bzero((caddr_t)&in6p->in6p_laddr, sizeof(in6p->in6p_laddr));
		splx(s);
		so->so_state &= ~SS_ISCONNECTED;		/* XXX */
		break;

	case PRU_SHUTDOWN:
		socantsendmore(so);
		break;

	case PRU_SEND:
#ifdef MAPPED_ADDR_ENABLED
		if (ip6_mapped_addr_on) {
			int ret_val;
			
			ret_val = udp6_mapped_send(so, m, addr6, control,
						   &error);
			if (ret_val != 0) {
				if (ret_val == 1)
					return (error);
				else /* ret_val shoud be 2. v4 attach failed */
					break;
			}
			/* else proceed to udp6_output() */
		}
#endif /* MAPPED_ADDR_ENABLED */
		return(udp6_output(in6p, m, addr6, control));

	case PRU_ABORT:
		soisdisconnected(so);
#ifdef MAPPED_ADDR_ENABLED
		if (inp)
			udp_detach(inp);
#endif /* MAPPED_ADDR_ENABLED */
		udp6_detach(in6p);
		break;

	case PRU_SOCKADDR:
#ifdef MAPPED_ADDR_ENABLED
		if (ip6_mapped_addr_on)
			if (udp6_mapped_sockaddr(so, addr6, &error))
				break;
#endif /* MAPPED_ADDR_ENABLED */
		in6_setsockaddr(in6p, addr6);
		break;

	case PRU_PEERADDR:
#ifdef MAPPED_ADDR_ENABLED
		if (ip6_mapped_addr_on)
			if (udp6_mapped_peeraddr(so, addr6, &error))
				break;
#endif /* MAPPED_ADDR_ENABLED */
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
	if (control) {
		printf("udp control data unexpectedly retained\n");
		m_freem(control);
	}
	if (m)
		m_freem(m);
	return(error);
}

static void
udp6_detach(in6p)
	struct in6pcb *in6p;
{
	int	s = splnet();

	if (in6p == udp6_last_in6pcb)
		udp6_last_in6pcb = &udb6;
	in6_pcbdetach(in6p);
	splx(s);
}

#ifdef __bsdi__
int *udp6_sysvars[] = UDP6CTL_VARS;

int
udp6_sysctl(name, namelen, oldp, oldlenp, newp, newlen)
	int	*name;
	u_int	namelen;
	void	*oldp;
	size_t	*oldlenp;
	void	*newp;
	size_t	newlen;
{
	if (name[0] >= UDP6CTL_MAXID)
		return (EOPNOTSUPP);
	switch (name[0]) {
	case UDP6CTL_STATS:
		return sysctl_rdtrunc(oldp, oldlenp, newp, &udp6stat,
		    sizeof(udp6stat));
	
	default:
		return (sysctl_int_arr(udp6_sysvars, name, namelen,
		    oldp, oldlenp, newp, newlen));
	}
}
#endif /*__bsdi__*/

#ifdef __NetBSD__
#include <vm/vm.h>
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

	case UDP6CTL_SENDMAX:
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
#endif
