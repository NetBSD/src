/*	$NetBSD: kern_sig_13.c,v 1.5 2000/03/30 11:27:14 augustss Exp $	*/

/*-
 * Copyright (c) 1998 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Charles M. Hannum.
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

#include <sys/param.h>
#include <sys/proc.h>
#include <sys/signal.h>
#include <sys/signalvar.h>
#include <sys/systm.h>

#include <sys/mount.h>
#include <sys/syscallargs.h>

#include <machine/limits.h>

#include <compat/common/compat_util.h>

void
native_sigset13_to_sigset(oss, ss)
	const sigset13_t *oss;
	sigset_t *ss;
{

	ss->__bits[0] = *oss;
	ss->__bits[1] = 0;
	ss->__bits[2] = 0;
	ss->__bits[3] = 0;
}

void
native_sigset_to_sigset13(ss, oss)
	const sigset_t *ss;
	sigset13_t *oss;
{

	*oss = ss->__bits[0];
}

void
native_sigaction13_to_sigaction(osa, sa)
	const struct sigaction13 *osa;
	struct sigaction *sa;
{

	sa->sa_handler = osa->sa_handler;
	native_sigset13_to_sigset(&osa->sa_mask, &sa->sa_mask);
	sa->sa_flags = osa->sa_flags;
}

void
native_sigaction_to_sigaction13(sa, osa)
	const struct sigaction *sa;
	struct sigaction13 *osa;
{

	osa->sa_handler = sa->sa_handler;
	native_sigset_to_sigset13(&sa->sa_mask, &osa->sa_mask);
	osa->sa_flags = sa->sa_flags;
}

void
native_sigaltstack13_to_sigaltstack(osa, sa)
	const struct sigaltstack13 *osa;
	struct sigaltstack *sa;
{

	sa->ss_sp = osa->ss_sp;
	sa->ss_size = osa->ss_size;
	sa->ss_flags = osa->ss_flags;
}

void
native_sigaltstack_to_sigaltstack13(sa, osa)
	const struct sigaltstack *sa;
	struct sigaltstack13 *osa;
{

	osa->ss_sp = sa->ss_sp;
	osa->ss_size = sa->ss_size;
	osa->ss_flags = sa->ss_flags;
}

int
compat_13_sys_sigaltstack(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct compat_13_sys_sigaltstack_args /* {
		syscallarg(const struct sigaltstack13 *) nss;
		syscallarg(struct sigaltstack13 *) oss;
	} */ *uap = v;
	struct sigaltstack13 ness, oess;
	struct sigaltstack nbss, obss;
	int error;

	if (SCARG(uap, nss)) {
		error = copyin(SCARG(uap, nss), &ness, sizeof(ness));
		if (error)
			return (error);
		native_sigaltstack13_to_sigaltstack(&ness, &nbss);
	}
	error = sigaltstack1(p,
	    SCARG(uap, nss) ? &nbss : 0, SCARG(uap, oss) ? &obss : 0);
	if (error)
		return (error);
	if (SCARG(uap, oss)) {
		native_sigaltstack_to_sigaltstack13(&obss, &oess);
		error = copyout(&oess, SCARG(uap, oss), sizeof(oess));
		if (error)
			return (error);
	}
	return (0);
}

int
compat_13_sys_sigaction(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct compat_13_sys_sigaction_args /* {
		syscallarg(int) signum;
		syscallarg(const struct sigaction13 *) nsa;
		syscallarg(struct sigaction13 *) osa;
	} */ *uap = v;
	struct sigaction13 nesa, oesa;
	struct sigaction nbsa, obsa;
	int error;

	if (SCARG(uap, nsa)) {
		error = copyin(SCARG(uap, nsa), &nesa, sizeof(nesa));
		if (error)
			return (error);
		native_sigaction13_to_sigaction(&nesa, &nbsa);
	}
	error = sigaction1(p, SCARG(uap, signum),
	    SCARG(uap, nsa) ? &nbsa : 0, SCARG(uap, osa) ? &obsa : 0);
	if (error)
		return (error);
	if (SCARG(uap, osa)) {
		native_sigaction_to_sigaction13(&obsa, &oesa);
		error = copyout(&oesa, SCARG(uap, osa), sizeof(oesa));
		if (error)
			return (error);
	}
	return (0);
}

int
compat_13_sys_sigprocmask(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct compat_13_sys_sigprocmask_args /* {
		syscallarg(int) how;
		syscallarg(int) mask;
	} */ *uap = v;
	sigset13_t ness, oess;
	sigset_t nbss, obss;
	int error;

	ness = SCARG(uap, mask);
	native_sigset13_to_sigset(&ness, &nbss);
	error = sigprocmask1(p, SCARG(uap, how), &nbss, &obss);
	if (error)
		return (error);
	native_sigset_to_sigset13(&obss, &oess);
	*retval = oess;
	return (0);
}

int
compat_13_sys_sigpending(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	sigset13_t ess;
	sigset_t bss;

	sigpending1(p, &bss);
	native_sigset_to_sigset13(&bss, &ess);
	*retval = ess;
	return (0);
}

int
compat_13_sys_sigsuspend(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct compat_13_sys_sigsuspend_args /* {
		syscallarg(sigset13_t) mask;
	} */ *uap = v;
	sigset13_t ess;
	sigset_t bss;

	ess = SCARG(uap, mask);
	native_sigset13_to_sigset(&ess, &bss);
	return (sigsuspend1(p, &bss));
}
