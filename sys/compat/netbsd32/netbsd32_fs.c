/*	$NetBSD: netbsd32_fs.c,v 1.63.2.3 2017/12/03 11:36:56 jdolecek Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: netbsd32_fs.c,v 1.63.2.3 2017/12/03 11:36:56 jdolecek Exp $");

#include <sys/param.h>
#include <sys/systm.h>
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

#include <fs/cd9660/cd9660_mount.h>
#include <fs/tmpfs/tmpfs_args.h>
#include <fs/msdosfs/bpb.h>
#include <fs/msdosfs/msdosfsmount.h>
#include <ufs/ufs/ufsmount.h>

#define NFS_ARGS_ONLY
#include <nfs/nfsmount.h>

#include <compat/netbsd32/netbsd32.h>
#include <compat/netbsd32/netbsd32_syscallargs.h>
#include <compat/netbsd32/netbsd32_conv.h>
#include <compat/sys/mount.h>


static int dofilereadv32(int, struct file *, struct netbsd32_iovec *,
			      int, off_t *, int, register_t *);
static int dofilewritev32(int, struct file *, struct netbsd32_iovec *,
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
		iov = kmem_alloc(iovlen * sizeof(*iov), KM_SLEEP);

	iovp = iov;
	for (i = 0; i < iovlen; iov32 += N_IOV32, i += N_IOV32) {
		n = iovlen - i;
		if (n > N_IOV32)
			n = N_IOV32;
		error = copyin(iov32, aiov32, n * sizeof (*iov32));
		if (error != 0) {
			if (iov != aiov)
				kmem_free(iov, iovlen * sizeof(*iov));
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
	file_t *fp;

	if ((fp = fd_getfile(fd)) == NULL)
		return (EBADF);

	if ((fp->f_flag & FREAD) == 0) {
		fd_putfile(fd);
		return (EBADF);
	}

	return (dofilereadv32(fd, fp,
	    (struct netbsd32_iovec *)SCARG_P32(uap, iovp),
	    SCARG(uap, iovcnt), &fp->f_offset, FOF_UPDATE_OFFSET, retval));
}

/* Damn thing copies in the iovec! */
int
dofilereadv32(int fd, struct file *fp, struct netbsd32_iovec *iovp, int iovcnt, off_t *offset, int flags, register_t *retval)
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
		iov = kmem_alloc(iovlen, KM_SLEEP);
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
	auio.uio_vmspace = curproc->p_vmspace;
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
		ktriov = kmem_alloc(iovlen, KM_SLEEP);
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
		kmem_free(ktriov, iovlen);
	}

	*retval = cnt;
done:
	if (needfree)
		kmem_free(needfree, iovlen);
out:
	fd_putfile(fd);
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
	file_t *fp;

	if ((fp = fd_getfile(fd)) == NULL)
		return (EBADF);

	if ((fp->f_flag & FWRITE) == 0) {
		fd_putfile(fd);
		return (EBADF);
	}

	return (dofilewritev32(fd, fp,
	    (struct netbsd32_iovec *)SCARG_P32(uap, iovp),
	    SCARG(uap, iovcnt), &fp->f_offset, FOF_UPDATE_OFFSET, retval));
}

int
dofilewritev32(int fd, struct file *fp, struct netbsd32_iovec *iovp, int iovcnt, off_t *offset, int flags, register_t *retval)
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
		iov = kmem_alloc(iovlen, KM_SLEEP);
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
	auio.uio_vmspace = curproc->p_vmspace;
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
		ktriov = kmem_alloc(iovlen, KM_SLEEP);
		memcpy((void *)ktriov, (void *)auio.uio_iov, iovlen);
	}

	cnt = auio.uio_resid;
	error = (*fp->f_ops->fo_write)(fp, offset, &auio, fp->f_cred, flags);
	if (error) {
		if (auio.uio_resid != cnt && (error == ERESTART ||
		    error == EINTR || error == EWOULDBLOCK))
			error = 0;
		if (error == EPIPE && (fp->f_flag & FNOSIGPIPE) == 0) {
			mutex_enter(proc_lock);
			psignal(curproc, SIGPIPE);
			mutex_exit(proc_lock);
		}
	}
	cnt -= auio.uio_resid;
	if (ktriov != NULL) {
		ktrgeniov(fd, UIO_WRITE, ktriov, cnt, error);
		kmem_free(ktriov, iovlen);
	}
	*retval = cnt;
done:
	if (needfree)
		kmem_free(needfree, iovlen);
out:
	fd_putfile(fd);
	return (error);
}

/*
 * Common routines to set access and modification times given a vnode.
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

static int
get_utimens32(const netbsd32_timespecp_t *tptr, struct timespec *ts,
    struct timespec **tsp)
{
	int error;
	struct netbsd32_timespec ts32[2];

	if (tptr == NULL) {
		*tsp = NULL;
		return 0;
	}

	error = copyin(tptr, ts32, sizeof(ts32));
	if (error)
		return error;
	netbsd32_to_timespec(&ts32[0], &ts[0]);
	netbsd32_to_timespec(&ts32[1], &ts[1]);

	*tsp = ts;
	return 0;
}

int
netbsd32___utimes50(struct lwp *l, const struct netbsd32___utimes50_args *uap, register_t *retval)
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
netbsd32_copyout_statvfs(const void *kp, void *up, size_t len)
{
	struct netbsd32_statvfs *sbuf_32;
	int error;

	sbuf_32 = kmem_alloc(sizeof(*sbuf_32), KM_SLEEP);
	netbsd32_from_statvfs(kp, sbuf_32);
	error = copyout(sbuf_32, up, sizeof(*sbuf_32));
	kmem_free(sbuf_32, sizeof(*sbuf_32));

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
		error = netbsd32_copyout_statvfs(sb, SCARG_P32(uap, buf), 0);
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
		error = netbsd32_copyout_statvfs(sb, SCARG_P32(uap, buf), 0);
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
	    SCARG(uap, flags), netbsd32_copyout_statvfs,
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
		error = netbsd32_copyout_statvfs(sb, SCARG_P32(uap, buf), 0);
	STATVFSBUF_PUT(sb);

	return error;
}

int
netbsd32___futimes50(struct lwp *l, const struct netbsd32___futimes50_args *uap, register_t *retval)
{
	/* {
		syscallarg(int) fd;
		syscallarg(const netbsd32_timevalp_t) tptr;
	} */
	int error;
	file_t *fp;
	struct timeval tv[2], *tvp;

	error = get_utimes32(SCARG_P32(uap, tptr), tv, &tvp);
	if (error != 0)
		return error;

	/* fd_getvnode() will use the descriptor for us */
	if ((error = fd_getvnode(SCARG(uap, fd), &fp)) != 0)
		return (error);

	error = do_sys_utimes(l, fp->f_vnode, NULL, 0, tvp, UIO_SYSSPACE);

	fd_putfile(SCARG(uap, fd));
	return (error);
}

int
netbsd32___getdents30(struct lwp *l,
    const struct netbsd32___getdents30_args *uap, register_t *retval)
{
	/* {
		syscallarg(int) fd;
		syscallarg(netbsd32_charp) buf;
		syscallarg(netbsd32_size_t) count;
	} */
	file_t *fp;
	int error, done;

	/* fd_getvnode() will use the descriptor for us */
	if ((error = fd_getvnode(SCARG(uap, fd), &fp)) != 0)
		return (error);
	if ((fp->f_flag & FREAD) == 0) {
		error = EBADF;
		goto out;
	}
	error = vn_readdir(fp, SCARG_P32(uap, buf),
	    UIO_USERSPACE, SCARG(uap, count), &done, l, 0, 0);
	ktrgenio(SCARG(uap, fd), UIO_READ, SCARG_P32(uap, buf), done, error);
	*retval = done;
 out:
	fd_putfile(SCARG(uap, fd));
	return (error);
}

int
netbsd32___lutimes50(struct lwp *l,
    const struct netbsd32___lutimes50_args *uap, register_t *retval)
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
netbsd32___stat50(struct lwp *l, const struct netbsd32___stat50_args *uap, register_t *retval)
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

	error = do_sys_stat(path, FOLLOW, &sb);
	if (error)
		return (error);
	netbsd32_from_stat(&sb, &sb32);
	error = copyout(&sb32, SCARG_P32(uap, ub), sizeof(sb32));
	return (error);
}

int
netbsd32___fstat50(struct lwp *l, const struct netbsd32___fstat50_args *uap, register_t *retval)
{
	/* {
		syscallarg(int) fd;
		syscallarg(netbsd32_statp_t) sb;
	} */
	struct netbsd32_stat sb32;
	struct stat ub;
	int error;

	error = do_sys_fstat(SCARG(uap, fd), &ub);
	if (error == 0) {
		netbsd32_from_stat(&ub, &sb32);
		error = copyout(&sb32, SCARG_P32(uap, sb), sizeof(sb32));
	}
	return (error);
}

int
netbsd32___lstat50(struct lwp *l, const struct netbsd32___lstat50_args *uap, register_t *retval)
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

	error = do_sys_stat(path, NOFOLLOW, &sb);
	if (error)
		return (error);
	netbsd32_from_stat(&sb, &sb32);
	error = copyout(&sb32, SCARG_P32(uap, ub), sizeof(sb32));
	return (error);
}

int
netbsd32___fhstat50(struct lwp *l, const struct netbsd32___fhstat50_args *uap, register_t *retval)
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
	if (error == 0) {
		netbsd32_from_stat(&sb, &sb32);
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
		syscallarg(netbsd32_off_t) offset;
	} */
	file_t *fp;
	struct vnode *vp;
	off_t offset;
	int error, fd = SCARG(uap, fd);

	if ((fp = fd_getfile(fd)) == NULL)
		return (EBADF);

	if ((fp->f_flag & FREAD) == 0) {
		fd_putfile(fd);
		return (EBADF);
	}

	vp = fp->f_vnode;
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

	return (dofilereadv32(fd, fp, SCARG_P32(uap, iovp),
	    SCARG(uap, iovcnt), &offset, 0, retval));

out:
	fd_putfile(fd);
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
		syscallarg(netbsd32_off_t) offset;
	} */
	file_t *fp;
	struct vnode *vp;
	off_t offset;
	int error, fd = SCARG(uap, fd);

	if ((fp = fd_getfile(fd)) == NULL)
		return (EBADF);

	if ((fp->f_flag & FWRITE) == 0) {
		fd_putfile(fd);
		return (EBADF);
	}

	vp = fp->f_vnode;
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

	return (dofilewritev32(fd, fp, SCARG_P32(uap, iovp),
	    SCARG(uap, iovcnt), &offset, 0, retval));

out:
	fd_putfile(fd);
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
	struct	cwdinfo *cwdi;

	if (len > MAXPATHLEN*4)
		len = MAXPATHLEN*4;
	else if (len < 2)
		return ERANGE;

	path = kmem_alloc(len, KM_SLEEP);
	bp = &path[len];
	bend = bp;
	*(--bp) = '\0';

	/*
	 * 5th argument here is "max number of vnodes to traverse".
	 * Since each entry takes up at least 2 bytes in the output buffer,
	 * limit it to N/2 vnodes for an N byte buffer.
	 */
#define GETCWD_CHECK_ACCESS 0x0001
	cwdi = p->p_cwdi;
	rw_enter(&cwdi->cwdi_lock, RW_READER);
	error = getcwd_common (cwdi->cwdi_cdir, NULL, &bp, path, len/2,
			       GETCWD_CHECK_ACCESS, l);
	rw_exit(&cwdi->cwdi_lock);

	if (error)
		goto out;
	lenused = bend - bp;
	*retval = lenused;
	/* put the result into user buffer */
	error = copyout(bp, SCARG_P32(uap, bufp), lenused);

out:
	kmem_free(path, len);
	return error;
}

int
netbsd32___mount50(struct lwp *l, const struct netbsd32___mount50_args *uap,
	register_t *retval)
{
	/* {
		syscallarg(netbsd32_charp) type;
		syscallarg(netbsd32_charp) path;
		syscallarg(int) flags;
		syscallarg(netbsd32_voidp) data;
		syscallarg(netbsd32_size_t) data_len;
	} */
	char mtype[MNAMELEN];
	union {
		struct netbsd32_ufs_args ufs_args;
		struct netbsd32_mfs_args mfs_args;
		struct netbsd32_iso_args iso_args;
		struct netbsd32_nfs_args nfs_args;
		struct netbsd32_msdosfs_args msdosfs_args;
		struct netbsd32_tmpfs_args tmpfs_args;
	} fs_args32;
	union {
		struct ufs_args ufs_args;
		struct mfs_args mfs_args;
		struct iso_args iso_args;
		struct nfs_args nfs_args;
		struct msdosfs_args msdosfs_args;
		struct tmpfs_args tmpfs_args;
	} fs_args;
	const char *type = SCARG_P32(uap, type);
	const char *path = SCARG_P32(uap, path);
	int flags = SCARG(uap, flags);
	void *data = SCARG_P32(uap, data);
	size_t data_len = SCARG(uap, data_len);
	enum uio_seg data_seg;
	size_t len;
	int error;
 
	error = copyinstr(type, mtype, sizeof(mtype), &len);
	if (error)
		return error;
	if (strcmp(mtype, MOUNT_TMPFS) == 0) {
		if (data_len != sizeof(fs_args32.tmpfs_args))
			return EINVAL;
		if ((flags & MNT_GETARGS) == 0) {
			error = copyin(data, &fs_args32.tmpfs_args, 
			    sizeof(fs_args32.tmpfs_args));
			if (error)
				return error;
			fs_args.tmpfs_args.ta_version =
			    fs_args32.tmpfs_args.ta_version;
			fs_args.tmpfs_args.ta_nodes_max =
			    fs_args32.tmpfs_args.ta_nodes_max;
			fs_args.tmpfs_args.ta_size_max =
			    fs_args32.tmpfs_args.ta_size_max;
			fs_args.tmpfs_args.ta_root_uid =
			    fs_args32.tmpfs_args.ta_root_uid;
			fs_args.tmpfs_args.ta_root_gid =
			    fs_args32.tmpfs_args.ta_root_gid;
			fs_args.tmpfs_args.ta_root_mode =
			    fs_args32.tmpfs_args.ta_root_mode;
		}
		data_seg = UIO_SYSSPACE;
		data = &fs_args.tmpfs_args;
		data_len = sizeof(fs_args.tmpfs_args);
	} else if (strcmp(mtype, MOUNT_MFS) == 0) {
		if (data_len != sizeof(fs_args32.mfs_args))
			return EINVAL;
		if ((flags & MNT_GETARGS) == 0) {
			error = copyin(data, &fs_args32.mfs_args, 
			    sizeof(fs_args32.mfs_args));
			if (error)
				return error;
			fs_args.mfs_args.fspec =
			    NETBSD32PTR64(fs_args32.mfs_args.fspec);
			memset(&fs_args.mfs_args._pad1, 0,
			    sizeof(fs_args.mfs_args._pad1));
			fs_args.mfs_args.base =
			    NETBSD32PTR64(fs_args32.mfs_args.base);
			fs_args.mfs_args.size = fs_args32.mfs_args.size;
		}
		data_seg = UIO_SYSSPACE;
		data = &fs_args.mfs_args;
		data_len = sizeof(fs_args.mfs_args);
	} else if ((strcmp(mtype, MOUNT_UFS) == 0) ||
		   (strcmp(mtype, MOUNT_EXT2FS) == 0) ||
		   (strcmp(mtype, MOUNT_LFS) == 0)) {
		if (data_len > sizeof(fs_args32.ufs_args))
			return EINVAL;
		if ((flags & MNT_GETARGS) == 0) {
			error = copyin(data, &fs_args32.ufs_args, 
			    sizeof(fs_args32.ufs_args));
			if (error)
				return error;
			fs_args.ufs_args.fspec =
			    NETBSD32PTR64(fs_args32.ufs_args.fspec);
		}
		data_seg = UIO_SYSSPACE;
		data = &fs_args.ufs_args;
		data_len = sizeof(fs_args.ufs_args);
	} else if (strcmp(mtype, MOUNT_CD9660) == 0) {
		if (data_len != sizeof(fs_args32.iso_args))
			return EINVAL;
		if ((flags & MNT_GETARGS) == 0) {
			error = copyin(data, &fs_args32.iso_args, 
			    sizeof(fs_args32.iso_args));
			if (error)
				return error;
			fs_args.iso_args.fspec =
			    NETBSD32PTR64(fs_args32.iso_args.fspec);
			memset(&fs_args.iso_args._pad1, 0,
			    sizeof(fs_args.iso_args._pad1));
			fs_args.iso_args.flags = fs_args32.iso_args.flags;
		}
		data_seg = UIO_SYSSPACE;
		data = &fs_args.iso_args;
		data_len = sizeof(fs_args.iso_args);
	} else if (strcmp(mtype, MOUNT_MSDOS) == 0) {
		if (data_len != sizeof(fs_args32.msdosfs_args))
			return EINVAL;
		if ((flags & MNT_GETARGS) == 0) {
			error = copyin(data, &fs_args32.msdosfs_args, 
			    sizeof(fs_args32.msdosfs_args));
			if (error)
				return error;
			fs_args.msdosfs_args.fspec =
			    NETBSD32PTR64(fs_args32.msdosfs_args.fspec);
			memset(&fs_args.msdosfs_args._pad1, 0,
			    sizeof(fs_args.msdosfs_args._pad1));
			fs_args.msdosfs_args.uid =
			    fs_args32.msdosfs_args.uid;
			fs_args.msdosfs_args.gid =
			    fs_args32.msdosfs_args.gid;
			fs_args.msdosfs_args.mask =
			    fs_args32.msdosfs_args.mask;
			fs_args.msdosfs_args.flags =
			    fs_args32.msdosfs_args.flags;
			fs_args.msdosfs_args.version =
			    fs_args32.msdosfs_args.version;
			fs_args.msdosfs_args.dirmask =
			    fs_args32.msdosfs_args.dirmask;
			fs_args.msdosfs_args.gmtoff =
			    fs_args32.msdosfs_args.gmtoff;
		}
		data_seg = UIO_SYSSPACE;
		data = &fs_args.msdosfs_args;
		data_len = sizeof(fs_args.msdosfs_args);
	} else if (strcmp(mtype, MOUNT_NFS) == 0) {
		if (data_len != sizeof(fs_args32.nfs_args))
			return EINVAL;
		if ((flags & MNT_GETARGS) == 0) {
			error = copyin(data, &fs_args32.nfs_args, 
			    sizeof(fs_args32.nfs_args));
			if (error)
				return error;
			fs_args.nfs_args.version = fs_args32.nfs_args.version;
			fs_args.nfs_args.addr =
			    NETBSD32PTR64(fs_args32.nfs_args.addr);
			memcpy(&fs_args.nfs_args.addrlen,
			    &fs_args32.nfs_args.addrlen,
			    offsetof(struct nfs_args, fh)
				- offsetof(struct nfs_args, addrlen));
			fs_args.nfs_args.fh =
			    NETBSD32PTR64(fs_args32.nfs_args.fh);
			memcpy(&fs_args.nfs_args.fhsize,
			    &fs_args32.nfs_args.fhsize,
			    offsetof(struct nfs_args, hostname)
				- offsetof(struct nfs_args, fhsize));
			fs_args.nfs_args.hostname =
			    NETBSD32PTR64(fs_args32.nfs_args.hostname);
		}
		data_seg = UIO_SYSSPACE;
		data = &fs_args.nfs_args;
		data_len = sizeof(fs_args.nfs_args);
	} else {
		data_seg = UIO_USERSPACE;
	}
	error = do_sys_mount(l, mtype, UIO_SYSSPACE, path, flags, data, data_seg,
	    data_len, retval);
	if (error)
		return error;
	if (flags & MNT_GETARGS) {
		data_len = *retval;
		if (strcmp(mtype, MOUNT_TMPFS) == 0) {
			if (data_len != sizeof(fs_args.tmpfs_args))
				return EINVAL;
			fs_args32.tmpfs_args.ta_version =
			    fs_args.tmpfs_args.ta_version;
			fs_args32.tmpfs_args.ta_nodes_max =
			    fs_args.tmpfs_args.ta_nodes_max;
			fs_args32.tmpfs_args.ta_size_max =
			    fs_args.tmpfs_args.ta_size_max;
			fs_args32.tmpfs_args.ta_root_uid =
			    fs_args.tmpfs_args.ta_root_uid;
			fs_args32.tmpfs_args.ta_root_gid =
			    fs_args.tmpfs_args.ta_root_gid;
			fs_args32.tmpfs_args.ta_root_mode =
			    fs_args.tmpfs_args.ta_root_mode;
			error = copyout(&fs_args32.tmpfs_args, data,
				    sizeof(fs_args32.tmpfs_args));
		} else if (strcmp(mtype, MOUNT_MFS) == 0) {
			if (data_len != sizeof(fs_args.mfs_args))
				return EINVAL;
			NETBSD32PTR32(fs_args32.mfs_args.fspec,
			    fs_args.mfs_args.fspec);
			memset(&fs_args32.mfs_args._pad1, 0,
			    sizeof(fs_args32.mfs_args._pad1));
			NETBSD32PTR32(fs_args32.mfs_args.base,
			    fs_args.mfs_args.base);
			fs_args32.mfs_args.size = fs_args.mfs_args.size;
			error = copyout(&fs_args32.mfs_args, data,
				    sizeof(fs_args32.mfs_args));
		} else if (strcmp(mtype, MOUNT_UFS) == 0) {
			if (data_len != sizeof(fs_args.ufs_args))
				return EINVAL;
			NETBSD32PTR32(fs_args32.ufs_args.fspec,
			    fs_args.ufs_args.fspec);
			error = copyout(&fs_args32.ufs_args, data, 
			    sizeof(fs_args32.ufs_args));
		} else if (strcmp(mtype, MOUNT_CD9660) == 0) {
			if (data_len != sizeof(fs_args.iso_args))
				return EINVAL;
			NETBSD32PTR32(fs_args32.iso_args.fspec,
			    fs_args.iso_args.fspec);
			memset(&fs_args32.iso_args._pad1, 0,
			    sizeof(fs_args32.iso_args._pad1));
			fs_args32.iso_args.flags = fs_args.iso_args.flags;
			error = copyout(&fs_args32.iso_args, data,
				    sizeof(fs_args32.iso_args));
		} else if (strcmp(mtype, MOUNT_NFS) == 0) {
			if (data_len != sizeof(fs_args.nfs_args))
				return EINVAL;
			error = copyin(data, &fs_args32.nfs_args, 
			    sizeof(fs_args32.nfs_args));
			if (error)
				return error;
			fs_args.nfs_args.version = fs_args32.nfs_args.version;
			NETBSD32PTR32(fs_args32.nfs_args.addr,
			    fs_args.nfs_args.addr);
			memcpy(&fs_args32.nfs_args.addrlen,
			    &fs_args.nfs_args.addrlen,
			    offsetof(struct nfs_args, fh)
				- offsetof(struct nfs_args, addrlen));
			NETBSD32PTR32(fs_args32.nfs_args.fh,
			    fs_args.nfs_args.fh);
			memcpy(&fs_args32.nfs_args.fhsize,
			    &fs_args.nfs_args.fhsize,
			    offsetof(struct nfs_args, hostname)
				- offsetof(struct nfs_args, fhsize));
			NETBSD32PTR32(fs_args32.nfs_args.hostname,
			    fs_args.nfs_args.hostname);
			error = copyout(&fs_args32.nfs_args, data,
			    sizeof(fs_args32.nfs_args));
		}
	}
	return error;
}

int
netbsd32_linkat(struct lwp *l, const struct netbsd32_linkat_args *uap,
		 register_t *retval)
{
	/* {
		syscallarg(int) fd1;
		syscallarg(const netbsd32_charp) name1;
		syscallarg(int) fd2;
		syscallarg(const netbsd32_charp) name2;
		syscallarg(int) flags;
	} */
	struct sys_linkat_args ua;

	NETBSD32TO64_UAP(fd1);
	NETBSD32TOP_UAP(name1, const char);
	NETBSD32TO64_UAP(fd2);
	NETBSD32TOP_UAP(name2, const char);
	NETBSD32TO64_UAP(flags);

	return sys_linkat(l, &ua, retval);
}

int
netbsd32_renameat(struct lwp *l, const struct netbsd32_renameat_args *uap,
		 register_t *retval)
{
	/* {
		syscallarg(int) fromfd;
		syscallarg(const netbsd32_charp) from;
		syscallarg(int) tofd;
		syscallarg(const netbsd32_charp) to;
	} */
	struct sys_renameat_args ua;

	NETBSD32TO64_UAP(fromfd);
	NETBSD32TOP_UAP(from, const char);
	NETBSD32TO64_UAP(tofd);
	NETBSD32TOP_UAP(to, const char);

	return sys_renameat(l, &ua, retval);
}

int
netbsd32_mkfifoat(struct lwp *l, const struct netbsd32_mkfifoat_args *uap,
		 register_t *retval)
{
	/* {
		syscallarg(int) fd;
		syscallarg(const netbsd32_charp) path;
		syscallarg(mode_t) mode;
	} */
	struct sys_mkfifoat_args ua;

	NETBSD32TO64_UAP(fd);
	NETBSD32TOP_UAP(path, const char);
	NETBSD32TO64_UAP(mode);

	return sys_mkfifoat(l, &ua, retval);
}

int
netbsd32_mknodat(struct lwp *l, const struct netbsd32_mknodat_args *uap,
		 register_t *retval)
{
	/* {
		syscallarg(int) fd;
		syscallarg(netbsd32_charp) path;
		syscallarg(mode_t) mode;
		syscallarg(int) pad;
		syscallarg(netbsd32_dev_t) dev;
	} */
	struct sys_mknodat_args ua;

	NETBSD32TO64_UAP(fd);
	NETBSD32TOP_UAP(path, const char);
	NETBSD32TO64_UAP(mode);
	NETBSD32TO64_UAP(PAD);
	NETBSD32TO64_UAP(dev);

	return sys_mknodat(l, &ua, retval);
}

int
netbsd32_mkdirat(struct lwp *l, const struct netbsd32_mkdirat_args *uap,
		 register_t *retval)
{
	/* {
		syscallarg(int) fd;
		syscallarg(netbsd32_charp) path;
		syscallarg(mode_t) mode;
	} */
	struct sys_mkdirat_args ua;

	NETBSD32TO64_UAP(fd);
	NETBSD32TOP_UAP(path, const char);
	NETBSD32TO64_UAP(mode);

	return sys_mkdirat(l, &ua, retval);
}

int
netbsd32_faccessat(struct lwp *l, const struct netbsd32_faccessat_args *uap,
		 register_t *retval)
{
	/* {
		syscallarg(int) fd;
		syscallarg(netbsd32_charp) path;
		syscallarg(int) amode;
		syscallarg(int) flag;
	} */
	struct sys_faccessat_args ua;

	NETBSD32TO64_UAP(fd);
	NETBSD32TOP_UAP(path, const char);
	NETBSD32TO64_UAP(amode);
	NETBSD32TO64_UAP(flag);

	return sys_faccessat(l, &ua, retval);
}

int
netbsd32_fchmodat(struct lwp *l, const struct netbsd32_fchmodat_args *uap,
		 register_t *retval)
{
	/* {
		syscallarg(int) fd;
		syscallarg(netbsd32_charp) path;
		syscallarg(mode_t) mode;
		syscallarg(int) flag;
	} */
	struct sys_fchmodat_args ua;

	NETBSD32TO64_UAP(fd);
	NETBSD32TOP_UAP(path, const char);
	NETBSD32TO64_UAP(mode);
	NETBSD32TO64_UAP(flag);

	return sys_fchmodat(l, &ua, retval);
}

int
netbsd32_fchownat(struct lwp *l, const struct netbsd32_fchownat_args *uap,
		 register_t *retval)
{
	/* {
		syscallarg(int) fd;
		syscallarg(netbsd32_charp) path;
		syscallarg(uid_t) owner;
		syscallarg(gid_t) group;
		syscallarg(int) flag;
	} */
	struct sys_fchownat_args ua;

	NETBSD32TO64_UAP(fd);
	NETBSD32TOP_UAP(path, const char);
	NETBSD32TO64_UAP(owner);
	NETBSD32TO64_UAP(group);
	NETBSD32TO64_UAP(flag);

	return sys_fchownat(l, &ua, retval);
}

int
netbsd32_fstatat(struct lwp *l, const struct netbsd32_fstatat_args *uap,
		 register_t *retval)
{
	/* {
		syscallarg(int) fd;
		syscallarg(netbsd32_charp) path;
		syscallarg(netbsd32_statp_t) buf;
		syscallarg(int) flag;
	} */
	struct netbsd32_stat sb32;
	struct stat sb;
	int follow;
	int error;

	follow = (SCARG(uap, flag) & AT_SYMLINK_NOFOLLOW) ? NOFOLLOW : FOLLOW;

	error = do_sys_statat(l, SCARG(uap, fd), SCARG_P32(uap, path),
	    follow, &sb);
	if (error)
		return error;
	netbsd32_from_stat(&sb, &sb32);
	return copyout(&sb32, SCARG_P32(uap, buf), sizeof(sb32));
}

int
netbsd32_utimensat(struct lwp *l, const struct netbsd32_utimensat_args *uap,
		 register_t *retval)
{
	/* {
		syscallarg(int) fd;
		syscallarg(netbsd32_charp) path;
		syscallarg(netbsd32_timespecp_t) tptr;
		syscallarg(int) flag;
	} */
	struct timespec ts[2], *tsp = NULL /* XXXgcc */;
	int follow;
	int error;

	error = get_utimens32(SCARG_P32(uap, tptr), ts, &tsp);
	if (error != 0)
		return error;

	follow = (SCARG(uap, flag) & AT_SYMLINK_NOFOLLOW) ? NOFOLLOW : FOLLOW;

	return do_sys_utimensat(l, SCARG(uap, fd), NULL, 
	    SCARG_P32(uap, path), follow, tsp, UIO_SYSSPACE);
}

int
netbsd32_openat(struct lwp *l, const struct netbsd32_openat_args *uap,
		 register_t *retval)
{
	/* {
		syscallarg(int) fd;
		syscallarg(netbsd32_charp) path;
		syscallarg(int) oflags;
		syscallarg(mode_t) mode;
	} */
	struct sys_openat_args ua;

	NETBSD32TO64_UAP(fd);
	NETBSD32TOP_UAP(path, const char);
	NETBSD32TO64_UAP(oflags);
	NETBSD32TO64_UAP(mode);

	return sys_openat(l, &ua, retval);
}

int
netbsd32_readlinkat(struct lwp *l, const struct netbsd32_readlinkat_args *uap,
		 register_t *retval)
{
	/* {
		syscallarg(int) fd;
		syscallarg(netbsd32_charp) path;
		syscallarg(netbsd32_charp) buf;
		syscallarg(netbsd32_size_t) bufsize;
	} */
	struct sys_readlinkat_args ua;

	NETBSD32TO64_UAP(fd);
	NETBSD32TOP_UAP(path, const char *);
	NETBSD32TOP_UAP(buf, char *);
	NETBSD32TOX_UAP(bufsize, size_t);

	return sys_readlinkat(l, &ua, retval);
}

int
netbsd32_symlinkat(struct lwp *l, const struct netbsd32_symlinkat_args *uap,
		 register_t *retval)
{
	/* {
		syscallarg(netbsd32_charp) path1;
		syscallarg(int) fd;
		syscallarg(netbsd32_charp) path2;
	} */
	struct sys_symlinkat_args ua;

	NETBSD32TOP_UAP(path1, const char *);
	NETBSD32TO64_UAP(fd);
	NETBSD32TOP_UAP(path2, const char *);

	return sys_symlinkat(l, &ua, retval);
}

int
netbsd32_unlinkat(struct lwp *l, const struct netbsd32_unlinkat_args *uap,
		 register_t *retval)
{
	/* {
		syscallarg(int) fd;
		syscallarg(netbsd32_charp) path;
		syscallarg(int) flag;
	} */
	struct sys_unlinkat_args ua;

	NETBSD32TO64_UAP(fd);
	NETBSD32TOP_UAP(path, const char *);
	NETBSD32TO64_UAP(flag);

	return sys_unlinkat(l, &ua, retval);
}

int
netbsd32_futimens(struct lwp *l, const struct netbsd32_futimens_args *uap,
		 register_t *retval)
{
	/* {
		syscallarg(int) fd;
		syscallarg(netbsd32_timespecp_t) tptr;
	} */
	struct timespec ts[2], *tsp = NULL /* XXXgcc */;
	file_t *fp;
	int error;

	error = get_utimens32(SCARG_P32(uap, tptr), ts, &tsp);
	if (error != 0)
		return error;

	/* fd_getvnode() will use the descriptor for us */
	if ((error = fd_getvnode(SCARG(uap, fd), &fp)) != 0)
		return (error);
	error = do_sys_utimensat(l, AT_FDCWD, fp->f_vnode, NULL, 0,
	    tsp, UIO_SYSSPACE);
	fd_putfile(SCARG(uap, fd));
	return (error);
}
