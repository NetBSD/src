/*	$NetBSD: l2cap_socket.c,v 1.25 2014/07/24 15:12:03 rtr Exp $	*/

/*-
 * Copyright (c) 2005 Iain Hibbert.
 * Copyright (c) 2006 Itronix Inc.
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
__KERNEL_RCSID(0, "$NetBSD: l2cap_socket.c,v 1.25 2014/07/24 15:12:03 rtr Exp $");

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
#include <netbt/l2cap.h>

/*
 * L2CAP Sockets
 *
 *	SOCK_SEQPACKET - normal L2CAP connection
 *
 *	SOCK_DGRAM - connectionless L2CAP - XXX not yet
 */

static void l2cap_connecting(void *);
static void l2cap_connected(void *);
static void l2cap_disconnected(void *, int);
static void *l2cap_newconn(void *, struct sockaddr_bt *, struct sockaddr_bt *);
static void l2cap_complete(void *, int);
static void l2cap_linkmode(void *, int);
static void l2cap_input(void *, struct mbuf *);

static const struct btproto l2cap_proto = {
	l2cap_connecting,
	l2cap_connected,
	l2cap_disconnected,
	l2cap_newconn,
	l2cap_complete,
	l2cap_linkmode,
	l2cap_input,
};

/* sysctl variables */
int l2cap_sendspace = 4096;
int l2cap_recvspace = 4096;

static int
l2cap_attach(struct socket *so, int proto)
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
	 * For L2CAP socket PCB we just use an l2cap_channel structure
	 * since we have nothing to add..
	 */
	error = soreserve(so, l2cap_sendspace, l2cap_recvspace);
	if (error)
		return error;

	return l2cap_attach_pcb((struct l2cap_channel **)&so->so_pcb,
				&l2cap_proto, so);
}

static void
l2cap_detach(struct socket *so)
{
	KASSERT(so->so_pcb != NULL);
	l2cap_detach_pcb((struct l2cap_channel **)&so->so_pcb);
	KASSERT(so->so_pcb == NULL);
}

static int
l2cap_accept(struct socket *so, struct mbuf *nam)
{
	struct l2cap_channel *pcb = so->so_pcb;
	struct sockaddr_bt *sa;

	KASSERT(solocked(so));
	KASSERT(nam != NULL);

	if (pcb == NULL)
		return EINVAL;

	sa = mtod(nam, struct sockaddr_bt *);
	nam->m_len = sizeof(struct sockaddr_bt);
	return l2cap_peeraddr_pcb(pcb, sa);
}

static int
l2cap_bind(struct socket *so, struct mbuf *nam)
{
	struct l2cap_channel *pcb = so->so_pcb;
	struct sockaddr_bt *sa;

	KASSERT(solocked(so));
	KASSERT(nam != NULL);

	if (pcb == NULL)
		return EINVAL;

	sa = mtod(nam, struct sockaddr_bt *);
	if (sa->bt_len != sizeof(struct sockaddr_bt))
		return EINVAL;

	if (sa->bt_family != AF_BLUETOOTH)
		return EAFNOSUPPORT;

	return l2cap_bind_pcb(pcb, sa);
}

static int
l2cap_listen(struct socket *so)
{
	struct l2cap_channel *pcb = so->so_pcb;

	KASSERT(solocked(so));

	if (pcb == NULL)
		return EINVAL;

	return l2cap_listen_pcb(pcb);
}

static int
l2cap_ioctl(struct socket *so, u_long cmd, void *nam, struct ifnet *ifp)
{
	return EPASSTHROUGH;
}

static int
l2cap_stat(struct socket *so, struct stat *ub)
{
	KASSERT(solocked(so));

	return 0;
}

static int
l2cap_peeraddr(struct socket *so, struct mbuf *nam)
{
	struct l2cap_channel *pcb = so->so_pcb;
	struct sockaddr_bt *sa;

	KASSERT(solocked(so));
	KASSERT(pcb != NULL);
	KASSERT(nam != NULL);

	sa = mtod(nam, struct sockaddr_bt *);
	nam->m_len = sizeof(struct sockaddr_bt);
	return l2cap_peeraddr_pcb(pcb, sa);
}

static int
l2cap_sockaddr(struct socket *so, struct mbuf *nam)
{
	struct l2cap_channel *pcb = so->so_pcb;
	struct sockaddr_bt *sa;

	KASSERT(solocked(so));
	KASSERT(pcb != NULL);
	KASSERT(nam != NULL);

	sa = mtod(nam, struct sockaddr_bt *);
	nam->m_len = sizeof(struct sockaddr_bt);
	return l2cap_sockaddr_pcb(pcb, sa);
}

static int
l2cap_recvoob(struct socket *so, struct mbuf *m, int flags)
{
	KASSERT(solocked(so));

	return EOPNOTSUPP;
}

static int
l2cap_sendoob(struct socket *so, struct mbuf *m, struct mbuf *control)
{
	KASSERT(solocked(so));

	if (m)
		m_freem(m);
	if (control)
		m_freem(control);

	return EOPNOTSUPP;
}

/*
 * User Request.
 * up is socket
 * m is optional mbuf chain containing message
 * nam is either
 *	optional mbuf chain containing an address
 *	message flags (PRU_RCVD)
 * ctl is either
 *	optional mbuf chain containing socket options
 *	optional interface pointer PRU_PURGEIF
 * l is pointer to process requesting action (if any)
 *
 * we are responsible for disposing of m and ctl if
 * they are mbuf chains
 */
static int
l2cap_usrreq(struct socket *up, int req, struct mbuf *m,
    struct mbuf *nam, struct mbuf *ctl, struct lwp *l)
{
	struct l2cap_channel *pcb = up->so_pcb;
	struct sockaddr_bt *sa;
	struct mbuf *m0;
	int err = 0;

	DPRINTFN(2, "%s\n", prurequests[req]);
	KASSERT(req != PRU_ATTACH);
	KASSERT(req != PRU_DETACH);
	KASSERT(req != PRU_ACCEPT);
	KASSERT(req != PRU_BIND);
	KASSERT(req != PRU_LISTEN);
	KASSERT(req != PRU_CONTROL);
	KASSERT(req != PRU_SENSE);
	KASSERT(req != PRU_PEERADDR);
	KASSERT(req != PRU_SOCKADDR);
	KASSERT(req != PRU_RCVOOB);
	KASSERT(req != PRU_SENDOOB);

	switch (req) {
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
		return l2cap_disconnect(pcb, up->so_linger);

	case PRU_ABORT:
		l2cap_disconnect(pcb, 0);
		soisdisconnected(up);
		l2cap_detach(up);
		return 0;

	case PRU_CONNECT:
		KASSERT(nam != NULL);
		sa = mtod(nam, struct sockaddr_bt *);

		if (sa->bt_len != sizeof(struct sockaddr_bt))
			return EINVAL;

		if (sa->bt_family != AF_BLUETOOTH)
			return EAFNOSUPPORT;

		soisconnecting(up);
		return l2cap_connect(pcb, sa);

	case PRU_SHUTDOWN:
		socantsendmore(up);
		break;

	case PRU_SEND:
		KASSERT(m != NULL);
		if (m->m_pkthdr.len == 0)
			break;

		if (m->m_pkthdr.len > pcb->lc_omtu) {
			err = EMSGSIZE;
			break;
		}

		m0 = m_copypacket(m, M_DONTWAIT);
		if (m0 == NULL) {
			err = ENOMEM;
			break;
		}

		if (ctl)	/* no use for that */
			m_freem(ctl);

		sbappendrecord(&up->so_snd, m);
		return l2cap_send(pcb, m0);

	case PRU_RCVD:
		return EOPNOTSUPP;	/* (no release) */

	case PRU_CONNECT2:
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
 * l2cap_ctloutput(req, socket, sockopt)
 *
 *	Apply configuration commands to channel. This corresponds to
 *	"Reconfigure Channel Request" in the L2CAP specification.
 */
int
l2cap_ctloutput(int req, struct socket *so, struct sockopt *sopt)
{
	struct l2cap_channel *pcb = so->so_pcb;
	int err = 0;

	DPRINTFN(2, "%s\n", prcorequests[req]);

	if (pcb == NULL)
		return EINVAL;

	if (sopt->sopt_level != BTPROTO_L2CAP)
		return ENOPROTOOPT;

	switch(req) {
	case PRCO_GETOPT:
		err = l2cap_getopt(pcb, sopt);
		break;

	case PRCO_SETOPT:
		err = l2cap_setopt(pcb, sopt);
		break;

	default:
		err = ENOPROTOOPT;
		break;
	}

	return err;
}

/**********************************************************************
 *
 *	L2CAP Protocol socket callbacks
 *
 */

static void
l2cap_connecting(void *arg)
{
	struct socket *so = arg;

	DPRINTF("Connecting\n");
	soisconnecting(so);
}

static void
l2cap_connected(void *arg)
{
	struct socket *so = arg;

	DPRINTF("Connected\n");
	soisconnected(so);
}

static void
l2cap_disconnected(void *arg, int err)
{
	struct socket *so = arg;

	DPRINTF("Disconnected (%d)\n", err);

	so->so_error = err;
	soisdisconnected(so);
}

static void *
l2cap_newconn(void *arg, struct sockaddr_bt *laddr,
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

static void
l2cap_complete(void *arg, int count)
{
	struct socket *so = arg;

	while (count-- > 0)
		sbdroprecord(&so->so_snd);

	sowwakeup(so);
}

static void
l2cap_linkmode(void *arg, int new)
{
	struct socket *so = arg;
	struct sockopt sopt;
	int mode;

	DPRINTF("auth %s, encrypt %s, secure %s\n",
		(new & L2CAP_LM_AUTH ? "on" : "off"),
		(new & L2CAP_LM_ENCRYPT ? "on" : "off"),
		(new & L2CAP_LM_SECURE ? "on" : "off"));

	sockopt_init(&sopt, BTPROTO_L2CAP, SO_L2CAP_LM, 0);
	(void)l2cap_getopt(so->so_pcb, &sopt);
	(void)sockopt_getint(&sopt, &mode);
	sockopt_destroy(&sopt);

	if (((mode & L2CAP_LM_AUTH) && !(new & L2CAP_LM_AUTH))
	    || ((mode & L2CAP_LM_ENCRYPT) && !(new & L2CAP_LM_ENCRYPT))
	    || ((mode & L2CAP_LM_SECURE) && !(new & L2CAP_LM_SECURE)))
		l2cap_disconnect(so->so_pcb, 0);
}

static void
l2cap_input(void *arg, struct mbuf *m)
{
	struct socket *so = arg;

	if (m->m_pkthdr.len > sbspace(&so->so_rcv)) {
		printf("%s: packet (%d bytes) dropped (socket buffer full)\n",
			__func__, m->m_pkthdr.len);
		m_freem(m);
		return;
	}

	DPRINTFN(10, "received %d bytes\n", m->m_pkthdr.len);

	sbappendrecord(&so->so_rcv, m);
	sorwakeup(so);
}

PR_WRAP_USRREQS(l2cap)

#define	l2cap_attach		l2cap_attach_wrapper
#define	l2cap_detach		l2cap_detach_wrapper
#define	l2cap_accept		l2cap_accept_wrapper
#define	l2cap_bind		l2cap_bind_wrapper
#define	l2cap_listen		l2cap_listen_wrapper
#define	l2cap_ioctl		l2cap_ioctl_wrapper
#define	l2cap_stat		l2cap_stat_wrapper
#define	l2cap_peeraddr		l2cap_peeraddr_wrapper
#define	l2cap_sockaddr		l2cap_sockaddr_wrapper
#define	l2cap_recvoob		l2cap_recvoob_wrapper
#define	l2cap_sendoob		l2cap_sendoob_wrapper
#define	l2cap_usrreq		l2cap_usrreq_wrapper

const struct pr_usrreqs l2cap_usrreqs = {
	.pr_attach	= l2cap_attach,
	.pr_detach	= l2cap_detach,
	.pr_accept	= l2cap_accept,
	.pr_bind	= l2cap_bind,
	.pr_listen	= l2cap_listen,
	.pr_ioctl	= l2cap_ioctl,
	.pr_stat	= l2cap_stat,
	.pr_peeraddr	= l2cap_peeraddr,
	.pr_sockaddr	= l2cap_sockaddr,
	.pr_recvoob	= l2cap_recvoob,
	.pr_sendoob	= l2cap_sendoob,
	.pr_generic	= l2cap_usrreq,
};
