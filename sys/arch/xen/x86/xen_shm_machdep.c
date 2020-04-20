/*      $NetBSD: xen_shm_machdep.c,v 1.13.10.1 2020/04/20 11:29:01 bouyer Exp $      */

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
__KERNEL_RCSID(0, "$NetBSD: xen_shm_machdep.c,v 1.13.10.1 2020/04/20 11:29:01 bouyer Exp $");

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
#include <xen/xen_shm.h>

/*
 * Helper routines for the backend drivers. This implements the necessary
 * functions to map a bunch of pages from foreign domains into our kernel VM
 * space, do I/O to it, and unmap it.
 *
 * At boot time, we grab some kernel VM space that we'll use to map the foreign
 * pages. We also maintain a virtual-to-machine mapping table to give back
 * the appropriate address to bus_dma if requested.
 *
 * If no more VM space is available, we return an error. The caller can then
 * register a callback which will be called when the required VM space is
 * available.
 */

int
xen_shm_map(int nentries, int domid, grant_ref_t *grefp, vaddr_t va,
    grant_handle_t *handlep, int flags)
{
	gnttab_map_grant_ref_t op[XENSHM_MAX_PAGES_PER_REQUEST];
	int ret, i;

#ifdef DIAGNOSTIC
	if (nentries > XENSHM_MAX_PAGES_PER_REQUEST) {
		panic("xen_shm_map: %d entries", nentries);
	}
#endif

	for (i = 0; i < nentries; i++) {
		op[i].host_addr = va + i * PAGE_SIZE;
		op[i].dom = domid;
		op[i].ref = grefp[i];
		op[i].flags = GNTMAP_host_map |
		    ((flags & XSHM_RO) ? GNTMAP_readonly : 0);
	}

	ret = HYPERVISOR_grant_table_op(GNTTABOP_map_grant_ref, op, nentries);
	if (__predict_false(ret)) {
		printf("%s: HYPERVISOR_grant_table_op failed\n", __func__);
		return EINVAL;
	}

	for (i = 0; i < nentries; i++) {
#ifdef DIAGNOSTIC
		if (__predict_false(op[i].status))
			panic("%s: op[%d] status %d", __func__, i,
			    op[i].status);
#endif
		handlep[i] = op[i].handle;
	}

	return 0;
}

void
xen_shm_unmap(vaddr_t va, int nentries, grant_handle_t *handlep)
{
	gnttab_unmap_grant_ref_t op[XENSHM_MAX_PAGES_PER_REQUEST];
	int ret, i;

#ifdef DIAGNOSTIC
	if (nentries > XENSHM_MAX_PAGES_PER_REQUEST) {
		panic("xen_shm_unmap: %d entries", nentries);
	}
#endif

	for (i = 0; i < nentries; i++) {
		op[i].host_addr = va + i * PAGE_SIZE;
		op[i].dev_bus_addr = 0;
		op[i].handle = handlep[i];
	}

	ret = HYPERVISOR_grant_table_op(GNTTABOP_unmap_grant_ref,
	    op, nentries);
	if (__predict_false(ret)) {
		panic("xen_shm_unmap: unmap failed");
	}
}
