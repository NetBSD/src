/*	$NetBSD: x86_xpmap.c,v 1.53.4.3 2016/12/05 10:54:59 skrll Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: x86_xpmap.c,v 1.53.4.3 2016/12/05 10:54:59 skrll Exp $");

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

#undef	XENDEBUG
/* #define XENDEBUG_SYNC */

#ifdef XENDEBUG
#define	XENPRINTF(x) printf x
#define	XENPRINTK2(x) /* printk x */
static char XBUF[256];
#else
#define	XENPRINTF(x)
#define	XENPRINTK2(x)
#endif

volatile shared_info_t *HYPERVISOR_shared_info;
/* Xen requires the start_info struct to be page aligned */
union start_info_union start_info_union __aligned(PAGE_SIZE);
unsigned long *xpmap_phys_to_machine_mapping;
kmutex_t pte_lock;

void xen_failsafe_handler(void);

#define HYPERVISOR_mmu_update_self(req, count, success_count) \
	HYPERVISOR_mmu_update((req), (count), (success_count), DOMID_SELF)

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
		XENPRINTF(("xen_set_ldt %#" PRIxVADDR " %d %p\n",
		    base, entries, ptp));
		pmap_pte_clearbits(ptp, PG_RW);
	}
	s = splvm();
	xpq_queue_set_ldt(base, entries);
	splx(s);
}

#ifdef XENDEBUG
void xpq_debug_dump(void);
#endif

#define XPQUEUE_SIZE 2048
static mmu_update_t xpq_queue_array[MAXCPUS][XPQUEUE_SIZE];
static int xpq_idx_array[MAXCPUS];

#ifdef i386
extern union descriptor tmpgdt[];
#endif

void
xpq_flush_queue(void)
{
	int i, ok = 0, ret;

	mmu_update_t *xpq_queue = xpq_queue_array[curcpu()->ci_cpuid];
	int xpq_idx = xpq_idx_array[curcpu()->ci_cpuid];

	XENPRINTK2(("flush queue %p entries %d\n", xpq_queue, xpq_idx));
	for (i = 0; i < xpq_idx; i++)
		XENPRINTK2(("%d: 0x%08" PRIx64 " 0x%08" PRIx64 "\n", i,
		    xpq_queue[i].ptr, xpq_queue[i].val));

retry:
	ret = HYPERVISOR_mmu_update_self(xpq_queue, xpq_idx, &ok);

	if (xpq_idx != 0 && ret < 0) {
		struct cpu_info *ci;
		CPU_INFO_ITERATOR cii;

		printf("xpq_flush_queue: %d entries (%d successful) on "
		    "cpu%d (%ld)\n",
		    xpq_idx, ok, curcpu()->ci_index, curcpu()->ci_cpuid);

		if (ok != 0) {
			xpq_queue += ok;
			xpq_idx -= ok;
			ok = 0;
			goto retry;
		}

		for (CPU_INFO_FOREACH(cii, ci)) {
			xpq_queue = xpq_queue_array[ci->ci_cpuid];
			xpq_idx = xpq_idx_array[ci->ci_cpuid];
			printf("cpu%d (%ld):\n", ci->ci_index, ci->ci_cpuid);
			for (i = 0; i < xpq_idx; i++) {
				printf("  0x%016" PRIx64 ": 0x%016" PRIx64 "\n",
				   xpq_queue[i].ptr, xpq_queue[i].val);
			}
#ifdef __x86_64__
			for (i = 0; i < PDIR_SLOT_PTE; i++) {
				if (ci->ci_kpm_pdir[i] == 0)
					continue;
				printf(" kpm_pdir[%d]: 0x%" PRIx64 "\n",
				    i, ci->ci_kpm_pdir[i]);
			}
#endif
		}
		panic("HYPERVISOR_mmu_update failed, ret: %d\n", ret);
	}
	xpq_idx_array[curcpu()->ci_cpuid] = 0;
}

static inline void
xpq_increment_idx(void)
{

	if (__predict_false(++xpq_idx_array[curcpu()->ci_cpuid] == XPQUEUE_SIZE))
		xpq_flush_queue();
}

void
xpq_queue_machphys_update(paddr_t ma, paddr_t pa)
{

	mmu_update_t *xpq_queue = xpq_queue_array[curcpu()->ci_cpuid];
	int xpq_idx = xpq_idx_array[curcpu()->ci_cpuid];

	XENPRINTK2(("xpq_queue_machphys_update ma=0x%" PRIx64 " pa=0x%" PRIx64
	    "\n", (int64_t)ma, (int64_t)pa));

	xpq_queue[xpq_idx].ptr = ma | MMU_MACHPHYS_UPDATE;
	xpq_queue[xpq_idx].val = pa >> PAGE_SHIFT;
	xpq_increment_idx();
#ifdef XENDEBUG_SYNC
	xpq_flush_queue();
#endif
}

void
xpq_queue_pte_update(paddr_t ptr, pt_entry_t val)
{

	mmu_update_t *xpq_queue = xpq_queue_array[curcpu()->ci_cpuid];
	int xpq_idx = xpq_idx_array[curcpu()->ci_cpuid];

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
xpq_queue_pin_table(paddr_t pa, int lvl)
{
	struct mmuext_op op;

	xpq_flush_queue();

	XENPRINTK2(("xpq_queue_pin_l%d_table: %#" PRIxPADDR "\n",
	    lvl + 1, pa));

	op.arg1.mfn = pa >> PAGE_SHIFT;
	op.cmd = lvl;

	if (HYPERVISOR_mmuext_op(&op, 1, NULL, DOMID_SELF) < 0)
		panic("xpq_queue_pin_table");
}

void
xpq_queue_unpin_table(paddr_t pa)
{
	struct mmuext_op op;

	xpq_flush_queue();

	XENPRINTK2(("xpq_queue_unpin_table: %#" PRIxPADDR "\n", pa));
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
	int s = splvm();

	xpq_flush_queue();

	XENPRINTK2(("xpq_queue_flush_cache\n"));
	asm("wbinvd":::"memory");
	splx(s); /* XXX: removeme */
}

void
xpq_queue_invlpg(vaddr_t va)
{
	struct mmuext_op op;
	xpq_flush_queue();

	XENPRINTK2(("xpq_queue_invlpg %#" PRIxVADDR "\n", va));
	op.cmd = MMUEXT_INVLPG_LOCAL;
	op.arg1.linear_addr = (va & ~PAGE_MASK);
	if (HYPERVISOR_mmuext_op(&op, 1, NULL, DOMID_SELF) < 0)
		panic("xpq_queue_invlpg");
}

void
xen_mcast_invlpg(vaddr_t va, kcpuset_t *kc)
{
	xcpumask_t xcpumask;
	mmuext_op_t op;

	kcpuset_export_u32(kc, &xcpumask.xcpum_km[0], sizeof(xcpumask));

	/* Flush pending page updates */
	xpq_flush_queue();

	op.cmd = MMUEXT_INVLPG_MULTI;
	op.arg1.linear_addr = va;
	op.arg2.vcpumask = &xcpumask.xcpum_xm;

	if (HYPERVISOR_mmuext_op(&op, 1, NULL, DOMID_SELF) < 0) {
		panic("xpq_queue_invlpg_all");
	}

	return;
}

void
xen_bcast_invlpg(vaddr_t va)
{
	mmuext_op_t op;

	/* Flush pending page updates */
	xpq_flush_queue();

	op.cmd = MMUEXT_INVLPG_ALL;
	op.arg1.linear_addr = va;

	if (HYPERVISOR_mmuext_op(&op, 1, NULL, DOMID_SELF) < 0) {
		panic("xpq_queue_invlpg_all");
	}

	return;
}

/* This is a synchronous call. */
void
xen_mcast_tlbflush(kcpuset_t *kc)
{
	xcpumask_t xcpumask;
	mmuext_op_t op;

	kcpuset_export_u32(kc, &xcpumask.xcpum_km[0], sizeof(xcpumask));

	/* Flush pending page updates */
	xpq_flush_queue();

	op.cmd = MMUEXT_TLB_FLUSH_MULTI;
	op.arg2.vcpumask = &xcpumask.xcpum_xm;

	if (HYPERVISOR_mmuext_op(&op, 1, NULL, DOMID_SELF) < 0) {
		panic("xpq_queue_invlpg_all");
	}

	return;
}

/* This is a synchronous call. */
void
xen_bcast_tlbflush(void)
{
	mmuext_op_t op;

	/* Flush pending page updates */
	xpq_flush_queue();

	op.cmd = MMUEXT_TLB_FLUSH_ALL;

	if (HYPERVISOR_mmuext_op(&op, 1, NULL, DOMID_SELF) < 0) {
		panic("xpq_queue_invlpg_all");
	}

	return;
}

/* This is a synchronous call. */
void
xen_vcpu_mcast_invlpg(vaddr_t sva, vaddr_t eva, kcpuset_t *kc)
{
	KASSERT(eva > sva);

	/* Flush pending page updates */
	xpq_flush_queue();

	/* Align to nearest page boundary */
	sva &= ~PAGE_MASK;
	eva &= ~PAGE_MASK;

	for ( ; sva <= eva; sva += PAGE_SIZE) {
		xen_mcast_invlpg(sva, kc);
	}

	return;
}

/* This is a synchronous call. */
void
xen_vcpu_bcast_invlpg(vaddr_t sva, vaddr_t eva)
{
	KASSERT(eva > sva);

	/* Flush pending page updates */
	xpq_flush_queue();

	/* Align to nearest page boundary */
	sva &= ~PAGE_MASK;
	eva &= ~PAGE_MASK;

	for ( ; sva <= eva; sva += PAGE_SIZE) {
		xen_bcast_invlpg(sva);
	}

	return;
}

/* Copy a page */
void
xen_copy_page(paddr_t srcpa, paddr_t dstpa)
{
	mmuext_op_t op;

	op.cmd = MMUEXT_COPY_PAGE;
	op.arg1.mfn = xpmap_ptom(dstpa) >> PAGE_SHIFT;
	op.arg2.src_mfn = xpmap_ptom(srcpa) >> PAGE_SHIFT;

	if (HYPERVISOR_mmuext_op(&op, 1, NULL, DOMID_SELF) < 0) {
		panic(__func__);
	}
}

/* Zero a physical page */
void
xen_pagezero(paddr_t pa)
{
	mmuext_op_t op;

	op.cmd = MMUEXT_CLEAR_PAGE;
	op.arg1.mfn = xpmap_ptom(pa) >> PAGE_SHIFT;

	if (HYPERVISOR_mmuext_op(&op, 1, NULL, DOMID_SELF) < 0) {
		panic(__func__);
	}
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

	mmu_update_t *xpq_queue = xpq_queue_array[curcpu()->ci_cpuid];
	int xpq_idx = xpq_idx_array[curcpu()->ci_cpuid];

	XENPRINTK2(("idx: %d\n", xpq_idx));
	for (i = 0; i < xpq_idx; i++) {
		snprintf(XBUF, sizeof(XBUF), "%" PRIx64 " %08" PRIx64,
		    xpq_queue[i].ptr, xpq_queue[i].val);
		if (++i < xpq_idx)
			snprintf(XBUF + strlen(XBUF),
			    sizeof(XBUF) - strlen(XBUF),
			    "%" PRIx64 " %08" PRIx64,
			    xpq_queue[i].ptr, xpq_queue[i].val);
		if (++i < xpq_idx)
			snprintf(XBUF + strlen(XBUF),
			    sizeof(XBUF) - strlen(XBUF),
			    "%" PRIx64 " %08" PRIx64,
			    xpq_queue[i].ptr, xpq_queue[i].val);
		if (++i < xpq_idx)
			snprintf(XBUF + strlen(XBUF),
			    sizeof(XBUF) - strlen(XBUF),
			    "%" PRIx64 " %08" PRIx64,
			    xpq_queue[i].ptr, xpq_queue[i].val);
		XENPRINTK2(("%d: %s\n", xpq_idx, XBUF));
	}
}
#endif


#if L2_SLOT_KERNBASE > 0
#define TABLE_L2_ENTRIES (2 * (NKL2_KIMG_ENTRIES + 1))
#else
#define TABLE_L2_ENTRIES (NKL2_KIMG_ENTRIES + 1)
#endif

#ifdef PAE
/*
 * For PAE, we consider a single contiguous L2 "superpage" of 4 pages, all of
 * them mapped by the L3 page. We also need a shadow page for L3[3].
 */
static const int l2_4_count = 6;
#elif defined(__x86_64__)
static const int l2_4_count = PTP_LEVELS;
#else
static const int l2_4_count = PTP_LEVELS - 1;
#endif

/*
 * Xen locore: get rid of the Xen bootstrap tables. Build and switch to new page
 * tables.
 */
vaddr_t
xen_locore(void)
{
	size_t count, oldcount, mapsize;
	vaddr_t bootstrap_tables, init_tables;

	xen_init_features();

	memset(xpq_idx_array, 0, sizeof(xpq_idx_array));

	xpmap_phys_to_machine_mapping =
	    (unsigned long *)xen_start_info.mfn_list;

	/* Space after Xen boostrap tables should be free */
	init_tables = xen_start_info.pt_base;
	bootstrap_tables = init_tables +
	    (xen_start_info.nr_pt_frames * PAGE_SIZE);

	/*
	 * Calculate how much space we need. First, everything mapped before
	 * the Xen bootstrap tables.
	 */
	mapsize = init_tables - KERNTEXTOFF;
	/* after the tables we'll have:
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
	count = TABLE_L2_ENTRIES;
#else
	count = (mapsize + (NBPD_L2 - 1)) >> L2_SHIFT;
#endif

	/*
	 * Now compute how many L2 pages we need exactly. This is useful only
	 * on i386, since the initial count for amd64 is already enough.
	 */
	while (KERNTEXTOFF + mapsize + (count + l2_4_count) * PAGE_SIZE >
	    KERNBASE + (count << L2_SHIFT)) {
		count++;
	}

#ifndef __x86_64__
	/*
	 * One more L2 page: we'll allocate several pages after kva_start
	 * in pmap_bootstrap() before pmap_growkernel(), which have not been
	 * counted here. It's not a big issue to allocate one more L2 as
	 * pmap_growkernel() will be called anyway.
	 */
	count++;
	nkptp[1] = count;
#endif

	/*
	 * Install bootstrap pages. We may need more L2 pages than will
	 * have the final table here, as it's installed after the final table.
	 */
	oldcount = count;

bootstrap_again:

	/*
	 * Xen space we'll reclaim may not be enough for our new page tables,
	 * move bootstrap tables if necessary.
	 */
	if (bootstrap_tables < init_tables + ((count + l2_4_count) * PAGE_SIZE))
		bootstrap_tables = init_tables +
		    ((count + l2_4_count) * PAGE_SIZE);

	/*
	 * Make sure the number of L2 pages we have is enough to map everything
	 * from KERNBASE to the bootstrap tables themselves.
	 */
	if (bootstrap_tables + ((oldcount + l2_4_count) * PAGE_SIZE) >
	    KERNBASE + (oldcount << L2_SHIFT)) {
		oldcount++;
		goto bootstrap_again;
	}

	/* Create temporary tables */
	xen_bootstrap_tables(init_tables, bootstrap_tables,
	    xen_start_info.nr_pt_frames, oldcount, false);

	/* Create final tables */
	xen_bootstrap_tables(bootstrap_tables, init_tables,
	    oldcount + l2_4_count, count, true);

	/* Zero out free space after tables */
	memset((void *)(init_tables + ((count + l2_4_count) * PAGE_SIZE)), 0,
	    (UPAGES + 1) * PAGE_SIZE);

	/* Finally, flush TLB. */
	xpq_queue_tlb_flush();

	return (init_tables + ((count + l2_4_count) * PAGE_SIZE));
}

/*
 * Build a new table and switch to it.
 * old_count is # of old tables (including PGD, PDTPE and PDE).
 * new_count is # of new tables (PTE only).
 * We assume the areas don't overlap.
 */
static void
xen_bootstrap_tables(vaddr_t old_pgd, vaddr_t new_pgd, size_t old_count,
    size_t new_count, bool final)
{
	pd_entry_t *pdtpe, *pde, *pte;
	pd_entry_t *bt_pgd;
	paddr_t addr;
	vaddr_t page, avail, map_end;
	int i;
	extern char __rodata_start;
	extern char __data_start;
	extern char __kernel_end;
	extern char *early_zerop; /* from pmap.c */
	pt_entry_t pg_nx;
	u_int descs[4];

	/*
	 * Set the NX/XD bit, if available. descs[3] = %edx.
	 */
	x86_cpuid(0x80000001, descs);
	pg_nx = (descs[3] & CPUID_NOX) ? PG_NX : 0;

	/*
	 * Layout of RW area after the kernel image:
	 *     xencons_interface (if present)
	 *     xenstore_interface (if present)
	 *     table pages (new_count + l2_4_count entries)
	 * Extra mappings (only when final is true):
	 *     UAREA
	 *     dummy user PGD (x86_64 only) / GDT page (i386 only)
	 *     HYPERVISOR_shared_info
	 *     early_zerop
	 *     ISA I/O mem (if needed)
	 */
	map_end = new_pgd + ((new_count + l2_4_count) * PAGE_SIZE);
	if (final) {
		map_end += (UPAGES + 1) * PAGE_SIZE;
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

	/*
	 * Create bootstrap page tables. What we need:
	 * - a PGD (level 4)
	 * - a PDTPE (level 3)
	 * - a PDE (level 2)
	 * - some PTEs (level 1)
	 */

	bt_pgd = (pd_entry_t *)new_pgd;
	memset(bt_pgd, 0, PAGE_SIZE);
	avail = new_pgd + PAGE_SIZE;

#if PTP_LEVELS > 3
	/* Per-cpu L4 */
	pd_entry_t *bt_cpu_pgd = bt_pgd;
	/* pmap_kernel() "shadow" L4 */
	bt_pgd = (pd_entry_t *)avail;
	memset(bt_pgd, 0, PAGE_SIZE);
	avail += PAGE_SIZE;

	/* Install L3 */
	pdtpe = (pd_entry_t *)avail;
	memset(pdtpe, 0, PAGE_SIZE);
	avail += PAGE_SIZE;

	addr = ((u_long)pdtpe) - KERNBASE;
	bt_pgd[pl4_pi(KERNTEXTOFF)] = bt_cpu_pgd[pl4_pi(KERNTEXTOFF)] =
	    xpmap_ptom_masked(addr) | PG_k | PG_V | PG_RW;
#else
	pdtpe = bt_pgd;
#endif

#if PTP_LEVELS > 2
	/* Level 2 */
	pde = (pd_entry_t *)avail;
	memset(pde, 0, PAGE_SIZE);
	avail += PAGE_SIZE;

	addr = ((u_long)pde) - KERNBASE;
	pdtpe[pl3_pi(KERNTEXTOFF)] =
	    xpmap_ptom_masked(addr) | PG_k | PG_V | PG_RW;
#elif defined(PAE)
	/* Our PAE-style level 2: 5 contigous pages (4 L2 + 1 shadow) */
	pde = (pd_entry_t *)avail;
	memset(pde, 0, PAGE_SIZE * 5);
	avail += PAGE_SIZE * 5;
	addr = ((u_long)pde) - KERNBASE;

	/*
	 * Enter L2 pages in L3. The real L2 kernel PD will be the last one
	 * (so that pde[L2_SLOT_KERN] always points to the shadow).
	 */
	for (i = 0; i < 3; i++, addr += PAGE_SIZE) {
		/*
		 * Xen doesn't want RW mappings in L3 entries, it'll add it
		 * itself.
		 */
		pdtpe[i] = xpmap_ptom_masked(addr) | PG_k | PG_V;
	}
	addr += PAGE_SIZE;
	pdtpe[3] = xpmap_ptom_masked(addr) | PG_k | PG_V;
#else
	pde = bt_pgd;
#endif

	/* Level 1 */
	page = KERNTEXTOFF;
	for (i = 0; i < new_count; i ++) {
		vaddr_t cur_page = page;

		pte = (pd_entry_t *)avail;
		avail += PAGE_SIZE;

		memset(pte, 0, PAGE_SIZE);
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
				pte[pl1_pi(page)] |= pg_nx;
			}
#endif

			pte[pl1_pi(page)] |= PG_k | PG_V;
			if (page < (vaddr_t)&__rodata_start) {
				/* Map the kernel text RX. */
				pte[pl1_pi(page)] |= PG_RO;
			} else if (page >= (vaddr_t)&__rodata_start &&
			    page < (vaddr_t)&__data_start) {
				/* Map the kernel rodata R. */
				pte[pl1_pi(page)] |= PG_RO | pg_nx;
			} else if (page >= old_pgd &&
			    page < old_pgd + (old_count * PAGE_SIZE)) {
				/* Map the old page tables R. */
				pte[pl1_pi(page)] |= PG_RO | pg_nx;
			} else if (page >= new_pgd &&
			    page < new_pgd + ((new_count + l2_4_count) * PAGE_SIZE)) {
				/* Map the new page tables R. */
				pte[pl1_pi(page)] |= PG_RO | pg_nx;
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
				pte[pl1_pi(page)] |= PG_RW | pg_nx;
			} else {
				/* Map the page RW. */
				pte[pl1_pi(page)] |= PG_RW | pg_nx;
			}

			page += PAGE_SIZE;
		}

		addr = ((u_long)pte) - KERNBASE;
		pde[pl2_pi(cur_page)] =
		    xpmap_ptom_masked(addr) | PG_k | PG_RW | PG_V;

		/* Mark readonly */
		xen_bt_set_readonly((vaddr_t)pte);
	}

	/* Install recursive page tables mapping */
#ifdef PAE
	/*
	 * We need a shadow page for the kernel's L2 page.
	 * The real L2 kernel PD will be the last one (so that
	 * pde[L2_SLOT_KERN] always points to the shadow).
	 */
	memcpy(&pde[L2_SLOT_KERN + NPDPG], &pde[L2_SLOT_KERN], PAGE_SIZE);
	cpu_info_primary.ci_kpm_pdir = &pde[L2_SLOT_KERN + NPDPG];
	cpu_info_primary.ci_kpm_pdirpa =
	    (vaddr_t) cpu_info_primary.ci_kpm_pdir - KERNBASE;

	/*
	 * We don't enter a recursive entry from the L3 PD. Instead, we enter
	 * the first 4 L2 pages, which includes the kernel's L2 shadow. But we
	 * have to enter the shadow after switching %cr3, or Xen will refcount
	 * some PTEs with the wrong type.
	 */
	addr = (u_long)pde - KERNBASE;
	for (i = 0; i < 3; i++, addr += PAGE_SIZE) {
		pde[PDIR_SLOT_PTE + i] = xpmap_ptom_masked(addr) | PG_k | PG_V |
		    pg_nx;
	}
#if 0
	addr += PAGE_SIZE; /* point to shadow L2 */
	pde[PDIR_SLOT_PTE + 3] = xpmap_ptom_masked(addr) | PG_k | PG_V;
#endif
	/* Mark tables RO, and pin the kernel's shadow as L2 */
	addr = (u_long)pde - KERNBASE;
	for (i = 0; i < 5; i++, addr += PAGE_SIZE) {
		xen_bt_set_readonly(((vaddr_t)pde) + PAGE_SIZE * i);
#if 0
		if (i == 2 || i == 3)
			continue;
		xpq_queue_pin_l2_table(xpmap_ptom_masked(addr));
#endif
	}
	if (final) {
		addr = (u_long)pde - KERNBASE + 3 * PAGE_SIZE;
		xpq_queue_pin_l2_table(xpmap_ptom_masked(addr));
	}
#if 0
	addr = (u_long)pde - KERNBASE + 2 * PAGE_SIZE;
	xpq_queue_pin_l2_table(xpmap_ptom_masked(addr));
#endif
#else /* PAE */

	/* Recursive entry in pmap_kernel(). */
	bt_pgd[PDIR_SLOT_PTE] = xpmap_ptom_masked((paddr_t)bt_pgd - KERNBASE)
	    | PG_k | PG_RO | PG_V | pg_nx;
#ifdef __x86_64__
	/* Recursive entry in higher-level per-cpu PD. */
	bt_cpu_pgd[PDIR_SLOT_PTE] = xpmap_ptom_masked((paddr_t)bt_cpu_pgd - KERNBASE)
	    | PG_k | PG_RO | PG_V | pg_nx;
#endif

	/* Mark tables RO */
	xen_bt_set_readonly((vaddr_t)pde);
#endif
#if PTP_LEVELS > 2 || defined(PAE)
	xen_bt_set_readonly((vaddr_t)pdtpe);
#endif
#if PTP_LEVELS > 3
	xen_bt_set_readonly(new_pgd);
#endif

	/* Pin the PGD */
#ifdef __x86_64__
	xpq_queue_pin_l4_table(xpmap_ptom_masked(new_pgd - KERNBASE));
#elif PAE
	xpq_queue_pin_l3_table(xpmap_ptom_masked(new_pgd - KERNBASE));
#else
	xpq_queue_pin_l2_table(xpmap_ptom_masked(new_pgd - KERNBASE));
#endif

	/* Save phys. addr of PDP, for libkvm. */
#ifdef PAE
	PDPpaddr = (u_long)pde - KERNBASE; /* PDP is the L2 with PAE */
#else
	PDPpaddr = (u_long)bt_pgd - KERNBASE;
#endif

	/* Switch to new tables */
	xpq_queue_pt_switch(xpmap_ptom_masked(new_pgd - KERNBASE));

#ifdef PAE
	if (final) {
		/* Save the address of the L3 page */
		cpu_info_primary.ci_pae_l3_pdir = pdtpe;
		cpu_info_primary.ci_pae_l3_pdirpa = (new_pgd - KERNBASE);

		/* Now enter the kernel's PTE mappings */
		addr = (u_long)pde - KERNBASE + PAGE_SIZE * 3;
		xpq_queue_pte_update(
		    xpmap_ptom(((vaddr_t)&pde[PDIR_SLOT_PTE + 3]) - KERNBASE),
		    xpmap_ptom_masked(addr) | PG_k | PG_V);
		xpq_flush_queue();
	}
#elif defined(__x86_64__)
	if (final) {
		/* Save the address of the real per-cpu L4 page. */
		cpu_info_primary.ci_kpm_pdir = bt_cpu_pgd;
		cpu_info_primary.ci_kpm_pdirpa = ((paddr_t)bt_cpu_pgd - KERNBASE);
	}
#endif
	__USE(pdtpe);

	/*
	 * Now we can safely reclaim the space taken by the old tables.
	 */

	/* Unpin old PGD */
	xpq_queue_unpin_table(xpmap_ptom_masked(old_pgd - KERNBASE));

	/* Mark old tables RW */
	page = old_pgd;
	addr = xpmap_mtop((paddr_t)pde[pl2_pi(page)] & PG_FRAME);
	pte = (pd_entry_t *)((u_long)addr + KERNBASE);
	pte += pl1_pi(page);
	while (page < old_pgd + (old_count * PAGE_SIZE) && page < map_end) {
		addr = xpmap_ptom(((u_long)pte) - KERNBASE);
		xpq_queue_pte_update(addr, *pte | PG_RW);
		page += PAGE_SIZE;
		/*
		 * Our PTEs are contiguous so it's safe to just "++" here.
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
xen_bt_set_readonly(vaddr_t page)
{
	pt_entry_t entry;

	entry = xpmap_ptom_masked(page - KERNBASE);
	entry |= PG_k | PG_V;

	HYPERVISOR_update_va_mapping(page, entry, UVMF_INVLPG);
}

#ifdef __x86_64__
void
xen_set_user_pgd(paddr_t page)
{
	struct mmuext_op op;
	int s = splvm();

	xpq_flush_queue();
	op.cmd = MMUEXT_NEW_USER_BASEPTR;
	op.arg1.mfn = xpmap_ptom_masked(page) >> PAGE_SHIFT;
	if (HYPERVISOR_mmuext_op(&op, 1, NULL, DOMID_SELF) < 0)
		panic("xen_set_user_pgd: failed to install new user page"
			" directory %#" PRIxPADDR, page);
	splx(s);
}
#endif /* __x86_64__ */
