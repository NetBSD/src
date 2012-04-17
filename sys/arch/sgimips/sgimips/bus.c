/*	$NetBSD: bus.c,v 1.63.2.1 2012/04/17 00:06:52 yamt Exp $	*/

/*
 * Copyright (c) 1998 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe of the Numerical Aerospace Simulation Facility,
 * NASA Ames Research Center.
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
__KERNEL_RCSID(0, "$NetBSD: bus.c,v 1.63.2.1 2012/04/17 00:06:52 yamt Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/endian.h>
#include <sys/bswap.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <sys/malloc.h>
#include <sys/proc.h>
#include <sys/mbuf.h>

#define _SGIMIPS_BUS_DMA_PRIVATE
#include <sys/bus.h>
#include <machine/cpu.h>
#include <machine/machtype.h>

#include <common/bus_dma/bus_dmamem_common.h>

#include <uvm/uvm_extern.h>

#include <mips/cpuregs.h>
#include <mips/locore.h>
#include <mips/cache.h>

#include <sgimips/mace/macereg.h>

#include "opt_sgimace.h"

static int	_bus_dmamap_load_buffer(bus_dmamap_t, void *, bus_size_t,
				struct vmspace *, int, vaddr_t *, int *, int);

struct sgimips_bus_dma_tag sgimips_default_bus_dma_tag = {
	_bus_dmamap_create,
	_bus_dmamap_destroy,
	_bus_dmamap_load,
	_bus_dmamap_load_mbuf,
	_bus_dmamap_load_uio,
	_bus_dmamap_load_raw,
	_bus_dmamap_unload,
	NULL,	
	_bus_dmamem_alloc,
	_bus_dmamem_free,
	_bus_dmamem_map,
	_bus_dmamem_unmap,
	_bus_dmamem_mmap,
};

void
sgimips_bus_dma_init(void)
{
	switch (mach_type) {
	/* R2000/R3000 */
	case MACH_SGI_IP6 | MACH_SGI_IP10:
	case MACH_SGI_IP12:
		sgimips_default_bus_dma_tag._dmamap_sync =
		    _bus_dmamap_sync_mips1;
		break;

	/* >=R4000*/
	case MACH_SGI_IP20:
	case MACH_SGI_IP22:
	case MACH_SGI_IP30:
	case MACH_SGI_IP32:
		sgimips_default_bus_dma_tag._dmamap_sync =
		    _bus_dmamap_sync_mips3;
		break;

	default:
		panic("sgimips_bus_dma_init: unsupported mach type IP%d\n",
		    mach_type);
	}
}

u_int8_t
bus_space_read_1(bus_space_tag_t t, bus_space_handle_t h, bus_size_t o)
{
	wbflush(); /* XXX ? */

	switch (t) {
	case SGIMIPS_BUS_SPACE_NORMAL:
		return *(volatile u_int8_t *)(vaddr_t)(h + o);
	case SGIMIPS_BUS_SPACE_IP6_DPCLOCK:
		return *(volatile u_int8_t *)(vaddr_t)(h + (o << 2));
	case SGIMIPS_BUS_SPACE_HPC:
		return *(volatile u_int8_t *)(vaddr_t)(h + (o << 2) + 3);
	case SGIMIPS_BUS_SPACE_MEM:
	case SGIMIPS_BUS_SPACE_IO:
		return *(volatile u_int8_t *)(vaddr_t)(h + (o | 3) - (o & 3));
	case SGIMIPS_BUS_SPACE_MACE:
		return *(volatile u_int8_t *)(vaddr_t)(h + (o << 8) + 7);
	default:
		panic("no bus tag");
	}
}

void
bus_space_write_1(bus_space_tag_t t, bus_space_handle_t h, bus_size_t o, u_int8_t v)
{
	switch (t) {
	case SGIMIPS_BUS_SPACE_NORMAL:
		*(volatile u_int8_t *)(vaddr_t)(h + o) = v;
		break;
	case SGIMIPS_BUS_SPACE_IP6_DPCLOCK:
		*(volatile u_int8_t *)(vaddr_t)(h + (o << 2)) = v;
		break;
	case SGIMIPS_BUS_SPACE_HPC:
		*(volatile u_int8_t *)(vaddr_t)(h + (o << 2) + 3) = v;
		break;
	case SGIMIPS_BUS_SPACE_MEM:
	case SGIMIPS_BUS_SPACE_IO:
		*(volatile u_int8_t *)(vaddr_t)(h + (o | 3) - (o & 3)) = v;
		break;
	case SGIMIPS_BUS_SPACE_MACE:
		*(volatile u_int8_t *)(vaddr_t)(h + (o << 8) + 7) = v;
		break;
	default:
		panic("no bus tag");
	}

	wbflush();	/* XXX */
}

u_int16_t
bus_space_read_2(bus_space_tag_t t, bus_space_handle_t h, bus_size_t o)
{
	wbflush(); /* XXX ? */

	switch (t) {
	case SGIMIPS_BUS_SPACE_NORMAL:
		return *(volatile u_int16_t *)(vaddr_t)(h + o);
	case SGIMIPS_BUS_SPACE_HPC:
		return *(volatile u_int16_t *)(vaddr_t)(h + (o << 2) + 1);
	case SGIMIPS_BUS_SPACE_MEM:
	case SGIMIPS_BUS_SPACE_IO:
		return *(volatile u_int16_t *)(vaddr_t)(h + (o | 2) - (o & 3));
	default:
		panic("no bus tag");
	}
}

void
bus_space_write_2(bus_space_tag_t t, bus_space_handle_t h, bus_size_t o, u_int16_t v)
{
	switch (t) {
	case SGIMIPS_BUS_SPACE_NORMAL:
		*(volatile u_int16_t *)(vaddr_t)(h + o) = v;
		break;
	case SGIMIPS_BUS_SPACE_HPC:
		*(volatile u_int16_t *)(vaddr_t)(h + (o << 2) + 1) = v;
		break;
	case SGIMIPS_BUS_SPACE_MEM:
	case SGIMIPS_BUS_SPACE_IO:
		*(volatile u_int16_t *)(vaddr_t)(h + (o | 2) - (o & 3)) = v;
		break;
	default:
		panic("no bus tag");
	}

	wbflush();	/* XXX */
}

u_int32_t
bus_space_read_4(bus_space_tag_t tag, bus_space_handle_t bsh, bus_size_t o)
{
	u_int32_t reg;
#ifdef MACE_NEEDS_DELAYS
	int s;
#endif

	switch (tag) {
		case SGIMIPS_BUS_SPACE_MACE:
#ifdef MACE_NEEDS_DELAYS
			s = splhigh();
			delay(10);
#endif
			wbflush();
			reg = (*(volatile u_int32_t *)(vaddr_t)(bsh + o));
#ifdef MACE_NEEDS_DELAYS
			delay(10);
			splx(s);
#endif
			break;
		default:
			wbflush();
			reg = (*(volatile u_int32_t *)(vaddr_t)(bsh + o));
			break;
	}
	return reg;
}


void
bus_space_write_4(bus_space_tag_t tag, bus_space_handle_t bsh,
	bus_size_t o, u_int32_t v)
{
#ifdef MACE_NEEDS_DELAYS
	int s;
#endif

	switch (tag) {
		case SGIMIPS_BUS_SPACE_MACE:
#ifdef MACE_NEEDS_DELAYS
			s = splhigh();
			delay(10);
#endif
			*(volatile u_int32_t *)(vaddr_t)((bsh) + (o)) = (v);
			wbflush();
#ifdef MACE_NEEDS_DELAYS
			delay(10);
			splx(s);
#endif
			break;
		default:
			*(volatile u_int32_t *)(vaddr_t)((bsh) + (o)) = (v);
			wbflush(); /* XXX */
			break;
	}
}

u_int16_t
bus_space_read_stream_2(bus_space_tag_t t, bus_space_handle_t h,
	bus_size_t o)
{
	u_int16_t v;
	wbflush(); /* XXX ? */

	switch (t) {
	case SGIMIPS_BUS_SPACE_NORMAL:
		return *(volatile u_int16_t *)(vaddr_t)(h + o);
	case SGIMIPS_BUS_SPACE_HPC:
		return *(volatile u_int16_t *)(vaddr_t)(h + (o << 2) + 1);
	case SGIMIPS_BUS_SPACE_MEM:
	case SGIMIPS_BUS_SPACE_IO:
		v = *(volatile u_int16_t *)(vaddr_t)(h + (o | 2) - (o & 3));
		return htole16(v);
	default:
		panic("no bus tag");
	}
}

u_int32_t
bus_space_read_stream_4(bus_space_tag_t t, bus_space_handle_t bsh,
	bus_size_t o)
{
	u_int32_t reg;
#ifdef MACE_NEEDS_DELAYS
	int s;
#endif

	switch (t) {
		case SGIMIPS_BUS_SPACE_MACE:
#ifdef MACE_NEEDS_DELAYS
			s = splhigh();
			delay(10);
#endif
			wbflush();
			reg = (*(volatile u_int32_t *)(vaddr_t)(bsh + o));
#ifdef MACE_NEEDS_DELAYS
			delay(10);
			splx(s);
#endif
			break;
		case SGIMIPS_BUS_SPACE_MEM:
		case SGIMIPS_BUS_SPACE_IO:
			wbflush();
			reg = (*(volatile u_int32_t *)(vaddr_t)(bsh + o));
			reg = htole32(reg);
			break;
		default:
			wbflush();
			reg = (*(volatile u_int32_t *)(vaddr_t)(bsh + o));
			break;
	}
	return reg;
}

void
bus_space_write_stream_2(bus_space_tag_t t, bus_space_handle_t h,
	bus_size_t o, u_int16_t v)
{
	switch (t) {
	case SGIMIPS_BUS_SPACE_NORMAL:
		*(volatile u_int16_t *)(vaddr_t)(h + o) = v;
		break;
	case SGIMIPS_BUS_SPACE_HPC:
		*(volatile u_int16_t *)(vaddr_t)(h + (o << 2) + 1) = v;
		break;
	case SGIMIPS_BUS_SPACE_MEM:
	case SGIMIPS_BUS_SPACE_IO:
		v = le16toh(v);
		*(volatile u_int16_t *)(vaddr_t)(h + (o | 2) - (o & 3)) = v;
		break;
	default:
		panic("no bus tag");
	}

	wbflush();	/* XXX */
}

void
bus_space_write_stream_4(bus_space_tag_t tag, bus_space_handle_t bsh,
	bus_size_t o, u_int32_t v)
{
#ifdef MACE_NEEDS_DELAYS
	int s;
#endif

	switch (tag) {
		case SGIMIPS_BUS_SPACE_MACE:
#ifdef MACE_NEEDS_DELAYS
			s = splhigh();
			delay(10);
#endif
			*(volatile u_int32_t *)(vaddr_t)((bsh) + (o)) = (v);
			wbflush();
#ifdef MACE_NEEDS_DELAYS
			delay(10);
			splx(s);
#endif
			break;
		case SGIMIPS_BUS_SPACE_IO:
		case SGIMIPS_BUS_SPACE_MEM:
			v = le32toh(v);
			*(volatile u_int32_t *)(vaddr_t)((bsh) + (o)) = (v);
			wbflush(); /* XXX */
			break;
		default:
			*(volatile u_int32_t *)(vaddr_t)((bsh) + (o)) = (v);
			wbflush(); /* XXX */
			break;
	}
}

#if defined(MIPS3)
u_int64_t
bus_space_read_8(bus_space_tag_t tag, bus_space_handle_t bsh, bus_size_t o)
{
	u_int64_t reg;
#ifdef MACE_NEEDS_DELAYS
	int s;
#endif

	/* see if we're properly aligned */
	KASSERT((o & 7) == 0);

	switch (tag) {
		case SGIMIPS_BUS_SPACE_MACE:
#ifdef MACE_NEEDS_DELAYS
			s = splhigh();
			delay(10);
#endif
			reg = mips3_ld((volatile uint64_t *)(vaddr_t)(bsh + o));
#ifdef MACE_NEEDS_DELAYS
			delay(10);
			splx(s);
#endif
			break;
		default:
			reg = mips3_ld((volatile uint64_t *)(vaddr_t)(bsh + o));
			break;
	}
	return reg;
}

void
bus_space_write_8(bus_space_tag_t tag, bus_space_handle_t bsh, bus_size_t o, u_int64_t v)
{
#ifdef MACE_NEEDS_DELAYS
	int s;
#endif

	/* see if we're properly aligned */
	KASSERT((o & 7) == 0);

	switch (tag) {
		case SGIMIPS_BUS_SPACE_MACE:
#ifdef MACE_NEEDS_DELAYS
			s = splhigh();
			delay(10);
#endif
			mips3_sd((volatile uint64_t *)(vaddr_t)(bsh + o), v);
#ifdef MACE_NEEDS_DELAYS
			delay(10);
			splx(s);
#endif
			break;
		default:
			mips3_sd((volatile uint64_t *)(vaddr_t)(bsh + o), v);
			break;
	}
}
#endif /* MIPS3 */

int
bus_space_map(bus_space_tag_t t, bus_addr_t bpa, bus_size_t size,
	      int flags, bus_space_handle_t *bshp)
{
	int cacheable = flags & BUS_SPACE_MAP_CACHEABLE;

	if (cacheable)
		*bshp = MIPS_PHYS_TO_KSEG0(bpa);
	else
		*bshp = MIPS_PHYS_TO_KSEG1(bpa);

/*
 * XXX
 */
	/* XXX O2 */
	if (bpa > 0x80000000 && bpa < 0x82000000)
		*bshp = MIPS_PHYS_TO_KSEG1(MACE_PCI_LOW_MEMORY +
		    (bpa & 0xfffffff));
	if (bpa < 0x00010000)
		*bshp = MIPS_PHYS_TO_KSEG1(MACE_PCI_LOW_IO + bpa);

	return 0;
}

int
bus_space_alloc(bus_space_tag_t t, bus_addr_t rstart, bus_addr_t rend,
		bus_size_t size, bus_size_t alignment, bus_size_t boundary,
		int flags, bus_addr_t *bpap, bus_space_handle_t *bshp)
{
	panic("bus_space_alloc: not implemented");
}

void
bus_space_free(bus_space_tag_t t, bus_space_handle_t bsh, bus_size_t size)
{
	panic("bus_space_free: not implemented");
}

void
bus_space_unmap(bus_space_tag_t t, bus_space_handle_t bsh, bus_size_t size)
{
	return;
}

int
bus_space_subregion(bus_space_tag_t t, bus_space_handle_t bsh,
		    bus_size_t offset, bus_size_t size,
		    bus_space_handle_t *nbshp)
{

	*nbshp = bsh + offset;
	return 0;
}

void *
bus_space_vaddr(bus_space_tag_t t, bus_space_handle_t bsh)
{
	switch(t) {
	case SGIMIPS_BUS_SPACE_NORMAL:
		return ((void *)bsh);

	case SGIMIPS_BUS_SPACE_HPC:
		panic("bus_space_vaddr not supported on HPC space!");

	case SGIMIPS_BUS_SPACE_MEM:
		return ((void *)bsh);

	case SGIMIPS_BUS_SPACE_IO:
		panic("bus_space_vaddr not supported on I/O space!");

	default:
		panic("no bus tag");
	}
}

/*
 * Common function for DMA map creation.  May be called by bus-specific
 * DMA map creation functions.
 */
int
_bus_dmamap_create(bus_dma_tag_t t, bus_size_t size, int nsegments,
		   bus_size_t maxsegsz, bus_size_t boundary, int flags,
		   bus_dmamap_t *dmamp)
{
	struct sgimips_bus_dmamap *map;
	void *mapstore;
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
	mapsize = sizeof(struct sgimips_bus_dmamap) +
	    (sizeof(bus_dma_segment_t) * (nsegments - 1));
	if ((mapstore = malloc(mapsize, M_DMAMAP,
	    (flags & BUS_DMA_NOWAIT) ? M_NOWAIT : M_WAITOK)) == NULL)
		return ENOMEM;

	memset(mapstore, 0, mapsize);
	map = (struct sgimips_bus_dmamap *)mapstore;
	map->_dm_size = size;
	map->_dm_segcnt = nsegments;
	map->_dm_maxmaxsegsz = maxsegsz;
	map->_dm_boundary = boundary;
	map->_dm_flags = flags & ~(BUS_DMA_WAITOK|BUS_DMA_NOWAIT);
	map->_dm_vmspace = NULL;
	map->dm_maxsegsz = maxsegsz;
	map->dm_mapsize = 0;		/* no valid mappings */
	map->dm_nsegs = 0;

	*dmamp = map;
	return 0;
}

/*
 * Common function for DMA map destruction.  May be called by bus-specific
 * DMA map destruction functions.
 */
void
_bus_dmamap_destroy(bus_dma_tag_t t, bus_dmamap_t map)
{

	free(map, M_DMAMAP);
}
extern	paddr_t kvtophys(vaddr_t);		/* XXX */

/*
 * Utility function to load a linear buffer.  lastaddrp holds state
 * between invocations (for multiple-buffer loads).  segp contains
 * the starting segment on entrance, and the ending segment on exit.
 * first indicates if this is the first invocation of this function.
 */
int
_bus_dmamap_load_buffer(bus_dmamap_t map, void *buf, bus_size_t buflen,
			struct vmspace *vm, int flags, vaddr_t *lastaddrp,
			int *segp, int first)
{
	bus_size_t sgsize;
	bus_addr_t lastaddr, baddr, bmask;
	paddr_t curaddr;
	vaddr_t vaddr = (vaddr_t)buf;
	int seg;

	lastaddr = *lastaddrp;
	bmask = ~(map->_dm_boundary - 1);

	for (seg = *segp; buflen > 0 ; ) {
		/*
		 * Get the physical address for this segment.
		 */
		if (!VMSPACE_IS_KERNEL_P(vm))
			(void) pmap_extract(vm_map_pmap(&vm->vm_map),
			    vaddr, &curaddr);
		else
			curaddr = kvtophys(vaddr);

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
		 * the previous segment if possible.
		 */
		if (first) {
			map->dm_segs[seg].ds_addr = curaddr;
			map->dm_segs[seg].ds_len = sgsize;
			map->dm_segs[seg]._ds_vaddr = vaddr;
			first = 0;
		} else {
			if (curaddr == lastaddr &&
			    (map->dm_segs[seg].ds_len + sgsize) <=
			     map->dm_maxsegsz &&
			    (map->_dm_boundary == 0 ||
			     (map->dm_segs[seg].ds_addr & bmask) ==
			     (curaddr & bmask)))
				map->dm_segs[seg].ds_len += sgsize;
			else {
				if (++seg >= map->_dm_segcnt)
					break;
				map->dm_segs[seg].ds_addr = curaddr;
				map->dm_segs[seg].ds_len = sgsize;
				map->dm_segs[seg]._ds_vaddr = vaddr;
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
		return EFBIG;		/* XXX Better return value here? */

	return 0;
}

/*
 * Common function for loading a direct-mapped DMA map with a linear
 * buffer.
 */
int
_bus_dmamap_load(bus_dma_tag_t t, bus_dmamap_t map, void *buf,
		 bus_size_t buflen, struct proc *p, int flags)
{
	vaddr_t lastaddr;
	int seg, error;
	struct vmspace *vm;

	/*
	 * Make sure that on error condition we return "no valid mappings".
	 */
	map->dm_mapsize = 0;
	map->dm_nsegs = 0;
	KASSERT(map->dm_maxsegsz <= map->_dm_maxmaxsegsz);

	if (buflen > map->_dm_size)
		return EINVAL;

	if (p != NULL) {
		vm = p->p_vmspace;
	} else {
		vm = vmspace_kernel();
	}

	seg = 0;
	error = _bus_dmamap_load_buffer(map, buf, buflen,
	    vm, flags, &lastaddr, &seg, 1);
	if (error == 0) {
		map->dm_mapsize = buflen;
		map->dm_nsegs = seg + 1;
		map->_dm_vmspace = vm;

		/*
		 * For linear buffers, we support marking the mapping
		 * as COHERENT.
		 *
		 * XXX Check TLB entries for cache-inhibit bits?
		 */
		if (buf >= (void *)MIPS_KSEG1_START &&
		    buf < (void *)MIPS_KSEG2_START)
			map->_dm_flags |= SGIMIPS_DMAMAP_COHERENT;
	}
	return error;
}

/*
 * Like _bus_dmamap_load(), but for mbufs.
 */
int
_bus_dmamap_load_mbuf(bus_dma_tag_t t, bus_dmamap_t map, struct mbuf *m0,
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
	KASSERT(map->dm_maxsegsz <= map->_dm_maxmaxsegsz);

#ifdef DIAGNOSTIC
	if ((m0->m_flags & M_PKTHDR) == 0)
		panic("_bus_dmamap_load_mbuf: no packet header");
#endif

	if (m0->m_pkthdr.len > map->_dm_size)
		return EINVAL;

	first = 1;
	seg = 0;
	error = 0;
	for (m = m0; m != NULL && error == 0; m = m->m_next) {
		if (m->m_len == 0)
			continue;
		error = _bus_dmamap_load_buffer(map, m->m_data, m->m_len,
		    vmspace_kernel(), flags, &lastaddr, &seg, first);
		first = 0;
	}
	if (error == 0) {
		map->dm_mapsize = m0->m_pkthdr.len;
		map->dm_nsegs = seg + 1;
		map->_dm_vmspace = vmspace_kernel();	/* always kernel */
	}
	return error;
}

/*
 * Like _bus_dmamap_load(), but for uios.
 */
int
_bus_dmamap_load_uio(bus_dma_tag_t t, bus_dmamap_t map, struct uio *uio,
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
	KASSERT(map->dm_maxsegsz <= map->_dm_maxmaxsegsz);

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
		minlen = resid < iov[i].iov_len ? resid : iov[i].iov_len;
		addr = (void *)iov[i].iov_base;

		error = _bus_dmamap_load_buffer(map, addr, minlen,
		    uio->uio_vmspace, flags, &lastaddr, &seg, first);
		first = 0;

		resid -= minlen;
	}
	if (error == 0) {
		map->dm_mapsize = uio->uio_resid;
		map->dm_nsegs = seg + 1;
		map->_dm_vmspace = uio->uio_vmspace;
	}
	return error;
}

/*
 * Like _bus_dmamap_load(), but for raw memory.
 */
int
_bus_dmamap_load_raw(bus_dma_tag_t t, bus_dmamap_t map, bus_dma_segment_t *segs,
		     int nsegs, bus_size_t size, int flags)
{

	panic("_bus_dmamap_load_raw: not implemented");
}

/*
 * Common function for unloading a DMA map.  May be called by
 * chipset-specific DMA map unload functions.
 */
void
_bus_dmamap_unload(bus_dma_tag_t t, bus_dmamap_t map)
{

	/*
	 * No resources to free; just mark the mappings as
	 * invalid.
	 */
	map->dm_maxsegsz = map->_dm_maxmaxsegsz;
	map->dm_mapsize = 0;
	map->dm_nsegs = 0;
	map->_dm_flags &= ~SGIMIPS_DMAMAP_COHERENT;
}

/* Common function from DMA map synchronization. May be called
 * by chipset-specific DMA map synchronization functions.
 *
 * This is the R3000 version.
 */
void
_bus_dmamap_sync_mips1(bus_dma_tag_t t, bus_dmamap_t map, bus_addr_t offset,
		       bus_size_t len, int ops)
{
	bus_size_t minlen;
	bus_addr_t addr;
	int i;

	/*
	 * Mixing PRE and POST operations is not allowed.
	 */
	 if ((ops & (BUS_DMASYNC_PREREAD|BUS_DMASYNC_PREWRITE)) != 0 &&
	     (ops & (BUS_DMASYNC_POSTREAD|BUS_DMASYNC_POSTWRITE)) != 0)
		 panic("_bus_dmamap_sync_mips1: mix PRE and POST");

#ifdef DIAGNOSTIC
	if (offset >= map->dm_mapsize)
		panic("_bus_dmamap_sync_mips1: bad offset %"PRIxPSIZE
		" (map size is %"PRIxPSIZE")"
		    , offset, map->dm_mapsize);
	if (len == 0 || (offset + len) > map->dm_mapsize)
		panic("_bus_dmamap_sync_mips1: bad length");
#endif

	/*
	 * The R3000 cache is write-through. Therefore, we only need
	 * to drain the write buffer on PREWRITE. The cache is not
	 * coherent, however, so we need to invalidate the data cache
	 * on PREREAD (should we do it POSTREAD instead?).
	 *
	 * POSTWRITE (and POSTREAD, currently) are noops.
	 */

	if (ops & BUS_DMASYNC_PREWRITE) {
		/*
		 * Flush the write buffer.
		 */
		 wbflush();
	 }

	/*
	 * If we're not doing PREREAD, nothing more to do.
	 */
	if ((ops & BUS_DMASYNC_PREREAD) == 0)
		return;

	/*
	 * No cache invalidation is necessary if the DMA map covers
	 * COHERENT DMA-safe memory (which is mapped un-cached).
	 */
	if (map->_dm_flags & SGIMIPS_DMAMAP_COHERENT)
		return;

	/*
	 * If we are going to hit something as large or larger
	 * than the entire data cache, just nail the whole thing.
	 *
	 * NOTE: Even though this is `wbinv_all', since the cache is
	 * write-through, it just invalidates it.
	 */
	if (len >= mips_cache_info.mci_pdcache_size) {
		mips_dcache_wbinv_all();
		return;
	}

	for (i = 0; i < map->dm_nsegs && len != 0; i++) {
		/* Find the beginning segment. */
		if (offset >= map->dm_segs[i].ds_len) {
			offset -= map->dm_segs[i].ds_len;
			continue;
		}

		/*
		 * Now at the first segment to sync; nail
		 * each segment until we have exhausted the
		 * length.
		 */
		minlen = len < map->dm_segs[i].ds_len - offset ?
		    len : map->dm_segs[i].ds_len - offset;

		addr = map->dm_segs[i].ds_addr;

#ifdef BUS_DMA_DEBUG
		printf("bus_dmamap_sync_mips1: flushing segment %d "
		    "(0x%lx..0x%lx) ...", i, addr + offset,
		    addr + offset + minlen - 1);
#endif
		mips_dcache_inv_range(
		    MIPS_PHYS_TO_KSEG0(addr + offset), minlen);
#ifdef BUS_DMA_DEBUG
		printf("\n");
#endif
		offset = 0;
		len -= minlen;
	}
}	

/*
 * Common function for DMA map synchronization.  May be called
 * by chipset-specific DMA map synchronization functions.
 *
 * This is the R4x00/R5k version.
 */
void
_bus_dmamap_sync_mips3(bus_dma_tag_t t, bus_dmamap_t map, bus_addr_t offset,
		       bus_size_t len, int ops)
{
	bus_size_t minlen;
	vaddr_t vaddr, start, end, preboundary, firstboundary, lastboundary;
	int i, useindex;

	/*
	 * Mixing PRE and POST operations is not allowed.
	 */
	if ((ops & (BUS_DMASYNC_PREREAD|BUS_DMASYNC_PREWRITE)) != 0 &&
	    (ops & (BUS_DMASYNC_POSTREAD|BUS_DMASYNC_POSTWRITE)) != 0)
		panic("_bus_dmamap_sync_mips3: mix PRE and POST");

#ifdef DIAGNOSTIC
	if (offset >= map->dm_mapsize)
		panic("_bus_dmamap_sync_mips3: bad offset %"PRIxPSIZE
		    "(map size is %"PRIxPSIZE")", offset, map->dm_mapsize);
	if (len == 0 || (offset + len) > map->dm_mapsize)
		panic("_bus_dmamap_sync_mips3: bad length");
#endif

	/*
	 * Since we're dealing with a virtually-indexed, write-back
	 * cache, we need to do the following things:
	 *
	 *      PREREAD -- Invalidate D-cache.  Note we might have
	 *      to also write-back here if we have to use an Index
	 *      op, or if the buffer start/end is not cache-line aligned.
 	 *
	 *      PREWRITE -- Write-back the D-cache.  If we have to use
	 *      an Index op, we also have to invalidate.  Note that if
	 *      we are doing PREREAD|PREWRITE, we can collapse everything
	 *      into a single op.
	 *
	 *      POSTREAD -- Nothing.
	 *
	 *      POSTWRITE -- Nothing.
	 */

	/*
	 * Flush the write buffer.
	 * XXX Is this always necessary?
	 */
	wbflush();

	/*
	 * No cache flushes are necessary if we're only doing
	 * POSTREAD or POSTWRITE (i.e. not doing PREREAD or PREWRITE).
	 */
	ops &= (BUS_DMASYNC_PREREAD|BUS_DMASYNC_PREWRITE);
	if (ops == 0)
		return;

	/*
	 * If the mapping is of COHERENT DMA-safe memory, no cache
	 * flush is necessary.
	 */
	if (map->_dm_flags & SGIMIPS_DMAMAP_COHERENT)
		return;

	/*
	 * If the mapping belongs to the kernel, or it belongs
	 * to the currently-running process (XXX actually, vmspace),
	 * then we can use Hit ops.  Otherwise, Index ops.
	 *
	 * This should be true the vast majority of the time.
	 */
	if (__predict_true(VMSPACE_IS_KERNEL_P(map->_dm_vmspace) ||
		map->_dm_vmspace == curproc->p_vmspace))
		useindex = 0;
	else
		useindex = 1;

	for (i = 0; i < map->dm_nsegs && len != 0; i++) {
		/* Find the beginning segment. */
		if (offset >= map->dm_segs[i].ds_len) {
			offset -= map->dm_segs[i].ds_len;
			continue;
		}

		/*
		 * Now at the first segment to sync; nail
		 * each segment until we have exhausted the
		 * length.
		 */
		minlen = len < map->dm_segs[i].ds_len - offset ?
		    len : map->dm_segs[i].ds_len - offset;

		vaddr = map->dm_segs[i]._ds_vaddr;

#ifdef BUS_DMA_DEBUG
		printf("bus_dmamap_sync_mips3: flushing segment %d "
		    "(0x%lx+%lx, 0x%lx+0x%lx) (olen = %ld)...", i,
		    vaddr, offset, vaddr, offset + minlen - 1, len);
#endif

		/*
		 * If we are forced to use Index ops, it's always a
		 * Write-back,Invalidate, so just do one test.
		 */
		if (__predict_false(useindex)) {
			mips_dcache_wbinv_range_index(vaddr + offset, minlen);
#ifdef BUS_DMA_DEBUG
			printf("\n");
#endif
			offset = 0;
			len -= minlen;
			continue;
 		}

		/* The code that follows is more correct than that in
		   mips/bus_dma.c. */
		start = vaddr + offset;
		switch (ops) {
		case BUS_DMASYNC_PREREAD|BUS_DMASYNC_PREWRITE:
			mips_dcache_wbinv_range(start, minlen);
			break;

		case BUS_DMASYNC_PREREAD: {
			struct mips_cache_info * const mci = &mips_cache_info;
			end = start + minlen;
			preboundary = start & ~mci->mci_dcache_align_mask;
			firstboundary = (start + mci->mci_dcache_align_mask)
			    & ~mci->mci_dcache_align_mask;
			lastboundary = end & ~mci->mci_dcache_align_mask;
			if (preboundary < start && preboundary < lastboundary)
				mips_dcache_wbinv_range(preboundary,
				    mci->mci_dcache_align);
			if (firstboundary < lastboundary)
				mips_dcache_inv_range(firstboundary,
				    lastboundary - firstboundary);
			if (lastboundary < end)
				mips_dcache_wbinv_range(lastboundary,
				    mci->mci_dcache_align);
			break;
		}

		case BUS_DMASYNC_PREWRITE:
			mips_dcache_wb_range(start, minlen);
			break;
		}
#ifdef BUS_DMA_DEBUG
		printf("\n");
#endif
		offset = 0;
		len -= minlen;
	}
}

/*
 * Common function for DMA-safe memory allocation.  May be called
 * by bus-specific DMA memory allocation functions.
 */
int
_bus_dmamem_alloc(bus_dma_tag_t t, bus_size_t size, bus_size_t alignment,
		  bus_size_t boundary, bus_dma_segment_t *segs,
		  int nsegs, int *rsegs, int flags)
{
	return (_bus_dmamem_alloc_range_common(t, size, alignment, boundary,
	    segs, nsegs, rsegs, flags,
	    mips_avail_start /*low*/, mips_avail_end - PAGE_SIZE /*high*/));
}

/*
 * Common function for freeing DMA-safe memory.  May be called by
 * bus-specific DMA memory free functions.
 */
void
_bus_dmamem_free(bus_dma_tag_t t, bus_dma_segment_t *segs, int nsegs)
{

	_bus_dmamem_free_common(t, segs, nsegs);
}

/*
 * Common function for mapping DMA-safe memory.  May be called by
 * bus-specific DMA memory map functions.
 */
int
_bus_dmamem_map(bus_dma_tag_t t, bus_dma_segment_t *segs, int nsegs,
		size_t size, void **kvap, int flags)
{
	vaddr_t va;
	bus_addr_t addr;
	int curseg;
	const uvm_flag_t kmflags =
	    (flags & BUS_DMA_NOWAIT) != 0 ? UVM_KMF_NOWAIT : 0;
	u_int pmapflags;

	/*
	 * If we're only mapping 1 segment, use KSEG0 or KSEG1, to avoid
	 * TLB thrashing.
	 */

	if (nsegs == 1) {
		if (flags & BUS_DMA_COHERENT)
			*kvap = (void *)MIPS_PHYS_TO_KSEG1(segs[0].ds_addr);
		else
			*kvap = (void *)MIPS_PHYS_TO_KSEG0(segs[0].ds_addr);
		return 0;
	}

	size = round_page(size);

	va = uvm_km_alloc(kernel_map, size, 0, UVM_KMF_VAONLY | kmflags);

	if (va == 0)
		return (ENOMEM);

	*kvap = (void *)va;

	pmapflags = VM_PROT_READ | VM_PROT_WRITE | PMAP_WIRED;
	if (flags & BUS_DMA_COHERENT)
		pmapflags |= PMAP_NOCACHE;

	for (curseg = 0; curseg < nsegs; curseg++) {
		for (addr = segs[curseg].ds_addr;
		    addr < (segs[curseg].ds_addr + segs[curseg].ds_len);
		    addr += PAGE_SIZE, va += PAGE_SIZE, size -= PAGE_SIZE) {
			if (size == 0)
				panic("_bus_dmamem_map: size botch");
			pmap_enter(pmap_kernel(), va, addr,
			    VM_PROT_READ | VM_PROT_WRITE,
			    pmapflags);
		}
	}
	pmap_update(pmap_kernel());

	return 0;
}

/*
 * Common function for unmapping DMA-safe memory.  May be called by
 * bus-specific DMA memory unmapping functions.
 */
void
_bus_dmamem_unmap(bus_dma_tag_t t, void *kva, size_t size)
{

	/*
	 * Nothing to do if we mapped it with KSEG0 or KSEG1 (i.e.
	 * not in KSEG2).
	 */
	if (kva >= (void *)MIPS_KSEG0_START &&
	    kva < (void *)MIPS_KSEG2_START)
		return;

	_bus_dmamem_unmap_common(t, kva, size);
}

/*
 * Common functin for mmap(2)'ing DMA-safe memory.  May be called by
 * bus-specific DMA mmap(2)'ing functions.
 */
paddr_t
_bus_dmamem_mmap(bus_dma_tag_t t, bus_dma_segment_t *segs, int nsegs,
		 off_t off, int prot, int flags)
{
	bus_addr_t rv;

	rv = _bus_dmamem_mmap_common(t, segs, nsegs, off, prot, flags);
	if (rv == (bus_addr_t)-1)
		return (-1);
	
#if defined(_MIPS_PADDR_T_64BIT) || defined(_LP64)
	return (mips_btop(rv | PGC_NOCACHE));
#else
	return (mips_btop(rv));
#endif
}

paddr_t
bus_space_mmap(bus_space_tag_t space, bus_addr_t addr, off_t off,
         int prot, int flags)
{

	if (flags & BUS_SPACE_MAP_CACHEABLE) {

		return mips_btop(MIPS_KSEG0_TO_PHYS(addr) + off);
	} else
#if defined(_MIPS_PADDR_T_64BIT) || defined(_LP64)
		return mips_btop((MIPS_KSEG1_TO_PHYS(addr) + off)
		    | PGC_NOCACHE);
#else
		return mips_btop((MIPS_KSEG1_TO_PHYS(addr) + off));
#endif
}

/*
 * Utility macros; do not use outside this file.
 */
#define	__PB_TYPENAME_PREFIX(BITS)	___CONCAT(u_int,BITS)
#define	__PB_TYPENAME(BITS)		___CONCAT(__PB_TYPENAME_PREFIX(BITS),_t)

/*
 *	void bus_space_read_multi_N(bus_space_tag_t tag,
 *	    bus_space_handle_t bsh, bus_size_t offset,
 *	    u_intN_t *addr, bus_size_t count);
 *
 * Read `count' 1, 2, 4, or 8 byte quantities from bus space
 * described by tag/handle/offset and copy into buffer provided.
 */

#define __SGIMIPS_bus_space_read_multi(BYTES,BITS)			\
void __CONCAT(bus_space_read_multi_,BYTES)				\
	(bus_space_tag_t, bus_space_handle_t, bus_size_t,		\
	__PB_TYPENAME(BITS) *, bus_size_t);				\
									\
void									\
__CONCAT(bus_space_read_multi_,BYTES)(					\
	bus_space_tag_t t,						\
	bus_space_handle_t h,						\
	bus_size_t o,							\
	__PB_TYPENAME(BITS) *a,						\
	bus_size_t c)							\
{									\
									\
	while (c--)							\
		*a++ = __CONCAT(bus_space_read_,BYTES)(t, h, o);	\
}

__SGIMIPS_bus_space_read_multi(1,8)
__SGIMIPS_bus_space_read_multi(2,16)
__SGIMIPS_bus_space_read_multi(4,32)

#undef __SGIMIPS_bus_space_read_multi

#define __SGIMIPS_bus_space_read_multi_stream(BYTES,BITS)		\
void __CONCAT(bus_space_read_multi_stream_,BYTES)			\
	(bus_space_tag_t, bus_space_handle_t, bus_size_t,		\
	__PB_TYPENAME(BITS) *, bus_size_t);				\
									\
void									\
__CONCAT(bus_space_read_multi_stream_,BYTES)(				\
	bus_space_tag_t t,						\
	bus_space_handle_t h,						\
	bus_size_t o,							\
	__PB_TYPENAME(BITS) *a,						\
	bus_size_t c)							\
{									\
									\
	while (c--)							\
		*a++ = __CONCAT(bus_space_read_stream_,BYTES)(t, h, o);	\
}


__SGIMIPS_bus_space_read_multi_stream(2,16)
__SGIMIPS_bus_space_read_multi_stream(4,32)

#undef __SGIMIPS_bus_space_read_multi_stream

/*
 *	void bus_space_read_region_N(bus_space_tag_t tag,
 *	    bus_space_handle_t bsh, bus_size_t offset,
 *	    u_intN_t *addr, bus_size_t count);
 *
 * Read `count' 1, 2, 4, or 8 byte quantities from bus space
 * described by tag/handle and starting at `offset' and copy into
 * buffer provided.
 */

#define __SGIMIPS_bus_space_read_region(BYTES,BITS)			\
void __CONCAT(bus_space_read_region_,BYTES)				\
	(bus_space_tag_t, bus_space_handle_t, bus_size_t,		\
	__PB_TYPENAME(BITS) *, bus_size_t);				\
									\
void									\
__CONCAT(bus_space_read_region_,BYTES)(					\
	bus_space_tag_t t,						\
	bus_space_handle_t h,						\
	bus_size_t o,							\
	__PB_TYPENAME(BITS) *a,						\
	bus_size_t c)							\
{									\
									\
	while (c--) {							\
		*a++ = __CONCAT(bus_space_read_,BYTES)(t, h, o);	\
		o += BYTES;						\
	}								\
}

__SGIMIPS_bus_space_read_region(1,8)
__SGIMIPS_bus_space_read_region(2,16)
__SGIMIPS_bus_space_read_region(4,32)

#undef __SGIMIPS_bus_space_read_region

/*
 *	void bus_space_read_region_stream_N(bus_space_tag_t tag,
 *	    bus_space_handle_t bsh, bus_size_t offset,
 *	    u_intN_t *addr, bus_size_t count);
 *
 * Read `count' 1, 2, 4, or 8 byte quantities from bus space
 * described by tag/handle and starting at `offset' and copy into
 * buffer provided. No byte order translation is done.
 */

#define __SGIMIPS_bus_space_read_region_stream(BYTES,BITS)		\
void __CONCAT(bus_space_read_region_stream_,BYTES)			\
	(bus_space_tag_t, bus_space_handle_t, bus_size_t,		\
	__PB_TYPENAME(BITS) *, bus_size_t);				\
									\
void									\
__CONCAT(bus_space_read_region_stream_,BYTES)(				\
	bus_space_tag_t t,						\
	bus_space_handle_t h,						\
	bus_size_t o,							\
	__PB_TYPENAME(BITS) *a,						\
	bus_size_t c)							\
{									\
									\
	while (c--) {							\
		*a++ = __CONCAT(bus_space_read_stream_,BYTES)(t, h, o);	\
		o += BYTES;						\
	}								\
}

__SGIMIPS_bus_space_read_region_stream(2,16)
__SGIMIPS_bus_space_read_region_stream(4,32)

#undef __SGIMIPS_bus_space_read_region_stream

/*
 *	void bus_space_write_multi_N(bus_space_tag_t tag,
 *	    bus_space_handle_t bsh, bus_size_t offset,
 *	    const u_intN_t *addr, bus_size_t count);
 *
 * Write `count' 1, 2, 4, or 8 byte quantities from the buffer
 * provided to bus space described by tag/handle/offset.
 */

#define __SGIMIPS_bus_space_write_multi(BYTES,BITS)			\
void __CONCAT(bus_space_write_multi_,BYTES)				\
	(bus_space_tag_t, bus_space_handle_t, bus_size_t,		\
	const __PB_TYPENAME(BITS) *, bus_size_t);			\
									\
void									\
__CONCAT(bus_space_write_multi_,BYTES)(					\
	bus_space_tag_t t,						\
	bus_space_handle_t h,						\
	bus_size_t o,							\
	const __PB_TYPENAME(BITS) *a,					\
	bus_size_t c)							\
{									\
									\
	while (c--)							\
		__CONCAT(bus_space_write_,BYTES)(t, h, o, *a++);	\
}

__SGIMIPS_bus_space_write_multi(1,8)
__SGIMIPS_bus_space_write_multi(2,16)
__SGIMIPS_bus_space_write_multi(4,32)

#undef __SGIMIPS_bus_space_write_multi

#define __SGIMIPS_bus_space_write_multi_stream(BYTES,BITS)		\
void __CONCAT(bus_space_write_multi_stream_,BYTES)			\
	(bus_space_tag_t, bus_space_handle_t, bus_size_t,		\
	const __PB_TYPENAME(BITS) *, bus_size_t);			\
									\
void									\
__CONCAT(bus_space_write_multi_stream_,BYTES)(				\
	bus_space_tag_t t,						\
	bus_space_handle_t h,						\
	bus_size_t o,							\
	const __PB_TYPENAME(BITS) *a,					\
	bus_size_t c)							\
{									\
									\
	while (c--)							\
		__CONCAT(bus_space_write_stream_,BYTES)(t, h, o, *a++);	\
}

__SGIMIPS_bus_space_write_multi_stream(2,16)
__SGIMIPS_bus_space_write_multi_stream(4,32)

#undef __SGIMIPS_bus_space_write_multi_stream

/*
 *	void bus_space_write_region_N(bus_space_tag_t tag,
 *	    bus_space_handle_t bsh, bus_size_t offset,
 *	    const u_intN_t *addr, bus_size_t count);
 *
 * Write `count' 1, 2, 4, or 8 byte quantities from the buffer provided
 * to bus space described by tag/handle starting at `offset'.
 */

#define __SGIMIPS_bus_space_write_region(BYTES,BITS)			\
void __CONCAT(bus_space_write_region_,BYTES)				\
	(bus_space_tag_t, bus_space_handle_t, bus_size_t,		\
	const __PB_TYPENAME(BITS) *, bus_size_t);			\
									\
void									\
__CONCAT(bus_space_write_region_,BYTES)(				\
	bus_space_tag_t t,						\
	bus_space_handle_t h,						\
	bus_size_t o,							\
	const __PB_TYPENAME(BITS) *a,					\
	bus_size_t c)							\
{									\
									\
	while (c--) {							\
		__CONCAT(bus_space_write_,BYTES)(t, h, o, *a++);	\
		o += BYTES;						\
	}								\
}

__SGIMIPS_bus_space_write_region(1,8)
__SGIMIPS_bus_space_write_region(2,16)
__SGIMIPS_bus_space_write_region(4,32)

#undef __SGIMIPS_bus_space_write_region

/*
 *	void bus_space_write_region_stream_N(bus_space_tag_t tag,
 *	    bus_space_handle_t bsh, bus_size_t offset,
 *	    const u_intN_t *addr, bus_size_t count);
 *
 * Write `count' 1, 2, 4, or 8 byte quantities from the buffer provided
 * to bus space described by tag/handle starting at `offset', but without
 * byte-order translation
 */

#define __SGIMIPS_bus_space_write_region_stream(BYTES,BITS)		\
void __CONCAT(bus_space_write_region_stream_,BYTES)			\
	(bus_space_tag_t, bus_space_handle_t, bus_size_t,		\
	const __PB_TYPENAME(BITS) *, bus_size_t);			\
									\
void									\
__CONCAT(bus_space_write_region_stream_,BYTES)(				\
	bus_space_tag_t t,						\
	bus_space_handle_t h,						\
	bus_size_t o,							\
	const __PB_TYPENAME(BITS) *a,					\
	bus_size_t c)							\
{									\
									\
	while (c--) {							\
		__CONCAT(bus_space_write_stream_,BYTES)(t, h, o, *a++);	\
		o += BYTES;						\
	}								\
}

__SGIMIPS_bus_space_write_region_stream(2,16)
__SGIMIPS_bus_space_write_region_stream(4,32)

#undef __SGIMIPS_bus_space_write_region_stream

/*
 *	void bus_space_set_multi_N(bus_space_tag_t tag,
 *	    bus_space_handle_t bsh, bus_size_t offset, u_intN_t val,
 *	    bus_size_t count);
 *
 * Write the 1, 2, 4, or 8 byte value `val' to bus space described
 * by tag/handle/offset `count' times.
 */

#define __SGIMIPS_bus_space_set_multi(BYTES,BITS)			\
void __CONCAT(bus_space_set_multi_,BYTES)				\
	(bus_space_tag_t, bus_space_handle_t, bus_size_t,		\
	__PB_TYPENAME(BITS), bus_size_t);				\
									\
void									\
__CONCAT(bus_space_set_multi_,BYTES)(					\
	bus_space_tag_t t,						\
	bus_space_handle_t h,						\
	bus_size_t o,							\
	__PB_TYPENAME(BITS) v,						\
	bus_size_t c)							\
{									\
									\
	while (c--)							\
		__CONCAT(bus_space_write_,BYTES)(t, h, o, v);		\
}

__SGIMIPS_bus_space_set_multi(1,8)
__SGIMIPS_bus_space_set_multi(2,16)
__SGIMIPS_bus_space_set_multi(4,32)

#undef __SGIMIPS_bus_space_set_multi

/*
 *	void bus_space_set_region_N(bus_space_tag_t tag,
 *	    bus_space_handle_t bsh, bus_size_t offset, u_intN_t val,
 *	    bus_size_t count);
 *
 * Write `count' 1, 2, 4, or 8 byte value `val' to bus space described
 * by tag/handle starting at `offset'.
 */

#define __SGIMIPS_bus_space_set_region(BYTES,BITS)			\
void __CONCAT(bus_space_set_region_,BYTES)				\
	(bus_space_tag_t, bus_space_handle_t, bus_size_t,		\
	__PB_TYPENAME(BITS), bus_size_t);				\
									\
void									\
__CONCAT(bus_space_set_region_,BYTES)(					\
	bus_space_tag_t t,						\
	bus_space_handle_t h,						\
	bus_size_t o,							\
	__PB_TYPENAME(BITS) v,						\
	bus_size_t c)							\
{									\
									\
	while (c--) {							\
		__CONCAT(bus_space_write_,BYTES)(t, h, o, v);		\
		o += BYTES;						\
	}								\
}

__SGIMIPS_bus_space_set_region(1,8)
__SGIMIPS_bus_space_set_region(2,16)
__SGIMIPS_bus_space_set_region(4,32)

#undef __SGIMIPS_bus_space_set_region

/*
 *	void bus_space_copy_region_N(bus_space_tag_t tag,
 *	    bus_space_handle_t bsh1, bus_size_t off1,
 *	    bus_space_handle_t bsh2, bus_size_t off2,
 *	    bus_size_t count);
 *
 * Copy `count' 1, 2, 4, or 8 byte values from bus space starting
 * at tag/bsh1/off1 to bus space starting at tag/bsh2/off2.
 */

#define	__SGIMIPS_copy_region(BYTES)					\
void __CONCAT(bus_space_copy_region_,BYTES)				\
	(bus_space_tag_t,						\
	    bus_space_handle_t bsh1, bus_size_t off1,			\
	    bus_space_handle_t bsh2, bus_size_t off2,			\
	    bus_size_t count);						\
									\
void									\
__CONCAT(bus_space_copy_region_,BYTES)(					\
	bus_space_tag_t t,						\
	bus_space_handle_t h1,						\
	bus_size_t o1,							\
	bus_space_handle_t h2,						\
	bus_size_t o2,							\
	bus_size_t c)							\
{									\
	bus_size_t o;							\
									\
	if ((h1 + o1) >= (h2 + o2)) {					\
		/* src after dest: copy forward */			\
		for (o = 0; c != 0; c--, o += BYTES)			\
			__CONCAT(bus_space_write_,BYTES)(t, h2, o2 + o,	\
			    __CONCAT(bus_space_read_,BYTES)(t, h1, o1 + o)); \
	} else {							\
		/* dest after src: copy backwards */			\
		for (o = (c - 1) * BYTES; c != 0; c--, o -= BYTES)	\
			__CONCAT(bus_space_write_,BYTES)(t, h2, o2 + o,	\
			    __CONCAT(bus_space_read_,BYTES)(t, h1, o1 + o)); \
	}								\
}

__SGIMIPS_copy_region(1)
__SGIMIPS_copy_region(2)
__SGIMIPS_copy_region(4)

#undef __SGIMIPS_copy_region

#undef __PB_TYPENAME_PREFIX
#undef __PB_TYPENAME

