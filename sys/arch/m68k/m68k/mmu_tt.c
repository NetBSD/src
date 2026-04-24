/*	$NetBSD: mmu_tt.c,v 1.1 2026/04/24 11:24:32 thorpej Exp $	*/

/*-
 * Copyright (c) 2025 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe.
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
__KERNEL_RCSID(0, "$NetBSD: mmu_tt.c,v 1.1 2026/04/24 11:24:32 thorpej Exp $");

#include "opt_m68k_arch.h"

#include <sys/param.h>
#include <sys/cpu.h>

#include <uvm/uvm_extern.h>

#include <m68k/mmu.h>
#if defined(M68030)
#include <m68k/mmu_30.h>
#endif
#if defined(M68040) || defined(M68060)
#include <m68k/mmu_40.h>
#endif

#ifdef M68030
/*
 * mmu_tt_chkpg30 --
 *	Check that a given page is TT's on a 68030.
 */
static bool
mmu_tt_chkpg30(uint32_t ttreg, paddr_t pa, vm_prot_t prot, int *out_flags)
{
	uint32_t lab = __SHIFTOUT(ttreg, TT30_LAB);
	uint32_t lam = __SHIFTOUT(ttreg, TT30_LAM);
	uint32_t pa_lab = __SHIFTOUT(pa, TT30_LAB);
	uint32_t fcb = __SHIFTOUT(ttreg, TT30_FCBASE);
	uint32_t fcm = __SHIFTOUT(ttreg, TT30_FCMASK);

	if ((ttreg & TT30_E) == 0) {
		/* This reg not enabled - not translated. */
		return false;
	}

	/* LAM and FCM are "ignore" bits. */
	lam = ~lam;
	fcm = ~fcm;

	if ((lab & lam) != (pa_lab & lam)) {
		/* Addresses don't match. */
		return false;
	}

	/* Check function. */
	if ((prot & UVM_PROT_EXEC) != 0 &&
	    (fcb & fcm) != (FC_SUPERP & fcm)) {
		return false;
	}
	if ((prot & (UVM_PROT_READ|UVM_PROT_WRITE)) != 0 &&
	    (fcb & fcm) != (FC_SUPERD & fcm)) {
		return false;
	}

	if ((ttreg & TT30_RWM) == 0) {
		if (ttreg & TT30_RW) {
			if (prot & UVM_PROT_WRITE) {
				return false;
			}
		} else {
			if (prot & UVM_PROT_READ) {
				return false;
			}
		}
	}

	if (ttreg & TT30_CI) {
		*out_flags = PMAP_NOCACHE;
	} else {
		*out_flags = 0;
	}

	return true;
}

static bool
mmu_range_checkonett30(uint32_t ttreg, paddr_t firstpg, paddr_t lastpg,
    vm_prot_t prot, int *out_flags)
{
	paddr_t pg;

	for (pg = firstpg; pg <= lastpg; pg++) {
		if (! mmu_tt_chkpg30(ttreg, m68k_ptob(pg), prot, out_flags)) {
			return false;
		}
	}
	return true;
}

static bool
mmu_range_is_tt30(paddr_t firstpg, paddr_t lastpg, vm_prot_t prot,
    int *out_flags)
{
	return mmu_range_checkonett30(mmu_tt30[MMU_TTREG_TT0], firstpg, lastpg,
				      prot, out_flags) ||
	       mmu_range_checkonett30(mmu_tt30[MMU_TTREG_TT1], firstpg, lastpg,
				      prot, out_flags);
}
#endif /* M68030 */

#if defined(M68040) || defined(M68060)
static bool
mmu_tt_checkpg40(uint32_t ttreg, paddr_t pa, vm_prot_t prot, int *out_flags)
{
	uint32_t lab = __SHIFTOUT(ttreg, TTR40_LAB);
	uint32_t lam = __SHIFTOUT(ttreg, TTR40_LAM);
	uint32_t pa_lab = __SHIFTOUT(pa, TTR40_LAB);

	if ((ttreg & TTR40_E) == 0) {
		/* This reg not enabled - not translated. */
		return false;
	}

	/* LAM are "ignore" bits. */
	lam = ~lam;

	if ((lab & lam) != (pa_lab & lam)) {
		/* Addresses don't match. */
		return false;
	}

	switch (ttreg & TTR40_SFIELD) {
	case TTR40_USER:
		/* Doesn't match Supervisor */
		return false;
	default:
		/* All other combinations do. */
		break;
	}

	if ((prot & UVM_PROT_WRITE) != 0 && (ttreg & TTR40_W) != 0) {
		return false;
	}

	switch (ttreg & TTR40_CM) {
	case PTE40_CM_NC_SER:
	case PTE40_CM_NC:
		*out_flags = PMAP_NOCACHE;
		break;
	default:
		*out_flags = 0;
		break;
	}

	return true;
}

static bool
mmu_range_checkonett40(uint32_t ttreg, paddr_t firstpg, paddr_t lastpg,
    vm_prot_t prot, int *out_flags)
{
	paddr_t pg;

	for (pg = firstpg; pg <= lastpg; pg++) {
		if (! mmu_tt_checkpg40(ttreg, m68k_ptob(pg), prot, out_flags)) {
			return false;
		}
	}
	return true;
}

static bool
mmu_range_is_tt40(paddr_t firstpg, paddr_t lastpg, vm_prot_t prot,
    int *out_flags)
{
	int rflag;

	if ((prot & UVM_PROT_EXEC) &&
	    !mmu_range_checkonett40(mmu_tt40[MMU_TTREG_ITT0],
				    firstpg, lastpg, UVM_PROT_EXEC, &rflag) &&
	    !mmu_range_checkonett40(mmu_tt40[MMU_TTREG_ITT1],
				    firstpg, lastpg, UVM_PROT_EXEC, &rflag)) {
		return false;
	}

	if ((prot & (UVM_PROT_READ|UVM_PROT_WRITE)) &&
	    !mmu_range_checkonett40(mmu_tt40[MMU_TTREG_DTT0],
				    firstpg, lastpg, prot, &rflag) &&
	    !mmu_range_checkonett40(mmu_tt40[MMU_TTREG_DTT1],
				    firstpg, lastpg, prot, &rflag)) {
		return false;
	}

	*out_flags = rflag;

	/* We know at least one space is selected. */
	return true;
}
#endif /* M68040 || M68060 */

/*
 * mmu_range_is_tt --
 *	Check to see that a physical address range is transparently
 *	translated for the specified usage (data, prog).
 */
bool
mmu_range_is_tt(paddr_t pa, psize_t len, vm_prot_t prot, int *out_flags)
{
	paddr_t lastpg = m68k_btop(pa + (len - 1));
	paddr_t firstpg = m68k_btop(pa);

	if ((prot & (UVM_PROT_READ|UVM_PROT_WRITE|UVM_PROT_EXEC)) == 0) {
		return false;
	}

	switch (mmutype) {
#ifdef M68030
	case MMU_68030:
		return mmu_range_is_tt30(firstpg, lastpg, prot, out_flags);
#endif
#if defined(M68040) || defined(M68060)
	case MMU_68040:
	case MMU_68060:
		return mmu_range_is_tt40(firstpg, lastpg, prot, out_flags);
#endif
	default:
		return false;
	}
}
