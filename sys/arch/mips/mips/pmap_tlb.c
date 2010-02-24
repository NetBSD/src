/*	$NetBSD: pmap_tlb.c,v 1.1.2.3 2010/02/24 00:30:21 matt Exp $	*/

/*-
 * Copyright (c) 2010 The NetBSD Foundation, Inc.
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

#include <sys/cdefs.h>

__KERNEL_RCSID(0, "$NetBSD: pmap_tlb.c,v 1.1.2.3 2010/02/24 00:30:21 matt Exp $");

/*
 * Manages address spaces in a TLB.
 *
 * Normally there is a 1:1 mapping between a TLB and a CPU.  However some
 * implementations may share a TLB between multiple CPUs (really CPU thread
 * contexts).  This requires the TLB abstraction to be separated from the
 * CPU abstraction.  It also requires that the TLB be locked while doing 
 * TLB activities.
 * 
 * For uniprocessors, since we know we can never have asynchronous 
 * reinitializations of the ASID space, we use a simple generational method
 * for ASIDs.  Simply allocate an ASID starting from MAX ASID - 1 and when
 * 0 (which is reserved for the kernel) is reached, increment the ASID
 * generation, flush the TLB of entries belonging to user ASIDs, and start
 * again from MAX ASID - 1.  At this point, all user pmaps have an invalid ASID.
 *
 * For multiprocessors, it just isn't that easy since we need to preserve the
 * ASIDs of any active LWPs.  We do use an algorithm similar to one used by
 * uniprocessors.  We allocate ASIDs until we have exhausted the supply, then
 * reinitialize the ASID space, and start allocating again.  However we need to
 * to preserve the ASIDs of any user pmaps assigned to a processor.  We do this
 * by keeping a bitmap of allocated ASIDs.  Whenever we reinitialize the TLB's
 * ASID space, we leave allocated any ASID in use by an "onproc" pmap.  When
 * allocating from the bitmap, we skip any ASID who has a corresponding bit set
 * in the bitmap.  Eventually this cause the bitmap to fill and cause a 
 * reinitialization of the ASID space.
 *
 * Each pmap has two bitmaps: pm_active and pm_onproc.  Each bit in pm_active
 * indicates whether that pmap has an allocated ASID for a CPU.  Each bit in
 * pm_onproc indicates that pmap's ASID is active (equal to the ASID in COP 0
 * register EntryHi) on a CPU.  The bit number comes from the CPU's cpu_index().
 * Even though these bitmaps contain the bits for all CPUs, the bits that
 * correspond to the bits beloning to the CPUs sharing a TLB can only be
 * manipulated while holding that TLB's lock.  Atomic ops must be used to 
 * update them since multiple CPUs may be changing different sets of bits at
 * same time but these sets never overlap.
 *
 * When a change to the local TLB may require a change in the TLB's of other
 * CPUs, we try to avoid sending an IPI if at all possible.  For instance, if
 * are updating a PTE and that PTE previously was invalid and therefore
 * couldn't support an active mapping, there's no need for an IPI since can be
 * no TLB entry to invalidate.  The other case is when we change a PTE to be
 * modified we just update the local TLB.  If another TLB has a stale entry,
 * a TLB MOD exception will be raised and that will cause the local TLB to be
 * updated.
 *
 * We never need to update a non-local TLB if the pmap doesn't have a valid
 * ASID for that TLB.  If it does have a valid ASID but isn't current "onproc"
 * we simply reset its ASID for that TLB and then time it goes "onproc" it
 * will allocate a new ASID and any existing TLB entries will be orphaned.
 * Only in the case that pmap has an "onproc" ASID do we actually have to send
 * an IPI.
 * 
 * Once we determined we must send an IPI to shootdown a TLB, we need to send
 * it to one of CPUs that share that TLB.  We choose the lowest numbered CPU
 * that has one of the pmap's ASID "onproc".  In reality, any CPU sharing that
 * TLB would do but interrupting an active CPU seems best.
 *
 * A TLB might have multiple shootdowns active concurrently.  The shootdown
 * logic compresses these into a few cases:
 *	0) nobody needs to have its TLB entries invalidated
 *	1) one ASID needs to have its TLB entries invalidated
 *	2) more than one ASID needs to have its TLB entries invalidated
 *	3) the kernel needs to have its TLB entries invalidated
 *	4) the kernel and one or more ASID need their TLB entries invalidated.
 *
 * And for each case we do:
 *	0) nothing,
 *	1) if that ASID is still "onproc", we invalidate the TLB entries for
 *	   that single ASID.  If not, just reset the pmap's ASID to invalidate
 *	   and let it allocated the next time it goes "onproc",
 *	2) we reinitialize the ASID space (preserving any "onproc" ASIDs) and
 *	   invalidate all non-wired non-global TLB entries,
 *	3) we invalidate all of the non-wired global TLB entries,
 *	4) we reinitialize the ASID space (again preserving any "onproc" ASIDs)
 *	   invalidate all non-wried TLB entries.
 *
 * As you can see, shootdowns are not concerned with addresses, just address
 * spaces.  Since the number of TLB entries is usually quite small, this avoids
 * a lot of overhead for not much gain.
 */

#include "opt_sysv.h"
#include "opt_cputype.h"
#include "opt_mips_cache.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/mutex.h>
#include <sys/atomic.h>
#include <sys/kernel.h>			/* for cold */
#include <sys/cpu.h>

#include <uvm/uvm.h>

#include <mips/cache.h>
#include <mips/cpuregs.h>
#include <mips/locore.h>
#include <mips/pte.h>

#ifdef MULTIPROCESSOR
static kmutex_t pmap_tlb0_mutex __aligned(32);
static struct pmap_tlb_info *pmap_tlbs[MAXCPUS] = {
	[0] = &pmap_tlb_info,
};
static u_int pmap_ntlbs = 1;
#endif

struct pmap_tlb_info pmap_tlb_info = {
#ifdef MULTIPROCESSOR
	.ti_asid_hint = 1,
	.ti_asids_free = MIPS_TLB_NUM_PIDS - 1,
	.ti_lock = &pmap_tlb0_mutex,
	.ti_cpu_mask = 1,
	.ti_tlbinvop = TLBINV_NOBODY,
	.ti_pais = LIST_HEAD_INITIALIZER(pmap_tlb_info.ti_pais),
#else
	.ti_asid_hint = MIPS_TLB_NUM_PIDS - 1,
#endif
	.ti_wired = MIPS3_TLB_WIRED_UPAGES,
};

#ifdef MULTIPROCESSOR
#define	__BITMAP_SET(bm, n) \
	((bm)[(n) / (8*sizeof(bm[0]))] |= 1LU << ((n) % (8*sizeof(bm[0]))))
#define	__BITMAP_CLR(bm, n) \
	((bm)[(n) / (8*sizeof(bm[0]))] &= ~(1LU << ((n) % (8*sizeof(bm[0])))))
#define	__BITMAP_ISSET_P(bm, n) \
	((bm)[(n) / (8*sizeof(bm[0]))] & (1LU << ((n) % (8*sizeof(bm[0])))))

#define	TLBINFO_ASID_MARK_USED(ti, asid) \
	__BITMAP_SET((ti)->ti_asid_bitmap, (asid))
#define	TLBINFO_ASID_INUSE_P(ti, asid) \
	__BITMAP_ISSET_P((ti)->ti_asid_bitmap, (asid))

static inline void
pmap_pai_reset(struct pmap_tlb_info *ti, struct pmap_asid_info *pai,
	struct pmap *pm)
{
	/*
	 * We must have an ASID but it must not be onproc (on a processor).
	 */
	KASSERT(pai->pai_asid);
	KASSERT((pm->pm_onproc & ti->ti_cpu_mask) == 0);
	LIST_REMOVE(pai, pai_link);
#ifdef DIAGNOSTIC
	pai->pai_link.le_prev = NULL;	/* tagged as unlinked */
#endif
	/*
	 * Note that we don't mark the ASID as not in use in the TLB's ASID
	 * bitmap (thus it can't allocated until the ASID space is exhausted
	 * and therefore reinitialized).  We don't want to flush the TLB for
	 * entries belonging to this ASID so we will let natural TLB entry
	 * replacement flush them out of the TLB.  Any new entries for this
	 * pmap will need a new ASID allocated.
	 */
	pai->pai_asid = 0; 

	/*
	 * The bits in pm_active belonging to this TLB can only be changed
	 * while this TLB's lock is held.
	 */
	atomic_and_32(&pm->pm_active, ~ti->ti_cpu_mask);
}

void
pmap_tlb_info_init(struct pmap_tlb_info *ti)
{
	if (ti == &pmap_tlb_info) {
		mutex_init(ti->ti_lock, MUTEX_DEFAULT, IPL_SCHED);
		return;
	}

	KASSERT(pmap_tlbs[pmap_ntlbs] == NULL);

	ti->ti_lock = mutex_obj_alloc(MUTEX_DEFAULT, IPL_SCHED);
	ti->ti_asid_bitmap[0] = 1;
	ti->ti_asid_hint = 1;
	ti->ti_asids_free = MIPS_TLB_NUM_PIDS - 1;
	ti->ti_tlbinvop = TLBINV_NOBODY,
	ti->ti_victim = NULL;
	ti->ti_cpu_mask = 0;
	ti->ti_index = pmap_ntlbs++;
	ti->ti_wired = 0;
	pmap_tlbs[ti->ti_index] = ti;
}

void
pmap_tlb_info_attach(struct pmap_tlb_info *ti, struct cpu_info *ci)
{
	KASSERT(!CPU_IS_PRIMARY(ci));
	KASSERT(ci->ci_data.cpu_idlelwp != NULL);
	KASSERT(cold);

	TLBINFO_LOCK(ti);
	uint32_t cpu_mask = 1 << cpu_index(ci);
	ti->ti_cpu_mask |= cpu_mask;
	ci->ci_tlb_info = ti;
	ci->ci_ksp_tlb_slot = ti->ti_wired++;
	/*
	 * Mark the kernel as active and "onproc" for this cpu.
	 */
	pmap_kernel()->pm_active |= cpu_mask;
	pmap_kernel()->pm_onproc |= cpu_mask;
	TLBINFO_UNLOCK(ti);
}
#endif /* MULTIPROCESSOR */

static void
pmap_tlb_asid_reinitialize(struct pmap_tlb_info *ti, bool all)
{
	if (all)
		tlb_invalidate_all();
	else
		tlb_invalidate_asids(1, MIPS_TLB_NUM_PIDS);

#ifdef MULTIPROCESSOR
	struct pmap_asid_info *pai, *next;

	/*
	 * First, clear the ASID bitmap (except for ASID 0 which belongs
	 * to the kernel).
	 */
	ti->ti_asids_free = MIPS_TLB_NUM_PIDS - 1;
	ti->ti_asid_bitmap[0] = 1;
	ti->ti_asid_hint = 1;
	for (size_t i = 1; i < __arraycount(ti->ti_asid_bitmap); i++)
		ti->ti_asid_bitmap[i] = 0;

	/*
	 * Now go through the active ASIDs and release them unless they
	 * are currently on a processor.  If they are on a process, then 
	 * mark them as in use.
	 */
	for (pai = LIST_FIRST(&ti->ti_pais); pai != NULL; pai = next) {
		struct pmap * const pm = PAI_PMAP(pai, ti);
		next = LIST_NEXT(pai, pai_link);
		if (pm->pm_onproc & ti->ti_cpu_mask) {
			KASSERT(pai->pai_asid != 0);
			TLBINFO_ASID_MARK_USED(ti, pai->pai_asid);
			ti->ti_asids_free--;
		} else {
			pmap_pai_reset(ti, pai, pm);
		}
	}

	ti->ti_tlbinvop = TLBINV_NOBODY;
	ti->ti_victim = NULL;
#else /* !MULTIPROCESSOR */
	ti->ti_asid_generation++;	/* ok to wrap to 0 */
	ti->ti_asid_hint = MIPS_TLB_NUM_PIDS - 1;
#endif
}

#ifdef MULTIPROCESSOR
void
pmap_tlb_shootdown_process(void)
{
	struct cpu_info * const ci = curcpu();
	struct pmap_tlb_info * const ti = ci->ci_tlb_info;
	struct pmap * const pm = curlwp->l_proc->p_vmspace->vm_map.pmap;

	TLBINFO_LOCK(ti);

	switch (ti->ti_tlbinvop) {
	case TLBINV_ONE: {
		/*
		 * We only need to invalidate one user ASID.
		 */
		struct pmap_asid_info * const pai = PMAP_PAI(ti->ti_victim, ti);
		KASSERT(ti->ti_victim != pmap_kernel());
		if (ti->ti_victim->pm_onproc & ti->ti_cpu_mask) {
			/*
			 * The victim is an active pmap so we will just
			 * invalidate its TLB entries.
			 */
			KASSERT(pai->pai_asid != 0);
			tlb_invalidate_asids(pai->pai_asid, pai->pai_asid + 1);
		} else if (pai->pai_asid) {
			/*
			 * The victim is no longer an active pmap for this TLB.
			 * So simply clear its ASID and when pmap_activate is
			 * next called for this pmap, it will allocate a new
			 * ASID.
			 */
			KASSERT((pm->pm_onproc & ti->ti_cpu_mask) == 0);
			pmap_pai_reset(ti, pai, PAI_PMAP(pai, ti));
		}
		break;
	}
	case TLBINV_ALLUSER:
		/*
		 * Flush all user TLB entries.
		 */
		pmap_tlb_asid_reinitialize(ti, false);
		break;
	case TLBINV_ALLKERNEL:
		/*
		 * We need to invalidate all global TLB entries.
		 */
		tlb_invalidate_globals();
		break;
	case TLBINV_ALL:
		/*
		 * Flush all the TLB entries (user and kernel).
		 */
		pmap_tlb_asid_reinitialize(ti, true);
		break;
	case TLBINV_NOBODY:
		/*
		 * Might be spurious or another SMT CPU sharing this TLB
		 * could have already done the work.
		 */
		break;
	}

	/*
	 * Indicate we are done with shutdown event.
	 */
	ti->ti_victim = NULL;
	ti->ti_tlbinvop = TLBINV_NOBODY;
	TLBINFO_UNLOCK(ti);
}

/*
 * This state machine could be encoded into an array of integers but since all
 * the values fit in 3 bits, the 5 entry "table" fits in a 16 bit value which
 * can be loaded in a single instruction.
 */
#define	TLBINV_MAP(op, nobody, one, alluser, allkernel, all)	\
	((((   (nobody) << 3*TLBINV_NOBODY)			\
	 | (      (one) << 3*TLBINV_ONE)			\
	 | (  (alluser) << 3*TLBINV_ALLUSER)			\
	 | ((allkernel) << 3*TLBINV_ALLKERNEL)			\
	 | (      (all) << 3*TLBINV_ALL)) >> 3*(op)) & 7)

#define	TLBINV_USER_MAP(op)	\
	TLBINV_MAP(op, TLBINV_ONE, TLBINV_ALLUSER, TLBINV_ALLUSER,	\
	    TLBINV_ALL, TLBINV_ALL)

#define	TLBINV_KERNEL_MAP(op)	\
	TLBINV_MAP(op, TLBINV_ALLKERNEL, TLBINV_ALL, TLBINV_ALL,	\
	    TLBINV_ALLKERNEL, TLBINV_ALL)

bool
pmap_tlb_shootdown_bystanders(pmap_t pm)
{
	/*
	 * We don't need to deal our own TLB.
	 */
	uint32_t pm_active = pm->pm_active & ~curcpu()->ci_tlb_info->ti_cpu_mask;
	const bool kernel_p = (pm == pmap_kernel());
	bool ipi_sent = false;

	/*
	 * If pm_active gets more bits set, then it's after all our changes
	 * have been made so they will already be cognizant of them.
	 */

	for (size_t i = 0; pm_active != 0; i++) {
		KASSERT(i < pmap_ntlbs);
		struct pmap_tlb_info * const ti = pmap_tlbs[i];
		KASSERT(tlbinfo_index(ti) == i);
		/*
		 * Skip this TLB if there are no active mappings for it.
		 */
		if ((pm_active & ti->ti_cpu_mask) == 0)
			continue;
		struct pmap_asid_info * const pai = PMAP_PAI(pm, ti);
		pm_active &= ~ti->ti_cpu_mask;
		TLBINFO_LOCK(ti);
		const uint32_t onproc = (pm->pm_onproc & ti->ti_cpu_mask);
		if (onproc != 0) {
			if (kernel_p) {
				ti->ti_tlbinvop =
				    TLBINV_KERNEL_MAP(ti->ti_tlbinvop);
				ti->ti_victim = NULL;
			} else {
				KASSERT(pai->pai_asid);
				if (__predict_false(ti->ti_victim == pm)) {
					KASSERT(ti->ti_tlbinvop == TLBINV_ONE);
					/*
					 * We still need to invalidate this one
					 * ASID so there's nothing to change.
					 */
				} else {
					ti->ti_tlbinvop =
					    TLBINV_USER_MAP(ti->ti_tlbinvop);
					if (ti->ti_tlbinvop == TLBINV_ONE)
						ti->ti_victim = pm;
					else
						ti->ti_victim = NULL;
				}
			}
			TLBINFO_UNLOCK(ti);
			/*
			 * Now we can send out the shootdown IPIs to a CPU
			 * that shares this TLB and is currently using this
			 * pmap.  That CPU will process the IPI and do the
			 * all the work.  Any other CPUs sharing that TLB
			 * will take advantage of that work.  pm_onproc might
			 * change now that we have released the lock but we
			 * can tolerate spurious shootdowns.
			 */
			KASSERT(onproc != 0);
			u_int j = ffs(onproc) - 1;
			cpu_send_ipi(cpu_lookup(j), IPI_SHOOTDOWN);
			ipi_sent = true;
			continue;
		}
		if (pm->pm_active & ti->ti_cpu_mask) {
			/*
			 * If this pmap has an ASID assigned but it's not
			 * currently running, nuke its ASID.  Next time the
			 * pmap is activated, it will allocate a new ASID.
			 * And best of all, we avoid an IPI.
			 */
			KASSERT(!kernel_p);
			pmap_pai_reset(ti, pai, pm);
			//ti->ti_evcnt_lazy_shots.ev_count++;
		}
		TLBINFO_UNLOCK(ti);
	}

	return ipi_sent;
}
#endif /* MULTIPROCESSOR */

int
pmap_tlb_update_addr(pmap_t pm, vaddr_t va, uint32_t pt_entry, bool need_ipi)
{
	struct pmap_tlb_info * const ti = curcpu()->ci_tlb_info;
	struct pmap_asid_info * const pai = PMAP_PAI(pm, ti);
	int rv = -1;

	TLBINFO_LOCK(ti);
	if (pm == pmap_kernel() || PMAP_PAI_ASIDVALID_P(pai, ti)) {
		va |= pai->pai_asid << MIPS_TLB_PID_SHIFT;
		rv = tlb_update(va, pt_entry);
	}
#ifdef MULTIPROCESSOR
	pm->pm_shootdown_pending = need_ipi;
#endif
	TLBINFO_UNLOCK(ti);

	return rv;
}

void
pmap_tlb_invalidate_addr(pmap_t pm, vaddr_t va)
{
	struct pmap_tlb_info * const ti = curcpu()->ci_tlb_info;
	struct pmap_asid_info * const pai = PMAP_PAI(pm, ti);

	TLBINFO_LOCK(ti);
	if (pm == pmap_kernel() || PMAP_PAI_ASIDVALID_P(pai, ti)) {
		va |= pai->pai_asid << MIPS_TLB_PID_SHIFT;
		tlb_invalidate_addr(va);
	}
#ifdef MULTIPROCESSOR
	pm->pm_shootdown_pending = 1;
#endif
	TLBINFO_UNLOCK(ti);
}

static inline void
pmap_tlb_asid_alloc(struct pmap_tlb_info *ti, pmap_t pm,
	struct pmap_asid_info *pai)
{
#ifdef MULTIPROCESSOR
	/*
	 * We shouldn't have an ASID assigned, and thusly must not be onproc
	 * nor active.
	 */
	KASSERT(pai->pai_asid == 0);
	KASSERT(pai->pai_link.le_prev == NULL);
	KASSERT((pm->pm_onproc & ti->ti_cpu_mask) == 0);
	KASSERT((pm->pm_active & ti->ti_cpu_mask) == 0);
	KASSERT(ti->ti_asid_hint < MIPS_TLB_NUM_PIDS);

	/*
	 * Let's see if the hinted ASID is free.  If not search for
	 * a new one.
	 */
	if (TLBINFO_ASID_INUSE_P(ti, ti->ti_asid_hint)) {
		const size_t words = __arraycount(ti->ti_asid_bitmap);
		const size_t nbpw = 8 * sizeof(ti->ti_asid_bitmap[0]);
		for (size_t i = 0; i < ti->ti_asid_hint / nbpw; i++) {
			KASSERT(~ti->ti_asid_bitmap[i] == 0);
		}
		for (size_t i = ti->ti_asid_hint / nbpw;; i++) {
			KASSERT(i < words);
			/*
			 * ffs was to find the first bit set while we want the
			 * to find the first bit cleared.
			 */
			const u_int bits = ~ti->ti_asid_bitmap[i]; 
			if (bits) {
				u_int n = ffs(bits) - 1;
				KASSERT(n < 32);
				ti->ti_asid_hint = n + i * nbpw;
				break;
			}
		}
		KASSERT(ti->ti_asid_hint != 0);
		KASSERT(!TLBINFO_ASID_INUSE_P(ti, ti->ti_asid_hint));
	}

	/*
	 * The hint contains our next ASID so take it and advance the hint.
	 * Mark it as used and insert the pai into the list of active asids.
	 * There is also one less asid free in this TLB.
	 */
	pai->pai_asid = ti->ti_asid_hint++;
	TLBINFO_ASID_MARK_USED(ti, pai->pai_asid);
	LIST_INSERT_HEAD(&ti->ti_pais, pai, pai_link);
	ti->ti_asids_free--;

	/*
	 * Mark that we now an active ASID for all CPUs sharing this TLB.
	 * The bits in pm_active belonging to this TLB can only be changed
	 * while this TLBs lock is held.
	 */
	atomic_or_32(&pm->pm_active, ti->ti_cpu_mask);
#else
	/*
	 * Just grab the next ASID and remember the current generation.
	 */
	KASSERT(ti->ti_asid_hint < MIPS_TLB_NUM_PIDS);
	KASSERT(ti->ti_asid_hint > 0);
	pai->pai_asid = ti->ti_asid_hint--;
	pai->pai_asid_generation = ti->ti_asid_generation;
#endif
}

/*
 * Acquire a TLB address space tag (called ASID or TLBPID) and return it.
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
void
pmap_tlb_asid_acquire(pmap_t pm, struct lwp *l)
{
	struct cpu_info * const ci = l->l_cpu;
	struct pmap_tlb_info * const ti = ci->ci_tlb_info;
	struct pmap_asid_info * const pai = PMAP_PAI(pm, ti);

	/*
	 * Kernels use a fixed ASID of 0 and don't need to acquire one.
	 */
	if (pm == pmap_kernel())
		return;

	TLBINFO_LOCK(ti);
	if (!PMAP_PAI_ASIDVALID_P(pai, ti)) {
		/*
		 * If we've run out ASIDs, reinitialize the ASID space.
		 */
		if (__predict_false(tlbinfo_noasids_p(ti))) {
			KASSERT(l == curlwp);
			pmap_tlb_asid_reinitialize(ti, false);
		}

		/*
		 * Get an ASID.
		 */
		pmap_tlb_asid_alloc(ti, pm, pai);
	}

	if (l == curlwp) {
#ifdef MULTIPROCESSOR
		/*
		 * The bits in pm_onproc belonging to this TLB can only
		 * be changed while this TLBs lock is held.
		 */
		atomic_or_32(&pm->pm_onproc, 1 << cpu_index(ci));
#endif
		tlb_set_asid(pai->pai_asid);
	}
	TLBINFO_UNLOCK(ti);

#ifdef DEBUGXX
	if (pmapdebug & (PDB_FOLLOW|PDB_TLBPID)) {
		printf("pmap_tlb_asid_alloc: curlwp %d.%d '%s' ",
		    curlwp->l_proc->p_pid, curlwp->l_lid,
		    curlwp->l_proc->p_comm);
		printf("segtab %p asid %d\n", pm->pm_segtab, pai->pai_asid);
	}
#endif
}

void
pmap_tlb_asid_deactivate(pmap_t pm)
{
#ifdef MULTIPROCESSOR
	/*
	 * The kernel pmap is aways onproc and active and must never have
	 * those bits cleared.
	 */
	if (pm != pmap_kernel()) {
		struct cpu_info * const ci = curcpu();
		struct pmap_tlb_info * const ti = ci->ci_tlb_info;
		TLBINFO_LOCK(ti);
		/*
		 * The bits in pm_onproc belonging to this TLB can only
		 * be changed while this TLBs lock is held.
		 */
		atomic_and_32(&pm->pm_onproc, ~(1 << cpu_index(ci)));
		TLBINFO_UNLOCK(ti);
	}
#endif
}

void
pmap_tlb_asid_release_all(struct pmap *pm)
{
#ifdef MULTIPROCESSOR
	KASSERT(pm != pmap_kernel());
	KASSERT(pm->pm_onproc == 0);
	for (u_int i = 0; pm->pm_active != 0; i++) {
		KASSERT(i < pmap_ntlbs);
		struct pmap_tlb_info * const ti = pmap_tlbs[i];
		if (pm->pm_active & ti->ti_cpu_mask) {
			struct pmap_asid_info * const pai = PMAP_PAI(pm, ti);
			TLBINFO_LOCK(ti);
			KASSERT(ti->ti_victim != pm);
			pmap_pai_reset(ti, pai, pm);
			TLBINFO_UNLOCK(ti);
		}
	}
#endif /* MULTIPROCESSOR */
}
