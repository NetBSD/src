/*	$NetBSD: netbsd32_fs.c,v 1.5 2001/05/30 11:37:28 mrg Exp $	*/

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

#if defined(_KERNEL_OPT)
#include "opt_ktrace.h"
#endif

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
#include <sys/syscallargs.h>
#include <sys/proc.h>

#include <compat/netbsd32/netbsd32.h>
#include <compat/netbsd32/netbsd32_syscallargs.h>
#include <compat/netbsd32/netbsd32_conv.h>


static int dofilereadv32 __P((struct proc *, int, struct file *, struct netbsd32_iovec *, 
			      int, off_t *, int, register_t *));
static int dofilewritev32 __P((struct proc *, int, struct file *, struct netbsd32_iovec *, 
			       int,  off_t *, int, register_t *));
static int change_utimes32 __P((struct vnode *, netbsd32_timevalp_t, struct proc *));

int
netbsd32_getfsstat(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct netbsd32_getfsstat_args /* {
		syscallarg(netbsd32_statfsp_t) buf;
		syscallarg(netbsd32_long) bufsize;
		syscallarg(int) flags;
	} */ *uap = v;
	struct mount *mp, *nmp;
	struct statfs *sp;
	struct netbsd32_statfs sb32;
	caddr_t sfsp;
	long count, maxcount, error;

	maxcount = SCARG(uap, bufsize) / sizeof(struct netbsd32_statfs);
	sfsp = (caddr_t)(u_long)SCARG(uap, buf);
	simple_lock(&mountlist_slock);
	count = 0;
	for (mp = mountlist.cqh_first; mp != (void *)&mountlist; mp = nmp) {
		if (vfs_busy(mp, LK_NOWAIT, &mountlist_slock)) {
			nmp = mp->mnt_list.cqe_next;
			continue;
		}
		if (sfsp && count < maxcount) {
			sp = &mp->mnt_stat;
			/*
			 * If MNT_NOWAIT or MNT_LAZY is specified, do not
			 * refresh the fsstat cache. MNT_WAIT or MNT_LAXY
			 * overrides MNT_NOWAIT.
			 */
			if (SCARG(uap, flags) != MNT_NOWAIT &&
			    SCARG(uap, flags) != MNT_LAZY &&
			    (SCARG(uap, flags) == MNT_WAIT ||
			     SCARG(uap, flags) == 0) &&
			    (error = VFS_STATFS(mp, sp, p)) != 0) {
				simple_lock(&mountlist_slock);
				nmp = mp->mnt_list.cqe_next;
				vfs_unbusy(mp);
				continue;
			}
			sp->f_flags = mp->mnt_flag & MNT_VISFLAGMASK;
			sp->f_oflags = sp->f_flags & 0xffff;
			netbsd32_from_statfs(sp, &sb32);
			error = copyout(&sb32, sfsp, sizeof(sb32));
			if (error) {
				vfs_unbusy(mp);
				return (error);
			}
			sfsp += sizeof(sb32);
		}
		count++;
		simple_lock(&mountlist_slock);
		nmp = mp->mnt_list.cqe_next;
		vfs_unbusy(mp);
	}
	simple_unlock(&mountlist_slock);
	if (sfsp && count > maxcount)
		*retval = maxcount;
	else
		*retval = count;
	return (0);
}

int
netbsd32_readv(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct netbsd32_readv_args /* {
		syscallarg(int) fd;
		syscallarg(const netbsd32_iovecp_t) iovp;
		syscallarg(int) iovcnt;
	} */ *uap = v;
	int fd = SCARG(uap, fd);
	struct file *fp;
	struct filedesc *fdp = p->p_fd;

	if ((u_int)fd >= fdp->fd_nfiles ||
	    (fp = fdp->fd_ofiles[fd]) == NULL ||
	    (fp->f_flag & FREAD) == 0)
		return (EBADF);

	return (dofilereadv32(p, fd, fp, (struct netbsd32_iovec *)(u_long)SCARG(uap, iovp), 
			      SCARG(uap, iovcnt), &fp->f_offset, FOF_UPDATE_OFFSET, retval));
}

/* Damn thing copies in the iovec! */
int
dofilereadv32(p, fd, fp, iovp, iovcnt, offset, flags, retval)
	struct proc *p;
	int fd;
	struct file *fp;
	struct netbsd32_iovec *iovp;
	int iovcnt;
	off_t *offset;
	int flags;
	register_t *retval;
{
	struct uio auio;
	struct iovec *iov;
	struct iovec *needfree;
	struct iovec aiov[UIO_SMALLIOV];
	long i, cnt, error = 0;
	u_int iovlen;
#ifdef KTRACE
	struct iovec *ktriov = NULL;
#endif

	/* note: can't use iovlen until iovcnt is validated */
	iovlen = iovcnt * sizeof(struct iovec);
	if ((u_int)iovcnt > UIO_SMALLIOV) {
		if ((u_int)iovcnt > IOV_MAX)
			return (EINVAL);
		MALLOC(iov, struct iovec *, iovlen, M_IOV, M_WAITOK);
		needfree = iov;
	} else if ((u_int)iovcnt > 0) {
		iov = aiov;
		needfree = NULL;
	} else
		return (EINVAL);

	auio.uio_iov = iov;
	auio.uio_iovcnt = iovcnt;
	auio.uio_rw = UIO_READ;
	auio.uio_segflg = UIO_USERSPACE;
	auio.uio_procp = p;
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
#ifdef KTRACE
	/*
	 * if tracing, save a copy of iovec
	 */
	if (KTRPOINT(p, KTR_GENIO))  {
		MALLOC(ktriov, struct iovec *, iovlen, M_TEMP, M_WAITOK);
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
	if (KTRPOINT(p, KTR_GENIO))
		if (error == 0) {
			ktrgenio(p, fd, UIO_READ, ktriov, cnt,
			    error);
		FREE(ktriov, M_TEMP);
	}
#endif
	*retval = cnt;
done:
	if (needfree)
		FREE(needfree, M_IOV);
	return (error);
}

int
netbsd32_writev(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct netbsd32_writev_args /* {
		syscallarg(int) fd;
		syscallarg(const netbsd32_iovecp_t) iovp;
		syscallarg(int) iovcnt;
	} */ *uap = v;
	int fd = SCARG(uap, fd);
	struct file *fp;
	struct filedesc *fdp = p->p_fd;

	if ((u_int)fd >= fdp->fd_nfiles ||
	    (fp = fdp->fd_ofiles[fd]) == NULL ||
	    (fp->f_flag & FWRITE) == 0)
		return (EBADF);

	return (dofilewritev32(p, fd, fp, (struct netbsd32_iovec *)(u_long)SCARG(uap, iovp),
			       SCARG(uap, iovcnt), &fp->f_offset, FOF_UPDATE_OFFSET, retval));
}

int
dofilewritev32(p, fd, fp, iovp, iovcnt, offset, flags, retval)
	struct proc *p;
	int fd;
	struct file *fp;
	struct netbsd32_iovec *iovp;
	int iovcnt;
	off_t *offset;
	int flags;
	register_t *retval;
{
	struct uio auio;
	struct iovec *iov;
	struct iovec *needfree;
	struct iovec aiov[UIO_SMALLIOV];
	long i, cnt, error = 0;
	u_int iovlen;
#ifdef KTRACE
	struct iovec *ktriov = NULL;
#endif

	/* note: can't use iovlen until iovcnt is validated */
	iovlen = iovcnt * sizeof(struct iovec);
	if ((u_int)iovcnt > UIO_SMALLIOV) {
		if ((u_int)iovcnt > IOV_MAX)
			return (EINVAL);
		MALLOC(iov, struct iovec *, iovlen, M_IOV, M_WAITOK);
		needfree = iov;
	} else if ((u_int)iovcnt > 0) {
		iov = aiov;
		needfree = NULL;
	} else
		return (EINVAL);

	auio.uio_iov = iov;
	auio.uio_iovcnt = iovcnt;
	auio.uio_rw = UIO_WRITE;
	auio.uio_segflg = UIO_USERSPACE;
	auio.uio_procp = p;
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
#ifdef KTRACE
	/*
	 * if tracing, save a copy of iovec
	 */
	if (KTRPOINT(p, KTR_GENIO))  {
		MALLOC(ktriov, struct iovec *, iovlen, M_TEMP, M_WAITOK);
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
			ktrgenio(p, fd, UIO_WRITE, ktriov, cnt,
			    error);
		FREE(ktriov, M_TEMP);
	}
#endif
	*retval = cnt;
done:
	if (needfree)
		FREE(needfree, M_IOV);
	return (error);
}

int
netbsd32_utimes(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct netbsd32_utimes_args /* {
		syscallarg(const netbsd32_charp) path;
		syscallarg(const netbsd32_timevalp_t) tptr;
	} */ *uap = v;
	int error;
	struct nameidata nd;

	NDINIT(&nd, LOOKUP, FOLLOW, UIO_USERSPACE, (char *)(u_long)SCARG(uap, path), p);
	if ((error = namei(&nd)) != 0)
		return (error);

	error = change_utimes32(nd.ni_vp, SCARG(uap, tptr), p);

	vrele(nd.ni_vp);
	return (error);
}

/*
 * Common routine to set access and modification times given a vnode.
 */
static int
change_utimes32(vp, tptr, p)
	struct vnode *vp;
	netbsd32_timevalp_t tptr;
	struct proc *p;
{
	struct netbsd32_timeval tv32[2];
	struct timeval tv[2];
	struct vattr vattr;
	int error;

	VATTR_NULL(&vattr);
	if (tptr == NULL) {
		microtime(&tv[0]);
		tv[1] = tv[0];
		vattr.va_vaflags |= VA_UTIMES_NULL;
	} else {
		error = copyin((caddr_t)(u_long)tptr, tv32, sizeof(tv32));
		if (error)
			return (error);
		netbsd32_to_timeval(&tv32[0], &tv[0]);
		netbsd32_to_timeval(&tv32[1], &tv[1]);
	}
	VOP_LEASE(vp, p, p->p_ucred, LEASE_WRITE);
	vn_lock(vp, LK_EXCLUSIVE | LK_RETRY);
	vattr.va_atime.tv_sec = tv[0].tv_sec;
	vattr.va_atime.tv_nsec = tv[0].tv_usec * 1000;
	vattr.va_mtime.tv_sec = tv[1].tv_sec;
	vattr.va_mtime.tv_nsec = tv[1].tv_usec * 1000;
	error = VOP_SETATTR(vp, &vattr, p->p_ucred, p);
	VOP_UNLOCK(vp, 0);
	return (error);
}

int
netbsd32_statfs(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct netbsd32_statfs_args /* {
		syscallarg(const netbsd32_charp) path;
		syscallarg(netbsd32_statfsp_t) buf;
	} */ *uap = v;
	struct mount *mp;
	struct statfs *sp;
	struct netbsd32_statfs s32;
	int error;
	struct nameidata nd;

	NDINIT(&nd, LOOKUP, FOLLOW, UIO_USERSPACE, (char *)(u_long)SCARG(uap, path), p);
	if ((error = namei(&nd)) != 0)
		return (error);
	mp = nd.ni_vp->v_mount;
	sp = &mp->mnt_stat;
	vrele(nd.ni_vp);
	if ((error = VFS_STATFS(mp, sp, p)) != 0)
		return (error);
	sp->f_flags = mp->mnt_flag & MNT_VISFLAGMASK;
	netbsd32_from_statfs(sp, &s32);
	return (copyout(&s32, (caddr_t)(u_long)SCARG(uap, buf), sizeof(s32)));
}

int
netbsd32_fstatfs(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct netbsd32_fstatfs_args /* {
		syscallarg(int) fd;
		syscallarg(netbsd32_statfsp_t) buf;
	} */ *uap = v;
	struct file *fp;
	struct mount *mp;
	struct statfs *sp;
	struct netbsd32_statfs s32;
	int error;

	/* getvnode() will use the descriptor for us */
	if ((error = getvnode(p->p_fd, SCARG(uap, fd), &fp)) != 0)
		return (error);
	mp = ((struct vnode *)fp->f_data)->v_mount;
	sp = &mp->mnt_stat;
	if ((error = VFS_STATFS(mp, sp, p)) != 0)
		goto out;
	sp->f_flags = mp->mnt_flag & MNT_VISFLAGMASK;
	netbsd32_from_statfs(sp, &s32);
	error = copyout(&s32, (caddr_t)(u_long)SCARG(uap, buf), sizeof(s32));
 out:
	FILE_UNUSE(fp, p);
	return (error);
}

int
netbsd32_futimes(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct netbsd32_futimes_args /* {
		syscallarg(int) fd;
		syscallarg(const netbsd32_timevalp_t) tptr;
	} */ *uap = v;
	int error;
	struct file *fp;

	/* getvnode() will use the descriptor for us */
	if ((error = getvnode(p->p_fd, SCARG(uap, fd), &fp)) != 0)
		return (error);

	error = change_utimes32((struct vnode *)fp->f_data, 
				SCARG(uap, tptr), p);
	FILE_UNUSE(fp, p);
	return (error);
}

int
netbsd32_getdents(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct netbsd32_getdents_args /* {
		syscallarg(int) fd;
		syscallarg(netbsd32_charp) buf;
		syscallarg(netbsd32_size_t) count;
	} */ *uap = v;
	struct file *fp;
	int error, done;

	/* getvnode() will use the descriptor for us */
	if ((error = getvnode(p->p_fd, SCARG(uap, fd), &fp)) != 0)
		return (error);
	if ((fp->f_flag & FREAD) == 0) {
		error = EBADF;
		goto out;
	}
	error = vn_readdir(fp, (caddr_t)(u_long)SCARG(uap, buf), UIO_USERSPACE,
			SCARG(uap, count), &done, p, 0, 0);
	*retval = done;
 out:
	FILE_UNUSE(fp, p);
	return (error);
}

int
netbsd32_lutimes(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct netbsd32_lutimes_args /* {
		syscallarg(const netbsd32_charp) path;
		syscallarg(const netbsd32_timevalp_t) tptr;
	} */ *uap = v;
	int error;
	struct nameidata nd;

	NDINIT(&nd, LOOKUP, NOFOLLOW, UIO_USERSPACE, (caddr_t)(u_long)SCARG(uap, path), p);
	if ((error = namei(&nd)) != 0)
		return (error);

	error = change_utimes32(nd.ni_vp, SCARG(uap, tptr), p);

	vrele(nd.ni_vp);
	return (error);
}

int
netbsd32___stat13(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct netbsd32___stat13_args /* {
		syscallarg(const netbsd32_charp) path;
		syscallarg(netbsd32_statp_t) ub;
	} */ *uap = v;
	struct netbsd32_stat sb32;
	struct stat sb;
	int error;
	struct nameidata nd;
	caddr_t sg;
	const char *path;

	path = (char *)(u_long)SCARG(uap, path);
	sg = stackgap_init(p->p_emul);
	CHECK_ALT_EXIST(p, &sg, path);

	NDINIT(&nd, LOOKUP, FOLLOW | LOCKLEAF, UIO_USERSPACE, path, p);
	if ((error = namei(&nd)) != 0)
		return (error);
	error = vn_stat(nd.ni_vp, &sb, p);
	vput(nd.ni_vp);
	if (error)
		return (error);
	netbsd32_from___stat13(&sb, &sb32);
	error = copyout(&sb32, (caddr_t)(u_long)SCARG(uap, ub), sizeof(sb32));
	return (error);
}

int
netbsd32___fstat13(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct netbsd32___fstat13_args /* {
		syscallarg(int) fd;
		syscallarg(netbsd32_statp_t) sb;
	} */ *uap = v;
	int fd = SCARG(uap, fd);
	struct filedesc *fdp = p->p_fd;
	struct file *fp;
	struct netbsd32_stat sb32;
	struct stat ub;
	int error = 0;

	if ((u_int)fd >= fdp->fd_nfiles ||
	    (fp = fdp->fd_ofiles[fd]) == NULL)
		return (EBADF);

	FILE_USE(fp);
	error = (*fp->f_ops->fo_stat)(fp, &ub, p);
	FILE_UNUSE(fp, p);

	if (error == 0) {
		netbsd32_from___stat13(&ub, &sb32);
		error = copyout(&sb32, (caddr_t)(u_long)SCARG(uap, sb), sizeof(sb32));
	}
	return (error);
}

int
netbsd32___lstat13(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct netbsd32___lstat13_args /* {
		syscallarg(const netbsd32_charp) path;
		syscallarg(netbsd32_statp_t) ub;
	} */ *uap = v;
	struct netbsd32_stat sb32;
	struct stat sb;
	int error;
	struct nameidata nd;
	caddr_t sg;
	const char *path;

	path = (char *)(u_long)SCARG(uap, path);
	sg = stackgap_init(p->p_emul);
	CHECK_ALT_EXIST(p, &sg, path);

	NDINIT(&nd, LOOKUP, FOLLOW | LOCKLEAF, UIO_USERSPACE, path, p);
	if ((error = namei(&nd)) != 0)
		return (error);
	error = vn_stat(nd.ni_vp, &sb, p);
	vput(nd.ni_vp);
	if (error)
		return (error);
	netbsd32_from___stat13(&sb, &sb32);
	error = copyout(&sb32, (caddr_t)(u_long)SCARG(uap, ub), sizeof(sb32));
	return (error);
}

int
netbsd32_preadv(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct netbsd32_preadv_args /* {
		syscallarg(int) fd;
		syscallarg(const netbsd32_iovecp_t) iovp;
		syscallarg(int) iovcnt;
		syscallarg(int) pad;
		syscallarg(off_t) offset;
	} */ *uap = v;
	struct filedesc *fdp = p->p_fd;
	struct file *fp;
	struct vnode *vp;
	off_t offset;
	int error, fd = SCARG(uap, fd);

	if ((u_int)fd >= fdp->fd_nfiles ||
	    (fp = fdp->fd_ofiles[fd]) == NULL ||
	    (fp->f_flag & FREAD) == 0)
		return (EBADF);

	vp = (struct vnode *)fp->f_data;
	if (fp->f_type != DTYPE_VNODE
	    || vp->v_type == VFIFO)
		return (ESPIPE);

	offset = SCARG(uap, offset);

	/*
	 * XXX This works because no file systems actually
	 * XXX take any action on the seek operation.
	 */
	if ((error = VOP_SEEK(vp, fp->f_offset, offset, fp->f_cred)) != 0)
		return (error);

	return (dofilereadv32(p, fd, fp, (struct netbsd32_iovec *)(u_long)SCARG(uap, iovp), SCARG(uap, iovcnt),
	    &offset, 0, retval));
}

int
netbsd32_pwritev(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct netbsd32_pwritev_args /* {
		syscallarg(int) fd;
		syscallarg(const netbsd32_iovecp_t) iovp;
		syscallarg(int) iovcnt;
		syscallarg(int) pad;
		syscallarg(off_t) offset;
	} */ *uap = v;
	struct filedesc *fdp = p->p_fd;
	struct file *fp;
	struct vnode *vp;
	off_t offset;
	int error, fd = SCARG(uap, fd);

	if ((u_int)fd >= fdp->fd_nfiles ||
	    (fp = fdp->fd_ofiles[fd]) == NULL ||
	    (fp->f_flag & FWRITE) == 0)
		return (EBADF);

	vp = (struct vnode *)fp->f_data;
	if (fp->f_type != DTYPE_VNODE
	    || vp->v_type == VFIFO)
		return (ESPIPE);

	offset = SCARG(uap, offset);

	/*
	 * XXX This works because no file systems actually
	 * XXX take any action on the seek operation.
	 */
	if ((error = VOP_SEEK(vp, fp->f_offset, offset, fp->f_cred)) != 0)
		return (error);

	return (dofilewritev32(p, fd, fp, (struct netbsd32_iovec *)(u_long)SCARG(uap, iovp), SCARG(uap, iovcnt),
	    &offset, 0, retval));
}

/*
 * Find pathname of process's current directory.
 *
 * Use vfs vnode-to-name reverse cache; if that fails, fall back
 * to reading directory contents.
 */
int
getcwd_common __P((struct vnode *, struct vnode *,
		   char **, char *, int, int, struct proc *));

int netbsd32___getcwd(p, v, retval) 
	struct proc *p;
	void   *v;
	register_t *retval;
{
	struct netbsd32___getcwd_args /* {
		syscallarg(char *) bufp;
		syscallarg(size_t) length;
	} */ *uap = v;

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
			       GETCWD_CHECK_ACCESS, p);

	if (error)
		goto out;
	lenused = bend - bp;
	*retval = lenused;
	/* put the result into user buffer */
	error = copyout(bp, (caddr_t)(u_long)SCARG(uap, bufp), lenused);

out:
	free(path, M_TEMP);
	return error;
}
