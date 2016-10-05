/*	$NetBSD: bus_dma.c,v 1.31.4.2 2016/10/05 20:55:32 skrll Exp $	*/

/*-
 * Copyright (c) 1997, 1998, 2001 The NetBSD Foundation, Inc.
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

#include <sys/cdefs.h>			/* RCS ID & Copyright macro defns */

__KERNEL_RCSID(0, "$NetBSD: bus_dma.c,v 1.31.4.2 2016/10/05 20:55:32 skrll Exp $");

#define _MIPS_BUS_DMA_PRIVATE

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/cpu.h>
#include <sys/device.h>
#include <sys/evcnt.h>
#include <sys/kernel.h>
#include <sys/malloc.h>
#include <sys/mbuf.h>
#include <sys/proc.h>
#include <sys/systm.h>

#include <uvm/uvm.h>

#include <mips/cache.h>
#ifdef _LP64
#include <mips/mips3_pte.h>
#endif

#include <mips/locore.h>

const struct mips_bus_dmamap_ops mips_bus_dmamap_ops = _BUS_DMAMAP_OPS_INITIALIZER;
const struct mips_bus_dmamem_ops mips_bus_dmamem_ops = _BUS_DMAMEM_OPS_INITIALIZER;
const struct mips_bus_dmatag_ops mips_bus_dmatag_ops = _BUS_DMATAG_OPS_INITIALIZER;

static struct evcnt bus_dma_creates =
	EVCNT_INITIALIZER(EVCNT_TYPE_MISC, NULL, "busdma", "creates");
static struct evcnt bus_dma_bounced_creates =
	EVCNT_INITIALIZER(EVCNT_TYPE_MISC, NULL, "busdma", "bounced creates");
static struct evcnt bus_dma_loads =
	EVCNT_INITIALIZER(EVCNT_TYPE_MISC, NULL, "busdma", "loads");
static struct evcnt bus_dma_bounced_loads =
	EVCNT_INITIALIZER(EVCNT_TYPE_MISC, NULL, "busdma", "bounced loads");
static struct evcnt bus_dma_read_bounces =
	EVCNT_INITIALIZER(EVCNT_TYPE_MISC, NULL, "busdma", "read bounces");
static struct evcnt bus_dma_write_bounces =
	EVCNT_INITIALIZER(EVCNT_TYPE_MISC, NULL, "busdma", "write bounces");
static struct evcnt bus_dma_bounced_unloads =
	EVCNT_INITIALIZER(EVCNT_TYPE_MISC, NULL, "busdma", "bounced unloads");
static struct evcnt bus_dma_unloads =
	EVCNT_INITIALIZER(EVCNT_TYPE_MISC, NULL, "busdma", "unloads");
static struct evcnt bus_dma_bounced_destroys =
	EVCNT_INITIALIZER(EVCNT_TYPE_MISC, NULL, "busdma", "bounced destroys");
static struct evcnt bus_dma_destroys =
	EVCNT_INITIALIZER(EVCNT_TYPE_MISC, NULL, "busdma", "destroys");

EVCNT_ATTACH_STATIC(bus_dma_creates);
EVCNT_ATTACH_STATIC(bus_dma_bounced_creates);
EVCNT_ATTACH_STATIC(bus_dma_loads);
EVCNT_ATTACH_STATIC(bus_dma_bounced_loads);
EVCNT_ATTACH_STATIC(bus_dma_read_bounces);
EVCNT_ATTACH_STATIC(bus_dma_write_bounces);
EVCNT_ATTACH_STATIC(bus_dma_unloads);
EVCNT_ATTACH_STATIC(bus_dma_bounced_unloads);
EVCNT_ATTACH_STATIC(bus_dma_destroys);
EVCNT_ATTACH_STATIC(bus_dma_bounced_destroys);

#define	STAT_INCR(x)	(bus_dma_ ## x.ev_count++)

paddr_t kvtophys(vaddr_t);	/* XXX */

/*
 * Utility function to load a linear buffer.  segp contains the starting
 * segment on entrance, and the ending segment on exit. first indicates
 * if this is the first invocation of this function.
 */
static int
_bus_dmamap_load_buffer(bus_dma_tag_t t, bus_dmamap_t map,
    void *buf, bus_size_t buflen, struct vmspace *vm, int flags,
    int *segp, vaddr_t lastvaddr, bool first)
{
	paddr_t baddr, curaddr, lastaddr;
	vaddr_t vaddr = (vaddr_t)buf;
	bus_dma_segment_t *ds = &map->dm_segs[*segp];
	bus_dma_segment_t * const eds = &map->dm_segs[map->_dm_segcnt];
	const bus_addr_t bmask = ~(map->_dm_boundary - 1);
	const bool d_cache_coherent =
	    (mips_options.mips_cpu_flags & CPU_MIPS_D_CACHE_COHERENT) != 0;

	lastaddr = ds->ds_addr + ds->ds_len;

	while (buflen > 0) {
		/*
		 * Get the physical address for this segment.
		 */
		if (!VMSPACE_IS_KERNEL_P(vm))
			(void) pmap_extract(vm_map_pmap(&vm->vm_map), vaddr,
			    &curaddr);
		else
			curaddr = kvtophys(vaddr);

		/*
		 * If we're beyond the current DMA window, indicate
		 * that and try to fall back onto something else.
		 */
		if (curaddr < t->_bounce_alloc_lo
		    || (t->_bounce_alloc_hi != 0
			&& curaddr >= t->_bounce_alloc_hi))
			return (EINVAL);
#if BUS_DMA_DEBUG
		printf("dma: addr %#"PRIxPADDR" -> %#"PRIxPADDR"\n", curaddr,
		    (curaddr - t->_bounce_alloc_lo) + t->_wbase);
#endif
		curaddr = (curaddr - t->_bounce_alloc_lo) + t->_wbase;

		/*
		 * Compute the segment size, and adjust counts.
		 */
		bus_size_t sgsize = PAGE_SIZE - ((uintptr_t)vaddr & PGOFSET);
		if (sgsize > buflen) {
			sgsize = buflen;
		}
		if (sgsize > map->dm_maxsegsz) {
			sgsize = map->dm_maxsegsz;
		}

		/*
		 * Make sure we don't cross any boundaries.
		 */
		if (map->_dm_boundary > 0) {
			baddr = (curaddr + map->_dm_boundary) & bmask;
			if (sgsize > baddr - curaddr) {
				sgsize = baddr - curaddr;
			}
		}

		/*
		 * Insert chunk into a segment, coalescing with
		 * the previous segment if possible.
		 */
		if (!first
		    && curaddr == lastaddr
		    && (d_cache_coherent
#ifndef __mips_o32
			|| !MIPS_CACHE_VIRTUAL_ALIAS
#endif
			|| vaddr == lastvaddr)
		    && (ds->ds_len + sgsize) <= map->dm_maxsegsz
		    && (map->_dm_boundary == 0
			|| ((ds->ds_addr ^ curaddr) & bmask) == 0)) {
			ds->ds_len += sgsize;
		} else {
			if (!first && ++ds >= eds)
				break;
			ds->ds_addr = curaddr;
			ds->ds_len = sgsize;
			ds->_ds_vaddr = (intptr_t)vaddr;
			first = false;
			/*
			 * If this segment uses the correct color, try to see
			 * if we can use a direct-mapped VA for the segment.
			 */
			if (!mips_cache_badalias(curaddr, vaddr)) {
#ifdef __mips_o32
				if (MIPS_KSEG0_P(curaddr + sgsize - 1)) {
					ds->_ds_vaddr =
					    MIPS_PHYS_TO_KSEG0(curaddr);
				}
#else
				/*
				 * All physical addresses can be accessed
				 * via XKPHYS.
				 */
		    		ds->_ds_vaddr =
				    MIPS_PHYS_TO_XKPHYS_CACHED(curaddr);
#endif
			}
			/* Make sure this is a valid kernel address */
			KASSERTMSG(ds->_ds_vaddr < 0,
			    "_ds_vaddr %#"PRIxREGISTER, ds->_ds_vaddr);
		}

		lastaddr = curaddr + sgsize;
		vaddr += sgsize;
		buflen -= sgsize;
		lastvaddr = vaddr;
	}

	*segp = ds - map->dm_segs;

	/*
	 * Did we fit?
	 */
	if (buflen != 0) {
		/*
		 * If there is a chained window, we will automatically
		 * fall back to it.
		 */
		return (EFBIG);		/* XXX better return value here? */
	}

	return (0);
}

#ifdef _MIPS_NEED_BUS_DMA_BOUNCE
static int _bus_dma_alloc_bouncebuf(bus_dma_tag_t t, bus_dmamap_t map,
	    bus_size_t size, int flags);
static void _bus_dma_free_bouncebuf(bus_dma_tag_t t, bus_dmamap_t map);
static int _bus_dma_uiomove(void *buf, struct uio *uio, size_t n,
	    int direction);

static int
_bus_dma_load_bouncebuf(bus_dma_tag_t t, bus_dmamap_t map, void *buf,
	size_t buflen, int buftype, int flags)
{
	struct mips_bus_dma_cookie * const cookie = map->_dm_cookie;
	struct vmspace * const vm = vmspace_kernel();
	int seg, error;

	KASSERT(cookie != NULL);
	KASSERT(cookie->id_flags & _BUS_DMA_MIGHT_NEED_BOUNCE);

	/*
	 * Allocate bounce pages, if necessary.
	 */
	if ((cookie->id_flags & _BUS_DMA_HAS_BOUNCE) == 0) {
		error = _bus_dma_alloc_bouncebuf(t, map, buflen, flags);
		if (error)
			return (error);
	}

	/*
	 * Cache a pointer to the caller's buffer and load the DMA map
	 * with the bounce buffer.
	 */
	cookie->id_origbuf = buf;
	cookie->id_origbuflen = buflen;
	cookie->id_buftype = buftype;
	seg = 0;
	error = _bus_dmamap_load_buffer(t, map, cookie->id_bouncebuf,
	    buflen, vm, flags, &seg, 0, true);
	if (error)
		return (error);

	STAT_INCR(bounced_loads);
	map->dm_mapsize = buflen;
	map->dm_nsegs = seg + 1;
	map->_dm_vmspace = vm;
	/*
	 * If our cache is coherent, then the map must be coherent too.
	 */
	if (mips_options.mips_cpu_flags & CPU_MIPS_D_CACHE_COHERENT)
		map->_dm_flags |= _BUS_DMAMAP_COHERENT;

	/* ...so _bus_dmamap_sync() knows we're bouncing */
	cookie->id_flags |= _BUS_DMA_IS_BOUNCING;
	return 0;
}
#endif /* _MIPS_NEED_BUS_DMA_BOUNCE */

/*
 * Common function for DMA map creation.  May be called by bus-specific
 * DMA map creation functions.
 */
int
_bus_dmamap_create(bus_dma_tag_t t, bus_size_t size, int nsegments,
    bus_size_t maxsegsz, bus_size_t boundary, int flags, bus_dmamap_t *dmamp)
{
	struct mips_bus_dmamap *map;
	void *mapstore;
	size_t mapsize;
	const int mallocflags = M_ZERO |
	    ((flags & BUS_DMA_NOWAIT) ? M_NOWAIT : M_WAITOK);

	int error = 0;

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
	mapsize = sizeof(struct mips_bus_dmamap) +
	    (sizeof(bus_dma_segment_t) * (nsegments - 1));
	if ((mapstore = malloc(mapsize, M_DMAMAP, mallocflags)) == NULL)
		return (ENOMEM);

	map = mapstore;
	map->_dm_size = size;
	map->_dm_segcnt = nsegments;
	map->_dm_maxmaxsegsz = maxsegsz;
	map->_dm_boundary = boundary;
	map->_dm_bounce_thresh = t->_bounce_thresh;
	map->_dm_flags = flags & ~(BUS_DMA_WAITOK|BUS_DMA_NOWAIT);
	map->_dm_vmspace = NULL;
	map->dm_maxsegsz = maxsegsz;
	map->dm_mapsize = 0;		/* no valid mappings */
	map->dm_nsegs = 0;

	*dmamp = map;

#ifdef _MIPS_NEED_BUS_DMA_BOUNCE
	struct mips_bus_dma_cookie *cookie;
	int cookieflags;
	void *cookiestore;
	size_t cookiesize;

	if (t->_bounce_thresh == 0 || _BUS_AVAIL_END <= t->_bounce_thresh)
		map->_dm_bounce_thresh = 0;
	cookieflags = 0;

	if (t->_may_bounce != NULL) {
		error = (*t->_may_bounce)(t, map, flags, &cookieflags);
		if (error != 0)
			goto out;
	}

	if (map->_dm_bounce_thresh != 0)
		cookieflags |= _BUS_DMA_MIGHT_NEED_BOUNCE;

	if ((cookieflags & _BUS_DMA_MIGHT_NEED_BOUNCE) == 0) {
		STAT_INCR(creates);
		return 0;
	}

	cookiesize = sizeof(struct mips_bus_dma_cookie) +
	    (sizeof(bus_dma_segment_t) * map->_dm_segcnt);

	/*
	 * Allocate our cookie.
	 */
	if ((cookiestore = malloc(cookiesize, M_DMAMAP, mallocflags)) == NULL) {
		error = ENOMEM;
		goto out;
	}
	cookie = (struct mips_bus_dma_cookie *)cookiestore;
	cookie->id_flags = cookieflags;
	map->_dm_cookie = cookie;
	STAT_INCR(bounced_creates);

	error = _bus_dma_alloc_bouncebuf(t, map, size, flags);
 out:
	if (error)
		_bus_dmamap_destroy(t, map);
#else
	STAT_INCR(creates);
#endif /* _MIPS_NEED_BUS_DMA_BOUNCE */

	return (error);
}

/*
 * Common function for DMA map destruction.  May be called by bus-specific
 * DMA map destruction functions.
 */
void
_bus_dmamap_destroy(bus_dma_tag_t t, bus_dmamap_t map)
{

#ifdef _MIPS_NEED_BUS_DMA_BOUNCE
	struct mips_bus_dma_cookie *cookie = map->_dm_cookie;

	/*
	 * Free any bounce pages this map might hold.
	 */
	if (cookie != NULL) {
		if (cookie->id_flags & _BUS_DMA_IS_BOUNCING)
			STAT_INCR(bounced_unloads);
		map->dm_nsegs = 0;
		if (cookie->id_flags & _BUS_DMA_HAS_BOUNCE)
			_bus_dma_free_bouncebuf(t, map);
		STAT_INCR(bounced_destroys);
		free(cookie, M_DMAMAP);
	} else
#endif
	STAT_INCR(destroys);
	if (map->dm_nsegs > 0)
		STAT_INCR(unloads);
	free(map, M_DMAMAP);
}

/*
 * Common function for loading a direct-mapped DMA map with a linear
 * buffer.  Called by bus-specific DMA map load functions with the
 * OR value appropriate for indicating "direct-mapped" for that
 * chipset.
 */
int
_bus_dmamap_load(bus_dma_tag_t t, bus_dmamap_t map, void *buf,
    bus_size_t buflen, struct proc *p, int flags)
{
	int seg, error;
	struct vmspace *vm;

	if (map->dm_nsegs > 0) {
#ifdef _MIPS_NEED_BUS_DMA_BOUNCE
		struct mips_bus_dma_cookie *cookie = map->_dm_cookie;
		if (cookie != NULL) {
			if (cookie->id_flags & _BUS_DMA_IS_BOUNCING) {
				STAT_INCR(bounced_unloads);
				cookie->id_flags &= ~_BUS_DMA_IS_BOUNCING;
			}
			cookie->id_buftype = _BUS_DMA_BUFTYPE_INVALID;
		} else
#endif
		STAT_INCR(unloads);
	}
	/*
	 * Make sure that on error condition we return "no valid mappings".
	 */
	map->dm_mapsize = 0;
	map->dm_nsegs = 0;
	KASSERT(map->dm_maxsegsz <= map->_dm_maxmaxsegsz);

	if (buflen > map->_dm_size)
		return (EINVAL);

	if (p != NULL) {
		vm = p->p_vmspace;
	} else {
		vm = vmspace_kernel();
	}

	seg = 0;
	error = _bus_dmamap_load_buffer(t, map, buf, buflen,
	    vm, flags, &seg, 0, true);
	if (error == 0) {
		map->dm_mapsize = buflen;
		map->dm_nsegs = seg + 1;
		map->_dm_vmspace = vm;

		STAT_INCR(loads);

		/*
		 * For linear buffers, we support marking the mapping
		 * as COHERENT.
		 *
		 * XXX Check TLB entries for cache-inhibit bits?
		 */
		if (mips_options.mips_cpu_flags & CPU_MIPS_D_CACHE_COHERENT)
			map->_dm_flags |= _BUS_DMAMAP_COHERENT;
		else if (MIPS_KSEG1_P(buf))
			map->_dm_flags |= _BUS_DMAMAP_COHERENT;
#ifdef _LP64
		else if (MIPS_XKPHYS_P((vaddr_t)buf)
		    && MIPS_XKPHYS_TO_CCA((vaddr_t)buf) == MIPS3_PG_TO_CCA(MIPS3_PG_UNCACHED))
			map->_dm_flags |= _BUS_DMAMAP_COHERENT;
#endif
		return 0;
	}
#ifdef _MIPS_NEED_BUS_DMA_BOUNCE
	struct mips_bus_dma_cookie *cookie = map->_dm_cookie;
	if (cookie != NULL && (cookie->id_flags & _BUS_DMA_MIGHT_NEED_BOUNCE)) {
		error = _bus_dma_load_bouncebuf(t, map, buf, buflen,
		    _BUS_DMA_BUFTYPE_LINEAR, flags);
	}
#endif
	return (error);
}

/*
 * Like _bus_dmamap_load(), but for mbufs.
 */
int
_bus_dmamap_load_mbuf(bus_dma_tag_t t, bus_dmamap_t map,
    struct mbuf *m0, int flags)
{
	int seg, error;
	struct mbuf *m;
	struct vmspace * vm = vmspace_kernel();

	if (map->dm_nsegs > 0) {
#ifdef _MIPS_NEED_BUS_DMA_BOUNCE
		struct mips_bus_dma_cookie *cookie = map->_dm_cookie;
		if (cookie != NULL) {
			if (cookie->id_flags & _BUS_DMA_IS_BOUNCING) {
				STAT_INCR(bounced_unloads);
				cookie->id_flags &= ~_BUS_DMA_IS_BOUNCING;
			}
			cookie->id_buftype = _BUS_DMA_BUFTYPE_INVALID;
		} else
#endif
		STAT_INCR(unloads);
	}

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
		return (EINVAL);

	vaddr_t lastvaddr = 0;
	bool first = true;
	seg = 0;
	error = 0;
	for (m = m0; m != NULL && error == 0; m = m->m_next) {
		if (m->m_len == 0)
			continue;
		error = _bus_dmamap_load_buffer(t, map, m->m_data, m->m_len,
		    vm, flags, &seg, lastvaddr, first);
		first = false;
		lastvaddr = (vaddr_t)m->m_data + m->m_len;
	}
	if (error == 0) {
		map->dm_mapsize = m0->m_pkthdr.len;
		map->dm_nsegs = seg + 1;
		map->_dm_vmspace = vm;		/* always kernel */
		/*
		 * If our cache is coherent, then the map must be coherent too.
		 */
		if (mips_options.mips_cpu_flags & CPU_MIPS_D_CACHE_COHERENT)
			map->_dm_flags |= _BUS_DMAMAP_COHERENT;
		return 0;
	}
#ifdef _MIPS_NEED_BUS_DMA_BOUNCE
	struct mips_bus_dma_cookie * cookie = map->_dm_cookie;
	if (cookie != NULL && (cookie->id_flags & _BUS_DMA_MIGHT_NEED_BOUNCE)) {
		error = _bus_dma_load_bouncebuf(t, map, m0, m0->m_pkthdr.len,
		    _BUS_DMA_BUFTYPE_MBUF, flags);
	}
#endif /* _MIPS_NEED_BUS_DMA_BOUNCE */
	return (error);
}

/*
 * Like _bus_dmamap_load(), but for uios.
 */
int
_bus_dmamap_load_uio(bus_dma_tag_t t, bus_dmamap_t map,
    struct uio *uio, int flags)
{
	int seg, i, error;
	bus_size_t minlen, resid;
	struct iovec *iov;
	void *addr;

	if (map->dm_nsegs > 0) {
#ifdef _MIPS_NEED_BUS_DMA_BOUNCE
		struct mips_bus_dma_cookie * const cookie = map->_dm_cookie;
		if (cookie != NULL) {
			if (cookie->id_flags & _BUS_DMA_IS_BOUNCING) {
				STAT_INCR(bounced_unloads);
				cookie->id_flags &= ~_BUS_DMA_IS_BOUNCING;
			}
			cookie->id_buftype = _BUS_DMA_BUFTYPE_INVALID;
		} else
#endif
		STAT_INCR(unloads);
	}
	/*
	 * Make sure that on error condition we return "no valid mappings."
	 */
	map->dm_mapsize = 0;
	map->dm_nsegs = 0;
	KASSERT(map->dm_maxsegsz <= map->_dm_maxmaxsegsz);

	resid = uio->uio_resid;
	iov = uio->uio_iov;

	vaddr_t lastvaddr = 0;
	bool first = true;
	seg = 0;
	error = 0;
	for (i = 0; i < uio->uio_iovcnt && resid != 0 && error == 0; i++) {
		/*
		 * Now at the first iovec to load.  Load each iovec
		 * until we have exhausted the residual count.
		 */
		minlen = resid < iov[i].iov_len ? resid : iov[i].iov_len;
		addr = (void *)iov[i].iov_base;

		error = _bus_dmamap_load_buffer(t, map, addr, minlen,
		    uio->uio_vmspace, flags, &seg, lastvaddr, first);
		first = false;
		lastvaddr = (vaddr_t)addr + minlen;

		resid -= minlen;
	}
	if (error == 0) {
		map->dm_mapsize = uio->uio_resid;
		map->dm_nsegs = seg + 1;
		map->_dm_vmspace = uio->uio_vmspace;
		/*
		 * If our cache is coherent, then the map must be coherent too.
		 */
		if (mips_options.mips_cpu_flags & CPU_MIPS_D_CACHE_COHERENT)
			map->_dm_flags |= _BUS_DMAMAP_COHERENT;
		return 0;
	}
#ifdef _MIPS_NEED_BUS_DMA_BOUNCE
	struct mips_bus_dma_cookie *cookie = map->_dm_cookie;
	if (cookie != NULL && (cookie->id_flags & _BUS_DMA_MIGHT_NEED_BOUNCE)) {
		error = _bus_dma_load_bouncebuf(t, map, uio, uio->uio_resid,
		    _BUS_DMA_BUFTYPE_UIO, flags);
	}
#endif
	return (error);
}

/*
 * Like _bus_dmamap_load(), but for raw memory.
 */
int
_bus_dmamap_load_raw(bus_dma_tag_t t, bus_dmamap_t map,
    bus_dma_segment_t *segs, int nsegs, bus_size_t size, int flags)
{

	struct vmspace * const vm = vmspace_kernel();
	const bool coherent_p = (mips_options.mips_cpu_flags & CPU_MIPS_D_CACHE_COHERENT);
	const bool cached_p = coherent_p || (flags & BUS_DMA_COHERENT) == 0;
	bus_size_t mapsize = 0;
	vaddr_t lastvaddr = 0;
	bool first = true;
	int curseg = 0;
	int error = 0;

	for (; error == 0 && nsegs-- > 0; segs++) {
		void *kva;
#ifdef _LP64
		if (cached_p) {
			kva = (void *)MIPS_PHYS_TO_XKPHYS_CACHED(segs->ds_addr);
		} else {
			kva = (void *)MIPS_PHYS_TO_XKPHYS_UNCACHED(segs->ds_addr);
		}
#else
		if (segs->ds_addr >= MIPS_PHYS_MASK)
			return EFBIG;
		if (cached_p) {
			kva = (void *)MIPS_PHYS_TO_KSEG0(segs->ds_addr);
		} else {
			kva = (void *)MIPS_PHYS_TO_KSEG1(segs->ds_addr);
		}
#endif	/* _LP64 */
		mapsize += segs->ds_len;
		error = _bus_dmamap_load_buffer(t, map, kva, segs->ds_len,
		    vm, flags, &curseg, lastvaddr, first);
		first = false;
		lastvaddr = (vaddr_t)kva + segs->ds_len;
	}
	if (error == 0) {
		map->dm_mapsize = mapsize;
		map->dm_nsegs = curseg + 1;
		map->_dm_vmspace = vm;		/* always kernel */
		/*
		 * If our cache is coherent, then the map must be coherent too.
		 */
		if (coherent_p)
			map->_dm_flags |= _BUS_DMAMAP_COHERENT;
		return 0;
	}
	/*
	 * If bus_dmamem_alloc didn't return memory that didn't need bouncing
	 * that's a bug which we will not workaround.
	 */
	return error;
}

/*
 * Common function for unloading a DMA map.  May be called by
 * chipset-specific DMA map unload functions.
 */
void
_bus_dmamap_unload(bus_dma_tag_t t, bus_dmamap_t map)
{
	if (map->dm_nsegs > 0) {
#ifdef _MIPS_NEED_BUS_DMA_BOUNCE
		struct mips_bus_dma_cookie *cookie = map->_dm_cookie;
		if (cookie != NULL) {
			if (cookie->id_flags & _BUS_DMA_IS_BOUNCING) {
				cookie->id_flags &= ~_BUS_DMA_IS_BOUNCING;
				STAT_INCR(bounced_unloads);
			}
			cookie->id_buftype = _BUS_DMA_BUFTYPE_INVALID;
		} else
#endif

		STAT_INCR(unloads);
	}
	/*
	 * No resources to free; just mark the mappings as
	 * invalid.
	 */
	map->dm_maxsegsz = map->_dm_maxmaxsegsz;
	map->dm_mapsize = 0;
	map->dm_nsegs = 0;
	map->_dm_flags &= ~_BUS_DMAMAP_COHERENT;
}

/*
 * Common function for DMA map synchronization.  May be called
 * by chipset-specific DMA map synchronization functions.
 *
 * This version works with the virtually-indexed, write-back cache
 * found in the MIPS-3/MIPS-4 CPUs available for the Algorithmics.
 */
void
_bus_dmamap_sync(bus_dma_tag_t t, bus_dmamap_t map, bus_addr_t offset,
    bus_size_t len, int ops)
{
	bus_size_t minlen;

#ifdef DIAGNOSTIC
	/*
	 * Mixing PRE and POST operations is not allowed.
	 */
	if ((ops & (BUS_DMASYNC_PREREAD|BUS_DMASYNC_PREWRITE)) != 0 &&
	    (ops & (BUS_DMASYNC_POSTREAD|BUS_DMASYNC_POSTWRITE)) != 0)
		panic("_bus_dmamap_sync: mix PRE and POST");

	if (offset >= map->dm_mapsize)
		panic("_bus_dmamap_sync: bad offset %"PRIxPADDR
			" (map size is %"PRIxPSIZE")",
				offset, map->dm_mapsize);
	if (len == 0 || (offset + len) > map->dm_mapsize)
		panic("_bus_dmamap_sync: bad length");
#endif

	/*
	 * Since we're dealing with a virtually-indexed, write-back
	 * cache, we need to do the following things:
	 *
	 *	PREREAD -- Invalidate D-cache.  Note we might have
	 *	to also write-back here if we have to use an Index
	 *	op, or if the buffer start/end is not cache-line aligned.
	 *
	 *	PREWRITE -- Write-back the D-cache.  If we have to use
	 *	an Index op, we also have to invalidate.  Note that if
	 *	we are doing PREREAD|PREWRITE, we can collapse everything
	 *	into a single op.
	 *
	 *	POSTREAD -- Nothing.
	 *
	 *	POSTWRITE -- Nothing.
	 */
#ifdef _MIPS_NEED_BUS_DMA_BOUNCE
	struct mips_bus_dma_cookie * const cookie = map->_dm_cookie;
	if (cookie != NULL && (cookie->id_flags & _BUS_DMA_IS_BOUNCING)
	    && (ops & BUS_DMASYNC_PREWRITE)) {
		STAT_INCR(write_bounces);
		/*
		 * Copy the caller's buffer to the bounce buffer.
		 */
		switch (cookie->id_buftype) {
		case _BUS_DMA_BUFTYPE_LINEAR:
			memcpy((char *)cookie->id_bouncebuf + offset,
			    cookie->id_origlinearbuf + offset, len);
			break;
		case _BUS_DMA_BUFTYPE_MBUF:
			m_copydata(cookie->id_origmbuf, offset, len,
			    (char *)cookie->id_bouncebuf + offset);
			break;
		case _BUS_DMA_BUFTYPE_UIO:
			_bus_dma_uiomove((char *)cookie->id_bouncebuf + offset,
			    cookie->id_origuio, len, UIO_WRITE);
			break;
#ifdef DIAGNOSTIC
		case _BUS_DMA_BUFTYPE_RAW:
			panic("_bus_dmamap_sync: _BUS_DMA_BUFTYPE_RAW");
			break;

		case _BUS_DMA_BUFTYPE_INVALID:
			panic("_bus_dmamap_sync: _BUS_DMA_BUFTYPE_INVALID");
			break;

		default:
			panic("_bus_dmamap_sync: unknown buffer type %d\n",
			    cookie->id_buftype);
			break;
#endif /* DIAGNOSTIC */
		}
	}
#endif /* _MIPS_NEED_BUS_DMA_BOUNCE */

	/*
	 * Flush the write buffer.
	 * XXX Is this always necessary?
	 */
	wbflush();

	/*
	 * If the mapping is of COHERENT DMA-safe memory or this isn't a
	 * PREREAD or PREWRITE, no cache flush is necessary.  Check to see
	 * if we need to bounce it.
	 */
	if ((map->_dm_flags & _BUS_DMAMAP_COHERENT)
	    || (ops & (BUS_DMASYNC_PREREAD|BUS_DMASYNC_PREWRITE)) == 0)
		goto bounce_it;

#ifdef _mips_o32
	/*
	 * If the mapping belongs to the kernel, or it belongs
	 * to the currently-running process (XXX actually, vmspace),
	 * then we can use Hit ops.  Otherwise, Index ops.
	 *
	 * This should be true the vast majority of the time.
	 */
	const bool useindex = (!VMSPACE_IS_KERNEL_P(map->_dm_vmspace)
	    && map->_dm_vmspace != curproc->p_vmspace);
#endif

	bus_dma_segment_t *seg = map->dm_segs;
	bus_dma_segment_t * const lastseg = seg + map->dm_nsegs;
	/*
	 * Skip segments until offset are within a segment.
	 */
	for (; offset >= seg->ds_len; seg++) {
		offset -= seg->ds_len;
	}

	for (; seg < lastseg && len != 0; seg++, offset = 0, len -= minlen) {
		/*
		 * Now at the first segment to sync; nail each segment until we
		 * have exhausted the length.
		 */
		register_t vaddr = seg->_ds_vaddr + offset;
		minlen = ulmin(len, seg->ds_len - offset);

#ifdef BUS_DMA_DEBUG
		printf("bus_dmamap_sync(op=%d: flushing segment %p "
		    "(0x%"PRIxREGISTER"+%"PRIxBUSADDR
		    ", 0x%"PRIxREGISTER"+0x%"PRIxBUSADDR
		    ") (olen = %"PRIxBUSADDR")...", op, seg,
		    vaddr - offset, offset,
		    vaddr - offset, offset + minlen - 1, len);
#endif

		/*
		 * If we are forced to use Index ops, it's always a
		 * Write-back,Invalidate, so just do one test.
		 */
#ifdef mips_o32
		if (__predict_false(useindex || vaddr == 0)) {
			mips_dcache_wbinv_range_index(vaddr, minlen);
#ifdef BUS_DMA_DEBUG
			printf("\n");
#endif
			continue;
		}
#endif

		switch (ops) {
		case BUS_DMASYNC_PREREAD|BUS_DMASYNC_PREWRITE:
			mips_dcache_wbinv_range(vaddr, minlen);
			break;

		case BUS_DMASYNC_PREREAD: {
			struct mips_cache_info * const mci = &mips_cache_info;
			register_t start = vaddr;
			register_t end = vaddr + minlen;
			register_t preboundary, firstboundary, lastboundary;
			register_t mask = mci->mci_dcache_align_mask;

			preboundary = start & ~mask;
			firstboundary = (start + mask) & ~mask;
			lastboundary = end & ~mask;
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
			mips_dcache_wb_range(vaddr, minlen);
			break;
		}
#ifdef BUS_DMA_DEBUG
		printf("\n");
#endif
	}

  bounce_it:
#ifdef _MIPS_NEED_BUS_DMA_BOUNCE
	if ((ops & BUS_DMASYNC_POSTREAD) == 0
	    || cookie == NULL
	    || (cookie->id_flags & _BUS_DMA_IS_BOUNCING) == 0)
		return;

	STAT_INCR(read_bounces);
	/*
	 * Copy the bounce buffer to the caller's buffer.
	 */
	switch (cookie->id_buftype) {
	case _BUS_DMA_BUFTYPE_LINEAR:
		memcpy(cookie->id_origlinearbuf + offset,
		    (char *)cookie->id_bouncebuf + offset, len);
		break;

	case _BUS_DMA_BUFTYPE_MBUF:
		m_copyback(cookie->id_origmbuf, offset, len,
		    (char *)cookie->id_bouncebuf + offset);
		break;

	case _BUS_DMA_BUFTYPE_UIO:
		_bus_dma_uiomove((char *)cookie->id_bouncebuf + offset,
		    cookie->id_origuio, len, UIO_READ);
		break;
#ifdef DIAGNOSTIC
	case _BUS_DMA_BUFTYPE_RAW:
		panic("_bus_dmamap_sync: _BUS_DMA_BUFTYPE_RAW");
		break;

	case _BUS_DMA_BUFTYPE_INVALID:
		panic("_bus_dmamap_sync: _BUS_DMA_BUFTYPE_INVALID");
		break;

	default:
		panic("_bus_dmamap_sync: unknown buffer type %d\n",
		    cookie->id_buftype);
		break;
#endif
	}
#endif /* _MIPS_NEED_BUS_DMA_BOUNCE */
	;
}

/*
 * Common function for DMA-safe memory allocation.  May be called
 * by bus-specific DMA memory allocation functions.
 */
int
_bus_dmamem_alloc(bus_dma_tag_t t, bus_size_t size, bus_size_t alignment,
    bus_size_t boundary, bus_dma_segment_t *segs, int nsegs, int *rsegs,
    int flags)
{
	bus_addr_t high;

	if (t->_bounce_alloc_hi != 0 && _BUS_AVAIL_END > t->_bounce_alloc_hi)
		high = trunc_page(t->_bounce_alloc_hi);
	else
		high = trunc_page(_BUS_AVAIL_END);

	return _bus_dmamem_alloc_range(t, size, alignment, boundary,
	    segs, nsegs, rsegs, flags, t->_bounce_alloc_lo, high);
}

/*
 * Allocate physical memory from the given physical address range.
 * Called by DMA-safe memory allocation methods.
 */
int
_bus_dmamem_alloc_range(bus_dma_tag_t t, bus_size_t size, bus_size_t alignment,
    bus_size_t boundary, bus_dma_segment_t *segs, int nsegs, int *rsegs,
    int flags, paddr_t low, paddr_t high)
{
	paddr_t curaddr, lastaddr;
	struct vm_page *m;
	struct pglist mlist;
	int curseg, error;

	/* Always round the size. */
	size = round_page(size);

	/*
	 * Allocate pages from the VM system.
	 */
	error = uvm_pglistalloc(size, low, high, alignment, boundary,
	    &mlist, nsegs, (flags & BUS_DMA_NOWAIT) == 0);
	if (error)
		return (error);

	/*
	 * Compute the location, size, and number of segments actually
	 * returned by the VM code.
	 */
	m = TAILQ_FIRST(&mlist);
	curseg = 0;
	lastaddr = segs[curseg].ds_addr = VM_PAGE_TO_PHYS(m);
	segs[curseg].ds_len = PAGE_SIZE;
	m = TAILQ_NEXT(m, pageq.queue);

	for (; m != NULL; m = TAILQ_NEXT(m, pageq.queue)) {
		curaddr = VM_PAGE_TO_PHYS(m);
#ifdef DIAGNOSTIC
		if (curaddr < low || curaddr >= high) {
			printf("uvm_pglistalloc returned non-sensical"
			    " address 0x%"PRIxPADDR"\n", curaddr);
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

	return (0);
}

/*
 * Common function for freeing DMA-safe memory.  May be called by
 * bus-specific DMA memory free functions.
 */
void
_bus_dmamem_free(bus_dma_tag_t t, bus_dma_segment_t *segs, int nsegs)
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
			TAILQ_INSERT_TAIL(&mlist, m, pageq.queue);
		}
	}

	uvm_pglistfree(&mlist);
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

	/*
	 * If we're only mapping 1 segment, use K0SEG, to avoid
	 * TLB thrashing.
	 */
#ifdef _LP64
	if (nsegs == 1) {
		if (((mips_options.mips_cpu_flags & CPU_MIPS_D_CACHE_COHERENT) == 0)
		&&  (flags & BUS_DMA_COHERENT))
			*kvap = (void *)MIPS_PHYS_TO_XKPHYS_UNCACHED(
			    segs[0].ds_addr);
		else
			*kvap = (void *)MIPS_PHYS_TO_XKPHYS_CACHED(
			    segs[0].ds_addr);
		return 0;
	}
#else
	if ((nsegs == 1) && (segs[0].ds_addr < MIPS_PHYS_MASK)) {
		if (((mips_options.mips_cpu_flags & CPU_MIPS_D_CACHE_COHERENT) == 0)
		&&  (flags & BUS_DMA_COHERENT))
			*kvap = (void *)MIPS_PHYS_TO_KSEG1(segs[0].ds_addr);
		else
			*kvap = (void *)MIPS_PHYS_TO_KSEG0(segs[0].ds_addr);
		return (0);
	}
#endif	/* _LP64 */

	size = round_page(size);

	va = uvm_km_alloc(kernel_map, size, 0, UVM_KMF_VAONLY | kmflags);

	if (va == 0)
		return (ENOMEM);

	*kvap = (void *)va;

	for (curseg = 0; curseg < nsegs; curseg++) {
		for (addr = trunc_page(segs[curseg].ds_addr);
		    addr < (segs[curseg].ds_addr + segs[curseg].ds_len);
		    addr += PAGE_SIZE, va += PAGE_SIZE, size -= PAGE_SIZE) {
			if (size == 0)
				panic("_bus_dmamem_map: size botch");
			pmap_enter(pmap_kernel(), va, addr,
			    VM_PROT_READ | VM_PROT_WRITE,
			    PMAP_WIRED | VM_PROT_READ | VM_PROT_WRITE);
		}
	}
	pmap_update(pmap_kernel());

	return (0);
}

/*
 * Common function for unmapping DMA-safe memory.  May be called by
 * bus-specific DMA memory unmapping functions.
 */
void
_bus_dmamem_unmap(bus_dma_tag_t t, void *kva, size_t size)
{

#ifdef DIAGNOSTIC
	if ((uintptr_t)kva & PGOFSET)
		panic("_bus_dmamem_unmap: bad alignment on %p", kva);
#endif

	/*
	 * Nothing to do if we mapped it with KSEG0 or KSEG1 (i.e.
	 * not in KSEG2 or XKSEG).
	 */
	if (MIPS_KSEG0_P(kva) || MIPS_KSEG1_P(kva))
		return;
#ifdef _LP64
	if (MIPS_XKPHYS_P((vaddr_t)kva))
		return;
#endif

	size = round_page(size);
	pmap_remove(pmap_kernel(), (vaddr_t)kva, (vaddr_t)kva + size);
	pmap_update(pmap_kernel());
	uvm_km_free(kernel_map, (vaddr_t)kva, size, UVM_KMF_VAONLY);
}

/*
 * Common functin for mmap(2)'ing DMA-safe memory.  May be called by
 * bus-specific DMA mmap(2)'ing functions.
 */
paddr_t
_bus_dmamem_mmap(bus_dma_tag_t t, bus_dma_segment_t *segs, int nsegs,
    off_t off, int prot, int flags)
{
	int i;
	paddr_t pa;

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

		pa = (paddr_t)segs[i].ds_addr + off;

/*
 * This is for machines which use normal RAM as video memory, so userland can
 * mmap() it and treat it like device memory, which is normally uncached.
 * Needed for X11 on SGI O2, will likely be needed on things like CI20.
 */
#if defined(_MIPS_PADDR_T_64BIT) || defined(_LP64)
		if (flags & BUS_DMA_PREFETCHABLE ) {
			return (mips_btop(pa | PGC_NOCACHE));
		} else
			return mips_btop(pa);
#else
		return mips_btop(pa);
#endif
	}

	/* Page not found. */
	return (-1);
}

#ifdef _MIPS_NEED_BUS_DMA_BOUNCE
static int
_bus_dma_alloc_bouncebuf(bus_dma_tag_t t, bus_dmamap_t map,
    bus_size_t size, int flags)
{
	struct mips_bus_dma_cookie *cookie = map->_dm_cookie;
	int error = 0;

#ifdef DIAGNOSTIC
	if (cookie == NULL)
		panic("_bus_dma_alloc_bouncebuf: no cookie");
#endif

	cookie->id_bouncebuflen = round_page(size);
	error = _bus_dmamem_alloc(t, cookie->id_bouncebuflen,
	    PAGE_SIZE, map->_dm_boundary, cookie->id_bouncesegs,
	    map->_dm_segcnt, &cookie->id_nbouncesegs, flags);
	if (error)
		goto out;
	error = _bus_dmamem_map(t, cookie->id_bouncesegs,
	    cookie->id_nbouncesegs, cookie->id_bouncebuflen,
	    (void **)&cookie->id_bouncebuf, flags);

 out:
	if (error) {
		_bus_dmamem_free(t, cookie->id_bouncesegs,
		    cookie->id_nbouncesegs);
		cookie->id_bouncebuflen = 0;
		cookie->id_nbouncesegs = 0;
	} else {
		cookie->id_flags |= _BUS_DMA_HAS_BOUNCE;
	}

	return (error);
}

static void
_bus_dma_free_bouncebuf(bus_dma_tag_t t, bus_dmamap_t map)
{
	struct mips_bus_dma_cookie *cookie = map->_dm_cookie;

#ifdef DIAGNOSTIC
	if (cookie == NULL)
		panic("_bus_dma_alloc_bouncebuf: no cookie");
#endif

	_bus_dmamem_unmap(t, cookie->id_bouncebuf, cookie->id_bouncebuflen);
	_bus_dmamem_free(t, cookie->id_bouncesegs,
	    cookie->id_nbouncesegs);
	cookie->id_bouncebuflen = 0;
	cookie->id_nbouncesegs = 0;
	cookie->id_flags &= ~_BUS_DMA_HAS_BOUNCE;
}

/*
 * This function does the same as uiomove, but takes an explicit
 * direction, and does not update the uio structure.
 */
static int
_bus_dma_uiomove(void *buf, struct uio *uio, size_t n, int direction)
{
	struct iovec *iov;
	int error;
	struct vmspace *vm;
	char *cp;
	size_t resid, cnt;
	int i;

	iov = uio->uio_iov;
	vm = uio->uio_vmspace;
	cp = buf;
	resid = n;

	for (i = 0; i < uio->uio_iovcnt && resid > 0; i++) {
		iov = &uio->uio_iov[i];
		if (iov->iov_len == 0)
			continue;
		cnt = MIN(resid, iov->iov_len);

		if (!VMSPACE_IS_KERNEL_P(vm) &&
		    (curlwp->l_cpu->ci_schedstate.spc_flags & SPCF_SHOULDYIELD)
		    != 0) {
			preempt();
		}
		if (direction == UIO_READ) {
			error = copyout_vmspace(vm, cp, iov->iov_base, cnt);
		} else {
			error = copyin_vmspace(vm, iov->iov_base, cp, cnt);
		}
		if (error)
			return (error);
		cp += cnt;
		resid -= cnt;
	}
	return (0);
}
#endif /* _MIPS_NEED_BUS_DMA_BOUNCE */

int
_bus_dmatag_subregion(bus_dma_tag_t tag, bus_addr_t min_addr,
		      bus_addr_t max_addr, bus_dma_tag_t *newtag, int flags)
{

#ifdef _MIPS_NEED_BUS_DMA_BOUNCE
	if ((((tag->_bounce_thresh != 0   && max_addr >= tag->_bounce_thresh)
	      && (tag->_bounce_alloc_hi != 0 && max_addr >= tag->_bounce_alloc_hi))
	     || (tag->_bounce_alloc_hi == 0 && max_addr > _BUS_AVAIL_END))
	    && (min_addr <= tag->_bounce_alloc_lo)) {
		*newtag = tag;
		/* if the tag must be freed, add a reference */
		if (tag->_tag_needs_free)
			(tag->_tag_needs_free)++;
		return 0;
	}

	if ((*newtag = malloc(sizeof(struct mips_bus_dma_tag), M_DMAMAP,
	    (flags & BUS_DMA_NOWAIT) ? M_NOWAIT : M_WAITOK)) == NULL)
		return ENOMEM;

	**newtag = *tag;
	(*newtag)->_tag_needs_free = 1;

	if (tag->_bounce_thresh == 0 || max_addr < tag->_bounce_thresh)
		(*newtag)->_bounce_thresh = max_addr;
	if (tag->_bounce_alloc_hi == 0 || max_addr < tag->_bounce_alloc_hi)
		(*newtag)->_bounce_alloc_hi = max_addr;
	if (min_addr > tag->_bounce_alloc_lo)
		(*newtag)->_bounce_alloc_lo = min_addr;
	(*newtag)->_wbase += (*newtag)->_bounce_alloc_lo - tag->_bounce_alloc_lo;

	return 0;
#else
	return EOPNOTSUPP;
#endif /* _MIPS_NEED_BUS_DMA_BOUNCE */
}

void
_bus_dmatag_destroy(bus_dma_tag_t tag)
{
#ifdef _MIPS_NEED_BUS_DMA_BOUNCE
	switch (tag->_tag_needs_free) {
	case 0:
		break;				/* not allocated with malloc */
	case 1:
		free(tag, M_DMAMAP);		/* last reference to tag */
		break;
	default:
		(tag->_tag_needs_free)--;	/* one less reference */
	}
#endif
}
