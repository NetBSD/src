/*	$NetBSD: subr_psref.c,v 1.7.2.3 2018/02/19 18:33:38 snj Exp $	*/

/*-
 * Copyright (c) 2016 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Taylor R. Campbell.
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
 * Passive references
 *
 *	Passive references are references to objects that guarantee the
 *	object will not be destroyed until the reference is released.
 *
 *	Passive references require no interprocessor synchronization to
 *	acquire or release.  However, destroying the target of passive
 *	references requires expensive interprocessor synchronization --
 *	xcalls to determine on which CPUs the object is still in use.
 *
 *	Passive references may be held only on a single CPU and by a
 *	single LWP.  They require the caller to allocate a little stack
 *	space, a struct psref object.  Sleeping while a passive
 *	reference is held is allowed, provided that the owner's LWP is
 *	bound to a CPU -- e.g., the owner is a softint or a bound
 *	kthread.  However, sleeping should be kept to a short duration,
 *	e.g. sleeping on an adaptive lock.
 *
 *	Passive references serve as an intermediate stage between
 *	reference counting and passive serialization (pserialize(9)):
 *
 *	- If you need references to transfer from CPU to CPU or LWP to
 *	  LWP, or if you need long-term references, you must use
 *	  reference counting, e.g. with atomic operations or locks,
 *	  which incurs interprocessor synchronization for every use --
 *	  cheaper than an xcall, but not scalable.
 *
 *	- If all users *guarantee* that they will not sleep, then it is
 *	  not necessary to use passive references: you may as well just
 *	  use the even cheaper pserialize(9), because you have
 *	  satisfied the requirements of a pserialize read section.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: subr_psref.c,v 1.7.2.3 2018/02/19 18:33:38 snj Exp $");

#include <sys/types.h>
#include <sys/condvar.h>
#include <sys/cpu.h>
#include <sys/intr.h>
#include <sys/kmem.h>
#include <sys/lwp.h>
#include <sys/mutex.h>
#include <sys/percpu.h>
#include <sys/psref.h>
#include <sys/queue.h>
#include <sys/xcall.h>

SLIST_HEAD(psref_head, psref);

static bool	_psref_held(const struct psref_target *, struct psref_class *,
		    bool);

/*
 * struct psref_class
 *
 *	Private global state for a class of passive reference targets.
 *	Opaque to callers.
 */
struct psref_class {
	kmutex_t		prc_lock;
	kcondvar_t		prc_cv;
	struct percpu		*prc_percpu; /* struct psref_cpu */
	ipl_cookie_t		prc_iplcookie;
	unsigned int		prc_xc_flags;
};

/*
 * struct psref_cpu
 *
 *	Private per-CPU state for a class of passive reference targets.
 *	Not exposed by the API.
 */
struct psref_cpu {
	struct psref_head	pcpu_head;
};

/*
 * psref_class_create(name, ipl)
 *
 *	Create a new passive reference class, with the given wchan name
 *	and ipl.
 */
struct psref_class *
psref_class_create(const char *name, int ipl)
{
	struct psref_class *class;

	ASSERT_SLEEPABLE();

	class = kmem_alloc(sizeof(*class), KM_SLEEP);
	class->prc_percpu = percpu_alloc(sizeof(struct psref_cpu));
	mutex_init(&class->prc_lock, MUTEX_DEFAULT, ipl);
	cv_init(&class->prc_cv, name);
	class->prc_iplcookie = makeiplcookie(ipl);
	class->prc_xc_flags = XC_HIGHPRI_IPL(ipl);

	return class;
}

#ifdef DIAGNOSTIC
static void
psref_cpu_drained_p(void *p, void *cookie, struct cpu_info *ci __unused)
{
	const struct psref_cpu *pcpu = p;
	bool *retp = cookie;

	if (!SLIST_EMPTY(&pcpu->pcpu_head))
		*retp = false;
}

static bool
psref_class_drained_p(const struct psref_class *prc)
{
	bool ret = true;

	percpu_foreach(prc->prc_percpu, &psref_cpu_drained_p, &ret);

	return ret;
}
#endif	/* DIAGNOSTIC */

/*
 * psref_class_destroy(class)
 *
 *	Destroy a passive reference class and free memory associated
 *	with it.  All targets in this class must have been drained and
 *	destroyed already.
 */
void
psref_class_destroy(struct psref_class *class)
{

	KASSERT(psref_class_drained_p(class));

	cv_destroy(&class->prc_cv);
	mutex_destroy(&class->prc_lock);
	percpu_free(class->prc_percpu, sizeof(struct psref_cpu));
	kmem_free(class, sizeof(*class));
}

/*
 * psref_target_init(target, class)
 *
 *	Initialize a passive reference target in the specified class.
 *	The caller is responsible for issuing a membar_producer after
 *	psref_target_init and before exposing a pointer to the target
 *	to other CPUs.
 */
void
psref_target_init(struct psref_target *target,
    struct psref_class *class)
{

	target->prt_class = class;
	target->prt_draining = false;
}

#ifdef DEBUG
static bool
psref_exist(struct psref_cpu *pcpu, struct psref *psref)
{
	struct psref *_psref;

	SLIST_FOREACH(_psref, &pcpu->pcpu_head, psref_entry) {
		if (_psref == psref)
			return true;
	}
	return false;
}

static void
psref_check_duplication(struct psref_cpu *pcpu, struct psref *psref,
    const struct psref_target *target)
{
	bool found = false;

	found = psref_exist(pcpu, psref);
	if (found) {
		panic("The psref is already in the list (acquiring twice?): "
		    "psref=%p target=%p", psref, target);
	}
}

static void
psref_check_existence(struct psref_cpu *pcpu, struct psref *psref,
    const struct psref_target *target)
{
	bool found = false;

	found = psref_exist(pcpu, psref);
	if (!found) {
		panic("The psref isn't in the list (releasing unused psref?): "
		    "psref=%p target=%p", psref, target);
	}
}
#endif /* DEBUG */

/*
 * psref_acquire(psref, target, class)
 *
 *	Acquire a passive reference to the specified target, which must
 *	be in the specified class.
 *
 *	The caller must guarantee that the target will not be destroyed
 *	before psref_acquire returns.
 *
 *	The caller must additionally guarantee that it will not switch
 *	CPUs before releasing the passive reference, either by
 *	disabling kpreemption and avoiding sleeps, or by being in a
 *	softint or in an LWP bound to a CPU.
 */
void
psref_acquire(struct psref *psref, const struct psref_target *target,
    struct psref_class *class)
{
	struct psref_cpu *pcpu;
	int s;

	KASSERTMSG((kpreempt_disabled() || cpu_softintr_p() ||
		ISSET(curlwp->l_pflag, LP_BOUND)),
	    "passive references are CPU-local,"
	    " but preemption is enabled and the caller is not"
	    " in a softint or CPU-bound LWP");
	KASSERTMSG((target->prt_class == class),
	    "mismatched psref target class: %p (ref) != %p (expected)",
	    target->prt_class, class);
	KASSERTMSG(!target->prt_draining, "psref target already destroyed: %p",
	    target);

	/* Block interrupts and acquire the current CPU's reference list.  */
	s = splraiseipl(class->prc_iplcookie);
	pcpu = percpu_getref(class->prc_percpu);

#ifdef DEBUG
	/* Sanity-check if the target is already acquired with the same psref.  */
	psref_check_duplication(pcpu, psref, target);
#endif

	/* Record our reference.  */
	SLIST_INSERT_HEAD(&pcpu->pcpu_head, psref, psref_entry);
	psref->psref_target = target;
	psref->psref_lwp = curlwp;
	psref->psref_cpu = curcpu();

	/* Release the CPU list and restore interrupts.  */
	percpu_putref(class->prc_percpu);
	splx(s);
}

/*
 * psref_release(psref, target, class)
 *
 *	Release a passive reference to the specified target, which must
 *	be in the specified class.
 *
 *	The caller must not have switched CPUs or LWPs since acquiring
 *	the passive reference.
 */
void
psref_release(struct psref *psref, const struct psref_target *target,
    struct psref_class *class)
{
	struct psref_cpu *pcpu;
	int s;

	KASSERTMSG((kpreempt_disabled() || cpu_softintr_p() ||
		ISSET(curlwp->l_pflag, LP_BOUND)),
	    "passive references are CPU-local,"
	    " but preemption is enabled and the caller is not"
	    " in a softint or CPU-bound LWP");
	KASSERTMSG((target->prt_class == class),
	    "mismatched psref target class: %p (ref) != %p (expected)",
	    target->prt_class, class);

	/* Make sure the psref looks sensible.  */
	KASSERTMSG((psref->psref_target == target),
	    "passive reference target mismatch: %p (ref) != %p (expected)",
	    psref->psref_target, target);
	KASSERTMSG((psref->psref_lwp == curlwp),
	    "passive reference transferred from lwp %p to lwp %p",
	    psref->psref_lwp, curlwp);
	KASSERTMSG((psref->psref_cpu == curcpu()),
	    "passive reference transferred from CPU %u to CPU %u",
	    cpu_index(psref->psref_cpu), cpu_index(curcpu()));

	/*
	 * Block interrupts and remove the psref from the current CPU's
	 * list.  No need to percpu_getref or get the head of the list,
	 * and the caller guarantees that we are bound to a CPU anyway
	 * (as does blocking interrupts).
	 */
	s = splraiseipl(class->prc_iplcookie);
	pcpu = percpu_getref(class->prc_percpu);
#ifdef DEBUG
	/* Sanity-check if the target is surely acquired before.  */
	psref_check_existence(pcpu, psref, target);
#endif
	SLIST_REMOVE(&pcpu->pcpu_head, psref, psref, psref_entry);
	percpu_putref(class->prc_percpu);
	splx(s);

	/* If someone is waiting for users to drain, notify 'em.  */
	if (__predict_false(target->prt_draining))
		cv_broadcast(&class->prc_cv);
}

/*
 * psref_copy(pto, pfrom, class)
 *
 *	Copy a passive reference from pfrom, which must be in the
 *	specified class, to pto.  Both pfrom and pto must later be
 *	released with psref_release.
 *
 *	The caller must not have switched CPUs or LWPs since acquiring
 *	pfrom, and must not switch CPUs or LWPs before releasing both
 *	pfrom and pto.
 */
void
psref_copy(struct psref *pto, const struct psref *pfrom,
    struct psref_class *class)
{
	struct psref_cpu *pcpu;
	int s;

	KASSERTMSG((kpreempt_disabled() || cpu_softintr_p() ||
		ISSET(curlwp->l_pflag, LP_BOUND)),
	    "passive references are CPU-local,"
	    " but preemption is enabled and the caller is not"
	    " in a softint or CPU-bound LWP");
	KASSERTMSG((pto != pfrom),
	    "can't copy passive reference to itself: %p",
	    pto);

	/* Make sure the pfrom reference looks sensible.  */
	KASSERTMSG((pfrom->psref_lwp == curlwp),
	    "passive reference transferred from lwp %p to lwp %p",
	    pfrom->psref_lwp, curlwp);
	KASSERTMSG((pfrom->psref_cpu == curcpu()),
	    "passive reference transferred from CPU %u to CPU %u",
	    cpu_index(pfrom->psref_cpu), cpu_index(curcpu()));
	KASSERTMSG((pfrom->psref_target->prt_class == class),
	    "mismatched psref target class: %p (ref) != %p (expected)",
	    pfrom->psref_target->prt_class, class);

	/* Block interrupts and acquire the current CPU's reference list.  */
	s = splraiseipl(class->prc_iplcookie);
	pcpu = percpu_getref(class->prc_percpu);

	/* Record the new reference.  */
	SLIST_INSERT_HEAD(&pcpu->pcpu_head, pto, psref_entry);
	pto->psref_target = pfrom->psref_target;
	pto->psref_lwp = curlwp;
	pto->psref_cpu = curcpu();

	/* Release the CPU list and restore interrupts.  */
	percpu_putref(class->prc_percpu);
	splx(s);
}

/*
 * struct psreffed
 *
 *	Global state for draining a psref target.
 */
struct psreffed {
	struct psref_class	*class;
	struct psref_target	*target;
	bool			ret;
};

static void
psreffed_p_xc(void *cookie0, void *cookie1 __unused)
{
	struct psreffed *P = cookie0;

	/*
	 * If we hold a psref to the target, then answer true.
	 *
	 * This is the only dynamic decision that may be made with
	 * psref_held.
	 *
	 * No need to lock anything here: every write transitions from
	 * false to true, so there can be no conflicting writes.  No
	 * need for a memory barrier here because P->ret is read only
	 * after xc_wait, which has already issued any necessary memory
	 * barriers.
	 */
	if (_psref_held(P->target, P->class, true))
		P->ret = true;
}

static bool
psreffed_p(struct psref_target *target, struct psref_class *class)
{
	struct psreffed P = {
		.class = class,
		.target = target,
		.ret = false,
	};

	if (__predict_true(mp_online)) {
		/*
		 * Ask all CPUs to say whether they hold a psref to the
		 * target.
		 */
		xc_wait(xc_broadcast(class->prc_xc_flags, &psreffed_p_xc, &P,
		                     NULL));
	} else
		psreffed_p_xc(&P, NULL);

	return P.ret;
}

/*
 * psref_target_destroy(target, class)
 *
 *	Destroy a passive reference target.  Waits for all existing
 *	references to drain.  Caller must guarantee no new references
 *	will be acquired once it calls psref_target_destroy, e.g. by
 *	removing the target from a global list first.  May sleep.
 */
void
psref_target_destroy(struct psref_target *target, struct psref_class *class)
{

	ASSERT_SLEEPABLE();

	KASSERTMSG((target->prt_class == class),
	    "mismatched psref target class: %p (ref) != %p (expected)",
	    target->prt_class, class);

	/* Request psref_release to notify us when done.  */
	KASSERTMSG(!target->prt_draining, "psref target already destroyed: %p",
	    target);
	target->prt_draining = true;

	/* Wait until there are no more references on any CPU.  */
	while (psreffed_p(target, class)) {
		/*
		 * This enter/wait/exit business looks wrong, but it is
		 * both necessary, because psreffed_p performs a
		 * low-priority xcall and hence cannot run while a
		 * mutex is locked, and OK, because the wait is timed
		 * -- explicit wakeups are only an optimization.
		 */
		mutex_enter(&class->prc_lock);
		(void)cv_timedwait(&class->prc_cv, &class->prc_lock, 1);
		mutex_exit(&class->prc_lock);
	}

	/* No more references.  Cause subsequent psref_acquire to kassert.  */
	target->prt_class = NULL;
}

static bool
_psref_held(const struct psref_target *target, struct psref_class *class,
    bool lwp_mismatch_ok)
{
	const struct psref_cpu *pcpu;
	const struct psref *psref;
	int s;
	bool held = false;

	KASSERTMSG((kpreempt_disabled() || cpu_softintr_p() ||
		ISSET(curlwp->l_pflag, LP_BOUND)),
	    "passive references are CPU-local,"
	    " but preemption is enabled and the caller is not"
	    " in a softint or CPU-bound LWP");
	KASSERTMSG((target->prt_class == class),
	    "mismatched psref target class: %p (ref) != %p (expected)",
	    target->prt_class, class);

	/* Block interrupts and acquire the current CPU's reference list.  */
	s = splraiseipl(class->prc_iplcookie);
	pcpu = percpu_getref(class->prc_percpu);

	/* Search through all the references on this CPU.  */
	SLIST_FOREACH(psref, &pcpu->pcpu_head, psref_entry) {
		/* Sanity-check the reference's CPU.  */
		KASSERTMSG((psref->psref_cpu == curcpu()),
		    "passive reference transferred from CPU %u to CPU %u",
		    cpu_index(psref->psref_cpu), cpu_index(curcpu()));

		/* If it doesn't match, skip it and move on.  */
		if (psref->psref_target != target)
			continue;

		/*
		 * Sanity-check the reference's LWP if we are asserting
		 * via psref_held that this LWP holds it, but not if we
		 * are testing in psref_target_destroy whether any LWP
		 * still holds it.
		 */
		KASSERTMSG((lwp_mismatch_ok || psref->psref_lwp == curlwp),
		    "passive reference transferred from lwp %p to lwp %p",
		    psref->psref_lwp, curlwp);

		/* Stop here and report that we found it.  */
		held = true;
		break;
	}

	/* Release the CPU list and restore interrupts.  */
	percpu_putref(class->prc_percpu);
	splx(s);

	return held;
}

/*
 * psref_held(target, class)
 *
 *	True if the current CPU holds a passive reference to target,
 *	false otherwise.  May be used only inside assertions.
 */
bool
psref_held(const struct psref_target *target, struct psref_class *class)
{

	return _psref_held(target, class, false);
}
