/*	$NetBSD: loadfile_machdep.c,v 1.2 2022/07/10 14:18:27 thorpej Exp $	*/

/*-
 * Copyright (c) 2021 The NetBSD Foundation, Inc.
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

#include "boot.h"

#include <sys/param.h>
#include <sys/boot_flag.h>
#include <sys/disklabel.h>

#include <lib/libsa/stand.h> 
#include <lib/libsa/loadfile.h>
#include <lib/libkern/libkern.h>
 
#include "openfirm.h" 

#ifdef KMAPPING_DEBUG
#define	DPRINTF		printf
#else
#define	DPRINTF		while (0) printf
#endif

static int
ofw_claimphys(paddr_t pa, vsize_t size)
{
	/* (2) phys, (2) size, (1) align -> 2 (phys) */
	uint32_t cells[2+2+1+2];
	uint32_t *p = cells;
	paddr_t result;

	if (ofw_address_cells == 2) {
		*p++ = ((uint64_t)pa) >> 32;
		*p++ = (uint32_t)pa;
	} else {
		*p++ = (uint32_t)pa;
	}

#if 0	/* No known Mac systems with 2. */
	if (ofw_size_cells == 2) {
		*p++ = ((uint64_t)size) >> 32;
		*p++ = (uint32_t)size;
	} else
#endif
		*p++ = (uint32_t)size;

	*p++ = 0;	/* align */

	if (OF_call_method("claim", ofw_memory_ihandle,
			   ofw_address_cells + ofw_size_cells + 1,
			   ofw_address_cells, (int *)cells) == -1) {
		return -1;
	}

	if (ofw_address_cells == 2) {
		uint64_t v;
		v = (uint64_t)(*p++) << 32;
		v |= *p++;
		result = (paddr_t)v;
	} else {
		result = *p++;
	}

	if (result != pa) {
		printf("!!! CLAIM PHYS 0x%lx RETURNED 0x%lx\n",
		    pa, result);
		return -1;
	}

	return 0;
}

static int
ofw_releasephys(paddr_t pa, vsize_t size)
{
	/* (2) phys, (2) size, -> nil */
	uint32_t cells[2+2];
	uint32_t *p = cells;

	if (ofw_address_cells == 2) {
		*p++ = ((uint64_t)pa) >> 32;
		*p++ = (uint32_t)pa;
	} else {
		*p++ = (uint32_t)pa;
	}

#if 0	/* No known Mac systems with 2. */
	if (ofw_size_cells == 2) {
		*p++ = ((uint64_t)size) >> 32;
		*p++ = (uint32_t)size;
	} else
#endif
		*p++ = (uint32_t)size;

	if (OF_call_method("release", ofw_memory_ihandle,
			   ofw_address_cells + ofw_size_cells,
			   0, (int *)cells) == -1) {
		return -1;
	}

	return 0;
}

static int
ofw_claimvirt(vaddr_t va, vsize_t size)
{
	/* (1) virt, (1) size, (1) align -> (1) virt */
	uint32_t cells[1+1+1+1];
	uint32_t *p = cells;
	vaddr_t result;

	*p++ = va;
	*p++ = size;
	*p++ = 0;		/* align */

	if (OF_call_method("claim", ofw_mmu_ihandle,
			   3, 1, (int *)cells) == -1) {
		return -1;
	}

	result = *p++;

	if (result != va) {
		printf("!!! CLAIM VIRT 0x%lx RETURNED 0x%lx\n",
		    va, result);
		return -1;
	}

	return 0;
}

static int
ofw_releasevirt(vaddr_t va, vsize_t size)
{
	/* (1) virt, (1) size -> nil */
	uint32_t cells[1+1];
	uint32_t *p = cells;

	*p++ = va;
	*p++ = size;

	if (OF_call_method("release", ofw_mmu_ihandle,
			   2, 0, (int *)cells) == -1) {
		return -1;
	}

	return 0;
}

static int
ofw_map(vaddr_t va, paddr_t pa, vsize_t size)
{
	/* (2) phys, (1) virt, (1) size, (1) mode -> nil */
	uint32_t cells[2+1+1+1];
	uint32_t *p = cells;

	if (ofw_address_cells == 2) {
		*p++ = ((uint64_t)pa) >> 32;
		*p++ = (uint32_t)pa;
	} else {
		*p++ = (uint32_t)pa;
	}

	*p++ = va;
	*p++ = size;
	*p++ = 0x00000010 /* PTE_SO | PTE_M */;	/* XXX magic numbers */

	if (OF_call_method("map", ofw_mmu_ihandle,
			   ofw_address_cells + 3, 0, (int *)cells) == -1) {
		return -1;
	}

	return 0;
}

static int
ofw_create_mapping(vaddr_t va, paddr_t pa, vsize_t size)
{
	if (ofw_claimphys(pa, size) == -1) {
		printf("!!! FAILED TO CLAIM PHYS 0x%lx size 0x%lx\n",
		    pa, size);
		return -1;
	}

	/*
	 * If we're running in real-mode, the claimphys is enough.
	 */
	if (ofw_real_mode) {
		return 0;
	}

	if (ofw_claimvirt(va, size) == -1) {
		printf("!!! FAILED TO CLAIM VIRT 0x%lx size 0x%lx\n",
		    va, size);
		ofw_releasephys(pa, size);
		return -1;
	}
	if (ofw_map(va, pa, size) == -1) {
		printf("!!! FAILED TO MAP PHYS 0x%lx @ 0x%lx size 0x%lx\n",
		    pa, va, size);
		ofw_releasevirt(va, size);
		ofw_releasephys(pa, size);
		return -1;
	}

	return 0;
}

/*
 * This structure describes a physically contiguous mapping
 * for the loaded kernel.  We assume VA==PA, which would be
 * equivalent to running in real-mode.
 */
#define	MAX_KMAPPINGS	64
struct kmapping {
	vaddr_t		vstart;
	vaddr_t		vend;
} kmappings[MAX_KMAPPINGS];

#define	TRUNC_PAGE(x)	((x) & ~((unsigned long)NBPG - 1))
#define	ROUND_PAGE(x)	TRUNC_PAGE((x) + (NBPG - 1))

/*
 * Enter a mapping for the loaded kernel.  If the start is within an
 * already mapped region, or if it starts immediately following a
 * previous region, we extend it.
 */
static int
kmapping_enter(vaddr_t va, vsize_t size)
{
	struct kmapping *km, *km_prev, *km_next, *km_last;
	vaddr_t va_end;
	int i;

	km_last = &kmappings[MAX_KMAPPINGS - 1];

	/* Round the region to a page. */
	va_end = ROUND_PAGE(va + size);

	/* Truncate the region to the nearest page boundary. */
	va = TRUNC_PAGE(va);

	/* Get the rounded size. */
	size = va_end - va;

	DPRINTF("kmapping_enter: va=0x%lx size=0x%lx\n",
	    va, size);

	/* Check to see if there's an overlapping entry in the table. */
	for (i = 0, km_prev = NULL; i < MAX_KMAPPINGS; i++, km_prev = km) {
		km = &kmappings[i];
		km_next = (km == km_last) ? NULL : (km + 1);

		if (km->vstart == km->vend) {
			/*
			 * Found an empty slot.  We are guaranteed there
			 * is not slot after this to merge with.
			 */
			DPRINTF("!!! entering into empty slot (%d)\n", i);
			goto enter_new;
		}

		if (va >= km->vstart) {
			if (va_end <= km->vend) {
				/* This region is already fully mapped. */
				DPRINTF("!!! matches existing region "
				       "va=0x%lx-0x%lx\n",
				       km->vstart, km->vend);
				return 0;
			}
			if (va > km->vend) {
				/* Requested region falls after this one. */
				continue;
			}

			/*
			 * We will extend this region.  Adjust the
			 * start and size.
			 */
			va = km->vend;
			size = va_end - va;

			/*
			 * If there is a slot after this one and it's
			 * not empty, see if these two can be merged.
			 */
			if (km_next != NULL &&
			    km_next->vstart != km_next->vend &&
			    va_end >= km_next->vstart) {
				if (va_end > km_next->vend) {
					/* Crazy! */
					printf("!!! GOBBLED UP KM_NEXT!\n");
					return 1;
				}
				va_end = km_next->vstart;
				size = va_end - va;

				DPRINTF("!!! merging va=0x%lx-0x%lx and "
				       "va=0x%lx-0x%lx\n",
				       km->vstart, km->vend,
				       km_next->vstart, km_next->vend);
				goto merge_two;
			}

			DPRINTF("!!! extending existing region "
			       "va=0x%lx-0x%lx to "
			       "va=0x%lx-0x%lx\n",
			       km->vstart, km->vend,
			       km->vstart, va_end);
			goto extend_existing;
		}

		/*
		 * We know that the new region starts before the current
		 * one.  Check to see of the end overlaps.
		 */
		if (va_end >= km->vstart) {
			va_end = km->vstart;
			size = va_end - va;

			/*
			 * No need to check for a merge here; we would
			 * have caught it above.
			 */
			goto prepend_existing;
		}

		/*
		 * Need to the new region in front of the current one.
		 * Make sure there's room.
		 */
		if (km_next == NULL || km_last->vstart != km_last->vend) {
			printf("!!! NO ROOM TO INSERT MAPPING\n");
			return 1;
		}

		if (km_prev == NULL) {
			DPRINTF("!!! entering before 0x%lx-0x%lx\n",
			    km->vstart, km->vend);
		} else {
			DPRINTF("!!! entering between 0x%lx-0x%lx and "
			       "0x%lx-0x%lx\n",
			       km_prev->vstart, km_prev->vend,
			       km->vstart, km->vend);
		}

		if (ofw_create_mapping(va, va, size) == -1) {
			return 1;
		}
		size = (uintptr_t)(&kmappings[MAX_KMAPPINGS]) -
		    (uintptr_t)(km_next + 1);
		memmove(km_next, km, size);
		km->vstart = va;
		km->vend = va_end;
		return 0;
	}

 extend_existing:
	if (ofw_create_mapping(va, va, size) == -1) {
		return 1;
	}
	km->vend = va_end;
	return 0;

 prepend_existing:
	if (ofw_create_mapping(va, va, size) == -1) {
		return 1;
	}
	km->vstart = va;
	return 0;

 enter_new:
	if (ofw_create_mapping(va, va, size) == -1) {
		return 1;
	}
	km->vstart = va;
	km->vend = va_end;
	return 0;

 merge_two:
	if (ofw_create_mapping(va, va, size) == -1) {
		return 1;
	}
	km->vend = km_next->vend;
	size =
	    (uintptr_t)(&kmappings[MAX_KMAPPINGS]) - (uintptr_t)(km_next + 1);
	if (size != 0) {
		memmove(km_next, km_next + 1, size);
	}
	km_last->vstart = km_last->vend = 0;
	return 0;
}

/*
 * loadfile() hooks.
 */
ssize_t
macppc_read(int f, void *addr, size_t size)
{
	if (kmapping_enter((vaddr_t)addr, size)) {
		return -1;
	}
	return read(f, addr, size);
}

void *
macppc_memcpy(void *dst, const void *src, size_t size)
{
	if (kmapping_enter((vaddr_t)dst, size)) {
		panic("macppc_memcpy: kmapping_enter failed");
	}
	return memcpy(dst, src, size);
}

void *
macppc_memset(void *dst, int c, size_t size)
{
	if (kmapping_enter((vaddr_t)dst, size)) {
		panic("macppc_memset: kmapping_enter failed");
	}
	return memset(dst, c, size);
}
