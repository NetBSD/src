/*	$NetBSD: mainbus.c,v 1.45.10.1 2009/05/13 17:17:43 jym Exp $	*/

/*-
 * Copyright (c) 2001, 2002 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Matthew Fredette.
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

/*	$OpenBSD: mainbus.c,v 1.13 2001/09/19 20:50:56 mickey Exp $	*/

/*
 * Copyright (c) 1998-2000 Michael Shalayeff
 * All rights reserved.
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
 *      This product includes software developed by Michael Shalayeff.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR OR HIS RELATIVES BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF MIND, USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: mainbus.c,v 1.45.10.1 2009/05/13 17:17:43 jym Exp $");

#include "locators.h"
#include "power.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/reboot.h>
#include <sys/extent.h>
#include <sys/mbuf.h>

#include <uvm/uvm_page.h>
#include <uvm/uvm.h>

#include <machine/pdc.h>
#include <machine/iomod.h>
#include <machine/autoconf.h>

#include <hp700/hp700/machdep.h>
#include <hp700/hp700/intr.h>
#include <hp700/dev/cpudevs.h>

static struct pdc_hpa pdc_hpa PDC_ALIGNMENT;

struct mainbus_softc {
	device_t sc_dv;

	hppa_hpa_t sc_hpa;
};

int	mbmatch(device_t, cfdata_t, void *);
void	mbattach(device_t, device_t, void *);

CFATTACH_DECL_NEW(mainbus, sizeof(struct mainbus_softc),
    mbmatch, mbattach, NULL, NULL);

extern struct cfdriver mainbus_cd;

static int mb_attached;

/* from machdep.c */
extern struct extent *hp700_io_extent;
extern struct extent *dma24_ex;

u_int8_t mbus_r1(void *, bus_space_handle_t, bus_size_t);
u_int16_t mbus_r2(void *, bus_space_handle_t, bus_size_t);
u_int32_t mbus_r4(void *, bus_space_handle_t, bus_size_t);
u_int64_t mbus_r8(void *, bus_space_handle_t, bus_size_t);
void mbus_w1(void *, bus_space_handle_t, bus_size_t, u_int8_t);
void mbus_w2(void *, bus_space_handle_t, bus_size_t, u_int16_t);
void mbus_w4(void *, bus_space_handle_t, bus_size_t, u_int32_t);
void mbus_w8(void *, bus_space_handle_t, bus_size_t, u_int64_t);
void mbus_rm_1(void *, bus_space_handle_t, bus_size_t, u_int8_t *, bus_size_t);
void mbus_rm_2(void *, bus_space_handle_t, bus_size_t, u_int16_t *, bus_size_t);
void mbus_rm_4(void *, bus_space_handle_t, bus_size_t, u_int32_t *, bus_size_t);
void mbus_rm_8(void *, bus_space_handle_t, bus_size_t, u_int64_t *, bus_size_t);
void mbus_wm_1(void *, bus_space_handle_t, bus_size_t, const u_int8_t *, bus_size_t);
void mbus_wm_2(void *, bus_space_handle_t, bus_size_t, const u_int16_t *, bus_size_t);
void mbus_wm_4(void *, bus_space_handle_t, bus_size_t, const u_int32_t *, bus_size_t);
void mbus_wm_8(void *, bus_space_handle_t, bus_size_t, const u_int64_t *, bus_size_t);
void mbus_rr_1(void *, bus_space_handle_t, bus_size_t, u_int8_t *, bus_size_t);
void mbus_rr_2(void *, bus_space_handle_t, bus_size_t, u_int16_t *, bus_size_t);
void mbus_rr_4(void *, bus_space_handle_t, bus_size_t, u_int32_t *, bus_size_t);
void mbus_rr_8(void *, bus_space_handle_t, bus_size_t, u_int64_t *, bus_size_t);
void mbus_wr_1(void *, bus_space_handle_t, bus_size_t, const u_int8_t *, bus_size_t);
void mbus_wr_2(void *, bus_space_handle_t, bus_size_t, const u_int16_t *, bus_size_t);
void mbus_wr_4(void *, bus_space_handle_t, bus_size_t, const u_int32_t *, bus_size_t);
void mbus_wr_8(void *, bus_space_handle_t, bus_size_t, const u_int64_t *, bus_size_t);
void mbus_sm_1(void *, bus_space_handle_t, bus_size_t, u_int8_t, bus_size_t);
void mbus_sm_2(void *, bus_space_handle_t, bus_size_t, u_int16_t, bus_size_t);
void mbus_sm_4(void *, bus_space_handle_t, bus_size_t, u_int32_t, bus_size_t);
void mbus_sm_8(void *, bus_space_handle_t, bus_size_t, u_int64_t, bus_size_t);
void mbus_sr_1(void *, bus_space_handle_t, bus_size_t, u_int8_t, bus_size_t);
void mbus_sr_2(void *, bus_space_handle_t, bus_size_t, u_int16_t, bus_size_t);
void mbus_sr_4(void *, bus_space_handle_t, bus_size_t, u_int32_t, bus_size_t);
void mbus_sr_8(void *, bus_space_handle_t, bus_size_t, u_int64_t, bus_size_t);
void mbus_cp_1(void *, bus_space_handle_t, bus_size_t, bus_space_handle_t, bus_size_t, bus_size_t);
void mbus_cp_2(void *, bus_space_handle_t, bus_size_t, bus_space_handle_t, bus_size_t, bus_size_t);
void mbus_cp_4(void *, bus_space_handle_t, bus_size_t, bus_space_handle_t, bus_size_t, bus_size_t);
void mbus_cp_8(void *, bus_space_handle_t, bus_size_t, bus_space_handle_t, bus_size_t, bus_size_t);

int mbus_add_mapping(bus_addr_t, bus_size_t, int, bus_space_handle_t *);
int mbus_remove_mapping(bus_space_handle_t, bus_size_t, bus_addr_t *);
int mbus_map(void *, bus_addr_t, bus_size_t, int, bus_space_handle_t *);
void mbus_unmap(void *, bus_space_handle_t, bus_size_t);
int mbus_alloc(void *, bus_addr_t, bus_addr_t, bus_size_t, bus_size_t, bus_size_t, int, bus_addr_t *, bus_space_handle_t *);
void mbus_free(void *, bus_space_handle_t, bus_size_t);
int mbus_subregion(void *, bus_space_handle_t, bus_size_t, bus_size_t, bus_space_handle_t *);
void mbus_barrier(void *, bus_space_handle_t, bus_size_t, bus_size_t, int);
void *mbus_vaddr(void *, bus_space_handle_t);

int mbus_dmamap_create(void *, bus_size_t, int, bus_size_t, bus_size_t, int, bus_dmamap_t *);
void mbus_dmamap_destroy(void *, bus_dmamap_t);
int mbus_dmamap_load(void *, bus_dmamap_t, void *, bus_size_t, struct proc *, int);
int mbus_dmamap_load_mbuf(void *, bus_dmamap_t, struct mbuf *, int);
int mbus_dmamap_load_uio(void *, bus_dmamap_t, struct uio *, int);
int mbus_dmamap_load_raw(void *, bus_dmamap_t, bus_dma_segment_t *, int, bus_size_t, int);
void mbus_dmamap_unload(void *, bus_dmamap_t);
void mbus_dmamap_sync(void *, bus_dmamap_t, bus_addr_t, bus_size_t, int);
int mbus_dmamem_alloc(void *, bus_size_t, bus_size_t, bus_size_t, bus_dma_segment_t *, int, int *, int);
void mbus_dmamem_free(void *, bus_dma_segment_t *, int);
int mbus_dmamem_map(void *, bus_dma_segment_t *, int, size_t, void **, int);
void mbus_dmamem_unmap(void *, void *, size_t);
paddr_t mbus_dmamem_mmap(void *, bus_dma_segment_t *, int, off_t, int, int);
int _bus_dmamap_load_buffer(bus_dma_tag_t t, bus_dmamap_t map, void *buf,
    bus_size_t buflen, struct vmspace *vm, int flags, paddr_t *lastaddrp,
    int *segp, int first);

int
mbus_add_mapping(bus_addr_t bpa, bus_size_t size, int flags,
    bus_space_handle_t *bshp)
{
	u_int frames;
#ifdef USE_BTLB
	vsize_t btlb_size;
	int error;
#endif /* USE_BTLB */

	/*
	 * We must be called with a page-aligned address in
	 * I/O space, and with a multiple of the page size.
	 */
	KASSERT((bpa & PGOFSET) == 0);
	KASSERT(bpa >= HPPA_IOSPACE);
	KASSERT((size & PGOFSET) == 0);

	/*
	 * Assume that this will succeed.
	 */
	*bshp = bpa;

	/*
	 * Loop while there is space left to map.
	 */
	frames = size >> PGSHIFT;
	while (frames > 0) {

		/*
		 * If this mapping is more than eight pages long,
		 * try to add a BTLB entry.
		 */
#ifdef USE_BTLB
		if (frames > 8 &&
		    frames >= hppa_btlb_size_min) {
			btlb_size = frames;
			if (btlb_size > hppa_btlb_size_max)
				btlb_size = hppa_btlb_size_max;
			btlb_size <<= PGSHIFT;
			error = hppa_btlb_insert(pmap_kernel()->pmap_space,
			    bpa, bpa, &btlb_size,
			    pmap_kernel()->pmap_pid |
			    pmap_prot(pmap_kernel(), VM_PROT_READ | VM_PROT_WRITE));
			if (error == 0) {
				bpa += btlb_size;
				frames -= (btlb_size >> PGSHIFT);
				continue;
			}
			else if (error != ENOMEM)
				return error;
		}
#endif /* USE_BTLB */

		/*
		 * Enter another single-page mapping.
		 */
		pmap_kenter_pa(bpa, bpa, VM_PROT_READ | VM_PROT_WRITE);
		bpa += PAGE_SIZE;
		frames--;
	}

	/* Success. */
	return 0;
}

/*
 * This removes a mapping added by mbus_add_mapping.
 */
int
mbus_remove_mapping(bus_space_handle_t bsh, bus_size_t size, bus_addr_t *bpap)
{
	bus_addr_t bpa;
	u_int frames;
#ifdef USE_BTLB
	vsize_t btlb_size;
	int error;
#endif /* USE_BTLB */

	/*
	 * We must be called with a page-aligned address in
	 * I/O space, and with a multiple of the page size.
	 */
	bpa = *bpap = bsh;
	KASSERT((bpa & PGOFSET) == 0);
	KASSERT(bpa >= HPPA_IOSPACE);
	KASSERT((size & PGOFSET) == 0);

	/*
	 * Loop while there is space left to unmap.
	 */
	frames = size >> PGSHIFT;
	while (frames > 0) {

		/*
		 * If this mapping is more than eight pages long,
		 * try to remove a BTLB entry.
		 */
#ifdef USE_BTLB
		if (frames > 8 &&
		    frames >= hppa_btlb_size_min) {
			btlb_size = frames;
			if (btlb_size > hppa_btlb_size_max)
				btlb_size = hppa_btlb_size_max;
			btlb_size <<= PGSHIFT;
			error = hppa_btlb_purge(pmap_kernel()->pmap_space,
						bpa, &btlb_size);
			if (error == 0) {
				bpa += btlb_size;
				frames -= (btlb_size >> PGSHIFT);
				continue;
			}
			else if (error != ENOENT)
				return error;
		}
#endif /* USE_BTLB */

		/*
		 * Remove another single-page mapping.
		 */
		pmap_kremove(bpa, PAGE_SIZE);
		bpa += PAGE_SIZE;
		frames--;
	}

	/* Success. */
	return 0;
}

int
mbus_map(void *v, bus_addr_t bpa, bus_size_t size, int flags,
    bus_space_handle_t *bshp)
{
	int error;
	bus_size_t offset;

	/*
	 * We must only be called with addresses in I/O space.
	 */
	KASSERT(bpa >= HPPA_IOSPACE);

	/*
	 * Page-align the I/O address and size.
	 */
	offset = (bpa & PGOFSET);
	bpa -= offset;
	size += offset;
	size = round_page(size);

	/*
	 * Allocate the region of I/O space.
	 */
	error = extent_alloc_region(hp700_io_extent, bpa, size, EX_NOWAIT);
	if (error)
		return (error);

	/*
	 * Map the region of I/O space.
	 */
	error = mbus_add_mapping(bpa, size, flags, bshp);
	*bshp |= offset;
	if (error) {
		if (extent_free(hp700_io_extent, bpa, size, EX_NOWAIT)) {
			printf ("bus_space_map: pa 0x%lx, size 0x%lx\n",
				bpa, size);
			printf ("bus_space_map: can't free region\n");
		}
	}

	return error;
}

void
mbus_unmap(void *v, bus_space_handle_t bsh, bus_size_t size)
{
	bus_size_t offset;
	bus_addr_t bpa;
	int error;

	/*
	 * Page-align the bus_space handle and size.
	 */
	offset = bsh & PGOFSET;
	bsh -= offset;
	size += offset;
	size = round_page(size);

	/*
	 * Unmap the region of I/O space.
	 */
	error = mbus_remove_mapping(bsh, size, &bpa);
	if (error)
		panic("mbus_unmap: can't unmap region (%d)", error);

	/*
	 * Free the region of I/O space.
	 */
	error = extent_free(hp700_io_extent, bpa, size, EX_NOWAIT);
	if (error) {
		printf("bus_space_unmap: ps 0x%lx, size 0x%lx\n",
		    bpa, size);
		panic("bus_space_unmap: can't free region (%d)", error);
	}
}

int
mbus_alloc(void *v, bus_addr_t rstart, bus_addr_t rend, bus_size_t size,
    bus_size_t align, bus_size_t boundary, int flags, bus_addr_t *addrp,
    bus_space_handle_t *bshp)
{
	bus_addr_t bpa;
	int error;

	if (rstart < hp700_io_extent->ex_start ||
	    rend > hp700_io_extent->ex_end)
		panic("bus_space_alloc: bad region start/end");

	/*
	 * Force the allocated region to be page-aligned.
	 */
	if (align < PAGE_SIZE)
		align = PAGE_SIZE;
	size = round_page(size);

	/*
	 * Allocate the region of I/O space.
	 */
	error = extent_alloc_subregion1(hp700_io_extent, rstart, rend, size,
					align, 0, boundary, EX_NOWAIT, &bpa);
	if (error)
		return (error);

	/*
	 * Map the region of I/O space.
	 */
	error = mbus_add_mapping(bpa, size, flags, bshp);
	if (error) {
		if (extent_free(hp700_io_extent, bpa, size, EX_NOWAIT)) {
			printf("bus_space_alloc: pa 0x%lx, size 0x%lx\n",
				bpa, size);
			printf("bus_space_alloc: can't free region\n");
		}
	}

	*addrp = bpa;

	return error;
}

void
mbus_free(void *v, bus_space_handle_t h, bus_size_t size)
{
	/* bus_space_unmap() does all that we need to do. */
	mbus_unmap(v, h, size);
}

int
mbus_subregion(void *v, bus_space_handle_t bsh, bus_size_t offset,
    bus_size_t size, bus_space_handle_t *nbshp)
{
	*nbshp = bsh + offset;
	return(0);
}

void
mbus_barrier(void *v, bus_space_handle_t h, bus_size_t o, bus_size_t l, int op)
{
	sync_caches();
}

void*
mbus_vaddr(void *v, bus_space_handle_t h)
{
	/*
	 * We must only be called with addresses in I/O space.
	 */
	KASSERT(h >= HPPA_IOSPACE);
	return (void*)h;
}

u_int8_t
mbus_r1(void *v, bus_space_handle_t h, bus_size_t o)
{
	return *((volatile u_int8_t *)(h + o));
}

u_int16_t
mbus_r2(void *v, bus_space_handle_t h, bus_size_t o)
{
	return *((volatile u_int16_t *)(h + o));
}

u_int32_t
mbus_r4(void *v, bus_space_handle_t h, bus_size_t o)
{
	return *((volatile u_int32_t *)(h + o));
}

u_int64_t
mbus_r8(void *v, bus_space_handle_t h, bus_size_t o)
{
	return *((volatile u_int64_t *)(h + o));
}

void
mbus_w1(void *v, bus_space_handle_t h, bus_size_t o, u_int8_t vv)
{
	*((volatile u_int8_t *)(h + o)) = vv;
}

void
mbus_w2(void *v, bus_space_handle_t h, bus_size_t o, u_int16_t vv)
{
	*((volatile u_int16_t *)(h + o)) = vv;
}

void
mbus_w4(void *v, bus_space_handle_t h, bus_size_t o, u_int32_t vv)
{
	*((volatile u_int32_t *)(h + o)) = vv;
}

void
mbus_w8(void *v, bus_space_handle_t h, bus_size_t o, u_int64_t vv)
{
	*((volatile u_int64_t *)(h + o)) = vv;
}


void
mbus_rm_1(void *v, bus_space_handle_t h, bus_size_t o, u_int8_t *a, bus_size_t c)
{
	h += o;
	while (c--)
		*(a++) = *(volatile u_int8_t *)h;
}

void
mbus_rm_2(void *v, bus_space_handle_t h, bus_size_t o, u_int16_t *a, bus_size_t c)
{
	h += o;
	while (c--)
		*(a++) = *(volatile u_int16_t *)h;
}

void
mbus_rm_4(void *v, bus_space_handle_t h, bus_size_t o, u_int32_t *a, bus_size_t c)
{
	h += o;
	while (c--)
		*(a++) = *(volatile u_int32_t *)h;
}

void
mbus_rm_8(void *v, bus_space_handle_t h, bus_size_t o, u_int64_t *a, bus_size_t c)
{
	h += o;
	while (c--)
		*(a++) = *(volatile u_int64_t *)h;
}

void
mbus_wm_1(void *v, bus_space_handle_t h, bus_size_t o, const u_int8_t *a, bus_size_t c)
{
	h += o;
	while (c--)
		*(volatile u_int8_t *)h = *(a++);
}

void
mbus_wm_2(void *v, bus_space_handle_t h, bus_size_t o, const u_int16_t *a, bus_size_t c)
{
	h += o;
	while (c--)
		*(volatile u_int16_t *)h = *(a++);
}

void
mbus_wm_4(void *v, bus_space_handle_t h, bus_size_t o, const u_int32_t *a, bus_size_t c)
{
	h += o;
	while (c--)
		*(volatile u_int32_t *)h = *(a++);
}

void
mbus_wm_8(void *v, bus_space_handle_t h, bus_size_t o, const u_int64_t *a, bus_size_t c)
{
	h += o;
	while (c--)
		*(volatile u_int64_t *)h = *(a++);
}

void
mbus_sm_1(void *v, bus_space_handle_t h, bus_size_t o, u_int8_t vv, bus_size_t c)
{
	h += o;
	while (c--)
		*(volatile u_int8_t *)h = vv;
}

void
mbus_sm_2(void *v, bus_space_handle_t h, bus_size_t o, u_int16_t vv, bus_size_t c)
{
	h += o;
	while (c--)
		*(volatile u_int16_t *)h = vv;
}

void
mbus_sm_4(void *v, bus_space_handle_t h, bus_size_t o, u_int32_t vv, bus_size_t c)
{
	h += o;
	while (c--)
		*(volatile u_int32_t *)h = vv;
}

void
mbus_sm_8(void *v, bus_space_handle_t h, bus_size_t o, u_int64_t vv, bus_size_t c)
{
	h += o;
	while (c--)
		*(volatile u_int64_t *)h = vv;
}

void mbus_rrm_2(void *v, bus_space_handle_t h, bus_size_t o, u_int16_t*a, bus_size_t c);
void mbus_rrm_4(void *v, bus_space_handle_t h, bus_size_t o, u_int32_t*a, bus_size_t c);
void mbus_rrm_8(void *v, bus_space_handle_t h, bus_size_t o, u_int64_t*a, bus_size_t c);

void mbus_wrm_2(void *v, bus_space_handle_t h, bus_size_t o, const u_int16_t *a, bus_size_t c);
void mbus_wrm_4(void *v, bus_space_handle_t h, bus_size_t o, const u_int32_t *a, bus_size_t c);
void mbus_wrm_8(void *v, bus_space_handle_t h, bus_size_t o, const u_int64_t *a, bus_size_t c);

void
mbus_rr_1(void *v, bus_space_handle_t h, bus_size_t o, u_int8_t *a, bus_size_t c)
{
	volatile u_int8_t *p;

	h += o;
	p = (void *)h;
	while (c--)
		*a++ = *p++;
}

void
mbus_rr_2(void *v, bus_space_handle_t h, bus_size_t o, u_int16_t *a, bus_size_t c)
{
	volatile u_int16_t *p;

	h += o;
	p = (void *)h;
	while (c--)
		*a++ = *p++;
}

void
mbus_rr_4(void *v, bus_space_handle_t h, bus_size_t o, u_int32_t *a, bus_size_t c)
{
	volatile u_int32_t *p;

	h += o;
	p = (void *)h;
	while (c--)
		*a++ = *p++;
}

void
mbus_rr_8(void *v, bus_space_handle_t h, bus_size_t o, u_int64_t *a, bus_size_t c)
{
	volatile u_int64_t *p;

	h += o;
	p = (void *)h;
	while (c--)
		*a++ = *p++;
}

void
mbus_wr_1(void *v, bus_space_handle_t h, bus_size_t o, const u_int8_t *a, bus_size_t c)
{
	volatile u_int8_t *p;

	h += o;
	p = (void *)h;
	while (c--)
		*p++ = *a++;
}

void
mbus_wr_2(void *v, bus_space_handle_t h, bus_size_t o, const u_int16_t *a, bus_size_t c)
{
	volatile u_int16_t *p;

	h += o;
	p = (void *)h;
	while (c--)
		*p++ = *a++;
}

void
mbus_wr_4(void *v, bus_space_handle_t h, bus_size_t o, const u_int32_t *a, bus_size_t c)
{
	volatile u_int32_t *p;

	h += o;
	p = (void *)h;
	while (c--)
		*p++ = *a++;
}

void
mbus_wr_8(void *v, bus_space_handle_t h, bus_size_t o, const u_int64_t *a, bus_size_t c)
{
	volatile u_int64_t *p;

	h += o;
	p = (void *)h;
	while (c--)
		*p++ = *a++;
}

void mbus_rrr_2(void *, bus_space_handle_t, bus_size_t, u_int16_t *, bus_size_t);
void mbus_rrr_4(void *, bus_space_handle_t, bus_size_t, u_int32_t *, bus_size_t);
void mbus_rrr_8(void *, bus_space_handle_t, bus_size_t, u_int64_t *, bus_size_t);

void mbus_wrr_2(void *, bus_space_handle_t, bus_size_t, const u_int16_t *, bus_size_t);
void mbus_wrr_4(void *, bus_space_handle_t, bus_size_t, const u_int32_t *, bus_size_t);
void mbus_wrr_8(void *, bus_space_handle_t, bus_size_t, const u_int64_t *, bus_size_t);

void
mbus_sr_1(void *v, bus_space_handle_t h, bus_size_t o, u_int8_t vv, bus_size_t c)
{
	volatile u_int8_t *p;

	h += o;
	p = (void *)h;
	while (c--)
		*p++ = vv;
}

void
mbus_sr_2(void *v, bus_space_handle_t h, bus_size_t o, u_int16_t vv, bus_size_t c)
{
	volatile u_int16_t *p;

	h += o;
	p = (void *)h;
	while (c--)
		*p++ = vv;
}

void
mbus_sr_4(void *v, bus_space_handle_t h, bus_size_t o, u_int32_t vv, bus_size_t c)
{
	volatile u_int32_t *p;

	h += o;
	p = (void *)h;
	while (c--)
		*p++ = vv;
}

void
mbus_sr_8(void *v, bus_space_handle_t h, bus_size_t o, u_int64_t vv, bus_size_t c)
{
	volatile u_int64_t *p;

	h += o;
	p = (void *)h;
	while (c--)
		*p++ = vv;
}

void
mbus_cp_1(void *v, bus_space_handle_t h1, bus_size_t o1,
	  bus_space_handle_t h2, bus_size_t o2, bus_size_t c)
{
	volatile u_int8_t *p1, *p2;

	h1 += o1;
	h2 += o2;
	p1 = (void *)h1;
	p2 = (void *)h2;
	while (c--)
		*p1++ = *p2++;
}

void
mbus_cp_2(void *v, bus_space_handle_t h1, bus_size_t o1,
	  bus_space_handle_t h2, bus_size_t o2, bus_size_t c)
{
	volatile u_int16_t *p1, *p2;

	h1 += o1;
	h2 += o2;
	p1 = (void *)h1;
	p2 = (void *)h2;
	while (c--)
		*p1++ = *p2++;
}

void
mbus_cp_4(void *v, bus_space_handle_t h1, bus_size_t o1,
	  bus_space_handle_t h2, bus_size_t o2, bus_size_t c)
{
	volatile u_int32_t *p1, *p2;

	h1 += o1;
	h2 += o2;
	p1 = (void *)h1;
	p2 = (void *)h2;
	while (c--)
		*p1++ = *p2++;
}

void
mbus_cp_8(void *v, bus_space_handle_t h1, bus_size_t o1,
	  bus_space_handle_t h2, bus_size_t o2, bus_size_t c)
{
	volatile u_int64_t *p1, *p2;

	h1 += o1;
	h2 += o2;
	p1 = (void *)h1;
	p2 = (void *)h2;
	while (c--)
		*p1++ = *p2++;
}


const struct hppa_bus_space_tag hppa_bustag = {
	NULL,

	mbus_map, mbus_unmap, mbus_subregion, mbus_alloc, mbus_free,
	mbus_barrier, mbus_vaddr,
	mbus_r1,    mbus_r2,   mbus_r4,   mbus_r8,
	mbus_w1,    mbus_w2,   mbus_w4,   mbus_w8,
	mbus_rm_1,  mbus_rm_2, mbus_rm_4, mbus_rm_8,
	mbus_wm_1,  mbus_wm_2, mbus_wm_4, mbus_wm_8,
	mbus_sm_1,  mbus_sm_2, mbus_sm_4, mbus_sm_8,
	/* *_stream_* are the same as non-stream for native busses */
		    mbus_rm_2, mbus_rm_4, mbus_rm_8,
		    mbus_wm_2, mbus_wm_4, mbus_wm_8,
	mbus_rr_1,  mbus_rr_2, mbus_rr_4, mbus_rr_8,
	mbus_wr_1,  mbus_wr_2, mbus_wr_4, mbus_wr_8,
	/* *_stream_* are the same as non-stream for native busses */
		    mbus_rr_2, mbus_rr_4, mbus_rr_8,
		    mbus_wr_2, mbus_wr_4, mbus_wr_8,
	mbus_sr_1,  mbus_sr_2, mbus_sr_4, mbus_sr_8,
	mbus_cp_1,  mbus_cp_2, mbus_cp_4, mbus_cp_8
};

/*
 * Common function for DMA map creation.  May be called by bus-specific
 * DMA map creation functions.
 */
int
mbus_dmamap_create(void *v, bus_size_t size, int nsegments, bus_size_t maxsegsz,
    bus_size_t boundary, int flags, bus_dmamap_t *dmamp)
{
	struct hppa_bus_dmamap *map;
	size_t mapsize;

	/*
	 * Allocate and initialize the DMA map.  The end of the map
	 * is a variable-sized array of segments, so we allocate enough
	 * room for them in one shot.
	 *
	 * Note we don't preserve the WAITOK or NOWAIT flags.  Preservation
	 * of ALLOCNOW notifies others that we've reserved these resources,
	 * and they are not to be freed.
	 *
	 * The bus_dmamap_t includes one bus_dma_segment_t, hence
	 * the (nsegments - 1).
	 */
	mapsize = sizeof(struct hppa_bus_dmamap) +
	    (sizeof(bus_dma_segment_t) * (nsegments - 1));
	map = malloc(mapsize, M_DMAMAP,
	    (flags & BUS_DMA_NOWAIT) ? M_NOWAIT : M_WAITOK);
	if (!map)
		return (ENOMEM);

	memset(map, 0, mapsize);
	map->_dm_size = size;
	map->_dm_segcnt = nsegments;
	map->_dm_maxsegsz = maxsegsz;
	map->_dm_boundary = boundary;
	map->_dm_flags = flags & ~(BUS_DMA_WAITOK|BUS_DMA_NOWAIT);
	map->dm_mapsize = 0;		/* no valid mappings */
	map->dm_nsegs = 0;

	*dmamp = map;
	return (0);
}

/*
 * Common function for DMA map destruction.  May be called by bus-specific
 * DMA map destruction functions.
 */
void
mbus_dmamap_destroy(void *v, bus_dmamap_t map)
{

	/*
	 * If the handle contains a valid mapping, unload it.
	 */
	if (map->dm_mapsize != 0)
		mbus_dmamap_unload(v, map);

	free(map, M_DMAMAP);
}

/*
 * load DMA map with a linear buffer.
 */
int
mbus_dmamap_load(void *v, bus_dmamap_t map, void *buf, bus_size_t buflen,
    struct proc *p, int flags)
{
	vaddr_t lastaddr;
	int seg, error;
	struct vmspace *vm;

	/*
	 * Make sure that on error condition we return "no valid mappings".
	 */
	map->dm_mapsize = 0;
	map->dm_nsegs = 0;

	if (buflen > map->_dm_size)
		return (EINVAL);

	if (p != NULL) {
		vm = p->p_vmspace;
	} else {
		vm = vmspace_kernel();
	}

	seg = 0;
	error = _bus_dmamap_load_buffer(NULL, map, buf, buflen, vm, flags,
	    &lastaddr, &seg, 1);
	if (error == 0) {
		map->dm_mapsize = buflen;
		map->dm_nsegs = seg + 1;
	}
	return (error);
}

/*
 * Like _bus_dmamap_load(), but for mbufs.
 */
int
mbus_dmamap_load_mbuf(void *v, bus_dmamap_t map, struct mbuf *m0,
    int flags)
{
	vaddr_t lastaddr;
	int seg, error, first;
	struct mbuf *m;

	/*
	 * Make sure that on error condition we return "no valid mappings."
	 */
	map->dm_mapsize = 0;
	map->dm_nsegs = 0;

#ifdef DIAGNOSTIC
	if ((m0->m_flags & M_PKTHDR) == 0)
		panic("_bus_dmamap_load_mbuf: no packet header");
#endif	/* DIAGNOSTIC */

	if (m0->m_pkthdr.len > map->_dm_size)
		return (EINVAL);

	first = 1;
	seg = 0;
	error = 0;
	for (m = m0; m != NULL && error == 0; m = m->m_next) {
		if (m->m_len == 0)
			continue;
		error = _bus_dmamap_load_buffer(NULL, map, m->m_data, m->m_len,
		    vmspace_kernel(), flags, &lastaddr, &seg, first);
		first = 0;
	}
	if (error == 0) {
		map->dm_mapsize = m0->m_pkthdr.len;
		map->dm_nsegs = seg + 1;
	}
	return (error);
}

/*
 * Like _bus_dmamap_load(), but for uios.
 */
int
mbus_dmamap_load_uio(void *v, bus_dmamap_t map, struct uio *uio,
    int flags)
{
	vaddr_t lastaddr;
	int seg, i, error, first;
	bus_size_t minlen, resid;
	struct iovec *iov;
	void *addr;

	/*
	 * Make sure that on error condition we return "no valid mappings."
	 */
	map->dm_mapsize = 0;
	map->dm_nsegs = 0;

	resid = uio->uio_resid;
	iov = uio->uio_iov;

	first = 1;
	seg = 0;
	error = 0;
	for (i = 0; i < uio->uio_iovcnt && resid != 0 && error == 0; i++) {
		/*
		 * Now at the first iovec to load.  Load each iovec
		 * until we have exhausted the residual count.
		 */
		minlen = MIN(resid, iov[i].iov_len);
		addr = (void *)iov[i].iov_base;

		error = _bus_dmamap_load_buffer(NULL, map, addr, minlen,
		    uio->uio_vmspace, flags, &lastaddr, &seg, first);
		first = 0;

		resid -= minlen;
	}
	if (error == 0) {
		map->dm_mapsize = uio->uio_resid;
		map->dm_nsegs = seg + 1;
	}
	return (error);
}

/*
 * Like bus_dmamap_load(), but for raw memory allocated with
 * bus_dmamem_alloc().
 */
int
mbus_dmamap_load_raw(void *v, bus_dmamap_t map, bus_dma_segment_t *segs,
    int nsegs, bus_size_t size, int flags)
{
	struct pglist *mlist;
	struct vm_page *m;
	paddr_t pa, pa_next;
	bus_size_t mapsize;
	bus_size_t pagesz = PAGE_SIZE;
	int seg;

	/*
	 * Make sure that on error condition we return "no valid mappings".
	 */
	map->dm_nsegs = 0;
	map->dm_mapsize = 0;

	/* Load the allocated pages. */
	mlist = segs[0]._ds_mlist;
	pa_next = 0;
	seg = -1;
	mapsize = size;
	for (m = TAILQ_FIRST(mlist); m != NULL; m = TAILQ_NEXT(m,pageq.queue)) {

		if (size == 0)
			panic("mbus_dmamem_load_raw: size botch");

		pa = VM_PAGE_TO_PHYS(m);
		if (pa != pa_next) {
			if (++seg >= map->_dm_segcnt)
				panic("mbus_dmamem_load_raw: nsegs botch");
			map->dm_segs[seg].ds_addr = pa;
			map->dm_segs[seg].ds_len = 0;
		}
		pa_next = pa + PAGE_SIZE;
		if (size < pagesz)
			pagesz = size;
		map->dm_segs[seg].ds_len += pagesz;
		size -= pagesz;
	}

	/* Make the map truly valid. */
	map->dm_nsegs = seg + 1;
	map->dm_mapsize = mapsize;

	return (0);
}

/*
 * unload a DMA map.
 */
void
mbus_dmamap_unload(void *v, bus_dmamap_t map)
{
	/*
	 * If this map was loaded with mbus_dmamap_load,
	 * we don't need to do anything.  If this map was
	 * loaded with mbus_dmamap_load_raw, we also don't
	 * need to do anything.
	 */

	/* Mark the mappings as invalid. */
	map->dm_mapsize = 0;
	map->dm_nsegs = 0;
}

void
mbus_dmamap_sync(void *v, bus_dmamap_t map, bus_addr_t offset, bus_size_t len,
    int ops)
{
	int i;
	/*
	 * Mixing of PRE and POST operations is not allowed.
	 */
	if ((ops & (BUS_DMASYNC_PREREAD|BUS_DMASYNC_PREWRITE)) != 0 &&
	    (ops & (BUS_DMASYNC_POSTREAD|BUS_DMASYNC_POSTWRITE)) != 0)
		panic("mbus_dmamap_sync: mix PRE and POST");

#ifdef DIAGNOSTIC
	if (offset >= map->dm_mapsize)
		panic("mbus_dmamap_sync: bad offset %lu (map size is %lu)",
		    offset, map->dm_mapsize);
	if (len == 0 || (offset + len) > map->dm_mapsize)
		panic("mbus_dmamap_sync: bad length");
#endif

	/*
	 * For a virtually-indexed write-back cache, we need
	 * to do the following things:
	 *
	 *	PREREAD -- Invalidate the D-cache.  We do this
	 *	here in case a write-back is required by the back-end.
	 *
	 *	PREWRITE -- Write-back the D-cache.  Note that if
	 *	we are doing a PREREAD|PREWRITE, we can collapse
	 *	the whole thing into a single Wb-Inv.
	 *
	 *	POSTREAD -- Nothing.
	 *
	 *	POSTWRITE -- Nothing.
	 */

	ops &= (BUS_DMASYNC_PREREAD|BUS_DMASYNC_PREWRITE);
	if (ops == 0)
		return;

	for (i = 0; len != 0 && i < map->dm_nsegs; i++) {
		if (offset >= map->dm_segs[i].ds_len)
			offset -= map->dm_segs[i].ds_len;
		else {
			bus_size_t l = map->dm_segs[i].ds_len - offset;

			if (l > len)
				l = len;

			fdcache(HPPA_SID_KERNEL, map->dm_segs[i]._ds_va +
			    offset, l);
			len -= l;
			offset = 0;
		}
	}

 	/* for either operation sync the shit away */
	__asm __volatile ("sync\n\tsyncdma\n\tsync\n\t"
	    "nop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop" ::: "memory");
}

/*
 * Common function for DMA-safe memory allocation.  May be called
 * by bus-specific DMA memory allocation functions.
 */
int
mbus_dmamem_alloc(void *v, bus_size_t size, bus_size_t alignment,
    bus_size_t boundary, bus_dma_segment_t *segs, int nsegs, int *rsegs,
    int flags)
{
	paddr_t low, high;
	struct pglist *mlist;
	struct vm_page *m;
	paddr_t pa, pa_next;
	int seg;
	int error;

	/* Always round the size. */
	size = round_page(size);

	/* Decide where we can allocate pages. */
	low = 0;
	high = ((flags & BUS_DMA_24BIT) ? (1 << 24) : 0) - 1;

	if ((mlist = malloc(sizeof(*mlist), M_DEVBUF,
	    (flags & BUS_DMA_NOWAIT) ? M_NOWAIT : M_WAITOK)) == NULL)
		return (ENOMEM);

	/*
	 * Allocate physical pages from the VM system.
	 */
	TAILQ_INIT(mlist);
	error = uvm_pglistalloc(size, low, high, 0, 0,
				mlist, nsegs, (flags & BUS_DMA_NOWAIT) == 0);

	/*
	 * If the allocation failed, and this is a 24-bit
	 * device, see if we have space left in the 24-bit
	 * region.
	 */
	if (error == ENOMEM && (flags & BUS_DMA_24BIT) && dma24_ex != NULL) {
		error = extent_alloc(dma24_ex, size, alignment, 0, 0, &pa);
		if (!error) {
			free(mlist, M_DEVBUF);
			/*
			 * A _ds_mlist value of NULL is the
			 * signal to mbus_dmamem_map that no
			 * real mapping needs to be done, and
			 * it is the signal to mbus_dmamem_free
			 * that an extent_free is needed.
			 */
			*rsegs = 1;
			segs[0].ds_addr = 0;
			segs[0].ds_len = size;
			segs[0]._ds_va = (vaddr_t)pa;
			segs[0]._ds_mlist = NULL;
			return (0);
		}
	}

	/* If we don't have the pages. */
	if (error) {
		free(mlist, M_DEVBUF);
		return (error);
	}

	/*
	 * Since, at least as of revision 1.17 of uvm_pglist.c,
	 * uvm_pglistalloc ignores its nsegs argument, we need
	 * to check that the pages returned conform to the
	 * caller's segment requirements.
	 */
	pa_next = 0;
	seg = -1;
	for (m = TAILQ_FIRST(mlist); m != NULL; m = TAILQ_NEXT(m,pageq.queue)) {
		pa = VM_PAGE_TO_PHYS(m);
		if (pa != pa_next) {
			if (++seg >= nsegs) {
				uvm_pglistfree(mlist);
				free(mlist, M_DEVBUF);
				return (ENOMEM);
			}
			segs[seg].ds_addr = 0;
			segs[seg].ds_len = 0;
			segs[seg]._ds_va = 0;
		}
		pa_next = pa + PAGE_SIZE;
	}
	*rsegs = seg + 1;

	/*
	 * Simply keep a pointer around to the linked list, so
	 * bus_dmamap_free() can return it.
	 *
	 * NOBODY SHOULD TOUCH THE pageq.queue FIELDS WHILE THESE PAGES
	 * ARE IN OUR CUSTODY.
	 */
	segs[0]._ds_mlist = mlist;

	/*
	 * We now have physical pages, but no kernel virtual addresses
	 * yet. These may be allocated in bus_dmamap_map.  Hence we
	 * save any alignment and boundary requirements in this DMA
	 * segment.
	 */
	return (0);
}

void
mbus_dmamem_free(void *v, bus_dma_segment_t *segs, int nsegs)
{

	/*
	 * Return the list of physical pages back to the VM system.
	 */
	if (segs[0]._ds_mlist != NULL) {
		uvm_pglistfree(segs[0]._ds_mlist);
		free(segs[0]._ds_mlist, M_DEVBUF);
	} else {
		extent_free(dma24_ex, segs[0]._ds_va, segs[0].ds_len,
				EX_NOWAIT);
	}
}

/*
 * Common function for mapping DMA-safe memory.  May be called by
 * bus-specific DMA memory map functions.
 */
int
mbus_dmamem_map(void *v, bus_dma_segment_t *segs, int nsegs, size_t size,
    void **kvap, int flags)
{
	struct vm_page *pg;
	struct pglist *pglist;
	vaddr_t va;
	paddr_t pa;
	const uvm_flag_t kmflags =
	    (flags & BUS_DMA_NOWAIT) != 0 ? UVM_KMF_NOWAIT : 0;

	size = round_page(size);

	/* 24-bit memory needs no mapping. */
	if (segs[0]._ds_mlist == NULL) {
		if (size > segs[0].ds_len)
			panic("mbus_dmamem_map: size botch");
		*kvap = (void *)segs[0]._ds_va;
		return (0);
	}

	/* Get a chunk of kernel virtual space. */
	va = uvm_km_alloc(kernel_map, size, 0, UVM_KMF_VAONLY | kmflags);
	if (va == 0)
		return (ENOMEM);

	/* Stash that in the first segment. */
	segs[0]._ds_va = va;
	*kvap = (void *)va;

	/* Map the allocated pages into the chunk. */
	pglist = segs[0]._ds_mlist;
	TAILQ_FOREACH(pg, pglist, pageq.queue) {
		KASSERT(size != 0);
		pa = VM_PAGE_TO_PHYS(pg);
		pmap_kenter_pa(va, pa, VM_PROT_READ | VM_PROT_WRITE | PMAP_NC);
		va += PAGE_SIZE;
		size -= PAGE_SIZE;
	}
	pmap_update();
	return (0);
}

/*
 * Common function for unmapping DMA-safe memory.  May be called by
 * bus-specific DMA memory unmapping functions.
 */
void
mbus_dmamem_unmap(void *v, void *kva, size_t size)
{

#ifdef DIAGNOSTIC
	if ((u_long)kva & PAGE_MASK)
		panic("mbus_dmamem_unmap");
#endif

	/*
	 * XXX fredette - this is gross, but it is needed
	 * to support the 24-bit DMA address stuff.
	 */
	if (dma24_ex != NULL && kva < (void *) (1 << 24))
		return;

	size = round_page(size);
	pmap_kremove((vaddr_t)kva, size);
	pmap_update(pmap_kernel());
	uvm_km_free(kernel_map, (vaddr_t)kva, size, UVM_KMF_VAONLY);
}

/*
 * Common functin for mmap(2)'ing DMA-safe memory.  May be called by
 * bus-specific DMA mmap(2)'ing functions.
 */
paddr_t
mbus_dmamem_mmap(void *v, bus_dma_segment_t *segs, int nsegs,
	off_t off, int prot, int flags)
{
	int i;

	for (i = 0; i < nsegs; i++) {
#ifdef DIAGNOSTIC
		if (off & PGOFSET)
			panic("_bus_dmamem_mmap: offset unaligned");
		if (segs[i].ds_addr & PGOFSET)
			panic("_bus_dmamem_mmap: segment unaligned");
		if (segs[i].ds_len & PGOFSET)
			panic("_bus_dmamem_mmap: segment size not multiple"
			    " of page size");
#endif	/* DIAGNOSTIC */
		if (off >= segs[i].ds_len) {
			off -= segs[i].ds_len;
			continue;
		}

		return (btop((u_long)segs[i].ds_addr + off));
	}

	/* Page not found. */
	return (-1);
}

int
_bus_dmamap_load_buffer(bus_dma_tag_t t, bus_dmamap_t map, void *buf,
    bus_size_t buflen, struct vmspace *vm, int flags, paddr_t *lastaddrp,
    int *segp, int first)
{
	bus_size_t sgsize;
	bus_addr_t curaddr, lastaddr, baddr, bmask;
	vaddr_t vaddr = (vaddr_t)buf;
	int seg;
	pmap_t pmap;

	pmap = vm_map_pmap(&vm->vm_map);

	lastaddr = *lastaddrp;
	bmask  = ~(map->_dm_boundary - 1);

	for (seg = *segp; buflen > 0; ) {
		bool ok;
		/*
		 * Get the physical address for this segment.
		 */
		ok = pmap_extract(pmap, vaddr, &curaddr);
		KASSERT(ok == true);

		/*
		 * Compute the segment size, and adjust counts.
		 */
		sgsize = PAGE_SIZE - ((u_long)vaddr & PGOFSET);
		if (buflen < sgsize)
			sgsize = buflen;

		/*
		 * Make sure we don't cross any boundaries.
		 */
		if (map->_dm_boundary > 0) {
			baddr = (curaddr + map->_dm_boundary) & bmask;
			if (sgsize > (baddr - curaddr))
				sgsize = (baddr - curaddr);
		}

		/*
		 * Insert chunk into a segment, coalescing with
		 * previous segment if possible.
		 */
		if (first) {
			map->dm_segs[seg].ds_addr = curaddr;
			map->dm_segs[seg].ds_len = sgsize;
			map->dm_segs[seg]._ds_va = vaddr;
			first = 0;
		} else {
			if (curaddr == lastaddr &&
			    (map->dm_segs[seg].ds_len + sgsize) <=
			     map->_dm_maxsegsz &&
			    (map->_dm_boundary == 0 ||
			     (map->dm_segs[seg].ds_addr & bmask) ==
			     (curaddr & bmask)))
				map->dm_segs[seg].ds_len += sgsize;
			else {
				if (++seg >= map->_dm_segcnt)
					break;
				map->dm_segs[seg].ds_addr = curaddr;
				map->dm_segs[seg].ds_len = sgsize;
				map->dm_segs[seg]._ds_va = vaddr;
			}
		}

		lastaddr = curaddr + sgsize;
		vaddr += sgsize;
		buflen -= sgsize;
	}

	*segp = seg;
	*lastaddrp = lastaddr;

	/*
	 * Did we fit?
	 */
	if (buflen != 0)
		return (EFBIG);		/* XXX better return value here? */
	return (0);
}

const struct hppa_bus_dma_tag hppa_dmatag = {
	NULL,
	mbus_dmamap_create, mbus_dmamap_destroy,
	mbus_dmamap_load, mbus_dmamap_load_mbuf,
	mbus_dmamap_load_uio, mbus_dmamap_load_raw,
	mbus_dmamap_unload, mbus_dmamap_sync,

	mbus_dmamem_alloc, mbus_dmamem_free, mbus_dmamem_map,
	mbus_dmamem_unmap, mbus_dmamem_mmap
};

int
mbmatch(device_t parent, cfdata_t cf, void *aux)
{

	/* there will be only one */
	if (mb_attached)
		return 0;

	return 1;
}

static void
mb_module_callback(device_t self, struct confargs *ca)
{
	if (ca->ca_type.iodc_type == HPPA_TYPE_NPROC ||
	    ca->ca_type.iodc_type == HPPA_TYPE_MEMORY)
		return;
	config_found_sm_loc(self, "gedoens", NULL, ca, mbprint, mbsubmatch);
}

static void
mb_cpu_mem_callback(device_t self, struct confargs *ca)
{
	if ((ca->ca_type.iodc_type == HPPA_TYPE_NPROC ||
	     ca->ca_type.iodc_type == HPPA_TYPE_MEMORY))
		config_found_sm_loc(self, "gedoens", NULL, ca, mbprint,
		    mbsubmatch);
}

void
mbattach(device_t parent, device_t self, void *aux)
{
	struct mainbus_softc *sc = device_private(self);
	struct confargs nca;
	bus_space_handle_t ioh;
	hppa_hpa_t hpabase;

	sc->sc_dv = self;

	mb_attached = 1;

	/* fetch the "default" cpu hpa */
	if (pdc_call((iodcio_t)pdc, 0, PDC_HPA, PDC_HPA_DFLT, &pdc_hpa) < 0)
		panic("mbattach: PDC_HPA failed");

	/*
	 * Map all of Fixed Physical, Local Broadcast, and
	 * Global Broadcast space.  These spaces are adjacent
	 * and in that order and run to the end of the address
	 * space.
	 */
	/*
	 * XXX fredette - this may be a copout, or it may
 	 * be a great idea.  I'm not sure which yet.
	 */
	if (bus_space_map(&hppa_bustag, pdc_hpa.hpa, 0 - pdc_hpa.hpa, 0, &ioh))
		panic("mbattach: can't map mainbus IO space");

	/*
	 * Local-Broadcast the HPA to all modules on the bus
	 */
	((struct iomod *)(pdc_hpa.hpa & FLEX_MASK))[FPA_IOMOD].io_flex =
		(void *)((pdc_hpa.hpa & FLEX_MASK) | DMA_ENABLE);

	sc->sc_hpa = pdc_hpa.hpa;
	aprint_normal(" [flex %lx]\n", pdc_hpa.hpa & FLEX_MASK);

	/* PDC first */
	memset(&nca, 0, sizeof(nca));
	nca.ca_name = "pdc";
	nca.ca_hpa = 0;
	nca.ca_iot = &hppa_bustag;
	nca.ca_dmatag = &hppa_dmatag;
	config_found(self, &nca, mbprint);

#if NPOWER > 0
	/* get some power */
	memset(&nca, 0, sizeof(nca));
	nca.ca_name = "power";
	nca.ca_irq = -1;
	nca.ca_iot = &hppa_bustag;
	config_found(self, &nca, mbprint);
#endif

	switch (cpu_hvers) {
	case HPPA_BOARD_HP809:
	case HPPA_BOARD_HP819:
	case HPPA_BOARD_HP829:
	case HPPA_BOARD_HP839:
	case HPPA_BOARD_HP849:
	case HPPA_BOARD_HP859:
	case HPPA_BOARD_HP869:
#if 0
	case HPPA_BOARD_HP770_J200:
	case HPPA_BOARD_HP770_J210:
	case HPPA_BOARD_HP770_J210XC:
	case HPPA_BOARD_HP780_J282:
	case HPPA_BOARD_HP782_J2240:
#endif
	case HPPA_BOARD_HP780_C160:
	case HPPA_BOARD_HP780_C180P:
	case HPPA_BOARD_HP780_C180XP:
	case HPPA_BOARD_HP780_C200:
	case HPPA_BOARD_HP780_C230:
	case HPPA_BOARD_HP780_C240:
	case HPPA_BOARD_HP785_C360:

	case HPPA_BOARD_HP800D:
		hpabase = HPPA_FPA;
		break;
	default:
		hpabase = 0;
		break;
	}

	/* Search and attach all CPUs and memory controllers. */
	memset(&nca, 0, sizeof(nca));
	nca.ca_name = "mainbus";
	nca.ca_hpa = 0;
	nca.ca_hpabase = hpabase;
	nca.ca_nmodules = MAXMODBUS;
	nca.ca_irq = HP700CF_IRQ_UNDEF;
	nca.ca_iot = &hppa_bustag;
	nca.ca_dmatag = &hppa_dmatag;
	nca.ca_dp.dp_bc[0] = nca.ca_dp.dp_bc[1] = nca.ca_dp.dp_bc[2] =
	nca.ca_dp.dp_bc[3] = nca.ca_dp.dp_bc[4] = nca.ca_dp.dp_bc[5] = -1;
	nca.ca_dp.dp_mod = -1;
	pdc_scanbus(self, &nca, mb_cpu_mem_callback);

	/* Search for IO hardware. */
	memset(&nca, 0, sizeof(nca));
	nca.ca_name = "mainbus";
	nca.ca_hpa = 0;
	nca.ca_hpabase = hpabase;
	nca.ca_nmodules = MAXMODBUS;
	nca.ca_irq = HP700CF_IRQ_UNDEF;
	nca.ca_iot = &hppa_bustag;
	nca.ca_dmatag = &hppa_dmatag;
	nca.ca_dp.dp_bc[0] = nca.ca_dp.dp_bc[1] = nca.ca_dp.dp_bc[2] =
	nca.ca_dp.dp_bc[3] = nca.ca_dp.dp_bc[4] = nca.ca_dp.dp_bc[5] = -1;
	nca.ca_dp.dp_mod = -1;
	pdc_scanbus(self, &nca, mb_module_callback);
}

/*
 * retrive CPU #N HPA value
 */
hppa_hpa_t
cpu_gethpa(int n)
{
	struct mainbus_softc *sc;

	sc = device_lookup_private(&mainbus_cd, 0);

	return sc->sc_hpa;
}

int
mbprint(void *aux, const char *pnp)
{
	int n;
	struct confargs *ca = aux;

	if (pnp)
		aprint_normal("\"%s\" at %s (type 0x%x, sv 0x%x)", ca->ca_name,
		    pnp, ca->ca_type.iodc_type, ca->ca_type.iodc_sv_model);
	if (ca->ca_hpa) {
		aprint_normal(" hpa 0x%lx path ", ca->ca_hpa);
		for (n = 0 ; n < 6 ; n++) {
			if ( ca->ca_dp.dp_bc[n] >= 0)
				printf( "%d/", ca->ca_dp.dp_bc[n]);
		}
		aprint_normal( "%d", ca->ca_dp.dp_mod);
		if (!pnp && ca->ca_irq >= 0) {
			aprint_normal(" irq %d", ca->ca_irq);
			if (ca->ca_type.iodc_type != HPPA_TYPE_BHA)
				aprint_normal(" ipl %d",
				    _hp700_intr_ipl_next());
		}
	}

	return (UNCONF);
}

int
mbsubmatch(device_t parent, cfdata_t cf, const int *ldesc, void *aux)
{
	struct confargs *ca = aux;
	int ret;
	int saved_irq;

	saved_irq = ca->ca_irq;
	if (cf->hp700cf_irq != HP700CF_IRQ_UNDEF)
		ca->ca_irq = cf->hp700cf_irq;
	if (!(ret = config_match(parent, cf, aux)))
		ca->ca_irq = saved_irq;
	return ret;
}
