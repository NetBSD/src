/*	$NetBSD: bus.c,v 1.28 2004/03/25 15:06:37 pooka Exp $	*/

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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the NetBSD
 *	Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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
__KERNEL_RCSID(0, "$NetBSD: bus.c,v 1.28 2004/03/25 15:06:37 pooka Exp $");

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
#include <machine/bus.h>
#include <machine/cpu.h>
#include <machine/machtype.h>

#include <uvm/uvm_extern.h>

#include <mips/cpuregs.h>
#include <mips/locore.h>
#include <mips/cache.h>

#include <sgimips/mace/macereg.h>

static int	_bus_dmamap_load_buffer(bus_dmamap_t, void *, bus_size_t,
				struct proc *, int, vaddr_t *, int *, int);

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
#ifdef MIPS1
	/* R2000/R3000 */
	case MACH_SGI_IP12:
		sgimips_default_bus_dma_tag._dmamap_sync =
		    _bus_dmamap_sync_mips1;
		break;
#endif

#ifdef MIPS3
	/* >=R4000*/
	case MACH_SGI_IP20:
	case MACH_SGI_IP22:
	case MACH_SGI_IP32:
		sgimips_default_bus_dma_tag._dmamap_sync =
		    _bus_dmamap_sync_mips3;
		break;
#endif

	default:
		panic("sgimips_bus_dma_init: unsupported mach type IP%d\n",
		    mach_type);
	}
}

u_int8_t
bus_space_read_1(t, h, o)
	bus_space_tag_t t;
	bus_space_handle_t h;
	bus_size_t o;
{
	wbflush(); /* XXX ? */

	switch (t) {
	case SGIMIPS_BUS_SPACE_NORMAL:
		return *(volatile u_int8_t *)(h + o);
	case SGIMIPS_BUS_SPACE_HPC:
		return *(volatile u_int8_t *)(h + (o << 2) + 3);
	case SGIMIPS_BUS_SPACE_MEM:
	case SGIMIPS_BUS_SPACE_IO:
		return *(volatile u_int8_t *)(h + (o | 3) - (o & 3));
	case SGIMIPS_BUS_SPACE_MACE:
		return *(volatile u_int8_t *)(h + (o << 8) + 7);
	default:
		panic("no bus tag");
	}
}

void
bus_space_write_1(t, h, o, v)
	bus_space_tag_t t;
	bus_space_handle_t h;
	bus_size_t o;
	u_int8_t v;
{
	switch (t) {
	case SGIMIPS_BUS_SPACE_NORMAL:
		*(volatile u_int8_t *)(h + o) = v;
		break;
	case SGIMIPS_BUS_SPACE_HPC:
		*(volatile u_int8_t *)(h + (o << 2) + 3) = v;
		break;
	case SGIMIPS_BUS_SPACE_MEM:
	case SGIMIPS_BUS_SPACE_IO:
		*(volatile u_int8_t *)(h + (o | 3) - (o & 3)) = v;
		break;
	case SGIMIPS_BUS_SPACE_MACE:
		*(volatile u_int8_t *)(h + (o << 8) + 7) = v;
		break;
	default:
		panic("no bus tag");
	}

	wbflush();	/* XXX */
}

u_int16_t
bus_space_read_2(t, h, o)
	bus_space_tag_t t;
	bus_space_handle_t h;
	bus_size_t o;
{
	wbflush(); /* XXX ? */

	switch (t) {
	case SGIMIPS_BUS_SPACE_NORMAL:
		return *(volatile u_int16_t *)(h + o);
	case SGIMIPS_BUS_SPACE_HPC:
		return *(volatile u_int16_t *)(h + (o << 2) + 1);
	case SGIMIPS_BUS_SPACE_MEM:
	case SGIMIPS_BUS_SPACE_IO:
		return *(volatile u_int16_t *)(h + (o | 2) - (o & 3));
	default:
		panic("no bus tag");
	}
}

void
bus_space_write_2(t, h, o, v)
	bus_space_tag_t t;
	bus_space_handle_t h;
	bus_size_t o;
	u_int16_t v;
{
	switch (t) {
	case SGIMIPS_BUS_SPACE_NORMAL:
		*(volatile u_int16_t *)(h + o) = v;
		break;
	case SGIMIPS_BUS_SPACE_HPC:
		*(volatile u_int16_t *)(h + (o << 2) + 1) = v;
		break;
	case SGIMIPS_BUS_SPACE_MEM:
	case SGIMIPS_BUS_SPACE_IO:
		*(volatile u_int16_t *)(h + (o | 2) - (o & 3)) = v;
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
	int s;

	switch (tag) {
		case SGIMIPS_BUS_SPACE_MACE:
			s = splhigh();
			delay(10);
			reg = (*(volatile u_int32_t *)(bsh + o));
			delay(10);
			splx(s);
			break;
		default:
			wbflush();
			reg = (*(volatile u_int32_t *)(bsh + o));
			break;
	}
	return reg;
}

void
bus_space_write_4(bus_space_tag_t tag, bus_space_handle_t bsh, bus_size_t o, u_int32_t v)
{
	int s;

	switch (tag) {
		case SGIMIPS_BUS_SPACE_MACE:
			s = splhigh();
			delay(10);
			*(volatile u_int32_t *)((bsh) + (o)) = (v);
			delay(10);
			splx(s);
			break;
		default:
			*(volatile u_int32_t *)((bsh) + (o)) = (v);
			wbflush(); /* XXX */
			break;
	}
}

#ifdef MIPS3
u_int64_t
bus_space_read_8(bus_space_tag_t tag, bus_space_handle_t bsh, bus_size_t o)
{
	u_int64_t reg;
	int s;

	switch (tag) {
		case SGIMIPS_BUS_SPACE_MACE:
			s = splhigh();
			delay(10);
			reg = mips3_ld( (u_int64_t *)(bsh + o));
			delay(10);
			splx(s);
			break;
		default:
			reg = mips3_ld( (u_int64_t *)(bsh + o));
			break;
	}
	return reg;
}

void
bus_space_write_8(bus_space_tag_t tag, bus_space_handle_t bsh, bus_size_t o, u_int64_t v)
{
	int s;

	switch (tag) {
		case SGIMIPS_BUS_SPACE_MACE:
			s = splhigh();
			delay(10);
			mips3_sd( (u_int64_t *)(bsh + o), v);
			delay(10);
			splx(s);
			break;
		default:
			mips3_sd( (u_int64_t *)(bsh + o), v);
			break;
	}
}
#endif /* MIPS3 */

int
bus_space_map(t, bpa, size, flags, bshp)
	bus_space_tag_t t;
	bus_addr_t bpa;
	bus_size_t size;
	int flags;
	bus_space_handle_t *bshp;
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
bus_space_alloc(t, rstart, rend, size, alignment, boundary, flags, bpap, bshp)
	bus_space_tag_t t;
	bus_addr_t rstart, rend;
	bus_size_t size, alignment, boundary;
	int flags;
	bus_addr_t *bpap;
	bus_space_handle_t *bshp;
{
	panic("bus_space_alloc: not implemented");
}

void
bus_space_free(t, bsh, size)
	bus_space_tag_t t;
	bus_space_handle_t bsh;
	bus_size_t size;
{
	panic("bus_space_free: not implemented");
}

void
bus_space_unmap(t, bsh, size)
	bus_space_tag_t t;
	bus_space_handle_t bsh;
	bus_size_t size;
{
	return;
}

int
bus_space_subregion(t, bsh, offset, size, nbshp)
	bus_space_tag_t t;
	bus_space_handle_t bsh;
	bus_size_t offset, size;
	bus_space_handle_t *nbshp;
{

	*nbshp = bsh + offset;
	return 0;
}

void *
bus_space_vaddr(t, bsh)
	bus_space_tag_t t;
	bus_space_handle_t bsh;
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
_bus_dmamap_create(t, size, nsegments, maxsegsz, boundary, flags, dmamp)
	bus_dma_tag_t t;
	bus_size_t size;
	int nsegments;
	bus_size_t maxsegsz;
	bus_size_t boundary;
	int flags;
	bus_dmamap_t *dmamp;
{
	struct sgimips_bus_dmamap *map;
	void *mapstore;
	size_t mapsize;

	/*
	 * Allcoate and initialize the DMA map.  The end of the map
	 * is a variable-sized array of segments, so we allocate enough
	 * room for them in one shot.
	 *
	 * Note we don't preserve the WAITOK or NOWAIT flags.  Preservation
	 * of ALLOCNOW notifes others that we've reserved these resources,
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
	map->_dm_maxsegsz = maxsegsz;
	map->_dm_boundary = boundary;
	map->_dm_flags = flags & ~(BUS_DMA_WAITOK|BUS_DMA_NOWAIT);
	map->_dm_proc = NULL;
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
_bus_dmamap_destroy(t, map)
	bus_dma_tag_t t;
	bus_dmamap_t map;
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
_bus_dmamap_load_buffer(map, buf, buflen, p, flags, lastaddrp, segp, first)
	bus_dmamap_t map;
	void *buf;
	bus_size_t buflen;
	struct proc *p;
	int flags;
	vaddr_t *lastaddrp;
	int *segp;
	int first;
{
	bus_size_t sgsize;
	bus_addr_t curaddr, lastaddr, baddr, bmask;
	vaddr_t vaddr = (vaddr_t)buf;
	int seg;

	lastaddr = *lastaddrp;
	bmask = ~(map->_dm_boundary - 1);

	for (seg = *segp; buflen > 0 ; ) {
		/*
		 * Get the physical address for this segment.
		 */
		if (p != NULL)
			(void) pmap_extract(p->p_vmspace->vm_map.pmap,
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
_bus_dmamap_load(t, map, buf, buflen, p, flags)
	bus_dma_tag_t t;
	bus_dmamap_t map;
	void *buf;
	bus_size_t buflen;
	struct proc *p;
	int flags;
{
	vaddr_t lastaddr;
	int seg, error;

	/*
	 * Make sure that on error condition we return "no valid mappings".
	 */
	map->dm_mapsize = 0;
	map->dm_nsegs = 0;

	if (buflen > map->_dm_size)
		return EINVAL;

	seg = 0;
	error = _bus_dmamap_load_buffer(map, buf, buflen,
	    p, flags, &lastaddr, &seg, 1);
	if (error == 0) {
		map->dm_mapsize = buflen;
		map->dm_nsegs = seg + 1;
		map->_dm_proc = p;

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
_bus_dmamap_load_mbuf(t, map, m0, flags)
	bus_dma_tag_t t;
	bus_dmamap_t map;
	struct mbuf *m0;
	int flags;
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
#endif

	if (m0->m_pkthdr.len > map->_dm_size)
		return EINVAL;

	first = 1;
	seg = 0;
	error = 0;
	for (m = m0; m != NULL && error == 0; m = m->m_next) {
		error = _bus_dmamap_load_buffer(map,
		    m->m_data, m->m_len, NULL, flags, &lastaddr, &seg, first);
		first = 0;
	}
	if (error == 0) {
		map->dm_mapsize = m0->m_pkthdr.len;
		map->dm_nsegs = seg + 1;
		map->_dm_proc = NULL;	/* always kernel */
	}
	return error;
}

/*
 * Like _bus_dmamap_load(), but for uios.
 */
int
_bus_dmamap_load_uio(t, map, uio, flags)
	bus_dma_tag_t t;
	bus_dmamap_t map;
	struct uio *uio;
	int flags;
{
	vaddr_t lastaddr;
	int seg, i, error, first;
	bus_size_t minlen, resid;
	struct proc *p = NULL;
	struct iovec *iov;
	caddr_t addr;

	/*
	 * Make sure that on error condition we return "no valid mappings."
	 */
	map->dm_mapsize = 0;
	map->dm_nsegs = 0;

	resid = uio->uio_resid;
	iov = uio->uio_iov;

	if (uio->uio_segflg == UIO_USERSPACE) {
		p = uio->uio_procp;
#ifdef DIAGNOSTIC
		if (p == NULL)
			panic("_bus_dmamap_load_uio: USERSPACE but no proc");
#endif
	}

	first = 1;
	seg = 0;
	error = 0;
	for (i = 0; i < uio->uio_iovcnt && resid != 0 && error == 0; i++) {
		/*
		 * Now at the first iovec to load.  Load each iovec
		 * until we have exhausted the residual count.
		 */
		minlen = resid < iov[i].iov_len ? resid : iov[i].iov_len;
		addr = (caddr_t)iov[i].iov_base;

		error = _bus_dmamap_load_buffer(map, addr, minlen,
		    p, flags, &lastaddr, &seg, first);
		first = 0;

		resid -= minlen;
	}
	if (error == 0) {
		map->dm_mapsize = uio->uio_resid;
		map->dm_nsegs = seg + 1;
		map->_dm_proc = p;
	}
	return error;
}

/*
 * Like _bus_dmamap_load(), but for raw memory.
 */
int
_bus_dmamap_load_raw(t, map, segs, nsegs, size, flags)
	bus_dma_tag_t t;
	bus_dmamap_t map;
	bus_dma_segment_t *segs;
	int nsegs;
	bus_size_t size;
	int flags;
{

	panic("_bus_dmamap_load_raw: not implemented");
}

/*
 * Common function for unloading a DMA map.  May be called by
 * chipset-specific DMA map unload functions.
 */
void
_bus_dmamap_unload(t, map)
	bus_dma_tag_t t;
	bus_dmamap_t map;
{

	/*
	 * No resources to free; just mark the mappings as
	 * invalid.
	 */
	map->dm_mapsize = 0;
	map->dm_nsegs = 0;
	map->_dm_flags &= ~SGIMIPS_DMAMAP_COHERENT;
}

#ifdef MIPS1
/* Common function from DMA map synchronization. May be called
 * by chipset-specific DMA map synchronization functions.
 *
 * This is the R3000 version.
 */
void
_bus_dmamap_sync_mips1(t, map, offset, len, ops)
	bus_dma_tag_t t;
	bus_dmamap_t map;
	bus_addr_t offset;
	bus_size_t len;
	int ops;
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
		panic("_bus_dmamap_sync_mips1: bad offset %lu (map size is %lu)"
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
	if (len >= mips_pdcache_size) {
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
#endif /* MIPS1 */

#ifdef MIPS3
/*
 * Common function for DMA map synchronization.  May be called
 * by chipset-specific DMA map synchronization functions.
 *
 * This is the R4x00/R5k version.
 */
void
_bus_dmamap_sync_mips3(t, map, offset, len, ops)
	bus_dma_tag_t t;
	bus_dmamap_t map;
	bus_addr_t offset;
	bus_size_t len;
	int ops;
{
	bus_size_t minlen;
	bus_addr_t addr, start, end, preboundary, firstboundary, lastboundary;
	int i, useindex;

	/*
	 * Mising PRE and POST operations is not allowed.
	 */
	if ((ops & (BUS_DMASYNC_PREREAD|BUS_DMASYNC_PREWRITE)) != 0 &&
	    (ops & (BUS_DMASYNC_POSTREAD|BUS_DMASYNC_POSTWRITE)) != 0)
		panic("_bus_dmamap_sync: mix PRE and POST");

#ifdef DIAGNOSTIC
	if (offset >= map->dm_mapsize)
		panic("_bus_dmamap_sync: bad offset %lu (map size is %lu)",
		      offset, map->dm_mapsize);
	if (len == 0 || (offset + len) > map->dm_mapsize)
		panic("_bus_dmamap_sync: bad length");
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
	if (__predict_true(map->_dm_proc == NULL || map->_dm_proc == curproc))
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

		addr = map->dm_segs[i]._ds_vaddr;

#ifdef BUS_DMA_DEBUG
		printf("bus_dmamap_sync: flushing segment %d "
		    "(0x%lx+%lx, 0x%lx+0x%lx) (olen = %ld)...", i,
		    addr, offset, addr, offset + minlen - 1, len);
#endif

		/*
		 * If we are forced to use Index ops, it's always a
		 * Write-back,Invalidate, so just do one test.
		 */
		if (__predict_false(useindex)) {
			mips_dcache_wbinv_range_index(addr + offset, minlen);
#ifdef BUS_DMA_DEBUG
			printf("\n");
#endif
			offset = 0;
			len -= minlen;
			continue;
 		}

		start = addr + offset;
		switch (ops) {
		case BUS_DMASYNC_PREREAD|BUS_DMASYNC_PREWRITE:
			mips_dcache_wbinv_range(start, minlen);
			break;

		case BUS_DMASYNC_PREREAD:
			end = start + minlen;
			preboundary = start & ~mips_dcache_align_mask;
			firstboundary = (start + mips_dcache_align_mask)
			    & ~mips_dcache_align_mask;
			lastboundary = end & ~mips_dcache_align_mask;
			if (preboundary < start && preboundary < lastboundary)
				mips_dcache_wbinv_range(preboundary,
				    mips_dcache_align);
			if (firstboundary < lastboundary)
				mips_dcache_inv_range(firstboundary,
				    lastboundary - firstboundary);
			if (lastboundary < end)
				mips_dcache_wbinv_range(lastboundary,
				    mips_dcache_align);
			break;

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
#endif /* MIPS3 */

/*
 * Common function for DMA-safe memory allocation.  May be called
 * by bus-specific DMA memory allocation functions.
 */
int
_bus_dmamem_alloc(t, size, alignment, boundary, segs, nsegs, rsegs, flags)
	bus_dma_tag_t t;
	bus_size_t size, alignment, boundary;
	bus_dma_segment_t *segs;
	int nsegs;
	int *rsegs;
	int flags;
{
	extern paddr_t avail_start, avail_end;
	vaddr_t curaddr, lastaddr;
	psize_t high;
	struct vm_page *m;
	struct pglist mlist;
	int curseg, error;

	/* Always round the size. */
	size = round_page(size);

	high = avail_end - PAGE_SIZE;

	/*
	 * Allocate pages from the VM system.
	 */
	error = uvm_pglistalloc(size, avail_start, high, alignment, boundary,
	    &mlist, nsegs, (flags & BUS_DMA_NOWAIT) == 0);
	if (error)
		return error;

	/*
	 * Compute the location, size, and number of segments actually
	 * returned by the VM code.
	 */
	m = mlist.tqh_first;
	curseg = 0;
	lastaddr = segs[curseg].ds_addr = VM_PAGE_TO_PHYS(m);
	segs[curseg].ds_len = PAGE_SIZE;
	m = m->pageq.tqe_next;

	for (; m != NULL; m = m->pageq.tqe_next) {
		curaddr = VM_PAGE_TO_PHYS(m);
#ifdef DIAGNOSTIC
		if (curaddr < avail_start || curaddr >= high) {
			printf("uvm_pglistalloc returned non-sensical"
			    " address 0x%lx\n", curaddr);
			panic("_bus_dmamem_alloc");
		}
#endif
		if (curaddr == (lastaddr + PAGE_SIZE))
			segs[curseg].ds_len += PAGE_SIZE;
		else {
			curseg++;
			segs[curseg].ds_addr = curaddr;
			segs[curseg].ds_len = PAGE_SIZE;
		}
		lastaddr = curaddr;
	}

	*rsegs = curseg + 1;

	return 0;
}

/*
 * Common function for freeing DMA-safe memory.  May be called by
 * bus-specific DMA memory free functions.
 */
void
_bus_dmamem_free(t, segs, nsegs)
	bus_dma_tag_t t;
	bus_dma_segment_t *segs;
	int nsegs;
{
	struct vm_page *m;
	bus_addr_t addr;
	struct pglist mlist;
	int curseg;

	/*
	 * Build a list of pages to free back to the VM system.
	 */
	TAILQ_INIT(&mlist);
	for (curseg = 0; curseg < nsegs; curseg++) {
		for (addr = segs[curseg].ds_addr;
		    addr < (segs[curseg].ds_addr + segs[curseg].ds_len);
		    addr += PAGE_SIZE) {
			m = PHYS_TO_VM_PAGE(addr);
			TAILQ_INSERT_TAIL(&mlist, m, pageq);
		}
	}

	uvm_pglistfree(&mlist);
}

/*
 * Common function for mapping DMA-safe memory.  May be called by
 * bus-specific DMA memory map functions.
 */
int
_bus_dmamem_map(t, segs, nsegs, size, kvap, flags)
	bus_dma_tag_t t;
	bus_dma_segment_t *segs;
	int nsegs;
	size_t size;
	caddr_t *kvap;
	int flags;
{
	vaddr_t va;
	bus_addr_t addr;
	int curseg;

	/*
	 * If we're only mapping 1 segment, use KSEG0 or KSEG1, to avoid
	 * TLB thrashing.
	 */
	if (nsegs == 1) {
		if (flags & BUS_DMA_COHERENT)
			*kvap = (caddr_t)MIPS_PHYS_TO_KSEG1(segs[0].ds_addr);
		else
			*kvap = (caddr_t)MIPS_PHYS_TO_KSEG0(segs[0].ds_addr);
		return 0;
	}

	size = round_page(size);

	va = uvm_km_valloc(kernel_map, size);

	if (va == 0)
		return (ENOMEM);

	*kvap = (caddr_t)va;

	for (curseg = 0; curseg < nsegs; curseg++) {
		for (addr = segs[curseg].ds_addr;
		    addr < (segs[curseg].ds_addr + segs[curseg].ds_len);
		    addr += PAGE_SIZE, va += PAGE_SIZE, size -= PAGE_SIZE) {
			if (size == 0)
				panic("_bus_dmamem_map: size botch");
			pmap_enter(pmap_kernel(), va, addr,
			    VM_PROT_READ | VM_PROT_WRITE,
			    VM_PROT_READ | VM_PROT_WRITE | PMAP_WIRED);

			/* XXX Do something about COHERENT here. */
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
_bus_dmamem_unmap(t, kva, size)
	bus_dma_tag_t t;
	caddr_t kva;
	size_t size;
{

#ifdef DIAGNOSTIC
	if ((u_long)kva & PGOFSET)
		panic("_bus_dmamem_unmap");
#endif

	/*
	 * Nothing to do if we mapped it with KSEG0 or KSEG1 (i.e.
	 * not in KSEG2).
	 */
	if (kva >= (caddr_t)MIPS_KSEG0_START &&
	    kva < (caddr_t)MIPS_KSEG2_START)
		return;

	size = round_page(size);
	uvm_km_free(kernel_map, (vaddr_t)kva, size);
}

/*
 * Common functin for mmap(2)'ing DMA-safe memory.  May be called by
 * bus-specific DMA mmap(2)'ing functions.
 */
paddr_t
_bus_dmamem_mmap(t, segs, nsegs, off, prot, flags)
	bus_dma_tag_t t;
	bus_dma_segment_t *segs;
	int nsegs;
	off_t off;
	int prot, flags;
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
#endif
		if (off >= segs[i].ds_len) {
			off -= segs[i].ds_len;
			continue;
		}

		return mips_btop((caddr_t)segs[i].ds_addr + off);
	}

	/* Page not found. */
	return -1;
}
