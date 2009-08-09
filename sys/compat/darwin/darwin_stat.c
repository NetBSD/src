/*	$NetBSD: darwin_stat.c,v 1.16 2009/08/09 22:49:00 haad Exp $ */

/*-
 * Copyright (c) 2003, 2008 The NetBSD Foundation, Inc.
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
__KERNEL_RCSID(0, "$NetBSD: darwin_stat.c,v 1.16 2009/08/09 22:49:00 haad Exp $");

#include <sys/types.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/filedesc.h>
#include <sys/mount.h>
#include <sys/namei.h>
#include <sys/proc.h>
#include <sys/stat.h>
#include <sys/syscallargs.h>
#include <sys/vfs_syscalls.h>

#include <compat/sys/signal.h>
#include <compat/sys/stat.h>

#include <compat/common/compat_util.h>

#include <compat/mach/mach_types.h>
#include <compat/mach/mach_vm.h>

#include <compat/darwin/darwin_types.h>
#include <compat/darwin/darwin_audit.h>
#include <compat/darwin/darwin_syscallargs.h>

int
darwin_sys_stat(struct lwp *l, const struct darwin_sys_stat_args *uap, register_t *retval)
{
	/* {
		syscallarg(char *) path;
		syscallarg(struct stat12 *) ub;
	} */
	struct stat12 sb12;
	struct stat sb;
	int error;

	error = do_sys_stat(SCARG(uap, path), FOLLOW, &sb);
	if (error != 0)
		return error;

	compat_12_stat_conv(&sb, &sb12);
	sb12.st_dev = native_to_darwin_dev(sb12.st_dev);
	sb12.st_rdev = native_to_darwin_dev(sb12.st_rdev);

	return copyout(&sb12, SCARG(uap, ub), sizeof(sb12));
}

int
darwin_sys_fstat(struct lwp *l, const struct darwin_sys_fstat_args *uap, register_t *retval)
{
	/* {
		syscallarg(int) fd;
		syscallarg(struct stat12 *) sb;
	} */
	struct stat12 sb12;
	struct stat sb;
	int error;

	error = do_sys_fstat(SCARG(uap, fd), &sb);
	if (error != 0)
		return error;

	compat_12_stat_conv(&sb, &sb12);
	sb12.st_dev = native_to_darwin_dev(sb12.st_dev);
	sb12.st_rdev = native_to_darwin_dev(sb12.st_rdev);

	return copyout(&sb12, SCARG(uap, sb), sizeof(sb12));
}

int
darwin_sys_lstat(struct lwp *l, const struct darwin_sys_lstat_args *uap, register_t *retval)
{
	/* {
		syscallarg(char *) path;
		syscallarg(struct stat12 *) ub;
	} */
	struct stat12 sb12;
	struct stat sb;
	int error;

	error = do_sys_stat(SCARG(uap, path), NOFOLLOW, &sb);
	if (error != 0)
		return error;

	compat_12_stat_conv(&sb, &sb12);
	sb12.st_dev = native_to_darwin_dev(sb12.st_dev);
	sb12.st_rdev = native_to_darwin_dev(sb12.st_rdev);

	return copyout(&sb12, SCARG(uap, ub), sizeof(sb12));
}

int
darwin_sys_mknod(struct lwp *l, const struct darwin_sys_mknod_args *uap, register_t *retval)
{
	/* {
		syscallarg(char) path;
		syscallarg(mode_t) mode;
		syscallarg(darwin_dev_t) dev:
	} */

	return do_sys_mknod(l, SCARG(uap, path), SCARG(uap, mode),
	    darwin_to_native_dev(SCARG(uap, dev)), retval, UIO_USERSPACE);
}
