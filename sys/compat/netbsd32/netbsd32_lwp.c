/*	$NetBSD: netbsd32_lwp.c,v 1.1.6.2 2007/02/27 16:53:38 yamt Exp $	*/

/*
 *  Copyright (c) 2005, 2006, 2007 The NetBSD Foundation.
 *  All rights reserved.
 *
 *  This code is derived from software contributed to the NetBSD Foundation
 *   by Quentin Garnier.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *  1. Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *  2. Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *  3. Neither the name of The NetBSD Foundation nor the names of its
 *     contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 *  ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 *  TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 *  PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 *  BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 *  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 *  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 *  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 *  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 *  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *  POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: netbsd32_lwp.c,v 1.1.6.2 2007/02/27 16:53:38 yamt Exp $");

#include <sys/types.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/dirent.h>
#include <sys/mount.h>
#include <sys/proc.h>
#include <sys/syscallargs.h>

#include <compat/netbsd32/netbsd32.h>
#include <compat/netbsd32/netbsd32_syscallargs.h>
#include <compat/netbsd32/netbsd32_conv.h>

/* Sycalls conversion */

int
netbsd32__lwp_create(struct lwp *l, void *v, register_t *retval)
{
	struct netbsd32__lwp_create_args /* {
		syscallarg(const netbsd32_ucontextp) ucp;
		syscallarg(netbsd32_u_long) flags;
		syscallarg(netbsd32_lwpidp) new_lwp;
	} */ *uap = v;
	struct sys__lwp_create_args ua;

	NETBSD32TOP_UAP(ucp, const ucontext_t);
	NETBSD32TO64_UAP(flags);
	NETBSD32TOP_UAP(new_lwp, lwpid_t);

	return sys__lwp_create(l, &ua, retval);
}

int
netbsd32__lwp_wait(struct lwp *l, void *v, register_t *retval)
{
	struct netbsd32__lwp_wait_args /* {
		syscallarg(lwpid_t) wait_for;
		syscallarg(netbsd32_lwpidp) departed;
	} */ *uap = v;
	struct sys__lwp_wait_args ua;

	NETBSD32TO64_UAP(wait_for);
	NETBSD32TOP_UAP(departed, lwpid_t);
	return sys__lwp_wait(l, &ua, retval);
}

int
netbsd32__lwp_suspend(struct lwp *l, void *v, register_t *retval)
{
	struct netbsd32__lwp_suspend_args /* {
		syscallarg(lwpid_t) target;
	} */ *uap = v;
	struct sys__lwp_suspend_args ua;

	NETBSD32TO64_UAP(target);
	return sys__lwp_suspend(l, &ua, retval);
}

int
netbsd32__lwp_continue(struct lwp *l, void *v, register_t *retval)
{
	struct netbsd32__lwp_continue_args /* {
		syscallarg(lwpid_t) target;
	} */ *uap = v;
	struct sys__lwp_continue_args ua;

	NETBSD32TO64_UAP(target);
	return sys__lwp_continue(l, &ua, retval);
}

int
netbsd32__lwp_wakeup(struct lwp *l, void *v, register_t *retval)
{
	struct netbsd32__lwp_wakeup_args /* {
		syscallarg(lwpid_t) target;
	} */ *uap = v;
	struct sys__lwp_wakeup_args ua;

	NETBSD32TO64_UAP(target);
	return sys__lwp_wakeup(l, &ua, retval);
}

int
netbsd32__lwp_setprivate(struct lwp *l, void *v, register_t *retval)
{
	struct netbsd32__lwp_setprivate_args /* {
		syscallarg(netbsd32_voidp) ptr;
	} */ *uap = v;
	struct sys__lwp_setprivate_args ua;

	NETBSD32TOP_UAP(ptr, void);
	return sys__lwp_setprivate(l, &ua, retval);
}

int
netbsd32__lwp_park(struct lwp *l, void *v, register_t *retval)
{
	struct netbsd32__lwp_park_args /* {
		syscallarg(const netbsd32_timespecp) ts;
		syscallarg(netbsd32_ucontextp) ucp;
		syscallarg(netbsd32_voidp) hint;
	} */ *uap = v;
	struct sys__lwp_park_args ua;
	struct timespec *tsp = NULL;

	if (SCARG(uap, ts)) {
		struct timespec ts;
		struct netbsd32_timespec ts32;
		int error;
		caddr_t sg;

		if ((error = copyin(NETBSD32PTR64(SCARG(uap, ts)), &ts32,
		    sizeof ts32)) != 0)
			return error;
		netbsd32_to_timespec(&ts32, &ts);

		sg = stackgap_init(l->l_proc, sizeof(ts));
		tsp = (struct timespec *)stackgap_alloc(l->l_proc, &sg,
		    sizeof(ts));
		if ((error = copyout(&ts, tsp, sizeof(ts))) != 0)
			return error;
	}

	/*
	 * sys__lwp_park() ignores the ucontext_t argument, so we won't be
	 * doing anything to it for now.  While that situation could change,
	 * there's still a good chance it will only be considered as an
	 * opaque value.
	 */

	SCARG(&ua, ts) = tsp;
	NETBSD32TOP_UAP(ucp, ucontext_t);
	NETBSD32TOP_UAP(hint, void);
	return sys__lwp_park(l, &ua, retval);
}

int
netbsd32__lwp_kill(struct lwp *l, void *v, register_t *retval)
{
	struct netbsd32__lwp_kill_args /* {
		syscallarg(lwpid_t) target;
		syscallarg(int) signo;
	} */ *uap = v;
	struct sys__lwp_kill_args ua;

	NETBSD32TO64_UAP(target);
	NETBSD32TO64_UAP(signo);
	return sys__lwp_kill(l, &ua, retval);
}
int
netbsd32__lwp_detach(struct lwp *l, void *v, register_t *retval)
{
	struct netbsd32__lwp_detach_args /* {
		syscallarg(lwpid_t) target;
	} */ *uap = v;
	struct sys__lwp_detach_args ua;

	NETBSD32TO64_UAP(target);
	return sys__lwp_detach(l, &ua, retval);
}

int
netbsd32__lwp_unpark(struct lwp *l, void *v, register_t *retval)
{
	struct netbsd32__lwp_unpark_args /* {
		syscallarg(lwpid_t) target;
		syscallarg(netbsd32_voidp) hint;
	} */ *uap = v;
	struct sys__lwp_unpark_args ua;

	NETBSD32TO64_UAP(target);
	NETBSD32TOP_UAP(hint, void);
	return sys__lwp_unpark(l, &ua, retval);
}

int
netbsd32__lwp_unpark_all(struct lwp *l, void *v, register_t *retval)
{
	struct netbsd32__lwp_unpark_all_args /* {
		syscallarg(const netbsd32_lwpidp) targets;
		syscallarg(netbsd32_size_t) ntargets;
		syscallarg(netbsd32_voidp) hint;
	} */ *uap = v;
	struct sys__lwp_unpark_all_args ua;

	NETBSD32TOP_UAP(targets, const lwpid_t);
	NETBSD32TOX_UAP(ntargets, size_t);
	NETBSD32TOP_UAP(hint, void);
	return sys__lwp_unpark_all(l, &ua, retval);
}
