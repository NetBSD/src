/*	$NetBSD: uipc_syscalls.c,v 1.99 2006/05/16 21:00:02 christos Exp $	*/

/*
 * Copyright (c) 1982, 1986, 1989, 1990, 1993
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
 *	@(#)uipc_syscalls.c	8.6 (Berkeley) 2/14/95
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: uipc_syscalls.c,v 1.99 2006/05/16 21:00:02 christos Exp $");

#include "opt_ktrace.h"
#include "opt_pipe.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/filedesc.h>
#include <sys/proc.h>
#include <sys/file.h>
#include <sys/buf.h>
#include <sys/malloc.h>
#include <sys/mbuf.h>
#include <sys/protosw.h>
#include <sys/socket.h>
#include <sys/socketvar.h>
#include <sys/signalvar.h>
#include <sys/un.h>
#ifdef KTRACE
#include <sys/ktrace.h>
#endif
#include <sys/event.h>

#include <sys/mount.h>
#include <sys/sa.h>
#include <sys/syscallargs.h>

#include <uvm/uvm_extern.h>

static void adjust_rights(struct mbuf *m, int len, struct lwp *l);

/*
 * System call interface to the socket abstraction.
 */
extern const struct fileops socketops;

int
sys_socket(struct lwp *l, void *v, register_t *retval)
{
	struct sys_socket_args /* {
		syscallarg(int)	domain;
		syscallarg(int)	type;
		syscallarg(int)	protocol;
	} */ *uap = v;

	struct proc	*p;
	struct filedesc	*fdp;
	struct socket	*so;
	struct file	*fp;
	int		fd, error;

	p = l->l_proc;
	fdp = p->p_fd;
	/* falloc() will use the desciptor for us */
	if ((error = falloc(p, &fp, &fd)) != 0)
		return (error);
	fp->f_flag = FREAD|FWRITE;
	fp->f_type = DTYPE_SOCKET;
	fp->f_ops = &socketops;
	error = socreate(SCARG(uap, domain), &so, SCARG(uap, type),
			 SCARG(uap, protocol), l);
	if (error) {
		FILE_UNUSE(fp, l);
		fdremove(fdp, fd);
		ffree(fp);
	} else {
		fp->f_data = (caddr_t)so;
		FILE_SET_MATURE(fp);
		FILE_UNUSE(fp, l);
		*retval = fd;
	}
	return (error);
}

/* ARGSUSED */
int
sys_bind(struct lwp *l, void *v, register_t *retval)
{
	struct sys_bind_args /* {
		syscallarg(int)				s;
		syscallarg(const struct sockaddr *)	name;
		syscallarg(unsigned int)		namelen;
	} */ *uap = v;
	struct proc	*p;
	struct file	*fp;
	struct mbuf	*nam;
	int		error;

	p = l->l_proc;
	/* getsock() will use the descriptor for us */
	if ((error = getsock(p->p_fd, SCARG(uap, s), &fp)) != 0)
		return (error);
	error = sockargs(&nam, SCARG(uap, name), SCARG(uap, namelen),
	    MT_SONAME);
	if (error) {
		FILE_UNUSE(fp, l);
		return (error);
	}
	MCLAIM(nam, ((struct socket *)fp->f_data)->so_mowner);
	error = sobind((struct socket *)fp->f_data, nam, l);
	m_freem(nam);
	FILE_UNUSE(fp, l);
	return (error);
}

/* ARGSUSED */
int
sys_listen(struct lwp *l, void *v, register_t *retval)
{
	struct sys_listen_args /* {
		syscallarg(int)	s;
		syscallarg(int)	backlog;
	} */ *uap = v;
	struct proc	*p;
	struct file	*fp;
	int		error;

	p = l->l_proc;
	/* getsock() will use the descriptor for us */
	if ((error = getsock(p->p_fd, SCARG(uap, s), &fp)) != 0)
		return (error);
	error = solisten((struct socket *)fp->f_data, SCARG(uap, backlog));
	FILE_UNUSE(fp, l);
	return (error);
}

int
sys_accept(struct lwp *l, void *v, register_t *retval)
{
	struct sys_accept_args /* {
		syscallarg(int)			s;
		syscallarg(struct sockaddr *)	name;
		syscallarg(unsigned int *)	anamelen;
	} */ *uap = v;
	struct proc	*p;
	struct filedesc	*fdp;
	struct file	*fp;
	struct mbuf	*nam;
	unsigned int	namelen;
	int		error, s, fd;
	struct socket	*so;
	int		fflag;

	p = l->l_proc;
	fdp = p->p_fd;
	if (SCARG(uap, name) && (error = copyin(SCARG(uap, anamelen),
	    &namelen, sizeof(namelen))))
		return (error);

	/* getsock() will use the descriptor for us */
	if ((error = getsock(fdp, SCARG(uap, s), &fp)) != 0)
		return (error);
	s = splsoftnet();
	so = (struct socket *)fp->f_data;
	FILE_UNUSE(fp, l);
	if (!(so->so_proto->pr_flags & PR_LISTEN)) {
		splx(s);
		return (EOPNOTSUPP);
	}
	if ((so->so_options & SO_ACCEPTCONN) == 0) {
		splx(s);
		return (EINVAL);
	}
	if ((so->so_state & SS_NBIO) && so->so_qlen == 0) {
		splx(s);
		return (EWOULDBLOCK);
	}
	while (so->so_qlen == 0 && so->so_error == 0) {
		if (so->so_state & SS_CANTRCVMORE) {
			so->so_error = ECONNABORTED;
			break;
		}
		error = tsleep((caddr_t)&so->so_timeo, PSOCK | PCATCH,
			       netcon, 0);
		if (error) {
			splx(s);
			return (error);
		}
	}
	if (so->so_error) {
		error = so->so_error;
		so->so_error = 0;
		splx(s);
		return (error);
	}
	fflag = fp->f_flag;
	/* falloc() will use the descriptor for us */
	if ((error = falloc(p, &fp, &fd)) != 0) {
		splx(s);
		return (error);
	}
	*retval = fd;

	/* connection has been removed from the listen queue */
	KNOTE(&so->so_rcv.sb_sel.sel_klist, 0);

	{ struct socket *aso = TAILQ_FIRST(&so->so_q);
	  if (soqremque(aso, 1) == 0)
		panic("accept");
	  so = aso;
	}
	fp->f_type = DTYPE_SOCKET;
	fp->f_flag = fflag;
	fp->f_ops = &socketops;
	fp->f_data = (caddr_t)so;
	FILE_UNUSE(fp, l);
	nam = m_get(M_WAIT, MT_SONAME);
	if ((error = soaccept(so, nam)) == 0 && SCARG(uap, name)) {
		if (namelen > nam->m_len)
			namelen = nam->m_len;
		/* SHOULD COPY OUT A CHAIN HERE */
		if ((error = copyout(mtod(nam, caddr_t),
		    (caddr_t)SCARG(uap, name), namelen)) == 0)
			error = copyout((caddr_t)&namelen,
			    (caddr_t)SCARG(uap, anamelen),
			    sizeof(*SCARG(uap, anamelen)));
	}
	/* if an error occurred, free the file descriptor */
	if (error) {
		fdremove(fdp, fd);
		ffree(fp);
	} else {
		FILE_SET_MATURE(fp);
	}
	m_freem(nam);
	splx(s);
	return (error);
}

/* ARGSUSED */
int
sys_connect(struct lwp *l, void *v, register_t *retval)
{
	struct sys_connect_args /* {
		syscallarg(int)				s;
		syscallarg(const struct sockaddr *)	name;
		syscallarg(unsigned int)		namelen;
	} */ *uap = v;
	struct proc	*p;
	struct file	*fp;
	struct socket	*so;
	struct mbuf	*nam;
	int		error, s;
	int		interrupted = 0;

	p = l->l_proc;
	/* getsock() will use the descriptor for us */
	if ((error = getsock(p->p_fd, SCARG(uap, s), &fp)) != 0)
		return (error);
	so = (struct socket *)fp->f_data;
	if (so->so_state & SS_ISCONNECTING) {
		error = EALREADY;
		goto out;
	}
	error = sockargs(&nam, SCARG(uap, name), SCARG(uap, namelen),
	    MT_SONAME);
	if (error)
		goto out;
	MCLAIM(nam, so->so_mowner);
	error = soconnect(so, nam, l);
	if (error)
		goto bad;
	if ((so->so_state & SS_NBIO) && (so->so_state & SS_ISCONNECTING)) {
		m_freem(nam);
		error = EINPROGRESS;
		goto out;
	}
	s = splsoftnet();
	while ((so->so_state & SS_ISCONNECTING) && so->so_error == 0) {
		error = tsleep((caddr_t)&so->so_timeo, PSOCK | PCATCH,
			       netcon, 0);
		if (error) {
			if (error == EINTR || error == ERESTART)
				interrupted = 1;
			break;
		}
	}
	if (error == 0) {
		error = so->so_error;
		so->so_error = 0;
	}
	splx(s);
 bad:
	if (!interrupted)
		so->so_state &= ~SS_ISCONNECTING;
	m_freem(nam);
	if (error == ERESTART)
		error = EINTR;
 out:
	FILE_UNUSE(fp, l);
	return (error);
}

int
sys_socketpair(struct lwp *l, void *v, register_t *retval)
{
	struct sys_socketpair_args /* {
		syscallarg(int)		domain;
		syscallarg(int)		type;
		syscallarg(int)		protocol;
		syscallarg(int *)	rsv;
	} */ *uap = v;
	struct proc *p;
	struct filedesc	*fdp;
	struct file	*fp1, *fp2;
	struct socket	*so1, *so2;
	int		fd, error, sv[2];

	p = l->l_proc;
	fdp = p->p_fd;
	error = socreate(SCARG(uap, domain), &so1, SCARG(uap, type),
			 SCARG(uap, protocol), l);
	if (error)
		return (error);
	error = socreate(SCARG(uap, domain), &so2, SCARG(uap, type),
			 SCARG(uap, protocol), l);
	if (error)
		goto free1;
	/* falloc() will use the descriptor for us */
	if ((error = falloc(p, &fp1, &fd)) != 0)
		goto free2;
	sv[0] = fd;
	fp1->f_flag = FREAD|FWRITE;
	fp1->f_type = DTYPE_SOCKET;
	fp1->f_ops = &socketops;
	fp1->f_data = (caddr_t)so1;
	if ((error = falloc(p, &fp2, &fd)) != 0)
		goto free3;
	fp2->f_flag = FREAD|FWRITE;
	fp2->f_type = DTYPE_SOCKET;
	fp2->f_ops = &socketops;
	fp2->f_data = (caddr_t)so2;
	sv[1] = fd;
	if ((error = soconnect2(so1, so2)) != 0)
		goto free4;
	if (SCARG(uap, type) == SOCK_DGRAM) {
		/*
		 * Datagram socket connection is asymmetric.
		 */
		 if ((error = soconnect2(so2, so1)) != 0)
			goto free4;
	}
	error = copyout((caddr_t)sv, (caddr_t)SCARG(uap, rsv),
	    2 * sizeof(int));
	FILE_SET_MATURE(fp1);
	FILE_SET_MATURE(fp2);
	FILE_UNUSE(fp1, l);
	FILE_UNUSE(fp2, l);
	return (error);
 free4:
	FILE_UNUSE(fp2, l);
	ffree(fp2);
	fdremove(fdp, sv[1]);
 free3:
	FILE_UNUSE(fp1, l);
	ffree(fp1);
	fdremove(fdp, sv[0]);
 free2:
	(void)soclose(so2);
 free1:
	(void)soclose(so1);
	return (error);
}

int
sys_sendto(struct lwp *l, void *v, register_t *retval)
{
	struct sys_sendto_args /* {
		syscallarg(int)				s;
		syscallarg(const void *)		buf;
		syscallarg(size_t)			len;
		syscallarg(int)				flags;
		syscallarg(const struct sockaddr *)	to;
		syscallarg(unsigned int)		tolen;
	} */ *uap = v;
	struct msghdr	msg;
	struct iovec	aiov;

	msg.msg_name = __UNCONST(SCARG(uap, to)); /* XXXUNCONST kills const */
	msg.msg_namelen = SCARG(uap, tolen);
	msg.msg_iov = &aiov;
	msg.msg_iovlen = 1;
	msg.msg_control = 0;
	msg.msg_flags = 0;
	aiov.iov_base = __UNCONST(SCARG(uap, buf)); /* XXXUNCONST kills const */
	aiov.iov_len = SCARG(uap, len);
	return (sendit(l, SCARG(uap, s), &msg, SCARG(uap, flags), retval));
}

int
sys_sendmsg(struct lwp *l, void *v, register_t *retval)
{
	struct sys_sendmsg_args /* {
		syscallarg(int)				s;
		syscallarg(const struct msghdr *)	msg;
		syscallarg(int)				flags;
	} */ *uap = v;
	struct msghdr	msg;
	struct iovec	aiov[UIO_SMALLIOV], *iov;
	int		error;

	error = copyin(SCARG(uap, msg), (caddr_t)&msg, sizeof(msg));
	if (error)
		return (error);
	if ((unsigned int)msg.msg_iovlen > UIO_SMALLIOV) {
		if ((unsigned int)msg.msg_iovlen > IOV_MAX)
			return (EMSGSIZE);
		iov = malloc(sizeof(struct iovec) * msg.msg_iovlen,
		    M_IOV, M_WAITOK);
	} else
		iov = aiov;
	if ((unsigned int)msg.msg_iovlen > 0) {
		error = copyin((caddr_t)msg.msg_iov, (caddr_t)iov,
		    (size_t)(msg.msg_iovlen * sizeof(struct iovec)));
		if (error)
			goto done;
	}
	msg.msg_iov = iov;
	msg.msg_flags = 0;
	error = sendit(l, SCARG(uap, s), &msg, SCARG(uap, flags), retval);
done:
	if (iov != aiov)
		free(iov, M_IOV);
	return (error);
}

int
sendit(struct lwp *l, int s, struct msghdr *mp, int flags, register_t *retsize)
{
	struct proc	*p;
	struct file	*fp;
	struct uio	auio;
	struct iovec	*iov;
	int		i, len, error;
	struct mbuf	*to, *control;
	struct socket	*so;
#ifdef KTRACE
	struct iovec	*ktriov;
#endif

#ifdef KTRACE
	ktriov = NULL;
#endif
	p = l->l_proc;
	/* getsock() will use the descriptor for us */
	if ((error = getsock(p->p_fd, s, &fp)) != 0)
		return (error);
	so = (struct socket *)fp->f_data;
	auio.uio_iov = mp->msg_iov;
	auio.uio_iovcnt = mp->msg_iovlen;
	auio.uio_rw = UIO_WRITE;
	auio.uio_offset = 0;			/* XXX */
	auio.uio_resid = 0;
	KASSERT(l == curlwp);
	auio.uio_vmspace = l->l_proc->p_vmspace;
	iov = mp->msg_iov;
	for (i = 0; i < mp->msg_iovlen; i++, iov++) {
#if 0
		/* cannot happen; iov_len is unsigned */
		if (iov->iov_len < 0) {
			error = EINVAL;
			goto out;
		}
#endif
		/*
		 * Writes return ssize_t because -1 is returned on error.
		 * Therefore, we must restrict the length to SSIZE_MAX to
		 * avoid garbage return values.
		 */
		auio.uio_resid += iov->iov_len;
		if (iov->iov_len > SSIZE_MAX || auio.uio_resid > SSIZE_MAX) {
			error = EINVAL;
			goto out;
		}
	}
	if (mp->msg_name) {
		error = sockargs(&to, mp->msg_name, mp->msg_namelen,
				 MT_SONAME);
		if (error)
			goto out;
		MCLAIM(to, so->so_mowner);
	} else
		to = 0;
	if (mp->msg_control) {
		if (mp->msg_controllen < sizeof(struct cmsghdr)) {
			error = EINVAL;
			goto bad;
		}
		error = sockargs(&control, mp->msg_control,
				 mp->msg_controllen, MT_CONTROL);
		if (error)
			goto bad;
		MCLAIM(control, so->so_mowner);
	} else
		control = 0;
#ifdef KTRACE
	if (KTRPOINT(p, KTR_GENIO)) {
		int iovlen = auio.uio_iovcnt * sizeof(struct iovec);

		ktriov = malloc(iovlen, M_TEMP, M_WAITOK);
		memcpy((caddr_t)ktriov, (caddr_t)auio.uio_iov, iovlen);
	}
#endif
	len = auio.uio_resid;
	error = (*so->so_send)(so, to, &auio, NULL, control, flags, l);
	if (error) {
		if (auio.uio_resid != len && (error == ERESTART ||
		    error == EINTR || error == EWOULDBLOCK))
			error = 0;
		if (error == EPIPE && (flags & MSG_NOSIGNAL) == 0)
			psignal(p, SIGPIPE);
	}
	if (error == 0)
		*retsize = len - auio.uio_resid;
#ifdef KTRACE
	if (ktriov != NULL) {
		if (error == 0)
			ktrgenio(l, s, UIO_WRITE, ktriov, *retsize, error);
		free(ktriov, M_TEMP);
	}
#endif
 bad:
	if (to)
		m_freem(to);
 out:
	FILE_UNUSE(fp, l);
	return (error);
}

int
sys_recvfrom(struct lwp *l, void *v, register_t *retval)
{
	struct sys_recvfrom_args /* {
		syscallarg(int)			s;
		syscallarg(void *)		buf;
		syscallarg(size_t)		len;
		syscallarg(int)			flags;
		syscallarg(struct sockaddr *)	from;
		syscallarg(unsigned int *)	fromlenaddr;
	} */ *uap = v;
	struct msghdr	msg;
	struct iovec	aiov;
	int		error;

	if (SCARG(uap, fromlenaddr)) {
		error = copyin((caddr_t)SCARG(uap, fromlenaddr),
			       (caddr_t)&msg.msg_namelen,
			       sizeof(msg.msg_namelen));
		if (error)
			return (error);
	} else
		msg.msg_namelen = 0;
	msg.msg_name = (caddr_t)SCARG(uap, from);
	msg.msg_iov = &aiov;
	msg.msg_iovlen = 1;
	aiov.iov_base = SCARG(uap, buf);
	aiov.iov_len = SCARG(uap, len);
	msg.msg_control = 0;
	msg.msg_flags = SCARG(uap, flags);
	return (recvit(l, SCARG(uap, s), &msg,
		       (caddr_t)SCARG(uap, fromlenaddr), retval));
}

int
sys_recvmsg(struct lwp *l, void *v, register_t *retval)
{
	struct sys_recvmsg_args /* {
		syscallarg(int)			s;
		syscallarg(struct msghdr *)	msg;
		syscallarg(int)			flags;
	} */ *uap = v;
	struct iovec	aiov[UIO_SMALLIOV], *uiov, *iov;
	struct msghdr	msg;
	int		error;

	error = copyin((caddr_t)SCARG(uap, msg), (caddr_t)&msg,
		       sizeof(msg));
	if (error)
		return (error);
	if ((unsigned int)msg.msg_iovlen > UIO_SMALLIOV) {
		if ((unsigned int)msg.msg_iovlen > IOV_MAX)
			return (EMSGSIZE);
		iov = malloc(sizeof(struct iovec) * msg.msg_iovlen,
		    M_IOV, M_WAITOK);
	} else
		iov = aiov;
	if ((unsigned int)msg.msg_iovlen > 0) {
		error = copyin((caddr_t)msg.msg_iov, (caddr_t)iov,
		    (size_t)(msg.msg_iovlen * sizeof(struct iovec)));
		if (error)
			goto done;
	}
	uiov = msg.msg_iov;
	msg.msg_iov = iov;
	msg.msg_flags = SCARG(uap, flags);
	if ((error = recvit(l, SCARG(uap, s), &msg, (caddr_t)0, retval)) == 0) {
		msg.msg_iov = uiov;
		error = copyout((caddr_t)&msg, (caddr_t)SCARG(uap, msg),
		    sizeof(msg));
	}
done:
	if (iov != aiov)
		free(iov, M_IOV);
	return (error);
}

/*
 * Adjust for a truncated SCM_RIGHTS control message.  This means
 *  closing any file descriptors that aren't entirely present in the
 *  returned buffer.  m is the mbuf holding the (already externalized)
 *  SCM_RIGHTS message; len is the length it is being truncated to.  p
 *  is the affected process.
 */
static void
adjust_rights(struct mbuf *m, int len, struct lwp *l)
{
	int nfd;
	int i;
	int nok;
	int *fdv;

	nfd = m->m_len < CMSG_SPACE(sizeof(int)) ? 0
	    : (m->m_len - CMSG_SPACE(sizeof(int))) / sizeof(int) + 1;
	nok = (len < CMSG_LEN(0)) ? 0 : ((len - CMSG_LEN(0)) / sizeof(int));
	fdv = (int *) CMSG_DATA(mtod(m,struct cmsghdr *));
	for (i = nok; i < nfd; i++)
		fdrelease(l,fdv[i]);
}

int
recvit(struct lwp *l, int s, struct msghdr *mp, caddr_t namelenp,
	register_t *retsize)
{
	struct proc	*p;
	struct file	*fp;
	struct uio	auio;
	struct iovec	*iov;
	int		i, len, error;
	struct mbuf	*from, *control;
	struct socket	*so;
#ifdef KTRACE
	struct iovec	*ktriov;
#endif

	p = l->l_proc;
	from = 0;
	control = 0;
#ifdef KTRACE
	ktriov = NULL;
#endif

	/* getsock() will use the descriptor for us */
	if ((error = getsock(p->p_fd, s, &fp)) != 0)
		return (error);
	so = (struct socket *)fp->f_data;
	auio.uio_iov = mp->msg_iov;
	auio.uio_iovcnt = mp->msg_iovlen;
	auio.uio_rw = UIO_READ;
	auio.uio_offset = 0;			/* XXX */
	auio.uio_resid = 0;
	KASSERT(l == curlwp);
	auio.uio_vmspace = l->l_proc->p_vmspace;
	iov = mp->msg_iov;
	for (i = 0; i < mp->msg_iovlen; i++, iov++) {
#if 0
		/* cannot happen iov_len is unsigned */
		if (iov->iov_len < 0) {
			error = EINVAL;
			goto out1;
		}
#endif
		/*
		 * Reads return ssize_t because -1 is returned on error.
		 * Therefore we must restrict the length to SSIZE_MAX to
		 * avoid garbage return values.
		 */
		auio.uio_resid += iov->iov_len;
		if (iov->iov_len > SSIZE_MAX || auio.uio_resid > SSIZE_MAX) {
			error = EINVAL;
			goto out1;
		}
	}
#ifdef KTRACE
	if (KTRPOINT(p, KTR_GENIO)) {
		int iovlen = auio.uio_iovcnt * sizeof(struct iovec);

		ktriov = malloc(iovlen, M_TEMP, M_WAITOK);
		memcpy((caddr_t)ktriov, (caddr_t)auio.uio_iov, iovlen);
	}
#endif
	len = auio.uio_resid;
	error = (*so->so_receive)(so, &from, &auio, NULL,
			  mp->msg_control ? &control : NULL, &mp->msg_flags);
	if (error) {
		if (auio.uio_resid != len && (error == ERESTART ||
		    error == EINTR || error == EWOULDBLOCK))
			error = 0;
	}
#ifdef KTRACE
	if (ktriov != NULL) {
		if (error == 0)
			ktrgenio(l, s, UIO_READ, ktriov,
			    len - auio.uio_resid, error);
		free(ktriov, M_TEMP);
	}
#endif
	if (error)
		goto out;
	*retsize = len - auio.uio_resid;
	if (mp->msg_name) {
		len = mp->msg_namelen;
		if (len <= 0 || from == 0)
			len = 0;
		else {
			if (len > from->m_len)
				len = from->m_len;
			/* else if len < from->m_len ??? */
			error = copyout(mtod(from, caddr_t),
					(caddr_t)mp->msg_name, (unsigned)len);
			if (error)
				goto out;
		}
		mp->msg_namelen = len;
		if (namelenp &&
		    (error = copyout((caddr_t)&len, namelenp, sizeof(int))))
			goto out;
	}
	if (mp->msg_control) {
		len = mp->msg_controllen;
		if (len <= 0 || control == 0)
			len = 0;
		else {
			struct mbuf *m = control;
			caddr_t q = (caddr_t)mp->msg_control;

			do {
				i = m->m_len;
				if (len < i) {
					mp->msg_flags |= MSG_CTRUNC;
					i = len;
					if (mtod(m, struct cmsghdr *)->
					    cmsg_type == SCM_RIGHTS)
						adjust_rights(m, len, l);
				}
				error = copyout(mtod(m, caddr_t), q,
				    (unsigned)i);
				m = m->m_next;
				if (m)
					i = ALIGN(i);
				q += i;
				len -= i;
				if (error != 0 || len <= 0)
					break;
			} while (m != NULL);
			while (m) {
				if (mtod(m, struct cmsghdr *)->
				    cmsg_type == SCM_RIGHTS)
					adjust_rights(m, 0, l);
				m = m->m_next;
			}
			len = q - (caddr_t)mp->msg_control;
		}
		mp->msg_controllen = len;
	}
 out:
	if (from)
		m_freem(from);
	if (control)
		m_freem(control);
 out1:
	FILE_UNUSE(fp, l);
	return (error);
}

/* ARGSUSED */
int
sys_shutdown(struct lwp *l, void *v, register_t *retval)
{
	struct sys_shutdown_args /* {
		syscallarg(int)	s;
		syscallarg(int)	how;
	} */ *uap = v;
	struct proc	*p;
	struct file	*fp;
	int		error;

	p = l->l_proc;
	/* getsock() will use the descriptor for us */
	if ((error = getsock(p->p_fd, SCARG(uap, s), &fp)) != 0)
		return (error);
	error = soshutdown((struct socket *)fp->f_data, SCARG(uap, how));
	FILE_UNUSE(fp, l);
	return (error);
}

/* ARGSUSED */
int
sys_setsockopt(struct lwp *l, void *v, register_t *retval)
{
	struct sys_setsockopt_args /* {
		syscallarg(int)			s;
		syscallarg(int)			level;
		syscallarg(int)			name;
		syscallarg(const void *)	val;
		syscallarg(unsigned int)	valsize;
	} */ *uap = v;
	struct proc	*p;
	struct file	*fp;
	struct mbuf	*m;
	struct socket	*so;
	int		error;
	unsigned int	len;

	p = l->l_proc;
	m = NULL;
	/* getsock() will use the descriptor for us */
	if ((error = getsock(p->p_fd, SCARG(uap, s), &fp)) != 0)
		return (error);
	so = (struct socket *)fp->f_data;
	len = SCARG(uap, valsize);
	if (len > MCLBYTES) {
		error = EINVAL;
		goto out;
	}
	if (SCARG(uap, val)) {
		m = m_get(M_WAIT, MT_SOOPTS);
		MCLAIM(m, so->so_mowner);
		if (len > MLEN)
			m_clget(m, M_WAIT);
		error = copyin(SCARG(uap, val), mtod(m, caddr_t), len);
		if (error) {
			(void) m_free(m);
			goto out;
		}
		m->m_len = SCARG(uap, valsize);
	}
	error = sosetopt(so, SCARG(uap, level), SCARG(uap, name), m);
 out:
	FILE_UNUSE(fp, l);
	return (error);
}

/* ARGSUSED */
int
sys_getsockopt(struct lwp *l, void *v, register_t *retval)
{
	struct sys_getsockopt_args /* {
		syscallarg(int)			s;
		syscallarg(int)			level;
		syscallarg(int)			name;
		syscallarg(void *)		val;
		syscallarg(unsigned int *)	avalsize;
	} */ *uap = v;
	struct proc	*p;
	struct file	*fp;
	struct mbuf	*m;
	unsigned int	op, i, valsize;
	int		error;

	p = l->l_proc;
	m = NULL;
	/* getsock() will use the descriptor for us */
	if ((error = getsock(p->p_fd, SCARG(uap, s), &fp)) != 0)
		return (error);
	if (SCARG(uap, val)) {
		error = copyin((caddr_t)SCARG(uap, avalsize),
			       (caddr_t)&valsize, sizeof(valsize));
		if (error)
			goto out;
	} else
		valsize = 0;
	if ((error = sogetopt((struct socket *)fp->f_data, SCARG(uap, level),
	    SCARG(uap, name), &m)) == 0 && SCARG(uap, val) && valsize &&
	    m != NULL) {
		op = 0;
		while (m && !error && op < valsize) {
			i = min(m->m_len, (valsize - op));
			error = copyout(mtod(m, caddr_t), SCARG(uap, val), i);
			op += i;
			SCARG(uap, val) = ((uint8_t *)SCARG(uap, val)) + i;
			m = m_free(m);
		}
		valsize = op;
		if (error == 0)
			error = copyout(&valsize,
					SCARG(uap, avalsize), sizeof(valsize));
	}
	if (m != NULL)
		(void) m_freem(m);
 out:
	FILE_UNUSE(fp, l);
	return (error);
}

#ifdef PIPE_SOCKETPAIR
/* ARGSUSED */
int
sys_pipe(struct lwp *l, void *v, register_t *retval)
{
	struct proc	*p;
	struct filedesc	*fdp;
	struct file	*rf, *wf;
	struct socket	*rso, *wso;
	int		fd, error;

	p = l->l_proc;
	fdp = p->p_fd;
	if ((error = socreate(AF_LOCAL, &rso, SOCK_STREAM, 0, l)) != 0)
		return (error);
	if ((error = socreate(AF_LOCAL, &wso, SOCK_STREAM, 0, l)) != 0)
		goto free1;
	/* remember this socket pair implements a pipe */
	wso->so_state |= SS_ISAPIPE;
	rso->so_state |= SS_ISAPIPE;
	/* falloc() will use the descriptor for us */
	if ((error = falloc(p, &rf, &fd)) != 0)
		goto free2;
	retval[0] = fd;
	rf->f_flag = FREAD;
	rf->f_type = DTYPE_SOCKET;
	rf->f_ops = &socketops;
	rf->f_data = (caddr_t)rso;
	if ((error = falloc(p, &wf, &fd)) != 0)
		goto free3;
	wf->f_flag = FWRITE;
	wf->f_type = DTYPE_SOCKET;
	wf->f_ops = &socketops;
	wf->f_data = (caddr_t)wso;
	retval[1] = fd;
	if ((error = unp_connect2(wso, rso, PRU_CONNECT2)) != 0)
		goto free4;
	FILE_SET_MATURE(rf);
	FILE_SET_MATURE(wf);
	FILE_UNUSE(rf, l);
	FILE_UNUSE(wf, l);
	return (0);
 free4:
	FILE_UNUSE(wf, l);
	ffree(wf);
	fdremove(fdp, retval[1]);
 free3:
	FILE_UNUSE(rf, l);
	ffree(rf);
	fdremove(fdp, retval[0]);
 free2:
	(void)soclose(wso);
 free1:
	(void)soclose(rso);
	return (error);
}
#endif /* PIPE_SOCKETPAIR */

/*
 * Get socket name.
 */
/* ARGSUSED */
int
sys_getsockname(struct lwp *l, void *v, register_t *retval)
{
	struct sys_getsockname_args /* {
		syscallarg(int)			fdes;
		syscallarg(struct sockaddr *)	asa;
		syscallarg(unsigned int *)	alen;
	} */ *uap = v;
	struct proc	*p;
	struct file	*fp;
	struct socket	*so;
	struct mbuf	*m;
	unsigned int	len;
	int		error;

	p = l->l_proc;
	/* getsock() will use the descriptor for us */
	if ((error = getsock(p->p_fd, SCARG(uap, fdes), &fp)) != 0)
		return (error);
	error = copyin((caddr_t)SCARG(uap, alen), (caddr_t)&len, sizeof(len));
	if (error)
		goto out;
	so = (struct socket *)fp->f_data;
	m = m_getclr(M_WAIT, MT_SONAME);
	MCLAIM(m, so->so_mowner);
	error = (*so->so_proto->pr_usrreq)(so, PRU_SOCKADDR, (struct mbuf *)0,
	    m, (struct mbuf *)0, (struct lwp *)0);
	if (error)
		goto bad;
	if (len > m->m_len)
		len = m->m_len;
	error = copyout(mtod(m, caddr_t), (caddr_t)SCARG(uap, asa), len);
	if (error == 0)
		error = copyout((caddr_t)&len, (caddr_t)SCARG(uap, alen),
		    sizeof(len));
 bad:
	m_freem(m);
 out:
	FILE_UNUSE(fp, l);
	return (error);
}

/*
 * Get name of peer for connected socket.
 */
/* ARGSUSED */
int
sys_getpeername(struct lwp *l, void *v, register_t *retval)
{
	struct sys_getpeername_args /* {
		syscallarg(int)			fdes;
		syscallarg(struct sockaddr *)	asa;
		syscallarg(unsigned int *)	alen;
	} */ *uap = v;
	struct proc	*p;
	struct file	*fp;
	struct socket	*so;
	struct mbuf	*m;
	unsigned int	len;
	int		error;

	p = l->l_proc;
	/* getsock() will use the descriptor for us */
	if ((error = getsock(p->p_fd, SCARG(uap, fdes), &fp)) != 0)
		return (error);
	so = (struct socket *)fp->f_data;
	if ((so->so_state & (SS_ISCONNECTED|SS_ISCONFIRMING)) == 0) {
		error = ENOTCONN;
		goto out;
	}
	error = copyin((caddr_t)SCARG(uap, alen), (caddr_t)&len, sizeof(len));
	if (error)
		goto out;
	m = m_getclr(M_WAIT, MT_SONAME);
	MCLAIM(m, so->so_mowner);
	error = (*so->so_proto->pr_usrreq)(so, PRU_PEERADDR, (struct mbuf *)0,
	    m, (struct mbuf *)0, (struct lwp *)0);
	if (error)
		goto bad;
	if (len > m->m_len)
		len = m->m_len;
	error = copyout(mtod(m, caddr_t), (caddr_t)SCARG(uap, asa), len);
	if (error)
		goto bad;
	error = copyout((caddr_t)&len, (caddr_t)SCARG(uap, alen), sizeof(len));
 bad:
	m_freem(m);
 out:
	FILE_UNUSE(fp, l);
	return (error);
}

/*
 * XXX In a perfect world, we wouldn't pass around socket control
 * XXX arguments in mbufs, and this could go away.
 */
int
sockargs(struct mbuf **mp, const void *bf, size_t buflen, int type)
{
	struct sockaddr	*sa;
	struct mbuf	*m;
	int		error;

	/*
	 * We can't allow socket names > UCHAR_MAX in length, since that
	 * will overflow sa_len.  Control data more than a page size in
	 * length is just too much.
	 */
	if (buflen > (type == MT_SONAME ? UCHAR_MAX : PAGE_SIZE))
		return (EINVAL);

	/* Allocate an mbuf to hold the arguments. */
	m = m_get(M_WAIT, type);
	/* can't claim.  don't who to assign it to. */
	if (buflen > MLEN) {
		/*
		 * Won't fit into a regular mbuf, so we allocate just
		 * enough external storage to hold the argument.
		 */
		MEXTMALLOC(m, buflen, M_WAITOK);
	}
	m->m_len = buflen;
	error = copyin(bf, mtod(m, caddr_t), buflen);
	if (error) {
		(void) m_free(m);
		return (error);
	}
	*mp = m;
	if (type == MT_SONAME) {
		sa = mtod(m, struct sockaddr *);
#if BYTE_ORDER != BIG_ENDIAN
		/*
		 * 4.3BSD compat thing - need to stay, since bind(2),
		 * connect(2), sendto(2) were not versioned for COMPAT_43.
		 */
		if (sa->sa_family == 0 && sa->sa_len < AF_MAX)
			sa->sa_family = sa->sa_len;
#endif
		sa->sa_len = buflen;
	}
	return (0);
}

int
getsock(struct filedesc *fdp, int fdes, struct file **fpp)
{
	struct file	*fp;

	if ((fp = fd_getfile(fdp, fdes)) == NULL)
		return (EBADF);

	FILE_USE(fp);

	if (fp->f_type != DTYPE_SOCKET) {
		FILE_UNUSE(fp, NULL);
		return (ENOTSOCK);
	}
	*fpp = fp;
	return (0);
}
