/*	$NetBSD: can.c,v 1.1.2.4 2017/02/05 11:45:11 bouyer Exp $	*/

/*-
 * Copyright (c) 2003, 2017 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Robert Swindells and Manuel Bouyer
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: can.c,v 1.1.2.4 2017/02/05 11:45:11 bouyer Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/mbuf.h>
#include <sys/ioctl.h>
#include <sys/domain.h>
#include <sys/protosw.h>
#include <sys/errno.h>
#include <sys/socket.h>
#include <sys/socketvar.h>
#include <sys/proc.h>
#include <sys/kauth.h>

#include <net/if.h>
#include <net/netisr.h>
#include <net/route.h>

#include <netcan/can.h>
#include <netcan/can_pcb.h>
#include <netcan/can_var.h>

struct canpcb canpcb;
#if 0
struct canpcb canrawpcb;
#endif

struct	canpcbtable cbtable;

struct ifqueue	canintrq;
int	canqmaxlen = IFQ_MAXLEN;

int can_copy_output = 0;
int can_output_cnt = 0;
struct mbuf *can_lastout;

int	can_sendspace = 4096;		/* really max datagram size */
int	can_recvspace = 40 * (1024 + sizeof(struct sockaddr_can));
					/* 40 1K datagrams */
#ifndef CANHASHSIZE
#define	CANHASHSIZE	128
#endif
int	canhashsize = CANHASHSIZE;

static int can_output(struct mbuf *, struct canpcb *);

static int can_control(struct socket *, u_long, void *, struct ifnet *);

void
can_init(void)
{
	canintrq.ifq_maxlen = canqmaxlen;
	IFQ_LOCK_INIT(&canintrq);
	can_pcbinit(&cbtable, canhashsize, canhashsize);
}

/*
 * Generic control operations (ioctl's).
 */
/* ARGSUSED */
static int
can_control(struct socket *so, u_long cmd, void *data, struct ifnet *ifp)
{
#if 0
	struct can_ifreq *cfr = (struct can_ifreq *)data;
	int error = 0;
#endif


	switch (cmd) {

	default:
		if (ifp == 0 || ifp->if_ioctl == 0)
			return (EOPNOTSUPP);
		return ((*ifp->if_ioctl)(ifp, cmd, data));
	}
	return (0);
}

static int
can_purgeif(struct socket *so, struct ifnet *ifp)
{
	return 0;
}

static int
can_output(struct mbuf *m, struct canpcb *canp)
{
	struct ifnet *ifp;
	int error = 0;
	struct m_tag *sotag;

	if (canp == NULL) {
		printf("can_output: no pcb\n");
		error = EINVAL;
		return error;
	}
	ifp = canp->canp_ifp;
	if (ifp == 0) {
		error = EDESTADDRREQ;
		goto bad;
	}
	sotag = m_tag_get(PACKET_TAG_SO, sizeof(struct socket *), PR_NOWAIT);
	if (sotag == NULL) {
		ifp->if_oerrors++;
		error = ENOMEM;
		goto bad;
	}
	*(struct socket **)(sotag + 1) = canp->canp_socket;
	m_tag_prepend(m, sotag);

	if (m->m_len <= ifp->if_mtu) {
		can_output_cnt++;
		error = (*ifp->if_output)(ifp, m, NULL, 0);
		return error;
	} else
		error = EMSGSIZE;
bad:
	m_freem(m);
	return (error);
}

/*
 * cleanup mbuf tag, keeping the PACKET_TAG_SO tag
 */
void
can_mbuf_tag_clean(struct mbuf *m)
{
	struct m_tag *sotag;

	sotag = m_tag_find(m, PACKET_TAG_SO, NULL);
	m_tag_delete_nonpersistent(m);
	if (sotag)
		m_tag_prepend(m, sotag);
}

/*
 * Process a received CAN frame
 * the packet is in the mbuf chain m with
 * the CAN header.
 */
void
can_input(struct ifnet *ifp, struct mbuf *m)
{
	struct ifqueue *inq;

	if ((ifp->if_flags & IFF_UP) == 0) {
		m_freem(m);
		return;
	}

	inq = &canintrq;
	
	IFQ_LOCK(inq);
	if (IF_QFULL(inq)) {
		IF_DROP(inq);
		IFQ_UNLOCK(inq);
		m_freem(m);
	} else {
		IF_ENQUEUE(inq, m);
		IFQ_UNLOCK(inq);
		schednetisr(NETISR_CAN);
		ifp->if_ipackets++;
		ifp->if_ibytes += m->m_pkthdr.len;
	}
}

void
canintr(void)
{
	int		rcv_ifindex;
	struct mbuf    *m;

	struct sockaddr_can from;
	struct canpcb   *canp;
	struct m_tag	*sotag;
	struct socket	*so;
	struct canpcb	*sender_canp;

	mutex_enter(softnet_lock);
	for (;;) {
		IFQ_LOCK(&canintrq);
		IF_DEQUEUE(&canintrq, m);
		IFQ_UNLOCK(&canintrq);

		if (m == NULL)	/* no more queued packets */
			break;

#if 0
		m_claim(m, &can_rx_mowner);
#endif
		sotag = m_tag_find(m, PACKET_TAG_SO, NULL);
		if (sotag) {
			so = *(struct socket **)(sotag + 1);
			sender_canp = sotocanpcb(so);
			m_tag_delete(m, sotag);
			/* if the sender doesn't want loopback, don't do it */
			if (sender_canp->canp_flags & CANP_NO_LOOPBACK) {
				m_freem(m);
				continue;
			}
		} else {
			sender_canp = NULL;
		}
		memset(&from, 0, sizeof(struct sockaddr_can));
		rcv_ifindex = m->m_pkthdr.rcvif_index;
		from.can_ifindex = rcv_ifindex;
		from.can_len = sizeof(struct sockaddr_can);
		from.can_family = AF_CAN;

		TAILQ_FOREACH(canp, &cbtable.canpt_queue, canp_queue) {
			struct mbuf *mc;

			/* don't loop back to sockets on other interfaces */
			if (canp->canp_ifp != NULL &&
			    canp->canp_ifp->if_index != rcv_ifindex) {
				continue;
			}
			/* don't loop back to myself if I don't want it */
			if (canp == sender_canp &&
			    (canp->canp_flags & CANP_RECEIVE_OWN) == 0)
				continue;

			/* skip if the accept filter doen't match this pkt */
			if (!can_pcbfilter(canp, m))
				continue;

			if (TAILQ_NEXT(canp, canp_queue) != NULL) {
				/*
				 * we can't be sure we won't need 
				 * the original mbuf later so copy 
				 */
				mc = m_copym(m, 0, M_COPYALL, M_NOWAIT);
				if (mc == NULL) {
					/* deliver this mbuf and abort */
					mc = m;
					m = NULL;
				}
			} else {
				mc = m;
				m = NULL;
			}
			if (sbappendaddr(&canp->canp_socket->so_rcv,
					 (struct sockaddr *) &from, mc,
					 (struct mbuf *) 0) == 0) {
				m_freem(mc);
			} else
				sorwakeup(canp->canp_socket);
			if (m == NULL)
				break;
		}
		/* If it didn't go anywhere just delete it */
		if (m) {
			m_freem(m);
		}
	}
	mutex_exit(softnet_lock);
}

static int
can_attach(struct socket *so, int proto)
{
	int error;

	KASSERT(sotocanpcb(so) == NULL);

	/* Assign the lock (must happen even if we will error out). */
	sosetlock(so);

#ifdef MBUFTRACE
	so->so_mowner = &can_mowner;
	so->so_rcv.sb_mowner = &can_rx_mowner;
	so->so_snd.sb_mowner = &can_tx_mowner;
#endif
	if (so->so_snd.sb_hiwat == 0 || so->so_rcv.sb_hiwat == 0) {
		error = soreserve(so, can_sendspace, can_recvspace);
		if (error) {
			return error;
		}
	}

	error = can_pcballoc(so, &cbtable);
	if (error) {
		return error;
	}
	KASSERT(solocked(so));

	return error;
}

static void
can_detach(struct socket *so)
{
	struct canpcb *canp;

	KASSERT(solocked(so));
	canp = sotocanpcb(so);
	can_pcbdetach(canp);
}

static int
can_accept(struct socket *so, struct sockaddr *nam)
{
	KASSERT(solocked(so));

	panic("can_accept");

	return EOPNOTSUPP;
}

static int
can_bind(struct socket *so, struct sockaddr *nam, struct lwp *l)
{
	struct canpcb *canp = sotocanpcb(so);
	struct sockaddr_can *scan = (struct sockaddr_can *)nam;

	KASSERT(solocked(so));
	KASSERT(nam != NULL);

	return can_pcbbind(canp, scan, l);
}

static int
can_listen(struct socket *so, struct lwp *l)
{
	KASSERT(solocked(so));

	return EOPNOTSUPP;
}

static int
can_connect(struct socket *so, struct sockaddr *nam, struct lwp *l)
{
	struct canpcb *canp = sotocanpcb(so);
	int error = 0;

	KASSERT(solocked(so));
	KASSERT(canp != NULL);
	KASSERT(nam != NULL);

	error = can_pcbconnect(canp, (struct sockaddr_can *)nam);
	if (! error)
		soisconnected(so);
	return error;
}

static int
can_connect2(struct socket *so, struct socket *so2)
{
	KASSERT(solocked(so));

	return EOPNOTSUPP;
}

static int
can_disconnect(struct socket *so)
{
	struct canpcb *canp = sotocanpcb(so);

	KASSERT(solocked(so));
	KASSERT(canp != NULL);

	/*soisdisconnected(so);*/
	so->so_state &= ~SS_ISCONNECTED;	/* XXX */
	can_pcbdisconnect(canp);
	can_pcbstate(canp, CANP_BOUND);		/* XXX */
	return 0;
}

static int
can_shutdown(struct socket *so)
{
	KASSERT(solocked(so));

	socantsendmore(so);
	return 0;
}

static int
can_abort(struct socket *so)
{
	KASSERT(solocked(so));

	panic("can_abort");

	return EOPNOTSUPP;
}

static int
can_ioctl(struct socket *so, u_long cmd, void *nam, struct ifnet *ifp)
{
	return can_control(so, cmd, nam, ifp);
}

static int
can_stat(struct socket *so, struct stat *ub)
{
	KASSERT(solocked(so));

	/* stat: don't bother with a blocksize. */
	return 0;
}

static int
can_peeraddr(struct socket *so, struct sockaddr *nam)
{
	KASSERT(solocked(so));
	KASSERT(sotocanpcb(so) != NULL);
	KASSERT(nam != NULL);

	return EOPNOTSUPP;
}

static int
can_sockaddr(struct socket *so, struct sockaddr *nam)
{
	KASSERT(solocked(so));
	KASSERT(sotocanpcb(so) != NULL);
	KASSERT(nam != NULL);

	can_setsockaddr(sotocanpcb(so), (struct sockaddr_can *)nam);

	return 0;
}

static int
can_rcvd(struct socket *so, int flags, struct lwp *l)
{
	KASSERT(solocked(so));

	return EOPNOTSUPP;
}

static int
can_recvoob(struct socket *so, struct mbuf *m, int flags)
{
	KASSERT(solocked(so));

	return EOPNOTSUPP;
}

static int
can_send(struct socket *so, struct mbuf *m, struct sockaddr *nam,
    struct mbuf *control, struct lwp *l)
{
	struct canpcb *canp = sotocanpcb(so);
	int error = 0;
	int s;

	if (control && control->m_len) {
		return EINVAL;
	}

	if (nam) {
		if ((so->so_state & SS_ISCONNECTED) != 0) {
			return EISCONN;
		}
		s = splnet();
		error = can_pcbbind(canp, (struct sockaddr_can *)nam, l);
		if (error) {
			splx(s);
			return error;
		}
	} else {
		if ((so->so_state & SS_ISCONNECTED) == 0) {
			return EDESTADDRREQ;
		}
	}
	error = can_output(m, canp);
	if (nam) {
		struct sockaddr_can lscan;
		memset(&lscan, 0, sizeof(lscan));
		lscan.can_family = AF_CAN;
		lscan.can_len = sizeof(lscan);
		can_pcbbind(canp, &lscan, l);
	}
	return error;
}

static int
can_sendoob(struct socket *so, struct mbuf *m, struct mbuf *control)
{
	KASSERT(solocked(so));

	m_freem(m);
	m_freem(control);

	return EOPNOTSUPP;
}

#if 0
int
can_usrreq(struct socket *so, int req, struct mbuf *m, struct mbuf *nam,
	   struct mbuf *control, struct lwp *l)
{
	struct canpcb *canp;
	int s;
	int error = 0;

	if (req == PRU_CONTROL)
		 return (can_control(so, (long)m, nam,
		     (struct ifnet *)control));

	if (req == PRU_PURGEIF) {
#if 0
		can_pcbpurgeif0(&udbtable, (struct ifnet *)control);
		can_purgeif((struct ifnet *)control);
		can_pcbpurgeif(&udbtable, (struct ifnet *)control);
#endif
		return (0);
	}

	s = splsoftnet();
	canp = sotocanpcb(so);
#ifdef DIAGNOSTIC
	if (req != PRU_SEND && req != PRU_SENDOOB && control)
		panic("can_usrreq: unexpected control mbuf");
#endif
	if (canp == 0 && req != PRU_ATTACH) {
		printf("can_usrreq: no pcb %p %d\n", canp, req);
		error = EINVAL;
		goto release;
	}

	/*
	 * Note: need to block can_input while changing
	 * the can pcb queue and/or pcb addresses.
	 */
	switch (req) {

	  case PRU_ATTACH:
	      if (canp != 0) {
			 error = EISCONN;
			 break;
		 }
#ifdef MBUFTRACE
		so->so_mowner = &can_mowner;
		so->so_rcv.sb_mowner = &can_rx_mowner;
		so->so_snd.sb_mowner = &can_tx_mowner;
#endif
		if (so->so_snd.sb_hiwat == 0 || so->so_rcv.sb_hiwat == 0) {
			error = soreserve(so, can_sendspace, can_recvspace);
			if (error)
				break;
		}
		error = can_pcballoc(so, &cbtable);
		if (error)
			break;
		canp = sotocanpcb(so);
#if 0
		inp->inp_ip.ip_ttl = ip_defttl;
#endif
		break;

	case PRU_DETACH:
		can_pcbdetach(canp);
		break;

	case PRU_BIND:
		error = can_pcbbind(canp, nam, l);
		break;

	case PRU_LISTEN:
		error = EOPNOTSUPP;
		break;

	case PRU_CONNECT:
		error = can_pcbconnect(canp, nam);
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
		can_pcbdisconnect(canp);
		can_pcbstate(canp, CANP_BOUND);		/* XXX */
		break;

	case PRU_SHUTDOWN:
		socantsendmore(so);
		break;

	case PRU_RCVD:
		error = EOPNOTSUPP;
		break;

	case PRU_SEND:
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

		break;

	case PRU_PEERADDR:
		error =  EOPNOTSUPP;
		break;

	default:
		panic("can_usrreq");
	}

release:
	splx(s);
	return (error);
}
#endif

#if 0
static void
can_notify(struct canpcb *canp, int errno)
{

	canp->canp_socket->so_error = errno;
	sorwakeup(canp->canp_socket);
	sowwakeup(canp->canp_socket);
}

void *
can_ctlinput(int cmd, struct sockaddr *sa, void *v)
{
	struct ip *ip = v;
	struct canhdr *uh;
	void (*notify) __P((struct inpcb *, int)) = can_notify;
	int errno;

	if (sa->sa_family != AF_CAN
	 || sa->sa_len != sizeof(struct sockaddr_can))
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
		uh = (struct canhdr *)((caddr_t)ip + (ip->ip_hl << 2));
		in_pcbnotify(&udbtable, satosin(sa)->sin_addr, uh->uh_dport,
		    ip->ip_src, uh->uh_sport, errno, notify);

		/* XXX mapped address case */
	} else
		can_pcbnotifyall(&cbtable, satoscan(sa)->scan_addr, errno,
		    notify);
	return NULL;
}
#endif

static int
can_raw_getop(struct canpcb *canp, struct sockopt *sopt)
{
	int optval = 0;
	int error;

	switch (sopt->sopt_name) {
	case CAN_RAW_LOOPBACK:
		optval = (canp->canp_flags & CANP_NO_LOOPBACK) ? 0 : 1;
		error = sockopt_set(sopt, &optval, sizeof(optval));
		break;
	case CAN_RAW_RECV_OWN_MSGS: 
		optval = (canp->canp_flags & CANP_RECEIVE_OWN) ? 1 : 0;
		error = sockopt_set(sopt, &optval, sizeof(optval));
		break;
	case CAN_RAW_FILTER:
		error = sockopt_set(sopt, canp->canp_filters,
		    sizeof(struct can_filter) * canp->canp_nfilters);
		break;
	default:
		error = ENOPROTOOPT;
		break;
	}
	return error;
}

static int
can_raw_setop(struct canpcb *canp, struct sockopt *sopt)
{
	int optval = 0;
	int error;

	switch (sopt->sopt_name) {
	case CAN_RAW_LOOPBACK:
		error = sockopt_getint(sopt, &optval);
		if (error == 0) {
			if (optval) {
				canp->canp_flags &= ~CANP_NO_LOOPBACK;
			} else {
				canp->canp_flags |= CANP_NO_LOOPBACK;
			}
		}
		break;
	case CAN_RAW_RECV_OWN_MSGS: 
		error = sockopt_getint(sopt, &optval);
		if (error == 0) {
			if (optval) {
				canp->canp_flags |= CANP_RECEIVE_OWN;
			} else {
				canp->canp_flags &= ~CANP_RECEIVE_OWN;
			}
		}
		break;
	case CAN_RAW_FILTER:
		{
		int nfilters = sopt->sopt_size / sizeof(struct can_filter);
		if (sopt->sopt_size % sizeof(struct can_filter) != 0)
			return EINVAL;
		error = can_pcbsetfilter(canp, sopt->sopt_data, nfilters);
		break;
		}
	default:
		error = ENOPROTOOPT;
		break;
	}
	return error;
}

/*
 * Called by getsockopt and setsockopt.
 *
 */
int
can_ctloutput(int op, struct socket *so, struct sockopt *sopt)
{
	struct canpcb *canp;
	int error;
	int s;

	if (so->so_proto->pr_domain->dom_family != PF_CAN)	
		return EAFNOSUPPORT;

	if (sopt->sopt_level != SOL_CAN_RAW)
		return EINVAL;

	s = splsoftnet();
	canp = sotocanpcb(so);
	if (canp == NULL) {
		splx(s);
		return ECONNRESET;
	}

	if (op == PRCO_SETOPT) {
		error = can_raw_setop(canp, sopt);
	} else if (op ==  PRCO_GETOPT) {
		error = can_raw_getop(canp, sopt);
	} else {
		error = EINVAL;
	}
	splx(s);
	return error;
}

PR_WRAP_USRREQS(can)
#define	can_attach	can_attach_wrapper
#define	can_detach	can_detach_wrapper
#define	can_accept	can_accept_wrapper
#define	can_bind	can_bind_wrapper
#define	can_listen	can_listen_wrapper
#define	can_connect	can_connect_wrapper
#define	can_connect2	can_connect2_wrapper
#define	can_disconnect	can_disconnect_wrapper
#define	can_shutdown	can_shutdown_wrapper
#define	can_abort	can_abort_wrapper
#define	can_ioctl	can_ioctl_wrapper
#define	can_stat	can_stat_wrapper
#define	can_peeraddr	can_peeraddr_wrapper
#define	can_sockaddr	can_sockaddr_wrapper
#define	can_rcvd	can_rcvd_wrapper
#define	can_recvoob	can_recvoob_wrapper
#define	can_send	can_send_wrapper
#define	can_sendoob	can_sendoob_wrapper
#define	can_purgeif	can_purgeif_wrapper

const struct pr_usrreqs can_usrreqs = {
	.pr_attach	= can_attach,
	.pr_detach	= can_detach,
	.pr_accept	= can_accept,
	.pr_bind	= can_bind,
	.pr_listen	= can_listen,
	.pr_connect	= can_connect,
	.pr_connect2	= can_connect2,
	.pr_disconnect	= can_disconnect,
	.pr_shutdown	= can_shutdown,
	.pr_abort	= can_abort,
	.pr_ioctl	= can_ioctl,
	.pr_stat	= can_stat,
	.pr_peeraddr	= can_peeraddr,
	.pr_sockaddr	= can_sockaddr,
	.pr_rcvd	= can_rcvd,
	.pr_recvoob	= can_recvoob,
	.pr_send	= can_send,
	.pr_sendoob	= can_sendoob,
	.pr_purgeif	= can_purgeif,
};
