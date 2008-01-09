/*	$NetBSD: netbsd32_fs.c,v 1.46.2.1 2008/01/09 01:51:36 matt Exp $	*/

/*
 * Copyright (c) 1998, 2001 Matthew R. Green
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
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: netbsd32_fs.c,v 1.46.2.1 2008/01/09 01:51:36 matt Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/malloc.h>
#include <sys/mount.h>
#include <sys/socket.h>
#include <sys/socketvar.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/ktrace.h>
#include <sys/resourcevar.h>
#include <sys/vnode.h>
#include <sys/file.h>
#include <sys/filedesc.h>
#include <sys/namei.h>
#include <sys/statvfs.h>
#include <sys/syscallargs.h>
#include <sys/proc.h>
#include <sys/dirent.h>
#include <sys/kauth.h>
#include <sys/vfs_syscalls.h>

#include <compat/netbsd32/netbsd32.h>
#include <compat/netbsd32/netbsd32_syscallargs.h>
#include <compat/netbsd32/netbsd32_conv.h>
#include <compat/sys/mount.h>


static int dofilereadv32(struct lwp *, int, struct file *, struct netbsd32_iovec *,
			      int, off_t *, int, register_t *);
static int dofilewritev32(struct lwp *, int, struct file *, struct netbsd32_iovec *,
			       int,  off_t *, int, register_t *);

struct iovec *
netbsd32_get_iov(struct netbsd32_iovec *iov32, int iovlen, struct iovec *aiov,
    int aiov_len)
{
#define N_IOV32 8
	struct netbsd32_iovec aiov32[N_IOV32];
	struct iovec *iov = aiov;
	struct iovec *iovp;
	int i, n, j;
	int error;

	if (iovlen < 0 || iovlen > IOV_MAX)
		return NULL;

	if (iovlen > aiov_len)
		iov = malloc(iovlen * sizeof (*iov), M_TEMP, M_WAITOK);

	iovp = iov;
	for (i = 0; i < iovlen; iov32 += N_IOV32, i += N_IOV32) {
		n = iovlen - i;
		if (n > N_IOV32)
			n = N_IOV32;
		error = copyin(iov32, aiov32, n * sizeof (*iov32));
		if (error != 0) {
			if (iov != aiov)
				free(iov, M_TEMP);
			return NULL;
		}
		for (j = 0; j < n; iovp++, j++) {
			iovp->iov_base = NETBSD32PTR64(aiov32[j].iov_base);
			iovp->iov_len = aiov32[j].iov_len;
		}
	}
	return iov;
#undef N_IOV32
}

int
netbsd32_readv(struct lwp *l, const struct netbsd32_readv_args *uap, register_t *retval)
{
	/* {
		syscallarg(int) fd;
		syscallarg(const netbsd32_iovecp_t) iovp;
		syscallarg(int) iovcnt;
	} */
	int fd = SCARG(uap, fd);
	struct proc *p = l->l_proc;
	struct file *fp;
	struct filedesc *fdp = p->p_fd;

	if ((fp = fd_getfile(fdp, fd)) == NULL)
		return (EBADF);

	if ((fp->f_flag & FREAD) == 0) {
		FILE_UNLOCK(fp);
		return (EBADF);
	}

	FILE_USE(fp);

	return (dofilereadv32(l, fd, fp,
	    (struct netbsd32_iovec *)SCARG_P32(uap, iovp),
	    SCARG(uap, iovcnt), &fp->f_offset, FOF_UPDATE_OFFSET, retval));
}

/* Damn thing copies in the iovec! */
int
dofilereadv32(struct lwp *l, int fd, struct file *fp, struct netbsd32_iovec *iovp, int iovcnt, off_t *offset, int flags, register_t *retval)
{
	struct uio auio;
	struct iovec *iov;
	struct iovec *needfree;
	struct iovec aiov[UIO_SMALLIOV];
	long i, cnt, error = 0;
	u_int iovlen;
	struct iovec *ktriov = NULL;

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
	auio.uio_vmspace = l->l_proc->p_vmspace;
	error = netbsd32_to_iovecin(iovp, iov, iovcnt);
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

	/*
	 * if tracing, save a copy of iovec
	 */
	if (ktrpoint(KTR_GENIO)) {
		ktriov = malloc(iovlen, M_TEMP, M_WAITOK);
		memcpy((void *)ktriov, (void *)auio.uio_iov, iovlen);
	}

	cnt = auio.uio_resid;
	error = (*fp->f_ops->fo_read)(fp, offset, &auio, fp->f_cred, flags);
	if (error)
		if (auio.uio_resid != cnt && (error == ERESTART ||
		    error == EINTR || error == EWOULDBLOCK))
			error = 0;
	cnt -= auio.uio_resid;

	if (ktriov != NULL) {
		ktrgeniov(fd, UIO_READ, ktriov, cnt, error);
		free(ktriov, M_TEMP);
	}

	*retval = cnt;
done:
	if (needfree)
		free(needfree, M_IOV);
out:
	FILE_UNUSE(fp, l);
	return (error);
}

int
netbsd32_writev(struct lwp *l, const struct netbsd32_writev_args *uap, register_t *retval)
{
	/* {
		syscallarg(int) fd;
		syscallarg(const netbsd32_iovecp_t) iovp;
		syscallarg(int) iovcnt;
	} */
	int fd = SCARG(uap, fd);
	struct file *fp;
	struct proc *p = l->l_proc;
	struct filedesc *fdp = p->p_fd;

	if ((fp = fd_getfile(fdp, fd)) == NULL)
		return (EBADF);

	if ((fp->f_flag & FWRITE) == 0) {
		FILE_UNLOCK(fp);
		return (EBADF);
	}

	FILE_USE(fp);

	return (dofilewritev32(l, fd, fp,
	    (struct netbsd32_iovec *)SCARG_P32(uap, iovp),
	    SCARG(uap, iovcnt), &fp->f_offset, FOF_UPDATE_OFFSET, retval));
}

int
dofilewritev32(struct lwp *l, int fd, struct file *fp, struct netbsd32_iovec *iovp, int iovcnt, off_t *offset, int flags, register_t *retval)
{
	struct uio auio;
	struct iovec *iov;
	struct iovec *needfree;
	struct iovec aiov[UIO_SMALLIOV];
	struct proc *p = l->l_proc;
	long i, cnt, error = 0;
	u_int iovlen;
	struct iovec *ktriov = NULL;

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
	auio.uio_vmspace = l->l_proc->p_vmspace;
	error = netbsd32_to_iovecin(iovp, iov, iovcnt);
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

	/*
	 * if tracing, save a copy of iovec
	 */
	if (ktrpoint(KTR_GENIO))  {
		ktriov = malloc(iovlen, M_TEMP, M_WAITOK);
		memcpy((void *)ktriov, (void *)auio.uio_iov, iovlen);
	}

	cnt = auio.uio_resid;
	error = (*fp->f_ops->fo_write)(fp, offset, &auio, fp->f_cred, flags);
	if (error) {
		if (auio.uio_resid != cnt && (error == ERESTART ||
		    error == EINTR || error == EWOULDBLOCK))
			error = 0;
		if (error == EPIPE) {
			mutex_enter(&proclist_mutex);
			psignal(p, SIGPIPE);
			mutex_exit(&proclist_mutex);
		}
	}
	cnt -= auio.uio_resid;
	if (ktriov != NULL) {
		ktrgenio(fd, UIO_WRITE, ktriov, cnt, error);
		free(ktriov, M_TEMP);
	}
	*retval = cnt;
done:
	if (needfree)
		free(needfree, M_IOV);
out:
	FILE_UNUSE(fp, l);
	return (error);
}

/*
 * Common routine to set access and modification times given a vnode.
 */
static int
get_utimes32(const netbsd32_timevalp_t *tptr, struct timeval *tv,
    struct timeval **tvp)
{
	int error;
	struct netbsd32_timeval tv32[2];

	if (tptr == NULL) {
		*tvp = NULL;
		return 0;
	}

	error = copyin(tptr, tv32, sizeof(tv32));
	if (error)
		return error;
	netbsd32_to_timeval(&tv32[0], &tv[0]);
	netbsd32_to_timeval(&tv32[1], &tv[1]);

	*tvp = tv;
	return 0;
}

int
netbsd32_utimes(struct lwp *l, const struct netbsd32_utimes_args *uap, register_t *retval)
{
	/* {
		syscallarg(const netbsd32_charp) path;
		syscallarg(const netbsd32_timevalp_t) tptr;
	} */
	int error;
	struct timeval tv[2], *tvp;

	error = get_utimes32(SCARG_P32(uap, tptr), tv, &tvp);
	if (error != 0)
		return error;

	return do_sys_utimes(l, NULL, SCARG_P32(uap, path), FOLLOW,
			    tvp, UIO_SYSSPACE);
}

static int
netbds32_copyout_statvfs(const void *kp, void *up, size_t len)
{
	struct netbsd32_statvfs *sbuf_32;
	int error;

	sbuf_32 = malloc(sizeof *sbuf_32, M_TEMP, M_WAITOK);
	netbsd32_from_statvfs(kp, sbuf_32);
	error = copyout(sbuf_32, up, sizeof(*sbuf_32));
	free(sbuf_32, M_TEMP);

	return error;
}

int
netbsd32_statvfs1(struct lwp *l, const struct netbsd32_statvfs1_args *uap, register_t *retval)
{
	/* {
		syscallarg(const netbsd32_charp) path;
		syscallarg(netbsd32_statvfsp_t) buf;
		syscallarg(int) flags;
	} */
	struct statvfs *sb;
	int error;

	sb = STATVFSBUF_GET();
	error = do_sys_pstatvfs(l, SCARG_P32(uap, path), SCARG(uap, flags), sb);
	if (error == 0)
		error = netbds32_copyout_statvfs(sb, SCARG_P32(uap, buf), 0);
	STATVFSBUF_PUT(sb);
	return error;
}

int
netbsd32_fstatvfs1(struct lwp *l, const struct netbsd32_fstatvfs1_args *uap, register_t *retval)
{
	/* {
		syscallarg(int) fd;
		syscallarg(netbsd32_statvfsp_t) buf;
		syscallarg(int) flags;
	} */
	struct statvfs *sb;
	int error;

	sb = STATVFSBUF_GET();
	error = do_sys_fstatvfs(l, SCARG(uap, fd), SCARG(uap, flags), sb);
	if (error == 0)
		error = netbds32_copyout_statvfs(sb, SCARG_P32(uap, buf), 0);
	STATVFSBUF_PUT(sb);
	return error;
}

int
netbsd32_getvfsstat(struct lwp *l, const struct netbsd32_getvfsstat_args *uap, register_t *retval)
{
	/* {
		syscallarg(netbsd32_statvfsp_t) buf;
		syscallarg(netbsd32_size_t) bufsize;
		syscallarg(int) flags;
	} */

	return do_sys_getvfsstat(l, SCARG_P32(uap, buf), SCARG(uap, bufsize),
	    SCARG(uap, flags), netbds32_copyout_statvfs,
	    sizeof (struct netbsd32_statvfs), retval);
}

int
netbsd32___fhstatvfs140(struct lwp *l, const struct netbsd32___fhstatvfs140_args *uap, register_t *retval)
{
	/* {
		syscallarg(const netbsd32_pointer_t) fhp;
		syscallarg(netbsd32_size_t) fh_size;
		syscallarg(netbsd32_statvfsp_t) buf;
		syscallarg(int) flags;
	} */
	struct statvfs *sb;
	int error;

	sb = STATVFSBUF_GET();
	error = do_fhstatvfs(l, SCARG_P32(uap, fhp), SCARG(uap, fh_size), sb,
	    SCARG(uap, flags));

	if (error == 0)
		error = netbds32_copyout_statvfs(sb, SCARG_P32(uap, buf), 0);
	STATVFSBUF_PUT(sb);

	return error;
}

int
netbsd32_futimes(struct lwp *l, const struct netbsd32_futimes_args *uap, register_t *retval)
{
	/* {
		syscallarg(int) fd;
		syscallarg(const netbsd32_timevalp_t) tptr;
	} */
	int error;
	struct file *fp;
	struct timeval tv[2], *tvp;

	error = get_utimes32(SCARG_P32(uap, tptr), tv, &tvp);
	if (error != 0)
		return error;

	/* getvnode() will use the descriptor for us */
	if ((error = getvnode(l->l_proc->p_fd, SCARG(uap, fd), &fp)) != 0)
		return (error);

	error = do_sys_utimes(l, fp->f_data, NULL, 0, tvp, UIO_SYSSPACE);

	FILE_UNUSE(fp, l);
	return (error);
}

int
netbsd32_sys___getdents30(struct lwp *l, const struct netbsd32_sys___getdents30_args *uap, register_t *retval)
{
	/* {
		syscallarg(int) fd;
		syscallarg(netbsd32_charp) buf;
		syscallarg(netbsd32_size_t) count;
	} */
	struct file *fp;
	int error, done;
	struct proc *p = l->l_proc;

	/* getvnode() will use the descriptor for us */
	if ((error = getvnode(p->p_fd, SCARG(uap, fd), &fp)) != 0)
		return (error);
	if ((fp->f_flag & FREAD) == 0) {
		error = EBADF;
		goto out;
	}
	error = vn_readdir(fp, SCARG_P32(uap, buf),
	    UIO_USERSPACE, SCARG(uap, count), &done, l, 0, 0);
	*retval = done;
 out:
	FILE_UNUSE(fp, l);
	return (error);
}

int
netbsd32_lutimes(struct lwp *l, const struct netbsd32_lutimes_args *uap, register_t *retval)
{
	/* {
		syscallarg(const netbsd32_charp) path;
		syscallarg(const netbsd32_timevalp_t) tptr;
	} */
	int error;
	struct timeval tv[2], *tvp;

	error = get_utimes32(SCARG_P32(uap, tptr), tv, &tvp);
	if (error != 0)
		return error;

	return do_sys_utimes(l, NULL, SCARG_P32(uap, path), NOFOLLOW,
			    tvp, UIO_SYSSPACE);
}

int
netbsd32_sys___stat30(struct lwp *l, const struct netbsd32_sys___stat30_args *uap, register_t *retval)
{
	/* {
		syscallarg(const netbsd32_charp) path;
		syscallarg(netbsd32_statp_t) ub;
	} */
	struct netbsd32_stat sb32;
	struct stat sb;
	int error;
	const char *path;

	path = SCARG_P32(uap, path);

	error = do_sys_stat(l, path, FOLLOW, &sb);
	if (error)
		return (error);
	netbsd32_from___stat30(&sb, &sb32);
	error = copyout(&sb32, SCARG_P32(uap, ub), sizeof(sb32));
	return (error);
}

int
netbsd32_sys___fstat30(struct lwp *l, const struct netbsd32_sys___fstat30_args *uap, register_t *retval)
{
	/* {
		syscallarg(int) fd;
		syscallarg(netbsd32_statp_t) sb;
	} */
	int fd = SCARG(uap, fd);
	struct proc *p = l->l_proc;
	struct filedesc *fdp = p->p_fd;
	struct file *fp;
	struct netbsd32_stat sb32;
	struct stat ub;
	int error = 0;

	if ((fp = fd_getfile(fdp, fd)) == NULL)
		return (EBADF);

	FILE_USE(fp);
	error = (*fp->f_ops->fo_stat)(fp, &ub, l);
	FILE_UNUSE(fp, l);

	if (error == 0) {
		netbsd32_from___stat30(&ub, &sb32);
		error = copyout(&sb32, SCARG_P32(uap, sb), sizeof(sb32));
	}
	return (error);
}

int
netbsd32_sys___lstat30(struct lwp *l, const struct netbsd32_sys___lstat30_args *uap, register_t *retval)
{
	/* {
		syscallarg(const netbsd32_charp) path;
		syscallarg(netbsd32_statp_t) ub;
	} */
	struct netbsd32_stat sb32;
	struct stat sb;
	int error;
	const char *path;

	path = SCARG_P32(uap, path);

	error = do_sys_stat(l, path, NOFOLLOW, &sb);
	if (error)
		return (error);
	netbsd32_from___stat30(&sb, &sb32);
	error = copyout(&sb32, SCARG_P32(uap, ub), sizeof(sb32));
	return (error);
}

int
netbsd32___fhstat40(struct lwp *l, const struct netbsd32___fhstat40_args *uap, register_t *retval)
{
	/* {
		syscallarg(const netbsd32_pointer_t) fhp;
		syscallarg(netbsd32_size_t) fh_size;
		syscallarg(netbsd32_statp_t) sb;
	} */
	struct stat sb;
	struct netbsd32_stat sb32;
	int error;

	error = do_fhstat(l, SCARG_P32(uap, fhp), SCARG(uap, fh_size), &sb);
	if (error != 0) {
		netbsd32_from___stat30(&sb, &sb32);
		error = copyout(&sb32, SCARG_P32(uap, sb), sizeof(sb));
	}
	return error;
}

int
netbsd32_preadv(struct lwp *l, const struct netbsd32_preadv_args *uap, register_t *retval)
{
	/* {
		syscallarg(int) fd;
		syscallarg(const netbsd32_iovecp_t) iovp;
		syscallarg(int) iovcnt;
		syscallarg(int) pad;
		syscallarg(off_t) offset;
	} */
	struct proc *p = l->l_proc;
	struct filedesc *fdp = p->p_fd;
	struct file *fp;
	struct vnode *vp;
	off_t offset;
	int error, fd = SCARG(uap, fd);

	if ((fp = fd_getfile(fdp, fd)) == NULL)
		return (EBADF);

	if ((fp->f_flag & FREAD) == 0) {
		FILE_UNLOCK(fp);
		return (EBADF);
	}

	FILE_USE(fp);

	vp = (struct vnode *)fp->f_data;
	if (fp->f_type != DTYPE_VNODE || vp->v_type == VFIFO) {
		error = ESPIPE;
		goto out;
	}

	offset = SCARG(uap, offset);

	/*
	 * XXX This works because no file systems actually
	 * XXX take any action on the seek operation.
	 */
	if ((error = VOP_SEEK(vp, fp->f_offset, offset, fp->f_cred)) != 0)
		goto out;

	return (dofilereadv32(l, fd, fp, SCARG_P32(uap, iovp),
	    SCARG(uap, iovcnt), &offset, 0, retval));

out:
	FILE_UNUSE(fp, l);
	return (error);
}

int
netbsd32_pwritev(struct lwp *l, const struct netbsd32_pwritev_args *uap, register_t *retval)
{
	/* {
		syscallarg(int) fd;
		syscallarg(const netbsd32_iovecp_t) iovp;
		syscallarg(int) iovcnt;
		syscallarg(int) pad;
		syscallarg(off_t) offset;
	} */
	struct proc *p = l->l_proc;
	struct filedesc *fdp = p->p_fd;
	struct file *fp;
	struct vnode *vp;
	off_t offset;
	int error, fd = SCARG(uap, fd);

	if ((fp = fd_getfile(fdp, fd)) == NULL)
		return (EBADF);

	if ((fp->f_flag & FWRITE) == 0) {
		FILE_UNLOCK(fp);
		return (EBADF);
	}

	FILE_USE(fp);

	vp = (struct vnode *)fp->f_data;
	if (fp->f_type != DTYPE_VNODE || vp->v_type == VFIFO) {
		error = ESPIPE;
		goto out;
	}

	offset = SCARG(uap, offset);

	/*
	 * XXX This works because no file systems actually
	 * XXX take any action on the seek operation.
	 */
	if ((error = VOP_SEEK(vp, fp->f_offset, offset, fp->f_cred)) != 0)
		goto out;

	return (dofilewritev32(l, fd, fp, SCARG_P32(uap, iovp),
	    SCARG(uap, iovcnt), &offset, 0, retval));

out:
	FILE_UNUSE(fp, l);
	return (error);
}

/*
 * Find pathname of process's current directory.
 *
 * Use vfs vnode-to-name reverse cache; if that fails, fall back
 * to reading directory contents.
 */
/* XXX NH Why does this exist */
int
getcwd_common(struct vnode *, struct vnode *,
		   char **, char *, int, int, struct lwp *);

int
netbsd32___getcwd(struct lwp *l, const struct netbsd32___getcwd_args *uap, register_t *retval)
{
	/* {
		syscallarg(char *) bufp;
		syscallarg(size_t) length;
	} */
	struct proc *p = l->l_proc;
	int     error;
	char   *path;
	char   *bp, *bend;
	int     len = (int)SCARG(uap, length);
	int	lenused;

	if (len > MAXPATHLEN*4)
		len = MAXPATHLEN*4;
	else if (len < 2)
		return ERANGE;

	path = (char *)malloc(len, M_TEMP, M_WAITOK);
	if (!path)
		return ENOMEM;

	bp = &path[len];
	bend = bp;
	*(--bp) = '\0';

	/*
	 * 5th argument here is "max number of vnodes to traverse".
	 * Since each entry takes up at least 2 bytes in the output buffer,
	 * limit it to N/2 vnodes for an N byte buffer.
	 */
#define GETCWD_CHECK_ACCESS 0x0001
	error = getcwd_common (p->p_cwdi->cwdi_cdir, NULL, &bp, path, len/2,
			       GETCWD_CHECK_ACCESS, l);

	if (error)
		goto out;
	lenused = bend - bp;
	*retval = lenused;
	/* put the result into user buffer */
	error = copyout(bp, SCARG_P32(uap, bufp), lenused);

out:
	free(path, M_TEMP);
	return error;
}
