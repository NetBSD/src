/*	$NetBSD: netbsd32_compat_13.c,v 1.20 2007/06/03 11:09:35 dsl Exp $	*/

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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: netbsd32_compat_13.c,v 1.20 2007/06/03 11:09:35 dsl Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/mount.h>
#include <sys/proc.h>
#include <sys/signal.h>
#include <sys/signalvar.h>
#include <sys/syscallargs.h>

#include <compat/netbsd32/netbsd32.h>
#include <compat/netbsd32/netbsd32_syscallargs.h>

#include <compat/sys/stat.h>
#include <compat/sys/signal.h>
#include <compat/sys/signalvar.h>

int
compat_13_netbsd32_sigaltstack13(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct compat_13_netbsd32_sigaltstack13_args /* {
		syscallarg(const netbsd32_sigaltstack13p_t) nss;
		syscallarg(netbsd32_sigaltstack13p_t) oss;
	} */ *uap = v;
	struct netbsd32_sigaltstack13 s32ss;
	struct sigaltstack nbss, obss;
	int error;

	if (SCARG(uap, nss)) {
		error = copyin(SCARG_P32(uap, nss), &s32ss, sizeof s32ss);
		if (error)
			return (error);
		nss.ss_sp = s32ss.ss_sp;
		nss.ss_size = s32ss.ss_size;
		nss.ss_flags = s32ss.ss_flags;
	}

	error = sigaltstack1(l,
	    SCARG(uap, nss) ? &nss : 0, SCARG(uap, oss) ? &oss : 0);
	if (error)
		return (error);

	if (SCARG(uap, oss)) {
		s32ss.ss_sp = oss.ss_sp;
		s32ss.ss_size =onss.ss_size;
		s32ss.ss_flags = oss.ss_flags;
		error = copyout(&s32ss, SCARG_P32(uap, oss), sizeof(s32ss));
		if (error)
			return (error);
	}
	return (0);
}


int
compat_13_netbsd32_sigprocmask(l, v, retval)
	struct lwp *l;
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
	error = sigprocmask1(l, SCARG(uap, how), &nbss, &obss);
	if (error)
		return (error);
	native_sigset_to_sigset13(&obss, &oess);
	*retval = oess;
	return (0);
}

int
compat_13_netbsd32_sigsuspend(l, v, retval)
	struct lwp *l;
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
	return (sigsuspend1(l, &bss));
}
