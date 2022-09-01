/*      $NetBSD: xen_shm_machdep.c,v 1.18 2022/09/01 12:29:00 bouyer Exp $      */

/*
 * Copyright (c) 2006 Manuel Bouyer.
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
__KERNEL_RCSID(0, "$NetBSD: xen_shm_machdep.c,v 1.18 2022/09/01 12:29:00 bouyer Exp $");

#include "opt_xen.h"

#include <sys/types.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/queue.h>
#include <sys/vmem.h>
#include <sys/kernel.h>
#include <uvm/uvm.h>

#include <machine/pmap.h>
#include <xen/hypervisor.h>
#include <xen/xen.h>
#include <xen/evtchn.h>
#include <xen/xenmem.h>
#include <xen/xen_shm.h>

/*
 * Helper routines for the backend drivers. This implements the necessary
 * functions to map a bunch of pages from foreign domains into our kernel VM
 * space, do I/O to it, and unmap it.
 */

/*
 * Map the memory referenced via grefp to supplied VA space.
 * If there is a failure for particular gref, no memory is mapped
 * and error is returned.
 */
int
xen_shm_map(int nentries, int domid, grant_ref_t *grefp, vaddr_t va,
    grant_handle_t *handlep, int flags)
{
	gnttab_map_grant_ref_t op[XENSHM_MAX_PAGES_PER_REQUEST];
	int ret, i;
#ifndef XENPV
	paddr_t base_paddr;
#endif
	

#ifdef DIAGNOSTIC
	if (nentries > XENSHM_MAX_PAGES_PER_REQUEST) {
		panic("xen_shm_map: %d entries", nentries);
	}
#endif
#ifndef XENPV
	base_paddr = xenmem_alloc_pa(nentries * PAGE_SIZE, PAGE_SIZE, false);
	if (base_paddr == 0)
		return ENOMEM;
#endif

	for (i = 0; i < nentries; i++) {
#ifndef XENPV
		op[i].host_addr = base_paddr + i * PAGE_SIZE;
#else
		op[i].host_addr = va + i * PAGE_SIZE;
#endif
		op[i].dom = domid;
		op[i].ref = grefp[i];
		op[i].flags = GNTMAP_host_map |
		    ((flags & XSHM_RO) ? GNTMAP_readonly : 0);
	}

	ret = HYPERVISOR_grant_table_op(GNTTABOP_map_grant_ref, op, nentries);
	if (__predict_false(ret < 0)) {
#ifdef DIAGNOSTIC
		printf("%s: HYPERVISOR_grant_table_op failed %d\n", __func__,
		    ret);
#endif
		ret = EINVAL;
		goto err1;
	}

	/*
	 * If ret is positive, it means there was an error in processing,
	 * and only first ret entries were actually handled. If it's zero,
	 * it only means all entries were processed, but there could still
	 * be failure.
	 */
	if (__predict_false(ret > 0 && ret < nentries)) {
		nentries = ret;
	}

	for (i = 0; i < nentries; i++) {
		if (__predict_false(op[i].status)) {
#ifdef DIAGNOSTIC
			printf("%s: op[%d] bad status %d gref %u\n", __func__,
			    i, op[i].status, grefp[i]);
#endif
			ret = 1;
			continue;
		}
		handlep[i] = op[i].handle;
	}

	if (__predict_false(ret > 0)) {
		int uncnt = 0;
		gnttab_unmap_grant_ref_t unop[XENSHM_MAX_PAGES_PER_REQUEST];

		/*
		 * When returning error, make sure the successfully mapped
		 * entries are unmapped before returning the error.
		 * xen_shm_unmap() can't be used, it assumes
		 * linear consecutive space.
		 */
		for (i = uncnt = 0; i < nentries; i++) {
			if (op[i].status == 0) {
#ifndef XENPV
				unop[uncnt].host_addr =
				    base_paddr + i * PAGE_SIZE;
#else
				unop[uncnt].host_addr = va + i * PAGE_SIZE;
#endif
				unop[uncnt].dev_bus_addr = 0;
				unop[uncnt].handle = handlep[i];
				uncnt++;
			}
		}
		if (uncnt > 0) {
			ret = HYPERVISOR_grant_table_op(
			    GNTTABOP_unmap_grant_ref, unop, uncnt);
			if (ret != 0) {
				panic("%s: unmap on error recovery failed"
				    " %d", __func__, ret);
			}
		}
#ifdef DIAGNOSTIC
		printf("%s: HYPERVISOR_grant_table_op bad entry\n",
		    __func__);
#endif
		ret =  EINVAL;
		goto err1;
	}
#ifndef XENPV
	for (i = 0; i < nentries; i++) {
		pmap_kenter_pa(va + i * PAGE_SIZE,
		    base_paddr + i * PAGE_SIZE,
		    VM_PROT_READ | VM_PROT_WRITE, 0);
	}
#endif

	return 0;
err1:
#ifndef XENPV
	xenmem_free_pa(base_paddr, nentries * PAGE_SIZE);
#endif
	return ret;
}

void
xen_shm_unmap(vaddr_t va, int nentries, grant_handle_t *handlep)
{
	gnttab_unmap_grant_ref_t op[XENSHM_MAX_PAGES_PER_REQUEST];
	int ret, i;
#ifndef XENPV
	paddr_t base_paddr;
	if (pmap_extract(pmap_kernel(), va, &base_paddr) != true)
		panic("xen_shm_unmap: unmapped va");
#endif

#ifdef DIAGNOSTIC
	if (nentries > XENSHM_MAX_PAGES_PER_REQUEST) {
		panic("xen_shm_unmap: %d entries", nentries);
	}
#endif

	for (i = 0; i < nentries; i++) {
#ifndef XENPV
		pmap_kremove(va + i * PAGE_SIZE, PAGE_SIZE);
		op[i].host_addr = base_paddr + i * PAGE_SIZE;
#else
		op[i].host_addr = va + i * PAGE_SIZE;
#endif
		op[i].dev_bus_addr = 0;
		op[i].handle = handlep[i];
	}

	ret = HYPERVISOR_grant_table_op(GNTTABOP_unmap_grant_ref,
	    op, nentries);
	if (__predict_false(ret)) {
		panic("xen_shm_unmap: unmap failed");
	}
#ifndef XENPV
	xenmem_free_pa(base_paddr, PAGE_SIZE * nentries);
#endif
}
