/*	$NetBSD: uvm_pdpolicy_clock.c,v 1.40 2022/04/12 20:27:56 andvar Exp $	*/
/*	NetBSD: uvm_pdaemon.c,v 1.72 2006/01/05 10:47:33 yamt Exp $	*/

/*-
 * Copyright (c) 2019, 2020 The NetBSD Foundation, Inc.
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
 * Copyright (c) 1997 Charles D. Cranor and Washington University.
 * Copyright (c) 1991, 1993, The Regents of the University of California.
 *
 * All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * The Mach Operating System project at Carnegie-Mellon University.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	@(#)vm_pageout.c        8.5 (Berkeley) 2/14/94
 * from: Id: uvm_pdaemon.c,v 1.1.2.32 1998/02/06 05:26:30 chs Exp
 *
 *
 * Copyright (c) 1987, 1990 Carnegie-Mellon University.
 * All rights reserved.
 *
 * Permission to use, copy, modify and distribute this software and
 * its documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 *
 * CARNEGIE MELLON ALLOWS FREE USE OF THIS SOFTWARE IN ITS "AS IS"
 * CONDITION.  CARNEGIE MELLON DISCLAIMS ANY LIABILITY OF ANY KIND
 * FOR ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 *
 * Carnegie Mellon requests users of this software to return to
 *
 *  Software Distribution Coordinator  or  Software.Distribution@CS.CMU.EDU
 *  School of Computer Science
 *  Carnegie Mellon University
 *  Pittsburgh PA 15213-3890
 *
 * any improvements or extensions that they make and grant Carnegie the
 * rights to redistribute these changes.
 */

#if defined(PDSIM)

#include "pdsim.h"

#else /* defined(PDSIM) */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: uvm_pdpolicy_clock.c,v 1.40 2022/04/12 20:27:56 andvar Exp $");

#include <sys/param.h>
#include <sys/proc.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/kmem.h>
#include <sys/atomic.h>

#include <uvm/uvm.h>
#include <uvm/uvm_pdpolicy.h>
#include <uvm/uvm_pdpolicy_impl.h>
#include <uvm/uvm_stat.h>

#endif /* defined(PDSIM) */

/*
 * per-CPU queue of pending page status changes.  128 entries makes for a
 * 1kB queue on _LP64 and has been found to be a reasonable compromise that
 * keeps lock contention events and wait times low, while not using too much
 * memory nor allowing global state to fall too far behind.
 */
#if !defined(CLOCK_PDQ_SIZE)
#define	CLOCK_PDQ_SIZE	128
#endif /* !defined(CLOCK_PDQ_SIZE) */

#define PQ_INACTIVE	0x00000010	/* page is in inactive list */
#define PQ_ACTIVE	0x00000020	/* page is in active list */

#if !defined(CLOCK_INACTIVEPCT)
#define	CLOCK_INACTIVEPCT	33
#endif /* !defined(CLOCK_INACTIVEPCT) */

struct uvmpdpol_globalstate {
	kmutex_t lock;			/* lock on state */
					/* <= compiler pads here */
	struct pglist s_activeq		/* allocated pages, in use */
	    __aligned(COHERENCY_UNIT);
	struct pglist s_inactiveq;	/* pages between the clock hands */
	int s_active;
	int s_inactive;
	int s_inactarg;
	struct uvm_pctparam s_anonmin;
	struct uvm_pctparam s_filemin;
	struct uvm_pctparam s_execmin;
	struct uvm_pctparam s_anonmax;
	struct uvm_pctparam s_filemax;
	struct uvm_pctparam s_execmax;
	struct uvm_pctparam s_inactivepct;
};

struct uvmpdpol_scanstate {
	bool ss_anonreact, ss_filereact, ss_execreact;
	struct vm_page ss_marker;
};

static void	uvmpdpol_pageactivate_locked(struct vm_page *);
static void	uvmpdpol_pagedeactivate_locked(struct vm_page *);
static void	uvmpdpol_pagedequeue_locked(struct vm_page *);
static bool	uvmpdpol_pagerealize_locked(struct vm_page *);
static struct uvm_cpu *uvmpdpol_flush(void);

static struct uvmpdpol_globalstate pdpol_state __cacheline_aligned;
static struct uvmpdpol_scanstate pdpol_scanstate;

PDPOL_EVCNT_DEFINE(reactexec)
PDPOL_EVCNT_DEFINE(reactfile)
PDPOL_EVCNT_DEFINE(reactanon)

static void
clock_tune(void)
{
	struct uvmpdpol_globalstate *s = &pdpol_state;

	s->s_inactarg = UVM_PCTPARAM_APPLY(&s->s_inactivepct,
	    s->s_active + s->s_inactive);
	if (s->s_inactarg <= uvmexp.freetarg) {
		s->s_inactarg = uvmexp.freetarg + 1;
	}
}

void
uvmpdpol_scaninit(void)
{
	struct uvmpdpol_globalstate *s = &pdpol_state;
	struct uvmpdpol_scanstate *ss = &pdpol_scanstate;
	int t;
	bool anonunder, fileunder, execunder;
	bool anonover, fileover, execover;
	bool anonreact, filereact, execreact;
	int64_t freepg, anonpg, filepg, execpg;

	/*
	 * decide which types of pages we want to reactivate instead of freeing
	 * to keep usage within the minimum and maximum usage limits.
	 * uvm_availmem() will sync the counters.
	 */

	freepg = uvm_availmem(false);
	anonpg = cpu_count_get(CPU_COUNT_ANONCLEAN) +
	    cpu_count_get(CPU_COUNT_ANONDIRTY) +
	    cpu_count_get(CPU_COUNT_ANONUNKNOWN);
	execpg = cpu_count_get(CPU_COUNT_EXECPAGES);
	filepg = cpu_count_get(CPU_COUNT_FILECLEAN) +
	    cpu_count_get(CPU_COUNT_FILEDIRTY) +
	    cpu_count_get(CPU_COUNT_FILEUNKNOWN) -
	    execpg;

	mutex_enter(&s->lock);
	t = s->s_active + s->s_inactive + freepg;
	anonunder = anonpg <= UVM_PCTPARAM_APPLY(&s->s_anonmin, t);
	fileunder = filepg <= UVM_PCTPARAM_APPLY(&s->s_filemin, t);
	execunder = execpg <= UVM_PCTPARAM_APPLY(&s->s_execmin, t);
	anonover = anonpg > UVM_PCTPARAM_APPLY(&s->s_anonmax, t);
	fileover = filepg > UVM_PCTPARAM_APPLY(&s->s_filemax, t);
	execover = execpg > UVM_PCTPARAM_APPLY(&s->s_execmax, t);
	anonreact = anonunder || (!anonover && (fileover || execover));
	filereact = fileunder || (!fileover && (anonover || execover));
	execreact = execunder || (!execover && (anonover || fileover));
	if (filereact && execreact && (anonreact || uvm_swapisfull())) {
		anonreact = filereact = execreact = false;
	}
	ss->ss_anonreact = anonreact;
	ss->ss_filereact = filereact;
	ss->ss_execreact = execreact;
	memset(&ss->ss_marker, 0, sizeof(ss->ss_marker));
	ss->ss_marker.flags = PG_MARKER;
	TAILQ_INSERT_HEAD(&pdpol_state.s_inactiveq, &ss->ss_marker, pdqueue);
	mutex_exit(&s->lock);
}

void
uvmpdpol_scanfini(void)
{
	struct uvmpdpol_globalstate *s = &pdpol_state;
	struct uvmpdpol_scanstate *ss = &pdpol_scanstate;

	mutex_enter(&s->lock);
	TAILQ_REMOVE(&pdpol_state.s_inactiveq, &ss->ss_marker, pdqueue);
	mutex_exit(&s->lock);
}

struct vm_page *
uvmpdpol_selectvictim(krwlock_t **plock)
{
	struct uvmpdpol_globalstate *s = &pdpol_state;
	struct uvmpdpol_scanstate *ss = &pdpol_scanstate;
	struct vm_page *pg;
	krwlock_t *lock;

	mutex_enter(&s->lock);
	while (/* CONSTCOND */ 1) {
		struct vm_anon *anon;
		struct uvm_object *uobj;

		pg = TAILQ_NEXT(&ss->ss_marker, pdqueue);
		if (pg == NULL) {
			break;
		}
		KASSERT((pg->flags & PG_MARKER) == 0);
		uvmexp.pdscans++;

		/*
		 * acquire interlock to stabilize page identity.
		 * if we have caught the page in a state of flux
		 * deal with it and retry.
		 */
		mutex_enter(&pg->interlock);
		if (uvmpdpol_pagerealize_locked(pg)) {
			mutex_exit(&pg->interlock);
			continue;
		}

		/*
		 * now prepare to move on to the next page.
		 */
		TAILQ_REMOVE(&pdpol_state.s_inactiveq, &ss->ss_marker,
		    pdqueue);
		TAILQ_INSERT_AFTER(&pdpol_state.s_inactiveq, pg,
		    &ss->ss_marker, pdqueue);

		/*
		 * enforce the minimum thresholds on different
		 * types of memory usage.  if reusing the current
		 * page would reduce that type of usage below its
		 * minimum, reactivate the page instead and move
		 * on to the next page.
		 */
		anon = pg->uanon;
		uobj = pg->uobject;
		if (uobj && UVM_OBJ_IS_VTEXT(uobj) && ss->ss_execreact) {
			uvmpdpol_pageactivate_locked(pg);
			mutex_exit(&pg->interlock);
			PDPOL_EVCNT_INCR(reactexec);
			continue;
		}
		if (uobj && UVM_OBJ_IS_VNODE(uobj) &&
		    !UVM_OBJ_IS_VTEXT(uobj) && ss->ss_filereact) {
			uvmpdpol_pageactivate_locked(pg);
			mutex_exit(&pg->interlock);
			PDPOL_EVCNT_INCR(reactfile);
			continue;
		}
		if ((anon || UVM_OBJ_IS_AOBJ(uobj)) && ss->ss_anonreact) {
			uvmpdpol_pageactivate_locked(pg);
			mutex_exit(&pg->interlock);
			PDPOL_EVCNT_INCR(reactanon);
			continue;
		}

		/*
		 * try to lock the object that owns the page.
		 *
		 * with the page interlock held, we can drop s->lock, which
		 * could otherwise serve as a barrier to us getting the
		 * object locked, because the owner of the object's lock may
		 * be blocked on s->lock (i.e. a deadlock).
		 *
		 * whatever happens, uvmpd_trylockowner() will release the
		 * interlock.  with the interlock dropped we can then
		 * re-acquire our own lock.  the order is:
		 *
		 *	object -> pdpol -> interlock.
	         */
	        mutex_exit(&s->lock);
        	lock = uvmpd_trylockowner(pg);
        	/* pg->interlock now released */
        	mutex_enter(&s->lock);
		if (lock == NULL) {
			/* didn't get it - try the next page. */
			continue;
		}

		/*
		 * move referenced pages back to active queue and skip to
		 * next page.
		 */
		if (pmap_is_referenced(pg)) {
			mutex_enter(&pg->interlock);
			uvmpdpol_pageactivate_locked(pg);
			mutex_exit(&pg->interlock);
			uvmexp.pdreact++;
			rw_exit(lock);
			continue;
		}

		/* we have a potential victim. */
		*plock = lock;
		break;
	}
	mutex_exit(&s->lock);
	return pg;
}

void
uvmpdpol_balancequeue(int swap_shortage)
{
	struct uvmpdpol_globalstate *s = &pdpol_state;
	int inactive_shortage;
	struct vm_page *p, marker;
	krwlock_t *lock;

	/*
	 * we have done the scan to get free pages.   now we work on meeting
	 * our inactive target.
	 */

	memset(&marker, 0, sizeof(marker));
	marker.flags = PG_MARKER;

	mutex_enter(&s->lock);
	TAILQ_INSERT_HEAD(&pdpol_state.s_activeq, &marker, pdqueue);
	for (;;) {
		inactive_shortage =
		    pdpol_state.s_inactarg - pdpol_state.s_inactive;
		if (inactive_shortage <= 0 && swap_shortage <= 0) {
			break;
		}
		p = TAILQ_NEXT(&marker, pdqueue);
		if (p == NULL) {
			break;
		}
		KASSERT((p->flags & PG_MARKER) == 0);

		/*
		 * acquire interlock to stabilize page identity.
		 * if we have caught the page in a state of flux
		 * deal with it and retry.
		 */
		mutex_enter(&p->interlock);
		if (uvmpdpol_pagerealize_locked(p)) {
			mutex_exit(&p->interlock);
			continue;
		}

		/*
		 * now prepare to move on to the next page.
		 */
		TAILQ_REMOVE(&pdpol_state.s_activeq, &marker, pdqueue);
		TAILQ_INSERT_AFTER(&pdpol_state.s_activeq, p, &marker,
		    pdqueue);

		/*
		 * try to lock the object that owns the page.  see comments
		 * in uvmpdol_selectvictim().
	         */
	        mutex_exit(&s->lock);
        	lock = uvmpd_trylockowner(p);
        	/* p->interlock now released */
        	mutex_enter(&s->lock);
		if (lock == NULL) {
			/* didn't get it - try the next page. */
			continue;
		}

		/*
		 * if there's a shortage of swap slots, try to free it.
		 */
		if (swap_shortage > 0 && (p->flags & PG_SWAPBACKED) != 0 &&
		    (p->flags & PG_BUSY) == 0) {
			if (uvmpd_dropswap(p)) {
				swap_shortage--;
			}
		}

		/*
		 * if there's a shortage of inactive pages, deactivate.
		 */
		if (inactive_shortage > 0) {
			pmap_clear_reference(p);
			mutex_enter(&p->interlock);
			uvmpdpol_pagedeactivate_locked(p);
			mutex_exit(&p->interlock);
			uvmexp.pddeact++;
			inactive_shortage--;
		}
		rw_exit(lock);
	}
	TAILQ_REMOVE(&pdpol_state.s_activeq, &marker, pdqueue);
	mutex_exit(&s->lock);
}

static void
uvmpdpol_pagedeactivate_locked(struct vm_page *pg)
{
	struct uvmpdpol_globalstate *s __diagused = &pdpol_state;

	KASSERT(mutex_owned(&s->lock));
	KASSERT(mutex_owned(&pg->interlock));
	KASSERT((pg->pqflags & (PQ_INTENT_MASK | PQ_INTENT_SET)) !=
	    (PQ_INTENT_D | PQ_INTENT_SET));

	if (pg->pqflags & PQ_ACTIVE) {
		TAILQ_REMOVE(&pdpol_state.s_activeq, pg, pdqueue);
		KASSERT(pdpol_state.s_active > 0);
		pdpol_state.s_active--;
	}
	if ((pg->pqflags & PQ_INACTIVE) == 0) {
		KASSERT(pg->wire_count == 0);
		TAILQ_INSERT_TAIL(&pdpol_state.s_inactiveq, pg, pdqueue);
		pdpol_state.s_inactive++;
	}
	pg->pqflags &= ~(PQ_ACTIVE | PQ_INTENT_SET);
	pg->pqflags |= PQ_INACTIVE;
}

void
uvmpdpol_pagedeactivate(struct vm_page *pg)
{

	KASSERT(uvm_page_owner_locked_p(pg, false));
	KASSERT(mutex_owned(&pg->interlock));

	/*
	 * we have to clear the reference bit now, as when it comes time to
	 * realize the intent we won't have the object locked any more.
	 */
	pmap_clear_reference(pg);
	uvmpdpol_set_intent(pg, PQ_INTENT_I);
}

static void
uvmpdpol_pageactivate_locked(struct vm_page *pg)
{
	struct uvmpdpol_globalstate *s __diagused = &pdpol_state;

	KASSERT(mutex_owned(&s->lock));
	KASSERT(mutex_owned(&pg->interlock));
	KASSERT((pg->pqflags & (PQ_INTENT_MASK | PQ_INTENT_SET)) !=
	    (PQ_INTENT_D | PQ_INTENT_SET));

	uvmpdpol_pagedequeue_locked(pg);
	TAILQ_INSERT_TAIL(&pdpol_state.s_activeq, pg, pdqueue);
	pdpol_state.s_active++;
	pg->pqflags &= ~(PQ_INACTIVE | PQ_INTENT_SET);
	pg->pqflags |= PQ_ACTIVE;
}

void
uvmpdpol_pageactivate(struct vm_page *pg)
{

	KASSERT(uvm_page_owner_locked_p(pg, false));
	KASSERT(mutex_owned(&pg->interlock));

	uvmpdpol_set_intent(pg, PQ_INTENT_A);
}

static void
uvmpdpol_pagedequeue_locked(struct vm_page *pg)
{
	struct uvmpdpol_globalstate *s __diagused = &pdpol_state;

	KASSERT(mutex_owned(&s->lock));
	KASSERT(mutex_owned(&pg->interlock));

	if (pg->pqflags & PQ_ACTIVE) {
		TAILQ_REMOVE(&pdpol_state.s_activeq, pg, pdqueue);
		KASSERT((pg->pqflags & PQ_INACTIVE) == 0);
		KASSERT(pdpol_state.s_active > 0);
		pdpol_state.s_active--;
	} else if (pg->pqflags & PQ_INACTIVE) {
		TAILQ_REMOVE(&pdpol_state.s_inactiveq, pg, pdqueue);
		KASSERT(pdpol_state.s_inactive > 0);
		pdpol_state.s_inactive--;
	}
	pg->pqflags &= ~(PQ_ACTIVE | PQ_INACTIVE | PQ_INTENT_SET);
}

void
uvmpdpol_pagedequeue(struct vm_page *pg)
{

	KASSERT(uvm_page_owner_locked_p(pg, true));
	KASSERT(mutex_owned(&pg->interlock));

	uvmpdpol_set_intent(pg, PQ_INTENT_D);
}

void
uvmpdpol_pageenqueue(struct vm_page *pg)
{

	KASSERT(uvm_page_owner_locked_p(pg, false));
	KASSERT(mutex_owned(&pg->interlock));

	uvmpdpol_set_intent(pg, PQ_INTENT_E);
}

void
uvmpdpol_anfree(struct vm_anon *an)
{
}

bool
uvmpdpol_pageisqueued_p(struct vm_page *pg)
{
	uint32_t pqflags;

	/*
	 * if there's an intent set, we have to consider it.  otherwise,
	 * return the actual state.  we may be called unlocked for the
	 * purpose of assertions, which is safe due to the page lifecycle.
	 */
	pqflags = atomic_load_relaxed(&pg->pqflags);
	if ((pqflags & PQ_INTENT_SET) != 0) {
		return (pqflags & PQ_INTENT_MASK) != PQ_INTENT_D;
	} else {
		return (pqflags & (PQ_ACTIVE | PQ_INACTIVE)) != 0;
	}
}

bool
uvmpdpol_pageactivate_p(struct vm_page *pg)
{
	uint32_t pqflags;

	/* consider intent in preference to actual state. */
	pqflags = atomic_load_relaxed(&pg->pqflags);
	if ((pqflags & PQ_INTENT_SET) != 0) {
		pqflags &= PQ_INTENT_MASK;
		return pqflags != PQ_INTENT_A && pqflags != PQ_INTENT_E;
	} else {
		/*
		 * TODO: Enabling this may be too much of a big hammer,
		 * since we do get useful information from activations.
		 * Think about it more and maybe come up with a heuristic
		 * or something.
		 *
		 * return (pqflags & PQ_ACTIVE) == 0;
		 */
		return true;
	}
}

void
uvmpdpol_estimatepageable(int *active, int *inactive)
{
	struct uvmpdpol_globalstate *s = &pdpol_state;

	/*
	 * Don't take any locks here.  This can be called from DDB, and in
	 * any case the numbers are stale the instant the lock is dropped,
	 * so it just doesn't matter.
	 */
	if (active) {
		*active = s->s_active;
	}
	if (inactive) {
		*inactive = s->s_inactive;
	}
}

#if !defined(PDSIM)
static int
min_check(struct uvm_pctparam *pct, int t)
{
	struct uvmpdpol_globalstate *s = &pdpol_state;
	int total = t;

	if (pct != &s->s_anonmin) {
		total += uvm_pctparam_get(&s->s_anonmin);
	}
	if (pct != &s->s_filemin) {
		total += uvm_pctparam_get(&s->s_filemin);
	}
	if (pct != &s->s_execmin) {
		total += uvm_pctparam_get(&s->s_execmin);
	}
	if (total > 95) {
		return EINVAL;
	}
	return 0;
}
#endif /* !defined(PDSIM) */

void
uvmpdpol_init(void)
{
	struct uvmpdpol_globalstate *s = &pdpol_state;

	mutex_init(&s->lock, MUTEX_DEFAULT, IPL_NONE);
	TAILQ_INIT(&s->s_activeq);
	TAILQ_INIT(&s->s_inactiveq);
	uvm_pctparam_init(&s->s_inactivepct, CLOCK_INACTIVEPCT, NULL);
	uvm_pctparam_init(&s->s_anonmin, 10, min_check);
	uvm_pctparam_init(&s->s_filemin, 10, min_check);
	uvm_pctparam_init(&s->s_execmin,  5, min_check);
	uvm_pctparam_init(&s->s_anonmax, 80, NULL);
	uvm_pctparam_init(&s->s_filemax, 50, NULL);
	uvm_pctparam_init(&s->s_execmax, 30, NULL);
}

void
uvmpdpol_init_cpu(struct uvm_cpu *ucpu)
{

	ucpu->pdq =
	    kmem_alloc(CLOCK_PDQ_SIZE * sizeof(struct vm_page *), KM_SLEEP);
	ucpu->pdqhead = CLOCK_PDQ_SIZE;
	ucpu->pdqtail = CLOCK_PDQ_SIZE;
}

void
uvmpdpol_reinit(void)
{
}

bool
uvmpdpol_needsscan_p(void)
{

	/*
	 * this must be an unlocked check: can be called from interrupt.
	 */
	return pdpol_state.s_inactive < pdpol_state.s_inactarg;
}

void
uvmpdpol_tune(void)
{
	struct uvmpdpol_globalstate *s = &pdpol_state;

	mutex_enter(&s->lock);
	clock_tune();
	mutex_exit(&s->lock);
}

/*
 * uvmpdpol_pagerealize_locked: take the intended state set on a page and
 * make it real.  return true if any work was done.
 */
static bool
uvmpdpol_pagerealize_locked(struct vm_page *pg)
{
	struct uvmpdpol_globalstate *s __diagused = &pdpol_state;

	KASSERT(mutex_owned(&s->lock));
	KASSERT(mutex_owned(&pg->interlock));

	switch (pg->pqflags & (PQ_INTENT_MASK | PQ_INTENT_SET)) {
	case PQ_INTENT_A | PQ_INTENT_SET:
	case PQ_INTENT_E | PQ_INTENT_SET:
		uvmpdpol_pageactivate_locked(pg);
		return true;
	case PQ_INTENT_I | PQ_INTENT_SET:
		uvmpdpol_pagedeactivate_locked(pg);
		return true;
	case PQ_INTENT_D | PQ_INTENT_SET:
		uvmpdpol_pagedequeue_locked(pg);
		return true;
	default:
		return false;
	}
}

/*
 * uvmpdpol_flush: return the current uvm_cpu with all of its pending
 * updates flushed to the global queues.  this routine may block, and
 * so can switch cpu.  the idea is to empty to queue on whatever cpu
 * we finally end up on.
 */
static struct uvm_cpu *
uvmpdpol_flush(void)
{
	struct uvmpdpol_globalstate *s __diagused = &pdpol_state;
	struct uvm_cpu *ucpu;
	struct vm_page *pg;

	KASSERT(kpreempt_disabled());

	mutex_enter(&s->lock);
	for (;;) {
		/*
		 * prefer scanning forwards (even though mutex_enter() is
		 * serializing) so as to not defeat any prefetch logic in
		 * the CPU.  that means elsewhere enqueuing backwards, like
		 * a stack, but not so important there as pages are being
		 * added singularly.
		 *
		 * prefetch the next "struct vm_page" while working on the
		 * current one.  this has a measurable and very positive
		 * effect in reducing the amount of time spent here under
		 * the global lock.
		 */
		ucpu = curcpu()->ci_data.cpu_uvm;
		KASSERT(ucpu->pdqhead <= ucpu->pdqtail);
		if (__predict_false(ucpu->pdqhead == ucpu->pdqtail)) {
			break;
		}
		pg = ucpu->pdq[ucpu->pdqhead++];
		if (__predict_true(ucpu->pdqhead != ucpu->pdqtail)) {
			__builtin_prefetch(ucpu->pdq[ucpu->pdqhead]);
		}
		mutex_enter(&pg->interlock);
		pg->pqflags &= ~PQ_INTENT_QUEUED;
		(void)uvmpdpol_pagerealize_locked(pg);
		mutex_exit(&pg->interlock);
	}
	mutex_exit(&s->lock);
	return ucpu;
}

/*
 * uvmpdpol_pagerealize: realize any intent set on the page.  in this
 * implementation, that means putting the page on a per-CPU queue to be
 * dealt with later.
 */
void
uvmpdpol_pagerealize(struct vm_page *pg)
{
	struct uvm_cpu *ucpu;

	/*
	 * drain the per per-CPU queue if full, then enter the page.
	 */
	kpreempt_disable();
	ucpu = curcpu()->ci_data.cpu_uvm;
	if (__predict_false(ucpu->pdqhead == 0)) {
		ucpu = uvmpdpol_flush();
	}
	ucpu->pdq[--(ucpu->pdqhead)] = pg;
	kpreempt_enable();
}

/*
 * uvmpdpol_idle: called from the system idle loop.  periodically purge any
 * pending updates back to the global queues.
 */
void
uvmpdpol_idle(struct uvm_cpu *ucpu)
{
	struct uvmpdpol_globalstate *s = &pdpol_state;
	struct vm_page *pg;

	KASSERT(kpreempt_disabled());

	/*
	 * if no pages in the queue, we have nothing to do.
	 */
	if (ucpu->pdqhead == ucpu->pdqtail) {
		ucpu->pdqtime = getticks();
		return;
	}

	/*
	 * don't do this more than ~8 times a second as it would needlessly
	 * exert pressure.
	 */
	if (getticks() - ucpu->pdqtime < (hz >> 3)) {
		return;
	}

	/*
	 * the idle LWP can't block, so we have to try for the lock.  if we
	 * get it, purge the per-CPU pending update queue.  continually
	 * check for a pending resched: in that case exit immediately.
	 */
	if (mutex_tryenter(&s->lock)) {
		while (ucpu->pdqhead != ucpu->pdqtail) {
			pg = ucpu->pdq[ucpu->pdqhead];
			if (!mutex_tryenter(&pg->interlock)) {
				break;
			}
			ucpu->pdqhead++;
			pg->pqflags &= ~PQ_INTENT_QUEUED;
			(void)uvmpdpol_pagerealize_locked(pg);
			mutex_exit(&pg->interlock);
			if (curcpu()->ci_want_resched) {
				break;
			}
		}
		if (ucpu->pdqhead == ucpu->pdqtail) {
			ucpu->pdqtime = getticks();
		}
		mutex_exit(&s->lock);
	}
}

#if !defined(PDSIM)

#include <sys/sysctl.h>	/* XXX SYSCTL_DESCR */

void
uvmpdpol_sysctlsetup(void)
{
	struct uvmpdpol_globalstate *s = &pdpol_state;

	uvm_pctparam_createsysctlnode(&s->s_anonmin, "anonmin",
	    SYSCTL_DESCR("Percentage of physical memory reserved "
	    "for anonymous application data"));
	uvm_pctparam_createsysctlnode(&s->s_filemin, "filemin",
	    SYSCTL_DESCR("Percentage of physical memory reserved "
	    "for cached file data"));
	uvm_pctparam_createsysctlnode(&s->s_execmin, "execmin",
	    SYSCTL_DESCR("Percentage of physical memory reserved "
	    "for cached executable data"));

	uvm_pctparam_createsysctlnode(&s->s_anonmax, "anonmax",
	    SYSCTL_DESCR("Percentage of physical memory which will "
	    "be reclaimed from other usage for "
	    "anonymous application data"));
	uvm_pctparam_createsysctlnode(&s->s_filemax, "filemax",
	    SYSCTL_DESCR("Percentage of physical memory which will "
	    "be reclaimed from other usage for cached "
	    "file data"));
	uvm_pctparam_createsysctlnode(&s->s_execmax, "execmax",
	    SYSCTL_DESCR("Percentage of physical memory which will "
	    "be reclaimed from other usage for cached "
	    "executable data"));

	uvm_pctparam_createsysctlnode(&s->s_inactivepct, "inactivepct",
	    SYSCTL_DESCR("Percentage of inactive queue of "
	    "the entire (active + inactive) queue"));
}

#endif /* !defined(PDSIM) */

#if defined(PDSIM)
void
pdsim_dump(const char *id)
{
#if defined(DEBUG)
	/* XXX */
#endif /* defined(DEBUG) */
}
#endif /* defined(PDSIM) */
