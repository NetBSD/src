/*	$NetBSD: int_bus_dma.c,v 1.13 2003/09/06 11:12:53 rearnsha Exp $	*/

/*
 * Copyright (c) 2002 Wasabi Systems, Inc.
 * All rights reserved.
 *
 * Written by Jason R. Thorpe for Wasabi Systems, Inc.
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
 *	This product includes software developed for the NetBSD Project by
 *	Wasabi Systems, Inc.
 * 4. The name of Wasabi Systems, Inc. may not be used to endorse
 *    or promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY WASABI SYSTEMS, INC. ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL WASABI SYSTEMS, INC
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * PCI DMA support for the ARM Integrator.
 */

#define	_ARM32_BUS_DMA_PRIVATE

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: int_bus_dma.c,v 1.13 2003/09/06 11:12:53 rearnsha Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/malloc.h>
#include <sys/mbuf.h>

#include <uvm/uvm_extern.h>

#include <machine/bootconfig.h>

#include <evbarm/integrator/int_bus_dma.h>

struct integrator_dma_cookie {
	int	id_flags;	/* flags; see below */

	/*
	 * Information about the original buffer used during
	 * DMA map syncs.  Note that origbuflen is only used
	 * for ID_BUFTYPE_LINEAR.
	 */
	void	*id_origbuf;		/* pointer to orig buffer if
					   bouncing */
	bus_size_t id_origbuflen;	/* ...and size */
	int	id_buftype;		/* type of buffer */

	void	*id_bouncebuf;		/* pointer to the bounce buffer */
	bus_size_t id_bouncebuflen;	/* ...and size */
	int	id_nbouncesegs;		/* number of valid bounce segs */
	bus_dma_segment_t id_bouncesegs[0]; /* array of bounce buffer
					       physical memory segments */
};
/* id_flags */
#define ID_MIGHT_NEED_BOUNCE	0x01	/* map could need bounce buffers */
#define ID_HAS_BOUNCE		0x02	/* map currently has bounce buffers */
#define ID_IS_BOUNCING		0x04	/* map is bouncing current xfer */

/* id_buftype */
#define ID_BUFTYPE_INVALID	0
#define ID_BUFTYPE_LINEAR	1
#define ID_BUFTYPE_MBUF		2
#define ID_BUFTYPE_UIO		3
#define ID_BUFTYPE_RAW		4

#define DEBUG(x)

static struct arm32_dma_range integrator_dma_ranges[DRAM_BLOCKS];

extern BootConfig bootconfig;

static int integrator_bus_dmamap_create(bus_dma_tag_t, bus_size_t, int,
    bus_size_t, bus_size_t, int, bus_dmamap_t *);
static void integrator_bus_dmamap_destroy(bus_dma_tag_t, bus_dmamap_t);
static int  integrator_bus_dmamap_load(bus_dma_tag_t, bus_dmamap_t, void *,
    bus_size_t, struct proc *, int);
static int  integrator_bus_dmamap_load_mbuf(bus_dma_tag_t, bus_dmamap_t,
    struct mbuf *, int);
static int  integrator_bus_dmamap_load_uio(bus_dma_tag_t, bus_dmamap_t,
    struct uio *, int);
static int  integrator_bus_dmamap_load_raw(bus_dma_tag_t, bus_dmamap_t,
    bus_dma_segment_t *, int, bus_size_t, int);
static void integrator_bus_dmamap_unload(bus_dma_tag_t, bus_dmamap_t);
static void integrator_bus_dmamap_sync(bus_dma_tag_t, bus_dmamap_t,
    bus_addr_t, bus_size_t, int);
static int  integrator_bus_dmamem_alloc(bus_dma_tag_t, bus_size_t,
    bus_size_t, bus_size_t, bus_dma_segment_t *, int, int *, int);
static int  integrator_dma_alloc_bouncebuf(bus_dma_tag_t, bus_dmamap_t,
    bus_size_t, int);
static void integrator_dma_free_bouncebuf(bus_dma_tag_t, bus_dmamap_t);


/*
 * Create an Integrator DMA map.
 */
static int
integrator_bus_dmamap_create(bus_dma_tag_t t, bus_size_t size, int nsegments,
    bus_size_t maxsegsz, bus_size_t boundary, int flags, bus_dmamap_t *dmamp)
{
	struct integrator_dma_cookie *cookie;
	bus_dmamap_t map;
	int error, cookieflags;
	void *cookiestore;
	size_t cookiesize;

	DEBUG(printf("I_bus_dmamap_create(tag %x, size %x, nseg %d, max %x,"
	    " boundary %x, flags %x, dmamap %p)\n", (unsigned) t,
	    (unsigned) size, nsegments, (unsigned) maxsegsz,
	    (unsigned)boundary, flags, dmamp));

	/* Call common function to create the basic map. */
	error = _bus_dmamap_create(t, size, nsegments, maxsegsz, boundary,
	    flags, dmamp);
	if (error)
		return (error);

	map = *dmamp;
	map->_dm_cookie = NULL;

	cookiesize = sizeof(struct integrator_dma_cookie);

	/*
	 * Some CM boards have private memory which is significantly
	 * faster than the normal memory stick.  To support this
	 * memory we have to bounce any DMA transfers.
	 *
	 * In order to DMA to arbitrary buffers, we use "bounce
	 * buffers" - pages in in the main PCI visible memory.  On DMA
	 * reads, DMA happens to the bounce buffers, and is copied
	 * into the caller's buffer.  On writes, data is copied into
	 * but bounce buffer, and the DMA happens from those pages.
	 * To software using the DMA mapping interface, this looks
	 * simply like a data cache.
	 *
	 * If we have private RAM in the system, we may need bounce
	 * buffers.  We check and remember that here.
	 */
#if 0
	cookieflags = ID_MIGHT_NEED_BOUNCE;
#else
	cookieflags = 0;
#endif
	cookiesize += (sizeof(bus_dma_segment_t) * map->_dm_segcnt);

	/*
	 * Allocate our cookie.
	 */
	if ((cookiestore = malloc(cookiesize, M_DMAMAP,
	    (flags & BUS_DMA_NOWAIT) ? M_NOWAIT : M_WAITOK)) == NULL) {
		error = ENOMEM;
		goto out;
	}
	memset(cookiestore, 0, cookiesize);
	cookie = (struct integrator_dma_cookie *)cookiestore;
	cookie->id_flags = cookieflags;
	map->_dm_cookie = cookie;

	if (cookieflags & ID_MIGHT_NEED_BOUNCE) {
		/*
		 * Allocate the bounce pages now if the caller
		 * wishes us to do so.
		 */
		if ((flags & BUS_DMA_ALLOCNOW) == 0)
			goto out;

		DEBUG(printf("I_bus_dmamap_create bouncebuf alloc\n"));
		error = integrator_dma_alloc_bouncebuf(t, map, size, flags);
	}

 out:
	if (error) {
		if (map->_dm_cookie != NULL)
			free(map->_dm_cookie, M_DMAMAP);
		_bus_dmamap_destroy(t, map);
		printf("I_bus_dmamap_create failed (%d)\n", error);
	}
	return (error);
}

/*
 * Destroy an ISA DMA map.
 */
static void
integrator_bus_dmamap_destroy(bus_dma_tag_t t, bus_dmamap_t map)
{
	struct integrator_dma_cookie *cookie = map->_dm_cookie;

	DEBUG(printf("I_bus_dmamap_destroy (tag %x, map %x)\n", (unsigned) t,
	    (unsigned) map));
	/*
	 * Free any bounce pages this map might hold.
	 */
	if (cookie->id_flags & ID_HAS_BOUNCE) {
		DEBUG(printf("I_bus_dmamap_destroy bouncebuf\n"));
		integrator_dma_free_bouncebuf(t, map);
	}

	free(cookie, M_DMAMAP);
	_bus_dmamap_destroy(t, map);
}

/*
 * Load an Integrator DMA map with a linear buffer.
 */
static int
integrator_bus_dmamap_load(bus_dma_tag_t t, bus_dmamap_t map, void *buf,
    bus_size_t buflen, struct proc *p, int flags)
{
	struct integrator_dma_cookie *cookie = map->_dm_cookie;
	int error;

	DEBUG(printf("I_bus_dmamap_load (tag %x, map %x, buf %p, len %u,"
	    " proc %p, flags %d)\n", (unsigned) t, (unsigned) map, buf,
	    (unsigned) buflen, p, flags));
	/*
	 * Make sure that on error condition we return "no valid mappings."
	 */
	map->dm_mapsize = 0;
	map->dm_nsegs = 0;

	/*
	 * Try to load the map the normal way.  If this errors out,
	 * and we can bounce, we will.
	 */
	error = _bus_dmamap_load(t, map, buf, buflen, p, flags);
	if (error == 0 ||
	    (error != 0 && (cookie->id_flags & ID_MIGHT_NEED_BOUNCE) == 0))
		return (error);

	/*
	 * First attempt failed; bounce it.
	 */

	/*
	 * Allocate bounce pages, if necessary.
	 */
	if ((cookie->id_flags & ID_HAS_BOUNCE) == 0) {
		DEBUG(printf("I_bus_dmamap_load alloc bouncebuf\n"));
		error = integrator_dma_alloc_bouncebuf(t, map, buflen, flags);
		if (error)
			return (error);
	}

	/*
	 * Cache a pointer to the caller's buffer and load the DMA map
	 * with the bounce buffer.
	 */
	cookie->id_origbuf = buf;
	cookie->id_origbuflen = buflen;
	cookie->id_buftype = ID_BUFTYPE_LINEAR;
	error = _bus_dmamap_load(t, map, cookie->id_bouncebuf, buflen,
	    NULL, flags);
	if (error) {
		/*
		 * Free the bounce pages, unless our resources
		 * are reserved for our exclusive use.
		 */
		if ((map->_dm_flags & BUS_DMA_ALLOCNOW) == 0)
			integrator_dma_free_bouncebuf(t, map);
		return (error);
	}

	/* ...so integrator_bus_dmamap_sync() knows we're bouncing */
	cookie->id_flags |= ID_IS_BOUNCING;
	return (0);
}

/*
 * Like integrator_bus_dmamap_load(), but for mbufs.
 */
static int
integrator_bus_dmamap_load_mbuf(bus_dma_tag_t t, bus_dmamap_t map,
    struct mbuf *m0, int flags)
{
	struct integrator_dma_cookie *cookie = map->_dm_cookie;
	int error;

	/*
	 * Make sure that on error condition we return "no valid mappings."
	 */
	map->dm_mapsize = 0;
	map->dm_nsegs = 0;

#ifdef DIAGNOSTIC
	if ((m0->m_flags & M_PKTHDR) == 0)
		panic("integrator_bus_dmamap_load_mbuf: no packet header");
#endif

	if (m0->m_pkthdr.len > map->_dm_size)
		return (EINVAL);

	/*
	 * Try to load the map the normal way.  If this errors out,
	 * and we can bounce, we will.
	 */
	error = _bus_dmamap_load_mbuf(t, map, m0, flags);
	if (error == 0 ||
	    (error != 0 && (cookie->id_flags & ID_MIGHT_NEED_BOUNCE) == 0))
		return (error);

	/*
	 * First attempt failed; bounce it.
	 *
	 * Allocate bounce pages, if necessary.
	 */
	if ((cookie->id_flags & ID_HAS_BOUNCE) == 0) {
		error = integrator_dma_alloc_bouncebuf(t, map,
		    m0->m_pkthdr.len, flags);
		if (error)
			return (error);
	}

	/*
	 * Cache a pointer to the caller's buffer and load the DMA map
	 * with the bounce buffer.
	 */
	cookie->id_origbuf = m0;
	cookie->id_origbuflen = m0->m_pkthdr.len;	/* not really used */
	cookie->id_buftype = ID_BUFTYPE_MBUF;
	error = _bus_dmamap_load(t, map, cookie->id_bouncebuf,
	    m0->m_pkthdr.len, NULL, flags);
	if (error) {
		/*
		 * Free the bounce pages, unless our resources
		 * are reserved for our exclusive use.
		 */
		if ((map->_dm_flags & BUS_DMA_ALLOCNOW) == 0)
			integrator_dma_free_bouncebuf(t, map);
		return (error);
	}

	/* ...so integrator_bus_dmamap_sync() knows we're bouncing */
	cookie->id_flags |= ID_IS_BOUNCING;
	return (0);
}

/*
 * Like integrator_bus_dmamap_load(), but for uios.
 */
static int
integrator_bus_dmamap_load_uio(bus_dma_tag_t t, bus_dmamap_t map,
    struct uio *uio, int flags)
{

	panic("integrator_bus_dmamap_load_uio: not implemented");
}

/*
 * Like intgrator_bus_dmamap_load(), but for raw memory allocated with
 * bus_dmamem_alloc().
 */
static int
integrator_bus_dmamap_load_raw(bus_dma_tag_t t, bus_dmamap_t map,
    bus_dma_segment_t *segs, int nsegs, bus_size_t size, int flags)
{

	panic("integrator_bus_dmamap_load_raw: not implemented");
}

/*
 * Unload an Integrator DMA map.
 */
static void
integrator_bus_dmamap_unload(bus_dma_tag_t t, bus_dmamap_t map)
{
	struct integrator_dma_cookie *cookie = map->_dm_cookie;

	/*
	 * If we have bounce pages, free them, unless they're
	 * reserved for our exclusive use.
	 */
	if ((cookie->id_flags & ID_HAS_BOUNCE) &&
	    (map->_dm_flags & BUS_DMA_ALLOCNOW) == 0)
		integrator_dma_free_bouncebuf(t, map);

	cookie->id_flags &= ~ID_IS_BOUNCING;
	cookie->id_buftype = ID_BUFTYPE_INVALID;

	/*
	 * Do the generic bits of the unload.
	 */
	_bus_dmamap_unload(t, map);
}

/*
 * Synchronize an Integrator DMA map.
 */
static void
integrator_bus_dmamap_sync(bus_dma_tag_t t, bus_dmamap_t map,
    bus_addr_t offset, bus_size_t len, int ops)
{
	struct integrator_dma_cookie *cookie = map->_dm_cookie;

	DEBUG(printf("I_bus_dmamap_sync (tag %x, map %x, offset %x, size %u,"
	    " ops %d\n", (unsigned)t, (unsigned)map, (unsigned)offset ,
	    (unsigned)len, ops));
	/*
	 * Mixing PRE and POST operations is not allowed.
	 */
	if ((ops & (BUS_DMASYNC_PREREAD|BUS_DMASYNC_PREWRITE)) != 0 &&
	    (ops & (BUS_DMASYNC_POSTREAD|BUS_DMASYNC_POSTWRITE)) != 0)
		panic("integrator_bus_dmamap_sync: mix PRE and POST");

#ifdef DIAGNOSTIC
	if ((ops & (BUS_DMASYNC_PREWRITE|BUS_DMASYNC_POSTREAD)) != 0) {
		if (offset >= map->dm_mapsize)
			panic("integrator_bus_dmamap_sync: bad offset");
		if (len == 0 || (offset + len) > map->dm_mapsize)
			panic("integrator_bus_dmamap_sync: bad length");
	}
#endif

	/*
	 * If we're not bouncing then use the standard code.
	 */
	if ((cookie->id_flags & ID_IS_BOUNCING) == 0) {
		_bus_dmamap_sync(t, map, offset, len, ops);
		return;
	}

	DEBUG(printf("dmamap_sync(");
	if (ops & BUS_DMASYNC_PREREAD)
		printf("preread ");
	if (ops & BUS_DMASYNC_PREWRITE)
		printf("prewrite ");
	if (ops & BUS_DMASYNC_POSTREAD)
		printf("postread ");
	if (ops & BUS_DMASYNC_POSTWRITE)
		printf("postwrite ");)

	switch (cookie->id_buftype) {
	case ID_BUFTYPE_LINEAR:
		if (ops & BUS_DMASYNC_PREWRITE) {
			/*
			 * Copy the caller's buffer to the bounce buffer.
			 */
			memcpy((char *)cookie->id_bouncebuf + offset,
			    (char *)cookie->id_origbuf + offset, len);
			cpu_dcache_wbinv_range((vaddr_t)cookie->id_bouncebuf +
			    offset, len);
		}
		if (ops & BUS_DMASYNC_PREREAD) {
			cpu_dcache_wbinv_range((vaddr_t)cookie->id_bouncebuf +
			    offset, len);
		}
		if (ops & BUS_DMASYNC_POSTREAD) {
			/*
			 * Copy the bounce buffer to the caller's buffer.
			 */
			memcpy((char *)cookie->id_origbuf + offset,
			    (char *)cookie->id_bouncebuf + offset, len);
		}

		/*
		 * Nothing to do for post-write.
		 */
		break;

	case ID_BUFTYPE_MBUF:
	    {
		struct mbuf *m, *m0 = cookie->id_origbuf;
		bus_size_t minlen, moff;

		if (ops & BUS_DMASYNC_PREWRITE) {
			/*
			 * Copy the caller's buffer to the bounce buffer.
			 */
			m_copydata(m0, offset, len,
			    (char *)cookie->id_bouncebuf + offset);
			cpu_dcache_wb_range((vaddr_t)cookie->id_bouncebuf +
			    offset, len);
		}
		if (ops & BUS_DMASYNC_PREREAD) {
			cpu_dcache_wbinv_range ((vaddr_t)cookie->id_bouncebuf +
			    offset, len);
		}
		if (ops & BUS_DMASYNC_POSTREAD) {
			/*
			 * Copy the bounce buffer to the caller's buffer.
			 */
			for (moff = offset, m = m0; m != NULL && len != 0;
			    m = m->m_next) {
				/* Find the beginning mbuf. */
				if (moff >= m->m_len) {
					moff -= m->m_len;
					continue;
				}

				/*
				 * Now at the first mbuf to sync; nail
				 * each one until we have exhausted the
				 * length.
				 */
				minlen = len < m->m_len - moff ?
				    len : m->m_len - moff;

				memcpy(mtod(m, caddr_t) + moff,
				    (char *)cookie->id_bouncebuf + offset,
				    minlen);

				moff = 0;
				len -= minlen;
				offset += minlen;
			}
		}
		/*
		 * Nothing to do for post-write.
		 */
		break;
	    }
	
	case ID_BUFTYPE_UIO:
		panic("integrator_bus_dmamap_sync: ID_BUFTYPE_UIO");
		break;

	case ID_BUFTYPE_RAW:
		panic("integrator_bus_dmamap_sync: ID_BUFTYPE_RAW");
		break;

	case ID_BUFTYPE_INVALID:
		panic("integrator_bus_dmamap_sync: ID_BUFTYPE_INVALID");
		break;

	default:
		printf("unknown buffer type %d\n", cookie->id_buftype);
		panic("integrator_bus_dmamap_sync");
	}
}

/*
 * Allocate memory safe for Integrator DMA.
 */
static int
integrator_bus_dmamem_alloc(bus_dma_tag_t t, bus_size_t size,
    bus_size_t alignment, bus_size_t boundary, bus_dma_segment_t *segs,
    int nsegs, int *rsegs, int flags)
{

	if (t->_ranges == NULL)
		return (ENOMEM);

	/* _bus_dmamem_alloc() does the range checks for us. */
	return (_bus_dmamem_alloc(t, size, alignment, boundary, segs, nsegs,
	    rsegs, flags));
}

/**********************************************************************
 * Integrator DMA utility functions
 **********************************************************************/

static int
integrator_dma_alloc_bouncebuf(bus_dma_tag_t t, bus_dmamap_t map,
    bus_size_t size, int flags)
{
	struct integrator_dma_cookie *cookie = map->_dm_cookie;
	int error = 0;

	DEBUG(printf("Alloc bouncebuf\n"));
	cookie->id_bouncebuflen = round_page(size);
	error = integrator_bus_dmamem_alloc(t, cookie->id_bouncebuflen,
	    NBPG, map->_dm_boundary, cookie->id_bouncesegs,
	    map->_dm_segcnt, &cookie->id_nbouncesegs, flags);
	if (error)
		goto out;
	{
		int seg;

		for (seg = 0; seg < cookie->id_nbouncesegs; seg++)
			DEBUG(printf("Seg %d @ PA 0x%08x+0x%x\n", seg,
			    (unsigned) cookie->id_bouncesegs[seg].ds_addr,
			    (unsigned) cookie->id_bouncesegs[seg].ds_len));
	}
	error = _bus_dmamem_map(t, cookie->id_bouncesegs,
	    cookie->id_nbouncesegs, cookie->id_bouncebuflen,
	    (caddr_t *)&cookie->id_bouncebuf, flags);

 out:
	if (error) {
		_bus_dmamem_free(t, cookie->id_bouncesegs,
		    cookie->id_nbouncesegs);
		cookie->id_bouncebuflen = 0;
		cookie->id_nbouncesegs = 0;
	} else {
		DEBUG(printf("Alloc bouncebuf OK\n"));
		cookie->id_flags |= ID_HAS_BOUNCE;
	}

	return (error);
}

static void
integrator_dma_free_bouncebuf(bus_dma_tag_t t, bus_dmamap_t map)
{
	struct integrator_dma_cookie *cookie = map->_dm_cookie;

	_bus_dmamem_unmap(t, cookie->id_bouncebuf,
	    cookie->id_bouncebuflen);
	_bus_dmamem_free(t, cookie->id_bouncesegs,
	    cookie->id_nbouncesegs);
	cookie->id_bouncebuflen = 0;
	cookie->id_nbouncesegs = 0;
	cookie->id_flags &= ~ID_HAS_BOUNCE;
}

void
integrator_pci_dma_init(bus_dma_tag_t dmat)
{
	struct arm32_dma_range *dr = integrator_dma_ranges;
	int i;
	int nranges = 0;
	
	for (i = 0; i < bootconfig.dramblocks; i++)
		if (bootconfig.dram[i].flags & BOOT_DRAM_CAN_DMA) {
			dr[nranges].dr_sysbase = bootconfig.dram[i].address;
			dr[nranges].dr_busbase = 
			    LOCAL_TO_CM_ALIAS(dr[nranges].dr_sysbase);
			dr[nranges].dr_len = bootconfig.dram[i].pages * NBPG;
			nranges++;
		}

	if (nranges == 0)
		panic ("integrator_pci_dma_init: No DMA capable memory"); 

	dmat->_ranges = dr;
	dmat->_nranges = nranges;

	dmat->_dmamap_create = integrator_bus_dmamap_create;
	dmat->_dmamap_destroy = integrator_bus_dmamap_destroy;
	dmat->_dmamap_load = integrator_bus_dmamap_load;
	dmat->_dmamap_load_mbuf = integrator_bus_dmamap_load_mbuf;
	dmat->_dmamap_load_uio = integrator_bus_dmamap_load_uio;
	dmat->_dmamap_load_raw = integrator_bus_dmamap_load_raw;
	dmat->_dmamap_unload = integrator_bus_dmamap_unload;
	dmat->_dmamap_sync_pre = integrator_bus_dmamap_sync;
	dmat->_dmamap_sync_post = integrator_bus_dmamap_sync;

	dmat->_dmamem_alloc = integrator_bus_dmamem_alloc;
	dmat->_dmamem_free = _bus_dmamem_free;
	dmat->_dmamem_map = _bus_dmamem_map;
	dmat->_dmamem_unmap = _bus_dmamem_unmap;
	dmat->_dmamem_mmap = _bus_dmamem_mmap;
}
