/*	$NetBSD: vm_page.c,v 1.48 1999/01/16 20:00:28 chuck Exp $	*/

#define	VM_PAGE_ALLOC_MEMORY_STATS

/*-
 * Copyright (c) 1997 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe of the Numerical Aerospace Simulation Facility,
 * NASA Ames Research Center.
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
 *	This product includes software developed by the NetBSD
 *	Foundation, Inc. and its contributors.
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

/* 
 * Copyright (c) 1991, 1993
 *	The Regents of the University of California.  All rights reserved.
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
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
 *	@(#)vm_page.c	8.4 (Berkeley) 1/9/95
 *
 *
 * Copyright (c) 1987, 1990 Carnegie-Mellon University.
 * All rights reserved.
 *
 * Authors: Avadis Tevanian, Jr., Michael Wayne Young
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
 *	Resident memory management module.
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/malloc.h>

#include <vm/vm.h>
#include <vm/vm_page.h>
#include <vm/vm_map.h>
#include <vm/vm_pageout.h>

#include <machine/cpu.h>

#define VERY_LOW_MEM()   (cnt.v_free_count <= vm_page_free_reserved)
#define KERN_OBJ(object) ((object) == kernel_object || (object) == kmem_object)

int	vm_page_free_reserved = 10;

/*
 * physical memory config is stored in vm_physmem.
 */

struct vm_physseg vm_physmem[VM_PHYSSEG_MAX];
int vm_nphysseg = 0;
static int vm_page_lost_count = 0; /* XXXCDC: DEBUG DEBUG */

/*
 *	These variables record the values returned by vm_page_bootstrap,
 *	for debugging purposes.
 *
 *	The implementation of vm_bootstrap_steal_memory here also uses
 *	them internally.
 */
static vaddr_t	virtual_space_start;
static vaddr_t	virtual_space_end;

vaddr_t	vm_bootstrap_steal_memory __P((vsize_t));

/*
 *	Associated with page of user-allocatable memory is a
 *	page structure.
 */

struct pglist	*vm_page_buckets;		/* Array of buckets */
int		vm_page_bucket_count = 0;	/* How big is array? */
int		vm_page_hash_mask;		/* Mask for hash function */
simple_lock_data_t	bucket_lock;		/* lock for all buckets XXX */
struct pglist	vm_page_bootbucket;		/* bootstrap bucket */

struct pglist	vm_page_queue_free;
struct pglist	vm_page_queue_active;
struct pglist	vm_page_queue_inactive;
simple_lock_data_t	vm_page_queue_lock;
simple_lock_data_t	vm_page_queue_free_lock;

/* has physical page allocation been initialized? */
boolean_t vm_page_startup_initialized;

vm_page_t	vm_page_array;
vsize_t	page_mask;
int		page_shift;

/*
 * local prototypes
 */

#if !defined(PMAP_STEAL_MEMORY)
static boolean_t vm_page_physget __P((paddr_t *));
#endif

/*
 * macros
 */

/*
 *	vm_page_hash:
 *
 *	Distributes the object/offset key pair among hash buckets.
 *
 *	NOTE:  This macro depends on vm_page_bucket_count being a power of 2.
 */
#define vm_page_hash(object, offset) \
	(((unsigned long)object+(unsigned long)atop(offset))&vm_page_hash_mask)

/*
 *	vm_set_page_size:
 *
 *	Sets the page size, perhaps based upon the memory
 *	size.  Must be called before any use of page-size
 *	dependent functions.
 *
 *	Sets page_shift and page_mask from cnt.v_page_size.
 */
void
vm_set_page_size()
{

	if (cnt.v_page_size == 0)
		cnt.v_page_size = DEFAULT_PAGE_SIZE;
	page_mask = cnt.v_page_size - 1;
	if ((page_mask & cnt.v_page_size) != 0)
		panic("vm_set_page_size: page size not a power of two");
	for (page_shift = 0; ; page_shift++)
		if ((1 << page_shift) == cnt.v_page_size)
			break;
}

/*
 * vm_page_bootstrap: initialize the resident memory module (called
 * from vm_mem_init()).
 *
 * - startp and endp are out params which return the boundaries of the
 *   free part of the kernel's virtual address space.
 */
void
vm_page_bootstrap(startp, endp)
	vaddr_t *startp, *endp;	/* OUT, OUT */
{
	paddr_t paddr;
	vm_page_t pagearray;
	int	lcv, freepages, pagecount, n, i;

	/*
	 * first init all the locks and queues.
	 */
	simple_lock_init(&vm_page_queue_free_lock);
	simple_lock_init(&vm_page_queue_lock);
	TAILQ_INIT(&vm_page_queue_free);
	TAILQ_INIT(&vm_page_queue_active);
	TAILQ_INIT(&vm_page_queue_inactive);

	/*
	 * init the <OBJ,OFFSET> => <PAGE> hash table buckets.   for now
	 * we just have one bucket (the bootstrap bucket).   later on we
	 * will malloc() new buckets as we dynamically resize the hash table.
	 */
	vm_page_bucket_count = 1;
	vm_page_hash_mask = 0;
	vm_page_buckets = &vm_page_bootbucket;
	TAILQ_INIT(vm_page_buckets);
	simple_lock_init(&bucket_lock);

	/*
	 * before calling this function the MD code is expected to register
	 * some free RAM with the vm_page_physload() function.   our job
	 * now is to allocate vm_page structures for this preloaded memory.
	 */
	if (vm_nphysseg == 0) 
		panic("vm_page_bootstrap: no memory pre-allocated");

	/*
	 * first calculate the number of free pages...  note that start/end
	 * are inclusive so you have to add one to get the number of pages.
	 *
	 * note that we use start/end rather than avail_start/avail_end.
	 * this allows us to allocate extra vm_page structures in case we
	 * want to return some memory to the pool after booting.
	 */
	freepages = 0;
	for (lcv = 0; lcv < vm_nphysseg; lcv++) {
		freepages = freepages +
		    (vm_physmem[lcv].end - vm_physmem[lcv].start);
	}

	/*
	 * we now know we have (PAGE_SIZE * freepages) bytes of memory we can
	 * use.   for each page of memory we use we need a vm_page structure.
	 * thus, the total number of pages we can use is the total size of
	 * the memory divided by the PAGE_SIZE plus the size of the vm_page
	 * structure.   we add one to freepages as a fudge factor to avoid
	 * truncation errors (since we can only allocate in terms of whole
	 * pages).
	 */
	pagecount = (PAGE_SIZE * (freepages + 1)) / 
		    (PAGE_SIZE + sizeof(struct vm_page));
	pagearray = (vm_page_t)
	    vm_bootstrap_steal_memory(pagecount * sizeof(struct vm_page));
	memset(pagearray, 0, pagecount * sizeof(struct vm_page));

	/*
	 * now init the page frames
	 */
	for (lcv = 0; lcv < vm_nphysseg; lcv++) {

		n = vm_physmem[lcv].end - vm_physmem[lcv].start;
		if (n > pagecount) {
			printf("vm_init: lost %d page(s) in init\n",
			    n - pagecount);
			vm_page_lost_count += (n - pagecount);
			n = pagecount;
		}

		/* set up page array pointers */
		vm_physmem[lcv].pgs = pagearray;
		pagearray += n;
		pagecount -= n;
		vm_physmem[lcv].lastpg = vm_physmem[lcv].pgs + (n - 1);

		/* init and free vm_pages (we've already zeroed them) */
		paddr = ptoa(vm_physmem[lcv].start);
		for (i = 0; i < n; i++, paddr += PAGE_SIZE) {
			vm_physmem[lcv].pgs[i].phys_addr = paddr;
			if (atop(paddr) >= vm_physmem[lcv].avail_start && 
			    atop(paddr) <= vm_physmem[lcv].avail_end)
				vm_page_free(&vm_physmem[lcv].pgs[i]);
		}
	}

	/*
	 * pass up the values of virtual_space_start and virtual_space_end
	 * (obtained by vm_bootstrap_steal_memory) to the upper layers of
	 * the VM.
	 */
	*startp = round_page(virtual_space_start);
	*endp = trunc_page(virtual_space_end);

	/*
	 * init pagedaemon lock
	 */
	simple_lock_init(&vm_pages_needed_lock);
}

/*
 * vm_bootstrap_steal_memory: steal memory from physmem for bootstrapping
 */
vaddr_t
vm_bootstrap_steal_memory(size)
	vsize_t size;
{
#if defined(PMAP_STEAL_MEMORY)
	vaddr_t addr;

	/*
	 * Defer this to machine-dependent code; we may need to allocate
	 * from a direct-mapped segment.
	 */
	addr = pmap_steal_memory(size, &virtual_space_start,
	    &virtual_space_end);

	/* round it the way we like it */
	virtual_space_start = round_page(virtual_space_start);
	virtual_space_end = trunc_page(virtual_space_end);

	return (addr);
#else /* ! PMAP_STEAL_MEMORY */
	vaddr_t addr, vaddr, paddr;

	/* round to page size */
	size = round_page(size);

	/*
	 * on first call to this function init ourselves.   we detect this
	 * by checking virtual_space_start/end which are in the zero'd BSS
	 * area.
	 */
	if (virtual_space_start == virtual_space_end) {
		pmap_virtual_space(&virtual_space_start, &virtual_space_end);
    
		/* round it the way we like it */
		virtual_space_start = round_page(virtual_space_start);
		virtual_space_end = trunc_page(virtual_space_end);
	}
  
	/*
	 * allocate virtual memory for this request
	 */
	addr = virtual_space_start;
	virtual_space_start += size;

	/*
	 * allocate and mapin physical pages to back new virtual pages
	 */
	for (vaddr = round_page(addr); vaddr < addr + size;
	    vaddr += PAGE_SIZE) {
		if (!vm_page_physget(&paddr))
			panic("vm_bootstrap_steal_memory: out of memory");

		/* XXX: should be wired, but some pmaps don't like that ... */
		pmap_enter(pmap_kernel(), vaddr, paddr,
		    VM_PROT_READ|VM_PROT_WRITE, FALSE);
	}
	return(addr);
#endif /* PMAP_STEAL_MEMORY */
}

#if !defined(PMAP_STEAL_MEMORY)
/*
 * vm_page_physget: "steal" one page from the vm_physmem structure.
 *
 * - attempt to allocate it off the end of a segment in which the "avail"
 *   values match the start/end values.   if we can't do that, then we
 *   will advance both values (making them equal, and removing some
 *   vm_page structures from the non-avail area).
 * - return false if out of memory.
 */
static boolean_t
vm_page_physget(paddrp)
	paddr_t *paddrp;

{
	int	lcv, x;

	/* pass 1: try allocating from a matching end */
#if (VM_PHYSSEG_STRAT == VM_PSTRAT_BIGFIRST)
	for (lcv = vm_nphysseg - 1 ; lcv >= 0 ; lcv--) 
#else
	for (lcv = 0 ; lcv < vm_nphysseg ; lcv++) 
#endif
	{
		if (vm_physmem[lcv].pgs)
			panic("vm_page_physget: called _after_ bootstrap");
    
		/* try from front */
		if (vm_physmem[lcv].avail_start == vm_physmem[lcv].start && 
		    vm_physmem[lcv].avail_start < vm_physmem[lcv].avail_end) {
			*paddrp = ptoa(vm_physmem[lcv].avail_start);
			vm_physmem[lcv].avail_start++;
			vm_physmem[lcv].start++;

			/* nothing left?   nuke it */
			if (vm_physmem[lcv].avail_start ==
			    vm_physmem[lcv].end) {
				if (vm_nphysseg == 1)
				    panic("vm_page_physget: out of memory!");
				vm_nphysseg--;
				for (x = lcv; x < vm_nphysseg; x++)
					/* structure copy */
					vm_physmem[x] = vm_physmem[x+1];
			}
			return(TRUE);
		}

		/* try from rear */
		if (vm_physmem[lcv].avail_end == vm_physmem[lcv].end &&
		    vm_physmem[lcv].avail_start < vm_physmem[lcv].avail_end) {
			*paddrp = ptoa(vm_physmem[lcv].avail_end - 1);
			vm_physmem[lcv].avail_end--;
			vm_physmem[lcv].end--;

			/* nothing left?   nuke it */
			if (vm_physmem[lcv].avail_end ==
			    vm_physmem[lcv].start) {
				if (vm_nphysseg == 1)
				    panic("vm_page_physget: out of memory!");
				vm_nphysseg--;
				for (x = lcv; x < vm_nphysseg; x++)
					/* structure copy */
					vm_physmem[x] = vm_physmem[x+1];
			}
			return(TRUE);
		}
	}

	/* pass2: forget about matching ends, just allocate something */
#if (VM_PHYSSEG_STRAT == VM_PSTRAT_BIGFIRST)
	for (lcv = vm_nphysseg - 1 ; lcv >= 0 ; lcv--) 
#else
	for (lcv = 0 ; lcv < vm_nphysseg ; lcv++) 
#endif
	{
		/* any room in this bank? */
		if (vm_physmem[lcv].avail_start >= vm_physmem[lcv].avail_end)
			continue;  /* nope */

		*paddrp = ptoa(vm_physmem[lcv].avail_start);
		vm_physmem[lcv].avail_start++;
		vm_physmem[lcv].start = vm_physmem[lcv].avail_start; /* truncate! */

		/* nothing left?   nuke it */
		if (vm_physmem[lcv].avail_start == vm_physmem[lcv].end) {
			if (vm_nphysseg == 1)
				panic("vm_page_physget: out of memory!");
			vm_nphysseg--;
			for (x = lcv; x < vm_nphysseg; x++)
				vm_physmem[x] = vm_physmem[x+1];  /* structure copy */
		}
		return(TRUE);
	}

	return(FALSE);	/* whoops! */
}
#endif /* ! PMAP_STEAL_MEMORY */

/*
 * vm_page_physload: load physical memory into VM system
 *
 * - all args are PFs
 * - all pages in start/end get vm_page structures
 * - areas marked by avail_start/avail_end get added to the free page pool
 * - we are limited to VM_PHYSSEG_MAX physical memory segments
 */
void
vm_page_physload(start, end, avail_start, avail_end)
	vaddr_t start, end, avail_start, avail_end;
{
	struct	vm_page *pgs;
	struct	vm_physseg *ps;
	int	preload, lcv, npages;
#if (VM_PHYSSEG_STRAT != VM_PSTRAT_RANDOM)
	int     x;
#endif
  
	if (page_shift == 0)
		panic("vm_page_physload: page size not set!");

	/*
	 * do we have room?
	 */
	if (vm_nphysseg == VM_PHYSSEG_MAX) {
	   printf("vm_page_physload: unable to load physical memory segment\n");
		printf("\t%d segments allocated, ignoring 0x%lx -> 0x%lx\n", 
		    VM_PHYSSEG_MAX, start, end);
		return;
	}

	/*
	 * check to see if this is a "preload" (i.e. vm_mem_init hasn't been
	 * called yet, so malloc is not available).
	 */
	for (lcv = 0; lcv < vm_nphysseg; lcv++) {
		if (vm_physmem[lcv].pgs)
			break;
	}
	preload = (lcv == vm_nphysseg);

	/*
	 * if VM is already running, attempt to malloc() vm_page structures
	 */
	if (!preload) {
#if defined(VM_PHYSSEG_NOADD)
		panic("vm_page_physload: tried to add RAM after vm_mem_init");
#else
/* XXXCDC: need some sort of lockout for this case */
		paddr_t paddr;

		/* # of pages */
		npages = end - start;
		MALLOC(pgs, struct vm_page *, sizeof(struct vm_page) * npages, 
		    M_VMPAGE, M_NOWAIT);
		if (pgs == NULL) {
	printf("vm_page_physload: can not malloc vm_page structs for segment\n");
			printf("\tignoring 0x%lx -> 0x%lx\n", start, end);
			return;
		}
		/* zero data, init phys_addr, and free pages */
		memset(pgs, 0, sizeof(struct vm_page) * npages);
		for (lcv = 0, paddr = ptoa(start); lcv < npages;
		    lcv++, paddr += PAGE_SIZE) {
			pgs[lcv].phys_addr = paddr;
			if (atop(paddr) >= avail_start &&
			    atop(paddr) <= avail_end)
				vm_page_free(&pgs[lcv]);
		}
/* XXXCDC: incomplete: need to update v_free_count, what else? */
/* XXXCDC: need hook to tell pmap to rebuild pv_list, etc... */
#endif
	} else {
		/* XXX/gcc complains if these don't get init'd */
		pgs = NULL; 
		npages = 0; 
	}

	/*
	 * now insert us in the proper place in vm_physmem[]
	 */
#if (VM_PHYSSEG_STRAT == VM_PSTRAT_RANDOM)
	/* random: put it at the end (easy!) */
	ps = &vm_physmem[vm_nphysseg];

#else
#if (VM_PHYSSEG_STRAT == VM_PSTRAT_BSEARCH)

	/* sort by address for binary search */
	for (lcv = 0 ; lcv < vm_nphysseg ; lcv++)
		if (start < vm_physmem[lcv].start)
			break;
	ps = &vm_physmem[lcv];

	/* move back other entries, if necessary ... */
	for (x = vm_nphysseg ; x > lcv ; x--)
		/* structure copy */
		vm_physmem[x] = vm_physmem[x - 1];

#else
#if (VM_PHYSSEG_STRAT == VM_PSTRAT_BIGFIRST)

	/* sort by largest segment first */
	for (lcv = 0 ; lcv < vm_nphysseg ; lcv++)
		if ((end - start) >
		    (vm_physmem[lcv].end - vm_physmem[lcv].start))
			break;
	ps = &vm_physmem[lcv];

	/* move back other entries, if necessary ... */
	for (x = vm_nphysseg ; x > lcv ; x--)
		/* structure copy */
		vm_physmem[x] = vm_physmem[x - 1];
  
#else
  
	panic("vm_page_physload: unknown physseg strategy selected!");

#endif
#endif
#endif

	ps->start = start;
	ps->end = end;
	ps->avail_start = avail_start;
	ps->avail_end = avail_end;
	if (preload) {
		ps->pgs = NULL;
	} else {
		ps->pgs = pgs;
		ps->lastpg = pgs + npages - 1;
	}
	vm_nphysseg++;

	/*
	 * done! 
	 */
	return;
}

/*
 * vm_page_physrehash: reallocate hash table based on number of
 * free pages.
 */
void
vm_page_physrehash()
{
	struct	pglist *newbuckets, *oldbuckets;
	struct	vm_page *pg;
	int	freepages, lcv, bucketcount, s, oldcount;

	/*
	 * compute number of pages that can go in the free pool
	 */
	freepages = 0;
	for (lcv = 0; lcv < vm_nphysseg; lcv++)
		freepages = freepages + (vm_physmem[lcv].avail_end -
		    vm_physmem[lcv].avail_start);

	/*
	 * compute number of buckets needed for this number of pages
	 */
	bucketcount = 1;
	while (bucketcount < freepages)
		bucketcount = bucketcount * 2;

	/*
	 * malloc new buckets
	 */
	MALLOC(newbuckets, struct pglist *, sizeof(struct pglist) * bucketcount, 
	    M_VMPBUCKET, M_NOWAIT);
	if (newbuckets == NULL) {
    printf("vm_page_physrehash: WARNING: could not grow page hash table\n");
		return;
	}
	for (lcv = 0; lcv < bucketcount; lcv++)
		TAILQ_INIT(&newbuckets[lcv]);
  
	/*
	 * now replace the old buckets with the new ones and rehash everything
	 */
	s = splimp();
	simple_lock(&bucket_lock);
	/* swap old for new ... */
	oldbuckets = vm_page_buckets;
	oldcount = vm_page_bucket_count;
	vm_page_buckets = newbuckets;
	vm_page_bucket_count = bucketcount;
	vm_page_hash_mask = bucketcount - 1;  /* power of 2 */

	/* ... and rehash */
	for (lcv = 0 ; lcv < oldcount ; lcv++) {
		while ((pg = oldbuckets[lcv].tqh_first) != NULL) {
			TAILQ_REMOVE(&oldbuckets[lcv], pg, hashq);
			TAILQ_INSERT_TAIL(&vm_page_buckets[
			    vm_page_hash(pg->object, pg->offset)], pg, hashq);
		}
	}
	simple_unlock(&bucket_lock);
	splx(s);

	/*
	 * free old bucket array if we malloc'd it previously 
	 */
	if (oldbuckets != &vm_page_bootbucket)
		FREE(oldbuckets, M_VMPBUCKET);

	/*
	 * done
	 */
	return;
}

#if 1 /* XXXCDC: TMP TMP TMP DEBUG DEBUG DEBUG */

void vm_page_physdump __P((void)); /* SHUT UP GCC */

/* call from DDB */
void
vm_page_physdump()
{
	int	lcv;

	printf("rehash: physical memory config [segs=%d of %d]:\n", 
	    vm_nphysseg, VM_PHYSSEG_MAX);
	for (lcv = 0 ; lcv < vm_nphysseg ; lcv++)
		printf("0x%lx->0x%lx [0x%lx->0x%lx]\n", vm_physmem[lcv].start,
		    vm_physmem[lcv].end, vm_physmem[lcv].avail_start,
		    vm_physmem[lcv].avail_end);
	printf("STRATEGY = ");

	switch (VM_PHYSSEG_STRAT) {
	case VM_PSTRAT_RANDOM:
		printf("RANDOM\n");
		break;

	case VM_PSTRAT_BSEARCH:
		printf("BSEARCH\n");
		break;

	case VM_PSTRAT_BIGFIRST:
		printf("BIGFIRST\n");
		break;

	default:
		printf("<<UNKNOWN>>!!!!\n");
	}
	printf("number of buckets = %d\n", vm_page_bucket_count);
	printf("number of lost pages = %d\n", vm_page_lost_count);
}
#endif

/*
 *	vm_page_insert:		[ internal use only ]
 *
 *	Inserts the given mem entry into the object/object-page
 *	table and object list.
 *
 *	The object and page must be locked.
 */
void
vm_page_insert(mem, object, offset)
	register vm_page_t	mem;
	register vm_object_t	object;
	register vaddr_t	offset;
{
	register struct pglist	*bucket;
	int			spl;

	VM_PAGE_CHECK(mem);

	if (mem->flags & PG_TABLED)
		panic("vm_page_insert: already inserted");

	/*
	 *	Record the object/offset pair in this page
	 */

	mem->object = object;
	mem->offset = offset;

	/*
	 *	Insert it into the object_object/offset hash table
	 */

	bucket = &vm_page_buckets[vm_page_hash(object, offset)];
	spl = splimp();
	simple_lock(&bucket_lock);
	TAILQ_INSERT_TAIL(bucket, mem, hashq);
	simple_unlock(&bucket_lock);
	(void) splx(spl);

	/*
	 *	Now link into the object's list of backed pages.
	 */

	TAILQ_INSERT_TAIL(&object->memq, mem, listq);
	mem->flags |= PG_TABLED;

	/*
	 *	And show that the object has one more resident
	 *	page.
	 */

	object->resident_page_count++;
}

/*
 *	vm_page_remove:		[ internal use only ]
 * XXX: used by device pager as well
 *
 *	Removes the given mem entry from the object/offset-page
 *	table and the object page list.
 *
 *	The object and page must be locked.
 */
void
vm_page_remove(mem)
	register vm_page_t	mem;
{
	register struct pglist	*bucket;
	int			spl;

	VM_PAGE_CHECK(mem);

#ifdef DIAGNOSTIC
	if (mem->flags & PG_FAULTING)
		panic("vm_page_remove: page is faulting");
#endif

	if (!(mem->flags & PG_TABLED))
		return;

	/*
	 *	Remove from the object_object/offset hash table
	 */

	bucket = &vm_page_buckets[vm_page_hash(mem->object, mem->offset)];
	spl = splimp();
	simple_lock(&bucket_lock);
	TAILQ_REMOVE(bucket, mem, hashq);
	simple_unlock(&bucket_lock);
	(void) splx(spl);

	/*
	 *	Now remove from the object's list of backed pages.
	 */

	TAILQ_REMOVE(&mem->object->memq, mem, listq);

	/*
	 *	And show that the object has one fewer resident
	 *	page.
	 */

	mem->object->resident_page_count--;

	mem->flags &= ~PG_TABLED;
}

/*
 *	vm_page_lookup:
 *
 *	Returns the page associated with the object/offset
 *	pair specified; if none is found, NULL is returned.
 *
 *	The object must be locked.  No side effects.
 */
vm_page_t
vm_page_lookup(object, offset)
	register vm_object_t	object;
	register vaddr_t	offset;
{
	register vm_page_t	mem;
	register struct pglist	*bucket;
	int			spl;

	/*
	 *	Search the hash table for this object/offset pair
	 */

	bucket = &vm_page_buckets[vm_page_hash(object, offset)];

	spl = splimp();
	simple_lock(&bucket_lock);
	for (mem = bucket->tqh_first; mem != NULL; mem = mem->hashq.tqe_next) {
		VM_PAGE_CHECK(mem);
		if ((mem->object == object) && (mem->offset == offset)) {
			simple_unlock(&bucket_lock);
			splx(spl);
			return(mem);
		}
	}

	simple_unlock(&bucket_lock);
	splx(spl);
	return(NULL);
}

/*
 *	vm_page_rename:
 *
 *	Move the given memory entry from its
 *	current object to the specified target object/offset.
 *
 *	The object must be locked.
 */
void
vm_page_rename(mem, new_object, new_offset)
	register vm_page_t	mem;
	register vm_object_t	new_object;
	vaddr_t		new_offset;
{

	if (mem->object == new_object)
		return;

	vm_page_lock_queues();	/* keep page from moving out from
				   under pageout daemon */
    	vm_page_remove(mem);
	vm_page_insert(mem, new_object, new_offset);
	vm_page_unlock_queues();
}

/*
 *	vm_page_alloc:
 *
 *	Allocate and return a memory cell associated
 *	with this VM object/offset pair.
 *
 *	Object must be locked.
 */

vm_page_t
vm_page_alloc(object, offset)
	vm_object_t	object;
	vaddr_t	offset;
{
	register vm_page_t	mem;
	int		spl;

	spl = splimp();				/* XXX */
	simple_lock(&vm_page_queue_free_lock);
	mem = vm_page_queue_free.tqh_first;

	if (VERY_LOW_MEM()) {
		if ((!KERN_OBJ(object) && curproc != pageout_daemon)
		   || mem == NULL) {
			simple_unlock(&vm_page_queue_free_lock);
			splx(spl);
			return(NULL);
		}
	}
#ifdef DIAGNOSTIC
	if (mem == NULL) /* because we now depend on VERY_LOW_MEM() */
		panic("vm_page_alloc");
#endif
	TAILQ_REMOVE(&vm_page_queue_free, mem, pageq);

	cnt.v_free_count--;
	simple_unlock(&vm_page_queue_free_lock);
	splx(spl);

	VM_PAGE_INIT(mem, object, offset);

	/*
	 *	Decide if we should poke the pageout daemon.
	 *	We do this if the free count is less than the low
	 *	water mark, or if the free count is less than the high
	 *	water mark (but above the low water mark) and the inactive
	 *	count is less than its target.
	 *
	 *	We don't have the counts locked ... if they change a little,
	 *	it doesn't really matter.
	 */

	if (cnt.v_free_count < cnt.v_free_min ||
	    (cnt.v_free_count < cnt.v_free_target &&
	     cnt.v_inactive_count < cnt.v_inactive_target))
		thread_wakeup(&vm_pages_needed);
	return (mem);
}

/*
 *	vm_page_free:
 *
 *	Returns the given page to the free list,
 *	disassociating it with any VM object.
 *
 *	Object and page must be locked prior to entry.
 */
void
vm_page_free(mem)
	register vm_page_t	mem;
{

	vm_page_remove(mem);
	vm_page_free1(mem);
}

/*
 *	vm_page_alloc1:
 *
 *	Allocate and return a memory cell with no associated object.
 */
vm_page_t
vm_page_alloc1()
{
	vm_page_t	mem;
	int		spl;

	spl = splimp();
	simple_lock(&vm_page_queue_free_lock);
	if (vm_page_queue_free.tqh_first == NULL) {
		simple_unlock(&vm_page_queue_free_lock);
		splx(spl);
		return (NULL);
	}

	mem = vm_page_queue_free.tqh_first;
	TAILQ_REMOVE(&vm_page_queue_free, mem, pageq);

	cnt.v_free_count--;
	simple_unlock(&vm_page_queue_free_lock);
	splx(spl);

	mem->flags = PG_BUSY | PG_CLEAN | PG_FAKE;
	mem->wire_count = 0;

	/*
	 *	Decide if we should poke the pageout daemon.
	 *	We do this if the free count is less than the low
	 *	water mark, or if the free count is less than the high
	 *	water mark (but above the low water mark) and the inactive
	 *	count is less than its target.
	 *
	 *	We don't have the counts locked ... if they change a little,
	 *	it doesn't really matter.
	 */

	if (cnt.v_free_count < cnt.v_free_min ||
	    (cnt.v_free_count < cnt.v_free_target &&
	     cnt.v_inactive_count < cnt.v_inactive_target))
		thread_wakeup((void *)&vm_pages_needed);

	return (mem);
}

/*
 *	vm_page_free1:
 *
 *	Returns the given page to the free list.
 *
 *	The page must already be disassociated with
 *	any objects.
 */
void
vm_page_free1(mem)
	vm_page_t mem;
{

	if (mem->flags & PG_ACTIVE) {
		TAILQ_REMOVE(&vm_page_queue_active, mem, pageq);
		mem->flags &= ~PG_ACTIVE;
		cnt.v_active_count--;
	}

	if (mem->flags & PG_INACTIVE) {
		TAILQ_REMOVE(&vm_page_queue_inactive, mem, pageq);
		mem->flags &= ~PG_INACTIVE;
		cnt.v_inactive_count--;
	}

	if (!(mem->flags & PG_FICTITIOUS)) {
		int	spl;

		spl = splimp();
		simple_lock(&vm_page_queue_free_lock);
		mem->flags |= PG_FREE;
		TAILQ_INSERT_TAIL(&vm_page_queue_free, mem, pageq);

		cnt.v_free_count++;
		simple_unlock(&vm_page_queue_free_lock);
		splx(spl);
	}
}

/*
 *	vm_page_wire:
 *
 *	Mark this page as wired down by yet
 *	another map, removing it from paging queues
 *	as necessary.
 *
 *	The page queues must be locked.
 */
void
vm_page_wire(mem)
	register vm_page_t	mem;
{

	VM_PAGE_CHECK(mem);

	if (mem->wire_count == 0) {
		if (mem->flags & PG_ACTIVE) {
			TAILQ_REMOVE(&vm_page_queue_active, mem, pageq);
			cnt.v_active_count--;
			mem->flags &= ~PG_ACTIVE;
		}
		if (mem->flags & PG_INACTIVE) {
			TAILQ_REMOVE(&vm_page_queue_inactive, mem, pageq);
			cnt.v_inactive_count--;
			mem->flags &= ~PG_INACTIVE;
		}
		cnt.v_wire_count++;
	}
	mem->wire_count++;
}

/*
 *	vm_page_unwire:
 *
 *	Release one wiring of this page, potentially
 *	enabling it to be paged again.
 *
 *	The page queues must be locked.
 */
void
vm_page_unwire(mem)
	register vm_page_t	mem;
{

	VM_PAGE_CHECK(mem);

	mem->wire_count--;
	if (mem->wire_count == 0) {
		TAILQ_INSERT_TAIL(&vm_page_queue_active, mem, pageq);
		cnt.v_active_count++;
		mem->flags |= PG_ACTIVE;
		cnt.v_wire_count--;
	}
}

/*
 *	vm_page_deactivate:
 *
 *	Returns the given page to the inactive list,
 *	indicating that no physical maps have access
 *	to this page.  [Used by the physical mapping system.]
 *
 *	The page queues must be locked.
 */
void
vm_page_deactivate(m)
	register vm_page_t	m;
{

	VM_PAGE_CHECK(m);

	/*
	 *	Only move active pages -- ignore locked or already
	 *	inactive ones.
	 */

	if (m->flags & PG_ACTIVE) {
		TAILQ_REMOVE(&vm_page_queue_active, m, pageq);
		m->flags &= ~PG_ACTIVE;
		cnt.v_active_count--;
		goto deact;
	}
	if ((m->flags & PG_INACTIVE) == 0) {
	deact:
		TAILQ_INSERT_TAIL(&vm_page_queue_inactive, m, pageq);
		m->flags |= PG_INACTIVE;
		cnt.v_inactive_count++;
		pmap_clear_reference(VM_PAGE_TO_PHYS(m));
		if (pmap_is_modified(VM_PAGE_TO_PHYS(m)))
			m->flags &= ~PG_CLEAN;
		if (m->flags & PG_CLEAN)
			m->flags &= ~PG_LAUNDRY;
		else
			m->flags |= PG_LAUNDRY;
	}
}

/*
 *	vm_page_activate:
 *
 *	Put the specified page on the active list (if appropriate).
 *
 *	The page queues must be locked.
 */
void
vm_page_activate(m)
	register vm_page_t	m;
{

	VM_PAGE_CHECK(m);

	if (m->flags & PG_INACTIVE) {
		TAILQ_REMOVE(&vm_page_queue_inactive, m, pageq);
		m->flags &= ~PG_INACTIVE;
		cnt.v_inactive_count--;
	}
	if (m->wire_count == 0) {
		if (m->flags & PG_ACTIVE)
			panic("vm_page_activate: already active");

		TAILQ_INSERT_TAIL(&vm_page_queue_active, m, pageq);
		m->flags |= PG_ACTIVE;
		cnt.v_active_count++;
	}
}

/*
 *	vm_page_zero_fill:
 *
 *	Zero-fill the specified page.
 *	Written as a standard pagein routine, to
 *	be used by the zero-fill object.
 */
boolean_t
vm_page_zero_fill(m)
	vm_page_t	m;
{

	VM_PAGE_CHECK(m);

	m->flags &= ~PG_CLEAN;
	pmap_zero_page(VM_PAGE_TO_PHYS(m));
	return(TRUE);
}

/*
 *	vm_page_copy:
 *
 *	Copy one page to another
 */
void
vm_page_copy(src_m, dest_m)
	vm_page_t	src_m;
	vm_page_t	dest_m;
{

	VM_PAGE_CHECK(src_m);
	VM_PAGE_CHECK(dest_m);

	dest_m->flags &= ~PG_CLEAN;
	pmap_copy_page(VM_PAGE_TO_PHYS(src_m), VM_PAGE_TO_PHYS(dest_m));
}

#ifdef VM_PAGE_ALLOC_MEMORY_STATS
#define	STAT_INCR(v)	(v)++
#define	STAT_DECR(v)	do { \
		if ((v) == 0) \
			printf("%s:%d -- Already 0!\n", __FILE__, __LINE__); \
		else \
			(v)--; \
	} while (0)
u_long	vm_page_alloc_memory_npages;
#else
#define	STAT_INCR(v)
#define	STAT_DECR(v)
#endif

/*
 *	vm_page_alloc_memory:
 *
 *	Allocate physical pages conforming to the restrictions
 *	provided:
 *
 *		size		The size of the allocation,
 *				rounded to page size.
 *
 *		low		The low address of the allowed
 *				allocation range.
 *
 *		high		The high address of the allowed
 *				allocation range.
 *
 *		alignment	Allocation must be aligned to this
 *				power-of-two boundary.
 *
 *		boundary	No segment in the allocation may
 *				cross this power-of-two boundary
 *				(relative to zero).
 *
 *	The allocated pages are placed at the tail of `rlist'; `rlist'
 *	is assumed to be properly initialized by the caller.  The
 *	number of memory segments that the allocated memory may
 *	occupy is specified in the `nsegs' arguement.
 *
 *	Returns 0 on success or an errno value to indicate mode
 *	of failure.
 *
 *	XXX This implementation could be improved.  It only
 *	XXX allocates a single segment.
 */
int
vm_page_alloc_memory(size, low, high, alignment, boundary,
    rlist, nsegs, waitok)
	vsize_t size;
	vaddr_t low, high, alignment, boundary;
	struct pglist *rlist;
	int nsegs, waitok;
{
	paddr_t try, idxpa, lastidxpa;
	int psi;
	struct vm_page *vm_page_array;
	int s, tryidx, idx, end, error;
	vm_page_t m;
	u_long pagemask;
#ifdef DEBUG
	vm_page_t tp;
#endif

#ifdef DIAGNOSTIC
	if ((alignment & (alignment - 1)) != 0)
		panic("vm_page_alloc_memory: alignment must be power of 2");

	if ((boundary & (boundary - 1)) != 0)
		panic("vm_page_alloc_memory: boundary must be power of 2");
#endif

	/*
	 * Our allocations are always page granularity, so our alignment
	 * must be, too.
	 */
	if (alignment < PAGE_SIZE)
		alignment = PAGE_SIZE;

	size = round_page(size);
	try = roundup(low, alignment);

	if (boundary != 0 && boundary < size)
		return (EINVAL);

	pagemask = ~(boundary - 1);

	/* Default to "lose". */
	error = ENOMEM;

	/*
	 * Block all memory allocation and lock the free list.
	 */
	s = splimp();
	simple_lock(&vm_page_queue_free_lock);

	/* Are there even any free pages? */
	if (vm_page_queue_free.tqh_first == NULL)
		goto out;

	for (;; try += alignment) {
		if (try + size > high) {
			/*
			 * We've run past the allowable range.
			 */
			goto out;
		}

		/*
		 * Make sure this is a managed physical page.
		 */

		if ((psi = vm_physseg_find(atop(try), &idx)) == -1)
			continue; /* managed? */
		if (vm_physseg_find(atop(try + size), NULL) != psi)
			continue; /* end must be in this segment */

		tryidx = idx;
		end = idx + (size / PAGE_SIZE);
		vm_page_array = vm_physmem[psi].pgs;
		/* XXX: emulates old global vm_page_array */

		/*
		 * Found a suitable starting page.  See of the range
		 * is free.
		 */
		for (; idx < end; idx++) {
			if (VM_PAGE_IS_FREE(&vm_page_array[idx]) == 0) {
				/*
				 * Page not available.
				 */
				break;
			}

			idxpa = VM_PAGE_TO_PHYS(&vm_page_array[idx]);

			if (idx > tryidx) {
				lastidxpa =
				    VM_PAGE_TO_PHYS(&vm_page_array[idx - 1]);

				if ((lastidxpa + PAGE_SIZE) != idxpa) {
					/*
					 * Region not contiguous.
					 */
					break;
				}
				if (boundary != 0 &&
				    ((lastidxpa ^ idxpa) & pagemask) != 0) {
					/*
					 * Region crosses boundary.
					 */
					break;
				}
			}
		}

		if (idx == end) {
			/*
			 * Woo hoo!  Found one.
			 */
			break;
		}
	}

	/*
	 * Okay, we have a chunk of memory that conforms to
	 * the requested constraints.
	 */
	idx = tryidx;
	while (idx < end) {
		m = &vm_page_array[idx];
#ifdef DEBUG
		for (tp = vm_page_queue_free.tqh_first; tp != NULL;
		    tp = tp->pageq.tqe_next) {
			if (tp == m)
				break;
		}
		if (tp == NULL)
			panic("vm_page_alloc_memory: page not on freelist");
#endif
		TAILQ_REMOVE(&vm_page_queue_free, m, pageq);
		cnt.v_free_count--;
		m->flags = PG_CLEAN;
		m->object = NULL;
		m->wire_count = 0;
		TAILQ_INSERT_TAIL(rlist, m, pageq);
		idx++;
		STAT_INCR(vm_page_alloc_memory_npages);
	}
	error = 0;

 out:
	simple_unlock(&vm_page_queue_free_lock);
	splx(s);
	return (error);
}

/*
 *	vm_page_free_memory:
 *
 *	Free a list of pages previously allocated by vm_page_alloc_memory().
 *	The pages are assumed to have no mappings.
 */
void
vm_page_free_memory(list)
	struct pglist *list;
{
	vm_page_t m;
	int s;

	/*
	 * Block all memory allocation and lock the free list.
	 */
	s = splimp();
	simple_lock(&vm_page_queue_free_lock);

	while ((m = list->tqh_first) != NULL) {
		TAILQ_REMOVE(list, m, pageq);
		m->flags = PG_FREE;
		TAILQ_INSERT_TAIL(&vm_page_queue_free, m, pageq);
		cnt.v_free_count++;
		STAT_DECR(vm_page_alloc_memory_npages);
	}

	simple_unlock(&vm_page_queue_free_lock);
	splx(s);
}
