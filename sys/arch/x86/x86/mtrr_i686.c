/*	$NetBSD: mtrr_i686.c,v 1.27.2.2 2017/12/03 11:36:50 jdolecek Exp $ */

/*-
 * Copyright (c) 2000, 2011 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Bill Sommerfeld.
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
__KERNEL_RCSID(0, "$NetBSD: mtrr_i686.c,v 1.27.2.2 2017/12/03 11:36:50 jdolecek Exp $");

#include "opt_multiprocessor.h"

#include <sys/param.h>
#include <sys/systm.h>

#include <sys/atomic.h>
#include <sys/cpu.h>
#include <sys/kmem.h>
#include <sys/proc.h>

#include <uvm/uvm_extern.h>

#include <machine/specialreg.h>
#include <machine/cpuvar.h>
#include <machine/cpufunc.h>
#include <machine/mtrr.h>

extern paddr_t avail_end;

static void i686_mtrr_reload(int);
static void i686_mtrr_init_cpu(struct cpu_info *);
static void i686_mtrr_reload_cpu(struct cpu_info *);
static void i686_mtrr_clean(struct proc *p);
static int i686_mtrr_set(struct mtrr *, int *n, struct proc *p, int flags);
static int i686_mtrr_get(struct mtrr *, int *n, struct proc *p, int flags);
static void i686_mtrr_dump(const char *tag);

static int i686_mtrr_validate(struct mtrr *, struct proc *p);
static void i686_soft2raw(void);
static void i686_raw2soft(void);
static void i686_mtrr_commit(void);
static int i686_mtrr_setone(struct mtrr *, struct proc *p);
static int i686_mtrr_conflict(uint8_t, uint8_t);

static struct mtrr_state
mtrr_raw[] = {
	{ MSR_MTRRphysBase0, 0 },
	{ MSR_MTRRphysMask0, 0 },
	{ MSR_MTRRphysBase1, 0 },
	{ MSR_MTRRphysMask1, 0 },
	{ MSR_MTRRphysBase2, 0 },
	{ MSR_MTRRphysMask2, 0 },
	{ MSR_MTRRphysBase3, 0 },
	{ MSR_MTRRphysMask3, 0 },
	{ MSR_MTRRphysBase4, 0 },
	{ MSR_MTRRphysMask4, 0 },
	{ MSR_MTRRphysBase5, 0 },
	{ MSR_MTRRphysMask5, 0 },
	{ MSR_MTRRphysBase6, 0 },
	{ MSR_MTRRphysMask6, 0 },
	{ MSR_MTRRphysBase7, 0 },
	{ MSR_MTRRphysMask7, 0 },
	{ MSR_MTRRphysBase8, 0 },
	{ MSR_MTRRphysMask8, 0 },
	{ MSR_MTRRphysBase9, 0 },
	{ MSR_MTRRphysMask9, 0 },
	{ MSR_MTRRphysBase10, 0 },
	{ MSR_MTRRphysMask10, 0 },
	{ MSR_MTRRphysBase11, 0 },
	{ MSR_MTRRphysMask11, 0 },
	{ MSR_MTRRphysBase12, 0 },
	{ MSR_MTRRphysMask12, 0 },
	{ MSR_MTRRphysBase13, 0 },
	{ MSR_MTRRphysMask13, 0 },
	{ MSR_MTRRphysBase14, 0 },
	{ MSR_MTRRphysMask14, 0 },
	{ MSR_MTRRphysBase15, 0 },
	{ MSR_MTRRphysMask15, 0 },
	{ MSR_MTRRfix64K_00000, 0 },
	{ MSR_MTRRfix16K_80000, 0 },
	{ MSR_MTRRfix16K_A0000, 0 },
	{ MSR_MTRRfix4K_C0000, 0 },
	{ MSR_MTRRfix4K_C8000, 0 },
	{ MSR_MTRRfix4K_D0000, 0 },
	{ MSR_MTRRfix4K_D8000, 0 },
	{ MSR_MTRRfix4K_E0000, 0 },
	{ MSR_MTRRfix4K_E8000, 0 },
	{ MSR_MTRRfix4K_F0000, 0 },
	{ MSR_MTRRfix4K_F8000, 0 },
	{ MSR_MTRRdefType, 0 },

};

static const int nmtrr_raw = __arraycount(mtrr_raw);
static int i686_mtrr_vcnt = 0;

static struct mtrr_state *mtrr_var_raw;
static struct mtrr_state *mtrr_fixed_raw;

static struct mtrr *mtrr_fixed;
static struct mtrr *mtrr_var;

struct mtrr_funcs i686_mtrr_funcs = {
	i686_mtrr_init_cpu,
	i686_mtrr_reload_cpu,
	i686_mtrr_clean,
	i686_mtrr_set,
	i686_mtrr_get,
	i686_mtrr_commit,
	i686_mtrr_dump
};

static kcpuset_t *		mtrr_waiting;

static uint64_t			i686_mtrr_cap;

static void
i686_mtrr_dump(const char *tag)
{
	int i;

	for (i = 0; i < nmtrr_raw; i++)
		printf("%s: %x: %016llx\n",
		    tag, mtrr_raw[i].msraddr,
		    (unsigned long long)rdmsr(mtrr_raw[i].msraddr));
}

/*
 * The Intel Archicture Software Developer's Manual volume 3 (systems
 * programming) section 9.12.8 describes a simple 15-step process for
 * updating the MTRR's on all processors on a multiprocessor system.
 * If synch is nonzero, assume we're being called from an IPI handler,
 * and synchronize with all running processors.
 */

/*
 * 1. Broadcast to all processor to execute the following code sequence.
 */

static void
i686_mtrr_reload(int synch)
{
	int i;
	/* XXX cr0 is 64-bit on amd64 too, but the upper bits are
	 * unused and must be zero so it does not matter too
	 * much. Need to change the prototypes of l/rcr0 too if you
	 * want to correct it. */
	uint32_t cr0;
	vaddr_t cr3, cr4;
	uint32_t origcr0;
	vaddr_t origcr4;

	/*
	 * 2. Disable interrupts
	 */
	x86_disable_intr();

#ifdef MULTIPROCESSOR
	if (synch) {
		/*
		 * 3. Wait for all processors to reach this point.
		 */
		kcpuset_atomic_set(mtrr_waiting, cpu_index(curcpu()));
		while (!kcpuset_match(mtrr_waiting, kcpuset_running)) {
			DELAY(10);
		}
	}
#endif

	/*
	 * 4. Enter the no-fill cache mode (set the CD flag in CR0 to 1 and
	 * the NW flag to 0)
	 */

	origcr0 = cr0 = rcr0();
	cr0 |= CR0_CD;
	cr0 &= ~CR0_NW;
	lcr0(cr0);

	/*
	 * 5. Flush all caches using the WBINVD instruction.
	 */

	wbinvd();

	/*
	 * 6. Clear the PGE flag in control register CR4 (if set).
	 */

	origcr4 = cr4 = rcr4();
	cr4 &= ~CR4_PGE;
	lcr4(cr4);

	/*
	 * 7. Flush all TLBs (execute a MOV from control register CR3
	 * to another register and then a move from that register back
	 * to CR3)
	 */

	cr3 = rcr3();
	lcr3(cr3);

	/*
	 * 8. Disable all range registers (by clearing the E flag in
	 * register MTRRdefType.  If only variable ranges are being
	 * modified, software may clear the valid bits for the
	 * affected register pairs instead.
	 */
	/* disable MTRRs (E = 0) */
	wrmsr(MSR_MTRRdefType, rdmsr(MSR_MTRRdefType) & ~MTRR_I686_ENABLE_MASK);

	/*
	 * 9. Update the MTRR's
	 */

	for (i = 0; i < nmtrr_raw; i++) {
		uint64_t val = mtrr_raw[i].msrval;
		uint32_t addr = mtrr_raw[i].msraddr;
		if (addr == 0)
			continue;
		if (addr == MSR_MTRRdefType)
			val &= ~MTRR_I686_ENABLE_MASK;
		wrmsr(addr, val);
	}

	/*
	 * 10. Enable all range registers (by setting the E flag in
	 * register MTRRdefType).  If only variable-range registers
	 * were modified and their individual valid bits were cleared,
	 * then set the valid bits for the affected ranges instead.
	 */

	wrmsr(MSR_MTRRdefType, rdmsr(MSR_MTRRdefType) | MTRR_I686_ENABLE_MASK);

	/*
	 * 11. Flush all caches and all TLB's a second time. (repeat
	 * steps 5, 7)
	 */

	wbinvd();
	lcr3(cr3);

	/*
	 * 12. Enter the normal cache mode to reenable caching (set the CD and
	 * NW flags in CR0 to 0)
	 */

	lcr0(origcr0);

	/*
	 * 13. Set the PGE flag in control register CR4, if previously
	 * cleared.
	 */

	lcr4(origcr4);

#ifdef MULTIPROCESSOR
	if (synch) {
		/*
		 * 14. Wait for all processors to reach this point.
		 */
		kcpuset_atomic_clear(mtrr_waiting, cpu_index(curcpu()));
		while (!kcpuset_iszero(mtrr_waiting)) {
			DELAY(10);
		}
	}
#endif

	/*
	 * 15. Enable interrupts.
	 */
	x86_enable_intr();
}

static void
i686_mtrr_reload_cpu(struct cpu_info *ci)
{
	i686_mtrr_reload(1);
}

void
i686_mtrr_init_first(void)
{
	int i;

	i686_mtrr_cap = rdmsr(MSR_MTRRcap);
	i686_mtrr_vcnt = i686_mtrr_cap & MTRR_I686_CAP_VCNT_MASK;

	if (i686_mtrr_vcnt > MTRR_I686_NVAR_MAX)
		printf("%s: FIXME: more than %d MTRRs (%d)\n", __FILE__,
		    MTRR_I686_NVAR_MAX, i686_mtrr_vcnt);
	else if (i686_mtrr_vcnt < MTRR_I686_NVAR_MAX) {
		for (i = MTRR_I686_NVAR_MAX - i686_mtrr_vcnt; i; i--) {
			mtrr_raw[(MTRR_I686_NVAR_MAX - i) * 2].msraddr = 0;
			mtrr_raw[(MTRR_I686_NVAR_MAX - i) * 2 + 1].msraddr = 0;
		}
	}

	for (i = 0; i < nmtrr_raw; i++) {
		if (mtrr_raw[i].msraddr)
			mtrr_raw[i].msrval = rdmsr(mtrr_raw[i].msraddr);
		else
			mtrr_raw[i].msrval = 0;
	}
#if 0
	mtrr_dump("init mtrr");
#endif

	kcpuset_create(&mtrr_waiting, true);

	mtrr_fixed =
	    kmem_zalloc(MTRR_I686_NFIXED_SOFT * sizeof(struct mtrr), KM_SLEEP);

	if (i686_mtrr_vcnt) {
		mtrr_var =
		    kmem_zalloc(i686_mtrr_vcnt * sizeof(struct mtrr), KM_SLEEP);
	}

	mtrr_var_raw = &mtrr_raw[0];
	mtrr_fixed_raw = &mtrr_raw[MTRR_I686_NVAR_MAX * 2];
	mtrr_funcs = &i686_mtrr_funcs;

	i686_raw2soft();
}

static void
i686_raw2soft(void)
{
	int i, j, idx;
	struct mtrr *mtrrp;
	uint64_t base, mask;

	for (i = 0; i < i686_mtrr_vcnt; i++) {
		mtrrp = &mtrr_var[i];
		memset(mtrrp, 0, sizeof *mtrrp);
		mask = mtrr_var_raw[i * 2 + 1].msrval;
		if (!mtrr_valid(mask))
			continue;
		base = mtrr_var_raw[i * 2].msrval;
		mtrrp->base = mtrr_base(base);
		mtrrp->type = mtrr_type(base);
		mtrrp->len = mtrr_len(mask);
		mtrrp->flags |= MTRR_VALID;
	}

	idx = 0;
	base = 0;
	for (i = 0; i < MTRR_I686_NFIXED_64K; i++, idx++) {
		mask = mtrr_fixed_raw[idx].msrval;
		for (j = 0; j < 8; j++) {
			mtrrp = &mtrr_fixed[idx * 8 + j];
			mtrrp->owner = 0;
			mtrrp->flags = MTRR_FIXED | MTRR_VALID;
			mtrrp->base = base;
			mtrrp->len = 65536;
			mtrrp->type = mask & 0xff;
			mask >>= 8;
			base += 65536;
		}
	}

	for (i = 0; i < MTRR_I686_NFIXED_16K; i++, idx++) {
		mask = mtrr_fixed_raw[idx].msrval;
		for (j = 0; j < 8; j++) {
			mtrrp = &mtrr_fixed[idx * 8 + j];
			mtrrp->owner = 0;
			mtrrp->flags = MTRR_FIXED | MTRR_VALID;
			mtrrp->base = base;
			mtrrp->len = 16384;
			mtrrp->type = mask & 0xff;
			mask >>= 8;
			base += 16384;
		}
	}

	for (i = 0; i < MTRR_I686_NFIXED_4K; i++, idx++) {
		mask = mtrr_fixed_raw[idx].msrval;
		for (j = 0; j < 8; j++) {
			mtrrp = &mtrr_fixed[idx * 8 + j];
			mtrrp->owner = 0;
			mtrrp->flags = MTRR_FIXED | MTRR_VALID;
			mtrrp->base = base;
			mtrrp->len = 4096;
			mtrrp->type = mask & 0xff;
			mask >>= 8;
			base += 4096;
		}
	}
}

static void
i686_soft2raw(void)
{
	int i, idx, j;
	uint64_t val;
	struct mtrr *mtrrp;

	for (i = 0; i < i686_mtrr_vcnt; i++) {
		mtrrp = &mtrr_var[i];
		mtrr_var_raw[i * 2].msrval = mtrr_base_value(mtrrp);
		mtrr_var_raw[i * 2 + 1].msrval = mtrr_mask_value(mtrrp);
		if (mtrrp->flags & MTRR_VALID)
			mtrr_var_raw[i * 2 + 1].msrval |= MTRR_I686_MASK_VALID;
	}

	idx = 0;
	for (i = 0; i < MTRR_I686_NFIXED_64K; i++, idx++) {
		val = 0;
		for (j = 0; j < 8; j++) {
			mtrrp = &mtrr_fixed[idx * 8 + j];
			val |= ((uint64_t)mtrrp->type << (j << 3));
		}
		mtrr_fixed_raw[idx].msrval = val;
	}

	for (i = 0; i < MTRR_I686_NFIXED_16K; i++, idx++) {
		val = 0;
		for (j = 0; j < 8; j++) {
			mtrrp = &mtrr_fixed[idx * 8 + j];
			val |= ((uint64_t)mtrrp->type << (j << 3));
		}
		mtrr_fixed_raw[idx].msrval = val;
	}

	for (i = 0; i < MTRR_I686_NFIXED_4K; i++, idx++) {
		val = 0;
		for (j = 0; j < 8; j++) {
			mtrrp = &mtrr_fixed[idx * 8 + j];
			val |= ((uint64_t)mtrrp->type << (j << 3));
		}
		mtrr_fixed_raw[idx].msrval = val;
	}
}

static void
i686_mtrr_init_cpu(struct cpu_info *ci)
{
	i686_mtrr_reload(0);
#if 0
	mtrr_dump(device_xname(ci->ci_dev));
#endif
}

static int
i686_mtrr_validate(struct mtrr *mtrrp, struct proc *p)
{
	uint64_t high;

	/*
	 * Must be at least page-aligned.
	 */
	if (mtrrp->base & 0xfff || mtrrp->len & 0xfff || mtrrp->len == 0)
		return EINVAL;

	/*
	 * Private mappings are bound to a process.
	 */
	if (p == NULL && (mtrrp->flags & MTRR_PRIVATE))
		return EINVAL;

	high = mtrrp->base + mtrrp->len;

	/*
	 * Check for bad types.
	 */
	if ((mtrrp->type == MTRR_TYPE_UNDEF1 || mtrrp->type == MTRR_TYPE_UNDEF2
	    || mtrrp->type > MTRR_TYPE_WB) && (mtrrp->flags & MTRR_VALID))
		return EINVAL;

	/* 
	 * If write-combining is requested, make sure that the WC feature   
	 * is supported by the processor.
	 */
	if (mtrrp->type == MTRR_TYPE_WC &&
	    !(i686_mtrr_cap & MTRR_I686_CAP_WC_MASK))
		return ENODEV;

	/*
	 * Only use fixed ranges < 1M.
	 */
	if ((mtrrp->flags & MTRR_FIXED) && high > 0x100000)
		return EINVAL;

	/*
	 * Check for the right alignment and size for fixed ranges.
	 * The requested range may span several actual MTRRs, but
	 * it must be properly aligned.
	 */
	if (mtrrp->flags & MTRR_FIXED) {
		if (mtrrp->base < MTRR_I686_16K_START) {
			if ((mtrrp->base  & 0xffff) != 0)
				return EINVAL;
		} else if (mtrrp->base < MTRR_I686_4K_START) {
			if ((mtrrp->base & 0x3fff) != 0)
				return EINVAL;
		} else {
			if ((mtrrp->base  & 0xfff) != 0)
				return EINVAL;
		}

		if (high < MTRR_I686_16K_START) {
			if ((high  & 0xffff) != 0)
				return EINVAL;
		} else if (high < MTRR_I686_4K_START) {
			if ((high & 0x3fff) != 0)
				return EINVAL;
		} else {
			if ((high & 0xfff) != 0)
				return EINVAL;
		}
	}

	return 0;
}

/*
 * Try to find a non-conflicting match on physical MTRRs for the
 * requested range. For fixed ranges, more than one actual MTRR
 * may be used.
 */
static int
i686_mtrr_setone(struct mtrr *mtrrp, struct proc *p)
{
	int i, error;
	struct mtrr *lowp, *highp, *mp, *freep;
	uint64_t low, high, curlow, curhigh;

	/*
	 * If explicitly requested, or if the range lies below 1M,
	 * try the fixed range MTRRs.
	 */
	if (mtrrp->flags & MTRR_FIXED ||
	    (mtrrp->base + mtrrp->len) <= 0x100000) {
		lowp = highp = NULL;
		for (i = 0; i < MTRR_I686_NFIXED_SOFT; i++) {
			if (mtrr_fixed[i].base == mtrrp->base + mtrrp->len) {
				highp = &mtrr_fixed[i];
				break;
			}
			if (mtrr_fixed[i].base == mtrrp->base) {
				lowp = &mtrr_fixed[i];
				/*
				 * If the requested upper bound is the 1M
				 * limit, search no further.
				 */
				if ((mtrrp->base + mtrrp->len) == 0x100000) {
					highp =
					    &mtrr_fixed[MTRR_I686_NFIXED_SOFT];
					break;
				} else {
					highp = &mtrr_fixed[i + 1];
					continue;
				}
			}
		}
		if (lowp == NULL || highp == NULL)
			panic("mtrr: fixed register screwup");
		error = 0;
		for (mp = lowp; mp < highp; mp++) {
			if ((mp->flags & MTRR_PRIVATE) && p != NULL
			     && p->p_pid != mp->owner) {
				error = EBUSY;
				break;
			}
		}
		if (error != 0) {
			if (mtrrp->flags & MTRR_FIXED)
				return error;
		} else {
			for (mp = lowp; mp < highp; mp++) {
				/*
				 * Can't invalidate fixed ranges, so
				 * just reset the 'private' flag,
				 * making the range available for
				 * changing again.
				 */
				if (!(mtrrp->flags & MTRR_VALID)) {
					mp->flags &= ~MTRR_PRIVATE;
					continue;
				}
				mp->type = mtrrp->type;
				if (mtrrp->flags & MTRR_PRIVATE) {
					/*
					 * Private mappings are bound to a
					 * process. This has been checked in
					 * i686_mtrr_validate()
					 */
					mp->flags |= MTRR_PRIVATE;
					mp->owner = p->p_pid;
				}
			}
			return 0;
		}
	}

	/*
	 * Try one of the variable range registers.
	 * XXX could be more sophisticated here by merging ranges.
	 */
	low = mtrrp->base;
	high = low + mtrrp->len - 1;
	freep = NULL;
	for (i = 0; i < i686_mtrr_vcnt; i++) {
		if (!(mtrr_var[i].flags & MTRR_VALID)) {
			freep = &mtrr_var[i];
			continue;
		}
		curlow = mtrr_var[i].base;
		curhigh = curlow + mtrr_var[i].len - 1;
		if (low == curlow && high == curhigh &&
		    (!(mtrr_var[i].flags & MTRR_PRIVATE) ||
		     ((mtrrp->flags & MTRR_PRIVATE) && (p != NULL) &&
		      (mtrr_var[i].owner == p->p_pid)))) {
			freep = &mtrr_var[i];
			break;
		}
		if (((high >= curlow && high < curhigh) ||
		    (low >= curlow && low < curhigh)) &&
	 	    (i686_mtrr_conflict(mtrr_var[i].type, mtrrp->type) ||
		     ((mtrr_var[i].flags & MTRR_PRIVATE) &&
 		      (!(mtrrp->flags & MTRR_PRIVATE) || (p == NULL) ||
		       (mtrr_var[i].owner != p->p_pid))))) {
			return EBUSY;
		}
	}
	if (freep == NULL)
		return EBUSY;
	mtrrp->flags &= ~MTRR_CANTSET;
	*freep = *mtrrp;
	freep->owner = (mtrrp->flags & MTRR_PRIVATE) ? p->p_pid : 0;

	return 0;
}

static int
i686_mtrr_conflict(uint8_t type1, uint8_t type2)
{
	if (type1 == MTRR_TYPE_UC || type2 == MTRR_TYPE_UC)
		return 0;
	if ((type1 == MTRR_TYPE_WT && type2 == MTRR_TYPE_WB) ||
	    (type1 == MTRR_TYPE_WB && type2 == MTRR_TYPE_WT))
		return 0;
	return 1;
}

static void
i686_mtrr_clean(struct proc *p)
{
	int i;

	for (i = 0; i < MTRR_I686_NFIXED_SOFT; i++) {
		if ((mtrr_fixed[i].flags & MTRR_PRIVATE) &&
		    (mtrr_fixed[i].owner == p->p_pid))
			mtrr_fixed[i].flags &= ~MTRR_PRIVATE;
	}

	for (i = 0; i < i686_mtrr_vcnt; i++) {
		if ((mtrr_var[i].flags & MTRR_PRIVATE) &&
		    (mtrr_var[i].owner == p->p_pid))
			mtrr_var[i].flags &= ~(MTRR_PRIVATE | MTRR_VALID);
	}

	i686_mtrr_commit();
}

static int
i686_mtrr_set(struct mtrr *mtrrp, int *n, struct proc *p, int flags)
{
	int i, error;
	struct mtrr mtrr;

	if (*n > (MTRR_I686_NFIXED_SOFT + MTRR_I686_NVAR_MAX)) {
		*n = 0;
		return EINVAL;
	}

	error = 0;
	for (i = 0; i < *n; i++) {
		if (flags & MTRR_GETSET_USER) {
			error = copyin(&mtrrp[i], &mtrr, sizeof mtrr);
			if (error != 0)
				break;
		} else
			mtrr = mtrrp[i];
		error = i686_mtrr_validate(&mtrr, p);
		if (error != 0)
			break;
		error = i686_mtrr_setone(&mtrr, p);
		if (error != 0)
			break;
		if (mtrr.flags & MTRR_PRIVATE)
			p->p_md.md_flags |= MDP_USEDMTRR;
	}
	*n = i;
	return error;
}

static int
i686_mtrr_get(struct mtrr *mtrrp, int *n, struct proc *p, int flags)
{
	int idx, i, error;

	if (mtrrp == NULL) {
		*n = MTRR_I686_NFIXED_SOFT + MTRR_I686_NVAR_MAX;
		return 0;
	}

	error = 0;

	for (idx = i = 0; i < MTRR_I686_NFIXED_SOFT && idx < *n; idx++, i++) {
		if (flags & MTRR_GETSET_USER) {
			error = copyout(&mtrr_fixed[i], &mtrrp[idx],
					sizeof *mtrrp);
			if (error != 0)
				break;
		} else
			memcpy(&mtrrp[idx], &mtrr_fixed[i], sizeof *mtrrp);
	}
	if (error != 0) {
		*n = idx;
		return error;
	}

	for (i = 0; i < i686_mtrr_vcnt && idx < *n; idx++, i++) {
		if (flags & MTRR_GETSET_USER) {
			error = copyout(&mtrr_var[i], &mtrrp[idx],
					sizeof *mtrrp);
			if (error != 0)
				break;
		} else
			memcpy(&mtrrp[idx], &mtrr_var[i], sizeof *mtrrp);
	}
	*n = idx;
	return error;
}

static void
i686_mtrr_commit(void)
{

	i686_soft2raw();
	kpreempt_disable();
#ifdef MULTIPROCESSOR
	x86_broadcast_ipi(X86_IPI_MTRR);
#endif
	i686_mtrr_reload(1);
	kpreempt_enable();
}
