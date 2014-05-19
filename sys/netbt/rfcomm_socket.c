/*	$NetBSD: rfcomm_socket.c,v 1.13 2014/05/19 02:51:24 rmind Exp $	*/

/*-
 * Copyright (c) 2006 Itronix Inc.
 * All rights reserved.
 *
 * Written by Iain Hibbert for Itronix Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of Itronix Inc. may not be used to endorse
 *    or promote products derived from this software without specific
 *    prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY ITRONIX INC. ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL ITRONIX INC. BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: rfcomm_socket.c,v 1.13 2014/05/19 02:51:24 rmind Exp $");

/* load symbolic names */
#ifdef BLUETOOTH_DEBUG
#define PRUREQUESTS
#define PRCOREQUESTS
#endif

#include <sys/param.h>
#include <sys/domain.h>
#include <sys/kernel.h>
#include <sys/mbuf.h>
#include <sys/proc.h>
#include <sys/protosw.h>
#include <sys/socket.h>
#include <sys/socketvar.h>
#include <sys/systm.h>

#include <netbt/bluetooth.h>
#include <netbt/rfcomm.h>

/****************************************************************************
 *
 *	RFCOMM SOCK_STREAM Sockets - serial line emulation
 *
 */

static void rfcomm_connecting(void *);
static void rfcomm_connected(void *);
static void rfcomm_disconnected(void *, int);
static void *rfcomm_newconn(void *, struct sockaddr_bt *, struct sockaddr_bt *);
static void rfcomm_complete(void *, int);
static void rfcomm_linkmode(void *, int);
static void rfcomm_input(void *, struct mbuf *);

static const struct btproto rfcomm_proto = {
	rfcomm_connecting,
	rfcomm_connected,
	rfcomm_disconnected,
	rfcomm_newconn,
	rfcomm_complete,
	rfcomm_linkmode,
	rfcomm_input,
};

/* sysctl variables */
int rfcomm_sendspace = 4096;
int rfcomm_recvspace = 4096;

static int
rfcomm_attach1(struct socket *so, int proto)
{
	int error;

	KASSERT(so->so_pcb == NULL);

	if (so->so_lock == NULL) {
		mutex_obj_hold(bt_lock);
		so->so_lock = bt_lock;
		solock(so);
	}
	KASSERT(solocked(so));

	/*
	 * Since we have nothing to add, we attach the DLC
	 * structure directly to our PCB pointer.
	 */
	error = soreserve(so, rfcomm_sendspace, rfcomm_recvspace);
	if (error)
		return error;

	error = rfcomm_attach((struct rfcomm_dlc **)&so->so_pcb,
				&rfcomm_proto, so);
	if (error)
		return error;

	error = rfcomm_rcvd(so->so_pcb, sbspace(&so->so_rcv));
	if (error) {
		rfcomm_detach((struct rfcomm_dlc **)&so->so_pcb);
		return error;
	}
	return 0;
}

static void
rfcomm_detach1(struct socket *so)
{
	struct rfcomm_dlc *pcb = so->so_pcb;

	KASSERT(pcb != NULL);
	rfcomm_detach((struct rfcomm_dlc **)&so->so_pcb);
	KASSERT(so->so_pcb == NULL);
}

/*
 * User Request.
 * up is socket
 * m is either
 *	optional mbuf chain containing message
 *	ioctl command (PRU_CONTROL)
 * nam is either
 *	optional mbuf chain containing an address
 *	ioctl data (PRU_CONTROL)
 *	message flags (PRU_RCVD)
 * ctl is either
 *	optional mbuf chain containing socket options
 *	optional interface pointer (PRU_CONTROL, PRU_PURGEIF)
 * l is pointer to process requesting action (if any)
 *
 * we are responsible for disposing of m and ctl if
 * they are mbuf chains
 */
static int
rfcomm_usrreq(struct socket *up, int req, struct mbuf *m,
		struct mbuf *nam, struct mbuf *ctl, struct lwp *l)
{
	struct rfcomm_dlc *pcb = up->so_pcb;
	struct sockaddr_bt *sa;
	struct mbuf *m0;
	int err = 0;

	DPRINTFN(2, "%s\n", prurequests[req]);
	KASSERT(req != PRU_ATTACH);
	KASSERT(req != PRU_DETACH);

	switch (req) {
	case PRU_CONTROL:
		return EPASSTHROUGH;

	case PRU_PURGEIF:
		return EOPNOTSUPP;
	}
	if (pcb == NULL) {
		err = EINVAL;
		goto release;
	}

	switch(req) {
	case PRU_DISCONNECT:
		soisdisconnecting(up);
		return rfcomm_disconnect(pcb, up->so_linger);

	case PRU_ABORT:
		rfcomm_disconnect(pcb, 0);
		soisdisconnected(up);
		rfcomm_detach1(up);
		return 0;

	case PRU_BIND:
		KASSERT(nam != NULL);
		sa = mtod(nam, struct sockaddr_bt *);

		if (sa->bt_len != sizeof(struct sockaddr_bt))
			return EINVAL;

		if (sa->bt_family != AF_BLUETOOTH)
			return EAFNOSUPPORT;

		return rfcomm_bind(pcb, sa);

	case PRU_CONNECT:
		KASSERT(nam != NULL);
		sa = mtod(nam, struct sockaddr_bt *);

		if (sa->bt_len != sizeof(struct sockaddr_bt))
			return EINVAL;

		if (sa->bt_family != AF_BLUETOOTH)
			return EAFNOSUPPORT;

		soisconnecting(up);
		return rfcomm_connect(pcb, sa);

	case PRU_PEERADDR:
		KASSERT(nam != NULL);
		sa = mtod(nam, struct sockaddr_bt *);
		nam->m_len = sizeof(struct sockaddr_bt);
		return rfcomm_peeraddr(pcb, sa);

	case PRU_SOCKADDR:
		KASSERT(nam != NULL);
		sa = mtod(nam, struct sockaddr_bt *);
		nam->m_len = sizeof(struct sockaddr_bt);
		return rfcomm_sockaddr(pcb, sa);

	case PRU_SHUTDOWN:
		socantsendmore(up);
		break;

	case PRU_SEND:
		KASSERT(m != NULL);

		if (ctl)	/* no use for that */
			m_freem(ctl);

		m0 = m_copypacket(m, M_DONTWAIT);
		if (m0 == NULL)
			return ENOMEM;

		sbappendstream(&up->so_snd, m);

		return rfcomm_send(pcb, m0);

	case PRU_SENSE:
		return 0;		/* (no release) */

	case PRU_RCVD:
		return rfcomm_rcvd(pcb, sbspace(&up->so_rcv));

	case PRU_RCVOOB:
		return EOPNOTSUPP;	/* (no release) */

	case PRU_LISTEN:
		return rfcomm_listen(pcb);

	case PRU_ACCEPT:
		KASSERT(nam != NULL);
		sa = mtod(nam, struct sockaddr_bt *);
		nam->m_len = sizeof(struct sockaddr_bt);
		return rfcomm_peeraddr(pcb, sa);

	case PRU_CONNECT2:
	case PRU_SENDOOB:
	case PRU_FASTTIMO:
	case PRU_SLOWTIMO:
	case PRU_PROTORCV:
	case PRU_PROTOSEND:
		err = EOPNOTSUPP;
		break;

	default:
		UNKNOWN(req);
		err = EOPNOTSUPP;
		break;
	}

release:
	if (m) m_freem(m);
	if (ctl) m_freem(ctl);
	return err;
}

/*
 * rfcomm_ctloutput(req, socket, sockopt)
 *
 */
int
rfcomm_ctloutput(int req, struct socket *so, struct sockopt *sopt)
{
	struct rfcomm_dlc *pcb = so->so_pcb;
	int err = 0;

	DPRINTFN(2, "%s\n", prcorequests[req]);

	if (pcb == NULL)
		return EINVAL;

	if (sopt->sopt_level != BTPROTO_RFCOMM)
		return ENOPROTOOPT;

	switch(req) {
	case PRCO_GETOPT:
		err = rfcomm_getopt(pcb, sopt);
		break;

	case PRCO_SETOPT:
		err = rfcomm_setopt(pcb, sopt);
		break;

	default:
		err = ENOPROTOOPT;
		break;
	}

	return err;
}

/**********************************************************************
 *
 * RFCOMM callbacks
 */

static void
rfcomm_connecting(void *arg)
{
	/* struct socket *so = arg; */

	KASSERT(arg != NULL);
	DPRINTF("Connecting\n");
}

static void
rfcomm_connected(void *arg)
{
	struct socket *so = arg;

	KASSERT(so != NULL);
	DPRINTF("Connected\n");
	soisconnected(so);
}

static void
rfcomm_disconnected(void *arg, int err)
{
	struct socket *so = arg;

	KASSERT(so != NULL);
	DPRINTF("Disconnected\n");

	so->so_error = err;
	soisdisconnected(so);
}

static void *
rfcomm_newconn(void *arg, struct sockaddr_bt *laddr,
    struct sockaddr_bt *raddr)
{
	struct socket *so = arg;

	DPRINTF("New Connection\n");
	so = sonewconn(so, false);
	if (so == NULL)
		return NULL;

	soisconnecting(so);

	return so->so_pcb;
}

/*
 * rfcomm_complete(rfcomm_dlc, length)
 *
 * length bytes are sent and may be removed from socket buffer
 */
static void
rfcomm_complete(void *arg, int length)
{
	struct socket *so = arg;

	sbdrop(&so->so_snd, length);
	sowwakeup(so);
}

/*
 * rfcomm_linkmode(rfcomm_dlc, new)
 *
 * link mode change notification.
 */
static void
rfcomm_linkmode(void *arg, int new)
{
	struct socket *so = arg;
	struct sockopt sopt;
	int mode;

	DPRINTF("auth %s, encrypt %s, secure %s\n",
		(new & RFCOMM_LM_AUTH ? "on" : "off"),
		(new & RFCOMM_LM_ENCRYPT ? "on" : "off"),
		(new & RFCOMM_LM_SECURE ? "on" : "off"));

	sockopt_init(&sopt, BTPROTO_RFCOMM, SO_RFCOMM_LM, 0);
	(void)rfcomm_getopt(so->so_pcb, &sopt);
	(void)sockopt_getint(&sopt, &mode);
	sockopt_destroy(&sopt);

	if (((mode & RFCOMM_LM_AUTH) && !(new & RFCOMM_LM_AUTH))
	    || ((mode & RFCOMM_LM_ENCRYPT) && !(new & RFCOMM_LM_ENCRYPT))
	    || ((mode & RFCOMM_LM_SECURE) && !(new & RFCOMM_LM_SECURE)))
		rfcomm_disconnect(so->so_pcb, 0);
}

/*
 * rfcomm_input(rfcomm_dlc, mbuf)
 */
static void
rfcomm_input(void *arg, struct mbuf *m)
{
	struct socket *so = arg;

	KASSERT(so != NULL);

	if (m->m_pkthdr.len > sbspace(&so->so_rcv)) {
		printf("%s: %d bytes dropped (socket buffer full)\n",
			__func__, m->m_pkthdr.len);
		m_freem(m);
		return;
	}

	DPRINTFN(10, "received %d bytes\n", m->m_pkthdr.len);

	sbappendstream(&so->so_rcv, m);
	sorwakeup(so);
}

PR_WRAP_USRREQ(rfcomm_usrreq)

#define	rfcomm_usrreq		rfcomm_usrreq_wrapper

const struct pr_usrreqs rfcomm_usrreqs = {
	.pr_attach	= rfcomm_attach1,
	.pr_detach	= rfcomm_detach1,
	.pr_generic	= rfcomm_usrreq,
};
