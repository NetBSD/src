/*	$NetBSD: uvm_stat.c,v 1.31.12.2 2012/02/09 03:05:01 matt Exp $	 */

/*
 *
 * Copyright (c) 1997 Charles D. Cranor and Washington University.
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed by Charles D. Cranor and
 *      Washington University.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * from: Id: uvm_stat.c,v 1.1.2.3 1997/12/19 15:01:00 mrg Exp
 */

/*
 * uvm_stat.c
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: uvm_stat.c,v 1.31.12.2 2012/02/09 03:05:01 matt Exp $");

#include "opt_uvmhist.h"
#include "opt_readahead.h"
#include "opt_ddb.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/cpu.h>

#include <uvm/uvm.h>
#include <uvm/uvm_ddb.h>

/*
 * globals
 */

#ifdef UVMHIST
struct uvm_history_head uvm_histories;
#endif

#ifdef UVMHIST_PRINT
int uvmhist_print_enabled = 1;
#endif

#ifdef DDB

/*
 * prototypes
 */

#ifdef UVMHIST
void uvmhist_dump(struct uvm_history *);
void uvm_hist(u_int32_t);
static void uvmhist_dump_histories(struct uvm_history *[]);
#endif
void uvmcnt_dump(void);


#ifdef UVMHIST
/* call this from ddb */
void
uvmhist_dump(struct uvm_history *l)
{
	int lcv, s;

	s = splhigh();
	lcv = l->f;
	do {
		if (l->e[lcv].fmt)
			uvmhist_entry_print(&l->e[lcv]);
		lcv = (lcv + 1) % l->n;
	} while (lcv != l->f);
	splx(s);
}

/*
 * print a merged list of uvm_history structures
 */
static void
uvmhist_dump_histories(struct uvm_history *hists[])
{
	struct timeval  tv;
	int	cur[MAXHISTS];
	int	s, lcv, hi;

	/* so we don't get corrupted lists! */
	s = splhigh();

	/* find the first of each list */
	for (lcv = 0; hists[lcv]; lcv++)
		 cur[lcv] = hists[lcv]->f;

	/*
	 * here we loop "forever", finding the next earliest
	 * history entry and printing it.  cur[X] is the current
	 * entry to test for the history in hists[X].  if it is
	 * -1, then this history is finished.
	 */
	for (;;) {
		hi = -1;
		tv.tv_sec = tv.tv_usec = 0;

		/* loop over each history */
		for (lcv = 0; hists[lcv]; lcv++) {
restart:
			if (cur[lcv] == -1)
				continue;

			/*
			 * if the format is empty, go to the next entry
			 * and retry.
			 */
			if (hists[lcv]->e[cur[lcv]].fmt == NULL) {
				cur[lcv] = (cur[lcv] + 1) % (hists[lcv]->n);
				if (cur[lcv] == hists[lcv]->f)
					cur[lcv] = -1;
				goto restart;
			}

			/*
			 * if the time hasn't been set yet, or this entry is
			 * earlier than the current tv, set the time and history
			 * index.
			 */
			if (tv.tv_sec == 0 ||
			    timercmp(&hists[lcv]->e[cur[lcv]].tv, &tv, <)) {
				tv = hists[lcv]->e[cur[lcv]].tv;
				hi = lcv;
			}
		}

		/* if we didn't find any entries, we must be done */
		if (hi == -1)
			break;

		/* print and move to the next entry */
		uvmhist_entry_print(&hists[hi]->e[cur[hi]]);
		cur[hi] = (cur[hi] + 1) % (hists[hi]->n);
		if (cur[hi] == hists[hi]->f)
			cur[hi] = -1;
	}
	splx(s);
}

/*
 * call this from ddb.  `bitmask' is from <uvm/uvm_stat.h>.  it
 * merges the named histories.
 */
void
uvm_hist(u_int32_t bitmask)	/* XXX only support 32 hists */
{
	struct uvm_history *hists[MAXHISTS + 1];
	int i = 0;

	if ((bitmask & UVMHIST_MAPHIST) || bitmask == 0)
		hists[i++] = &maphist;

	if ((bitmask & UVMHIST_PDHIST) || bitmask == 0)
		hists[i++] = &pdhist;

	if ((bitmask & UVMHIST_UBCHIST) || bitmask == 0)
		hists[i++] = &ubchist;

	if ((bitmask & UVMHIST_LOANHIST) || bitmask == 0)
		hists[i++] = &loanhist;

	hists[i] = NULL;

	uvmhist_dump_histories(hists);
}

/*
 * uvmhist_print: ddb hook to print uvm history
 */
void
uvmhist_print(void (*pr)(const char *, ...))
{
	uvmhist_dump(LIST_FIRST(&uvm_histories));
}

#endif /* UVMHIST */

/*
 * uvmexp_print: ddb hook to print interesting uvm counters
 */
void
uvmexp_print(void (*pr)(const char *, ...))
{
	struct uvm_pggroup *grp;
	int active, inactive;

	uvm_estimatepageable(&active, &inactive);

	(*pr)("Current UVM status:\n");
	(*pr)("  pagesize=%d (0x%x), pagemask=0x%x, pageshift=%d,"
	    " ncolors=%d, npggroups=%d\n",
	    uvmexp.pagesize, uvmexp.pagesize, uvmexp.pagemask,
	    uvmexp.pageshift, uvmexp.ncolors, uvmexp.npggroups);
	(*pr)("  %d VM pages: %d active, %d inactive, %d wired, %d free\n",
	    uvmexp.npages, active, inactive, uvmexp.wired,
	    uvmexp.free);
	(*pr)("  pages  %d anon, %d file, %d exec\n",
	    uvmexp.anonpages, uvmexp.filepages, uvmexp.execpages);
	(*pr)("  freemin=%d, free-target=%d, wired-max=%d\n",
	    uvmexp.freemin, uvmexp.freetarg, uvmexp.wiredmax);
	STAILQ_FOREACH(grp, &uvm.page_groups, pgrp_uvm_link) {
		if (uvmexp.npggroups > 1) {
			(*pr)("   [%zd]: pages=%u, free=%u, freemin=%u,"
			    " free-target=%u, wired-max=%u\n",
			    grp - uvm.pggroups, grp->pgrp_npages, 
			    grp->pgrp_free, grp->pgrp_freemin,
			    grp->pgrp_freetarg, grp->pgrp_wiredmax);
			(*pr)("        active=%u, inactive=%u, kmem=%u,"
			    " anon=%u, file=%u, exec=%u\n",
			    grp->pgrp_active, grp->pgrp_inactive,
			    grp->pgrp_kmempages, grp->pgrp_anonpages,
			    grp->pgrp_filepages, grp->pgrp_execpages);
		} else {
			(*pr)("  active=%u, inactive=%u\n",
			    grp->pgrp_active, grp->pgrp_inactive);
		}
	}
	(*pr)("  faults=%d, traps=%d, intrs=%d, ctxswitch=%d\n",
	    uvmexp.faults, uvmexp.traps, uvmexp.intrs, uvmexp.swtch);
	(*pr)("  softint=%d, syscalls=%d, swapins=%d, swapouts=%d\n",
	    uvmexp.softs, uvmexp.syscalls, uvmexp.swapins, uvmexp.swapouts);

	(*pr)("  color counts:\n");
	for (u_int pggroup = 0; pggroup < uvmexp.ncolors; pggroup++) {
		const struct pgfreelist * const gpgfl = &uvm.page_free[pggroup];
		(*pr)("   color#%u: hit=%"PRIu64", miss=%"PRIu64
		    ", fail=%"PRIu64", any=%"PRIu64"\n", pggroup,
		    gpgfl->pgfl_colorhit, gpgfl->pgfl_colormiss,
		    gpgfl->pgfl_colorfail, gpgfl->pgfl_colorany);
#if defined(MULTIPROCESSOR)
		if (ncpu > 1) {
			CPU_INFO_ITERATOR cii;
			struct cpu_info *ci;
			for (CPU_INFO_FOREACH(cii, ci)) {
				const struct uvm_cpu * const ucpu =
				    &uvm.cpus[cpu_index(ci)];
				const struct pgfreelist * const pgfl =
				    &ucpu->page_free[pggroup];
				(*pr)("    cpu#%u: hit=%"PRIu64", miss=%"PRIu64
				    ", fail=%"PRIu64", any=%"PRIu64"\n",
				    cpu_index(ci),
				    pgfl->pgfl_colorhit, pgfl->pgfl_colormiss,
				    pgfl->pgfl_colorfail, pgfl->pgfl_colorany);
			}
			for (CPU_INFO_FOREACH(cii, ci)) {
				const struct uvm_cpu * const ucpu =
				    &uvm.cpus[cpu_index(ci)];
				(*pr)("   cpu%u: cpuhit=%"PRIu64", cpumiss=%"PRIu64"\n",
				    cpu_index(ci),
				    ucpu->page_cpuhit, ucpu->page_cpumiss);
			}
		}
#endif
	}
	(*pr)("  fault counts:\n");
	(*pr)("    noram=%d, noanon=%d, pgwait=%d, pgrele=%d\n",
	    uvmexp.fltnoram, uvmexp.fltnoanon, uvmexp.fltpgwait,
	    uvmexp.fltpgrele);
	(*pr)("    ok relocks(total)=%d(%d), anget(retrys)=%d(%d), "
	    "amapcopy=%d\n", uvmexp.fltrelckok, uvmexp.fltrelck,
	    uvmexp.fltanget, uvmexp.fltanretry, uvmexp.fltamcopy);
	(*pr)("    neighbor anon/obj pg=%d/%d, gets(lock/unlock)=%d/%d\n",
	    uvmexp.fltnamap, uvmexp.fltnomap, uvmexp.fltlget, uvmexp.fltget);
	(*pr)("    cases: anon=%d, anoncow=%d, obj=%d, prcopy=%d, przero=%d\n",
	    uvmexp.flt_anon, uvmexp.flt_acow, uvmexp.flt_obj, uvmexp.flt_prcopy,
	    uvmexp.flt_przero);

	(*pr)("  page daemon counts:\n");
	(*pr)("    woke=%d\n", uvmexp.pdwoke);
	STAILQ_FOREACH(grp, &uvm.page_groups, pgrp_uvm_link) {
		(*pr)("   group#%zd\n", grp - uvm.pggroups);
		(*pr)("    revs=%"PRIu64", scans=%"PRIu64
		    ", obscans=%"PRIu64", anscans=%"PRIu64"\n",
		    grp->pgrp_pdrevs, grp->pgrp_pdscans,
		    grp->pgrp_pdobscan, grp->pgrp_pdanscan);
		(*pr)("    busy=%"PRIu64", freed=%"PRIu64", reactivate=%"PRIu64
		    ", deactivate=%"PRIu64"\n",
		    grp->pgrp_pdbusy, grp->pgrp_pdfreed, grp->pgrp_pdreact,
		    grp->pgrp_pddeact);
		(*pr)("    pageouts=%"PRIu64", pending=%"PRIu64
		    ", paging=%"PRIu64"\n",
		    grp->pgrp_pdpageouts, grp->pgrp_pdpending,
		    grp->pgrp_paging);
	}
	(*pr)("  swap counts:\n");
	(*pr)("    nswapdev=%"PRIu64", swpgavail=%"PRIu64"\n",
	    uvmexp.nswapdev, uvmexp.swpgavail);
	(*pr)("    swpages=%d, swpginuse=%d, swpgonly=%d, nswget=%"PRIu64"\n",
	    uvmexp.swpages, uvmexp.swpginuse, uvmexp.swpgonly, uvmexp.nswget);
}
#endif

#if defined(READAHEAD_STATS)

#define	UVM_RA_EVCNT_DEFINE(name) \
struct evcnt uvm_ra_##name = \
EVCNT_INITIALIZER(EVCNT_TYPE_MISC, NULL, "readahead", #name); \
EVCNT_ATTACH_STATIC(uvm_ra_##name);

UVM_RA_EVCNT_DEFINE(total);
UVM_RA_EVCNT_DEFINE(hit);
UVM_RA_EVCNT_DEFINE(miss);

#endif /* defined(READAHEAD_STATS) */
