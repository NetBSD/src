/*	$NetBSD: netbsd32_lwp.c,v 1.7.8.2 2007/12/26 21:39:06 ad Exp $	*/

/*
 *  Copyright (c) 2005, 2006, 2007 The NetBSD Foundation.
 *  All rights reserved.
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
__KERNEL_RCSID(0, "$NetBSD: netbsd32_lwp.c,v 1.7.8.2 2007/12/26 21:39:06 ad Exp $");

#include <sys/types.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/dirent.h>
#include <sys/mount.h>
#include <sys/proc.h>
#include <sys/syscallargs.h>
#include <sys/lwpctl.h>

#include <compat/netbsd32/netbsd32.h>
#include <compat/netbsd32/netbsd32_syscallargs.h>
#include <compat/netbsd32/netbsd32_conv.h>

/* Sycalls conversion */

int
netbsd32__lwp_create(struct lwp *l, const struct netbsd32__lwp_create_args *uap, register_t *retval)
{
	/* {
		syscallarg(const netbsd32_ucontextp) ucp;
		syscallarg(netbsd32_u_long) flags;
		syscallarg(netbsd32_lwpidp) new_lwp;
	} */
	struct sys__lwp_create_args ua;

	NETBSD32TOP_UAP(ucp, const ucontext_t);
	NETBSD32TO64_UAP(flags);
	NETBSD32TOP_UAP(new_lwp, lwpid_t);

	return sys__lwp_create(l, &ua, retval);
}

int
netbsd32__lwp_wait(struct lwp *l, const struct netbsd32__lwp_wait_args *uap, register_t *retval)
{
	/* {
		syscallarg(lwpid_t) wait_for;
		syscallarg(netbsd32_lwpidp) departed;
	} */
	struct sys__lwp_wait_args ua;

	NETBSD32TO64_UAP(wait_for);
	NETBSD32TOP_UAP(departed, lwpid_t);
	return sys__lwp_wait(l, &ua, retval);
}

int
netbsd32__lwp_suspend(struct lwp *l, const struct netbsd32__lwp_suspend_args *uap, register_t *retval)
{
	/* {
		syscallarg(lwpid_t) target;
	} */
	struct sys__lwp_suspend_args ua;

	NETBSD32TO64_UAP(target);
	return sys__lwp_suspend(l, &ua, retval);
}

int
netbsd32__lwp_continue(struct lwp *l, const struct netbsd32__lwp_continue_args *uap, register_t *retval)
{
	/* {
		syscallarg(lwpid_t) target;
	} */
	struct sys__lwp_continue_args ua;

	NETBSD32TO64_UAP(target);
	return sys__lwp_continue(l, &ua, retval);
}

int
netbsd32__lwp_wakeup(struct lwp *l, const struct netbsd32__lwp_wakeup_args *uap, register_t *retval)
{
	/* {
		syscallarg(lwpid_t) target;
	} */
	struct sys__lwp_wakeup_args ua;

	NETBSD32TO64_UAP(target);
	return sys__lwp_wakeup(l, &ua, retval);
}

int
netbsd32__lwp_setprivate(struct lwp *l, const struct netbsd32__lwp_setprivate_args *uap, register_t *retval)
{
	/* {
		syscallarg(netbsd32_voidp) ptr;
	} */
	struct sys__lwp_setprivate_args ua;

	NETBSD32TOP_UAP(ptr, void);
	return sys__lwp_setprivate(l, &ua, retval);
}

int
netbsd32__lwp_park(struct lwp *l, const struct netbsd32__lwp_park_args *uap, register_t *retval)
{
	/* {
		syscallarg(const netbsd32_timespecp) ts;
		syscallarg(lwpid_t) unpark;
		syscallarg(netbsd32_voidp) hint;
		syscallarg(netbsd32_voidp) unparkhint;
	} */
	struct timespec ts, *tsp;
	struct netbsd32_timespec ts32;
	int error;

	if (SCARG_P32(uap, ts) == NULL)
		tsp = NULL;
	else {
		error = copyin(SCARG_P32(uap, ts), &ts32, sizeof ts32);
		if (error != 0)
			return error;
		netbsd32_to_timespec(&ts32, &ts);
		tsp = &ts;
	}

	if (SCARG(uap, unpark) != 0) {
		error = lwp_unpark(SCARG(uap, unpark),
		    SCARG_P32(uap, unparkhint));
		if (error != 0)
			return error;
	}

	return lwp_park(tsp, SCARG_P32(uap, hint));
}

int
netbsd32__lwp_kill(struct lwp *l, const struct netbsd32__lwp_kill_args *uap, register_t *retval)
{
	/* {
		syscallarg(lwpid_t) target;
		syscallarg(int) signo;
	} */
	struct sys__lwp_kill_args ua;

	NETBSD32TO64_UAP(target);
	NETBSD32TO64_UAP(signo);
	return sys__lwp_kill(l, &ua, retval);
}
int
netbsd32__lwp_detach(struct lwp *l, const struct netbsd32__lwp_detach_args *uap, register_t *retval)
{
	/* {
		syscallarg(lwpid_t) target;
	} */
	struct sys__lwp_detach_args ua;

	NETBSD32TO64_UAP(target);
	return sys__lwp_detach(l, &ua, retval);
}

int
netbsd32__lwp_unpark(struct lwp *l, const struct netbsd32__lwp_unpark_args *uap, register_t *retval)
{
	/* {
		syscallarg(lwpid_t) target;
		syscallarg(netbsd32_voidp) hint;
	} */
	struct sys__lwp_unpark_args ua;

	NETBSD32TO64_UAP(target);
	NETBSD32TOP_UAP(hint, void);
	return sys__lwp_unpark(l, &ua, retval);
}

int
netbsd32__lwp_unpark_all(struct lwp *l, const struct netbsd32__lwp_unpark_all_args *uap, register_t *retval)
{
	/* {
		syscallarg(const netbsd32_lwpidp) targets;
		syscallarg(netbsd32_size_t) ntargets;
		syscallarg(netbsd32_voidp) hint;
	} */
	struct sys__lwp_unpark_all_args ua;

	NETBSD32TOP_UAP(targets, const lwpid_t);
	NETBSD32TOX_UAP(ntargets, size_t);
	NETBSD32TOP_UAP(hint, void);
	return sys__lwp_unpark_all(l, &ua, retval);
}

int
netbsd32__lwp_setname(struct lwp *l, const struct netbsd32__lwp_setname_args *uap, register_t *retval)
{
	/* {
		syscallarg(lwpid_t) target;
		syscallarg(const netbsd32_charp) name;
	} */
	struct sys__lwp_setname_args ua;

	NETBSD32TO64_UAP(target);
	NETBSD32TOP_UAP(name, char *);
	return sys__lwp_setname(l, &ua, retval);
}

int
netbsd32__lwp_getname(struct lwp *l, const struct netbsd32__lwp_getname_args *uap, register_t *retval)
{
	/* {
		syscallarg(lwpid_t) target;
		syscallarg(netbsd32_charp) name;
		syscallarg(netbsd32_size_t) len;
	} */
	struct sys__lwp_getname_args ua;

	NETBSD32TO64_UAP(target);
	NETBSD32TOP_UAP(name, char *);
	NETBSD32TOX_UAP(len, size_t);
	return sys__lwp_getname(l, &ua, retval);
}

int
netbsd32__lwp_ctl(struct lwp *l, const struct netbsd32__lwp_ctl_args *uap, register_t *retval)
{
	/* {
		syscallarg(int) features;
		syscallarg(netbsd32_pointer_t) address;
	} */
	struct sys__lwp_ctl_args ua;

	NETBSD32TO64_UAP(features);
	NETBSD32TOP_UAP(address, struct lwpctl *);
	return sys__lwp_ctl(l, &ua, retval);
}
