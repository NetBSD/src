/*	$NetBSD: gdt.c,v 1.1 2001/06/19 00:21:16 fvdl Exp $	*/

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

/*
 * Modified to deal with variable-length entries for NetBSD/x86_64 by
 * fvdl@wasabisystems.com, may 2001
 * XXX this file should be shared with the i386 code, the difference
 * can be hidden in macros.
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/lock.h>
#include <sys/user.h>

#include <uvm/uvm_extern.h>

#include <machine/gdt.h>

#define	MINGDTSIZ	2048
#define	MAXGDTSIZ	65536

int gdt_size;		/* size of GDT in bytes */
int gdt_dyncount;	/* number of dyn. allocated GDT entries in use */
int gdt_dynavail;
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
 * 0) We partition the GDT into two areas, one of the slots before gdt_dyncount,
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
	int slot = 0, oslot;
	struct sys_segment_descriptor *gdt;

	gdt = (struct sys_segment_descriptor *)&gdtstore[DYNSEL_START];
	proclist_lock_read();
	for (p = allproc.lh_first; p != 0; p = p->p_list.le_next) {
		pmap = p->p_vmspace->vm_map.pmap;
		oslot = IDXDYNSEL(p->p_md.md_tss_sel);
		if (oslot >= gdt_dyncount) {
			while (gdt[slot].sd_type != SDT_SYSNULL) {
				if (++slot >= gdt_dyncount)
					panic("gdt_compact botch 1");
			}
			gdt[slot] = gdt[oslot];
			gdt[oslot].sd_type = SDT_SYSNULL;
			p->p_md.md_tss_sel = GDYNSEL(slot, SEL_KPL);
		}
		simple_lock(&pmap->pm_lock);
		oslot = IDXDYNSEL(pmap->pm_ldt_sel);
		if (oslot >= gdt_dyncount) {
			while (gdt[slot].sd_type != SDT_SYSNULL) {
				if (++slot >= gdt_dyncount)
					panic("gdt_compact botch 2");
			}
			gdt[slot] = gdt[oslot];
			gdt[oslot].sd_type = SDT_SYSNULL;
			pmap->pm_ldt_sel = GDYNSEL(slot, SEL_KPL);
			/*
			 * XXXSMP: if the pmap is in use on other
			 * processors, they need to reload thier
			 * LDT!
			 */
		}
		simple_unlock(&pmap->pm_lock);
	}
#ifdef DIAGNOSTIC
	for (; slot < gdt_dyncount; slot++)
		if (gdt[slot].sd_type == SDT_SYSNULL)
			panic("gdt_compact botch 3");

	for (slot = gdt_dyncount; slot < gdt_dynavail; slot++)
		if (gdt[slot].sd_type != SDT_SYSNULL)
			panic("gdt_compact botch 4");

#endif
	gdt_next = gdt_dyncount;
	gdt_free = GNULL_SEL;
	proclist_unlock_read();
}

/*
 * Initialize the GDT.
 */
void
gdt_init()
{
	struct region_descriptor region;
	char *old_gdt;

	lockinit(&gdt_lock_store, PZERO, "gdtlck", 0, 0);

	gdt_size = MINGDTSIZ;
	gdt_dyncount = 0;
	gdt_next = 0;
	gdt_free = GNULL_SEL;
	gdt_dynavail =
	    (gdt_size - DYNSEL_START) / sizeof (struct sys_segment_descriptor);

	old_gdt = gdtstore;
	gdtstore = (char *)uvm_km_valloc(kernel_map, MAXGDTSIZ);
	uvm_map_pageable(kernel_map, (vaddr_t)gdtstore,
	    (vaddr_t)gdtstore + MINGDTSIZ, FALSE, FALSE);
	memcpy(gdtstore, old_gdt, DYNSEL_START);

	setregion(&region, gdtstore, (u_int16_t)(MAXGDTSIZ - 1));
	lgdt(&region);
}

/*
 * Grow or shrink the GDT.
 */
void
gdt_grow()
{
	size_t old_len, new_len;

	old_len = gdt_size;
	gdt_size <<= 1;
	new_len = old_len << 1;
	gdt_dynavail =
	    (gdt_size - DYNSEL_START) / sizeof (struct sys_segment_descriptor);

	uvm_map_pageable(kernel_map, (vaddr_t)gdtstore + old_len,
	    (vaddr_t)gdtstore + new_len, FALSE, FALSE);
}

void
gdt_shrink()
{
	size_t old_len, new_len;

	old_len = gdt_size;
	gdt_size >>= 1;
	new_len = old_len >> 1;
	gdt_dynavail =
	    (gdt_size - DYNSEL_START) / sizeof (struct sys_segment_descriptor);

	uvm_map_pageable(kernel_map, (vaddr_t)gdtstore + new_len,
	    (vaddr_t)gdtstore + old_len, TRUE, FALSE);
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
gdt_get_slot()
{
	int slot;
	struct sys_segment_descriptor *gdt;

	gdt = (struct sys_segment_descriptor *)&gdtstore[DYNSEL_START];

	gdt_lock();

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
	struct sys_segment_descriptor *gdt;

	gdt = (struct sys_segment_descriptor *)&gdtstore[DYNSEL_START];

	gdt_lock();
	gdt_dyncount--;

	gdt[slot].sd_type = SDT_SYSNULL;
	/* 
	 * shrink the GDT if we're using less than 1/4 of it.
	 * Shrinking at that point means we'll still have room for
	 * almost 2x as many processes as are now running without
	 * having to grow the GDT.
	 */
	if (gdt_size > MINGDTSIZ && gdt_dyncount <= gdt_dynavail / 4) {
		gdt_compact();
		gdt_shrink();
	} else {
		gdt[slot].sd_xx3 = gdt_free;
		gdt_free = slot;
	}

	gdt_unlock();
}

void
tss_alloc(p)
	struct proc *p;
{
	struct pcb *pcb = &p->p_addr->u_pcb;
	int slot;
	struct sys_segment_descriptor *gdt;

	gdt = (struct sys_segment_descriptor *)&gdtstore[DYNSEL_START];

	slot = gdt_get_slot();
#if 0
	printf("tss_alloc: slot %d addr %p\n", slot, &gdt[slot]);
#endif
	set_sys_segment(&gdt[slot], &pcb->pcb_tss, sizeof (struct x86_64_tss)-1,
	    SDT_SYS386TSS, SEL_KPL, 0);
	p->p_md.md_tss_sel = GDYNSEL(slot, SEL_KPL);
#if 0
	printf("sel %x\n", p->p_md.md_tss_sel);
	printf("lolimit %lx lobase %lx type %lx dpl %lx p %lx hilimit %lx\n"
	       "xx1 %lx gran %lx hibase %lx xx2 %lx zero %lx xx3 %lx pad %lx\n",
		(unsigned long)gdt[slot].sd_lolimit,
		(unsigned long)gdt[slot].sd_lobase,
		(unsigned long)gdt[slot].sd_type,
		(unsigned long)gdt[slot].sd_dpl,
		(unsigned long)gdt[slot].sd_p,
		(unsigned long)gdt[slot].sd_hilimit,
		(unsigned long)gdt[slot].sd_xx1,
		(unsigned long)gdt[slot].sd_gran,
		(unsigned long)gdt[slot].sd_hibase,
		(unsigned long)gdt[slot].sd_xx2,
		(unsigned long)gdt[slot].sd_zero,
		(unsigned long)gdt[slot].sd_xx3);
#endif
}

void
tss_free(p)
	struct proc *p;
{

	gdt_put_slot(IDXDYNSEL(p->p_md.md_tss_sel));
}

void
ldt_alloc(pmap, ldt, len)
	struct pmap *pmap;
	char *ldt;
	size_t len;
{
	int slot;
	struct sys_segment_descriptor *gdt;

	gdt = (struct sys_segment_descriptor *)&gdtstore[DYNSEL_START];

	slot = gdt_get_slot();
	set_sys_segment(&gdt[slot], ldt, len - 1, SDT_SYSLDT, SEL_KPL, 0);
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
	slot = IDXDYNSEL(pmap->pm_ldt_sel);
	simple_unlock(&pmap->pm_lock);

	gdt_put_slot(slot);
}
