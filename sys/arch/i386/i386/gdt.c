/*	$NetBSD: gdt.c,v 1.40.2.2 2008/01/09 01:46:35 matt Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: gdt.c,v 1.40.2.2 2008/01/09 01:46:35 matt Exp $");

#include "opt_multiprocessor.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/mutex.h>
#include <sys/user.h>

#include <uvm/uvm.h>

#include <machine/gdt.h>

int gdt_size;		/* total number of GDT entries */
int gdt_count;		/* number of GDT entries in use */
int gdt_next;		/* next available slot for sweeping */
int gdt_free;		/* next free slot; terminated with GNULL_SEL */

static kmutex_t gdt_lock_store;

static inline void gdt_lock(void);
static inline void gdt_unlock(void);
void gdt_init(void);
void gdt_grow(void);

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

	mutex_enter(&gdt_lock_store);
}

static inline void
gdt_unlock()
{

	mutex_exit(&gdt_lock_store);
}

void
setgdt(int sel, const void *base, size_t limit,
    int type, int dpl, int def32, int gran)
{
	struct segment_descriptor *sd = &gdt[sel].sd;
	CPU_INFO_ITERATOR cii;
	struct cpu_info *ci;

	setsegment(sd, base, limit, type, dpl, def32, gran);
	for (CPU_INFO_FOREACH(cii, ci)) {
		if (ci->ci_gdt != NULL)
			ci->ci_gdt[sel].sd = *sd;
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

	mutex_init(&gdt_lock_store, MUTEX_DEFAULT, IPL_NONE);

	max_len = MAXGDTSIZ * sizeof(gdt[0]);
	min_len = MINGDTSIZ * sizeof(gdt[0]);

	gdt_size = MINGDTSIZ;
	gdt_count = NGDT;
	gdt_next = NGDT;
	gdt_free = GNULL_SEL;

	old_gdt = gdt;
	gdt = (union descriptor *)uvm_km_alloc(kernel_map, max_len,
	    0, UVM_KMF_VAONLY);
	for (va = (vaddr_t)gdt; va < (vaddr_t)gdt + min_len; va += PAGE_SIZE) {
		pg = uvm_pagealloc(NULL, 0, NULL, UVM_PGA_ZERO);
		if (pg == NULL) {
			panic("gdt_init: no pages");
		}
		pmap_kenter_pa(va, VM_PAGE_TO_PHYS(pg),
		    VM_PROT_READ | VM_PROT_WRITE);
	}
	pmap_update(pmap_kernel());
	memcpy(gdt, old_gdt, NGDT * sizeof(gdt[0]));
	ci->ci_gdt = gdt;
	setsegment(&ci->ci_gdt[GCPU_SEL].sd, ci, 0xfffff,
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

	ci->ci_gdt = (union descriptor *)uvm_km_alloc(kernel_map, max_len,
	    0, UVM_KMF_VAONLY);
	for (va = (vaddr_t)ci->ci_gdt; va < (vaddr_t)ci->ci_gdt + min_len;
	    va += PAGE_SIZE) {
		while ((pg = uvm_pagealloc(NULL, 0, NULL, UVM_PGA_ZERO))
		    == NULL) {
			uvm_wait("gdt_alloc_cpu");
		}
		pmap_kenter_pa(va, VM_PAGE_TO_PHYS(pg),
		    VM_PROT_READ | VM_PROT_WRITE);
	}
	pmap_update(pmap_kernel());
	memset(ci->ci_gdt, 0, min_len);
	memcpy(ci->ci_gdt, gdt, gdt_count * sizeof(gdt[0]));
	setsegment(&ci->ci_gdt[GCPU_SEL].sd, ci, 0xfffff,
	    SDT_MEMRWA, SEL_KPL, 1, 1);
}


/*
 * Load appropriate gdt descriptor; we better be running on *ci
 * (for the most part, this is how a CPU knows who it is).
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
 * Grow the GDT.
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

	pmap_update(pmap_kernel());
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
gdt_put_slot(int slot)
{

	gdt_lock();
	gdt_count--;

	gdt[slot].gd.gd_type = SDT_SYSNULL;
	gdt[slot].gd.gd_selector = gdt_free;
	gdt_free = slot;

	gdt_unlock();
}

int
tss_alloc(const struct i386tss *tss)
{
	int slot;

	slot = gdt_get_slot();
	setgdt(slot, tss, sizeof(struct i386tss) + IOMAPSIZE - 1,
	    SDT_SYS386TSS, SEL_KPL, 0, 0);
	return GSEL(slot, SEL_KPL);
}

void
tss_free(int sel)
{

	gdt_put_slot(IDXSEL(sel));
}

/*
 * Caller must have pmap locked for both of these functions.
 */
int
ldt_alloc(union descriptor *ldtp, size_t len)
{
	int slot;

	slot = gdt_get_slot();
	setgdt(slot, ldtp, len - 1, SDT_SYSLDT, SEL_KPL, 0, 0);

	return GSEL(slot, SEL_KPL);
}

void
ldt_free(int sel)
{
	int slot;

	slot = IDXSEL(sel);

	gdt_put_slot(slot);
}
