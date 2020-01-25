/*	$NetBSD: vfs_syscalls_50.c,v 1.24.2.1 2020/01/25 22:38:44 ad Exp $	*/

/*-
 * Copyright (c) 2008 The NetBSD Foundation, Inc.
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
__KERNEL_RCSID(0, "$NetBSD: vfs_syscalls_50.c,v 1.24.2.1 2020/01/25 22:38:44 ad Exp $");

#if defined(_KERNEL_OPT)
#include "opt_compat_netbsd.h"
#include "opt_quota.h"
#endif

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
#include <sys/kauth.h>
#include <sys/time.h>
#include <sys/syscall.h>
#include <sys/syscallvar.h>
#include <sys/syscallargs.h>
#include <sys/vfs_syscalls.h>
#ifndef LFS
#define LFS
#endif
#include <sys/syscallargs.h>

#include <ufs/lfs/lfs_extern.h>

#include <compat/common/compat_util.h>
#include <compat/common/compat_mod.h>
#include <compat/sys/time.h>
#include <compat/sys/stat.h>
#include <compat/sys/dirent.h>
#include <compat/sys/mount.h>

static void cvtstat(struct stat30 *, const struct stat *);

static const struct syscall_package vfs_syscalls_50_syscalls[] = {
	{ SYS_compat_50___stat30, 0, (sy_call_t *)compat_50_sys___stat30 },
	{ SYS_compat_50___fstat30, 0, (sy_call_t *)compat_50_sys___fstat30 },
	{ SYS_compat_50___lstat30, 0, (sy_call_t *)compat_50_sys___lstat30 },
	{ SYS_compat_50___fhstat40, 0, (sy_call_t *)compat_50_sys___fhstat40 },
	{ SYS_compat_50_utimes, 0, (sy_call_t *)compat_50_sys_utimes },
	{ SYS_compat_50_lfs_segwait, 0,
	    (sy_call_t *)compat_50_sys_lfs_segwait } ,
	{ SYS_compat_50_futimes, 0, (sy_call_t *)compat_50_sys_futimes },
	{ SYS_compat_50_lutimes, 0, (sy_call_t *)compat_50_sys_lutimes },
	{ SYS_compat_50_mknod, 0, (sy_call_t *)compat_50_sys_mknod },
	{ 0, 0, NULL }
};

/*
 * Convert from a new to an old stat structure.
 */
static void
cvtstat(struct stat30 *ost, const struct stat *st)
{

	ost->st_dev = st->st_dev;
	ost->st_ino = st->st_ino;
	ost->st_mode = st->st_mode;
	ost->st_nlink = st->st_nlink;
	ost->st_uid = st->st_uid;
	ost->st_gid = st->st_gid;
	ost->st_rdev = st->st_rdev;
	timespec_to_timespec50(&st->st_atimespec, &ost->st_atimespec);
	timespec_to_timespec50(&st->st_mtimespec, &ost->st_mtimespec);
	timespec_to_timespec50(&st->st_ctimespec, &ost->st_ctimespec);
	timespec_to_timespec50(&st->st_birthtimespec, &ost->st_birthtimespec);
	ost->st_size = st->st_size;
	ost->st_blocks = st->st_blocks;
	ost->st_blksize = st->st_blksize;
	ost->st_flags = st->st_flags;
	ost->st_gen = st->st_gen;
	memset(ost->st_spare, 0, sizeof(ost->st_spare));
}

/*
 * Get file status; this version follows links.
 */
/* ARGSUSED */
int
compat_50_sys___stat30(struct lwp *l, const struct compat_50_sys___stat30_args *uap, register_t *retval)
{
	/* {
		syscallarg(const char *) path;
		syscallarg(struct stat30 *) ub;
	} */
	struct stat sb;
	struct stat30 osb;
	int error;

	error = do_sys_stat(SCARG(uap, path), FOLLOW, &sb);
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
compat_50_sys___lstat30(struct lwp *l, const struct compat_50_sys___lstat30_args *uap, register_t *retval)
{
	/* {
		syscallarg(const char *) path;
		syscallarg(struct stat30 *) ub;
	} */
	struct stat sb;
	struct stat30 osb;
	int error;

	error = do_sys_stat(SCARG(uap, path), NOFOLLOW, &sb);
	if (error)
		return error;
	cvtstat(&osb, &sb);
	error = copyout(&osb, SCARG(uap, ub), sizeof (osb));
	return error;
}

/*
 * Return status information about a file descriptor.
 */
/* ARGSUSED */
int
compat_50_sys___fstat30(struct lwp *l, const struct compat_50_sys___fstat30_args *uap, register_t *retval)
{
	/* {
		syscallarg(int) fd;
		syscallarg(struct stat30 *) sb;
	} */
	struct stat sb;
	struct stat30 osb;
	int error;

	error = do_sys_fstat(SCARG(uap, fd), &sb);
	if (error)
		return error;
	cvtstat(&osb, &sb);
	error = copyout(&osb, SCARG(uap, sb), sizeof (osb));
	return error;
}

/* ARGSUSED */
int
compat_50_sys___fhstat40(struct lwp *l, const struct compat_50_sys___fhstat40_args *uap, register_t *retval)
{
	/* {
		syscallarg(const void *) fhp;
		syscallarg(size_t) fh_size;
		syscallarg(struct stat30 *) sb;
	} */
	struct stat sb;
	struct stat30 osb;
	int error;

	error = do_fhstat(l, SCARG(uap, fhp), SCARG(uap, fh_size), &sb);
	if (error)
		return error;
	cvtstat(&osb, &sb);
	error = copyout(&osb, SCARG(uap, sb), sizeof (osb));
	return error;
}

static int
compat_50_do_sys_utimes(struct lwp *l, struct vnode *vp, const char *path,
    int flag, const struct timeval50 *tptr)
{
	struct timeval tv[2], *tvp;
	struct timeval50 tv50[2];
	if (tptr) {
		int error = copyin(tptr, tv50, sizeof(tv50));
		if (error)
			return error;
		timeval50_to_timeval(&tv50[0], &tv[0]);
		timeval50_to_timeval(&tv50[1], &tv[1]);
		tvp = tv;
	} else
		tvp = NULL;
	return do_sys_utimes(l, vp, path, flag, tvp, UIO_SYSSPACE);
}
    
/*
 * Set the access and modification times given a path name; this
 * version follows links.
 */
/* ARGSUSED */
int
compat_50_sys_utimes(struct lwp *l, const struct compat_50_sys_utimes_args *uap,
    register_t *retval)
{
	/* {
		syscallarg(const char *) path;
		syscallarg(const struct timeval50 *) tptr;
	} */

	return compat_50_do_sys_utimes(l, NULL, SCARG(uap, path), FOLLOW,
	    SCARG(uap, tptr));
}

/*
 * Set the access and modification times given a file descriptor.
 */
/* ARGSUSED */
int
compat_50_sys_futimes(struct lwp *l,
    const struct compat_50_sys_futimes_args *uap, register_t *retval)
{
	/* {
		syscallarg(int) fd;
		syscallarg(const struct timeval50 *) tptr;
	} */
	int error;
	struct file *fp;

	/* fd_getvnode() will use the descriptor for us */
	if ((error = fd_getvnode(SCARG(uap, fd), &fp)) != 0)
		return error;
	error = compat_50_do_sys_utimes(l, fp->f_vnode, NULL, 0,
	    SCARG(uap, tptr));
	fd_putfile(SCARG(uap, fd));
	return error;
}

/*
 * Set the access and modification times given a path name; this
 * version does not follow links.
 */
int
compat_50_sys_lutimes(struct lwp *l,
    const struct compat_50_sys_lutimes_args *uap, register_t *retval)
{
	/* {
		syscallarg(const char *) path;
		syscallarg(const struct timeval50 *) tptr;
	} */

	return compat_50_do_sys_utimes(l, NULL, SCARG(uap, path), NOFOLLOW,
	    SCARG(uap, tptr));
}

int
compat_50_sys_lfs_segwait(struct lwp *l,
    const struct compat_50_sys_lfs_segwait_args *uap, register_t *retval)
{
	/* {
		syscallarg(fsid_t *) fsidp;
		syscallarg(struct timeval50 *) tv;
	} */
#ifdef notyet
/* XXX need to check presence of LFS at run-time XXX */
	struct timeval atv;
	struct timeval50 atv50;
	fsid_t fsid;
	int error;

	/* XXX need we be su to segwait? */
	error = kauth_authorize_system(l->l_cred, KAUTH_SYSTEM_LFS,
	    KAUTH_REQ_SYSTEM_LFS_SEGWAIT, NULL, NULL, NULL);
	if (error)
		return (error);
	if ((error = copyin(SCARG(uap, fsidp), &fsid, sizeof(fsid_t))) != 0)
		return (error);

	if (SCARG(uap, tv)) {
		error = copyin(SCARG(uap, tv), &atv50, sizeof(atv50));
		if (error)
			return (error);
		timeval50_to_timeval(&atv50, &atv);
		if (itimerfix(&atv))
			return (EINVAL);
	} else /* NULL or invalid */
		atv.tv_sec = atv.tv_usec = 0;
	return lfs_segwait(&fsid, &atv);
#else
	return ENOSYS;
#endif
}

int
compat_50_sys_mknod(struct lwp *l,
    const struct compat_50_sys_mknod_args *uap, register_t *retval)
{
	/* {
		syscallarg(const char *) path;
		syscallarg(mode_t) mode;
		syscallarg(uint32_t) dev;
	} */
	return do_sys_mknod(l, SCARG(uap, path), SCARG(uap, mode),
	    SCARG(uap, dev), UIO_USERSPACE);
}

int             
vfs_syscalls_50_init(void)
{               
        
        return syscall_establish(NULL, vfs_syscalls_50_syscalls);
}       
        
int
vfs_syscalls_50_fini(void)
{               

        return syscall_disestablish(NULL, vfs_syscalls_50_syscalls);
}         
