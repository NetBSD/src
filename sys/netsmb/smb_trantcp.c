/*	$NetBSD: smb_trantcp.c,v 1.1 2000/12/07 03:48:10 deberg Exp $	*/

/*
 * Copyright (c) 2000, Boris Popov
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *    This product includes software developed by Boris Popov.
 * 4. Neither the name of the author nor the names of any co-contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/malloc.h>
#include <sys/mbuf.h>
#include <sys/proc.h>
#include <sys/protosw.h>
#include <sys/socket.h>
#include <sys/socketvar.h>
#include <sys/poll.h>
#include <sys/uio.h>
#include <uvm/uvm_extern.h>
#include <sys/sysctl.h>

#include <net/if.h>
#include <net/route.h>

#include <netinet/in.h>
#include <netinet/tcp.h>

#include <sys/subr_mbuf.h>

#include <netsmb/netbios.h>

#include <netsmb/smb.h>
#include <netsmb/smb_conn.h>
#include <netsmb/smb_tran.h>
#include <netsmb/smb_trantcp.h>
#include <netsmb/smb_subr.h>

#define M_NBDATA	M_PCB

static int smb_tcpsndbuf = 10 * 1024;
static int smb_tcprcvbuf = 10 * 1024;

#ifdef SYSCTL_DECL
SYSCTL_DECL(_net_smb);
#endif

#ifndef NetBSD
SYSCTL_INT(_net_smb, OID_AUTO, tcpsndbuf, CTLFLAG_RW, &smb_tcpsndbuf, 0, "");
SYSCTL_INT(_net_smb, OID_AUTO, tcprcvbuf, CTLFLAG_RW, &smb_tcprcvbuf, 0, "");
#endif

#ifndef NetBSD
#define nb_sosend(so,m,flags,p)	(so)->so_proto->pr_usrreqs->pru_sosend( \
				    so, NULL, 0, m, 0, flags, p)
#else
#if 0
#define nb_sosend(so,m,flags,p) (*(so)->so_proto->pr_usrreq)(so, PRU_SEND, \
				    m, (struct mbuf *)0, (struct mbuf *)0, p)
#endif
#define nb_sosend(so,m,flags,p) (*so->so_send)(so, NULL, (struct uio *)0, m, \
				    (struct mbuf *)0, flags)
#endif

static int  nbssn_recv(struct nbpcb *nbp, struct mbuf **mpp, int *lenp,
	u_int8_t *rpcodep, struct proc *p);
static int  smb_nbst_disconnect(struct smb_vc *vcp, struct proc *p);

static int
nb_setsockopt_int(struct socket *so, int level, int name, int val)
{
#ifndef NetBSD
	struct sockopt sopt;

	bzero(&sopt, sizeof(sopt));
	sopt.sopt_level = level;
	sopt.sopt_name = name;
	sopt.sopt_val = &val;
	sopt.sopt_valsize = sizeof(val);
	return sosetopt(so, &sopt);
#else
	struct mbuf *m;
	MGET(m, M_WAIT, MT_SOOPTS);
	*mtod(m, int *) = val;
	m->m_len = sizeof(int);
	return sosetopt(so, level, name, m);
#endif
}

static __inline int
nb_poll(struct nbpcb *nbp, int events, struct proc *p)
{
#ifndef NetBSD
	return nbp->nbp_tso->so_proto->pr_usrreqs->pru_sopoll(nbp->nbp_tso,
	    events, NULL, p);
#else
	register struct socket *so = nbp->nbp_tso;
        int revents = 0;
        register int s = splsoftnet();

        if (events & (POLLIN | POLLRDNORM))
                if (soreadable(so))
                        revents |= events & (POLLIN | POLLRDNORM);

        if (events & (POLLOUT | POLLWRNORM))
                if (sowriteable(so))
                        revents |= events & (POLLOUT | POLLWRNORM);

        if (events & (POLLPRI | POLLRDBAND))
                if (so->so_oobmark || (so->so_state & SS_RCVATMARK))
                        revents |= events & (POLLPRI | POLLRDBAND);

        if (revents == 0) {
                if (events & (POLLIN | POLLPRI | POLLRDNORM | POLLRDBAND)) {
                        selrecord(p, &so->so_rcv.sb_sel);
                        so->so_rcv.sb_flags |= SB_SEL;
                }

                if (events & (POLLOUT | POLLWRNORM)) {
                        selrecord(p, &so->so_snd.sb_sel);
                        so->so_snd.sb_flags |= SB_SEL;
                }
        }

        splx(s);
        return (revents);
#endif
}

static int
nbssn_rselect(struct nbpcb *nbp, struct timeval *tv, int events, struct proc *p)
{
#ifndef NetBSD
	struct timeval atv, rtv, ttv;
	int s, timo, error;

	if (tv) {
		atv = *tv;
		if (itimerfix(&atv)) {
			error = EINVAL;
			goto done;
		}
		getmicrouptime(&rtv);
		timevaladd(&atv, &rtv);
	}
	timo = 0;
retry:
	p->p_flag |= P_SELECT;
	error = nb_poll(nbp, events, p);
	if (error) {
		error = 0;
		goto done;
	}
	if (tv) {
		getmicrouptime(&rtv);
		if (timevalcmp(&rtv, &atv, >=))
			goto done;
		ttv = atv;
		timevalsub(&ttv, &rtv);
		timo = tvtohz(&ttv);
	}
	s = splhigh();
	if ((p->p_flag & P_SELECT) == 0) {
		splx(s);
		goto retry;
	}
	p->p_flag &= ~P_SELECT;
	error = tsleep((caddr_t)&selwait, PSOCK, "nbsel", timo);
	splx(s);
done:
	p->p_flag &= ~P_SELECT;
	if (error == ERESTART)
		return 0;
	return error;
#else
	struct timeval atv;
	int s, timo, error;

	if (tv) {
		atv = *tv;
		if (itimerfix(&atv)) {
			error = EINVAL;
			goto done;
		}
		s = splclock();
		timeradd(&atv, &time, &atv);
		timo = hzto(&atv);
		if (timo == 0)
			timo = 1;
		splx(s);
	} else
		timo = 0;
retry:
	p->p_flag |= P_SELECT;
	error = nb_poll(nbp, events, p);
	if (error) {
		error = 0;
		goto done;
	}
	s = splhigh();
	if (timo && timercmp(&time, &atv, >=)) {
		splx(s);
		goto done;
	}
	if ((p->p_flag & P_SELECT) == 0) {
		splx(s);
		goto retry;
	}
	p->p_flag &= ~P_SELECT;
	error = tsleep((caddr_t)&selwait, PSOCK, "nbsel", timo);
	splx(s);
done:
	p->p_flag &= ~P_SELECT;
	if (error == ERESTART)
		return 0;
	return error;
#endif
}

static int
nb_intr(struct nbpcb *nbp, struct proc *p)
{
	return 0;
}

static int
nb_sethdr(struct mbuf *m, u_int8_t type, u_int32_t len)
{
	u_int32_t *p = mtod(m, u_int32_t *);

	*p = htonl((len & 0x1FFFF) | (type << 24));
	return 0;
}

static int
nb_put_name(struct mbdata *mbp, struct sockaddr_nb *snb)
{
	int error;
	u_char seglen, *cp;

	cp = snb->snb_name;
	if (*cp == 0)
		return EINVAL;
	NBDEBUG("[%s]\n", cp);
	for (;;) {
		seglen = (*cp) + 1;
		error = mb_put_mem(mbp, cp, seglen, MB_MSYSTEM);
		if (error)
			return error;
		if (seglen == 1)
			break;
		cp += seglen;
	}
	return 0;
}

static int
nb_connect_in(struct nbpcb *nbp, struct sockaddr_in *to, struct proc *p)
{
	struct socket *so;
	int error, s;
#ifdef NetBSD
	struct mbuf *m;
#endif

#ifndef NetBSD
	error = socreate(AF_INET, &so, SOCK_STREAM, IPPROTO_TCP, p);
#else
	error = socreate(AF_INET, &so, SOCK_STREAM, IPPROTO_TCP);
#endif
	if (error)
		return error;
	nbp->nbp_tso = so;
	so->so_rcv.sb_timeo = (5 * hz);
	so->so_snd.sb_timeo = (5 * hz);
	error = soreserve(so, nbp->nbp_sndbuf, nbp->nbp_rcvbuf);
	if (error)
		goto bad;
	nb_setsockopt_int(so, SOL_SOCKET, SO_KEEPALIVE, 1);
	nb_setsockopt_int(so, IPPROTO_TCP, TCP_NODELAY, 1);
	so->so_rcv.sb_flags &= ~SB_NOINTR;
	so->so_snd.sb_flags &= ~SB_NOINTR;
#ifndef NetBSD
	error = soconnect(so, (struct sockaddr*)to, p);
#else
	MGET(m, M_WAIT, MT_SONAME);
	*mtod(m, struct sockaddr *) = *(struct sockaddr*)to;
	m->m_len = sizeof(struct sockaddr);
	error = soconnect(so, m);
	m_free(m);
#endif
	if (error)
		goto bad;

	s = splnet();
	while ((so->so_state & SS_ISCONNECTING) && so->so_error == 0) {
		tsleep(&so->so_timeo, PSOCK, "nbcon", 2 * hz);
		if ((so->so_state & SS_ISCONNECTING) && so->so_error == 0 &&
			(error = nb_intr(nbp, p)) != 0) {
			so->so_state &= ~SS_ISCONNECTING;
			splx(s);
			goto bad;
		}
	}
	if (so->so_error) {
		error = so->so_error;
		so->so_error = 0;
		splx(s);
		goto bad;
	}
	splx(s);
	return 0;
bad:
	smb_nbst_disconnect(nbp->nbp_vc, p);
	return error;
}

static int
nbssn_rq_request(struct nbpcb *nbp, struct proc *p)
{
	struct mbdata mb, *mbp = &mb;
	struct mbuf *m0;
	struct timeval tv;
	struct sockaddr_in sin;
	u_short port;
	u_int8_t rpcode;
	int error, rplen;

	error = mb_init(mbp);
	if (error)
		return error;
	mb_put_dwordle(mbp, 0);
	nb_put_name(mbp, nbp->nbp_paddr);
	nb_put_name(mbp, nbp->nbp_laddr);
	nb_sethdr(mbp->mb_top, NB_SSN_REQUEST, mb_fixhdr(mbp) - 4);
	error = nb_sosend(nbp->nbp_tso, mbp->mb_top, 0, p);
	if (!error) {
		nbp->nbp_state = NBST_RQSENT;
	}
	mbp->mb_top = NULL;
	mb_done(mbp);
	if (error)
		return error;
	tv.tv_sec = nbp->nbp_timo;
	tv.tv_usec = 0;
	error = nbssn_rselect(nbp, &tv, POLLIN, p);
	if (error == EWOULDBLOCK) {	/* Timeout */
		NBDEBUG("initial request timeout\n");
		return ETIMEDOUT;
	}
	if (error)			/* restart or interrupt */
		return error;
	error = nbssn_recv(nbp, &m0, &rplen, &rpcode, p);
	if (error) {
		NBDEBUG("recv() error %d\n", error);
		return error;
	}
	/*
	 * Process NETBIOS reply
	 */
	if (m0)
		mb_initm(mbp, m0);
	error = 0;
	do {
		if (rpcode == NB_SSN_POSRESP) {
			nbp->nbp_state = NBST_SESSION;
			nbp->nbp_flags |= NBF_CONNECTED;
			break;
		}
		if (rpcode != NB_SSN_RTGRESP) {
			error = ECONNABORTED;
			break;
		}
		if (rplen != 6) {
			error = ECONNABORTED;
			break;
		}
		mb_get_mem(mbp, (caddr_t)&sin.sin_addr, 4, MB_MSYSTEM);
		mb_get_word(mbp, &port);
		sin.sin_port = port;
		nbp->nbp_state = NBST_RETARGET;
		smb_nbst_disconnect(nbp->nbp_vc, p);
		error = nb_connect_in(nbp, &sin, p);
		if (!error)
			error = nbssn_rq_request(nbp, p);
		if (error) {
			smb_nbst_disconnect(nbp->nbp_vc, p);
			break;
		}
	} while(0);
	mb_done(mbp);
	return error;
}

static int
nbssn_recvhdr(struct nbpcb *nbp, int *lenp,
	u_int8_t *rpcodep, int flags, struct proc *p)
{
	struct socket *so = nbp->nbp_tso;
	struct uio auio;
	struct iovec aio;
	u_int32_t len;
	int error;

	aio.iov_base = (caddr_t)&len;
	aio.iov_len = sizeof(len);
	auio.uio_iov = &aio;
	auio.uio_iovcnt = 1;
	auio.uio_segflg = UIO_SYSSPACE;
	auio.uio_rw = UIO_READ;
	auio.uio_offset = 0;
	auio.uio_resid = sizeof(len);
	auio.uio_procp = p;
#ifndef NetBSD
	error = so->so_proto->pr_usrreqs->pru_soreceive
	    (so, (struct sockaddr **)NULL, &auio,
	    (struct mbuf **)NULL, (struct mbuf **)NULL, &flags);
#else
	error = (*so->so_receive)(so, (struct mbuf **)0, &auio,
	    (struct mbuf **)NULL, (struct mbuf **)NULL, &flags);
#endif
	if (error)
		return error;
	if (auio.uio_resid > 0) {
		SMBSDEBUG("short reply\n");
		return EPIPE;
	}
	len = ntohl(len);
	*rpcodep = (len >> 24) & 0xFF;
	len &= 0x1ffff;
	if (len > SMB_MAXPKTLEN) {
		SMBERROR("packet too long (%d)\n", len);
		return EFBIG;
	}
	*lenp = len;
	return 0;
}

static int
nbssn_recv(struct nbpcb *nbp, struct mbuf **mpp, int *lenp,
	u_int8_t *rpcodep, struct proc *p)
{
	struct socket *so = nbp->nbp_tso;
	struct uio auio;
	struct mbuf *m;
	u_int8_t rpcode;
	int len;
	int error, rcvflg;

	if (so == NULL)
		return ENOTCONN;

	if (mpp)
		*mpp = NULL;
	for(;;) {
		m = NULL;
		error = nbssn_recvhdr(nbp, &len, &rpcode, MSG_WAITALL, p);
		if (so->so_state &
		    (SS_ISDISCONNECTING | SS_ISDISCONNECTED | SS_CANTRCVMORE)) {
			nbp->nbp_state = NBST_CLOSED;
			NBDEBUG("session closed by peer\n");
			return ECONNRESET;
		}
		if (error)
			return error;
		if (len == 0 && nbp->nbp_state != NBST_SESSION)
			break;
		bzero(&auio, sizeof(auio));
		auio.uio_resid = len;
		auio.uio_procp = p;
		do {
			rcvflg = MSG_WAITALL;
#ifndef NetBSD
			error = so->so_proto->pr_usrreqs->pru_soreceive
			    (so, (struct sockaddr **)NULL,
			    &auio, &m, (struct mbuf **)NULL, &rcvflg);
#else
			error = (*so->so_receive)(so, (struct mbuf **)0,
			    &auio, &m, (struct mbuf **)NULL, &rcvflg);
#endif
		} while (error == EWOULDBLOCK || error == EINTR ||
				 error == ERESTART);
		if (error)
			break;
		if (auio.uio_resid > 0) {
			SMBERROR("packet is shorter than expected\n");
			error = EPIPE;
			break;
		}
		if (nbp->nbp_state == NBST_SESSION &&
		    rpcode == NB_SSN_MESSAGE)
			break;
		NBDEBUG("non-session packet %x\n", rpcode);
		if (m)
			m_freem(m);
	}
	if (error) {
		if (m)
			m_freem(m);
		return error;
	}
	if (mpp)
		*mpp = m;
	else
		m_freem(m);
	*lenp = len;
	*rpcodep = rpcode;
	return 0;
}

int
nb_dupsockaddr(struct sockaddr_nb *src, struct sockaddr_nb **dst)
{
	struct sockaddr_nb *snb;

	snb = (struct sockaddr_nb*)dup_sockaddr((struct sockaddr*)src, 1);
	if (snb == NULL) 
		return ENOMEM;
	*dst = snb;
	return 0;
}

/*
 * SMB transport interface
 */
static int
smb_nbst_create(struct smb_vc *vcp, struct proc *p)
{
	struct nbpcb *nbp;

	MALLOC(nbp, struct nbpcb *, sizeof *nbp, M_NBDATA, M_WAITOK);
	if (nbp == NULL)
		return ENOBUFS;
	bzero(nbp, sizeof *nbp);
	nbp->nbp_timo = 5;	/* XXX: sysctl ? */
	nbp->nbp_state = NBST_CLOSED;
	nbp->nbp_vc = vcp;
	nbp->nbp_sndbuf = smb_tcpsndbuf;
	nbp->nbp_rcvbuf = smb_tcprcvbuf;
	vcp->vc_tdata = nbp;
	return 0;
}

static int
smb_nbst_done(struct smb_vc *vcp, struct proc *p)
{
	struct nbpcb *nbp = vcp->vc_tdata;

	if (nbp == NULL)
		return ENOTCONN;
	smb_nbst_disconnect(vcp, p);
	if (nbp->nbp_laddr)
		free(nbp->nbp_laddr, M_SONAME);
	if (nbp->nbp_paddr)
		free(nbp->nbp_paddr, M_SONAME);
	free(nbp, M_NBDATA);
	return 0;
}

static int
smb_nbst_bind(struct smb_vc *vcp, struct sockaddr *sap, struct proc *p)
{
	struct nbpcb *nbp = vcp->vc_tdata;
	struct sockaddr_nb *snb;
	int error, slen;

	NBDEBUG("\n");
	error = EINVAL;
	do {
		if (nbp->nbp_flags & NBF_LOCADDR)
			break;
		/*
		 * It is possible to create NETBIOS name in the kernel,
		 * but nothing prevents us to do it in the user space.
		 */
		if (sap == NULL)
			break;
		slen = sap->sa_len;
		if (slen < NB_MINSALEN)
			break;
		error = nb_dupsockaddr((struct sockaddr_nb *)sap, &snb);
		if (error)
			break;
		nbp->nbp_laddr = snb;
		nbp->nbp_flags |= NBF_LOCADDR;
	} while(0);
	return error;
}

static int
smb_nbst_connect(struct smb_vc *vcp, struct sockaddr *sap, struct proc *p)
{
	struct nbpcb *nbp = vcp->vc_tdata;
	struct sockaddr_in sin;
	struct sockaddr_nb *snb;
	int error, slen, rtt;

	NBDEBUG("\n");
	if (nbp->nbp_tso != NULL)
		return EISCONN;
	if (nbp->nbp_laddr == NULL)
		return EINVAL;
	slen = sap->sa_len;
	if (slen < NB_MINSALEN)
		return EINVAL;
	if (nbp->nbp_paddr) {
		free(nbp->nbp_paddr, M_SONAME);
		nbp->nbp_paddr = NULL;
	}
	error = nb_dupsockaddr((struct sockaddr_nb *)sap, &snb);
	if (error)
		return error;
	nbp->nbp_paddr = snb;
	sin = snb->snb_addrin;
	nbp->nbp_rtt = 0;
	error = nb_connect_in(nbp, &sin, p);
	if (error)
		return error;
	rtt = nbp->nbp_rtt ? nbp->nbp_rtt : 1;
	nbp->nbp_timo = rtt * 4;	/* emperic value ... */
	error = nbssn_rq_request(nbp, p);
	if (error)
		smb_nbst_disconnect(vcp, p);
	return error;
}

static int
smb_nbst_disconnect(struct smb_vc *vcp, struct proc *p)
{
	struct nbpcb *nbp = vcp->vc_tdata;
	struct socket *so;

	if (nbp == NULL || nbp->nbp_tso == NULL)
		return ENOTCONN;
	if ((so = nbp->nbp_tso) != NULL) {
		nbp->nbp_flags &= ~NBF_CONNECTED;
		nbp->nbp_tso = (struct socket *)NULL;
		soshutdown(so, 2);
		soclose(so);
	}
	if (nbp->nbp_state != NBST_RETARGET) {
		nbp->nbp_state = NBST_CLOSED;
	}
	return 0;
}

static int
smb_nbst_send(struct smb_vc *vcp, struct mbuf *m0, struct proc *p)
{
	struct nbpcb *nbp = vcp->vc_tdata;
	int error;

	if (nbp->nbp_state != NBST_SESSION) {
		error = ENOTCONN;
		goto abort;
	}
	M_PREPEND(m0, 4, M_WAITOK);
	if (m0 == NULL)
		return ENOBUFS;
	nb_sethdr(m0, NB_SSN_MESSAGE, m_fixhdr(m0) - 4);
	error = nb_sosend(nbp->nbp_tso, m0, 0, p);
	return error;
abort:
	if (m0)
		m_freem(m0);
	return error;
}


static int
smb_nbst_recv(struct smb_vc *vcp, struct mbuf **mpp, struct proc *p)
{
	struct nbpcb *nbp = vcp->vc_tdata;
	u_int8_t rpcode;
	int error, rplen;

	nbp->nbp_flags |= NBF_RECVLOCK;
	error = nbssn_recv(nbp, mpp, &rplen, &rpcode, p);
	nbp->nbp_flags &= ~NBF_RECVLOCK;
	return error;
}

static void
smb_nbst_timo(struct smb_vc *vcp)
{
	struct nbpcb *nbp = vcp->vc_tdata;
	struct proc *p = curproc;	/* XXX */
	u_int8_t rpcode;
	int len;
	int error;

	nbp->nbp_rtt++;
	if ((nbp->nbp_flags & NBF_RECVLOCK) || nbp->nbp_state != NBST_SESSION)
		return;
	error = nbssn_recvhdr(nbp, &len, &rpcode, MSG_PEEK | MSG_DONTWAIT, p);
	if (error)
		return;
	if (rpcode == NB_SSN_KEEPALIVE) {
		nbssn_recvhdr(nbp, &len, &rpcode, MSG_WAITALL, p);
	}
	return;
}

static void
smb_nbst_intr(struct smb_vc *vcp)
{
	struct nbpcb *nbp = vcp->vc_tdata;

	if (nbp == NULL || nbp->nbp_tso == NULL)
		return;
	sorwakeup(nbp->nbp_tso);
	sowwakeup(nbp->nbp_tso);
}

static int
smb_nbst_bufsz(struct smb_vc *vcp, int *sndsz, int *rcvsz)
{
	struct nbpcb *nbp = vcp->vc_tdata;

	if (sndsz)
		*sndsz = nbp->nbp_sndbuf;
	if (rcvsz)
		*sndsz = nbp->nbp_rcvbuf;
	return 0;
}

struct smb_tran_desc smb_tran_nbtcp_desc = {
	SMBT_NBTCP,
	smb_nbst_create, smb_nbst_done,
	smb_nbst_bind, smb_nbst_connect, smb_nbst_disconnect,
	smb_nbst_send, smb_nbst_recv,
	smb_nbst_timo, smb_nbst_intr,
	smb_nbst_bufsz
};

