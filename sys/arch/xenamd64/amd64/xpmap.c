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

#include "opt_xen.h"

#include <sys/types.h>
#include <sys/param.h>

#include <uvm/uvm.h>

#include <machine/pmap.h>
#include <machine/pte.h>
#include <xen/xen3-public/xen.h>
#include <xen/hypervisor.h>
#include <xen/xen.h>
#include <machine/cpufunc.h>

#include <dev/isa/isareg.h>
#include <machine/isa_machdep.h>

extern volatile struct xencons_interface *xencons_interface; /* XXX */
extern struct xenstore_domain_interface *xenstore_interface; /* XXX */

static vaddr_t xen_arch_pmap_bootstrap(void);
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

vaddr_t
xen_pmap_bootstrap() 
{
	printk("xen_pmap_bootstrap\n");
	/* Make machine to physical mapping available */
	xpmap_phys_to_machine_mapping = (paddr_t *) xen_start_info.mfn_list;
 
	/* Round first_avail to PAGE_SIZE */
	return xen_arch_pmap_bootstrap();
}


/*
 * Function to get rid of Xen bootstrap tables
 */

static vaddr_t
xen_arch_pmap_bootstrap()
{
	int count, iocount = 0;
	vaddr_t bootstrap_tables, init_tables;

	init_tables = xen_start_info.pt_base;
	printk("xen_arch_pmap_bootstrap init_tables=0x%lx\n", init_tables);

	/* Space after Xen boostrap tables should be free */
	bootstrap_tables = xen_start_info.pt_base +
		(xen_start_info.nr_pt_frames * PAGE_SIZE);

	/* Calculate how many tables we need */
	count = TABLE_L2_ENTRIES;

#ifdef DOM0OPS
	if (xen_start_info.flags & SIF_INITDOMAIN) {
		/* space for ISA I/O mem */
		iocount = IOM_SIZE / PAGE_SIZE;
	}
#endif

	/* 
	 * Xen space we'll reclaim may not be enough for our new page tables,
	 * move bootstrap tables if necessary
	 */

	if (bootstrap_tables < init_tables + ((count+3+iocount) * PAGE_SIZE))
		bootstrap_tables = init_tables +
					((count+3+iocount) * PAGE_SIZE);

	/* Create temporary tables */
	xen_bootstrap_tables(xen_start_info.pt_base, bootstrap_tables,
		xen_start_info.nr_pt_frames, count, 0);

	/* get vaddr space for the shared info and the console pages */

	/* Create final tables */
	xen_bootstrap_tables(bootstrap_tables, init_tables,
	    count + 3, count, 1);

	return (init_tables + ((count + 3) * PAGE_SIZE));
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

	printk("xen_bootstrap_tables(0x%lx, 0x%lx, %d, %d)\n",
	    old_pgd, new_pgd, old_count, new_count);
	text_end = ((vaddr_t)&__data_start) & ~PAGE_MASK;
	/*
	 * size of R/W area after kernel text:
	 *  xencons_interface (if present)
	 *  xenstore_interface (if present)
	 *  table pages (new_count + 3 entries)
	 *  UAREA 
	 *  dummy user PGD
	 * extra mappings (only when final is true):
	 *  HYPERVISOR_shared_info
	 *  ISA I/O mem (if needed)
	 */
	map_end = new_pgd + ((new_count + 3 + UPAGES + 1) * NBPG);
	if (final) {
		HYPERVISOR_shared_info = (struct shared_info *)map_end;
		map_end += NBPG;
	}
#ifdef DOM0OPS
	if (final && (xen_start_info.flags & SIF_INITDOMAIN)) {
		/* ISA I/O mem */
		atdevbase = map_end;
		map_end += IOM_SIZE;
	}
#endif /* DOM0OPS */

	printk("xen_bootstrap_tables text_end 0x%lx map_end 0x%lx\n", text_end,
	map_end);

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

	/* Install level 3 */
	pdtpe = (pd_entry_t *) avail;
	memset (pdtpe, 0, PAGE_SIZE);
	avail += PAGE_SIZE;

	addr = ((paddr_t) pdtpe) - KERNBASE;
	bt_pgd[pl4_pi(KERNTEXTOFF)] =
	    xpmap_ptom_masked(addr) | PG_u | PG_RW | PG_V;

	printk("L3 va 0x%lx pa 0x%lx entry 0x%lx -> L4[0x%x]\n", pdtpe, addr, bt_pgd[pl4_pi(KERNTEXTOFF)], pl4_pi(KERNTEXTOFF));

	/* Level 2 */
	pde = (pd_entry_t *) avail;
	memset(pde, 0, PAGE_SIZE);
	avail += PAGE_SIZE;

	addr = ((paddr_t) pde) - KERNBASE;
	pdtpe[pl3_pi(KERNTEXTOFF)] =
	    xpmap_ptom_masked(addr) | PG_u | PG_RW | PG_V;
	printk("L2 va 0x%lx pa 0x%lx entry 0x%lx -> L3[0x%x]\n", pde, addr, pdtpe[pl3_pi(KERNTEXTOFF)], pl3_pi(KERNTEXTOFF));

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
				printk("HYPERVISOR_shared_info va 0x%lx pte 0x%lx\n", HYPERVISOR_shared_info, pte[pl1_pi(page)]);
			}
			if (xpmap_ptom_masked(page - KERNBASE) ==
			    (xen_start_info.console_mfn << PAGE_SHIFT)) {
				xencons_interface = (void *)page;
				pte[pl1_pi(page)] =
				    (xen_start_info.console_mfn << PAGE_SHIFT);
				printk("xencons_interface va 0x%lx pte 0x%lx\n", xencons_interface, pte[pl1_pi(page)]);
			}
			if (xpmap_ptom_masked(page - KERNBASE) ==
			    (xen_start_info.store_mfn << PAGE_SHIFT)) {
				xenstore_interface = (void *)page;
				pte[pl1_pi(page)] =
				    (xen_start_info.store_mfn << PAGE_SHIFT);
				printk("xenstore_interface va 0x%lx pte 0x%lx\n", xenstore_interface, pte[pl1_pi(page)]);
			}
#ifdef DOM0OPS
			if (page >= (vaddr_t)atdevbase &&
			    page < (vaddr_t)atdevbase + IOM_SIZE) {
				pte[pl1_pi(page)] =
				    IOM_BEGIN + (page - (vaddr_t)atdevbase);
			}
#endif
			pte[pl1_pi(page)] |= PG_u | PG_V;
			if (page < text_end) {
				/* map kernel text RO */
				pte[pl1_pi(page)] |= 0;
			} else if (page >= old_pgd
			    && page < old_pgd + (old_count * PAGE_SIZE)) {
				/* map old page tables RO */
				pte[pl1_pi(page)] |= 0;
			} else if (page >= new_pgd &&
			    page < new_pgd + ((new_count + 3) * PAGE_SIZE)) {
				/* map new page tables RO */
				pte[pl1_pi(page)] |= 0;
			} else {
				/* map page RW */
				pte[pl1_pi(page)] |= PG_RW;
			}
			if (page  == old_pgd)
				printk("va 0x%lx pa 0x%lx entry 0x%lx -> L1[0x%x]\n", page, page - KERNBASE, pte[pl1_pi(page)], pl1_pi(page));
			page += PAGE_SIZE;
		}

		addr = ((paddr_t) pte) - KERNBASE;
		pde[pl2_pi(cur_page)] =
		    xpmap_ptom_masked(addr) | PG_u | PG_RW | PG_V;
		printk("L1 va 0x%lx pa 0x%lx entry 0x%lx -> L2[0x%x]\n", pte, addr, pde[pl2_pi(cur_page)], pl2_pi(cur_page));
		/* Mark readonly */
		xen_bt_set_readonly((vaddr_t) pte);
	}

	/* Install recursive page tables mapping */
	bt_pgd[PDIR_SLOT_PTE] =
	    xpmap_ptom_masked(new_pgd - KERNBASE) | PG_u | PG_V;
	printk("bt_pgd[PDIR_SLOT_PTE] va 0x%lx pa 0x%lx entry 0x%lx\n", new_pgd, new_pgd - KERNBASE, bt_pgd[PDIR_SLOT_PTE]);

	/* Mark tables RO */
	xen_bt_set_readonly((vaddr_t) pde);
	xen_bt_set_readonly((vaddr_t) pdtpe);
	xen_bt_set_readonly(new_pgd);
	/* Pin the PGD */
	printk("pin PDG\n");
	xpq_queue_pin_table(xpmap_ptom_masked(new_pgd - KERNBASE));
	/* Switch to new tables */
	printk("switch to PDG\n");
	xpq_queue_pt_switch(xpmap_ptom_masked(new_pgd - KERNBASE));
	printk("bt_pgd[PDIR_SLOT_PTE] now entry 0x%lx\n",bt_pgd[PDIR_SLOT_PTE]);
	printk("L4_BASE va 0x%lx\n", (long)L4_BASE);
	printk("value 0x%lx\n", *L4_BASE);
	printk("[PDIR_SLOT_PTE] 0x%lx\n", L4_BASE[PDIR_SLOT_PTE]);

	/* Now we can safely reclaim space taken by old tables */
	
	printk("unpin old PDG\n");
	/* Unpin old PGD */
	xpq_queue_unpin_table(xpmap_ptom_masked(old_pgd - KERNBASE));
	/* Mark old tables RW */
	page = old_pgd;
	addr = (paddr_t) pde[pl2_pi(page)] & PG_FRAME;
	addr = xpmap_mtop(addr);
	pte = (pd_entry_t *) (addr + KERNBASE);
	pte += pl1_pi(page);
	printk("*pde 0x%lx addr 0x%lx pte 0x%lx\n", pde[pl2_pi(page)], addr, pte);
	while (page < old_pgd + (old_count * PAGE_SIZE) && page < map_end) {
		addr = xpmap_ptom(((paddr_t) pte) - KERNBASE);
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
	entry |= PG_u | PG_V;

	HYPERVISOR_update_va_mapping (page, entry, UVMF_INVLPG);
}
