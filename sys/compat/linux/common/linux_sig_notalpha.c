/*	$NetBSD: linux_sig_notalpha.c,v 1.24 2002/02/15 16:48:03 christos Exp $	*/

/*-
 * Copyright (c) 1995, 1998 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Frank van der Linden and Eric Haszlakiewicz.
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
 *	This product includes software developed by the NetBSD
 *	Foundation, Inc. and its contributors.
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

/*
 * heavily from: svr4_signal.c,v 1.7 1995/01/09 01:04:21 christos Exp
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: linux_sig_notalpha.c,v 1.24 2002/02/15 16:48:03 christos Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/filedesc.h>
#include <sys/mount.h>
#include <sys/kernel.h>
#include <sys/signal.h>
#include <sys/signalvar.h>

#include <sys/syscallargs.h>

#include <compat/linux/common/linux_types.h>
#include <compat/linux/common/linux_signal.h>
#include <compat/linux/common/linux_util.h>

#include <compat/linux/linux_syscallargs.h>

/* Used on: arm, i386, m68k, mips, sparc, sparc64 */
/* Not used on: alpha */

/*
 * The Linux signal() system call. I think that the signal() in the C
 * library actually calls sigaction, so I doubt this one is ever used.
 * But hey, it can't hurt having it here. The same restrictions as for
 * sigaction() apply.
 */
int
linux_sys_signal(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct linux_sys_signal_args /* {
		syscallarg(int) signum;
		syscallarg(linux_handler_t) handler;
	} */ *uap = v;
	struct sigaction nbsa, obsa;
	int error, sig;

	*retval = -1;
	sig = SCARG(uap, signum);
	if (sig < 0 || sig >= LINUX__NSIG)
		return (EINVAL);

	nbsa.sa_handler = SCARG(uap, handler);
	sigemptyset(&nbsa.sa_mask);
	nbsa.sa_flags = SA_RESETHAND | SA_NODEFER;
	error = sigaction1(p, linux_to_native_sig[sig],
	    &nbsa, &obsa);
	if (error == 0)
		*retval = (int)obsa.sa_handler;
	return (error);
}


/* ARGSUSED */
int
linux_sys_siggetmask(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	sigset_t bss;
	linux_old_sigset_t lss;
	int error;

	error = sigprocmask1(p, SIG_SETMASK, 0, &bss);
	if (error)
		return (error);
	native_to_linux_old_sigset(&lss, &bss);
	return (0);
}

/*
 * The following three functions fiddle with a process' signal mask.
 * Convert the signal masks because of the different signal
 * values for Linux. The need for this is the reason why
 * they are here, and have not been mapped directly.
 */
int
linux_sys_sigsetmask(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct linux_sys_sigsetmask_args /* {
		syscallarg(linux_old_sigset_t) mask;
	} */ *uap = v;
	sigset_t nbss, obss;
	linux_old_sigset_t nlss, olss;
	int error;

	nlss = SCARG(uap, mask);
	linux_old_to_native_sigset(&nbss, &nlss);
	error = sigprocmask1(p, SIG_SETMASK, &nbss, &obss);
	if (error)
		return (error);
	native_to_linux_old_sigset(&olss, &obss);
	*retval = olss;
	return (0);
}

int
linux_sys_sigprocmask(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct linux_sys_sigprocmask_args /* {
		syscallarg(int) how;
		syscallarg(const linux_old_sigset_t *) set;
		syscallarg(linux_old_sigset_t *) oset;
	} */ *uap = v;

	return(linux_sigprocmask1(p, SCARG(uap, how), 
				SCARG(uap, set), SCARG(uap, oset)));
}

/*
 * The deprecated pause(2), which is really just an instance
 * of sigsuspend(2).
 */
int
linux_sys_pause(p, v, retval)
	struct proc *p;
	void *v;	
	register_t *retval;
{	

	return (sigsuspend1(p, 0));
}

