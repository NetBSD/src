/*	$NetBSD: wired_map_machdep.c,v 1.5.64.1 2009/08/26 03:46:38 matt Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: wired_map_machdep.c,v 1.5.64.1 2009/08/26 03:46:38 matt Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/malloc.h>
#include <sys/extent.h>

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

static struct extent *arc_wired_map_ex;
static long wired_map_ex_storage[EXTENT_FIXED_STORAGE_SIZE(8) / sizeof(long)];

void
arc_init_wired_map(void)
{

	mips3_nwired_page = 0;
	arc_wired_map_ex = extent_create("wired_map",
	    VM_MIN_WIRED_MAP_ADDRESS, VM_MAX_WIRED_MAP_ADDRESS, M_DEVBUF,
	    (void *)wired_map_ex_storage, sizeof(wired_map_ex_storage),
	    EX_NOWAIT);
	if (arc_wired_map_ex == NULL)
		panic("arc_init_wired_map: can't create extent");
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

	error = extent_alloc_region(arc_wired_map_ex, va, pg_size, EX_NOWAIT);
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
CTASSERT(sizeof(vaddr_t) == sizeof(u_long));
vaddr_t
arc_map_wired(paddr_t pa, vsize_t size)
{
	vaddr_t va;
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
		printf("arc_map_wired(0x%llx, 0x%lx): %d is not enough\n",
		    pa + off, size - off,
		    MIPS3_NWIRED_ENTRY - mips3_nwired_page);
#endif
		return 0; /* free wired TLB is not enough */
	}

	error = extent_alloc(arc_wired_map_ex, size, MIPS3_WIRED_SIZE,
	    0, EX_NOWAIT, (u_long *)&va);
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
