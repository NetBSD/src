/*	$NetBSD: netbsd32_signal.c,v 1.6 2003/01/18 08:28:26 thorpej Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: netbsd32_signal.c,v 1.6 2003/01/18 08:28:26 thorpej Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/malloc.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/signalvar.h>
#include <sys/proc.h>

#include <compat/netbsd32/netbsd32.h>
#include <compat/netbsd32/netbsd32_syscallargs.h>

int
netbsd32_sigaction(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct netbsd32_sigaction_args /* {
		syscallarg(int) signum;
		syscallarg(const netbsd32_sigactionp_t) nsa;
		syscallarg(netbsd32_sigactionp_t) osa;
	} */ *uap = v;
	struct sigaction nsa, osa;
	struct netbsd32_sigaction *sa32p, sa32;
	int error;

	if (SCARG(uap, nsa)) {
		sa32p =
		    (struct netbsd32_sigaction *)NETBSD32PTR64(SCARG(uap, nsa));
		if (copyin(sa32p, &sa32, sizeof(sa32)))
			return EFAULT;
		nsa.sa_handler = (void *)NETBSD32PTR64(sa32.netbsd32_sa_handler);
		nsa.sa_mask = sa32.netbsd32_sa_mask;
		nsa.sa_flags = sa32.netbsd32_sa_flags;
	}
	error = sigaction1(l->l_proc, SCARG(uap, signum), 
			   SCARG(uap, nsa) ? &nsa : 0, 
			   SCARG(uap, osa) ? &osa : 0,
			   NULL, 0);
 
	if (error)
		return (error);

	if (SCARG(uap, osa)) {
		sa32.netbsd32_sa_handler = (netbsd32_sigactionp_t)(u_long)osa.sa_handler;
		sa32.netbsd32_sa_mask = osa.sa_mask;
		sa32.netbsd32_sa_flags = osa.sa_flags;
		sa32p =
		    (struct netbsd32_sigaction *)NETBSD32PTR64(SCARG(uap, osa));
		if (copyout(&sa32, sa32p, sizeof(sa32)))
			return EFAULT;
	}

	return (0);
}

int
netbsd32___sigaltstack14(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct netbsd32___sigaltstack14_args /* {
		syscallarg(const netbsd32_sigaltstackp_t) nss;
		syscallarg(netbsd32_sigaltstackp_t) oss;
	} */ *uap = v;
	struct netbsd32_sigaltstack s32;
	struct sigaltstack nss, oss;
	int error;

	if (SCARG(uap, nss)) {
		error = copyin((caddr_t)NETBSD32PTR64(SCARG(uap, nss)), &s32,
		    sizeof(s32));
		if (error)
			return (error);
		nss.ss_sp = (void *)NETBSD32PTR64(s32.ss_sp);
		nss.ss_size = (size_t)s32.ss_size;
		nss.ss_flags = s32.ss_flags;
	}
	error = sigaltstack1(l->l_proc,
	    SCARG(uap, nss) ? &nss : 0, SCARG(uap, oss) ? &oss : 0);
	if (error)
		return (error);
	if (SCARG(uap, oss)) {
		s32.ss_sp = (netbsd32_voidp)(u_long)oss.ss_sp;
		s32.ss_size = (netbsd32_size_t)oss.ss_size;
		s32.ss_flags = oss.ss_flags;
		error = copyout(&s32, (caddr_t)NETBSD32PTR64(SCARG(uap, oss)),
		    sizeof(s32));
		if (error)
			return (error);
	}
	return (0);
}

/* ARGSUSED */
int
netbsd32___sigaction14(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct netbsd32___sigaction14_args /* {
		syscallarg(int) signum;
		syscallarg(const struct sigaction *) nsa;
		syscallarg(struct sigaction *) osa;
	} */ *uap = v;
	struct netbsd32_sigaction sa32;
	struct sigaction nsa, osa;
	int error;

	if (SCARG(uap, nsa)) {
		error = copyin((caddr_t)NETBSD32PTR64(SCARG(uap, nsa)), &sa32,
		    sizeof(sa32));
		if (error)
			return (error);
		nsa.sa_handler = (void *)NETBSD32PTR64(sa32.netbsd32_sa_handler);
		nsa.sa_mask = sa32.netbsd32_sa_mask;
		nsa.sa_flags = sa32.netbsd32_sa_flags;
	}
	error = sigaction1(l->l_proc, SCARG(uap, signum),
	    SCARG(uap, nsa) ? &nsa : 0, SCARG(uap, osa) ? &osa : 0,
	    NULL, 0);
	if (error)
		return (error);
	if (SCARG(uap, osa)) {
		sa32.netbsd32_sa_handler = (netbsd32_voidp)(u_long)osa.sa_handler;
		sa32.netbsd32_sa_mask = osa.sa_mask;
		sa32.netbsd32_sa_flags = osa.sa_flags;
		error = copyout(&sa32, (caddr_t)NETBSD32PTR64(SCARG(uap, osa)),
		    sizeof(sa32));
		if (error)
			return (error);
	}
	return (0);
}

/* ARGSUSED */
int
netbsd32___sigaction_sigtramp(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct netbsd32___sigaction_sigtramp_args /* {
		syscallarg(int) signum;
		syscallarg(const netbsd32_sigactionp_t) nsa;
		syscallarg(netbsd32_sigactionp_t) osa;
		syscallarg(netbsd32_voidp) tramp;
		syscallarg(int) vers;
	} */ *uap = v;
	struct proc *p = l->l_proc;
	struct netbsd32_sigaction sa32;
	struct sigaction nsa, osa;
	int error;

	if (SCARG(uap, nsa)) {
		error = copyin((caddr_t)NETBSD32PTR64(SCARG(uap, nsa)), &sa32,
		    sizeof(sa32));
		if (error)
			return (error);
		nsa.sa_handler = (void *)NETBSD32PTR64(sa32.netbsd32_sa_handler);
		nsa.sa_mask = sa32.netbsd32_sa_mask;
		nsa.sa_flags = sa32.netbsd32_sa_flags;
	}
	error = sigaction1(p, SCARG(uap, signum),
	    SCARG(uap, nsa) ? &nsa : 0, SCARG(uap, osa) ? &osa : 0,
	    NETBSD32PTR64(SCARG(uap, tramp)), SCARG(uap, vers));
	if (error)
		return (error);
	if (SCARG(uap, osa)) {
		sa32.netbsd32_sa_handler = (netbsd32_voidp)(u_long)osa.sa_handler;
		sa32.netbsd32_sa_mask = osa.sa_mask;
		sa32.netbsd32_sa_flags = osa.sa_flags;
		error = copyout(&sa32, (caddr_t)NETBSD32PTR64(SCARG(uap, osa)),
		    sizeof(sa32));
		if (error)
			return (error);
	}
	return (0);
}
