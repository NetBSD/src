/*	$NetBSD: vfs_syscalls_90.c,v 1.1 2019/09/22 22:59:38 christos Exp $	*/

/*-
 * Copyright (c) 2005, 2008 The NetBSD Foundation, Inc.
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
__KERNEL_RCSID(0, "$NetBSD: vfs_syscalls_90.c,v 1.1 2019/09/22 22:59:38 christos Exp $");

#if defined(_KERNEL_OPT)
#include "opt_compat_netbsd.h"
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
#include <sys/malloc.h>
#include <sys/kauth.h>
#include <sys/vfs_syscalls.h>
#include <sys/syscall.h>
#include <sys/syscallvar.h>
#include <sys/syscallargs.h>

#include <compat/common/compat_mod.h>
#include <compat/common/compat_util.h>
#include <compat/sys/statvfs.h>

static const struct syscall_package vfs_syscalls_90_syscalls[] = {
	{ SYS_compat_90_getvfsstat, 0, (sy_call_t *)compat_90_sys_getvfsstat },
	{ SYS_compat_90_statvfs1, 0, (sy_call_t *)compat_90_sys_statvfs1 },
	{ SYS_compat_90_fstatvfs1, 0, (sy_call_t *)compat_90_sys_fstatvfs1 },
	{ SYS_compat_90_fhstatvfs1, 0, (sy_call_t *)compat_90_sys_fhstatvfs1 },
	{ 0,0, NULL }
};


int
compat_90_sys_getvfsstat(struct lwp *l,
    const struct compat_90_sys_getvfsstat_args *uap, register_t *retval)
{
	/* {
		syscallarg(struct statvfs90 *) buf;
		syscallarg(size_t) bufsize;
		syscallarg(int)	flags;
	} */

	return do_sys_getvfsstat(l, SCARG(uap, buf), SCARG(uap, bufsize),
	    SCARG(uap, flags), statvfs_to_statvfs90_copy,
	    sizeof(struct statvfs90), retval);
}

int
compat_90_sys_statvfs1(struct lwp *l,
    const struct compat_90_sys_statvfs1_args *uap, register_t *retval)
{
	/* {
		syscallarg(const char *) path;
		syscallarg(struct statvfs90 *) buf;
		syscallarg(int)	flags;
	} */

	struct statvfs *sb = STATVFSBUF_GET();
	int error = do_sys_pstatvfs(l, SCARG(uap, path), SCARG(uap, flags), sb);

	if (!error)
		error = statvfs_to_statvfs90_copy(sb, SCARG(uap, buf),
		    sizeof(struct statvfs90));

	STATVFSBUF_PUT(sb);
	return error;
}

int
compat_90_sys_fstatvfs1(struct lwp *l,
    const struct compat_90_sys_fstatvfs1_args *uap, register_t *retval)
{
	/* {
		syscallarg(int) fd;
		syscallarg(struct statvfs90 *) buf;
		syscallarg(int)	flags;
	} */

	struct statvfs *sb = STATVFSBUF_GET();
	int error = do_sys_fstatvfs(l, SCARG(uap, fd), SCARG(uap, flags), sb);

	if (!error)
		error = statvfs_to_statvfs90_copy(sb, SCARG(uap, buf),
		    sizeof(struct statvfs90));

	STATVFSBUF_PUT(sb);
	return error;
}

int
compat_90_sys_fhstatvfs1(struct lwp *l,
    const struct compat_90_sys_fhstatvfs1_args *uap, register_t *retval)
{
	/* {
		syscallarg(const void *) fhp;
		syscallarg(size_t) fh_size;
		syscallarg(struct statvfs90 *) buf;
		syscallarg(int)	flags;
	} */

	struct statvfs *sb = STATVFSBUF_GET();
	int error = do_fhstatvfs(l, SCARG(uap, fhp), SCARG(uap, fh_size),
	    sb, SCARG(uap, flags));

	if (!error)
		error = statvfs_to_statvfs90_copy(sb, SCARG(uap, buf),
		    sizeof(struct statvfs90));

	STATVFSBUF_PUT(sb);
	return error;
}

int
vfs_syscalls_90_init(void)
{

	return syscall_establish(NULL, vfs_syscalls_90_syscalls);
}

int
vfs_syscalls_90_fini(void)
{

	return syscall_disestablish(NULL, vfs_syscalls_90_syscalls);
}
