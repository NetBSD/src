/*	$NetBSD: darwin_stat.c,v 1.1 2003/09/06 11:18:03 manu Exp $ */

/*-
 * Copyright (c) 2003 The NetBSD Foundation, Inc.
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
__KERNEL_RCSID(0, "$NetBSD: darwin_stat.c,v 1.1 2003/09/06 11:18:03 manu Exp $");

#include <sys/types.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/mount.h>
#include <sys/sa.h>
#include <sys/proc.h>
#include <sys/stat.h>
#include <sys/syscallargs.h>

#include <compat/common/compat_util.h>

#include <compat/mach/mach_types.h>
#include <compat/mach/mach_vm.h>

#include <compat/darwin/darwin_syscallargs.h>

#define native_to_darwin_dev(x) ((dev_t)(((major(x)) << 24) | minor((x))))

int
darwin_sys_stat(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct darwin_sys_stat_args /* {
		syscallarg(char *) path;
		syscallarg(struct stat12 *) ub;
	} */ *uap = v;
	struct compat_12_sys_stat_args cup;
	struct proc *p = l->l_proc;
	caddr_t sg = stackgap_init(p, 0);
	struct stat12 st;
	struct stat12 *stp;
	int error;

	CHECK_ALT_EXIST(p, &sg, SCARG(uap, path));
	stp = stackgap_alloc(p, &sg, sizeof(*stp));

	SCARG(&cup, path) = SCARG(uap, path);
	SCARG(&cup, ub) = stp;

	if ((error = compat_12_sys_stat(l, &cup, retval)) != 0)
		return error;

	if ((error = copyin(stp, &st, sizeof(st))) != 0)
		return error;

	st.st_dev = native_to_darwin_dev(st.st_dev);
	st.st_rdev = native_to_darwin_dev(st.st_rdev);

	if ((error = copyout(&st, SCARG(uap, ub), sizeof(st))) != 0)
		return error;

	return 0;
}

int
darwin_sys_fstat(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct darwin_sys_fstat_args /* {
		syscallarg(int) fd;
		syscallarg(struct stat12 *) ub;
	} */ *uap = v;
	struct compat_12_sys_fstat_args cup;
	struct proc *p = l->l_proc;
	caddr_t sg = stackgap_init(p, 0);
	struct stat12 st;
	struct stat12 *stp;
	int error;

	stp = stackgap_alloc(p, &sg, sizeof(*stp));

	SCARG(&cup, fd) = SCARG(uap, fd);
	SCARG(&cup, sb) = stp;

	if ((error = compat_12_sys_fstat(l, &cup, retval)) != 0)
		return error;

	if ((error = copyin(stp, &st, sizeof(st))) != 0)
		return error;

	st.st_dev = native_to_darwin_dev(st.st_dev);
	st.st_rdev = native_to_darwin_dev(st.st_rdev);

	if ((error = copyout(&st, SCARG(uap, sb), sizeof(st))) != 0)
		return error;

	return 0;
}

int
darwin_sys_lstat(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct darwin_sys_lstat_args /* {
		syscallarg(char *) path;
		syscallarg(struct stat12 *) ub;
	} */ *uap = v;
	struct compat_12_sys_lstat_args cup;
	struct proc *p = l->l_proc;
	caddr_t sg = stackgap_init(p, 0);
	struct stat12 st;
	struct stat12 *stp;
	int error;

	CHECK_ALT_EXIST(p, &sg, SCARG(uap, path));
	stp = stackgap_alloc(p, &sg, sizeof(*stp));

	SCARG(&cup, path) = SCARG(uap, path);
	SCARG(&cup, ub) = stp;

	if ((error = compat_12_sys_lstat(l, &cup, retval)) != 0)
		return error;

	if ((error = copyin(stp, &st, sizeof(st))) != 0)
		return error;

	st.st_dev = native_to_darwin_dev(st.st_dev);
	st.st_rdev = native_to_darwin_dev(st.st_rdev);

	if ((error = copyout(&st, SCARG(uap, ub), sizeof(st))) != 0)
		return error;

	return 0;
}

