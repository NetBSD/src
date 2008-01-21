/* $NetBSD: osf1_file.c,v 1.19.2.4 2008/01/21 09:41:57 yamt Exp $ */

/*
 * Copyright (c) 1999 Christopher G. Demetriou.  All rights reserved.
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
 *      This product includes software developed by Christopher G. Demetriou
 *	for the NetBSD Project.
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
 */

/*
 * Copyright (c) 1994, 1995 Carnegie-Mellon University.
 * All rights reserved.
 *
 * Author: Chris G. Demetriou
 *
 * Permission to use, copy, modify and distribute this software and
 * its documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 *
 * CARNEGIE MELLON ALLOWS FREE USE OF THIS SOFTWARE IN ITS "AS IS"
 * CONDITION.  CARNEGIE MELLON DISCLAIMS ANY LIABILITY OF ANY KIND
 * FOR ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 *
 * Carnegie Mellon requests users of this software to return to
 *
 *  Software Distribution Coordinator  or  Software.Distribution@CS.CMU.EDU
 *  School of Computer Science
 *  Carnegie Mellon University
 *  Pittsburgh PA 15213-3890
 *
 * any improvements or extensions that they make and grant Carnegie the
 * rights to redistribute these changes.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: osf1_file.c,v 1.19.2.4 2008/01/21 09:41:57 yamt Exp $");

#if defined(_KERNEL_OPT)
#include "opt_syscall_debug.h"
#endif

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/namei.h>
#include <sys/proc.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/filedesc.h>
#include <sys/kernel.h>
#include <sys/malloc.h>
#include <sys/mman.h>
#include <sys/mount.h>
#include <sys/signal.h>
#include <sys/signalvar.h>
#include <sys/reboot.h>
#include <sys/syscallargs.h>
#include <sys/exec.h>
#include <sys/vnode.h>
#include <sys/socketvar.h>
#include <sys/resource.h>
#include <sys/resourcevar.h>
#include <sys/wait.h>
#include <sys/vfs_syscalls.h>

#include <compat/osf1/osf1.h>
#include <compat/osf1/osf1_syscallargs.h>
#include <compat/common/compat_util.h>
#include <compat/osf1/osf1_cvt.h>

int
osf1_sys_access(struct lwp *l, const struct osf1_sys_access_args *uap, register_t *retval)
{
	struct sys_access_args a;
	unsigned long leftovers;

	SCARG(&a, path) = SCARG(uap, path);

	/* translate flags */
	SCARG(&a, flags) = emul_flags_translate(osf1_access_flags_xtab,
	    SCARG(uap, flags), &leftovers);
	if (leftovers != 0)
		return (EINVAL);

	return sys_access(l, &a, retval);
}

int
osf1_sys_execve(struct lwp *l, const struct osf1_sys_execve_args *uap, register_t *retval)
{
	struct sys_execve_args ap;

	SCARG(&ap, path) = SCARG(uap, path);
	SCARG(&ap, argp) = SCARG(uap, argp);
	SCARG(&ap, envp) = SCARG(uap, envp);

	return sys_execve(l, &ap, retval);
}

/*
 * Get file status; this version does not follow links.
 */
/* ARGSUSED */
int
osf1_sys_lstat(struct lwp *l, const struct osf1_sys_lstat_args *uap, register_t *retval)
{
	struct stat sb;
	struct osf1_stat osb;
	int error;

	error = do_sys_stat(l, SCARG(uap, path), NOFOLLOW, &sb);
	if (error)
		return (error);
	osf1_cvt_stat_from_native(&sb, &osb);
	error = copyout(&osb, SCARG(uap, ub), sizeof (osb));
	return (error);
}

/*
 * Get file status; this version does not follow links.
 */
/* ARGSUSED */
int
osf1_sys_lstat2(struct lwp *l, const struct osf1_sys_lstat2_args *uap, register_t *retval)
{
	struct stat sb;
	struct osf1_stat2 osb;
	int error;

	error = do_sys_stat(l, SCARG(uap, path), NOFOLLOW, &sb);
	if (error)
		return (error);
	osf1_cvt_stat2_from_native(&sb, &osb);
	error = copyout((void *)&osb, (void *)SCARG(uap, ub), sizeof (osb));
	return (error);
}

int
osf1_sys_mknod(struct lwp *l, const struct osf1_sys_mknod_args *uap, register_t *retval)
{
	struct sys_mknod_args a;

	SCARG(&a, path) = SCARG(uap, path);
	SCARG(&a, mode) = SCARG(uap, mode);
	SCARG(&a, dev) = osf1_cvt_dev_to_native(SCARG(uap, dev));

	return sys_mknod(l, &a, retval);
}

int
osf1_sys_open(struct lwp *l, const struct osf1_sys_open_args *uap, register_t *retval)
{
	struct sys_open_args a;
	const char *path;
	unsigned long leftovers;
#ifdef SYSCALL_DEBUG
	char pnbuf[1024];

	if (scdebug &&
	    copyinstr(SCARG(uap, path), pnbuf, sizeof pnbuf, NULL) == 0)
		printf("osf1_open: open: %s\n", pnbuf);
#endif

	/* translate flags */
	SCARG(&a, flags) = emul_flags_translate(osf1_open_flags_xtab,
	    SCARG(uap, flags), &leftovers);
	if (leftovers != 0)
		return (EINVAL);

	/* copy mode, no translation necessary */
	SCARG(&a, mode) = SCARG(uap, mode);

	/* pick appropriate path */
	path = SCARG(uap, path);
	SCARG(&a, path) = path;

	return sys_open(l, &a, retval);
}

int
osf1_sys_pathconf(struct lwp *l, const struct osf1_sys_pathconf_args *uap, register_t *retval)
{
	struct sys_pathconf_args a;
	int error;

	SCARG(&a, path) = SCARG(uap, path);

	error = osf1_cvt_pathconf_name_to_native(SCARG(uap, name),
	    &SCARG(&a, name));

	if (error == 0)
		error = sys_pathconf(l, &a, retval);

	return (error);
}

/*
 * Get file status; this version follows links.
 */
/* ARGSUSED */
int
osf1_sys_stat(struct lwp *l, const struct osf1_sys_stat_args *uap, register_t *retval)
{
	struct stat sb;
	struct osf1_stat osb;
	int error;

	error = do_sys_stat(l, SCARG(uap, path), FOLLOW, &sb);
	if (error)
		return (error);
	osf1_cvt_stat_from_native(&sb, &osb);
	error = copyout((void *)&osb, (void *)SCARG(uap, ub), sizeof (osb));
	return (error);
}

/*
 * Get file status; this version follows links.
 */
/* ARGSUSED */
int
osf1_sys_stat2(struct lwp *l, const struct osf1_sys_stat2_args *uap, register_t *retval)
{
	struct stat sb;
	struct osf1_stat2 osb;
	int error;

	error = do_sys_stat(l, SCARG(uap, path), FOLLOW, &sb);
	if (error)
		return (error);
	osf1_cvt_stat2_from_native(&sb, &osb);
	error = copyout((void *)&osb, (void *)SCARG(uap, ub), sizeof (osb));
	return (error);
}

int
osf1_sys_truncate(struct lwp *l, const struct osf1_sys_truncate_args *uap, register_t *retval)
{
	struct sys_truncate_args a;

	SCARG(&a, path) = SCARG(uap, path);
	SCARG(&a, pad) = 0;
	SCARG(&a, length) = SCARG(uap, length);

	return sys_truncate(l, &a, retval);
}

int
osf1_sys_utimes(struct lwp *l, const struct osf1_sys_utimes_args *uap, register_t *retval)
{
	struct osf1_timeval otv;
	struct timeval tv[2], *tvp;
	int error;

	if (SCARG(uap, tptr) == NULL)
		tvp = NULL;
	else {
		/* get the OSF/1 timeval argument */
		error = copyin(SCARG(uap, tptr), &otv, sizeof otv);
		if (error != 0)
			return error;

		/* fill in and copy out the NetBSD timeval */
		tv[0].tv_sec = otv.tv_sec;
		tv[0].tv_usec = otv.tv_usec;
		/* Set access and modified to the same time */
		tv[1].tv_sec = otv.tv_sec;
		tv[1].tv_usec = otv.tv_usec;
		tvp = tv;
	}

	return do_sys_utimes(l, NULL, SCARG(uap, path), FOLLOW,
			    tvp, UIO_SYSSPACE);
}
