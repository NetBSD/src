/*	$NetBSD: linux_sigaction.c,v 1.20.2.4 2002/02/28 04:12:57 nathanw Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: linux_sigaction.c,v 1.20.2.4 2002/02/28 04:12:57 nathanw Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/lwp.h>
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

/* Used on: alpha (aka osf_sigaction), arm, i386, m68k, mips, ppc */
/* Not used on: sparc, sparc64 */

/*
 * The Linux sigaction() system call. Do the usual conversions,
 * and just call sigaction().
 */
int
linux_sys_sigaction(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct linux_sys_sigaction_args /* {
		syscallarg(int) signum;
		syscallarg(const struct linux_old_sigaction *) nsa;
		syscallarg(struct linux_old_sigaction *) osa;
	} */ *uap = v;
	struct proc *p = l->l_proc;
	struct linux_old_sigaction nlsa, olsa;
	struct sigaction nbsa, obsa;
	int error, sig;

	/* XXX XAX handle switch to RT signal handler */
	/* XXX XAX and update emuldata->ps_siginfo. */

	if (SCARG(uap, nsa)) {
		error = copyin(SCARG(uap, nsa), &nlsa, sizeof(nlsa));
		if (error)
			return (error);
		linux_old_to_native_sigaction(&nbsa, &nlsa);
	}
	sig = SCARG(uap, signum);
	if (sig < 0 || sig >= LINUX__NSIG)
		return (EINVAL);
	if (sig > 0 && !linux_to_native_sig[sig]) {
		/* Pretend that we did something useful for unknown signals. */
		obsa.sa_handler = SIG_IGN;
		sigemptyset(&obsa.sa_mask);
		obsa.sa_flags = 0;
	} else {
		error = sigaction1(p, linux_to_native_sig[sig],
		    SCARG(uap, nsa) ? &nbsa : 0, SCARG(uap, osa) ? &obsa : 0);
		if (error)
			return (error);
	}
	if (SCARG(uap, osa)) {
		native_to_linux_old_sigaction(&olsa, &obsa);
		error = copyout(&olsa, SCARG(uap, osa), sizeof(olsa));
		if (error)
			return (error);
	}
	return (0);
}
