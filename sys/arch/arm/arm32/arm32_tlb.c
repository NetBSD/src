/*-
 * Copyright (c) 2013 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Matt Thomas of 3am Software Foundry.
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

#include "opt_cputypes.h"
#include "opt_multiprocessor.h"

#include <sys/cdefs.h>
__KERNEL_RCSID(1, "$NetBSD: arm32_tlb.c,v 1.12 2018/08/15 06:00:02 skrll Exp $");

#include <sys/param.h>
#include <sys/types.h>

#include <uvm/uvm.h>

#include <arm/locore.h>

bool arm_has_tlbiasid_p;	// CPU supports TLBIASID system coprocessor op
bool arm_has_mpext_p;		// CPU supports MP extensions

tlb_asid_t
tlb_get_asid(void)
{
	return armreg_contextidr_read() & 0xff;
}

void
tlb_set_asid(tlb_asid_t asid)
{
	arm_dsb();
	if (asid == KERNEL_PID) {
		armreg_ttbcr_write(armreg_ttbcr_read() | TTBCR_S_PD0);
		arm_isb();
	}
	armreg_contextidr_write(asid);
	arm_isb();
}

void
tlb_invalidate_all(void)
{
	const bool vivt_icache_p = arm_pcache.icache_type == CACHE_TYPE_VIVT;
	arm_dsb();
	if (arm_has_mpext_p) {
		armreg_tlbiallis_write(0);
	} else {
		armreg_tlbiall_write(0);
	}
	arm_isb();
	if (__predict_false(vivt_icache_p)) {
		if (arm_has_tlbiasid_p) {
			armreg_icialluis_write(0);
		} else {
			armreg_iciallu_write(0);
		}
	}
	arm_dsb();
	arm_isb();
}

void
tlb_invalidate_globals(void)
{
	tlb_invalidate_all();
}

void
tlb_invalidate_asids(tlb_asid_t lo, tlb_asid_t hi)
{
	const bool vivt_icache_p = arm_pcache.icache_type == CACHE_TYPE_VIVT;
	arm_dsb();
	if (arm_has_tlbiasid_p) {
		for (; lo <= hi; lo++) {
			if (arm_has_mpext_p) {
				armreg_tlbiasidis_write(lo);
			} else {
				armreg_tlbiasid_write(lo);
			}
		}
		arm_dsb();
		arm_isb();
		if (__predict_false(vivt_icache_p)) {
			if (arm_has_mpext_p) {
				armreg_icialluis_write(0);
			} else {
				armreg_iciallu_write(0);
			}
		}
	} else {
		armreg_tlbiall_write(0);
		arm_isb();
		if (__predict_false(vivt_icache_p)) {
			armreg_iciallu_write(0);
		}
	}
	arm_isb();
}

void
tlb_invalidate_addr(vaddr_t va, tlb_asid_t asid)
{
	arm_dsb();
	va = trunc_page(va) | asid;
	for (vaddr_t eva = va + PAGE_SIZE; va < eva; va += L2_S_SIZE) {
		if (arm_has_mpext_p) {
			armreg_tlbimvais_write(va);
		} else {
			armreg_tlbimva_write(va);
		}
	}
	arm_isb();
}

bool
tlb_update_addr(vaddr_t va, tlb_asid_t asid, pt_entry_t pte, bool insert_p)
{
	tlb_invalidate_addr(va, asid);
	return true;
}

#if !defined(MULTIPROCESSOR) && defined(CPU_CORTEXA5)
static u_int
tlb_cortex_a5_record_asids(u_long *mapp, tlb_asid_t asid_max)
{
	u_int nasids = 0;
	for (size_t va_index = 0; va_index < 63; va_index++) {
		for (size_t way = 0; way < 2; way++) {
			armreg_tlbdataop_write(
			     __SHIFTIN(way, ARM_TLBDATAOP_WAY)
			     | __SHIFTIN(va_index, ARM_A5_TLBDATAOP_INDEX));
			arm_isb();
			const uint64_t d = ((uint64_t) armreg_tlbdata1_read())
			    | armreg_tlbdata0_read();
			if (!(d & ARM_TLBDATA_VALID)
			    || !(d & ARM_A5_TLBDATA_nG))
				continue;

			const tlb_asid_t asid = __SHIFTOUT(d,
			    ARM_A5_TLBDATA_ASID);
			const u_long mask = 1L << (asid & 31);
			const size_t idx = asid >> 5;
			if (mapp[idx] & mask)
				continue;

			mapp[idx] |= mask;
			nasids++;
		}
	}
	return nasids;
}
#endif

#if !defined(MULTIPROCESSOR) && defined(CPU_CORTEXA7)
static u_int
tlb_cortex_a7_record_asids(u_long *mapp, tlb_asid_t asid_max)
{
	u_int nasids = 0;
	for (size_t va_index = 0; va_index < 128; va_index++) {
		for (size_t way = 0; way < 2; way++) {
			armreg_tlbdataop_write(
			     __SHIFTIN(way, ARM_TLBDATAOP_WAY)
			     | __SHIFTIN(va_index, ARM_A7_TLBDATAOP_INDEX));
			arm_isb();
			const uint32_t d0 = armreg_tlbdata0_read();
			const uint32_t d1 = armreg_tlbdata1_read();
			if (!(d0 & ARM_TLBDATA_VALID)
			    || !(d1 & ARM_A7_TLBDATA1_nG))
				continue;

			const uint64_t d01 = ((uint64_t) d1)|d0;
			const tlb_asid_t asid = __SHIFTOUT(d01,
			    ARM_A7_TLBDATA01_ASID);
			const u_long mask = 1L << (asid & 31);
			const size_t idx = asid >> 5;
			if (mapp[idx] & mask)
				continue;

			mapp[idx] |= mask;
			nasids++;
		}
	}
	return nasids;
}
#endif

u_int
tlb_record_asids(u_long *mapp, tlb_asid_t asid_max)
{
#ifndef MULTIPROCESSOR
#ifdef CPU_CORTEXA5
	if (CPU_ID_CORTEX_A5_P(curcpu()->ci_arm_cpuid))
		return tlb_cortex_a5_record_asids(mapp, asid_max);
#endif
#ifdef CPU_CORTEXA7
	if (CPU_ID_CORTEX_A7_P(curcpu()->ci_arm_cpuid))
		return tlb_cortex_a7_record_asids(mapp, asid_max);
#endif
#endif /* MULTIPROCESSOR */
#ifdef DIAGNOSTIC
	mapp[0] = 0xfffffffe;
	mapp[1] = 0xffffffff;
	mapp[2] = 0xffffffff;
	mapp[3] = 0xffffffff;
	mapp[4] = 0xffffffff;
	mapp[5] = 0xffffffff;
	mapp[6] = 0xffffffff;
	mapp[7] = 0xffffffff;
#endif
	return 255;
}

void
tlb_walk(void *ctx, bool (*func)(void *, vaddr_t, tlb_asid_t, pt_entry_t))
{
	/* no way to view the TLB */
}
