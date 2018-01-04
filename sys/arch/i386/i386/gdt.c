/*	$NetBSD: gdt.c,v 1.68 2018/01/04 20:38:31 maxv Exp $	*/

/*
 * Copyright (c) 1996, 1997, 2009 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by John T. Kohl, by Charles M. Hannum, and by Andrew Doran.
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: gdt.c,v 1.68 2018/01/04 20:38:31 maxv Exp $");

#include "opt_multiprocessor.h"
#include "opt_xen.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/mutex.h>
#include <sys/cpu.h>

#include <uvm/uvm.h>

#include <machine/gdt.h>

#define NSLOTS(sz)	\
	(((sz) - DYNSEL_START) / sizeof(union descriptor))
#define NDYNSLOTS	NSLOTS(MAXGDTSIZ)

typedef struct {
	bool busy[NDYNSLOTS];
	size_t nslots;
} gdt_bitmap_t;

/* size of GDT in bytes */
#ifdef XEN
const size_t gdt_size = FIRST_RESERVED_GDT_BYTE;
#else
const size_t gdt_size = MAXGDTSIZ;
#endif

/* bitmap of busy slots */
static gdt_bitmap_t gdt_bitmap;

#ifndef XEN
static int ldt_count;	/* number of LDTs */
static int ldt_max = 1000;/* max number of LDTs */
static void setgdt(int, const void *, size_t, int, int, int, int);
static int gdt_get_slot(void);
static void gdt_put_slot(int);
#endif
void gdt_init(void);

void
update_descriptor(union descriptor *table, union descriptor *entry)
{
#ifndef XEN
	*table = *entry;
#else
	paddr_t pa;
	pt_entry_t *ptp;

	ptp = kvtopte((vaddr_t)table);
	pa = (*ptp & PG_FRAME) | ((vaddr_t)table & ~PG_FRAME);
	if (HYPERVISOR_update_descriptor(pa, entry->raw[0], entry->raw[1]))
		panic("HYPERVISOR_update_descriptor failed\n");
#endif
}

#ifndef XEN
/*
 * Called on a newly-allocated GDT slot, so no race between CPUs.
 */
static void
setgdt(int slot, const void *base, size_t limit, int type, int dpl, int def32,
    int gran)
{
	struct segment_descriptor *sd;
	CPU_INFO_ITERATOR cii;
	struct cpu_info *ci;
	int idx;

	idx = IDXSEL(GDYNSEL(slot, SEL_KPL));
	sd = &gdtstore[idx].sd;
	setsegment(sd, base, limit, type, dpl, def32, gran);
	for (CPU_INFO_FOREACH(cii, ci)) {
		if (ci->ci_gdt != NULL)
			update_descriptor(&ci->ci_gdt[idx],
			    (union descriptor *)sd);
	}
}
#endif

/*
 * Initialize the GDT. We already have a gdtstore, which was temporarily used
 * by the bootstrap code. Now, we allocate a new gdtstore, and put it in cpu0.
 */
void
gdt_init(void)
{
	union descriptor *old_gdt;
	struct vm_page *pg;
	vaddr_t va;
	struct cpu_info *ci = &cpu_info_primary;

	/* Initialize the global values */
	memset(&gdt_bitmap.busy, 0, sizeof(gdt_bitmap.busy));
	gdt_bitmap.nslots = NSLOTS(gdt_size);

	old_gdt = gdtstore;

	/* Allocate gdt_size bytes of memory. */
	gdtstore = (union descriptor *)uvm_km_alloc(kernel_map, gdt_size, 0,
	    UVM_KMF_VAONLY);
	for (va = (vaddr_t)gdtstore; va < (vaddr_t)gdtstore + gdt_size;
	    va += PAGE_SIZE) {
		pg = uvm_pagealloc(NULL, 0, NULL, UVM_PGA_ZERO);
		if (pg == NULL) {
			panic("gdt_init: no pages");
		}
		pmap_kenter_pa(va, VM_PAGE_TO_PHYS(pg),
		    VM_PROT_READ | VM_PROT_WRITE, 0);
	}
	pmap_update(pmap_kernel());

	/* Copy the initial bootstrap GDT into the new area. */
	memcpy(gdtstore, old_gdt, NGDT * sizeof(gdtstore[0]));
	ci->ci_gdt = gdtstore;
	setsegment(&ci->ci_gdt[GCPU_SEL].sd, ci,
	    sizeof(struct cpu_info) - 1, SDT_MEMRWA, SEL_KPL, 1, 0);

	gdt_init_cpu(ci);
}

/*
 * Allocate shadow GDT for a secondary CPU. It contains the same values as the
 * GDT present in cpu0 (gdtstore).
 */
void
gdt_alloc_cpu(struct cpu_info *ci)
{
	struct vm_page *pg;
	vaddr_t va;

	ci->ci_gdt = (union descriptor *)uvm_km_alloc(kernel_map, gdt_size,
	    0, UVM_KMF_VAONLY);
	for (va = (vaddr_t)ci->ci_gdt; va < (vaddr_t)ci->ci_gdt + gdt_size;
	    va += PAGE_SIZE) {
		while ((pg = uvm_pagealloc(NULL, 0, NULL, UVM_PGA_ZERO))
		    == NULL) {
			uvm_wait("gdt_alloc_cpu");
		}
		pmap_kenter_pa(va, VM_PAGE_TO_PHYS(pg),
		    VM_PROT_READ | VM_PROT_WRITE, 0);
	}
	pmap_update(pmap_kernel());

	memcpy(ci->ci_gdt, gdtstore, gdt_size);

	setsegment(&ci->ci_gdt[GCPU_SEL].sd, ci,
	    sizeof(struct cpu_info) - 1, SDT_MEMRWA, SEL_KPL, 1, 0);
}

/*
 * Load appropriate GDT descriptor into the currently running CPU, which must
 * be ci.
 */
void
gdt_init_cpu(struct cpu_info *ci)
{
#ifndef XEN
	struct region_descriptor region;

	setregion(&region, ci->ci_gdt, gdt_size - 1);
	lgdt(&region);
#else
	size_t len = roundup(gdt_size, PAGE_SIZE);
	unsigned long frames[len >> PAGE_SHIFT];
	vaddr_t va;
	pt_entry_t *ptp;
	size_t f;

	for (va = (vaddr_t)ci->ci_gdt, f = 0;
	    va < (vaddr_t)ci->ci_gdt + gdt_size;
	    va += PAGE_SIZE, f++) {
		KASSERT(va >= VM_MIN_KERNEL_ADDRESS);
		ptp = kvtopte(va);
		frames[f] = *ptp >> PAGE_SHIFT;

		/* 
		 * Our own
		 * 	pmap_pte_clearbits(ptp, PG_RW)
		 * but without spl(), since %fs is not set up properly yet; ie
		 * curcpu() won't work at this point and spl() will break.
		 */
		if (HYPERVISOR_update_va_mapping((vaddr_t)va,
		    *ptp & ~PG_RW, UVMF_INVLPG) < 0) {
			panic("%s page RO update failed.\n", __func__);
		}
	}

	if (HYPERVISOR_set_gdt(frames, gdt_size / sizeof(gdtstore[0])))
		panic("HYPERVISOR_set_gdt failed!\n");
	lgdt_finish();
#endif
}

#ifndef XEN
static int
gdt_get_slot(void)
{
	size_t i;

	KASSERT(mutex_owned(&cpu_lock));

	for (i = 0; i < gdt_bitmap.nslots; i++) {
		if (!gdt_bitmap.busy[i]) {
			gdt_bitmap.busy[i] = true;
			return (int)i;
		}
	}
	panic("gdt_get_slot: out of memory");

	/* NOTREACHED */
	return 0;
}

static void
gdt_put_slot(int slot)
{
	KASSERT(mutex_owned(&cpu_lock));
	KASSERT(slot < gdt_bitmap.nslots);
	gdt_bitmap.busy[slot] = false;
}

int
tss_alloc(const struct i386tss *tss)
{
	int slot;

	mutex_enter(&cpu_lock);
	slot = gdt_get_slot();
	setgdt(slot, tss, sizeof(struct i386tss) + IOMAPSIZE - 1,
	    SDT_SYS386TSS, SEL_KPL, 0, 0);
	mutex_exit(&cpu_lock);

	return GDYNSEL(slot, SEL_KPL);
}

void
tss_free(int sel)
{

	mutex_enter(&cpu_lock);
	gdt_put_slot(IDXDYNSEL(sel));
	mutex_exit(&cpu_lock);
}

int
ldt_alloc(union descriptor *ldtp, size_t len)
{
	int slot;

	KASSERT(mutex_owned(&cpu_lock));

	if (ldt_count >= ldt_max) {
		return -1;
	}
	ldt_count++;

	slot = gdt_get_slot();
	setgdt(slot, ldtp, len - 1, SDT_SYSLDT, SEL_KPL, 0, 0);

	return GDYNSEL(slot, SEL_KPL);
}

void
ldt_free(int sel)
{
	int slot;

	KASSERT(mutex_owned(&cpu_lock));
	KASSERT(ldt_count > 0);

	slot = IDXDYNSEL(sel);
	gdt_put_slot(slot);
	ldt_count--;
}
#endif
