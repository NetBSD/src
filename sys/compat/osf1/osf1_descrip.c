/* $NetBSD: osf1_descrip.c,v 1.26.4.2 2009/06/20 07:20:18 yamt Exp $ */

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
__KERNEL_RCSID(0, "$NetBSD: osf1_descrip.c,v 1.26.4.2 2009/06/20 07:20:18 yamt Exp $");

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

#include <compat/osf1/osf1.h>
#include <compat/osf1/osf1_syscallargs.h>
#include <compat/osf1/osf1_cvt.h>

int
osf1_sys_fcntl(struct lwp *l, const struct osf1_sys_fcntl_args *uap, register_t *retval)
{
	struct sys_fcntl_args a;
	struct osf1_flock oflock;
	struct flock nflock;
	unsigned long xfl, leftovers;
	int error;

	SCARG(&a, fd) = SCARG(uap, fd);

	leftovers = 0;
	switch (SCARG(uap, cmd)) {
	case OSF1_F_DUPFD:
		SCARG(&a, cmd) = F_DUPFD;
		SCARG(&a, arg) = SCARG(uap, arg);
		break;

	case OSF1_F_GETFD:
		SCARG(&a, cmd) = F_GETFD;
		SCARG(&a, arg) = 0;		/* ignored */
		break;

	case OSF1_F_SETFD:
		SCARG(&a, cmd) = F_SETFD;
		SCARG(&a, arg) = (void *)emul_flags_translate(
		    osf1_fcntl_getsetfd_flags_xtab,
		    (unsigned long)SCARG(uap, arg), &leftovers);
		break;

	case OSF1_F_GETFL:
		SCARG(&a, cmd) = F_GETFL;
		SCARG(&a, arg) = 0;		/* ignored */
		break;

	case OSF1_F_SETFL:
		SCARG(&a, cmd) = F_SETFL;
		xfl = emul_flags_translate(osf1_open_flags_xtab,
		    (unsigned long)SCARG(uap, arg), &leftovers);
		xfl |= emul_flags_translate(osf1_fcntl_getsetfl_flags_xtab,
		    leftovers, &leftovers);
		SCARG(&a, arg) = (void *)xfl;
		break;

	case OSF1_F_GETOWN:		/* XXX not yet supported */
	case OSF1_F_SETOWN:		/* XXX not yet supported */
		/* XXX translate. */
		return (EINVAL);

	case OSF1_F_GETLK:
	case OSF1_F_SETLK:
	case OSF1_F_SETLKW:
		if (SCARG(uap, cmd) == OSF1_F_GETLK)
			SCARG(&a, cmd) = F_GETLK;
		else if (SCARG(uap, cmd) == OSF1_F_SETLK)
			SCARG(&a, cmd) = F_SETLK;
		else if (SCARG(uap, cmd) == OSF1_F_SETLKW)
			SCARG(&a, cmd) = F_SETLKW;

		error = copyin(SCARG(uap, arg), &oflock, sizeof oflock);
		if (error != 0)
			return error;
		error = osf1_cvt_flock_to_native(&oflock, &nflock);
		if (error != 0)
			return error;
		error = do_fcntl_lock(SCARG(uap, fd), SCARG(&a, cmd), &nflock);
		if (SCARG(&a, cmd) != F_GETLK || error != 0)
			return error;
		osf1_cvt_flock_from_native(&nflock, &oflock);
		return copyout(&oflock, SCARG(uap, arg), sizeof oflock);

	case OSF1_F_RGETLK:		/* [lock mgr op] XXX not supported */
	case OSF1_F_RSETLK:		/* [lock mgr op] XXX not supported */
	case OSF1_F_CNVT:		/* [lock mgr op] XXX not supported */
	case OSF1_F_RSETLKW:		/* [lock mgr op] XXX not supported */
	case OSF1_F_PURGEFS:		/* [lock mgr op] XXX not supported */
	case OSF1_F_PURGENFS:		/* [DECsafe op] XXX not supported */
	default:
		/* XXX syslog? */
		return (EINVAL);
	}
	if (leftovers != 0)
		return (EINVAL);

	error = sys_fcntl(l, &a, retval);

	if (error)
		return error;

	switch (SCARG(uap, cmd)) {
	case OSF1_F_GETFD:
		retval[0] = emul_flags_translate(
		    osf1_fcntl_getsetfd_flags_rxtab, retval[0], NULL);
		break;

	case OSF1_F_GETFL:
		xfl = emul_flags_translate(osf1_open_flags_rxtab,
		    retval[0], &leftovers);
		xfl |= emul_flags_translate(osf1_fcntl_getsetfl_flags_rxtab,
		    leftovers, NULL);
		retval[0] = xfl;
		break;
	}

	return error;
}

int
osf1_sys_fpathconf(struct lwp *l, const struct osf1_sys_fpathconf_args *uap, register_t *retval)
{
	struct sys_fpathconf_args a;
	int error;

	SCARG(&a, fd) = SCARG(uap, fd);

	error = osf1_cvt_pathconf_name_to_native(SCARG(uap, name),
	    &SCARG(&a, name));

	if (error == 0)
		error = sys_fpathconf(l, &a, retval);

	return (error);
}

/*
 * Return status information about a file descriptor.
 */
int
osf1_sys_fstat(struct lwp *l, const struct osf1_sys_fstat_args *uap, register_t *retval)
{
	struct stat ub;
	struct osf1_stat oub;
	int error;

	error = do_sys_fstat(SCARG(uap, fd), &ub);
	osf1_cvt_stat_from_native(&ub, &oub);
	if (error == 0)
		error = copyout(&oub, SCARG(uap, sb), sizeof(oub));

	return (error);
}

/*
 * Return status information about a file descriptor.
 */
int
osf1_sys_fstat2(struct lwp *l, const struct osf1_sys_fstat2_args *uap, register_t *retval)
{
	struct stat ub;
	struct osf1_stat2 oub;
	int error;

	error = do_sys_fstat(SCARG(uap, fd), &ub);
	osf1_cvt_stat2_from_native(&ub, &oub);
	if (error == 0)
		error = copyout(&oub, SCARG(uap, sb), sizeof(oub));

	return (error);
}

int
osf1_sys_ftruncate(struct lwp *l, const struct osf1_sys_ftruncate_args *uap, register_t *retval)
{
	struct sys_ftruncate_args a;

	SCARG(&a, fd) = SCARG(uap, fd);
	SCARG(&a, PAD) = 0;
	SCARG(&a, length) = SCARG(uap, length);

	return sys_ftruncate(l, &a, retval);
}

int
osf1_sys_lseek(struct lwp *l, const struct osf1_sys_lseek_args *uap, register_t *retval)
{
	struct sys_lseek_args a;

	SCARG(&a, fd) = SCARG(uap, fd);
	SCARG(&a, PAD) = 0;
	SCARG(&a, offset) = SCARG(uap, offset);
	SCARG(&a, whence) = SCARG(uap, whence);

	return sys_lseek(l, &a, retval);
}
