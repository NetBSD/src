/* $NetBSD: riscv_tlb.c,v 1.2 2023/09/03 08:48:20 skrll Exp $ */

/*
 * Copyright (c) 2014, 2019, 2021 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Matt Thomas (of 3am Software Foundry), Maxime Villard, and
 * Nick Hudson.
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

#include "opt_riscv_debug.h"
#include "opt_multiprocessor.h"

#include <sys/cdefs.h>
__RCSID("$NetBSD: riscv_tlb.c,v 1.2 2023/09/03 08:48:20 skrll Exp $");

#include <sys/param.h>
#include <sys/types.h>

#include <sys/cpu.h>

#include <uvm/uvm_extern.h>

#include <riscv/sbi.h>

tlb_asid_t
tlb_get_asid(void)
{
	return csr_asid_read();
}

void
tlb_set_asid(tlb_asid_t asid, struct pmap *pm)
{
	csr_asid_write(asid);
}

void
tlb_invalidate_all(void)
{
	asm volatile("sfence.vma"
	    : /* output operands */
	    : /* input operands */
	    : "memory");
}

void
tlb_invalidate_globals(void)
{
	tlb_invalidate_all();
}

void
tlb_invalidate_asids(tlb_asid_t lo, tlb_asid_t hi)
{
	tlb_asid_t asid;
	for (asid = lo; asid <= hi; asid++) {
		asm volatile("sfence.vma zero, %[asid]"
		    : /* output operands */
		    : [asid] "r" (asid)
		    : "memory");
	}
#ifdef MULTIPROCESSOR
#if PMAP_TLB_MAX == 1
	const cpuid_t myhartid = curcpu()->ci_cpuid;
	unsigned long hartmask = 0;
	struct cpu_info *ci;
	CPU_INFO_ITERATOR cii;
	for (CPU_INFO_FOREACH(cii, ci)) {
		const cpuid_t hartid = ci->ci_cpuid;

		if (hartid == myhartid)
			continue;

		KASSERT(hartid < sizeof(unsigned long) * NBBY);
		hartmask |= __BIT(hartid);
	}
	for (asid = lo; asid <= hi; asid++) {
		struct sbiret sbiret = sbi_remote_sfence_vma_asid(hartmask,
		    0 /* hartmask_base */, 0, ~0UL, asid);

		KASSERTMSG(sbiret.error == SBI_SUCCESS, "error %ld",
		    sbiret.error);
	}
#endif
#endif
}

void
tlb_invalidate_addr(vaddr_t va, tlb_asid_t asid)
{
	if (asid == KERNEL_PID) {
		asm volatile("sfence.vma %[va]"
		    : /* output operands */
		    : [va] "r" (va)
		    : "memory");
	} else {
		asm volatile("sfence.vma %[va], %[asid]"
		    : /* output operands */
		    : [va] "r" (va), [asid] "r" (asid)
		    : "memory");
	}
#ifdef MULTIPROCESSOR
#if PMAP_TLB_MAX == 1
	const cpuid_t myhartid = curcpu()->ci_cpuid;
	unsigned long hartmask = 0;
	struct cpu_info *ci;
	CPU_INFO_ITERATOR cii;
	for (CPU_INFO_FOREACH(cii, ci)) {
		const cpuid_t hartid = ci->ci_cpuid;
		if (hartid == myhartid)
			continue;

		KASSERT(hartid < sizeof(unsigned long) * NBBY);
		hartmask |= __BIT(hartid);
	}
	struct sbiret sbiret = sbi_remote_sfence_vma(hartmask,
	    0 /* hartmask_base */, va, PAGE_SIZE);

	KASSERTMSG(sbiret.error == SBI_SUCCESS, "error %ld",
	    sbiret.error);
#endif
#endif
}

bool
tlb_update_addr(vaddr_t va, tlb_asid_t asid, pt_entry_t pte, bool insert_p)
{
	KASSERT((va & PAGE_MASK) == 0);

	tlb_invalidate_addr(va, asid);

	return true;
}

u_int
tlb_record_asids(u_long *ptr, tlb_asid_t asid_max)
{
	memset(ptr, 0xff, PMAP_TLB_NUM_PIDS / NBBY);
	ptr[0] = -2UL;

	return PMAP_TLB_NUM_PIDS - 1;
}

void
tlb_walk(void *ctx, bool (*func)(void *, vaddr_t, tlb_asid_t, pt_entry_t))
{
	/* no way to view the TLB */
}
