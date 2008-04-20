/*	$NetBSD: uipc_usrreq.c,v 1.111 2008/04/20 07:47:18 mlelstv Exp $	*/

/*-
 * Copyright (c) 1998, 2000, 2004, 2008 The NetBSD Foundation, Inc.
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
 *	@(#)uipc_usrreq.c	8.9 (Berkeley) 5/14/95
 */

/*
 * Copyright (c) 1997 Christopher G. Demetriou.  All rights reserved.
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
__KERNEL_RCSID(0, "$NetBSD: uipc_usrreq.c,v 1.111 2008/04/20 07:47:18 mlelstv Exp $");

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
#include <sys/kauth.h>
#include <sys/kmem.h>
#include <sys/atomic.h>

/*
 * Unix communications domain.
 *
 * TODO:
 *	SEQPACKET, RDM
 *	rethink name space problems
 *	need a proper out-of-band
 */
const struct sockaddr_un sun_noname = {
	.sun_len = sizeof(sun_noname),
	.sun_family = AF_LOCAL,
};
ino_t	unp_ino;			/* prototype for fake inode numbers */

struct mbuf *unp_addsockcred(struct lwp *, struct mbuf *);

int
unp_output(struct mbuf *m, struct mbuf *control, struct unpcb *unp,
	struct lwp *l)
{
	struct socket *so2;
	const struct sockaddr_un *sun;

	so2 = unp->unp_conn->unp_socket;
	if (unp->unp_addr)
		sun = unp->unp_addr;
	else
		sun = &sun_noname;
	if (unp->unp_conn->unp_flags & UNP_WANTCRED)
		control = unp_addsockcred(l, control);
	if (sbappendaddr(&so2->so_rcv, (const struct sockaddr *)sun, m,
	    control) == 0) {
		unp_dispose(control);
		m_freem(control);
		m_freem(m);
		so2->so_rcv.sb_overflowed++;
		return (ENOBUFS);
	} else {
		sorwakeup(so2);
		return (0);
	}
}

void
unp_setsockaddr(struct unpcb *unp, struct mbuf *nam)
{
	const struct sockaddr_un *sun;

	if (unp->unp_addr)
		sun = unp->unp_addr;
	else
		sun = &sun_noname;
	nam->m_len = sun->sun_len;
	if (nam->m_len > MLEN)
		MEXTMALLOC(nam, nam->m_len, M_WAITOK);
	memcpy(mtod(nam, void *), sun, (size_t)nam->m_len);
}

void
unp_setpeeraddr(struct unpcb *unp, struct mbuf *nam)
{
	const struct sockaddr_un *sun;

	if (unp->unp_conn && unp->unp_conn->unp_addr)
		sun = unp->unp_conn->unp_addr;
	else
		sun = &sun_noname;
	nam->m_len = sun->sun_len;
	if (nam->m_len > MLEN)
		MEXTMALLOC(nam, nam->m_len, M_WAITOK);
	memcpy(mtod(nam, void *), sun, (size_t)nam->m_len);
}

/*ARGSUSED*/
int
uipc_usrreq(struct socket *so, int req, struct mbuf *m, struct mbuf *nam,
	struct mbuf *control, struct lwp *l)
{
	struct unpcb *unp = sotounpcb(so);
	struct socket *so2;
	struct proc *p;
	u_int newhiwat;
	int error = 0;

	if (req == PRU_CONTROL)
		return (EOPNOTSUPP);

#ifdef DIAGNOSTIC
	if (req != PRU_SEND && req != PRU_SENDOOB && control)
		panic("uipc_usrreq: unexpected control mbuf");
#endif
	p = l ? l->l_proc : NULL;
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
		KASSERT(l != NULL);
		error = unp_bind(unp, nam, l);
		break;

	case PRU_LISTEN:
		if (unp->unp_vnode == 0)
			error = EINVAL;
		break;

	case PRU_CONNECT:
		KASSERT(l != NULL);
		error = unp_connect(so, nam, l);
		break;

	case PRU_CONNECT2:
		error = unp_connect2(so, (struct socket *)nam, PRU_CONNECT2);
		break;

	case PRU_DISCONNECT:
		unp_disconnect(unp);
		break;

	case PRU_ACCEPT:
		unp_setpeeraddr(unp, nam);
		/*
		 * Mark the initiating STREAM socket as connected *ONLY*
		 * after it's been accepted.  This prevents a client from
		 * overrunning a server and receiving ECONNREFUSED.
		 */
		if (unp->unp_conn != NULL &&
		    (unp->unp_conn->unp_socket->so_state & SS_ISCONNECTING))
			soisconnected(unp->unp_conn->unp_socket);
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
			newhiwat = snd->sb_hiwat + unp->unp_cc - rcv->sb_cc;
			(void)chgsbsize(so2->so_uidinfo,
			    &snd->sb_hiwat, newhiwat, RLIM_INFINITY);
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
		if (control) {
			KASSERT(l != NULL);
			if ((error = unp_internalize(&control, l)) != 0) {
				m_freem(control);
				m_freem(m);
				break;
			}
		}
		switch (so->so_type) {

		case SOCK_DGRAM: {
			if (nam) {
				if ((so->so_state & SS_ISCONNECTED) != 0)
					error = EISCONN;
				else {
					KASSERT(l != NULL);
					error = unp_connect(so, nam, l);
				}
			} else {
				if ((so->so_state & SS_ISCONNECTED) == 0)
					error = ENOTCONN;
			}
			if (error) {
				unp_dispose(control);
				m_freem(control);
				m_freem(m);
				break;
			}
			KASSERT(p != NULL);
			error = unp_output(m, control, unp, l);
			if (nam)
				unp_disconnect(unp);
			break;
		}

		case SOCK_STREAM:
#define	rcv (&so2->so_rcv)
#define	snd (&so->so_snd)
			if (unp->unp_conn == NULL) {
				error = ENOTCONN;
				break;
			}
			so2 = unp->unp_conn->unp_socket;
			if (unp->unp_conn->unp_flags & UNP_WANTCRED) {
				/*
				 * Credentials are passed only once on
				 * SOCK_STREAM.
				 */
				unp->unp_conn->unp_flags &= ~UNP_WANTCRED;
				control = unp_addsockcred(l, control);
			}
			/*
			 * Send to paired receive port, and then reduce
			 * send buffer hiwater marks to maintain backpressure.
			 * Wake up readers.
			 */
			if (control) {
				if (sbappendcontrol(rcv, m, control) == 0) {
					unp_dispose(control);
					m_freem(control);
				}
			} else
				sbappend(rcv, m);
			snd->sb_mbmax -=
			    rcv->sb_mbcnt - unp->unp_conn->unp_mbcnt;
			unp->unp_conn->unp_mbcnt = rcv->sb_mbcnt;
			newhiwat = snd->sb_hiwat -
			    (rcv->sb_cc - unp->unp_conn->unp_cc);
			(void)chgsbsize(so->so_uidinfo,
			    &snd->sb_hiwat, newhiwat, RLIM_INFINITY);
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

		KASSERT(so->so_head == NULL);
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
uipc_ctloutput(int op, struct socket *so, int level, int optname,
	struct mbuf **mp)
{
	struct unpcb *unp = sotounpcb(so);
	struct mbuf *m = *mp;
	int optval = 0, error = 0;

	if (level != 0) {
		error = ENOPROTOOPT;
		if (op == PRCO_SETOPT && m)
			(void) m_free(m);
	} else switch (op) {

	case PRCO_SETOPT:
		switch (optname) {
		case LOCAL_CREDS:
		case LOCAL_CONNWAIT:
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
				case LOCAL_CONNWAIT:
					OPTSET(UNP_CONNWAIT);
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
		case LOCAL_PEEREID:
			if (unp->unp_flags & UNP_EIDSVALID) {
				*mp = m = m_get(M_WAIT, MT_SOOPTS);
				m->m_len = sizeof(struct unpcbid);
				*mtod(m, struct unpcbid *) = unp->unp_connid;
			} else {
				error = EINVAL;
			}
			break;
		case LOCAL_CREDS:
			*mp = m = m_get(M_WAIT, MT_SOOPTS);
			m->m_len = sizeof(int);

#define	OPTBIT(bit)	(unp->unp_flags & (bit) ? 1 : 0)

			optval = OPTBIT(UNP_WANTCRED);
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

u_int	unp_rights;			/* file descriptors in flight */

int
unp_attach(struct socket *so)
{
	struct unpcb *unp;
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
	memset((void *)unp, 0, sizeof(*unp));
	unp->unp_socket = so;
	so->so_pcb = unp;
	nanotime(&unp->unp_ctime);
	return (0);
}

void
unp_detach(struct unpcb *unp)
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
unp_bind(struct unpcb *unp, struct mbuf *nam, struct lwp *l)
{
	struct sockaddr_un *sun;
	vnode_t *vp;
	struct vattr vattr;
	size_t addrlen;
	struct proc *p;
	int error;
	struct nameidata nd;

	if (unp->unp_vnode != 0)
		return (EINVAL);

	if ((unp->unp_flags & UNP_BUSY) != 0) {
		/*
		 * EALREADY may not be strictly accurate, but since this
		 * is a major application error it's hardly a big deal.
		 */
		return (EALREADY);
	}
	unp->unp_flags |= UNP_BUSY;

	p = l->l_proc;
	/*
	 * Allocate the new sockaddr.  We have to allocate one
	 * extra byte so that we can ensure that the pathname
	 * is nul-terminated.
	 */
	addrlen = nam->m_len + 1;
	sun = malloc(addrlen, M_SONAME, M_WAITOK);
	m_copydata(nam, 0, nam->m_len, (void *)sun);
	*(((char *)sun) + nam->m_len) = '\0';

	NDINIT(&nd, CREATE, FOLLOW | LOCKPARENT | TRYEMULROOT, UIO_SYSSPACE,
	    sun->sun_path);

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
	vattr.va_mode = ACCESSPERMS & ~(p->p_cwdi->cwdi_cmask);
	error = VOP_CREATE(nd.ni_dvp, &nd.ni_vp, &nd.ni_cnd, &vattr);
	if (error)
		goto bad;
	vp = nd.ni_vp;
	vp->v_socket = unp->unp_socket;
	unp->unp_vnode = vp;
	unp->unp_addrlen = addrlen;
	unp->unp_addr = sun;
	unp->unp_connid.unp_pid = p->p_pid;
	unp->unp_connid.unp_euid = kauth_cred_geteuid(p->p_cred);
	unp->unp_connid.unp_egid = kauth_cred_getegid(p->p_cred);
	unp->unp_flags |= UNP_EIDSBIND;
	VOP_UNLOCK(vp, 0);
	unp->unp_flags &= ~UNP_BUSY;
	return (0);

 bad:
	free(sun, M_SONAME);
	unp->unp_flags &= ~UNP_BUSY;
	return (error);
}

int
unp_connect(struct socket *so, struct mbuf *nam, struct lwp *l)
{
	struct sockaddr_un *sun;
	vnode_t *vp;
	struct socket *so2, *so3;
	struct unpcb *unp, *unp2, *unp3;
	size_t addrlen;
	struct proc *p;
	int error;
	struct nameidata nd;

	unp = sotounpcb(so);
	if ((unp->unp_flags & UNP_BUSY) != 0) {
		/*
		 * EALREADY may not be strictly accurate, but since this
		 * is a major application error it's hardly a big deal.
		 */
		return (EALREADY);
	}
	unp->unp_flags |= UNP_BUSY;

	p = l->l_proc;
	/*
	 * Allocate a temporary sockaddr.  We have to allocate one extra
	 * byte so that we can ensure that the pathname is nul-terminated.
	 * When we establish the connection, we copy the other PCB's
	 * sockaddr to our own.
	 */
	addrlen = nam->m_len + 1;
	sun = malloc(addrlen, M_SONAME, M_WAITOK);
	m_copydata(nam, 0, nam->m_len, (void *)sun);
	*(((char *)sun) + nam->m_len) = '\0';

	NDINIT(&nd, LOOKUP, FOLLOW | LOCKLEAF | TRYEMULROOT, UIO_SYSSPACE,
	    sun->sun_path);

	if ((error = namei(&nd)) != 0)
		goto bad2;
	vp = nd.ni_vp;
	if (vp->v_type != VSOCK) {
		error = ENOTSOCK;
		goto bad;
	}
	if ((error = VOP_ACCESS(vp, VWRITE, l->l_cred)) != 0)
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
		unp3->unp_connid.unp_pid = p->p_pid;
		unp3->unp_connid.unp_euid = kauth_cred_geteuid(p->p_cred);
		unp3->unp_connid.unp_egid = kauth_cred_getegid(p->p_cred);
		unp3->unp_flags |= UNP_EIDSVALID;
		so2 = so3;
		if (unp2->unp_flags & UNP_EIDSBIND) {
			unp->unp_connid = unp2->unp_connid;
			unp->unp_flags |= UNP_EIDSVALID;
		}
	}
	error = unp_connect2(so, so2, PRU_CONNECT);
 bad:
	vput(vp);
 bad2:
	free(sun, M_SONAME);
	unp->unp_flags &= ~UNP_BUSY;
	return (error);
}

int
unp_connect2(struct socket *so, struct socket *so2, int req)
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
		if (req == PRU_CONNECT &&
		    ((unp->unp_flags | unp2->unp_flags) & UNP_CONNWAIT))
			soisconnecting(so);
		else
			soisconnected(so);
		soisconnected(so2);
		break;

	default:
		panic("unp_connect2");
	}
	return (0);
}

void
unp_disconnect(struct unpcb *unp)
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
unp_abort(struct unpcb *unp)
{
	unp_detach(unp);
}
#endif

void
unp_shutdown(struct unpcb *unp)
{
	struct socket *so;

	if (unp->unp_socket->so_type == SOCK_STREAM && unp->unp_conn &&
	    (so = unp->unp_conn->unp_socket))
		socantrcvmore(so);
}

void
unp_drop(struct unpcb *unp, int errno)
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
unp_drain(void)
{

}
#endif

int
unp_externalize(struct mbuf *rights, struct lwp *l)
{
	struct cmsghdr *cm = mtod(rights, struct cmsghdr *);
	struct proc *p = l->l_proc;
	int i, *fdp;
	file_t **rp;
	file_t *fp;
	int nfds, error = 0;

	nfds = (cm->cmsg_len - CMSG_ALIGN(sizeof(*cm))) /
	    sizeof(file_t *);
	rp = (file_t **)CMSG_DATA(cm);

	fdp = malloc(nfds * sizeof(int), M_TEMP, M_WAITOK);
	rw_enter(&p->p_cwdi->cwdi_lock, RW_READER);

	/* Make sure the recipient should be able to see the descriptors.. */
	if (p->p_cwdi->cwdi_rdir != NULL) {
		rp = (file_t **)CMSG_DATA(cm);
		for (i = 0; i < nfds; i++) {
			fp = *rp++;
			/*
			 * If we are in a chroot'ed directory, and
			 * someone wants to pass us a directory, make
			 * sure it's inside the subtree we're allowed
			 * to access.
			 */
			if (fp->f_type == DTYPE_VNODE) {
				vnode_t *vp = (vnode_t *)fp->f_data;
				if ((vp->v_type == VDIR) &&
				    !vn_isunder(vp, p->p_cwdi->cwdi_rdir, l)) {
					error = EPERM;
					break;
				}
			}
		}
	}

 restart:
	rp = (file_t **)CMSG_DATA(cm);
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
		if ((error = fd_alloc(p, 0, &fdp[i])) != 0) {
			/*
			 * Back out what we've done so far.
			 */
			for (--i; i >= 0; i--) {
				fd_abort(p, NULL, fdp[i]);
			}
			if (error == ENOSPC) {
				fd_tryexpand(p);
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
	}

	/*
	 * Now that adding them has succeeded, update all of the
	 * descriptor passing state.	 */
	rp = (file_t **)CMSG_DATA(cm);
	for (i = 0; i < nfds; i++) {
		fp = *rp++;
		atomic_dec_uint(&unp_rights);
		fd_affix(p, fp, fdp[i]);
		mutex_enter(&fp->f_lock);
		fp->f_msgcount--;
		mutex_exit(&fp->f_lock);
		/*
		 * Note that fd_affix() adds a reference to the file.
		 * The file may already have been closed by another
		 * LWP in the process, so we must drop the reference
		 * added by unp_internalize() with closef().
		 */
		closef(fp);
	}

	/*
	 * Copy temporary array to message and adjust length, in case of
	 * transition from large file_t pointers to ints.
	 */
	memcpy(CMSG_DATA(cm), fdp, nfds * sizeof(int));
	cm->cmsg_len = CMSG_LEN(nfds * sizeof(int));
	rights->m_len = CMSG_SPACE(nfds * sizeof(int));
 out:
	rw_exit(&p->p_cwdi->cwdi_lock);
	free(fdp, M_TEMP);
	return (error);
}

int
unp_internalize(struct mbuf **controlp, struct lwp *l)
{
	struct filedesc *fdescp = curlwp->l_fd;
	struct mbuf *control = *controlp;
	struct cmsghdr *newcm, *cm = mtod(control, struct cmsghdr *);
	file_t **rp, **files;
	file_t *fp;
	int i, fd, *fdp;
	int nfds, error;

	KASSERT(l == curlwp);

	error = 0;
	newcm = NULL;

	/* Sanity check the control message header. */
	if (cm->cmsg_type != SCM_RIGHTS || cm->cmsg_level != SOL_SOCKET ||
	    cm->cmsg_len != control->m_len)
		return (EINVAL);

	/*
	 * Verify that the file descriptors are valid, and acquire
	 * a reference to each.
	 */
	nfds = (cm->cmsg_len - CMSG_ALIGN(sizeof(*cm))) / sizeof(int);
	fdp = (int *)CMSG_DATA(cm);
	for (i = 0; i < nfds; i++) {
		fd = *fdp++;
		if ((fp = fd_getfile(fd)) == NULL) {
			nfds = i + 1;
			error = EBADF;
			goto out;
		}
	}

	/* Allocate new space and copy header into it. */
	newcm = malloc(CMSG_SPACE(nfds * sizeof(file_t *)), M_MBUF, M_WAITOK);
	if (newcm == NULL) {
		error = E2BIG;
		goto out;
	}
	memcpy(newcm, cm, sizeof(struct cmsghdr));
	files = (file_t **)CMSG_DATA(newcm);

	/*
	 * Transform the file descriptors into file_t pointers, in
	 * reverse order so that if pointers are bigger than ints, the
	 * int won't get until we're done.  No need to lock, as we have
	 * already validated the descriptors with fd_getfile().
	 */
	fdp = (int *)CMSG_DATA(cm) + nfds;
	rp = files + nfds;
	for (i = 0; i < nfds; i++) {
		fp = fdescp->fd_ofiles[*--fdp]->ff_file;
		KASSERT(fp != NULL);
		mutex_enter(&fp->f_lock);
		*--rp = fp;
		fp->f_count++;
		fp->f_msgcount++;
		mutex_exit(&fp->f_lock);
		atomic_inc_uint(&unp_rights);
	}

 out:
 	/* Release descriptor references. */
	fdp = (int *)CMSG_DATA(cm);
	for (i = 0; i < nfds; i++) {
		fd_putfile(*fdp++);
	}

	if (error == 0) {
		if (control->m_flags & M_EXT) {
			m_freem(control);
			*controlp = control = m_get(M_WAIT, MT_CONTROL);
		}
		MEXTADD(control, newcm, CMSG_SPACE(nfds * sizeof(file_t *)),
		    M_MBUF, NULL, NULL);
		cm = newcm;
		/*
		 * Adjust message & mbuf to note amount of space
		 * actually used.
		 */
		cm->cmsg_len = CMSG_LEN(nfds * sizeof(file_t *));
		control->m_len = CMSG_SPACE(nfds * sizeof(file_t *));
	}

	return error;
}

struct mbuf *
unp_addsockcred(struct lwp *l, struct mbuf *control)
{
	struct cmsghdr *cmp;
	struct sockcred *sc;
	struct mbuf *m, *n;
	int len, space, i;

	len = CMSG_LEN(SOCKCREDSIZE(kauth_cred_ngroups(l->l_cred)));
	space = CMSG_SPACE(SOCKCREDSIZE(kauth_cred_ngroups(l->l_cred)));

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
	sc->sc_uid = kauth_cred_getuid(l->l_cred);
	sc->sc_euid = kauth_cred_geteuid(l->l_cred);
	sc->sc_gid = kauth_cred_getgid(l->l_cred);
	sc->sc_egid = kauth_cred_getegid(l->l_cred);
	sc->sc_ngroups = kauth_cred_ngroups(l->l_cred);
	for (i = 0; i < sc->sc_ngroups; i++)
		sc->sc_groups[i] = kauth_cred_group(l->l_cred, i);

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
 */
void
unp_gc(void)
{
	file_t *fp, *nextfp;
	struct socket *so, *so1;
	file_t **extra_ref, **fpp;
	int nunref, nslots, i;

	if (atomic_swap_uint(&unp_gcing, 1) == 1)
		return;

 restart:
 	nslots = nfiles * 2;
 	extra_ref = kmem_alloc(nslots * sizeof(file_t *), KM_SLEEP);

	mutex_enter(&filelist_lock);
	unp_defer = 0;

	/* Clear mark bits */
	LIST_FOREACH(fp, &filehead, f_list) {
		atomic_and_uint(&fp->f_flag, ~(FMARK|FDEFER));
	}

	/*
	 * Iterate over the set of descriptors, marking ones believed
	 * (based on refcount) to be referenced from a process, and
	 * marking for rescan descriptors which are queued on a socket.
	 */
	do {
		LIST_FOREACH(fp, &filehead, f_list) {
			mutex_enter(&fp->f_lock);
			if (fp->f_flag & FDEFER) {
				atomic_and_uint(&fp->f_flag, ~FDEFER);
				unp_defer--;
				KASSERT(fp->f_count != 0);
			} else {
				if (fp->f_count == 0 ||
				    (fp->f_flag & FMARK) ||
				    fp->f_count == fp->f_msgcount) {
					mutex_exit(&fp->f_lock);
					continue;
				}
			}
			atomic_or_uint(&fp->f_flag, FMARK);

			if (fp->f_type != DTYPE_SOCKET ||
			    (so = (struct socket *)fp->f_data) == NULL ||
			    so->so_proto->pr_domain != &unixdomain ||
			    (so->so_proto->pr_flags&PR_RIGHTS) == 0) {
				mutex_exit(&fp->f_lock);
				continue;
			}
#ifdef notdef
			if (so->so_rcv.sb_flags & SB_LOCK) {
				mutex_exit(&fp->f_lock);
				mutex_exit(&filelist_lock);
				kmem_free(extra_ref, nslots * sizeof(file_t *));
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
			mutex_exit(&fp->f_lock);

			unp_scan(so->so_rcv.sb_mb, unp_mark, 0);
			/*
			 * Mark descriptors referenced from sockets queued
			 * on the accept queue as well.
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
	if (nslots < nfiles) {
		mutex_exit(&filelist_lock);
		kmem_free(extra_ref, nslots * sizeof(file_t *));
		goto restart;
	}
	for (nunref = 0, fp = LIST_FIRST(&filehead), fpp = extra_ref; fp != 0;
	    fp = nextfp) {
		nextfp = LIST_NEXT(fp, f_list);
		mutex_enter(&fp->f_lock);
		if (fp->f_count != 0 &&
		    fp->f_count == fp->f_msgcount && !(fp->f_flag & FMARK)) {
			*fpp++ = fp;
			nunref++;
			fp->f_count++;
		}
		mutex_exit(&fp->f_lock);
	}
	mutex_exit(&filelist_lock);

	for (i = nunref, fpp = extra_ref; --i >= 0; ++fpp) {
		fp = *fpp;
		if (fp->f_type == DTYPE_SOCKET)
			sorflush(fp->f_data);
	}
	for (i = nunref, fpp = extra_ref; --i >= 0; ++fpp) {
		closef(*fpp);
	}
	kmem_free(extra_ref, nslots * sizeof(file_t *));
	atomic_swap_uint(&unp_gcing, 0);
}

void
unp_dispose(struct mbuf *m)
{

	if (m)
		unp_scan(m, unp_discard, 1);
}

void
unp_scan(struct mbuf *m0, void (*op)(file_t *), int discard)
{
	struct mbuf *m;
	file_t **rp;
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
				    / sizeof(file_t *);
				rp = (file_t **)CMSG_DATA(cm);
				for (i = 0; i < qfds; i++) {
					file_t *fp = *rp;
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
unp_mark(file_t *fp)
{

	if (fp == NULL)
		return;

	/* If we're already deferred, don't screw up the defer count */
	mutex_enter(&fp->f_lock);
	if (fp->f_flag & (FMARK | FDEFER)) {
		mutex_exit(&fp->f_lock);
		return;
	}

	/*
	 * Minimize the number of deferrals...  Sockets are the only
	 * type of descriptor which can hold references to another
	 * descriptor, so just mark other descriptors, and defer
	 * unmarked sockets for the next pass.
	 */
	if (fp->f_type == DTYPE_SOCKET) {
		unp_defer++;
		KASSERT(fp->f_count != 0);
		atomic_or_uint(&fp->f_flag, FDEFER);
	} else {
		atomic_or_uint(&fp->f_flag, FMARK);
	}
	mutex_exit(&fp->f_lock);
	return;
}

void
unp_discard(file_t *fp)
{

	if (fp == NULL)
		return;

	mutex_enter(&fp->f_lock);
	KASSERT(fp->f_count > 0);
	fp->f_msgcount--;
	mutex_exit(&fp->f_lock);
	atomic_dec_uint(&unp_rights);
	(void)closef(fp);
}
