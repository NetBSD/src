/*	$NetBSD: uvm_pdpolicy_clock.c,v 1.12.16.2 2012/02/09 03:05:01 matt Exp $	*/
/*	NetBSD: uvm_pdaemon.c,v 1.72 2006/01/05 10:47:33 yamt Exp $	*/

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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by Charles D. Cranor,
 *      Washington University, the University of California, Berkeley and
 *      its contributors.
 * 4. Neither the name of the University nor the names of its contributors
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
__KERNEL_RCSID(0, "$NetBSD: uvm_pdpolicy_clock.c,v 1.12.16.2 2012/02/09 03:05:01 matt Exp $");

#include <sys/param.h>
#include <sys/proc.h>
#include <sys/systm.h>
#include <sys/kernel.h>

#include <uvm/uvm.h>
#include <uvm/uvm_pdpolicy.h>
#include <uvm/uvm_pdpolicy_impl.h>

#endif /* defined(PDSIM) */

#define PQ_INACTIVE	PQ_PRIVATE1	/* page is in inactive list */
#define PQ_ACTIVE	PQ_PRIVATE2	/* page is in active list */

#if !defined(CLOCK_INACTIVEPCT)
#define	CLOCK_INACTIVEPCT	33
#endif /* !defined(CLOCK_INACTIVEPCT) */

struct uvmpdpol_scanstate {
	struct vm_page *ss_nextpg;
	bool ss_first;
	bool ss_anonreact, ss_filereact, ss_execreact;
};

struct uvmpdpol_groupstate {
	struct pglist gs_activeq;	/* allocated pages, in use */
	struct pglist gs_inactiveq;	/* pages between the clock hands */
	u_int gs_active;
	u_int gs_inactive;
	u_int gs_inactarg;
	struct uvmpdpol_scanstate gs_scanstate;
};

struct uvmpdpol_globalstate {
	struct uvmpdpol_groupstate *s_pggroups;
	struct uvm_pctparam s_anonmin;
	struct uvm_pctparam s_filemin;
	struct uvm_pctparam s_execmin;
	struct uvm_pctparam s_anonmax;
	struct uvm_pctparam s_filemax;
	struct uvm_pctparam s_execmax;
	struct uvm_pctparam s_inactivepct;
};


static struct uvmpdpol_globalstate pdpol_state;

PDPOL_EVCNT_DEFINE(reactexec)
PDPOL_EVCNT_DEFINE(reactfile)
PDPOL_EVCNT_DEFINE(reactanon)

#ifdef DEBUG
static size_t
clock_pglist_count(struct pglist *pglist)
{
	size_t count = 0;
	struct vm_page *pg;
	TAILQ_FOREACH(pg, pglist, pageq.queue) {
		count++;
	}
	return count;
}
#endif

static size_t
clock_space(void)
{
	return sizeof(struct uvmpdpol_groupstate);
}

static void
clock_tune(struct uvm_pggroup *grp)
{
	struct uvmpdpol_globalstate * const s = &pdpol_state;
	struct uvmpdpol_groupstate * const gs = grp->pgrp_gs;

	gs->gs_inactarg = UVM_PCTPARAM_APPLY(&s->s_inactivepct,
	    gs->gs_active + gs->gs_inactive);
	if (gs->gs_inactarg <= grp->pgrp_freetarg) {
		gs->gs_inactarg = grp->pgrp_freetarg + 1;
	}
}

void
uvmpdpol_scaninit(struct uvm_pggroup *grp)
{
	struct uvmpdpol_globalstate * const s = &pdpol_state;
	struct uvmpdpol_groupstate * const gs = grp->pgrp_gs;
	struct uvmpdpol_scanstate * const ss = &gs->gs_scanstate;
	bool anonunder, fileunder, execunder;
	bool anonover, fileover, execover;
	bool anonreact, filereact, execreact;

	/*
	 * decide which types of pages we want to reactivate instead of freeing
	 * to keep usage within the minimum and maximum usage limits.
	 */

	u_int t = gs->gs_active + gs->gs_inactive + grp->pgrp_free;
	anonunder = grp->pgrp_anonpages <= UVM_PCTPARAM_APPLY(&s->s_anonmin, t);
	fileunder = grp->pgrp_filepages <= UVM_PCTPARAM_APPLY(&s->s_filemin, t);
	execunder = grp->pgrp_execpages <= UVM_PCTPARAM_APPLY(&s->s_execmin, t);
	anonover = grp->pgrp_anonpages > UVM_PCTPARAM_APPLY(&s->s_anonmax, t);
	fileover = grp->pgrp_filepages > UVM_PCTPARAM_APPLY(&s->s_filemax, t);
	execover = grp->pgrp_execpages > UVM_PCTPARAM_APPLY(&s->s_execmax, t);
	anonreact = anonunder || (!anonover && (fileover || execover));
	filereact = fileunder || (!fileover && (anonover || execover));
	execreact = execunder || (!execover && (anonover || fileover));
	if (filereact && execreact && (anonreact || uvm_swapisfull())) {
		anonreact = filereact = execreact = false;
	}
	ss->ss_anonreact = anonreact;
	ss->ss_filereact = filereact;
	ss->ss_execreact = execreact;

	ss->ss_first = true;
}

struct vm_page *
uvmpdpol_selectvictim(struct uvm_pggroup *grp)
{
	struct uvmpdpol_groupstate * const gs = grp->pgrp_gs;
	struct uvmpdpol_scanstate * const ss = &gs->gs_scanstate;
	struct vm_page *pg;
	UVMHIST_FUNC(__func__); UVMHIST_CALLED(pdhist);

	KASSERT(mutex_owned(&uvm_pageqlock));

	while (/* CONSTCOND */ 1) {
		struct vm_anon *anon;
		struct uvm_object *uobj;

		KDASSERT(gs->gs_inactive == clock_pglist_count(&gs->gs_inactiveq));

		if (ss->ss_first) {
			pg = TAILQ_FIRST(&gs->gs_inactiveq);
			ss->ss_first = false;
			UVMHIST_LOG(pdhist, "  select first inactive page: %p",
			    pg, 0, 0, 0);
		} else {
			pg = ss->ss_nextpg;
			if (pg != NULL && (pg->pqflags & PQ_INACTIVE) == 0) {
				pg = TAILQ_FIRST(&gs->gs_inactiveq);
			}
			UVMHIST_LOG(pdhist, "  select next inactive page: %p",
			    pg, 0, 0, 0);
		}
		if (pg == NULL) {
			break;
		}
		ss->ss_nextpg = TAILQ_NEXT(pg, pageq.queue);

		grp->pgrp_pdscans++;

		/*
		 * move referenced pages back to active queue and
		 * skip to next page.
		 */

		if (pmap_is_referenced(pg)) {
			uvmpdpol_pageactivate(pg);
			grp->pgrp_pdreact++;
			continue;
		}

		anon = pg->uanon;
		uobj = pg->uobject;

		/*
		 * enforce the minimum thresholds on different
		 * types of memory usage.  if reusing the current
		 * page would reduce that type of usage below its
		 * minimum, reactivate the page instead and move
		 * on to the next page.
		 */

		if (uobj && UVM_OBJ_IS_VTEXT(uobj) && ss->ss_execreact) {
			uvmpdpol_pageactivate(pg);
			PDPOL_EVCNT_INCR(reactexec);
			continue;
		}
		if (uobj && UVM_OBJ_IS_VNODE(uobj) &&
		    !UVM_OBJ_IS_VTEXT(uobj) && ss->ss_filereact) {
			uvmpdpol_pageactivate(pg);
			PDPOL_EVCNT_INCR(reactfile);
			continue;
		}
		if ((anon || UVM_OBJ_IS_AOBJ(uobj)) && ss->ss_anonreact) {
			uvmpdpol_pageactivate(pg);
			PDPOL_EVCNT_INCR(reactanon);
			continue;
		}

		break;
	}

	return pg;
}

void
uvmpdpol_balancequeue(struct uvm_pggroup *grp, u_int swap_shortage)
{
	struct uvmpdpol_groupstate * const gs = grp->pgrp_gs;

	struct vm_page *pg, *nextpg;

	/*
	 * we have done the scan to get free pages.   now we work on meeting
	 * our inactive target.
	 */

	KDASSERT(gs->gs_active == clock_pglist_count(&gs->gs_activeq));

	u_int inactive_shortage = gs->gs_inactarg - gs->gs_inactive;
	for (pg = TAILQ_FIRST(&gs->gs_activeq);
	     pg != NULL && (inactive_shortage > 0 || swap_shortage > 0);
	     pg = nextpg) {
		nextpg = TAILQ_NEXT(pg, pageq.queue);

		/*
		 * if there's a shortage of swap slots, try to free it.
		 */

#ifdef VMSWAP
		if (swap_shortage > 0 && (pg->pqflags & PQ_SWAPBACKED) != 0) {
			if (uvmpd_trydropswap(pg)) {
				swap_shortage--;
			}
		}
#endif

		/*
		 * if there's a shortage of inactive pages, deactivate.
		 */

		if (inactive_shortage > 0) {
			/* no need to check wire_count as pg is "active" */
			uvmpdpol_pagedeactivate(pg);
			grp->pgrp_pddeact++;
			inactive_shortage--;
		}
	}

	KDASSERT(gs->gs_active == clock_pglist_count(&gs->gs_activeq));
}

void
uvmpdpol_pagedeactivate(struct vm_page *pg)
{
	struct uvm_pggroup * const grp = uvm_page_to_pggroup(pg);
	struct uvmpdpol_groupstate * const gs = grp->pgrp_gs;

	KASSERT(mutex_owned(&uvm_pageqlock));

	KDASSERT(gs->gs_active == clock_pglist_count(&gs->gs_activeq));

	if (pg->pqflags & PQ_ACTIVE) {
		TAILQ_REMOVE(&gs->gs_activeq, pg, pageq.queue);
		pg->pqflags &= ~PQ_ACTIVE;
		KASSERT(gs->gs_active > 0);
		gs->gs_active--;
		grp->pgrp_active--;
	}

	KDASSERT(gs->gs_active == clock_pglist_count(&gs->gs_activeq));
	KDASSERT(gs->gs_inactive == clock_pglist_count(&gs->gs_inactiveq));

	if ((pg->pqflags & PQ_INACTIVE) == 0) {
		KASSERT(pg->wire_count == 0);
		pmap_clear_reference(pg);
		TAILQ_INSERT_TAIL(&gs->gs_inactiveq, pg, pageq.queue);
		pg->pqflags |= PQ_INACTIVE;
		gs->gs_inactive++;
		grp->pgrp_inactive++;
	}

	KDASSERT(gs->gs_inactive == clock_pglist_count(&gs->gs_inactiveq));
}

void
uvmpdpol_pageactivate(struct vm_page *pg)
{
	struct uvm_pggroup * const grp = uvm_page_to_pggroup(pg);
	struct uvmpdpol_groupstate * const gs = grp->pgrp_gs;

	uvmpdpol_pagedequeue(pg);
	TAILQ_INSERT_TAIL(&gs->gs_activeq, pg, pageq.queue);
	pg->pqflags |= PQ_ACTIVE;
	gs->gs_active++;
	grp->pgrp_active++;

	KDASSERT(gs->gs_active == clock_pglist_count(&gs->gs_activeq));
}

void
uvmpdpol_pagedequeue(struct vm_page *pg)
{
	struct uvm_pggroup * const grp = uvm_page_to_pggroup(pg);
	struct uvmpdpol_groupstate * const gs = grp->pgrp_gs;

	KDASSERT(gs->gs_active == clock_pglist_count(&gs->gs_activeq));

	if (pg->pqflags & PQ_ACTIVE) {
		KASSERT(mutex_owned(&uvm_pageqlock));
		TAILQ_REMOVE(&gs->gs_activeq, pg, pageq.queue);
		pg->pqflags &= ~PQ_ACTIVE;
		KASSERT(gs->gs_active > 0);
		gs->gs_active--;
		grp->pgrp_active--;
	}

	KDASSERT(gs->gs_active == clock_pglist_count(&gs->gs_activeq));
	KDASSERT(gs->gs_inactive == clock_pglist_count(&gs->gs_inactiveq));

	if (pg->pqflags & PQ_INACTIVE) {
		KASSERT(mutex_owned(&uvm_pageqlock));
		TAILQ_REMOVE(&gs->gs_inactiveq, pg, pageq.queue);
		pg->pqflags &= ~PQ_INACTIVE;
		KASSERT(gs->gs_inactive > 0);
		gs->gs_inactive--;
		grp->pgrp_inactive--;
	}

	KDASSERT(gs->gs_inactive == clock_pglist_count(&gs->gs_inactiveq));
}

void
uvmpdpol_pageenqueue(struct vm_page *pg)
{

	uvmpdpol_pageactivate(pg);
}

void
uvmpdpol_anfree(struct vm_anon *an)
{
}

bool
uvmpdpol_pageisqueued_p(struct vm_page *pg)
{

	return (pg->pqflags & (PQ_ACTIVE | PQ_INACTIVE)) != 0;
}

void
uvmpdpol_estimatepageable(u_int *activep, u_int *inactivep)
{
	struct uvm_pggroup *grp;
	u_int active = 0, inactive = 0;

	STAILQ_FOREACH(grp, &uvm.page_groups, pgrp_uvm_link) {
		struct uvmpdpol_groupstate * const gs = grp->pgrp_gs;

		KDASSERT(gs->gs_active == clock_pglist_count(&gs->gs_activeq));
		KDASSERT(gs->gs_inactive == clock_pglist_count(&gs->gs_inactiveq));

		active += gs->gs_active;
		inactive += gs->gs_inactive;
	}

	if (activep) {
		*activep = active;
	}
	if (inactivep) {
		*inactivep = inactive;
	}
}

#if !defined(PDSIM)
static int
min_check(struct uvm_pctparam *pct, int t)
{
	struct uvmpdpol_globalstate * const s = &pdpol_state;
	u_int total = t;

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
uvmpdpol_init(void *new_gs, size_t npggroups)
{
	struct uvmpdpol_globalstate * const s = &pdpol_state;
	struct uvmpdpol_groupstate *gs = new_gs;

	s->s_pggroups = gs;

	struct uvm_pggroup *grp = uvm.pggroups;
	for (size_t pggroup = 0; pggroup < npggroups; pggroup++, gs++, grp++) {
		TAILQ_INIT(&gs->gs_activeq);
		TAILQ_INIT(&gs->gs_inactiveq);
		grp->pgrp_gs = gs;
		KASSERT(gs->gs_active == 0);
		KASSERT(gs->gs_inactive == 0);
		KASSERT(grp->pgrp_active == 0);
		KASSERT(grp->pgrp_inactive == 0);
	}
	uvm_pctparam_init(&s->s_inactivepct, CLOCK_INACTIVEPCT, NULL);
	uvm_pctparam_init(&s->s_anonmin, 10, min_check);
	uvm_pctparam_init(&s->s_filemin, 10, min_check);
	uvm_pctparam_init(&s->s_execmin,  5, min_check);
	uvm_pctparam_init(&s->s_anonmax, 80, NULL);
	uvm_pctparam_init(&s->s_filemax, 50, NULL);
	uvm_pctparam_init(&s->s_execmax, 30, NULL);

	STAILQ_FOREACH(grp, &uvm.page_groups, pgrp_uvm_link) {
		uvmpdpol_scaninit(grp);
	}
}

void
uvmpdpol_reinit(void)
{
}

bool
uvmpdpol_needsscan_p(struct uvm_pggroup *grp)
{
	struct uvmpdpol_groupstate * const gs = grp->pgrp_gs;

	return gs->gs_inactive < gs->gs_inactarg;
}

void
uvmpdpol_tune(struct uvm_pggroup *grp)
{

	clock_tune(grp);
}

size_t
uvmpdpol_space(void)
{

	return clock_space();
}

void
uvmpdpol_recolor(void *new_gs, struct uvm_pggroup *grparray,
	size_t npggroups, size_t old_ncolors)
{
	struct uvmpdpol_globalstate * const s = &pdpol_state;
	struct uvmpdpol_groupstate * src_gs = s->s_pggroups;
	struct uvmpdpol_groupstate * const gs = new_gs;

	s->s_pggroups = gs;

	for (size_t i = 0; i < npggroups; i++) {
		struct uvmpdpol_groupstate * const dst_gs = &gs[i];
		TAILQ_INIT(&dst_gs->gs_activeq);
		TAILQ_INIT(&dst_gs->gs_inactiveq);
		uvm.pggroups[i].pgrp_gs = dst_gs;
	}

	const size_t old_npggroups = VM_NPGGROUP(old_ncolors);
	for (size_t i = 0; i < old_npggroups; i++, src_gs++) {
		struct vm_page *pg;
		KDASSERT(src_gs->gs_inactive == clock_pglist_count(&src_gs->gs_inactiveq));
		while ((pg = TAILQ_FIRST(&src_gs->gs_inactiveq)) != NULL) {
			u_int pggroup = VM_PAGE_TO_PGGROUP(pg, uvmexp.ncolors);
			struct uvmpdpol_groupstate * const xgs = &gs[pggroup];

			TAILQ_INSERT_TAIL(&xgs->gs_inactiveq, pg, pageq.queue);
			src_gs->gs_inactive--;
			xgs->gs_inactive++;
			uvm.pggroups[pggroup].pgrp_inactive++;
			KDASSERT(xgs->gs_inactive == clock_pglist_count(&xgs->gs_inactiveq));
		}
		KASSERT(src_gs->gs_inactive == 0);

		KDASSERT(src_gs->gs_active == clock_pglist_count(&src_gs->gs_activeq));
		while ((pg = TAILQ_FIRST(&src_gs->gs_activeq)) != NULL) {
			u_int pggroup = VM_PAGE_TO_PGGROUP(pg, uvmexp.ncolors);
			struct uvmpdpol_groupstate * const xgs = &gs[pggroup];

			TAILQ_INSERT_TAIL(&xgs->gs_activeq, pg, pageq.queue);
			src_gs->gs_active--;
			xgs->gs_active++;
			KDASSERT(xgs->gs_active == clock_pglist_count(&xgs->gs_activeq));
			uvm.pggroups[pggroup].pgrp_active++;
		}
		KASSERT(src_gs->gs_active == 0);
	}

	struct uvm_pggroup *grp;
	STAILQ_FOREACH(grp, &uvm.page_groups, pgrp_uvm_link) {
		clock_tune(grp);
		uvmpdpol_scaninit(grp);
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
