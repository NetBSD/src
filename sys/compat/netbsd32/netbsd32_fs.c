/*	$NetBSD: netbsd32_fs.c,v 1.24.4.6 2006/05/12 23:06:36 elad Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: netbsd32_fs.c,v 1.24.4.6 2006/05/12 23:06:36 elad Exp $");

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
#include <sys/sa.h>
#include <sys/statvfs.h>
#include <sys/syscallargs.h>
#include <sys/proc.h>
#include <sys/dirent.h>
#include <sys/kauth.h>

#include <compat/netbsd32/netbsd32.h>
#include <compat/netbsd32/netbsd32_syscallargs.h>
#include <compat/netbsd32/netbsd32_conv.h>


static int dofilereadv32 __P((struct lwp *, int, struct file *, struct netbsd32_iovec *,
			      int, off_t *, int, register_t *));
static int dofilewritev32 __P((struct lwp *, int, struct file *, struct netbsd32_iovec *,
			       int,  off_t *, int, register_t *));
static int change_utimes32 __P((struct vnode *, netbsd32_timevalp_t, struct lwp *));

int
netbsd32_readv(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct netbsd32_readv_args /* {
		syscallarg(int) fd;
		syscallarg(const netbsd32_iovecp_t) iovp;
		syscallarg(int) iovcnt;
	} */ *uap = v;
	int fd = SCARG(uap, fd);
	struct proc *p = l->l_proc;
	struct file *fp;
	struct filedesc *fdp = p->p_fd;

	if ((fp = fd_getfile(fdp, fd)) == NULL)
		return (EBADF);

	if ((fp->f_flag & FREAD) == 0)
		return (EBADF);

	FILE_USE(fp);

	return (dofilereadv32(l, fd, fp,
	    (struct netbsd32_iovec *)NETBSD32PTR64(SCARG(uap, iovp)),
	    SCARG(uap, iovcnt), &fp->f_offset, FOF_UPDATE_OFFSET, retval));
}

/* Damn thing copies in the iovec! */
int
dofilereadv32(l, fd, fp, iovp, iovcnt, offset, flags, retval)
	struct lwp *l;
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
#ifdef KTRACE
	/*
	 * if tracing, save a copy of iovec
	 */
	if (KTRPOINT(l->l_proc, KTR_GENIO))  {
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
	if (KTRPOINT(l->l_proc, KTR_GENIO))
		if (error == 0) {
			ktrgenio(l, fd, UIO_READ, ktriov, cnt,
			    error);
		free(ktriov, M_TEMP);
	}
#endif
	*retval = cnt;
done:
	if (needfree)
		free(needfree, M_IOV);
out:
	FILE_UNUSE(fp, l);
	return (error);
}

int
netbsd32_writev(l, v, retval)
	struct lwp *l;
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
	struct proc *p = l->l_proc;
	struct filedesc *fdp = p->p_fd;

	if ((fp = fd_getfile(fdp, fd)) == NULL)
		return (EBADF);

	if ((fp->f_flag & FWRITE) == 0)
		return (EBADF);

	FILE_USE(fp);

	return (dofilewritev32(l, fd, fp,
	    (struct netbsd32_iovec *)NETBSD32PTR64(SCARG(uap, iovp)),
	    SCARG(uap, iovcnt), &fp->f_offset, FOF_UPDATE_OFFSET, retval));
}

int
dofilewritev32(l, fd, fp, iovp, iovcnt, offset, flags, retval)
	struct lwp *l;
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
	struct proc *p = l->l_proc;
	long i, cnt, error = 0;
	u_int iovlen;
#ifdef KTRACE
	struct iovec *ktriov = NULL;
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
			ktrgenio(l, fd, UIO_WRITE, ktriov, cnt,
			    error);
		free(ktriov, M_TEMP);
	}
#endif
	*retval = cnt;
done:
	if (needfree)
		free(needfree, M_IOV);
out:
	FILE_UNUSE(fp, l);
	return (error);
}

int
netbsd32_utimes(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct netbsd32_utimes_args /* {
		syscallarg(const netbsd32_charp) path;
		syscallarg(const netbsd32_timevalp_t) tptr;
	} */ *uap = v;
	int error;
	struct nameidata nd;

	NDINIT(&nd, LOOKUP, FOLLOW, UIO_USERSPACE,
	    (char *)NETBSD32PTR64(SCARG(uap, path)), l);
	if ((error = namei(&nd)) != 0)
		return (error);

	error = change_utimes32(nd.ni_vp, SCARG(uap, tptr), l);

	vrele(nd.ni_vp);
	return (error);
}

/*
 * Common routine to set access and modification times given a vnode.
 */
static int
change_utimes32(vp, tptr, l)
	struct vnode *vp;
	netbsd32_timevalp_t tptr;
	struct lwp *l;
{
	struct netbsd32_timeval tv32[2];
	struct timeval tv[2];
	struct proc *p = l->l_proc;
	struct vattr vattr;
	int error;

	VATTR_NULL(&vattr);
	if (tptr == 0) {
		microtime(&tv[0]);
		tv[1] = tv[0];
		vattr.va_vaflags |= VA_UTIMES_NULL;
	} else {
		error = copyin((caddr_t)NETBSD32PTR64(tptr), tv32,
		    sizeof(tv32));
		if (error)
			return (error);
		netbsd32_to_timeval(&tv32[0], &tv[0]);
		netbsd32_to_timeval(&tv32[1], &tv[1]);
	}
	VOP_LEASE(vp, l, p->p_cred, LEASE_WRITE);
	vn_lock(vp, LK_EXCLUSIVE | LK_RETRY);
	vattr.va_atime.tv_sec = tv[0].tv_sec;
	vattr.va_atime.tv_nsec = tv[0].tv_usec * 1000;
	vattr.va_mtime.tv_sec = tv[1].tv_sec;
	vattr.va_mtime.tv_nsec = tv[1].tv_usec * 1000;
	error = VOP_SETATTR(vp, &vattr, p->p_cred, l);
	VOP_UNLOCK(vp, 0);
	return (error);
}

int
netbsd32_statvfs1(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct netbsd32_statvfs1_args /* {
		syscallarg(const netbsd32_charp) path;
		syscallarg(netbsd32_statvfsp_t) buf;
		syscallarg(int) flags;
	} */ *uap = v;
	struct mount *mp;
	struct statvfs *sbuf;
	struct netbsd32_statvfs *s32;
	struct nameidata nd;
	int error;

	NDINIT(&nd, LOOKUP, FOLLOW, UIO_USERSPACE,
	    (char *)NETBSD32PTR64(SCARG(uap, path)), l);
	if ((error = namei(&nd)) != 0)
		return (error);
	/* Allocating on the stack would blow it up */
	sbuf = (struct statvfs *)malloc(sizeof(struct statvfs), M_TEMP,
	    M_WAITOK);
	mp = nd.ni_vp->v_mount;
	vrele(nd.ni_vp);
	if ((error = dostatvfs(mp, sbuf, l, SCARG(uap, flags), 1)) != 0)
		goto out;
	s32 = (struct netbsd32_statvfs *)
	    malloc(sizeof(struct netbsd32_statvfs), M_TEMP, M_WAITOK);
	netbsd32_from_statvfs(sbuf, s32);
	error = copyout(s32, (caddr_t)NETBSD32PTR64(SCARG(uap, buf)),
	    sizeof(struct netbsd32_statvfs));
	free(s32, M_TEMP);
out:
	free(sbuf, M_TEMP);
	return (error);
}

int
netbsd32_fstatvfs1(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct netbsd32_fstatvfs1_args /* {
		syscallarg(int) fd;
		syscallarg(netbsd32_statvfsp_t) buf;
		syscallarg(int) flags;
	} */ *uap = v;
	struct proc *p = l->l_proc;
	struct file *fp;
	struct mount *mp;
	struct statvfs *sbuf;
	struct netbsd32_statvfs *s32;
	int error;

	/* getvnode() will use the descriptor for us */
	if ((error = getvnode(p->p_fd, SCARG(uap, fd), &fp)) != 0)
		return (error);
	mp = ((struct vnode *)fp->f_data)->v_mount;
	sbuf = (struct statvfs *)malloc(sizeof(struct statvfs), M_TEMP,
	    M_WAITOK);
	if ((error = dostatvfs(mp, sbuf, l, SCARG(uap, flags), 1)) != 0)
		goto out;
	s32 = (struct netbsd32_statvfs *)
	    malloc(sizeof(struct netbsd32_statvfs), M_TEMP, M_WAITOK);
	netbsd32_from_statvfs(sbuf, s32);
	error = copyout(s32, (caddr_t)NETBSD32PTR64(SCARG(uap, buf)),
	    sizeof(struct netbsd32_statvfs));
	free(s32, M_TEMP);
 out:
	free(sbuf, M_TEMP);
	FILE_UNUSE(fp, l);
	return error;
}

int
netbsd32_getvfsstat(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct netbsd32_getvfsstat_args /* {
		syscallarg(netbsd32_statvfsp_t) buf;
		syscallarg(netbsd32_size_t) bufsize;
		syscallarg(int) flags;
	} */ *uap = v;
	int root = 0;
	struct proc *p = l->l_proc;
	struct mount *mp, *nmp;
	struct statvfs *sbuf;
	struct netbsd32_statvfs *sfsp;
	struct netbsd32_statvfs *s32;
	size_t count, maxcount;
	int error = 0;

	maxcount = SCARG(uap, bufsize) / sizeof(struct netbsd32_statvfs);
	sfsp = (struct netbsd32_statvfs *)NETBSD32PTR64(SCARG(uap, buf));
	sbuf = (struct statvfs *)malloc(sizeof(struct statvfs), M_TEMP,
	    M_WAITOK);
	s32 = (struct netbsd32_statvfs *)
	    malloc(sizeof(struct netbsd32_statvfs), M_TEMP, M_WAITOK);
	simple_lock(&mountlist_slock);
	count = 0;
	for (mp = CIRCLEQ_FIRST(&mountlist); mp != (void *)&mountlist;
	     mp = nmp) {
		if (vfs_busy(mp, LK_NOWAIT, &mountlist_slock)) {
			nmp = CIRCLEQ_NEXT(mp, mnt_list);
			continue;
		}
		if (sfsp && count < maxcount) {
			error = dostatvfs(mp, sbuf, l, SCARG(uap, flags), 0);
			if (error) {
				simple_lock(&mountlist_slock);
				nmp = CIRCLEQ_NEXT(mp, mnt_list);
				vfs_unbusy(mp);
				continue;
			}
			netbsd32_from_statvfs(sbuf, s32);
			error = copyout(s32, sfsp, sizeof(*sfsp));
			if (error) {
				vfs_unbusy(mp);
				goto out;
			}
			sfsp++;
			root |= strcmp(sbuf->f_mntonname, "/") == 0;
		}
		count++;
		simple_lock(&mountlist_slock);
		nmp = CIRCLEQ_NEXT(mp, mnt_list);
		vfs_unbusy(mp);
	}
	simple_unlock(&mountlist_slock);
	if (root == 0 && p->p_cwdi->cwdi_rdir) {
		/*
		 * fake a root entry
		 */
		if ((error = dostatvfs(p->p_cwdi->cwdi_rdir->v_mount, sbuf, l,
		    SCARG(uap, flags), 1)) != 0)
			goto out;
		if (sfsp) {
			netbsd32_from_statvfs(sbuf, s32);
			error = copyout(s32, sfsp, sizeof(*sfsp));
		}
		count++;
	}
	if (sfsp && count > maxcount)
		*retval = maxcount;
	else
		*retval = count;

out:
	free(s32, M_TEMP);
	free(sbuf, M_TEMP);
	return (error);
}

int
netbsd32_fhstatvfs1(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct netbsd32_fhstatvfs1_args /* {
		syscallarg(const netbsd32_fhandlep_t) fhp;
		syscallarg(netbsd32_statvfsp_t) buf;
		syscallarg(int) flags;
	} */ *uap = v;
	struct proc *p = l->l_proc;
	struct statvfs *sbuf;
	struct netbsd32_statvfs *s32;
	fhandle_t fh;
	struct mount *mp;
	struct vnode *vp;
	int error;

	/*
	 * Must be super user
	 */
	if ((error = kauth_authorize_generic(p->p_cred, KAUTH_GENERIC_ISSUSER, &p->p_acflag)) != 0)
		return error;

	if ((error = copyin((caddr_t)NETBSD32PTR64(SCARG(uap, fhp)), &fh,
	    sizeof(fhandle_t))) != 0)
		return error;

	if ((mp = vfs_getvfs(&fh.fh_fsid)) == NULL)
		return ESTALE;
	if ((error = VFS_FHTOVP(mp, &fh.fh_fid, &vp)))
		return error;

	sbuf = (struct statvfs *)malloc(sizeof(struct statvfs), M_TEMP,
	    M_WAITOK);
	mp = vp->v_mount;
	if ((error = dostatvfs(mp, sbuf, l, SCARG(uap, flags), 1)) != 0) {
		vput(vp);
		goto out;
	}
	vput(vp);

	s32 = (struct netbsd32_statvfs *)
	    malloc(sizeof(struct netbsd32_statvfs), M_TEMP, M_WAITOK);
	netbsd32_from_statvfs(sbuf, s32);
	error = copyout(s32, (caddr_t)NETBSD32PTR64(SCARG(uap, buf)),
	    sizeof(struct netbsd32_statvfs));
	free(s32, M_TEMP);

out:
	free(sbuf, M_TEMP);
	return (error);
}

int
netbsd32_futimes(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct netbsd32_futimes_args /* {
		syscallarg(int) fd;
		syscallarg(const netbsd32_timevalp_t) tptr;
	} */ *uap = v;
	int error;
	struct file *fp;
	struct proc *p = l->l_proc;

	/* getvnode() will use the descriptor for us */
	if ((error = getvnode(p->p_fd, SCARG(uap, fd), &fp)) != 0)
		return (error);

	error = change_utimes32((struct vnode *)fp->f_data,
				SCARG(uap, tptr), l);
	FILE_UNUSE(fp, l);
	return (error);
}

int
netbsd32_sys___getdents30(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct netbsd32_sys___getdents30_args /* {
		syscallarg(int) fd;
		syscallarg(netbsd32_charp) buf;
		syscallarg(netbsd32_size_t) count;
	} */ *uap = v;
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
	error = vn_readdir(fp, (caddr_t)NETBSD32PTR64(SCARG(uap, buf)),
	    UIO_USERSPACE, SCARG(uap, count), &done, l, 0, 0);
	*retval = done;
 out:
	FILE_UNUSE(fp, l);
	return (error);
}

int
netbsd32_lutimes(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct netbsd32_lutimes_args /* {
		syscallarg(const netbsd32_charp) path;
		syscallarg(const netbsd32_timevalp_t) tptr;
	} */ *uap = v;
	int error;
	struct nameidata nd;

	NDINIT(&nd, LOOKUP, NOFOLLOW, UIO_USERSPACE,
	    (caddr_t)NETBSD32PTR64(SCARG(uap, path)), l);
	if ((error = namei(&nd)) != 0)
		return (error);

	error = change_utimes32(nd.ni_vp, SCARG(uap, tptr), l);

	vrele(nd.ni_vp);
	return (error);
}

int
netbsd32_sys___stat30(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct netbsd32_sys___stat30_args /* {
		syscallarg(const netbsd32_charp) path;
		syscallarg(netbsd32_statp_t) ub;
	} */ *uap = v;
	struct netbsd32_stat sb32;
	struct stat sb;
	int error;
	struct nameidata nd;
	caddr_t sg;
	const char *path;
	struct proc *p = l->l_proc;

	path = (char *)NETBSD32PTR64(SCARG(uap, path));
	sg = stackgap_init(p, 0);
	CHECK_ALT_EXIST(l, &sg, path);

	NDINIT(&nd, LOOKUP, FOLLOW | LOCKLEAF, UIO_USERSPACE, path, l);
	if ((error = namei(&nd)) != 0)
		return (error);
	error = vn_stat(nd.ni_vp, &sb, l);
	vput(nd.ni_vp);
	if (error)
		return (error);
	netbsd32_from___stat30(&sb, &sb32);
	error = copyout(&sb32, (caddr_t)NETBSD32PTR64(SCARG(uap, ub)),
	    sizeof(sb32));
	return (error);
}

int
netbsd32_sys___fstat30(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct netbsd32_sys___fstat30_args /* {
		syscallarg(int) fd;
		syscallarg(netbsd32_statp_t) sb;
	} */ *uap = v;
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
		error = copyout(&sb32, (caddr_t)NETBSD32PTR64(SCARG(uap, sb)),
		    sizeof(sb32));
	}
	return (error);
}

int
netbsd32_sys___lstat30(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct netbsd32_sys___lstat30_args /* {
		syscallarg(const netbsd32_charp) path;
		syscallarg(netbsd32_statp_t) ub;
	} */ *uap = v;
	struct netbsd32_stat sb32;
	struct stat sb;
	int error;
	struct nameidata nd;
	caddr_t sg;
	const char *path;
	struct proc *p = l->l_proc;

	path = (char *)NETBSD32PTR64(SCARG(uap, path));
	sg = stackgap_init(p, 0);
	CHECK_ALT_EXIST(l, &sg, path);

	NDINIT(&nd, LOOKUP, NOFOLLOW | LOCKLEAF, UIO_USERSPACE, path, l);
	if ((error = namei(&nd)) != 0)
		return (error);
	error = vn_stat(nd.ni_vp, &sb, l);
	vput(nd.ni_vp);
	if (error)
		return (error);
	netbsd32_from___stat30(&sb, &sb32);
	error = copyout(&sb32, (caddr_t)NETBSD32PTR64(SCARG(uap, ub)),
	    sizeof(sb32));
	return (error);
}

int netbsd32_sys___fhstat30(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct netbsd32_sys___fhstat30_args /* {
		syscallarg(const netbsd32_fhandlep_t) fhp;
		syscallarg(netbsd32_statp_t) sb;
	} */ *uap = v;
	struct proc *p = l->l_proc;
	struct stat sb;
	struct netbsd32_stat sb32;
	int error;
	fhandle_t fh;
	struct mount *mp;
	struct vnode *vp;

	/*
	 * Must be super user
	 */
	if ((error = kauth_authorize_generic(p->p_cred, KAUTH_GENERIC_ISSUSER,
	    &p->p_acflag)))
		return (error);

	if ((error = copyin(NETBSD32PTR64(SCARG(uap, fhp)), &fh,
	    sizeof(fhandle_t))) != 0)
		return (error);

	if ((mp = vfs_getvfs(&fh.fh_fsid)) == NULL)
		return (ESTALE);
	if (mp->mnt_op->vfs_fhtovp == NULL)
		return EOPNOTSUPP;
	if ((error = VFS_FHTOVP(mp, &fh.fh_fid, &vp)))
		return (error);
	error = vn_stat(vp, &sb, l);
	vput(vp);
	if (error)
		return (error);
	netbsd32_from___stat30(&sb, &sb32);
	error = copyout(&sb32, NETBSD32PTR64(SCARG(uap, sb)), sizeof(sb));
	return (error);
}

int
netbsd32_preadv(l, v, retval)
	struct lwp *l;
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
	struct proc *p = l->l_proc;
	struct filedesc *fdp = p->p_fd;
	struct file *fp;
	struct vnode *vp;
	off_t offset;
	int error, fd = SCARG(uap, fd);

	if ((fp = fd_getfile(fdp, fd)) == NULL)
		return (EBADF);

	if ((fp->f_flag & FREAD) == 0)
		return (EBADF);

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

	return (dofilereadv32(l, fd, fp,
	    (struct netbsd32_iovec *)NETBSD32PTR64(SCARG(uap, iovp)),
	    SCARG(uap, iovcnt), &offset, 0, retval));

out:
	FILE_UNUSE(fp, l);
	return (error);
}

int
netbsd32_pwritev(l, v, retval)
	struct lwp *l;
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
	struct proc *p = l->l_proc;
	struct filedesc *fdp = p->p_fd;
	struct file *fp;
	struct vnode *vp;
	off_t offset;
	int error, fd = SCARG(uap, fd);

	if ((fp = fd_getfile(fdp, fd)) == NULL)
		return (EBADF);

	if ((fp->f_flag & FWRITE) == 0)
		return (EBADF);

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

	return (dofilewritev32(l, fd, fp,
	    (struct netbsd32_iovec *)NETBSD32PTR64(SCARG(uap, iovp)),
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
getcwd_common __P((struct vnode *, struct vnode *,
		   char **, char *, int, int, struct lwp *));

int netbsd32___getcwd(l, v, retval)
	struct lwp *l;
	void   *v;
	register_t *retval;
{
	struct netbsd32___getcwd_args /* {
		syscallarg(char *) bufp;
		syscallarg(size_t) length;
	} */ *uap = v;
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
	error = copyout(bp, (caddr_t)NETBSD32PTR64(SCARG(uap, bufp)), lenused);

out:
	free(path, M_TEMP);
	return error;
}
