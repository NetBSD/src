/*	$NetBSD: gdt.c,v 1.25.6.1 2015/12/27 12:09:28 skrll Exp $	*/

/*-
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

/*
 * Modified to deal with variable-length entries for NetBSD/x86_64 by
 * fvdl@wasabisystems.com, may 2001
 * XXX this file should be shared with the i386 code, the difference
 * can be hidden in macros.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: gdt.c,v 1.25.6.1 2015/12/27 12:09:28 skrll Exp $");

#include "opt_multiprocessor.h"
#include "opt_xen.h"
#include "opt_user_ldt.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/mutex.h>
#include <sys/cpu.h>

#include <uvm/uvm.h>

#include <machine/gdt.h>

#ifdef XEN
#include <xen/hypervisor.h>
#endif

int gdt_size;		/* size of GDT in bytes */
int gdt_dyncount;	/* number of dyn. allocated GDT entries in use */
int gdt_dynavail;
int gdt_next;		/* next available slot for sweeping */
int gdt_free;		/* next free slot; terminated with GNULL_SEL */

void gdt_init(void);
void gdt_grow(void);
int gdt_get_slot(void);
void gdt_put_slot(int);

void
update_descriptor(void *tp, void *ep)
{
	uint64_t *table, *entry;

	table = tp;
	entry = ep;

#ifndef XEN
	*table = *entry;
#else
	paddr_t pa;

	if (!pmap_extract_ma(pmap_kernel(), (vaddr_t)table, &pa) ||
	    HYPERVISOR_update_descriptor(pa, *entry))
		panic("HYPERVISOR_update_descriptor failed\n");
#endif
}

void
set_sys_gdt(int slot, void *base, size_t limit,
	    int type, int dpl, int gran)
{
	union {
		struct sys_segment_descriptor sd;
		uint64_t bits[2];
	} d;
	CPU_INFO_ITERATOR cii;
	struct cpu_info *ci;
	int idx;

	set_sys_segment(&d.sd, base, limit, type, dpl, gran);
	idx = IDXSEL(GDYNSEL(slot, SEL_KPL));
	for (CPU_INFO_FOREACH(cii, ci)) {
		KASSERT(ci->ci_gdt != NULL);
		update_descriptor(&ci->ci_gdt[idx + 0], &d.bits[0]);
		update_descriptor(&ci->ci_gdt[idx + 1], &d.bits[1]);
	}
}

/*
 * Initialize the GDT.
 */
void
gdt_init(void)
{
	char *old_gdt;
	struct vm_page *pg;
	vaddr_t va;
	struct cpu_info *ci = &cpu_info_primary;

	gdt_size = MINGDTSIZ;
	gdt_dyncount = 0;
	gdt_next = 0;
	gdt_free = GNULL_SEL;
	gdt_dynavail =
	    (gdt_size - DYNSEL_START) / sizeof (struct sys_segment_descriptor);

	old_gdt = gdtstore;
	gdtstore = (char *)uvm_km_alloc(kernel_map, MAXGDTSIZ, 0,
	    UVM_KMF_VAONLY);
	for (va = (vaddr_t)gdtstore; va < (vaddr_t)gdtstore + MINGDTSIZ;
	    va += PAGE_SIZE) {
		pg = uvm_pagealloc(NULL, 0, NULL, UVM_PGA_ZERO);
		if (pg == NULL) {
			panic("gdt_init: no pages");
		}
		pmap_kenter_pa(va, VM_PAGE_TO_PHYS(pg),
		    VM_PROT_READ | VM_PROT_WRITE, 0);
	}
	pmap_update(pmap_kernel());
	memcpy(gdtstore, old_gdt, DYNSEL_START);
	ci->ci_gdt = (void *)gdtstore;
#ifndef XEN
	set_sys_segment(GDT_ADDR_SYS(gdtstore, GLDT_SEL), ldtstore,
	    LDT_SIZE - 1, SDT_SYSLDT, SEL_KPL, 0);
#endif
	gdt_init_cpu(ci);
}

/*
 * Allocate shadow GDT for a slave CPU.
 */
void
gdt_alloc_cpu(struct cpu_info *ci)
{
	int max_len = MAXGDTSIZ;
	int min_len = MINGDTSIZ;
	struct vm_page *pg;
	vaddr_t va;

	ci->ci_gdt = (union descriptor *)uvm_km_alloc(kernel_map, max_len,
	    0, UVM_KMF_VAONLY);
	for (va = (vaddr_t)ci->ci_gdt; va < (vaddr_t)ci->ci_gdt + min_len;
	    va += PAGE_SIZE) {
		while ((pg = uvm_pagealloc(NULL, 0, NULL, UVM_PGA_ZERO))
		    == NULL) {
			uvm_wait("gdt_alloc_cpu");
		}
		pmap_kenter_pa(va, VM_PAGE_TO_PHYS(pg),
		    VM_PROT_READ | VM_PROT_WRITE, 0);
	}
	pmap_update(pmap_kernel());
	memset(ci->ci_gdt, 0, min_len);
	memcpy(ci->ci_gdt, gdtstore, gdt_size);
}


/*
 * Load appropriate gdt descriptor; we better be running on *ci
 * (for the most part, this is how a CPU knows who it is).
 */
void
gdt_init_cpu(struct cpu_info *ci)
{
	struct region_descriptor region;

#ifndef XEN
	setregion(&region, ci->ci_gdt, (uint16_t)(MAXGDTSIZ - 1));
#else
	setregion(&region, ci->ci_gdt, (uint16_t)(gdt_size - 1));
#endif
	lgdt(&region);
}

#ifdef MULTIPROCESSOR
void
gdt_reload_cpu(struct cpu_info *ci)
{
	struct region_descriptor region;

#ifndef XEN
	setregion(&region, ci->ci_gdt, MAXGDTSIZ - 1);
#else
	setregion(&region, ci->ci_gdt, gdt_size - 1);
#endif
	lgdt(&region);
}
#endif


/*
 * Grow or shrink the GDT.
 */
void
gdt_grow(void)
{
	size_t old_len, new_len;
	struct vm_page *pg;
	vaddr_t va;

	old_len = gdt_size;
	gdt_size <<= 1;
	new_len = old_len << 1;
	gdt_dynavail =
	    (gdt_size - DYNSEL_START) / sizeof (struct sys_segment_descriptor);

	for (va = (vaddr_t)gdtstore + old_len; va < (vaddr_t)gdtstore + new_len;
	    va += PAGE_SIZE) {
		while ((pg = uvm_pagealloc(NULL, 0, NULL, UVM_PGA_ZERO)) ==
		       NULL) {
			uvm_wait("gdt_grow");
		}
		pmap_kenter_pa(va, VM_PAGE_TO_PHYS(pg),
		    VM_PROT_READ | VM_PROT_WRITE, 0);
	}
	pmap_update(pmap_kernel());
}

/*
 * Allocate a GDT slot as follows:
 * 1) If there are entries on the free list, use those.
 * 2) If there are fewer than gdt_dynavail entries in use, there are free slots
 *    near the end that we can sweep through.
 * 3) As a last resort, we increase the size of the GDT, and sweep through
 *    the new slots.
 */
int
gdt_get_slot(void)
{
	int slot;
	struct sys_segment_descriptor *gdt;

	gdt = (struct sys_segment_descriptor *)&gdtstore[DYNSEL_START];

	KASSERT(mutex_owned(&cpu_lock));

	if (gdt_free != GNULL_SEL) {
		slot = gdt_free;
		gdt_free = gdt[slot].sd_xx3;	/* XXXfvdl res. field abuse */
	} else {
#ifdef DIAGNOSTIC
		if (gdt_next != gdt_dyncount)
			panic("gdt_get_slot botch 1");
#endif
		if (gdt_next >= gdt_dynavail) {
#ifdef DIAGNOSTIC
			if (gdt_size >= MAXGDTSIZ)
				panic("gdt_get_slot botch 2");
#endif
			gdt_grow();
		}
		slot = gdt_next++;
	}

	gdt_dyncount++;
	return (slot);
}

/*
 * Deallocate a GDT slot, putting it on the free list.
 */
void
gdt_put_slot(int slot)
{
	struct sys_segment_descriptor *gdt;

	KASSERT(mutex_owned(&cpu_lock));

	gdt = (struct sys_segment_descriptor *)&gdtstore[DYNSEL_START];

	gdt_dyncount--;
	gdt[slot].sd_type = SDT_SYSNULL;
	gdt[slot].sd_xx3 = gdt_free;
	gdt_free = slot;
}

int
tss_alloc(struct x86_64_tss *tss)
{
#ifndef XEN
	int slot;

	mutex_enter(&cpu_lock);
	slot = gdt_get_slot();
	set_sys_gdt(slot, tss, sizeof (struct x86_64_tss) - 1,
	    SDT_SYS386TSS, SEL_KPL, 0);
	mutex_exit(&cpu_lock);
	return GDYNSEL(slot, SEL_KPL);
#else  /* XEN */
	/* TSS, what for? */
	return GSEL(GNULL_SEL, SEL_KPL);
#endif
}

void
tss_free(int sel)
{
#ifndef XEN
	mutex_enter(&cpu_lock);
	gdt_put_slot(IDXDYNSEL(sel));
	mutex_exit(&cpu_lock);
#else
	KASSERT(sel == GSEL(GNULL_SEL, SEL_KPL));
#endif
}

#ifdef USER_LDT
void
ldt_alloc(struct pmap *pmap, char *ldt, size_t len)
{
	int slot;
	struct sys_segment_descriptor *gdt;

	KASSERT(mutex_owned(&cpu_lock));

	gdt = (struct sys_segment_descriptor *)&gdtstore[DYNSEL_START];

	slot = gdt_get_slot();
	set_sys_gdt(slot, ldt, len - 1, SDT_SYSLDT, SEL_KPL, 0);
	pmap->pm_ldt_sel = GSEL(slot, SEL_KPL);
}

void
ldt_free(struct pmap *pmap)
{
	int slot;

	KASSERT(mutex_owned(&cpu_lock));

	slot = IDXDYNSEL(pmap->pm_ldt_sel);

	gdt_put_slot(slot);
}
#endif

#ifdef XEN
void
lgdt(struct region_descriptor *desc)
{
	paddr_t frames[16];
	int i;
	vaddr_t va;

	/*
	 * XXX: Xen even checks descriptors AFTER limit.
	 * Zero out last frame after limit if needed.
	 */
	va = desc->rd_base + desc->rd_limit + 1;
	__PRINTK(("memset 0x%lx -> 0x%lx\n", va, roundup(va, PAGE_SIZE)));
	memset((void *) va, 0, roundup(va, PAGE_SIZE) - va);
	for (i = 0; i < roundup(desc->rd_limit, PAGE_SIZE) >> PAGE_SHIFT; i++) {

		/*
		 * The lgdt instruction uses virtual addresses,
		 * do some translation for Xen.
		 * Mark pages R/O too, else Xen will refuse to use them.
		 */

		frames[i] = ((paddr_t) xpmap_ptetomach(
				(pt_entry_t *) (desc->rd_base + (i << PAGE_SHIFT))))
			>> PAGE_SHIFT;
		__PRINTK(("frames[%d] = 0x%lx (pa 0x%lx)\n", i, frames[i],
		    xpmap_mtop(frames[i] << PAGE_SHIFT)));
		pmap_pte_clearbits(kvtopte(desc->rd_base + (i << PAGE_SHIFT)),
		    PG_RW);
	}
	__PRINTK(("HYPERVISOR_set_gdt(%d)\n", (desc->rd_limit + 1) >> 3));

	if (HYPERVISOR_set_gdt(frames, (desc->rd_limit + 1) >> 3))
		panic("lgdt(): HYPERVISOR_set_gdt() failed");
	lgdt_finish();
}
#endif
