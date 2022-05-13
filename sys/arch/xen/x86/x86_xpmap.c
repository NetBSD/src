/*	$NetBSD: x86_xpmap.c,v 1.84.4.2 2022/05/13 11:12:49 martin Exp $	*/

/*
 * Copyright (c) 2017 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Maxime Villard.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
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

/*
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
__KERNEL_RCSID(0, "$NetBSD: x86_xpmap.c,v 1.84.4.2 2022/05/13 11:12:49 martin Exp $");

#include "opt_xen.h"
#include "opt_ddb.h"
#include "ksyms.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/mutex.h>
#include <sys/cpu.h>

#include <uvm/uvm.h>

#include <x86/pmap.h>
#include <machine/gdt.h>
#include <xen/xenfunc.h>

#include <dev/isa/isareg.h>
#include <machine/isa_machdep.h>

#ifdef XENDEBUG
#define	__PRINTK(x) printk x
#else
#define	__PRINTK(x)
#endif

/* Xen requires the start_info struct to be page aligned */
union start_info_union start_info_union __aligned(PAGE_SIZE);

volatile shared_info_t *HYPERVISOR_shared_info __read_mostly;
unsigned long *xpmap_phys_to_machine_mapping __read_mostly;
kmutex_t pte_lock __cacheline_aligned;
vaddr_t xen_dummy_page;
pt_entry_t xpmap_pg_nx __read_mostly;

#define XPQUEUE_SIZE 2048
static mmu_update_t xpq_queue_array[MAXCPUS][XPQUEUE_SIZE];

void xen_failsafe_handler(void);

extern volatile struct xencons_interface *xencons_interface; /* XXX */
extern struct xenstore_domain_interface *xenstore_interface; /* XXX */

static void xen_bt_set_readonly(vaddr_t);
static void xen_bootstrap_tables(vaddr_t, vaddr_t, size_t, size_t, bool);

vaddr_t xen_locore(void);

/*
 * kcpuset internally uses an array of uint32_t while xen uses an array of
 * u_long. As we're little-endian we can cast one to the other.
 */
typedef union {
#ifdef _LP64
	uint32_t xcpum_km[2];
#else
	uint32_t xcpum_km[1];
#endif
	u_long xcpum_xm;
} xcpumask_t;

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
		pmap_pte_clearbits(ptp, PTE_W);
	}
	s = splvm(); /* XXXSMP */
	xpq_queue_set_ldt(base, entries);
	splx(s);
}

void
xpq_flush_queue(void)
{
	mmu_update_t *xpq_queue;
	int done = 0, ret;
	size_t xpq_idx;

	xpq_idx = curcpu()->ci_xpq_idx;
	xpq_queue = xpq_queue_array[curcpu()->ci_cpuid];

retry:
	ret = HYPERVISOR_mmu_update(xpq_queue, xpq_idx, &done, DOMID_SELF);

	if (ret < 0 && xpq_idx != 0) {
		printf("xpq_flush_queue: %zu entries (%d successful) on "
		    "cpu%d (%ld)\n",
		    xpq_idx, done, curcpu()->ci_index, curcpu()->ci_cpuid);

		if (done != 0) {
			xpq_queue += done;
			xpq_idx -= done;
			done = 0;
			goto retry;
		}

		panic("HYPERVISOR_mmu_update failed, ret: %d\n", ret);
	}
	curcpu()->ci_xpq_idx = 0;
}

static inline void
xpq_increment_idx(void)
{

	if (__predict_false(++curcpu()->ci_xpq_idx == XPQUEUE_SIZE))
		xpq_flush_queue();
}

void
xpq_queue_machphys_update(paddr_t ma, paddr_t pa)
{
	mmu_update_t *xpq_queue = xpq_queue_array[curcpu()->ci_cpuid];
	size_t xpq_idx = curcpu()->ci_xpq_idx;

	xpq_queue[xpq_idx].ptr = ma | MMU_MACHPHYS_UPDATE;
	xpq_queue[xpq_idx].val = pa >> PAGE_SHIFT;
	xpq_increment_idx();
}

void
xpq_queue_pte_update(paddr_t ptr, pt_entry_t val)
{
	mmu_update_t *xpq_queue = xpq_queue_array[curcpu()->ci_cpuid];
	size_t xpq_idx = curcpu()->ci_xpq_idx;

	xpq_queue[xpq_idx].ptr = ptr | MMU_NORMAL_PT_UPDATE;
	xpq_queue[xpq_idx].val = val;
	xpq_increment_idx();
}

void
xpq_queue_pt_switch(paddr_t pa)
{
	struct mmuext_op op;

	xpq_flush_queue();

	op.cmd = MMUEXT_NEW_BASEPTR;
	op.arg1.mfn = pa >> PAGE_SHIFT;
	if (HYPERVISOR_mmuext_op(&op, 1, NULL, DOMID_SELF) < 0)
		panic(__func__);
}

void
xpq_queue_pin_table(paddr_t pa, int lvl)
{
	struct mmuext_op op;

	xpq_flush_queue();

	op.cmd = lvl;
	op.arg1.mfn = pa >> PAGE_SHIFT;
	if (HYPERVISOR_mmuext_op(&op, 1, NULL, DOMID_SELF) < 0)
		panic(__func__);
}

void
xpq_queue_unpin_table(paddr_t pa)
{
	struct mmuext_op op;

	xpq_flush_queue();

	op.cmd = MMUEXT_UNPIN_TABLE;
	op.arg1.mfn = pa >> PAGE_SHIFT;
	if (HYPERVISOR_mmuext_op(&op, 1, NULL, DOMID_SELF) < 0)
		panic(__func__);
}

void
xpq_queue_set_ldt(vaddr_t va, uint32_t entries)
{
	struct mmuext_op op;

	xpq_flush_queue();

	KASSERT(va == (va & ~PAGE_MASK));
	op.cmd = MMUEXT_SET_LDT;
	op.arg1.linear_addr = va;
	op.arg2.nr_ents = entries;
	if (HYPERVISOR_mmuext_op(&op, 1, NULL, DOMID_SELF) < 0)
		panic(__func__);
}

void
xpq_queue_tlb_flush(void)
{
	struct mmuext_op op;

	xpq_flush_queue();

	op.cmd = MMUEXT_TLB_FLUSH_LOCAL;
	if (HYPERVISOR_mmuext_op(&op, 1, NULL, DOMID_SELF) < 0)
		panic(__func__);
}

void
xpq_flush_cache(void)
{
	int s = splvm(); /* XXXSMP */

	xpq_flush_queue();

	asm("wbinvd":::"memory");
	splx(s); /* XXX: removeme */
}

void
xpq_queue_invlpg(vaddr_t va)
{
	struct mmuext_op op;

	xpq_flush_queue();

	op.cmd = MMUEXT_INVLPG_LOCAL;
	op.arg1.linear_addr = (va & ~PAGE_MASK);
	if (HYPERVISOR_mmuext_op(&op, 1, NULL, DOMID_SELF) < 0)
		panic(__func__);
}

void
xen_mcast_invlpg(vaddr_t va, kcpuset_t *kc)
{
	xcpumask_t xcpumask;
	mmuext_op_t op;

	kcpuset_export_u32(kc, &xcpumask.xcpum_km[0], sizeof(xcpumask));

	xpq_flush_queue();

	op.cmd = MMUEXT_INVLPG_MULTI;
	op.arg1.linear_addr = va;
	set_xen_guest_handle(op.arg2.vcpumask, &xcpumask.xcpum_xm);

	if (HYPERVISOR_mmuext_op(&op, 1, NULL, DOMID_SELF) < 0)
		panic(__func__);
}

void
xen_bcast_invlpg(vaddr_t va)
{
	mmuext_op_t op;

	xpq_flush_queue();

	op.cmd = MMUEXT_INVLPG_ALL;
	op.arg1.linear_addr = va;

	if (HYPERVISOR_mmuext_op(&op, 1, NULL, DOMID_SELF) < 0)
		panic(__func__);
}

/* This is a synchronous call. */
void
xen_mcast_tlbflush(kcpuset_t *kc)
{
	xcpumask_t xcpumask;
	mmuext_op_t op;

	kcpuset_export_u32(kc, &xcpumask.xcpum_km[0], sizeof(xcpumask));

	xpq_flush_queue();

	op.cmd = MMUEXT_TLB_FLUSH_MULTI;
	set_xen_guest_handle(op.arg2.vcpumask, &xcpumask.xcpum_xm);

	if (HYPERVISOR_mmuext_op(&op, 1, NULL, DOMID_SELF) < 0)
		panic(__func__);
}

/* This is a synchronous call. */
void
xen_bcast_tlbflush(void)
{
	mmuext_op_t op;

	xpq_flush_queue();

	op.cmd = MMUEXT_TLB_FLUSH_ALL;

	if (HYPERVISOR_mmuext_op(&op, 1, NULL, DOMID_SELF) < 0)
		panic(__func__);
}

void
xen_copy_page(paddr_t srcpa, paddr_t dstpa)
{
	mmuext_op_t op;

	op.cmd = MMUEXT_COPY_PAGE;
	op.arg1.mfn = xpmap_ptom(dstpa) >> PAGE_SHIFT;
	op.arg2.src_mfn = xpmap_ptom(srcpa) >> PAGE_SHIFT;

	if (HYPERVISOR_mmuext_op(&op, 1, NULL, DOMID_SELF) < 0)
		panic(__func__);
}

void
xen_pagezero(paddr_t pa)
{
	mmuext_op_t op;

	op.cmd = MMUEXT_CLEAR_PAGE;
	op.arg1.mfn = xpmap_ptom(pa) >> PAGE_SHIFT;

	if (HYPERVISOR_mmuext_op(&op, 1, NULL, DOMID_SELF) < 0)
		panic(__func__);
}

int
xpq_update_foreign(paddr_t ptr, pt_entry_t val, int dom, u_int flags)
{
	mmu_update_t op;
	int ok;
	int err;

	xpq_flush_queue();

	op.ptr = ptr;
	if (flags & PMAP_MD_XEN_NOTR)
		op.ptr |= MMU_PT_UPDATE_NO_TRANSLATE;
	op.val = val;
	/*
	 * here we return a negative error number as Xen error to
	 * pmap_enter_ma. only calls from privcmd.c should end here, and
	 * it can deal with it.
	 */
	if ((err = HYPERVISOR_mmu_update(&op, 1, &ok, dom)) < 0) {
		return err;
	}
	return 0;
}

#if L2_SLOT_KERNBASE > 0
#define TABLE_L2_ENTRIES (2 * (NKL2_KIMG_ENTRIES + 1))
#else
#define TABLE_L2_ENTRIES (NKL2_KIMG_ENTRIES + 1)
#endif

#ifdef __x86_64__
#define PDIRSZ	PTP_LEVELS
#else
/*
 * For PAE, we need an L3 page, a single contiguous L2 "superpage" of 4 pages
 * (all of them mapped by the L3 page), and a shadow page for L3[3].
 */
#define PDIRSZ	(1 + 4 + 1)
#endif

/*
 * Xen locore: get rid of the Xen bootstrap tables. Build and switch to new page
 * tables.
 *
 * Virtual address space of the kernel when leaving this function:
 * +--------------+------------------+-------------+------------+---------------
 * | KERNEL IMAGE | BOOTSTRAP TABLES | PROC0 UAREA | DUMMY PAGE | HYPER. SHARED
 * +--------------+------------------+-------------+------------+---------------
 *
 * ------+-----------------+-------------+
 *  INFO | EARLY ZERO PAGE | ISA I/O MEM |
 * ------+-----------------+-------------+
 *
 * DUMMY PAGE is either a PDG for amd64 or a GDT for i386.
 *
 * (HYPER. SHARED INFO + EARLY ZERO PAGE + ISA I/O MEM) have no physical
 * addresses preallocated.
 */
vaddr_t
xen_locore(void)
{
	size_t nL2, oldcount, mapsize;
	vaddr_t our_tables, xen_tables;
	u_int descs[4];

	xen_init_features();

	xpmap_phys_to_machine_mapping =
	    (unsigned long *)xen_start_info.mfn_list;

	/* Set the NX/XD bit, if available. descs[3] = %edx. */
	x86_cpuid(0x80000001, descs);
	xpmap_pg_nx = (descs[3] & CPUID_NOX) ? PG_NX : 0;

	/* Space after Xen boostrap tables should be free */
	xen_tables = xen_start_info.pt_base;
	our_tables = xen_tables + (xen_start_info.nr_pt_frames * PAGE_SIZE);

	/*
	 * Calculate how much space we need. First, everything mapped before
	 * the Xen bootstrap tables.
	 */
	mapsize = xen_tables - KERNTEXTOFF;

	/* After the tables we'll have:
	 *  - UAREA
	 *  - dummy user PGD (x86_64)
	 *  - HYPERVISOR_shared_info
	 *  - early_zerop
	 *  - ISA I/O mem (if needed)
	 */
	mapsize += UPAGES * PAGE_SIZE;
#ifdef __x86_64__
	mapsize += PAGE_SIZE;
#endif
	mapsize += PAGE_SIZE;
	mapsize += PAGE_SIZE;
#ifdef DOM0OPS
	if (xendomain_is_dom0()) {
		mapsize += IOM_SIZE;
	}
#endif

	/*
	 * At this point, mapsize doesn't include the table size.
	 */
#ifdef __x86_64__
	nL2 = TABLE_L2_ENTRIES;
#else
	nL2 = (mapsize + (NBPD_L2 - 1)) >> L2_SHIFT;
#endif

	/*
	 * Now compute how many L2 pages we need exactly. This is useful only
	 * on i386, since the initial count for amd64 is already enough.
	 */
	while (KERNTEXTOFF + mapsize + (nL2 + PDIRSZ) * PAGE_SIZE >
	    KERNBASE + (nL2 << L2_SHIFT)) {
		nL2++;
	}

#ifdef i386
	/*
	 * One more L2 page: we'll allocate several pages after kva_start
	 * in pmap_bootstrap() before pmap_growkernel(), which have not been
	 * counted here. It's not a big issue to allocate one more L2 as
	 * pmap_growkernel() will be called anyway.
	 */
	nL2++;
	nkptp[1] = nL2;
#endif

	/*
	 * Install bootstrap pages. We may need more L2 pages than will
	 * have the final table here, as it's installed after the final table.
	 */
	oldcount = nL2;

bootstrap_again:

	/*
	 * Xen space we'll reclaim may not be enough for our new page tables,
	 * move bootstrap tables if necessary.
	 */
	if (our_tables < xen_tables + ((nL2 + PDIRSZ) * PAGE_SIZE))
		our_tables = xen_tables + ((nL2 + PDIRSZ) * PAGE_SIZE);

	/*
	 * Make sure the number of L2 pages we have is enough to map everything
	 * from KERNBASE to the bootstrap tables themselves.
	 */
	if (our_tables + ((oldcount + PDIRSZ) * PAGE_SIZE) >
	    KERNBASE + (oldcount << L2_SHIFT)) {
		oldcount++;
		goto bootstrap_again;
	}

	/* Create temporary tables */
	xen_bootstrap_tables(xen_tables, our_tables,
	    xen_start_info.nr_pt_frames, oldcount, false);

	/* Create final tables */
	xen_bootstrap_tables(our_tables, xen_tables,
	    oldcount + PDIRSZ, nL2, true);

	/* Zero out PROC0 UAREA and DUMMY PAGE. */
	memset((void *)(xen_tables + ((nL2 + PDIRSZ) * PAGE_SIZE)), 0,
	    (UPAGES + 1) * PAGE_SIZE);

	/* Finally, flush TLB. */
	xpq_queue_tlb_flush();

	return (xen_tables + ((nL2 + PDIRSZ) * PAGE_SIZE));
}

/*
 * Build a new table and switch to it.
 * old_count is # of old tables (including L4, L3 and L2).
 * new_count is # of new tables (PTE only).
 * We assume the areas don't overlap.
 */
static void
xen_bootstrap_tables(vaddr_t old_pgd, vaddr_t new_pgd, size_t old_count,
    size_t new_count, bool final)
{
	pd_entry_t *L4cpu, *L4, *L3, *L2, *pte;
	paddr_t addr;
	vaddr_t page, avail, map_end;
	int i;
	extern char __rodata_start;
	extern char __data_start;
	extern char __kernel_end;
	extern char *early_zerop; /* from pmap.c */
#ifdef i386
	extern union descriptor tmpgdt[];
#endif

	/*
	 * Layout of RW area after the kernel image:
	 *     xencons_interface (if present)
	 *     xenstore_interface (if present)
	 *     table pages (new_count + PDIRSZ entries)
	 * Extra mappings (only when final is true):
	 *     UAREA
	 *     dummy user PGD (x86_64 only) / GDT page (i386 only)
	 *     HYPERVISOR_shared_info
	 *     early_zerop
	 *     ISA I/O mem (if needed)
	 */
	map_end = new_pgd + ((new_count + PDIRSZ) * PAGE_SIZE);
	if (final) {
		map_end += UPAGES * PAGE_SIZE;
		xen_dummy_page = (vaddr_t)map_end;
		map_end += PAGE_SIZE;
		HYPERVISOR_shared_info = (shared_info_t *)map_end;
		map_end += PAGE_SIZE;
		early_zerop = (char *)map_end;
		map_end += PAGE_SIZE;
	}

	/*
	 * We always set atdevbase, as it's used by init386 to find the first
	 * available VA. map_end is updated only if we are dom0, so
	 * atdevbase -> atdevbase + IOM_SIZE will be mapped only in
	 * this case.
	 */
	if (final) {
		atdevbase = map_end;
#ifdef DOM0OPS
		if (xendomain_is_dom0()) {
			/* ISA I/O mem */
			map_end += IOM_SIZE;
		}
#endif
	}

	__PRINTK(("xen_bootstrap_tables map_end 0x%lx\n", map_end));
	__PRINTK(("console %#lx ", xen_start_info.console_mfn));
	__PRINTK(("xenstore %#" PRIx32 "\n", xen_start_info.store_mfn));

	avail = new_pgd;

	/*
	 * Create our page tables.
	 */

#ifdef __x86_64__
	/* per-cpu L4 */
	L4cpu = (pd_entry_t *)avail;
	memset(L4cpu, 0, PAGE_SIZE);
	avail += PAGE_SIZE;

	/* pmap_kernel L4 */
	L4 = (pd_entry_t *)avail;
	memset(L4, 0, PAGE_SIZE);
	avail += PAGE_SIZE;

	/* L3 */
	L3 = (pd_entry_t *)avail;
	memset(L3, 0, PAGE_SIZE);
	avail += PAGE_SIZE;

	/* link L4->L3 */
	addr = ((u_long)L3) - KERNBASE;
	L4cpu[pl4_pi(KERNTEXTOFF)] = xpmap_ptom_masked(addr) | PTE_P | PTE_W;
	L4[pl4_pi(KERNTEXTOFF)] = xpmap_ptom_masked(addr) | PTE_P | PTE_W;

	/* L2 */
	L2 = (pd_entry_t *)avail;
	memset(L2, 0, PAGE_SIZE);
	avail += PAGE_SIZE;

	/* link L3->L2 */
	addr = ((u_long)L2) - KERNBASE;
	L3[pl3_pi(KERNTEXTOFF)] = xpmap_ptom_masked(addr) | PTE_P | PTE_W;
#else
	/* no L4 on i386PAE */
	__USE(L4cpu);
	__USE(L4);

	/* L3 */
	L3 = (pd_entry_t *)avail;
	memset(L3, 0, PAGE_SIZE);
	avail += PAGE_SIZE;

	/*
	 * Our PAE-style level 2, 5 contiguous pages (4 L2 + 1 shadow).
	 *                  +-----------------+----------------+---------+
	 * Physical layout: | 3 * USERLAND L2 | L2 KERN SHADOW | L2 KERN |
	 *                  +-----------------+----------------+---------+
	 * However, we enter L3[3] into L2 KERN, and not L2 KERN SHADOW.
	 * This way, L2[L2_SLOT_KERN] always points to the shadow.
	 */
	L2 = (pd_entry_t *)avail;
	memset(L2, 0, PAGE_SIZE * 5);
	avail += PAGE_SIZE * 5;

	/*
	 * Link L2 pages in L3, with a special case for L2 KERN. Xen doesn't
	 * want RW permissions in L3 entries, it'll add them itself.
	 */
	addr = ((u_long)L2) - KERNBASE;
	for (i = 0; i < 3; i++, addr += PAGE_SIZE) {
		L3[i] = xpmap_ptom_masked(addr) | PTE_P;
	}
	addr += PAGE_SIZE;
	L3[3] = xpmap_ptom_masked(addr) | PTE_P;
#endif

	/* Level 1 */
	page = KERNTEXTOFF;
	for (i = 0; i < new_count; i ++) {
		vaddr_t cur_page = page;

		pte = (pd_entry_t *)avail;
		memset(pte, 0, PAGE_SIZE);
		avail += PAGE_SIZE;

		while (pl2_pi(page) == pl2_pi(cur_page)) {
			if (page >= map_end) {
				/* not mapped at all */
				pte[pl1_pi(page)] = 0;
				page += PAGE_SIZE;
				continue;
			}
			pte[pl1_pi(page)] = xpmap_ptom_masked(page - KERNBASE);
			if (page == (vaddr_t)HYPERVISOR_shared_info) {
				pte[pl1_pi(page)] = xen_start_info.shared_info;
			}
			if ((xpmap_ptom_masked(page - KERNBASE) >> PAGE_SHIFT)
			    == xen_start_info.console.domU.mfn) {
				xencons_interface = (void *)page;
				pte[pl1_pi(page)] = xen_start_info.console_mfn;
				pte[pl1_pi(page)] <<= PAGE_SHIFT;
			}
			if ((xpmap_ptom_masked(page - KERNBASE) >> PAGE_SHIFT)
			    == xen_start_info.store_mfn) {
				xenstore_interface = (void *)page;
				pte[pl1_pi(page)] = xen_start_info.store_mfn;
				pte[pl1_pi(page)] <<= PAGE_SHIFT;
			}
#ifdef DOM0OPS
			if (page >= (vaddr_t)atdevbase &&
			    page < (vaddr_t)atdevbase + IOM_SIZE) {
				pte[pl1_pi(page)] =
				    IOM_BEGIN + (page - (vaddr_t)atdevbase);
				pte[pl1_pi(page)] |= xpmap_pg_nx;
			}
#endif

			pte[pl1_pi(page)] |= PTE_P;
			if (page < (vaddr_t)&__rodata_start) {
				/* Map the kernel text RX. Nothing to do. */
			} else if (page >= (vaddr_t)&__rodata_start &&
			    page < (vaddr_t)&__data_start) {
				/* Map the kernel rodata R. */
				pte[pl1_pi(page)] |= xpmap_pg_nx;
			} else if (page >= old_pgd &&
			    page < old_pgd + (old_count * PAGE_SIZE)) {
				/* Map the old page tables R. */
				pte[pl1_pi(page)] |= xpmap_pg_nx;
			} else if (page >= new_pgd &&
			    page < new_pgd + ((new_count + PDIRSZ) * PAGE_SIZE)) {
				/* Map the new page tables R. */
				pte[pl1_pi(page)] |= xpmap_pg_nx;
#ifdef i386
			} else if (page == (vaddr_t)tmpgdt) {
				/*
				 * Map bootstrap gdt R/O. Later, we will re-add
				 * this page to uvm after making it writable.
				 */
				pte[pl1_pi(page)] = 0;
				page += PAGE_SIZE;
				continue;
#endif
			} else if (page >= (vaddr_t)&__data_start &&
			    page < (vaddr_t)&__kernel_end) {
				/* Map the kernel data+bss RW. */
				pte[pl1_pi(page)] |= PTE_W | xpmap_pg_nx;
			} else {
				/* Map the page RW. */
				pte[pl1_pi(page)] |= PTE_W | xpmap_pg_nx;
			}

			page += PAGE_SIZE;
		}

		addr = ((u_long)pte) - KERNBASE;
		L2[pl2_pi(cur_page)] = xpmap_ptom_masked(addr) | PTE_W | PTE_P;

		/* Mark readonly */
		xen_bt_set_readonly((vaddr_t)pte);
	}

	/* Install recursive page tables mapping */
#ifdef __x86_64__
	/* Recursive entry in pmap_kernel(). */
	L4[PDIR_SLOT_PTE] = xpmap_ptom_masked((paddr_t)L4 - KERNBASE)
	    | PTE_P | xpmap_pg_nx;
	/* Recursive entry in higher-level per-cpu PD. */
	L4cpu[PDIR_SLOT_PTE] = xpmap_ptom_masked((paddr_t)L4cpu - KERNBASE)
	    | PTE_P | xpmap_pg_nx;

	/* Mark tables RO */
	xen_bt_set_readonly((vaddr_t)L2);
#else
	/* Copy L2 KERN into L2 KERN SHADOW, and reference the latter in cpu0. */
	memcpy(&L2[L2_SLOT_KERN + NPDPG], &L2[L2_SLOT_KERN], PAGE_SIZE);
	cpu_info_primary.ci_kpm_pdir = &L2[L2_SLOT_KERN + NPDPG];
	cpu_info_primary.ci_kpm_pdirpa =
	    (vaddr_t)cpu_info_primary.ci_kpm_pdir - KERNBASE;

	/*
	 * We don't enter a recursive entry from the L3 PD. Instead, we enter
	 * the first 4 L2 pages, which includes the kernel's L2 shadow. But we
	 * have to enter the shadow after switching %cr3, or Xen will refcount
	 * some PTEs with the wrong type.
	 */
	addr = (u_long)L2 - KERNBASE;
	for (i = 0; i < 3; i++, addr += PAGE_SIZE) {
		L2[PDIR_SLOT_PTE + i] = xpmap_ptom_masked(addr) | PTE_P |
		    xpmap_pg_nx;
	}

	/* Mark tables RO, and pin L2 KERN SHADOW. */
	addr = (u_long)L2 - KERNBASE;
	for (i = 0; i < 5; i++, addr += PAGE_SIZE) {
		xen_bt_set_readonly(((vaddr_t)L2) + PAGE_SIZE * i);
	}
	if (final) {
		addr = (u_long)L2 - KERNBASE + 3 * PAGE_SIZE;
		xpq_queue_pin_l2_table(xpmap_ptom_masked(addr));
	}
#endif

	xen_bt_set_readonly((vaddr_t)L3);
#ifdef __x86_64__
	xen_bt_set_readonly((vaddr_t)L4cpu);
#endif

	/* Pin the PGD */
#ifdef __x86_64__
	xpq_queue_pin_l4_table(xpmap_ptom_masked(new_pgd - KERNBASE));
#else
	xpq_queue_pin_l3_table(xpmap_ptom_masked(new_pgd - KERNBASE));
#endif

	/* Save phys. addr of PDP, for libkvm. */
#ifdef __x86_64__
	PDPpaddr = (u_long)L4 - KERNBASE;
#else
	PDPpaddr = (u_long)L2 - KERNBASE; /* PDP is the L2 with PAE */
#endif

	/* Switch to new tables */
	xpq_queue_pt_switch(xpmap_ptom_masked(new_pgd - KERNBASE));

	if (final) {
#ifdef __x86_64__
		/* Save the address of the real per-cpu L4 page. */
		cpu_info_primary.ci_kpm_pdir = L4cpu;
		cpu_info_primary.ci_kpm_pdirpa = ((paddr_t)L4cpu - KERNBASE);
#else
		/* Save the address of the L3 page */
		cpu_info_primary.ci_pae_l3_pdir = L3;
		cpu_info_primary.ci_pae_l3_pdirpa = (new_pgd - KERNBASE);

		/* Now enter the kernel's PTE mappings */
		addr = (u_long)L2 - KERNBASE + PAGE_SIZE * 3;
		xpq_queue_pte_update(
		    xpmap_ptom(((vaddr_t)&L2[PDIR_SLOT_PTE + 3]) - KERNBASE),
		    xpmap_ptom_masked(addr) | PTE_P);
		xpq_flush_queue();
#endif
	}

	/*
	 * Now we can safely reclaim the space taken by the old tables.
	 */

	/* Unpin old PGD */
	xpq_queue_unpin_table(xpmap_ptom_masked(old_pgd - KERNBASE));

	/* Mark old tables RW if used, unmap otherwise */
	page = old_pgd;
	addr = xpmap_mtop((paddr_t)L2[pl2_pi(page)] & PG_FRAME);
	pte = (pd_entry_t *)((u_long)addr + KERNBASE);
	pte += pl1_pi(page);
	while (page < old_pgd + (old_count * PAGE_SIZE) && page < map_end) {
		addr = xpmap_ptom(((u_long)pte) - KERNBASE);
		xpq_queue_pte_update(addr, *pte | PTE_W);
		page += PAGE_SIZE;
		/*
		 * Our PTEs are contiguous so it's safe to just "++" here.
		 */
		pte++;
	}
	while (page < old_pgd + (old_count * PAGE_SIZE)) {
		addr = xpmap_ptom(((u_long)pte) - KERNBASE);
		xpq_queue_pte_update(addr, 0);
		page += PAGE_SIZE;
		pte++;
	}
	xpq_flush_queue();
}

/*
 * Mark a page read-only, assuming vaddr = paddr + KERNBASE.
 */
static void
xen_bt_set_readonly(vaddr_t page)
{
	pt_entry_t entry;

	entry = xpmap_ptom_masked(page - KERNBASE);
	entry |= PTE_P | xpmap_pg_nx;

	HYPERVISOR_update_va_mapping(page, entry, UVMF_INVLPG);
}

#ifdef __x86_64__
void
xen_set_user_pgd(paddr_t page)
{
	struct mmuext_op op;
	int s = splvm(); /* XXXSMP */

	xpq_flush_queue();
	op.cmd = MMUEXT_NEW_USER_BASEPTR;
	op.arg1.mfn = xpmap_ptom_masked(page) >> PAGE_SHIFT;
	if (HYPERVISOR_mmuext_op(&op, 1, NULL, DOMID_SELF) < 0)
		panic("xen_set_user_pgd: failed to install new user page"
			" directory %#" PRIxPADDR, page);
	splx(s);
}
#endif /* __x86_64__ */
