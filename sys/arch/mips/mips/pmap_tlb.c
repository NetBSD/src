/*	$NetBSD: pmap_tlb.c,v 1.1.2.1 2010/01/22 07:41:10 matt Exp $	*/

/*-
 * Copyright (c) 1998, 2001 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Matt Thomas at 3am Software Foundry.
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
 * Copyright (c) 1992, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * the Systems Programming Group of the University of Utah Computer
 * Science Department and Ralph Campbell.
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
 *	@(#)pmap.c	8.4 (Berkeley) 1/26/94
 */

#include <sys/cdefs.h>

__KERNEL_RCSID(0, "$NetBSD: pmap_tlb.c,v 1.1.2.1 2010/01/22 07:41:10 matt Exp $");

/*
 *	Manages physical address maps.
 *
 *	In addition to hardware address maps, this
 *	module is called upon to provide software-use-only
 *	maps which may or may not be stored in the same
 *	form as hardware maps.  These pseudo-maps are
 *	used to store intermediate results from copy
 *	operations to and from address spaces.
 *
 *	Since the information managed by this module is
 *	also stored by the logical address mapping module,
 *	this module may throw away valid virtual-to-physical
 *	mappings at almost any time.  However, invalidations
 *	of virtual-to-physical mappings must be done as
 *	requested.
 *
 *	In order to cope with hardware architectures which
 *	make virtual-to-physical map invalidates expensive,
 *	this module may delay invalidate or reduced protection
 *	operations until such time as they are actually
 *	necessary.  This module is given full information as
 *	to which processors are currently using which maps,
 *	and to when physical maps must be made correct.
 */

/* XXX simonb 2002/02/26
 *
 * MIPS3_PLUS is used to conditionally compile the r4k MMU support.
 * This is bogus - for example, some IDT MIPS-II CPUs have r4k style
 * MMUs (and 32-bit ones at that).
 *
 * On the other hand, it's not likely that we'll ever support the R6000
 * (is it?), so maybe that can be an "if MIPS2 or greater" check.
 *
 * Also along these lines are using totally separate functions for
 * r3k-style and r4k-style MMUs and removing all the MIPS_HAS_R4K_MMU
 * checks in the current functions.
 *
 * These warnings probably applies to other files under sys/arch/mips.
 */

#include "opt_sysv.h"
#include "opt_cputype.h"
#include "opt_mips_cache.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/user.h>
#include <sys/buf.h>
#include <sys/pool.h>
#include <sys/mutex.h>
#include <sys/atomic.h>
#ifdef SYSVSHM
#include <sys/shm.h>
#endif
#include <sys/socketvar.h>	/* XXX: for sock_loan_thresh */

#include <uvm/uvm.h>

#include <mips/cache.h>
#include <mips/cpuregs.h>
#include <mips/locore.h>
#include <mips/pte.h>

CTASSERT(NBPG >= sizeof(struct segtab));

#define	NTLBS	(PAGE_SIZE / sizeof(struct tlb))

int
pmap_tlb_update(pmap_t pm, vaddr_t va, uint32_t pte)
{
	struct cpu_info * const ci = curcpu();
	struct pmap_asid_info * const pai = PMAP_PAI(pm, ci);

	if (pm != pmap_kernel() && !PMAP_PAI_ASIDVALID_P(pai, ci))
		return -1;

	va |= pai->pai_asid << MIPS_TLB_PID_SHIFT;

	return tlb_update(va, pte);
}

void
pmap_tlb_invalidate_addr(pmap_t pm, vaddr_t va)
{
	struct cpu_info * const ci = curcpu();
	struct pmap_asid_info * const pai = PMAP_PAI(pm, ci);

	if (pm != pmap_kernel() && !PMAP_PAI_ASIDVALID_P(pai, ci))
		return;

	va |= pai->pai_asid << MIPS_TLB_PID_SHIFT;

	tlb_invalidate_addr(va);
}

void
pmap_tlb_invalidate_asid(pmap_t pm)
{
	struct cpu_info * const ci = curcpu();
	struct pmap_asid_info * const pai = PMAP_PAI(pm, ci);

	if (pm != pmap_kernel() && !PMAP_PAI_ASIDVALID_P(pai, ci))
		return;

	tlb_invalidate_asids(pai->pai_asid, pai->pai_asid + 1);
}

/*
 * Allocate a TLB address space tag (called ASID or TLBPID) and return it.
 * Each cpu has its own ASID space which might be a subset of the entire
 * ASID available (A core might have 256 ASIDs shared among N hw-threads).
 * To avoid dealing with locking, we just partition the ASIDs among the
 * hw-threads so each has its own independent space.
 *
 * Since all hw-threads share the TLB, we can't invalidate all non-global TLB
 * entries.  Instead we need to make sure they match the proper range of ASIDs
 * reserved for that CPU.
 *
 * Therefore, when we allocate a new ASID, we just take the next number. When
 * we run out of numbers, we flush the ASIDs the TLB, increment the generation
 * count and start over. The low ASID of the range is reserved for kernel use
 * (even though we only use 0 for that purpose).
 */
uint32_t
pmap_tlb_asid_alloc(pmap_t pmap, struct cpu_info *ci)
{
	struct pmap_asid_info * const pai = PMAP_PAI(pmap, ci);

	if (!PMAP_PAI_ASIDVALID_P(pai, ci)) {
//		TLB_LOCK(ci);
		if (ci->ci_pmap_asid_next == ci->ci_pmap_asid_max) {
			tlb_invalidate_asids(ci->ci_pmap_asid_reserved + 1,
			    ci->ci_pmap_asid_max);
			ci->ci_pmap_asid_generation++; /* ok to wrap to 0 */
			ci->ci_pmap_asid_next =		/* 0 means invalid */
			    ci->ci_pmap_asid_reserved + 1;
		}
		pai->pai_asid = ci->ci_pmap_asid_next++;
		pai->pai_asid_generation = ci->ci_pmap_asid_generation;
//		TLB_UNLOCK(ci);
	}

#ifdef DEBUGXX
	if (pmapdebug & (PDB_FOLLOW|PDB_TLBPID)) {
		printf("pmap_tlb_asid_alloc: curlwp %d.%d '%s' ",
		    curlwp->l_proc->p_pid, curlwp->l_lid,
		    curlwp->l_proc->p_comm);
		printf("segtab %p asid %d\n", pmap->pm_segtab, pai->pai_asid);
	}
#endif
	return pai->pai_asid;
}
