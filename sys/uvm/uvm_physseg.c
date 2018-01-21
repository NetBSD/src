/* $NetBSD: uvm_physseg.c,v 1.9 2018/01/21 17:58:43 christos Exp $ */

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
 *	@(#)vm_page.h   7.3 (Berkeley) 4/21/91
 * from: Id: uvm_page.h,v 1.1.2.6 1998/02/04 02:31:42 chuck Exp
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
 * Consolidated API from uvm_page.c and others.
 * Consolidated and designed by Cherry G. Mathew <cherry@zyx.in>
 * rbtree(3) backing implementation by:
 * Santhosh N. Raju <santhosh.raju@gmail.com>
 */

#ifdef _KERNEL_OPT
#include "opt_uvm.h"
#endif

#include <sys/param.h>
#include <sys/types.h>
#include <sys/extent.h>
#include <sys/kmem.h>

#include <uvm/uvm.h>
#include <uvm/uvm_page.h>
#include <uvm/uvm_param.h>
#include <uvm/uvm_pdpolicy.h>
#include <uvm/uvm_physseg.h>

/*
 * uvm_physseg: describes one segment of physical memory
 */
struct uvm_physseg {
	struct  rb_node rb_node;	/* tree information */
	paddr_t	start;			/* PF# of first page in segment */
	paddr_t	end;			/* (PF# of last page in segment) + 1 */
	paddr_t	avail_start;		/* PF# of first free page in segment */
	paddr_t	avail_end;		/* (PF# of last free page in segment) +1  */
	struct	vm_page *pgs;		/* vm_page structures (from start) */
	struct  extent *ext;		/* extent(9) structure to manage pgs[] */
	int	free_list;		/* which free list they belong on */
	u_int	start_hint;		/* start looking for free pages here */
					/* protected by uvm_fpageqlock */
#ifdef __HAVE_PMAP_PHYSSEG
	struct	pmap_physseg pmseg;	/* pmap specific (MD) data */
#endif
};

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

#if defined(UVM_HOTPLUG) /* rbtree impementation */

#define		HANDLE_TO_PHYSSEG_NODE(h)	((struct uvm_physseg *)(h))
#define		PHYSSEG_NODE_TO_HANDLE(u)	((uvm_physseg_t)(u))

struct uvm_physseg_graph {
	struct rb_tree rb_tree;		/* Tree for entries */
	int            nentries;	/* Number of entries */
};

static struct uvm_physseg_graph uvm_physseg_graph;

/*
 * Note on kmem(9) allocator usage:
 * We take the conservative approach that plug/unplug are allowed to
 * fail in high memory stress situations.
 *
 * We want to avoid re-entrant situations in which one plug/unplug
 * operation is waiting on a previous one to complete, since this
 * makes the design more complicated than necessary.
 *
 * We may review this and change its behaviour, once the use cases
 * become more obvious.
 */

/*
 * Special alloc()/free() functions for boot time support:
 * We assume that alloc() at boot time is only for new 'vm_physseg's
 * This allows us to use a static array for memory allocation at boot
 * time. Thus we avoid using kmem(9) which is not ready at this point
 * in boot.
 *
 * After kmem(9) is ready, we use it. We currently discard any free()s
 * to this static array, since the size is small enough to be a
 * trivial waste on all architectures we run on.
 */

static size_t nseg = 0;
static struct uvm_physseg uvm_physseg[VM_PHYSSEG_MAX];

static void *
uvm_physseg_alloc(size_t sz)
{
	/*
	 * During boot time, we only support allocating vm_physseg
	 * entries from the static array.
	 * We need to assert for this.
	 */

	if (__predict_false(uvm.page_init_done == false)) {
		if (sz % sizeof(struct uvm_physseg))
			panic("%s: tried to alloc size other than multiple"
			    " of struct uvm_physseg at boot\n", __func__);

		size_t n = sz / sizeof(struct uvm_physseg);
		nseg += n;

		KASSERT(nseg > 0 && nseg <= VM_PHYSSEG_MAX);

		return &uvm_physseg[nseg - n];
	}

	return kmem_zalloc(sz, KM_NOSLEEP);
}

static void
uvm_physseg_free(void *p, size_t sz)
{
	/*
	 * This is a bit tricky. We do allow simulation of free()
	 * during boot (for eg: when MD code is "steal"ing memory,
	 * and the segment has been exhausted (and thus needs to be
	 * free() - ed.
	 * free() also complicates things because we leak the
	 * free(). Therefore calling code can't assume that free()-ed
	 * memory is available for alloc() again, at boot time.
	 *
	 * Thus we can't explicitly disallow free()s during
	 * boot time. However, the same restriction for alloc()
	 * applies to free(). We only allow uvm_physseg related free()s
	 * via this function during boot time.
	 */

	if (__predict_false(uvm.page_init_done == false)) {
		if (sz % sizeof(struct uvm_physseg))
			panic("%s: tried to free size other than struct uvm_physseg"
			    " at boot\n", __func__);

	}

	/*
	 * Could have been in a single if(){} block - split for
	 * clarity
	 */

	if ((struct uvm_physseg *)p >= uvm_physseg &&
	    (struct uvm_physseg *)p < (uvm_physseg + VM_PHYSSEG_MAX)) {
		if (sz % sizeof(struct uvm_physseg))
			panic("%s: tried to free() other than struct uvm_physseg"
			    " from static array\n", __func__);

		if ((sz / sizeof(struct uvm_physseg)) >= VM_PHYSSEG_MAX)
			panic("%s: tried to free() the entire static array!", __func__);
		return; /* Nothing to free */
	}

	kmem_free(p, sz);
}

/* XXX: Multi page size */
bool
uvm_physseg_plug(paddr_t pfn, size_t pages, uvm_physseg_t *psp)
{
	int preload;
	size_t slabpages;
	struct uvm_physseg *ps, *current_ps = NULL;
	struct vm_page *slab = NULL, *pgs = NULL;

#ifdef DEBUG
	paddr_t off;
	uvm_physseg_t upm;
	upm = uvm_physseg_find(pfn, &off);

	ps = HANDLE_TO_PHYSSEG_NODE(upm);

	if (ps != NULL) /* XXX; do we allow "update" plugs ? */
		return false;
#endif

	/*
	 * do we have room?
	 */

	ps = uvm_physseg_alloc(sizeof (struct uvm_physseg));
	if (ps == NULL) {
		printf("uvm_page_physload: unable to load physical memory "
		    "segment\n");
		printf("\t%d segments allocated, ignoring 0x%"PRIxPADDR" -> 0x%"PRIxPADDR"\n",
		    VM_PHYSSEG_MAX, pfn, pfn + pages + 1);
		printf("\tincrease VM_PHYSSEG_MAX\n");
		return false;
	}

	/* span init */
	ps->start = pfn;
	ps->end = pfn + pages;

	/*
	 * XXX: Ugly hack because uvmexp.npages accounts for only
	 * those pages in the segment included below as well - this
	 * should be legacy and removed.
	 */

	ps->avail_start = ps->start;
	ps->avail_end = ps->end;

	/*
	 * check to see if this is a "preload" (i.e. uvm_page_init hasn't been
	 * called yet, so kmem is not available).
	 */

	preload = 1; /* We are going to assume it is a preload */

	RB_TREE_FOREACH(current_ps, &(uvm_physseg_graph.rb_tree)) {
		/* If there are non NULL pages then we are not in a preload */
		if (current_ps->pgs != NULL) {
			preload = 0;
			/* Try to scavenge from earlier unplug()s. */
			pgs = uvm_physseg_seg_alloc_from_slab(current_ps, pages);

			if (pgs != NULL) {
				break;
			}
		}
	}


	/*
	 * if VM is already running, attempt to kmem_alloc vm_page structures
	 */

	if (!preload) {
		if (pgs == NULL) { /* Brand new */
			/* Iteratively try alloc down from uvmexp.npages */
			for (slabpages = (size_t) uvmexp.npages; slabpages >= pages; slabpages--) {
				slab = kmem_zalloc(sizeof *pgs * (long unsigned int)slabpages, KM_NOSLEEP);
				if (slab != NULL)
					break;
			}

			if (slab == NULL) {
				uvm_physseg_free(ps, sizeof(struct uvm_physseg));
				return false;
			}

			uvm_physseg_seg_chomp_slab(ps, slab, (size_t) slabpages);
			/* We allocate enough for this plug */
			pgs = uvm_physseg_seg_alloc_from_slab(ps, pages);

			if (pgs == NULL) {
				printf("unable to uvm_physseg_seg_alloc_from_slab() from backend\n");
				return false;
			}
		} else {
			/* Reuse scavenged extent */
			ps->ext = current_ps->ext;
		}

		physmem += pages;
		uvmpdpol_reinit();
	} else { /* Boot time - see uvm_page.c:uvm_page_init() */
		pgs = NULL;
		ps->pgs = pgs;
	}

	/*
	 * now insert us in the proper place in uvm_physseg_graph.rb_tree
	 */

	current_ps = rb_tree_insert_node(&(uvm_physseg_graph.rb_tree), ps);
	if (current_ps != ps) {
		panic("uvm_page_physload: Duplicate address range detected!");
	}
	uvm_physseg_graph.nentries++;

	/*
	 * uvm_pagefree() requires the PHYS_TO_VM_PAGE(pgs[i]) on the
	 * newly allocated pgs[] to return the correct value. This is
	 * a bit of a chicken and egg problem, since it needs
	 * uvm_physseg_find() to succeed. For this, the node needs to
	 * be inserted *before* uvm_physseg_init_seg() happens.
	 *
	 * During boot, this happens anyway, since
	 * uvm_physseg_init_seg() is called later on and separately
	 * from uvm_page.c:uvm_page_init().
	 * In the case of hotplug we need to ensure this.
	 */

	if (__predict_true(!preload))
		uvm_physseg_init_seg(ps, pgs);

	if (psp != NULL)
		*psp = ps;

	return true;
}

static int
uvm_physseg_compare_nodes(void *ctx, const void *nnode1, const void *nnode2)
{
	const struct uvm_physseg *enode1 = nnode1;
	const struct uvm_physseg *enode2 = nnode2;

	KASSERT(enode1->start < enode2->start || enode1->start >= enode2->end);
	KASSERT(enode2->start < enode1->start || enode2->start >= enode1->end);

	if (enode1->start < enode2->start)
		return -1;
	if (enode1->start >= enode2->end)
		return 1;
	return 0;
}

static int
uvm_physseg_compare_key(void *ctx, const void *nnode, const void *pkey)
{
	const struct uvm_physseg *enode = nnode;
	const paddr_t pa = *(const paddr_t *) pkey;

	if(enode->start <= pa && pa < enode->end)
		return 0;
	if (enode->start < pa)
		return -1;
	if (enode->end > pa)
		return 1;

	return 0;
}

static const rb_tree_ops_t uvm_physseg_tree_ops = {
	.rbto_compare_nodes = uvm_physseg_compare_nodes,
	.rbto_compare_key = uvm_physseg_compare_key,
	.rbto_node_offset = offsetof(struct uvm_physseg, rb_node),
	.rbto_context = NULL
};

/*
 * uvm_physseg_init: init the physmem
 *
 * => physmem unit should not be in use at this point
 */

void
uvm_physseg_init(void)
{
	rb_tree_init(&(uvm_physseg_graph.rb_tree), &uvm_physseg_tree_ops);
	uvm_physseg_graph.nentries = 0;
}

uvm_physseg_t
uvm_physseg_get_next(uvm_physseg_t upm)
{
	/* next of invalid is invalid, not fatal */
	if (uvm_physseg_valid_p(upm) == false)
		return UVM_PHYSSEG_TYPE_INVALID;

	return (uvm_physseg_t) rb_tree_iterate(&(uvm_physseg_graph.rb_tree), upm,
	    RB_DIR_RIGHT);
}

uvm_physseg_t
uvm_physseg_get_prev(uvm_physseg_t upm)
{
	/* prev of invalid is invalid, not fatal */
	if (uvm_physseg_valid_p(upm) == false)
		return UVM_PHYSSEG_TYPE_INVALID;

	return (uvm_physseg_t) rb_tree_iterate(&(uvm_physseg_graph.rb_tree), upm,
	    RB_DIR_LEFT);
}

uvm_physseg_t
uvm_physseg_get_last(void)
{
	return (uvm_physseg_t) RB_TREE_MAX(&(uvm_physseg_graph.rb_tree));
}

uvm_physseg_t
uvm_physseg_get_first(void)
{
	return (uvm_physseg_t) RB_TREE_MIN(&(uvm_physseg_graph.rb_tree));
}

paddr_t
uvm_physseg_get_highest_frame(void)
{
	struct uvm_physseg *ps =
	    (uvm_physseg_t) RB_TREE_MAX(&(uvm_physseg_graph.rb_tree));

	return ps->end - 1;
}

/*
 * uvm_page_physunload: unload physical memory and return it to
 * caller.
 */
bool
uvm_page_physunload(uvm_physseg_t upm, int freelist, paddr_t *paddrp)
{
	struct uvm_physseg *seg;

	if (__predict_true(uvm.page_init_done == true))
		panic("%s: unload attempted after uvm_page_init()\n", __func__);

	seg = HANDLE_TO_PHYSSEG_NODE(upm);

	if (seg->free_list != freelist) {
		paddrp = NULL;
		return false;
	}

	/*
	 * During cold boot, what we're about to unplug hasn't been
	 * put on the uvm freelist, nor has uvmexp.npages been
	 * updated. (This happens in uvm_page.c:uvm_page_init())
	 *
	 * For hotplug, we assume here that the pages being unloaded
	 * here are completely out of sight of uvm (ie; not on any uvm
	 * lists), and that  uvmexp.npages has been suitably
	 * decremented before we're called.
	 *
	 * XXX: will avail_end == start if avail_start < avail_end?
	 */

	/* try from front */
	if (seg->avail_start == seg->start &&
	    seg->avail_start < seg->avail_end) {
		*paddrp = ctob(seg->avail_start);
		return uvm_physseg_unplug(seg->avail_start, 1);
	}

	/* try from rear */
	if (seg->avail_end == seg->end &&
	    seg->avail_start < seg->avail_end) {
		*paddrp = ctob(seg->avail_end - 1);
		return uvm_physseg_unplug(seg->avail_end - 1, 1);
	}

	return false;
}

bool
uvm_page_physunload_force(uvm_physseg_t upm, int freelist, paddr_t *paddrp)
{
	struct uvm_physseg *seg;

	seg = HANDLE_TO_PHYSSEG_NODE(upm);

	if (__predict_true(uvm.page_init_done == true))
		panic("%s: unload attempted after uvm_page_init()\n", __func__);
	/* any room in this bank? */
	if (seg->avail_start >= seg->avail_end) {
		paddrp = NULL;
		return false; /* nope */
	}

	*paddrp = ctob(seg->avail_start);

	/* Always unplug from front */
	return uvm_physseg_unplug(seg->avail_start, 1);
}


/*
 * vm_physseg_find: find vm_physseg structure that belongs to a PA
 */
uvm_physseg_t
uvm_physseg_find(paddr_t pframe, psize_t *offp)
{
	struct uvm_physseg * ps = NULL;

	ps = rb_tree_find_node(&(uvm_physseg_graph.rb_tree), &pframe);

	if(ps != NULL && offp != NULL)
		*offp = pframe - ps->start;

	return ps;
}

#else  /* UVM_HOTPLUG */

/*
 * physical memory config is stored in vm_physmem.
 */

#define	VM_PHYSMEM_PTR(i)	(&vm_physmem[i])
#if VM_PHYSSEG_MAX == 1
#define VM_PHYSMEM_PTR_SWAP(i, j) /* impossible */
#else
#define VM_PHYSMEM_PTR_SWAP(i, j)					      \
	do { vm_physmem[(i)] = vm_physmem[(j)]; } while (0)
#endif

#define		HANDLE_TO_PHYSSEG_NODE(h)	(VM_PHYSMEM_PTR((int)h))
#define		PHYSSEG_NODE_TO_HANDLE(u)	((int)((vsize_t) (u - vm_physmem) / sizeof(struct uvm_physseg)))

static struct uvm_physseg vm_physmem[VM_PHYSSEG_MAX];	/* XXXCDC: uvm.physmem */
static int vm_nphysseg = 0;				/* XXXCDC: uvm.nphysseg */
#define	vm_nphysmem	vm_nphysseg

void
uvm_physseg_init(void)
{
	/* XXX: Provisioning for rb_tree related init(s) */
	return;
}

int
uvm_physseg_get_next(uvm_physseg_t lcv)
{
	/* next of invalid is invalid, not fatal */
	if (uvm_physseg_valid_p(lcv) == false)
		return UVM_PHYSSEG_TYPE_INVALID;

	return (lcv + 1);
}

int
uvm_physseg_get_prev(uvm_physseg_t lcv)
{
	/* prev of invalid is invalid, not fatal */
	if (uvm_physseg_valid_p(lcv) == false)
		return UVM_PHYSSEG_TYPE_INVALID;

	return (lcv - 1);
}

int
uvm_physseg_get_last(void)
{
	return (vm_nphysseg - 1);
}

int
uvm_physseg_get_first(void)
{
	return 0;
}

paddr_t
uvm_physseg_get_highest_frame(void)
{
	int lcv;
	paddr_t last = 0;
	struct uvm_physseg *ps;

	for (lcv = 0; lcv < vm_nphysseg; lcv++) {
		ps = VM_PHYSMEM_PTR(lcv);
		if (last < ps->end)
			last = ps->end;
	}

	return last;
}


static struct vm_page *
uvm_post_preload_check(void)
{
	int preload, lcv;

	/*
	 * check to see if this is a "preload" (i.e. uvm_page_init hasn't been
	 * called yet, so kmem is not available).
	 */

	for (lcv = 0 ; lcv < vm_nphysmem ; lcv++) {
		if (VM_PHYSMEM_PTR(lcv)->pgs)
			break;
	}
	preload = (lcv == vm_nphysmem);

	/*
	 * if VM is already running, attempt to kmem_alloc vm_page structures
	 */

	if (!preload) {
		panic("Tried to add RAM after uvm_page_init");
	}

	return NULL;
}

/*
 * uvm_page_physunload: unload physical memory and return it to
 * caller.
 */
bool
uvm_page_physunload(uvm_physseg_t psi, int freelist, paddr_t *paddrp)
{
	int x;
	struct uvm_physseg *seg;

	uvm_post_preload_check();

	seg = VM_PHYSMEM_PTR(psi);

	if (seg->free_list != freelist) {
		paddrp = NULL;
		return false;
	}

	/* try from front */
	if (seg->avail_start == seg->start &&
	    seg->avail_start < seg->avail_end) {
		*paddrp = ctob(seg->avail_start);
		seg->avail_start++;
		seg->start++;
		/* nothing left?   nuke it */
		if (seg->avail_start == seg->end) {
			if (vm_nphysmem == 1)
				panic("uvm_page_physget: out of memory!");
			vm_nphysmem--;
			for (x = psi ; x < vm_nphysmem ; x++)
				/* structure copy */
				VM_PHYSMEM_PTR_SWAP(x, x + 1);
		}
		return (true);
	}

	/* try from rear */
	if (seg->avail_end == seg->end &&
	    seg->avail_start < seg->avail_end) {
		*paddrp = ctob(seg->avail_end - 1);
		seg->avail_end--;
		seg->end--;
		/* nothing left?   nuke it */
		if (seg->avail_end == seg->start) {
			if (vm_nphysmem == 1)
				panic("uvm_page_physget: out of memory!");
			vm_nphysmem--;
			for (x = psi ; x < vm_nphysmem ; x++)
				/* structure copy */
				VM_PHYSMEM_PTR_SWAP(x, x + 1);
		}
		return (true);
	}

	return false;
}

bool
uvm_page_physunload_force(uvm_physseg_t psi, int freelist, paddr_t *paddrp)
{
	int x;
	struct uvm_physseg *seg;

	uvm_post_preload_check();

	seg = VM_PHYSMEM_PTR(psi);

	/* any room in this bank? */
	if (seg->avail_start >= seg->avail_end) {
		paddrp = NULL;
		return false; /* nope */
	}

	*paddrp = ctob(seg->avail_start);
	seg->avail_start++;
	/* truncate! */
	seg->start = seg->avail_start;

	/* nothing left?   nuke it */
	if (seg->avail_start == seg->end) {
		if (vm_nphysmem == 1)
			panic("uvm_page_physget: out of memory!");
		vm_nphysmem--;
		for (x = psi ; x < vm_nphysmem ; x++)
			/* structure copy */
			VM_PHYSMEM_PTR_SWAP(x, x + 1);
	}
	return (true);
}

bool
uvm_physseg_plug(paddr_t pfn, size_t pages, uvm_physseg_t *psp)
{
	int lcv;
	struct vm_page *pgs;
	struct uvm_physseg *ps;

#ifdef DEBUG
	paddr_t off;
	uvm_physseg_t upm;
	upm = uvm_physseg_find(pfn, &off);

	if (uvm_physseg_valid_p(upm)) /* XXX; do we allow "update" plugs ? */
		return false;
#endif

	paddr_t start = pfn;
	paddr_t end = pfn + pages;
	paddr_t avail_start = start;
	paddr_t avail_end = end;

	if (uvmexp.pagesize == 0)
		panic("uvm_page_physload: page size not set!");

	/*
	 * do we have room?
	 */

	if (vm_nphysmem == VM_PHYSSEG_MAX) {
		printf("uvm_page_physload: unable to load physical memory "
		    "segment\n");
		printf("\t%d segments allocated, ignoring 0x%llx -> 0x%llx\n",
		    VM_PHYSSEG_MAX, (long long)start, (long long)end);
		printf("\tincrease VM_PHYSSEG_MAX\n");
		if (psp != NULL)
			*psp = UVM_PHYSSEG_TYPE_INVALID_OVERFLOW;
		return false;
	}

	/*
	 * check to see if this is a "preload" (i.e. uvm_page_init hasn't been
	 * called yet, so kmem is not available).
	 */
	pgs = uvm_post_preload_check();

	/*
	 * now insert us in the proper place in vm_physmem[]
	 */

#if (VM_PHYSSEG_STRAT == VM_PSTRAT_RANDOM)
	/* random: put it at the end (easy!) */
	ps = VM_PHYSMEM_PTR(vm_nphysmem);
	lcv = vm_nphysmem;
#elif (VM_PHYSSEG_STRAT == VM_PSTRAT_BSEARCH)
	{
		int x;
		/* sort by address for binary search */
		for (lcv = 0 ; lcv < vm_nphysmem ; lcv++)
			if (start < VM_PHYSMEM_PTR(lcv)->start)
				break;
		ps = VM_PHYSMEM_PTR(lcv);
		/* move back other entries, if necessary ... */
		for (x = vm_nphysmem ; x > lcv ; x--)
			/* structure copy */
			VM_PHYSMEM_PTR_SWAP(x, x - 1);
	}
#elif (VM_PHYSSEG_STRAT == VM_PSTRAT_BIGFIRST)
	{
		int x;
		/* sort by largest segment first */
		for (lcv = 0 ; lcv < vm_nphysmem ; lcv++)
			if ((end - start) >
			    (VM_PHYSMEM_PTR(lcv)->end - VM_PHYSMEM_PTR(lcv)->start))
				break;
		ps = VM_PHYSMEM_PTR(lcv);
		/* move back other entries, if necessary ... */
		for (x = vm_nphysmem ; x > lcv ; x--)
			/* structure copy */
			VM_PHYSMEM_PTR_SWAP(x, x - 1);
	}
#else
	panic("uvm_page_physload: unknown physseg strategy selected!");
#endif

	ps->start = start;
	ps->end = end;
	ps->avail_start = avail_start;
	ps->avail_end = avail_end;

	ps->pgs = pgs;

	vm_nphysmem++;

	if (psp != NULL)
		*psp = lcv;

	return true;
}

/*
 * when VM_PHYSSEG_MAX is 1, we can simplify these functions
 */

#if VM_PHYSSEG_MAX == 1
static inline int vm_physseg_find_contig(struct uvm_physseg *, int, paddr_t, psize_t *);
#elif (VM_PHYSSEG_STRAT == VM_PSTRAT_BSEARCH)
static inline int vm_physseg_find_bsearch(struct uvm_physseg *, int, paddr_t, psize_t *);
#else
static inline int vm_physseg_find_linear(struct uvm_physseg *, int, paddr_t, psize_t *);
#endif

/*
 * vm_physseg_find: find vm_physseg structure that belongs to a PA
 */
int
uvm_physseg_find(paddr_t pframe, psize_t *offp)
{

#if VM_PHYSSEG_MAX == 1
	return vm_physseg_find_contig(vm_physmem, vm_nphysseg, pframe, offp);
#elif (VM_PHYSSEG_STRAT == VM_PSTRAT_BSEARCH)
	return vm_physseg_find_bsearch(vm_physmem, vm_nphysseg, pframe, offp);
#else
	return vm_physseg_find_linear(vm_physmem, vm_nphysseg, pframe, offp);
#endif
}

#if VM_PHYSSEG_MAX == 1
static inline int
vm_physseg_find_contig(struct uvm_physseg *segs, int nsegs, paddr_t pframe, psize_t *offp)
{

	/* 'contig' case */
	if (pframe >= segs[0].start && pframe < segs[0].end) {
		if (offp)
			*offp = pframe - segs[0].start;
		return(0);
	}
	return(-1);
}

#elif (VM_PHYSSEG_STRAT == VM_PSTRAT_BSEARCH)

static inline int
vm_physseg_find_bsearch(struct uvm_physseg *segs, int nsegs, paddr_t pframe, psize_t *offp)
{
	/* binary search for it */
	int	start, len, guess;

	/*
	 * if try is too large (thus target is less than try) we reduce
	 * the length to trunc(len/2) [i.e. everything smaller than "try"]
	 *
	 * if the try is too small (thus target is greater than try) then
	 * we set the new start to be (try + 1).   this means we need to
	 * reduce the length to (round(len/2) - 1).
	 *
	 * note "adjust" below which takes advantage of the fact that
	 *  (round(len/2) - 1) == trunc((len - 1) / 2)
	 * for any value of len we may have
	 */

	for (start = 0, len = nsegs ; len != 0 ; len = len / 2) {
		guess = start + (len / 2);	/* try in the middle */

		/* start past our try? */
		if (pframe >= segs[guess].start) {
			/* was try correct? */
			if (pframe < segs[guess].end) {
				if (offp)
					*offp = pframe - segs[guess].start;
				return guess;            /* got it */
			}
			start = guess + 1;	/* next time, start here */
			len--;			/* "adjust" */
		} else {
			/*
			 * pframe before try, just reduce length of
			 * region, done in "for" loop
			 */
		}
	}
	return(-1);
}

#else

static inline int
vm_physseg_find_linear(struct uvm_physseg *segs, int nsegs, paddr_t pframe, psize_t *offp)
{
	/* linear search for it */
	int	lcv;

	for (lcv = 0; lcv < nsegs; lcv++) {
		if (pframe >= segs[lcv].start &&
		    pframe < segs[lcv].end) {
			if (offp)
				*offp = pframe - segs[lcv].start;
			return(lcv);		   /* got it */
		}
	}
	return(-1);
}
#endif
#endif /* UVM_HOTPLUG */

bool
uvm_physseg_valid_p(uvm_physseg_t upm)
{
	struct uvm_physseg *ps;

	if (upm == UVM_PHYSSEG_TYPE_INVALID ||
	    upm == UVM_PHYSSEG_TYPE_INVALID_EMPTY ||
	    upm == UVM_PHYSSEG_TYPE_INVALID_OVERFLOW)
		return false;

	/*
	 * This is the delicate init dance -
	 * needs to go with the dance.
	 */
	if (uvm.page_init_done != true)
		return true;

	ps = HANDLE_TO_PHYSSEG_NODE(upm);

	/* Extra checks needed only post uvm_page_init() */
	if (ps->pgs == NULL)
		return false;

	/* XXX: etc. */

	return true;

}

/*
 * Boot protocol dictates that these must be able to return partially
 * initialised segments.
 */
paddr_t
uvm_physseg_get_start(uvm_physseg_t upm)
{
	if (uvm_physseg_valid_p(upm) == false)
		return (paddr_t) -1;

	return HANDLE_TO_PHYSSEG_NODE(upm)->start;
}

paddr_t
uvm_physseg_get_end(uvm_physseg_t upm)
{
	if (uvm_physseg_valid_p(upm) == false)
		return (paddr_t) -1;

	return HANDLE_TO_PHYSSEG_NODE(upm)->end;
}

paddr_t
uvm_physseg_get_avail_start(uvm_physseg_t upm)
{
	if (uvm_physseg_valid_p(upm) == false)
		return (paddr_t) -1;

	return HANDLE_TO_PHYSSEG_NODE(upm)->avail_start;
}

#if defined(UVM_PHYSSEG_LEGACY)
void
uvm_physseg_set_avail_start(uvm_physseg_t upm, paddr_t avail_start)
{
	struct uvm_physseg *ps = HANDLE_TO_PHYSSEG_NODE(upm);

#if defined(DIAGNOSTIC)
	paddr_t avail_end;
	avail_end = uvm_physseg_get_avail_end(upm);
	KASSERT(uvm_physseg_valid_p(upm));
	KASSERT(avail_start < avail_end && avail_start >= ps->start);
#endif

	ps->avail_start = avail_start;
}
void uvm_physseg_set_avail_end(uvm_physseg_t upm, paddr_t avail_end)
{
	struct uvm_physseg *ps = HANDLE_TO_PHYSSEG_NODE(upm);

#if defined(DIAGNOSTIC)
	paddr_t avail_start;
	avail_start = uvm_physseg_get_avail_start(upm);
	KASSERT(uvm_physseg_valid_p(upm));
	KASSERT(avail_end > avail_start && avail_end <= ps->end);
#endif

	ps->avail_end = avail_end;
}

#endif /* UVM_PHYSSEG_LEGACY */

paddr_t
uvm_physseg_get_avail_end(uvm_physseg_t upm)
{
	if (uvm_physseg_valid_p(upm) == false)
		return (paddr_t) -1;

	return HANDLE_TO_PHYSSEG_NODE(upm)->avail_end;
}

struct vm_page *
uvm_physseg_get_pg(uvm_physseg_t upm, paddr_t idx)
{
	KASSERT(uvm_physseg_valid_p(upm));
	return &HANDLE_TO_PHYSSEG_NODE(upm)->pgs[idx];
}

#ifdef __HAVE_PMAP_PHYSSEG
struct pmap_physseg *
uvm_physseg_get_pmseg(uvm_physseg_t upm)
{
	KASSERT(uvm_physseg_valid_p(upm));
	return &(HANDLE_TO_PHYSSEG_NODE(upm)->pmseg);
}
#endif

int
uvm_physseg_get_free_list(uvm_physseg_t upm)
{
	KASSERT(uvm_physseg_valid_p(upm));
	return HANDLE_TO_PHYSSEG_NODE(upm)->free_list;
}

u_int
uvm_physseg_get_start_hint(uvm_physseg_t upm)
{
	KASSERT(uvm_physseg_valid_p(upm));
	return HANDLE_TO_PHYSSEG_NODE(upm)->start_hint;
}

bool
uvm_physseg_set_start_hint(uvm_physseg_t upm, u_int start_hint)
{
	if (uvm_physseg_valid_p(upm) == false)
		return false;

	HANDLE_TO_PHYSSEG_NODE(upm)->start_hint = start_hint;
	return true;
}

void
uvm_physseg_init_seg(uvm_physseg_t upm, struct vm_page *pgs)
{
	psize_t i;
	psize_t n;
	paddr_t paddr;
	struct uvm_physseg *seg;

	KASSERT(upm != UVM_PHYSSEG_TYPE_INVALID && pgs != NULL);

	seg = HANDLE_TO_PHYSSEG_NODE(upm);
	KASSERT(seg != NULL);
	KASSERT(seg->pgs == NULL);

	n = seg->end - seg->start;
	seg->pgs = pgs;

	/* init and free vm_pages (we've already zeroed them) */
	paddr = ctob(seg->start);
	for (i = 0 ; i < n ; i++, paddr += PAGE_SIZE) {
		seg->pgs[i].phys_addr = paddr;
#ifdef __HAVE_VM_PAGE_MD
		VM_MDPAGE_INIT(&seg->pgs[i]);
#endif
		if (atop(paddr) >= seg->avail_start &&
		    atop(paddr) < seg->avail_end) {
			uvmexp.npages++;
			mutex_enter(&uvm_pageqlock);
			/* add page to free pool */
			uvm_pagefree(&seg->pgs[i]);
			mutex_exit(&uvm_pageqlock);
		}
	}
}

void
uvm_physseg_seg_chomp_slab(uvm_physseg_t upm, struct vm_page *pgs, size_t n)
{
	struct uvm_physseg *seg = HANDLE_TO_PHYSSEG_NODE(upm);

	/* max number of pre-boot unplug()s allowed */
#define UVM_PHYSSEG_BOOT_UNPLUG_MAX VM_PHYSSEG_MAX

	static char btslab_ex_storage[EXTENT_FIXED_STORAGE_SIZE(UVM_PHYSSEG_BOOT_UNPLUG_MAX)];

	if (__predict_false(uvm.page_init_done == false)) {
		seg->ext = extent_create("Boot time slab", (u_long) pgs, (u_long) (pgs + n),
		    (void *)btslab_ex_storage, sizeof(btslab_ex_storage), 0);
	} else {
		seg->ext = extent_create("Hotplug slab", (u_long) pgs, (u_long) (pgs + n), NULL, 0, 0);
	}

	KASSERT(seg->ext != NULL);

}

struct vm_page *
uvm_physseg_seg_alloc_from_slab(uvm_physseg_t upm, size_t pages)
{
	int err;
	struct uvm_physseg *seg;
	struct vm_page *pgs = NULL;

	KASSERT(pages > 0);

	seg = HANDLE_TO_PHYSSEG_NODE(upm);

	if (__predict_false(seg->ext == NULL)) {
		/*
		 * This is a situation unique to boot time.
		 * It shouldn't happen at any point other than from
		 * the first uvm_page.c:uvm_page_init() call
		 * Since we're in a loop, we can get away with the
		 * below.
		 */
		KASSERT(uvm.page_init_done != true);

		uvm_physseg_t upmp = uvm_physseg_get_prev(upm);
		KASSERT(upmp != UVM_PHYSSEG_TYPE_INVALID);

		seg->ext = HANDLE_TO_PHYSSEG_NODE(upmp)->ext;

		KASSERT(seg->ext != NULL);
	}

	/* We allocate enough for this segment */
	err = extent_alloc(seg->ext, sizeof(*pgs) * pages, 1, 0, EX_BOUNDZERO, (u_long *)&pgs);

	if (err != 0) {
#ifdef DEBUG
		printf("%s: extent_alloc failed with error: %d \n",
		    __func__, err);
#endif
	}

	return pgs;
}

/*
 * uvm_page_physload: load physical memory into VM system
 *
 * => all args are PFs
 * => all pages in start/end get vm_page structures
 * => areas marked by avail_start/avail_end get added to the free page pool
 * => we are limited to VM_PHYSSEG_MAX physical memory segments
 */

uvm_physseg_t
uvm_page_physload(paddr_t start, paddr_t end, paddr_t avail_start,
    paddr_t avail_end, int free_list)
{
	struct uvm_physseg *ps;
	uvm_physseg_t upm;

	if (__predict_true(uvm.page_init_done == true))
		panic("%s: unload attempted after uvm_page_init()\n", __func__);
	if (uvmexp.pagesize == 0)
		panic("uvm_page_physload: page size not set!");
	if (free_list >= VM_NFREELIST || free_list < VM_FREELIST_DEFAULT)
		panic("uvm_page_physload: bad free list %d", free_list);
	if (start >= end)
		panic("uvm_page_physload: start >= end");

	if (uvm_physseg_plug(start, end - start, &upm) == false) {
		panic("uvm_physseg_plug() failed at boot.");
		/* NOTREACHED */
		return UVM_PHYSSEG_TYPE_INVALID; /* XXX: correct type */
	}

	ps = HANDLE_TO_PHYSSEG_NODE(upm);

	/* Legacy */
	ps->avail_start = avail_start;
	ps->avail_end = avail_end;

	ps->free_list = free_list; /* XXX: */


	return upm;
}

bool
uvm_physseg_unplug(paddr_t pfn, size_t pages)
{
	uvm_physseg_t upm;
	paddr_t off = 0, start __diagused, end;
	struct uvm_physseg *seg;

	upm = uvm_physseg_find(pfn, &off);

	if (!uvm_physseg_valid_p(upm)) {
		printf("%s: Tried to unplug from unknown offset\n", __func__);
		return false;
	}

	seg = HANDLE_TO_PHYSSEG_NODE(upm);

	start = uvm_physseg_get_start(upm);
	end = uvm_physseg_get_end(upm);

	if (end < (pfn + pages)) {
		printf("%s: Tried to unplug oversized span \n", __func__);
		return false;
	}

	KASSERT(pfn == start + off); /* sanity */

	if (__predict_true(uvm.page_init_done == true)) {
		/* XXX: KASSERT() that seg->pgs[] are not on any uvm lists */
		if (extent_free(seg->ext, (u_long)(seg->pgs + off), sizeof(struct vm_page) * pages, EX_MALLOCOK | EX_NOWAIT) != 0)
			return false;
	}

	if (off == 0 && (pfn + pages) == end) {
#if defined(UVM_HOTPLUG) /* rbtree implementation */
		int segcount = 0;
		struct uvm_physseg *current_ps;
		/* Complete segment */
		if (uvm_physseg_graph.nentries == 1)
			panic("%s: out of memory!", __func__);

		if (__predict_true(uvm.page_init_done == true)) {
			RB_TREE_FOREACH(current_ps, &(uvm_physseg_graph.rb_tree)) {
				if (seg->ext == current_ps->ext)
					segcount++;
			}
			KASSERT(segcount > 0);

			if (segcount == 1) {
				extent_destroy(seg->ext);
			}

			/*
			 * We assume that the unplug will succeed from
			 *  this point onwards
			 */
			uvmexp.npages -= (int) pages;
		}

		rb_tree_remove_node(&(uvm_physseg_graph.rb_tree), upm);
		memset(seg, 0, sizeof(struct uvm_physseg));
		uvm_physseg_free(seg, sizeof(struct uvm_physseg));
		uvm_physseg_graph.nentries--;
#else /* UVM_HOTPLUG */
		int x;
		if (vm_nphysmem == 1)
			panic("uvm_page_physget: out of memory!");
		vm_nphysmem--;
		for (x = upm ; x < vm_nphysmem ; x++)
			/* structure copy */
			VM_PHYSMEM_PTR_SWAP(x, x + 1);
#endif /* UVM_HOTPLUG */
		/* XXX: KASSERT() that seg->pgs[] are not on any uvm lists */
		return true;
	}

	if (off > 0 &&
	    (pfn + pages) < end) {
#if defined(UVM_HOTPLUG) /* rbtree implementation */
		/* middle chunk - need a new segment */
		struct uvm_physseg *ps, *current_ps;
		ps = uvm_physseg_alloc(sizeof (struct uvm_physseg));
		if (ps == NULL) {
			printf("%s: Unable to allocated new fragment vm_physseg \n",
			    __func__);
			return false;
		}

		/* Remove middle chunk */
		if (__predict_true(uvm.page_init_done == true)) {
			KASSERT(seg->ext != NULL);
			ps->ext = seg->ext;

			/* XXX: KASSERT() that seg->pgs[] are not on any uvm lists */
			/*
			 * We assume that the unplug will succeed from
			 *  this point onwards
			 */
			uvmexp.npages -= (int) pages;
		}

		ps->start = pfn + pages;
		ps->avail_start = ps->start; /* XXX: Legacy */

		ps->end = seg->end;
		ps->avail_end = ps->end; /* XXX: Legacy */

		seg->end = pfn;
		seg->avail_end = seg->end; /* XXX: Legacy */


		/*
		 * The new pgs array points to the beginning of the
		 * tail fragment.
		 */
		if (__predict_true(uvm.page_init_done == true))
			ps->pgs = seg->pgs + off + pages;

		current_ps = rb_tree_insert_node(&(uvm_physseg_graph.rb_tree), ps);
		if (current_ps != ps) {
			panic("uvm_page_physload: Duplicate address range detected!");
		}
		uvm_physseg_graph.nentries++;
#else /* UVM_HOTPLUG */
		panic("%s: can't unplug() from the middle of a segment without"
		    " UVM_HOTPLUG\n",  __func__);
		/* NOTREACHED */
#endif /* UVM_HOTPLUG */
		return true;
	}

	if (off == 0 && (pfn + pages) < end) {
		/* Remove front chunk */
		if (__predict_true(uvm.page_init_done == true)) {
			/* XXX: KASSERT() that seg->pgs[] are not on any uvm lists */
			/*
			 * We assume that the unplug will succeed from
			 *  this point onwards
			 */
			uvmexp.npages -= (int) pages;
		}

		/* Truncate */
		seg->start = pfn + pages;
		seg->avail_start = seg->start; /* XXX: Legacy */

		/*
		 * Move the pgs array start to the beginning of the
		 * tail end.
		 */
		if (__predict_true(uvm.page_init_done == true))
			seg->pgs += pages;

		return true;
	}

	if (off > 0 && (pfn + pages) == end) {
		/* back chunk */


		/* Truncate! */
		seg->end = pfn;
		seg->avail_end = seg->end; /* XXX: Legacy */

		uvmexp.npages -= (int) pages;

		return true;
	}

	printf("%s: Tried to unplug unknown range \n", __func__);

	return false;
}
