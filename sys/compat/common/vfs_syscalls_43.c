/*	$NetBSD: vfs_syscalls_43.c,v 1.37.2.1 2007/05/13 17:36:20 ad Exp $	*/

/*
 * Copyright (c) 1989, 1993
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
 *	@(#)vfs_syscalls.c	8.28 (Berkeley) 12/10/94
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: vfs_syscalls_43.c,v 1.37.2.1 2007/05/13 17:36:20 ad Exp $");

#if defined(_KERNEL_OPT)
#include "fs_union.h"
#endif

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/filedesc.h>
#include <sys/kernel.h>
#include <sys/proc.h>
#include <sys/file.h>
#include <sys/vnode.h>
#include <sys/namei.h>
#include <sys/dirent.h>
#include <sys/socket.h>
#include <sys/socketvar.h>
#include <sys/stat.h>
#include <sys/malloc.h>
#include <sys/ioctl.h>
#include <sys/fcntl.h>
#include <sys/syslog.h>
#include <sys/unistd.h>
#include <sys/resourcevar.h>
#include <sys/sysctl.h>

#include <sys/mount.h>
#include <sys/syscallargs.h>
#include <sys/vfs_syscalls.h>

#include <compat/sys/stat.h>
#include <compat/sys/mount.h>

static void cvtstat __P((struct stat *, struct stat43 *));

/*
 * Convert from an old to a new stat structure.
 */
static void
cvtstat(st, ost)
	struct stat *st;
	struct stat43 *ost;
{

	ost->st_dev = st->st_dev;
	ost->st_ino = st->st_ino;
	ost->st_mode = st->st_mode & 0xffff;
	ost->st_nlink = st->st_nlink;
	ost->st_uid = st->st_uid;
	ost->st_gid = st->st_gid;
	ost->st_rdev = st->st_rdev;
	if (st->st_size < (quad_t)1 << 32)
		ost->st_size = st->st_size;
	else
		ost->st_size = -2;
	ost->st_atime = st->st_atime;
	ost->st_mtime = st->st_mtime;
	ost->st_ctime = st->st_ctime;
	ost->st_blksize = st->st_blksize;
	ost->st_blocks = st->st_blocks;
	ost->st_flags = st->st_flags;
	ost->st_gen = st->st_gen;
}

/*
 * Get file status; this version follows links.
 */
/* ARGSUSED */
int
compat_43_sys_stat(struct lwp *l, void *v, register_t *retval)
{
	struct compat_43_sys_stat_args /* {
		syscallarg(char *) path;
		syscallarg(struct stat43 *) ub;
	} */ *uap = v;
	struct stat sb;
	struct stat43 osb;
	int error;

	error = do_sys_stat(l, SCARG(uap, path), FOLLOW, &sb);
	if (error)
		return (error);
	cvtstat(&sb, &osb);
	error = copyout((void *)&osb, (void *)SCARG(uap, ub), sizeof (osb));
	return (error);
}

/*
 * Get file status; this version does not follow links.
 */
/* ARGSUSED */
int
compat_43_sys_lstat(struct lwp *l, void *v, register_t *retval)
{
	struct compat_43_sys_lstat_args /* {
		syscallarg(char *) path;
		syscallarg(struct ostat *) ub;
	} */ *uap = v;
	struct vnode *vp, *dvp;
	struct stat sb, sb1;
	struct stat43 osb;
	int error;
	struct nameidata nd;
	int ndflags;

	ndflags = NOFOLLOW | LOCKLEAF | LOCKPARENT;
again:
	NDINIT(&nd, LOOKUP, ndflags, UIO_USERSPACE, SCARG(uap, path), l);
	if ((error = namei(&nd))) {
		if (error == EISDIR && (ndflags & LOCKPARENT) != 0) {
			/*
			 * Should only happen on '/'. Retry without LOCKPARENT;
			 * this is safe since the vnode won't be a VLNK.
			 */
			ndflags &= ~LOCKPARENT;
			goto again;
		}
		return (error);
	}
	/*
	 * For symbolic links, always return the attributes of its
	 * containing directory, except for mode, size, and links.
	 */
	vp = nd.ni_vp;
	dvp = nd.ni_dvp;
	if (vp->v_type != VLNK) {
		if ((ndflags & LOCKPARENT) != 0) {
			if (dvp == vp)
				vrele(dvp);
			else
				vput(dvp);
		}
		error = vn_stat(vp, &sb, l);
		vput(vp);
		if (error)
			return (error);
	} else {
		error = vn_stat(dvp, &sb, l);
		vput(dvp);
		if (error) {
			vput(vp);
			return (error);
		}
		error = vn_stat(vp, &sb1, l);
		vput(vp);
		if (error)
			return (error);
		sb.st_mode &= ~S_IFDIR;
		sb.st_mode |= S_IFLNK;
		sb.st_nlink = sb1.st_nlink;
		sb.st_size = sb1.st_size;
		sb.st_blocks = sb1.st_blocks;
	}
	cvtstat(&sb, &osb);
	error = copyout((void *)&osb, (void *)SCARG(uap, ub), sizeof (osb));
	return (error);
}

/*
 * Return status information about a file descriptor.
 */
/* ARGSUSED */
int
compat_43_sys_fstat(struct lwp *l, void *v, register_t *retval)
{
	struct compat_43_sys_fstat_args /* {
		syscallarg(int) fd;
		syscallarg(struct stat43 *) sb;
	} */ *uap = v;
	struct proc *p = l->l_proc;
	int fd = SCARG(uap, fd);
	struct filedesc *fdp = p->p_fd;
	struct file *fp;
	struct stat ub;
	struct stat43 oub;
	int error;

	if ((fp = fd_getfile(fdp, fd)) == NULL)
		return (EBADF);

	FILE_USE(fp);
	error = (*fp->f_ops->fo_stat)(fp, &ub, l);
	FILE_UNUSE(fp, l);

	if (error == 0) {
		cvtstat(&ub, &oub);
		error = copyout((void *)&oub, (void *)SCARG(uap, sb),
		    sizeof (oub));
	}


	return (error);
}


/*
 * Truncate a file given a file descriptor.
 */
/* ARGSUSED */
int
compat_43_sys_ftruncate(struct lwp *l, void *v, register_t *retval)
{
	struct compat_43_sys_ftruncate_args /* {
		syscallarg(int) fd;
		syscallarg(long) length;
	} */ *uap = v;
	struct sys_ftruncate_args /* {
		syscallarg(int) fd;
		syscallarg(int) pad;
		syscallarg(off_t) length;
	} */ nuap;

	SCARG(&nuap, fd) = SCARG(uap, fd);
	SCARG(&nuap, length) = SCARG(uap, length);
	return (sys_ftruncate(l, &nuap, retval));
}

/*
 * Truncate a file given its path name.
 */
/* ARGSUSED */
int
compat_43_sys_truncate(struct lwp *l, void *v, register_t *retval)
{
	struct compat_43_sys_truncate_args /* {
		syscallarg(char *) path;
		syscallarg(long) length;
	} */ *uap = v;
	struct sys_truncate_args /* {
		syscallarg(char *) path;
		syscallarg(int) pad;
		syscallarg(off_t) length;
	} */ nuap;

	SCARG(&nuap, path) = SCARG(uap, path);
	SCARG(&nuap, length) = SCARG(uap, length);
	return (sys_truncate(l, &nuap, retval));
}


/*
 * Reposition read/write file offset.
 */
int
compat_43_sys_lseek(struct lwp *l, void *v, register_t *retval)
{
	struct compat_43_sys_lseek_args /* {
		syscallarg(int) fd;
		syscallarg(long) offset;
		syscallarg(int) whence;
	} */ *uap = v;
	struct sys_lseek_args /* {
		syscallarg(int) fd;
		syscallarg(int) pad;
		syscallarg(off_t) offset;
		syscallarg(int) whence;
	} */ nuap;
	off_t qret;
	int error;

	SCARG(&nuap, fd) = SCARG(uap, fd);
	SCARG(&nuap, offset) = SCARG(uap, offset);
	SCARG(&nuap, whence) = SCARG(uap, whence);
	error = sys_lseek(l, &nuap, (void *)&qret);
	*(long *)retval = qret;
	return (error);
}


/*
 * Create a file.
 */
int
compat_43_sys_creat(struct lwp *l, void *v, register_t *retval)
{
	struct compat_43_sys_creat_args /* {
		syscallarg(char *) path;
		syscallarg(int) mode;
	} */ *uap = v;
	struct sys_open_args /* {
		syscallarg(char *) path;
		syscallarg(int) flags;
		syscallarg(int) mode;
	} */ nuap;

	SCARG(&nuap, path) = SCARG(uap, path);
	SCARG(&nuap, mode) = SCARG(uap, mode);
	SCARG(&nuap, flags) = O_WRONLY | O_CREAT | O_TRUNC;
	return (sys_open(l, &nuap, retval));
}

/*ARGSUSED*/
int
compat_43_sys_quota(struct lwp *l, void *v,
    register_t *retval)
{

	return (ENOSYS);
}


/*
 * Read a block of directory entries in a file system independent format.
 */
int
compat_43_sys_getdirentries(struct lwp *l, void *v, register_t *retval)
{
	struct compat_43_sys_getdirentries_args /* {
		syscallarg(int) fd;
		syscallarg(char *) buf;
		syscallarg(u_int) count;
		syscallarg(long *) basep;
	} */ *uap = v;
	struct proc *p = l->l_proc;
	struct vnode *vp;
	struct file *fp;
	struct uio auio, kuio;
	struct iovec aiov, kiov;
	struct dirent *dp, *edp;
	char *dirbuf;
	size_t count = min(MAXBSIZE, (size_t)SCARG(uap, count));

	int error, eofflag, readcnt;
	long loff;

	/* getvnode() will use the descriptor for us */
	if ((error = getvnode(p->p_fd, SCARG(uap, fd), &fp)) != 0)
		return (error);
	if ((fp->f_flag & FREAD) == 0) {
		error = EBADF;
		goto out;
	}
	vp = (struct vnode *)fp->f_data;
unionread:
	if (vp->v_type != VDIR) {
		error = EINVAL;
		goto out;
	}
	aiov.iov_base = SCARG(uap, buf);
	aiov.iov_len = count;
	auio.uio_iov = &aiov;
	auio.uio_iovcnt = 1;
	auio.uio_rw = UIO_READ;
	auio.uio_resid = count;
	KASSERT(l == curlwp);
	auio.uio_vmspace = l->l_proc->p_vmspace;
	vn_lock(vp, LK_EXCLUSIVE | LK_RETRY);
	loff = auio.uio_offset = fp->f_offset;
#	if (BYTE_ORDER != LITTLE_ENDIAN)
		if ((vp->v_mount->mnt_iflag & IMNT_DTYPE) == 0) {
			error = VOP_READDIR(vp, &auio, fp->f_cred, &eofflag,
			    (off_t **)0, (int *)0);
			fp->f_offset = auio.uio_offset;
		} else
#	endif
	{
		kuio = auio;
		kuio.uio_iov = &kiov;
		kiov.iov_len = count;
		dirbuf = malloc(count, M_TEMP, M_WAITOK);
		kiov.iov_base = dirbuf;
		UIO_SETUP_SYSSPACE(&kuio);
		error = VOP_READDIR(vp, &kuio, fp->f_cred, &eofflag,
			    (off_t **)0, (int *)0);
		fp->f_offset = kuio.uio_offset;
		if (error == 0) {
			readcnt = count - kuio.uio_resid;
			edp = (struct dirent *)&dirbuf[readcnt];
			for (dp = (struct dirent *)dirbuf; dp < edp; ) {
#				if (BYTE_ORDER == LITTLE_ENDIAN)
					/*
					 * The expected low byte of
					 * dp->d_namlen is our dp->d_type.
					 * The high MBZ byte of dp->d_namlen
					 * is our dp->d_namlen.
					 */
					dp->d_type = dp->d_namlen;
					dp->d_namlen = 0;
#				else
					/*
					 * The dp->d_type is the high byte
					 * of the expected dp->d_namlen,
					 * so must be zero'ed.
					 */
					dp->d_type = 0;
#				endif
				if (dp->d_reclen > 0) {
					dp = (struct dirent *)
					    ((char *)dp + dp->d_reclen);
				} else {
					error = EIO;
					break;
				}
			}
			if (dp >= edp)
				error = uiomove(dirbuf, readcnt, &auio);
		}
		free(dirbuf, M_TEMP);
	}
	VOP_UNLOCK(vp, 0);
	if (error)
		goto out;

#ifdef UNION
{
	extern int (**union_vnodeop_p) __P((void *));
	extern struct vnode *union_dircache __P((struct vnode *));

	if ((count == auio.uio_resid) &&
	    (vp->v_op == union_vnodeop_p)) {
		struct vnode *lvp;

		lvp = union_dircache(vp);
		if (lvp != NULLVP) {
			struct vattr va;

			/*
			 * If the directory is opaque,
			 * then don't show lower entries
			 */
			error = VOP_GETATTR(vp, &va, fp->f_cred, l);
			if (va.va_flags & OPAQUE) {
				vput(lvp);
				lvp = NULL;
			}
		}

		if (lvp != NULLVP) {
			error = VOP_OPEN(lvp, FREAD, fp->f_cred, l);
			VOP_UNLOCK(lvp, 0);

			if (error) {
				vrele(lvp);
				goto out;
			}
			fp->f_data = (void *) lvp;
			fp->f_offset = 0;
			error = vn_close(vp, FREAD, fp->f_cred, l);
			if (error)
				goto out;
			vp = lvp;
			goto unionread;
		}
	}
}
#endif /* UNION */

	if ((count == auio.uio_resid) &&
	    (vp->v_flag & VROOT) &&
	    (vp->v_mount->mnt_flag & MNT_UNION)) {
		struct vnode *tvp = vp;
		vp = vp->v_mount->mnt_vnodecovered;
		VREF(vp);
		fp->f_data = (void *) vp;
		fp->f_offset = 0;
		vrele(tvp);
		goto unionread;
	}
	error = copyout((void *)&loff, (void *)SCARG(uap, basep),
	    sizeof(long));
	*retval = count - auio.uio_resid;
 out:
	FILE_UNUSE(fp, l);
	return (error);
}

/*
 * sysctl helper routine for vfs.generic.conf lookups.
 */
#if defined(COMPAT_09) || defined(COMPAT_43) || defined(COMPAT_44)
static int
sysctl_vfs_generic_conf(SYSCTLFN_ARGS)
{
        struct vfsconf vfc;
        extern const char * const mountcompatnames[];
        extern int nmountcompatnames;
	struct sysctlnode node;
	struct vfsops *vfsp;
	u_int vfsnum;

	if (namelen != 1)
		return (ENOTDIR);
	vfsnum = name[0];
	if (vfsnum >= nmountcompatnames ||
	    mountcompatnames[vfsnum] == NULL)
		return (EOPNOTSUPP);
	vfsp = vfs_getopsbyname(mountcompatnames[vfsnum]);
	if (vfsp == NULL)
		return (EOPNOTSUPP);

	vfc.vfc_vfsops = vfsp;
	strncpy(vfc.vfc_name, vfsp->vfs_name, MFSNAMELEN);
	vfc.vfc_typenum = vfsnum;
	vfc.vfc_refcount = vfsp->vfs_refcount;
	vfc.vfc_flags = 0;
	vfc.vfc_mountroot = vfsp->vfs_mountroot;
	vfc.vfc_next = NULL;
	vfs_delref(vfsp);

	node = *rnode;
	node.sysctl_data = &vfc;
	return (sysctl_lookup(SYSCTLFN_CALL(&node)));
}

/*
 * Top level filesystem related information gathering.
 */
SYSCTL_SETUP(compat_sysctl_vfs_setup, "compat sysctl vfs subtree setup")
{
	extern int nmountcompatnames;

	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT|CTLFLAG_IMMEDIATE,
		       CTLTYPE_INT, "maxtypenum",
		       SYSCTL_DESCR("Highest valid filesystem type number"),
		       NULL, nmountcompatnames, NULL, 0,
		       CTL_VFS, VFS_GENERIC, VFS_MAXTYPENUM, CTL_EOL);
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_STRUCT, "conf",
		       SYSCTL_DESCR("Filesystem configuration information"),
		       sysctl_vfs_generic_conf, 0, NULL,
		       sizeof(struct vfsconf),
		       CTL_VFS, VFS_GENERIC, VFS_CONF, CTL_EOL);
}
#endif
