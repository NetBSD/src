/* $NetBSD: isadma_bounce.c,v 1.13.30.2 2016/03/19 11:29:55 skrll Exp $ */
/* NetBSD: isadma_bounce.c,v 1.2 2000/06/01 05:49:36 thorpej Exp  */

/*-
 * Copyright (c) 1996, 1997, 1998, 2000 The NetBSD Foundation, Inc.
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

__KERNEL_RCSID(0, "$NetBSD: isadma_bounce.c,v 1.13.30.2 2016/03/19 11:29:55 skrll Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/syslog.h>
#include <sys/device.h>
#include <sys/malloc.h>
#include <sys/proc.h>
#include <sys/mbuf.h>

#define _ARC_BUS_DMA_PRIVATE
#include <sys/bus.h>

#include <dev/isa/isareg.h>
#include <dev/isa/isavar.h>

#include <uvm/uvm_extern.h>

/*
 * Cookie used by bouncing ISA DMA.  A pointer to one of these is stashed
 * in the DMA map.
 */
struct isadma_bounce_cookie {
	int	id_flags;		/* flags; see below */

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
	bus_dma_segment_t id_bouncesegs[1]; /* array of bounce buffer
					       physical memory segments */
};

/* id_flags */
#define	ID_MIGHT_NEED_BOUNCE	0x01	/* map could need bounce buffers */
#define	ID_HAS_BOUNCE		0x02	/* map currently has bounce buffers */
#define	ID_IS_BOUNCING		0x04	/* map is bouncing current xfer */

/* id_buftype */
#define	ID_BUFTYPE_INVALID	0
#define	ID_BUFTYPE_LINEAR	1
#define	ID_BUFTYPE_MBUF		2
#define	ID_BUFTYPE_UIO		3
#define	ID_BUFTYPE_RAW		4

int	isadma_bounce_dmamap_create(bus_dma_tag_t, bus_size_t, int,
	    bus_size_t, bus_size_t, int, bus_dmamap_t *);
void	isadma_bounce_dmamap_destroy(bus_dma_tag_t, bus_dmamap_t);
int	isadma_bounce_dmamap_load(bus_dma_tag_t, bus_dmamap_t, void *,
	    bus_size_t, struct proc *, int);
int	isadma_bounce_dmamap_load_mbuf(bus_dma_tag_t, bus_dmamap_t,
	    struct mbuf *, int);
int	isadma_bounce_dmamap_load_uio(bus_dma_tag_t, bus_dmamap_t,
	    struct uio *, int);
int	isadma_bounce_dmamap_load_raw(bus_dma_tag_t, bus_dmamap_t,
	    bus_dma_segment_t *, int, bus_size_t, int);
void	isadma_bounce_dmamap_unload(bus_dma_tag_t, bus_dmamap_t);
void	isadma_bounce_dmamap_sync(bus_dma_tag_t, bus_dmamap_t,
	    bus_addr_t, bus_size_t, int);
int	isadma_bounce_dmamem_alloc(bus_dma_tag_t, bus_size_t, bus_size_t,
	    bus_size_t, bus_dma_segment_t *, int, int *, int);

static int isadma_bounce_alloc_bouncebuf(bus_dma_tag_t, bus_dmamap_t,
	    bus_size_t, int);
static void isadma_bounce_free_bouncebuf(bus_dma_tag_t, bus_dmamap_t);

void
isadma_bounce_tag_init(bus_dma_tag_t t)
{
	/*
	 * Initialize the DMA tag used for ISA DMA.
	 */

	_bus_dma_tag_init(t);

	t->_dmamap_create = isadma_bounce_dmamap_create;
	t->_dmamap_destroy = isadma_bounce_dmamap_destroy;
	t->_dmamap_load = isadma_bounce_dmamap_load;
	t->_dmamap_load_mbuf = isadma_bounce_dmamap_load_mbuf;
	t->_dmamap_load_uio = isadma_bounce_dmamap_load_uio;
	t->_dmamap_load_raw = isadma_bounce_dmamap_load_raw;
	t->_dmamap_unload = isadma_bounce_dmamap_unload;
	t->_dmamap_sync = isadma_bounce_dmamap_sync;
	t->_dmamem_alloc = isadma_bounce_dmamem_alloc;
}

/*
 * Create an ISA DMA map.
 */
int
isadma_bounce_dmamap_create(bus_dma_tag_t t, bus_size_t size, int nsegments,
    bus_size_t maxsegsz, bus_size_t boundary, int flags, bus_dmamap_t *dmamp)
{
	struct isadma_bounce_cookie *cookie;
	bus_dmamap_t map;
	int error, cookieflags;
	void *cookiestore;
	size_t cookiesize;

	/* Call common function to create the basic map. */
	error = _bus_dmamap_create(t, size, nsegments, maxsegsz, boundary,
	    flags, dmamp);
	if (error)
		return error;

	map = *dmamp;
	map->_dm_cookie = NULL;

	cookiesize = sizeof(*cookie);

	/*
	 * ISA only has 24-bits of address space.  This means
	 * we can't DMA to pages over 16M.  In order to DMA to
	 * arbitrary buffers, we use "bounce buffers" - pages
	 * in memory below the 16M boundary.  On DMA reads,
	 * DMA happens to the bounce buffers, and is copied into
	 * the caller's buffer.  On writes, data is copied into
	 * but bounce buffer, and the DMA happens from those
	 * pages.  To software using the DMA mapping interface,
	 * this looks simply like a data cache.
	 *
	 * If we have more than 16M of RAM in the system, we may
	 * need bounce buffers.  We check and remember that here.
	 *
	 * ...or, there is an opposite case.  The most segments
	 * a transfer will require is (maxxfer / PAGE_SIZE) + 1.  If
	 * the caller can't handle that many segments (e.g. the
	 * ISA DMA controller), we may have to bounce it as well.
	 */
	cookieflags = 0;
	if (pmap_limits.avail_end > ISA_DMA_BOUNCE_THRESHOLD ||
	    ((map->_dm_size / PAGE_SIZE) + 1) > map->_dm_segcnt) {
		cookieflags |= ID_MIGHT_NEED_BOUNCE;
		cookiesize += (sizeof(bus_dma_segment_t) *
		    (map->_dm_segcnt - 1));
	}

	/*
	 * Allocate our cookie.
	 */
	if ((cookiestore = malloc(cookiesize, M_DMAMAP,
	    (flags & BUS_DMA_NOWAIT) ? M_NOWAIT : M_WAITOK)) == NULL) {
		error = ENOMEM;
		goto out;
	}
	memset(cookiestore, 0, cookiesize);
	cookie = (struct isadma_bounce_cookie *)cookiestore;
	cookie->id_flags = cookieflags;
	map->_dm_cookie = cookie;

	if (cookieflags & ID_MIGHT_NEED_BOUNCE) {
		/*
		 * Allocate the bounce pages now if the caller
		 * wishes us to do so.
		 */
		if ((flags & BUS_DMA_ALLOCNOW) == 0)
			goto out;

		error = isadma_bounce_alloc_bouncebuf(t, map, size, flags);
	}

 out:
	if (error) {
		if (map->_dm_cookie != NULL)
			free(map->_dm_cookie, M_DMAMAP);
		_bus_dmamap_destroy(t, map);
	}
	return error;
}

/*
 * Destroy an ISA DMA map.
 */
void
isadma_bounce_dmamap_destroy(bus_dma_tag_t t, bus_dmamap_t map)
{
	struct isadma_bounce_cookie *cookie = map->_dm_cookie;

	/*
	 * Free any bounce pages this map might hold.
	 */
	if (cookie->id_flags & ID_HAS_BOUNCE)
		isadma_bounce_free_bouncebuf(t, map);

	free(cookie, M_DMAMAP);
	_bus_dmamap_destroy(t, map);
}

/*
 * Load an ISA DMA map with a linear buffer.
 */
int
isadma_bounce_dmamap_load(bus_dma_tag_t t, bus_dmamap_t map, void *buf,
    bus_size_t buflen, struct proc *p, int flags)
{
	struct isadma_bounce_cookie *cookie = map->_dm_cookie;
	int error;

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
	if (error == 0 || (cookie->id_flags & ID_MIGHT_NEED_BOUNCE) == 0)
		return error;

	/*
	 * First attempt failed; bounce it.
	 */

	/*
	 * Allocate bounce pages, if necessary.
	 */
	if ((cookie->id_flags & ID_HAS_BOUNCE) == 0) {
		error = isadma_bounce_alloc_bouncebuf(t, map, buflen, flags);
		if (error)
			return error;
	}

	/*
	 * Cache a pointer to the caller's buffer and load the DMA map
	 * with the bounce buffer.
	 */
	cookie->id_origbuf = buf;
	cookie->id_origbuflen = buflen;
	cookie->id_buftype = ID_BUFTYPE_LINEAR;
	error = _bus_dmamap_load(t, map, cookie->id_bouncebuf, buflen,
	    p, flags);
	if (error) {
		/*
		 * Free the bounce pages, unless our resources
		 * are reserved for our exclusive use.
		 */
		if ((map->_dm_flags & BUS_DMA_ALLOCNOW) == 0)
			isadma_bounce_free_bouncebuf(t, map);
		return error;
	}

	/* ...so isadma_bounce_dmamap_sync() knows we're bouncing */
	cookie->id_flags |= ID_IS_BOUNCING;
	return 0;
}

/*
 * Like isadma_bounce_dmamap_load(), but for mbufs.
 */
int
isadma_bounce_dmamap_load_mbuf(bus_dma_tag_t t, bus_dmamap_t map,
    struct mbuf *m0, int flags)
{
	struct isadma_bounce_cookie *cookie = map->_dm_cookie;
	int error;

	/*
	 * Make sure on error condition we return "no valid mappings."
	 */
	map->dm_mapsize = 0;
	map->dm_nsegs = 0;

#ifdef DIAGNOSTIC
	if ((m0->m_flags & M_PKTHDR) == 0)
		panic("isadma_bounce_dmamap_load_mbuf: no packet header");
#endif

	if (m0->m_pkthdr.len > map->_dm_size)
		return EINVAL;

	/*
	 * Try to load the map the normal way.  If this errors out,
	 * and we can bounce, we will.
	 */
	error = _bus_dmamap_load_mbuf(t, map, m0, flags);
	if (error == 0 || (cookie->id_flags & ID_MIGHT_NEED_BOUNCE) == 0)
		return error;

	/*
	 * First attempt failed; bounce it.
	 */

	/*
	 * Allocate bounce pages, if necessary.
	 */
	if ((cookie->id_flags & ID_HAS_BOUNCE) == 0) {
		error = isadma_bounce_alloc_bouncebuf(t, map, m0->m_pkthdr.len,
		    flags);
		if (error)
			return error;
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
			isadma_bounce_free_bouncebuf(t, map);
		return error;
	}

	/* ...so isadma_bounce_dmamap_sync() knows we're bouncing */
	cookie->id_flags |= ID_IS_BOUNCING;
	return 0;
}

/*
 * Like isadma_bounce_dmamap_load(), but for uios.
 */
int
isadma_bounce_dmamap_load_uio(bus_dma_tag_t t, bus_dmamap_t map,
    struct uio *uio, int flags)
{

	panic("isadma_bounce_dmamap_load_uio: not implemented");
}

/*
 * Like isadma_bounce_dmamap_load(), but for raw memory allocated with
 * bus_dmamem_alloc().
 */
int
isadma_bounce_dmamap_load_raw(bus_dma_tag_t t, bus_dmamap_t map,
    bus_dma_segment_t *segs, int nsegs, bus_size_t size, int flags)
{

	panic("isadma_bounce_dmamap_load_raw: not implemented");
}

/*
 * Unload an ISA DMA map.
 */
void
isadma_bounce_dmamap_unload(bus_dma_tag_t t, bus_dmamap_t map)
{
	struct isadma_bounce_cookie *cookie = map->_dm_cookie;

	/*
	 * If we have bounce pages, free them, unless they're
	 * reserved for our exclusive use.
	 */
	if ((cookie->id_flags & ID_HAS_BOUNCE) &&
	    (map->_dm_flags & BUS_DMA_ALLOCNOW) == 0)
		isadma_bounce_free_bouncebuf(t, map);

	cookie->id_flags &= ~ID_IS_BOUNCING;
	cookie->id_buftype = ID_BUFTYPE_INVALID;

	/*
	 * Do the generic bits of the unload.
	 */
	_bus_dmamap_unload(t, map);
}

/*
 * Synchronize an ISA DMA map.
 */
void
isadma_bounce_dmamap_sync(bus_dma_tag_t t, bus_dmamap_t map, bus_addr_t offset,
    bus_size_t len, int ops)
{
	struct isadma_bounce_cookie *cookie = map->_dm_cookie;
	void (*sync)(bus_dma_tag_t, bus_dmamap_t, bus_addr_t, bus_size_t, int);

	sync = _bus_dmamap_sync;

	/*
	 * Mixing PRE and POST operations is not allowed.
	 */
	if ((ops & (BUS_DMASYNC_PREREAD|BUS_DMASYNC_PREWRITE)) != 0 &&
	    (ops & (BUS_DMASYNC_POSTREAD|BUS_DMASYNC_POSTWRITE)) != 0)
		panic("isadma_bounce_dmamap_sync: mix PRE and POST");

#ifdef DIAGNOSTIC
	if ((ops & (BUS_DMASYNC_PREWRITE|BUS_DMASYNC_POSTREAD)) != 0) {
		if (offset >= map->dm_mapsize)
			panic("isadma_bounce_dmamap_sync: bad offset");
		if (len == 0 || (offset + len) > map->dm_mapsize)
			panic("isadma_bounce_dmamap_sync: bad length");
	}
#endif

	/*
	 * If we're not bouncing, just drain the write buffer
	 * and flush cache.
	 */
	if ((cookie->id_flags & ID_IS_BOUNCING) == 0) {
		((*sync)(t, map, offset, len, ops));
		return;
	}

	/*
	 * XXX
	 * This should be needed in BUS_DMASYNC_POSTREAD case only,
	 * if _mips3_bus_dmamap_sync() used "Hit_Invalidate on POSTREAD",
	 * rather than "Hit_Write_Back_Invalidate on PREREAD".
	 */
	if (ops & (BUS_DMASYNC_PREREAD|BUS_DMASYNC_POSTREAD))
		((*sync)(t, map, offset, len, ops));

	switch (cookie->id_buftype) {
	case ID_BUFTYPE_LINEAR:
		/*
		 * Nothing to do for pre-read.
		 */

		if (ops & BUS_DMASYNC_PREWRITE) {
			/*
			 * Copy the caller's buffer to the bounce buffer.
			 */
			memcpy((char *)cookie->id_bouncebuf + offset,
			    (char *)cookie->id_origbuf + offset, len);
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

		/*
		 * Nothing to do for pre-read.
		 */

		if (ops & BUS_DMASYNC_PREWRITE) {
			/*
			 * Copy the caller's buffer to the bounce buffer.
			 */
			m_copydata(m0, offset, len,
			    (char *)cookie->id_bouncebuf + offset);
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

				memcpy(mtod(m, char *) + moff,
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
		panic("isadma_bounce_dmamap_sync: ID_BUFTYPE_UIO");
		break;

	case ID_BUFTYPE_RAW:
		panic("isadma_bounce_dmamap_sync: ID_BUFTYPE_RAW");
		break;

	case ID_BUFTYPE_INVALID:
		panic("isadma_bounce_dmamap_sync: ID_BUFTYPE_INVALID");
		break;

	default:
		printf("unknown buffer type %d\n", cookie->id_buftype);
		panic("isadma_bounce_dmamap_sync");
	}

	if (ops & BUS_DMASYNC_PREWRITE)
		((*sync)(t, map, offset, len, ops));
}

/*
 * Allocate memory safe for ISA DMA.
 */
int
isadma_bounce_dmamem_alloc(bus_dma_tag_t t, bus_size_t size,
    bus_size_t alignment, bus_size_t boundary, bus_dma_segment_t *segs,
    int nsegs, int *rsegs, int flags)
{
	paddr_t high;

	if (pmap_limits.avail_end > ISA_DMA_BOUNCE_THRESHOLD)
		high = trunc_page(ISA_DMA_BOUNCE_THRESHOLD);
	else
		high = trunc_page(pmap_limits.avail_end);

	return _bus_dmamem_alloc_range(t, size, alignment, boundary,
	    segs, nsegs, rsegs, flags, 0, high);
}

/**********************************************************************
 * ISA DMA utility functions
 **********************************************************************/

static int
isadma_bounce_alloc_bouncebuf(bus_dma_tag_t t, bus_dmamap_t map,
    bus_size_t size, int flags)
{
	struct isadma_bounce_cookie *cookie = map->_dm_cookie;
	int error = 0;

	cookie->id_bouncebuflen = round_page(size);
	error = isadma_bounce_dmamem_alloc(t, cookie->id_bouncebuflen,
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
	} else
		cookie->id_flags |= ID_HAS_BOUNCE;

	return error;
}

static void
isadma_bounce_free_bouncebuf(bus_dma_tag_t t, bus_dmamap_t map)
{
	struct isadma_bounce_cookie *cookie = map->_dm_cookie;

	_bus_dmamem_unmap(t, cookie->id_bouncebuf,
	    cookie->id_bouncebuflen);
	_bus_dmamem_free(t, cookie->id_bouncesegs,
	    cookie->id_nbouncesegs);
	cookie->id_bouncebuflen = 0;
	cookie->id_nbouncesegs = 0;
	cookie->id_flags &= ~ID_HAS_BOUNCE;
}
