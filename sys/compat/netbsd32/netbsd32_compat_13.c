/*	$NetBSD: netbsd32_compat_13.c,v 1.9 2001/08/25 15:06:02 mrg Exp $	*/

/*
 * Copyright (c) 1998, 2001 Matthew R. Green
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/mount.h>
#include <sys/proc.h>
#include <sys/signal.h>
#include <sys/signalvar.h>
#include <sys/syscallargs.h>

#include <compat/netbsd32/netbsd32.h>
#include <compat/netbsd32/netbsd32_syscallargs.h>

int
compat_13_netbsd32_sigaltstack13(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct compat_13_netbsd32_sigaltstack13_args /* {
		syscallarg(const netbsd32_sigaltstack13p_t) nss;
		syscallarg(netbsd32_sigaltstack13p_t) oss;
	} */ *uap = v;
	struct compat_13_sys_sigaltstack_args ua;
	struct sigaltstack13 ss13, *nss13up, *oss13up;
	struct netbsd32_sigaltstack13 s32ss;
	caddr_t sg;
	int error;

	if (!SCARG(uap, nss))
		return (EINVAL);

	sg = stackgap_init(p->p_emul);

	SCARG(&ua, nss) = nss13up = stackgap_alloc(&sg, sizeof(*nss13up));
	if (SCARG(uap, oss))
		SCARG(&ua, oss) = oss13up = stackgap_alloc(&sg, sizeof(*oss13up));
	else
		SCARG(&ua, oss) = NULL;

	error = copyin((caddr_t)(u_long)SCARG(uap, nss), &s32ss, sizeof s32ss);
	if (error)
		return (error);
	ss13.ss_sp = (char *)(u_long)s32ss.ss_sp;
	ss13.ss_size = s32ss.ss_size;
	ss13.ss_flags = s32ss.ss_flags;
	error = copyout(&ss13, nss13up, sizeof *nss13up);
	if (error)
		return (error);

	error = compat_13_sys_sigaltstack(p, &ua, retval);
	if (error)
		return (error);

	if (SCARG(uap, oss)) {
		error = copyin(nss13up, &ss13, sizeof *nss13up);
		if (error)
			return (error);
		s32ss.ss_sp = (netbsd32_charp)(u_long)ss13.ss_sp;
		s32ss.ss_size = ss13.ss_size;
		s32ss.ss_flags = ss13.ss_flags;
		error = copyout(&s32ss, (caddr_t)(u_long)SCARG(uap, nss), sizeof s32ss);
		if (error)
			return (error);
	}

	return (0);
}


int
compat_13_netbsd32_sigprocmask(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct compat_13_netbsd32_sigprocmask_args /* {
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
compat_13_netbsd32_sigsuspend(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct compat_13_netbsd32_sigsuspend_args /* {
		syscallarg(sigset13_t) mask;
	} */ *uap = v;
	sigset13_t ess;
	sigset_t bss;

	ess = SCARG(uap, mask);
	native_sigset13_to_sigset(&ess, &bss);
	return (sigsuspend1(p, &bss));
}
