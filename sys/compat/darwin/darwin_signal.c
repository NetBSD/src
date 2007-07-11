/*	$NetBSD: darwin_signal.c,v 1.24.4.1 2007/07/11 20:03:53 mjf Exp $ */

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
__KERNEL_RCSID(0, "$NetBSD: darwin_signal.c,v 1.24.4.1 2007/07/11 20:03:53 mjf Exp $");

#include <sys/types.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/mount.h>
#include <sys/proc.h>
#include <sys/signal.h>
#include <sys/syscallargs.h>

#include <compat/sys/signal.h>
#include <compat/sys/signalvar.h>

#include <compat/common/compat_util.h>

#include <compat/mach/mach_types.h>
#include <compat/mach/mach_vm.h>
#include <compat/mach/mach_port.h>
#include <compat/mach/mach_exception.h>

#include <compat/darwin/darwin_audit.h>
#include <compat/darwin/darwin_exec.h>
#include <compat/darwin/darwin_signal.h>
#include <compat/darwin/darwin_syscallargs.h>

int
darwin_sys_sigaction(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct darwin_sys_sigaction_args /* {
		syscallarg(int) signum;
		syscallarg(struct darwin___sigaction *) nsa;
		syscallarg(struct sigaction13 *) osa;
	} */ *uap = v;
	struct darwin___sigaction dsa;
	struct sigaction nsa, osa;
	struct sigaction13 sa13;
	int error;

	if ((error = copyin(SCARG(uap, nsa), &dsa, sizeof(dsa))) != 0)
		return error;

	nsa.sa_handler = dsa.darwin_sa_handler.__sa_handler;
	native_sigset13_to_sigset(&dsa.darwin_sa_mask, &nsa.sa_mask);
	if (dsa.darwin_sa_flags & ~DARWIN_SA_ALLBITS) {
		DPRINTF(("darwin_sys_sigaction: ignoring bits (flags = %x)\n",
		    dsa.darwin_sa_flags));
	}
	nsa.sa_flags = dsa.darwin_sa_flags & DARWIN_SA_ALLBITS;

	error = sigaction1(l, SCARG(uap, signum), &nsa, &osa,
	    dsa.darwin_sa_tramp, 1);
	if (error != 0)
		return error;

	if (SCARG(uap, osa) == NULL)
		return 0;

	/* XXX: The returned structure has a different type to that supplied */
	sa13.osa_handler = osa.sa_handler;
	sa13.osa_mask = osa.sa_mask.__bits[0];
	native_sigset_to_sigset13(&osa.sa_mask, &sa13.osa_mask);
	sa13.osa_flags = osa.sa_flags;

	return copyout(&sa13, SCARG(uap, osa), sizeof(sa13));
}

void
darwin_trapsignal(l, ksi)
	struct lwp *l;
	struct ksiginfo *ksi;
{
	if (mach_trapsignal1(l, ksi) != 0)
		trapsignal(l, ksi);

	return;
}

int
darwin_tracesig(p, signo)
	struct proc *p;
	int signo;
{
	struct darwin_emuldata *ded;
	struct lwp *l;
	int code[2];
	int error;

	/*
	 * If the process does not have softsignals,
	 * we are done, normal signal delivery should
	 * occur.
	 */
	ded = (struct darwin_emuldata *)p->p_emuldata;
	if ((ded->ded_flags & DARWIN_DED_SIGEXC) == 0)
		return 0;

	code[0] = MACH_SOFT_SIGNAL;
	code[1] = signo;
	l = proc_representative_lwp(p, NULL, 0);
	error = mach_exception(l, MACH_EXC_SOFTWARE, code);

	/* Inhibit normal signal delivery */
	return EINVAL;
}

int
darwin_sys_sigprocmask(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct darwin_sys_sigprocmask_args /* {
		syscallarg(int) how;
		syscallarg(sigset13_t *) set;
		syscallarg(sigset13_t *) oset;
	} */ *uap = v;
	int error;
	sigset13_t kdset;
	sigset_t kbset, kboset;

	if (SCARG(uap, set) != NULL) {
		error = copyin(SCARG(uap, set), &kdset, sizeof(kdset));
		if (error != 0)
			return error;
		native_sigset13_to_sigset(&kdset, &kbset);
		error = sigprocmask1(l, SCARG(uap, how), &kbset, &kboset);
	} else
		error = sigprocmask1(l, SCARG(uap, how), NULL, &kboset);

	if (SCARG(uap, oset) == NULL || error != 0)
		return error;

	native_sigset_to_sigset13(&kboset, &kdset);
	return copyout(&kdset, SCARG(uap, oset), sizeof(kdset));
}

void
native_to_darwin_siginfo(ksi, dsi)
	const struct ksiginfo *ksi;
	struct darwin___siginfo *dsi;
{
	dsi->darwin_si_signo = ksi->ksi_signo;
	dsi->darwin_si_errno = ksi->ksi_errno;
	dsi->darwin_si_code = ksi->ksi_code;
	dsi->darwin_si_pid = ksi->ksi_pid;
	dsi->darwin_si_uid = ksi->ksi_uid;
	dsi->darwin_si_status = ksi->ksi_status;
	dsi->darwin_si_addr = ksi->ksi_addr;
	(void)memcpy(&dsi->darwin_si_value,
	    &ksi->ksi_value, sizeof(dsi->darwin_si_value));
	dsi->darwin_si_band = ksi->ksi_band;

	return;
}
