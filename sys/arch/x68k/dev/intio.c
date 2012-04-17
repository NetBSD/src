/*	$NetBSD: intio.c,v 1.42.14.1 2012/04/17 00:07:02 yamt Exp $	*/

/*-
 * Copyright (c) 1998 The NetBSD Foundation, Inc.
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

/*
 * NetBSD/x68k internal I/O virtual bus.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: intio.c,v 1.42.14.1 2012/04/17 00:07:02 yamt Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/malloc.h>
#include <sys/mbuf.h>
#include <sys/extent.h>
#include <uvm/uvm_extern.h>

#include <machine/bus.h>
#include <machine/cpu.h>
#include <machine/frame.h>

#include <arch/x68k/dev/intiovar.h>


/*
 * bus_space(9) interface
 */
static int intio_bus_space_map(bus_space_tag_t, bus_addr_t, bus_size_t, int, bus_space_handle_t *);
static void intio_bus_space_unmap(bus_space_tag_t, bus_space_handle_t, bus_size_t);
static int intio_bus_space_subregion(bus_space_tag_t, bus_space_handle_t, bus_size_t, bus_size_t, bus_space_handle_t *);

static struct x68k_bus_space intio_bus = {
#if 0
	X68K_INTIO_BUS,
#endif
	intio_bus_space_map, intio_bus_space_unmap, intio_bus_space_subregion,
	x68k_bus_space_alloc, x68k_bus_space_free,
#if 0
	x68k_bus_space_barrier,
#endif

	0
};

/*
 * bus_dma(9) interface
 */
#define	INTIO_DMA_BOUNCE_THRESHOLD	(16 * 1024 * 1024)
int	_intio_bus_dmamap_create(bus_dma_tag_t, bus_size_t, int,
	    bus_size_t, bus_size_t, int, bus_dmamap_t *);
void	_intio_bus_dmamap_destroy(bus_dma_tag_t, bus_dmamap_t);
int	_intio_bus_dmamap_load(bus_dma_tag_t, bus_dmamap_t, void *,
	    bus_size_t, struct proc *, int);
int	_intio_bus_dmamap_load_mbuf(bus_dma_tag_t, bus_dmamap_t,
	    struct mbuf *, int);
int	_intio_bus_dmamap_load_uio(bus_dma_tag_t, bus_dmamap_t,
	    struct uio *, int);
int	_intio_bus_dmamap_load_raw(bus_dma_tag_t, bus_dmamap_t,
	    bus_dma_segment_t *, int, bus_size_t, int);
void	_intio_bus_dmamap_unload(bus_dma_tag_t, bus_dmamap_t);
void	_intio_bus_dmamap_sync(bus_dma_tag_t, bus_dmamap_t,
	    bus_addr_t, bus_size_t, int);

int	_intio_bus_dmamem_alloc(bus_dma_tag_t, bus_size_t, bus_size_t,
	    bus_size_t, bus_dma_segment_t *, int, int *, int);

int	_intio_dma_alloc_bouncebuf(bus_dma_tag_t, bus_dmamap_t,
	    bus_size_t, int);
void	_intio_dma_free_bouncebuf(bus_dma_tag_t, bus_dmamap_t);

struct x68k_bus_dma intio_bus_dma = {
	INTIO_DMA_BOUNCE_THRESHOLD,
	_intio_bus_dmamap_create,
	_intio_bus_dmamap_destroy,
	_intio_bus_dmamap_load,
	_intio_bus_dmamap_load_mbuf,
	_intio_bus_dmamap_load_uio,
	_intio_bus_dmamap_load_raw,
	_intio_bus_dmamap_unload,
	_intio_bus_dmamap_sync,
	_intio_bus_dmamem_alloc,
	x68k_bus_dmamem_free,
	x68k_bus_dmamem_map,
	x68k_bus_dmamem_unmap,
	x68k_bus_dmamem_mmap,
};

/*
 * autoconf stuff
 */
static int intio_match(device_t, cfdata_t, void *);
static void intio_attach(device_t, device_t, void *);
static int intio_search(device_t, cfdata_t, const int *, void *);
static int intio_print(void *, const char *);
static void intio_alloc_system_ports(struct intio_softc*);

CFATTACH_DECL_NEW(intio, sizeof(struct intio_softc),
    intio_match, intio_attach, NULL, NULL);

extern struct cfdriver intio_cd;

static int intio_attached;

static struct intio_interrupt_vector {
	intio_intr_handler_t	iiv_handler;
	void			*iiv_arg;
	struct evcnt		*iiv_evcnt;
} iiv[256] = {{0,},};

#ifdef DEBUG
int intio_debug = 0;
#endif

static int
intio_match(device_t parent, cfdata_t cf, void *aux)
{

	if (strcmp(aux, intio_cd.cd_name) != 0)
		return (0);
	if (intio_attached)
		return (0);

	return (1);
}

static void
intio_attach(device_t parent, device_t self, void *aux)
{
	struct intio_softc *sc = device_private(self);
	struct intio_attach_args ia;

	intio_attached = 1;

	aprint_normal(" mapped at %8p\n", intiobase);

	sc->sc_map = extent_create("intiomap",
				  INTIOBASE,
				  INTIOBASE + 0x400000,
				  NULL, 0, EX_NOWAIT);
	intio_alloc_system_ports(sc);

	sc->sc_bst = &intio_bus;
	sc->sc_bst->x68k_bus_device = self;
	sc->sc_dmat = &intio_bus_dma;
	sc->sc_dmac = 0;

	memset(iiv, 0, sizeof(struct intio_interrupt_vector) * 256);

	ia.ia_bst = sc->sc_bst;
	ia.ia_dmat = sc->sc_dmat;

	config_search_ia(intio_search, self, "intio", &ia);
}

static int
intio_search(device_t parent, cfdata_t cf, const int *ldesc, void *aux)
{
	struct intio_softc *sc = device_private(parent);
	struct intio_attach_args *ia = aux;

	ia->ia_bst = sc->sc_bst;
	ia->ia_dmat = sc->sc_dmat;
	ia->ia_name = cf->cf_name;
	ia->ia_addr = cf->cf_addr;
	ia->ia_intr = cf->cf_intr;
	ia->ia_dma = cf->cf_dma;
	ia->ia_dmaintr = cf->cf_dmaintr;

	if (config_match(parent, cf, ia) > 0)
		config_attach(parent, cf, ia, intio_print);

	return (0);
}

static int
intio_print(void *aux, const char *name)
{
	struct intio_attach_args *ia = aux;

/*	if (ia->ia_addr > 0)	*/
		aprint_normal(" addr 0x%06x", ia->ia_addr);
	if (ia->ia_intr > 0)
		aprint_normal(" intr 0x%02x", ia->ia_intr);
	if (ia->ia_dma >= 0) {
		aprint_normal(" using DMA ch%d", ia->ia_dma);
		if (ia->ia_dmaintr > 0)
			aprint_normal(" intr 0x%02x and 0x%02x",
				ia->ia_dmaintr, ia->ia_dmaintr+1);
	}

	return (QUIET);
}

/*
 * intio memory map manager
 */

int
intio_map_allocate_region(device_t parent, struct intio_attach_args *ia,
    enum intio_map_flag flag)
{
	struct intio_softc *sc = device_private(parent);
	struct extent *map = sc->sc_map;
	int r;

	r = extent_alloc_region(map, ia->ia_addr, ia->ia_size, 0);
#ifdef DEBUG
	if (intio_debug)
		extent_print(map);
#endif
	if (r == 0) {
		if (flag != INTIO_MAP_ALLOCATE)
		extent_free(map, ia->ia_addr, ia->ia_size, 0);
		return 0;
	}

	return -1;
}

int
intio_map_free_region(device_t parent, struct intio_attach_args *ia)
{
	struct intio_softc *sc = device_private(parent);
	struct extent *map = sc->sc_map;

	extent_free(map, ia->ia_addr, ia->ia_size, 0);
#ifdef DEBUG
	if (intio_debug)
		extent_print(map);
#endif
	return 0;
}

void
intio_alloc_system_ports(struct intio_softc *sc)
{
	extent_alloc_region(sc->sc_map, INTIO_SYSPORT, 16, 0);
	extent_alloc_region(sc->sc_map, INTIO_SICILIAN, 0x2000, 0);
}


/*
 * intio bus space stuff.
 */
static int
intio_bus_space_map(bus_space_tag_t t, bus_addr_t bpa, bus_size_t size,
    int flags, bus_space_handle_t *bshp)
{
	/*
	 * Intio bus is mapped permanently.
	 */
	*bshp = (bus_space_handle_t)IIOV(bpa);

	/*
	 * Some devices are mapped on odd or even addresses only.
	 */
	if ((flags & BUS_SPACE_MAP_SHIFTED_MASK) == BUS_SPACE_MAP_SHIFTED_ODD)
		*bshp += 0x80000001;
	if ((flags & BUS_SPACE_MAP_SHIFTED_MASK) == BUS_SPACE_MAP_SHIFTED_EVEN)
		*bshp += 0x80000000;

	return (0);
}

static void
intio_bus_space_unmap(bus_space_tag_t t, bus_space_handle_t bsh,
    bus_size_t size)
{
	return;
}

static int
intio_bus_space_subregion(bus_space_tag_t t, bus_space_handle_t bsh,
    bus_size_t offset, bus_size_t size, bus_space_handle_t *nbshp)
{

	*nbshp = bsh + offset;
	return (0);
}


/*
 * interrupt handler
 */
int
intio_intr_establish(int vector, const char *name, intio_intr_handler_t handler,
    void *arg)
{

	return intio_intr_establish_ext(vector, name, "intr", handler, arg);
}

int
intio_intr_establish_ext(int vector, const char *name1, const char *name2,
	intio_intr_handler_t handler, void *arg)
{
	struct evcnt *evcnt;

	if (vector < 16)
		panic("Invalid interrupt vector");
	if (iiv[vector].iiv_handler)
		return EBUSY;

	evcnt = malloc(sizeof(*evcnt), M_DEVBUF, M_NOWAIT);
	if (evcnt == NULL)
		return ENOMEM;
	evcnt_attach_dynamic(evcnt, EVCNT_TYPE_INTR, NULL, name1, name2);

	iiv[vector].iiv_handler = handler;
	iiv[vector].iiv_arg = arg;
	iiv[vector].iiv_evcnt = evcnt;

	return 0;
}

int
intio_intr_disestablish(int vector, void *arg)
{
	if (iiv[vector].iiv_handler == 0 || iiv[vector].iiv_arg != arg)
		return EINVAL;
	iiv[vector].iiv_handler = 0;
	iiv[vector].iiv_arg = 0;
	evcnt_detach(iiv[vector].iiv_evcnt);
	free(iiv[vector].iiv_evcnt, M_DEVBUF);

	return 0;
}

int
intio_intr(struct frame *frame)
{
	int vector = frame->f_vector / 4;

	if (iiv[vector].iiv_handler == 0) {
		printf("Stray interrupt: %d type %x, pc %x\n",
			vector, frame->f_format, frame->f_pc);
		return 0;
	}

	iiv[vector].iiv_evcnt->ev_count++;

	return (*(iiv[vector].iiv_handler))(iiv[vector].iiv_arg);
}

/*
 * Intio I/O controller interrupt
 */
static u_int8_t intio_ivec = 0;

void
intio_set_ivec(int vec)
{
	vec &= 0xfc;

	if (intio_ivec && intio_ivec != (vec & 0xfc))
		panic("Wrong interrupt vector for Sicilian.");

	intio_ivec = vec;
	intio_set_sicilian_ivec(vec);
}


/*
 * intio bus DMA stuff.  stolen from arch/i386/isa/isa_machdep.c
 */

/*
 * Create an INTIO DMA map.
 */
int
_intio_bus_dmamap_create(bus_dma_tag_t t, bus_size_t size, int nsegments,
    bus_size_t maxsegsz, bus_size_t boundary, int flags, bus_dmamap_t *dmamp)
{
	struct intio_dma_cookie *cookie;
	bus_dmamap_t map;
	int error, cookieflags;
	size_t cookiesize;
	extern paddr_t avail_end;

	/* Call common function to create the basic map. */
	error = x68k_bus_dmamap_create(t, size, nsegments, maxsegsz, boundary,
	    flags, dmamp);
	if (error)
		return (error);

	map = *dmamp;
	map->x68k_dm_cookie = NULL;

	cookiesize = sizeof(struct intio_dma_cookie);

	/*
	 * INTIO only has 24-bits of address space.  This means
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
	 * DMAC), we may have to bounce it as well.
	 */
	if (avail_end <= t->_bounce_thresh)
		/* Bouncing not necessary due to memory size. */
		map->x68k_dm_bounce_thresh = 0;
	cookieflags = 0;
	if (map->x68k_dm_bounce_thresh != 0 ||
	    ((map->x68k_dm_size / PAGE_SIZE) + 1) > map->x68k_dm_segcnt) {
		cookieflags |= ID_MIGHT_NEED_BOUNCE;
		cookiesize += (sizeof(bus_dma_segment_t) * map->x68k_dm_segcnt);
	}

	/*
	 * Allocate our cookie.
	 */
	cookie = malloc(cookiesize, M_DMAMAP, 
	    ((flags & BUS_DMA_NOWAIT) ? M_NOWAIT : M_WAITOK) | M_ZERO);
	if (cookie == NULL) {
		error = ENOMEM;
		goto out;
	}
	cookie->id_flags = cookieflags;
	map->x68k_dm_cookie = cookie;

	if (cookieflags & ID_MIGHT_NEED_BOUNCE) {
		/*
		 * Allocate the bounce pages now if the caller
		 * wishes us to do so.
		 */
		if ((flags & BUS_DMA_ALLOCNOW) == 0)
			goto out;

		error = _intio_dma_alloc_bouncebuf(t, map, size, flags);
	}

 out:
	if (error) {
		if (map->x68k_dm_cookie != NULL)
			free(map->x68k_dm_cookie, M_DMAMAP);
		x68k_bus_dmamap_destroy(t, map);
	}
	return (error);
}

/*
 * Destroy an INTIO DMA map.
 */
void
_intio_bus_dmamap_destroy(bus_dma_tag_t t, bus_dmamap_t map)
{
	struct intio_dma_cookie *cookie = map->x68k_dm_cookie;

	/*
	 * Free any bounce pages this map might hold.
	 */
	if (cookie->id_flags & ID_HAS_BOUNCE)
		_intio_dma_free_bouncebuf(t, map);

	free(cookie, M_DMAMAP);
	x68k_bus_dmamap_destroy(t, map);
}

/*
 * Load an INTIO DMA map with a linear buffer.
 */
int
_intio_bus_dmamap_load(bus_dma_tag_t t, bus_dmamap_t map, void *buf,
    bus_size_t buflen, struct proc *p, int flags)
{
	struct intio_dma_cookie *cookie = map->x68k_dm_cookie;
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
	error = x68k_bus_dmamap_load(t, map, buf, buflen, p, flags);
	if (error == 0 ||
	    (error != 0 && (cookie->id_flags & ID_MIGHT_NEED_BOUNCE) == 0))
		return (error);

	/*
	 * Allocate bounce pages, if necessary.
	 */
	if ((cookie->id_flags & ID_HAS_BOUNCE) == 0) {
		error = _intio_dma_alloc_bouncebuf(t, map, buflen, flags);
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
	error = x68k_bus_dmamap_load(t, map, cookie->id_bouncebuf, buflen,
	    p, flags);
	if (error) {
		/*
		 * Free the bounce pages, unless our resources
		 * are reserved for our exclusive use.
		 */
		if ((map->x68k_dm_flags & BUS_DMA_ALLOCNOW) == 0)
			_intio_dma_free_bouncebuf(t, map);
		return (error);
	}

	/* ...so _intio_bus_dmamap_sync() knows we're bouncing */
	cookie->id_flags |= ID_IS_BOUNCING;
	return (0);
}

/*
 * Like _intio_bus_dmamap_load(), but for mbufs.
 */
int
_intio_bus_dmamap_load_mbuf(bus_dma_tag_t t, bus_dmamap_t map, struct mbuf *m0,
    int flags)
{
	struct intio_dma_cookie *cookie = map->x68k_dm_cookie;
	int error;

	/*
	 * Make sure on error condition we return "no valid mappings."
	 */
	map->dm_mapsize = 0;
	map->dm_nsegs = 0;

#ifdef DIAGNOSTIC
	if ((m0->m_flags & M_PKTHDR) == 0)
		panic("_intio_bus_dmamap_load_mbuf: no packet header");
#endif

	if (m0->m_pkthdr.len > map->x68k_dm_size)
		return (EINVAL);

	/*
	 * Try to load the map the normal way.  If this errors out,
	 * and we can bounce, we will.
	 */
	error = x68k_bus_dmamap_load_mbuf(t, map, m0, flags);
	if (error == 0 ||
	    (error != 0 && (cookie->id_flags & ID_MIGHT_NEED_BOUNCE) == 0))
		return (error);

	/*
	 * Allocate bounce pages, if necessary.
	 */
	if ((cookie->id_flags & ID_HAS_BOUNCE) == 0) {
		error = _intio_dma_alloc_bouncebuf(t, map, m0->m_pkthdr.len,
		    flags);
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
	error = x68k_bus_dmamap_load(t, map, cookie->id_bouncebuf,
	    m0->m_pkthdr.len, NULL, flags);
	if (error) {
		/*
		 * Free the bounce pages, unless our resources
		 * are reserved for our exclusive use.
		 */
		if ((map->x68k_dm_flags & BUS_DMA_ALLOCNOW) == 0)
			_intio_dma_free_bouncebuf(t, map);
		return (error);
	}

	/* ...so _intio_bus_dmamap_sync() knows we're bouncing */
	cookie->id_flags |= ID_IS_BOUNCING;
	return (0);
}

/*
 * Like _intio_bus_dmamap_load(), but for uios.
 */
int
_intio_bus_dmamap_load_uio(bus_dma_tag_t t, bus_dmamap_t map, struct uio *uio,
    int flags)
{
	panic("_intio_bus_dmamap_load_uio: not implemented");
}

/*
 * Like _intio_bus_dmamap_load(), but for raw memory allocated with
 * bus_dmamem_alloc().
 */
int
_intio_bus_dmamap_load_raw(bus_dma_tag_t t, bus_dmamap_t map,
    bus_dma_segment_t *segs, int nsegs, bus_size_t size, int flags)
{

	panic("_intio_bus_dmamap_load_raw: not implemented");
}

/*
 * Unload an INTIO DMA map.
 */
void
_intio_bus_dmamap_unload(bus_dma_tag_t t, bus_dmamap_t map)
{
	struct intio_dma_cookie *cookie = map->x68k_dm_cookie;

	/*
	 * If we have bounce pages, free them, unless they're
	 * reserved for our exclusive use.
	 */
	if ((cookie->id_flags & ID_HAS_BOUNCE) &&
	    (map->x68k_dm_flags & BUS_DMA_ALLOCNOW) == 0)
		_intio_dma_free_bouncebuf(t, map);

	cookie->id_flags &= ~ID_IS_BOUNCING;
	cookie->id_buftype = ID_BUFTYPE_INVALID;

	/*
	 * Do the generic bits of the unload.
	 */
	x68k_bus_dmamap_unload(t, map);
}

/*
 * Synchronize an INTIO DMA map.
 */
void
_intio_bus_dmamap_sync(bus_dma_tag_t t, bus_dmamap_t map, bus_addr_t offset,
    bus_size_t len, int ops)
{
	struct intio_dma_cookie *cookie = map->x68k_dm_cookie;

	/*
	 * Mixing PRE and POST operations is not allowed.
	 */
	if ((ops & (BUS_DMASYNC_PREREAD|BUS_DMASYNC_PREWRITE)) != 0 &&
	    (ops & (BUS_DMASYNC_POSTREAD|BUS_DMASYNC_POSTWRITE)) != 0)
		panic("_intio_bus_dmamap_sync: mix PRE and POST");

#ifdef DIAGNOSTIC
	if ((ops & (BUS_DMASYNC_PREWRITE|BUS_DMASYNC_POSTREAD)) != 0) {
		if (offset >= map->dm_mapsize)
			panic("_intio_bus_dmamap_sync: bad offset");
		if (len == 0 || (offset + len) > map->dm_mapsize)
			panic("_intio_bus_dmamap_sync: bad length");
	}
#endif

	/*
	 * If we're not bouncing, just return; nothing to do.
	 */
	if ((cookie->id_flags & ID_IS_BOUNCING) == 0)
		return;

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
		panic("_intio_bus_dmamap_sync: ID_BUFTYPE_UIO");
		break;

	case ID_BUFTYPE_RAW:
		panic("_intio_bus_dmamap_sync: ID_BUFTYPE_RAW");
		break;

	case ID_BUFTYPE_INVALID:
		panic("_intio_bus_dmamap_sync: ID_BUFTYPE_INVALID");
		break;

	default:
		printf("unknown buffer type %d\n", cookie->id_buftype);
		panic("_intio_bus_dmamap_sync");
	}
}

/*
 * Allocate memory safe for INTIO DMA.
 */
int
_intio_bus_dmamem_alloc(bus_dma_tag_t t, bus_size_t size, bus_size_t alignment,
    bus_size_t boundary, bus_dma_segment_t *segs, int nsegs, int *rsegs,
    int flags)
{
	paddr_t high;
	extern paddr_t avail_end;

	if (avail_end > INTIO_DMA_BOUNCE_THRESHOLD)
		high = trunc_page(INTIO_DMA_BOUNCE_THRESHOLD);
	else
		high = trunc_page(avail_end);

	return (x68k_bus_dmamem_alloc_range(t, size, alignment, boundary,
	    segs, nsegs, rsegs, flags, 0, high));
}

/**********************************************************************
 * INTIO DMA utility functions
 **********************************************************************/

int
_intio_dma_alloc_bouncebuf(bus_dma_tag_t t, bus_dmamap_t map, bus_size_t size,
    int flags)
{
	struct intio_dma_cookie *cookie = map->x68k_dm_cookie;
	int error = 0;

	cookie->id_bouncebuflen = round_page(size);
	error = _intio_bus_dmamem_alloc(t, cookie->id_bouncebuflen,
	    PAGE_SIZE, map->x68k_dm_boundary, cookie->id_bouncesegs,
	    map->x68k_dm_segcnt, &cookie->id_nbouncesegs, flags);
	if (error)
		goto out;
	error = x68k_bus_dmamem_map(t, cookie->id_bouncesegs,
	    cookie->id_nbouncesegs, cookie->id_bouncebuflen,
	    (void **)&cookie->id_bouncebuf, flags);

 out:
	if (error) {
		x68k_bus_dmamem_free(t, cookie->id_bouncesegs,
		    cookie->id_nbouncesegs);
		cookie->id_bouncebuflen = 0;
		cookie->id_nbouncesegs = 0;
	} else {
		cookie->id_flags |= ID_HAS_BOUNCE;
	}

	return (error);
}

void
_intio_dma_free_bouncebuf(bus_dma_tag_t t, bus_dmamap_t map)
{
	struct intio_dma_cookie *cookie = map->x68k_dm_cookie;

	x68k_bus_dmamem_unmap(t, cookie->id_bouncebuf,
	    cookie->id_bouncebuflen);
	x68k_bus_dmamem_free(t, cookie->id_bouncesegs,
	    cookie->id_nbouncesegs);
	cookie->id_bouncebuflen = 0;
	cookie->id_nbouncesegs = 0;
	cookie->id_flags &= ~ID_HAS_BOUNCE;
}
