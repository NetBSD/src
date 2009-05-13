/*	$NetBSD: cltp_usrreq.c,v 1.34.14.1 2009/05/13 17:22:41 jym Exp $	*/

/*
 * Copyright (c) 1989, 1993
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
 *	@(#)cltp_usrreq.c	8.1 (Berkeley) 6/10/93
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: cltp_usrreq.c,v 1.34.14.1 2009/05/13 17:22:41 jym Exp $");

#ifndef CLTPOVAL_SRC		/* XXX -- till files gets changed */
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

#include <net/if.h>
#include <net/route.h>

#include <netiso/argo_debug.h>
#include <netiso/iso.h>
#include <netiso/iso_pcb.h>
#include <netiso/iso_var.h>
#include <netiso/clnp.h>
#include <netiso/cltp_var.h>
#include <netiso/tp_param.h>
#include <netiso/tp_var.h>

#include <machine/stdarg.h>
#endif


/*
 * CLTP protocol implementation.
 * Per ISO 8602, December, 1987.
 */
void
cltp_init(void)
{

	cltb.isop_next = cltb.isop_prev = &cltb;
}

int cltp_cksum = 1;
struct isopcb   cltb;
struct cltpstat cltpstat;


/* ARGUSED */
void
cltp_input(struct mbuf *m0, ...)
{
	struct sockaddr *srcsa, *dstsa;
	u_int           cons_channel;
	struct isopcb *isop;
	struct mbuf *m = m0;
	struct mbuf *m_src = 0;
	u_char *up = mtod(m, u_char *);
	struct sockaddr_iso *src;
	int             len, hdrlen = *up + 1, dlen = 0;
	u_char         *uplim = up + hdrlen;
	void *        dtsap = NULL;
	va_list ap;

	va_start(ap, m0);
	srcsa = va_arg(ap, struct sockaddr *);
	dstsa = va_arg(ap, struct sockaddr *);
	cons_channel = va_arg(ap, int);
	va_end(ap);
	src = satosiso(srcsa);

	for (len = 0; m; m = m->m_next)
		len += m->m_len;
	up += 2;		/* skip header */
	while (up < uplim)
		switch (*up) {	/* process options */
		case CLTPOVAL_SRC:
			src->siso_tlen = up[1];
			src->siso_len = up[1] +
			    ((const char *)TSEL(src) - (const char *)src);
			if (src->siso_len < sizeof(*src))
				src->siso_len = sizeof(*src);
			else if (src->siso_len > sizeof(*src)) {
				MGET(m_src, M_DONTWAIT, MT_SONAME);
				if (m_src == 0)
					goto bad;
				m_src->m_len = src->siso_len;
				src = mtod(m_src, struct sockaddr_iso *);
				memcpy((void *) src, (void *) srcsa, srcsa->sa_len);
			}
			memcpy(WRITABLE_TSEL(src), (char *)up + 2, up[1]);
			up += 2 + src->siso_tlen;
			continue;

		case CLTPOVAL_DST:
			dtsap = 2 + (char *)up;
			dlen = up[1];
			up += 2 + dlen;
			continue;

		case CLTPOVAL_CSM:
			if (iso_check_csum(m0, len)) {
				cltpstat.cltps_badsum++;
				goto bad;
			}
			up += 4;
			continue;

		default:
			printf("clts: unknown option (%x)\n", up[0]);
			cltpstat.cltps_hdrops++;
			goto bad;
		}
	if (dlen == 0 || src->siso_tlen == 0)
		goto bad;
	for (isop = cltb.isop_next;; isop = isop->isop_next) {
		if (isop == &cltb) {
			cltpstat.cltps_noport++;
			goto bad;
		}
		if (isop->isop_laddr &&
		    memcmp(TSEL(isop->isop_laddr), dtsap, dlen) == 0)
			break;
	}
	m = m0;
	m->m_len -= hdrlen;
	m->m_data += hdrlen;
	if (sbappendaddr(&isop->isop_socket->so_rcv, sisotosa(src), m,
			 (struct mbuf *) 0) == 0)
		goto bad;
	cltpstat.cltps_ipackets++;
	sorwakeup(isop->isop_socket);
	m0 = 0;
bad:
	if (src != satosiso(srcsa))
		m_freem(m_src);
	if (m0)
		m_freem(m0);
}

/*
 * Notify a cltp user of an asynchronous error;
 * just wake up so that he can collect error status.
 */
void
cltp_notify(struct isopcb *isop)
{

	sorwakeup(isop->isop_socket);
	sowwakeup(isop->isop_socket);
}

void
cltp_ctlinput(
    int cmd,
    const struct sockaddr *sa,
    void *dummy)
{
	const struct sockaddr_iso *siso;

	if ((unsigned)cmd >= PRC_NCMDS)
		return;
	if (sa->sa_family != AF_ISO && sa->sa_family != AF_CCITT)
		return;
	siso = satocsiso(sa);
	if (siso == 0 || siso->siso_nlen == 0)
		return;

	switch (cmd) {
	case PRC_ROUTEDEAD:
	case PRC_REDIRECT_NET:
	case PRC_REDIRECT_HOST:
	case PRC_REDIRECT_TOSNET:
	case PRC_REDIRECT_TOSHOST:
		iso_pcbnotify(&cltb, siso,
			      (int) isoctlerrmap[cmd], iso_rtchange);
		break;

	default:
		if (isoctlerrmap[cmd] == 0)
			return;	/* XXX */
		iso_pcbnotify(&cltb, siso, (int) isoctlerrmap[cmd],
			      cltp_notify);
	}
}

int
cltp_output(struct mbuf *m, ...)
{
	struct isopcb *isop;
	int    len;
	struct sockaddr_iso *siso;
	int             hdrlen, error = 0, docsum;
	u_char *up;
	va_list ap;

	va_start(ap, m);
	isop = va_arg(ap, struct isopcb *);
	va_end(ap);

	if (isop->isop_laddr == 0 || isop->isop_faddr == 0) {
		error = ENOTCONN;
		goto bad;
	}
	/*
	 * Calculate data length and get a mbuf for CLTP header.
	 */
	hdrlen = 2 + 2 + isop->isop_laddr->siso_tlen
		+ 2 + isop->isop_faddr->siso_tlen;
	docsum = /* isop->isop_flags & CLNP_NO_CKSUM */ cltp_cksum;
	if (docsum)
		hdrlen += 4;
	M_PREPEND(m, hdrlen, M_WAIT);
	len = m->m_pkthdr.len;
	/*
	 * Fill in mbuf with extended CLTP header
	 */
	up = mtod(m, u_char *);
	up[0] = hdrlen - 1;
	up[1] = UD_TPDU_type;
	up[2] = CLTPOVAL_SRC;
	up[3] = (siso = isop->isop_laddr)->siso_tlen;
	up += 4;
	memcpy((void *) up, TSEL(siso), siso->siso_tlen);
	up += siso->siso_tlen;
	up[0] = CLTPOVAL_DST;
	up[1] = (siso = isop->isop_faddr)->siso_tlen;
	up += 2;
	memcpy((void *) up, TSEL(siso), siso->siso_tlen);
	/*
	 * Stuff checksum and output datagram.
	 */
	if (docsum) {
		up += siso->siso_tlen;
		up[0] = CLTPOVAL_CSM;
		up[1] = 2;
		iso_gen_csum(m, 2 + up - mtod(m, u_char *), len);
	}
	cltpstat.cltps_opackets++;
	return (tpclnp_output(m, len, isop, !docsum));
bad:
	m_freem(m);
	return (error);
}

u_long          cltp_sendspace = 9216;	/* really max datagram size */
u_long          cltp_recvspace = 40 * (1024 + sizeof(struct sockaddr_iso));
/* 40 1K datagrams */


/* ARGSUSED */
int
cltp_usrreq(struct socket *so, int req, struct mbuf *m, struct mbuf *nam, struct mbuf *control, struct lwp *l)
{
	struct isopcb *isop;
	int s;
	int error = 0;

	if (req == PRU_CONTROL)
		return (iso_control(so, (long)m, (void *)nam,
		    (struct ifnet *)control, l));

	if (req == PRU_PURGEIF) {
		mutex_enter(softnet_lock);
		iso_purgeif((struct ifnet *)control);
		mutex_exit(softnet_lock);
		return (0);
	}

	s = splsoftnet();
	isop = sotoisopcb(so);
#ifdef DIAGNOSTIC
	if (req != PRU_SEND && req != PRU_SENDOOB && control)
		panic("cltp_usrreq: unexpected control mbuf");
#endif
	if (isop == 0 && req != PRU_ATTACH) {
		error = EINVAL;
		goto release;
	}

	switch (req) {

	case PRU_ATTACH:
		sosetlock(so);
		if (isop != 0) {
			error = EISCONN;
			break;
		}
		if (so->so_snd.sb_hiwat == 0 || so->so_rcv.sb_hiwat == 0) {
			error = soreserve(so, cltp_sendspace, cltp_recvspace);
			if (error)
				break;
		}
		error = iso_pcballoc(so, &cltb);
		if (error)
			break;
		break;

	case PRU_DETACH:
		iso_pcbdetach(isop);
		break;

	case PRU_BIND:
		error = iso_pcbbind(isop, nam, l);
		break;

	case PRU_LISTEN:
		error = EOPNOTSUPP;
		break;

	case PRU_CONNECT:
		error = iso_pcbconnect(isop, nam, l);
		if (error)
			break;
		soisconnected(so);
		break;

	case PRU_CONNECT2:
		error = EOPNOTSUPP;
		break;

	case PRU_DISCONNECT:
		soisdisconnected(so);
		iso_pcbdisconnect(isop);
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
		if (nam) {
			if ((so->so_state & SS_ISCONNECTED) != 0) {
				error = EISCONN;
				goto die;
			}
			error = iso_pcbconnect(isop, nam, l);
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
		error = cltp_output(m, isop);
		if (nam)
			iso_pcbdisconnect(isop);
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
		iso_getnetaddr(isop, nam, TP_LOCAL);
		break;

	case PRU_PEERADDR:
		iso_getnetaddr(isop, nam, TP_FOREIGN);
		break;

	default:
		panic("cltp_usrreq");
	}

release:
	splx(s);
	return (error);
}
