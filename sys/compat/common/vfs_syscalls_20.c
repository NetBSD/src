/*	$NetBSD: vfs_syscalls_20.c,v 1.46 2020/06/28 14:37:53 christos Exp $	*/

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
 *	@(#)vfs_syscalls.c	8.42 (Berkeley) 7/31/95
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: vfs_syscalls_20.c,v 1.46 2020/06/28 14:37:53 christos Exp $");

#ifdef _KERNEL_OPT
#include "opt_compat_netbsd.h"
#endif

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/namei.h>
#include <sys/filedesc.h>
#include <sys/kernel.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/vnode.h>
#include <sys/mount.h>
#include <sys/proc.h>
#include <sys/uio.h>
#include <sys/dirent.h>
#include <sys/sysctl.h>
#include <sys/syscall.h>
#include <sys/syscallvar.h>
#include <sys/syscallargs.h>
#include <sys/kauth.h>
#include <sys/vfs_syscalls.h>

#include <compat/common/compat_mod.h>

#include <compat/sys/mount.h>
#include <compat/sys/statvfs.h>

static const struct syscall_package vfs_syscalls_20_syscalls[] = {
	{ SYS_compat_20_fhstatfs, 0, (sy_call_t *)compat_20_sys_fhstatfs },
	{ SYS_compat_20_fstatfs, 0, (sy_call_t *)compat_20_sys_fstatfs },
	{ SYS_compat_20_getfsstat, 0, (sy_call_t *)compat_20_sys_getfsstat }, 
	{ SYS_compat_20_statfs, 0, (sy_call_t *)compat_20_sys_statfs },
	{ 0, 0, NULL }
};

/*
 * Get filesystem statistics.
 */
/* ARGSUSED */
int
compat_20_sys_statfs(struct lwp *l, const struct compat_20_sys_statfs_args *uap, register_t *retval)
{
	/* {
		syscallarg(const char *) path;
		syscallarg(struct statfs12 *) buf;
	} */
	struct mount *mp;
	struct statvfs *sbuf;
	int error;
	struct vnode *vp;

	error = namei_simple_user(SCARG(uap, path),
			NSM_FOLLOW_TRYEMULROOT, &vp);
	if (error != 0)
		return error;

	mp = vp->v_mount;

	sbuf = STATVFSBUF_GET();
	if ((error = dostatvfs(mp, sbuf, l, 0, 1)) != 0)
		goto done;

	error = statvfs_to_statfs12_copy(sbuf, SCARG(uap, buf), 0);
done:
	vrele(vp);
	STATVFSBUF_PUT(sbuf);
	return error;
}

/*
 * Get filesystem statistics.
 */
/* ARGSUSED */
int
compat_20_sys_fstatfs(struct lwp *l, const struct compat_20_sys_fstatfs_args *uap, register_t *retval)
{
	/* {
		syscallarg(int) fd;
		syscallarg(struct statfs12 *) buf;
	} */
	struct file *fp;
	struct mount *mp;
	struct statvfs *sbuf;
	int error;

	/* fd_getvnode() will use the descriptor for us */
	if ((error = fd_getvnode(SCARG(uap, fd), &fp)) != 0)
		return (error);
	mp = fp->f_vnode->v_mount;
	sbuf = STATVFSBUF_GET();
	if ((error = dostatvfs(mp, sbuf, l, 0, 1)) != 0)
		goto out;
	error = statvfs_to_statfs12_copy(sbuf, SCARG(uap, buf), 0);
 out:
	fd_putfile(SCARG(uap, fd));
	STATVFSBUF_PUT(sbuf);
	return error;
}


/*
 * Get statistics on all filesystems.
 */
int
compat_20_sys_getfsstat(struct lwp *l, const struct compat_20_sys_getfsstat_args *uap, register_t *retval)
{
	/* {
		syscallarg(struct statfs12 *) buf;
		syscallarg(long) bufsize;
		syscallarg(int) flags;
	} */
	return do_sys_getvfsstat(l, SCARG(uap, buf), SCARG(uap, bufsize),
	    SCARG(uap, flags), statvfs_to_statfs12_copy,
	    sizeof(struct statfs12), retval);
}

int
compat_20_sys_fhstatfs(struct lwp *l, const struct compat_20_sys_fhstatfs_args *uap, register_t *retval)
{
	/* {
		syscallarg(const struct compat_30_fhandle *) fhp;
		syscallarg(struct statfs12 *) buf;
	} */
	struct statvfs *sbuf;
	struct compat_30_fhandle fh;
	struct mount *mp;
	struct vnode *vp;
	int error;

	/*
	 * Must be super user
	 */
	if ((error = kauth_authorize_system(l->l_cred,
	    KAUTH_SYSTEM_FILEHANDLE, 0, NULL, NULL, NULL)))
		return (error);

	if ((error = copyin(SCARG(uap, fhp), &fh, sizeof(fh))) != 0)
		return (error);

	if ((mp = vfs_getvfs(&fh.fh_fsid)) == NULL)
		return (ESTALE);
	error = VFS_FHTOVP(mp, (struct fid*)&fh.fh_fid, LK_EXCLUSIVE, &vp);
	if (error != 0)
		return (error);
	mp = vp->v_mount;
	VOP_UNLOCK(vp);
	sbuf = STATVFSBUF_GET();
	if ((error = VFS_STATVFS(mp, sbuf)) != 0)
		goto out;
	error = statvfs_to_statfs12_copy(sbuf, SCARG(uap, buf), 0);
out:
	vrele(vp);
	STATVFSBUF_PUT(sbuf);
	return error;
}

int
vfs_syscalls_20_init(void)
{

	return syscall_establish(NULL, vfs_syscalls_20_syscalls);
}

int
vfs_syscalls_20_fini(void)
{

	return syscall_disestablish(NULL, vfs_syscalls_20_syscalls);
}
