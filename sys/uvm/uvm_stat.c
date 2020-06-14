/*	$NetBSD: uvm_stat.c,v 1.46 2020/06/14 21:41:42 ad Exp $	 */

/*
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
__KERNEL_RCSID(0, "$NetBSD: uvm_stat.c,v 1.46 2020/06/14 21:41:42 ad Exp $");

#include "opt_readahead.h"
#include "opt_ddb.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/cpu.h>

#include <uvm/uvm.h>
#include <uvm/uvm_ddb.h>

#ifdef DDB

#include <sys/pool.h>

/*
 * uvmexp_print: ddb hook to print interesting uvm counters
 */
void
uvmexp_print(void (*pr)(const char *, ...)
    __attribute__((__format__(__printf__,1,2))))
{
	int64_t anonpg, execpg, filepg;
	int active, inactive;
	int poolpages, freepg;

	uvm_estimatepageable(&active, &inactive);
	poolpages = pool_totalpages_locked();

	/* this will sync all counters. */
	freepg = uvm_availmem(false);

	anonpg = cpu_count_get(CPU_COUNT_ANONCLEAN) +
	    cpu_count_get(CPU_COUNT_ANONDIRTY) +
	    cpu_count_get(CPU_COUNT_ANONUNKNOWN);
	execpg = cpu_count_get(CPU_COUNT_EXECPAGES);
	filepg = cpu_count_get(CPU_COUNT_FILECLEAN) +
	    cpu_count_get(CPU_COUNT_FILEDIRTY) +
	    cpu_count_get(CPU_COUNT_FILEUNKNOWN) -
	    execpg;

	(*pr)("Current UVM status:\n");
	(*pr)("  pagesize=%d (0x%x), pagemask=0x%x, pageshift=%d, ncolors=%d\n",
	    uvmexp.pagesize, uvmexp.pagesize, uvmexp.pagemask,
	    uvmexp.pageshift, uvmexp.ncolors);
	(*pr)("  %d VM pages: %d active, %d inactive, %d wired, %d free\n",
	    uvmexp.npages, active, inactive, uvmexp.wired, freepg);
	(*pr)("  pages  %" PRId64 " anon, %" PRId64 " file, %" PRId64 " exec\n",
	    anonpg, filepg, execpg);
	(*pr)("  freemin=%d, free-target=%d, wired-max=%d\n",
	    uvmexp.freemin, uvmexp.freetarg, uvmexp.wiredmax);
	(*pr)("  resv-pg=%d, resv-kernel=%d\n",
	    uvmexp.reserve_pagedaemon, uvmexp.reserve_kernel);
	(*pr)("  bootpages=%d, poolpages=%d\n",
	    uvmexp.bootpages, poolpages);

	(*pr)("  faults=%" PRId64 ", traps=%" PRId64 ", "
	      "intrs=%" PRId64 ", ctxswitch=%" PRId64 "\n",
	    cpu_count_get(CPU_COUNT_NFAULT),
	    cpu_count_get(CPU_COUNT_NTRAP),
	    cpu_count_get(CPU_COUNT_NINTR),
	    cpu_count_get(CPU_COUNT_NSWTCH));
	(*pr)("   softint=%" PRId64 ", syscalls=%" PRId64 "\n",
	    cpu_count_get(CPU_COUNT_NSOFT),
	    cpu_count_get(CPU_COUNT_NSYSCALL));

	(*pr)("  fault counts:\n");
	(*pr)("    noram=%" PRId64 ", noanon=%" PRId64 ", pgwait=%" PRId64
	    ", pgrele=%" PRId64 "\n",
	    cpu_count_get(CPU_COUNT_FLTNORAM),
	    cpu_count_get(CPU_COUNT_FLTNOANON),
	    cpu_count_get(CPU_COUNT_FLTPGWAIT),
	    cpu_count_get(CPU_COUNT_FLTPGRELE));
	(*pr)("    ok relocks(total)=%" PRId64 "(%" PRId64 "), "
	    "anget(retrys)=%" PRId64 "(%" PRId64 "), amapcopy=%" PRId64 "\n",
	    cpu_count_get(CPU_COUNT_FLTRELCKOK),
	    cpu_count_get(CPU_COUNT_FLTRELCK),
	    cpu_count_get(CPU_COUNT_FLTANGET),
	    cpu_count_get(CPU_COUNT_FLTANRETRY),
	    cpu_count_get(CPU_COUNT_FLTAMCOPY));
	(*pr)("    neighbor anon/obj pg=%" PRId64 "/%" PRId64
	    ", gets(lock/unlock)=%" PRId64 "/%" PRId64 "\n",
	    cpu_count_get(CPU_COUNT_FLTNAMAP),
	    cpu_count_get(CPU_COUNT_FLTNOMAP),
	    cpu_count_get(CPU_COUNT_FLTLGET),
	    cpu_count_get(CPU_COUNT_FLTGET));
	(*pr)("    cases: anon=%" PRId64 ", anoncow=%" PRId64 ", obj=%" PRId64
	    ", prcopy=%" PRId64 ", przero=%" PRId64 "\n",
	    cpu_count_get(CPU_COUNT_FLT_ANON),
	    cpu_count_get(CPU_COUNT_FLT_ACOW),
	    cpu_count_get(CPU_COUNT_FLT_OBJ),
	    cpu_count_get(CPU_COUNT_FLT_PRCOPY),
	    cpu_count_get(CPU_COUNT_FLT_PRZERO));

	(*pr)("  daemon and swap counts:\n");
	(*pr)("    woke=%d, revs=%d, scans=%d, obscans=%d, anscans=%d\n",
	    uvmexp.pdwoke, uvmexp.pdrevs, uvmexp.pdscans, uvmexp.pdobscan,
	    uvmexp.pdanscan);
	(*pr)("    busy=%d, freed=%d, reactivate=%d, deactivate=%d\n",
	    uvmexp.pdbusy, uvmexp.pdfreed, uvmexp.pdreact, uvmexp.pddeact);
	(*pr)("    pageouts=%d, pending=%d, nswget=%d\n", uvmexp.pdpageouts,
	    uvmexp.pdpending, uvmexp.nswget);
	(*pr)("    nswapdev=%d, swpgavail=%d\n",
	    uvmexp.nswapdev, uvmexp.swpgavail);
	(*pr)("    swpages=%d, swpginuse=%d, swpgonly=%d, paging=%d\n",
	    uvmexp.swpages, uvmexp.swpginuse, uvmexp.swpgonly, uvmexp.paging);
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
