/*	$NetBSD: pecoff_misc.c,v 1.2.8.2 2002/06/23 17:44:40 jdolecek Exp $	*/

/*-
 * Copyright (c) 1998 The NetBSD Foundation, Inc.
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
__KERNEL_RCSID(0, "$NetBSD: pecoff_misc.c,v 1.2.8.2 2002/06/23 17:44:40 jdolecek Exp $");

#if defined(_KERNEL_OPT)
#include "opt_ktrace.h"
#include "opt_nfsserver.h"
#include "opt_compat_netbsd.h"
#include "opt_sysv.h"
#include "opt_compat_43.h"

#include "fs_lfs.h"
#include "fs_nfs.h"
#endif

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/mount.h>
#include <sys/fcntl.h>
#include <sys/proc.h>

#include <sys/syscallargs.h>

#include <compat/common/compat_util.h>
#include <compat/pecoff/pecoff_syscallargs.h>

int
pecoff_sys_open(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct pecoff_sys_open_args *uap = v;
	caddr_t sg = stackgap_init(p, 0);

#if 0
	if (SCARG(uap, flags) & O_CREAT)
		CHECK_ALT_CREAT(p, &sg, SCARG(uap, path));
	else 
#endif
		CHECK_ALT_EXIST(p, &sg, SCARG(uap, path));

	return sys_open(p, v, retval);
}


int
pecoff_sys_link(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct pecoff_sys_link_args *uap = v;
	caddr_t sg = stackgap_init(p, 0);

	CHECK_ALT_EXIST(p, &sg, SCARG(uap, path));
#if 0
	CHECK_ALT_CREAT(p, &sg, SCARG(uap, link));
#endif

	return sys_link(p, v, retval);
}


int
pecoff_sys_unlink(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct pecoff_sys_unlink_args *uap = v;
	caddr_t sg = stackgap_init(p, 0);

	CHECK_ALT_EXIST(p, &sg, SCARG(uap, path));

	return sys_unlink(p, v, retval);
}


int
pecoff_sys_chdir(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct pecoff_sys_chdir_args *uap = v;
	caddr_t sg = stackgap_init(p, 0);

	CHECK_ALT_EXIST(p, &sg, SCARG(uap, path));

	return sys_chdir(p, v, retval);
}


int
pecoff_sys_chmod(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct pecoff_sys_chmod_args *uap = v;
	caddr_t sg = stackgap_init(p, 0);

	CHECK_ALT_EXIST(p, &sg, SCARG(uap, path));

	return sys_chmod(p, v, retval);
}


int
pecoff_sys_chown(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct pecoff_sys_chown_args *uap = v;
	caddr_t sg = stackgap_init(p, 0);

	CHECK_ALT_EXIST(p, &sg, SCARG(uap, path));

	return sys_chown(p, v, retval);
}


int
pecoff_sys_unmount(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct pecoff_sys_unmount_args *uap = v;
	caddr_t sg = stackgap_init(p, 0);

	CHECK_ALT_EXIST(p, &sg, SCARG(uap, path));

	return sys_unmount(p, v, retval);
}


int
pecoff_sys_access(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct pecoff_sys_access_args *uap = v;
	caddr_t sg = stackgap_init(p, 0);

	CHECK_ALT_EXIST(p, &sg, SCARG(uap, path));

	return sys_access(p, v, retval);
}


int
pecoff_sys_chflags(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct pecoff_sys_chflags_args *uap = v;
	caddr_t sg = stackgap_init(p, 0);

	CHECK_ALT_EXIST(p, &sg, SCARG(uap, path));

	return sys_chflags(p, v, retval);
}


#ifdef PECOFF_INCLUDE_COMPAT
int
pecoff_compat_43_sys_stat(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct pecoff_compat_43_sys_stat_args *uap = v;
	caddr_t sg = stackgap_init(p, 0);

	CHECK_ALT_EXIST(p, &sg, SCARG(uap, path));

	return compat_43_sys_stat(p, v, retval);
}


int
pecoff_compat_43_sys_lstat(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct pecoff_compat_43_sys_lstat_args *uap = v;
	caddr_t sg = stackgap_init(p, 0);

	CHECK_ALT_EXIST(p, &sg, SCARG(uap, path));

	return compat_43_sys_lstat(p, v, retval);
}
#endif /* PECOFF_INCLUDE_COMPAT */


int
pecoff_sys_revoke(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct pecoff_sys_revoke_args *uap = v;
	caddr_t sg = stackgap_init(p, 0);

	CHECK_ALT_EXIST(p, &sg, SCARG(uap, path));

	return sys_revoke(p, v, retval);
}


int
pecoff_sys_symlink(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct pecoff_sys_symlink_args *uap = v;
	caddr_t sg = stackgap_init(p, 0);

	CHECK_ALT_EXIST(p, &sg, SCARG(uap, path));
#if 0
	CHECK_ALT_CREAT(p, &sg, SCARG(uap, link));
#endif

	return sys_symlink(p, v, retval);
}


int
pecoff_sys_readlink(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct pecoff_sys_readlink_args *uap = v;
	caddr_t sg = stackgap_init(p, 0);

	CHECK_ALT_SYMLINK(p, &sg, SCARG(uap, path));

	return sys_readlink(p, v, retval);
}


int
pecoff_sys_execve(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct pecoff_sys_execve_args *uap = v;
	caddr_t sg = stackgap_init(p, 0);

	CHECK_ALT_EXIST(p, &sg, SCARG(uap, path));

	return sys_execve(p, v, retval);
}


int
pecoff_sys_chroot(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct pecoff_sys_chroot_args *uap = v;
	caddr_t sg = stackgap_init(p, 0);

	CHECK_ALT_EXIST(p, &sg, SCARG(uap, path));

	return sys_chroot(p, v, retval);
}


int
pecoff_sys_rename(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct pecoff_sys_rename_args *uap = v;
	caddr_t sg = stackgap_init(p, 0);

	CHECK_ALT_EXIST(p, &sg, SCARG(uap, from));
#if 0
	CHECK_ALT_CREAT(p, &sg, SCARG(uap, to));
#endif

	return sys_rename(p, v, retval);
}


#ifdef PECOFF_INCLUDE_COMPAT
int
pecoff_compat_43_sys_truncate(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct pecoff_compat_43_sys_truncate_args *uap = v;
	caddr_t sg = stackgap_init(p, 0);

	CHECK_ALT_EXIST(p, &sg, SCARG(uap, path));

	return compat_43_sys_truncate(p, v, retval);
}
#endif /* PECOFF_INCLUDE_COMPAT */


int
pecoff_sys_rmdir(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct pecoff_sys_rmdir_args *uap = v;
	caddr_t sg = stackgap_init(p, 0);

	CHECK_ALT_EXIST(p, &sg, SCARG(uap, path));

	return sys_rmdir(p, v, retval);
}


int
pecoff_sys_utimes(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct pecoff_sys_utimes_args *uap = v;
	caddr_t sg = stackgap_init(p, 0);

	CHECK_ALT_EXIST(p, &sg, SCARG(uap, path));

	return sys_utimes(p, v, retval);
}


int
pecoff_sys_statfs(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct pecoff_sys_statfs_args *uap = v;
	caddr_t sg = stackgap_init(p, 0);

	CHECK_ALT_EXIST(p, &sg, SCARG(uap, path));

	return sys_statfs(p, v, retval);
}


int
pecoff_sys_getfh(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct pecoff_sys_getfh_args *uap = v;
	caddr_t sg = stackgap_init(p, 0);

	CHECK_ALT_EXIST(p, &sg, SCARG(uap, fname));

	return sys_getfh(p, v, retval);
}


#ifdef PECOFF_INCLUDE_COMPAT
int
pecoff_compat_12_sys_stat(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct pecoff_compat_12_sys_stat_args *uap = v;
	caddr_t sg = stackgap_init(p, 0);

	CHECK_ALT_EXIST(p, &sg, SCARG(uap, path));

	return compat_12_sys_stat(p, v, retval);
}


int
pecoff_compat_12_sys_lstat(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct pecoff_compat_12_sys_lstat_args *uap = v;
	caddr_t sg = stackgap_init(p, 0);

	CHECK_ALT_EXIST(p, &sg, SCARG(uap, path));

	return compat_12_sys_lstat(p, v, retval);
}
#endif /* PECOFF_INCLUDE_COMPAT */

int
pecoff_sys_pathconf(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct pecoff_sys_pathconf_args *uap = v;
	caddr_t sg = stackgap_init(p, 0);

	CHECK_ALT_EXIST(p, &sg, SCARG(uap, path));

	return sys_pathconf(p, v, retval);
}


int
pecoff_sys_truncate(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct pecoff_sys_truncate_args *uap = v;
	caddr_t sg = stackgap_init(p, 0);

	CHECK_ALT_EXIST(p, &sg, SCARG(uap, path));

	return sys_truncate(p, v, retval);
}


int
pecoff_sys_undelete(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct pecoff_sys_undelete_args *uap = v;
	caddr_t sg = stackgap_init(p, 0);

	CHECK_ALT_EXIST(p, &sg, SCARG(uap, path));

	return sys_undelete(p, v, retval);
}


int
pecoff_sys___posix_rename(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct pecoff_sys___posix_rename_args *uap = v;
	caddr_t sg = stackgap_init(p, 0);

	CHECK_ALT_EXIST(p, &sg, SCARG(uap, from));
#if 0
	CHECK_ALT_CREAT(p, &sg, SCARG(uap, to));
#endif

	return sys___posix_rename(p, v, retval);
}


int
pecoff_sys_lchmod(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct pecoff_sys_lchmod_args *uap = v;
	caddr_t sg = stackgap_init(p, 0);

	CHECK_ALT_SYMLINK(p, &sg, SCARG(uap, path));

	return sys_lchmod(p, v, retval);
}


int
pecoff_sys_lchown(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct pecoff_sys_lchown_args *uap = v;
	caddr_t sg = stackgap_init(p, 0);

	CHECK_ALT_SYMLINK(p, &sg, SCARG(uap, path));

	return sys_lchown(p, v, retval);
}


int
pecoff_sys_lutimes(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct pecoff_sys_lutimes_args *uap = v;
	caddr_t sg = stackgap_init(p, 0);

	CHECK_ALT_SYMLINK(p, &sg, SCARG(uap, path));

	return sys_lutimes(p, v, retval);
}


int
pecoff_sys___posix_chown(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct pecoff_sys___posix_chown_args *uap = v;
	caddr_t sg = stackgap_init(p, 0);

	CHECK_ALT_EXIST(p, &sg, SCARG(uap, path));

	return sys___posix_chown(p, v, retval);
}
