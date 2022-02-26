/*	$NetBSD: vmalloc.h,v 1.12 2022/02/26 15:57:22 rillig Exp $	*/

/*-
 * Copyright (c) 2013, 2018 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Taylor R. Campbell.
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

#ifndef _LINUX_VMALLOC_H_
#define _LINUX_VMALLOC_H_

#include <uvm/uvm_extern.h>

#include <linux/mm.h>
#include <linux/mm_types.h>
#include <linux/overflow.h>
#include <linux/slab.h>

#include <asm/page.h>

struct notifier_block;

/*
 * XXX vmalloc and kvmalloc both use kmalloc.  If you change that, be
 * sure to update this so kvfree in <linux/mm.h> still works on vmalloc
 * addresses.
 */

static inline bool
is_vmalloc_addr(void *addr)
{
	return true;
}

static inline void *
vmalloc(unsigned long size)
{
	return kmalloc(size, GFP_KERNEL);
}

static inline void *
vmalloc_user(unsigned long size)
{
	return kzalloc(size, GFP_KERNEL);
}

static inline void *
vzalloc(unsigned long size)
{
	return kzalloc(size, GFP_KERNEL);
}

static inline void
vfree(void *ptr)
{
	kfree(ptr);
}

#define	PAGE_KERNEL	UVM_PROT_RW

/*
 * vmap(pages, npages, flags, prot)
 *
 *	Map pages[0], pages[1], ..., pages[npages-1] into contiguous
 *	kernel virtual address space with the specified protection, and
 *	return a KVA pointer to the start.
 *
 *	prot may be a bitwise ior of UVM_PROT_READ/WRITE/EXEC and
 *	PMAP_* cache flags accepted by pmap_enter().
 */
static inline void *
vmap(struct page **pages, unsigned npages, unsigned long flags,
    pgprot_t protflags)
{
	vm_prot_t justprot = protflags & UVM_PROT_ALL;
	vaddr_t va;
	unsigned i;

	/* Allocate some KVA, or return NULL if we can't.  */
	va = uvm_km_alloc(kernel_map, (vsize_t)npages << PAGE_SHIFT, PAGE_SIZE,
	    UVM_KMF_VAONLY|UVM_KMF_NOWAIT);
	if (va == 0)
		return NULL;

	/* Ask pmap to map the KVA to the specified page addresses.  */
	for (i = 0; i < npages; i++) {
		pmap_kenter_pa(va + i*PAGE_SIZE, page_to_phys(pages[i]),
		    justprot, protflags);
	}

	/* Commit the pmap updates.  */
	pmap_update(pmap_kernel());

	return (void *)va;
}

/*
 * vunmap(ptr, npages)
 *
 *	Unmap the KVA pages starting at ptr that were mapped by a call
 *	to vmap with the same npages parameter.
 */
static inline void
vunmap(void *ptr, unsigned npages)
{
	vaddr_t va = (vaddr_t)ptr;

	/* Ask pmap to unmap the KVA.  */
	pmap_kremove(va, (vsize_t)npages << PAGE_SHIFT);

	/* Commit the pmap updates.  */
	pmap_update(pmap_kernel());

	/*
	 * Now that the pmap is no longer mapping the KVA we allocated
	 * on any CPU, it is safe to free the KVA.
	 */
	uvm_km_free(kernel_map, va, (vsize_t)npages << PAGE_SHIFT,
	    UVM_KMF_VAONLY);
}

static inline int
register_vmap_purge_notifier(struct notifier_block *nb __unused)
{
	return 0;
}

static inline int
unregister_vmap_purge_notifier(struct notifier_block *nb __unused)
{
	return 0;
}

#endif  /* _LINUX_VMALLOC_H_ */
