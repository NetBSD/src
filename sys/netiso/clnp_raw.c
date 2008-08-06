/*	$NetBSD: clnp_raw.c,v 1.30 2008/08/06 15:01:23 plunky Exp $	*/

/*-
 * Copyright (c) 1991, 1993
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
 *	@(#)clnp_raw.c	8.1 (Berkeley) 6/10/93
 */

/***********************************************************
				Copyright IBM Corporation 1987

                      All Rights Reserved

Permission to use, copy, modify, and distribute this software and its
documentation for any purpose and without fee is hereby granted,
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in
supporting documentation, and that the name of IBM not be
used in advertising or publicity pertaining to distribution of the
software without specific, written prior permission.

IBM DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING
ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL
IBM BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR
ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
SOFTWARE.

******************************************************************/

/*
 * ARGO Project, Computer Sciences Dept., University of Wisconsin - Madison
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: clnp_raw.c,v 1.30 2008/08/06 15:01:23 plunky Exp $");

#include <sys/param.h>
#include <sys/mbuf.h>
#include <sys/domain.h>
#include <sys/protosw.h>
#include <sys/socket.h>
#include <sys/socketvar.h>
#include <sys/errno.h>
#include <sys/time.h>
#include <sys/systm.h>
#include <sys/proc.h>

#include <net/if.h>
#include <net/route.h>
#include <net/raw_cb.h>

#include <netiso/iso.h>
#include <netiso/iso_pcb.h>
#include <netiso/clnp.h>
#include <netiso/clnp_stat.h>
#include <netiso/argo_debug.h>

#include <netiso/tp_user.h>	/* XXX -- defines SOL_NETWORK */

#include <machine/stdarg.h>

struct sockproto rclnp_proto = {PF_ISO, 0};

/*
 * FUNCTION:		rclnp_input
 *
 * PURPOSE:		Setup generic address and protocol structures for
 *			raw input routine, then pass them along with the
 *			mbuf chain.
 *
 * RETURNS:		none
 *
 * SIDE EFFECTS:
 *
 * NOTES:		The protocol field of rclnp_proto is set to zero
 *			indicating no protocol.
 */
void
rclnp_input(struct mbuf *m, ...)
{
	struct sockaddr_iso *src;	/* ptr to src address */
	struct sockaddr_iso *dst;	/* ptr to dest address */
	int             hdrlen;	/* length (in bytes) of clnp header */
	va_list ap;

	va_start(ap, m);
	src = va_arg(ap, struct sockaddr_iso *);
	dst = va_arg(ap, struct sockaddr_iso *);
	hdrlen = va_arg(ap, int);
	va_end(ap);
#ifdef	TROLL
	if (trollctl.tr_ops & TR_CHUCK) {
		m_freem(m);
		return;
	}
#endif				/* TROLL */

	raw_input(m, &rclnp_proto, sisotosa(src), sisotosa(dst));
}

/*
 * FUNCTION:		rclnp_output
 *
 * PURPOSE:			Prepare to send a raw clnp packet. Setup src and dest
 *					addresses, count the number of bytes to send, and
 *					call clnp_output.
 *
 * RETURNS:			success - 0
 *					failure - an appropriate error code
 *
 * SIDE EFFECTS:
 *
 * NOTES:
 */
int
rclnp_output(struct mbuf *m0, ...)
{
	struct socket  *so;	/* socket to send from */
	struct rawisopcb *rp;	/* ptr to raw cb */
	int             error;	/* return value of function */
	int             flags;	/* flags for clnp_output */
	va_list ap;

	va_start(ap, m0);
	so = va_arg(ap, struct socket *);
	va_end(ap);
	rp = sotorawisopcb(so);

	if ((m0->m_flags & M_PKTHDR) == 0)
		return (EINVAL);
	/*
	 * Set up src address. If user has bound socket to an address, use it.
	 * Otherwise, do not specify src (clnp_output will fill it in).
	 */
	if (rp->risop_rcb.rcb_laddr) {
		if (rp->risop_isop.isop_sladdr.siso_family != AF_ISO) {
	bad:
			m_freem(m0);
			return (EAFNOSUPPORT);
		}
	}
	/* set up dest address */
	if (rp->risop_rcb.rcb_faddr == 0)
		goto bad;
	rp->risop_isop.isop_sfaddr = *satosiso(rp->risop_rcb.rcb_faddr);
	rp->risop_isop.isop_faddr = &rp->risop_isop.isop_sfaddr;

	/* get flags and ship it off */
	flags = rp->risop_flags & CLNP_VFLAGS;

	error = clnp_output(m0, &rp->risop_isop, m0->m_pkthdr.len,
			    flags | CLNP_NOCACHE);

	return (error);
}

/*
 * FUNCTION:		rclnp_ctloutput
 *
 * PURPOSE:			Raw clnp socket option processing
 *				All options are stored inside a sockopt.
 *
 * RETURNS:			success - 0
 *					failure - unix error code
 *
 * SIDE EFFECTS:
 *
 * NOTES:
 */
int
rclnp_ctloutput(
	int             op,	/* type of operation */
	struct socket  *so,	/* ptr to socket */
	struct sockopt *sopt)	/* socket options */
{
	int             error = 0;
	struct rawisopcb *rp = sotorawisopcb(so);	/* raw cb ptr */

#ifdef ARGO_DEBUG
	if (argo_debug[D_CTLOUTPUT]) {
		printf("rclnp_ctloutput: op = x%x, level = x%x, name = x%x\n",
		    op, sopt->sopt_level, sopt->sopt_name);
		printf("rclnp_ctloutput: %d bytes of data\n", sopt->sopt_size);
		dump_buf(sopt->sopt_data, sopt->sopt_size);
	}
#endif

#ifdef SOL_NETWORK
	if (sopt->sopt_level != SOL_NETWORK)
		return (EINVAL);
#endif

	switch (op) {
	case PRCO_SETOPT:
		switch (sopt->sopt_name) {
		case CLNPOPT_FLAGS:{
			u_short flags;

			error = sockopt_get(sopt, &flags, sizeof(flags));
			if (error)
				break;

			/*
			 *	Don't allow invalid flags to be set
			 */
			if ((flags & (CLNP_VFLAGS)) != flags)
				error = EINVAL;
			else
				rp->risop_flags |= flags;

			break;
			}

		case CLNPOPT_OPTS: {
			struct mbuf *m;

			m = sockopt_getmbuf(sopt);
			if (m == NULL) {
				error = ENOBUFS;
				break;
			}

			error = clnp_set_opts(&rp->risop_isop.isop_options, &m);
			m_freem(m);
			if (error)
				break;
			rp->risop_isop.isop_optindex = m_get(M_WAIT, MT_SOOPTS);
			(void) clnp_opt_sanity(rp->risop_isop.isop_options,
				 mtod(rp->risop_isop.isop_options, void *),
					 rp->risop_isop.isop_options->m_len,
					  mtod(rp->risop_isop.isop_optindex,
					       struct clnp_optidx *));
			break;
			}
		}
		break;

	case PRCO_GETOPT:
#ifdef notdef
		/* commented out to keep hi C quiet */
		switch (sopt->sopt_name) {
		default:
			error = EINVAL;
			break;
		}
#endif				/* notdef */
		break;
	default:
		error = EINVAL;
		break;
	}
	return error;
}

/* ARGSUSED */
int
clnp_usrreq(struct socket *so, int req, struct mbuf *m, struct mbuf *nam,
	struct mbuf *control, struct lwp *l)
{
	int    error = 0;
	struct rawisopcb *rp = sotorawisopcb(so);
	struct proc *p;

	p = l ? l->l_proc : NULL;
	rp = sotorawisopcb(so);
	switch (req) {

	case PRU_ATTACH:
		sosetlock(so);
		if (rp != 0) {
			error = EISCONN;
			break;
		}
		MALLOC(rp, struct rawisopcb *, sizeof *rp, M_PCB,
		    M_WAITOK|M_ZERO);
		if (rp == 0)
			return (ENOBUFS);
		so->so_pcb = rp;
		break;

	case PRU_DETACH:
		if (rp->risop_isop.isop_options)
			m_freem(rp->risop_isop.isop_options);
		rtcache_free(&rp->risop_isop.isop_route);
		if (rp->risop_rcb.rcb_laddr)
			rp->risop_rcb.rcb_laddr = 0;
		/* free clnp cached hdr if necessary */
		if (rp->risop_isop.isop_clnpcache != NULL) {
			struct clnp_cache *clcp =
			mtod(rp->risop_isop.isop_clnpcache, struct clnp_cache *);
			if (clcp->clc_hdr != NULL) {
				m_free(clcp->clc_hdr);
			}
			m_free(rp->risop_isop.isop_clnpcache);
		}
		if (rp->risop_isop.isop_optindex != NULL)
			m_free(rp->risop_isop.isop_optindex);

		break;

	case PRU_BIND:
		{
			struct sockaddr_iso *addr = mtod(nam, struct sockaddr_iso *);

			if (nam->m_len != sizeof(*addr))
				return (EINVAL);
			if ((ifnet.tqh_first == 0) ||
			    (addr->siso_family != AF_ISO) ||
			    (addr->siso_addr.isoa_len &&
			     ifa_ifwithaddr(sisotosa(addr)) == 0))
				return (EADDRNOTAVAIL);
			rp->risop_isop.isop_sladdr = *addr;
			rp->risop_rcb.rcb_laddr = sisotosa(
							   (rp->risop_isop.isop_laddr = &rp->risop_isop.isop_sladdr));
			return (0);
		}
	case PRU_CONNECT:
		{
			struct sockaddr_iso *addr = mtod(nam, struct sockaddr_iso *);

			if ((nam->m_len > sizeof(*addr)) || (addr->siso_len > sizeof(*addr)))
				return (EINVAL);
			if (ifnet.tqh_first == 0)
				return (EADDRNOTAVAIL);

			/* copy the address */
			if (addr->siso_family != AF_ISO)
				rp->risop_isop.isop_sfaddr = *addr;

			/* initialize address pointers */
			rp->risop_isop.isop_faddr = &rp->risop_isop.isop_sfaddr;
			rp->risop_rcb.rcb_faddr = sisotosa(
				rp->risop_isop.isop_faddr);

			/* address setup, mark socket connected */
			soisconnected(so);

			return (0);
		}
	}
	error = raw_usrreq(so, req, m, nam, control, l);

	if (error && req == PRU_ATTACH && so->so_pcb)
		free((void *) rp, M_PCB);
	return (error);
}
