/*	$NetBSD: gdt.c,v 1.22.2.8 2002/02/24 01:58:57 sommerfeld Exp $	*/

/*-
 * Copyright (c) 1996, 1997 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by John T. Kohl and Charles M. Hannum.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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
__KERNEL_RCSID(0, "$NetBSD: gdt.c,v 1.22.2.8 2002/02/24 01:58:57 sommerfeld Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/lock.h>
#include <sys/user.h>

#include <uvm/uvm.h>

#include <machine/gdt.h>

int gdt_size;		/* total number of GDT entries */
int gdt_count;		/* number of GDT entries in use */
int gdt_next;		/* next available slot for sweeping */
int gdt_free;		/* next free slot; terminated with GNULL_SEL */

struct lock gdt_lock_store;

static __inline void gdt_lock __P((void));
static __inline void gdt_unlock __P((void));
#if 0
void gdt_compact __P((void));
#endif
void gdt_init __P((void));
void gdt_grow __P((void));
#if 0
void gdt_shrink __P((void));
#endif
int gdt_get_slot __P((void));
void gdt_put_slot __P((int));

/*
 * Lock and unlock the GDT, to avoid races in case gdt_{ge,pu}t_slot() sleep
 * waiting for memory.
 *
 * Note that the locking done here is not sufficient for multiprocessor
 * systems.  A freshly allocated slot will still be of type SDT_SYSNULL for
 * some time after the GDT is unlocked, so gdt_compact() could attempt to
 * reclaim it.
 */
static __inline void
gdt_lock()
{

	(void) lockmgr(&gdt_lock_store, LK_EXCLUSIVE, NULL);
}

static __inline void
gdt_unlock()
{

	(void) lockmgr(&gdt_lock_store, LK_RELEASE, NULL);
}

void
setgdt(int sel, void *base, size_t limit,
    int type, int dpl, int def32, int gran)
{
	struct segment_descriptor *sd = &gdt[sel].sd;
	CPU_INFO_ITERATOR cii;
	struct cpu_info *ci;

	setsegment(sd, base, limit, type, dpl, def32, gran);
	for (CPU_INFO_FOREACH(cii, ci))
		ci->ci_gdt[sel].sd = *sd;
}

#if 0
/*
 * Compact the GDT as follows:
 * 0) We partition the GDT into two areas, one of the slots before gdt_count,
 *    and one of the slots after.  After compaction, the former part should be
 *    completely filled, and the latter part should be completely empty.
 * 1) Step through the process list, looking for TSS and LDT descriptors in
 *    the second section, and swap them with empty slots in the first section.
 * 2) Arrange for new allocations to sweep through the empty section.  Since
 *    we're sweeping through all of the empty entries, and we'll create a free
 *    list as things are deallocated, we do not need to create a new free list
 *    here.
 */
void
gdt_compact()
{
	struct proc *p;
	pmap_t pmap;
	int slot = NGDT, oslot;

	proclist_lock_read();
	for (p = allproc.lh_first; p != 0; p = p->p_list.le_next) {
		pmap = p->p_vmspace->vm_map.pmap;
		oslot = IDXSEL(p->p_md.md_tss_sel);
		if (oslot >= gdt_count) {
			while (gdt[slot].sd.sd_type != SDT_SYSNULL) {
				if (++slot >= gdt_count)
					panic("gdt_compact botch 1");
			}
			gdt[slot] = gdt[oslot];
			gdt[oslot].gd.gd_type = SDT_SYSNULL;
			p->p_md.md_tss_sel = GSEL(slot, SEL_KPL);
		}
		simple_lock(&pmap->pm_lock);
		oslot = IDXSEL(pmap->pm_ldt_sel);
		if (oslot >= gdt_count) {
			while (gdt[slot].sd.sd_type != SDT_SYSNULL) {
				if (++slot >= gdt_count)
					panic("gdt_compact botch 2");
			}
			gdt[slot] = gdt[oslot];
			gdt[oslot].gd.gd_type = SDT_SYSNULL;
			pmap->pm_ldt_sel = GSEL(slot, SEL_KPL);
			/*
			 * XXXSMP: if the pmap is in use on other
			 * processors, they need to reload their
			 * LDT!
			 */
		}
		simple_unlock(&pmap->pm_lock);
	}
	for (; slot < gdt_count; slot++)
		if (gdt[slot].gd.gd_type == SDT_SYSNULL)
			panic("gdt_compact botch 3");
	for (slot = gdt_count; slot < gdt_size; slot++)
		if (gdt[slot].gd.gd_type != SDT_SYSNULL)
			panic("gdt_compact botch 4");
	gdt_next = gdt_count;
	gdt_free = GNULL_SEL;
	proclist_unlock_read();
}
#endif

/*
 * Initialize the GDT subsystem.  Called from autoconf() relatively
 * late in boot.
 */
void
gdt_init()
{

	lockinit(&gdt_lock_store, PZERO, "gdtlck", 0, 0);
}

/*
 * Initialize the boot cpu's GDT; called very early in boot.
 */
void
gdt_init_cpu0(struct cpu_info *ci)
{
	size_t max_len, min_len;
	union descriptor *old_gdt;
	struct vm_page *pg;
	vaddr_t va;

	max_len = MAXGDTSIZ * sizeof(gdt[0]);
	min_len = MINGDTSIZ * sizeof(gdt[0]);

	gdt_size = MINGDTSIZ;
	gdt_count = NGDT;
	gdt_next = NGDT;
	gdt_free = GNULL_SEL;

	old_gdt = gdt;
	gdt = (union descriptor *)uvm_km_valloc(kernel_map, max_len);
	for (va = (vaddr_t)gdt; va < (vaddr_t)gdt + min_len; va += PAGE_SIZE) {
		pg = uvm_pagealloc(NULL, 0, NULL, UVM_PGA_ZERO);
		if (pg == NULL) {
			panic("gdt_init: no pages");
		}
		pmap_kenter_pa(va, VM_PAGE_TO_PHYS(pg),
		    VM_PROT_READ | VM_PROT_WRITE);
	}
	memcpy(gdt, old_gdt, NGDT * sizeof(gdt[0]));
	ci->ci_gdt = gdt;
	setsegment(&ci->ci_gdt[GCPU_SEL].sd, ci, sizeof(struct cpu_info)-1,
	    SDT_MEMRWA, SEL_KPL, 1, 1);

	gdt_init_cpu(ci);
}

/*
 * Allocate shadow GDT for a slave cpu.
 */
void
gdt_alloc_cpu(struct cpu_info *ci)
{
	int max_len = MAXGDTSIZ * sizeof(gdt[0]);
	int min_len = MINGDTSIZ * sizeof(gdt[0]);

	ci->ci_gdt = (union descriptor *)uvm_km_valloc(kernel_map, max_len);
	uvm_map_pageable(kernel_map, (vaddr_t)ci->ci_gdt,
	    (vaddr_t)ci->ci_gdt + min_len, FALSE, FALSE);
	memset(ci->ci_gdt, 0, min_len);
	memcpy(ci->ci_gdt, gdt, NGDT * sizeof(gdt[0]));
	setsegment(&ci->ci_gdt[GCPU_SEL].sd, ci, sizeof(struct cpu_info)-1,
	    SDT_MEMRWA, SEL_KPL, 1, 1);
}


/*
 * Load appropriate gdt descriptor; we better be running on *ci
 * (for the most part, this is how a cpu knows who it is).
 */
void
gdt_init_cpu(struct cpu_info *ci)
{
	struct region_descriptor region;
	size_t max_len;

	max_len = MAXGDTSIZ * sizeof(gdt[0]);
	setregion(&region, ci->ci_gdt, max_len - 1);
	lgdt(&region);
}

#ifdef MULTIPROCESSOR

void
gdt_reload_cpu(struct cpu_info *ci)
{
	struct region_descriptor region;
	size_t max_len;

	max_len = MAXGDTSIZ * sizeof(gdt[0]);
	setregion(&region, ci->ci_gdt, max_len - 1);
	lgdt(&region);
}
#endif


/*
 * Grow or shrink the GDT.
 */
void
gdt_grow()
{
	size_t old_len, new_len;
	CPU_INFO_ITERATOR cii;
	struct cpu_info *ci;
	struct vm_page *pg;
	vaddr_t va;

	old_len = gdt_size * sizeof(gdt[0]);
	gdt_size <<= 1;
	new_len = old_len << 1;

	for (CPU_INFO_FOREACH(cii, ci)) {
		for (va = (vaddr_t)(ci->ci_gdt) + old_len;
		     va < (vaddr_t)(ci->ci_gdt) + new_len;
		     va += PAGE_SIZE) {
			while ((pg = uvm_pagealloc(NULL, 0, NULL, UVM_PGA_ZERO)) ==
			    NULL) {
				uvm_wait("gdt_grow");
			}
			pmap_kenter_pa(va, VM_PAGE_TO_PHYS(pg),
			    VM_PROT_READ | VM_PROT_WRITE);
		}
	}
}

#if 0
void
gdt_shrink()
{
	size_t old_len, new_len;
	struct vm_page *pg;
	paddr_t pa;
	vaddr_t va;

	old_len = gdt_size * sizeof(gdt[0]);
	gdt_size >>= 1;
	new_len = old_len >> 1;

	for (va = (vaddr_t)gdt + new_len; va < (vaddr_t)gdt + old_len;
	    va += PAGE_SIZE) {
		if (!pmap_extract(pmap_kernel(), va, &pa)) {
			panic("gdt_shrink botch");
		}
		pg = PHYS_TO_VM_PAGE(pa);
		pmap_kremove(va, PAGE_SIZE);
		uvm_pagefree(pg);
	}
}
#endif
/*
 * Allocate a GDT slot as follows:
 * 1) If there are entries on the free list, use those.
 * 2) If there are fewer than gdt_size entries in use, there are free slots
 *    near the end that we can sweep through.
 * 3) As a last resort, we increase the size of the GDT, and sweep through
 *    the new slots.
 */
int
gdt_get_slot()
{
	int slot;

	gdt_lock();

	if (gdt_free != GNULL_SEL) {
		slot = gdt_free;
		gdt_free = gdt[slot].gd.gd_selector;
	} else {
		if (gdt_next != gdt_count)
			panic("gdt_get_slot botch 1");
		if (gdt_next >= gdt_size) {
			if (gdt_size >= MAXGDTSIZ)
				panic("gdt_get_slot botch 2");
			gdt_grow();
		}
		slot = gdt_next++;
	}

	gdt_count++;
	gdt_unlock();
	return (slot);
}

/*
 * Deallocate a GDT slot, putting it on the free list.
 */
void
gdt_put_slot(int slot)
{

	gdt_lock();
	gdt_count--;

	gdt[slot].gd.gd_type = SDT_SYSNULL;
#if 0
	/*
	 * shrink the GDT if we're using less than 1/4 of it.
	 * Shrinking at that point means we'll still have room for
	 * almost 2x as many processes as are now running without
	 * having to grow the GDT.
	 */
	if (gdt_size > MINGDTSIZ && gdt_count <= gdt_size / 4) {
		gdt_compact();
		gdt_shrink();
	} else {
#endif
		gdt[slot].gd.gd_selector = gdt_free;
		gdt_free = slot;
#if 0
	}
#endif

	gdt_unlock();
}

int
tss_alloc(struct pcb *pcb)
{
	int slot;

	slot = gdt_get_slot();
	setgdt(slot, &pcb->pcb_tss, sizeof(struct pcb) - 1,
	    SDT_SYS386TSS, SEL_KPL, 0, 0);
	return GSEL(slot, SEL_KPL);
}

void
tss_free(int sel)
{

	gdt_put_slot(IDXSEL(sel));
}

void
ldt_alloc(struct pmap *pmap, union descriptor *ldt, size_t len)
{
	int slot;

	slot = gdt_get_slot();
	setgdt(slot, ldt, len - 1, SDT_SYSLDT, SEL_KPL, 0, 0);
	simple_lock(&pmap->pm_lock);
	pmap->pm_ldt_sel = GSEL(slot, SEL_KPL);
	simple_unlock(&pmap->pm_lock);
}

void
ldt_free(struct pmap *pmap)
{
	int slot;

	simple_lock(&pmap->pm_lock);
	slot = IDXSEL(pmap->pm_ldt_sel);
	simple_unlock(&pmap->pm_lock);

	gdt_put_slot(slot);
}
