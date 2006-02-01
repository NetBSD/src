/*      $NetBSD: xen_shm_machdep.c,v 1.12.2.2 2006/02/01 14:51:42 yamt Exp $      */

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
#include <machine/xen_shm.h>

/*
 * Helper routines for the backend drivers. This implement the necessary
 * functions to map a bunch of pages from foreign domains in our kernel VM
 * space, do I/O to it, and unmap it.
 *
 * At boot time, we grap some kernel VM space that we'll use to map the foreign
 * pages. We also maintain a virtual to machine mapping table to give back
 * the appropriate address to bus_dma if requested.
 * If no more VM space is available, we return an error. The caller can then
 * register a callback which will be called when the required VM space is
 * available.
 */

/* pointers to our VM space */
vaddr_t xen_shm_base_address;
u_long xen_shm_base_address_pg;
vaddr_t xen_shm_end_address;

/* Grab enouth VM space to map an entire vbd ring. */
#ifdef XEN3
#define BLKIF_RING_SIZE __RING_SIZE((blkif_sring_t *)0, PAGE_SIZE)
#endif
#define XENSHM_NPAGES (BLKIF_RING_SIZE * (BLKIF_MAX_SEGMENTS_PER_REQUEST + 1))

vsize_t xen_shm_size = (XENSHM_NPAGES * PAGE_SIZE);

paddr_t _xen_shm_vaddr2ma[XENSHM_NPAGES];

/* vm space management */
struct extent *xen_shm_ex;

/* callbacks are registered in a FIFO list. */

SIMPLEQ_HEAD(xen_shm_callback_head, xen_shm_callback_entry) xen_shm_callbacks;
struct xen_shm_callback_entry {
	SIMPLEQ_ENTRY(xen_shm_callback_entry) xshmc_entries;
	int (*xshmc_callback)(void *); /* our callback */
	void *xshmc_arg; /* cookie passed to the callback */
};
/* a pool of struct xen_shm_callback_entry */
struct pool xen_shm_callback_pool;

/* for ratecheck(9) */
struct timeval xen_shm_errintvl = { 60, 0 };  /* a minute, each */

void
xen_shm_init()
{
	SIMPLEQ_INIT(&xen_shm_callbacks);
	pool_init(&xen_shm_callback_pool, sizeof(struct xen_shm_callback_entry),
	    0, 0, 0, "xshmc", NULL);
	/* ensure we'll always get items */
	if (pool_prime(&xen_shm_callback_pool,
	    PAGE_SIZE / sizeof(struct xen_shm_callback_entry)) != 0) {
		panic("xen_shm_init can't prime pool");
	}

	xen_shm_base_address = uvm_km_alloc(kernel_map, xen_shm_size, 0,
	    UVM_KMF_VAONLY);
	xen_shm_end_address = xen_shm_base_address + xen_shm_size;
	xen_shm_base_address_pg = xen_shm_base_address >> PAGE_SHIFT;
	if (xen_shm_base_address == 0) {
		panic("xen_shm_init no VM space");
	}
	xen_shm_ex = extent_create("xen_shm",
	    xen_shm_base_address_pg,
	    (xen_shm_end_address >> PAGE_SHIFT) - 1,
	    M_DEVBUF, NULL, 0, EX_NOCOALESCE | EX_NOWAIT);
	if (xen_shm_ex == NULL) {
		panic("xen_shm_init no extent");
	}
	memset(_xen_shm_vaddr2ma, -1, sizeof(_xen_shm_vaddr2ma));
}

int
xen_shm_map(paddr_t *ma, int nentries, int domid, vaddr_t *vap, int flags)
{
	int i, s;
	vaddr_t new_va;
	u_long new_va_pg;
	multicall_entry_t mcl[XENSHM_MAX_PAGES_PER_REQUEST];
	int remap_prot = PG_V | PG_RW | PG_U | PG_M;

#ifdef DIAGNOSTIC
	if (nentries > XENSHM_MAX_PAGES_PER_REQUEST) {
		printf("xen_shm_map: %d entries\n", nentries);
		panic("xen_shm_map");
	}
#endif
	s = splvm(); /* splvm is the lowest level blocking disk and net IRQ */
	/*
	 * if a driver is waiting for ressources, don't try to allocate
	 * yet. This is to avoid a flood of small requests stalling large
	 * ones.
	 */
	if (__predict_false(SIMPLEQ_FIRST(&xen_shm_callbacks) != NULL) &&
	    (flags & XSHM_CALLBACK) == 0) {
#ifdef DEBUG
		static struct timeval lasttime;
#endif
		splx(s);
#ifdef DEBUG
		if (ratecheck(&lasttime, &xen_shm_errintvl))
			printf("xen_shm_map: ENOMEM1\n");
#endif
		return ENOMEM;
	}
	/* allocate the needed virtual space */
	if (extent_alloc(xen_shm_ex, nentries, 1, 0, EX_NOWAIT, &new_va_pg)
	    != 0) {
#ifdef DEBUG
		static struct timeval lasttime;
#endif
		splx(s);
#ifdef DEBUG
		if (ratecheck(&lasttime, &xen_shm_errintvl))
			printf("xen_shm_map: ENOMEM\n");
#endif
		return ENOMEM;
	}
	splx(s);

	new_va = new_va_pg << PAGE_SHIFT;
	for (i = 0; i < nentries; i++, new_va_pg++) {
		mcl[i].op = __HYPERVISOR_update_va_mapping_otherdomain;
		mcl[i].args[0] = new_va_pg;
		mcl[i].args[1] = ma[i] | remap_prot;
		mcl[i].args[2] = 0;
		mcl[i].args[3] = domid;
#ifdef DIAGNOSTIC
		if ((new_va_pg - xen_shm_base_address_pg) >=
		    BLKIF_RING_SIZE * XENSHM_MAX_PAGES_PER_REQUEST ||
		    (new_va_pg - xen_shm_base_address_pg) < 0) {
			printf("new_va_pg 0x%lx "
			    "xen_shm_base_address_pg 0x%lx\n",
			    new_va_pg, xen_shm_base_address_pg);
			panic("xen_shm_map: out of _xen_shm_vaddr2ma\n");
		}
		if (_xen_shm_vaddr2ma[new_va_pg - xen_shm_base_address_pg]
		    != -1) {
			printf("new new_va_pg 0x%lx not free\n", new_va_pg);
			extent_print(xen_shm_ex);
			panic("xen_shm_map: new_va_pg not free");
		}
#endif
		_xen_shm_vaddr2ma[new_va_pg - xen_shm_base_address_pg] = 
		    ma[i];
	}
	if (HYPERVISOR_multicall(mcl, nentries) != 0)
	    panic("xen_shm_map: HYPERVISOR_multicall");

	for (i = 0; i < nentries; i++) {
		if ((mcl[i].args[5] != 0)) {
			printf("xen_shm_map: mcl[%d] failed\n", i);
			xen_shm_unmap(new_va, ma, nentries, domid);
			return EINVAL;
		}
	}
	*vap = new_va;
	return 0;
}

void
xen_shm_unmap(vaddr_t va, paddr_t *pa, int nentries, int domid)
{
#ifdef XEN3
	multicall_entry_t mcl[XENSHM_MAX_PAGES_PER_REQUEST + 1];
	struct mmuext_op extop;
#else
	multicall_entry_t mcl[XENSHM_MAX_PAGES_PER_REQUEST];
#endif
	int i, s;
	struct xen_shm_callback_entry *xshmc;

#ifdef DIAGNOSTIC
	if (nentries > XENSHM_MAX_PAGES_PER_REQUEST) {
		printf("xen_shm_unmap: %d entries\n", nentries);
		panic("xen_shm_unmap");
	}
#endif

	va = va >> PAGE_SHIFT;
	for (i = 0; i < nentries; i++) {
		mcl[i].op = __HYPERVISOR_update_va_mapping;
		mcl[i].args[0] = va + i;
		mcl[i].args[1] = 0;
		mcl[i].args[2] = 0;
#ifdef DIAGNOSTIC
		if ((va + i - xen_shm_base_address_pg) >=
		    BLKIF_RING_SIZE * XENSHM_MAX_PAGES_PER_REQUEST ||
		    (va + i - xen_shm_base_address_pg) < 0) {
			printf("va 0x%lx i 0x%x "
			    "xen_shm_base_address_pg 0x%lx\n",
			    va, i, xen_shm_base_address_pg);
			panic("xen_shm_unmap: out of _xen_shm_vaddr2ma\n");
		}
#endif
		_xen_shm_vaddr2ma[va + i - xen_shm_base_address_pg] = -1;
	}
#ifdef XEN3
	mcl[nentries].op = __HYPERVISOR_mmuext_op;
	extop.cmd = MMUEXT_TLB_FLUSH_LOCAL;
	mcl[i].args[0] = (long)&extop;
	if (HYPERVISOR_multicall(mcl, nentries + 1) != 0)
		panic("xen_shm_unmap");
#else
	mcl[nentries - 1].args[2] = UVMF_FLUSH_TLB;
	if (HYPERVISOR_multicall(mcl, nentries) != 0)
		panic("xen_shm_unmap");
#endif
	s = splvm(); /* splvm is the lowest level blocking disk and net IRQ */
	if (extent_free(xen_shm_ex, va, nentries, EX_NOWAIT) != 0)
		panic("xen_shm_unmap: extent_free");
	while (__predict_false((xshmc = SIMPLEQ_FIRST(&xen_shm_callbacks))
	    != NULL)) {
		SIMPLEQ_REMOVE_HEAD(&xen_shm_callbacks, xshmc_entries);
		splx(s);
		if (xshmc->xshmc_callback(xshmc->xshmc_arg) == 0) {
			/* callback succeeded */
			s = splvm();
			pool_put(&xen_shm_callback_pool, xshmc);
		} else {
			/* callback failed, probably out of ressources */
			s = splvm();
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

	s = splvm();
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

#ifdef DIAGNOSTIC
	if (((va >> PAGE_SHIFT) - xen_shm_base_address_pg) >=
	    BLKIF_RING_SIZE * XENSHM_MAX_PAGES_PER_REQUEST ||
	    ((va >> PAGE_SHIFT) - xen_shm_base_address_pg) < 0) {
		printf("va 0x%lx xen_shm_base_address_pg 0x%lx\n",
		    (va >> PAGE_SHIFT), xen_shm_base_address_pg);
		panic("xen_shm_vaddr2ma: out of _xen_shm_vaddr2ma\n");
	}
	if (_xen_shm_vaddr2ma[(va >> PAGE_SHIFT) - xen_shm_base_address_pg]
	    == -1) {
		printf("xen_shm_vaddr2ma: request for unmapped va 0x%lx\n",
		    va);
		panic("xen_shm_vaddr2ma: unmapped va");
	}
#endif
	*map = _xen_shm_vaddr2ma[(va >> PAGE_SHIFT) - xen_shm_base_address_pg];
	*map |= (va & PAGE_MASK);
	return 0;
}
