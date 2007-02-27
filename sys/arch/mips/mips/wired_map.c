/*	$NetBSD: wired_map.c,v 1.2.28.1 2007/02/27 16:52:10 yamt Exp $	*/

/*-
 * Copyright (c) 2005 Tadpole Computer Inc.
 * All rights reserved.
 *
 * Written by Garrett D'Amore for Tadpole Computer Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of Tadpole Computer Inc. may not be used to endorse
 *    or promote products derived from this software without specific
 *    prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY TADPOLE COMPUTER INC. ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL TADPOLE COMPUTER INC.
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */ 
/*
 * Copyright (C) 2000 Shuichiro URATA.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * This code is derived from similiar code in the ARC port of NetBSD, but
 * it now bears little resemblence to it owing to quite different needs
 * from the mapping logic.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: wired_map.c,v 1.2.28.1 2007/02/27 16:52:10 yamt Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <uvm/uvm_extern.h>
#include <machine/cpu.h>
#include <machine/pte.h>
#include <machine/vmparam.h>
#include <machine/wired_map.h>

#include <mips/locore.h>
#include <mips/pte.h>

struct wired_map_entry mips3_wired_map[MIPS3_NWIRED_ENTRY];
int mips3_nwired_page;

/*
 * Lower layer API, to supply an explicit page size.  It only wires a
 * single page at a time.
 */
bool
mips3_wired_enter_page(vaddr_t va, paddr_t pa, vsize_t pgsize)
{
	struct tlb tlb;
	vaddr_t va0;
	int found, index;

	/* make sure entries are aligned */
	KASSERT((va & (pgsize - 1)) == 0);
	KASSERT((pa & (pgsize - 1)) == 0);

	/* TLB entries come in pairs: this is the first address of the pair */
	va0 = va & ~MIPS3_WIRED_ENTRY_OFFMASK(pgsize);

	found = 0;
	for (index = 0; index < mips3_nwired_page; index++) {
		if (mips3_wired_map[index].va == va0) {
			if ((va & pgsize) == 0) {
				/* EntryLo0 */
				mips3_wired_map[index].pa0 = pa;
			} else {
				/* EntryLo1 */
				mips3_wired_map[index].pa1 = pa;
			}
			found = 1;
			break;
		}
	}

	if (found == 0) {
		/* we have to allocate a new wired entry */
		if (mips3_nwired_page >= MIPS3_NWIRED_ENTRY) {
#ifdef DIAGNOSTIC
			printf("mips3_wired_map: entries exhausted\n");
#endif
			return FALSE;
		}

		index = mips3_nwired_page;
		mips3_nwired_page++;
		if (va == va0) {
			/* EntryLo0 */
			mips3_wired_map[index].pa0 = pa;
			mips3_wired_map[index].pa1 = 0;
		} else {
			/* EntryLo1 */
			mips3_wired_map[index].pa0 = 0;
			mips3_wired_map[index].pa1 = pa;
		}
		mips3_wired_map[index].va = va0;
		mips3_wired_map[index].pgmask = MIPS3_PG_SIZE_TO_MASK(pgsize);

		/* Allocate new wired entry */
		mips3_cp0_wired_write(MIPS3_TLB_WIRED_UPAGES +
		    mips3_nwired_page + 1);
	}

	/* map it */
	tlb.tlb_mask = mips3_wired_map[index].pgmask;
	tlb.tlb_hi = mips3_vad_to_vpn(va0);
	if (mips3_wired_map[index].pa0 == 0)
		tlb.tlb_lo0 = MIPS3_PG_G;
	else
		tlb.tlb_lo0 =
		    mips3_paddr_to_tlbpfn(mips3_wired_map[index].pa0) |
		    MIPS3_PG_IOPAGE(
			    PMAP_CCA_FOR_PA(mips3_wired_map[index].pa0));
	if (mips3_wired_map[index].pa1 == 0)
		tlb.tlb_lo1 = MIPS3_PG_G;
	else
		tlb.tlb_lo1 = mips3_paddr_to_tlbpfn(
			mips3_wired_map[index].pa1) |
		    MIPS3_PG_IOPAGE(
			    PMAP_CCA_FOR_PA(mips3_wired_map[index].pa1));
	MachTLBWriteIndexedVPS(MIPS3_TLB_WIRED_UPAGES + index, &tlb);
	return TRUE;
}


/*
 * Wire down a mapping from a virtual to physical address.  The size
 * of the region must be a multiple of MIPS3_WIRED_SIZE, with
 * matching alignment.
 *
 * Typically the caller will just pass a physaddr that is the same as
 * the vaddr with bits 35-32 set nonzero.
 */
bool
mips3_wired_enter_region(vaddr_t va, paddr_t pa, vsize_t size)
{
	vaddr_t	vend;
	/*
	 * This routine allows for for wired mappings to be set up,
	 * and handles previously defined mappings and mapping
	 * overlaps reasonably well.  However, caution should be used
	 * not to attempt to change the mapping for a page unless you
	 * are certain that you are the only user of the virtual
	 * address space, otherwise chaos may ensue.
	 */

	/* offsets within the page have to be identical */
	KASSERT((va & MIPS3_WIRED_OFFMASK) == (pa & MIPS3_WIRED_OFFMASK));

	vend = va + size;
	/* adjust for alignment */
	va &= ~MIPS3_WIRED_OFFMASK;
	pa &= ~MIPS3_WIRED_OFFMASK;

	while (va < vend) {
		if (!mips3_wired_enter_page(va, pa, MIPS3_WIRED_SIZE))
			return FALSE;
		va += MIPS3_WIRED_SIZE;
		pa += MIPS3_WIRED_SIZE;
	}
	return TRUE;
}
