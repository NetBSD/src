/*	$NetBSD: udp_usrreq.c,v 1.48 1999/07/01 08:12:52 itojun Exp $	*/

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
 *	@(#)udp_usrreq.c	8.6 (Berkeley) 5/23/95
 */
#include "ipkdb.h"

/* XXX MAPPED_ADDR_ENABLED should be revisited */

#include <sys/param.h>
#include <sys/malloc.h>
#include <sys/mbuf.h>
#ifdef MAPPED_ADDR_ENABLED
#include <sys/domain.h>
#endif /* MAPPED_ADDR_ENABLED */
#include <sys/protosw.h>
#include <sys/socket.h>
#include <sys/socketvar.h>
#include <sys/errno.h>
#include <sys/stat.h>
#include <sys/systm.h>
#include <sys/proc.h>

#include <vm/vm.h>
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

#include <machine/stdarg.h>

#ifdef IPSEC
#include <netinet6/ipsec.h>
#include <netkey/key.h>
#include <netkey/key_debug.h>
#endif /*IPSEC*/

/*
 * UDP protocol implementation.
 * Per RFC 768, August, 1980.
 */
#ifndef	COMPAT_42
int	udpcksum = 1;
#else
int	udpcksum = 0;		/* XXX */
#endif

static	void udp_notify __P((struct inpcb *, int));

#ifndef UDBHASHSIZE
#define	UDBHASHSIZE	128
#endif
int	udbhashsize = UDBHASHSIZE;

void
udp_init()
{

	in_pcbinit(&udbtable, udbhashsize, udbhashsize);
}

void
#if __STDC__
udp_input(struct mbuf *m, ...)
#else
udp_input(m, va_alist)
	struct mbuf *m;
	va_dcl
#endif
{
	int proto;
	register struct ip *ip;
	register struct udphdr *uh;
	register struct inpcb *inp;
	struct mbuf *opts = 0;
	int len;
	struct ip save_ip;
	int iphlen;
	va_list ap;
	struct sockaddr_in udpsrc;
#ifdef MAPPED_ADDR_ENABLED
	struct sockaddr_in6 mapped;
#endif
	struct sockaddr *sa;

	va_start(ap, m);
	iphlen = va_arg(ap, int);
	proto = va_arg(ap, int);
	va_end(ap);

	udpstat.udps_ipackets++;

	/*
	 * Strip IP options, if any; should skip this,
	 * make available to user, and use on returned packets,
	 * but we don't yet have a way to check the checksum
	 * with options still present.
	 */
	if (iphlen > sizeof (struct ip)) {
		ip_stripoptions(m, (struct mbuf *)0);
		iphlen = sizeof(struct ip);
	}

	/*
	 * Get IP and UDP header together in first mbuf.
	 */
	ip = mtod(m, struct ip *);
	if (m->m_len < iphlen + sizeof(struct udphdr)) {
		if ((m = m_pullup(m, iphlen + sizeof(struct udphdr))) == 0) {
			udpstat.udps_hdrops++;
			return;
		}
		ip = mtod(m, struct ip *);
	}
	uh = (struct udphdr *)((caddr_t)ip + iphlen);

	/*
	 * Make mbuf data length reflect UDP length.
	 * If not enough data to reflect UDP length, drop.
	 */
	len = ntohs((u_int16_t)uh->uh_ulen);
	if (ip->ip_len != iphlen + len) {
		if (ip->ip_len < iphlen + len) {
			udpstat.udps_badlen++;
			goto bad;
		}
		m_adj(m, iphlen + len - ip->ip_len);
	}
	/*
	 * Save a copy of the IP header in case we want restore it
	 * for sending an ICMP error message in response.
	 */
	save_ip = *ip;

	/*
	 * Checksum extended UDP header and data.
	 */
	if (uh->uh_sum) {
		bzero(((struct ipovly *)ip)->ih_x1,
		    sizeof ((struct ipovly *)ip)->ih_x1);
		((struct ipovly *)ip)->ih_len = uh->uh_ulen;
		if (in_cksum(m, len + sizeof (struct ip)) != 0) {
			udpstat.udps_badsum++;
			m_freem(m);
			return;
		}
	}

	/*
	 * Construct sockaddr format source address.
	 */
	udpsrc.sin_family = AF_INET;
	udpsrc.sin_len = sizeof(struct sockaddr_in);
	udpsrc.sin_addr = ip->ip_src;
	udpsrc.sin_port = uh->uh_sport;
	bzero((caddr_t)udpsrc.sin_zero, sizeof(udpsrc.sin_zero));

	if (IN_MULTICAST(ip->ip_dst.s_addr) ||
	    in_broadcast(ip->ip_dst, m->m_pkthdr.rcvif)) {
		struct inpcb *last;
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

		iphlen += sizeof(struct udphdr);
		/*
		 * KAME note: usually we drop udpiphdr from mbuf here.
		 * we need udpiphdr for iPsec processing so we do that later.
		 */
		/*
		 * Locate pcb(s) for datagram.
		 * (Algorithm copied from raw_intr().)
		 */
		last = NULL;
		for (inp = udbtable.inpt_queue.cqh_first;
		    inp != (struct inpcb *)&udbtable.inpt_queue;
		    inp = inp->inp_queue.cqe_next) {
			if (inp->inp_lport != uh->uh_dport)
				continue;
			if (!in_nullhost(inp->inp_laddr)) {
				if (!in_hosteq(inp->inp_laddr, ip->ip_dst))
					continue;
			}
			if (!in_nullhost(inp->inp_faddr)) {
				if (!in_hosteq(inp->inp_faddr, ip->ip_src) ||
				    inp->inp_fport != uh->uh_sport)
					continue;
			}

			if (last != NULL) {
				struct mbuf *n;

#ifdef IPSEC
				/* check AH/ESP integrity. */
				if (last != NULL && ipsec4_in_reject(m, last)) {
					ipsecstat.in_polvio++;
					/* do not inject data to pcb */
				} else
#endif /*IPSEC*/
				if ((n = m_copy(m, 0, M_COPYALL)) != NULL) {
					m_adj(m, iphlen);
					if (last->inp_flags & INP_CONTROLOPTS
					    || last->inp_socket->so_options &
					       SO_TIMESTAMP) {
						ip_savecontrol(last, &opts,
						    ip, n);
					}
					sa = (struct sockaddr *)&udpsrc;
#ifdef MAPPED_ADDR_ENABLED
					if (last->inp_socket->so_proto->pr_domain->dom_family == AF_INET6) {
						in6_sin_2_v4mapsin6(&udpsrc,
								    &mapped);
						sa = (struct sockaddr *)&mapped;
					}
#endif /* MAPPED_ADDR_ENABLED */
					if (sbappendaddr(
					    &last->inp_socket->so_rcv,
					    sa, n, opts) == 0) {
						m_freem(n);
						if (opts)
							m_freem(opts);
					} else
						sorwakeup(last->inp_socket);
					opts = 0;
				}
			}
			last = inp;
			/*
			 * Don't look for additional matches if this one does
			 * not have either the SO_REUSEPORT or SO_REUSEADDR
			 * socket options set.  This heuristic avoids searching
			 * through all pcbs in the common case of a non-shared
			 * port.  It * assumes that an application will never
			 * clear these options after setting them.
			 */
			if ((last->inp_socket->so_options &
			    (SO_REUSEPORT|SO_REUSEADDR)) == 0)
				break;
		}

		if (last == NULL) {
			/*
			 * No matching pcb found; discard datagram.
			 * (No need to send an ICMP Port Unreachable
			 * for a broadcast or multicast datgram.)
			 */
			udpstat.udps_noport++;
			udpstat.udps_noportbcast++;
			goto bad;
		}
#ifdef IPSEC
		/* check AH/ESP integrity. */
		if (last != NULL && ipsec4_in_reject(m, last)) {
			ipsecstat.in_polvio++;
			goto bad;
		}
#endif /*IPSEC*/
		if (last->inp_flags & INP_CONTROLOPTS ||
		    last->inp_socket->so_options & SO_TIMESTAMP)
			ip_savecontrol(last, &opts, ip, m);
		sa = (struct sockaddr *)&udpsrc;
#ifdef MAPPED_ADDR_ENABLED
		if (last->inp_socket->so_proto->pr_domain->dom_family ==
		    AF_INET6) {
			in6_sin_2_v4mapsin6(&udpsrc, &mapped);
			sa = (struct sockaddr *)&mapped;
		}
#endif /* MAPPED_ADDR_ENABLED */
		if (sbappendaddr(&last->inp_socket->so_rcv, sa, m, opts) == 0) {
			udpstat.udps_fullsock++;
			goto bad;
		}
		sorwakeup(last->inp_socket);
		return;
	}
	/*
	 * Locate pcb for datagram.
	 */
	inp = in_pcblookup_connect(&udbtable, ip->ip_src, uh->uh_sport,
	    ip->ip_dst, uh->uh_dport);
	if (inp == 0) {
		++udpstat.udps_pcbhashmiss;
		inp = in_pcblookup_bind(&udbtable, ip->ip_dst, uh->uh_dport);
		if (inp == 0) {
			udpstat.udps_noport++;
			if (m->m_flags & (M_BCAST | M_MCAST)) {
				udpstat.udps_noportbcast++;
				goto bad;
			}
			*ip = save_ip;
#if NIPKDB > 0
			if (checkipkdb(&ip->ip_src,
				       uh->uh_sport,
				       uh->uh_dport,
				       m,
				       iphlen + sizeof(struct udphdr),
				       len - sizeof(struct udphdr)))
			/* It was a debugger connect packet, just drop it now */
				goto bad;
#endif
			icmp_error(m, ICMP_UNREACH, ICMP_UNREACH_PORT, 0, 0);
			return;
		}
	}
#ifdef IPSEC
	if (inp != NULL && ipsec4_in_reject(m, inp)) {
		ipsecstat.in_polvio++;
		goto bad;
	}
#endif /*IPSEC*/

	/*
	 * Stuff source address and datagram in user buffer.
	 */
	if (inp->inp_flags & INP_CONTROLOPTS ||
	    inp->inp_socket->so_options & SO_TIMESTAMP)
		ip_savecontrol(inp, &opts, ip, m);
	iphlen += sizeof(struct udphdr);
	m->m_len -= iphlen;
	m->m_pkthdr.len -= iphlen;
	m->m_data += iphlen;
	sa = (struct sockaddr *)&udpsrc;
#ifdef MAPPED_ADDR_ENABLED
	if (inp->inp_socket->so_proto->pr_domain->dom_family == AF_INET6) {
		in6_sin_2_v4mapsin6(&udpsrc, &mapped);
		sa = (struct sockaddr *)&mapped;
	}
#endif /* MAPPED_ADDR_ENABLED */
	if (sbappendaddr(&inp->inp_socket->so_rcv, sa, m, opts) == 0) {
		udpstat.udps_fullsock++;
		goto bad;
	}
	sorwakeup(inp->inp_socket);
	return;
bad:
	m_freem(m);
	if (opts)
		m_freem(opts);
}

/*
 * Notify a udp user of an asynchronous error;
 * just wake up so that he can collect error status.
 */
static void
udp_notify(inp, errno)
	register struct inpcb *inp;
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
	register struct ip *ip = v;
	register struct udphdr *uh;
	extern int inetctlerrmap[];
	void (*notify) __P((struct inpcb *, int)) = udp_notify;
	int errno;

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
	register struct inpcb *inp;
	register struct udpiphdr *ui;
	register int len = m->m_pkthdr.len;
	int error = 0;
	va_list ap;

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
	if ((len + sizeof(struct udpiphdr)) > IP_MAXPACKET) {
		error = EMSGSIZE;
		goto release;
	}

	/*
	 * Fill in mbuf with extended UDP header
	 * and addresses and length put into network format.
	 */
	ui = mtod(m, struct udpiphdr *);
	bzero(ui->ui_x1, sizeof ui->ui_x1);
	ui->ui_pr = IPPROTO_UDP;
	ui->ui_len = htons((u_int16_t)len + sizeof (struct udphdr));
	ui->ui_src = inp->inp_laddr;
	ui->ui_dst = inp->inp_faddr;
	ui->ui_sport = inp->inp_lport;
	ui->ui_dport = inp->inp_fport;
	ui->ui_ulen = ui->ui_len;

	/*
	 * Stuff checksum and output datagram.
	 */
	ui->ui_sum = 0;
	if (udpcksum) {
	    if ((ui->ui_sum = in_cksum(m, sizeof (struct udpiphdr) + len)) == 0)
		ui->ui_sum = 0xffff;
	}
	((struct ip *)ui)->ip_len = sizeof (struct udpiphdr) + len;
	((struct ip *)ui)->ip_ttl = inp->inp_ip.ip_ttl;	/* XXX */
	((struct ip *)ui)->ip_tos = inp->inp_ip.ip_tos;	/* XXX */
	udpstat.udps_opackets++;

#ifdef IPSEC
	m->m_pkthdr.rcvif = (struct ifnet *)inp->inp_socket;
#endif /*IPSEC*/

	return (ip_output(m, inp->inp_options, &inp->inp_route,
	    inp->inp_socket->so_options & (SO_DONTROUTE | SO_BROADCAST),
	    inp->inp_moptions));

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
	register struct inpcb *inp;
	int s;
	register int error = 0;

	if (req == PRU_CONTROL)
		return (in_control(so, (long)m, (caddr_t)nam,
		    (struct ifnet *)control, p));

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
#ifdef IPSEC
		inp = (struct inpcb *)so->so_pcb;
		error = ipsec_init_policy(&inp->inp_sp);
#endif /*IPSEC*/
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
			if (error) {
			die:
				m_freem(m);
				break;
			}
		} else {
			if ((so->so_state & SS_ISCONNECTED) == 0) {
				error = ENOTCONN;
				goto die;
			}
		}
		error = udp_output(m, inp);
		if (nam) {
			in_pcbdisconnect(inp);
			inp->inp_laddr = laddr;		/* XXX */
			in_pcbstate(inp, INP_BOUND);	/* XXX */
		}
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
