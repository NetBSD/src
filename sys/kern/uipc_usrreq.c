/*	$NetBSD: uipc_usrreq.c,v 1.65 2003/07/23 22:17:54 itojun Exp $	*/

/*-
 * Copyright (c) 1998, 2000 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe of the Numerical Aerospace Simulation Facility,
 * NASA Ames Research Center.
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
 *	This product includes software developed by the NetBSD
 *	Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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

/*
 * Copyright (c) 1997 Christopher G. Demetriou.  All rights reserved.
 * Copyright (c) 1982, 1986, 1989, 1991, 1993
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
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
 *	@(#)uipc_usrreq.c	8.9 (Berkeley) 5/14/95
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: uipc_usrreq.c,v 1.65 2003/07/23 22:17:54 itojun Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/filedesc.h>
#include <sys/domain.h>
#include <sys/protosw.h>
#include <sys/socket.h>
#include <sys/socketvar.h>
#include <sys/unpcb.h>
#include <sys/un.h>
#include <sys/namei.h>
#include <sys/vnode.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/mbuf.h>

/*
 * Unix communications domain.
 *
 * TODO:
 *	SEQPACKET, RDM
 *	rethink name space problems
 *	need a proper out-of-band
 */
struct	sockaddr_un sun_noname = { sizeof(sun_noname), AF_LOCAL };
ino_t	unp_ino;			/* prototype for fake inode numbers */

struct mbuf *unp_addsockcred __P((struct proc *, struct mbuf *));

int
unp_output(m, control, unp, p)
	struct mbuf *m, *control;
	struct unpcb *unp;
	struct proc *p;
{
	struct socket *so2;
	struct sockaddr_un *sun;

	so2 = unp->unp_conn->unp_socket;
	if (unp->unp_addr)
		sun = unp->unp_addr;
	else
		sun = &sun_noname;
	if (unp->unp_conn->unp_flags & UNP_WANTCRED)
		control = unp_addsockcred(p, control);
	if (sbappendaddr(&so2->so_rcv, (struct sockaddr *)sun, m,
	    control) == 0) {
		m_freem(control);
		m_freem(m);
		return (ENOBUFS);
	} else {
		sorwakeup(so2);
		return (0);
	}
}

void
unp_setsockaddr(unp, nam)
	struct unpcb *unp;
	struct mbuf *nam;
{
	struct sockaddr_un *sun;

	if (unp->unp_addr)
		sun = unp->unp_addr;
	else
		sun = &sun_noname;
	nam->m_len = sun->sun_len;
	if (nam->m_len > MLEN)
		MEXTMALLOC(nam, nam->m_len, M_WAITOK);
	memcpy(mtod(nam, caddr_t), sun, (size_t)nam->m_len);
}

void
unp_setpeeraddr(unp, nam)
	struct unpcb *unp;
	struct mbuf *nam;
{
	struct sockaddr_un *sun;

	if (unp->unp_conn && unp->unp_conn->unp_addr)
		sun = unp->unp_conn->unp_addr;
	else
		sun = &sun_noname;
	nam->m_len = sun->sun_len;
	if (nam->m_len > MLEN)
		MEXTMALLOC(nam, nam->m_len, M_WAITOK);
	memcpy(mtod(nam, caddr_t), sun, (size_t)nam->m_len);
}

/*ARGSUSED*/
int
uipc_usrreq(so, req, m, nam, control, p)
	struct socket *so;
	int req;
	struct mbuf *m, *nam, *control;
	struct proc *p;
{
	struct unpcb *unp = sotounpcb(so);
	struct socket *so2;
	int error = 0;

	if (req == PRU_CONTROL)
		return (EOPNOTSUPP);

#ifdef DIAGNOSTIC
	if (req != PRU_SEND && req != PRU_SENDOOB && control)
		panic("uipc_usrreq: unexpected control mbuf");
#endif
	if (unp == 0 && req != PRU_ATTACH) {
		error = EINVAL;
		goto release;
	}

	switch (req) {

	case PRU_ATTACH:
		if (unp != 0) {
			error = EISCONN;
			break;
		}
		error = unp_attach(so);
		break;

	case PRU_DETACH:
		unp_detach(unp);
		break;

	case PRU_BIND:
		error = unp_bind(unp, nam, p);
		break;

	case PRU_LISTEN:
		if (unp->unp_vnode == 0)
			error = EINVAL;
		break;

	case PRU_CONNECT:
		error = unp_connect(so, nam, p);
		break;

	case PRU_CONNECT2:
		error = unp_connect2(so, (struct socket *)nam);
		break;

	case PRU_DISCONNECT:
		unp_disconnect(unp);
		break;

	case PRU_ACCEPT:
		unp_setpeeraddr(unp, nam);
		break;

	case PRU_SHUTDOWN:
		socantsendmore(so);
		unp_shutdown(unp);
		break;

	case PRU_RCVD:
		switch (so->so_type) {

		case SOCK_DGRAM:
			panic("uipc 1");
			/*NOTREACHED*/

		case SOCK_STREAM:
#define	rcv (&so->so_rcv)
#define snd (&so2->so_snd)
			if (unp->unp_conn == 0)
				break;
			so2 = unp->unp_conn->unp_socket;
			/*
			 * Adjust backpressure on sender
			 * and wakeup any waiting to write.
			 */
			snd->sb_mbmax += unp->unp_mbcnt - rcv->sb_mbcnt;
			unp->unp_mbcnt = rcv->sb_mbcnt;
			snd->sb_hiwat += unp->unp_cc - rcv->sb_cc;
			unp->unp_cc = rcv->sb_cc;
			sowwakeup(so2);
#undef snd
#undef rcv
			break;

		default:
			panic("uipc 2");
		}
		break;

	case PRU_SEND:
		/*
		 * Note: unp_internalize() rejects any control message
		 * other than SCM_RIGHTS, and only allows one.  This
		 * has the side-effect of preventing a caller from
		 * forging SCM_CREDS.
		 */
		if (control && (error = unp_internalize(control, p)))
			break;
		switch (so->so_type) {

		case SOCK_DGRAM: {
			if (nam) {
				if ((so->so_state & SS_ISCONNECTED) != 0) {
					error = EISCONN;
					goto die;
				}
				error = unp_connect(so, nam, p);
				if (error) {
				die:
					m_freem(control);
					m_freem(m);
					break;
				}
			} else {
				if ((so->so_state & SS_ISCONNECTED) == 0) {
					error = ENOTCONN;
					goto die;
				}
			}
			error = unp_output(m, control, unp, p);
			if (nam)
				unp_disconnect(unp);
			break;
		}

		case SOCK_STREAM:
#define	rcv (&so2->so_rcv)
#define	snd (&so->so_snd)
			if (unp->unp_conn == 0)
				panic("uipc 3");
			so2 = unp->unp_conn->unp_socket;
			if (unp->unp_conn->unp_flags & UNP_WANTCRED) {
				/*
				 * Credentials are passed only once on
				 * SOCK_STREAM.
				 */
				unp->unp_conn->unp_flags &= ~UNP_WANTCRED;
				control = unp_addsockcred(p, control);
			}
			/*
			 * Send to paired receive port, and then reduce
			 * send buffer hiwater marks to maintain backpressure.
			 * Wake up readers.
			 */
			if (control) {
				if (sbappendcontrol(rcv, m, control) == 0)
					m_freem(control);
			} else
				sbappend(rcv, m);
			snd->sb_mbmax -=
			    rcv->sb_mbcnt - unp->unp_conn->unp_mbcnt;
			unp->unp_conn->unp_mbcnt = rcv->sb_mbcnt;
			snd->sb_hiwat -= rcv->sb_cc - unp->unp_conn->unp_cc;
			unp->unp_conn->unp_cc = rcv->sb_cc;
			sorwakeup(so2);
#undef snd
#undef rcv
			break;

		default:
			panic("uipc 4");
		}
		break;

	case PRU_ABORT:
		unp_drop(unp, ECONNABORTED);

#ifdef DIAGNOSTIC
		if (so->so_pcb == 0)
			panic("uipc 5: drop killed pcb");
#endif
		unp_detach(unp);
		break;

	case PRU_SENSE:
		((struct stat *) m)->st_blksize = so->so_snd.sb_hiwat;
		if (so->so_type == SOCK_STREAM && unp->unp_conn != 0) {
			so2 = unp->unp_conn->unp_socket;
			((struct stat *) m)->st_blksize += so2->so_rcv.sb_cc;
		}
		((struct stat *) m)->st_dev = NODEV;
		if (unp->unp_ino == 0)
			unp->unp_ino = unp_ino++;
		((struct stat *) m)->st_atimespec =
		    ((struct stat *) m)->st_mtimespec =
		    ((struct stat *) m)->st_ctimespec = unp->unp_ctime;
		((struct stat *) m)->st_ino = unp->unp_ino;
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
		unp_setsockaddr(unp, nam);
		break;

	case PRU_PEERADDR:
		unp_setpeeraddr(unp, nam);
		break;

	default:
		panic("piusrreq");
	}

release:
	return (error);
}

/*
 * Unix domain socket option processing.
 */
int
uipc_ctloutput(op, so, level, optname, mp)
	int op;
	struct socket *so;
	int level, optname;
	struct mbuf **mp;
{
	struct unpcb *unp = sotounpcb(so);
	struct mbuf *m = *mp;
	int optval = 0, error = 0;

	if (level != 0) {
		error = EINVAL;
		if (op == PRCO_SETOPT && m)
			(void) m_free(m);
	} else switch (op) {

	case PRCO_SETOPT:
		switch (optname) {
		case LOCAL_CREDS:
			if (m == NULL || m->m_len != sizeof(int))
				error = EINVAL;
			else {
				optval = *mtod(m, int *);
				switch (optname) {
#define	OPTSET(bit) \
	if (optval) \
		unp->unp_flags |= (bit); \
	else \
		unp->unp_flags &= ~(bit);

				case LOCAL_CREDS:
					OPTSET(UNP_WANTCRED);
					break;
				}
			}
			break;
#undef OPTSET

		default:
			error = ENOPROTOOPT;
			break;
		}
		if (m)
			(void) m_free(m);
		break;

	case PRCO_GETOPT:
		switch (optname) {
		case LOCAL_CREDS:
			*mp = m = m_get(M_WAIT, MT_SOOPTS);
			m->m_len = sizeof(int);
			switch (optname) {

#define	OPTBIT(bit)	(unp->unp_flags & (bit) ? 1 : 0)

			case LOCAL_CREDS:
				optval = OPTBIT(UNP_WANTCRED);
				break;
			}
			*mtod(m, int *) = optval;
			break;
#undef OPTBIT

		default:
			error = ENOPROTOOPT;
			break;
		}
		break;
	}
	return (error);
}

/*
 * Both send and receive buffers are allocated PIPSIZ bytes of buffering
 * for stream sockets, although the total for sender and receiver is
 * actually only PIPSIZ.
 * Datagram sockets really use the sendspace as the maximum datagram size,
 * and don't really want to reserve the sendspace.  Their recvspace should
 * be large enough for at least one max-size datagram plus address.
 */
#define	PIPSIZ	4096
u_long	unpst_sendspace = PIPSIZ;
u_long	unpst_recvspace = PIPSIZ;
u_long	unpdg_sendspace = 2*1024;	/* really max datagram size */
u_long	unpdg_recvspace = 4*1024;

int	unp_rights;			/* file descriptors in flight */

int
unp_attach(so)
	struct socket *so;
{
	struct unpcb *unp;
	struct timeval tv;
	int error;
	
	if (so->so_snd.sb_hiwat == 0 || so->so_rcv.sb_hiwat == 0) {
		switch (so->so_type) {

		case SOCK_STREAM:
			error = soreserve(so, unpst_sendspace, unpst_recvspace);
			break;

		case SOCK_DGRAM:
			error = soreserve(so, unpdg_sendspace, unpdg_recvspace);
			break;

		default:
			panic("unp_attach");
		}
		if (error)
			return (error);
	}
	unp = malloc(sizeof(*unp), M_PCB, M_NOWAIT);
	if (unp == NULL)
		return (ENOBUFS);
	memset((caddr_t)unp, 0, sizeof(*unp));
	unp->unp_socket = so;
	so->so_pcb = unp;
	microtime(&tv);
	TIMEVAL_TO_TIMESPEC(&tv, &unp->unp_ctime);
	return (0);
}

void
unp_detach(unp)
	struct unpcb *unp;
{
	
	if (unp->unp_vnode) {
		unp->unp_vnode->v_socket = 0;
		vrele(unp->unp_vnode);
		unp->unp_vnode = 0;
	}
	if (unp->unp_conn)
		unp_disconnect(unp);
	while (unp->unp_refs)
		unp_drop(unp->unp_refs, ECONNRESET);
	soisdisconnected(unp->unp_socket);
	unp->unp_socket->so_pcb = 0;
	if (unp->unp_addr)
		free(unp->unp_addr, M_SONAME);
	if (unp_rights) {
		/*
		 * Normally the receive buffer is flushed later,
		 * in sofree, but if our receive buffer holds references
		 * to descriptors that are now garbage, we will dispose
		 * of those descriptor references after the garbage collector
		 * gets them (resulting in a "panic: closef: count < 0").
		 */
		sorflush(unp->unp_socket);
		free(unp, M_PCB);
		unp_gc();
	} else
		free(unp, M_PCB);
}

int
unp_bind(unp, nam, p)
	struct unpcb *unp;
	struct mbuf *nam;
	struct proc *p;
{
	struct sockaddr_un *sun;
	struct vnode *vp;
	struct vattr vattr;
	size_t addrlen;
	int error;
	struct nameidata nd;

	if (unp->unp_vnode != 0)
		return (EINVAL);

	/*
	 * Allocate the new sockaddr.  We have to allocate one
	 * extra byte so that we can ensure that the pathname
	 * is nul-terminated.
	 */
	addrlen = nam->m_len + 1;
	sun = malloc(addrlen, M_SONAME, M_WAITOK);
	m_copydata(nam, 0, nam->m_len, (caddr_t)sun);
	*(((char *)sun) + nam->m_len) = '\0';

	NDINIT(&nd, CREATE, FOLLOW | LOCKPARENT, UIO_SYSSPACE,
	    sun->sun_path, p);

/* SHOULD BE ABLE TO ADOPT EXISTING AND wakeup() ALA FIFO's */
	if ((error = namei(&nd)) != 0)
		goto bad;
	vp = nd.ni_vp;
	if (vp != NULL) {
		VOP_ABORTOP(nd.ni_dvp, &nd.ni_cnd);
		if (nd.ni_dvp == vp)
			vrele(nd.ni_dvp);
		else
			vput(nd.ni_dvp);
		vrele(vp);
		error = EADDRINUSE;
		goto bad;
	}
	VATTR_NULL(&vattr);
	vattr.va_type = VSOCK;
	vattr.va_mode = ACCESSPERMS;
	VOP_LEASE(nd.ni_dvp, p, p->p_ucred, LEASE_WRITE);
	error = VOP_CREATE(nd.ni_dvp, &nd.ni_vp, &nd.ni_cnd, &vattr);
	if (error)
		goto bad;
	vp = nd.ni_vp;
	vp->v_socket = unp->unp_socket;
	unp->unp_vnode = vp;
	unp->unp_addrlen = addrlen;
	unp->unp_addr = sun;
	VOP_UNLOCK(vp, 0);
	return (0);

 bad:
	free(sun, M_SONAME);
	return (error);
}

int
unp_connect(so, nam, p)
	struct socket *so;
	struct mbuf *nam;
	struct proc *p;
{
	struct sockaddr_un *sun;
	struct vnode *vp;
	struct socket *so2, *so3;
	struct unpcb *unp2, *unp3;
	size_t addrlen;
	int error;
	struct nameidata nd;

	/*
	 * Allocate a temporary sockaddr.  We have to allocate one extra
	 * byte so that we can ensure that the pathname is nul-terminated.
	 * When we establish the connection, we copy the other PCB's
	 * sockaddr to our own.
	 */
	addrlen = nam->m_len + 1;
	sun = malloc(addrlen, M_SONAME, M_WAITOK);
	m_copydata(nam, 0, nam->m_len, (caddr_t)sun);
	*(((char *)sun) + nam->m_len) = '\0';

	NDINIT(&nd, LOOKUP, FOLLOW | LOCKLEAF, UIO_SYSSPACE, sun->sun_path, p);

	if ((error = namei(&nd)) != 0)
		goto bad2;
	vp = nd.ni_vp;
	if (vp->v_type != VSOCK) {
		error = ENOTSOCK;
		goto bad;
	}
	if ((error = VOP_ACCESS(vp, VWRITE, p->p_ucred, p)) != 0)
		goto bad;
	so2 = vp->v_socket;
	if (so2 == 0) {
		error = ECONNREFUSED;
		goto bad;
	}
	if (so->so_type != so2->so_type) {
		error = EPROTOTYPE;
		goto bad;
	}
	if (so->so_proto->pr_flags & PR_CONNREQUIRED) {
		if ((so2->so_options & SO_ACCEPTCONN) == 0 ||
		    (so3 = sonewconn(so2, 0)) == 0) {
			error = ECONNREFUSED;
			goto bad;
		}
		unp2 = sotounpcb(so2);
		unp3 = sotounpcb(so3);
		if (unp2->unp_addr) {
			unp3->unp_addr = malloc(unp2->unp_addrlen,
			    M_SONAME, M_WAITOK);
			memcpy(unp3->unp_addr, unp2->unp_addr,
			    unp2->unp_addrlen);
			unp3->unp_addrlen = unp2->unp_addrlen;
		}
		unp3->unp_flags = unp2->unp_flags;
		so2 = so3;
	}
	error = unp_connect2(so, so2);
 bad:
	vput(vp);
 bad2:
	free(sun, M_SONAME);
	return (error);
}

int
unp_connect2(so, so2)
	struct socket *so;
	struct socket *so2;
{
	struct unpcb *unp = sotounpcb(so);
	struct unpcb *unp2;

	if (so2->so_type != so->so_type)
		return (EPROTOTYPE);
	unp2 = sotounpcb(so2);
	unp->unp_conn = unp2;
	switch (so->so_type) {

	case SOCK_DGRAM:
		unp->unp_nextref = unp2->unp_refs;
		unp2->unp_refs = unp;
		soisconnected(so);
		break;

	case SOCK_STREAM:
		unp2->unp_conn = unp;
		soisconnected(so);
		soisconnected(so2);
		break;

	default:
		panic("unp_connect2");
	}
	return (0);
}

void
unp_disconnect(unp)
	struct unpcb *unp;
{
	struct unpcb *unp2 = unp->unp_conn;

	if (unp2 == 0)
		return;
	unp->unp_conn = 0;
	switch (unp->unp_socket->so_type) {

	case SOCK_DGRAM:
		if (unp2->unp_refs == unp)
			unp2->unp_refs = unp->unp_nextref;
		else {
			unp2 = unp2->unp_refs;
			for (;;) {
				if (unp2 == 0)
					panic("unp_disconnect");
				if (unp2->unp_nextref == unp)
					break;
				unp2 = unp2->unp_nextref;
			}
			unp2->unp_nextref = unp->unp_nextref;
		}
		unp->unp_nextref = 0;
		unp->unp_socket->so_state &= ~SS_ISCONNECTED;
		break;

	case SOCK_STREAM:
		soisdisconnected(unp->unp_socket);
		unp2->unp_conn = 0;
		soisdisconnected(unp2->unp_socket);
		break;
	}
}

#ifdef notdef
unp_abort(unp)
	struct unpcb *unp;
{

	unp_detach(unp);
}
#endif

void
unp_shutdown(unp)
	struct unpcb *unp;
{
	struct socket *so;

	if (unp->unp_socket->so_type == SOCK_STREAM && unp->unp_conn &&
	    (so = unp->unp_conn->unp_socket))
		socantrcvmore(so);
}

void
unp_drop(unp, errno)
	struct unpcb *unp;
	int errno;
{
	struct socket *so = unp->unp_socket;

	so->so_error = errno;
	unp_disconnect(unp);
	if (so->so_head) {
		so->so_pcb = 0;
		sofree(so);
		if (unp->unp_addr)
			free(unp->unp_addr, M_SONAME);
		free(unp, M_PCB);
	}
}

#ifdef notdef
unp_drain()
{

}
#endif

int
unp_externalize(rights)
	struct mbuf *rights;
{
	struct proc *p = curproc;		/* XXX */
	struct cmsghdr *cm = mtod(rights, struct cmsghdr *);
	int i, *fdp;
	struct file **rp;
	struct file *fp;
	int nfds, error = 0;

	nfds = (cm->cmsg_len - CMSG_ALIGN(sizeof(*cm))) /
	    sizeof(struct file *);
	rp = (struct file **)CMSG_DATA(cm);

	fdp = malloc(nfds * sizeof(int), M_TEMP, M_WAITOK);

	/* Make sure the recipient should be able to see the descriptors.. */
	if (p->p_cwdi->cwdi_rdir != NULL) {
		rp = (struct file **)CMSG_DATA(cm);
		for (i = 0; i < nfds; i++) {
			fp = *rp++;
			/*
			 * If we are in a chroot'ed directory, and
			 * someone wants to pass us a directory, make
			 * sure it's inside the subtree we're allowed
			 * to access.
			 */
			if (fp->f_type == DTYPE_VNODE) {
				struct vnode *vp = (struct vnode *)fp->f_data;
				if ((vp->v_type == VDIR) &&
				    !vn_isunder(vp, p->p_cwdi->cwdi_rdir, p)) {
					error = EPERM;
					break;
				}
			}
		}
	}

 restart:
	rp = (struct file **)CMSG_DATA(cm);
	if (error != 0) {
		for (i = 0; i < nfds; i++) {
			fp = *rp;
			/*
			 * zero the pointer before calling unp_discard,
			 * since it may end up in unp_gc()..
			 */
			*rp++ = 0;
			unp_discard(fp);
		}
		goto out;
	}

	/*
	 * First loop -- allocate file descriptor table slots for the
	 * new descriptors.
	 */
	for (i = 0; i < nfds; i++) {
		fp = *rp++;
		if ((error = fdalloc(p, 0, &fdp[i])) != 0) {
			/*
			 * Back out what we've done so far.
			 */
			for (--i; i >= 0; i--)
				fdremove(p->p_fd, fdp[i]);

			if (error == ENOSPC) {
				fdexpand(p);
				error = 0;
			} else {
				/*
				 * This is the error that has historically
				 * been returned, and some callers may
				 * expect it.
				 */
				error = EMSGSIZE;
			}
			goto restart;
		}

		/*
		 * Make the slot reference the descriptor so that
		 * fdalloc() works properly.. We finalize it all
		 * in the loop below.
		 */
		p->p_fd->fd_ofiles[fdp[i]] = fp;
	}

	/*
	 * Now that adding them has succeeded, update all of the
	 * descriptor passing state.
	 */
	rp = (struct file **)CMSG_DATA(cm);
	for (i = 0; i < nfds; i++) {
		fp = *rp++;
		fp->f_msgcount--;
		unp_rights--;
	}

	/*
	 * Copy temporary array to message and adjust length, in case of
	 * transition from large struct file pointers to ints.
	 */
	memcpy(CMSG_DATA(cm), fdp, nfds * sizeof(int));
	cm->cmsg_len = CMSG_LEN(nfds * sizeof(int));
	rights->m_len = CMSG_SPACE(nfds * sizeof(int));
 out:
	free(fdp, M_TEMP);
	return (error);
}

int
unp_internalize(control, p)
	struct mbuf *control;
	struct proc *p;
{
	struct filedesc *fdescp = p->p_fd;
	struct cmsghdr *cm = mtod(control, struct cmsghdr *);
	struct file **rp;
	struct file *fp;
	int i, fd, *fdp;
	int nfds;
	u_int neededspace;

	/* Sanity check the control message header */
	/*
	 * NetBSD uses only SOL_SOCKET as the level, but linux uses 1.
	 * We accept both here, because it is too much of a pain to
	 * massage the messages in the compat code, since it involves
	 * allocating stuff on the stack, copying in and out etc.
	 */
	if (cm->cmsg_type != SCM_RIGHTS ||
	    (cm->cmsg_level != SOL_SOCKET && cm->cmsg_level != 1) ||
	    cm->cmsg_len != control->m_len)
		return (EINVAL);

	/* Verify that the file descriptors are valid */
	nfds = (cm->cmsg_len - CMSG_ALIGN(sizeof(*cm))) / sizeof(int);
	fdp = (int *)CMSG_DATA(cm);
	for (i = 0; i < nfds; i++) {
		fd = *fdp++;
		if ((fp = fd_getfile(fdescp, fd)) == NULL)
			return (EBADF);
		simple_unlock(&fp->f_slock);
	}

	/* Make sure we have room for the struct file pointers */
 morespace:
	neededspace = CMSG_SPACE(nfds * sizeof(struct file *)) -
	    control->m_len;
	if (neededspace > M_TRAILINGSPACE(control)) {

		/* if we already have a cluster, the message is just too big */
		if (control->m_flags & M_EXT)
			return (E2BIG);

		/* allocate a cluster and try again */
		m_clget(control, M_WAIT);
		if ((control->m_flags & M_EXT) == 0)
			return (ENOBUFS);	/* allocation failed */

		/* copy the data to the cluster */
		memcpy(mtod(control, char *), cm, cm->cmsg_len);
		cm = mtod(control, struct cmsghdr *);
		goto morespace;
	}

	/* adjust message & mbuf to note amount of space actually used. */
	cm->cmsg_len = CMSG_LEN(nfds * sizeof(struct file *));
	control->m_len = CMSG_SPACE(nfds * sizeof(struct file *));

	/*
	 * Transform the file descriptors into struct file pointers, in
	 * reverse order so that if pointers are bigger than ints, the
	 * int won't get until we're done.
	 */
	fdp = ((int *)CMSG_DATA(cm)) + nfds - 1;
	rp = ((struct file **)CMSG_DATA(cm)) + nfds - 1;
	for (i = 0; i < nfds; i++) {
		fp = fdescp->fd_ofiles[*fdp--];
		simple_lock(&fp->f_slock);
#ifdef DIAGNOSTIC
		if (fp->f_iflags & FIF_WANTCLOSE)
			panic("unp_internalize: file already closed");
#endif
		*rp-- = fp;
		fp->f_count++;
		fp->f_msgcount++;
		simple_unlock(&fp->f_slock);
		unp_rights++;
	}
	return (0);
}

struct mbuf *
unp_addsockcred(p, control)
	struct proc *p;
	struct mbuf *control;
{
	struct cmsghdr *cmp;
	struct sockcred *sc;
	struct mbuf *m, *n;
	int len, space, i;

	len = CMSG_LEN(SOCKCREDSIZE(p->p_ucred->cr_ngroups));
	space = CMSG_SPACE(SOCKCREDSIZE(p->p_ucred->cr_ngroups));

	m = m_get(M_WAIT, MT_CONTROL);
	if (space > MLEN) {
		if (space > MCLBYTES)
			MEXTMALLOC(m, space, M_WAITOK);
		else
			m_clget(m, M_WAIT);
		if ((m->m_flags & M_EXT) == 0) {
			m_free(m);
			return (control);
		}
	}

	m->m_len = space;
	m->m_next = NULL;
	cmp = mtod(m, struct cmsghdr *);
	sc = (struct sockcred *)CMSG_DATA(cmp);
	cmp->cmsg_len = len;
	cmp->cmsg_level = SOL_SOCKET;
	cmp->cmsg_type = SCM_CREDS;
	sc->sc_uid = p->p_cred->p_ruid;
	sc->sc_euid = p->p_ucred->cr_uid;
	sc->sc_gid = p->p_cred->p_rgid;
	sc->sc_egid = p->p_ucred->cr_gid;
	sc->sc_ngroups = p->p_ucred->cr_ngroups;
	for (i = 0; i < sc->sc_ngroups; i++)
		sc->sc_groups[i] = p->p_ucred->cr_groups[i];

	/*
	 * If a control message already exists, append us to the end.
	 */
	if (control != NULL) {
		for (n = control; n->m_next != NULL; n = n->m_next)
			;
		n->m_next = m;
	} else
		control = m;

	return (control);
}

int	unp_defer, unp_gcing;
extern	struct domain unixdomain;

/*
 * Comment added long after the fact explaining what's going on here.
 * Do a mark-sweep GC of file descriptors on the system, to free up
 * any which are caught in flight to an about-to-be-closed socket.
 *
 * Traditional mark-sweep gc's start at the "root", and mark
 * everything reachable from the root (which, in our case would be the
 * process table).  The mark bits are cleared during the sweep.
 *
 * XXX For some inexplicable reason (perhaps because the file
 * descriptor tables used to live in the u area which could be swapped
 * out and thus hard to reach), we do multiple scans over the set of
 * descriptors, using use *two* mark bits per object (DEFER and MARK).
 * Whenever we find a descriptor which references other descriptors,
 * the ones it references are marked with both bits, and we iterate
 * over the whole file table until there are no more DEFER bits set.
 * We also make an extra pass *before* the GC to clear the mark bits,
 * which could have been cleared at almost no cost during the previous
 * sweep.
 *
 * XXX MP: this needs to run with locks such that no other thread of
 * control can create or destroy references to file descriptors. it
 * may be necessary to defer the GC until later (when the locking
 * situation is more hospitable); it may be necessary to push this
 * into a separate thread.
 */
void
unp_gc()
{
	struct file *fp, *nextfp;
	struct socket *so, *so1;
	struct file **extra_ref, **fpp;
	int nunref, i;

	if (unp_gcing)
		return;
	unp_gcing = 1;
	unp_defer = 0;

	/* Clear mark bits */
	LIST_FOREACH(fp, &filehead, f_list)
		fp->f_flag &= ~(FMARK|FDEFER);

	/*
	 * Iterate over the set of descriptors, marking ones believed
	 * (based on refcount) to be referenced from a process, and
	 * marking for rescan descriptors which are queued on a socket.
	 */
	do {
		LIST_FOREACH(fp, &filehead, f_list) {
			if (fp->f_flag & FDEFER) {
				fp->f_flag &= ~FDEFER;
				unp_defer--;
#ifdef DIAGNOSTIC
				if (fp->f_count == 0)
					panic("unp_gc: deferred unreferenced socket");
#endif
			} else {
				if (fp->f_count == 0)
					continue;
				if (fp->f_flag & FMARK)
					continue;
				if (fp->f_count == fp->f_msgcount)
					continue;
			}
			fp->f_flag |= FMARK;

			if (fp->f_type != DTYPE_SOCKET ||
			    (so = (struct socket *)fp->f_data) == 0)
				continue;
			if (so->so_proto->pr_domain != &unixdomain ||
			    (so->so_proto->pr_flags&PR_RIGHTS) == 0)
				continue;
#ifdef notdef
			if (so->so_rcv.sb_flags & SB_LOCK) {
				/*
				 * This is problematical; it's not clear
				 * we need to wait for the sockbuf to be
				 * unlocked (on a uniprocessor, at least),
				 * and it's also not clear what to do
				 * if sbwait returns an error due to receipt
				 * of a signal.  If sbwait does return
				 * an error, we'll go into an infinite
				 * loop.  Delete all of this for now.
				 */
				(void) sbwait(&so->so_rcv);
				goto restart;
			}
#endif
			unp_scan(so->so_rcv.sb_mb, unp_mark, 0);
			/*
			 * mark descriptors referenced from sockets queued on the accept queue as well.
			 */
			if (so->so_options & SO_ACCEPTCONN) {
				TAILQ_FOREACH(so1, &so->so_q0, so_qe) {
					unp_scan(so1->so_rcv.sb_mb, unp_mark, 0);
				}
				TAILQ_FOREACH(so1, &so->so_q, so_qe) {
					unp_scan(so1->so_rcv.sb_mb, unp_mark, 0);
				}
			}
			
		}
	} while (unp_defer);
	/*
	 * Sweep pass.  Find unmarked descriptors, and free them.
	 *
	 * We grab an extra reference to each of the file table entries
	 * that are not otherwise accessible and then free the rights
	 * that are stored in messages on them.
	 *
	 * The bug in the original code is a little tricky, so I'll describe
	 * what's wrong with it here.
	 *
	 * It is incorrect to simply unp_discard each entry for f_msgcount
	 * times -- consider the case of sockets A and B that contain
	 * references to each other.  On a last close of some other socket,
	 * we trigger a gc since the number of outstanding rights (unp_rights)
	 * is non-zero.  If during the sweep phase the gc code un_discards,
	 * we end up doing a (full) closef on the descriptor.  A closef on A
	 * results in the following chain.  Closef calls soo_close, which
	 * calls soclose.   Soclose calls first (through the switch
	 * uipc_usrreq) unp_detach, which re-invokes unp_gc.  Unp_gc simply
	 * returns because the previous instance had set unp_gcing, and
	 * we return all the way back to soclose, which marks the socket
	 * with SS_NOFDREF, and then calls sofree.  Sofree calls sorflush
	 * to free up the rights that are queued in messages on the socket A,
	 * i.e., the reference on B.  The sorflush calls via the dom_dispose
	 * switch unp_dispose, which unp_scans with unp_discard.  This second
	 * instance of unp_discard just calls closef on B.
	 *
	 * Well, a similar chain occurs on B, resulting in a sorflush on B,
	 * which results in another closef on A.  Unfortunately, A is already
	 * being closed, and the descriptor has already been marked with
	 * SS_NOFDREF, and soclose panics at this point.
	 *
	 * Here, we first take an extra reference to each inaccessible
	 * descriptor.  Then, if the inaccessible descriptor is a
	 * socket, we call sorflush in case it is a Unix domain
	 * socket.  After we destroy all the rights carried in
	 * messages, we do a last closef to get rid of our extra
	 * reference.  This is the last close, and the unp_detach etc
	 * will shut down the socket.
	 *
	 * 91/09/19, bsy@cs.cmu.edu
	 */
	extra_ref = malloc(nfiles * sizeof(struct file *), M_FILE, M_WAITOK);
	for (nunref = 0, fp = LIST_FIRST(&filehead), fpp = extra_ref; fp != 0;
	    fp = nextfp) {
		nextfp = LIST_NEXT(fp, f_list);
		simple_lock(&fp->f_slock);
		if (fp->f_count != 0 &&
		    fp->f_count == fp->f_msgcount && !(fp->f_flag & FMARK)) {
			*fpp++ = fp;
			nunref++;
			fp->f_count++;
		}
		simple_unlock(&fp->f_slock);
	}
	for (i = nunref, fpp = extra_ref; --i >= 0; ++fpp) {
		fp = *fpp;
		simple_lock(&fp->f_slock);
		FILE_USE(fp);
		if (fp->f_type == DTYPE_SOCKET)
			sorflush((struct socket *)fp->f_data);
		FILE_UNUSE(fp, NULL);
	}
	for (i = nunref, fpp = extra_ref; --i >= 0; ++fpp) {
		fp = *fpp;
		simple_lock(&fp->f_slock);
		FILE_USE(fp);
		(void) closef(fp, (struct proc *)0);
	}
	free((caddr_t)extra_ref, M_FILE);
	unp_gcing = 0;
}

void
unp_dispose(m)
	struct mbuf *m;
{

	if (m)
		unp_scan(m, unp_discard, 1);
}

void
unp_scan(m0, op, discard)
	struct mbuf *m0;
	void (*op) __P((struct file *));
	int discard;
{
	struct mbuf *m;
	struct file **rp;
	struct cmsghdr *cm;
	int i;
	int qfds;

	while (m0) {
		for (m = m0; m; m = m->m_next) {
			if (m->m_type == MT_CONTROL &&
			    m->m_len >= sizeof(*cm)) {
				cm = mtod(m, struct cmsghdr *);
				if (cm->cmsg_level != SOL_SOCKET ||
				    cm->cmsg_type != SCM_RIGHTS)
					continue;
				qfds = (cm->cmsg_len - CMSG_ALIGN(sizeof(*cm)))
				    / sizeof(struct file *);
				rp = (struct file **)CMSG_DATA(cm);
				for (i = 0; i < qfds; i++) {
					struct file *fp = *rp;
					if (discard)
						*rp = 0;
					(*op)(fp);
					rp++;
				}
				break;		/* XXX, but saves time */
			}
		}
		m0 = m0->m_nextpkt;
	}
}

void
unp_mark(fp)
	struct file *fp;
{
	if (fp == NULL)
		return;
	
	if (fp->f_flag & FMARK)
		return;

	/* If we're already deferred, don't screw up the defer count */
	if (fp->f_flag & FDEFER)
		return;

	/*
	 * Minimize the number of deferrals...  Sockets are the only
	 * type of descriptor which can hold references to another
	 * descriptor, so just mark other descriptors, and defer
	 * unmarked sockets for the next pass.
	 */
	if (fp->f_type == DTYPE_SOCKET) {
		unp_defer++;
		if (fp->f_count == 0)
			panic("unp_mark: queued unref");
		fp->f_flag |= FDEFER;
	} else {
		fp->f_flag |= FMARK;
	}
	return;
}

void
unp_discard(fp)
	struct file *fp;
{
	if (fp == NULL)
		return;
	simple_lock(&fp->f_slock);
	fp->f_usecount++;	/* i.e. FILE_USE(fp) sans locking */
	fp->f_msgcount--;
	simple_unlock(&fp->f_slock);
	unp_rights--;
	(void) closef(fp, (struct proc *)0);
}
