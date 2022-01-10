/* $NetBSD: kvm_aarch64.c,v 1.11 2022/01/10 19:51:30 christos Exp $ */

/*-
 * Copyright (c) 2014, 2018 The NetBSD Foundation, Inc.
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

#include <sys/param.h>
#include <sys/proc.h>
#include <sys/stat.h>
#include <sys/kcore.h>
#include <sys/types.h>
#include <unistd.h>
#include <nlist.h>
#include <kvm.h>

#include <machine/kcore.h>
#include <machine/armreg.h>
#include <machine/pte.h>
#include <machine/vmparam.h>

#include <limits.h>
#include <db.h>
#include <stdlib.h>

#include "kvm_private.h"

__RCSID("$NetBSD: kvm_aarch64.c,v 1.11 2022/01/10 19:51:30 christos Exp $");

/*ARGSUSED*/
void
_kvm_freevtop(kvm_t *kd)
{
	return;
}

/*ARGSUSED*/
int
_kvm_initvtop(kvm_t *kd)
{
	return (0);
}

int
_kvm_kvatop(kvm_t *kd, vaddr_t va, paddr_t *pa)
{
        if (ISALIVE(kd)) {
                _kvm_err(kd, 0, "vatop called in live kernel!");
                return(0);
        }

	if ((va & AARCH64_DIRECTMAP_MASK) != AARCH64_DIRECTMAP_START) {
		/*
		 * Bogus address (not in KV space): punt.
		 */
		_kvm_err(kd, 0, "invalid kernel virtual address");
lose:
		*pa = -1;
		return 0;
	}

	const cpu_kcore_hdr_t * const cpu_kh = kd->cpu_data;
	const uint64_t tg1 = cpu_kh->kh_tcr1 & TCR_TG1;
	const u_int t1siz = __SHIFTOUT(cpu_kh->kh_tcr1, TCR_T1SZ);
	const u_int inputsz = 64 - t1siz;

	/*
	 * Real kernel virtual address: do the translation.
	 */

	u_int page_shift;

	switch (tg1) {
	case TCR_TG1_4KB:
		page_shift = 12;
		break;
	case TCR_TG1_16KB:
		page_shift = 14;
		break;
	case TCR_TG1_64KB:
		page_shift = 16;
		break;
	default:
		goto lose;
	}

	const size_t page_size = 1 << page_shift;
	const uint64_t page_mask = __BITS(page_shift - 1, 0);
	const uint64_t page_addr = __BITS(47, page_shift);
	const u_int pte_shift = page_shift - 3;

	/* how many levels of page tables do we have? */
	u_int levels = howmany(inputsz - page_shift, pte_shift);

	/* restrict va to the valid VA bits */
	va &= __BITS(inputsz - 1, 0);

	u_int addr_shift = page_shift + (levels - 1) * pte_shift;

	/* clear out the unused low bits of the table address */
	paddr_t pte_addr = cpu_kh->kh_ttbr1 & TTBR_BADDR;

	for (;;) {
		pt_entry_t pte;

		/* now index into the pte table */
		const uint64_t idx_mask = __BITS(addr_shift + pte_shift - 1,
						 addr_shift);
		pte_addr += 8 * __SHIFTOUT(va, idx_mask);

		/* Find and read the PTE. */
		if (_kvm_pread(kd, kd->pmfd, &pte, sizeof(pte),
		    _kvm_pa2off(kd, pte_addr)) != sizeof(pte)) {
			_kvm_syserr(kd, 0, "could not read pte");
			goto lose;
		}

		/* Find and read the L2 PTE. */
		if ((pte & LX_VALID) == 0) {
			_kvm_err(kd, 0, "invalid translation (invalid pte)");
			goto lose;
		}

		if ((pte & LX_TYPE) == LX_TYPE_BLK) {
			const size_t blk_size = 1 << addr_shift;
			const uint64_t blk_mask = __BITS(addr_shift - 1, 0);

			*pa = (pte & page_addr & ~blk_mask) | (va & blk_mask);
			return blk_size - (va & blk_mask);
		}
		if (--levels == 0) {
			*pa = (pte & page_addr) | (va & page_mask);
			return page_size - (va & page_mask); 
		}

		/*
		 * Read next level of page table
		 */

		pte_addr = pte & page_addr;
		addr_shift -= pte_shift;
	}
}

/*
 * Translate a physical address to a file-offset in the crash dump.
 */
off_t
_kvm_pa2off(kvm_t *kd, paddr_t pa)
{
	const cpu_kcore_hdr_t * const cpu_kh = kd->cpu_data;
	off_t off = 0;

	for (const phys_ram_seg_t *ramsegs = cpu_kh->kh_ramsegs;
	     ramsegs->size != 0; ramsegs++) {
		if (pa >= ramsegs->start
		    && pa < ramsegs->start + ramsegs->size) {
			off += pa - ramsegs->start;
			break;
		}
		off += ramsegs->size;
	}

	return kd->dump_off + off;
}

/*
 * Machine-dependent initialization for ALL open kvm descriptors,
 * not just those for a kernel crash dump.  Some architectures
 * have to deal with these NOT being constants!  (i.e. m68k)
 */
int
_kvm_mdopen(kvm_t *kd)
{

	kd->min_uva = VM_MIN_ADDRESS;
	kd->max_uva = VM_MAXUSER_ADDRESS;

	return (0);
}
