/*	$NetBSD: netbsd32_sa.c,v 1.11 2009/12/10 14:10:35 matt Exp $	*/

/*
 *  Copyright (c) 2005 The NetBSD Foundation.
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
__KERNEL_RCSID(0, "$NetBSD: netbsd32_sa.c,v 1.11 2009/12/10 14:10:35 matt Exp $");

#include <sys/types.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/dirent.h>
#include <sys/mount.h>
#include <sys/proc.h>
#include <sys/sa.h>
#include <sys/savar.h>
#include <sys/syscallargs.h>

#include <compat/netbsd32/netbsd32.h>
#include <compat/netbsd32/netbsd32_syscallargs.h>
#include <compat/netbsd32/netbsd32_conv.h>
#include <compat/netbsd32/netbsd32_sa.h>

const struct sa_emul saemul_netbsd32 = {
	sizeof(ucontext32_t),
	sizeof(struct netbsd32_sa_t),
	sizeof(netbsd32_sa_tp),
	netbsd32_sacopyout,  
	netbsd32_upcallconv,
	netbsd32_cpu_upcall,
	(void (*)(struct lwp *, void *))getucontext32_sa,
#ifdef KERN_SA
	netbsd32_sa_ucsp
#else
	NULL
#endif
}; 

/* SA emulation helpers */
int
netbsd32_sacopyout(int type, const void *src, void *dst)
{
	switch (type) {
	case SAOUT_UCONTEXT:
		{
			const ucontext_t *u = src;
			ucontext32_t u32;

			memset(&u32, 0, sizeof(u32));
			u32.uc_flags = u->uc_flags;
			u32.uc_stack.ss_sp = (uintptr_t)u->uc_stack.ss_sp;
			u32.uc_stack.ss_size = u->uc_stack.ss_size;
			u32.uc_stack.ss_flags = u->uc_stack.ss_flags;

			return copyout(&u32, dst, sizeof(u32));
		} break;
	case SAOUT_SA_T:
		{
			const struct sa_t *sa = src;
			struct netbsd32_sa_t sa32;

			sa32.sa_id = sa->sa_id;
			sa32.sa_cpu = sa->sa_cpu;
			NETBSD32PTR32(sa32.sa_context, sa->sa_context);

			return copyout(&sa32, dst, sizeof(sa32));
		} break;
	case SAOUT_SAP_T:
		{
			void * const *p = src;
			netbsd32_pointer_t p32;

			NETBSD32PTR32(p32, *p);
			return copyout(&p32, dst, sizeof(p32));
		} break;
	}
	return EINVAL;
}

int
netbsd32_upcallconv(struct lwp *l, int type, size_t *pargsize, void **parg,
    void (**pfunc)(void *))
{
	switch (type & SA_UPCALL_TYPE_MASK) {
	case SA_UPCALL_SIGNAL:
	case SA_UPCALL_SIGEV:
		{
			siginfo32_t si32;
			siginfo_t *si = *parg;

			netbsd32_si_to_si32(&si32, si);

			/*
			 * This is so wrong, but assuming
			 * sizeof(siginfo32_t) < sizeof(siginfo_t) is not
			 * very dangerous.
			 */
			memcpy(*parg, &si32, sizeof(si32));
			*pargsize = sizeof(si32);
		}
	}

	return 0;
}

void *
netbsd32_sa_ucsp(void *arg)
{
	ucontext32_t *uc32 = arg;

	return NETBSD32IPTR64(_UC_MACHINE32_SP(uc32));
}

/* Sycalls conversion */

int
netbsd32_sa_register(struct lwp *l,
    const struct netbsd32_sa_register_args *uap, register_t *retval)
{
#ifdef COMPAT_40
	/* {
		syscallarg(netbsd32_sa_upcall_t) new;
		syscallarg(netbsd32_sa_upcallp_t) old;
		syscallarg(int) flags;
		syscallarg(netbsd32_ssize_t) stackinfo_offset;
	} */
	sa_upcall_t prev;
	int error;

	error = dosa_register(l, NETBSD32PTR64(SCARG(uap, new)), &prev,
	    SCARG(uap, flags), SCARG(uap, stackinfo_offset));
	if (error)
		return error;

	if (NETBSD32PTR64(SCARG(uap, old))) {
		
		netbsd32_sa_upcall_t old;
		NETBSD32PTR32(old, prev);
		return copyout(&old, NETBSD32PTR64(SCARG(uap, old)),
		    sizeof(old));
	}

	return 0;
#else
	return ENOSYS;
#endif	
}

#ifdef COMPAT_40
static int
netbsd32_sa_copyin_stack(stack_t *stacks, int index, stack_t *dest)
{
	stack32_t s32, *stacks32;
	int error;

	stacks32 = (stack32_t *)stacks;
	error = copyin(stacks32 + index, &s32, sizeof(s32));
	if (error)
		return error;

 	dest->ss_sp = NETBSD32IPTR64(s32.ss_sp);
	dest->ss_size = s32.ss_size;
	dest->ss_flags = s32.ss_flags;

	return 0;
}
#endif

int
netbsd32_sa_stacks(struct lwp *l, const struct netbsd32_sa_stacks_args *uap,
    register_t *retval)
{
#ifdef COMPAT_40
	 /* {
		syscallarg(int) num;
		syscallarg(netbsd32_stackp_t) stacks;
	} */

	return sa_stacks1(l, retval, SCARG(uap, num),
	    NETBSD32PTR64(SCARG(uap, stacks)), netbsd32_sa_copyin_stack);
#else
	return ENOSYS;
#endif
}

int
netbsd32_sa_setconcurrency(struct lwp *l,
    const struct netbsd32_sa_setconcurrency_args *uap, register_t *retval)
{
	/* {
		syscallarg(int) concurrency;
	} */
	struct sys_sa_setconcurrency_args ua;

	NETBSD32TO64_UAP(concurrency);
	return sys_sa_setconcurrency(l, &ua, retval);
}

int
netbsd32_sa_preempt(struct lwp *l, const struct netbsd32_sa_preempt_args *uap,
    register_t *retval)
{
	 /* {
		syscallarg(int) sa_id;
	} */
	struct sys_sa_preempt_args ua;

	NETBSD32TO64_UAP(sa_id);
	return sys_sa_preempt(l, &ua, retval);
}
