/*      $NetBSD: xen_shm_machdep.c,v 1.12 2018/07/27 09:22:40 maxv Exp $      */

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
__KERNEL_RCSID(0, "$NetBSD: xen_shm_machdep.c,v 1.12 2018/07/27 09:22:40 maxv Exp $");

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

/* Grab enough VM space to map an entire vbd ring. */
/* Xen3 linux guests seems to eat more pages, gives enough for 10 vbd rings */
#define BLKIF_RING_SIZE __RING_SIZE((blkif_sring_t *)0, PAGE_SIZE)
#define XENSHM_NPAGES (BLKIF_RING_SIZE * (BLKIF_MAX_SEGMENTS_PER_REQUEST + 1) * 10)

/* vm space management */
static vmem_t *xen_shm_arena __read_mostly;

/* callbacks are registered in a FIFO list. */
static SIMPLEQ_HEAD(xen_shm_callback_head, xen_shm_callback_entry)
    xen_shm_callbacks;

struct xen_shm_callback_entry {
	SIMPLEQ_ENTRY(xen_shm_callback_entry) xshmc_entries;
	int (*xshmc_callback)(void *); /* our callback */
	void *xshmc_arg; /* cookie passed to the callback */
};

/* a pool of struct xen_shm_callback_entry */
static struct pool xen_shm_callback_pool;

#ifdef DEBUG
/* for ratecheck(9) */
static struct timeval xen_shm_errintvl = { 60, 0 };  /* a minute, each */
#endif

void
xen_shm_init(void)
{
	vaddr_t xen_shm_base_address;
	vaddr_t xen_shm_end_address;
	u_long xen_shm_base_address_pg;
	vsize_t xen_shm_size;

	SIMPLEQ_INIT(&xen_shm_callbacks);
	pool_init(&xen_shm_callback_pool, sizeof(struct xen_shm_callback_entry),
	    0, 0, 0, "xshmc", NULL, IPL_VM);
	/* ensure we'll always get items */
	if (pool_prime(&xen_shm_callback_pool,
	    PAGE_SIZE / sizeof(struct xen_shm_callback_entry)) != 0) {
		panic("xen_shm_init can't prime pool");
	}

	xen_shm_size = (XENSHM_NPAGES * PAGE_SIZE);

	xen_shm_base_address = uvm_km_alloc(kernel_map, xen_shm_size, 0,
	    UVM_KMF_VAONLY);
	xen_shm_end_address = xen_shm_base_address + xen_shm_size;
	xen_shm_base_address_pg = xen_shm_base_address >> PAGE_SHIFT;
	if (xen_shm_base_address == 0) {
		panic("xen_shm_init no VM space");
	}
	xen_shm_arena = vmem_create("xen_shm", xen_shm_base_address_pg,
	    (xen_shm_end_address >> PAGE_SHIFT) - 1 - xen_shm_base_address_pg,
	    1, NULL, NULL, NULL, 1, VM_NOSLEEP, IPL_VM);
	if (xen_shm_arena == NULL) {
		panic("xen_shm_init no arena");
	}
}

int
xen_shm_map(int nentries, int domid, grant_ref_t *grefp, vaddr_t *vap,
    grant_handle_t *handlep, int flags)
{
	gnttab_map_grant_ref_t op[XENSHM_MAX_PAGES_PER_REQUEST];
	vmem_addr_t new_va_pg;
	vaddr_t new_va;
	int ret, i, s;

#ifdef DIAGNOSTIC
	if (nentries > XENSHM_MAX_PAGES_PER_REQUEST) {
		panic("xen_shm_map: %d entries", nentries);
	}
#endif

	/* XXXSMP */
	s = splvm(); /* splvm is the lowest level blocking disk and net IRQ */

	/*
	 * If a driver is waiting for resources, don't try to allocate
	 * yet. This is to avoid a flood of small requests stalling large
	 * ones.
	 */
	if (__predict_false(SIMPLEQ_FIRST(&xen_shm_callbacks) != NULL) &&
	    (flags & XSHM_CALLBACK) == 0) {
		splx(s);
#ifdef DEBUG
		static struct timeval lasttime;
		if (ratecheck(&lasttime, &xen_shm_errintvl))
			printf("xen_shm_map: ENOMEM1\n");
#endif
		return ENOMEM;
	}

	/* Allocate the needed virtual space. */
	if (vmem_alloc(xen_shm_arena, nentries,
	    VM_INSTANTFIT | VM_NOSLEEP, &new_va_pg) != 0) {
		splx(s);
#ifdef DEBUG
		static struct timeval lasttime;
		if (ratecheck(&lasttime, &xen_shm_errintvl))
			printf("xen_shm_map: ENOMEM\n");
#endif
		return ENOMEM;
	}
	splx(s);

	new_va = new_va_pg << PAGE_SHIFT;
	for (i = 0; i < nentries; i++) {
		op[i].host_addr = new_va + i * PAGE_SIZE;
		op[i].dom = domid;
		op[i].ref = grefp[i];
		op[i].flags = GNTMAP_host_map |
		    ((flags & XSHM_RO) ? GNTMAP_readonly : 0);
	}

	ret = HYPERVISOR_grant_table_op(GNTTABOP_map_grant_ref, op, nentries);
	if (__predict_false(ret)) {
		panic("xen_shm_map: HYPERVISOR_grant_table_op failed");
	}

	for (i = 0; i < nentries; i++) {
		if (__predict_false(op[i].status))
			return op[i].status;
		handlep[i] = op[i].handle;
	}

	*vap = new_va;
	return 0;
}

void
xen_shm_unmap(vaddr_t va, int nentries, grant_handle_t *handlep)
{
	gnttab_unmap_grant_ref_t op[XENSHM_MAX_PAGES_PER_REQUEST];
	struct xen_shm_callback_entry *xshmc;
	int ret, i, s;

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

	va = va >> PAGE_SHIFT;

	/* XXXSMP */
	s = splvm(); /* splvm is the lowest level blocking disk and net IRQ */

	vmem_free(xen_shm_arena, va, nentries);
	while (__predict_false((xshmc = SIMPLEQ_FIRST(&xen_shm_callbacks))
	    != NULL)) {
		SIMPLEQ_REMOVE_HEAD(&xen_shm_callbacks, xshmc_entries);
		splx(s);
		if (xshmc->xshmc_callback(xshmc->xshmc_arg) == 0) {
			/* callback succeeded */
			s = splvm(); /* XXXSMP */
			pool_put(&xen_shm_callback_pool, xshmc);
		} else {
			/* callback failed, probably out of resources */
			s = splvm(); /* XXXSMP */
			SIMPLEQ_INSERT_TAIL(&xen_shm_callbacks, xshmc,
			    xshmc_entries);
			break;
		}
	}

	splx(s);
}

int
xen_shm_callback(int (*callback)(void *), void *arg)
{
	struct xen_shm_callback_entry *xshmc;
	int s;

	s = splvm(); /* XXXSMP */
	xshmc = pool_get(&xen_shm_callback_pool, PR_NOWAIT);
	if (xshmc == NULL) {
		splx(s);
		return ENOMEM;
	}
	xshmc->xshmc_arg = arg;
	xshmc->xshmc_callback = callback;
	SIMPLEQ_INSERT_TAIL(&xen_shm_callbacks, xshmc, xshmc_entries);
	splx(s);
	return 0;
}
