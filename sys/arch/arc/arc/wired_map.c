/*	$NetBSD: wired_map.c,v 1.6 2002/03/05 16:11:57 simonb Exp $	*/

/*-
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

#include <sys/param.h>
#include <sys/systm.h>
#include <uvm/uvm_extern.h>
#include <machine/cpu.h>
#include <mips/locore.h>
#include <mips/pte.h>
#include <arc/arc/wired_map.h>

#define MIPS3_PG_SIZE_MASK_TO_SIZE(sizemask)	\
	((((sizemask) | 0x00001fff) + 1) / 2)

#define VA_FREE_START	0xe0000000	/* XXX */

#define ARC_TLB_WIRED_ENTRIES	8	/* upper limit */
#define ARC_WIRED_PG_MASK	MIPS3_PG_SIZE_16M
#define ARC_WIRED_PAGE_SIZE	MIPS3_PG_SIZE_MASK_TO_SIZE(ARC_WIRED_PG_MASK)
#define ARC_WIRED_ENTRY_SIZE	(ARC_WIRED_PAGE_SIZE * 2)
#define ARC_WIRED_ENTRY_OFFMASK	(ARC_WIRED_ENTRY_SIZE - 1)

static struct wired_map_entry {
	paddr_t	pa0;
	paddr_t	pa1;
	vsize_t	size;
	vaddr_t	va;
} wired_map[ARC_TLB_WIRED_ENTRIES];

boolean_t arc_wired_map_paddr_entry __P((paddr_t pa,
	    vaddr_t *vap, vsize_t *sizep));
boolean_t arc_wired_map_vaddr_entry __P((vaddr_t va,
	    paddr_t *pap, vsize_t *sizep));

static int	nwired;
static vaddr_t	va_free;

void
arc_init_wired_map()
{
	nwired = 0;
	va_free = VA_FREE_START;
}

void
arc_enter_wired(va, pa0, pa1, pg_size)
	vaddr_t va;
	paddr_t pa0;
	paddr_t pa1;
	u_int32_t pg_size;
{
	struct tlb tlb;

	if (nwired >= ARC_TLB_WIRED_ENTRIES)
		panic("arc_enter_wired: wired entry exausted");

	wired_map[nwired].va = va;
	wired_map[nwired].pa0 = pa0;
	wired_map[nwired].pa1 = pa1;
	wired_map[nwired].size = MIPS3_PG_SIZE_MASK_TO_SIZE(pg_size);

	/* Allocate new wired entry */
	mips3_cp0_wired_write(MIPS3_TLB_WIRED_UPAGES + nwired + 1);

	/* Map to it */
	tlb.tlb_mask = pg_size;
	tlb.tlb_hi = mips3_vad_to_vpn(va);
	if (pa0 == 0)
		tlb.tlb_lo0 = MIPS3_PG_G;
	else
		tlb.tlb_lo0 = mips3_paddr_to_tlbpfn(pa0) | \
		    MIPS3_PG_IOPAGE(PMAP_CCA_FOR_PA(pa0));
	if (pa1 == 0)
		tlb.tlb_lo1 = MIPS3_PG_G;
	else
		tlb.tlb_lo1 = mips3_paddr_to_tlbpfn(pa1) | \
		    MIPS3_PG_IOPAGE(PMAP_CCA_FOR_PA(pa1));
	mips3_TLBWriteIndexedVPS(MIPS3_TLB_WIRED_UPAGES + nwired,
	    &tlb);

	if (va_free < va + wired_map[nwired].size * 2) {
		va_free = va + wired_map[nwired].size * 2;
	}

	nwired++;
}

boolean_t
arc_wired_map_paddr_entry(pa, vap, sizep)
	paddr_t pa;
	vaddr_t *vap;
	vsize_t *sizep;
{
	int n = nwired;
	struct wired_map_entry *entry = wired_map;

	for (; --n >= 0; entry++) {
		if (entry->pa0 != 0 &&
		    pa >= entry->pa0 && pa < entry->pa0 + entry->size) {
			*vap = entry->va;
			*sizep = entry->size;
			return (1);
		}
		if (entry->pa1 != 0 &&
		    pa >= entry->pa1 && pa < entry->pa1 + entry->size) {
			*vap = entry->va + entry->size;
			*sizep = entry->size;
			return (1);
		}
	}
	return (0);
}

/* XXX: Using tlbp makes this easier... */
boolean_t
arc_wired_map_vaddr_entry(va, pap, sizep)
	vaddr_t va;
	paddr_t *pap;
	vsize_t *sizep;
{
	int n = nwired;
	struct wired_map_entry *entry = wired_map;

	for (; --n >= 0; entry++) {
		if (va >= entry->va && va < entry->va + entry->size * 2) {
			paddr_t pa = (va < entry->va + entry->size)
				? entry->pa0 : entry->pa1;

			if (pa != 0) {
				*pap = pa;
				*sizep = entry->size;
				return (1);
			}
		}
	}
	return (0);
}

vaddr_t
arc_contiguously_wired_mapped(pa, size)
	paddr_t pa;
	int size;
{
	paddr_t p;
	vaddr_t rva, va;
	vsize_t vsize, offset;

	if (!arc_wired_map_paddr_entry(pa, &rva, &vsize))
		return (0); /* not wired mapped */
	/* XXX: same physical address may be wired mapped more than once */
	offset = (vsize_t)pa & (vsize - 1);
	pa -= offset;
	size += offset;
	va = rva;
	for (;;) {
		pa += vsize;
		va += vsize;
		size -= vsize;
		if (size <= 0)
			break;
		if (!arc_wired_map_vaddr_entry(va, &p, &vsize) || p != pa)
			return (0); /* not contiguously wired mapped */
	}
	return (rva + offset);
}

/* Allocate new wired entries */
vaddr_t
arc_map_wired(pa, size)
	paddr_t pa;
	int size;
{
	vaddr_t va, rva;
	vsize_t off;

	/* XXX: may be already partially wired mapped */

	off = pa & ARC_WIRED_ENTRY_OFFMASK;
	rva = (va_free + ARC_WIRED_ENTRY_OFFMASK) & ~ARC_WIRED_ENTRY_OFFMASK;
	pa &= ~(paddr_t)ARC_WIRED_ENTRY_OFFMASK;
	va = rva;
	size += off;

	if ((size + ARC_WIRED_ENTRY_SIZE - 1) / ARC_WIRED_ENTRY_SIZE >
	    ARC_TLB_WIRED_ENTRIES - nwired) {
#ifdef DIAGNOSTIC
		printf("arc_map_wired(0x%llx, 0x%lx): %d is not enough\n",
		       pa + off, size - off, ARC_TLB_WIRED_ENTRIES - nwired);
#endif
		return (0); /* free wired TLB is not enough */
	}

	while (size > 0) {
		arc_enter_wired(va, pa, pa + ARC_WIRED_PAGE_SIZE,
		    ARC_WIRED_PG_MASK);
		pa += ARC_WIRED_ENTRY_SIZE;
		va += ARC_WIRED_ENTRY_SIZE;
		size -= ARC_WIRED_ENTRY_SIZE;
	}

	return (rva + off);
}

boolean_t
arc_wired_map_extract(va, pap)
	vaddr_t va;
	paddr_t *pap;
{
	paddr_t pa;
	vsize_t size;

	if (arc_wired_map_vaddr_entry(va, &pa, &size)) {
		*pap = pa + (va & (size - 1));
		return (1);
	} else {
		return (0);
	}
}
