/*      $NetBSD: xen_shm_machdep.c,v 1.1.2.1 2005/02/16 13:46:29 bouyer Exp $      */

/*
 * Copyright (c) 2005 Manuel Bouyer.
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
 *      This product includes software developed by Manuel Bouyer.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
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
 *
 */

#include <sys/types.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/malloc.h>
#include <sys/queue.h>
#include <sys/extent.h>
#include <sys/kernel.h>
#include <uvm/uvm.h>

#include <machine/pmap.h>
#include <machine/hypervisor.h>
#include <machine/xen.h>
#include <machine/evtchn.h>
#include <machine/ctrl_if.h>
#include <machine/xen_shm.h>

/*
 * Helper routines for the backend drivers. This implement the necessary
 * functions to map a bunch of pages from foreing domains in our kernel VM
 * space, do I/O to it, and unmap it.
 *
 * At boot time, we grap some kernel VM space that we'll use to map the foreing
 * pages. We also maintain a virtual to machine mapping table to give back
 * the appropriate address to bus_dma if requested.
 */


vaddr_t xen_shm_base_address;
u_long xen_shm_base_address_pg;
vaddr_t xen_shm_end_address;
/*
 * Grab enouth VM space to map an entire vbd ring. Make it a variable to that
 * it can be patched in the binary.
 */
#define XENSHM_MAX_PAGES_PER_REQUEST (BLKIF_MAX_SEGMENTS_PER_REQUEST + 1)

vsize_t xen_shm_size =
    (BLKIF_RING_SIZE * XENSHM_MAX_PAGES_PER_REQUEST * PAGE_SIZE);

paddr_t _xen_shm_vaddr2ma[BLKIF_RING_SIZE * XENSHM_MAX_PAGES_PER_REQUEST];

/* vm space management */
struct extent *xen_shm_ex;

void
xen_shm_init()
{
	xen_shm_base_address = uvm_km_valloc(kernel_map, xen_shm_size);
	xen_shm_end_address = xen_shm_base_address + xen_shm_size;
	xen_shm_base_address_pg = xen_shm_base_address >> PAGE_SHIFT;
	if (xen_shm_base_address == 0) {
		panic("xen_shm_init no VM space");
	}
	xen_shm_ex = extent_create("xen_shm",
	    xen_shm_base_address_pg,
	    xen_shm_end_address >> PAGE_SHIFT,
	    M_DEVBUF, NULL, 0, EX_NOCOALESCE | EX_NOWAIT);
	if (xen_shm_ex == NULL) {
		panic("xen_shm_init no extent");
	}
	memset(_xen_shm_vaddr2ma, -1, sizeof(_xen_shm_vaddr2ma));
}

vaddr_t
xen_shm_map(paddr_t *ma, int nentries, int domid)
{
	int i;
	vaddr_t new_va;
	u_long new_va_pg;
	multicall_entry_t mcl[XENSHM_MAX_PAGES_PER_REQUEST];
	int remap_prot = PG_V | PG_RW | PG_U | PG_M;

	/* allocate the needed virtual space */
	if (extent_alloc(xen_shm_ex, nentries, 1, 0, EX_NOWAIT, &new_va_pg)
	    != 0)
		return 0;
	new_va = new_va_pg << PAGE_SHIFT;
	for (i = 0; i < nentries; i++, new_va_pg++) {
		mcl[i].op = __HYPERVISOR_update_va_mapping_otherdomain;
		mcl[i].args[0] = new_va_pg;
		mcl[i].args[1] = ma[i] | remap_prot;
		mcl[i].args[2] = 0;
		mcl[i].args[3] = domid;
		_xen_shm_vaddr2ma[new_va_pg - xen_shm_base_address_pg] = 
		    ma[i];
	}
	if (HYPERVISOR_multicall(mcl, nentries) != 0)
	    panic("xen_shm_map: HYPERVISOR_multicall");

	for (i = 0; i < nentries; i++) {
		if ((mcl[i].args[5] != 0)) {
			printf("xen_shm_map: mcl[%d] failed\n", i);
			xen_shm_unmap(new_va, ma, nentries, domid);
			return 0;
		}
	}
	return new_va;
}

void
xen_shm_unmap(vaddr_t va, paddr_t *pa, int nentries, int domid)
{
	multicall_entry_t mcl[XENSHM_MAX_PAGES_PER_REQUEST];
	int i;

	va = va >> PAGE_SHIFT;
	for (i = 0; i < nentries; i++) {
		mcl[i].op = __HYPERVISOR_update_va_mapping;
		mcl[i].args[0] = va + i;
		mcl[i].args[1] = 0;
		mcl[i].args[2] = 0;
		_xen_shm_vaddr2ma[va + i - xen_shm_base_address_pg] = -1;
	}
	mcl[nentries - 1].args[2] = UVMF_FLUSH_TLB;
	if (HYPERVISOR_multicall(mcl, nentries) != 0)
		panic("xen_shm_unmap");
	if (extent_free(xen_shm_ex, va, nentries, EX_NOWAIT) != 0)
		panic("xen_shm_unmap: extent_free");
}

/*
 * Shared memory pages are managed by drivers, and are not known from
 * the pmap. This tests if va is a shared memory page, and if so
 * returns the machine address (there's no physical address for these pages)
 */
int
xen_shm_vaddr2ma(vaddr_t va, paddr_t *map)
{
	if (va <  xen_shm_base_address || va >=  xen_shm_end_address)
		return -1;

	*map = _xen_shm_vaddr2ma[(va >> PAGE_SHIFT) - xen_shm_base_address_pg];
	*map |= (va & PAGE_MASK);
	return 0;
}
