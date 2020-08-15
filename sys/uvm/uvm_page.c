/*	$NetBSD: uvm_page.c,v 1.246 2020/08/15 01:27:22 tnn Exp $	*/

/*-
 * Copyright (c) 2019, 2020 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Andrew Doran.
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
 * Copyright (c) 1997 Charles D. Cranor and Washington University.
 * Copyright (c) 1991, 1993, The Regents of the University of California.
 *
 * All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * The Mach Operating System project at Carnegie-Mellon University.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	@(#)vm_page.c   8.3 (Berkeley) 3/21/94
 * from: Id: uvm_page.c,v 1.1.2.18 1998/02/06 05:24:42 chs Exp
 *
 *
 * Copyright (c) 1987, 1990 Carnegie-Mellon University.
 * All rights reserved.
 *
 * Permission to use, copy, modify and distribute this software and
 * its documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 *
 * CARNEGIE MELLON ALLOWS FREE USE OF THIS SOFTWARE IN ITS "AS IS"
 * CONDITION.  CARNEGIE MELLON DISCLAIMS ANY LIABILITY OF ANY KIND
 * FOR ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 *
 * Carnegie Mellon requests users of this software to return to
 *
 *  Software Distribution Coordinator  or  Software.Distribution@CS.CMU.EDU
 *  School of Computer Science
 *  Carnegie Mellon University
 *  Pittsburgh PA 15213-3890
 *
 * any improvements or extensions that they make and grant Carnegie the
 * rights to redistribute these changes.
 */

/*
 * uvm_page.c: page ops.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: uvm_page.c,v 1.246 2020/08/15 01:27:22 tnn Exp $");

#include "opt_ddb.h"
#include "opt_uvm.h"
#include "opt_uvmhist.h"
#include "opt_readahead.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/sched.h>
#include <sys/kernel.h>
#include <sys/vnode.h>
#include <sys/proc.h>
#include <sys/radixtree.h>
#include <sys/atomic.h>
#include <sys/cpu.h>

#include <uvm/uvm.h>
#include <uvm/uvm_ddb.h>
#include <uvm/uvm_pdpolicy.h>
#include <uvm/uvm_pgflcache.h>

/*
 * number of pages per-CPU to reserve for the kernel.
 */
#ifndef	UVM_RESERVED_PAGES_PER_CPU
#define	UVM_RESERVED_PAGES_PER_CPU	5
#endif
int vm_page_reserve_kernel = UVM_RESERVED_PAGES_PER_CPU;

/*
 * physical memory size;
 */
psize_t physmem;

/*
 * local variables
 */

/*
 * these variables record the values returned by vm_page_bootstrap,
 * for debugging purposes.  The implementation of uvm_pageboot_alloc
 * and pmap_startup here also uses them internally.
 */

static vaddr_t      virtual_space_start;
static vaddr_t      virtual_space_end;

/*
 * we allocate an initial number of page colors in uvm_page_init(),
 * and remember them.  We may re-color pages as cache sizes are
 * discovered during the autoconfiguration phase.  But we can never
 * free the initial set of buckets, since they are allocated using
 * uvm_pageboot_alloc().
 */

static size_t recolored_pages_memsize /* = 0 */;
static char *recolored_pages_mem;

/*
 * freelist locks - one per bucket.
 */

union uvm_freelist_lock	uvm_freelist_locks[PGFL_MAX_BUCKETS]
    __cacheline_aligned;

/*
 * basic NUMA information.
 */

static struct uvm_page_numa_region {
	struct uvm_page_numa_region	*next;
	paddr_t				start;
	paddr_t				size;
	u_int				numa_id;
} *uvm_page_numa_region;

#ifdef DEBUG
kmutex_t uvm_zerochecklock __cacheline_aligned;
vaddr_t uvm_zerocheckkva;
#endif /* DEBUG */

/*
 * These functions are reserved for uvm(9) internal use and are not
 * exported in the header file uvm_physseg.h
 *
 * Thus they are redefined here.
 */
void uvm_physseg_init_seg(uvm_physseg_t, struct vm_page *);
void uvm_physseg_seg_chomp_slab(uvm_physseg_t, struct vm_page *, size_t);

/* returns a pgs array */
struct vm_page *uvm_physseg_seg_alloc_from_slab(uvm_physseg_t, size_t);

/*
 * inline functions
 */

/*
 * uvm_pageinsert: insert a page in the object.
 *
 * => caller must lock object
 * => call should have already set pg's object and offset pointers
 *    and bumped the version counter
 */

static inline void
uvm_pageinsert_object(struct uvm_object *uobj, struct vm_page *pg)
{

	KASSERT(uobj == pg->uobject);
	KASSERT(rw_write_held(uobj->vmobjlock));
	KASSERT((pg->flags & PG_TABLED) == 0);

	if ((pg->flags & PG_STAT) != 0) {
		/* Cannot use uvm_pagegetdirty(): not yet in radix tree. */
		const unsigned int status = pg->flags & (PG_CLEAN | PG_DIRTY);

		if ((pg->flags & PG_FILE) != 0) {
			if (uobj->uo_npages == 0) {
				struct vnode *vp = (struct vnode *)uobj;
				mutex_enter(vp->v_interlock);
				KASSERT((vp->v_iflag & VI_PAGES) == 0);
				vp->v_iflag |= VI_PAGES;
				vholdl(vp);
				mutex_exit(vp->v_interlock);
			}
			if (UVM_OBJ_IS_VTEXT(uobj)) {
				cpu_count(CPU_COUNT_EXECPAGES, 1);
			}
			cpu_count(CPU_COUNT_FILEUNKNOWN + status, 1);
		} else {
			cpu_count(CPU_COUNT_ANONUNKNOWN + status, 1);
		}
	}
	pg->flags |= PG_TABLED;
	uobj->uo_npages++;
}

static inline int
uvm_pageinsert_tree(struct uvm_object *uobj, struct vm_page *pg)
{
	const uint64_t idx = pg->offset >> PAGE_SHIFT;
	int error;

	KASSERT(rw_write_held(uobj->vmobjlock));

	error = radix_tree_insert_node(&uobj->uo_pages, idx, pg);
	if (error != 0) {
		return error;
	}
	if ((pg->flags & PG_CLEAN) == 0) {
		uvm_obj_page_set_dirty(pg);
	}
	KASSERT(((pg->flags & PG_CLEAN) == 0) ==
		uvm_obj_page_dirty_p(pg));
	return 0;
}

/*
 * uvm_page_remove: remove page from object.
 *
 * => caller must lock object
 */

static inline void
uvm_pageremove_object(struct uvm_object *uobj, struct vm_page *pg)
{

	KASSERT(uobj == pg->uobject);
	KASSERT(rw_write_held(uobj->vmobjlock));
	KASSERT(pg->flags & PG_TABLED);

	if ((pg->flags & PG_STAT) != 0) {
		/* Cannot use uvm_pagegetdirty(): no longer in radix tree. */
		const unsigned int status = pg->flags & (PG_CLEAN | PG_DIRTY);

		if ((pg->flags & PG_FILE) != 0) {
			if (uobj->uo_npages == 1) {
				struct vnode *vp = (struct vnode *)uobj;
				mutex_enter(vp->v_interlock);
				KASSERT((vp->v_iflag & VI_PAGES) != 0);
				vp->v_iflag &= ~VI_PAGES;
				holdrelel(vp);
				mutex_exit(vp->v_interlock);
			}
			if (UVM_OBJ_IS_VTEXT(uobj)) {
				cpu_count(CPU_COUNT_EXECPAGES, -1);
			}
			cpu_count(CPU_COUNT_FILEUNKNOWN + status, -1);
		} else {
			cpu_count(CPU_COUNT_ANONUNKNOWN + status, -1);
		}
	}
	uobj->uo_npages--;
	pg->flags &= ~PG_TABLED;
	pg->uobject = NULL;
}

static inline void
uvm_pageremove_tree(struct uvm_object *uobj, struct vm_page *pg)
{
	struct vm_page *opg __unused;

	KASSERT(rw_write_held(uobj->vmobjlock));

	opg = radix_tree_remove_node(&uobj->uo_pages, pg->offset >> PAGE_SHIFT);
	KASSERT(pg == opg);
}

static void
uvm_page_init_bucket(struct pgfreelist *pgfl, struct pgflbucket *pgb, int num)
{
	int i;

	pgb->pgb_nfree = 0;
	for (i = 0; i < uvmexp.ncolors; i++) {
		LIST_INIT(&pgb->pgb_colors[i]);
	}
	pgfl->pgfl_buckets[num] = pgb;
}

/*
 * uvm_page_init: init the page system.   called from uvm_init().
 *
 * => we return the range of kernel virtual memory in kvm_startp/kvm_endp
 */

void
uvm_page_init(vaddr_t *kvm_startp, vaddr_t *kvm_endp)
{
	static struct uvm_cpu boot_cpu __cacheline_aligned;
	psize_t freepages, pagecount, bucketsize, n;
	struct pgflbucket *pgb;
	struct vm_page *pagearray;
	char *bucketarray;
	uvm_physseg_t bank;
	int fl, b;

	KASSERT(ncpu <= 1);

	/*
	 * init the page queues and free page queue locks, except the
	 * free list; we allocate that later (with the initial vm_page
	 * structures).
	 */

	curcpu()->ci_data.cpu_uvm = &boot_cpu;
	uvmpdpol_init();
	for (b = 0; b < __arraycount(uvm_freelist_locks); b++) {
		mutex_init(&uvm_freelist_locks[b].lock, MUTEX_DEFAULT, IPL_VM);
	}

	/*
	 * allocate vm_page structures.
	 */

	/*
	 * sanity check:
	 * before calling this function the MD code is expected to register
	 * some free RAM with the uvm_page_physload() function.   our job
	 * now is to allocate vm_page structures for this memory.
	 */

	if (uvm_physseg_get_last() == UVM_PHYSSEG_TYPE_INVALID)
		panic("uvm_page_bootstrap: no memory pre-allocated");

	/*
	 * first calculate the number of free pages...
	 *
	 * note that we use start/end rather than avail_start/avail_end.
	 * this allows us to allocate extra vm_page structures in case we
	 * want to return some memory to the pool after booting.
	 */

	freepages = 0;

	for (bank = uvm_physseg_get_first();
	     uvm_physseg_valid_p(bank) ;
	     bank = uvm_physseg_get_next(bank)) {
		freepages += (uvm_physseg_get_end(bank) - uvm_physseg_get_start(bank));
	}

	/*
	 * Let MD code initialize the number of colors, or default
	 * to 1 color if MD code doesn't care.
	 */
	if (uvmexp.ncolors == 0)
		uvmexp.ncolors = 1;
	uvmexp.colormask = uvmexp.ncolors - 1;
	KASSERT((uvmexp.colormask & uvmexp.ncolors) == 0);

	/* We always start with only 1 bucket. */
	uvm.bucketcount = 1;

	/*
	 * we now know we have (PAGE_SIZE * freepages) bytes of memory we can
	 * use.   for each page of memory we use we need a vm_page structure.
	 * thus, the total number of pages we can use is the total size of
	 * the memory divided by the PAGE_SIZE plus the size of the vm_page
	 * structure.   we add one to freepages as a fudge factor to avoid
	 * truncation errors (since we can only allocate in terms of whole
	 * pages).
	 */
	pagecount = ((freepages + 1) << PAGE_SHIFT) /
	    (PAGE_SIZE + sizeof(struct vm_page));
	bucketsize = offsetof(struct pgflbucket, pgb_colors[uvmexp.ncolors]);
	bucketsize = roundup2(bucketsize, coherency_unit);
	bucketarray = (void *)uvm_pageboot_alloc(
	    bucketsize * VM_NFREELIST +
	    pagecount * sizeof(struct vm_page));
	pagearray = (struct vm_page *)
	    (bucketarray + bucketsize * VM_NFREELIST);

	for (fl = 0; fl < VM_NFREELIST; fl++) {
		pgb = (struct pgflbucket *)(bucketarray + bucketsize * fl);
		uvm_page_init_bucket(&uvm.page_free[fl], pgb, 0);
	}
	memset(pagearray, 0, pagecount * sizeof(struct vm_page));

	/*
	 * init the freelist cache in the disabled state.
	 */
	uvm_pgflcache_init();

	/*
	 * init the vm_page structures and put them in the correct place.
	 */
	/* First init the extent */

	for (bank = uvm_physseg_get_first(),
		 uvm_physseg_seg_chomp_slab(bank, pagearray, pagecount);
	     uvm_physseg_valid_p(bank);
	     bank = uvm_physseg_get_next(bank)) {

		n = uvm_physseg_get_end(bank) - uvm_physseg_get_start(bank);
		uvm_physseg_seg_alloc_from_slab(bank, n);
		uvm_physseg_init_seg(bank, pagearray);

		/* set up page array pointers */
		pagearray += n;
		pagecount -= n;
	}

	/*
	 * pass up the values of virtual_space_start and
	 * virtual_space_end (obtained by uvm_pageboot_alloc) to the upper
	 * layers of the VM.
	 */

	*kvm_startp = round_page(virtual_space_start);
	*kvm_endp = trunc_page(virtual_space_end);
#ifdef DEBUG
	/*
	 * steal kva for uvm_pagezerocheck().
	 */
	uvm_zerocheckkva = *kvm_startp;
	*kvm_startp += PAGE_SIZE;
	mutex_init(&uvm_zerochecklock, MUTEX_DEFAULT, IPL_VM);
#endif /* DEBUG */

	/*
	 * init various thresholds.
	 */

	uvmexp.reserve_pagedaemon = 1;
	uvmexp.reserve_kernel = vm_page_reserve_kernel;

	/*
	 * done!
	 */

	uvm.page_init_done = true;
}

/*
 * uvm_pgfl_lock: lock all freelist buckets
 */

void
uvm_pgfl_lock(void)
{
	int i;

	for (i = 0; i < __arraycount(uvm_freelist_locks); i++) {
		mutex_spin_enter(&uvm_freelist_locks[i].lock);
	}
}

/*
 * uvm_pgfl_unlock: unlock all freelist buckets
 */

void
uvm_pgfl_unlock(void)
{
	int i;

	for (i = 0; i < __arraycount(uvm_freelist_locks); i++) {
		mutex_spin_exit(&uvm_freelist_locks[i].lock);
	}
}

/*
 * uvm_setpagesize: set the page size
 *
 * => sets page_shift and page_mask from uvmexp.pagesize.
 */

void
uvm_setpagesize(void)
{

	/*
	 * If uvmexp.pagesize is 0 at this point, we expect PAGE_SIZE
	 * to be a constant (indicated by being a non-zero value).
	 */
	if (uvmexp.pagesize == 0) {
		if (PAGE_SIZE == 0)
			panic("uvm_setpagesize: uvmexp.pagesize not set");
		uvmexp.pagesize = PAGE_SIZE;
	}
	uvmexp.pagemask = uvmexp.pagesize - 1;
	if ((uvmexp.pagemask & uvmexp.pagesize) != 0)
		panic("uvm_setpagesize: page size %u (%#x) not a power of two",
		    uvmexp.pagesize, uvmexp.pagesize);
	for (uvmexp.pageshift = 0; ; uvmexp.pageshift++)
		if ((1 << uvmexp.pageshift) == uvmexp.pagesize)
			break;
}

/*
 * uvm_pageboot_alloc: steal memory from physmem for bootstrapping
 */

vaddr_t
uvm_pageboot_alloc(vsize_t size)
{
	static bool initialized = false;
	vaddr_t addr;
#if !defined(PMAP_STEAL_MEMORY)
	vaddr_t vaddr;
	paddr_t paddr;
#endif

	/*
	 * on first call to this function, initialize ourselves.
	 */
	if (initialized == false) {
		pmap_virtual_space(&virtual_space_start, &virtual_space_end);

		/* round it the way we like it */
		virtual_space_start = round_page(virtual_space_start);
		virtual_space_end = trunc_page(virtual_space_end);

		initialized = true;
	}

	/* round to page size */
	size = round_page(size);
	uvmexp.bootpages += atop(size);

#if defined(PMAP_STEAL_MEMORY)

	/*
	 * defer bootstrap allocation to MD code (it may want to allocate
	 * from a direct-mapped segment).  pmap_steal_memory should adjust
	 * virtual_space_start/virtual_space_end if necessary.
	 */

	addr = pmap_steal_memory(size, &virtual_space_start,
	    &virtual_space_end);

	return(addr);

#else /* !PMAP_STEAL_MEMORY */

	/*
	 * allocate virtual memory for this request
	 */
	if (virtual_space_start == virtual_space_end ||
	    (virtual_space_end - virtual_space_start) < size)
		panic("uvm_pageboot_alloc: out of virtual space");

	addr = virtual_space_start;

#ifdef PMAP_GROWKERNEL
	/*
	 * If the kernel pmap can't map the requested space,
	 * then allocate more resources for it.
	 */
	if (uvm_maxkaddr < (addr + size)) {
		uvm_maxkaddr = pmap_growkernel(addr + size);
		if (uvm_maxkaddr < (addr + size))
			panic("uvm_pageboot_alloc: pmap_growkernel() failed");
	}
#endif

	virtual_space_start += size;

	/*
	 * allocate and mapin physical pages to back new virtual pages
	 */

	for (vaddr = round_page(addr) ; vaddr < addr + size ;
	    vaddr += PAGE_SIZE) {

		if (!uvm_page_physget(&paddr))
			panic("uvm_pageboot_alloc: out of memory");

		/*
		 * Note this memory is no longer managed, so using
		 * pmap_kenter is safe.
		 */
		pmap_kenter_pa(vaddr, paddr, VM_PROT_READ|VM_PROT_WRITE, 0);
	}
	pmap_update(pmap_kernel());
	return(addr);
#endif	/* PMAP_STEAL_MEMORY */
}

#if !defined(PMAP_STEAL_MEMORY)
/*
 * uvm_page_physget: "steal" one page from the vm_physmem structure.
 *
 * => attempt to allocate it off the end of a segment in which the "avail"
 *    values match the start/end values.   if we can't do that, then we
 *    will advance both values (making them equal, and removing some
 *    vm_page structures from the non-avail area).
 * => return false if out of memory.
 */

/* subroutine: try to allocate from memory chunks on the specified freelist */
static bool uvm_page_physget_freelist(paddr_t *, int);

static bool
uvm_page_physget_freelist(paddr_t *paddrp, int freelist)
{
	uvm_physseg_t lcv;

	/* pass 1: try allocating from a matching end */
#if (VM_PHYSSEG_STRAT == VM_PSTRAT_BIGFIRST)
	for (lcv = uvm_physseg_get_last(); uvm_physseg_valid_p(lcv); lcv = uvm_physseg_get_prev(lcv))
#else
	for (lcv = uvm_physseg_get_first(); uvm_physseg_valid_p(lcv); lcv = uvm_physseg_get_next(lcv))
#endif
	{
		if (uvm.page_init_done == true)
			panic("uvm_page_physget: called _after_ bootstrap");

		/* Try to match at front or back on unused segment */
		if (uvm_page_physunload(lcv, freelist, paddrp))
			return true;
	}

	/* pass2: forget about matching ends, just allocate something */
#if (VM_PHYSSEG_STRAT == VM_PSTRAT_BIGFIRST)
	for (lcv = uvm_physseg_get_last(); uvm_physseg_valid_p(lcv); lcv = uvm_physseg_get_prev(lcv))
#else
	for (lcv = uvm_physseg_get_first(); uvm_physseg_valid_p(lcv); lcv = uvm_physseg_get_next(lcv))
#endif
	{
		/* Try the front regardless. */
		if (uvm_page_physunload_force(lcv, freelist, paddrp))
			return true;
	}
	return false;
}

bool
uvm_page_physget(paddr_t *paddrp)
{
	int i;

	/* try in the order of freelist preference */
	for (i = 0; i < VM_NFREELIST; i++)
		if (uvm_page_physget_freelist(paddrp, i) == true)
			return (true);
	return (false);
}
#endif /* PMAP_STEAL_MEMORY */

/*
 * PHYS_TO_VM_PAGE: find vm_page for a PA.   used by MI code to get vm_pages
 * back from an I/O mapping (ugh!).   used in some MD code as well.
 */
struct vm_page *
uvm_phys_to_vm_page(paddr_t pa)
{
	paddr_t pf = atop(pa);
	paddr_t	off;
	uvm_physseg_t	upm;

	upm = uvm_physseg_find(pf, &off);
	if (upm != UVM_PHYSSEG_TYPE_INVALID)
		return uvm_physseg_get_pg(upm, off);
	return(NULL);
}

paddr_t
uvm_vm_page_to_phys(const struct vm_page *pg)
{

	return pg->phys_addr & ~(PAGE_SIZE - 1);
}

/*
 * uvm_page_numa_load: load NUMA range description.
 */
void
uvm_page_numa_load(paddr_t start, paddr_t size, u_int numa_id)
{
	struct uvm_page_numa_region *d;

	KASSERT(numa_id < PGFL_MAX_BUCKETS);

	d = kmem_alloc(sizeof(*d), KM_SLEEP);
	d->start = start;
	d->size = size;
	d->numa_id = numa_id;
	d->next = uvm_page_numa_region;
	uvm_page_numa_region = d;
}

/*
 * uvm_page_numa_lookup: lookup NUMA node for the given page.
 */
static u_int
uvm_page_numa_lookup(struct vm_page *pg)
{
	struct uvm_page_numa_region *d;
	static bool warned;
	paddr_t pa;

	KASSERT(uvm_page_numa_region != NULL);

	pa = VM_PAGE_TO_PHYS(pg);
	for (d = uvm_page_numa_region; d != NULL; d = d->next) {
		if (pa >= d->start && pa < d->start + d->size) {
			return d->numa_id;
		}
	}

	if (!warned) {
		printf("uvm_page_numa_lookup: failed, first pg=%p pa=%#"
		    PRIxPADDR "\n", pg, VM_PAGE_TO_PHYS(pg));
		warned = true;
	}

	return 0;
}

/*
 * uvm_page_redim: adjust freelist dimensions if they have changed.
 */

static void
uvm_page_redim(int newncolors, int newnbuckets)
{
	struct pgfreelist npgfl;
	struct pgflbucket *opgb, *npgb;
	struct pgflist *ohead, *nhead;
	struct vm_page *pg;
	size_t bucketsize, bucketmemsize, oldbucketmemsize;
	int fl, ob, oc, nb, nc, obuckets, ocolors;
	char *bucketarray, *oldbucketmem, *bucketmem;

	KASSERT(((newncolors - 1) & newncolors) == 0);

	/* Anything to do? */
	if (newncolors <= uvmexp.ncolors &&
	    newnbuckets == uvm.bucketcount) {
		return;
	}
	if (uvm.page_init_done == false) {
		uvmexp.ncolors = newncolors;
		return;
	}

	bucketsize = offsetof(struct pgflbucket, pgb_colors[newncolors]);
	bucketsize = roundup2(bucketsize, coherency_unit);
	bucketmemsize = bucketsize * newnbuckets * VM_NFREELIST +
	    coherency_unit - 1;
	bucketmem = kmem_zalloc(bucketmemsize, KM_SLEEP);
	bucketarray = (char *)roundup2((uintptr_t)bucketmem, coherency_unit);

	ocolors = uvmexp.ncolors;
	obuckets = uvm.bucketcount;

	/* Freelist cache musn't be enabled. */
	uvm_pgflcache_pause();

	/* Make sure we should still do this. */
	uvm_pgfl_lock();
	if (newncolors <= uvmexp.ncolors &&
	    newnbuckets == uvm.bucketcount) {
		uvm_pgfl_unlock();
		uvm_pgflcache_resume();
		kmem_free(bucketmem, bucketmemsize);
		return;
	}

	uvmexp.ncolors = newncolors;
	uvmexp.colormask = uvmexp.ncolors - 1;
	uvm.bucketcount = newnbuckets;

	for (fl = 0; fl < VM_NFREELIST; fl++) {
		/* Init new buckets in new freelist. */
		memset(&npgfl, 0, sizeof(npgfl));
		for (nb = 0; nb < newnbuckets; nb++) {
			npgb = (struct pgflbucket *)bucketarray;
			uvm_page_init_bucket(&npgfl, npgb, nb);
			bucketarray += bucketsize;
		}
		/* Now transfer pages from the old freelist. */
		for (nb = ob = 0; ob < obuckets; ob++) {
			opgb = uvm.page_free[fl].pgfl_buckets[ob];
			for (oc = 0; oc < ocolors; oc++) {
				ohead = &opgb->pgb_colors[oc];
				while ((pg = LIST_FIRST(ohead)) != NULL) {
					LIST_REMOVE(pg, pageq.list);
					/*
					 * Here we decide on the NEW color &
					 * bucket for the page.  For NUMA
					 * we'll use the info that the
					 * hardware gave us.  For non-NUMA
					 * assign take physical page frame
					 * number and cache color into
					 * account.  We do this to try and
					 * avoid defeating any memory
					 * interleaving in the hardware.
					 */
					KASSERT(
					    uvm_page_get_bucket(pg) == ob);
					KASSERT(fl ==
					    uvm_page_get_freelist(pg));
					if (uvm_page_numa_region != NULL) {
						nb = uvm_page_numa_lookup(pg);
					} else {
						nb = atop(VM_PAGE_TO_PHYS(pg))
						    / uvmexp.ncolors / 8
						    % newnbuckets;
					}
					uvm_page_set_bucket(pg, nb);
					npgb = npgfl.pgfl_buckets[nb];
					npgb->pgb_nfree++;
					nc = VM_PGCOLOR(pg);
					nhead = &npgb->pgb_colors[nc];
					LIST_INSERT_HEAD(nhead, pg, pageq.list);
				}
			}
		}
		/* Install the new freelist. */
		memcpy(&uvm.page_free[fl], &npgfl, sizeof(npgfl));
	}

	/* Unlock and free the old memory. */
	oldbucketmemsize = recolored_pages_memsize;
	oldbucketmem = recolored_pages_mem;
	recolored_pages_memsize = bucketmemsize;
	recolored_pages_mem = bucketmem;

	uvm_pgfl_unlock();
	uvm_pgflcache_resume();

	if (oldbucketmemsize) {
		kmem_free(oldbucketmem, oldbucketmemsize);
	}

	/*
	 * this calls uvm_km_alloc() which may want to hold
	 * uvm_freelist_lock.
	 */
	uvm_pager_realloc_emerg();
}

/*
 * uvm_page_recolor: Recolor the pages if the new color count is
 * larger than the old one.
 */

void
uvm_page_recolor(int newncolors)
{

	uvm_page_redim(newncolors, uvm.bucketcount);
}

/*
 * uvm_page_rebucket: Determine a bucket structure and redim the free
 * lists to match.
 */

void
uvm_page_rebucket(void)
{
	u_int min_numa, max_numa, npackage, shift;
	struct cpu_info *ci, *ci2, *ci3;
	CPU_INFO_ITERATOR cii;

	/*
	 * If we have more than one NUMA node, and the maximum NUMA node ID
	 * is less than PGFL_MAX_BUCKETS, then we'll use NUMA distribution
	 * for free pages.
	 */
	min_numa = (u_int)-1;
	max_numa = 0;
	for (CPU_INFO_FOREACH(cii, ci)) {
		if (ci->ci_numa_id < min_numa) {
			min_numa = ci->ci_numa_id;
		}
		if (ci->ci_numa_id > max_numa) {
			max_numa = ci->ci_numa_id;
		}
	}
	if (min_numa != max_numa && max_numa < PGFL_MAX_BUCKETS) {
		aprint_debug("UVM: using NUMA allocation scheme\n");
		for (CPU_INFO_FOREACH(cii, ci)) {
			ci->ci_data.cpu_uvm->pgflbucket = ci->ci_numa_id;
		}
	 	uvm_page_redim(uvmexp.ncolors, max_numa + 1);
	 	return;
	}

	/*
	 * Otherwise we'll go with a scheme to maximise L2/L3 cache locality
	 * and minimise lock contention.  Count the total number of CPU
	 * packages, and then try to distribute the buckets among CPU
	 * packages evenly.
	 */
	npackage = curcpu()->ci_nsibling[CPUREL_PACKAGE1ST];

	/*
	 * Figure out how to arrange the packages & buckets, and the total
	 * number of buckets we need.  XXX 2 may not be the best factor.
	 */
	for (shift = 0; npackage > PGFL_MAX_BUCKETS; shift++) {
		npackage >>= 1;
	}
 	uvm_page_redim(uvmexp.ncolors, npackage);

 	/*
 	 * Now tell each CPU which bucket to use.  In the outer loop, scroll
 	 * through all CPU packages.
 	 */
 	npackage = 0;
	ci = curcpu();
	ci2 = ci->ci_sibling[CPUREL_PACKAGE1ST];
	do {
		/*
		 * In the inner loop, scroll through all CPUs in the package
		 * and assign the same bucket ID.
		 */
		ci3 = ci2;
		do {
			ci3->ci_data.cpu_uvm->pgflbucket = npackage >> shift;
			ci3 = ci3->ci_sibling[CPUREL_PACKAGE];
		} while (ci3 != ci2);
		npackage++;
		ci2 = ci2->ci_sibling[CPUREL_PACKAGE1ST];
	} while (ci2 != ci->ci_sibling[CPUREL_PACKAGE1ST]);

	aprint_debug("UVM: using package allocation scheme, "
	    "%d package(s) per bucket\n", 1 << shift);
}

/*
 * uvm_cpu_attach: initialize per-CPU data structures.
 */

void
uvm_cpu_attach(struct cpu_info *ci)
{
	struct uvm_cpu *ucpu;

	/* Already done in uvm_page_init(). */
	if (!CPU_IS_PRIMARY(ci)) {
		/* Add more reserve pages for this CPU. */
		uvmexp.reserve_kernel += vm_page_reserve_kernel;

		/* Allocate per-CPU data structures. */
		ucpu = kmem_zalloc(sizeof(struct uvm_cpu) + coherency_unit - 1,
		    KM_SLEEP);
		ucpu = (struct uvm_cpu *)roundup2((uintptr_t)ucpu,
		    coherency_unit);
		ci->ci_data.cpu_uvm = ucpu;
	} else {
		ucpu = ci->ci_data.cpu_uvm;
	}

	uvmpdpol_init_cpu(ucpu);

	/*
	 * Attach RNG source for this CPU's VM events
	 */
        rnd_attach_source(&ucpu->rs, ci->ci_data.cpu_name, RND_TYPE_VM,
	    RND_FLAG_COLLECT_TIME|RND_FLAG_COLLECT_VALUE|
	    RND_FLAG_ESTIMATE_VALUE);
}

/*
 * uvm_availmem: fetch the total amount of free memory in pages.  this can
 * have a detrimental effect on performance due to false sharing; don't call
 * unless needed.
 *
 * some users can request the amount of free memory so often that it begins
 * to impact upon performance.  if calling frequently and an inexact value
 * is okay, call with cached = true.
 */

int
uvm_availmem(bool cached)
{
	int64_t fp;

	cpu_count_sync(cached);
	if ((fp = cpu_count_get(CPU_COUNT_FREEPAGES)) < 0) {
		/*
		 * XXXAD could briefly go negative because it's impossible
		 * to get a clean snapshot.  address this for other counters
		 * used as running totals before NetBSD 10 although less
		 * important for those.
		 */
		fp = 0;
	}
	return (int)fp;
}

/*
 * uvm_pagealloc_pgb: helper routine that tries to allocate any color from a
 * specific freelist and specific bucket only.
 *
 * => must be at IPL_VM or higher to protect per-CPU data structures.
 */

static struct vm_page *
uvm_pagealloc_pgb(struct uvm_cpu *ucpu, int f, int b, int *trycolorp, int flags)
{
	int c, trycolor, colormask;
	struct pgflbucket *pgb;
	struct vm_page *pg;
	kmutex_t *lock;
	bool fill;

	/*
	 * Skip the bucket if empty, no lock needed.  There could be many
	 * empty freelists/buckets.
	 */
	pgb = uvm.page_free[f].pgfl_buckets[b];
	if (pgb->pgb_nfree == 0) {
		return NULL;
	}

	/* Skip bucket if low on memory. */
	lock = &uvm_freelist_locks[b].lock;
	mutex_spin_enter(lock);
	if (__predict_false(pgb->pgb_nfree <= uvmexp.reserve_kernel)) {
		if ((flags & UVM_PGA_USERESERVE) == 0 ||
		    (pgb->pgb_nfree <= uvmexp.reserve_pagedaemon &&
		     curlwp != uvm.pagedaemon_lwp)) {
			mutex_spin_exit(lock);
		     	return NULL;
		}
		fill = false;
	} else {
		fill = true;
	}

	/* Try all page colors as needed. */
	c = trycolor = *trycolorp;
	colormask = uvmexp.colormask;
	do {
		pg = LIST_FIRST(&pgb->pgb_colors[c]);
		if (__predict_true(pg != NULL)) {
			/*
			 * Got a free page!  PG_FREE must be cleared under
			 * lock because of uvm_pglistalloc().
			 */
			LIST_REMOVE(pg, pageq.list);
			KASSERT(pg->flags == PG_FREE);
			pg->flags = PG_BUSY | PG_CLEAN | PG_FAKE;
			pgb->pgb_nfree--;

			/*
			 * While we have the bucket locked and our data
			 * structures fresh in L1 cache, we have an ideal
			 * opportunity to grab some pages for the freelist
			 * cache without causing extra contention.  Only do
			 * so if we found pages in this CPU's preferred
			 * bucket.
			 */
			if (__predict_true(b == ucpu->pgflbucket && fill)) {
				uvm_pgflcache_fill(ucpu, f, b, c);
			}
			mutex_spin_exit(lock);
			KASSERT(uvm_page_get_bucket(pg) == b);
			CPU_COUNT(c == trycolor ?
			    CPU_COUNT_COLORHIT : CPU_COUNT_COLORMISS, 1);
			CPU_COUNT(CPU_COUNT_CPUMISS, 1);
			*trycolorp = c;
			return pg;
		}
		c = (c + 1) & colormask;
	} while (c != trycolor);
	mutex_spin_exit(lock);

	return NULL;
}

/*
 * uvm_pagealloc_pgfl: helper routine for uvm_pagealloc_strat that allocates
 * any color from any bucket, in a specific freelist.
 *
 * => must be at IPL_VM or higher to protect per-CPU data structures.
 */

static struct vm_page *
uvm_pagealloc_pgfl(struct uvm_cpu *ucpu, int f, int *trycolorp, int flags)
{
	int b, trybucket, bucketcount;
	struct vm_page *pg;

	/* Try for the exact thing in the per-CPU cache. */
	if ((pg = uvm_pgflcache_alloc(ucpu, f, *trycolorp)) != NULL) {
		CPU_COUNT(CPU_COUNT_CPUHIT, 1);
		CPU_COUNT(CPU_COUNT_COLORHIT, 1);
		return pg;
	}

	/* Walk through all buckets, trying our preferred bucket first. */
	trybucket = ucpu->pgflbucket;
	b = trybucket;
	bucketcount = uvm.bucketcount;
	do {
		pg = uvm_pagealloc_pgb(ucpu, f, b, trycolorp, flags);
		if (pg != NULL) {
			return pg;
		}
		b = (b + 1 == bucketcount ? 0 : b + 1);
	} while (b != trybucket);

	return NULL;
}

/*
 * uvm_pagealloc_strat: allocate vm_page from a particular free list.
 *
 * => return null if no pages free
 * => wake up pagedaemon if number of free pages drops below low water mark
 * => if obj != NULL, obj must be locked (to put in obj's tree)
 * => if anon != NULL, anon must be locked (to put in anon)
 * => only one of obj or anon can be non-null
 * => caller must activate/deactivate page if it is not wired.
 * => free_list is ignored if strat == UVM_PGA_STRAT_NORMAL.
 * => policy decision: it is more important to pull a page off of the
 *	appropriate priority free list than it is to get a page from the
 *	correct bucket or color bin.  This is because we live with the
 *	consequences of a bad free list decision for the entire
 *	lifetime of the page, e.g. if the page comes from memory that
 *	is slower to access.
 */

struct vm_page *
uvm_pagealloc_strat(struct uvm_object *obj, voff_t off, struct vm_anon *anon,
    int flags, int strat, int free_list)
{
	int color, lcv, error, s;
	struct uvm_cpu *ucpu;
	struct vm_page *pg;
	lwp_t *l;

	KASSERT(obj == NULL || anon == NULL);
	KASSERT(anon == NULL || (flags & UVM_FLAG_COLORMATCH) || off == 0);
	KASSERT(off == trunc_page(off));
	KASSERT(obj == NULL || rw_write_held(obj->vmobjlock));
	KASSERT(anon == NULL || anon->an_lock == NULL ||
	    rw_write_held(anon->an_lock));

	/*
	 * This implements a global round-robin page coloring
	 * algorithm.
	 */

	s = splvm();
	ucpu = curcpu()->ci_data.cpu_uvm;
	if (flags & UVM_FLAG_COLORMATCH) {
		color = atop(off) & uvmexp.colormask;
	} else {
		color = ucpu->pgflcolor;
	}

	/*
	 * fail if any of these conditions is true:
	 * [1]  there really are no free pages, or
	 * [2]  only kernel "reserved" pages remain and
	 *        reserved pages have not been requested.
	 * [3]  only pagedaemon "reserved" pages remain and
	 *        the requestor isn't the pagedaemon.
	 * we make kernel reserve pages available if called by a
	 * kernel thread.
	 */
	l = curlwp;
	if (__predict_true(l != NULL) && (l->l_flag & LW_SYSTEM) != 0) {
		flags |= UVM_PGA_USERESERVE;
	}

 again:
	switch (strat) {
	case UVM_PGA_STRAT_NORMAL:
		/* Check freelists: descending priority (ascending id) order. */
		for (lcv = 0; lcv < VM_NFREELIST; lcv++) {
			pg = uvm_pagealloc_pgfl(ucpu, lcv, &color, flags);
			if (pg != NULL) {
				goto gotit;
			}
		}

		/* No pages free!  Have pagedaemon free some memory. */
		splx(s);
		uvm_kick_pdaemon();
		return NULL;

	case UVM_PGA_STRAT_ONLY:
	case UVM_PGA_STRAT_FALLBACK:
		/* Attempt to allocate from the specified free list. */
		KASSERT(free_list >= 0 && free_list < VM_NFREELIST);
		pg = uvm_pagealloc_pgfl(ucpu, free_list, &color, flags);
		if (pg != NULL) {
			goto gotit;
		}

		/* Fall back, if possible. */
		if (strat == UVM_PGA_STRAT_FALLBACK) {
			strat = UVM_PGA_STRAT_NORMAL;
			goto again;
		}

		/* No pages free!  Have pagedaemon free some memory. */
		splx(s);
		uvm_kick_pdaemon();
		return NULL;

	case UVM_PGA_STRAT_NUMA:
		/*
		 * NUMA strategy (experimental): allocating from the correct
		 * bucket is more important than observing freelist
		 * priority.  Look only to the current NUMA node; if that
		 * fails, we need to look to other NUMA nodes, so retry with
		 * the normal strategy.
		 */
		for (lcv = 0; lcv < VM_NFREELIST; lcv++) {
			pg = uvm_pgflcache_alloc(ucpu, lcv, color);
			if (pg != NULL) {
				CPU_COUNT(CPU_COUNT_CPUHIT, 1);
				CPU_COUNT(CPU_COUNT_COLORHIT, 1);
				goto gotit;
			}
			pg = uvm_pagealloc_pgb(ucpu, lcv,
			    ucpu->pgflbucket, &color, flags);
			if (pg != NULL) {
				goto gotit;
			}
		}
		strat = UVM_PGA_STRAT_NORMAL;
		goto again;

	default:
		panic("uvm_pagealloc_strat: bad strat %d", strat);
		/* NOTREACHED */
	}

 gotit:
	/*
	 * We now know which color we actually allocated from; set
	 * the next color accordingly.
	 */

	ucpu->pgflcolor = (color + 1) & uvmexp.colormask;

	/*
	 * while still at IPL_VM, update allocation statistics.
	 */

    	CPU_COUNT(CPU_COUNT_FREEPAGES, -1);
	if (anon) {
		CPU_COUNT(CPU_COUNT_ANONCLEAN, 1);
	}
	splx(s);
	KASSERT(pg->flags == (PG_BUSY|PG_CLEAN|PG_FAKE));

	/*
	 * assign the page to the object.  as the page was free, we know
	 * that pg->uobject and pg->uanon are NULL.  we only need to take
	 * the page's interlock if we are changing the values.
	 */
	if (anon != NULL || obj != NULL) {
		mutex_enter(&pg->interlock);
	}
	pg->offset = off;
	pg->uobject = obj;
	pg->uanon = anon;
	KASSERT(uvm_page_owner_locked_p(pg, true));
	if (anon) {
		anon->an_page = pg;
		pg->flags |= PG_ANON;
		mutex_exit(&pg->interlock);
	} else if (obj) {
		/*
		 * set PG_FILE|PG_AOBJ before the first uvm_pageinsert.
		 */
		if (UVM_OBJ_IS_VNODE(obj)) {
			pg->flags |= PG_FILE;
		} else if (UVM_OBJ_IS_AOBJ(obj)) {
			pg->flags |= PG_AOBJ;
		}
		uvm_pageinsert_object(obj, pg);
		mutex_exit(&pg->interlock);
		error = uvm_pageinsert_tree(obj, pg);
		if (error != 0) {
			mutex_enter(&pg->interlock);
			uvm_pageremove_object(obj, pg);
			mutex_exit(&pg->interlock);
			uvm_pagefree(pg);
			return NULL;
		}
	}

#if defined(UVM_PAGE_TRKOWN)
	pg->owner_tag = NULL;
#endif
	UVM_PAGE_OWN(pg, "new alloc");

	if (flags & UVM_PGA_ZERO) {
		/* A zero'd page is not clean. */
		if (obj != NULL || anon != NULL) {
			uvm_pagemarkdirty(pg, UVM_PAGE_STATUS_DIRTY);
		}
		pmap_zero_page(VM_PAGE_TO_PHYS(pg));
	}

	return(pg);
}

/*
 * uvm_pagereplace: replace a page with another
 *
 * => object must be locked
 * => page interlocks must be held
 */

void
uvm_pagereplace(struct vm_page *oldpg, struct vm_page *newpg)
{
	struct uvm_object *uobj = oldpg->uobject;
	struct vm_page *pg __diagused;
	uint64_t idx;

	KASSERT((oldpg->flags & PG_TABLED) != 0);
	KASSERT(uobj != NULL);
	KASSERT((newpg->flags & PG_TABLED) == 0);
	KASSERT(newpg->uobject == NULL);
	KASSERT(rw_write_held(uobj->vmobjlock));
	KASSERT(mutex_owned(&oldpg->interlock));
	KASSERT(mutex_owned(&newpg->interlock));

	newpg->uobject = uobj;
	newpg->offset = oldpg->offset;
	idx = newpg->offset >> PAGE_SHIFT;
	pg = radix_tree_replace_node(&uobj->uo_pages, idx, newpg);
	KASSERT(pg == oldpg);
	if (((oldpg->flags ^ newpg->flags) & PG_CLEAN) != 0) {
		if ((newpg->flags & PG_CLEAN) != 0) {
			uvm_obj_page_clear_dirty(newpg);
		} else {
			uvm_obj_page_set_dirty(newpg);
		}
	}
	/*
	 * oldpg's PG_STAT is stable.  newpg is not reachable by others yet.
	 */
	newpg->flags |=
	    (newpg->flags & ~PG_STAT) | (oldpg->flags & PG_STAT);
	uvm_pageinsert_object(uobj, newpg);
	uvm_pageremove_object(uobj, oldpg);
}

/*
 * uvm_pagerealloc: reallocate a page from one object to another
 *
 * => both objects must be locked
 */

int
uvm_pagerealloc(struct vm_page *pg, struct uvm_object *newobj, voff_t newoff)
{
	int error = 0;

	/*
	 * remove it from the old object
	 */

	if (pg->uobject) {
		uvm_pageremove_tree(pg->uobject, pg);
		uvm_pageremove_object(pg->uobject, pg);
	}

	/*
	 * put it in the new object
	 */

	if (newobj) {
		mutex_enter(&pg->interlock);
		pg->uobject = newobj;
		pg->offset = newoff;
		if (UVM_OBJ_IS_VNODE(newobj)) {
			pg->flags |= PG_FILE;
		} else if (UVM_OBJ_IS_AOBJ(newobj)) {
			pg->flags |= PG_AOBJ;
		}
		uvm_pageinsert_object(newobj, pg);
		mutex_exit(&pg->interlock);
		error = uvm_pageinsert_tree(newobj, pg);
		if (error != 0) {
			mutex_enter(&pg->interlock);
			uvm_pageremove_object(newobj, pg);
			mutex_exit(&pg->interlock);
		}
	}

	return error;
}

#ifdef DEBUG
/*
 * check if page is zero-filled
 */
void
uvm_pagezerocheck(struct vm_page *pg)
{
	int *p, *ep;

	KASSERT(uvm_zerocheckkva != 0);

	/*
	 * XXX assuming pmap_kenter_pa and pmap_kremove never call
	 * uvm page allocator.
	 *
	 * it might be better to have "CPU-local temporary map" pmap interface.
	 */
	mutex_spin_enter(&uvm_zerochecklock);
	pmap_kenter_pa(uvm_zerocheckkva, VM_PAGE_TO_PHYS(pg), VM_PROT_READ, 0);
	p = (int *)uvm_zerocheckkva;
	ep = (int *)((char *)p + PAGE_SIZE);
	pmap_update(pmap_kernel());
	while (p < ep) {
		if (*p != 0)
			panic("zero page isn't zero-filled");
		p++;
	}
	pmap_kremove(uvm_zerocheckkva, PAGE_SIZE);
	mutex_spin_exit(&uvm_zerochecklock);
	/*
	 * pmap_update() is not necessary here because no one except us
	 * uses this VA.
	 */
}
#endif /* DEBUG */

/*
 * uvm_pagefree: free page
 *
 * => erase page's identity (i.e. remove from object)
 * => put page on free list
 * => caller must lock owning object (either anon or uvm_object)
 * => assumes all valid mappings of pg are gone
 */

void
uvm_pagefree(struct vm_page *pg)
{
	struct pgfreelist *pgfl;
	struct pgflbucket *pgb;
	struct uvm_cpu *ucpu;
	kmutex_t *lock;
	int bucket, s;
	bool locked;

#ifdef DEBUG
	if (pg->uobject == (void *)0xdeadbeef &&
	    pg->uanon == (void *)0xdeadbeef) {
		panic("uvm_pagefree: freeing free page %p", pg);
	}
#endif /* DEBUG */

	KASSERT((pg->flags & PG_PAGEOUT) == 0);
	KASSERT(!(pg->flags & PG_FREE));
	KASSERT(pg->uobject == NULL || rw_write_held(pg->uobject->vmobjlock));
	KASSERT(pg->uobject != NULL || pg->uanon == NULL ||
		rw_write_held(pg->uanon->an_lock));

	/*
	 * remove the page from the object's tree before acquiring any page
	 * interlocks: this can acquire locks to free radixtree nodes.
	 */
	if (pg->uobject != NULL) {
		uvm_pageremove_tree(pg->uobject, pg);
	}

	/*
	 * if the page is loaned, resolve the loan instead of freeing.
	 */

	if (pg->loan_count) {
		KASSERT(pg->wire_count == 0);

		/*
		 * if the page is owned by an anon then we just want to
		 * drop anon ownership.  the kernel will free the page when
		 * it is done with it.  if the page is owned by an object,
		 * remove it from the object and mark it dirty for the benefit
		 * of possible anon owners.
		 *
		 * regardless of previous ownership, wakeup any waiters,
		 * unbusy the page, and we're done.
		 */

		uvm_pagelock(pg);
		locked = true;
		if (pg->uobject != NULL) {
			uvm_pageremove_object(pg->uobject, pg);
			pg->flags &= ~(PG_FILE|PG_AOBJ);
		} else if (pg->uanon != NULL) {
			if ((pg->flags & PG_ANON) == 0) {
				pg->loan_count--;
			} else {
				const unsigned status = uvm_pagegetdirty(pg);
				pg->flags &= ~PG_ANON;
				cpu_count(CPU_COUNT_ANONUNKNOWN + status, -1);
			}
			pg->uanon->an_page = NULL;
			pg->uanon = NULL;
		}
		if (pg->pqflags & PQ_WANTED) {
			wakeup(pg);
		}
		pg->pqflags &= ~PQ_WANTED;
		pg->flags &= ~(PG_BUSY|PG_RELEASED|PG_PAGER1);
#ifdef UVM_PAGE_TRKOWN
		pg->owner_tag = NULL;
#endif
		KASSERT((pg->flags & PG_STAT) == 0);
		if (pg->loan_count) {
			KASSERT(pg->uobject == NULL);
			if (pg->uanon == NULL) {
				uvm_pagedequeue(pg);
			}
			uvm_pageunlock(pg);
			return;
		}
	} else if (pg->uobject != NULL || pg->uanon != NULL ||
	           pg->wire_count != 0) {
		uvm_pagelock(pg);
		locked = true;
	} else {
		locked = false;
	}

	/*
	 * remove page from its object or anon.
	 */
	if (pg->uobject != NULL) {
		uvm_pageremove_object(pg->uobject, pg);
	} else if (pg->uanon != NULL) {
		const unsigned int status = uvm_pagegetdirty(pg);
		pg->uanon->an_page = NULL;
		pg->uanon = NULL;
		cpu_count(CPU_COUNT_ANONUNKNOWN + status, -1);
	}

	/*
	 * if the page was wired, unwire it now.
	 */

	if (pg->wire_count) {
		pg->wire_count = 0;
		atomic_dec_uint(&uvmexp.wired);
	}
	if (locked) {
		/*
		 * wake anyone waiting on the page.
		 */
		if ((pg->pqflags & PQ_WANTED) != 0) {
			pg->pqflags &= ~PQ_WANTED;
			wakeup(pg);
		}

		/*
		 * now remove the page from the queues.
		 */
		uvm_pagedequeue(pg);
		uvm_pageunlock(pg);
	} else {
		KASSERT(!uvmpdpol_pageisqueued_p(pg));
	}

	/*
	 * and put on free queue
	 */

#ifdef DEBUG
	pg->uobject = (void *)0xdeadbeef;
	pg->uanon = (void *)0xdeadbeef;
#endif /* DEBUG */

	/* Try to send the page to the per-CPU cache. */
	s = splvm();
    	CPU_COUNT(CPU_COUNT_FREEPAGES, 1);
	ucpu = curcpu()->ci_data.cpu_uvm;
	bucket = uvm_page_get_bucket(pg);
	if (bucket == ucpu->pgflbucket && uvm_pgflcache_free(ucpu, pg)) {
		splx(s);
		return;
	}

	/* Didn't work.  Never mind, send it to a global bucket. */
	pgfl = &uvm.page_free[uvm_page_get_freelist(pg)];
	pgb = pgfl->pgfl_buckets[bucket];
	lock = &uvm_freelist_locks[bucket].lock;

	mutex_spin_enter(lock);
	/* PG_FREE must be set under lock because of uvm_pglistalloc(). */
	pg->flags = PG_FREE;
	LIST_INSERT_HEAD(&pgb->pgb_colors[VM_PGCOLOR(pg)], pg, pageq.list);
	pgb->pgb_nfree++;
	mutex_spin_exit(lock);
	splx(s);
}

/*
 * uvm_page_unbusy: unbusy an array of pages.
 *
 * => pages must either all belong to the same object, or all belong to anons.
 * => if pages are object-owned, object must be locked.
 * => if pages are anon-owned, anons must be locked.
 * => caller must make sure that anon-owned pages are not PG_RELEASED.
 */

void
uvm_page_unbusy(struct vm_page **pgs, int npgs)
{
	struct vm_page *pg;
	int i;
	UVMHIST_FUNC(__func__); UVMHIST_CALLED(ubchist);

	for (i = 0; i < npgs; i++) {
		pg = pgs[i];
		if (pg == NULL || pg == PGO_DONTCARE) {
			continue;
		}

		KASSERT(uvm_page_owner_locked_p(pg, true));
		KASSERT(pg->flags & PG_BUSY);
		KASSERT((pg->flags & PG_PAGEOUT) == 0);
		if (pg->flags & PG_RELEASED) {
			UVMHIST_LOG(ubchist, "releasing pg %#jx",
			    (uintptr_t)pg, 0, 0, 0);
			KASSERT(pg->uobject != NULL ||
			    (pg->uanon != NULL && pg->uanon->an_ref > 0));
			pg->flags &= ~PG_RELEASED;
			uvm_pagefree(pg);
		} else {
			UVMHIST_LOG(ubchist, "unbusying pg %#jx",
			    (uintptr_t)pg, 0, 0, 0);
			KASSERT((pg->flags & PG_FAKE) == 0);
			pg->flags &= ~PG_BUSY;
			uvm_pagelock(pg);
			uvm_pagewakeup(pg);
			uvm_pageunlock(pg);
			UVM_PAGE_OWN(pg, NULL);
		}
	}
}

/*
 * uvm_pagewait: wait for a busy page
 *
 * => page must be known PG_BUSY
 * => object must be read or write locked
 * => object will be unlocked on return
 */

void
uvm_pagewait(struct vm_page *pg, krwlock_t *lock, const char *wmesg)
{

	KASSERT(rw_lock_held(lock));
	KASSERT((pg->flags & PG_BUSY) != 0);
	KASSERT(uvm_page_owner_locked_p(pg, false));

	mutex_enter(&pg->interlock);
	pg->pqflags |= PQ_WANTED;
	rw_exit(lock);
	UVM_UNLOCK_AND_WAIT(pg, &pg->interlock, false, wmesg, 0);
}

/*
 * uvm_pagewakeup: wake anyone waiting on a page
 *
 * => page interlock must be held
 */

void
uvm_pagewakeup(struct vm_page *pg)
{
	UVMHIST_FUNC(__func__); UVMHIST_CALLED(ubchist);

	KASSERT(mutex_owned(&pg->interlock));

	UVMHIST_LOG(ubchist, "waking pg %#jx", (uintptr_t)pg, 0, 0, 0);

	if ((pg->pqflags & PQ_WANTED) != 0) {
		wakeup(pg);
		pg->pqflags &= ~PQ_WANTED;
	}
}

/*
 * uvm_pagewanted_p: return true if someone is waiting on the page
 *
 * => object must be write locked (lock out all concurrent access)
 */

bool
uvm_pagewanted_p(struct vm_page *pg)
{

	KASSERT(uvm_page_owner_locked_p(pg, true));

	return (atomic_load_relaxed(&pg->pqflags) & PQ_WANTED) != 0;
}

#if defined(UVM_PAGE_TRKOWN)
/*
 * uvm_page_own: set or release page ownership
 *
 * => this is a debugging function that keeps track of who sets PG_BUSY
 *	and where they do it.   it can be used to track down problems
 *	such a process setting "PG_BUSY" and never releasing it.
 * => page's object [if any] must be locked
 * => if "tag" is NULL then we are releasing page ownership
 */
void
uvm_page_own(struct vm_page *pg, const char *tag)
{

	KASSERT((pg->flags & (PG_PAGEOUT|PG_RELEASED)) == 0);
	KASSERT(uvm_page_owner_locked_p(pg, true));

	/* gain ownership? */
	if (tag) {
		KASSERT((pg->flags & PG_BUSY) != 0);
		if (pg->owner_tag) {
			printf("uvm_page_own: page %p already owned "
			    "by proc %d.%d [%s]\n", pg,
			    pg->owner, pg->lowner, pg->owner_tag);
			panic("uvm_page_own");
		}
		pg->owner = curproc->p_pid;
		pg->lowner = curlwp->l_lid;
		pg->owner_tag = tag;
		return;
	}

	/* drop ownership */
	KASSERT((pg->flags & PG_BUSY) == 0);
	if (pg->owner_tag == NULL) {
		printf("uvm_page_own: dropping ownership of an non-owned "
		    "page (%p)\n", pg);
		panic("uvm_page_own");
	}
	pg->owner_tag = NULL;
}
#endif

/*
 * uvm_pagelookup: look up a page
 *
 * => caller should lock object to keep someone from pulling the page
 *	out from under it
 */

struct vm_page *
uvm_pagelookup(struct uvm_object *obj, voff_t off)
{
	struct vm_page *pg;
	bool ddb __diagused = false;
#ifdef DDB
	extern int db_active;
	ddb = db_active != 0;
#endif

	KASSERT(ddb || rw_lock_held(obj->vmobjlock));

	pg = radix_tree_lookup_node(&obj->uo_pages, off >> PAGE_SHIFT);

	KASSERT(pg == NULL || obj->uo_npages != 0);
	KASSERT(pg == NULL || (pg->flags & (PG_RELEASED|PG_PAGEOUT)) == 0 ||
		(pg->flags & PG_BUSY) != 0);
	return pg;
}

/*
 * uvm_pagewire: wire the page, thus removing it from the daemon's grasp
 *
 * => caller must lock objects
 * => caller must hold pg->interlock
 */

void
uvm_pagewire(struct vm_page *pg)
{

	KASSERT(uvm_page_owner_locked_p(pg, true));
	KASSERT(mutex_owned(&pg->interlock));
#if defined(READAHEAD_STATS)
	if ((pg->flags & PG_READAHEAD) != 0) {
		uvm_ra_hit.ev_count++;
		pg->flags &= ~PG_READAHEAD;
	}
#endif /* defined(READAHEAD_STATS) */
	if (pg->wire_count == 0) {
		uvm_pagedequeue(pg);
		atomic_inc_uint(&uvmexp.wired);
	}
	pg->wire_count++;
	KASSERT(pg->wire_count > 0);	/* detect wraparound */
}

/*
 * uvm_pageunwire: unwire the page.
 *
 * => activate if wire count goes to zero.
 * => caller must lock objects
 * => caller must hold pg->interlock
 */

void
uvm_pageunwire(struct vm_page *pg)
{

	KASSERT(uvm_page_owner_locked_p(pg, true));
	KASSERT(pg->wire_count != 0);
	KASSERT(!uvmpdpol_pageisqueued_p(pg));
	KASSERT(mutex_owned(&pg->interlock));
	pg->wire_count--;
	if (pg->wire_count == 0) {
		uvm_pageactivate(pg);
		KASSERT(uvmexp.wired != 0);
		atomic_dec_uint(&uvmexp.wired);
	}
}

/*
 * uvm_pagedeactivate: deactivate page
 *
 * => caller must lock objects
 * => caller must check to make sure page is not wired
 * => object that page belongs to must be locked (so we can adjust pg->flags)
 * => caller must clear the reference on the page before calling
 * => caller must hold pg->interlock
 */

void
uvm_pagedeactivate(struct vm_page *pg)
{

	KASSERT(uvm_page_owner_locked_p(pg, false));
	KASSERT(mutex_owned(&pg->interlock));
	if (pg->wire_count == 0) {
		KASSERT(uvmpdpol_pageisqueued_p(pg));
		uvmpdpol_pagedeactivate(pg);
	}
}

/*
 * uvm_pageactivate: activate page
 *
 * => caller must lock objects
 * => caller must hold pg->interlock
 */

void
uvm_pageactivate(struct vm_page *pg)
{

	KASSERT(uvm_page_owner_locked_p(pg, false));
	KASSERT(mutex_owned(&pg->interlock));
#if defined(READAHEAD_STATS)
	if ((pg->flags & PG_READAHEAD) != 0) {
		uvm_ra_hit.ev_count++;
		pg->flags &= ~PG_READAHEAD;
	}
#endif /* defined(READAHEAD_STATS) */
	if (pg->wire_count == 0) {
		uvmpdpol_pageactivate(pg);
	}
}

/*
 * uvm_pagedequeue: remove a page from any paging queue
 *
 * => caller must lock objects
 * => caller must hold pg->interlock
 */
void
uvm_pagedequeue(struct vm_page *pg)
{

	KASSERT(uvm_page_owner_locked_p(pg, true));
	KASSERT(mutex_owned(&pg->interlock));
	if (uvmpdpol_pageisqueued_p(pg)) {
		uvmpdpol_pagedequeue(pg);
	}
}

/*
 * uvm_pageenqueue: add a page to a paging queue without activating.
 * used where a page is not really demanded (yet).  eg. read-ahead
 *
 * => caller must lock objects
 * => caller must hold pg->interlock
 */
void
uvm_pageenqueue(struct vm_page *pg)
{

	KASSERT(uvm_page_owner_locked_p(pg, false));
	KASSERT(mutex_owned(&pg->interlock));
	if (pg->wire_count == 0 && !uvmpdpol_pageisqueued_p(pg)) {
		uvmpdpol_pageenqueue(pg);
	}
}

/*
 * uvm_pagelock: acquire page interlock
 */
void
uvm_pagelock(struct vm_page *pg)
{

	mutex_enter(&pg->interlock);
}

/*
 * uvm_pagelock2: acquire two page interlocks
 */
void
uvm_pagelock2(struct vm_page *pg1, struct vm_page *pg2)
{

	if (pg1 < pg2) {
		mutex_enter(&pg1->interlock);
		mutex_enter(&pg2->interlock);
	} else {
		mutex_enter(&pg2->interlock);
		mutex_enter(&pg1->interlock);
	}
}

/*
 * uvm_pageunlock: release page interlock, and if a page replacement intent
 * is set on the page, pass it to uvmpdpol to make real.
 *
 * => caller must hold pg->interlock
 */
void
uvm_pageunlock(struct vm_page *pg)
{

	if ((pg->pqflags & PQ_INTENT_SET) == 0 ||
	    (pg->pqflags & PQ_INTENT_QUEUED) != 0) {
	    	mutex_exit(&pg->interlock);
	    	return;
	}
	pg->pqflags |= PQ_INTENT_QUEUED;
	mutex_exit(&pg->interlock);
	uvmpdpol_pagerealize(pg);
}

/*
 * uvm_pageunlock2: release two page interlocks, and for both pages if a
 * page replacement intent is set on the page, pass it to uvmpdpol to make
 * real.
 *
 * => caller must hold pg->interlock
 */
void
uvm_pageunlock2(struct vm_page *pg1, struct vm_page *pg2)
{

	if ((pg1->pqflags & PQ_INTENT_SET) == 0 ||
	    (pg1->pqflags & PQ_INTENT_QUEUED) != 0) {
	    	mutex_exit(&pg1->interlock);
	    	pg1 = NULL;
	} else {
		pg1->pqflags |= PQ_INTENT_QUEUED;
		mutex_exit(&pg1->interlock);
	}

	if ((pg2->pqflags & PQ_INTENT_SET) == 0 ||
	    (pg2->pqflags & PQ_INTENT_QUEUED) != 0) {
	    	mutex_exit(&pg2->interlock);
	    	pg2 = NULL;
	} else {
		pg2->pqflags |= PQ_INTENT_QUEUED;
		mutex_exit(&pg2->interlock);
	}

	if (pg1 != NULL) {
		uvmpdpol_pagerealize(pg1);
	}
	if (pg2 != NULL) {
		uvmpdpol_pagerealize(pg2);
	}
}

/*
 * uvm_pagezero: zero fill a page
 *
 * => if page is part of an object then the object should be locked
 *	to protect pg->flags.
 */

void
uvm_pagezero(struct vm_page *pg)
{

	uvm_pagemarkdirty(pg, UVM_PAGE_STATUS_DIRTY);
	pmap_zero_page(VM_PAGE_TO_PHYS(pg));
}

/*
 * uvm_pagecopy: copy a page
 *
 * => if page is part of an object then the object should be locked
 *	to protect pg->flags.
 */

void
uvm_pagecopy(struct vm_page *src, struct vm_page *dst)
{

	uvm_pagemarkdirty(dst, UVM_PAGE_STATUS_DIRTY);
	pmap_copy_page(VM_PAGE_TO_PHYS(src), VM_PAGE_TO_PHYS(dst));
}

/*
 * uvm_pageismanaged: test it see that a page (specified by PA) is managed.
 */

bool
uvm_pageismanaged(paddr_t pa)
{

	return (uvm_physseg_find(atop(pa), NULL) != UVM_PHYSSEG_TYPE_INVALID);
}

/*
 * uvm_page_lookup_freelist: look up the free list for the specified page
 */

int
uvm_page_lookup_freelist(struct vm_page *pg)
{
	uvm_physseg_t upm;

	upm = uvm_physseg_find(atop(VM_PAGE_TO_PHYS(pg)), NULL);
	KASSERT(upm != UVM_PHYSSEG_TYPE_INVALID);
	return uvm_physseg_get_free_list(upm);
}

/*
 * uvm_page_owner_locked_p: return true if object associated with page is
 * locked.  this is a weak check for runtime assertions only.
 */

bool
uvm_page_owner_locked_p(struct vm_page *pg, bool exclusive)
{

	if (pg->uobject != NULL) {
		return exclusive
		    ? rw_write_held(pg->uobject->vmobjlock)
		    : rw_lock_held(pg->uobject->vmobjlock);
	}
	if (pg->uanon != NULL) {
		return exclusive
		    ? rw_write_held(pg->uanon->an_lock)
		    : rw_lock_held(pg->uanon->an_lock);
	}
	return true;
}

/*
 * uvm_pagereadonly_p: return if the page should be mapped read-only
 */

bool
uvm_pagereadonly_p(struct vm_page *pg)
{
	struct uvm_object * const uobj = pg->uobject;

	KASSERT(uobj == NULL || rw_lock_held(uobj->vmobjlock));
	KASSERT(uobj != NULL || rw_lock_held(pg->uanon->an_lock));
	if ((pg->flags & PG_RDONLY) != 0) {
		return true;
	}
	if (uvm_pagegetdirty(pg) == UVM_PAGE_STATUS_CLEAN) {
		return true;
	}
	if (uobj == NULL) {
		return false;
	}
	return UVM_OBJ_NEEDS_WRITEFAULT(uobj);
}

#ifdef PMAP_DIRECT
/*
 * Call pmap to translate physical address into a virtual and to run a callback
 * for it. Used to avoid actually mapping the pages, pmap most likely uses direct map
 * or equivalent.
 */
int
uvm_direct_process(struct vm_page **pgs, u_int npages, voff_t off, vsize_t len,
            int (*process)(void *, size_t, void *), void *arg)
{
	int error = 0;
	paddr_t pa;
	size_t todo;
	voff_t pgoff = (off & PAGE_MASK);
	struct vm_page *pg;

	KASSERT(npages > 0 && len > 0);

	for (int i = 0; i < npages; i++) {
		pg = pgs[i];

		KASSERT(len > 0);

		/*
		 * Caller is responsible for ensuring all the pages are
		 * available.
		 */
		KASSERT(pg != NULL && pg != PGO_DONTCARE);

		pa = VM_PAGE_TO_PHYS(pg);
		todo = MIN(len, PAGE_SIZE - pgoff);

		error = pmap_direct_process(pa, pgoff, todo, process, arg);
		if (error)
			break;

		pgoff = 0;
		len -= todo;
	}

	KASSERTMSG(error != 0 || len == 0, "len %lu != 0 for non-error", len);
	return error;
}
#endif /* PMAP_DIRECT */

#if defined(DDB) || defined(DEBUGPRINT)

/*
 * uvm_page_printit: actually print the page
 */

static const char page_flagbits[] = UVM_PGFLAGBITS;
static const char page_pqflagbits[] = UVM_PQFLAGBITS;

void
uvm_page_printit(struct vm_page *pg, bool full,
    void (*pr)(const char *, ...))
{
	struct vm_page *tpg;
	struct uvm_object *uobj;
	struct pgflbucket *pgb;
	struct pgflist *pgl;
	char pgbuf[128];

	(*pr)("PAGE %p:\n", pg);
	snprintb(pgbuf, sizeof(pgbuf), page_flagbits, pg->flags);
	(*pr)("  flags=%s\n", pgbuf);
	snprintb(pgbuf, sizeof(pgbuf), page_pqflagbits, pg->pqflags);
	(*pr)("  pqflags=%s\n", pgbuf);
	(*pr)("  uobject=%p, uanon=%p, offset=0x%llx\n",
	    pg->uobject, pg->uanon, (long long)pg->offset);
	(*pr)("  loan_count=%d wire_count=%d bucket=%d freelist=%d\n",
	    pg->loan_count, pg->wire_count, uvm_page_get_bucket(pg),
	    uvm_page_get_freelist(pg));
	(*pr)("  pa=0x%lx\n", (long)VM_PAGE_TO_PHYS(pg));
#if defined(UVM_PAGE_TRKOWN)
	if (pg->flags & PG_BUSY)
		(*pr)("  owning process = %d.%d, tag=%s\n",
		    pg->owner, pg->lowner, pg->owner_tag);
	else
		(*pr)("  page not busy, no owner\n");
#else
	(*pr)("  [page ownership tracking disabled]\n");
#endif

	if (!full)
		return;

	/* cross-verify object/anon */
	if ((pg->flags & PG_FREE) == 0) {
		if (pg->flags & PG_ANON) {
			if (pg->uanon == NULL || pg->uanon->an_page != pg)
			    (*pr)("  >>> ANON DOES NOT POINT HERE <<< (%p)\n",
				(pg->uanon) ? pg->uanon->an_page : NULL);
			else
				(*pr)("  anon backpointer is OK\n");
		} else {
			uobj = pg->uobject;
			if (uobj) {
				(*pr)("  checking object list\n");
				tpg = uvm_pagelookup(uobj, pg->offset);
				if (tpg)
					(*pr)("  page found on object list\n");
				else
			(*pr)("  >>> PAGE NOT FOUND ON OBJECT LIST! <<<\n");
			}
		}
	}

	/* cross-verify page queue */
	if (pg->flags & PG_FREE) {
		int fl = uvm_page_get_freelist(pg);
		int b = uvm_page_get_bucket(pg);
		pgb = uvm.page_free[fl].pgfl_buckets[b];
		pgl = &pgb->pgb_colors[VM_PGCOLOR(pg)];
		(*pr)("  checking pageq list\n");
		LIST_FOREACH(tpg, pgl, pageq.list) {
			if (tpg == pg) {
				break;
			}
		}
		if (tpg)
			(*pr)("  page found on pageq list\n");
		else
			(*pr)("  >>> PAGE NOT FOUND ON PAGEQ LIST! <<<\n");
	}
}

/*
 * uvm_page_printall - print a summary of all managed pages
 */

void
uvm_page_printall(void (*pr)(const char *, ...))
{
	uvm_physseg_t i;
	paddr_t pfn;
	struct vm_page *pg;

	(*pr)("%18s %4s %4s %18s %18s"
#ifdef UVM_PAGE_TRKOWN
	    " OWNER"
#endif
	    "\n", "PAGE", "FLAG", "PQ", "UOBJECT", "UANON");
	for (i = uvm_physseg_get_first();
	     uvm_physseg_valid_p(i);
	     i = uvm_physseg_get_next(i)) {
		for (pfn = uvm_physseg_get_start(i);
		     pfn < uvm_physseg_get_end(i);
		     pfn++) {
			pg = PHYS_TO_VM_PAGE(ptoa(pfn));

			(*pr)("%18p %04x %08x %18p %18p",
			    pg, pg->flags, pg->pqflags, pg->uobject,
			    pg->uanon);
#ifdef UVM_PAGE_TRKOWN
			if (pg->flags & PG_BUSY)
				(*pr)(" %d [%s]", pg->owner, pg->owner_tag);
#endif
			(*pr)("\n");
		}
	}
}

/*
 * uvm_page_print_freelists - print a summary freelists
 */

void
uvm_page_print_freelists(void (*pr)(const char *, ...))
{
	struct pgfreelist *pgfl;
	struct pgflbucket *pgb;
	int fl, b, c;

	(*pr)("There are %d freelists with %d buckets of %d colors.\n\n",
	    VM_NFREELIST, uvm.bucketcount, uvmexp.ncolors);

	for (fl = 0; fl < VM_NFREELIST; fl++) {
		pgfl = &uvm.page_free[fl];
		(*pr)("freelist(%d) @ %p\n", fl, pgfl);
		for (b = 0; b < uvm.bucketcount; b++) {
			pgb = uvm.page_free[fl].pgfl_buckets[b];
			(*pr)("    bucket(%d) @ %p, nfree = %d, lock @ %p:\n",
			    b, pgb, pgb->pgb_nfree,
			    &uvm_freelist_locks[b].lock);
			for (c = 0; c < uvmexp.ncolors; c++) {
				(*pr)("        color(%d) @ %p, ", c,
				    &pgb->pgb_colors[c]);
				(*pr)("first page = %p\n",
				    LIST_FIRST(&pgb->pgb_colors[c]));
			}
		}
	}
}

#endif /* DDB || DEBUGPRINT */
