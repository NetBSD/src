/*	$NetBSD: idrp_usrreq.c,v 1.12 2003/08/07 16:33:35 agc Exp $	*/

/*
 * Copyright (c) 1992, 1993
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
 *	@(#)idrp_usrreq.c	8.1 (Berkeley) 6/10/93
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: idrp_usrreq.c,v 1.12 2003/08/07 16:33:35 agc Exp $");

#include <sys/param.h>
#include <sys/proc.h>
#include <sys/systm.h>
#include <sys/malloc.h>
#include <sys/mbuf.h>
#include <sys/socket.h>
#include <sys/socketvar.h>
#include <sys/protosw.h>
#include <sys/errno.h>

#include <net/raw_cb.h>
#include <net/route.h>
#include <net/if.h>

#include <netiso/argo_debug.h>
#include <netiso/iso.h>
#include <netiso/clnp.h>
#include <netiso/clnl.h>
#include <netiso/iso_pcb.h>
#include <netiso/iso_var.h>
#include <netiso/idrp_var.h>

#include <machine/stdarg.h>

LIST_HEAD(, rawcb) idrp_pcb;
struct isopcb idrp_isop;
static struct sockaddr_iso idrp_addrs[2] =
{{sizeof(idrp_addrs), AF_ISO,}, {sizeof(idrp_addrs[1]), AF_ISO,}};

/*
 * IDRP initialization
 */
void
idrp_init()
{
	extern struct clnl_protosw clnl_protox[256];

	LIST_INIT(&idrp_pcb);

	idrp_isop.isop_next = idrp_isop.isop_prev = &idrp_isop;
	idrp_isop.isop_faddr = &idrp_isop.isop_sfaddr;
	idrp_isop.isop_laddr = &idrp_isop.isop_sladdr;
	idrp_isop.isop_sladdr = idrp_addrs[1];
	idrp_isop.isop_sfaddr = idrp_addrs[1];
	clnl_protox[ISO10747_IDRP].clnl_input = idrp_input;
}

/*
 * CALLED FROM:
 * 	tpclnp_input().
 * FUNCTION and ARGUMENTS:
 * Take a packet (m) from clnp, strip off the clnp header
 * and mke suitable for the idrp socket.
 * No return value.
 */
void
#if __STDC__
idrp_input(struct mbuf *m, ...)
#else
idrp_input(m, va_alist)
	struct mbuf *m;
	va_dcl
#endif
{
	struct sockaddr_iso *src, *dst;
	va_list ap;

	va_start(ap, m);
	src = va_arg(ap, struct sockaddr_iso *);
	dst = va_arg(ap, struct sockaddr_iso *);
	va_end(ap);

	if (idrp_isop.isop_socket == 0) {
bad:		m_freem(m);
		return;
	}
	bzero(idrp_addrs[0].siso_data, sizeof(idrp_addrs[0].siso_data));
	bcopy((caddr_t) & (src->siso_addr), (caddr_t) & idrp_addrs[0].siso_addr,
	      1 + src->siso_nlen);
	bzero(idrp_addrs[1].siso_data, sizeof(idrp_addrs[1].siso_data));
	bcopy((caddr_t) & (dst->siso_addr), (caddr_t) & idrp_addrs[1].siso_addr,
	      1 + dst->siso_nlen);
	if (sbappendaddr(&idrp_isop.isop_socket->so_rcv,
			 sisotosa(idrp_addrs), m, (struct mbuf *) 0) == 0)
		goto bad;
	sorwakeup(idrp_isop.isop_socket);
}

int
#if __STDC__
idrp_output(struct mbuf *m, ...)
#else
idrp_output(m, va_alist)
	struct mbuf    *m;
	va_dcl
#endif
{
	struct sockaddr_iso *siso;
	int             s = splsoftnet(), i;
	va_list ap;

	va_start(ap, m);
	siso = va_arg(ap, struct sockaddr_iso *);
	va_end(ap);

	bcopy((caddr_t) & (siso->siso_addr),
	  (caddr_t) & idrp_isop.isop_sfaddr.siso_addr, 1 + siso->siso_nlen);
	siso++;
	bcopy((caddr_t) & (siso->siso_addr),
	  (caddr_t) & idrp_isop.isop_sladdr.siso_addr, 1 + siso->siso_nlen);
	i = clnp_output(m, idrp_isop, m->m_pkthdr.len, 0);
	splx(s);
	return (i);
}

u_long          idrp_sendspace = 3072;	/* really max datagram size */
u_long          idrp_recvspace = 40 * 1024;	/* 40 1K datagrams */

/* ARGSUSED */
int
idrp_usrreq(so, req, m, nam, control, p)
	struct socket *so;
	int req;
	struct mbuf *m, *nam, *control;
	struct proc *p;
{
	struct rawcb *rp;
	int error = 0;

	if (req == PRU_CONTROL)
		return (EOPNOTSUPP);

	rp = sotorawcb(so);
#ifdef DIAGNOSTIC
	if (req != PRU_SEND && req != PRU_SENDOOB && control)
		panic("idrp_usrreq: unexpected control mbuf");
#endif
	if (rp == 0 && req != PRU_ATTACH) {
		error = EINVAL;
		goto release;
	}

	/*
	 * Note: need to block idrp_input while changing the udp pcb queue
	 * and/or pcb addresses.
	 */
	switch (req) {

	case PRU_ATTACH:
		if (rp != 0) {
			error = EISCONN;
			break;
		}
		if (so->so_snd.sb_hiwat == 0 || so->so_rcv.sb_hiwat == 0) {
			error = soreserve(so, idrp_sendspace, idrp_recvspace);
			if (error)
				break;
		}
		MALLOC(rp, struct rawcb *, sizeof(*rp), M_PCB, M_WAITOK);
		if (rp == 0) {
			error = ENOBUFS;
			break;
		}
		bzero(rp, sizeof(*rp));
		rp->rcb_socket = so;
		LIST_INSERT_HEAD(&idrp_pcb, rp, rcb_list);
		so->so_pcb = rp;
		break;

	case PRU_SEND:
		if (control && control->m_len) {
			m_freem(control);
			m_freem(m);
			error = EINVAL;
			break;
		}
		if (nam == NULL) {
			m_freem(m);
			error = EINVAL;
			break;
		}
		/* error checking here */
		error = idrp_output(m, mtod(nam, struct sockaddr_iso *));
		break;

	case PRU_SENDOOB:
		m_freem(control);
		m_freem(m);
		error = EOPNOTSUPP;
		break;

	case PRU_DETACH:
		raw_detach(rp);
		break;

	case PRU_SHUTDOWN:
		socantsendmore(so);
		break;

	case PRU_SENSE:
		/*
		 * stat: don't bother with a blocksize.
		 */
		return (0);

	default:
		error = EOPNOTSUPP;
		break;
	}

release:
	return (error);
}
