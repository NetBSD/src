/* $NetBSD: pmap.c,v 1.1 2014/08/10 05:47:37 matt Exp $ */

/*-
 * Copyright (c) 2014 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Matt Thomas of 3am Software Foundry.
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

__KERNEL_RCSID(1, "$NetBSD: pmap.c,v 1.1 2014/08/10 05:47:37 matt Exp $");

#include <sys/param.h>
#include <sys/types.h>
#include <sys/kmem.h>
#include <sys/vmem.h>

#include <uvm/uvm.h>

static struct pmap kernel_pmap;

struct pmap * const kernel_pmap_ptr = &kernel_pmap;

vmem_t *pmap_asid_arena;

void
pmap_init(void)
{
	pmap_asid_arena = vmem_create("asid", 0, 65536, 1, NULL, NULL,
	    NULL, 2, VM_SLEEP, IPL_VM);
}

void
pmap_virtual_space(vaddr_t *vstartp, vaddr_t *vendp)
{
}

vaddr_t
pmap_steal_memory(vsize_t size, vaddr_t *vstartp, vaddr_t *vendp)
{
	return 0;
}

struct pmap *
pmap_create(void)
{
	return NULL;
}

void
pmap_destroy(struct pmap *pm)
{
}

void
pmap_reference(struct pmap *pm)
{
}

long
pmap_resident_count(struct pmap *pm)
{
	return pm->pm_stats.resident_count;
}

long
pmap_wired_count(struct pmap *pm)
{
	return pm->pm_stats.wired_count;
}

vaddr_t
pmap_growkernel(vaddr_t maxkvaddr)
{
	return maxkvaddr;
}

int
pmap_enter(struct pmap *pm, vaddr_t va, paddr_t pa, vm_prot_t prot, u_int flags)
{
	return 0;
}

void
pmap_remove(struct pmap *pm, vaddr_t sva, vaddr_t eva)
{

}

void
pmap_remove_all(struct pmap *pm)
{
}

void
pmap_protect(struct pmap *pm, vaddr_t sva, vaddr_t eva, vm_prot_t prot)
{
}

void
pmap_unwire(struct pmap *pm, vaddr_t va)
{
}

bool
pmap_extract(struct pmap *pm, vaddr_t va, paddr_t *pap)
{
	return false;
}

void
pmap_kenter_pa(vaddr_t va, paddr_t pa, vm_prot_t prot, u_int flags)
{
}

void
pmap_kremove(vaddr_t va, vsize_t size)
{
}

void
pmap_copy(struct pmap *dst_map, struct pmap *src_map, vaddr_t dst_addr,
    vsize_t len, vaddr_t src_addr)
{
}

void
pmap_update(struct pmap *pm)
{
}

void
pmap_activate(struct lwp *l)
{
}

void
pmap_deactivate(struct lwp *l)
{
}

void
pmap_page_protect(struct vm_page *pg, vm_prot_t prot)
{
}

bool
pmap_clear_modify(struct vm_page *pg)
{
	struct vm_page_md * const mdpg = VM_PAGE_TO_MD(pg);
	const bool rv = (mdpg->mdpg_attrs & VM_PAGE_MD_MODIFIED) != 0;
	mdpg->mdpg_attrs &= ~VM_PAGE_MD_MODIFIED;
	return rv;
}

bool
pmap_clear_reference(struct vm_page *pg)
{
	struct vm_page_md * const mdpg = VM_PAGE_TO_MD(pg);
	const bool rv = (mdpg->mdpg_attrs & VM_PAGE_MD_REFERENCED) != 0;
	mdpg->mdpg_attrs &= ~VM_PAGE_MD_REFERENCED;
	return rv;
}

bool
pmap_is_modified(struct vm_page *pg)
{
	struct vm_page_md * const mdpg = VM_PAGE_TO_MD(pg);
	return (mdpg->mdpg_attrs & VM_PAGE_MD_MODIFIED) != 0;
}

bool
pmap_is_referenced(struct vm_page *pg)
{
	struct vm_page_md * const mdpg = VM_PAGE_TO_MD(pg);
	return (mdpg->mdpg_attrs & VM_PAGE_MD_REFERENCED) != 0;
}

paddr_t
pmap_phys_address(paddr_t cookie)
{
	return cookie;
}
