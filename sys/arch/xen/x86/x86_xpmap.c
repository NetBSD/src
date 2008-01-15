/*	$NetBSD: x86_xpmap.c,v 1.5 2008/01/15 19:55:53 bouyer Exp $	*/

/*
 * Copyright (c) 2006 Mathieu Ropert <mro@adviseo.fr>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

/*
 * Copyright (c) 2006, 2007 Manuel Bouyer.
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
__KERNEL_RCSID(0, "$NetBSD: x86_xpmap.c,v 1.5 2008/01/15 19:55:53 bouyer Exp $");

#include "opt_xen.h"
#include "opt_ddb.h"
#include "ksyms.h"

#include <sys/param.h>
#include <sys/systm.h>

#include <uvm/uvm.h>

#include <machine/pmap.h>
#include <machine/gdt.h>
#include <xen/xenfunc.h>

#include <dev/isa/isareg.h>
#include <machine/isa_machdep.h>

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

/* on x86_64 kernel runs in ring 3 */
#ifdef __x86_64__
#define PG_k PG_u
#else
#define PG_k 0
#endif

volatile shared_info_t *HYPERVISOR_shared_info;
union start_info_union start_info_union;
paddr_t *xpmap_phys_to_machine_mapping;

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


void
xen_set_ldt(vaddr_t base, uint32_t entries)
{
	vaddr_t va;
	vaddr_t end;
	pt_entry_t *ptp;
	int s;

#ifdef __x86_64__
	end = base + (entries << 3);
#else
	end = base + entries * sizeof(union descriptor);
#endif

	for (va = base; va < end; va += PAGE_SIZE) {
		KASSERT(va >= VM_MIN_KERNEL_ADDRESS);
		ptp = kvtopte(va);
		XENPRINTF(("xen_set_ldt %p %d %p\n", (void *)base,
			      entries, ptp));
		pmap_pte_clearbits(ptp, PG_RW);
	}
	s = splvm();
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
	    HYPERVISOR_mmu_update_self(xpq_queue, xpq_idx, &ok) < 0) {
		printf("xpq_flush_queue: %d entries \n", xpq_idx);
		for (i = 0; i < xpq_idx; i++)
			printf("0x%016" PRIx64 ": 0x%016" PRIx64 "\n",
			   (u_int64_t)xpq_queue[i].ptr,
			   (u_int64_t)xpq_queue[i].val);
		panic("HYPERVISOR_mmu_update failed\n");
	}
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


extern volatile struct xencons_interface *xencons_interface; /* XXX */
extern struct xenstore_domain_interface *xenstore_interface; /* XXX */

static void xen_bt_set_readonly (vaddr_t);
static void xen_bootstrap_tables (vaddr_t, vaddr_t, int, int, int);

/* How many PDEs ? */
#if L2_SLOT_KERNBASE > 0
#define TABLE_L2_ENTRIES (2 * (NKL2_KIMG_ENTRIES + 1))
#else
#define TABLE_L2_ENTRIES (NKL2_KIMG_ENTRIES + 1)
#endif

/* 
 * Construct and switch to new pagetables
 * first_avail is the first vaddr we can use after
 * we get rid of Xen pagetables
 */

vaddr_t xen_pmap_bootstrap (void);

/*
 * Function to get rid of Xen bootstrap tables
 */

vaddr_t
xen_pmap_bootstrap()
{
	int count, oldcount;
	long mapsize;
	const int l2_4_count = PTP_LEVELS - 1;
	vaddr_t bootstrap_tables, init_tables;

	xpmap_phys_to_machine_mapping = (paddr_t *) xen_start_info.mfn_list;
	init_tables = xen_start_info.pt_base;
	__PRINTK(("xen_arch_pmap_bootstrap init_tables=0x%lx\n", init_tables));

	/* Space after Xen boostrap tables should be free */
	bootstrap_tables = xen_start_info.pt_base +
		(xen_start_info.nr_pt_frames * PAGE_SIZE);

	/*
	 * Calculate how many space we need
	 * first everything mapped before the Xen bootstrap tables
	 */
	mapsize = init_tables - KERNTEXTOFF;
	/* after the tables we'll have:
	 *  - UAREA
	 *  - dummy user PGD (x86_64)
	 *  - HYPERVISOR_shared_info
	 *  - ISA I/O mem (if needed)
	 */
	mapsize += UPAGES * NBPG;
#ifdef __x86_64__
	mapsize += NBPG;
#endif
	mapsize += NBPG;

#ifdef DOM0OPS
	if (xen_start_info.flags & SIF_INITDOMAIN) {
		/* space for ISA I/O mem */
		mapsize += IOM_SIZE;
	}
#endif
	/* at this point mapsize doens't include the table size */

#ifdef __x86_64__
	count = TABLE_L2_ENTRIES;
#else
	count = (mapsize + (NBPD_L2 -1)) >> L2_SHIFT;
#endif /* __x86_64__ */
	
	/* now compute how many L2 pages we need exactly */
	XENPRINTK(("bootstrap_final mapsize 0x%lx count %d\n", mapsize, count));
	while (mapsize + (count + l2_4_count) * PAGE_SIZE + KERNTEXTOFF >
	    ((long)count << L2_SHIFT) + KERNBASE) {
		count++;
	}
#ifndef __x86_64__
	/*
	 * one more L2 page: we'll alocate several pages after kva_start
	 * in pmap_bootstrap() before pmap_growkernel(), which have not been
	 * counted here. It's not a big issue to allocate one more L2 as
	 * pmap_growkernel() will be called anyway.
	 */
	count++;
	nkptp[1] = count;
#endif

	/*
	 * install bootstrap pages. We may need more L2 pages than will
	 * have the final table here, as it's installed after the final table
	 */
	oldcount = count;

bootstrap_again:
	XENPRINTK(("bootstrap_again oldcount %d\n", oldcount));
	/* 
	 * Xen space we'll reclaim may not be enough for our new page tables,
	 * move bootstrap tables if necessary
	 */
	if (bootstrap_tables < init_tables + ((count + l2_4_count) * PAGE_SIZE))
		bootstrap_tables = init_tables +
					((count + l2_4_count) * PAGE_SIZE);
	/* make sure we have enough to map the bootstrap_tables */
	if (bootstrap_tables + ((oldcount + l2_4_count) * PAGE_SIZE) > 
	    ((long)oldcount << L2_SHIFT) + KERNBASE) {
		oldcount++;
		goto bootstrap_again;
	}

	/* Create temporary tables */
	xen_bootstrap_tables(xen_start_info.pt_base, bootstrap_tables,
		xen_start_info.nr_pt_frames, oldcount, 0);

	/* Create final tables */
	xen_bootstrap_tables(bootstrap_tables, init_tables,
	    oldcount + l2_4_count, count, 1);

	/* zero out free space after tables */
	memset((void *)(init_tables + ((count + l2_4_count) * PAGE_SIZE)), 0,
	    (UPAGES + 1) * NBPG);
	return (init_tables + ((count + l2_4_count) * PAGE_SIZE));
}


/*
 * Build a new table and switch to it
 * old_count is # of old tables (including PGD, PDTPE and PDE)
 * new_count is # of new tables (PTE only)
 * we assume areas don't overlap
 */


static void
xen_bootstrap_tables (vaddr_t old_pgd, vaddr_t new_pgd,
	int old_count, int new_count, int final)
{
	pd_entry_t *pdtpe, *pde, *pte;
	pd_entry_t *cur_pgd, *bt_pgd;
	paddr_t addr, page;
	vaddr_t avail, text_end, map_end;
	int i;
	extern char __data_start;

	__PRINTK(("xen_bootstrap_tables(0x%lx, 0x%lx, %d, %d)\n",
	    old_pgd, new_pgd, old_count, new_count));
	text_end = ((vaddr_t)&__data_start) & ~PAGE_MASK;
	/*
	 * size of R/W area after kernel text:
	 *  xencons_interface (if present)
	 *  xenstore_interface (if present)
	 *  table pages (new_count + (PTP_LEVELS - 1) entries)
	 * extra mappings (only when final is true):
	 *  UAREA
	 *  dummy user PGD (x86_64 only)/gdt page (i386 only)
	 *  HYPERVISOR_shared_info
	 *  ISA I/O mem (if needed)
	 */
	map_end = new_pgd + ((new_count + PTP_LEVELS - 1) * NBPG);
	if (final) {
		map_end += (UPAGES + 1) * NBPG;
		HYPERVISOR_shared_info = (shared_info_t *)map_end;
		map_end += NBPG;
	}
	/*
	 * we always set atdevbase, as it's used by init386 to find the first
	 * available VA. map_end is updated only if we are dom0, so
	 * atdevbase -> atdevbase + IOM_SIZE will be mapped only in
	 * this case.
	 */
	if (final)
		atdevbase = map_end;
#ifdef DOM0OPS
	if (final && (xen_start_info.flags & SIF_INITDOMAIN)) {
		/* ISA I/O mem */
		map_end += IOM_SIZE;
	}
#endif /* DOM0OPS */

	__PRINTK(("xen_bootstrap_tables text_end 0x%lx map_end 0x%lx\n",
	    text_end, map_end));

	/* 
	 * Create bootstrap page tables
	 * What we need:
	 * - a PGD (level 4)
	 * - a PDTPE (level 3)
	 * - a PDE (level2)
	 * - some PTEs (level 1)
	 */
	
	cur_pgd = (pd_entry_t *) old_pgd;
	bt_pgd = (pd_entry_t *) new_pgd;
	memset (bt_pgd, 0, PAGE_SIZE);
	avail = new_pgd + PAGE_SIZE;
#if PTP_LEVELS > 3
	/* Install level 3 */
	pdtpe = (pd_entry_t *) avail;
	memset (pdtpe, 0, PAGE_SIZE);
	avail += PAGE_SIZE;

	addr = ((paddr_t) pdtpe) - KERNBASE;
	bt_pgd[pl4_pi(KERNTEXTOFF)] =
	    xpmap_ptom_masked(addr) | PG_k | PG_RW | PG_V;

	__PRINTK(("L3 va 0x%lx pa 0x%lx entry 0x%lx -> L4[0x%x]\n",
	    pdtpe, addr, bt_pgd[pl4_pi(KERNTEXTOFF)], pl4_pi(KERNTEXTOFF)));
#else
	pdtpe = bt_pgd;
#endif /* PTP_LEVELS > 3 */

#if PTP_LEVELS > 2
	/* Level 2 */
	pde = (pd_entry_t *) avail;
	memset(pde, 0, PAGE_SIZE);
	avail += PAGE_SIZE;

	addr = ((paddr_t) pde) - KERNBASE;
	pdtpe[pl3_pi(KERNTEXTOFF)] =
	    xpmap_ptom_masked(addr) | PG_k | PG_RW | PG_V;
	__PRINTK(("L2 va 0x%lx pa 0x%lx entry 0x%lx -> L3[0x%x]\n",
	    pde, addr, pdtpe[pl3_pi(KERNTEXTOFF)], pl3_pi(KERNTEXTOFF)));
#else
	pde = bt_pgd;
#endif /* PTP_LEVELS > 3 */

	/* Level 1 */
	page = KERNTEXTOFF;
	for (i = 0; i < new_count; i ++) {
		paddr_t cur_page = page;

		pte = (pd_entry_t *) avail;
		avail += PAGE_SIZE;

		memset(pte, 0, PAGE_SIZE);
		while (pl2_pi(page) == pl2_pi (cur_page)) {
			if (page >= map_end) {
				/* not mapped at all */
				pte[pl1_pi(page)] = 0;
				page += PAGE_SIZE;
				continue;
			}
			pte[pl1_pi(page)] = xpmap_ptom_masked(page - KERNBASE);
			if (page == (vaddr_t)HYPERVISOR_shared_info) {
				pte[pl1_pi(page)] = xen_start_info.shared_info;
				__PRINTK(("HYPERVISOR_shared_info "
				    "va 0x%lx pte 0x%lx\n",
				    HYPERVISOR_shared_info, pte[pl1_pi(page)]));
			}
#ifdef XEN3
			if (xpmap_ptom_masked(page - KERNBASE) ==
			    (xen_start_info.console_mfn << PAGE_SHIFT)) {
				xencons_interface = (void *)page;
				pte[pl1_pi(page)] =
				    (xen_start_info.console_mfn << PAGE_SHIFT);
				__PRINTK(("xencons_interface "
				    "va 0x%lx pte 0x%lx\n",
				    xencons_interface, pte[pl1_pi(page)]));
			}
			if (xpmap_ptom_masked(page - KERNBASE) ==
			    (xen_start_info.store_mfn << PAGE_SHIFT)) {
				xenstore_interface = (void *)page;
				pte[pl1_pi(page)] =
				    (xen_start_info.store_mfn << PAGE_SHIFT);
				__PRINTK(("xenstore_interface "
				    "va 0x%lx pte 0x%lx\n",
				    xenstore_interface, pte[pl1_pi(page)]));
			}
#endif /* XEN3 */
#ifdef DOM0OPS
			if (page >= (vaddr_t)atdevbase &&
			    page < (vaddr_t)atdevbase + IOM_SIZE) {
				pte[pl1_pi(page)] =
				    IOM_BEGIN + (page - (vaddr_t)atdevbase);
			}
#endif
			pte[pl1_pi(page)] |= PG_k | PG_V;
			if (page < text_end) {
				/* map kernel text RO */
				pte[pl1_pi(page)] |= 0;
			} else if (page >= old_pgd
			    && page < old_pgd + (old_count * PAGE_SIZE)) {
				/* map old page tables RO */
				pte[pl1_pi(page)] |= 0;
			} else if (page >= new_pgd &&
			    page < new_pgd + ((new_count + PTP_LEVELS - 1) * PAGE_SIZE)) {
				/* map new page tables RO */
				pte[pl1_pi(page)] |= 0;
			} else {
				/* map page RW */
				pte[pl1_pi(page)] |= PG_RW;
			}
			if (page  == old_pgd || page >= new_pgd)
				__PRINTK(("va 0x%lx pa 0x%lx "
				    "entry 0x%lx -> L1[0x%x]\n",
				    page, page - KERNBASE,
				    pte[pl1_pi(page)], pl1_pi(page)));
			page += PAGE_SIZE;
		}

		addr = ((paddr_t) pte) - KERNBASE;
		pde[pl2_pi(cur_page)] =
		    xpmap_ptom_masked(addr) | PG_k | PG_RW | PG_V;
		__PRINTK(("L1 va 0x%lx pa 0x%lx entry 0x%lx -> L2[0x%x]\n",
		    pte, addr, pde[pl2_pi(cur_page)], pl2_pi(cur_page)));
		/* Mark readonly */
		xen_bt_set_readonly((vaddr_t) pte);
	}

	/* Install recursive page tables mapping */
	bt_pgd[PDIR_SLOT_PTE] =
	    xpmap_ptom_masked(new_pgd - KERNBASE) | PG_k | PG_V;
	__PRINTK(("bt_pgd[PDIR_SLOT_PTE] va 0x%lx pa 0x%lx entry 0x%lx\n",
	    new_pgd, new_pgd - KERNBASE, bt_pgd[PDIR_SLOT_PTE]));

	/* Mark tables RO */
	xen_bt_set_readonly((vaddr_t) pde);
#if PTP_LEVELS > 2
	xen_bt_set_readonly((vaddr_t) pdtpe);
#endif
#if PTP_LEVELS > 3
	xen_bt_set_readonly(new_pgd);
#endif
	/* Pin the PGD */
	__PRINTK(("pin PDG\n"));
	xpq_queue_pin_table(xpmap_ptom_masked(new_pgd - KERNBASE));
#ifdef __i386__
	/* Save phys. addr of PDP, for libkvm. */
	PDPpaddr = new_pgd;
#endif
	/* Switch to new tables */
	__PRINTK(("switch to PDG\n"));
	xpq_queue_pt_switch(xpmap_ptom_masked(new_pgd - KERNBASE));
	__PRINTK(("bt_pgd[PDIR_SLOT_PTE] now entry 0x%lx\n",
	    bt_pgd[PDIR_SLOT_PTE]));

	/* Now we can safely reclaim space taken by old tables */
	
	__PRINTK(("unpin old PDG\n"));
	/* Unpin old PGD */
	xpq_queue_unpin_table(xpmap_ptom_masked(old_pgd - KERNBASE));
	/* Mark old tables RW */
	page = old_pgd;
	addr = (paddr_t) pde[pl2_pi(page)] & PG_FRAME;
	addr = xpmap_mtop(addr);
	pte = (pd_entry_t *) (addr + KERNBASE);
	pte += pl1_pi(page);
	__PRINTK(("*pde 0x%lx addr 0x%lx pte 0x%lx\n",
	    pde[pl2_pi(page)], addr, pte));
	while (page < old_pgd + (old_count * PAGE_SIZE) && page < map_end) {
		addr = xpmap_ptom(((paddr_t) pte) - KERNBASE);
		XENPRINTK(("addr 0x%lx pte 0x%lx *pte 0x%lx\n",
		   addr, pte, *pte));
		xpq_queue_pte_update((pt_entry_t *) addr, *pte | PG_RW);
		page += PAGE_SIZE;
		/* 
		 * Our ptes are contiguous
		 * so it's safe to just "++" here
		 */
		pte++;
	}
	xpq_flush_queue();
}


/*
 * Bootstrap helper functions
 */

/*
 * Mark a page readonly
 * XXX: assuming vaddr = paddr + KERNBASE
 */

static void
xen_bt_set_readonly (vaddr_t page)
{
	pt_entry_t entry;

	entry = xpmap_ptom_masked(page - KERNBASE);
	entry |= PG_k | PG_V;

	HYPERVISOR_update_va_mapping (page, entry, UVMF_INVLPG);
}

#ifdef __x86_64__
void
xen_set_user_pgd(paddr_t page)
{
	struct mmuext_op op;
	int s = splvm();

	xpq_flush_queue();
	op.cmd = MMUEXT_NEW_USER_BASEPTR;
	op.arg1.mfn = xpmap_phys_to_machine_mapping[page >> PAGE_SHIFT];
        if (HYPERVISOR_mmuext_op(&op, 1, NULL, DOMID_SELF) < 0)
		panic("xen_set_user_pgd: failed to install new user page"
			" directory %lx", page);
	splx(s);
}
#endif /* __x86_64__ */
