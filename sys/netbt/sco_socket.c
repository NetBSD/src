/*	$NetBSD: sco_socket.c,v 1.28 2014/07/30 10:04:26 rtr Exp $	*/

/*-
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
__KERNEL_RCSID(0, "$NetBSD: sco_socket.c,v 1.28 2014/07/30 10:04:26 rtr Exp $");

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
#include <netbt/hci.h>
#include <netbt/sco.h>

/*******************************************************************************
 *
 * SCO SOCK_SEQPACKET sockets - low latency audio data
 */

static void sco_connecting(void *);
static void sco_connected(void *);
static void sco_disconnected(void *, int);
static void *sco_newconn(void *, struct sockaddr_bt *, struct sockaddr_bt *);
static void sco_complete(void *, int);
static void sco_linkmode(void *, int);
static void sco_input(void *, struct mbuf *);

static const struct btproto sco_proto = {
	sco_connecting,
	sco_connected,
	sco_disconnected,
	sco_newconn,
	sco_complete,
	sco_linkmode,
	sco_input,
};

int sco_sendspace = 4096;
int sco_recvspace = 4096;

static int
sco_attach(struct socket *so, int proto)
{
	int error;

	KASSERT(so->so_pcb == NULL);

	if (so->so_lock == NULL) {
		mutex_obj_hold(bt_lock);
		so->so_lock = bt_lock;
		solock(so);
	}
	KASSERT(solocked(so));

	error = soreserve(so, sco_sendspace, sco_recvspace);
	if (error) {
		return error;
	}
	return sco_attach_pcb((struct sco_pcb **)&so->so_pcb, &sco_proto, so);
}

static void
sco_detach(struct socket *so)
{
	KASSERT(so->so_pcb != NULL);
	sco_detach_pcb((struct sco_pcb **)&so->so_pcb);
	KASSERT(so->so_pcb == NULL);
}

static int
sco_accept(struct socket *so, struct mbuf *nam)
{
	struct sco_pcb *pcb = so->so_pcb;
	struct sockaddr_bt *sa;

	KASSERT(solocked(so));
	KASSERT(nam != NULL);

	if (pcb == NULL)
		return EINVAL;

	sa = mtod(nam, struct sockaddr_bt *);
	nam->m_len = sizeof(struct sockaddr_bt);
	return sco_peeraddr_pcb(pcb, sa);
}

static int
sco_bind(struct socket *so, struct mbuf *nam)
{
	struct sco_pcb *pcb = so->so_pcb;
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

	return sco_bind_pcb(pcb, sa);
}

static int
sco_listen(struct socket *so)
{
	struct sco_pcb *pcb = so->so_pcb;

	KASSERT(solocked(so));

	if (pcb == NULL)
		return EINVAL;

	return sco_listen_pcb(pcb);
}

static int
sco_connect(struct socket *so, struct mbuf *nam)
{
	struct sco_pcb *pcb = so->so_pcb;
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

	soisconnecting(so);
	return sco_connect_pcb(pcb, sa);
}

static int
sco_ioctl(struct socket *so, u_long cmd, void *nam, struct ifnet *ifp)
{
	return EOPNOTSUPP;
}

static int
sco_stat(struct socket *so, struct stat *ub)
{
	KASSERT(solocked(so));

	return 0;
}

static int
sco_peeraddr(struct socket *so, struct mbuf *nam)
{
	struct sco_pcb *pcb = (struct sco_pcb *)so->so_pcb;
	struct sockaddr_bt *sa;

	KASSERT(solocked(so));
	KASSERT(pcb != NULL);
	KASSERT(nam != NULL);

	sa = mtod(nam, struct sockaddr_bt *);
	nam->m_len = sizeof(struct sockaddr_bt);
	return sco_peeraddr_pcb(pcb, sa);
}

static int
sco_sockaddr(struct socket *so, struct mbuf *nam)
{
	struct sco_pcb *pcb = (struct sco_pcb *)so->so_pcb;
	struct sockaddr_bt *sa;

	KASSERT(solocked(so));
	KASSERT(pcb != NULL);
	KASSERT(nam != NULL);

	sa = mtod(nam, struct sockaddr_bt *);
	nam->m_len = sizeof(struct sockaddr_bt);
	return sco_sockaddr_pcb(pcb, sa);
}

static int
sco_recvoob(struct socket *so, struct mbuf *m, int flags)
{
	KASSERT(solocked(so));

	return EOPNOTSUPP;
}

static int
sco_sendoob(struct socket *so, struct mbuf *m, struct mbuf *control)
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
 * nam is optional mbuf chain containing an address
 * ctl is optional mbuf chain containing socket options
 * l is pointer to process requesting action (if any)
 *
 * we are responsible for disposing of m and ctl if
 * they are mbuf chains
 */
static int
sco_usrreq(struct socket *up, int req, struct mbuf *m,
    struct mbuf *nam, struct mbuf *ctl, struct lwp *l)
{
	struct sco_pcb *pcb = (struct sco_pcb *)up->so_pcb;
	struct mbuf *m0;
	int err = 0;

	DPRINTFN(2, "%s\n", prurequests[req]);
	KASSERT(req != PRU_ATTACH);
	KASSERT(req != PRU_DETACH);
	KASSERT(req != PRU_ACCEPT);
	KASSERT(req != PRU_BIND);
	KASSERT(req != PRU_LISTEN);
	KASSERT(req != PRU_CONNECT);
	KASSERT(req != PRU_CONTROL);
	KASSERT(req != PRU_SENSE);
	KASSERT(req != PRU_PEERADDR);
	KASSERT(req != PRU_SOCKADDR);
	KASSERT(req != PRU_RCVOOB);
	KASSERT(req != PRU_SENDOOB);

	switch(req) {
	case PRU_PURGEIF:
		return EOPNOTSUPP;
	}

	/* anything after here *requires* a pcb */
	if (pcb == NULL) {
		err = EINVAL;
		goto release;
	}

	switch(req) {
	case PRU_DISCONNECT:
		soisdisconnecting(up);
		return sco_disconnect(pcb, up->so_linger);

	case PRU_ABORT:
		sco_disconnect(pcb, 0);
		soisdisconnected(up);
		sco_detach(up);
		return 0;

	case PRU_SHUTDOWN:
		socantsendmore(up);
		break;

	case PRU_SEND:
		KASSERT(m != NULL);
		if (m->m_pkthdr.len == 0)
			break;

		if (m->m_pkthdr.len > pcb->sp_mtu) {
			err = EMSGSIZE;
			break;
		}

		m0 = m_copypacket(m, M_DONTWAIT);
		if (m0 == NULL) {
			err = ENOMEM;
			break;
		}

		if (ctl) /* no use for that */
			m_freem(ctl);

		sbappendrecord(&up->so_snd, m);
		return sco_send(pcb, m0);

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
 * get/set socket options
 */
int
sco_ctloutput(int req, struct socket *so, struct sockopt *sopt)
{
	struct sco_pcb *pcb = (struct sco_pcb *)so->so_pcb;
	int err = 0;

	DPRINTFN(2, "req %s\n", prcorequests[req]);

	if (pcb == NULL)
		return EINVAL;

	if (sopt->sopt_level != BTPROTO_SCO)
		return ENOPROTOOPT;

	switch(req) {
	case PRCO_GETOPT:
		err = sco_getopt(pcb, sopt);
		break;

	case PRCO_SETOPT:
		err = sco_setopt(pcb, sopt);
		break;

	default:
		err = ENOPROTOOPT;
		break;
	}

	return err;
}

/*****************************************************************************
 *
 *	SCO Protocol socket callbacks
 *
 */
static void
sco_connecting(void *arg)
{
	struct socket *so = arg;

	DPRINTF("Connecting\n");
	soisconnecting(so);
}

static void
sco_connected(void *arg)
{
	struct socket *so = arg;

	DPRINTF("Connected\n");
	soisconnected(so);
}

static void
sco_disconnected(void *arg, int err)
{
	struct socket *so = arg;

	DPRINTF("Disconnected (%d)\n", err);

	so->so_error = err;
	soisdisconnected(so);
}

static void *
sco_newconn(void *arg, struct sockaddr_bt *laddr,
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
sco_complete(void *arg, int num)
{
	struct socket *so = arg;

	while (num-- > 0)
		sbdroprecord(&so->so_snd);

	sowwakeup(so);
}

static void
sco_linkmode(void *arg, int mode)
{
}

static void
sco_input(void *arg, struct mbuf *m)
{
	struct socket *so = arg;

	/*
	 * since this data is time sensitive, if the buffer
	 * is full we just dump data until the latest one
	 * will fit.
	 */

	while (m->m_pkthdr.len > sbspace(&so->so_rcv))
		sbdroprecord(&so->so_rcv);

	DPRINTFN(10, "received %d bytes\n", m->m_pkthdr.len);

	sbappendrecord(&so->so_rcv, m);
	sorwakeup(so);
}

PR_WRAP_USRREQS(sco)

#define	sco_attach		sco_attach_wrapper
#define	sco_detach		sco_detach_wrapper
#define	sco_accept		sco_accept_wrapper
#define	sco_bind		sco_bind_wrapper
#define	sco_listen		sco_listen_wrapper
#define	sco_connect		sco_connect_wrapper
#define	sco_ioctl		sco_ioctl_wrapper
#define	sco_stat		sco_stat_wrapper
#define	sco_peeraddr		sco_peeraddr_wrapper
#define	sco_sockaddr		sco_sockaddr_wrapper
#define	sco_recvoob		sco_recvoob_wrapper
#define	sco_sendoob		sco_sendoob_wrapper
#define	sco_usrreq		sco_usrreq_wrapper

const struct pr_usrreqs sco_usrreqs = {
	.pr_attach	= sco_attach,
	.pr_detach	= sco_detach,
	.pr_accept	= sco_accept,
	.pr_bind	= sco_bind,
	.pr_listen	= sco_listen,
	.pr_connect	= sco_connect,
	.pr_ioctl	= sco_ioctl,
	.pr_stat	= sco_stat,
	.pr_peeraddr	= sco_peeraddr,
	.pr_sockaddr	= sco_sockaddr,
	.pr_recvoob	= sco_recvoob,
	.pr_sendoob	= sco_sendoob,
	.pr_generic	= sco_usrreq,
};
