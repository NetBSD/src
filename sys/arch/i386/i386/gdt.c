/*	$NetBSD: gdt.c,v 1.22.2.4 2001/01/07 22:59:23 sommerfeld Exp $	*/

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

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/lock.h>
#include <sys/user.h>

#include <uvm/uvm_extern.h>

#include <machine/gdt.h>

#define	MINGDTSIZ	512
#define	MAXGDTSIZ	8192

int gdt_size;		/* total number of GDT entries */
int gdt_count;		/* number of GDT entries in use */
int gdt_next;		/* next available slot for sweeping */
int gdt_free;		/* next free slot; terminated with GNULL_SEL */

struct lock gdt_lock_store;

static __inline void gdt_lock __P((void));
static __inline void gdt_unlock __P((void));
void gdt_compact __P((void));
void gdt_init __P((void));
void gdt_grow __P((void));
void gdt_shrink __P((void));
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
			 * processors, they need to reload thier
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

/*
 * Initialize the GDT.
 */
void
gdt_init()
{
	size_t max_len, min_len;
	union descriptor *old_gdt;

	lockinit(&gdt_lock_store, PZERO, "gdtlck", 0, 0);

	max_len = MAXGDTSIZ * sizeof(gdt[0]);
	min_len = MINGDTSIZ * sizeof(gdt[0]);

	gdt_size = MINGDTSIZ;
	gdt_count = NGDT;
	gdt_next = NGDT;
	gdt_free = GNULL_SEL;

	old_gdt = gdt;
	gdt = (union descriptor *)uvm_km_valloc(kernel_map, max_len);
	uvm_map_pageable(kernel_map, (vaddr_t)gdt,
	    (vaddr_t)gdt + min_len, FALSE, FALSE);
	memcpy(gdt, old_gdt, NGDT * sizeof(gdt[0]));

	gdt_init_cpu();
}

/*
 * Load appropriate gdt descriptor.
 */

void gdt_init_cpu()
{
	struct region_descriptor region;
	size_t max_len;
	
	max_len = MAXGDTSIZ * sizeof(gdt[0]);
	setregion(&region, gdt, max_len - 1);
	lgdt(&region);
}

/*
 * Grow or shrink the GDT.
 */
void
gdt_grow()
{
	size_t old_len, new_len;

	old_len = gdt_size * sizeof(gdt[0]);
	gdt_size <<= 1;
	new_len = old_len << 1;

	uvm_map_pageable(kernel_map, (vaddr_t)gdt + old_len,
	    (vaddr_t)gdt + new_len, FALSE, FALSE);
}

void
gdt_shrink()
{
	size_t old_len, new_len;

	old_len = gdt_size * sizeof(gdt[0]);
	gdt_size >>= 1;
	new_len = old_len >> 1;

	uvm_map_pageable(kernel_map, (vaddr_t)gdt + new_len,
	    (vaddr_t)gdt + old_len, TRUE, FALSE);
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
gdt_put_slot(slot)
	int slot;
{

	gdt_lock();
	gdt_count--;

	gdt[slot].gd.gd_type = SDT_SYSNULL;
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
		gdt[slot].gd.gd_selector = gdt_free;
		gdt_free = slot;
	}

	gdt_unlock();
}

int
tss_alloc(pcb)
	struct pcb *pcb;
{
	int slot;

	slot = gdt_get_slot();
	setsegment(&gdt[slot].sd, &pcb->pcb_tss, sizeof(struct pcb) - 1,
	    SDT_SYS386TSS, SEL_KPL, 0, 0);
	return GSEL(slot, SEL_KPL);
}

void
tss_free(sel)
	int sel;
{

	gdt_put_slot(IDXSEL(sel));
}

void
ldt_alloc(pmap, ldt, len)
	struct pmap *pmap;
	union descriptor *ldt;
	size_t len;
{
	int slot;

	slot = gdt_get_slot();
	setsegment(&gdt[slot].sd, ldt, len - 1, SDT_SYSLDT, SEL_KPL, 0, 0);
	simple_lock(&pmap->pm_lock);
	pmap->pm_ldt_sel = GSEL(slot, SEL_KPL);
	simple_unlock(&pmap->pm_lock);
}

void
ldt_free(pmap)
	struct pmap *pmap;
{
	int slot;

	simple_lock(&pmap->pm_lock);
	slot = IDXSEL(pmap->pm_ldt_sel);
	simple_unlock(&pmap->pm_lock);

	gdt_put_slot(slot);
}
