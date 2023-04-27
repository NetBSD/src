/*	$NetBSD: pmap_devmap.c,v 1.2 2023/04/27 06:23:31 skrll Exp $	*/

/*-
 * Copyright (c) 2022 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Nick Hudson
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
__RCSID("$NetBSD: pmap_devmap.c,v 1.2 2023/04/27 06:23:31 skrll Exp $");


#include <sys/param.h>
#include <sys/types.h>

#include <uvm/uvm.h>
#include <uvm/uvm_extern.h>
#include <uvm/pmap/pmap_devmap.h>


static const struct pmap_devmap *pd_table;
static vaddr_t pd_root;

bool pmap_devmap_bootstrap_done = false;

bool
pmap_devmap_bootstrapped_p(void)
{
	return pmap_devmap_bootstrap_done;
}


vaddr_t
pmap_devmap_root(void)
{
	return pd_root;
}

/*
 * Register the devmap table.  This is provided in case early console
 * initialization needs to register mappings created by bootstrap code
 * before pmap_devmap_bootstrap() is called.
 */
void
pmap_devmap_register(const struct pmap_devmap *table)
{

	pd_table = table;
}

/*
 * Map all of the static regions in the devmap table, and remember
 * the devmap table so other parts of the kernel can look up entries
 * later.
 */
void
pmap_devmap_bootstrap(vaddr_t root, const struct pmap_devmap *table)
{
	UVMHIST_FUNC(__func__);
	UVMHIST_CALLARGS(maphist, "(root=%#jx, table=%p)", root,
	    (uintptr_t)table, 0, 0);

	bool done = false;

	pd_root = root;
	pd_table = table;

	for (size_t i = 0; table[i].pd_size != 0; i++) {
		const struct pmap_devmap * const pdp = &table[i];
		const vaddr_t vmax = __type_max_u(vaddr_t);
		const paddr_t pmax = __type_max_u(paddr_t);

		KASSERT(pdp->pd_size != 0);
		KASSERTMSG(vmax - pdp->pd_va >= pdp->pd_size - 1,
		    "va %" PRIxVADDR " sz %" PRIxPSIZE, pdp->pd_va,
		    pdp->pd_size);
		KASSERTMSG(pmax - pdp->pd_pa >= pdp->pd_size - 1,
		    "pa %" PRIxPADDR " sz %" PRIxPSIZE, pdp->pd_pa,
		    pdp->pd_size);
		UVMHIST_LOG(maphist,
		    "pa %#jx -> %#jx @ va %#jx -> %#jx",
		    pdp->pd_pa, pdp->pd_pa + pdp->pd_size - 1,
		    pdp->pd_va, pdp->pd_va + pdp->pd_size - 1);

		pmap_kenter_range(pdp->pd_va, pdp->pd_pa, pdp->pd_size,
		    pdp->pd_prot, pdp->pd_flags);
		done = true;
	}
	if (done)
		pmap_devmap_bootstrap_done = true;
}

/*
 * Find the table entry that fully covers the physical address
 * range [pa, pa + size)
 */
const struct pmap_devmap *
pmap_devmap_find_pa(paddr_t pa, psize_t size)
{
	if (pd_table == NULL)
		return NULL;

	for (size_t i = 0; pd_table[i].pd_size != 0; i++) {
		const paddr_t pd_pa = pd_table[i].pd_pa;
		const psize_t pd_sz = pd_table[i].pd_size;

		if (pa < pd_pa)
			continue;

		const psize_t pdiff = pa - pd_pa;

		if (pd_sz < pdiff)
			continue;

		if (pd_sz - pdiff >= size)
			return &pd_table[i];
	}

	return NULL;
}

/*
 * Find the table entry that fully covers the virtual address
 * range [va, va + size)
 */
const struct pmap_devmap *
pmap_devmap_find_va(vaddr_t va, vsize_t size)
{
	if (pd_table == NULL)
		return NULL;

	for (size_t i = 0; pd_table[i].pd_size != 0; i++) {
		const vaddr_t pd_va = pd_table[i].pd_va;
		const vsize_t pd_sz = pd_table[i].pd_size;

		if (va < pd_va)
			continue;

		const vsize_t vdiff = va - pd_va;

		if (pd_sz < vdiff)
			continue;

		if (pd_sz - vdiff >= size)
			return &pd_table[i];

	}

	return NULL;
}
