/*-
 * Copyright (c) 2012 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Matt Thomas of 3am Software Foundry.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
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

__KERNEL_RCSID(1, "$NetBSD: kern_sa_60.c,v 1.1.42.3 2018/03/18 09:00:55 pgoyette Exp $");

#include <sys/systm.h>
#include <sys/syscall.h>
#include <sys/syscallvar.h>
#include <sys/syscallargs.h>

#include <compat/common/compat_mod.h>

static const struct syscall_package kern_sa_60_syscalls[] = {
	{ SYS_compat_60_sa_register, 0,
	     (sy_call_t *)compat_60_sys_sa_register },
	{ SYS_compat_60_sa_stacks, 0, (sy_call_t *)compat_60_sys_sa_stacks },
	{ SYS_compat_60_sa_enable, 0, (sy_call_t *)compat_60_sys_sa_enable },
	{ SYS_compat_60_sa_setconcurrency, 0,
	     (sy_call_t *)compat_60_sys_sa_setconcurrency },
	{ SYS_compat_60_sa_yield, 0, (sy_call_t *)compat_60_sys_sa_yield },
	{ SYS_compat_60_sa_preempt, 0, (sy_call_t *)compat_60_sys_sa_preempt },
	{ 0, 0, NULL }
};

int
compat_60_sys_sa_register(lwp_t *l,
	const struct compat_60_sys_sa_register_args *uap,
	register_t *retval)
{
	return sys_nosys(l, uap, retval);
}

int
compat_60_sys_sa_stacks(lwp_t *l,
	const struct compat_60_sys_sa_stacks_args *uap,
	register_t *retval)
{
	return sys_nosys(l, uap, retval);
}

int
compat_60_sys_sa_enable(lwp_t *l,
	const void *uap,
	register_t *retval)
{
	return sys_nosys(l, uap, retval);
}

int
compat_60_sys_sa_setconcurrency(lwp_t *l,
	const struct compat_60_sys_sa_setconcurrency_args *uap,
	register_t *retval)
{
	return sys_nosys(l, uap, retval);
}

int
compat_60_sys_sa_yield(lwp_t *l,
	const void *uap,
	register_t *retval)
{
	return sys_nosys(l, uap, retval);
}

int
compat_60_sys_sa_preempt(lwp_t *l,
	const struct compat_60_sys_sa_preempt_args *uap,
	register_t *retval)
{
	return sys_nosys(l, uap, retval);
}

int
kern_sa_60_init(void)
{

	return syscall_establish(NULL, kern_sa_60_syscalls);
}

int
kern_sa_60_fini(void)
{

	return syscall_disestablish(NULL, kern_sa_60_syscalls);
}
