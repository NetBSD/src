/*	$NetBSD: compat_file.c,v 1.1.2.2 2002/12/11 06:37:03 thorpej Exp $ */

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
__KERNEL_RCSID(0, "$NetBSD: compat_file.c,v 1.1.2.2 2002/12/11 06:37:03 thorpej Exp $");

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
bsd_sys_open(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct sys_open_args /* {
		syscallarg(char *) path;
		syscallarg(int) flags;
		syscallarg(int) mode;
	} */ *uap = v;
	caddr_t sg = stackgap_init(p, 0);

	if (SCARG(uap, flags) & O_CREAT)
		CHECK_ALT_CREAT(p, &sg, SCARG(uap, path));
	else
		CHECK_ALT_EXIST(p, &sg, SCARG(uap, path));
	return sys_open(p, uap, retval);
}

int
bsd_compat_43_sys_creat(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct compat_43_sys_creat_args /* {
		syscallarg(char *) path;
		syscallarg(int) mode;
	} */ *uap = v;
	caddr_t sg  = stackgap_init(p, 0);

	CHECK_ALT_CREAT(p, &sg, SCARG(uap, path));
	return compat_43_sys_creat(p, uap, retval);
}

int
bsd_sys_link(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct sys_link_args /* {
		syscallarg(char *) path;
		syscallarg(char *) link;
	} */ *uap = v;
	caddr_t sg = stackgap_init(p, 0);

	CHECK_ALT_EXIST(p, &sg, SCARG(uap, path));
	CHECK_ALT_CREAT(p, &sg, SCARG(uap, link));
	return sys_link(p, uap, retval);
}

int
bsd_sys_unlink(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct sys_unlink_args /* {
		syscallarg(char *) path;
	} */ *uap = v;
	caddr_t sg = stackgap_init(p, 0);

	CHECK_ALT_EXIST(p, &sg, SCARG(uap, path));
	return sys_unlink(p, uap, retval);
}

int
bsd_sys_chdir(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct sys_chdir_args /* {
		syscallarg(char *) path;
	} */ *uap = v;
	caddr_t sg = stackgap_init(p, 0);

	CHECK_ALT_EXIST(p, &sg, SCARG(uap, path));
	return sys_chdir(p, uap, retval);
}

int
bsd_sys_mknod(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct sys_mknod_args /* {
		syscallarg(char *) path;
		syscallarg(int) mode;
		syscallarg(int) dev;
	} */ *uap = v;
	caddr_t sg = stackgap_init(p, 0);

	CHECK_ALT_CREAT(p, &sg, SCARG(uap, path));
	return sys_mknod(p, uap, retval);
}

int
bsd_sys_chmod(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct sys_chmod_args /* {
		syscallarg(char *) path;
		syscallarg(int) mode;
	} */ *uap = v;
	caddr_t sg = stackgap_init(p, 0);

	CHECK_ALT_EXIST(p, &sg, SCARG(uap, path));
	return sys_chmod(p, uap, retval);
}

int
bsd_sys_chown(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct sys_chown_args /* {
		syscallarg(char *) path;
		syscallarg(int) uid;
		syscallarg(int) gid;
	} */ *uap = v;
	caddr_t sg = stackgap_init(p, 0);

	CHECK_ALT_EXIST(p, &sg, SCARG(uap, path));
	return sys_chown(p, uap, retval);
}

int
bsd_sys_mount(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct sys_mount_args /* {
		syscallarg(char *) type;
		syscallarg(char *) path;
		syscallarg(int) flags;
		syscallarg(void *) data;
	} */ *uap = v;
	caddr_t sg = stackgap_init(p, 0);

	CHECK_ALT_EXIST(p, &sg, SCARG(uap, path));
	return sys_mount(p, uap, retval);
}

int
bsd_sys_unmount(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct sys_unmount_args /* {
		syscallarg(char *) path;
		syscallarg(int) flags;
	} */ *uap = v;
	caddr_t sg = stackgap_init(p, 0);

	CHECK_ALT_EXIST(p, &sg, SCARG(uap, path));
	return sys_unmount(p, uap, retval);
}

int
bsd_sys_access(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct sys_access_args /* {
		syscallarg(char *) path;
		syscallarg(int) flags;
	} */ *uap = v;
	caddr_t sg = stackgap_init(p, 0);

	CHECK_ALT_EXIST(p, &sg, SCARG(uap, path));
	return sys_access(p, uap, retval);
}

int
bsd_sys_chflags(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct sys_chflags_args /* {
		syscallarg(char *) path;
		syscallarg(int) flags;
	} */ *uap = v;
	caddr_t sg = stackgap_init(p, 0);

	CHECK_ALT_EXIST(p, &sg, SCARG(uap, path));
	return sys_chflags(p, uap, retval);
}

int
bsd_compat_43_sys_stat(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct compat_43_sys_stat_args /* {
		syscallarg(char *) path;
		syscallarg(struct stat43 *) ub;
	} */ *uap = v;
	caddr_t sg = stackgap_init(p, 0);

	CHECK_ALT_EXIST(p, &sg, SCARG(uap, path));
	return compat_43_sys_stat(p, uap, retval);
}

int
bsd_compat_43_sys_lstat(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct compat_43_sys_lstat_args /* {
		syscallarg(char *) path;
		syscallarg(struct stat43 *) ub;
	} */ *uap = v;
	caddr_t sg = stackgap_init(p, 0);

	CHECK_ALT_EXIST(p, &sg, SCARG(uap, path));
	return compat_43_sys_lstat(p, uap, retval);
}

int
bsd_sys_acct(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct sys_acct_args /* {
		syscallarg(char *) path;
	} */ *uap = v;
	caddr_t sg = stackgap_init(p, 0);

	CHECK_ALT_EXIST(p, &sg, SCARG(uap, path));
	return sys_acct(p, uap, retval);
}

int
bsd_sys_revoke(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct sys_revoke_args /* {
		syscallarg(char *) path;
	} */ *uap = v;
	caddr_t sg = stackgap_init(p, 0);

	CHECK_ALT_EXIST(p, &sg, SCARG(uap, path));
	return sys_revoke(p, uap, retval);
}

int
bsd_sys_symlink(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct sys_symlink_args /* {
		syscallarg(char *) path;
		syscallarg(char *) link;
	} */ *uap = v;
	caddr_t sg = stackgap_init(p, 0);

	CHECK_ALT_EXIST(p, &sg, SCARG(uap, path));
	CHECK_ALT_CREAT(p, &sg, SCARG(uap, link));
	return sys_symlink(p, uap, retval);
}

int
bsd_sys_readlink(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct sys_readlink_args /* {
		syscallarg(char *) path;
		syscallarg(char *) buf;
		syscallarg(int) count;
	} */ *uap = v;
	caddr_t sg = stackgap_init(p, 0);

	CHECK_ALT_SYMLINK(p, &sg, SCARG(uap, path));
	return sys_readlink(p, uap, retval);
}

int
bsd_sys_execve(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct sys_execve_args /* {
		syscallarg(char *) path;
		syscallarg(char **) argp;
		syscallarg(char **) envp;
	} */ *uap = v;
	struct sys_execve_args ap;
	caddr_t sg;

	sg = stackgap_init(p, 0);
	CHECK_ALT_EXIST(p, &sg, SCARG(uap, path));

	SCARG(&ap, path) = SCARG(uap, path);
	SCARG(&ap, argp) = SCARG(uap, argp);
	SCARG(&ap, envp) = SCARG(uap, envp);

	return sys_execve(p, &ap, retval);
}

int
bsd_sys_chroot(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct sys_chroot_args /* {
		syscallarg(char *) path;
	} */ *uap = v;
	caddr_t sg = stackgap_init(p, 0);

	CHECK_ALT_EXIST(p, &sg, SCARG(uap, path));
	return sys_chroot(p, uap, retval);
}

int
bsd_compat_12_sys_swapon(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct compat_12_sys_swapon_args /* {
		syscallarg(char *) name;
	} */ *uap = v;
	caddr_t sg = stackgap_init(p, 0);

	CHECK_ALT_EXIST(p, &sg, SCARG(uap, name));
	return compat_12_sys_swapon(p, uap, retval);
}

int
bsd_sys_rename(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct sys_rename_args /* {
		syscallarg(char *) from;
		syscallarg(char *) to;
	} */ *uap = v;
	caddr_t sg = stackgap_init(p, 0);

	CHECK_ALT_EXIST(p, &sg, SCARG(uap, from));
	CHECK_ALT_CREAT(p, &sg, SCARG(uap, to));
	return sys_rename(p, uap, retval);
}

int
bsd_compat_43_sys_truncate(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct compat_43_sys_truncate_args /* {
		syscallarg(char *) path;
		syscallarg(long) length;
	} */ *uap = v;
	caddr_t sg = stackgap_init(p, 0);

	CHECK_ALT_EXIST(p, &sg, SCARG(uap, path));
	return compat_43_sys_truncate(p, uap, retval);
}

int
bsd_sys_mkfifo(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct sys_mkfifo_args /* {
		syscallarg(char *) path;
		syscallarg(int) mode;
	} */ *uap = v;
	caddr_t sg = stackgap_init(p, 0);

	CHECK_ALT_CREAT(p, &sg, SCARG(uap, path));
	return sys_mkfifo(p, uap, retval);
}

int
bsd_sys_mkdir(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct sys_mkdir_args /* {
		syscallarg(char *) path;
		syscallarg(int) mode;
	} */ *uap = v;
	caddr_t sg = stackgap_init(p, 0);

	CHECK_ALT_CREAT(p, &sg, SCARG(uap, path));
	return sys_mkdir(p, uap, retval);
}

int
bsd_sys_rmdir(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct sys_rmdir_args /* {
		syscallarg(char *) path;
	} */ *uap = v;
	caddr_t sg = stackgap_init(p, 0);

	CHECK_ALT_EXIST(p, &sg, SCARG(uap, path));
	return sys_rmdir(p, uap, retval);
}

int
bsd_sys_utimes(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct sys_utimes_args /* {
		syscallarg(char *) path;
		syscallarg(struct timeval *) tptr;
	} */ *uap = v;
	caddr_t sg = stackgap_init(p, 0);

	CHECK_ALT_EXIST(p, &sg, SCARG(uap, path));
	return sys_utimes(p, uap, retval);
}

int
bsd_sys_quotactl(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct sys_quotactl_args /* {
		syscallarg(char *) path;
		syscallarg(int) cmd;
		syscallarg(int) uid;
		syscallarg(caddr_t) arg;
	} */ *uap = v;
	caddr_t sg = stackgap_init(p, 0);

	CHECK_ALT_EXIST(p, &sg, SCARG(uap, path));
	return sys_quotactl(p, uap, retval);
}

int
bsd_sys_statfs(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct sys_statfs_args /* {
		syscallarg(char *) path;
		syscallarg(struct statfs *) buf;
	} */ *uap = v;
	caddr_t sg = stackgap_init(p, 0);

	CHECK_ALT_EXIST(p, &sg, SCARG(uap, path));
	return sys_statfs(p, uap, retval);
}

#ifdef NFS
int
bsd_sys_getfh(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct sys_getfh_args /* {
		syscallarg(char *) fname;
		syscallarg(fhandle_t *) fhp;
	} */ *uap = v;
	caddr_t sg = stackgap_init(p, 0);

	CHECK_ALT_EXIST(p, &sg, SCARG(uap, fname));
	return sys_getfh(p, uap, retval);
}
#endif /* NFS */

int
bsd_compat_12_sys_stat(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct compat_12_sys_stat_args /* {
		syscallarg(char *) path;
		syscallarg(struct stat12 *) ub;
	} */ *uap = v;
	caddr_t sg = stackgap_init(p, 0);

	CHECK_ALT_EXIST(p, &sg, SCARG(uap, path));
	return compat_12_sys_stat(p, uap, retval);
}

int
bsd_compat_12_sys_lstat(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct compat_12_sys_lstat_args /* {
		syscallarg(char *) path;
		syscallarg(struct stat12 *) ub;
	} */ *uap = v;
	caddr_t sg = stackgap_init(p, 0);

	CHECK_ALT_EXIST(p, &sg, SCARG(uap, path));
	return compat_12_sys_lstat(p, uap, retval);
}

int
bsd_sys_pathconf(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct sys_pathconf_args /* {
		syscallarg(char *) path;
		syscallarg(int) name;
	} */ *uap = v;
	caddr_t sg = stackgap_init(p, 0);

	CHECK_ALT_EXIST(p, &sg, SCARG(uap, path));
	return sys_pathconf(p, uap, retval);
}

int
bsd_sys_truncate(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct sys_truncate_args /* {
		syscallarg(char *) path;
		syscallarg(int) pad;
		syscallarg(off_t) length;
	} */ *uap = v;
	caddr_t sg = stackgap_init(p, 0);

	CHECK_ALT_EXIST(p, &sg, SCARG(uap, path));
	return sys_truncate(p, uap, retval);
}

int
bsd_sys_undelete(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct sys_undelete_args /* {
		syscallarg(char *) path;
	} */ *uap = v;
	caddr_t sg = stackgap_init(p, 0);

	CHECK_ALT_EXIST(p, &sg, SCARG(uap, path));
	return sys_undelete(p, uap, retval);
}

int
bsd_sys_lchmod(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct sys_lchmod_args /* {
		syscallarg(char *) path;
		syscallarg(mode_t) mode;
	} */ *uap = v;
	caddr_t sg = stackgap_init(p, 0);

	CHECK_ALT_SYMLINK(p, &sg, SCARG(uap, path));
	return sys_lchmod(p, uap, retval);
}

int
bsd_sys_lchown(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct sys_lchown_args /* {
		syscallarg(char *) path;
		syscallarg(int) uid;
		syscallarg(int) gid;
	} */ *uap = v;
	caddr_t sg = stackgap_init(p, 0);

	CHECK_ALT_SYMLINK(p, &sg, SCARG(uap, path));
	return sys_lchown(p, uap, retval);
}

int
bsd_sys_lutimes(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct sys_lutimes_args /* {
		syscallarg(char *) path;
		syscallarg(struct timeval *) tptr;
	} */ *uap = v;
	caddr_t sg = stackgap_init(p, 0);

	CHECK_ALT_EXIST(p, &sg, SCARG(uap, path));
	return sys_lutimes(p, uap, retval);
}

int
bsd_sys___stat13(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct sys___stat13_args /* {
		syscallarg(char *) path;
		syscallarg(struct stat *) ub;
	} */ *uap = v;
	caddr_t sg = stackgap_init(p, 0);

	CHECK_ALT_EXIST(p, &sg, SCARG(uap, path));
	return sys___stat13(p, uap, retval);
}

int
bsd_sys___lstat13(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct sys___lstat13_args /* {
		syscallarg(char *) path;
		syscallarg(struct stat *) ub;
	} */ *uap = v;
	caddr_t sg = stackgap_init(p, 0);

	CHECK_ALT_EXIST(p, &sg, SCARG(uap, path));
	return sys___lstat13(p, uap, retval);
}

int
bsd_sys___posix_chown(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct sys___posix_chown_args /* {
		syscallarg(char *) path;
		syscallarg(int) uid;
		syscallarg(int) gid;
	} */ *uap = v;
	caddr_t sg = stackgap_init(p, 0);

	CHECK_ALT_EXIST(p, &sg, SCARG(uap, path));
	return sys___posix_chown(p, uap, retval);
}

int
bsd_sys___posix_lchown(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct sys___posix_lchown_args /* {
		syscallarg(char *) path;
		syscallarg(int) uid;
		syscallarg(int) gid;
	} */ *uap = v;
	caddr_t sg = stackgap_init(p, 0);

	CHECK_ALT_EXIST(p, &sg, SCARG(uap, path));
	return sys___posix_lchown(p, uap, retval);
}

int
bsd_sys_lchflags(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct sys_lchflags_args /* {
		syscallarg(char *) path;
		syscallarg(int) flags;
	} */ *uap = v;
	caddr_t sg = stackgap_init(p, 0);

	CHECK_ALT_EXIST(p, &sg, SCARG(uap, path));
	return sys_lchflags(p, uap, retval);
}

#endif /* defined(COMPAT_DARWIN) */
