/*	$NetBSD: x86_xpmap.c,v 1.1.2.2 2007/10/25 23:59:24 bouyer Exp $	*/

/*
 * Copyright (c) 2006 Manuel Bouyer.
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
 *	This product includes software developed by Manuel Bouyer.
 * 4. The name of the author may not be used to endorse or promote products
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
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

/*
 *
 * Copyright (c) 2004 Christian Limpach.
 * All rights reserved.
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
 *      This product includes software developed by Christian Limpach.
 * 4. The name of the author may not be used to endorse or promote products
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
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */


#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: x86_xpmap.c,v 1.1.2.2 2007/10/25 23:59:24 bouyer Exp $");

#include "opt_xen.h"

#include <sys/param.h>
#include <sys/systm.h>

#include <uvm/uvm.h>

#include <machine/gdt.h>
#include <xen/xenfunc.h>

#undef	XENDEBUG
/* #define XENDEBUG_SYNC */
/* #define	XENDEBUG_LOW */

#ifdef XENDEBUG
#define	XENPRINTF(x) printf x
#define	XENPRINTK(x) printk x
#define	XENPRINTK2(x) /* printk x */

static char XBUF[256];
#else
#define	XENPRINTF(x)
#define	XENPRINTK(x)
#define	XENPRINTK2(x)
#endif
#define	PRINTF(x) printf x
#define	PRINTK(x) printk x

volatile shared_info_t *HYPERVISOR_shared_info;
union start_info_union start_info_union;

void xen_failsafe_handler(void);

#ifdef XEN3
#define HYPERVISOR_mmu_update_self(req, count, success_count) \
	HYPERVISOR_mmu_update((req), (count), (success_count), DOMID_SELF)
#else
#define HYPERVISOR_mmu_update_self(req, count, success_count) \
	HYPERVISOR_mmu_update((req), (count), (success_count))
#endif

void
xen_failsafe_handler(void)
{

	panic("xen_failsafe_handler called!\n");
}


#ifndef __x86_64__
void
xen_update_descriptor(union descriptor *table, union descriptor *entry)
{
	paddr_t pa;
	pt_entry_t *ptp;

	ptp = kvtopte((vaddr_t)table);
	pa = (*ptp & PG_FRAME) | ((vaddr_t)table & ~PG_FRAME);
	if (HYPERVISOR_update_descriptor(pa, entry->raw[0], entry->raw[1]))
		panic("HYPERVISOR_update_descriptor failed\n");
}
#endif

void
xen_set_ldt(vaddr_t base, uint32_t entries)
{
	vaddr_t va;
	vaddr_t end;
	pt_entry_t *ptp, *maptp;
	int s;

#ifdef __x86_64__
	end = base + (entries << 3);
#else
	end = base + entries * sizeof(union descriptor);
#endif

	for (va = base; va < end; va += PAGE_SIZE) {
		KASSERT(va >= VM_MIN_KERNEL_ADDRESS);
		ptp = kvtopte(va);
		maptp = (pt_entry_t *)vtomach((vaddr_t)ptp);
		XENPRINTF(("xen_set_ldt %p %d %p %p\n", (void *)base,
			      entries, ptp, maptp));
		printf("xen_set_ldt %p %d %p %p\n", (void *)base,
			      entries, ptp, maptp);
		PTE_CLEARBITS(ptp, maptp, PG_RW);
	}
	s = splvm();
	PTE_UPDATES_FLUSH();

	xpq_queue_set_ldt(base, entries);
	xpq_flush_queue();
	splx(s);
}

#ifdef XENDEBUG
void xpq_debug_dump(void);
#endif

#define XPQUEUE_SIZE 2048
static mmu_update_t xpq_queue[XPQUEUE_SIZE];
static int xpq_idx = 0;

void
xpq_flush_queue()
{
	int i, ok;

	XENPRINTK2(("flush queue %p entries %d\n", xpq_queue, xpq_idx));
	for (i = 0; i < xpq_idx; i++)
		XENPRINTK2(("%d: %p %08x\n", i, (u_int)xpq_queue[i].ptr,
		    (u_int)xpq_queue[i].val));
	if (xpq_idx != 0 &&
	    HYPERVISOR_mmu_update_self(xpq_queue, xpq_idx, &ok) < 0)
		panic("HYPERVISOR_mmu_update failed\n");
	xpq_idx = 0;
}

static inline void
xpq_increment_idx(void)
{

	xpq_idx++;
	if (__predict_false(xpq_idx == XPQUEUE_SIZE))
		xpq_flush_queue();
}

void
xpq_queue_machphys_update(paddr_t ma, paddr_t pa)
{
	XENPRINTK2(("xpq_queue_machphys_update ma=%p pa=%p\n", (void *)ma, (void *)pa));
	xpq_queue[xpq_idx].ptr = ma | MMU_MACHPHYS_UPDATE;
	xpq_queue[xpq_idx].val = (pa - XPMAP_OFFSET) >> PAGE_SHIFT;
	xpq_increment_idx();
#ifdef XENDEBUG_SYNC
	xpq_flush_queue();
#endif
}

void
xpq_queue_pde_update(pd_entry_t *ptr, pd_entry_t val)
{

	KASSERT(((paddr_t)ptr & 3) == 0);
	xpq_queue[xpq_idx].ptr = (paddr_t)ptr | MMU_NORMAL_PT_UPDATE;
	xpq_queue[xpq_idx].val = val;
	xpq_increment_idx();
#ifdef XENDEBUG_SYNC
	xpq_flush_queue();
#endif
}

void
xpq_queue_pte_update(pt_entry_t *ptr, pt_entry_t val)
{

	KASSERT(((paddr_t)ptr & 3) == 0);
	xpq_queue[xpq_idx].ptr = (paddr_t)ptr | MMU_NORMAL_PT_UPDATE;
	xpq_queue[xpq_idx].val = val;
	xpq_increment_idx();
#ifdef XENDEBUG_SYNC
	xpq_flush_queue();
#endif
}

#ifdef XEN3
void
xpq_queue_pt_switch(paddr_t pa)
{
	struct mmuext_op op;
	xpq_flush_queue();

	XENPRINTK2(("xpq_queue_pt_switch: %p %p\n", (void *)pa, (void *)pa));
	op.cmd = MMUEXT_NEW_BASEPTR;
	op.arg1.mfn = pa >> PAGE_SHIFT;
	if (HYPERVISOR_mmuext_op(&op, 1, NULL, DOMID_SELF) < 0)
		panic("xpq_queue_pt_switch");
}

void
xpq_queue_pin_table(paddr_t pa)
{
	struct mmuext_op op;
	xpq_flush_queue();

	XENPRINTK2(("xpq_queue_pin_table: %p %p\n", (void *)pa, (void *)pa));
	op.arg1.mfn = pa >> PAGE_SHIFT;

#ifdef __x86_64__
	op.cmd = MMUEXT_PIN_L4_TABLE;
#else
	op.cmd = MMUEXT_PIN_L2_TABLE;
#endif
	if (HYPERVISOR_mmuext_op(&op, 1, NULL, DOMID_SELF) < 0)
		panic("xpq_queue_pin_table");
}

void
xpq_queue_unpin_table(paddr_t pa)
{
	struct mmuext_op op;
	xpq_flush_queue();

	XENPRINTK2(("xpq_queue_unpin_table: %p %p\n", (void *)pa, (void *)pa));
	op.arg1.mfn = pa >> PAGE_SHIFT;
	op.cmd = MMUEXT_UNPIN_TABLE;
	if (HYPERVISOR_mmuext_op(&op, 1, NULL, DOMID_SELF) < 0)
		panic("xpq_queue_unpin_table");
}

void
xpq_queue_set_ldt(vaddr_t va, uint32_t entries)
{
	struct mmuext_op op;
	xpq_flush_queue();

	XENPRINTK2(("xpq_queue_set_ldt\n"));
	KASSERT(va == (va & ~PAGE_MASK));
	op.cmd = MMUEXT_SET_LDT;
	op.arg1.linear_addr = va;
	op.arg2.nr_ents = entries;
	if (HYPERVISOR_mmuext_op(&op, 1, NULL, DOMID_SELF) < 0)
		panic("xpq_queue_set_ldt");
}

void
xpq_queue_tlb_flush()
{
	struct mmuext_op op;
	xpq_flush_queue();

	XENPRINTK2(("xpq_queue_tlb_flush\n"));
	op.cmd = MMUEXT_TLB_FLUSH_LOCAL;
	if (HYPERVISOR_mmuext_op(&op, 1, NULL, DOMID_SELF) < 0)
		panic("xpq_queue_tlb_flush");
}

void
xpq_flush_cache()
{
	struct mmuext_op op;
	int s = splvm();
	xpq_flush_queue();

	XENPRINTK2(("xpq_queue_flush_cache\n"));
	op.cmd = MMUEXT_FLUSH_CACHE;
	if (HYPERVISOR_mmuext_op(&op, 1, NULL, DOMID_SELF) < 0)
		panic("xpq_flush_cache");
	splx(s);
}

void
xpq_queue_invlpg(vaddr_t va)
{
	struct mmuext_op op;
	xpq_flush_queue();

	XENPRINTK2(("xpq_queue_invlpg %p\n", (void *)va));
	op.cmd = MMUEXT_INVLPG_LOCAL;
	op.arg1.linear_addr = (va & ~PAGE_MASK);
	if (HYPERVISOR_mmuext_op(&op, 1, NULL, DOMID_SELF) < 0)
		panic("xpq_queue_invlpg");
}

int
xpq_update_foreign(pt_entry_t *ptr, pt_entry_t val, int dom)
{
	mmu_update_t op;
	int ok;
	xpq_flush_queue();

	op.ptr = (paddr_t)ptr;
	op.val = val;
	if (HYPERVISOR_mmu_update(&op, 1, &ok, dom) < 0)
		return EFAULT;
	return (0);
}
#else /* XEN3 */
void
xpq_queue_pt_switch(paddr_t pa)
{

	XENPRINTK2(("xpq_queue_pt_switch: %p %p\n", (void *)pa, (void *)pa));
	xpq_queue[xpq_idx].ptr = pa | MMU_EXTENDED_COMMAND;
	xpq_queue[xpq_idx].val = MMUEXT_NEW_BASEPTR;
	xpq_increment_idx();
}

void
xpq_queue_pin_table(paddr_t pa)
{

	XENPRINTK2(("xpq_queue_pin_table: %p %p\n", (void *)pa, (void *)pa));
	xpq_queue[xpq_idx].ptr = pa | MMU_EXTENDED_COMMAND;
	xpq_queue[xpq_idx].val = MMUEXT_PIN_L2_TABLE;
	xpq_increment_idx();
}

void
xpq_queue_unpin_table(paddr_t pa)
{

	XENPRINTK2(("xpq_queue_unpin_table: %p %p\n", (void *)pa, (void *)pa));
	xpq_queue[xpq_idx].ptr = pa | MMU_EXTENDED_COMMAND;
	xpq_queue[xpq_idx].val = MMUEXT_UNPIN_TABLE;
	xpq_increment_idx();
}

void
xpq_queue_set_ldt(vaddr_t va, uint32_t entries)
{

	XENPRINTK2(("xpq_queue_set_ldt\n"));
	KASSERT(va == (va & ~PAGE_MASK));
	xpq_queue[xpq_idx].ptr = MMU_EXTENDED_COMMAND | va;
	xpq_queue[xpq_idx].val = MMUEXT_SET_LDT | (entries << MMUEXT_CMD_SHIFT);
	xpq_increment_idx();
}

void
xpq_queue_tlb_flush()
{

	XENPRINTK2(("xpq_queue_tlb_flush\n"));
	xpq_queue[xpq_idx].ptr = MMU_EXTENDED_COMMAND;
	xpq_queue[xpq_idx].val = MMUEXT_TLB_FLUSH;
	xpq_increment_idx();
}

void
xpq_flush_cache()
{
	int s = splvm();

	XENPRINTK2(("xpq_queue_flush_cache\n"));
	xpq_queue[xpq_idx].ptr = MMU_EXTENDED_COMMAND;
	xpq_queue[xpq_idx].val = MMUEXT_FLUSH_CACHE;
	xpq_increment_idx();
	xpq_flush_queue();
	splx(s);
}

void
xpq_queue_invlpg(vaddr_t va)
{

	XENPRINTK2(("xpq_queue_invlpg %p\n", (void *)va));
	xpq_queue[xpq_idx].ptr = (va & ~PAGE_MASK) | MMU_EXTENDED_COMMAND;
	xpq_queue[xpq_idx].val = MMUEXT_INVLPG;
	xpq_increment_idx();
}

int
xpq_update_foreign(pt_entry_t *ptr, pt_entry_t val, int dom)
{
	mmu_update_t xpq_up[3];

	xpq_up[0].ptr = MMU_EXTENDED_COMMAND;
	xpq_up[0].val = MMUEXT_SET_FOREIGNDOM | (dom << 16);
	xpq_up[1].ptr = (paddr_t)ptr;
	xpq_up[1].val = val;
	if (HYPERVISOR_mmu_update_self(xpq_up, 2, NULL) < 0)
		return EFAULT;
	return (0);
}
#endif /* XEN3 */

#ifdef XENDEBUG
void
xpq_debug_dump()
{
	int i;

	XENPRINTK2(("idx: %d\n", xpq_idx));
	for (i = 0; i < xpq_idx; i++) {
		sprintf(XBUF, "%x %08x ", (u_int)xpq_queue[i].ptr,
		    (u_int)xpq_queue[i].val);
		if (++i < xpq_idx)
			sprintf(XBUF + strlen(XBUF), "%x %08x ",
			    (u_int)xpq_queue[i].ptr, (u_int)xpq_queue[i].val);
		if (++i < xpq_idx)
			sprintf(XBUF + strlen(XBUF), "%x %08x ",
			    (u_int)xpq_queue[i].ptr, (u_int)xpq_queue[i].val);
		if (++i < xpq_idx)
			sprintf(XBUF + strlen(XBUF), "%x %08x ",
			    (u_int)xpq_queue[i].ptr, (u_int)xpq_queue[i].val);
		XENPRINTK2(("%d: %s\n", xpq_idx, XBUF));
	}
}
#endif
