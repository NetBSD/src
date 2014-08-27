/*	$NetBSD: linux_kmap.c,v 1.7 2014/08/27 16:09:16 riastradh Exp $	*/

/*-
 * Copyright (c) 2013 The NetBSD Foundation, Inc.
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: linux_kmap.c,v 1.7 2014/08/27 16:09:16 riastradh Exp $");

#include <sys/types.h>
#include <sys/kmem.h>
#include <sys/mutex.h>
#include <sys/rbtree.h>

#include <uvm/uvm_extern.h>

#include <linux/highmem.h>

/*
 * XXX Kludgerific implementation of Linux kmap_atomic, which is
 * required not to fail.  To accomodate this, we reserve one page of
 * kva at boot (or load) and limit the system to at most kmap_atomic in
 * use at a time.
 */

/*
 * XXX Use direct-mapped physical pages where available, e.g. amd64.
 *
 * XXX ...or add an abstraction to uvm for this.  (uvm_emap?)
 */

static kmutex_t linux_kmap_atomic_lock;
static vaddr_t linux_kmap_atomic_vaddr;

static kmutex_t linux_kmap_lock;
static rb_tree_t linux_kmap_entries;

struct linux_kmap_entry {
	paddr_t		lke_paddr;
	vaddr_t		lke_vaddr;
	unsigned int	lke_refcnt;
	rb_node_t	lke_node;
};

static int
lke_compare_nodes(void *ctx __unused, const void *an, const void *bn)
{
	const struct linux_kmap_entry *const a = an;
	const struct linux_kmap_entry *const b = bn;

	if (a->lke_paddr < b->lke_paddr)
		return -1;
	else if (a->lke_paddr > b->lke_paddr)
		return +1;
	else
		return 0;
}

static int
lke_compare_key(void *ctx __unused, const void *node, const void *key)
{
	const struct linux_kmap_entry *const lke = node;
	const paddr_t *const paddrp = key;

	if (lke->lke_paddr < *paddrp)
		return -1;
	else if (lke->lke_paddr > *paddrp)
		return +1;
	else
		return 0;
}

static const rb_tree_ops_t linux_kmap_entry_ops = {
	.rbto_compare_nodes = &lke_compare_nodes,
	.rbto_compare_key = &lke_compare_key,
	.rbto_node_offset = offsetof(struct linux_kmap_entry, lke_node),
	.rbto_context = NULL,
};

int
linux_kmap_init(void)
{

	/* IPL_VM since interrupt handlers use kmap_atomic.  */
	mutex_init(&linux_kmap_atomic_lock, MUTEX_DEFAULT, IPL_VM);

	linux_kmap_atomic_vaddr = uvm_km_alloc(kernel_map, PAGE_SIZE, 0,
	    (UVM_KMF_VAONLY | UVM_KMF_WAITVA));

	KASSERT(linux_kmap_atomic_vaddr != 0);
	KASSERT(!pmap_extract(pmap_kernel(), linux_kmap_atomic_vaddr, NULL));

	mutex_init(&linux_kmap_lock, MUTEX_DEFAULT, IPL_NONE);
	rb_tree_init(&linux_kmap_entries, &linux_kmap_entry_ops);

	return 0;
}

void
linux_kmap_fini(void)
{

	KASSERT(RB_TREE_MIN(&linux_kmap_entries) == NULL);
#if 0				/* XXX no rb_tree_destroy */
	rb_tree_destroy(&linux_kmap_entries);
#endif
	mutex_destroy(&linux_kmap_lock);

	KASSERT(linux_kmap_atomic_vaddr != 0);
	KASSERT(!pmap_extract(pmap_kernel(), linux_kmap_atomic_vaddr, NULL));

	uvm_km_free(kernel_map, linux_kmap_atomic_vaddr, PAGE_SIZE,
	    (UVM_KMF_VAONLY | UVM_KMF_WAITVA));

	mutex_destroy(&linux_kmap_atomic_lock);
}

void *
kmap_atomic(struct page *page)
{
	const paddr_t paddr = uvm_vm_page_to_phys(&page->p_vmp);

	mutex_spin_enter(&linux_kmap_atomic_lock);

	KASSERT(linux_kmap_atomic_vaddr != 0);
	KASSERT(!pmap_extract(pmap_kernel(), linux_kmap_atomic_vaddr, NULL));

	const vaddr_t vaddr = linux_kmap_atomic_vaddr;
	const int prot = (VM_PROT_READ | VM_PROT_WRITE);
	const int flags = 0;
	pmap_kenter_pa(vaddr, paddr, prot, flags);
	pmap_update(pmap_kernel());

	return (void *)vaddr;
}

void
kunmap_atomic(void *addr)
{
	const vaddr_t vaddr = (vaddr_t)addr;

	KASSERT(mutex_owned(&linux_kmap_atomic_lock));
	KASSERT(linux_kmap_atomic_vaddr == vaddr);
	KASSERT(pmap_extract(pmap_kernel(), vaddr, NULL));

	pmap_kremove(vaddr, PAGE_SIZE);
	pmap_update(pmap_kernel());

	mutex_spin_exit(&linux_kmap_atomic_lock);
}

void *
kmap(struct page *page)
{
	const paddr_t paddr = VM_PAGE_TO_PHYS(&page->p_vmp);

	ASSERT_SLEEPABLE();

	const vaddr_t vaddr = uvm_km_alloc(kernel_map, PAGE_SIZE, 0,
	    (UVM_KMF_VAONLY | UVM_KMF_WAITVA));
	KASSERT(vaddr != 0);

	struct linux_kmap_entry *const lke = kmem_alloc(sizeof(*lke),
	    KM_SLEEP);
	lke->lke_paddr = paddr;
	lke->lke_vaddr = vaddr;

	mutex_enter(&linux_kmap_lock);
	struct linux_kmap_entry *const collision __unused =
	    rb_tree_insert_node(&linux_kmap_entries, lke);
	KASSERT(collision == lke);
	mutex_exit(&linux_kmap_lock);

	KASSERT(!pmap_extract(pmap_kernel(), vaddr, NULL));
	const int prot = (VM_PROT_READ | VM_PROT_WRITE);
	const int flags = 0;
	pmap_kenter_pa(vaddr, paddr, prot, flags);
	pmap_update(pmap_kernel());

	return (void *)vaddr;
}

void
kunmap(struct page *page)
{
	const paddr_t paddr = VM_PAGE_TO_PHYS(&page->p_vmp);

	ASSERT_SLEEPABLE();

	mutex_enter(&linux_kmap_lock);
	struct linux_kmap_entry *const lke =
	    rb_tree_find_node(&linux_kmap_entries, &paddr);
	KASSERT(lke != NULL);
	rb_tree_remove_node(&linux_kmap_entries, lke);
	mutex_exit(&linux_kmap_lock);

	const vaddr_t vaddr = lke->lke_vaddr;
	kmem_free(lke, sizeof(*lke));

	KASSERT(pmap_extract(pmap_kernel(), vaddr, NULL));

	pmap_kremove(vaddr, PAGE_SIZE);
	pmap_update(pmap_kernel());

	uvm_km_free(kernel_map, vaddr, PAGE_SIZE, UVM_KMF_VAONLY);
}
