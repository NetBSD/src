/*	$NetBSD: kern_kthread.c,v 1.48 2023/07/17 10:55:27 riastradh Exp $	*/

/*-
 * Copyright (c) 1998, 1999, 2007, 2009, 2019 The NetBSD Foundation, Inc.
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
__KERNEL_RCSID(0, "$NetBSD: kern_kthread.c,v 1.48 2023/07/17 10:55:27 riastradh Exp $");

#include <sys/param.h>
#include <sys/cpu.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/kthread.h>
#include <sys/mutex.h>
#include <sys/sched.h>
#include <sys/kmem.h>
#include <sys/msan.h>

#include <uvm/uvm_extern.h>

static kmutex_t		kthread_lock;
static kcondvar_t	kthread_cv;

void
kthread_sysinit(void)
{

	mutex_init(&kthread_lock, MUTEX_DEFAULT, IPL_NONE);
	cv_init(&kthread_cv, "kthrwait");
}

/*
 * kthread_create: create a kernel thread, that is, system-only LWP.
 */
int
kthread_create(pri_t pri, int flag, struct cpu_info *ci,
    void (*func)(void *), void *arg, lwp_t **lp, const char *fmt, ...)
{
	lwp_t *l;
	vaddr_t uaddr;
	int error, lc;
	va_list ap;

	KASSERT((flag & KTHREAD_INTR) == 0 || (flag & KTHREAD_MPSAFE) != 0);

	uaddr = uvm_uarea_system_alloc(
	   (flag & (KTHREAD_INTR|KTHREAD_IDLE)) == KTHREAD_IDLE ? ci : NULL);
	if (uaddr == 0) {
		return ENOMEM;
	}
	kmsan_orig((void *)uaddr, USPACE, KMSAN_TYPE_POOL, __RET_ADDR);
	if ((flag & KTHREAD_TS) != 0) {
		lc = SCHED_OTHER;
	} else {
		lc = SCHED_RR;
	}

	error = lwp_create(&lwp0, &proc0, uaddr, LWP_DETACHED, NULL,
	    0, func, arg, &l, lc, &lwp0.l_sigmask, &lwp0.l_sigstk);
	if (error) {
		uvm_uarea_system_free(uaddr);
		return error;
	}
	if (fmt != NULL) {
		l->l_name = kmem_alloc(MAXCOMLEN, KM_SLEEP);
		va_start(ap, fmt);
		vsnprintf(l->l_name, MAXCOMLEN, fmt, ap);
		va_end(ap);
	}

	/*
	 * Set parameters.
	 */
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
	lwp_changepri(l, pri);
	if (ci != NULL) {
		if (ci != l->l_cpu) {
			lwp_unlock_to(l, ci->ci_schedstate.spc_lwplock);
			lwp_lock(l);
		}
		l->l_pflag |= LP_BOUND;
		l->l_cpu = ci;
	}

	if ((flag & KTHREAD_MUSTJOIN) != 0) {
		KASSERT(lp != NULL);
		l->l_pflag |= LP_MUSTJOIN;
	}
	if ((flag & KTHREAD_INTR) != 0) {
		l->l_pflag |= LP_INTR;
	}
	if ((flag & KTHREAD_MPSAFE) == 0) {
		l->l_pflag &= ~LP_MPSAFE;
	}

	/*
	 * Set the new LWP running, unless the caller has requested
	 * otherwise.
	 */
	KASSERT(l->l_stat == LSIDL);
	if ((flag & KTHREAD_IDLE) == 0) {
		setrunnable(l);
		/* LWP now unlocked */
	} else {
		lwp_unlock(l);
	}
	mutex_exit(proc0.p_lock);

	/* All done! */
	if (lp != NULL) {
		*lp = l;
	}
	return 0;
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

	/* Barrier for joining. */
	if (l->l_pflag & LP_MUSTJOIN) {
		bool *exitedp;

		mutex_enter(&kthread_lock);
		while ((exitedp = l->l_private) == NULL) {
			cv_wait(&kthread_cv, &kthread_lock);
		}
		KASSERT(!*exitedp);
		*exitedp = true;
		cv_broadcast(&kthread_cv);
		mutex_exit(&kthread_lock);
	}

	/* If the kernel lock is held, we need to drop it now. */
	if ((l->l_pflag & LP_MPSAFE) == 0) {
		KERNEL_UNLOCK_LAST(l);
	}

	/* And exit.. */
	lwp_exit(l);
	panic("kthread_exit");
}

/*
 * Wait for a kthread to exit, as pthread_join().
 */
int
kthread_join(lwp_t *l)
{
	bool exited = false;

	KASSERT((l->l_flag & LW_SYSTEM) != 0);
	KASSERT((l->l_pflag & LP_MUSTJOIN) != 0);

	/*
	 * - Ask the kthread to write to `exited'.
	 * - After this, touching l is forbidden -- it may be freed.
	 * - Wait until the kthread has written to `exited'.
	 */
	mutex_enter(&kthread_lock);
	KASSERT(l->l_private == NULL);
	l->l_private = &exited;
	cv_broadcast(&kthread_cv);
	while (!exited) {
		cv_wait(&kthread_cv, &kthread_lock);
	}
	mutex_exit(&kthread_lock);

	return 0;
}

/*
 * kthread_fpu_enter()
 *
 *	Allow the current lwp, which must be a kthread, to use the FPU.
 *	Return a cookie that must be passed to kthread_fpu_exit when
 *	done.  Must be used only in thread context.  Recursive -- you
 *	can call kthread_fpu_enter several times in a row as long as
 *	you pass the cookies in reverse order to kthread_fpu_exit.
 */
int
kthread_fpu_enter(void)
{
	struct lwp *l = curlwp;
	int s;

	KASSERTMSG(!cpu_intr_p(),
	    "%s is not allowed in interrupt context", __func__);
	KASSERTMSG(!cpu_softintr_p(),
	    "%s is not allowed in interrupt context", __func__);

	/*
	 * Remember whether this thread already had FPU access, and
	 * mark this thread as having FPU access.
	 */
	lwp_lock(l);
	KASSERTMSG(l->l_flag & LW_SYSTEM,
	    "%s is allowed only in kthreads", __func__);
	s = l->l_flag & LW_SYSTEM_FPU;
	l->l_flag |= LW_SYSTEM_FPU;
	lwp_unlock(l);

	/* Take MD steps to enable the FPU if necessary.  */
	if (s == 0)
		kthread_fpu_enter_md();

	return s;
}

/*
 * kthread_fpu_exit(s)
 *
 *	Restore the current lwp's FPU access to what it was before the
 *	matching call to kthread_fpu_enter() that returned s.  Must be
 *	used only in thread context.
 */
void
kthread_fpu_exit(int s)
{
	struct lwp *l = curlwp;

	KASSERT(s == (s & LW_SYSTEM_FPU));
	KASSERTMSG(!cpu_intr_p(),
	    "%s is not allowed in interrupt context", __func__);
	KASSERTMSG(!cpu_softintr_p(),
	    "%s is not allowed in interrupt context", __func__);

	lwp_lock(l);
	KASSERTMSG(l->l_flag & LW_SYSTEM,
	    "%s is allowed only in kthreads", __func__);
	KASSERT(l->l_flag & LW_SYSTEM_FPU);
	l->l_flag ^= s ^ LW_SYSTEM_FPU;
	lwp_unlock(l);

	/* Take MD steps to zero and disable the FPU if necessary.  */
	if (s == 0)
		kthread_fpu_exit_md();
}
