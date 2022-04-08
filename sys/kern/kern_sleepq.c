/*	$NetBSD: kern_sleepq.c,v 1.71 2022/04/08 10:17:55 andvar Exp $	*/

/*-
 * Copyright (c) 2006, 2007, 2008, 2009, 2019, 2020 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Andrew Doran.
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

/*
 * Sleep queue implementation, used by turnstiles and general sleep/wakeup
 * interfaces.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: kern_sleepq.c,v 1.71 2022/04/08 10:17:55 andvar Exp $");

#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/cpu.h>
#include <sys/intr.h>
#include <sys/pool.h>
#include <sys/proc.h> 
#include <sys/resourcevar.h>
#include <sys/sched.h>
#include <sys/systm.h>
#include <sys/sleepq.h>
#include <sys/ktrace.h>

/*
 * for sleepq_abort:
 * During autoconfiguration or after a panic, a sleep will simply lower the
 * priority briefly to allow interrupts, then return.  The priority to be
 * used (IPL_SAFEPRI) is machine-dependent, thus this value is initialized and
 * maintained in the machine-dependent layers.  This priority will typically
 * be 0, or the lowest priority that is safe for use on the interrupt stack;
 * it can be made higher to block network software interrupts after panics.
 */
#ifndef	IPL_SAFEPRI
#define	IPL_SAFEPRI	0
#endif

static int	sleepq_sigtoerror(lwp_t *, int);

/* General purpose sleep table, used by mtsleep() and condition variables. */
sleeptab_t	sleeptab __cacheline_aligned;
sleepqlock_t	sleepq_locks[SLEEPTAB_HASH_SIZE] __cacheline_aligned;

/*
 * sleeptab_init:
 *
 *	Initialize a sleep table.
 */
void
sleeptab_init(sleeptab_t *st)
{
	static bool again;
	int i;

	for (i = 0; i < SLEEPTAB_HASH_SIZE; i++) {
		if (!again) {
			mutex_init(&sleepq_locks[i].lock, MUTEX_DEFAULT,
			    IPL_SCHED);
		}
		sleepq_init(&st->st_queue[i]);
	}
	again = true;
}

/*
 * sleepq_init:
 *
 *	Prepare a sleep queue for use.
 */
void
sleepq_init(sleepq_t *sq)
{

	LIST_INIT(sq);
}

/*
 * sleepq_remove:
 *
 *	Remove an LWP from a sleep queue and wake it up.
 */
void
sleepq_remove(sleepq_t *sq, lwp_t *l)
{
	struct schedstate_percpu *spc;
	struct cpu_info *ci;

	KASSERT(lwp_locked(l, NULL));

	if ((l->l_syncobj->sobj_flag & SOBJ_SLEEPQ_NULL) == 0) {
		KASSERT(sq != NULL);
		LIST_REMOVE(l, l_sleepchain);
	} else {
		KASSERT(sq == NULL);
	}

	l->l_syncobj = &sched_syncobj;
	l->l_wchan = NULL;
	l->l_sleepq = NULL;
	l->l_flag &= ~LW_SINTR;

	ci = l->l_cpu;
	spc = &ci->ci_schedstate;

	/*
	 * If not sleeping, the LWP must have been suspended.  Let whoever
	 * holds it stopped set it running again.
	 */
	if (l->l_stat != LSSLEEP) {
		KASSERT(l->l_stat == LSSTOP || l->l_stat == LSSUSPENDED);
		lwp_setlock(l, spc->spc_lwplock);
		return;
	}

	/*
	 * If the LWP is still on the CPU, mark it as LSONPROC.  It may be
	 * about to call mi_switch(), in which case it will yield.
	 */
	if ((l->l_pflag & LP_RUNNING) != 0) {
		l->l_stat = LSONPROC;
		l->l_slptime = 0;
		lwp_setlock(l, spc->spc_lwplock);
		return;
	}

	/* Update sleep time delta, call the wake-up handler of scheduler */
	l->l_slpticksum += (getticks() - l->l_slpticks);
	sched_wakeup(l);

	/* Look for a CPU to wake up */
	l->l_cpu = sched_takecpu(l);
	ci = l->l_cpu;
	spc = &ci->ci_schedstate;

	/*
	 * Set it running.
	 */
	spc_lock(ci);
	lwp_setlock(l, spc->spc_mutex);
	sched_setrunnable(l);
	l->l_stat = LSRUN;
	l->l_slptime = 0;
	sched_enqueue(l);
	sched_resched_lwp(l, true);
	/* LWP & SPC now unlocked, but we still hold sleep queue lock. */
}

/*
 * sleepq_insert:
 *
 *	Insert an LWP into the sleep queue, optionally sorting by priority.
 */
static void
sleepq_insert(sleepq_t *sq, lwp_t *l, syncobj_t *sobj)
{

	if ((sobj->sobj_flag & SOBJ_SLEEPQ_NULL) != 0) {
		KASSERT(sq == NULL); 
		return;
	}
	KASSERT(sq != NULL);

	if ((sobj->sobj_flag & SOBJ_SLEEPQ_SORTED) != 0) {
		lwp_t *l2, *l_last = NULL;
		const pri_t pri = lwp_eprio(l);

		LIST_FOREACH(l2, sq, l_sleepchain) {
			l_last = l2;
			if (lwp_eprio(l2) < pri) {
				LIST_INSERT_BEFORE(l2, l, l_sleepchain);
				return;
			}
		}
		/*
		 * Ensure FIFO ordering if no waiters are of lower priority.
		 */
		if (l_last != NULL) {
			LIST_INSERT_AFTER(l_last, l, l_sleepchain);
			return;
		}
	}

	LIST_INSERT_HEAD(sq, l, l_sleepchain);
}

/*
 * sleepq_enqueue:
 *
 *	Enter an LWP into the sleep queue and prepare for sleep.  The sleep
 *	queue must already be locked, and any interlock (such as the kernel
 *	lock) must have be released (see sleeptab_lookup(), sleepq_enter()).
 */
void
sleepq_enqueue(sleepq_t *sq, wchan_t wchan, const char *wmesg, syncobj_t *sobj,
    bool catch_p)
{
	lwp_t *l = curlwp;

	KASSERT(lwp_locked(l, NULL));
	KASSERT(l->l_stat == LSONPROC);
	KASSERT(l->l_wchan == NULL && l->l_sleepq == NULL);
	KASSERT((l->l_flag & LW_SINTR) == 0);

	l->l_syncobj = sobj;
	l->l_wchan = wchan;
	l->l_sleepq = sq;
	l->l_wmesg = wmesg;
	l->l_slptime = 0;
	l->l_stat = LSSLEEP;
	if (catch_p)
		l->l_flag |= LW_SINTR;

	sleepq_insert(sq, l, sobj);

	/* Save the time when thread has slept */
	l->l_slpticks = getticks();
	sched_slept(l);
}

/*
 * sleepq_transfer:
 *
 *	Move an LWP from one sleep queue to another.  Both sleep queues
 *	must already be locked.
 *
 *	The LWP will be updated with the new sleepq, wchan, wmesg,
 *	sobj, and mutex.  The interruptible flag will also be updated.
 */
void
sleepq_transfer(lwp_t *l, sleepq_t *from_sq, sleepq_t *sq, wchan_t wchan,
    const char *wmesg, syncobj_t *sobj, kmutex_t *mp, bool catch_p)
{

	KASSERT(l->l_sleepq == from_sq);

	LIST_REMOVE(l, l_sleepchain);
	l->l_syncobj = sobj;
	l->l_wchan = wchan;
	l->l_sleepq = sq;
	l->l_wmesg = wmesg;

	if (catch_p)
		l->l_flag = LW_SINTR | LW_CATCHINTR;
	else
		l->l_flag = ~(LW_SINTR | LW_CATCHINTR);

	/*
	 * This allows the transfer from one sleepq to another where
	 * it is known that they're both protected by the same lock.
	 */
	if (mp != NULL)
		lwp_setlock(l, mp);

	sleepq_insert(sq, l, sobj);
}

/*
 * sleepq_uncatch:
 *
 *	Mark the LWP as no longer sleeping interruptibly.
 */
void
sleepq_uncatch(lwp_t *l)
{
	l->l_flag = ~(LW_SINTR | LW_CATCHINTR);
}

/*
 * sleepq_block:
 *
 *	After any intermediate step such as releasing an interlock, switch.
 * 	sleepq_block() may return early under exceptional conditions, for
 * 	example if the LWP's containing process is exiting.
 *
 *	timo is a timeout in ticks.  timo = 0 specifies an infinite timeout.
 */
int
sleepq_block(int timo, bool catch_p)
{
	int error = 0, sig;
	struct proc *p;
	lwp_t *l = curlwp;
	bool early = false;
	int biglocks = l->l_biglocks;

	ktrcsw(1, 0);

	/*
	 * If sleeping interruptably, check for pending signals, exits or
	 * core dump events.
	 *
	 * Note the usage of LW_CATCHINTR.  This expresses our intent
	 * to catch or not catch sleep interruptions, which might change
	 * while we are sleeping.  It is independent from LW_SINTR because
	 * we don't want to leave LW_SINTR set when the LWP is not asleep.
	 */
	if (catch_p) {
		if ((l->l_flag & (LW_CANCELLED|LW_WEXIT|LW_WCORE)) != 0) {
			l->l_flag &= ~LW_CANCELLED;
			error = EINTR;
			early = true;
		} else if ((l->l_flag & LW_PENDSIG) != 0 && sigispending(l, 0))
			early = true;
		l->l_flag |= LW_CATCHINTR;
	} else
		l->l_flag &= ~LW_CATCHINTR;

	if (early) {
		/* lwp_unsleep() will release the lock */
		lwp_unsleep(l, true);
	} else {
		/*
		 * The LWP may have already been awoken if the caller
		 * dropped the sleep queue lock between sleepq_enqueue() and
		 * sleepq_block().  If that happens l_stat will be LSONPROC
		 * and mi_switch() will treat this as a preemption.  No need
		 * to do anything special here.
		 */
		if (timo) {
			l->l_flag &= ~LW_STIMO;
			callout_schedule(&l->l_timeout_ch, timo);
		}
		spc_lock(l->l_cpu);
		mi_switch(l);

		/* The LWP and sleep queue are now unlocked. */
		if (timo) {
			/*
			 * Even if the callout appears to have fired, we
			 * need to stop it in order to synchronise with
			 * other CPUs.  It's important that we do this in
			 * this LWP's context, and not during wakeup, in
			 * order to keep the callout & its cache lines
			 * co-located on the CPU with the LWP.
			 */
			(void)callout_halt(&l->l_timeout_ch, NULL);
			error = (l->l_flag & LW_STIMO) ? EWOULDBLOCK : 0;
		}
	}

	/*
	 * LW_CATCHINTR is only modified in this function OR when we
	 * are asleep (with the sleepq locked).  We can therefore safely
	 * test it unlocked here as it is guaranteed to be stable by
	 * virtue of us running.
	 *
	 * We do not bother clearing it if set; that would require us
	 * to take the LWP lock, and it doesn't seem worth the hassle
	 * considering it is only meaningful here inside this function,
	 * and is set to reflect intent upon entry.
	 */
	if ((l->l_flag & LW_CATCHINTR) != 0 && error == 0) {
		p = l->l_proc;
		if ((l->l_flag & (LW_CANCELLED | LW_WEXIT | LW_WCORE)) != 0)
			error = EINTR;
		else if ((l->l_flag & LW_PENDSIG) != 0) {
			/*
			 * Acquiring p_lock may cause us to recurse
			 * through the sleep path and back into this
			 * routine, but is safe because LWPs sleeping
			 * on locks are non-interruptable and we will
			 * not recurse again.
			 */
			mutex_enter(p->p_lock);
			if (((sig = sigispending(l, 0)) != 0 &&
			    (sigprop[sig] & SA_STOP) == 0) ||
			    (sig = issignal(l)) != 0)
				error = sleepq_sigtoerror(l, sig);
			mutex_exit(p->p_lock);
		}
	}

	ktrcsw(0, 0);
	if (__predict_false(biglocks != 0)) {
		KERNEL_LOCK(biglocks, NULL);
	}
	return error;
}

/*
 * sleepq_wake:
 *
 *	Wake zero or more LWPs blocked on a single wait channel.
 */
void
sleepq_wake(sleepq_t *sq, wchan_t wchan, u_int expected, kmutex_t *mp)
{
	lwp_t *l, *next;

	KASSERT(mutex_owned(mp));

	for (l = LIST_FIRST(sq); l != NULL; l = next) {
		KASSERT(l->l_sleepq == sq);
		KASSERT(l->l_mutex == mp);
		next = LIST_NEXT(l, l_sleepchain);
		if (l->l_wchan != wchan)
			continue;
		sleepq_remove(sq, l);
		if (--expected == 0)
			break;
	}

	mutex_spin_exit(mp);
}

/*
 * sleepq_unsleep:
 *
 *	Remove an LWP from its sleep queue and set it runnable again. 
 *	sleepq_unsleep() is called with the LWP's mutex held, and will
 *	release it if "unlock" is true.
 */
void
sleepq_unsleep(lwp_t *l, bool unlock)
{
	sleepq_t *sq = l->l_sleepq;
	kmutex_t *mp = l->l_mutex;

	KASSERT(lwp_locked(l, mp));
	KASSERT(l->l_wchan != NULL);

	sleepq_remove(sq, l);
	if (unlock) {
		mutex_spin_exit(mp);
	}
}

/*
 * sleepq_timeout:
 *
 *	Entered via the callout(9) subsystem to time out an LWP that is on a
 *	sleep queue.
 */
void
sleepq_timeout(void *arg)
{
	lwp_t *l = arg;

	/*
	 * Lock the LWP.  Assuming it's still on the sleep queue, its
	 * current mutex will also be the sleep queue mutex.
	 */
	lwp_lock(l);

	if (l->l_wchan == NULL) {
		/* Somebody beat us to it. */
		lwp_unlock(l);
		return;
	}

	l->l_flag |= LW_STIMO;
	lwp_unsleep(l, true);
}

/*
 * sleepq_sigtoerror:
 *
 *	Given a signal number, interpret and return an error code.
 */
static int
sleepq_sigtoerror(lwp_t *l, int sig)
{
	struct proc *p = l->l_proc;
	int error;

	KASSERT(mutex_owned(p->p_lock));

	/*
	 * If this sleep was canceled, don't let the syscall restart.
	 */
	if ((SIGACTION(p, sig).sa_flags & SA_RESTART) == 0)
		error = EINTR;
	else
		error = ERESTART;

	return error;
}

/*
 * sleepq_abort:
 *
 *	After a panic or during autoconfiguration, lower the interrupt
 *	priority level to give pending interrupts a chance to run, and
 *	then return.  Called if sleepq_dontsleep() returns non-zero, and
 *	always returns zero.
 */
int
sleepq_abort(kmutex_t *mtx, int unlock)
{ 
	int s;

	s = splhigh();
	splx(IPL_SAFEPRI);
	splx(s);
	if (mtx != NULL && unlock != 0)
		mutex_exit(mtx);

	return 0;
}

/*
 * sleepq_reinsert:
 *
 *	Move the position of the lwp in the sleep queue after a possible
 *	change of the lwp's effective priority.
 */
static void
sleepq_reinsert(sleepq_t *sq, lwp_t *l)
{

	KASSERT(l->l_sleepq == sq);
	if ((l->l_syncobj->sobj_flag & SOBJ_SLEEPQ_SORTED) == 0) { 
		return;
	}

	/*
	 * Don't let the sleep queue become empty, even briefly.
	 * cv_signal() and cv_broadcast() inspect it without the
	 * sleep queue lock held and need to see a non-empty queue
	 * head if there are waiters.
	 */
	if (LIST_FIRST(sq) == l && LIST_NEXT(l, l_sleepchain) == NULL) {
		return;
	}
	LIST_REMOVE(l, l_sleepchain);
	sleepq_insert(sq, l, l->l_syncobj);
}

/*
 * sleepq_changepri:
 *
 *	Adjust the priority of an LWP residing on a sleepq.
 */
void
sleepq_changepri(lwp_t *l, pri_t pri)
{
	sleepq_t *sq = l->l_sleepq;

	KASSERT(lwp_locked(l, NULL));

	l->l_priority = pri;
	sleepq_reinsert(sq, l);
}

/*
 * sleepq_changepri:
 *
 *	Adjust the lended priority of an LWP residing on a sleepq.
 */
void
sleepq_lendpri(lwp_t *l, pri_t pri)
{
	sleepq_t *sq = l->l_sleepq;

	KASSERT(lwp_locked(l, NULL));

	l->l_inheritedprio = pri;
	l->l_auxprio = MAX(l->l_inheritedprio, l->l_protectprio);
	sleepq_reinsert(sq, l);
}
