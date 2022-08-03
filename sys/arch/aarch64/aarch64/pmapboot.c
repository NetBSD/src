/*	$NetBSD: pmapboot.c,v 1.18 2022/08/03 17:55:05 ryo Exp $	*/

/*
 * Copyright (c) 2018 Ryo Shimizu <ryo@nerv.org>
 * All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: pmapboot.c,v 1.18 2022/08/03 17:55:05 ryo Exp $");

#include "opt_arm_debug.h"
#include "opt_ddb.h"
#include "opt_multiprocessor.h"
#include "opt_pmap.h"
#include "opt_pmapboot.h"

#include <sys/param.h>
#include <sys/types.h>

#include <uvm/uvm.h>

#include <arm/cpufunc.h>

#include <aarch64/armreg.h>
#ifdef DDB
#include <aarch64/db_machdep.h>
#endif
#include <aarch64/machdep.h>
#include <aarch64/pmap.h>
#include <aarch64/pte.h>

#include <arm/cpufunc.h>

#define OPTIMIZE_TLB_CONTIG

static void
pmapboot_protect_entry(pt_entry_t *pte, vm_prot_t clrprot)
{
	extern uint64_t pmap_attr_gp;

	if (clrprot & VM_PROT_READ)
		*pte &= ~LX_BLKPAG_AF;
	if (clrprot & VM_PROT_WRITE) {
		*pte &= ~LX_BLKPAG_AP;
		*pte |= LX_BLKPAG_AP_RO;
		*pte |= pmap_attr_gp;
	}
	if (clrprot & VM_PROT_EXECUTE)
		*pte |= LX_BLKPAG_UXN|LX_BLKPAG_PXN;
}

/*
 * like pmap_protect(), but not depend on struct pmap.
 * this work before pmap_bootstrap().
 * 'clrprot' specified by bit of VM_PROT_{READ,WRITE,EXECUTE}
 * will be dropped from a pte entry.
 *
 * require direct (cached) mappings because TLB entries are already cached on.
 */
int
pmapboot_protect(vaddr_t sva, vaddr_t eva, vm_prot_t clrprot)
{
	int idx;
	vaddr_t va;
	paddr_t pa;
	pd_entry_t *l0, *l1, *l2, *l3;

	switch (aarch64_addressspace(sva)) {
	case AARCH64_ADDRSPACE_LOWER:
		/* 0x0000xxxxxxxxxxxx */
		pa = (reg_ttbr0_el1_read() & TTBR_BADDR);
		break;
	case AARCH64_ADDRSPACE_UPPER:
		/* 0xFFFFxxxxxxxxxxxx */
		pa = (reg_ttbr1_el1_read() & TTBR_BADDR);
		break;
	default:
		return -1;
	}
	l0 = (pd_entry_t *)AARCH64_PA_TO_KVA(pa);

	for (va = sva; va < eva;) {
		idx = l0pde_index(va);
		if (!l0pde_valid(l0[idx]))
			return -1;
		pa = l0pde_pa(l0[idx]);
		l1 = (pd_entry_t *)AARCH64_PA_TO_KVA(pa);

		idx = l1pde_index(va);
		if (!l1pde_valid(l1[idx]))
			return -1;
		if (l1pde_is_block(l1[idx])) {
			pmapboot_protect_entry(&l1[idx], clrprot);
			va += L1_SIZE;
			continue;
		}
		pa = l1pde_pa(l1[idx]);
		l2 = (pd_entry_t *)AARCH64_PA_TO_KVA(pa);

		idx = l2pde_index(va);
		if (!l2pde_valid(l2[idx]))
			return -1;
		if (l2pde_is_block(l2[idx])) {
			pmapboot_protect_entry(&l2[idx], clrprot);
			va += L2_SIZE;
			continue;
		}
		pa = l2pde_pa(l2[idx]);
		l3 = (pd_entry_t *)AARCH64_PA_TO_KVA(pa);

		idx = l3pte_index(va);
		if (!l3pte_valid(l3[idx]))
			return -1;
		if (!l3pte_is_page(l3[idx]))
			return -1;

		pmapboot_protect_entry(&l3[idx], clrprot);
		va += L3_SIZE;
	}

	return 0;
}


/*
 * these function will be called from locore without MMU.
 * the load address varies depending on the bootloader.
 * cannot use absolute addressing to refer text/data/bss.
 *
 * (*pr) function may be minimal printf. (when provided from locore)
 * it supports only maximum 7 argument, and only '%d', '%x', and '%s' formats.
 */

#ifdef VERBOSE_INIT_ARM
#define VPRINTF(fmt, args...)	\
	while (pr != NULL) { pr(fmt, ## args); break; }
#else
#define VPRINTF(fmt, args...)	__nothing
#endif

#ifdef PMAPBOOT_DEBUG
static void
pmapboot_pte_print(pt_entry_t pte, int level,
    void (*pr)(const char *, ...) __printflike(1, 2))
{
#ifdef DDB
	db_pte_print(pte, level, pr);
#else
	__USE(level);
	pr(" %s PA=%016lx\n",
	    l0pde_valid(pte) ? "VALID" : "INVALID",
	    l0pde_pa(pte));
#endif
}
#define PMAPBOOT_DPRINTF(fmt, args...)	\
	while (pr != NULL) { pr(fmt, ## args); break; }
#define PMAPBOOT_DPRINT_PTE(pte, l)	\
	while (pr != NULL) { pmapboot_pte_print((pte), (l), pr); break; }
#else /* PMAPBOOT_DEBUG */
#define PMAPBOOT_DPRINTF(fmt, args...)	__nothing
#define PMAPBOOT_DPRINT_PTE(pte, l)	__nothing
#endif /* PMAPBOOT_DEBUG */


#ifdef OPTIMIZE_TLB_CONTIG
static inline bool
tlb_contiguous_p(vaddr_t va, paddr_t pa, vaddr_t start, vaddr_t end,
    vsize_t blocksize)
{
	/*
	 * when using 4KB granule, 16 adjacent and aligned entries can be
	 * unified to one TLB cache entry.
	 * in other size of granule, not supported.
	 */
	const vaddr_t mask = (blocksize << 4) - 1;

	/* if the output address doesn't align it can't be contiguous */
	if ((va & mask) != (pa & mask))
		return false;

	if ((va & ~mask) >= start && (va | mask) <= end)
		return true;

	return false;
}
#endif /* OPTIMIZE_TLB_CONTIG */

/*
 * pmapboot_enter() accesses pagetables by physical address.
 * this should be called while identity mapping (VA=PA) available.
 */
void
pmapboot_enter(vaddr_t va, paddr_t pa, psize_t size, psize_t blocksize,
    pt_entry_t attr, void (*pr)(const char *, ...) __printflike(1, 2))
{
	int level, idx0, idx1, idx2, idx3, nskip = 0;
	int ttbr __unused;
	vaddr_t va_end;
	pd_entry_t *l0, *l1, *l2, *l3, pte;
#ifdef OPTIMIZE_TLB_CONTIG
	vaddr_t va_start;
	pd_entry_t *ll;
	int i, llidx;
#endif

	switch (blocksize) {
	case L1_SIZE:
		level = 1;
		break;
	case L2_SIZE:
		level = 2;
		break;
	case L3_SIZE:
		level = 3;
		break;
	default:
		panic("%s: bad blocksize (%" PRIxPSIZE ")", __func__, blocksize);
	}

	VPRINTF("pmapboot_enter: va=0x%lx, pa=0x%lx, size=0x%lx, "
	    "blocksize=0x%lx, attr=0x%016lx\n",
	    va, pa, size, blocksize, attr);

	pa &= ~(blocksize - 1);
	va_end = (va + size + blocksize - 1) & ~(blocksize - 1);
	va &= ~(blocksize - 1);
#ifdef OPTIMIZE_TLB_CONTIG
	va_start = va;
#endif

	attr |= LX_BLKPAG_OS_BOOT;

	switch (aarch64_addressspace(va)) {
	case AARCH64_ADDRSPACE_LOWER:
		/* 0x0000xxxxxxxxxxxx */
		l0 = (pd_entry_t *)(reg_ttbr0_el1_read() & TTBR_BADDR);
		ttbr = 0;
		break;
	case AARCH64_ADDRSPACE_UPPER:
		/* 0xFFFFxxxxxxxxxxxx */
		l0 = (pd_entry_t *)(reg_ttbr1_el1_read() & TTBR_BADDR);
		ttbr = 1;
		break;
	default:
		panic("%s: unknown address space (%d/%" PRIxVADDR ")", __func__,
		    aarch64_addressspace(va), va);
	}

	while (va < va_end) {
#ifdef OPTIMIZE_TLB_CONTIG
		ll = NULL;
		llidx = -1;
#endif

		idx0 = l0pde_index(va);
		if (l0[idx0] == 0) {
			l1 = pmapboot_pagealloc();
			if (l1 == NULL) {
				VPRINTF("pmapboot_enter: "
				    "cannot allocate L1 page\n");
				panic("%s: can't allocate memory", __func__);
			}

			pte = (uint64_t)l1 | L0_TABLE;
			l0[idx0] = pte;
			PMAPBOOT_DPRINTF("TTBR%d[%d] (new)\t= %016lx:",
			    ttbr, idx0, pte);
			PMAPBOOT_DPRINT_PTE(pte, 0);
		} else {
			l1 = (uint64_t *)(l0[idx0] & LX_TBL_PA);
		}

		idx1 = l1pde_index(va);
		if (level == 1) {
			pte = pa |
			    L1_BLOCK |
			    LX_BLKPAG_AF |
#ifdef MULTIPROCESSOR
			    LX_BLKPAG_SH_IS |
#endif
			    attr;
#ifdef OPTIMIZE_TLB_CONTIG
			if (tlb_contiguous_p(va, pa, va_start, va_end, blocksize))
				pte |= LX_BLKPAG_CONTIG;
			ll = l1;
			llidx = idx1;
#endif

			if (l1pde_valid(l1[idx1]) && l1[idx1] != pte) {
				nskip++;
				goto nextblk;
			}

			l1[idx1] = pte;
			PMAPBOOT_DPRINTF("TTBR%d[%d][%d]\t= %016lx:", ttbr,
			    idx0, idx1, pte);
			PMAPBOOT_DPRINT_PTE(pte, 1);
			goto nextblk;
		}

		if (!l1pde_valid(l1[idx1])) {
			l2 = pmapboot_pagealloc();
			if (l2 == NULL) {
				VPRINTF("pmapboot_enter: "
				    "cannot allocate L2 page\n");
				panic("%s: can't allocate memory", __func__);
			}

			pte = (uint64_t)l2 | L1_TABLE;
			l1[idx1] = pte;
			PMAPBOOT_DPRINTF("TTBR%d[%d][%d] (new)\t= %016lx:",
			    ttbr, idx0, idx1, pte);
			PMAPBOOT_DPRINT_PTE(pte, 1);
		} else {
			l2 = (uint64_t *)(l1[idx1] & LX_TBL_PA);
		}

		idx2 = l2pde_index(va);
		if (level == 2) {
			pte = pa |
			    L2_BLOCK |
			    LX_BLKPAG_AF |
#ifdef MULTIPROCESSOR
			    LX_BLKPAG_SH_IS |
#endif
			    attr;
#ifdef OPTIMIZE_TLB_CONTIG
			if (tlb_contiguous_p(va, pa, va_start, va_end, blocksize))
				pte |= LX_BLKPAG_CONTIG;
			ll = l2;
			llidx = idx2;
#endif
			if (l2pde_valid(l2[idx2]) && l2[idx2] != pte) {
				nskip++;
				goto nextblk;
			}

			l2[idx2] = pte;
			PMAPBOOT_DPRINTF("TTBR%d[%d][%d][%d]\t= %016lx:", ttbr,
			    idx0, idx1, idx2, pte);
			PMAPBOOT_DPRINT_PTE(pte, 2);
			goto nextblk;
		}

		if (!l2pde_valid(l2[idx2])) {
			l3 = pmapboot_pagealloc();
			if (l3 == NULL) {
				VPRINTF("pmapboot_enter: "
				    "cannot allocate L3 page\n");
				panic("%s: can't allocate memory", __func__);
			}

			pte = (uint64_t)l3 | L2_TABLE;
			l2[idx2] = pte;
			PMAPBOOT_DPRINTF("TTBR%d[%d][%d][%d] (new)\t= %016lx:",
			    ttbr, idx0, idx1, idx2, pte);
			PMAPBOOT_DPRINT_PTE(pte, 2);
		} else {
			l3 = (uint64_t *)(l2[idx2] & LX_TBL_PA);
		}

		idx3 = l3pte_index(va);

		pte = pa |
		    L3_PAGE |
		    LX_BLKPAG_AF |
#ifdef MULTIPROCESSOR
		    LX_BLKPAG_SH_IS |
#endif
		    attr;
#ifdef OPTIMIZE_TLB_CONTIG
		if (tlb_contiguous_p(va, pa, va_start, va_end, blocksize))
			pte |= LX_BLKPAG_CONTIG;
		ll = l3;
		llidx = idx3;
#endif
		if (l3pte_valid(l3[idx3]) && l3[idx3] != pte) {
			nskip++;
			goto nextblk;
		}

		l3[idx3] = pte;
		PMAPBOOT_DPRINTF("TTBR%d[%d][%d][%d][%d]\t= %lx:", ttbr,
		    idx0, idx1, idx2, idx3, pte);
		PMAPBOOT_DPRINT_PTE(pte, 3);
 nextblk:
#ifdef OPTIMIZE_TLB_CONTIG
		/*
		 * when overwriting a pte entry the contiguous bit in entries
		 * before/after the entry should be cleared.
		 */
		if (ll != NULL) {
			if (va == va_start && (llidx & 15) != 0) {
				/* clear CONTIG flag before this pte entry */
				for (i = (llidx & ~15); i < llidx; i++) {
					ll[i] &= ~LX_BLKPAG_CONTIG;
				}
			}
			if (va == va_end && (llidx & 15) != 15) {
				/* clear CONTIG flag after this pte entry */
				for (i = (llidx + 1); i < ((llidx + 16) & ~15);
				    i++) {
					ll[i] &= ~LX_BLKPAG_CONTIG;
				}
			}
		}
#endif
		switch (level) {
		case 1:
			va += L1_SIZE;
			pa += L1_SIZE;
			break;
		case 2:
			va += L2_SIZE;
			pa += L2_SIZE;
			break;
		case 3:
			va += L3_SIZE;
			pa += L3_SIZE;
			break;
		}
	}

	dsb(ish);

	if (nskip != 0)
		panic("%s: overlapping/incompatible mappings (%d)", __func__, nskip);
}

paddr_t pmapboot_pagebase __attribute__((__section__(".data")));

pd_entry_t *
pmapboot_pagealloc(void)
{
	extern long kernend_extra;

	if (kernend_extra < 0)
		return NULL;

	paddr_t pa = pmapboot_pagebase + kernend_extra;
	kernend_extra += PAGE_SIZE;

	char *s = (char *)pa;
	char *e = s + PAGE_SIZE;

	while (s < e)
		*s++ = 0;

	return (pd_entry_t *)pa;
}

void
pmapboot_enter_range(vaddr_t va, paddr_t pa, psize_t size, pt_entry_t attr,
    void (*pr)(const char *, ...) __printflike(1, 2))
{
	vaddr_t vend;
	vsize_t left, mapsize, nblocks;

	vend = round_page(va + size);
	va = trunc_page(va);
	left = vend - va;

	/* align the start address to L2 blocksize */
	nblocks = ulmin(left / L3_SIZE,
	    Ln_ENTRIES - __SHIFTOUT(va, L3_ADDR_BITS));
	if (((va & L3_ADDR_BITS) != 0) && (nblocks > 0)) {
		mapsize = nblocks * L3_SIZE;
		VPRINTF("Creating L3 tables: %016lx-%016lx : %016lx-%016lx\n",
		    va, va + mapsize - 1, pa, pa + mapsize - 1);
		pmapboot_enter(va, pa, mapsize, L3_SIZE, attr, pr);
		va += mapsize;
		pa += mapsize;
		left -= mapsize;
	}

	/* align the start address to L1 blocksize */
	nblocks = ulmin(left / L2_SIZE,
	    Ln_ENTRIES - __SHIFTOUT(va, L2_ADDR_BITS));
	if (((va & L2_ADDR_BITS) != 0) && (nblocks > 0)) {
		mapsize = nblocks * L2_SIZE;
		VPRINTF("Creating L2 tables: %016lx-%016lx : %016lx-%016lx\n",
		    va, va + mapsize - 1, pa, pa + mapsize - 1);
		pmapboot_enter(va, pa, mapsize, L2_SIZE, attr, pr);
		va += mapsize;
		pa += mapsize;
		left -= mapsize;
	}

	nblocks = left / L1_SIZE;
	if (nblocks > 0) {
		mapsize = nblocks * L1_SIZE;
		VPRINTF("Creating L1 tables: %016lx-%016lx : %016lx-%016lx\n",
		    va, va + mapsize - 1, pa, pa + mapsize - 1);
		pmapboot_enter(va, pa, mapsize, L1_SIZE, attr, pr);
		va += mapsize;
		pa += mapsize;
		left -= mapsize;
	}

	if ((left & L2_ADDR_BITS) != 0) {
		nblocks = left / L2_SIZE;
		mapsize = nblocks * L2_SIZE;
		VPRINTF("Creating L2 tables: %016lx-%016lx : %016lx-%016lx\n",
		    va, va + mapsize - 1, pa, pa + mapsize - 1);
		pmapboot_enter(va, pa, mapsize, L2_SIZE, attr, pr);
		va += mapsize;
		pa += mapsize;
		left -= mapsize;
	}

	if ((left & L3_ADDR_BITS) != 0) {
		nblocks = left / L3_SIZE;
		mapsize = nblocks * L3_SIZE;
		VPRINTF("Creating L3 tables: %016lx-%016lx : %016lx-%016lx\n",
		    va, va + mapsize - 1, pa, pa + mapsize - 1);
		pmapboot_enter(va, pa, mapsize, L3_SIZE, attr, pr);
		va += mapsize;
		pa += mapsize;
		left -= mapsize;
	}
}
