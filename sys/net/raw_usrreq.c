/*	$NetBSD: raw_usrreq.c,v 1.25.8.1 2006/05/24 10:58:56 yamt Exp $	*/

/*
 * Copyright (c) 1980, 1986, 1993
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
 *	@(#)raw_usrreq.c	8.1 (Berkeley) 6/10/93
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: raw_usrreq.c,v 1.25.8.1 2006/05/24 10:58:56 yamt Exp $");

#include <sys/param.h>
#include <sys/mbuf.h>
#include <sys/domain.h>
#include <sys/protosw.h>
#include <sys/socket.h>
#include <sys/socketvar.h>
#include <sys/errno.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/kauth.h>

#include <net/if.h>
#include <net/route.h>
#include <net/netisr.h>
#include <net/raw_cb.h>

#include <machine/stdarg.h>
/*
 * Initialize raw connection block q.
 */
void
raw_init(void)
{

	LIST_INIT(&rawcb);
}


/*
 * Raw protocol input routine.  Find the socket
 * associated with the packet(s) and move them over.  If
 * nothing exists for this packet, drop it.
 */
/*
 * Raw protocol interface.
 */
void
raw_input(struct mbuf *m0, ...)
{
	struct rawcb *rp;
	struct mbuf *m = m0;
	int sockets = 0;
	struct socket *last;
	va_list ap;
	struct sockproto *proto;
	struct sockaddr *src, *dst;

	va_start(ap, m0);
	proto = va_arg(ap, struct sockproto *);
	src = va_arg(ap, struct sockaddr *);
	dst = va_arg(ap, struct sockaddr *);
	va_end(ap);

	last = 0;
	LIST_FOREACH(rp, &rawcb, rcb_list) {
		if (rp->rcb_proto.sp_family != proto->sp_family)
			continue;
		if (rp->rcb_proto.sp_protocol  &&
		    rp->rcb_proto.sp_protocol != proto->sp_protocol)
			continue;
		/*
		 * We assume the lower level routines have
		 * placed the address in a canonical format
		 * suitable for a structure comparison.
		 *
		 * Note that if the lengths are not the same
		 * the comparison will fail at the first byte.
		 */
#define	equal(a1, a2) \
  (bcmp((caddr_t)(a1), (caddr_t)(a2), a1->sa_len) == 0)
		if (rp->rcb_laddr && !equal(rp->rcb_laddr, dst))
			continue;
		if (rp->rcb_faddr && !equal(rp->rcb_faddr, src))
			continue;
		if (last) {
			struct mbuf *n;
			if ((n = m_copy(m, 0, (int)M_COPYALL)) != NULL) {
				if (sbappendaddr(&last->so_rcv, src,
				    n, (struct mbuf *)0) == 0)
					/* should notify about lost packet */
					m_freem(n);
				else {
					sorwakeup(last);
					sockets++;
				}
			}
		}
		last = rp->rcb_socket;
	}
	if (last) {
		if (sbappendaddr(&last->so_rcv, src,
		    m, (struct mbuf *)0) == 0)
			m_freem(m);
		else {
			sorwakeup(last);
			sockets++;
		}
	} else
		m_freem(m);
}

/*ARGSUSED*/
void *
raw_ctlinput(int cmd, struct sockaddr *arg, void *d)
{

	if ((unsigned)cmd >= PRC_NCMDS)
		return NULL;
	return NULL;
	/* INCOMPLETE */
}

void
raw_setsockaddr(struct rawcb *rp, struct mbuf *nam)
{

	nam->m_len = rp->rcb_laddr->sa_len;
	bcopy(rp->rcb_laddr, mtod(nam, caddr_t), (size_t)nam->m_len);
}

void
raw_setpeeraddr(struct rawcb *rp, struct mbuf *nam)
{

	nam->m_len = rp->rcb_faddr->sa_len;
	bcopy(rp->rcb_faddr, mtod(nam, caddr_t), (size_t)nam->m_len);
}

/*ARGSUSED*/
int
raw_usrreq(struct socket *so, int req, struct mbuf *m, struct mbuf *nam,
    struct mbuf *control, struct lwp *l)
{
	struct rawcb *rp;
	struct proc *p;
	int s;
	int error = 0;

	if (req == PRU_CONTROL)
		return (EOPNOTSUPP);

	p = l ? l->l_proc : NULL;
	s = splsoftnet();
	rp = sotorawcb(so);
#ifdef DIAGNOSTIC
	if (req != PRU_SEND && req != PRU_SENDOOB && control)
		panic("raw_usrreq: unexpected control mbuf");
#endif
	if (rp == 0 && req != PRU_ATTACH) {
		error = EINVAL;
		goto release;
	}

	switch (req) {

	/*
	 * Allocate a raw control block and fill in the
	 * necessary info to allow packets to be routed to
	 * the appropriate raw interface routine.
	 */
	case PRU_ATTACH:
		if (p == 0 || (error = kauth_authorize_generic(p->p_cred, KAUTH_GENERIC_ISSUSER, &p->p_acflag))) {
			error = EACCES;
			break;
		}
		error = raw_attach(so, (int)(long)nam);
		break;

	/*
	 * Destroy state just before socket deallocation.
	 * Flush data or not depending on the options.
	 */
	case PRU_DETACH:
		raw_detach(rp);
		break;

	/*
	 * If a socket isn't bound to a single address,
	 * the raw input routine will hand it anything
	 * within that protocol family (assuming there's
	 * nothing else around it should go to).
	 */
	case PRU_BIND:
	case PRU_LISTEN:
	case PRU_CONNECT:
	case PRU_CONNECT2:
		error = EOPNOTSUPP;
		break;

	case PRU_DISCONNECT:
		soisdisconnected(so);
		raw_disconnect(rp);
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
		if (nam) {
			if ((so->so_state & SS_ISCONNECTED) != 0) {
				error = EISCONN;
				goto die;
			}
			error = (*so->so_proto->pr_usrreq)(so, PRU_CONNECT,
			    (struct mbuf *)0, nam, (struct mbuf *)0, l);
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
		error = (*so->so_proto->pr_output)(m, so);
		if (nam)
			raw_disconnect(rp);
		break;

	case PRU_SENSE:
		/*
		 * stat: don't bother with a blocksize.
		 */
		return (0);

	/*
	 * Not supported.
	 */
	case PRU_RCVOOB:
		error = EOPNOTSUPP;
		break;

	case PRU_SENDOOB:
		m_freem(control);
		m_freem(m);
		error = EOPNOTSUPP;
		break;

	case PRU_SOCKADDR:
		if (rp->rcb_laddr == 0) {
			error = EINVAL;
			break;
		}
		raw_setsockaddr(rp, nam);
		break;

	case PRU_PEERADDR:
		if (rp->rcb_faddr == 0) {
			error = ENOTCONN;
			break;
		}
		raw_setpeeraddr(rp, nam);
		break;

	default:
		panic("raw_usrreq");
	}

release:
	splx(s);
	return (error);
}
