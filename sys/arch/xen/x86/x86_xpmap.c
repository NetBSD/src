/*	$NetBSD: x86_xpmap.c,v 1.8.4.3 2009/08/19 18:46:54 yamt Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: x86_xpmap.c,v 1.8.4.3 2009/08/19 18:46:54 yamt Exp $");

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
/* Xen requires the start_info struct to be page aligned */
union start_info_union start_info_union __aligned(PAGE_SIZE);
unsigned long *xpmap_phys_to_machine_mapping;

void xen_failsafe_handler(void);

#define HYPERVISOR_mmu_update_self(req, count, success_count) \
	HYPERVISOR_mmu_update((req), (count), (success_count), DOMID_SELF)

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
xpq_flush_queue(void)
{
	int i, ok;

	XENPRINTK2(("flush queue %p entries %d\n", xpq_queue, xpq_idx));
	for (i = 0; i < xpq_idx; i++)
		XENPRINTK2(("%d: %p %08" PRIx64 "\n", i,
		    (uint64_t)xpq_queue[i].ptr, (uint64_t)xpq_queue[i].val));
	if (xpq_idx != 0 &&
	    HYPERVISOR_mmu_update_self(xpq_queue, xpq_idx, &ok) < 0) {
		printf("xpq_flush_queue: %d entries \n", xpq_idx);
		for (i = 0; i < xpq_idx; i++)
			printf("0x%016" PRIx64 ": 0x%016" PRIx64 "\n",
			   (uint64_t)xpq_queue[i].ptr,
			   (uint64_t)xpq_queue[i].val);
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
	XENPRINTK2(("xpq_queue_machphys_update ma=0x%" PRIx64 " pa=0x%" PRIx64
	    "\n", (int64_t)ma, (int64_t)pa));
	xpq_queue[xpq_idx].ptr = ma | MMU_MACHPHYS_UPDATE;
	xpq_queue[xpq_idx].val = (pa - XPMAP_OFFSET) >> PAGE_SHIFT;
	xpq_increment_idx();
#ifdef XENDEBUG_SYNC
	xpq_flush_queue();
#endif
}

void
xpq_queue_pte_update(paddr_t ptr, pt_entry_t val)
{

	KASSERT((ptr & 3) == 0);
	xpq_queue[xpq_idx].ptr = (paddr_t)ptr | MMU_NORMAL_PT_UPDATE;
	xpq_queue[xpq_idx].val = val;
	xpq_increment_idx();
#ifdef XENDEBUG_SYNC
	xpq_flush_queue();
#endif
}

void
xpq_queue_pt_switch(paddr_t pa)
{
	struct mmuext_op op;
	xpq_flush_queue();

	XENPRINTK2(("xpq_queue_pt_switch: 0x%" PRIx64 " 0x%" PRIx64 "\n",
	    (int64_t)pa, (int64_t)pa));
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

	XENPRINTK2(("xpq_queue_pin_table: 0x%" PRIx64 " 0x%" PRIx64 "\n",
	    (int64_t)pa, (int64_t)pa));
	op.arg1.mfn = pa >> PAGE_SHIFT;

#if defined(__x86_64__)
	op.cmd = MMUEXT_PIN_L4_TABLE;
#else
	op.cmd = MMUEXT_PIN_L2_TABLE;
#endif
	if (HYPERVISOR_mmuext_op(&op, 1, NULL, DOMID_SELF) < 0)
		panic("xpq_queue_pin_table");
}

#ifdef PAE
static void
xpq_queue_pin_l3_table(paddr_t pa)
{
	struct mmuext_op op;
	xpq_flush_queue();

	XENPRINTK2(("xpq_queue_pin_l2_table: 0x%" PRIx64 " 0x%" PRIx64 "\n",
	    (int64_t)pa, (int64_t)pa));
	op.arg1.mfn = pa >> PAGE_SHIFT;

	op.cmd = MMUEXT_PIN_L3_TABLE;
	if (HYPERVISOR_mmuext_op(&op, 1, NULL, DOMID_SELF) < 0)
		panic("xpq_queue_pin_table");
}
#endif

void
xpq_queue_unpin_table(paddr_t pa)
{
	struct mmuext_op op;
	xpq_flush_queue();

	XENPRINTK2(("xpq_queue_unpin_table: 0x%" PRIx64 " 0x%" PRIx64 "\n",
	    (int64_t)pa, (int64_t)pa));
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
xpq_queue_tlb_flush(void)
{
	struct mmuext_op op;
	xpq_flush_queue();

	XENPRINTK2(("xpq_queue_tlb_flush\n"));
	op.cmd = MMUEXT_TLB_FLUSH_LOCAL;
	if (HYPERVISOR_mmuext_op(&op, 1, NULL, DOMID_SELF) < 0)
		panic("xpq_queue_tlb_flush");
}

void
xpq_flush_cache(void)
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
xpq_update_foreign(paddr_t ptr, pt_entry_t val, int dom)
{
	mmu_update_t op;
	int ok;
	xpq_flush_queue();

	op.ptr = ptr;
	op.val = val;
	if (HYPERVISOR_mmu_update(&op, 1, &ok, dom) < 0)
		return EFAULT;
	return (0);
}

#ifdef XENDEBUG
void
xpq_debug_dump(void)
{
	int i;

	XENPRINTK2(("idx: %d\n", xpq_idx));
	for (i = 0; i < xpq_idx; i++) {
		snprintf(XBUF, sizeof(XBUF), "%" PRIx64 " %08" PRIx64,
		    (uint64_t)xpq_queue[i].ptr, (uint64_t)xpq_queue[i].val);
		if (++i < xpq_idx)
			snprintf(XBUF + strlen(XBUF),
			    sizeof(XBUF) - strlen(XBUF),
			    "%" PRIx64 " %08" PRIx64,
			    (uint64_t)xpq_queue[i].ptr,
			    (uint64_t)xpq_queue[i].val);
		if (++i < xpq_idx)
			snprintf(XBUF + strlen(XBUF),
			    sizeof(XBUF) - strlen(XBUF),
			    "%" PRIx64 " %08" PRIx64, 
			    (uint64_t)xpq_queue[i].ptr,
			    (uint64_t)xpq_queue[i].val);
		if (++i < xpq_idx)
			snprintf(XBUF + strlen(XBUF),
			    sizeof(XBUF) - strlen(XBUF),
			    "%" PRIx64 " %08" PRIx64,
			    (uint64_t)xpq_queue[i].ptr,
			    (uint64_t)xpq_queue[i].val);
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

/* How many PDP do we need: */
#ifdef PAE
/*
 * For PAE, we consider a single contigous L2 "superpage" of 4 pages,
 * all of them mapped by the L3 page. We also need a shadow page
 * for L3[3].
 */
static const int l2_4_count = 6;
#else
static const int l2_4_count = PTP_LEVELS - 1;
#endif

vaddr_t
xen_pmap_bootstrap(void)
{
	int count, oldcount;
	long mapsize;
	vaddr_t bootstrap_tables, init_tables;

	xpmap_phys_to_machine_mapping =
	    (unsigned long *)xen_start_info.mfn_list;
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
	if (xendomain_is_dom0()) {
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
	paddr_t addr;
	vaddr_t page, avail, text_end, map_end;
	int i;
	extern char __data_start;

	__PRINTK(("xen_bootstrap_tables(0x%lx, 0x%lx, %d, %d)\n",
	    old_pgd, new_pgd, old_count, new_count));
	text_end = ((vaddr_t)&__data_start) & ~PAGE_MASK;
	/*
	 * size of R/W area after kernel text:
	 *  xencons_interface (if present)
	 *  xenstore_interface (if present)
	 *  table pages (new_count + l2_4_count entries)
	 * extra mappings (only when final is true):
	 *  UAREA
	 *  dummy user PGD (x86_64 only)/gdt page (i386 only)
	 *  HYPERVISOR_shared_info
	 *  ISA I/O mem (if needed)
	 */
	map_end = new_pgd + ((new_count + l2_4_count) * NBPG);
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
	if (final && xendomain_is_dom0()) {
		/* ISA I/O mem */
		map_end += IOM_SIZE;
	}
#endif /* DOM0OPS */

	__PRINTK(("xen_bootstrap_tables text_end 0x%lx map_end 0x%lx\n",
	    text_end, map_end));
	__PRINTK(("console 0x%lx ", xen_start_info.console.domU.mfn));
	__PRINTK(("xenstore 0x%lx\n", xen_start_info.store_mfn));

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

	addr = ((u_long) pdtpe) - KERNBASE;
	bt_pgd[pl4_pi(KERNTEXTOFF)] =
	    xpmap_ptom_masked(addr) | PG_k | PG_RW | PG_V;

	__PRINTK(("L3 va 0x%lx pa 0x%" PRIx64 " entry 0x%" PRIx64 " -> L4[0x%x]\n",
	    pdtpe, (uint64_t)addr, (uint64_t)bt_pgd[pl4_pi(KERNTEXTOFF)],
	    pl4_pi(KERNTEXTOFF)));
#else
	pdtpe = bt_pgd;
#endif /* PTP_LEVELS > 3 */

#if PTP_LEVELS > 2
	/* Level 2 */
	pde = (pd_entry_t *) avail;
	memset(pde, 0, PAGE_SIZE);
	avail += PAGE_SIZE;

	addr = ((u_long) pde) - KERNBASE;
	pdtpe[pl3_pi(KERNTEXTOFF)] =
	    xpmap_ptom_masked(addr) | PG_k | PG_V | PG_RW;
	__PRINTK(("L2 va 0x%lx pa 0x%" PRIx64 " entry 0x%" PRIx64 " -> L3[0x%x]\n",
	    pde, (int64_t)addr, (int64_t)pdtpe[pl3_pi(KERNTEXTOFF)],
	    pl3_pi(KERNTEXTOFF)));
#elif defined(PAE)
	/* our PAE-style level 2: 5 contigous pages (4 L2 + 1 shadow) */
	pde = (pd_entry_t *) avail;
	memset(pde, 0, PAGE_SIZE * 5);
	avail += PAGE_SIZE * 5;
	addr = ((u_long) pde) - KERNBASE;
	/*
	 * enter L2 pages in the L3.
	 * The real L2 kernel PD will be the last one (so that
	 * pde[L2_SLOT_KERN] always point to the shadow).
	 */
	for (i = 0; i < 3; i++, addr += PAGE_SIZE) {
		/*
		 * Xen doens't want R/W mappings in L3 entries, it'll add it
		 * itself.
		 */
		pdtpe[i] = xpmap_ptom_masked(addr) | PG_k | PG_V;
		__PRINTK(("L2 va 0x%lx pa 0x%" PRIx64 " entry 0x%" PRIx64
		    " -> L3[0x%x]\n", (vaddr_t)pde + PAGE_SIZE * i,
		    (int64_t)addr, (int64_t)pdtpe[i], i));
	}
	addr += PAGE_SIZE;
	pdtpe[3] = xpmap_ptom_masked(addr) | PG_k | PG_V;
	__PRINTK(("L2 va 0x%lx pa 0x%" PRIx64 " entry 0x%" PRIx64
	    " -> L3[0x%x]\n", (vaddr_t)pde + PAGE_SIZE * 4,
	    (int64_t)addr, (int64_t)pdtpe[3], 3));

#else /* PAE */
	pde = bt_pgd;
#endif /* PTP_LEVELS > 2 */

	/* Level 1 */
	page = KERNTEXTOFF;
	for (i = 0; i < new_count; i ++) {
		vaddr_t cur_page = page;

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
				    "va 0x%lx pte 0x%" PRIx64 "\n",
				    HYPERVISOR_shared_info, (int64_t)pte[pl1_pi(page)]));
			}
			if ((xpmap_ptom_masked(page - KERNBASE) >> PAGE_SHIFT)
			    == xen_start_info.console.domU.mfn) {
				xencons_interface = (void *)page;
				pte[pl1_pi(page)] = xen_start_info.console.domU.mfn;
				pte[pl1_pi(page)] <<= PAGE_SHIFT;
				__PRINTK(("xencons_interface "
				    "va 0x%lx pte 0x%" PRIx64 "\n",
				    xencons_interface, (int64_t)pte[pl1_pi(page)]));
			}
			if ((xpmap_ptom_masked(page - KERNBASE) >> PAGE_SHIFT)
			    == xen_start_info.store_mfn) {
				xenstore_interface = (void *)page;
				pte[pl1_pi(page)] = xen_start_info.store_mfn;
				pte[pl1_pi(page)] <<= PAGE_SHIFT;
				__PRINTK(("xenstore_interface "
				    "va 0x%lx pte 0x%" PRIx64 "\n",
				    xenstore_interface, (int64_t)pte[pl1_pi(page)]));
			}
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
			    page < new_pgd + ((new_count + l2_4_count) * PAGE_SIZE)) {
				/* map new page tables RO */
				pte[pl1_pi(page)] |= 0;
			} else {
				/* map page RW */
				pte[pl1_pi(page)] |= PG_RW;
			}
				
			if ((page  >= old_pgd && page < old_pgd + (old_count * PAGE_SIZE))
			    || page >= new_pgd) {
				__PRINTK(("va 0x%lx pa 0x%lx "
				    "entry 0x%" PRIx64 " -> L1[0x%x]\n",
				    page, page - KERNBASE,
				    (int64_t)pte[pl1_pi(page)], pl1_pi(page)));
			}
			page += PAGE_SIZE;
		}

		addr = ((u_long) pte) - KERNBASE;
		pde[pl2_pi(cur_page)] =
		    xpmap_ptom_masked(addr) | PG_k | PG_RW | PG_V;
		__PRINTK(("L1 va 0x%lx pa 0x%" PRIx64 " entry 0x%" PRIx64
		    " -> L2[0x%x]\n", pte, (int64_t)addr,
		    (int64_t)pde[pl2_pi(cur_page)], pl2_pi(cur_page)));
		/* Mark readonly */
		xen_bt_set_readonly((vaddr_t) pte);
	}

	/* Install recursive page tables mapping */
#ifdef PAE
	/*
	 * we need a shadow page for the kernel's L2 page
	 * The real L2 kernel PD will be the last one (so that
	 * pde[L2_SLOT_KERN] always point to the shadow.
	 */
	memcpy(&pde[L2_SLOT_KERN + NPDPG], &pde[L2_SLOT_KERN], PAGE_SIZE);
	pmap_kl2pd = &pde[L2_SLOT_KERN + NPDPG];
	pmap_kl2paddr = (u_long)pmap_kl2pd - KERNBASE;

	/*
	 * We don't enter a recursive entry from the L3 PD. Instead,
	 * we enter the first 4 L2 pages, which includes the kernel's L2
	 * shadow. But we have to entrer the shadow after switching
	 * %cr3, or Xen will refcount some PTE with the wrong type.
	 */
	addr = (u_long)pde - KERNBASE;
	for (i = 0; i < 3; i++, addr += PAGE_SIZE) {
		pde[PDIR_SLOT_PTE + i] = xpmap_ptom_masked(addr) | PG_k | PG_V;
		__PRINTK(("pde[%d] va 0x%lx pa 0x%lx entry 0x%" PRIx64 "\n",
		    (int)(PDIR_SLOT_PTE + i), pde + PAGE_SIZE * i, (long)addr,
		    (int64_t)pde[PDIR_SLOT_PTE + i]));
	}
#if 0
	addr += PAGE_SIZE; /* point to shadow L2 */
	pde[PDIR_SLOT_PTE + 3] = xpmap_ptom_masked(addr) | PG_k | PG_V;
	__PRINTK(("pde[%d] va 0x%lx pa 0x%lx entry 0x%" PRIx64 "\n",
	    (int)(PDIR_SLOT_PTE + 3), pde + PAGE_SIZE * 4, (long)addr,
	    (int64_t)pde[PDIR_SLOT_PTE + 3]));
#endif
	/* Mark tables RO, and pin the kernel's shadow as L2 */
	addr = (u_long)pde - KERNBASE;
	for (i = 0; i < 5; i++, addr += PAGE_SIZE) {
		xen_bt_set_readonly(((vaddr_t)pde) + PAGE_SIZE * i);
		if (i == 2 || i == 3)
			continue;
#if 0
		__PRINTK(("pin L2 %d addr 0x%" PRIx64 "\n", i, (int64_t)addr));
		xpq_queue_pin_table(xpmap_ptom_masked(addr));
#endif
	}
	if (final) {
		addr = (u_long)pde - KERNBASE + 3 * PAGE_SIZE;
		__PRINTK(("pin L2 %d addr 0x%" PRIx64 "\n", 2, (int64_t)addr));
		xpq_queue_pin_table(xpmap_ptom_masked(addr));
	}
#if 0
	addr = (u_long)pde - KERNBASE + 2 * PAGE_SIZE;
	__PRINTK(("pin L2 %d addr 0x%" PRIx64 "\n", 2, (int64_t)addr));
	xpq_queue_pin_table(xpmap_ptom_masked(addr));
#endif
#else /* PAE */
	/* recursive entry in higher-level PD */
	bt_pgd[PDIR_SLOT_PTE] =
	    xpmap_ptom_masked(new_pgd - KERNBASE) | PG_k | PG_V;
	__PRINTK(("bt_pgd[PDIR_SLOT_PTE] va 0x%lx pa 0x%" PRIx64
	    " entry 0x%" PRIx64 "\n", new_pgd, (int64_t)new_pgd - KERNBASE,
	    (int64_t)bt_pgd[PDIR_SLOT_PTE]));
	/* Mark tables RO */
	xen_bt_set_readonly((vaddr_t) pde);
#endif
#if PTP_LEVELS > 2 || defined(PAE)
	xen_bt_set_readonly((vaddr_t) pdtpe);
#endif
#if PTP_LEVELS > 3
	xen_bt_set_readonly(new_pgd);
#endif
	/* Pin the PGD */
	__PRINTK(("pin PGD\n"));
#ifdef PAE
	xpq_queue_pin_l3_table(xpmap_ptom_masked(new_pgd - KERNBASE));
#else
	xpq_queue_pin_table(xpmap_ptom_masked(new_pgd - KERNBASE));
#endif
#ifdef __i386__
	/* Save phys. addr of PDP, for libkvm. */
	PDPpaddr = (long)pde;
#ifdef PAE
	/* also save the address of the L3 page */
	pmap_l3pd = pdtpe;
	pmap_l3paddr = (new_pgd - KERNBASE);
#endif /* PAE */
#endif /* i386 */
	/* Switch to new tables */
	__PRINTK(("switch to PGD\n"));
	xpq_queue_pt_switch(xpmap_ptom_masked(new_pgd - KERNBASE));
	__PRINTK(("bt_pgd[PDIR_SLOT_PTE] now entry 0x%" PRIx64 "\n",
	    (int64_t)bt_pgd[PDIR_SLOT_PTE]));
#ifdef PAE
	if (final) {
		/* now enter kernel's PTE mappings */
		addr =  (u_long)pde - KERNBASE + PAGE_SIZE * 3;
		xpq_queue_pte_update(
		    xpmap_ptom(((vaddr_t)&pde[PDIR_SLOT_PTE + 3]) - KERNBASE), 
		    xpmap_ptom_masked(addr) | PG_k | PG_V);
		xpq_flush_queue();
	}
#endif



	/* Now we can safely reclaim space taken by old tables */
	
	__PRINTK(("unpin old PGD\n"));
	/* Unpin old PGD */
	xpq_queue_unpin_table(xpmap_ptom_masked(old_pgd - KERNBASE));
	/* Mark old tables RW */
	page = old_pgd;
	addr = (paddr_t) pde[pl2_pi(page)] & PG_FRAME;
	addr = xpmap_mtop(addr);
	pte = (pd_entry_t *) ((u_long)addr + KERNBASE);
	pte += pl1_pi(page);
	__PRINTK(("*pde 0x%" PRIx64 " addr 0x%" PRIx64 " pte 0x%lx\n",
	    (int64_t)pde[pl2_pi(page)], (int64_t)addr, (long)pte));
	while (page < old_pgd + (old_count * PAGE_SIZE) && page < map_end) {
		addr = xpmap_ptom(((u_long) pte) - KERNBASE);
		XENPRINTK(("addr 0x%" PRIx64 " pte 0x%lx *pte 0x%" PRIx64 "\n",
		   (int64_t)addr, (long)pte, (int64_t)*pte));
		xpq_queue_pte_update(addr, *pte | PG_RW);
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
