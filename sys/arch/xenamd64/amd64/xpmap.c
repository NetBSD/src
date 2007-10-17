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

#include <sys/types.h>
#include <sys/param.h>

#include <uvm/uvm.h>

#include <machine/pmap.h>
#include <machine/pte.h>
#include <xen/xen3-public/xen.h>
#include <xen/hypervisor.h>
#include <xen/xen.h>
#include <machine/cpufunc.h>

volatile shared_info_t *HYPERVISOR_shared_info;
union start_info_union start_info_union;

extern volatile struct xencons_interface *xencons_interface; /* XXX */
extern struct xenstore_domain_interface *xenstore_interface; /* XXX */

void xen_failsafe_handler(void);

static vaddr_t xen_arch_pmap_bootstrap(vaddr_t);
static void xen_bt_set_readonly (vaddr_t);
static void xen_bootstrap_tables (vaddr_t, vaddr_t, int, int);
static void xen_set_pgd(paddr_t);

static void xpq_dump (void);

void
xen_failsafe_handler(void)
{
	panic("xen_failsafe_handler called!\n");
}


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

vaddr_t xen_pmap_bootstrap (vaddr_t);

vaddr_t xen_pmap_bootstrap (vaddr_t first_avail) 
{
	printk("xen_pmap_bootstrap first_avail=0x%lx\n", first_avail);
	/* Make machine to physical mapping available */
	xpmap_phys_to_machine_mapping = (paddr_t *) xen_start_info.mfn_list;
 
	/* Round first_avail to PAGE_SIZE */
	first_avail = round_page (first_avail);  
	return xen_arch_pmap_bootstrap(first_avail);
}


/*
 * Function to get rid of Xen bootstrap tables
 */

static vaddr_t
xen_arch_pmap_bootstrap(vaddr_t first_avail)
{
	int count;
	vaddr_t bootstrap_tables, init_tables;

	printk("xen_arch_pmap_bootstrap(%lx)\n", first_avail);

	/* XXX: using old xen_start_info seems broken, use Xen PGD instead */
	init_tables = xen_start_info.pt_base;
	printk("xen_arch_pmap_bootstrap init_tables=0x%lx\n", init_tables);

	/* Space after Xen boostrap tables should be free */
	bootstrap_tables = xen_start_info.pt_base +
		(xen_start_info.nr_pt_frames * PAGE_SIZE);

	/* Calculate how many tables we need */
	count = TABLE_L2_ENTRIES;

	/* 
	 * Xen space we'll reclaim may not be enough for our new page tables,
	 * move bootstrap tables if necessary
	 */

	if (bootstrap_tables < init_tables + ((count + 3) * PAGE_SIZE))
		bootstrap_tables = init_tables + ((count + 3) * PAGE_SIZE);

	HYPERVISOR_shared_info = 0;
	xencons_interface = 0;
	xenstore_interface = 0;
	/* Create temporary tables */
	xen_bootstrap_tables(xen_start_info.pt_base, bootstrap_tables,
		xen_start_info.nr_pt_frames, count);

	/* get vaddr space for the shared info and the console pages */
	HYPERVISOR_shared_info = (struct shared_info *)first_avail;
	first_avail += PAGE_SIZE;
	if ((xen_start_info.flags & SIF_INITDOMAIN) == 0) {
		xencons_interface = (struct xencons_interface *)first_avail;
		first_avail += PAGE_SIZE;
		xenstore_interface = (struct xenstore_domain_interface *)first_avail;
		first_avail += PAGE_SIZE;
	}

	/* Create final tables */
	xen_bootstrap_tables(bootstrap_tables, first_avail, count + 3, count);

	return (first_avail + ((count + 3) * PAGE_SIZE));
}


/*
 * Build a new table and switch to it
 * old_count is # of old tables (including PGD, PDTPE and PDE)
 * new_count is # of new tables (PTE only)
 * we assume areas don't overlap
 */


static void
xen_bootstrap_tables (vaddr_t old_pgd, vaddr_t new_pgd,
	int old_count, int new_count)
{
	pd_entry_t *pdtpe, *pde, *pte;
	pd_entry_t *cur_pgd, *bt_pgd;
	paddr_t addr, page;
	vaddr_t avail, text_end, map_end;
	int i;
	extern char _etext;

	printk("xen_bootstrap_tables(0x%lx, 0x%lx, %d, %d)\n",
	    old_pgd, new_pgd, old_count, new_count);
	text_end = ((vaddr_t)&_etext + PAGE_MASK) & ~PAGE_MASK;
	/*
	 * size of R/W area after kernel text:
	 * table pages
	 * UAREA 
	 * dummy user PGD
	 */
	map_end = new_pgd + (((new_count + 3) + UPAGES + 1) * NBPG);
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
			if (page == (vaddr_t)xencons_interface) {
				pte[pl1_pi(page)] =
				    (xen_start_info.console_mfn << PAGE_SHIFT);
				printk("xencons_interface va 0x%lx pte 0x%lx\n", xencons_interface, pte[pl1_pi(page)]);
			}
			if (page == (vaddr_t)xenstore_interface) {
				pte[pl1_pi(page)] =
				    (xen_start_info.store_mfn << PAGE_SHIFT);
				printk("xenstore_interface va 0x%lx pte 0x%lx\n", xenstore_interface, pte[pl1_pi(page)]);
			}
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
	xen_pgd_pin(new_pgd - KERNBASE);
	/* Switch to new tables */
	printk("switch to PDG\n");
	xen_set_pgd(new_pgd - KERNBASE);
	printk("bt_pgd[PDIR_SLOT_PTE] now entry 0x%lx\n",bt_pgd[PDIR_SLOT_PTE]);
	printk("L4_BASE va 0x%lx\n", (long)L4_BASE);
	printk("value 0x%lx\n", *L4_BASE);
	printk("[PDIR_SLOT_PTE] 0x%lx\n", L4_BASE[PDIR_SLOT_PTE]);

	/* Now we can safely reclaim space taken by old tables */
	
	printk("unpin old PDG\n");
	/* Unpin old PGD */
	xen_pgd_unpin(old_pgd - KERNBASE);
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
xen_pgd_pin(paddr_t page)
{
	int ret;
        struct mmuext_op op;
        op.cmd = MMUEXT_PIN_L4_TABLE;
        op.arg1.mfn = xpmap_phys_to_machine_mapping[page >> PAGE_SHIFT];
        ret = HYPERVISOR_mmuext_op(&op, 1, NULL, DOMID_SELF);
	if (ret) {
		printk("MMUEXT_PIN_L4_TABLE ret %d\n", ret);
		panic("MMUEXT_PIN_L4_TABLE");
	}
}

void
xen_pgd_unpin(paddr_t page)
{
	int ret;
        struct mmuext_op op;
        op.cmd = MMUEXT_UNPIN_TABLE;
        op.arg1.mfn = xpmap_phys_to_machine_mapping[page >> PAGE_SHIFT];
        ret = HYPERVISOR_mmuext_op(&op, 1, NULL, DOMID_SELF);
	if (ret) {
		printk("MMUEXT_UNPIN_TABLE ret %d\n", ret);
		panic("MMUEXT_UNPIN_TABLE");
	}
}

static void
xen_set_pgd(paddr_t page)
{
        struct mmuext_op op;
        op.cmd = MMUEXT_NEW_BASEPTR;
        op.arg1.mfn = xpmap_phys_to_machine_mapping[page >> PAGE_SHIFT];
        if (HYPERVISOR_mmuext_op(&op, 1, NULL, DOMID_SELF) < 0)
		panic("xen_set_pgd: failed to install new page directory %lx",
			page);
}

void
xen_set_user_pgd(paddr_t page)
{
	struct mmuext_op op;
	op.cmd = MMUEXT_NEW_USER_BASEPTR;
	op.arg1.mfn = xpmap_phys_to_machine_mapping[page >> PAGE_SHIFT];
        if (HYPERVISOR_mmuext_op(&op, 1, NULL, DOMID_SELF) < 0)
		panic("xen_set_user_pgd: failed to install new user page"
			" directory %lx", page);
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


/*
 * Xen pmap helper functions
 */

/*
 * MMU update ops queues, ported from NetBSD/xen (i386)
 */


#ifndef XPQUEUE_SIZE
#define XPQUEUE_SIZE 2048
#endif

static mmu_update_t xpq_queue[XPQUEUE_SIZE];
static int xpq_idx = 0;

void
xpq_flush_queue()
{
        int ok;

        if (xpq_idx != 0 &&
            HYPERVISOR_mmu_update(xpq_queue, xpq_idx, &ok, DOMID_SELF) < 0) {
		xpq_dump();
                panic("xpq_flush_queue(): HYPERVISOR_mmu_update failed\n");
	}

        xpq_idx = 0;
}

static inline void
xpq_increment_idx(void)
{
        xpq_idx++;
        if (xpq_idx == XPQUEUE_SIZE)
                xpq_flush_queue();
}

void
xpq_queue_pte_update(pt_entry_t *ptr, pt_entry_t val)
{
        xpq_queue[xpq_idx].ptr = (paddr_t)ptr | MMU_NORMAL_PT_UPDATE;
        xpq_queue[xpq_idx].val = val;
        xpq_increment_idx();
}

void
xpq_queue_tlb_flush()
{
        struct mmuext_op op;
        xpq_flush_queue();

        op.cmd = MMUEXT_TLB_FLUSH_LOCAL;
        if (HYPERVISOR_mmuext_op(&op, 1, NULL, DOMID_SELF) < 0)
                panic("xpq_queue_tlb_flush(): local TLB flush failed");
}

void
xpq_flush_cache()
{
        struct mmuext_op op;
        int s = splvm();
        xpq_flush_queue();

        op.cmd = MMUEXT_FLUSH_CACHE;
        if (HYPERVISOR_mmuext_op(&op, 1, NULL, DOMID_SELF) < 0)
                panic("xpq_flush_cache(): cache flush failed");
        splx(s);
}

void
xpq_queue_invlpg(vaddr_t va)
{
        struct mmuext_op op;
        xpq_flush_queue();

        op.cmd = MMUEXT_INVLPG_LOCAL;
        op.arg1.linear_addr = (va & PG_FRAME);
        if (HYPERVISOR_mmuext_op(&op, 1, NULL, DOMID_SELF) < 0) {
		printf("xpq_queue_invlpg(): unable to invalidate va %lx\n", va);
                panic("xpq_queue_invlpg(): tlb invalidation failed");
	}
}

static void
xpq_dump(void)
{
	int i;

	printf("<<< xpq queue dump >>>\n");

	for (i = 0; i < xpq_idx; i++)
		printf("%d: ptr %lx, val %lx (pa %lx ma %lx)\n", i, 
			xpq_queue[i].ptr, xpq_queue[i].val,
			xpmap_mtop(xpq_queue[i].val & PG_FRAME),
			xpq_queue[i].val & PG_FRAME);

	printf("<<< end of xpq queue dump >>>\n");
}
