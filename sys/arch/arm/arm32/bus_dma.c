/*	$NetBSD: bus_dma.c,v 1.77 2013/02/14 08:24:39 matt Exp $	*/

/*-
 * Copyright (c) 1996, 1997, 1998 The NetBSD Foundation, Inc.
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

#define _ARM32_BUS_DMA_PRIVATE

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: bus_dma.c,v 1.77 2013/02/14 08:24:39 matt Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/proc.h>
#include <sys/buf.h>
#include <sys/reboot.h>
#include <sys/conf.h>
#include <sys/file.h>
#include <sys/malloc.h>
#include <sys/mbuf.h>
#include <sys/vnode.h>
#include <sys/device.h>

#include <uvm/uvm.h>

#include <sys/bus.h>
#include <machine/cpu.h>

#include <arm/cpufunc.h>

#ifdef BUSDMA_COUNTERS
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
static struct evcnt bus_dma_sync_prereadwrite = 
	EVCNT_INITIALIZER(EVCNT_TYPE_MISC, NULL, "busdma", "sync prereadwrite");
static struct evcnt bus_dma_sync_preread_begin =
	EVCNT_INITIALIZER(EVCNT_TYPE_MISC, NULL, "busdma", "sync preread begin");
static struct evcnt bus_dma_sync_preread =
	EVCNT_INITIALIZER(EVCNT_TYPE_MISC, NULL, "busdma", "sync preread");
static struct evcnt bus_dma_sync_preread_tail =
	EVCNT_INITIALIZER(EVCNT_TYPE_MISC, NULL, "busdma", "sync preread tail");
static struct evcnt bus_dma_sync_prewrite = 
	EVCNT_INITIALIZER(EVCNT_TYPE_MISC, NULL, "busdma", "sync prewrite");
static struct evcnt bus_dma_sync_postread = 
	EVCNT_INITIALIZER(EVCNT_TYPE_MISC, NULL, "busdma", "sync postread");
static struct evcnt bus_dma_sync_postreadwrite = 
	EVCNT_INITIALIZER(EVCNT_TYPE_MISC, NULL, "busdma", "sync postreadwrite");
static struct evcnt bus_dma_sync_postwrite = 
	EVCNT_INITIALIZER(EVCNT_TYPE_MISC, NULL, "busdma", "sync postwrite");

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
EVCNT_ATTACH_STATIC(bus_dma_sync_prereadwrite);
EVCNT_ATTACH_STATIC(bus_dma_sync_preread_begin);
EVCNT_ATTACH_STATIC(bus_dma_sync_preread);
EVCNT_ATTACH_STATIC(bus_dma_sync_preread_tail);
EVCNT_ATTACH_STATIC(bus_dma_sync_prewrite);
EVCNT_ATTACH_STATIC(bus_dma_sync_postread);
EVCNT_ATTACH_STATIC(bus_dma_sync_postreadwrite);
EVCNT_ATTACH_STATIC(bus_dma_sync_postwrite);

#define	STAT_INCR(x)	(bus_dma_ ## x.ev_count++)
#else
#define	STAT_INCR(x)	/*(bus_dma_ ## x.ev_count++)*/
#endif

int	_bus_dmamap_load_buffer(bus_dma_tag_t, bus_dmamap_t, void *,
	    bus_size_t, struct vmspace *, int);
static struct arm32_dma_range *
	_bus_dma_paddr_inrange(struct arm32_dma_range *, int, paddr_t);

/*
 * Check to see if the specified page is in an allowed DMA range.
 */
inline struct arm32_dma_range *
_bus_dma_paddr_inrange(struct arm32_dma_range *ranges, int nranges,
    bus_addr_t curaddr)
{
	struct arm32_dma_range *dr;
	int i;

	for (i = 0, dr = ranges; i < nranges; i++, dr++) {
		if (curaddr >= dr->dr_sysbase &&
		    round_page(curaddr) <= (dr->dr_sysbase + dr->dr_len))
			return (dr);
	}

	return (NULL);
}

/*
 * Check to see if the specified busaddr is in an allowed DMA range.
 */
static inline paddr_t
_bus_dma_busaddr_to_paddr(bus_dma_tag_t t, bus_addr_t curaddr)
{
	struct arm32_dma_range *dr;
	u_int i;

	if (t->_nranges == 0)
		return curaddr;

	for (i = 0, dr = t->_ranges; i < t->_nranges; i++, dr++) {
		if (dr->dr_busbase <= curaddr
		    && round_page(curaddr) <= dr->dr_busbase + dr->dr_len)
			return curaddr - dr->dr_busbase + dr->dr_sysbase;
	}
	panic("%s: curaddr %#lx not in range", __func__, curaddr);
}

/*
 * Common function to load the specified physical address into the
 * DMA map, coalescing segments and boundary checking as necessary.
 */
static int
_bus_dmamap_load_paddr(bus_dma_tag_t t, bus_dmamap_t map,
    bus_addr_t paddr, bus_size_t size, bool coherent)
{
	bus_dma_segment_t * const segs = map->dm_segs;
	int nseg = map->dm_nsegs;
	bus_addr_t lastaddr;
	bus_addr_t bmask = ~(map->_dm_boundary - 1);
	bus_addr_t curaddr;
	bus_size_t sgsize;
	uint32_t _ds_flags = coherent ? _BUS_DMAMAP_COHERENT : 0;

	if (nseg > 0)
		lastaddr = segs[nseg-1].ds_addr + segs[nseg-1].ds_len;
	else
		lastaddr = 0xdead;
	
 again:
	sgsize = size;

	/* Make sure we're in an allowed DMA range. */
	if (t->_ranges != NULL) {
		/* XXX cache last result? */
		const struct arm32_dma_range * const dr =
		    _bus_dma_paddr_inrange(t->_ranges, t->_nranges, paddr);
		if (dr == NULL)
			return (EINVAL);

		/*
		 * If this region is coherent, mark the segment as coherent.
		 */
		_ds_flags |= dr->dr_flags & _BUS_DMAMAP_COHERENT;

		/*
		 * In a valid DMA range.  Translate the physical
		 * memory address to an address in the DMA window.
		 */
		curaddr = (paddr - dr->dr_sysbase) + dr->dr_busbase;
#if 0
		printf("%p: %#lx: range %#lx/%#lx/%#lx/%#x: %#x <-- %#lx\n",
		    t, paddr, dr->dr_sysbase, dr->dr_busbase,
		    dr->dr_len, dr->dr_flags, _ds_flags, curaddr);
#endif
	} else
		curaddr = paddr;

	/*
	 * Make sure we don't cross any boundaries.
	 */
	if (map->_dm_boundary > 0) {
		bus_addr_t baddr;	/* next boundary address */

		baddr = (curaddr + map->_dm_boundary) & bmask;
		if (sgsize > (baddr - curaddr))
			sgsize = (baddr - curaddr);
	}

	/*
	 * Insert chunk into a segment, coalescing with the
	 * previous segment if possible.
	 */
	if (nseg > 0 && curaddr == lastaddr &&
	    segs[nseg-1].ds_len + sgsize <= map->dm_maxsegsz &&
	    ((segs[nseg-1]._ds_flags ^ _ds_flags) & _BUS_DMAMAP_COHERENT) == 0 &&
	    (map->_dm_boundary == 0 ||
	     (segs[nseg-1].ds_addr & bmask) == (curaddr & bmask))) {
	     	/* coalesce */
		segs[nseg-1].ds_len += sgsize;
	} else if (nseg >= map->_dm_segcnt) {
		return (EFBIG);
	} else {
		/* new segment */
		segs[nseg].ds_addr = curaddr;
		segs[nseg].ds_len = sgsize;
		segs[nseg]._ds_flags = _ds_flags;
		nseg++;
	}

	lastaddr = curaddr + sgsize;

	paddr += sgsize;
	size -= sgsize;
	if (size > 0)
		goto again;

	map->_dm_flags &= (_ds_flags & _BUS_DMAMAP_COHERENT);
	map->dm_nsegs = nseg;
	return (0);
}

#ifdef _ARM32_NEED_BUS_DMA_BOUNCE
static int _bus_dma_alloc_bouncebuf(bus_dma_tag_t t, bus_dmamap_t map,
	    bus_size_t size, int flags);
static void _bus_dma_free_bouncebuf(bus_dma_tag_t t, bus_dmamap_t map);
static int _bus_dma_uiomove(void *buf, struct uio *uio, size_t n,
	    int direction);

static int
_bus_dma_load_bouncebuf(bus_dma_tag_t t, bus_dmamap_t map, void *buf,
	size_t buflen, int buftype, int flags)
{
	struct arm32_bus_dma_cookie * const cookie = map->_dm_cookie;
	struct vmspace * const vm = vmspace_kernel();
	int error;

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
	error = _bus_dmamap_load_buffer(t, map, cookie->id_bouncebuf,
	    buflen, vm, flags);
	if (error)
		return (error);

	STAT_INCR(bounced_loads);
	map->dm_mapsize = buflen;
	map->_dm_vmspace = vm;
	map->_dm_buftype = buftype;

	/* ...so _bus_dmamap_sync() knows we're bouncing */
	map->_dm_flags |= _BUS_DMAMAP_IS_BOUNCING;
	cookie->id_flags |= _BUS_DMA_IS_BOUNCING;
	return 0;
}
#endif /* _ARM32_NEED_BUS_DMA_BOUNCE */

/*
 * Common function for DMA map creation.  May be called by bus-specific
 * DMA map creation functions.
 */
int
_bus_dmamap_create(bus_dma_tag_t t, bus_size_t size, int nsegments,
    bus_size_t maxsegsz, bus_size_t boundary, int flags, bus_dmamap_t *dmamp)
{
	struct arm32_bus_dmamap *map;
	void *mapstore;
	size_t mapsize;

#ifdef DEBUG_DMA
	printf("dmamap_create: t=%p size=%lx nseg=%x msegsz=%lx boundary=%lx flags=%x\n",
	    t, size, nsegments, maxsegsz, boundary, flags);
#endif	/* DEBUG_DMA */

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
	mapsize = sizeof(struct arm32_bus_dmamap) +
	    (sizeof(bus_dma_segment_t) * (nsegments - 1));
	const int mallocflags = M_ZERO|(flags & BUS_DMA_NOWAIT) ? M_NOWAIT : M_WAITOK;
	if ((mapstore = malloc(mapsize, M_DMAMAP, mallocflags)) == NULL)
		return (ENOMEM);

	map = (struct arm32_bus_dmamap *)mapstore;
	map->_dm_size = size;
	map->_dm_segcnt = nsegments;
	map->_dm_maxmaxsegsz = maxsegsz;
	map->_dm_boundary = boundary;
	map->_dm_flags = flags & ~(BUS_DMA_WAITOK|BUS_DMA_NOWAIT);
	map->_dm_origbuf = NULL;
	map->_dm_buftype = _BUS_DMA_BUFTYPE_INVALID;
	map->_dm_vmspace = vmspace_kernel();
	map->_dm_cookie = NULL;
	map->dm_maxsegsz = maxsegsz;
	map->dm_mapsize = 0;		/* no valid mappings */
	map->dm_nsegs = 0;

	*dmamp = map;

#ifdef _ARM32_NEED_BUS_DMA_BOUNCE
	struct arm32_bus_dma_cookie *cookie;
	int cookieflags;
	void *cookiestore;
	size_t cookiesize;
	int error;

	cookieflags = 0;

	if (t->_may_bounce != NULL) {
		error = (*t->_may_bounce)(t, map, flags, &cookieflags);
		if (error != 0)
			goto out;
	}

	if (t->_ranges != NULL)
		cookieflags |= _BUS_DMA_MIGHT_NEED_BOUNCE;

	if ((cookieflags & _BUS_DMA_MIGHT_NEED_BOUNCE) == 0) {
		STAT_INCR(creates);
		return 0;
	}

	cookiesize = sizeof(struct arm32_bus_dma_cookie) +
	    (sizeof(bus_dma_segment_t) * map->_dm_segcnt);

	/*
	 * Allocate our cookie.
	 */
	if ((cookiestore = malloc(cookiesize, M_DMAMAP, mallocflags)) == NULL) {
		error = ENOMEM;
		goto out;
	}
	cookie = (struct arm32_bus_dma_cookie *)cookiestore;
	cookie->id_flags = cookieflags;
	map->_dm_cookie = cookie;
	STAT_INCR(bounced_creates);

	error = _bus_dma_alloc_bouncebuf(t, map, size, flags);
 out:
	if (error)
		_bus_dmamap_destroy(t, map);
#else
	STAT_INCR(creates);
#endif /* _ARM32_NEED_BUS_DMA_BOUNCE */

#ifdef DEBUG_DMA
	printf("dmamap_create:map=%p\n", map);
#endif	/* DEBUG_DMA */
	return (0);
}

/*
 * Common function for DMA map destruction.  May be called by bus-specific
 * DMA map destruction functions.
 */
void
_bus_dmamap_destroy(bus_dma_tag_t t, bus_dmamap_t map)
{

#ifdef DEBUG_DMA
	printf("dmamap_destroy: t=%p map=%p\n", t, map);
#endif	/* DEBUG_DMA */
#ifdef _ARM32_NEED_BUS_DMA_BOUNCE
	struct arm32_bus_dma_cookie *cookie = map->_dm_cookie;

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
 * Common function for loading a DMA map with a linear buffer.  May
 * be called by bus-specific DMA map load functions.
 */
int
_bus_dmamap_load(bus_dma_tag_t t, bus_dmamap_t map, void *buf,
    bus_size_t buflen, struct proc *p, int flags)
{
	struct vmspace *vm;
	int error;

#ifdef DEBUG_DMA
	printf("dmamap_load: t=%p map=%p buf=%p len=%lx p=%p f=%d\n",
	    t, map, buf, buflen, p, flags);
#endif	/* DEBUG_DMA */

	if (map->dm_nsegs > 0) {
#ifdef _ARM32_NEED_BUS_DMA_BOUNCE
		struct arm32_bus_dma_cookie *cookie = map->_dm_cookie;
		if (cookie != NULL) {
			if (cookie->id_flags & _BUS_DMA_IS_BOUNCING) {
				STAT_INCR(bounced_unloads);
				cookie->id_flags &= ~_BUS_DMA_IS_BOUNCING;
				map->_dm_flags &= ~_BUS_DMAMAP_IS_BOUNCING;
			}
		} else
#endif
		STAT_INCR(unloads);
	}

	/*
	 * Make sure that on error condition we return "no valid mappings".
	 */
	map->dm_mapsize = 0;
	map->dm_nsegs = 0;
	map->_dm_buftype = _BUS_DMA_BUFTYPE_INVALID;
	KASSERTMSG(map->dm_maxsegsz <= map->_dm_maxmaxsegsz,
	    "dm_maxsegsz %lu _dm_maxmaxsegsz %lu",
	    map->dm_maxsegsz, map->_dm_maxmaxsegsz);

	if (buflen > map->_dm_size)
		return (EINVAL);

	if (p != NULL) {
		vm = p->p_vmspace;
	} else {
		vm = vmspace_kernel();
	}

	/* _bus_dmamap_load_buffer() clears this if we're not... */
	map->_dm_flags |= _BUS_DMAMAP_COHERENT;

	error = _bus_dmamap_load_buffer(t, map, buf, buflen, vm, flags);
	if (error == 0) {
		map->dm_mapsize = buflen;
		map->_dm_vmspace = vm;
		map->_dm_origbuf = buf;
		map->_dm_buftype = _BUS_DMA_BUFTYPE_LINEAR;
		return 0;
	}
#ifdef _ARM32_NEED_BUS_DMA_BOUNCE
	struct arm32_bus_dma_cookie * const cookie = map->_dm_cookie;
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
_bus_dmamap_load_mbuf(bus_dma_tag_t t, bus_dmamap_t map, struct mbuf *m0,
    int flags)
{
	int error;
	struct mbuf *m;

#ifdef DEBUG_DMA
	printf("dmamap_load_mbuf: t=%p map=%p m0=%p f=%d\n",
	    t, map, m0, flags);
#endif	/* DEBUG_DMA */

	if (map->dm_nsegs > 0) {
#ifdef _ARM32_NEED_BUS_DMA_BOUNCE
		struct arm32_bus_dma_cookie *cookie = map->_dm_cookie;
		if (cookie != NULL) {
			if (cookie->id_flags & _BUS_DMA_IS_BOUNCING) {
				STAT_INCR(bounced_unloads);
				cookie->id_flags &= ~_BUS_DMA_IS_BOUNCING;
				map->_dm_flags &= ~_BUS_DMAMAP_IS_BOUNCING;
			}
		} else
#endif
		STAT_INCR(unloads);
	}

	/*
	 * Make sure that on error condition we return "no valid mappings."
	 */
	map->dm_mapsize = 0;
	map->dm_nsegs = 0;
	map->_dm_buftype = _BUS_DMA_BUFTYPE_INVALID;
	KASSERTMSG(map->dm_maxsegsz <= map->_dm_maxmaxsegsz,
	    "dm_maxsegsz %lu _dm_maxmaxsegsz %lu",
	    map->dm_maxsegsz, map->_dm_maxmaxsegsz);

#ifdef DIAGNOSTIC
	if ((m0->m_flags & M_PKTHDR) == 0)
		panic("_bus_dmamap_load_mbuf: no packet header");
#endif	/* DIAGNOSTIC */

	if (m0->m_pkthdr.len > map->_dm_size)
		return (EINVAL);

	/* _bus_dmamap_load_paddr() clears this if we're not... */
	map->_dm_flags |= _BUS_DMAMAP_COHERENT;

	error = 0;
	for (m = m0; m != NULL && error == 0; m = m->m_next) {
		int offset;
		int remainbytes;
		const struct vm_page * const *pgs;
		paddr_t paddr;
		int size;

		if (m->m_len == 0)
			continue;
		/*
		 * Don't allow reads in read-only mbufs.
		 */
		if (M_ROMAP(m) && (flags & BUS_DMA_READ)) {
			error = EFAULT;
			break;
		}
		switch (m->m_flags & (M_EXT|M_CLUSTER|M_EXT_PAGES)) {
		case M_EXT|M_CLUSTER:
			/* XXX KDASSERT */
			KASSERT(m->m_ext.ext_paddr != M_PADDR_INVALID);
			paddr = m->m_ext.ext_paddr +
			    (m->m_data - m->m_ext.ext_buf);
			size = m->m_len;
			error = _bus_dmamap_load_paddr(t, map, paddr, size,
			    false);
			break;
		
		case M_EXT|M_EXT_PAGES:
			KASSERT(m->m_ext.ext_buf <= m->m_data);
			KASSERT(m->m_data <=
			    m->m_ext.ext_buf + m->m_ext.ext_size);
			
			offset = (vaddr_t)m->m_data -
			    trunc_page((vaddr_t)m->m_ext.ext_buf);
			remainbytes = m->m_len;

			/* skip uninteresting pages */
			pgs = (const struct vm_page * const *)
			    m->m_ext.ext_pgs + (offset >> PAGE_SHIFT);
			
			offset &= PAGE_MASK;	/* offset in the first page */

			/* load each page */
			while (remainbytes > 0) {
				const struct vm_page *pg;

				size = MIN(remainbytes, PAGE_SIZE - offset);

				pg = *pgs++;
				KASSERT(pg);
				paddr = VM_PAGE_TO_PHYS(pg) + offset;

				error = _bus_dmamap_load_paddr(t, map,
				    paddr, size, false);
				if (error)
					break;
				offset = 0;
				remainbytes -= size;
			}
			break;

		case 0:
			paddr = m->m_paddr + M_BUFOFFSET(m) +
			    (m->m_data - M_BUFADDR(m));
			size = m->m_len;
			error = _bus_dmamap_load_paddr(t, map, paddr, size,
			    false);
			break;

		default:
			error = _bus_dmamap_load_buffer(t, map, m->m_data,
			    m->m_len, vmspace_kernel(), flags);
		}
	}
	if (error == 0) {
		map->dm_mapsize = m0->m_pkthdr.len;
		map->_dm_origbuf = m0;
		map->_dm_buftype = _BUS_DMA_BUFTYPE_MBUF;
		map->_dm_vmspace = vmspace_kernel();	/* always kernel */
		return 0;
	}
#ifdef _ARM32_NEED_BUS_DMA_BOUNCE
	struct arm32_bus_dma_cookie * const cookie = map->_dm_cookie;
	if (cookie != NULL && (cookie->id_flags & _BUS_DMA_MIGHT_NEED_BOUNCE)) {
		error = _bus_dma_load_bouncebuf(t, map, m0, m0->m_pkthdr.len,
		    _BUS_DMA_BUFTYPE_MBUF, flags);
	}        
#endif           
	return (error);
}

/*
 * Like _bus_dmamap_load(), but for uios.
 */
int
_bus_dmamap_load_uio(bus_dma_tag_t t, bus_dmamap_t map, struct uio *uio,
    int flags)
{
	int i, error;
	bus_size_t minlen, resid;
	struct iovec *iov;
	void *addr;

	/*
	 * Make sure that on error condition we return "no valid mappings."
	 */
	map->dm_mapsize = 0;
	map->dm_nsegs = 0;
	KASSERTMSG(map->dm_maxsegsz <= map->_dm_maxmaxsegsz,
	    "dm_maxsegsz %lu _dm_maxmaxsegsz %lu",
	    map->dm_maxsegsz, map->_dm_maxmaxsegsz);

	resid = uio->uio_resid;
	iov = uio->uio_iov;

	/* _bus_dmamap_load_buffer() clears this if we're not... */
	map->_dm_flags |= _BUS_DMAMAP_COHERENT;

	error = 0;
	for (i = 0; i < uio->uio_iovcnt && resid != 0 && error == 0; i++) {
		/*
		 * Now at the first iovec to load.  Load each iovec
		 * until we have exhausted the residual count.
		 */
		minlen = resid < iov[i].iov_len ? resid : iov[i].iov_len;
		addr = (void *)iov[i].iov_base;

		error = _bus_dmamap_load_buffer(t, map, addr, minlen,
		    uio->uio_vmspace, flags);

		resid -= minlen;
	}
	if (error == 0) {
		map->dm_mapsize = uio->uio_resid;
		map->_dm_origbuf = uio;
		map->_dm_buftype = _BUS_DMA_BUFTYPE_UIO;
		map->_dm_vmspace = uio->uio_vmspace;
	}
	return (error);
}

/*
 * Like _bus_dmamap_load(), but for raw memory allocated with
 * bus_dmamem_alloc().
 */
int
_bus_dmamap_load_raw(bus_dma_tag_t t, bus_dmamap_t map,
    bus_dma_segment_t *segs, int nsegs, bus_size_t size, int flags)
{

	panic("_bus_dmamap_load_raw: not implemented");
}

/*
 * Common function for unloading a DMA map.  May be called by
 * bus-specific DMA map unload functions.
 */
void
_bus_dmamap_unload(bus_dma_tag_t t, bus_dmamap_t map)
{

#ifdef DEBUG_DMA
	printf("dmamap_unload: t=%p map=%p\n", t, map);
#endif	/* DEBUG_DMA */

	/*
	 * No resources to free; just mark the mappings as
	 * invalid.
	 */
	map->dm_mapsize = 0;
	map->dm_nsegs = 0;
	map->_dm_origbuf = NULL;
	map->_dm_buftype = _BUS_DMA_BUFTYPE_INVALID;
	map->_dm_vmspace = NULL;
}

static void
_bus_dmamap_sync_segment(vaddr_t va, paddr_t pa, vsize_t len, int ops, bool readonly_p)
{
	KASSERT((va & PAGE_MASK) == (pa & PAGE_MASK));
#if 0
	printf("sync_segment: va=%#lx pa=%#lx len=%#lx ops=%#x ro=%d\n",
	    va, pa, len, ops, readonly_p);
#endif

	switch (ops) {
	case BUS_DMASYNC_PREREAD|BUS_DMASYNC_PREWRITE:
		if (!readonly_p) {
			STAT_INCR(sync_prereadwrite);
			cpu_dcache_wbinv_range(va, len);
			cpu_sdcache_wbinv_range(va, pa, len);
			break;
		}
		/* FALLTHROUGH */

	case BUS_DMASYNC_PREREAD: {
		const size_t line_size = arm_dcache_align;
		const size_t line_mask = arm_dcache_align_mask;
		vsize_t misalignment = va & line_mask;
		if (misalignment) {
			va -= misalignment;
			pa -= misalignment;
			len += misalignment;
			STAT_INCR(sync_preread_begin);
			cpu_dcache_wbinv_range(va, line_size);
			cpu_sdcache_wbinv_range(va, pa, line_size);
			if (len <= line_size)
				break;
			va += line_size;
			pa += line_size;
			len -= line_size;
		}
		misalignment = len & line_mask;
		len -= misalignment;
		if (len > 0) {
			STAT_INCR(sync_preread);
			cpu_dcache_inv_range(va, len);
			cpu_sdcache_inv_range(va, pa, len);
		}
		if (misalignment) {
			va += len;
			pa += len;
			STAT_INCR(sync_preread_tail);
			cpu_dcache_wbinv_range(va, line_size);
			cpu_sdcache_wbinv_range(va, pa, line_size);
		}
		break;
	}

	case BUS_DMASYNC_PREWRITE:
		STAT_INCR(sync_prewrite);
		cpu_dcache_wb_range(va, len);
		cpu_sdcache_wb_range(va, pa, len);
		break;

#ifdef CPU_CORTEX
	/*
	 * Cortex CPUs can do speculative loads so we need to clean the cache
	 * after a DMA read to deal with any speculatively loaded cache lines.
	 * Since these can't be dirty, we can just invalidate them and don't
	 * have to worry about having to write back their contents.
	 */
	case BUS_DMASYNC_POSTREAD|BUS_DMASYNC_POSTWRITE:
		STAT_INCR(sync_postreadwrite);
		cpu_dcache_inv_range(va, len);
		cpu_sdcache_inv_range(va, pa, len);
		break;
	case BUS_DMASYNC_POSTREAD:
		STAT_INCR(sync_postread);
		cpu_dcache_inv_range(va, len);
		cpu_sdcache_inv_range(va, pa, len);
		break;
#endif
	}
}

static inline void
_bus_dmamap_sync_linear(bus_dma_tag_t t, bus_dmamap_t map, bus_addr_t offset,
    bus_size_t len, int ops)
{
	bus_dma_segment_t *ds = map->dm_segs;
	vaddr_t va = (vaddr_t) map->_dm_origbuf;
#ifdef _ARM32_NEED_BUS_DMA_BOUNCE
	if (map->_dm_flags & _BUS_DMAMAP_IS_BOUNCING) {
		struct arm32_bus_dma_cookie * const cookie = map->_dm_cookie;
		va = (vaddr_t) cookie->id_bouncebuf;
	}
#endif

	while (len > 0) {
		while (offset >= ds->ds_len) {
			offset -= ds->ds_len;
			va += ds->ds_len;
			ds++;
		}

		paddr_t pa = _bus_dma_busaddr_to_paddr(t, ds->ds_addr + offset);
		size_t seglen = min(len, ds->ds_len - offset);

		if ((ds->_ds_flags & _BUS_DMAMAP_COHERENT) == 0)
			_bus_dmamap_sync_segment(va + offset, pa, seglen, ops,
			    false);

		offset += seglen;
		len -= seglen;
	}
}

static inline void
_bus_dmamap_sync_mbuf(bus_dma_tag_t t, bus_dmamap_t map, bus_size_t offset,
    bus_size_t len, int ops)
{
	bus_dma_segment_t *ds = map->dm_segs;
	struct mbuf *m = map->_dm_origbuf;
	bus_size_t voff = offset;
	bus_size_t ds_off = offset;

	while (len > 0) {
		/* Find the current dma segment */
		while (ds_off >= ds->ds_len) {
			ds_off -= ds->ds_len;
			ds++;
		}
		/* Find the current mbuf. */
		while (voff >= m->m_len) {
			voff -= m->m_len;
			m = m->m_next;
		}

		/*
		 * Now at the first mbuf to sync; nail each one until
		 * we have exhausted the length.
		 */
		vsize_t seglen = min(len, min(m->m_len - voff, ds->ds_len - ds_off));
		vaddr_t va = mtod(m, vaddr_t) + voff;
		paddr_t pa = _bus_dma_busaddr_to_paddr(t, ds->ds_addr + ds_off);

		/*
		 * We can save a lot of work here if we know the mapping
		 * is read-only at the MMU:
		 *
		 * If a mapping is read-only, no dirty cache blocks will
		 * exist for it.  If a writable mapping was made read-only,
		 * we know any dirty cache lines for the range will have
		 * been cleaned for us already.  Therefore, if the upper
		 * layer can tell us we have a read-only mapping, we can
		 * skip all cache cleaning.
		 *
		 * NOTE: This only works if we know the pmap cleans pages
		 * before making a read-write -> read-only transition.  If
		 * this ever becomes non-true (e.g. Physically Indexed
		 * cache), this will have to be revisited.
		 */

		if ((ds->_ds_flags & _BUS_DMAMAP_COHERENT) == 0)
			_bus_dmamap_sync_segment(va, pa, seglen, ops,
			    M_ROMAP(m));
		voff += seglen;
		ds_off += seglen;
		len -= seglen;
	}
}

static inline void
_bus_dmamap_sync_uio(bus_dma_tag_t t, bus_dmamap_t map, bus_addr_t offset,
    bus_size_t len, int ops)
{
	bus_dma_segment_t *ds = map->dm_segs;
	struct uio *uio = map->_dm_origbuf;
	struct iovec *iov = uio->uio_iov;
	bus_size_t voff = offset;
	bus_size_t ds_off = offset;

	while (len > 0) {
		/* Find the current dma segment */
		while (ds_off >= ds->ds_len) {
			ds_off -= ds->ds_len;
			ds++;
		}

		/* Find the current iovec. */
		while (voff >= iov->iov_len) {
			voff -= iov->iov_len;
			iov++;
		}

		/*
		 * Now at the first iovec to sync; nail each one until
		 * we have exhausted the length.
		 */
		vsize_t seglen = min(len, min(iov->iov_len - voff, ds->ds_len - ds_off));
		vaddr_t va = (vaddr_t) iov->iov_base + voff;
		paddr_t pa = _bus_dma_busaddr_to_paddr(t, ds->ds_addr + ds_off);

		if ((ds->_ds_flags & _BUS_DMAMAP_COHERENT) == 0)
			_bus_dmamap_sync_segment(va, pa, seglen, ops, false);

		voff += seglen;
		ds_off += seglen;
		len -= seglen;
	}
}

/*
 * Common function for DMA map synchronization.  May be called
 * by bus-specific DMA map synchronization functions.
 *
 * This version works for the Virtually Indexed Virtually Tagged
 * cache found on 32-bit ARM processors.
 *
 * XXX Should have separate versions for write-through vs.
 * XXX write-back caches.  We currently assume write-back
 * XXX here, which is not as efficient as it could be for
 * XXX the write-through case.
 */
void
_bus_dmamap_sync(bus_dma_tag_t t, bus_dmamap_t map, bus_addr_t offset,
    bus_size_t len, int ops)
{
#ifdef DEBUG_DMA
	printf("dmamap_sync: t=%p map=%p offset=%lx len=%lx ops=%x\n",
	    t, map, offset, len, ops);
#endif	/* DEBUG_DMA */

	/*
	 * Mixing of PRE and POST operations is not allowed.
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
	 *	POSTREAD -- Re-invalidate the D-cache in case speculative
	 *	memory accesses caused cachelines to become valid with now
	 *	invalid data.
	 *
	 *	POSTWRITE -- Nothing.
	 */
#ifdef _ARM32_NEED_BUS_DMA_BOUNCE
	const bool bouncing = (map->_dm_flags & _BUS_DMAMAP_IS_BOUNCING);
#else
	const bool bouncing = false;
#endif

	const int pre_ops = ops & (BUS_DMASYNC_PREREAD|BUS_DMASYNC_PREWRITE);
#ifdef CPU_CORTEX
	const int post_ops = ops & (BUS_DMASYNC_POSTREAD|BUS_DMASYNC_POSTWRITE);
#else
	const int post_ops = 0;
#endif
	if (!bouncing && pre_ops == 0 && post_ops == BUS_DMASYNC_POSTWRITE) {
		STAT_INCR(sync_postwrite);
		return;
	}
	KASSERTMSG(bouncing || pre_ops != 0 || (post_ops & BUS_DMASYNC_POSTREAD),
	    "pre_ops %#x post_ops %#x", pre_ops, post_ops);
#ifdef _ARM32_NEED_BUS_DMA_BOUNCE
	if (bouncing && (ops & BUS_DMASYNC_PREWRITE)) {
		struct arm32_bus_dma_cookie * const cookie = map->_dm_cookie;
		STAT_INCR(write_bounces);
		char * const dataptr = (char *)cookie->id_bouncebuf + offset;
		/*
		 * Copy the caller's buffer to the bounce buffer.
		 */
		switch (map->_dm_buftype) {
		case _BUS_DMA_BUFTYPE_LINEAR:
			memcpy(dataptr, cookie->id_origlinearbuf + offset, len);
			break;
		case _BUS_DMA_BUFTYPE_MBUF:
			m_copydata(cookie->id_origmbuf, offset, len, dataptr);
			break;
		case _BUS_DMA_BUFTYPE_UIO:
			_bus_dma_uiomove(dataptr, cookie->id_origuio, len, UIO_WRITE);
			break;
#ifdef DIAGNOSTIC
		case _BUS_DMA_BUFTYPE_RAW:
			panic("_bus_dmamap_sync(pre): _BUS_DMA_BUFTYPE_RAW");
			break;

		case _BUS_DMA_BUFTYPE_INVALID:
			panic("_bus_dmamap_sync(pre): _BUS_DMA_BUFTYPE_INVALID");
			break;

		default:
			panic("_bus_dmamap_sync(pre): map %p: unknown buffer type %d\n",
			    map, map->_dm_buftype);
			break;
#endif /* DIAGNOSTIC */
		}
	}
#endif /* _ARM32_NEED_BUS_DMA_BOUNCE */

	/* Skip cache frobbing if mapping was COHERENT. */
	if (!bouncing && (map->_dm_flags & _BUS_DMAMAP_COHERENT)) {
		/* Drain the write buffer. */
		if (pre_ops & BUS_DMASYNC_PREWRITE)
			cpu_drain_writebuf();
		return;
	}

#ifdef _ARM32_NEED_BUS_DMA_BOUNCE
	if (bouncing && ((map->_dm_flags & _BUS_DMAMAP_COHERENT) || pre_ops == 0)) {
		goto bounce_it;
	}
#endif /* _ARM32_NEED_BUS_DMA_BOUNCE */

	/*
	 * If the mapping belongs to a non-kernel vmspace, and the
	 * vmspace has not been active since the last time a full
	 * cache flush was performed, we don't need to do anything.
	 */
	if (__predict_false(!VMSPACE_IS_KERNEL_P(map->_dm_vmspace) &&
	    vm_map_pmap(&map->_dm_vmspace->vm_map)->pm_cstate.cs_cache_d == 0))
		return;

	int buftype = map->_dm_buftype;
#ifdef _ARM32_NEED_BUS_DMA_BOUNCE
	if (bouncing) {
		buftype = _BUS_DMA_BUFTYPE_LINEAR;
	}
#endif

	switch (buftype) {
	case _BUS_DMA_BUFTYPE_LINEAR:
		_bus_dmamap_sync_linear(t, map, offset, len, ops);
		break;

	case _BUS_DMA_BUFTYPE_MBUF:
		_bus_dmamap_sync_mbuf(t, map, offset, len, ops);
		break;

	case _BUS_DMA_BUFTYPE_UIO:
		_bus_dmamap_sync_uio(t, map, offset, len, ops);
		break;

	case _BUS_DMA_BUFTYPE_RAW:
		panic("_bus_dmamap_sync: _BUS_DMA_BUFTYPE_RAW");
		break;

	case _BUS_DMA_BUFTYPE_INVALID:
		panic("_bus_dmamap_sync: _BUS_DMA_BUFTYPE_INVALID");
		break;

	default:
		panic("_bus_dmamap_sync: map %p: unknown buffer type %d\n",
		    map, map->_dm_buftype);
	}

	/* Drain the write buffer. */
	cpu_drain_writebuf();

#ifdef _ARM32_NEED_BUS_DMA_BOUNCE
  bounce_it:
	if (!bouncing || (ops & BUS_DMASYNC_POSTREAD) == 0)
		return;

	struct arm32_bus_dma_cookie * const cookie = map->_dm_cookie;
	char * const dataptr = (char *)cookie->id_bouncebuf + offset;
	STAT_INCR(read_bounces);
	/*
	 * Copy the bounce buffer to the caller's buffer.
	 */
	switch (map->_dm_buftype) {
	case _BUS_DMA_BUFTYPE_LINEAR:
		memcpy(cookie->id_origlinearbuf + offset, dataptr, len);
		break;

	case _BUS_DMA_BUFTYPE_MBUF:
		m_copyback(cookie->id_origmbuf, offset, len, dataptr);
		break;

	case _BUS_DMA_BUFTYPE_UIO:
		_bus_dma_uiomove(dataptr, cookie->id_origuio, len, UIO_READ);
		break;
#ifdef DIAGNOSTIC
	case _BUS_DMA_BUFTYPE_RAW:
		panic("_bus_dmamap_sync(post): _BUS_DMA_BUFTYPE_RAW");
		break;

	case _BUS_DMA_BUFTYPE_INVALID:
		panic("_bus_dmamap_sync(post): _BUS_DMA_BUFTYPE_INVALID");
		break;

	default:
		panic("_bus_dmamap_sync(post): map %p: unknown buffer type %d\n",
		    map, map->_dm_buftype);
		break;
#endif
	}
#endif /* _ARM32_NEED_BUS_DMA_BOUNCE */
}

/*
 * Common function for DMA-safe memory allocation.  May be called
 * by bus-specific DMA memory allocation functions.
 */

extern paddr_t physical_start;
extern paddr_t physical_end;

int
_bus_dmamem_alloc(bus_dma_tag_t t, bus_size_t size, bus_size_t alignment,
    bus_size_t boundary, bus_dma_segment_t *segs, int nsegs, int *rsegs,
    int flags)
{
	struct arm32_dma_range *dr;
	int error, i;

#ifdef DEBUG_DMA
	printf("dmamem_alloc t=%p size=%lx align=%lx boundary=%lx "
	    "segs=%p nsegs=%x rsegs=%p flags=%x\n", t, size, alignment,
	    boundary, segs, nsegs, rsegs, flags);
#endif

	if ((dr = t->_ranges) != NULL) {
		error = ENOMEM;
		for (i = 0; i < t->_nranges; i++, dr++) {
			if (dr->dr_len == 0
			    || (dr->dr_flags & _BUS_DMAMAP_NOALLOC))
				continue;
			error = _bus_dmamem_alloc_range(t, size, alignment,
			    boundary, segs, nsegs, rsegs, flags,
			    trunc_page(dr->dr_sysbase),
			    trunc_page(dr->dr_sysbase + dr->dr_len));
			if (error == 0)
				break;
		}
	} else {
		error = _bus_dmamem_alloc_range(t, size, alignment, boundary,
		    segs, nsegs, rsegs, flags, trunc_page(physical_start),
		    trunc_page(physical_end));
	}

#ifdef DEBUG_DMA
	printf("dmamem_alloc: =%d\n", error);
#endif

	return(error);
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

#ifdef DEBUG_DMA
	printf("dmamem_free: t=%p segs=%p nsegs=%x\n", t, segs, nsegs);
#endif	/* DEBUG_DMA */

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
	paddr_t pa;
	int curseg;
	pt_entry_t *ptep;
	const uvm_flag_t kmflags = UVM_KMF_VAONLY
	    | ((flags & BUS_DMA_NOWAIT) != 0 ? UVM_KMF_NOWAIT : 0);
	vsize_t align = 0;

#ifdef DEBUG_DMA
	printf("dmamem_map: t=%p segs=%p nsegs=%x size=%lx flags=%x\n", t,
	    segs, nsegs, (unsigned long)size, flags);
#endif	/* DEBUG_DMA */

#ifdef PMAP_MAP_POOLPAGE
	/*
	 * If all of memory is mapped, and we are mapping a single physically
	 * contiguous area then this area is already mapped.  Let's see if we
	 * avoid having a separate mapping for it.
	 */
	if (nsegs == 1) {
		/*
		 * If this is a non-COHERENT mapping, then the existing kernel
		 * mapping is already compatible with it.
		 */
		bool direct_mapable = (flags & BUS_DMA_COHERENT) == 0;
		pa = segs[0].ds_addr;

		/*
		 * This is a COHERENT mapping which, unless this address is in
		 * a COHERENT dma range, will not be compatible.
		 */
		if (t->_ranges != NULL) {
			const struct arm32_dma_range * const dr =
			    _bus_dma_paddr_inrange(t->_ranges, t->_nranges, pa);
			if (dr != NULL
			    && (dr->dr_flags & _BUS_DMAMAP_COHERENT)) {
				direct_mapable = true;
			}
		}

		if (direct_mapable) {
			*kvap = (void *)PMAP_MAP_POOLPAGE(pa);
#ifdef DEBUG_DMA
			printf("dmamem_map: =%p\n", *kvap);
#endif	/* DEBUG_DMA */
			return 0;
		}
	}
#endif

	size = round_page(size);
	if (__predict_true(size > L2_L_SIZE)) {
#if (ARM_MMU_V6 + ARM_MMU_V7) > 0
		if (size >= L1_SS_SIZE)
			align = L1_SS_SIZE;
		else
#endif
		if (size >= L1_S_SIZE)
			align = L1_S_SIZE;
		else
			align = L2_S_SIZE;
	}

	va = uvm_km_alloc(kernel_map, size, align, kmflags);
	if (__predict_false(va == 0 && align > 0)) {
		align = 0;
		va = uvm_km_alloc(kernel_map, size, 0, kmflags);
	}

	if (va == 0)
		return (ENOMEM);

	*kvap = (void *)va;

	for (curseg = 0; curseg < nsegs; curseg++) {
		for (pa = segs[curseg].ds_addr;
		    pa < (segs[curseg].ds_addr + segs[curseg].ds_len);
		    pa += PAGE_SIZE, va += PAGE_SIZE, size -= PAGE_SIZE) {
			bool uncached = (flags & BUS_DMA_COHERENT);
#ifdef DEBUG_DMA
			printf("wiring p%lx to v%lx", pa, va);
#endif	/* DEBUG_DMA */
			if (size == 0)
				panic("_bus_dmamem_map: size botch");

			const struct arm32_dma_range * const dr =
			    _bus_dma_paddr_inrange(t->_ranges, t->_nranges, pa);
			/*
			 * If this dma region is coherent then there is
			 * no need for an uncached mapping.
			 */
			if (dr != NULL
			    && (dr->dr_flags & _BUS_DMAMAP_COHERENT)) {
				uncached = false;
			}

			pmap_kenter_pa(va, pa,
			    VM_PROT_READ | VM_PROT_WRITE, PMAP_WIRED);

			/*
			 * If the memory must remain coherent with the
			 * cache then we must make the memory uncacheable
			 * in order to maintain virtual cache coherency.
			 * We must also guarantee the cache does not already
			 * contain the virtal addresses we are making
			 * uncacheable.
			 */
			if (uncached) {
				cpu_dcache_wbinv_range(va, PAGE_SIZE);
				cpu_sdcache_wbinv_range(va, pa, PAGE_SIZE);
				cpu_drain_writebuf();
				ptep = vtopte(va);
				*ptep &= ~L2_S_CACHE_MASK;
				PTE_SYNC(ptep);
				tlb_flush();
			}
#ifdef DEBUG_DMA
			ptep = vtopte(va);
			printf(" pte=v%p *pte=%x\n", ptep, *ptep);
#endif	/* DEBUG_DMA */
		}
	}
	pmap_update(pmap_kernel());
#ifdef DEBUG_DMA
	printf("dmamem_map: =%p\n", *kvap);
#endif	/* DEBUG_DMA */
	return (0);
}

/*
 * Common function for unmapping DMA-safe memory.  May be called by
 * bus-specific DMA memory unmapping functions.
 */
void
_bus_dmamem_unmap(bus_dma_tag_t t, void *kva, size_t size)
{

#ifdef DEBUG_DMA
	printf("dmamem_unmap: t=%p kva=%p size=%zx\n", t, kva, size);
#endif	/* DEBUG_DMA */
#ifdef DIAGNOSTIC
	if ((u_long)kva & PGOFSET)
		panic("_bus_dmamem_unmap");
#endif	/* DIAGNOSTIC */

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
_bus_dmamem_mmap(bus_dma_tag_t t, bus_dma_segment_t *segs, int nsegs,
    off_t off, int prot, int flags)
{
	paddr_t map_flags;
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

		map_flags = 0;
		if (flags & BUS_DMA_PREFETCHABLE)
			map_flags |= ARM32_MMAP_WRITECOMBINE;

		return (arm_btop((u_long)segs[i].ds_addr + off) | map_flags);
		
	}

	/* Page not found. */
	return (-1);
}

/**********************************************************************
 * DMA utility functions
 **********************************************************************/

/*
 * Utility function to load a linear buffer.  lastaddrp holds state
 * between invocations (for multiple-buffer loads).  segp contains
 * the starting segment on entrace, and the ending segment on exit.
 * first indicates if this is the first invocation of this function.
 */
int
_bus_dmamap_load_buffer(bus_dma_tag_t t, bus_dmamap_t map, void *buf,
    bus_size_t buflen, struct vmspace *vm, int flags)
{
	bus_size_t sgsize;
	bus_addr_t curaddr;
	vaddr_t vaddr = (vaddr_t)buf;
	int error;
	pmap_t pmap;

#ifdef DEBUG_DMA
	printf("_bus_dmamem_load_buffer(buf=%p, len=%lx, flags=%d)\n",
	    buf, buflen, flags);
#endif	/* DEBUG_DMA */

	pmap = vm_map_pmap(&vm->vm_map);

	while (buflen > 0) {
		/*
		 * Get the physical address for this segment.
		 *
		 * XXX Doesn't support checking for coherent mappings
		 * XXX in user address space.
		 */
		bool coherent;
		if (__predict_true(pmap == pmap_kernel())) {
			pd_entry_t *pde;
			pt_entry_t *ptep;
			(void) pmap_get_pde_pte(pmap, vaddr, &pde, &ptep);
			if (__predict_false(pmap_pde_section(pde))) {
				paddr_t s_frame = L1_S_FRAME;
				paddr_t s_offset = L1_S_OFFSET;
#if (ARM_MMU_V6 + ARM_MMU_V7) > 0
				if (__predict_false(pmap_pde_supersection(pde))) {
					s_frame = L1_SS_FRAME;
					s_offset = L1_SS_OFFSET;
				}
#endif
				curaddr = (*pde & s_frame) | (vaddr & s_offset);
				coherent = (*pde & L1_S_CACHE_MASK) == 0;
			} else {
				pt_entry_t pte = *ptep;
				KDASSERTMSG((pte & L2_TYPE_MASK) != L2_TYPE_INV,
				    "va=%#"PRIxVADDR" pde=%#x ptep=%p pte=%#x",
				    vaddr, *pde, ptep, pte);
				if (__predict_false((pte & L2_TYPE_MASK)
						    == L2_TYPE_L)) {
					curaddr = (pte & L2_L_FRAME) |
					    (vaddr & L2_L_OFFSET);
					coherent = (pte & L2_L_CACHE_MASK) == 0;
				} else {
					curaddr = (pte & L2_S_FRAME) |
					    (vaddr & L2_S_OFFSET);
					coherent = (pte & L2_S_CACHE_MASK) == 0;
				}
			}
		} else {
			(void) pmap_extract(pmap, vaddr, &curaddr);
			coherent = false;
		}

		/*
		 * Compute the segment size, and adjust counts.
		 */
		sgsize = PAGE_SIZE - ((u_long)vaddr & PGOFSET);
		if (buflen < sgsize)
			sgsize = buflen;

		error = _bus_dmamap_load_paddr(t, map, curaddr, sgsize,
		    coherent);
		if (error)
			return (error);

		vaddr += sgsize;
		buflen -= sgsize;
	}

	return (0);
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

	KASSERTMSG(boundary == 0 || (boundary & (boundary-1)) == 0,
	    "invalid boundary %#lx", boundary);

#ifdef DEBUG_DMA
	printf("alloc_range: t=%p size=%lx align=%lx boundary=%lx segs=%p nsegs=%x rsegs=%p flags=%x lo=%lx hi=%lx\n",
	    t, size, alignment, boundary, segs, nsegs, rsegs, flags, low, high);
#endif	/* DEBUG_DMA */

	/* Always round the size. */
	size = round_page(size);

	/*
	 * We accept boundaries < size, splitting in multiple segments
	 * if needed. uvm_pglistalloc does not, so compute an appropriate
	 * boundary: next power of 2 >= size
	 */
	bus_size_t uboundary = boundary;
	if (uboundary <= PAGE_SIZE) {
		uboundary = 0;
	} else {
		while (uboundary < size) {
			uboundary <<= 1;
		}
	}

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
#ifdef DEBUG_DMA
		printf("alloc: page %lx\n", lastaddr);
#endif	/* DEBUG_DMA */
	m = TAILQ_NEXT(m, pageq.queue);

	for (; m != NULL; m = TAILQ_NEXT(m, pageq.queue)) {
		curaddr = VM_PAGE_TO_PHYS(m);
		KASSERTMSG(low <= curaddr && curaddr < high,
		    "uvm_pglistalloc returned non-sensicaladdress %#lx "
		    "(low=%#lx, high=%#lx\n", curaddr, low, high);
#ifdef DEBUG_DMA
		printf("alloc: page %lx\n", curaddr);
#endif	/* DEBUG_DMA */
		if (curaddr == lastaddr + PAGE_SIZE
		    && (lastaddr & boundary) == (curaddr & boundary))
			segs[curseg].ds_len += PAGE_SIZE;
		else {
			curseg++;
			if (curseg >= nsegs) {
				uvm_pglistfree(&mlist);
				return EFBIG;
			}
			segs[curseg].ds_addr = curaddr;
			segs[curseg].ds_len = PAGE_SIZE;
		}
		lastaddr = curaddr;
	}

	*rsegs = curseg + 1;

	return (0);
}

/*
 * Check if a memory region intersects with a DMA range, and return the
 * page-rounded intersection if it does.
 */
int
arm32_dma_range_intersect(struct arm32_dma_range *ranges, int nranges,
    paddr_t pa, psize_t size, paddr_t *pap, psize_t *sizep)
{
	struct arm32_dma_range *dr;
	int i;

	if (ranges == NULL)
		return (0);

	for (i = 0, dr = ranges; i < nranges; i++, dr++) {
		if (dr->dr_sysbase <= pa &&
		    pa < (dr->dr_sysbase + dr->dr_len)) {
			/*
			 * Beginning of region intersects with this range.
			 */
			*pap = trunc_page(pa);
			*sizep = round_page(min(pa + size,
			    dr->dr_sysbase + dr->dr_len) - pa);
			return (1);
		}
		if (pa < dr->dr_sysbase && dr->dr_sysbase < (pa + size)) {
			/*
			 * End of region intersects with this range.
			 */
			*pap = trunc_page(dr->dr_sysbase);
			*sizep = round_page(min((pa + size) - dr->dr_sysbase,
			    dr->dr_len));
			return (1);
		}
	}

	/* No intersection found. */
	return (0);
}

#ifdef _ARM32_NEED_BUS_DMA_BOUNCE
static int
_bus_dma_alloc_bouncebuf(bus_dma_tag_t t, bus_dmamap_t map,
    bus_size_t size, int flags)
{
	struct arm32_bus_dma_cookie *cookie = map->_dm_cookie;
	int error = 0;

#ifdef DIAGNOSTIC
	if (cookie == NULL)
		panic("_bus_dma_alloc_bouncebuf: no cookie");
#endif

	cookie->id_bouncebuflen = round_page(size);
	error = _bus_dmamem_alloc(t, cookie->id_bouncebuflen,
	    PAGE_SIZE, map->_dm_boundary, cookie->id_bouncesegs,
	    map->_dm_segcnt, &cookie->id_nbouncesegs, flags);
	if (error == 0) {
		error = _bus_dmamem_map(t, cookie->id_bouncesegs,
		    cookie->id_nbouncesegs, cookie->id_bouncebuflen,
		    (void **)&cookie->id_bouncebuf, flags);
		if (error) {
			_bus_dmamem_free(t, cookie->id_bouncesegs,
			    cookie->id_nbouncesegs);
			cookie->id_bouncebuflen = 0;
			cookie->id_nbouncesegs = 0;
		} else {
			cookie->id_flags |= _BUS_DMA_HAS_BOUNCE;
		}
	} else {
		cookie->id_bouncebuflen = 0;
		cookie->id_nbouncesegs = 0;
	}

	return (error);
}

static void
_bus_dma_free_bouncebuf(bus_dma_tag_t t, bus_dmamap_t map)
{
	struct arm32_bus_dma_cookie *cookie = map->_dm_cookie;

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
#endif /* _ARM32_NEED_BUS_DMA_BOUNCE */

int
_bus_dmatag_subregion(bus_dma_tag_t tag, bus_addr_t min_addr,
    bus_addr_t max_addr, bus_dma_tag_t *newtag, int flags)
{

#ifdef _ARM32_NEED_BUS_DMA_BOUNCE
	struct arm32_dma_range *dr;
	bool subset = false;
	size_t nranges = 0;
	size_t i;
	for (i = 0, dr = tag->_ranges; i < tag->_nranges; i++, dr++) {
		if (dr->dr_sysbase <= min_addr 
		    && max_addr <= dr->dr_sysbase + dr->dr_len - 1) {
			subset = true;
		}
		if (min_addr <= dr->dr_sysbase + dr->dr_len
		    && max_addr >= dr->dr_sysbase) {
			nranges++;
		}
	}
	if (subset) {
		*newtag = tag;
		/* if the tag must be freed, add a reference */
		if (tag->_tag_needs_free)
			(tag->_tag_needs_free)++;
		return 0;
	}
	if (nranges == 0) {
		nranges = 1;
	}

	size_t mallocsize = sizeof(*tag) + nranges * sizeof(*dr);
	if ((*newtag = malloc(mallocsize, M_DMAMAP,
	    (flags & BUS_DMA_NOWAIT) ? M_NOWAIT : M_WAITOK)) == NULL)
		return ENOMEM;

	dr = (void *)(*newtag + 1);
	**newtag = *tag;
	(*newtag)->_tag_needs_free = 1;
	(*newtag)->_ranges = dr;
	(*newtag)->_nranges = nranges;

	if (tag->_ranges == NULL) {
		dr->dr_sysbase = min_addr;
		dr->dr_busbase = min_addr;
		dr->dr_len = max_addr + 1 - min_addr;
	} else {
		for (i = 0; i < nranges; i++) {
			if (min_addr > dr->dr_sysbase + dr->dr_len
			    || max_addr < dr->dr_sysbase)
				continue;
			dr[0] = tag->_ranges[i];
			if (dr->dr_sysbase < min_addr) {
				psize_t diff = min_addr - dr->dr_sysbase;
				dr->dr_busbase += diff;
				dr->dr_len -= diff;
				dr->dr_sysbase += diff;
			}
			if (max_addr != 0xffffffff
			    && max_addr + 1 < dr->dr_sysbase + dr->dr_len) {
				dr->dr_len = max_addr + 1 - dr->dr_sysbase;
			}
			dr++;
		}
	}

	return 0;
#else
	return EOPNOTSUPP;
#endif /* _ARM32_NEED_BUS_DMA_BOUNCE */
}

void
_bus_dmatag_destroy(bus_dma_tag_t tag)
{
#ifdef _ARM32_NEED_BUS_DMA_BOUNCE
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
