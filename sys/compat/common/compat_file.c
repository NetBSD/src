/*	$NetBSD: compat_file.c,v 1.2 2003/01/22 17:48:02 christos Exp $ */

/*-
 * Copyright (c) 2002 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Emmanuel Dreyfus.
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
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: compat_file.c,v 1.2 2003/01/22 17:48:02 christos Exp $");

#include "opt_compat_darwin.h"

/* Build this file only if we have an emulation that needs it */
#if (defined(COMPAT_DARWIN)) /* Add COMPAT_FREEBSD and others here */

#if defined(_KERNEL_OPT)
#include "fs_nfs.h"
#endif

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <sys/filedesc.h>
#include <sys/ioctl.h>
#include <sys/kernel.h>
#include <sys/mount.h>
#include <sys/namei.h>
#include <sys/malloc.h>

#include <sys/syscallargs.h>

#include <compat/common/compat_file.h>
#include <compat/common/compat_util.h>

int
bsd_sys_open(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct sys_open_args /* {
		syscallarg(char *) path;
		syscallarg(int) flags;
		syscallarg(int) mode;
	} */ *uap = v;
	struct proc *p = l->l_proc;
	caddr_t sg = stackgap_init(p, 0);

	if (SCARG(uap, flags) & O_CREAT)
		CHECK_ALT_CREAT(p, &sg, SCARG(uap, path));
	else
		CHECK_ALT_EXIST(p, &sg, SCARG(uap, path));
	return sys_open(l, uap, retval);
}

int
bsd_compat_43_sys_creat(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct compat_43_sys_creat_args /* {
		syscallarg(char *) path;
		syscallarg(int) mode;
	} */ *uap = v;
	struct proc *p = l->l_proc;
	caddr_t sg  = stackgap_init(p, 0);

	CHECK_ALT_CREAT(p, &sg, SCARG(uap, path));
	return compat_43_sys_creat(l, uap, retval);
}

int
bsd_sys_link(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct sys_link_args /* {
		syscallarg(char *) path;
		syscallarg(char *) link;
	} */ *uap = v;
	struct proc *p = l->l_proc;
	caddr_t sg = stackgap_init(p, 0);

	CHECK_ALT_EXIST(p, &sg, SCARG(uap, path));
	CHECK_ALT_CREAT(p, &sg, SCARG(uap, link));
	return sys_link(l, uap, retval);
}

int
bsd_sys_unlink(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct sys_unlink_args /* {
		syscallarg(char *) path;
	} */ *uap = v;
	struct proc *p = l->l_proc;
	caddr_t sg = stackgap_init(p, 0);

	CHECK_ALT_EXIST(p, &sg, SCARG(uap, path));
	return sys_unlink(l, uap, retval);
}

int
bsd_sys_chdir(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct sys_chdir_args /* {
		syscallarg(char *) path;
	} */ *uap = v;
	struct proc *p = l->l_proc;
	caddr_t sg = stackgap_init(p, 0);

	CHECK_ALT_EXIST(p, &sg, SCARG(uap, path));
	return sys_chdir(l, uap, retval);
}

int
bsd_sys_mknod(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct sys_mknod_args /* {
		syscallarg(char *) path;
		syscallarg(int) mode;
		syscallarg(int) dev;
	} */ *uap = v;
	struct proc *p = l->l_proc;
	caddr_t sg = stackgap_init(p, 0);

	CHECK_ALT_CREAT(p, &sg, SCARG(uap, path));
	return sys_mknod(l, uap, retval);
}

int
bsd_sys_chmod(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct sys_chmod_args /* {
		syscallarg(char *) path;
		syscallarg(int) mode;
	} */ *uap = v;
	struct proc *p = l->l_proc;
	caddr_t sg = stackgap_init(p, 0);

	CHECK_ALT_EXIST(p, &sg, SCARG(uap, path));
	return sys_chmod(l, uap, retval);
}

int
bsd_sys_chown(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct sys_chown_args /* {
		syscallarg(char *) path;
		syscallarg(int) uid;
		syscallarg(int) gid;
	} */ *uap = v;
	struct proc *p = l->l_proc;
	caddr_t sg = stackgap_init(p, 0);

	CHECK_ALT_EXIST(p, &sg, SCARG(uap, path));
	return sys_chown(l, uap, retval);
}

int
bsd_sys_mount(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct sys_mount_args /* {
		syscallarg(char *) type;
		syscallarg(char *) path;
		syscallarg(int) flags;
		syscallarg(void *) data;
	} */ *uap = v;
	struct proc *p = l->l_proc;
	caddr_t sg = stackgap_init(p, 0);

	CHECK_ALT_EXIST(p, &sg, SCARG(uap, path));
	return sys_mount(l, uap, retval);
}

int
bsd_sys_unmount(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct sys_unmount_args /* {
		syscallarg(char *) path;
		syscallarg(int) flags;
	} */ *uap = v;
	struct proc *p = l->l_proc;
	caddr_t sg = stackgap_init(p, 0);

	CHECK_ALT_EXIST(p, &sg, SCARG(uap, path));
	return sys_unmount(l, uap, retval);
}

int
bsd_sys_access(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct sys_access_args /* {
		syscallarg(char *) path;
		syscallarg(int) flags;
	} */ *uap = v;
	struct proc *p = l->l_proc;
	caddr_t sg = stackgap_init(p, 0);

	CHECK_ALT_EXIST(p, &sg, SCARG(uap, path));
	return sys_access(l, uap, retval);
}

int
bsd_sys_chflags(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct sys_chflags_args /* {
		syscallarg(char *) path;
		syscallarg(int) flags;
	} */ *uap = v;
	struct proc *p = l->l_proc;
	caddr_t sg = stackgap_init(p, 0);

	CHECK_ALT_EXIST(p, &sg, SCARG(uap, path));
	return sys_chflags(l, uap, retval);
}

int
bsd_compat_43_sys_stat(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct compat_43_sys_stat_args /* {
		syscallarg(char *) path;
		syscallarg(struct stat43 *) ub;
	} */ *uap = v;
	struct proc *p = l->l_proc;
	caddr_t sg = stackgap_init(p, 0);

	CHECK_ALT_EXIST(p, &sg, SCARG(uap, path));
	return compat_43_sys_stat(l, uap, retval);
}

int
bsd_compat_43_sys_lstat(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct compat_43_sys_lstat_args /* {
		syscallarg(char *) path;
		syscallarg(struct stat43 *) ub;
	} */ *uap = v;
	struct proc *p = l->l_proc;
	caddr_t sg = stackgap_init(p, 0);

	CHECK_ALT_EXIST(p, &sg, SCARG(uap, path));
	return compat_43_sys_lstat(l, uap, retval);
}

int
bsd_sys_acct(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct sys_acct_args /* {
		syscallarg(char *) path;
	} */ *uap = v;
	struct proc *p = l->l_proc;
	caddr_t sg = stackgap_init(p, 0);

	CHECK_ALT_EXIST(p, &sg, SCARG(uap, path));
	return sys_acct(l, uap, retval);
}

int
bsd_sys_revoke(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct sys_revoke_args /* {
		syscallarg(char *) path;
	} */ *uap = v;
	struct proc *p = l->l_proc;
	caddr_t sg = stackgap_init(p, 0);

	CHECK_ALT_EXIST(p, &sg, SCARG(uap, path));
	return sys_revoke(l, uap, retval);
}

int
bsd_sys_symlink(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct sys_symlink_args /* {
		syscallarg(char *) path;
		syscallarg(char *) link;
	} */ *uap = v;
	struct proc *p = l->l_proc;
	caddr_t sg = stackgap_init(p, 0);

	CHECK_ALT_EXIST(p, &sg, SCARG(uap, path));
	CHECK_ALT_CREAT(p, &sg, SCARG(uap, link));
	return sys_symlink(l, uap, retval);
}

int
bsd_sys_readlink(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct sys_readlink_args /* {
		syscallarg(char *) path;
		syscallarg(char *) buf;
		syscallarg(int) count;
	} */ *uap = v;
	struct proc *p = l->l_proc;
	caddr_t sg = stackgap_init(p, 0);

	CHECK_ALT_SYMLINK(p, &sg, SCARG(uap, path));
	return sys_readlink(l, uap, retval);
}

int
bsd_sys_execve(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct sys_execve_args /* {
		syscallarg(char *) path;
		syscallarg(char **) argp;
		syscallarg(char **) envp;
	} */ *uap = v;
	struct sys_execve_args ap;
	struct proc *p = l->l_proc;
	caddr_t sg;

	sg = stackgap_init(p, 0);
	CHECK_ALT_EXIST(p, &sg, SCARG(uap, path));

	SCARG(&ap, path) = SCARG(uap, path);
	SCARG(&ap, argp) = SCARG(uap, argp);
	SCARG(&ap, envp) = SCARG(uap, envp);

	return sys_execve(l, &ap, retval);
}

int
bsd_sys_chroot(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct sys_chroot_args /* {
		syscallarg(char *) path;
	} */ *uap = v;
	struct proc *p = l->l_proc;
	caddr_t sg = stackgap_init(p, 0);

	CHECK_ALT_EXIST(p, &sg, SCARG(uap, path));
	return sys_chroot(l, uap, retval);
}

int
bsd_compat_12_sys_swapon(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct compat_12_sys_swapon_args /* {
		syscallarg(char *) name;
	} */ *uap = v;
	struct proc *p = l->l_proc;
	caddr_t sg = stackgap_init(p, 0);

	CHECK_ALT_EXIST(p, &sg, SCARG(uap, name));
	return compat_12_sys_swapon(l, uap, retval);
}

int
bsd_sys_rename(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct sys_rename_args /* {
		syscallarg(char *) from;
		syscallarg(char *) to;
	} */ *uap = v;
	struct proc *p = l->l_proc;
	caddr_t sg = stackgap_init(p, 0);

	CHECK_ALT_EXIST(p, &sg, SCARG(uap, from));
	CHECK_ALT_CREAT(p, &sg, SCARG(uap, to));
	return sys_rename(l, uap, retval);
}

int
bsd_compat_43_sys_truncate(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct compat_43_sys_truncate_args /* {
		syscallarg(char *) path;
		syscallarg(long) length;
	} */ *uap = v;
	struct proc *p = l->l_proc;
	caddr_t sg = stackgap_init(p, 0);

	CHECK_ALT_EXIST(p, &sg, SCARG(uap, path));
	return compat_43_sys_truncate(l, uap, retval);
}

int
bsd_sys_mkfifo(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct sys_mkfifo_args /* {
		syscallarg(char *) path;
		syscallarg(int) mode;
	} */ *uap = v;
	struct proc *p = l->l_proc;
	caddr_t sg = stackgap_init(p, 0);

	CHECK_ALT_CREAT(p, &sg, SCARG(uap, path));
	return sys_mkfifo(l, uap, retval);
}

int
bsd_sys_mkdir(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct sys_mkdir_args /* {
		syscallarg(char *) path;
		syscallarg(int) mode;
	} */ *uap = v;
	struct proc *p = l->l_proc;
	caddr_t sg = stackgap_init(p, 0);

	CHECK_ALT_CREAT(p, &sg, SCARG(uap, path));
	return sys_mkdir(l, uap, retval);
}

int
bsd_sys_rmdir(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct sys_rmdir_args /* {
		syscallarg(char *) path;
	} */ *uap = v;
	struct proc *p = l->l_proc;
	caddr_t sg = stackgap_init(p, 0);

	CHECK_ALT_EXIST(p, &sg, SCARG(uap, path));
	return sys_rmdir(l, uap, retval);
}

int
bsd_sys_utimes(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct sys_utimes_args /* {
		syscallarg(char *) path;
		syscallarg(struct timeval *) tptr;
	} */ *uap = v;
	struct proc *p = l->l_proc;
	caddr_t sg = stackgap_init(p, 0);

	CHECK_ALT_EXIST(p, &sg, SCARG(uap, path));
	return sys_utimes(l, uap, retval);
}

int
bsd_sys_quotactl(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct sys_quotactl_args /* {
		syscallarg(char *) path;
		syscallarg(int) cmd;
		syscallarg(int) uid;
		syscallarg(caddr_t) arg;
		struct proc *p = l->l_proc;
	} */ *uap = v;
	struct proc *p = l->l_proc;
	caddr_t sg = stackgap_init(p, 0);

	CHECK_ALT_EXIST(p, &sg, SCARG(uap, path));
	return sys_quotactl(l, uap, retval);
}

int
bsd_sys_statfs(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct sys_statfs_args /* {
		syscallarg(char *) path;
		syscallarg(struct statfs *) buf;
	} */ *uap = v;
	struct proc *p = l->l_proc;
	caddr_t sg = stackgap_init(p, 0);

	CHECK_ALT_EXIST(p, &sg, SCARG(uap, path));
	return sys_statfs(l, uap, retval);
}

#ifdef NFS
int
bsd_sys_getfh(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct sys_getfh_args /* {
		syscallarg(char *) fname;
		syscallarg(fhandle_t *) fhp;
	} */ *uap = v;
	struct proc *p = l->l_proc;
	caddr_t sg = stackgap_init(p, 0);

	CHECK_ALT_EXIST(p, &sg, SCARG(uap, fname));
	return sys_getfh(l, uap, retval);
}
#endif /* NFS */

int
bsd_compat_12_sys_stat(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct compat_12_sys_stat_args /* {
		syscallarg(char *) path;
		syscallarg(struct stat12 *) ub;
	} */ *uap = v;
	struct proc *p = l->l_proc;
	caddr_t sg = stackgap_init(p, 0);

	CHECK_ALT_EXIST(p, &sg, SCARG(uap, path));
	return compat_12_sys_stat(l, uap, retval);
}

int
bsd_compat_12_sys_lstat(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct compat_12_sys_lstat_args /* {
		syscallarg(char *) path;
		syscallarg(struct stat12 *) ub;
	} */ *uap = v;
	struct proc *p = l->l_proc;
	caddr_t sg = stackgap_init(p, 0);

	CHECK_ALT_EXIST(p, &sg, SCARG(uap, path));
	return compat_12_sys_lstat(l, uap, retval);
}

int
bsd_sys_pathconf(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct sys_pathconf_args /* {
		syscallarg(char *) path;
		syscallarg(int) name;
	} */ *uap = v;
	struct proc *p = l->l_proc;
	caddr_t sg = stackgap_init(p, 0);

	CHECK_ALT_EXIST(p, &sg, SCARG(uap, path));
	return sys_pathconf(l, uap, retval);
}

int
bsd_sys_truncate(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct sys_truncate_args /* {
		syscallarg(char *) path;
		syscallarg(int) pad;
		syscallarg(off_t) length;
	} */ *uap = v;
	struct proc *p = l->l_proc;
	caddr_t sg = stackgap_init(p, 0);

	CHECK_ALT_EXIST(p, &sg, SCARG(uap, path));
	return sys_truncate(l, uap, retval);
}

int
bsd_sys_undelete(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct sys_undelete_args /* {
		syscallarg(char *) path;
	} */ *uap = v;
	struct proc *p = l->l_proc;
	caddr_t sg = stackgap_init(p, 0);

	CHECK_ALT_EXIST(p, &sg, SCARG(uap, path));
	return sys_undelete(l, uap, retval);
}

int
bsd_sys_lchmod(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct sys_lchmod_args /* {
		syscallarg(char *) path;
		syscallarg(mode_t) mode;
	} */ *uap = v;
	struct proc *p = l->l_proc;
	caddr_t sg = stackgap_init(p, 0);

	CHECK_ALT_SYMLINK(p, &sg, SCARG(uap, path));
	return sys_lchmod(l, uap, retval);
}

int
bsd_sys_lchown(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct sys_lchown_args /* {
		syscallarg(char *) path;
		syscallarg(int) uid;
		syscallarg(int) gid;
	} */ *uap = v;
	struct proc *p = l->l_proc;
	caddr_t sg = stackgap_init(p, 0);

	CHECK_ALT_SYMLINK(p, &sg, SCARG(uap, path));
	return sys_lchown(l, uap, retval);
}

int
bsd_sys_lutimes(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct sys_lutimes_args /* {
		syscallarg(char *) path;
		syscallarg(struct timeval *) tptr;
	} */ *uap = v;
	struct proc *p = l->l_proc;
	caddr_t sg = stackgap_init(p, 0);

	CHECK_ALT_EXIST(p, &sg, SCARG(uap, path));
	return sys_lutimes(l, uap, retval);
}

int
bsd_sys___stat13(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct sys___stat13_args /* {
		syscallarg(char *) path;
		syscallarg(struct stat *) ub;
	} */ *uap = v;
	struct proc *p = l->l_proc;
	caddr_t sg = stackgap_init(p, 0);

	CHECK_ALT_EXIST(p, &sg, SCARG(uap, path));
	return sys___stat13(l, uap, retval);
}

int
bsd_sys___lstat13(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct sys___lstat13_args /* {
		syscallarg(char *) path;
		syscallarg(struct stat *) ub;
	} */ *uap = v;
	struct proc *p = l->l_proc;
	caddr_t sg = stackgap_init(p, 0);

	CHECK_ALT_EXIST(p, &sg, SCARG(uap, path));
	return sys___lstat13(l, uap, retval);
}

int
bsd_sys___posix_chown(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct sys___posix_chown_args /* {
		syscallarg(char *) path;
		syscallarg(int) uid;
		syscallarg(int) gid;
	} */ *uap = v;
	struct proc *p = l->l_proc;
	caddr_t sg = stackgap_init(p, 0);

	CHECK_ALT_EXIST(p, &sg, SCARG(uap, path));
	return sys___posix_chown(l, uap, retval);
}

int
bsd_sys___posix_lchown(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct sys___posix_lchown_args /* {
		syscallarg(char *) path;
		syscallarg(int) uid;
		syscallarg(int) gid;
	} */ *uap = v;
	struct proc *p = l->l_proc;
	caddr_t sg = stackgap_init(p, 0);

	CHECK_ALT_EXIST(p, &sg, SCARG(uap, path));
	return sys___posix_lchown(l, uap, retval);
}

int
bsd_sys_lchflags(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct sys_lchflags_args /* {
		syscallarg(char *) path;
		syscallarg(int) flags;
	} */ *uap = v;
	struct proc *p = l->l_proc;
	caddr_t sg = stackgap_init(p, 0);

	CHECK_ALT_EXIST(p, &sg, SCARG(uap, path));
	return sys_lchflags(l, uap, retval);
}

#endif /* defined(COMPAT_DARWIN) */
