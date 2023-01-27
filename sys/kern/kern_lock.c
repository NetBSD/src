/*	$NetBSD: kern_lock.c,v 1.182 2023/01/27 09:28:41 ozaki-r Exp $	*/

/*-
 * Copyright (c) 2002, 2006, 2007, 2008, 2009, 2020 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe of the Numerical Aerospace Simulation Facility,
 * NASA Ames Research Center, and by Andrew Doran.
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
__KERNEL_RCSID(0, "$NetBSD: kern_lock.c,v 1.182 2023/01/27 09:28:41 ozaki-r Exp $");

#ifdef _KERNEL_OPT
#include "opt_lockdebug.h"
#endif

#include <sys/param.h>
#include <sys/proc.h>
#include <sys/lock.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/lockdebug.h>
#include <sys/cpu.h>
#include <sys/syslog.h>
#include <sys/atomic.h>
#include <sys/lwp.h>
#include <sys/pserialize.h>

#if defined(DIAGNOSTIC) && !defined(LOCKDEBUG)
#include <sys/ksyms.h>
#endif

#include <machine/lock.h>

#include <dev/lockstat.h>

#define	RETURN_ADDRESS	(uintptr_t)__builtin_return_address(0)

bool	kernel_lock_dodebug;

__cpu_simple_lock_t kernel_lock[CACHE_LINE_SIZE / sizeof(__cpu_simple_lock_t)]
    __cacheline_aligned;

void
assert_sleepable(void)
{
	const char *reason;
	uint64_t pctr;
	bool idle;

	if (__predict_false(panicstr != NULL)) {
		return;
	}

	LOCKDEBUG_BARRIER(kernel_lock, 1);

	/*
	 * Avoid disabling/re-enabling preemption here since this
	 * routine may be called in delicate situations.
	 */
	do {
		pctr = lwp_pctr();
		__insn_barrier();
		idle = CURCPU_IDLE_P();
		__insn_barrier();
	} while (pctr != lwp_pctr());

	reason = NULL;
	if (idle && !cold) {
		reason = "idle";
	}
	if (cpu_intr_p()) {
		reason = "interrupt";
	}
	if (cpu_softintr_p()) {
		reason = "softint";
	}
	if (!pserialize_not_in_read_section()) {
		reason = "pserialize";
	}

	if (reason) {
		panic("%s: %s caller=%p", __func__, reason,
		    (void *)RETURN_ADDRESS);
	}
}

/*
 * Functions for manipulating the kernel_lock.  We put them here
 * so that they show up in profiles.
 */

#define	_KERNEL_LOCK_ABORT(msg)						\
    LOCKDEBUG_ABORT(__func__, __LINE__, kernel_lock, &_kernel_lock_ops, msg)

#ifdef LOCKDEBUG
#define	_KERNEL_LOCK_ASSERT(cond)					\
do {									\
	if (!(cond))							\
		_KERNEL_LOCK_ABORT("assertion failed: " #cond);		\
} while (/* CONSTCOND */ 0)
#else
#define	_KERNEL_LOCK_ASSERT(cond)	/* nothing */
#endif

static void	_kernel_lock_dump(const volatile void *, lockop_printer_t);

lockops_t _kernel_lock_ops = {
	.lo_name = "Kernel lock",
	.lo_type = LOCKOPS_SPIN,
	.lo_dump = _kernel_lock_dump,
};

#ifdef LOCKDEBUG

#include <ddb/ddb.h>

static void
kernel_lock_trace_ipi(void *cookie)
{

	printf("%s[%d %s]: hogging kernel lock\n", cpu_name(curcpu()),
	    curlwp->l_lid,
	    curlwp->l_name ? curlwp->l_name : curproc->p_comm);
	db_stacktrace();
}

#endif

/*
 * Initialize the kernel lock.
 */
void
kernel_lock_init(void)
{

	__cpu_simple_lock_init(kernel_lock);
	kernel_lock_dodebug = LOCKDEBUG_ALLOC(kernel_lock, &_kernel_lock_ops,
	    RETURN_ADDRESS);
}
CTASSERT(CACHE_LINE_SIZE >= sizeof(__cpu_simple_lock_t));

/*
 * Print debugging information about the kernel lock.
 */
static void
_kernel_lock_dump(const volatile void *junk, lockop_printer_t pr)
{
	struct cpu_info *ci = curcpu();

	(void)junk;

	pr("curcpu holds : %18d wanted by: %#018lx\n",
	    ci->ci_biglock_count, (long)ci->ci_biglock_wanted);
}

/*
 * Acquire 'nlocks' holds on the kernel lock.
 *
 * Although it may not look it, this is one of the most central, intricate
 * routines in the kernel, and tons of code elsewhere depends on its exact
 * behaviour.  If you change something in here, expect it to bite you in the
 * rear.
 */
void
_kernel_lock(int nlocks)
{
	struct cpu_info *ci;
	LOCKSTAT_TIMER(spintime);
	LOCKSTAT_FLAG(lsflag);
	struct lwp *owant;
#ifdef LOCKDEBUG
	static struct cpu_info *kernel_lock_holder;
	u_int spins = 0;
	u_int starttime = getticks();
#endif
	int s;
	struct lwp *l = curlwp;

	_KERNEL_LOCK_ASSERT(nlocks > 0);

	s = splvm();
	ci = curcpu();
	if (ci->ci_biglock_count != 0) {
		_KERNEL_LOCK_ASSERT(__SIMPLELOCK_LOCKED_P(kernel_lock));
		ci->ci_biglock_count += nlocks;
		l->l_blcnt += nlocks;
		splx(s);
		return;
	}

	_KERNEL_LOCK_ASSERT(l->l_blcnt == 0);
	LOCKDEBUG_WANTLOCK(kernel_lock_dodebug, kernel_lock, RETURN_ADDRESS,
	    0);

	if (__predict_true(__cpu_simple_lock_try(kernel_lock))) {
#ifdef LOCKDEBUG
		kernel_lock_holder = curcpu();
#endif
		ci->ci_biglock_count = nlocks;
		l->l_blcnt = nlocks;
		LOCKDEBUG_LOCKED(kernel_lock_dodebug, kernel_lock, NULL,
		    RETURN_ADDRESS, 0);
		splx(s);
		return;
	}

	/*
	 * To remove the ordering constraint between adaptive mutexes
	 * and kernel_lock we must make it appear as if this thread is
	 * blocking.  For non-interlocked mutex release, a store fence
	 * is required to ensure that the result of any mutex_exit()
	 * by the current LWP becomes visible on the bus before the set
	 * of ci->ci_biglock_wanted becomes visible.
	 */
	membar_producer();
	owant = ci->ci_biglock_wanted;
	ci->ci_biglock_wanted = l;
#if defined(DIAGNOSTIC) && !defined(LOCKDEBUG)
	l->l_ld_wanted = __builtin_return_address(0);
#endif

	/*
	 * Spin until we acquire the lock.  Once we have it, record the
	 * time spent with lockstat.
	 */
	LOCKSTAT_ENTER(lsflag);
	LOCKSTAT_START_TIMER(lsflag, spintime);

	do {
		splx(s);
		while (__SIMPLELOCK_LOCKED_P(kernel_lock)) {
#ifdef LOCKDEBUG
			if (SPINLOCK_SPINOUT(spins) && start_init_exec &&
			    (getticks() - starttime) > 10*hz) {
				ipi_msg_t msg = {
					.func = kernel_lock_trace_ipi,
				};
				kpreempt_disable();
				ipi_unicast(&msg, kernel_lock_holder);
				ipi_wait(&msg);
				kpreempt_enable();
				_KERNEL_LOCK_ABORT("spinout");
			}
#endif
			SPINLOCK_BACKOFF_HOOK;
			SPINLOCK_SPIN_HOOK;
		}
		s = splvm();
	} while (!__cpu_simple_lock_try(kernel_lock));

	ci->ci_biglock_count = nlocks;
	l->l_blcnt = nlocks;
	LOCKSTAT_STOP_TIMER(lsflag, spintime);
	LOCKDEBUG_LOCKED(kernel_lock_dodebug, kernel_lock, NULL,
	    RETURN_ADDRESS, 0);
	if (owant == NULL) {
		LOCKSTAT_EVENT_RA(lsflag, kernel_lock,
		    LB_KERNEL_LOCK | LB_SPIN, 1, spintime, RETURN_ADDRESS);
	}
	LOCKSTAT_EXIT(lsflag);
	splx(s);

	/*
	 * Now that we have kernel_lock, reset ci_biglock_wanted.  This
	 * store must be unbuffered (immediately visible on the bus) in
	 * order for non-interlocked mutex release to work correctly.
	 * It must be visible before a mutex_exit() can execute on this
	 * processor.
	 *
	 * Note: only where CAS is available in hardware will this be
	 * an unbuffered write, but non-interlocked release cannot be
	 * done on CPUs without CAS in hardware.
	 */
	(void)atomic_swap_ptr(&ci->ci_biglock_wanted, owant);

	/*
	 * Issue a memory barrier as we have acquired a lock.  This also
	 * prevents stores from a following mutex_exit() being reordered
	 * to occur before our store to ci_biglock_wanted above.
	 */
#ifndef __HAVE_ATOMIC_AS_MEMBAR
	membar_enter();
#endif

#ifdef LOCKDEBUG
	kernel_lock_holder = curcpu();
#endif
}

/*
 * Release 'nlocks' holds on the kernel lock.  If 'nlocks' is zero, release
 * all holds.
 */
void
_kernel_unlock(int nlocks, int *countp)
{
	struct cpu_info *ci;
	u_int olocks;
	int s;
	struct lwp *l = curlwp;

	_KERNEL_LOCK_ASSERT(nlocks < 2);

	olocks = l->l_blcnt;

	if (olocks == 0) {
		_KERNEL_LOCK_ASSERT(nlocks <= 0);
		if (countp != NULL)
			*countp = 0;
		return;
	}

	_KERNEL_LOCK_ASSERT(__SIMPLELOCK_LOCKED_P(kernel_lock));

	if (nlocks == 0)
		nlocks = olocks;
	else if (nlocks == -1) {
		nlocks = 1;
		_KERNEL_LOCK_ASSERT(olocks == 1);
	}
	s = splvm();
	ci = curcpu();
	_KERNEL_LOCK_ASSERT(ci->ci_biglock_count >= l->l_blcnt);
	if (ci->ci_biglock_count == nlocks) {
		LOCKDEBUG_UNLOCKED(kernel_lock_dodebug, kernel_lock,
		    RETURN_ADDRESS, 0);
		ci->ci_biglock_count = 0;
		__cpu_simple_unlock(kernel_lock);
		l->l_blcnt -= nlocks;
		splx(s);
		if (l->l_dopreempt)
			kpreempt(0);
	} else {
		ci->ci_biglock_count -= nlocks;
		l->l_blcnt -= nlocks;
		splx(s);
	}

	if (countp != NULL)
		*countp = olocks;
}

bool
_kernel_locked_p(void)
{
	return __SIMPLELOCK_LOCKED_P(kernel_lock);
}
