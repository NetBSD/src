/*	$NetBSD: gdt.c,v 1.8.26.1 2007/05/07 10:55:06 yamt Exp $	*/
/*	NetBSD: gdt.c,v 1.32 2004/02/13 11:36:13 wiz Exp 	*/

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
__KERNEL_RCSID(0, "$NetBSD: gdt.c,v 1.8.26.1 2007/05/07 10:55:06 yamt Exp $");

#include "opt_multiprocessor.h"
#include "opt_xen.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/lock.h>
#include <sys/user.h>

#include <uvm/uvm.h>

#include <machine/gdt.h>

int gdt_size[2];	/* total number of GDT entries */
int gdt_count[2];	/* number of GDT entries in use */
int gdt_next[2];	/* next available slot for sweeping */
int gdt_free[2];	/* next free slot; terminated with GNULL_SEL */

struct lock gdt_lock_store;

static inline void gdt_lock(void);
static inline void gdt_unlock(void);
void gdt_init(void);
void gdt_grow(int);
int gdt_get_slot(void);
int gdt_get_slot1(int);
void gdt_put_slot(int);
void gdt_put_slot1(int, int);

/*
 * Lock and unlock the GDT, to avoid races in case gdt_{ge,pu}t_slot() sleep
 * waiting for memory.
 *
 * Note that the locking done here is not sufficient for multiprocessor
 * systems.  A freshly allocated slot will still be of type SDT_SYSNULL for
 * some time after the GDT is unlocked, so gdt_compact() could attempt to
 * reclaim it.
 */
static inline void
gdt_lock()
{

	(void) lockmgr(&gdt_lock_store, LK_EXCLUSIVE, NULL);
}

static inline void
gdt_unlock()
{

	(void) lockmgr(&gdt_lock_store, LK_RELEASE, NULL);
}

void
setgdt(int sel, void *base, size_t limit,
    int type, int dpl, int def32, int gran)
{
	struct segment_descriptor sd;
	CPU_INFO_ITERATOR cii;
	struct cpu_info *ci;

	if (type == SDT_SYS386TSS) {
		/* printk("XXX TSS descriptor not supported in GDT\n"); */
		return;
	}

	setsegment(&sd, base, limit, type, dpl, def32, gran);
	for (CPU_INFO_FOREACH(cii, ci)) {
		if (ci->ci_gdt != NULL) {
#ifndef XEN
			ci->ci_gdt[sel].sd = sd;
#else
			xen_update_descriptor(&ci->ci_gdt[sel],
			    (union descriptor *)&sd);
#endif
		}
	}
}

/*
 * Initialize the GDT subsystem.  Called from autoconf().
 */
void
gdt_init()
{
	size_t max_len, min_len;
	union descriptor *old_gdt;
	struct vm_page *pg;
	vaddr_t va;
	struct cpu_info *ci = &cpu_info_primary;

	lockinit(&gdt_lock_store, PZERO, "gdtlck", 0, 0);

	max_len = MAXGDTSIZ * sizeof(gdt[0]);
	min_len = MINGDTSIZ * sizeof(gdt[0]);

	gdt_size[0] = MINGDTSIZ;
	gdt_count[0] = NGDT;
	gdt_next[0] = NGDT;
	gdt_free[0] = GNULL_SEL;

	gdt_size[1] = 0;
	gdt_count[1] = MAXGDTSIZ;
	gdt_next[1] = MAXGDTSIZ;
	gdt_free[1] = GNULL_SEL;

	old_gdt = gdt;
	gdt = (union descriptor *)uvm_km_alloc(kernel_map, max_len + max_len, 0,
	    UVM_KMF_VAONLY);
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
 * Allocate shadow GDT for a slave CPU.
 */
void
gdt_alloc_cpu(struct cpu_info *ci)
{
	int max_len = MAXGDTSIZ * sizeof(gdt[0]);
	int min_len = MINGDTSIZ * sizeof(gdt[0]);
	struct vm_page *pg;
	vaddr_t va;

	ci->ci_gdt = (union descriptor *)uvm_km_alloc(kernel_map, max_len, 0,
	    UVM_KMF_VAONLY);
	for (va = (vaddr_t)ci->ci_gdt; va < (vaddr_t)ci->ci_gdt + min_len;
	    va += PAGE_SIZE) {
		while ((pg = uvm_pagealloc(NULL, 0, NULL, UVM_PGA_ZERO))
		    == NULL) {
			uvm_wait("gdt_alloc_cpu");
		}
		pmap_kenter_pa(va, VM_PAGE_TO_PHYS(pg),
		    VM_PROT_READ | VM_PROT_WRITE);
	}
	memset(ci->ci_gdt, 0, min_len);
	memcpy(ci->ci_gdt, gdt, gdt_count[0] * sizeof(gdt[0]));
	setsegment(&ci->ci_gdt[GCPU_SEL].sd, ci, sizeof(struct cpu_info)-1,
	    SDT_MEMRWA, SEL_KPL, 1, 1);
}


/*
 * Load appropriate gdt descriptor; we better be running on *ci
 * (for the most part, this is how a CPU knows who it is).
 */
void
gdt_init_cpu(struct cpu_info *ci)
{
#ifndef XEN
	struct region_descriptor region;
	size_t max_len;

	max_len = MAXGDTSIZ * sizeof(gdt[0]);
	setregion(&region, ci->ci_gdt, max_len - 1);
	lgdt(&region);
#else
	size_t len = gdt_size[0] * sizeof(gdt[0]);
	unsigned long frames[len >> PAGE_SHIFT];
	vaddr_t va;
	pt_entry_t *ptp;
	pt_entry_t *maptp;
	int f;

	for (va = (vaddr_t)ci->ci_gdt, f = 0;
	     va < (vaddr_t)ci->ci_gdt + len;
	     va += PAGE_SIZE, f++) {
		KASSERT(va >= VM_MIN_KERNEL_ADDRESS);
		ptp = kvtopte(va);
		frames[f] = *ptp >> PAGE_SHIFT;
		maptp = (pt_entry_t *)vtomach((vaddr_t)ptp);
		PTE_CLEARBITS(ptp, maptp, PG_RW);
	}
	PTE_UPDATES_FLUSH();
	/* printk("loading gdt %x, %d entries, %d pages", */
	    /* frames[0] << PAGE_SHIFT, gdt_size[0], len >> PAGE_SHIFT); */
	if (HYPERVISOR_set_gdt(frames, gdt_size[0]))
		panic("HYPERVISOR_set_gdt failed!\n");
	lgdt_finish();
#endif
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
 * Grow the GDT.
 */
void
gdt_grow(int which)
{
	size_t old_len, new_len, max_len;
	CPU_INFO_ITERATOR cii;
	struct cpu_info *ci;
	struct vm_page *pg;
	vaddr_t va;

	old_len = gdt_size[which] * sizeof(gdt[0]);
	gdt_size[which] <<= 1;
	new_len = old_len << 1;

	if (which != 0) {
		max_len = MAXGDTSIZ * sizeof(gdt[0]);
		if (old_len == 0) {
			gdt_size[which] = MINGDTSIZ;
			new_len = gdt_size[which] * sizeof(gdt[0]);
		}
		for (va = (vaddr_t)(cpu_info_primary.ci_gdt) + old_len + max_len;
		     va < (vaddr_t)(cpu_info_primary.ci_gdt) + new_len + max_len;
		     va += PAGE_SIZE) {
			while ((pg = uvm_pagealloc(NULL, 0, NULL, UVM_PGA_ZERO)) ==
			    NULL) {
				uvm_wait("gdt_grow");
			}
			pmap_kenter_pa(va, VM_PAGE_TO_PHYS(pg),
			    VM_PROT_READ | VM_PROT_WRITE);
		}
		return;
	}

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
	return gdt_get_slot1(0);
}

int
gdt_get_slot1(int which)
{
	size_t offset;
	int slot;

	gdt_lock();

	if (gdt_free[which] != GNULL_SEL) {
		slot = gdt_free[which];
		gdt_free[which] = gdt[slot].gd.gd_selector;
	} else {
		offset = which * MAXGDTSIZ * sizeof(gdt[0]);
		if (gdt_next[which] != gdt_count[which] + offset)
			panic("gdt_get_slot botch 1");
		if (gdt_next[which] - offset >= gdt_size[which]) {
			if (gdt_size[which] >= MAXGDTSIZ)
				panic("gdt_get_slot botch 2");
			gdt_grow(which);
		}
		slot = gdt_next[which]++;
	}

	gdt_count[which]++;
	gdt_unlock();
	return (slot);
}

/*
 * Deallocate a GDT slot, putting it on the free list.
 */
void
gdt_put_slot(int slot)
{
	gdt_put_slot1(slot, 0);
}

void
gdt_put_slot1(int slot, int which)
{
	union descriptor d;
	d.raw[0] = 0;
	d.raw[1] = 0;

	gdt_lock();
	gdt_count[which]--;

	d.gd.gd_type = SDT_SYSNULL;
	d.gd.gd_selector = gdt_free[which];
	xen_update_descriptor(&gdt[slot], &d);

	gdt_free[which] = slot;

	gdt_unlock();
}

int
tss_alloc(struct pcb *pcb)
{
#ifndef XEN
	int slot;

	slot = gdt_get_slot();
	setgdt(slot, &pcb->pcb_tss, sizeof(struct pcb) - 1,
	    SDT_SYS386TSS, SEL_KPL, 0, 0);
	return GSEL(slot, SEL_KPL);
#else

	return GSEL(GNULL_SEL, SEL_KPL);
#endif
}

void
tss_free(int sel)
{

#ifndef XEN
	gdt_put_slot(IDXSEL(sel));
#else
	KASSERT(sel == GSEL(GNULL_SEL, SEL_KPL));
#endif
}

/*
 * Caller must have pmap locked for both of these functions.
 */
int
ldt_alloc(union descriptor *ldtp, size_t len)
{
	int slot;

	slot = gdt_get_slot1(1);
#ifndef XEN
	setgdt(slot, ldtp, len - 1, SDT_SYSLDT, SEL_KPL, 0, 0);
#else
	cpu_info_primary.ci_gdt[slot].ld.ld_base = (uint32_t)ldtp;
	cpu_info_primary.ci_gdt[slot].ld.ld_entries =
		len / sizeof(union descriptor);
#endif
	return GSEL(slot, SEL_KPL);
}

void
ldt_free(int sel)
{
	int slot;

	slot = IDXSEL(sel);

	gdt_put_slot1(slot, 1);
}
