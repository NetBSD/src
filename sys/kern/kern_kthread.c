/*	$NetBSD: kern_kthread.c,v 1.23.2.4 2010/08/11 22:54:39 yamt Exp $	*/

/*-
 * Copyright (c) 1998, 1999, 2007, 2009 The NetBSD Foundation, Inc.
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
__KERNEL_RCSID(0, "$NetBSD: kern_kthread.c,v 1.23.2.4 2010/08/11 22:54:39 yamt Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/kthread.h>
#include <sys/proc.h>
#include <sys/sched.h>
#include <sys/kmem.h>

#include <uvm/uvm_extern.h>

/*
 * note that stdarg.h and the ansi style va_start macro is used for both
 * ansi and traditional c complers.
 * XXX: this requires that stdarg.h define: va_alist and va_dcl
 */
#include <machine/stdarg.h>

/*
 * Fork a kernel thread.  Any process can request this to be done.
 *
 * With joinable kthreads KTHREAD_JOINABLE flag this should be known.
 * 1. If you specify KTHREAD_JOINABLE, you must call kthread_join() to reap
 *    the thread. It will not be automatically reaped by the system.
 * 2. For any given call to kthread_create(KTHREAD_JOINABLE), you may call
 *    kthread_join() only once on the returned lwp_t *.
 */
int
kthread_create(pri_t pri, int flag, struct cpu_info *ci,
	       void (*func)(void *), void *arg,
	       lwp_t **lp, const char *fmt, ...)
{
	lwp_t *l;
	vaddr_t uaddr;
	int error, lc, lwp_flags;
	va_list ap;

	lwp_flags = LWP_DETACHED;
	
	uaddr = uvm_uarea_alloc();
	if (uaddr == 0) {
		return ENOMEM;
	}
	if ((flag & KTHREAD_TS) != 0) {
		lc = SCHED_OTHER;
	} else {
		lc = SCHED_RR;
	}

	if ((flag & KTHREAD_JOINABLE) != 0) {
		lwp_flags &= ~LWP_DETACHED;
	}

	error = lwp_create(&lwp0, &proc0, uaddr, lwp_flags, NULL,
	    0, func, arg, &l, lc);
	if (error) {
		uvm_uarea_free(uaddr);
		return error;
	}
	if (fmt != NULL) {
		l->l_name = kmem_alloc(MAXCOMLEN, KM_SLEEP);
		if (l->l_name == NULL) {
			kthread_destroy(l);
			return ENOMEM;
		}
		va_start(ap, fmt);
		vsnprintf(l->l_name, MAXCOMLEN, fmt, ap);
		va_end(ap);
	}

	/*
	 * Set parameters.
	 */
	if ((flag & KTHREAD_INTR) != 0) {
		KASSERT((flag & KTHREAD_MPSAFE) != 0);
	}

	/* Joinable kthread can't be NULL. */
	if ((flag & KTHREAD_JOINABLE) != 0) {
		KASSERT(l != NULL);
	}
	
	if (pri == PRI_NONE) {
		if ((flag & KTHREAD_TS) != 0) {
			/* Maximum user priority level. */
			pri = MAXPRI_USER;
		} else {
			/* Minimum kernel priority level. */
			pri = PRI_KTHREAD;
		}
	}
	mutex_enter(proc0.p_lock);
	lwp_lock(l);
	l->l_priority = pri;
	if (ci != NULL) {
		if (ci != l->l_cpu) {
			lwp_unlock_to(l, ci->ci_schedstate.spc_mutex);
			lwp_lock(l);
		}
		l->l_pflag |= LP_BOUND;
		l->l_cpu = ci;
	}
	if ((flag & KTHREAD_INTR) != 0)
		l->l_pflag |= LP_INTR;
	if ((flag & KTHREAD_MPSAFE) == 0)
		l->l_pflag &= ~LP_MPSAFE;

	/*
	 * Set the new LWP running, unless the caller has requested
	 * otherwise.
	 */
	if ((flag & KTHREAD_IDLE) == 0) {
		l->l_stat = LSRUN;
		sched_enqueue(l, false);
		lwp_unlock(l);
	} else
		lwp_unlock_to(l, ci->ci_schedstate.spc_lwplock);
	mutex_exit(proc0.p_lock);

	/* All done! */
	if (lp != NULL)
		*lp = l;

	return (0);
}

/*
 * Cause a kernel thread to exit.  Assumes the exiting thread is the
 * current context.
 */
void
kthread_exit(int ecode)
{
	const char *name;
	lwp_t *l = curlwp;

	/* We can't do much with the exit code, so just report it. */
	if (ecode != 0) {
		if ((name = l->l_name) == NULL)
			name = "unnamed";
		printf("WARNING: kthread `%s' (%d) exits with status %d\n",
		    name, l->l_lid, ecode);
	}

	/* And exit.. */
	lwp_exit(l);

	/*
	 * XXX Fool the compiler.  Making exit1() __noreturn__ is a can
	 * XXX of worms right now.
	 */
	for (;;)
		;
}

/*
 * Destroy an inactive kthread.  The kthread must be in the LSIDL state.
 */
void
kthread_destroy(lwp_t *l)
{
	proc_t *p;
	
	KASSERT((l->l_flag & LW_SYSTEM) != 0);
	KASSERT(l->l_stat == LSIDL);

	p = l->l_proc;
	
	/* Add LRP_DETACHED flag because we can have joinable kthread now. */
	mutex_enter(p->p_lock);
	l->l_prflag |= LPR_DETACHED;
	mutex_exit(p->p_lock);
	
	lwp_exit(l);
}

/*
 * Wait for a kthread to exit, as pthread_join().
 */
int
kthread_join(lwp_t *l)
{
	lwpid_t departed;
	proc_t *p;
	int error;

	KASSERT((l->l_flag & LW_SYSTEM) != 0);
	KASSERT((l->l_prflag & LPR_DETACHED) == 0);
	
	p = l->l_proc;

	mutex_enter(p->p_lock);
	error = lwp_wait1(curlwp, l->l_lid, &departed, LWPWAIT_EXITCONTROL);
	mutex_exit(p->p_lock);

	return error;
}
