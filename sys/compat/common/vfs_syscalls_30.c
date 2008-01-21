/*	$NetBSD: vfs_syscalls_30.c,v 1.9.6.6 2008/01/21 09:40:45 yamt Exp $	*/

/*-
 * Copyright (c) 2005 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Christos Zoulas.
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
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
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
#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: vfs_syscalls_30.c,v 1.9.6.6 2008/01/21 09:40:45 yamt Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/namei.h>
#include <sys/filedesc.h>
#include <sys/kernel.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/socketvar.h>
#include <sys/vnode.h>
#include <sys/mount.h>
#include <sys/proc.h>
#include <sys/uio.h>
#include <sys/dirent.h>
#include <sys/malloc.h>
#include <sys/kauth.h>
#include <sys/vfs_syscalls.h>

#include <sys/syscallargs.h>

#include <compat/common/compat_util.h>
#include <compat/sys/stat.h>
#include <compat/sys/dirent.h>
#include <compat/sys/mount.h>

static void cvtstat(struct stat13 *, const struct stat *);

/*
 * Convert from a new to an old stat structure.
 */
static void
cvtstat(struct stat13 *ost, const struct stat *st)
{

	ost->st_dev = st->st_dev;
	ost->st_ino = (uint32_t)st->st_ino;
	ost->st_mode = st->st_mode;
	ost->st_nlink = st->st_nlink;
	ost->st_uid = st->st_uid;
	ost->st_gid = st->st_gid;
	ost->st_rdev = st->st_rdev;
	ost->st_atimespec = st->st_atimespec;
	ost->st_mtimespec = st->st_mtimespec;
	ost->st_ctimespec = st->st_ctimespec;
	ost->st_birthtimespec = st->st_birthtimespec;
	ost->st_size = st->st_size;
	ost->st_blocks = st->st_blocks;
	ost->st_blksize = st->st_blksize;
	ost->st_flags = st->st_flags;
	ost->st_gen = st->st_gen;
}

/*
 * Get file status; this version follows links.
 */
/* ARGSUSED */
int
compat_30_sys___stat13(struct lwp *l, const struct compat_30_sys___stat13_args *uap, register_t *retval)
{
	/* {
		syscallarg(const char *) path;
		syscallarg(struct stat13 *) ub;
	} */
	struct stat sb;
	struct stat13 osb;
	int error;

	error = do_sys_stat(l, SCARG(uap, path), FOLLOW, &sb);
	if (error)
		return error;
	cvtstat(&osb, &sb);
	error = copyout(&osb, SCARG(uap, ub), sizeof (osb));
	return error;
}


/*
 * Get file status; this version does not follow links.
 */
/* ARGSUSED */
int
compat_30_sys___lstat13(struct lwp *l, const struct compat_30_sys___lstat13_args *uap, register_t *retval)
{
	/* {
		syscallarg(const char *) path;
		syscallarg(struct stat13 *) ub;
	} */
	struct stat sb;
	struct stat13 osb;
	int error;

	error = do_sys_stat(l, SCARG(uap, path), NOFOLLOW, &sb);
	if (error)
		return error;
	cvtstat(&osb, &sb);
	error = copyout(&osb, SCARG(uap, ub), sizeof (osb));
	return error;
}

/* ARGSUSED */
int
compat_30_sys_fhstat(struct lwp *l, const struct compat_30_sys_fhstat_args *uap, register_t *retval)
{
	/* {
		syscallarg(const struct compat_30_fhandle *) fhp;
		syscallarg(struct stat13 *) sb;
	} */
	struct stat sb;
	struct stat13 osb;
	int error;
	struct compat_30_fhandle fh;
	struct mount *mp;
	struct vnode *vp;

	/*
	 * Must be super user
	 */
	if ((error = kauth_authorize_system(l->l_cred, KAUTH_SYSTEM_FILEHANDLE,
	    0, NULL, NULL, NULL)))
		return (error);

	if ((error = copyin(SCARG(uap, fhp), &fh, sizeof(fh))) != 0)
		return (error);

	if ((mp = vfs_getvfs(&fh.fh_fsid)) == NULL)
		return (ESTALE);
	if (mp->mnt_op->vfs_fhtovp == NULL)
		return EOPNOTSUPP;
	if ((error = VFS_FHTOVP(mp, (struct fid*)&fh.fh_fid, &vp)))
		return (error);
	error = vn_stat(vp, &sb, l);
	vput(vp);
	if (error)
		return (error);
	cvtstat(&osb, &sb);
	error = copyout(&osb, SCARG(uap, sb), sizeof(sb));
	return (error);
}

/*
 * Return status information about a file descriptor.
 */
/* ARGSUSED */
int
compat_30_sys___fstat13(struct lwp *l, const struct compat_30_sys___fstat13_args *uap, register_t *retval)
{
	/* {
		syscallarg(int) fd;
		syscallarg(struct stat13 *) sb;
	} */
	struct proc *p = l->l_proc;
	int fd = SCARG(uap, fd);
	struct filedesc *fdp = p->p_fd;
	struct file *fp;
	struct stat sb;
	struct stat13 osb;
	int error;

	if ((fp = fd_getfile(fdp, fd)) == NULL)
		return EBADF;

	FILE_USE(fp);
	error = (*fp->f_ops->fo_stat)(fp, &sb, l);
	FILE_UNUSE(fp, l);

	if (error)
		return error;
	cvtstat(&osb, &sb);
	error = copyout(&osb, SCARG(uap, sb), sizeof (osb));
	return error;
}

/*
 * Read a block of directory entries in a file system independent format.
 */
int
compat_30_sys_getdents(struct lwp *l, const struct compat_30_sys_getdents_args *uap, register_t *retval)
{
	/* {
		syscallarg(int) fd;
		syscallarg(char *) buf;
		syscallarg(size_t) count;
	} */
	struct proc *p = l->l_proc;
	struct dirent *bdp;
	struct vnode *vp;
	char *inp, *tbuf;	/* BSD-format */
	int len, reclen;	/* BSD-format */
	char *outp;		/* NetBSD-3.0-format */
	int resid;	
	struct file *fp;
	struct uio auio;
	struct iovec aiov;
	struct dirent12 idb;
	off_t off;		/* true file offset */
	int buflen, error, eofflag;
	off_t *cookiebuf = NULL, *cookie;
	int ncookies;

	/* getvnode() will use the descriptor for us */
	if ((error = getvnode(p->p_fd, SCARG(uap, fd), &fp)) != 0)
		return error;

	if ((fp->f_flag & FREAD) == 0) {
		error = EBADF;
		goto out1;
	}

	vp = (struct vnode *)fp->f_data;
	if (vp->v_type != VDIR) {
		error = EINVAL;
		goto out1;
	}

	buflen = min(MAXBSIZE, SCARG(uap, count));
	tbuf = malloc(buflen, M_TEMP, M_WAITOK);
	vn_lock(vp, LK_EXCLUSIVE | LK_RETRY);
	off = fp->f_offset;
again:
	aiov.iov_base = tbuf;
	aiov.iov_len = buflen;
	auio.uio_iov = &aiov;
	auio.uio_iovcnt = 1;
	auio.uio_rw = UIO_READ;
	auio.uio_resid = buflen;
	auio.uio_offset = off;
	UIO_SETUP_SYSSPACE(&auio);
	/*
         * First we read into the malloc'ed buffer, then
         * we massage it into user space, one record at a time.
         */
	error = VOP_READDIR(vp, &auio, fp->f_cred, &eofflag, &cookiebuf,
	    &ncookies);
	if (error)
		goto out;

	inp = tbuf;
	outp = SCARG(uap, buf);
	resid = SCARG(uap, count);
	if ((len = buflen - auio.uio_resid) == 0)
		goto eof;

	for (cookie = cookiebuf; len > 0; len -= reclen) {
		bdp = (struct dirent *)inp;
		reclen = bdp->d_reclen;
		if (reclen & _DIRENT_ALIGN(bdp))
			panic("netbsd30_getdents: bad reclen %d", reclen);
		if (cookie)
			off = *cookie++; /* each entry points to the next */
		else
			off += reclen;
		if ((off >> 32) != 0) {
			compat_offseterr(vp, "netbsd30_getdents");
			error = EINVAL;
			goto out;
		}
		if (bdp->d_namlen >= sizeof(idb.d_name))
			idb.d_namlen = sizeof(idb.d_name) - 1;
		else
			idb.d_namlen = bdp->d_namlen;
		idb.d_reclen = _DIRENT_SIZE(&idb);
		if (reclen > len || resid < idb.d_reclen) {
			/* entry too big for buffer, so just stop */
			outp++;
			break;
		}
		/*
		 * Massage in place to make a NetBSD-3.0-shaped dirent
		 * (otherwise we have to worry about touching user memory
		 * outside of the copyout() call).
		 */
		idb.d_fileno = (u_int32_t)bdp->d_fileno;
		idb.d_type = bdp->d_type;
		(void)memcpy(idb.d_name, bdp->d_name, idb.d_namlen);
		memset(idb.d_name + idb.d_namlen, 0,
		    idb.d_reclen - _DIRENT_NAMEOFF(&idb) - idb.d_namlen);
		if ((error = copyout(&idb, outp, idb.d_reclen)) != 0)
			goto out;
		/* advance past this real entry */
		inp += reclen;
		/* advance output past NetBSD-3.0-shaped entry */
		outp += idb.d_reclen;
		resid -= idb.d_reclen;
	}

	/* if we squished out the whole block, try again */
	if (outp == SCARG(uap, buf))
		goto again;
	fp->f_offset = off;	/* update the vnode offset */

eof:
	*retval = SCARG(uap, count) - resid;
out:
	VOP_UNLOCK(vp, 0);
	if (cookiebuf)
		free(cookiebuf, M_TEMP);
	free(tbuf, M_TEMP);
out1:
	FILE_UNUSE(fp, l);
	return error;
}

/*
 * Get file handle system call
 */
int
compat_30_sys_getfh(struct lwp *l, const struct compat_30_sys_getfh_args *uap, register_t *retval)
{
	/* {
		syscallarg(char *) fname;
		syscallarg(struct compat_30_fhandle *) fhp;
	} */
	struct vnode *vp;
	struct compat_30_fhandle fh;
	int error;
	struct nameidata nd;
	size_t sz;

	/*
	 * Must be super user
	 */
	error = kauth_authorize_system(l->l_cred, KAUTH_SYSTEM_FILEHANDLE,
	    0, NULL, NULL, NULL);
	if (error)
		return (error);
	NDINIT(&nd, LOOKUP, FOLLOW | LOCKLEAF | TRYEMULROOT, UIO_USERSPACE,
	    SCARG(uap, fname));
	error = namei(&nd);
	if (error)
		return (error);
	vp = nd.ni_vp;
	sz = sizeof(struct compat_30_fhandle);
	error = vfs_composefh(vp, (void *)&fh, &sz);
	vput(vp);
	if (sz != FHANDLE_SIZE_COMPAT) {
		error = EINVAL;
	}
	if (error)
		return (error);
	error = copyout(&fh, SCARG(uap, fhp), sizeof(struct compat_30_fhandle));
	return (error);
}

/*
 * Open a file given a file handle.
 *
 * Check permissions, allocate an open file structure,
 * and call the device open routine if any.
 */
int
compat_30_sys_fhopen(struct lwp *l, const struct compat_30_sys_fhopen_args *uap, register_t *retval)
{
	/* {
		syscallarg(const fhandle_t *) fhp;
		syscallarg(int) flags;
	} */

	return dofhopen(l, SCARG(uap, fhp), FHANDLE_SIZE_COMPAT,
	    SCARG(uap, flags), retval);
}

/* ARGSUSED */
int
compat_30_sys___fhstat30(struct lwp *l, const struct compat_30_sys___fhstat30_args *uap_30, register_t *retval)
{
	/* {
		syscallarg(const fhandle_t *) fhp;
		syscallarg(struct stat *) sb;
	} */
	struct sys___fhstat40_args uap;

	SCARG(&uap, fhp) = SCARG(uap_30, fhp);
	SCARG(&uap, fh_size) = FHANDLE_SIZE_COMPAT;
	SCARG(&uap, sb) = SCARG(uap_30, sb);

	return sys___fhstat40(l, &uap, retval);
}

/* ARGSUSED */
int
compat_30_sys_fhstatvfs1(struct lwp *l, const struct compat_30_sys_fhstatvfs1_args *uap_30, register_t *retval)
{
	/* {
		syscallarg(const fhandle_t *) fhp;
		syscallarg(struct statvfs *) buf;
		syscallarg(int)	flags;
	} */
	struct sys___fhstatvfs140_args uap;

	SCARG(&uap, fhp) = SCARG(uap_30, fhp);
	SCARG(&uap, fh_size) = FHANDLE_SIZE_COMPAT;
	SCARG(&uap, buf) = SCARG(uap_30, buf);
	SCARG(&uap, flags) = SCARG(uap_30, flags);

	return sys___fhstatvfs140(l, &uap, retval);
}
