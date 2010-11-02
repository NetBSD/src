/*	$NetBSD: uvm_kmguard.c,v 1.4 2010/11/02 20:49:48 skrll Exp $	*/

/*-
 * Copyright (c) 2009 The NetBSD Foundation, Inc.
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
 * A simple memory allocator for debugging.  It tries to catch:
 *
 * - Overflow, in realtime
 * - Underflow, at free
 * - Invalid pointer/size passed, at free
 * - Use-after-free
 */
#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: uvm_kmguard.c,v 1.4 2010/11/02 20:49:48 skrll Exp $");

#include <sys/param.h>
#include <sys/malloc.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/pool.h>
#include <sys/atomic.h>

#include <uvm/uvm.h>
#include <uvm/uvm_kmguard.h>

#define	CANARY(va, size)	((void *)((va) ^ 0x9deeba9 ^ (size)))
#define	MAXSIZE			(PAGE_SIZE - sizeof(void *))

void
uvm_kmguard_init(struct uvm_kmguard *kg, u_int *depth, size_t *size,
		 struct vm_map *map)
{
	vaddr_t va;

	/*
	 * if not enabled, we have nothing to do.
	 */

	if (*depth == 0) {
		return;
	}
	*depth = roundup((*depth), PAGE_SIZE / sizeof(void *));
	KASSERT(*depth != 0);

	/*	
	 * allocate fifo.
	 */

	va = uvm_km_alloc(kernel_map, *depth * sizeof(void *), PAGE_SIZE,
	    UVM_KMF_WIRED | UVM_KMF_ZERO);
	if (va == 0) {
		*depth = 0;
		*size = 0;
	} else {
		*size = MAXSIZE;
	}

	/*
	 * init object.
	 */

	kg->kg_map = map;
	kg->kg_fifo = (void *)va;
	kg->kg_depth = *depth;
	kg->kg_rotor = 0;

	printf("uvm_kmguard(%p): depth %d\n", kg, *depth);
}

void *
uvm_kmguard_alloc(struct uvm_kmguard *kg, size_t len, bool waitok)
{
	struct vm_page *pg;
	void **p;
	vaddr_t va;
	int flag;

	/*
	 * can't handle >PAGE_SIZE allocations.  let the caller handle it
	 * normally.
	 */

	if (len > MAXSIZE) {
		return NULL;
	}

	/*
	 * allocate two pages of kernel VA, but do not map anything in yet.
	 */

	if (waitok) {
		flag = UVM_KMF_WAITVA;
	} else {
		flag = UVM_KMF_TRYLOCK | UVM_KMF_NOWAIT;
	}
	va = vm_map_min(kg->kg_map);
	if (__predict_false(uvm_map(kg->kg_map, &va, PAGE_SIZE*2, NULL,
	    UVM_UNKNOWN_OFFSET, PAGE_SIZE, UVM_MAPFLAG(UVM_PROT_ALL,
	    UVM_PROT_ALL, UVM_INH_NONE, UVM_ADV_RANDOM, flag
	    | UVM_FLAG_QUANTUM)) != 0)) {
		return NULL;
	}

	/*
	 * allocate a single page and map in at the start of the two page
	 * block.
	 */	

	for (;;) {
		pg = uvm_pagealloc(NULL, va - vm_map_min(kg->kg_map), NULL, 0);
		if (__predict_true(pg != NULL)) {
			break;
		}
		if (waitok) {
			uvm_wait("kmguard");	/* sleep here */
			continue;
		} else {
			uvm_km_free(kg->kg_map, va, PAGE_SIZE*2,
			    UVM_KMF_VAONLY);
			return NULL;
		}
	}
	pg->flags &= ~PG_BUSY;	/* new page */
	UVM_PAGE_OWN(pg, NULL);
	pmap_kenter_pa(va, VM_PAGE_TO_PHYS(pg),
	    VM_PROT_READ | VM_PROT_WRITE, PMAP_KMPAGE);
       	pmap_update(pmap_kernel());

	/*
	 * offset the returned pointer so that the unmapped guard page
	 * sits immediately after the returned object.
	 */

	p = (void **)((va + PAGE_SIZE - len) & ~(uintptr_t)ALIGNBYTES);
	p[-1] = CANARY(va, len);
	return (void *)p; 
}

bool
uvm_kmguard_free(struct uvm_kmguard *kg, size_t len, void *p)
{
	vaddr_t va;
	u_int rotor;
	void **c;

	if (len > MAXSIZE) {
		return false;
	}

	/*
	 * first, check that everything is as it should be.
	 */

	va = trunc_page((vaddr_t)p);
	c = (void **)((va + PAGE_SIZE - len) & ~(uintptr_t)ALIGNBYTES);
	KASSERT(p == (void *)c);
	KASSERT(c[-1] == CANARY(va, len));
	KASSERT(pmap_extract(pmap_kernel(), va, NULL));
	KASSERT(!pmap_extract(pmap_kernel(), va + PAGE_SIZE, NULL));

	/*
	 * unmap and free the first page.  the second page is never
	 * allocated .
	 */

	uvm_km_pgremove_intrsafe(kg->kg_map, va, va + PAGE_SIZE * 2);
	pmap_kremove(va, PAGE_SIZE * 2);
	pmap_update(pmap_kernel());

	/*
	 * put the VA allocation into the list and swap an old one
	 * out to free.  this behaves mostly like a fifo.
	 */

	rotor = atomic_inc_uint_nv(&kg->kg_rotor) % kg->kg_depth;
	va = (vaddr_t)atomic_swap_ptr(&kg->kg_fifo[rotor], (void *)va);
	if (va != 0) {
		uvm_km_free(kg->kg_map, va, PAGE_SIZE*2, UVM_KMF_VAONLY);
	}

	return true;
}
