/*	$NetBSD: ultrix_pathname.c,v 1.36.2.1 2009/07/23 23:31:44 jym Exp $	*/

/*
 * Copyright (c) 1992, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This software was developed by the Computer Systems Engineering group
 * at Lawrence Berkeley Laboratory under DARPA contract BG 91-66 and
 * contributed to Berkeley.
 *
 * All advertising materials mentioning features or use of this software
 * must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Lawrence Berkeley Laboratory.
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
 *
 *	@(#)sun_misc.c	8.1 (Berkeley) 6/18/93
 *
 * from: Header: sun_misc.c,v 1.16 93/04/07 02:46:27 torek Exp
 */


/*
 * Ultrix emulation filesystem-namespace compatibility module.
 *
 * Ultrix system calls that examine the filesysten namespace
 * are implemented here.  Each system call has a wrapper that
 * first checks if the given file exists at a special `emulation'
 * pathname: the given path, prefixex with '/emul/ultrix', and
 * if that pathname exists, it is used instead of the providd pathname.
 *
 * Used to locate OS-specific files (shared libraries, config files,
 * etc) used by emul processes at their `normal' pathnames, without
 * polluting, or conflicting with, the native filesysten namespace.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: ultrix_pathname.c,v 1.36.2.1 2009/07/23 23:31:44 jym Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/namei.h>
#include <sys/file.h>
#include <sys/filedesc.h>
#include <sys/ioctl.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <sys/vnode.h>
#include <sys/syscallargs.h>
#include <sys/proc.h>

#include <compat/ultrix/ultrix_syscallargs.h>
#include <compat/common/compat_util.h>

static int ultrixstatfs(struct statvfs *, void *);

int
ultrix_sys_creat(struct lwp *l, const struct ultrix_sys_creat_args *uap, register_t *retval)
{
	struct sys_open_args ap;

	SCARG(&ap, path) = SCARG(uap, path);
	SCARG(&ap, flags) = O_WRONLY | O_CREAT | O_TRUNC;
	SCARG(&ap, mode) = SCARG(uap, mode);

	return sys_open(l, &ap, retval);
}


int
ultrix_sys_access(struct lwp *l, const struct ultrix_sys_access_args *uap, register_t *retval)
{

	return sys_access(l, (const struct sys_access_args *)uap, retval);
}

int
ultrix_sys_stat(struct lwp *l, const struct ultrix_sys_stat_args *uap, register_t *retval)
{

	return compat_43_sys_stat(l,
	    (const struct compat_43_sys_stat_args *)uap, retval);
}

int
ultrix_sys_lstat(struct lwp *l, const struct ultrix_sys_lstat_args *uap, register_t *retval)
{

	return compat_43_sys_lstat(l,
	    (const struct compat_43_sys_lstat_args *)uap, retval);
}

int
ultrix_sys_execv(struct lwp *l, const struct ultrix_sys_execv_args *uap, register_t *retval)
{
	/* {
		syscallarg(const char *) path;
		syscallarg(char **) argv;
	} */
	struct sys_execve_args ap;

	SCARG(&ap, path) = SCARG(uap, path);
	SCARG(&ap, argp) = SCARG(uap, argp);
	SCARG(&ap, envp) = NULL;

	return sys_execve(l, &ap, retval);
}

int
ultrix_sys_execve(struct lwp *l, const struct ultrix_sys_execve_args *uap, register_t *retval)
{
	/* {
		syscallarg(const char *) path;
		syscallarg(char **) argv;
		syscallarg(char **) envp;
	} */
	struct sys_execve_args ap;

	SCARG(&ap, path) = SCARG(uap, path);
	SCARG(&ap, argp) = SCARG(uap, argp);
	SCARG(&ap, envp) = SCARG(uap, envp);

	return sys_execve(l, &ap, retval);
}

int
ultrix_sys_open(struct lwp *l, const struct ultrix_sys_open_args *uap, register_t *retval)
{
	struct proc *p = l->l_proc;
	int q, r;
	int noctty;
	int ret;
	struct sys_open_args ap;

	/* convert open flags into NetBSD flags */

	q = SCARG(uap, flags);
	noctty = q & 0x8000;
	r =	(q & (0x0001 | 0x0002 | 0x0008 | 0x0040 | 0x0200 | 0x0400 | 0x0800));
	r |=	((q & (0x0004 | 0x1000 | 0x4000)) ? O_NONBLOCK : 0);
	r |=	((q & 0x0080) ? O_SHLOCK : 0);
	r |=	((q & 0x0100) ? O_EXLOCK : 0);
	r |=	((q & 0x2000) ? O_FSYNC : 0);

	SCARG(&ap, path) = SCARG(uap, path);
	SCARG(&ap, flags) = r;
	SCARG(&ap, mode) = SCARG(uap, mode);
	ret = sys_open(l, &ap, retval);

	/* XXXSMP */
	if (!ret && !noctty && SESS_LEADER(p) && !(p->p_lflag & PL_CONTROLT)) {
		file_t *fp;
		int fd;

		fd = (int)*retval;
		fp = fd_getfile(fd);

		/* ignore any error, just give it a try */
		if (fp != NULL) {
			if (fp->f_type == DTYPE_VNODE)
				(fp->f_ops->fo_ioctl)(fp, TIOCSCTTY, NULL);
			fd_putfile(fd);
		}
	}
	return ret;
}


struct ultrix_statfs {
	long	f_type;		/* type of info, zero for now */
	long	f_bsize;	/* fundamental file system block size */
	long	f_blocks;	/* total blocks in file system */
	long	f_bfree;	/* free blocks */
	long	f_bavail;	/* free blocks available to non-super-user */
	long	f_files;	/* total file nodes in file system */
	long	f_ffree;	/* free file nodes in fs */
	fsid_t	f_fsid;		/* file system id */
	long	f_spare[7];	/* spare for later */
};

/*
 * Custruct ultrix statfs result from native.
 * XXX should this be the same as returned by Ultrix getmnt(2)?
 * XXX Ultrix predates DEV_BSIZE.  Is  conversion of disk space from 1k
 *  block units to DEV_BSIZE necessary?
 */
static int
ultrixstatfs(struct statvfs *sp, void *buf)
{
	struct ultrix_statfs ssfs;

	memset(&ssfs, 0, sizeof ssfs);
	ssfs.f_type = 0;
	ssfs.f_bsize = sp->f_bsize;
	ssfs.f_blocks = sp->f_blocks;
	ssfs.f_bfree = sp->f_bfree;
	ssfs.f_bavail = sp->f_bavail;
	ssfs.f_files = sp->f_files;
	ssfs.f_ffree = sp->f_ffree;
	ssfs.f_fsid = sp->f_fsidx;
	return copyout((void *)&ssfs, buf, sizeof ssfs);
}


int
ultrix_sys_statfs(struct lwp *l, const struct ultrix_sys_statfs_args *uap, register_t *retval)
{
	struct mount *mp;
	struct statvfs *sp;
	int error;
	struct vnode *vp;

	error = namei_simple_user(SCARG(uap, path),
				NSM_FOLLOW_TRYEMULROOT, &vp);
	if (error != 0)
		return error;

	mp = vp->v_mount;
	sp = &mp->mnt_stat;
	vrele(vp);
	if ((error = VFS_STATVFS(mp, sp)) != 0)
		return error;
	sp->f_flag = mp->mnt_flag & MNT_VISFLAGMASK;
	return ultrixstatfs(sp, (void *)SCARG(uap, buf));
}

/*
 * sys_fstatfs() takes an fd, not a path, and so needs no emul
 * pathname processing;  but it's similar enough to sys_statvfs() that
 * it goes here anyway.
 */
int
ultrix_sys_fstatfs(struct lwp *l, const struct ultrix_sys_fstatfs_args *uap, register_t *retval)
{
	file_t *fp;
	struct mount *mp;
	struct statvfs *sp;
	int error;

	/* fd_getvnode() will use the descriptor for us */
	if ((error = fd_getvnode(SCARG(uap, fd), &fp)) != 0)
		return error;
	mp = ((struct vnode *)fp->f_data)->v_mount;
	sp = &mp->mnt_stat;
	if ((error = VFS_STATVFS(mp, sp)) != 0)
		goto out;
	sp->f_flag = mp->mnt_flag & MNT_VISFLAGMASK;
	error = ultrixstatfs(sp, (void *)SCARG(uap, buf));
 out:
 	fd_putfile(SCARG(uap, fd));
	return error;
}

int
ultrix_sys_mknod(struct lwp *l, const struct ultrix_sys_mknod_args *uap, register_t *retval)
{

	if (S_ISFIFO(SCARG(uap, mode)))
		return sys_mkfifo(l, (const struct sys_mkfifo_args *)uap,
		    retval);

	return compat_50_sys_mknod(l,
				   (const struct compat_50_sys_mknod_args *)uap,
				   retval);
}
