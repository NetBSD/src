/*	$NetBSD: gdt.c,v 1.3 2003/01/26 00:05:39 fvdl Exp $	*/

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

#include <uvm/uvm.h>

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
 * Initialize the GDT.
 */
void
gdt_init()
{
	struct region_descriptor region;
	char *old_gdt;
	struct vm_page *pg;
	vaddr_t va;

	lockinit(&gdt_lock_store, PZERO, "gdtlck", 0, 0);

	gdt_size = MINGDTSIZ;
	gdt_dyncount = 0;
	gdt_next = 0;
	gdt_free = GNULL_SEL;
	gdt_dynavail =
	    (gdt_size - DYNSEL_START) / sizeof (struct sys_segment_descriptor);

	old_gdt = gdtstore;
	gdtstore = (char *)uvm_km_valloc(kernel_map, MAXGDTSIZ);
	for (va = (vaddr_t)gdtstore; va < (vaddr_t)gdtstore + MINGDTSIZ;
	    va += PAGE_SIZE) {
		pg = uvm_pagealloc(NULL, 0, NULL, UVM_PGA_ZERO);
		if (pg == NULL) {
			panic("gdt_init: no pages");
		}
		pmap_kenter_pa(va, VM_PAGE_TO_PHYS(pg),
		    VM_PROT_READ | VM_PROT_WRITE);
	}
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
		    VM_PROT_READ | VM_PROT_WRITE);
	}
}

void
gdt_shrink()
{
	size_t old_len, new_len;
	struct vm_page *pg;
	paddr_t pa;
	vaddr_t va;

	old_len = gdt_size;
	gdt_size >>= 1;
	new_len = old_len >> 1;
	gdt_dynavail =
	    (gdt_size - DYNSEL_START) / sizeof (struct sys_segment_descriptor);

	for (va = (vaddr_t)gdtstore + new_len; va < (vaddr_t)gdtstore + old_len;
	    va += PAGE_SIZE) {
		if (!pmap_extract(pmap_kernel(), va, &pa)) {
			panic("gdt_shrink botch");
		}
		pg = PHYS_TO_VM_PAGE(pa);
		pmap_kremove(va, PAGE_SIZE);
		uvm_pagefree(pg);
	}
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
		gdt_shrink();
	} else {
		gdt[slot].sd_xx3 = gdt_free;
		gdt_free = slot;
	}

	gdt_unlock();
}

int
tss_alloc(pcb)
	struct pcb *pcb;
{
	int slot;
	struct sys_segment_descriptor *gdt;

	gdt = (struct sys_segment_descriptor *)&gdtstore[DYNSEL_START];

	slot = gdt_get_slot();
#if 0
	printf("tss_alloc: slot %d addr %p\n", slot, &gdt[slot]);
#endif
	set_sys_segment(&gdt[slot], &pcb->pcb_tss, sizeof (struct x86_64_tss)-1,
	    SDT_SYS386TSS, SEL_KPL, 0);
#if 0
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
	return GDYNSEL(slot, SEL_KPL);
}

void
tss_free(int sel)
{

	gdt_put_slot(IDXDYNSEL(sel));
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
