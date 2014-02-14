/*	pmap_tlb.c,v 1.1.2.22 2012/01/19 08:28:50 matt Exp	*/

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

__KERNEL_RCSID(0, "pmap_tlb.c,v 1.1.2.22 2012/01/19 08:28:50 matt Exp");

/*
 * Manages address spaces in a TLB.
 *
 * Normally there is a 1:1 mapping between a TLB and a CPU.  However, some
 * implementations may share a TLB between multiple CPUs (really CPU thread
 * contexts).  This requires the TLB abstraction to be separated from the
 * CPU abstraction.  It also requires that the TLB be locked while doing
 * TLB activities.
 *
 * For each TLB, we track the ASIDs in use in a bitmap and a list of pmaps
 * that have a valid ASID.
 *
 * We allocate ASIDs in increasing order until we have exhausted the supply,
 * then reinitialize the ASID space, and start allocating again at 1.  When
 * allocating from the ASID bitmap, we skip any ASID who has a corresponding
 * bit set in the ASID bitmap.  Eventually this causes the ASID bitmap to fill
 * and, when completely filled, a reinitialization of the ASID space.
 *
 * To reinitialize the ASID space, the ASID bitmap is reset and then the ASIDs
 * of non-kernel TLB entries get recorded in the ASID bitmap.  If the entries
 * in TLB consume more than half of the ASID space, all ASIDs are invalidated,
 * the ASID bitmap is recleared, and the list of pmaps is emptied.  Otherwise,
 * (the normal case), any ASID present in the TLB (even those which are no
 * longer used by a pmap) will remain active (allocated) and all other ASIDs
 * will be freed.  If the size of the TLB is much smaller than the ASID space,
 * this algorithm completely avoids TLB invalidation.
 *
 * For multiprocessors, we also have to deal TLB invalidation requests from
 * other CPUs, some of which are dealt with the reinitialization of the ASID
 * space.  Whereas above we keep the ASIDs of those pmaps which have active
 * TLB entries, this type of reinitialization preserves the ASIDs of any
 * "onproc" user pmap and all other ASIDs will be freed.  We must do this
 * since we can't change the current ASID.
 *
 * Each pmap has two bitmaps: pm_active and pm_onproc.  Each bit in pm_active
 * indicates whether that pmap has an allocated ASID for a CPU.  Each bit in
 * pm_onproc indicates that pmap's ASID is active (equal to the ASID in COP 0
 * register EntryHi) on a CPU.  The bit number comes from the CPU's cpu_index().
 * Even though these bitmaps contain the bits for all CPUs, the bits that
 * correspond to the bits belonging to the CPUs sharing a TLB can only be
 * manipulated while holding that TLB's lock.  Atomic ops must be used to
 * update them since multiple CPUs may be changing different sets of bits at
 * same time but these sets never overlap.
 *
 * When a change to the local TLB may require a change in the TLB's of other
 * CPUs, we try to avoid sending an IPI if at all possible.  For instance, if
 * we are updating a PTE and that PTE previously was invalid and therefore
 * couldn't support an active mapping, there's no need for an IPI since there
 * can't be a TLB entry to invalidate.  The other case is when we change a PTE
 * to be modified we just update the local TLB.  If another TLB has a stale
 * entry, a TLB MOD exception will be raised and that will cause the local TLB
 * to be updated.
 *
 * We never need to update a non-local TLB if the pmap doesn't have a valid
 * ASID for that TLB.  If it does have a valid ASID but isn't current "onproc"
 * we simply reset its ASID for that TLB and then when it goes "onproc" it
 * will allocate a new ASID and any existing TLB entries will be orphaned.
 * Only in the case that pmap has an "onproc" ASID do we actually have to send
 * an IPI.
 *
 * Once we determined we must send an IPI to shootdown a TLB, we need to send
 * it to one of CPUs that share that TLB.  We choose the lowest numbered CPU
 * that has one of the pmap's ASID "onproc".  In reality, any CPU sharing that
 * TLB would do, but interrupting an active CPU seems best.
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
 *	   and let it be allocated the next time it goes "onproc",
 *	2) we reinitialize the ASID space (preserving any "onproc" ASIDs) and
 *	   invalidate all non-wired non-global TLB entries,
 *	3) we invalidate all of the non-wired global TLB entries,
 *	4) we reinitialize the ASID space (again preserving any "onproc" ASIDs)
 *	   invalidate all non-wired TLB entries.
 *
 * As you can see, shootdowns are not concerned with addresses, just address
 * spaces.  Since the number of TLB entries is usually quite small, this avoids
 * a lot of overhead for not much gain.
 */

#include "opt_multiprocessor.h"
#include "opt_sysv.h"
#include "opt_cputype.h"
#include "opt_mips_cache.h"
#include "opt_multiprocessor.h"

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

static kmutex_t pmap_tlb0_mutex __cacheline_aligned;
#ifdef MULTIPROCESSOR
static kmutex_t pmap_tlb0_hwmutex __cacheline_aligned;
#endif

struct pmap_tlb_info pmap_tlb0_info = {
	.ti_name = "tlb0",
	.ti_asid_hint = 1,
	.ti_asid_mask = __builtin_constant_p(MIPS_TLB_NUM_PIDS) ? MIPS_TLB_NUM_PIDS - 1 : 0,
	.ti_asid_max = __builtin_constant_p(MIPS_TLB_NUM_PIDS) ? MIPS_TLB_NUM_PIDS - 1 : 0,
	.ti_asids_free = __builtin_constant_p(MIPS_TLB_NUM_PIDS) ? MIPS_TLB_NUM_PIDS - 1 : 0,
	.ti_asid_bitmap[0] = 1,
	.ti_wired = MIPS3_TLB_WIRED_UPAGES,
	.ti_lock = &pmap_tlb0_mutex,
	.ti_pais = LIST_HEAD_INITIALIZER(pmap_tlb_info.ti_pais),
#ifdef MULTIPROCESSOR
	.ti_hwlock = &pmap_tlb0_hwmutex,
	.ti_cpu_mask = 1,
	.ti_tlbinvop = TLBINV_NOBODY,
#endif
};

#ifdef MULTIPROCESSOR
struct pmap_tlb_info *pmap_tlbs[MAXCPUS] = {
	[0] = &pmap_tlb0_info,
};
u_int pmap_ntlbs = 1;
#endif
#define	__BITMAP_SET(bm, n) \
	((bm)[(n) / (8*sizeof(bm[0]))] |= 1LU << ((n) % (8*sizeof(bm[0]))))
#define	__BITMAP_CLR(bm, n) \
	((bm)[(n) / (8*sizeof(bm[0]))] &= ~(1LU << ((n) % (8*sizeof(bm[0])))))
#define	__BITMAP_ISSET_P(bm, n) \
	(((bm)[(n) / (8*sizeof(bm[0]))] & (1LU << ((n) % (8*sizeof(bm[0]))))) != 0)

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
	KASSERT(pai->pai_asid > KERNEL_PID);
#ifdef MULTIPROCESSOR
	KASSERT((pm->pm_onproc & ti->ti_cpu_mask) == 0);
#endif
	LIST_REMOVE(pai, pai_link);
#ifdef DIAGNOSTIC
	pai->pai_link.le_prev = NULL;	/* tagged as unlinked */
#endif
	/*
	 * Note that we don't mark the ASID as not in use in the TLB's ASID
	 * bitmap (thus it can't be allocated until the ASID space is exhausted
	 * and therefore reinitialized).  We don't want to flush the TLB for
	 * entries belonging to this ASID so we will let natural TLB entry
	 * replacement flush them out of the TLB.  Any new entries for this
	 * pmap will need a new ASID allocated.
	 */
	pai->pai_asid = 0;

#ifdef MULTIPROCESSOR
	/*
	 * The bits in pm_active belonging to this TLB can only be changed
	 * while this TLB's lock is held.
	 */
	atomic_and_32(&pm->pm_active, ~ti->ti_cpu_mask);
#endif /* MULTIPROCESSOR */
}

void
pmap_tlb_info_evcnt_attach(struct pmap_tlb_info *ti)
{
	KDASSERT(ti->ti_name[0] == 't');
	KDASSERT(ti->ti_name[1] == 'l');
	KDASSERT(ti->ti_name[2] == 'b');
	evcnt_attach_dynamic(&ti->ti_evcnt_asid_reinits,
	    EVCNT_TYPE_MISC, NULL,
	    ti->ti_name, "asid pool reinit");
	evcnt_attach_dynamic(&ti->ti_evcnt_asid_reclaims,
	    EVCNT_TYPE_MISC, NULL,
	    ti->ti_name, "asid pool reclaims");
}

void
pmap_tlb_info_init(struct pmap_tlb_info *ti)
{
	const struct mips_options * const opts = &mips_options;
#ifdef MULTIPROCESSOR
	if (ti == &pmap_tlb0_info) {
#endif /* MULTIPROCESSOR */
		KASSERT(ti == &pmap_tlb0_info);
		mutex_init(ti->ti_lock, MUTEX_DEFAULT, IPL_SCHED);
#ifdef MULTIPROCESSOR
		mutex_init(ti->ti_hwlock, MUTEX_DEFAULT, IPL_SCHED);
#endif
		KASSERT(opts->mips_num_tlb_asids > 0);
		if (!CPUISMIPSNN || !__builtin_constant_p(MIPS_TLB_NUM_PIDS)
		    || ti->ti_asid_max != opts->mips_num_tlb_asids - 1) {
			KASSERT(ti->ti_asid_max + 1 <= sizeof(ti->ti_asid_bitmap)*8);
			ti->ti_asid_max = opts->mips_num_tlb_asids - 1;
			ti->ti_asids_free = ti->ti_asid_max;
			ti->ti_asid_mask = ti->ti_asid_max;
			/*
			 * Now figure out what mask we need to focus on
			 * asid_max.
			 */
			while ((ti->ti_asid_mask + 1) & ti->ti_asid_mask) {
				ti->ti_asid_mask |= ti->ti_asid_mask >> 1;
			}
		}
		return;
#ifdef MULTIPROCESSOR
	}

	KASSERT(pmap_tlbs[pmap_ntlbs] == NULL);

	if (ti->ti_lock == NULL)
		ti->ti_lock = mutex_obj_alloc(MUTEX_DEFAULT, IPL_SCHED);
	else
		mutex_init(ti->ti_lock, MUTEX_DEFAULT, IPL_SCHED);
	if (ti->ti_hwlock == NULL)
		ti->ti_hwlock = mutex_obj_alloc(MUTEX_DEFAULT, IPL_SCHED);
	else
		mutex_init(ti->ti_hwlock, MUTEX_DEFAULT, IPL_SCHED);
	ti->ti_asid_bitmap[0] = 1;
	ti->ti_asid_hint = 1;
	ti->ti_asid_max = pmap_tlb0_info.ti_asid_max;
	ti->ti_asid_mask = pmap_tlb0_info.ti_asid_mask;
	ti->ti_asids_free = ti->ti_asid_max;
	ti->ti_tlbinvop = TLBINV_NOBODY,
	ti->ti_victim = NULL;
	ti->ti_cpu_mask = 0;
	ti->ti_index = pmap_ntlbs++;
	snprintf(ti->ti_name, sizeof(ti->ti_name), "tlb%u", ti->ti_index);

	KASSERT(ti != &pmap_tlb0_info);
	pmap_tlb_info_evcnt_attach(ti);

	/*
	 * If we are reserving a tlb slot for mapping cpu_info,
	 * allocate it now.
	 */
	ti->ti_wired = (cpu_info_store.ci_tlb_slot >= 0);
	pmap_tlbs[ti->ti_index] = ti;
	KDASSERT(ti->ti_name[0] == 't');
	KDASSERT(ti->ti_name[1] == 'l');
	KDASSERT(ti->ti_name[2] == 'b');
#endif /* MULTIPROCESSOR */
}

#ifdef MULTIPROCESSOR
void
pmap_tlb_info_attach(struct pmap_tlb_info *ti, struct cpu_info *ci)
{
	KASSERT(!CPU_IS_PRIMARY(ci));
	KASSERT(ci->ci_data.cpu_idlelwp != NULL);
	KASSERT(cold);

	KDASSERT(ti->ti_name[0] == 't');
	KDASSERT(ti->ti_name[1] == 'l');
	KDASSERT(ti->ti_name[2] == 'b');

	TLBINFO_LOCK(ti);
	uint32_t cpu_mask = 1 << cpu_index(ci);
	ti->ti_cpu_mask |= cpu_mask;
	ci->ci_tlb_info = ti;
	ci->ci_ksp_tlb_slot = ti->ti_wired++;
	/*
	 * If we need a tlb slot for mapping cpu_info, use 0.  If we don't
	 * need one then ci_tlb_slot will be -1, and so will ci->ci_tlb_slot
	 */
	ci->ci_tlb_slot = -(cpu_info_store.ci_tlb_slot < 0);
	/*
	 * Mark the kernel as active and "onproc" for this cpu.  We assume
	 * we are the only CPU running so atomic ops are not needed.
	 */
	pmap_kernel()->pm_active |= cpu_mask;
	pmap_kernel()->pm_onproc |= cpu_mask;
	TLBINFO_UNLOCK(ti);
}
#endif /* MULTIPROCESSOR */

#ifdef DIAGNOSTIC
static size_t
pmap_tlb_asid_count(struct pmap_tlb_info *ti)
{
	size_t count = 0;
	for (uint32_t asid = 1; asid <= ti->ti_asid_max; asid++) {
		count += TLBINFO_ASID_INUSE_P(ti, asid);
	}
	return count;
}
#endif

static void
pmap_tlb_asid_reinitialize(struct pmap_tlb_info *ti, enum tlb_invalidate_op op)
{
	const size_t asid_bitmap_words =
	    ti->ti_asid_max / (8 * sizeof(ti->ti_asid_bitmap[0]));
	/*
	 * First, clear the ASID bitmap (except for ASID 0 which belongs
	 * to the kernel).
	 */
	ti->ti_asids_free = ti->ti_asid_max;
	ti->ti_asid_hint = 1;
	ti->ti_asid_bitmap[0] = 1;
	for (size_t word = 1; word <= asid_bitmap_words; word++)
		ti->ti_asid_bitmap[word] = 0;
#ifdef DIAGNOSTIC
	for (size_t word = asid_bitmap_words + 1;
	     word < __arraycount(ti->ti_asid_bitmap);
	     word++) {
		KASSERT(ti->ti_asid_bitmap[word] == 0);
	}
#endif

	switch (op) {
	case TLBINV_ALL:
		tlb_invalidate_all();
		break;
	case TLBINV_ALLUSER:
		tlb_invalidate_asids(1, ti->ti_asid_max + 1);
		break;
	case TLBINV_NOBODY: {
		/*
		 * If we are just reclaiming ASIDs in the TLB, let's go find
		 * what ASIDs are in use in the TLB.  Since this is a
		 * semi-expensive operation, we don't want to do it too often.
		 * So if more half of the ASIDs are in use, we don't have
		 * enough free ASIDs so invalidate the TLB entries with ASIDs
		 * and clear the ASID bitmap.  That will force everyone to
		 * allocate a new ASID.
		 */
		pmap_tlb_asid_check();
		const u_int asids_found = tlb_record_asids(ti->ti_asid_bitmap,
		    ti->ti_asid_mask);
		pmap_tlb_asid_check();
		KASSERT(asids_found == pmap_tlb_asid_count(ti));
		if (__predict_false(asids_found >= ti->ti_asid_max / 2)) {
			tlb_invalidate_asids(1, ti->ti_asid_max + 1);
			ti->ti_evcnt_asid_reinits.ev_count++;
			ti->ti_asid_bitmap[0] = 1;
			for (size_t word = 1; word <= asid_bitmap_words; word++)
				ti->ti_asid_bitmap[word] = 0;
		} else {
			ti->ti_evcnt_asid_reclaims.ev_count++;
			ti->ti_asids_free -= asids_found;
		}
		break;
	}
	default:
		panic("%s: unexpected op %d", __func__, op);
	}

	/*
	 * Now go through the active ASIDs.  If the ASID is on a processor or
	 * we aren't invalidating all ASIDs and the TLB has an entry owned by
	 * that ASID, mark it as in use.  Otherwise release the ASID.
	 */
	struct pmap_asid_info *pai, *next;
	for (pai = LIST_FIRST(&ti->ti_pais); pai != NULL; pai = next) {
		struct pmap * const pm = PAI_PMAP(pai, ti);
		next = LIST_NEXT(pai, pai_link);
		KASSERT(pai->pai_asid > KERNEL_PID);
#ifdef MULTIPROCESSOR
		if (pm->pm_onproc & ti->ti_cpu_mask) {
			if (!TLBINFO_ASID_INUSE_P(ti, pai->pai_asid)) {
				TLBINFO_ASID_MARK_USED(ti, pai->pai_asid);
				ti->ti_asids_free--;
			}
			continue;
		}
#endif /* MULTIPROCESSOR */
		if (TLBINFO_ASID_INUSE_P(ti, pai->pai_asid)) {
			KASSERT(op == TLBINV_NOBODY);
		} else {
			pmap_pai_reset(ti, pai, pm);
		}
	}
#ifdef DIAGNOSTIC
	size_t free_count = ti->ti_asid_max - pmap_tlb_asid_count(ti);
	if (free_count != ti->ti_asids_free)
		panic("%s: bitmap error: %zu != %u",
		    __func__, free_count, ti->ti_asids_free);
#endif
}

#ifdef MULTIPROCESSOR
void
pmap_tlb_shootdown_process(void)
{
	struct cpu_info * const ci = curcpu();
	struct pmap_tlb_info * const ti = ci->ci_tlb_info;
#ifdef DIAGNOSTIC
	struct pmap * const curpmap = curlwp->l_proc->p_vmspace->vm_map.pmap;
#endif

	KDASSERT(ti->ti_name[0] == 't');
	KDASSERT(ti->ti_name[1] == 'l');
	KDASSERT(ti->ti_name[2] == 'b');

#if 0
	KASSERT(cpu_intr_p());
	KASSERTMSG(ci->ci_cpl >= IPL_SCHED,
	    "%s: cpl (%d) < IPL_SCHED (%d)",
	    __func__, ci->ci_cpl, IPL_SCHED);
#endif
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
			KASSERT(pai->pai_asid > KERNEL_PID);
			pmap_tlb_asid_check();
			tlb_invalidate_asids(pai->pai_asid, pai->pai_asid + 1);
			pmap_tlb_asid_check();
		} else if (pai->pai_asid) {
			/*
			 * The victim is no longer an active pmap for this TLB.
			 * So simply clear its ASID and when pmap_activate is
			 * next called for this pmap, it will allocate a new
			 * ASID.
			 */
			KASSERT((curpmap->pm_onproc & ti->ti_cpu_mask) == 0);
			pmap_pai_reset(ti, pai, PAI_PMAP(pai, ti));
		}
		break;
	}
	case TLBINV_ALLUSER:
		/*
		 * Flush all user TLB entries.
		 */
		pmap_tlb_asid_reinitialize(ti, TLBINV_ALLUSER);
		break;
	case TLBINV_ALLKERNEL:
		/*
		 * We need to invalidate all global TLB entries.
		 */
		pmap_tlb_asid_check();
		tlb_invalidate_globals();
		pmap_tlb_asid_check();
		break;
	case TLBINV_ALL:
		/*
		 * Flush all the TLB entries (user and kernel).
		 */
		pmap_tlb_asid_reinitialize(ti, TLBINV_ALL);
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

static struct cpu_info *
pmap_tlb_target_bystander(struct pmap_tlb_info *ti, struct pmap *pm,
	bool kernel_p)
{
	struct pmap_asid_info * const pai = PMAP_PAI(pm, ti);
	TLBINFO_LOCK(ti);
	const uint32_t onproc = (pm->pm_onproc & ti->ti_cpu_mask);
	if (onproc == 0) {
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
		return NULL;
	}
	if (kernel_p) {
		ti->ti_tlbinvop = TLBINV_KERNEL_MAP(ti->ti_tlbinvop);
		ti->ti_victim = NULL;
	} else {
		KASSERT(pai->pai_asid > KERNEL_PID);
		if (__predict_false(ti->ti_victim == pm)) {
			KASSERT(ti->ti_tlbinvop == TLBINV_ONE);
			/*
			 * We still need to invalidate this one
			 * ASID so there's nothing to change.
			 */
		} else {
			ti->ti_tlbinvop = TLBINV_USER_MAP(ti->ti_tlbinvop);
			if (ti->ti_tlbinvop == TLBINV_ONE)
				ti->ti_victim = pm;
			else
				ti->ti_victim = NULL;
		}
	}
	TLBINFO_UNLOCK(ti);
	/*
	 * Return a pointer to the cpu_info of one of the tlb_info's cpus
	 */
	const u_int j = ffs(onproc) - 1;
	return cpu_lookup(j);
}

bool
pmap_tlb_shootdown_bystanders(pmap_t pm, uint32_t pending)
{
	struct cpu_info * const ci = curcpu();
	struct pmap_tlb_info * const curti = ci->ci_tlb_info;
	uint32_t pm_active = pm->pm_active & ~curti->ti_cpu_mask;
	const bool kernel_p = (pm == pmap_kernel());
	bool ipi_sent = false;

	KDASSERT(curti->ti_name[0] == 't');
	KDASSERT(curti->ti_name[1] == 'l');
	KDASSERT(curti->ti_name[2] == 'b');

	if (__predict_false(pending & ~curti->ti_cpu_mask) != 0) {
		/*
		 * Now if another cpu (not sharing this tlb_info) wants a
		 * shootdown, then they must mean us since this pmap is
		 * obviously active.  But since we cleared their bit, they
		 * won't know they need to do it.  So we do it ourselves
		 * and save them from sending an IPI.
		 */
		if (pmap_tlb_target_bystander(curti, pm, kernel_p) != NULL)
			pmap_tlb_shootdown_process();
	}

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
		if ((pm_active & ti->ti_cpu_mask) == 0) {
			continue;
		}
		pm_active &= ~ti->ti_cpu_mask;
		struct cpu_info * const ipi_ci =
		    pmap_tlb_target_bystander(ti, pm, kernel_p);
		if (ipi_ci != NULL) {
			/*
			 * Now we can send out the shootdown IPIs to a CPU
			 * that shares this TLB and is currently using this
			 * pmap.  That CPU will process the IPI and do the
			 * all the work.  Any other CPUs sharing that TLB
			 * will take advantage of that work.  pm_onproc might
			 * change now that we have released the lock but we
			 * can tolerate spurious shootdowns.
			 */
			cpu_send_ipi(ipi_ci, IPI_SHOOTDOWN);
			ipi_sent = true;
		}
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

	KDASSERT(ti->ti_name[0] == 't');
	KDASSERT(ti->ti_name[1] == 'l');
	KDASSERT(ti->ti_name[2] == 'b');

	KASSERT(kpreempt_disabled());

	TLBINFO_LOCK(ti);
	if (pm == pmap_kernel() || PMAP_PAI_ASIDVALID_P(pai, ti)) {
		va |= pai->pai_asid << MIPS_TLB_PID_SHIFT;
		pmap_tlb_asid_check();
		rv = tlb_update(va, pt_entry);
		pmap_tlb_asid_check();
	}
#ifdef MULTIPROCESSOR
	if (need_ipi && (pm->pm_active & ~ti->ti_cpu_mask) != 0) {
		atomic_or_uint(&pm->pm_shootdown_pending, 1 << cpu_number());
	}
#endif
	TLBINFO_UNLOCK(ti);

	return rv;
}

void
pmap_tlb_invalidate_addr(pmap_t pm, vaddr_t va)
{
	struct pmap_tlb_info * const ti = curcpu()->ci_tlb_info;
	struct pmap_asid_info * const pai = PMAP_PAI(pm, ti);

	KDASSERT(ti->ti_name[0] == 't');
	KDASSERT(ti->ti_name[1] == 'l');
	KDASSERT(ti->ti_name[2] == 'b');

	KASSERT(kpreempt_disabled());

	TLBINFO_LOCK(ti);
	if (pm == pmap_kernel() || PMAP_PAI_ASIDVALID_P(pai, ti)) {
		va |= pai->pai_asid << MIPS_TLB_PID_SHIFT;
		pmap_tlb_asid_check();
		tlb_invalidate_addr(va);
		pmap_tlb_asid_check();
	}
#ifdef MULTIPROCESSOR
	if ((pm->pm_active & ~ti->ti_cpu_mask) != 0) {
		atomic_or_uint(&pm->pm_shootdown_pending, 1 << cpu_number());
	}
#endif
	TLBINFO_UNLOCK(ti);
}

static inline void
pmap_tlb_asid_alloc(struct pmap_tlb_info *ti, pmap_t pm,
	struct pmap_asid_info *pai)
{
	/*
	 * We shouldn't have an ASID assigned, and thusly must not be onproc
	 * nor active.
	 */
	KASSERT(pai->pai_asid == 0);
	KASSERT(pai->pai_link.le_prev == NULL);
#ifdef MULTIPROCESSOR
	KASSERT((pm->pm_onproc & ti->ti_cpu_mask) == 0);
	KASSERT((pm->pm_active & ti->ti_cpu_mask) == 0);
#endif
	KASSERT(ti->ti_asids_free > 0);
	KASSERT(ti->ti_asid_hint <= ti->ti_asid_max);

	/*
	 * Let's see if the hinted ASID is free.  If not search for
	 * a new one.
	 */
	if (__predict_false(TLBINFO_ASID_INUSE_P(ti, ti->ti_asid_hint))) {
		const size_t nbpw = 8 * sizeof(ti->ti_asid_bitmap[0]);
		for (size_t i = 0; i < ti->ti_asid_hint / nbpw; i++) {
			KASSERT(~ti->ti_asid_bitmap[i] == 0);
		}
		for (size_t i = ti->ti_asid_hint / nbpw;; i++) {
			KASSERT(i <= ti->ti_asid_max / nbpw);
			/*
			 * ffs wants to find the first bit set while we want
			 * to find the first bit cleared.
			 */
			u_long bits = ~ti->ti_asid_bitmap[i];
			if (__predict_true(bits)) {
				u_int n = 0;
				if ((bits & 0xffffffff) == 0)  {
					bits = (bits >> 31) >> 1;
					KASSERT(bits);
					n += 32;
				}
				n += ffs(bits) - 1;
				KASSERT(n < nbpw);
				ti->ti_asid_hint = n + i * nbpw;
				break;
			}
		}
		KASSERT(ti->ti_asid_hint != 0);
		KASSERT(TLBINFO_ASID_INUSE_P(ti, ti->ti_asid_hint-1));
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

#ifdef MULTIPROCESSOR
	/*
	 * Mark that we now have an active ASID for all CPUs sharing this TLB.
	 * The bits in pm_active belonging to this TLB can only be changed
	 * while this TLBs lock is held.
	 */
	atomic_or_32(&pm->pm_active, ti->ti_cpu_mask);
#endif
}

/*
 * Acquire a TLB address space tag (called ASID or TLBPID) and return it.
 * ASID might have already been previously acquired.
 */
void
pmap_tlb_asid_acquire(pmap_t pm, struct lwp *l)
{
	struct cpu_info * const ci = l->l_cpu;
	struct pmap_tlb_info * const ti = ci->ci_tlb_info;
	struct pmap_asid_info * const pai = PMAP_PAI(pm, ti);

	KDASSERT(ti->ti_name[0] == 't');
	KDASSERT(ti->ti_name[1] == 'l');
	KDASSERT(ti->ti_name[2] == 'b');

	KASSERT(kpreempt_disabled());

	/*
	 * Kernels use a fixed ASID of 0 and don't need to acquire one.
	 */
	if (pm == pmap_kernel())
		return;

	TLBINFO_LOCK(ti);
	if (__predict_false(!PMAP_PAI_ASIDVALID_P(pai, ti))) {
		/*
		 * If we've run out ASIDs, reinitialize the ASID space.
		 */
		if (__predict_false(tlbinfo_noasids_p(ti))) {
			KASSERT(l == curlwp);
			pmap_tlb_asid_reinitialize(ti, TLBINV_NOBODY);
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
		atomic_or_ulong(&ci->ci_flags, CPUF_USERPMAP);
#endif /* MULTIPROCESSOR */
		ci->ci_pmap_asid_cur = pai->pai_asid;
		tlb_set_asid(pai->pai_asid);
		pmap_tlb_asid_check();
	} else {
		printf("%s: l (%p) != curlwp %p\n", __func__, l, curlwp);
	}
	TLBINFO_UNLOCK(ti);

#ifdef DEBUGXX
	if (pmapdebug & (PDB_FOLLOW|PDB_TLBPID)) {
		printf("%s: curlwp %d.%d '%s' ", __func__,
		    curlwp->l_proc->p_pid, curlwp->l_lid,
		    curlwp->l_proc->p_comm);
		printf("segtab %p asid %d\n", pm->pm_segtab, pai->pai_asid);
	}
#endif
}

void
pmap_tlb_asid_deactivate(pmap_t pm)
{
	KASSERT(kpreempt_disabled());
#ifdef MULTIPROCESSOR
	/*
	 * The kernel pmap is aways onproc and active and must never have
	 * those bits cleared.  If pmap_remove_all was called, it has already
	 * deactivated the pmap and thusly onproc will be 0 so there's nothing
	 * to do.
	 */
	if (pm != pmap_kernel() && pm->pm_onproc != 0) {
		struct cpu_info * const ci = curcpu();
		const uint32_t cpu_mask = 1 << cpu_index(ci);
		KASSERT(!cpu_intr_p());
		KASSERTMSG(pm->pm_onproc & cpu_mask,
		    "%s: pmap %p onproc %#x doesn't include cpu %d (%p)",
		    __func__, pm, pm->pm_onproc, cpu_index(ci), ci);
		/*
		 * The bits in pm_onproc that belong to this TLB can
		 * be changed while this TLBs lock is not held as long
		 * as we use atomic ops.
		 */
		atomic_and_32(&pm->pm_onproc, ~cpu_mask);
		atomic_and_ulong(&ci->ci_flags, ~CPUF_USERPMAP);
	}
#elif defined(DEBUG)
	curcpu()->ci_pmap_asid_cur = 0;
	tlb_set_asid(0);
	pmap_tlb_asid_check();
#endif
}

void
pmap_tlb_asid_release_all(struct pmap *pm)
{
	KASSERT(pm != pmap_kernel());
	KASSERT(kpreempt_disabled());
#ifdef MULTIPROCESSOR
	KASSERT(pm->pm_onproc == 0);
	for (u_int i = 0; pm->pm_active != 0; i++) {
		KASSERT(i < pmap_ntlbs);
		struct pmap_tlb_info * const ti = pmap_tlbs[i];
		KDASSERT(ti->ti_name[0] == 't');
		KDASSERT(ti->ti_name[1] == 'l');
		KDASSERT(ti->ti_name[2] == 'b');

		if (pm->pm_active & ti->ti_cpu_mask) {
			struct pmap_asid_info * const pai = PMAP_PAI(pm, ti);
			TLBINFO_LOCK(ti);
			KASSERT(ti->ti_victim != pm);
			pmap_pai_reset(ti, pai, pm);
			TLBINFO_UNLOCK(ti);
		}
	}
#else
	/*
	 * Handle the case of an UP kernel which only has, at most, one ASID.
	 * If the pmap has an ASID allocated, free it.
	 */
	struct pmap_tlb_info * const ti = curcpu()->ci_tlb_info;
	struct pmap_asid_info * const pai = PMAP_PAI(pm, ti);
	KDASSERT(ti->ti_name[0] == 't');
	KDASSERT(ti->ti_name[1] == 'l');
	KDASSERT(ti->ti_name[2] == 'b');
	TLBINFO_LOCK(ti);
	if (pai->pai_asid) {
		pmap_pai_reset(ti, pai, pm);
	}
	TLBINFO_UNLOCK(ti);
#endif /* MULTIPROCESSOR */
}

void
pmap_tlb_asid_check(void)
{
#ifdef DEBUG
	kpreempt_disable();
	uint32_t tlb_hi;
	__asm("mfc0 %0,$%1" : "=r"(tlb_hi) : "n"(MIPS_COP_0_TLB_HI));
	uint32_t asid = (tlb_hi & MIPS_TLB_PID) >> MIPS_TLB_PID_SHIFT;
	KDASSERTMSG(asid == curcpu()->ci_pmap_asid_cur,
	   "tlb_hi (%#x) asid (%#x) != current asid (%#x)",
	    tlb_hi, asid, curcpu()->ci_pmap_asid_cur);
	kpreempt_enable();
#endif
}
