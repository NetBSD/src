/*	$NetBSD: wired_map_machdep.c,v 1.9 2023/12/20 06:36:02 thorpej Exp $	*/

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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: wired_map_machdep.c,v 1.9 2023/12/20 06:36:02 thorpej Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/vmem_impl.h>

#include <uvm/uvm_extern.h>
#include <machine/cpu.h>
#include <machine/wired_map.h>
#include <machine/vmparam.h>
#include <mips/locore.h>
#include <mips/pte.h>

static bool arc_wired_map_paddr_entry(paddr_t pa, vaddr_t *vap,
    vsize_t *sizep);
static bool arc_wired_map_vaddr_entry(vaddr_t va, paddr_t *pap,
    vsize_t *sizep);

#define	ARC_WIRED_MAP_BTCOUNT	VMEM_EST_BTCOUNT(1, 8)

static vmem_t *arc_wired_map_arena;
static struct vmem arc_wired_map_arena_store;
static struct vmem_btag arc_wired_map_btag_store[ARC_WIRED_MAP_BTCOUNT];

void
arc_init_wired_map(void)
{
	int error __diagused;

	mips3_nwired_page = 0;

	arc_wired_map_arena = vmem_init(&arc_wired_map_arena_store,
					"wired_map",	/* name */
					0,		/* addr */
					0,		/* size */
					1,		/* quantum */
					NULL,		/* importfn */
					NULL,		/* releasefn */
					NULL,		/* source */
					0,		/* qcache_max */
					VM_NOSLEEP | VM_PRIVTAGS,
					IPL_NONE);
	KASSERT(arc_wired_map_arena != NULL);

	vmem_add_bts(arc_wired_map_arena, arc_wired_map_btag_store,
	    ARC_WIRED_MAP_BTCOUNT);
	error = vmem_add(arc_wired_map_arena, VM_MIN_WIRED_MAP_ADDRESS,
	    VM_MAX_WIRED_MAP_ADDRESS - VM_MIN_WIRED_MAP_ADDRESS,
	    VM_NOSLEEP);
	KASSERT(error == 0);
}

void
arc_wired_enter_page(vaddr_t va, paddr_t pa, vaddr_t pg_size)
{
	int error;

	KASSERT((va & (pg_size - 1)) == 0);

	if (va < VM_MIN_WIRED_MAP_ADDRESS ||
	    va + pg_size > VM_MAX_WIRED_MAP_ADDRESS) {
#ifdef DIAGNOSTIC
		printf("arc_wired_enter_page: invalid va range.\n");
#endif
		return;
	}

	error = vmem_xalloc_addr(arc_wired_map_arena, va, pg_size, VM_NOSLEEP);
	if (error) {
#ifdef DIAGNOSTIC
		printf("arc_wired_enter_page: cannot allocate region.\n");
#endif
		return;
	}

	mips3_wired_enter_page(va, pa, pg_size);
}

static bool
arc_wired_map_paddr_entry(paddr_t pa, vaddr_t *vap, vsize_t *sizep)
{
	vsize_t size;
	int n;
	struct wired_map_entry *entry;

	n = mips3_nwired_page;
	for (entry = mips3_wired_map; --n >= 0; entry++) {
		size = MIPS3_PG_SIZE_MASK_TO_SIZE(entry->pgmask);
		if (entry->pa0 != 0 &&
		    pa >= entry->pa0 && pa < entry->pa0 + size) {
			*vap = entry->va;
			*sizep = size;
			return true;
		}
		if (entry->pa1 != 0 &&
		    pa >= entry->pa1 && pa < entry->pa1 + size) {
			*vap = entry->va + size;
			*sizep = size;
			return true;
		}
	}
	return false;
}

/* XXX: Using tlbp makes this easier... */
static bool
arc_wired_map_vaddr_entry(vaddr_t va, paddr_t *pap, vsize_t *sizep)
{
	vsize_t size;
	int n;
	struct wired_map_entry *entry;

	n = mips3_nwired_page;
	for (entry = mips3_wired_map; --n >= 0; entry++) {
		size = MIPS3_PG_SIZE_MASK_TO_SIZE(entry->pgmask);
		if (va >= entry->va && va < entry->va + size * 2) {
			paddr_t pa = (va < entry->va + size)
			    ? entry->pa0 : entry->pa1;

			if (pa != 0) {
				*pap = pa;
				*sizep = size;
				return true;
			}
		}
	}
	return false;
}

vaddr_t
arc_contiguously_wired_mapped(paddr_t pa, vsize_t size)
{
	paddr_t p;
	vaddr_t rva, va;
	vsize_t vsize, offset;

	if (!arc_wired_map_paddr_entry(pa, &rva, &vsize))
		return 0;	/* not wired mapped */
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
			return 0; /* not contiguously wired mapped */
	}
	return rva + offset;
}

/* Allocate new wired entries */
vaddr_t
arc_map_wired(paddr_t pa, vsize_t size)
{
	vmem_addr_t va;
	vsize_t off;
	int error;

	/* XXX: may be already partially wired mapped */

	off = pa & MIPS3_WIRED_OFFMASK;
	pa &= ~(paddr_t)MIPS3_WIRED_OFFMASK;
	size += off;

	if ((size + MIPS3_WIRED_ENTRY_SIZE(MIPS3_WIRED_SIZE) - 1) /
	    MIPS3_WIRED_ENTRY_SIZE(MIPS3_WIRED_SIZE) >
	    MIPS3_NWIRED_ENTRY - mips3_nwired_page) {
#ifdef DIAGNOSTIC
		printf("arc_map_wired(0x%"PRIxPADDR", 0x%"PRIxVSIZE"): %d is not enough\n",
		    pa + off, size - off,
		    MIPS3_NWIRED_ENTRY - mips3_nwired_page);
#endif
		return 0; /* free wired TLB is not enough */
	}

	error = vmem_xalloc(arc_wired_map_arena, size,
			    MIPS3_WIRED_SIZE,		/* align */
			    0,				/* phase */
			    0,				/* nocross */
			    VMEM_ADDR_MIN,		/* minaddr */
			    VMEM_ADDR_MAX,		/* maxaddr */
			    VM_BESTFIT | VM_NOSLEEP,
			    &va);
	if (error) {
#ifdef DIAGNOSTIC
		printf("arc_map_wired: can't allocate region\n");
#endif
		return 0;
	}
	mips3_wired_enter_region(va, pa, MIPS3_WIRED_SIZE);

	return va + off;
}

bool
arc_wired_map_extract(vaddr_t va, paddr_t *pap)
{
	paddr_t pa;
	vsize_t size;

	if (arc_wired_map_vaddr_entry(va, &pa, &size)) {
		*pap = pa + (va & (size - 1));
		return true;
	} else {
		return false;
	}
}
