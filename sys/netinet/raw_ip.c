/*	$NetBSD: raw_ip.c,v 1.66 2003/01/30 23:43:33 thorpej Exp $	*/

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
 * Copyright (c) 1982, 1986, 1988, 1993
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
 *	@(#)raw_ip.c	8.7 (Berkeley) 5/15/95
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: raw_ip.c,v 1.66 2003/01/30 23:43:33 thorpej Exp $");

#include "opt_ipsec.h"
#include "opt_mrouting.h"

#include <sys/param.h>
#include <sys/malloc.h>
#include <sys/mbuf.h>
#include <sys/socket.h>
#include <sys/protosw.h>
#include <sys/socketvar.h>
#include <sys/errno.h>
#include <sys/systm.h>
#include <sys/proc.h>

#include <net/if.h>
#include <net/route.h>

#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include <netinet/ip_var.h>
#include <netinet/ip_mroute.h>
#include <netinet/ip_icmp.h>
#include <netinet/in_pcb.h>
#include <netinet/in_var.h>

#include <machine/stdarg.h>

#ifdef IPSEC
#include <netinet6/ipsec.h>
#endif /*IPSEC*/

struct inpcbtable rawcbtable;

int	 rip_pcbnotify __P((struct inpcbtable *, struct in_addr,
    struct in_addr, int, int, void (*) __P((struct inpcb *, int))));
int	 rip_bind __P((struct inpcb *, struct mbuf *));
int	 rip_connect __P((struct inpcb *, struct mbuf *));
void	 rip_disconnect __P((struct inpcb *));

/*
 * Nominal space allocated to a raw ip socket.
 */
#define	RIPSNDQ		8192
#define	RIPRCVQ		8192

/*
 * Raw interface to IP protocol.
 */

/*
 * Initialize raw connection block q.
 */
void
rip_init()
{

	in_pcbinit(&rawcbtable, 1, 1);
}

/*
 * Setup generic address and protocol structures
 * for raw_input routine, then pass them along with
 * mbuf chain.
 */
void
#if __STDC__
rip_input(struct mbuf *m, ...)
#else
rip_input(m, va_alist)
	struct mbuf *m;
	va_dcl
#endif
{
	int proto;
	struct ip *ip = mtod(m, struct ip *);
	struct inpcb *inp;
	struct inpcb *last = 0;
	struct mbuf *opts = 0;
	struct sockaddr_in ripsrc;
	va_list ap;

	va_start(ap, m);
	(void)va_arg(ap, int);		/* ignore value, advance ap */
	proto = va_arg(ap, int);
	va_end(ap);

	ripsrc.sin_family = AF_INET;
	ripsrc.sin_len = sizeof(struct sockaddr_in);
	ripsrc.sin_addr = ip->ip_src;
	ripsrc.sin_port = 0;
	bzero((caddr_t)ripsrc.sin_zero, sizeof(ripsrc.sin_zero));

	/*
	 * XXX Compatibility: programs using raw IP expect ip_len
	 * XXX to have the header length subtracted, and in host order.
	 * XXX ip_off is also expected to be host order.
	 */
	ip->ip_len = ntohs(ip->ip_len) - (ip->ip_hl << 2);
	NTOHS(ip->ip_off);

	CIRCLEQ_FOREACH(inp, &rawcbtable.inpt_queue, inp_queue) {
		if (inp->inp_ip.ip_p && inp->inp_ip.ip_p != proto)
			continue;
		if (!in_nullhost(inp->inp_laddr) &&
		    !in_hosteq(inp->inp_laddr, ip->ip_dst))
			continue;
		if (!in_nullhost(inp->inp_faddr) &&
		    !in_hosteq(inp->inp_faddr, ip->ip_src))
			continue;
		if (last) {
			struct mbuf *n;

#ifdef IPSEC
			/* check AH/ESP integrity. */
			if (ipsec4_in_reject_so(m, last->inp_socket)) {
				ipsecstat.in_polvio++;
				/* do not inject data to pcb */
			} else
#endif /*IPSEC*/
			if ((n = m_copy(m, 0, (int)M_COPYALL)) != NULL) {
				if (last->inp_flags & INP_CONTROLOPTS ||
				    last->inp_socket->so_options & SO_TIMESTAMP)
					ip_savecontrol(last, &opts, ip, n);
				if (sbappendaddr(&last->inp_socket->so_rcv,
				    sintosa(&ripsrc), n, opts) == 0) {
					/* should notify about lost packet */
					m_freem(n);
					if (opts)
						m_freem(opts);
				} else
					sorwakeup(last->inp_socket);
				opts = NULL;
			}
		}
		last = inp;
	}
#ifdef IPSEC
	/* check AH/ESP integrity. */
	if (last && ipsec4_in_reject_so(m, last->inp_socket)) {
		m_freem(m);
		ipsecstat.in_polvio++;
		ipstat.ips_delivered--;
		/* do not inject data to pcb */
	} else
#endif /*IPSEC*/
	if (last) {
		if (last->inp_flags & INP_CONTROLOPTS ||
		    last->inp_socket->so_options & SO_TIMESTAMP)
			ip_savecontrol(last, &opts, ip, m);
		if (sbappendaddr(&last->inp_socket->so_rcv,
		    sintosa(&ripsrc), m, opts) == 0) {
			m_freem(m);
			if (opts)
				m_freem(opts);
		} else
			sorwakeup(last->inp_socket);
	} else {
		if (inetsw[ip_protox[ip->ip_p]].pr_input == rip_input) {
			icmp_error(m, ICMP_UNREACH, ICMP_UNREACH_PROTOCOL,
			    0, 0);
			ipstat.ips_noproto++;
			ipstat.ips_delivered--;
		} else
			m_freem(m);
	}
	return;
}

int
rip_pcbnotify(table, faddr, laddr, proto, errno, notify)
	struct inpcbtable *table;
	struct in_addr faddr, laddr;
	int proto;
	int errno;
	void (*notify) __P((struct inpcb *, int));
{
	struct inpcb *inp, *ninp;
	int nmatch;

	nmatch = 0;
	for (inp = CIRCLEQ_FIRST(&table->inpt_queue);
	    inp != (struct inpcb *)&table->inpt_queue;
	    inp = ninp) {
		ninp = inp->inp_queue.cqe_next;
		if (inp->inp_ip.ip_p && inp->inp_ip.ip_p != proto)
			continue;
		if (in_hosteq(inp->inp_faddr, faddr) &&
		    in_hosteq(inp->inp_laddr, laddr)) {
			(*notify)(inp, errno);
			nmatch++;
		}
	}

	return nmatch;
}

void *
rip_ctlinput(cmd, sa, v)
	int cmd;
	struct sockaddr *sa;
	void *v;
{
	struct ip *ip = v;
	void (*notify) __P((struct inpcb *, int)) = in_rtchange;
	int errno;

	if (sa->sa_family != AF_INET ||
	    sa->sa_len != sizeof(struct sockaddr_in))
		return NULL;
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
		rip_pcbnotify(&rawcbtable, satosin(sa)->sin_addr,
		    ip->ip_src, ip->ip_p, errno, notify);

		/* XXX mapped address case */
	} else
		in_pcbnotifyall(&rawcbtable, satosin(sa)->sin_addr, errno,
		    notify);
	return NULL;
}

/*
 * Generate IP header and pass packet to ip_output.
 * Tack on options user may have setup with control call.
 */
int
#if __STDC__
rip_output(struct mbuf *m, ...)
#else
rip_output(m, va_alist)
	struct mbuf *m;
	va_dcl
#endif
{
	struct inpcb *inp;
	struct ip *ip;
	struct mbuf *opts;
	int flags;
	va_list ap;

	va_start(ap, m);
	inp = va_arg(ap, struct inpcb *);
	va_end(ap);

	flags =
	    (inp->inp_socket->so_options & SO_DONTROUTE) | IP_ALLOWBROADCAST
	    | IP_RETURNMTU;

	/*
	 * If the user handed us a complete IP packet, use it.
	 * Otherwise, allocate an mbuf for a header and fill it in.
	 */
	if ((inp->inp_flags & INP_HDRINCL) == 0) {
		if ((m->m_pkthdr.len + sizeof(struct ip)) > IP_MAXPACKET) {
			m_freem(m);
			return (EMSGSIZE);
		}
		M_PREPEND(m, sizeof(struct ip), M_WAIT);
		ip = mtod(m, struct ip *);
		ip->ip_tos = 0;
		ip->ip_off = htons(0);
		ip->ip_p = inp->inp_ip.ip_p;
		ip->ip_len = htons(m->m_pkthdr.len);
		ip->ip_src = inp->inp_laddr;
		ip->ip_dst = inp->inp_faddr;
		ip->ip_ttl = MAXTTL;
		opts = inp->inp_options;
	} else {
		if (m->m_pkthdr.len > IP_MAXPACKET) {
			m_freem(m);
			return (EMSGSIZE);
		}
		ip = mtod(m, struct ip *);

		/*
		 * If the mbuf is read-only, we need to allocate
		 * a new mbuf for the header, since we need to
		 * modify the header.
		 */
		if (M_READONLY(m)) {
			int hlen = ip->ip_hl << 2;

			m = m_copyup(m, hlen, (max_linkhdr + 3) & ~3);
			if (m == NULL)
				return (ENOMEM);	/* XXX */
			ip = mtod(m, struct ip *);
		}

		/* XXX userland passes ip_len and ip_off in host order */
		if (m->m_pkthdr.len != ip->ip_len) {
			m_freem(m);
			return (EINVAL);
		}
		HTONS(ip->ip_len);
		HTONS(ip->ip_off);
		if (ip->ip_id == 0)
			ip->ip_id = htons(ip_id++);
		opts = NULL;
		/* XXX prevent ip_output from overwriting header fields */
		flags |= IP_RAWOUTPUT;
		ipstat.ips_rawout++;
	}
#ifdef IPSEC
	if (ipsec_setsocket(m, inp->inp_socket) != 0) {
		m_freem(m);
		return ENOBUFS;
	}
#endif /*IPSEC*/
	return (ip_output(m, opts, &inp->inp_route, flags, inp->inp_moptions,
	    &inp->inp_errormtu));
}

/*
 * Raw IP socket option processing.
 */
int
rip_ctloutput(op, so, level, optname, m)
	int op;
	struct socket *so;
	int level, optname;
	struct mbuf **m;
{
	struct inpcb *inp = sotoinpcb(so);
	int error = 0;

	if (level != IPPROTO_IP) {
		error = ENOPROTOOPT;
		if (op == PRCO_SETOPT && *m != 0)
			(void) m_free(*m);
	} else switch (op) {

	case PRCO_SETOPT:
		switch (optname) {
		case IP_HDRINCL:
			if (*m == 0 || (*m)->m_len < sizeof (int))
				error = EINVAL;
			else {
				if (*mtod(*m, int *))
					inp->inp_flags |= INP_HDRINCL;
				else
					inp->inp_flags &= ~INP_HDRINCL;
			}
			if (*m != 0)
				(void) m_free(*m);
			break;

#ifdef MROUTING
		case MRT_INIT:
		case MRT_DONE:
		case MRT_ADD_VIF:
		case MRT_DEL_VIF:
		case MRT_ADD_MFC:
		case MRT_DEL_MFC:
		case MRT_ASSERT:
			error = ip_mrouter_set(so, optname, m);
			break;
#endif

		default:
			error = ip_ctloutput(op, so, level, optname, m);
			break;
		}
		break;

	case PRCO_GETOPT:
		switch (optname) {
		case IP_HDRINCL:
			*m = m_get(M_WAIT, MT_SOOPTS);
			(*m)->m_len = sizeof (int);
			*mtod(*m, int *) = inp->inp_flags & INP_HDRINCL ? 1 : 0;
			break;

#ifdef MROUTING
		case MRT_VERSION:
		case MRT_ASSERT:
			error = ip_mrouter_get(so, optname, m);
			break;
#endif

		default:
			error = ip_ctloutput(op, so, level, optname, m);
			break;
		}
		break;
	}
	return (error);
}

int
rip_bind(inp, nam)
	struct inpcb *inp;
	struct mbuf *nam;
{
	struct sockaddr_in *addr = mtod(nam, struct sockaddr_in *);

	if (nam->m_len != sizeof(*addr))
		return (EINVAL);
	if (TAILQ_FIRST(&ifnet) == 0)
		return (EADDRNOTAVAIL);
	if (addr->sin_family != AF_INET &&
	    addr->sin_family != AF_IMPLINK)
		return (EAFNOSUPPORT);
	if (!in_nullhost(addr->sin_addr) &&
	    ifa_ifwithaddr(sintosa(addr)) == 0)
		return (EADDRNOTAVAIL);
	inp->inp_laddr = addr->sin_addr;
	return (0);
}

int
rip_connect(inp, nam)
	struct inpcb *inp;
	struct mbuf *nam;
{
	struct sockaddr_in *addr = mtod(nam, struct sockaddr_in *);

	if (nam->m_len != sizeof(*addr))
		return (EINVAL);
	if (TAILQ_FIRST(&ifnet) == 0)
		return (EADDRNOTAVAIL);
	if (addr->sin_family != AF_INET &&
	    addr->sin_family != AF_IMPLINK)
		return (EAFNOSUPPORT);
	inp->inp_faddr = addr->sin_addr;
	return (0);
}

void
rip_disconnect(inp)
	struct inpcb *inp;
{

	inp->inp_faddr = zeroin_addr;
}

u_long	rip_sendspace = RIPSNDQ;
u_long	rip_recvspace = RIPRCVQ;

/*ARGSUSED*/
int
rip_usrreq(so, req, m, nam, control, p)
	struct socket *so;
	int req;
	struct mbuf *m, *nam, *control;
	struct proc *p;
{
	struct inpcb *inp;
	int s;
	int error = 0;
#ifdef MROUTING
	extern struct socket *ip_mrouter;
#endif

	if (req == PRU_CONTROL)
		return (in_control(so, (long)m, (caddr_t)nam,
		    (struct ifnet *)control, p));

	if (req == PRU_PURGEIF) {
		in_pcbpurgeif0(&rawcbtable, (struct ifnet *)control);
		in_purgeif((struct ifnet *)control);
		in_pcbpurgeif(&rawcbtable, (struct ifnet *)control);
		return (0);
	}

	s = splsoftnet();
	inp = sotoinpcb(so);
#ifdef DIAGNOSTIC
	if (req != PRU_SEND && req != PRU_SENDOOB && control)
		panic("rip_usrreq: unexpected control mbuf");
#endif
	if (inp == 0 && req != PRU_ATTACH) {
		error = EINVAL;
		goto release;
	}

	switch (req) {

	case PRU_ATTACH:
		if (inp != 0) {
			error = EISCONN;
			break;
		}
		if (p == 0 || (error = suser(p->p_ucred, &p->p_acflag))) {
			error = EACCES;
			break;
		}
		if (so->so_snd.sb_hiwat == 0 || so->so_rcv.sb_hiwat == 0) {
			error = soreserve(so, rip_sendspace, rip_recvspace);
			if (error)
				break;
		}
		error = in_pcballoc(so, &rawcbtable);
		if (error)
			break;
		inp = sotoinpcb(so);
		inp->inp_ip.ip_p = (long)nam;
		break;

	case PRU_DETACH:
#ifdef MROUTING
		if (so == ip_mrouter)
			ip_mrouter_done();
#endif
		in_pcbdetach(inp);
		break;

	case PRU_BIND:
		error = rip_bind(inp, nam);
		break;

	case PRU_LISTEN:
		error = EOPNOTSUPP;
		break;

	case PRU_CONNECT:
		error = rip_connect(inp, nam);
		if (error)
			break;
		soisconnected(so);
		break;

	case PRU_CONNECT2:
		error = EOPNOTSUPP;
		break;

	case PRU_DISCONNECT:
		soisdisconnected(so);
		rip_disconnect(inp);
		break;

	/*
	 * Mark the connection as being incapable of further input.
	 */
	case PRU_SHUTDOWN:
		socantsendmore(so);
		break;

	case PRU_RCVD:
		error = EOPNOTSUPP;
		break;

	/*
	 * Ship a packet out.  The appropriate raw output
	 * routine handles any massaging necessary.
	 */
	case PRU_SEND:
		if (control && control->m_len) {
			m_freem(control);
			m_freem(m);
			error = EINVAL;
			break;
		}
	{
		if (nam) {
			if ((so->so_state & SS_ISCONNECTED) != 0) {
				error = EISCONN;
				goto die;
			}
			error = rip_connect(inp, nam);
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
		error = rip_output(m, inp);
		if (nam)
			rip_disconnect(inp);
	}
		break;

	case PRU_SENSE:
		/*
		 * stat: don't bother with a blocksize.
		 */
		splx(s);
		return (0);

	case PRU_RCVOOB:
		error = EOPNOTSUPP;
		break;

	case PRU_SENDOOB:
		m_freem(control);
		m_freem(m);
		error = EOPNOTSUPP;
		break;

	case PRU_SOCKADDR:
		in_setsockaddr(inp, nam);
		break;

	case PRU_PEERADDR:
		in_setpeeraddr(inp, nam);
		break;

	default:
		panic("rip_usrreq");
	}

release:
	splx(s);
	return (error);
}
