/*	$NetBSD: kern_idle.c,v 1.1.2.9 2007/05/13 17:02:58 ad Exp $	*/

/*-
 * Copyright (c)2002, 2006, 2007 YAMAMOTO Takashi,
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
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>

__KERNEL_RCSID(0, "$NetBSD: kern_idle.c,v 1.1.2.9 2007/05/13 17:02:58 ad Exp $");

#include <sys/param.h>
#include <sys/cpu.h>
#include <sys/idle.h>
#include <sys/lwp.h>
#include <sys/lockdebug.h>
#include <sys/kmem.h>

#include <uvm/uvm.h>
#include <uvm/uvm_extern.h>

#define	PIDLELWP	(MAXPRI + 1)	/* lowest priority */

void
idle_loop(void *dummy)
{
#if defined(DIAGNOSTIC)
	struct cpu_info *ci = curcpu();
#endif /* defined(DIAGNOSTIC) */
	struct lwp *l = curlwp;

	KERNEL_UNLOCK_ALL(l, NULL);
	l->l_usrpri = PIDLELWP;
	l->l_stat = LSONPROC;
	while (1 /* CONSTCOND */) {
		KERNEL_LOCK_ASSERT_UNLOCKED();
		LOCKDEBUG_BARRIER(NULL, 0);
		KASSERT((l->l_flag & LW_IDLE) != 0);
		KASSERT(ci == curcpu());
		KASSERT(l == curlwp);
		KASSERT(CURCPU_IDLE_P());
		KASSERT(l->l_usrpri == PIDLELWP);

		if (uvm.page_idle_zero) {
			if (sched_curcpu_runnable_p()) {
				goto schedule;
			}
			uvm_pageidlezero();
		}
		if (!sched_curcpu_runnable_p()) {
			cpu_idle();
			if (!sched_curcpu_runnable_p()) {
				continue;
			}
		}
schedule:
		KASSERT(l->l_mutex == &l->l_cpu->ci_schedstate.spc_lwplock);
		lwp_lock(l);
		mi_switch(l);
		KASSERT(curlwp == l);
		KASSERT(l->l_stat == LSONPROC);
	}
}

int
create_idle_lwp(struct cpu_info *ci)
{
	struct proc *p = &proc0;
	struct lwp *l;
	char *name;
	vaddr_t uaddr;
	bool inmem;
	cpuid_t cpuid;
	int error;

	KASSERT(ci->ci_data.cpu_idlelwp == NULL);
	inmem = uvm_uarea_alloc(&uaddr);
	if (uaddr == 0) {
		return ENOMEM;
	}
	error = newlwp(&lwp0, p, uaddr, inmem, 0, NULL, 0, idle_loop, NULL, &l);
	if (error != 0) {
		panic("create_idle_lwp: newlwp failed");
	}
	PHOLD(l);
	l->l_flag |= (LW_IDLE | LW_BOUND);
	l->l_cpu = ci;
	l->l_mutex = &ci->ci_schedstate.spc_lwplock;
	ci->ci_data.cpu_idlelwp = l;
	name = kmem_alloc(MAXCOMLEN, KM_NOSLEEP);
	if (name != NULL) {
#ifdef MULTIPROCESSOR		/* XXX should be mandatory */
		cpuid = ci->ci_cpuid;
#else
		cpuid = 0;
#endif
		snprintf(name, MAXCOMLEN, "idle:%d", (int)cpuid);
		lwp_lock(l);
		l->l_name = name;
		lwp_unlock(l);
	}
	return error;
}
