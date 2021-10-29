/*-
 * Copyright (c) 2021 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Nick Hudson
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
__KERNEL_RCSID(1, "$NetBSD: aarch64_tlb.c,v 1.2 2021/10/29 07:55:04 skrll Exp $");

#include <sys/param.h>
#include <sys/types.h>

#include <uvm/uvm.h>

#include <arm/cpufunc.h>

#include <aarch64/armreg.h>

tlb_asid_t
tlb_get_asid(void)
{

	return __SHIFTOUT(reg_ttbr0_el1_read(), TTBR_ASID);
}

void
tlb_set_asid(tlb_asid_t asid, pmap_t pm)
{
	const uint64_t ttbr =
	    __SHIFTIN(asid, TTBR_ASID) |
	    __SHIFTIN(pmap_l0pa(pm), TTBR_BADDR);

	cpu_set_ttbr0(ttbr);
}

void
tlb_invalidate_all(void)
{

	aarch64_tlbi_all();
}

void
tlb_invalidate_globals(void)
{
	tlb_invalidate_all();
}

void
tlb_invalidate_asids(tlb_asid_t lo, tlb_asid_t hi)
{
	for (; lo <= hi; lo++) {
		aarch64_tlbi_by_asid(lo);
	}
}

void
tlb_invalidate_addr(vaddr_t va, tlb_asid_t asid)
{
	KASSERT((va & PAGE_MASK) == 0);

	aarch64_tlbi_by_asid_va(asid, va);
}

bool
tlb_update_addr(vaddr_t va, tlb_asid_t asid, pt_entry_t pte, bool insert_p)
{
	KASSERT((va & PAGE_MASK) == 0);

	tlb_invalidate_addr(va, asid);

	return true;
}

u_int
tlb_record_asids(u_long *mapp, tlb_asid_t asid_max)
{
	KASSERT(asid_max == pmap_md_tlb_asid_max());

#if DIAGNOSTIC
	memset(mapp, 0xff, (asid_max + 1) / NBBY);
	mapp[0] ^= __BITS(0, KERNEL_PID);
#endif
	return asid_max;
}

void
tlb_walk(void *ctx, bool (*func)(void *, vaddr_t, tlb_asid_t, pt_entry_t))
{

	/* no way to view the TLB */
}
