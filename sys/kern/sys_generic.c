/*	$NetBSD: sys_generic.c,v 1.77 2003/08/07 16:31:54 agc Exp $	*/

/*
 * Copyright (c) 1982, 1986, 1989, 1993
 *	The Regents of the University of California.  All rights reserved.
 * (c) UNIX System Laboratories, Inc.
 * All or some portions of this file are derived from material licensed
 * to the University of California by American Telephone and Telegraph
 * Co. or Unix System Laboratories, Inc. and are reproduced herein with
 * the permission of UNIX System Laboratories, Inc.
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
 *	@(#)sys_generic.c	8.9 (Berkeley) 2/14/95
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: sys_generic.c,v 1.77 2003/08/07 16:31:54 agc Exp $");

#include "opt_ktrace.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/filedesc.h>
#include <sys/ioctl.h>
#include <sys/file.h>
#include <sys/proc.h>
#include <sys/socketvar.h>
#include <sys/signalvar.h>
#include <sys/uio.h>
#include <sys/kernel.h>
#include <sys/stat.h>
#include <sys/malloc.h>
#include <sys/poll.h>
#ifdef KTRACE
#include <sys/ktrace.h>
#endif

#include <sys/mount.h>
#include <sys/sa.h>
#include <sys/syscallargs.h>

int selscan __P((struct proc *, fd_mask *, fd_mask *, int, register_t *));
int pollscan __P((struct proc *, struct pollfd *, int, register_t *));

/*
 * Read system call.
 */
/* ARGSUSED */
int
sys_read(struct lwp *l, void *v, register_t *retval)
{
	struct sys_read_args /* {
		syscallarg(int)		fd;
		syscallarg(void *)	buf;
		syscallarg(size_t)	nbyte;
	} */ *uap = v;
	int		fd;
	struct file	*fp;
	struct proc	*p;
	struct filedesc	*fdp;

	fd = SCARG(uap, fd);
	p = l->l_proc;
	fdp = p->p_fd;

	if ((fp = fd_getfile(fdp, fd)) == NULL)
		return (EBADF);

	if ((fp->f_flag & FREAD) == 0) {
		simple_unlock(&fp->f_slock);
		return (EBADF);
	}

	FILE_USE(fp);

	/* dofileread() will unuse the descriptor for us */
	return (dofileread(p, fd, fp, SCARG(uap, buf), SCARG(uap, nbyte),
	    &fp->f_offset, FOF_UPDATE_OFFSET, retval));
}

int
dofileread(struct proc *p, int fd, struct file *fp, void *buf, size_t nbyte,
	off_t *offset, int flags, register_t *retval)
{
	struct uio	auio;
	struct iovec	aiov;
	size_t		cnt;
	int		error;
#ifdef KTRACE
	struct iovec	ktriov;
#endif
	error = 0;

	aiov.iov_base = (caddr_t)buf;
	aiov.iov_len = nbyte;
	auio.uio_iov = &aiov;
	auio.uio_iovcnt = 1;
	auio.uio_resid = nbyte;
	auio.uio_rw = UIO_READ;
	auio.uio_segflg = UIO_USERSPACE;
	auio.uio_procp = p;

	/*
	 * Reads return ssize_t because -1 is returned on error.  Therefore
	 * we must restrict the length to SSIZE_MAX to avoid garbage return
	 * values.
	 */
	if (auio.uio_resid > SSIZE_MAX) {
		error = EINVAL;
		goto out;
	}

#ifdef KTRACE
	/*
	 * if tracing, save a copy of iovec
	 */
	if (KTRPOINT(p, KTR_GENIO))
		ktriov = aiov;
#endif
	cnt = auio.uio_resid;
	error = (*fp->f_ops->fo_read)(fp, offset, &auio, fp->f_cred, flags);
	if (error)
		if (auio.uio_resid != cnt && (error == ERESTART ||
		    error == EINTR || error == EWOULDBLOCK))
			error = 0;
	cnt -= auio.uio_resid;
#ifdef KTRACE
	if (KTRPOINT(p, KTR_GENIO) && error == 0)
		ktrgenio(p, fd, UIO_READ, &ktriov, cnt, error);
#endif
	*retval = cnt;
 out:
	FILE_UNUSE(fp, p);
	return (error);
}

/*
 * Scatter read system call.
 */
int
sys_readv(struct lwp *l, void *v, register_t *retval)
{
	struct sys_readv_args /* {
		syscallarg(int)				fd;
		syscallarg(const struct iovec *)	iovp;
		syscallarg(int)				iovcnt;
	} */ *uap = v;
	int		fd;
	struct file	*fp;
	struct proc	*p;
	struct filedesc	*fdp;

	fd = SCARG(uap, fd);
	p = l->l_proc;
	fdp = p->p_fd;

	if ((fp = fd_getfile(fdp, fd)) == NULL)
		return (EBADF);

	if ((fp->f_flag & FREAD) == 0) {
		simple_unlock(&fp->f_slock);
		return (EBADF);
	}

	FILE_USE(fp);

	/* dofilereadv() will unuse the descriptor for us */
	return (dofilereadv(p, fd, fp, SCARG(uap, iovp), SCARG(uap, iovcnt),
	    &fp->f_offset, FOF_UPDATE_OFFSET, retval));
}

int
dofilereadv(struct proc *p, int fd, struct file *fp, const struct iovec *iovp,
	int iovcnt, off_t *offset, int flags, register_t *retval)
{
	struct uio	auio;
	struct iovec	*iov, *needfree, aiov[UIO_SMALLIOV];
	int		i, error;
	size_t		cnt;
	u_int		iovlen;
#ifdef KTRACE
	struct iovec	*ktriov;
#endif

	error = 0;
#ifdef KTRACE
	ktriov = NULL;
#endif
	/* note: can't use iovlen until iovcnt is validated */
	iovlen = iovcnt * sizeof(struct iovec);
	if ((u_int)iovcnt > UIO_SMALLIOV) {
		if ((u_int)iovcnt > IOV_MAX) {
			error = EINVAL;
			goto out;
		}
		iov = malloc(iovlen, M_IOV, M_WAITOK);
		needfree = iov;
	} else if ((u_int)iovcnt > 0) {
		iov = aiov;
		needfree = NULL;
	} else {
		error = EINVAL;
		goto out;
	}

	auio.uio_iov = iov;
	auio.uio_iovcnt = iovcnt;
	auio.uio_rw = UIO_READ;
	auio.uio_segflg = UIO_USERSPACE;
	auio.uio_procp = p;
	error = copyin(iovp, iov, iovlen);
	if (error)
		goto done;
	auio.uio_resid = 0;
	for (i = 0; i < iovcnt; i++) {
		auio.uio_resid += iov->iov_len;
		/*
		 * Reads return ssize_t because -1 is returned on error.
		 * Therefore we must restrict the length to SSIZE_MAX to
		 * avoid garbage return values.
		 */
		if (iov->iov_len > SSIZE_MAX || auio.uio_resid > SSIZE_MAX) {
			error = EINVAL;
			goto done;
		}
		iov++;
	}
#ifdef KTRACE
	/*
	 * if tracing, save a copy of iovec
	 */
	if (KTRPOINT(p, KTR_GENIO))  {
		ktriov = malloc(iovlen, M_TEMP, M_WAITOK);
		memcpy((caddr_t)ktriov, (caddr_t)auio.uio_iov, iovlen);
	}
#endif
	cnt = auio.uio_resid;
	error = (*fp->f_ops->fo_read)(fp, offset, &auio, fp->f_cred, flags);
	if (error)
		if (auio.uio_resid != cnt && (error == ERESTART ||
		    error == EINTR || error == EWOULDBLOCK))
			error = 0;
	cnt -= auio.uio_resid;
#ifdef KTRACE
	if (ktriov != NULL) {
		if (error == 0)
			ktrgenio(p, fd, UIO_READ, ktriov, cnt, error);
		free(ktriov, M_TEMP);
	}
#endif
	*retval = cnt;
 done:
	if (needfree)
		free(needfree, M_IOV);
 out:
	FILE_UNUSE(fp, p);
	return (error);
}

/*
 * Write system call
 */
int
sys_write(struct lwp *l, void *v, register_t *retval)
{
	struct sys_write_args /* {
		syscallarg(int)			fd;
		syscallarg(const void *)	buf;
		syscallarg(size_t)		nbyte;
	} */ *uap = v;
	int		fd;
	struct file	*fp;
	struct proc	*p;
	struct filedesc	*fdp;

	fd = SCARG(uap, fd);
	p = l->l_proc;
	fdp = p->p_fd;

	if ((fp = fd_getfile(fdp, fd)) == NULL)
		return (EBADF);

	if ((fp->f_flag & FWRITE) == 0) {
		simple_unlock(&fp->f_slock);
		return (EBADF);
	}

	FILE_USE(fp);

	/* dofilewrite() will unuse the descriptor for us */
	return (dofilewrite(p, fd, fp, SCARG(uap, buf), SCARG(uap, nbyte),
	    &fp->f_offset, FOF_UPDATE_OFFSET, retval));
}

int
dofilewrite(struct proc *p, int fd, struct file *fp, const void *buf,
	size_t nbyte, off_t *offset, int flags, register_t *retval)
{
	struct uio	auio;
	struct iovec	aiov;
	size_t		cnt;
	int		error;
#ifdef KTRACE
	struct iovec	ktriov;
#endif

	error = 0;
	aiov.iov_base = (caddr_t)buf;		/* XXX kills const */
	aiov.iov_len = nbyte;
	auio.uio_iov = &aiov;
	auio.uio_iovcnt = 1;
	auio.uio_resid = nbyte;
	auio.uio_rw = UIO_WRITE;
	auio.uio_segflg = UIO_USERSPACE;
	auio.uio_procp = p;

	/*
	 * Writes return ssize_t because -1 is returned on error.  Therefore
	 * we must restrict the length to SSIZE_MAX to avoid garbage return
	 * values.
	 */
	if (auio.uio_resid > SSIZE_MAX) {
		error = EINVAL;
		goto out;
	}

#ifdef KTRACE
	/*
	 * if tracing, save a copy of iovec
	 */
	if (KTRPOINT(p, KTR_GENIO))
		ktriov = aiov;
#endif
	cnt = auio.uio_resid;
	error = (*fp->f_ops->fo_write)(fp, offset, &auio, fp->f_cred, flags);
	if (error) {
		if (auio.uio_resid != cnt && (error == ERESTART ||
		    error == EINTR || error == EWOULDBLOCK))
			error = 0;
		if (error == EPIPE)
			psignal(p, SIGPIPE);
	}
	cnt -= auio.uio_resid;
#ifdef KTRACE
	if (KTRPOINT(p, KTR_GENIO) && error == 0)
		ktrgenio(p, fd, UIO_WRITE, &ktriov, cnt, error);
#endif
	*retval = cnt;
 out:
	FILE_UNUSE(fp, p);
	return (error);
}

/*
 * Gather write system call
 */
int
sys_writev(struct lwp *l, void *v, register_t *retval)
{
	struct sys_writev_args /* {
		syscallarg(int)				fd;
		syscallarg(const struct iovec *)	iovp;
		syscallarg(int)				iovcnt;
	} */ *uap = v;
	int		fd;
	struct file	*fp;
	struct proc	*p;
	struct filedesc	*fdp;

	fd = SCARG(uap, fd);
	p = l->l_proc;
	fdp = p->p_fd;

	if ((fp = fd_getfile(fdp, fd)) == NULL)
		return (EBADF);

	if ((fp->f_flag & FWRITE) == 0) {
		simple_unlock(&fp->f_slock);
		return (EBADF);
	}

	FILE_USE(fp);

	/* dofilewritev() will unuse the descriptor for us */
	return (dofilewritev(p, fd, fp, SCARG(uap, iovp), SCARG(uap, iovcnt),
	    &fp->f_offset, FOF_UPDATE_OFFSET, retval));
}

int
dofilewritev(struct proc *p, int fd, struct file *fp, const struct iovec *iovp,
	int iovcnt, off_t *offset, int flags, register_t *retval)
{
	struct uio	auio;
	struct iovec	*iov, *needfree, aiov[UIO_SMALLIOV];
	int		i, error;
	size_t		cnt;
	u_int		iovlen;
#ifdef KTRACE
	struct iovec	*ktriov;
#endif

	error = 0;
#ifdef KTRACE
	ktriov = NULL;
#endif
	/* note: can't use iovlen until iovcnt is validated */
	iovlen = iovcnt * sizeof(struct iovec);
	if ((u_int)iovcnt > UIO_SMALLIOV) {
		if ((u_int)iovcnt > IOV_MAX) {
			error = EINVAL;
			goto out;
		}
		iov = malloc(iovlen, M_IOV, M_WAITOK);
		needfree = iov;
	} else if ((u_int)iovcnt > 0) {
		iov = aiov;
		needfree = NULL;
	} else {
		error = EINVAL;
		goto out;
	}

	auio.uio_iov = iov;
	auio.uio_iovcnt = iovcnt;
	auio.uio_rw = UIO_WRITE;
	auio.uio_segflg = UIO_USERSPACE;
	auio.uio_procp = p;
	error = copyin(iovp, iov, iovlen);
	if (error)
		goto done;
	auio.uio_resid = 0;
	for (i = 0; i < iovcnt; i++) {
		auio.uio_resid += iov->iov_len;
		/*
		 * Writes return ssize_t because -1 is returned on error.
		 * Therefore we must restrict the length to SSIZE_MAX to
		 * avoid garbage return values.
		 */
		if (iov->iov_len > SSIZE_MAX || auio.uio_resid > SSIZE_MAX) {
			error = EINVAL;
			goto done;
		}
		iov++;
	}
#ifdef KTRACE
	/*
	 * if tracing, save a copy of iovec
	 */
	if (KTRPOINT(p, KTR_GENIO))  {
		ktriov = malloc(iovlen, M_TEMP, M_WAITOK);
		memcpy((caddr_t)ktriov, (caddr_t)auio.uio_iov, iovlen);
	}
#endif
	cnt = auio.uio_resid;
	error = (*fp->f_ops->fo_write)(fp, offset, &auio, fp->f_cred, flags);
	if (error) {
		if (auio.uio_resid != cnt && (error == ERESTART ||
		    error == EINTR || error == EWOULDBLOCK))
			error = 0;
		if (error == EPIPE)
			psignal(p, SIGPIPE);
	}
	cnt -= auio.uio_resid;
#ifdef KTRACE
	if (KTRPOINT(p, KTR_GENIO))
		if (error == 0) {
			ktrgenio(p, fd, UIO_WRITE, ktriov, cnt, error);
		free(ktriov, M_TEMP);
	}
#endif
	*retval = cnt;
 done:
	if (needfree)
		free(needfree, M_IOV);
 out:
	FILE_UNUSE(fp, p);
	return (error);
}

/*
 * Ioctl system call
 */
/* ARGSUSED */
int
sys_ioctl(struct lwp *l, void *v, register_t *retval)
{
	struct sys_ioctl_args /* {
		syscallarg(int)		fd;
		syscallarg(u_long)	com;
		syscallarg(caddr_t)	data;
	} */ *uap = v;
	struct file	*fp;
	struct proc	*p;
	struct filedesc	*fdp;
	u_long		com;
	int		error;
	u_int		size;
	caddr_t		data, memp;
	int		tmp;
#define	STK_PARAMS	128
	u_long		stkbuf[STK_PARAMS/sizeof(u_long)];

	error = 0;
	p = l->l_proc;
	fdp = p->p_fd;

	if ((fp = fd_getfile(fdp, SCARG(uap, fd))) == NULL)
		return (EBADF);

	FILE_USE(fp);

	if ((fp->f_flag & (FREAD | FWRITE)) == 0) {
		error = EBADF;
		com = 0;
		goto out;
	}

	switch (com = SCARG(uap, com)) {
	case FIONCLEX:
		fdp->fd_ofileflags[SCARG(uap, fd)] &= ~UF_EXCLOSE;
		goto out;

	case FIOCLEX:
		fdp->fd_ofileflags[SCARG(uap, fd)] |= UF_EXCLOSE;
		goto out;
	}

	/*
	 * Interpret high order word to find amount of data to be
	 * copied to/from the user's address space.
	 */
	size = IOCPARM_LEN(com);
	if (size > IOCPARM_MAX) {
		error = ENOTTY;
		goto out;
	}
	memp = NULL;
	if (size > sizeof(stkbuf)) {
		memp = (caddr_t)malloc((u_long)size, M_IOCTLOPS, M_WAITOK);
		data = memp;
	} else
		data = (caddr_t)stkbuf;
	if (com&IOC_IN) {
		if (size) {
			error = copyin(SCARG(uap, data), data, size);
			if (error) {
				if (memp)
					free(memp, M_IOCTLOPS);
				goto out;
			}
#ifdef KTRACE
			if (KTRPOINT(p, KTR_GENIO)) {
				struct iovec iov;
				iov.iov_base = SCARG(uap, data);
				iov.iov_len = size;
				ktrgenio(p, SCARG(uap, fd), UIO_WRITE, &iov,
					size, 0);
			}
#endif
		} else
			*(caddr_t *)data = SCARG(uap, data);
	} else if ((com&IOC_OUT) && size)
		/*
		 * Zero the buffer so the user always
		 * gets back something deterministic.
		 */
		memset(data, 0, size);
	else if (com&IOC_VOID)
		*(caddr_t *)data = SCARG(uap, data);

	switch (com) {

	case FIONBIO:
		if ((tmp = *(int *)data) != 0)
			fp->f_flag |= FNONBLOCK;
		else
			fp->f_flag &= ~FNONBLOCK;
		error = (*fp->f_ops->fo_ioctl)(fp, FIONBIO, (caddr_t)&tmp, p);
		break;

	case FIOASYNC:
		if ((tmp = *(int *)data) != 0)
			fp->f_flag |= FASYNC;
		else
			fp->f_flag &= ~FASYNC;
		error = (*fp->f_ops->fo_ioctl)(fp, FIOASYNC, (caddr_t)&tmp, p);
		break;

	case FIOSETOWN:
		tmp = *(int *)data;
		if (fp->f_type == DTYPE_SOCKET) {
			((struct socket *)fp->f_data)->so_pgid = tmp;
			error = 0;
			break;
		}
		if (tmp <= 0) {
			tmp = -tmp;
		} else {
			struct proc *p1 = pfind(tmp);
			if (p1 == 0) {
				error = ESRCH;
				break;
			}
			tmp = p1->p_pgrp->pg_id;
		}
		error = (*fp->f_ops->fo_ioctl)
			(fp, TIOCSPGRP, (caddr_t)&tmp, p);
		break;

	case FIOGETOWN:
		if (fp->f_type == DTYPE_SOCKET) {
			error = 0;
			*(int *)data = ((struct socket *)fp->f_data)->so_pgid;
			break;
		}
		error = (*fp->f_ops->fo_ioctl)(fp, TIOCGPGRP, data, p);
		if (error == 0)
			*(int *)data = -*(int *)data;
		break;

	default:
		error = (*fp->f_ops->fo_ioctl)(fp, com, data, p);
		/*
		 * Copy any data to user, size was
		 * already set and checked above.
		 */
		if (error == 0 && (com&IOC_OUT) && size) {
			error = copyout(data, SCARG(uap, data), size);
#ifdef KTRACE
			if (KTRPOINT(p, KTR_GENIO)) {
				struct iovec iov;
				iov.iov_base = SCARG(uap, data);
				iov.iov_len = size;
				ktrgenio(p, SCARG(uap, fd), UIO_READ, &iov,
					size, error);
			}
#endif
		}
		break;
	}
	if (memp)
		free(memp, M_IOCTLOPS);
 out:
	FILE_UNUSE(fp, p);
	switch (error) {
	case -1:
		printf("sys_ioctl: _IO%s%s('%c', %lu, %lu) returned -1: "
		    "pid=%d comm=%s\n",
		    (com & IOC_IN) ? "W" : "", (com & IOC_OUT) ? "R" : "",
		    (char)IOCGROUP(com), (com & 0xff), IOCPARM_LEN(com),
		    p->p_pid, p->p_comm);
		/* FALLTHROUGH */
	case EPASSTHROUGH:
		error = ENOTTY;
		/* FALLTHROUGH */
	default:
		return (error);
	}
}

int	selwait, nselcoll;

/*
 * Select system call.
 */
int
sys_select(struct lwp *l, void *v, register_t *retval)
{
	struct sys_select_args /* {
		syscallarg(int)			nd;
		syscallarg(fd_set *)		in;
		syscallarg(fd_set *)		ou;
		syscallarg(fd_set *)		ex;
		syscallarg(struct timeval *)	tv;
	} */ *uap = v;
	struct proc	*p;
	caddr_t		bits;
	char		smallbits[howmany(FD_SETSIZE, NFDBITS) *
			    sizeof(fd_mask) * 6];
	struct		timeval atv;
	int		s, ncoll, error, timo;
	size_t		ni;

	error = 0;
	p = l->l_proc;
	if (SCARG(uap, nd) < 0)
		return (EINVAL);
	if (SCARG(uap, nd) > p->p_fd->fd_nfiles) {
		/* forgiving; slightly wrong */
		SCARG(uap, nd) = p->p_fd->fd_nfiles;
	}
	ni = howmany(SCARG(uap, nd), NFDBITS) * sizeof(fd_mask);
	if (ni * 6 > sizeof(smallbits))
		bits = malloc(ni * 6, M_TEMP, M_WAITOK);
	else
		bits = smallbits;

#define	getbits(name, x)						\
	if (SCARG(uap, name)) {						\
		error = copyin(SCARG(uap, name), bits + ni * x, ni);	\
		if (error)						\
			goto done;					\
	} else								\
		memset(bits + ni * x, 0, ni);
	getbits(in, 0);
	getbits(ou, 1);
	getbits(ex, 2);
#undef	getbits

	timo = 0;
	if (SCARG(uap, tv)) {
		error = copyin(SCARG(uap, tv), (caddr_t)&atv,
			sizeof(atv));
		if (error)
			goto done;
		if (itimerfix(&atv)) {
			error = EINVAL;
			goto done;
		}
		s = splclock();
		timeradd(&atv, &time, &atv);
		splx(s);
	}

 retry:
	ncoll = nselcoll;
	l->l_flag |= L_SELECT;
	error = selscan(p, (fd_mask *)(bits + ni * 0),
			   (fd_mask *)(bits + ni * 3), SCARG(uap, nd), retval);
	if (error || *retval)
		goto done;
	if (SCARG(uap, tv)) {
		/*
		 * We have to recalculate the timeout on every retry.
		 */
		timo = hzto(&atv);
		if (timo <= 0)
			goto done;
	}
	s = splsched();
	if ((l->l_flag & L_SELECT) == 0 || nselcoll != ncoll) {
		splx(s);
		goto retry;
	}
	l->l_flag &= ~L_SELECT;
	error = tsleep((caddr_t)&selwait, PSOCK | PCATCH, "select", timo);
	splx(s);
	if (error == 0)
		goto retry;
 done:
	l->l_flag &= ~L_SELECT;
	/* select is not restarted after signals... */
	if (error == ERESTART)
		error = EINTR;
	if (error == EWOULDBLOCK)
		error = 0;
	if (error == 0) {

#define	putbits(name, x)						\
		if (SCARG(uap, name)) {					\
			error = copyout(bits + ni * x, SCARG(uap, name), ni); \
			if (error)					\
				goto out;				\
		}
		putbits(in, 3);
		putbits(ou, 4);
		putbits(ex, 5);
#undef putbits
	}
 out:
	if (ni * 6 > sizeof(smallbits))
		free(bits, M_TEMP);
	return (error);
}

int
selscan(struct proc *p, fd_mask *ibitp, fd_mask *obitp, int nfd,
	register_t *retval)
{
	struct filedesc	*fdp;
	int		msk, i, j, fd, n;
	fd_mask		ibits, obits;
	struct file	*fp;
	static const int flag[3] = { POLLRDNORM | POLLHUP | POLLERR,
			       POLLWRNORM | POLLHUP | POLLERR,
			       POLLRDBAND };

	fdp = p->p_fd;
	n = 0;
	for (msk = 0; msk < 3; msk++) {
		for (i = 0; i < nfd; i += NFDBITS) {
			ibits = *ibitp++;
			obits = 0;
			while ((j = ffs(ibits)) && (fd = i + --j) < nfd) {
				ibits &= ~(1 << j);
				if ((fp = fd_getfile(fdp, fd)) == NULL)
					return (EBADF);
				FILE_USE(fp);
				if ((*fp->f_ops->fo_poll)(fp, flag[msk], p)) {
					obits |= (1 << j);
					n++;
				}
				FILE_UNUSE(fp, p);
			}
			*obitp++ = obits;
		}
	}
	*retval = n;
	return (0);
}

/*
 * Poll system call.
 */
int
sys_poll(struct lwp *l, void *v, register_t *retval)
{
	struct sys_poll_args /* {
		syscallarg(struct pollfd *)	fds;
		syscallarg(u_int)		nfds;
		syscallarg(int)			timeout;
	} */ *uap = v;
	struct proc	*p;
	caddr_t		bits;
	char		smallbits[32 * sizeof(struct pollfd)];
	struct timeval	atv;
	int		s, ncoll, error, timo;
	size_t		ni;

	error = 0;
	p = l->l_proc;
	if (SCARG(uap, nfds) > p->p_fd->fd_nfiles) {
		/* forgiving; slightly wrong */
		SCARG(uap, nfds) = p->p_fd->fd_nfiles;
	}
	ni = SCARG(uap, nfds) * sizeof(struct pollfd);
	if (ni > sizeof(smallbits))
		bits = malloc(ni, M_TEMP, M_WAITOK);
	else
		bits = smallbits;

	error = copyin(SCARG(uap, fds), bits, ni);
	if (error)
		goto done;

	timo = 0;
	if (SCARG(uap, timeout) != INFTIM) {
		atv.tv_sec = SCARG(uap, timeout) / 1000;
		atv.tv_usec = (SCARG(uap, timeout) % 1000) * 1000;
		if (itimerfix(&atv)) {
			error = EINVAL;
			goto done;
		}
		s = splclock();
		timeradd(&atv, &time, &atv);
		splx(s);
	}

 retry:
	ncoll = nselcoll;
	l->l_flag |= L_SELECT;
	error = pollscan(p, (struct pollfd *)bits, SCARG(uap, nfds), retval);
	if (error || *retval)
		goto done;
	if (SCARG(uap, timeout) != INFTIM) {
		/*
		 * We have to recalculate the timeout on every retry.
		 */
		timo = hzto(&atv);
		if (timo <= 0)
			goto done;
	}
	s = splsched();
	if ((l->l_flag & L_SELECT) == 0 || nselcoll != ncoll) {
		splx(s);
		goto retry;
	}
	l->l_flag &= ~L_SELECT;
	error = tsleep((caddr_t)&selwait, PSOCK | PCATCH, "select", timo);
	splx(s);
	if (error == 0)
		goto retry;
 done:
	l->l_flag &= ~L_SELECT;
	/* poll is not restarted after signals... */
	if (error == ERESTART)
		error = EINTR;
	if (error == EWOULDBLOCK)
		error = 0;
	if (error == 0) {
		error = copyout(bits, SCARG(uap, fds), ni);
		if (error)
			goto out;
	}
 out:
	if (ni > sizeof(smallbits))
		free(bits, M_TEMP);
	return (error);
}

int
pollscan(struct proc *p, struct pollfd *fds, int nfd, register_t *retval)
{
	struct filedesc	*fdp;
	int		i, n;
	struct file	*fp;

	fdp = p->p_fd;
	n = 0;
	for (i = 0; i < nfd; i++, fds++) {
		if (fds->fd >= fdp->fd_nfiles) {
			fds->revents = POLLNVAL;
			n++;
		} else if (fds->fd < 0) {
			fds->revents = 0;
		} else {
			if ((fp = fd_getfile(fdp, fds->fd)) == NULL) {
				fds->revents = POLLNVAL;
				n++;
			} else {
				FILE_USE(fp);
				fds->revents = (*fp->f_ops->fo_poll)(fp,
				    fds->events | POLLERR | POLLHUP, p);
				if (fds->revents != 0)
					n++;
				FILE_UNUSE(fp, p);
			}
		}
	}
	*retval = n;
	return (0);
}

/*ARGSUSED*/
int
seltrue(dev_t dev, int events, struct proc *p)
{

	return (events & (POLLIN | POLLOUT | POLLRDNORM | POLLWRNORM));
}

/*
 * Record a select request.
 */
void
selrecord(struct proc *selector, struct selinfo *sip)
{
	struct lwp	*l;
	struct proc	*p;
	pid_t		mypid;

	mypid = selector->p_pid;
	if (sip->sel_pid == mypid)
		return;
	if (sip->sel_pid && (p = pfind(sip->sel_pid))) {
		LIST_FOREACH(l, &p->p_lwps, l_sibling) {
			if (l->l_wchan == (caddr_t)&selwait) {
				sip->sel_collision = 1;
				return;
			}
		}
	}

	sip->sel_pid = mypid;
}

/*
 * Do a wakeup when a selectable event occurs.
 */
void
selwakeup(sip)
	struct selinfo *sip;
{
	struct lwp *l;
	struct proc *p;
	int s;

	if (sip->sel_pid == 0)
		return;
	if (sip->sel_collision) {
		sip->sel_pid = 0;
		nselcoll++;
		sip->sel_collision = 0;
		wakeup((caddr_t)&selwait);
		return;
	}
	p = pfind(sip->sel_pid);
	sip->sel_pid = 0;
	if (p != NULL) {
		LIST_FOREACH(l, &p->p_lwps, l_sibling) {
			SCHED_LOCK(s);
			if (l->l_wchan == (caddr_t)&selwait) {
				if (l->l_stat == LSSLEEP)
					setrunnable(l);
				else
					unsleep(l);
			} else if (l->l_flag & L_SELECT)
				l->l_flag &= ~L_SELECT;
			SCHED_UNLOCK(s);
		}
	}
}
