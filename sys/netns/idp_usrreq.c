/*	$NetBSD: idp_usrreq.c,v 1.25.6.1 2006/04/22 11:40:14 simonb Exp $	*/

/*
 * Copyright (c) 1984, 1985, 1986, 1987, 1993
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
 *	@(#)idp_usrreq.c	8.2 (Berkeley) 1/9/95
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: idp_usrreq.c,v 1.25.6.1 2006/04/22 11:40:14 simonb Exp $");

#include "opt_ns.h"			/* NSIP: Xerox NS over IP */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/malloc.h>
#include <sys/mbuf.h>
#include <sys/protosw.h>
#include <sys/socket.h>
#include <sys/socketvar.h>
#include <sys/errno.h>
#include <sys/stat.h>
#include <sys/proc.h>

#include <net/if.h>
#include <net/route.h>

#include <netns/ns.h>
#include <netns/ns_pcb.h>
#include <netns/ns_if.h>
#include <netns/ns_var.h>
#include <netns/idp.h>
#include <netns/idp_var.h>
#include <netns/ns_error.h>

#include <machine/stdarg.h>
/*
 * IDP protocol implementation.
 */

struct	sockaddr_ns idp_ns = { sizeof(idp_ns), AF_NS };

/*
 *  This may also be called for raw listeners.
 */
void
idp_input(struct mbuf *m, ...)
{
	struct nspcb *nsp;
	struct idp *idp = mtod(m, struct idp *);
	struct ifnet *ifp = m->m_pkthdr.rcvif;
	va_list ap;

	va_start(ap, m);
	nsp = va_arg(ap, struct nspcb *);
	va_end(ap);

	if (nsp==0)
		panic("No nspcb");
	/*
	 * Construct sockaddr format source address.
	 * Stuff source address and datagram in user buffer.
	 */
	idp_ns.sns_addr = idp->idp_sna;
	if (ns_neteqnn(idp->idp_sna.x_net, ns_zeronet) && ifp) {
		struct ifaddr *ifa;

		for (ifa = ifp->if_addrlist.tqh_first; ifa != 0;
		    ifa = ifa->ifa_list.tqe_next) {
			if (ifa->ifa_addr->sa_family == AF_NS) {
				idp_ns.sns_addr.x_net =
					IA_SNS(ifa)->sns_addr.x_net;
				break;
			}
		}
	}
	nsp->nsp_rpt = idp->idp_pt;
	if ( ! (nsp->nsp_flags & NSP_RAWIN) ) {
		m->m_len -= sizeof (struct idp);
		m->m_pkthdr.len -= sizeof (struct idp);
		m->m_data += sizeof (struct idp);
	}
	if (sbappendaddr(&nsp->nsp_socket->so_rcv, snstosa(&idp_ns), m,
	    (struct mbuf *)0) == 0)
		goto bad;
	sorwakeup(nsp->nsp_socket);
	return;
bad:
	m_freem(m);
}

void
idp_abort(struct nspcb *nsp)
{
	struct socket *so = nsp->nsp_socket;

	ns_pcbdisconnect(nsp);
	soisdisconnected(so);
}
/*
 * Drop connection, reporting
 * the specified error.
 */
void
idp_drop(struct nspcb *nsp, int errno)
{
	struct socket *so = nsp->nsp_socket;

#if 0
	/*
	 * someday, in the xerox world
	 * we will generate error protocol packets
	 * announcing that the socket has gone away.
	 */
	if (TCPS_HAVERCVDSYN(tp->t_state)) {
		tp->t_state = TCPS_CLOSED;
		(void) tcp_output(tp);
	}
#endif
	so->so_error = errno;
	ns_pcbdisconnect(nsp);
	soisdisconnected(so);
}

int
idp_output(struct mbuf *m0, ...)
{
	struct nspcb *nsp;
	struct mbuf *m;
	struct idp *idp;
	int len = m0->m_pkthdr.len;
	extern int idpcksum;
	va_list ap;

	va_start(ap, m0);
	nsp = va_arg(ap, struct nspcb *);
	va_end(ap);

	/*
	 * Make sure packet is actually of even length.
	 */

	if (len & 1) {
		struct mbuf *mprev;

		for (mprev = NULL, m = m0; m; m = m->m_next)
			mprev = m;

		m = mprev;
		if ((m->m_flags & M_EXT) == 0 &&
			(m->m_len + m->m_data < &m->m_dat[MLEN])) {
			m->m_len++;
		} else {
			struct mbuf *m1 = m_get(M_DONTWAIT, MT_DATA);

			if (m1 == 0) {
				m_freem(m0);
				return (ENOBUFS);
			}
			m1->m_len = 1;
			* mtod(m1, char *) = 0;
			m->m_next = m1;
		}
		m0->m_pkthdr.len++;
	}

	/*
	 * Fill in mbuf with extended IDP header
	 * and addresses and length put into network format.
	 */
	m = m0;
	if (nsp->nsp_flags & NSP_RAWOUT) {
		idp = mtod(m, struct idp *);
	} else {
		M_PREPEND(m, sizeof (struct idp), M_DONTWAIT);
		if (m == 0)
			return (ENOBUFS);
		idp = mtod(m, struct idp *);
		idp->idp_tc = 0;
		idp->idp_pt = nsp->nsp_dpt;
		idp->idp_sna = nsp->nsp_laddr;
		idp->idp_dna = nsp->nsp_faddr;
		len += sizeof (struct idp);
	}

	idp->idp_len = htons((u_int16_t)len);

	if (idpcksum) {
		idp->idp_sum = 0;
		len = ((len - 1) | 1) + 1;
		idp->idp_sum = ns_cksum(m, len);
	} else
		idp->idp_sum = 0xffff;

	/*
	 * Use cached route for previous datagram if
	 * possible.  If the previous net was the same
	 * and the interface was a broadcast medium, or
	 * if the previous destination was identical,
	 * then we are ok.
	 *
	 * NB: We don't handle broadcasts because that
	 *     would require 3 subroutine calls.
	 */
	return (ns_output(m, &nsp->nsp_route,
	    nsp->nsp_socket->so_options & (SO_DONTROUTE | SO_BROADCAST)));
}
/* ARGSUSED */
int
idp_ctloutput(int req, struct socket *so, int level, int name,
	struct mbuf **value)
{
	struct mbuf *m;
	struct nspcb *nsp = sotonspcb(so);
	int mask, error = 0;

	if (nsp == NULL)
		return (EINVAL);

	switch (req) {

	case PRCO_GETOPT:
		if (value==NULL)
			return (EINVAL);
		m = m_get(M_DONTWAIT, MT_DATA);
		if (m==NULL)
			return (ENOBUFS);
		switch (name) {

		case SO_ALL_PACKETS:
			mask = NSP_ALL_PACKETS;
			goto get_flags;

		case SO_HEADERS_ON_INPUT:
			mask = NSP_RAWIN;
			goto get_flags;

		case SO_HEADERS_ON_OUTPUT:
			mask = NSP_RAWOUT;
		get_flags:
			m->m_len = sizeof(short);
			*mtod(m, short *) = nsp->nsp_flags & mask;
			break;

		case SO_DEFAULT_HEADERS:
			m->m_len = sizeof(struct idp);
			{
				struct idp *idp = mtod(m, struct idp *);
				idp->idp_len = 0;
				idp->idp_sum = 0;
				idp->idp_tc = 0;
				idp->idp_pt = nsp->nsp_dpt;
				idp->idp_dna = nsp->nsp_faddr;
				idp->idp_sna = nsp->nsp_laddr;
			}
			break;

		case SO_SEQNO:
			m->m_len = sizeof(long);
			*mtod(m, long *) = ns_pexseq++;
			break;

		default:
			error = EINVAL;
		}
		*value = m;
		break;

	case PRCO_SETOPT:
		switch (name) {
			int *ok;

		case SO_ALL_PACKETS:
			mask = NSP_ALL_PACKETS;
			goto set_head;

		case SO_HEADERS_ON_INPUT:
			mask = NSP_RAWIN;
			goto set_head;

		case SO_HEADERS_ON_OUTPUT:
			mask = NSP_RAWOUT;
		set_head:
			if (value && *value) {
				ok = mtod(*value, int *);
				if (*ok)
					nsp->nsp_flags |= mask;
				else
					nsp->nsp_flags &= ~mask;
			} else error = EINVAL;
			break;

		case SO_DEFAULT_HEADERS:
			{
				struct idp *idp
				    = mtod(*value, struct idp *);
				nsp->nsp_dpt = idp->idp_pt;
			}
			break;
#ifdef NSIP

		case SO_NSIP_ROUTE:
			error = nsip_route(*value);
			break;
#endif /* NSIP */
		default:
			error = EINVAL;
		}
		if (value && *value)
			m_freem(*value);
		break;
	}
	return (error);
}

u_long	idp_sendspace = 2048;
u_long	idp_recvspace = 2048;

/*ARGSUSED*/
int
idp_usrreq(struct socket *so, int req, struct mbuf *m, struct mbuf *nam,
	struct mbuf *control, struct lwp *l)
{
	struct nspcb *nsp;
	struct proc *p;
	int s;
	int error = 0;

	p = l ? l->l_proc : NULL;
	if (req == PRU_CONTROL)
                return (ns_control(so, (u_long)m, (caddr_t)nam,
		    (struct ifnet *)control, p));

	if (req == PRU_PURGEIF) {
		ns_purgeif((struct ifnet *)control);
		return (0);
	}

	s = splsoftnet();
	nsp = sotonspcb(so);
	if (nsp == 0 && req != PRU_ATTACH) {
		error = EINVAL;
		goto release;
	}

	switch (req) {

	case PRU_ATTACH:
		if (nsp != 0) {
			error = EISCONN;
			break;
		}
		if ((error = soreserve(so, idp_sendspace, idp_recvspace)) ||
		    (error = ns_pcballoc(so, &nspcb)))
			break;
		break;

	case PRU_DETACH:
		ns_pcbdetach(nsp);
		break;

	case PRU_BIND:
		error = ns_pcbbind(nsp, nam, p);
		break;

	case PRU_LISTEN:
		error = EOPNOTSUPP;
		break;

	case PRU_CONNECT:
		error = ns_pcbconnect(nsp, nam);
		if (error)
			break;
		soisconnected(so);
		break;

	case PRU_CONNECT2:
		error = EOPNOTSUPP;
		break;

	case PRU_DISCONNECT:
		soisdisconnected(so);
		ns_pcbdisconnect(nsp);
		break;

	case PRU_SHUTDOWN:
		socantsendmore(so);
		break;

	case PRU_RCVD:
		error = EOPNOTSUPP;
		break;

	case PRU_SEND:
	{
		struct ns_addr laddr;

		if (nam) {
			laddr = nsp->nsp_laddr;
			if ((so->so_state & SS_ISCONNECTED) != 0) {
				error = EISCONN;
				break;
			}
			error = ns_pcbconnect(nsp, nam);
			if (error)
				break;
		} else {
			if ((so->so_state & SS_ISCONNECTED) == 0) {
				error = ENOTCONN;
				break;
			}
		}
		error = idp_output(m, nsp);
		if (nam) {
			ns_pcbdisconnect(nsp);
			nsp->nsp_laddr = laddr;
		}
	}
		break;

	case PRU_SENSE:
		/*
		 * stat: don't bother with a blocksize.
		 */
		splx(s);
		return (0);

	/*
	 * Not supported.
	 */
	case PRU_RCVOOB:
		error = EOPNOTSUPP;
		break;

	case PRU_SENDOOB:
		m_freem(m);
		error = EOPNOTSUPP;
		break;

	case PRU_SOCKADDR:
		ns_setsockaddr(nsp, nam);
		break;

	case PRU_PEERADDR:
		ns_setpeeraddr(nsp, nam);
		break;

	default:
		panic("idp_usrreq");
	}

release:
	splx(s);
	return (error);
}

/*ARGSUSED*/
int
idp_raw_usrreq(struct socket *so, int req, struct mbuf *m, struct mbuf *nam,
	struct mbuf *control, struct lwp *l)
{
	int error = 0;
	struct proc *p;
	struct nspcb *nsp = sotonspcb(so);

	p = l->l_proc;

	switch (req) {

	case PRU_ATTACH:
		if (nsp != 0) {
			error = EISCONN;
			break;
		}
		if (p == 0 || (error = suser(p->p_ucred, &p->p_acflag))) {
			error = EACCES;
			break;
		}
		if ((error = soreserve(so, idp_sendspace, idp_recvspace)) ||
		    (error = ns_pcballoc(so, &nspcb)))
			break;
		nsp = sotonspcb(so);
		nsp->nsp_faddr.x_host = ns_broadhost;
		nsp->nsp_flags = NSP_RAWIN | NSP_RAWOUT;
		break;

	default:
		error = idp_usrreq(so, req, m, nam, control, l);
		break;
	}
	return (error);
}
