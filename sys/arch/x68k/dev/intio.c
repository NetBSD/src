/*	$NetBSD: intio.c,v 1.1.2.3 1999/01/30 15:07:40 minoura Exp $	*/

/*
 *
 * Copyright (c) 1998 NetBSD Foundation, Inc.
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
 *      This product includes software developed by Charles D. Cranor and
 *      Washington University.
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
 */

/*
 * NetBSD/x68k internal I/O virtual bus.
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/malloc.h>
#include <sys/mbuf.h>
#include <sys/extent.h>
#include <vm/vm.h>

#include <machine/bus.h>
#include <machine/cpu.h>
#include <machine/frame.h>

#include <arch/x68k/dev/intiovar.h>
#include <arch/x68k/dev/mfp.h>


/*
 * bus_space(9) interface
 */
static int intio_bus_space_map __P((bus_space_tag_t, bus_addr_t, bus_size_t, int, bus_space_handle_t *));
static void intio_bus_space_unmap __P((bus_space_tag_t, bus_space_handle_t, bus_size_t));
static int intio_bus_space_subregion __P((bus_space_tag_t, bus_space_handle_t, bus_size_t, bus_size_t, bus_space_handle_t *));

static struct x68k_bus_space intio_bus = {
#if 0
	X68K_INTIO_BUS,
#endif
	intio_bus_space_map, intio_bus_space_unmap, intio_bus_space_subregion,
	x68k_bus_space_alloc, x68k_bus_space_free,
#if 0
	x68k_bus_space_barrier,
#endif
	x68k_bus_space_read_1, x68k_bus_space_read_2, x68k_bus_space_read_4,
	x68k_bus_space_read_multi_1, x68k_bus_space_read_multi_2,
	x68k_bus_space_read_multi_4,
	x68k_bus_space_read_region_1, x68k_bus_space_read_region_2,
	x68k_bus_space_read_region_4,

	x68k_bus_space_write_1, x68k_bus_space_write_2, x68k_bus_space_write_4,
	x68k_bus_space_write_multi_1, x68k_bus_space_write_multi_2,
	x68k_bus_space_write_multi_4,
	x68k_bus_space_write_region_1, x68k_bus_space_write_region_2,
	x68k_bus_space_write_region_4,

	x68k_bus_space_set_region_1, x68k_bus_space_set_region_2,
	x68k_bus_space_set_region_4,
	x68k_bus_space_copy_region_1, x68k_bus_space_copy_region_2,
	x68k_bus_space_copy_region_4,

	0
};

/*
 * bus_dma(9) interface
 */
#define	INTIO_DMA_BOUNCE_THRESHOLD	(16 * 1024 * 1024)
int	_intio_bus_dmamap_create __P((bus_dma_tag_t, bus_size_t, int,
	    bus_size_t, bus_size_t, int, bus_dmamap_t *));
void	_intio_bus_dmamap_destroy __P((bus_dma_tag_t, bus_dmamap_t));
int	_intio_bus_dmamap_load __P((bus_dma_tag_t, bus_dmamap_t, void *,
	    bus_size_t, struct proc *, int));
int	_intio_bus_dmamap_load_mbuf __P((bus_dma_tag_t, bus_dmamap_t,
	    struct mbuf *, int));
int	_intio_bus_dmamap_load_uio __P((bus_dma_tag_t, bus_dmamap_t,
	    struct uio *, int));
int	_intio_bus_dmamap_load_raw __P((bus_dma_tag_t, bus_dmamap_t,
	    bus_dma_segment_t *, int, bus_size_t, int));
void	_intio_bus_dmamap_unload __P((bus_dma_tag_t, bus_dmamap_t));
void	_intio_bus_dmamap_sync __P((bus_dma_tag_t, bus_dmamap_t,
	    bus_addr_t, bus_size_t, int));

int	_intio_bus_dmamem_alloc __P((bus_dma_tag_t, bus_size_t, bus_size_t,
	    bus_size_t, bus_dma_segment_t *, int, int *, int));

int	_intio_dma_alloc_bouncebuf __P((bus_dma_tag_t, bus_dmamap_t,
	    bus_size_t, int));
void	_intio_dma_free_bouncebuf __P((bus_dma_tag_t, bus_dmamap_t));

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
static int intio_match __P((struct device *, struct cfdata *, void *));
static void intio_attach __P((struct device *, struct device *, void *));
static int intio_search __P((struct device *, struct cfdata *cf, void *));
static int intio_print __P((void *, const char *));
static void intio_alloc_system_ports __P((struct intio_softc*));

struct cfattach intio_ca = {
	sizeof(struct intio_softc), intio_match, intio_attach
};

static struct intio_interrupt_vector {
	intio_intr_handler_t	iiv_handler;
	void			*iiv_arg;
} iiv[256] = {0,};

extern struct cfdriver intio_cd;

/* used in console initialization */
extern int x68k_realconfig;
int x68k_config_found __P((struct cfdata *, struct device *,
			   void *, cfprint_t));
static struct cfdata *cfdata_intiobus = NULL;

static int
intio_match(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;		/* NULL */
{
	if (strcmp(aux, intio_cd.cd_name) != 0)
		return (0);
	if (cf->cf_unit != 0)
		return (0);
	if (x68k_realconfig == 0)
		cfdata_intiobus = cf; /* XXX */

	return (1);
}


/* used in console initialization: configure only MFP */
static struct intio_attach_args initial_ia = {
	&intio_bus,
	0/*XXX*/,

	"mfp",			/* ia_name */
	MFP_ADDR,		/* ia_addr */
	MFP_INTR,		/* ia_intr */
	-1,			/* ia_errintr */
	-1			/* ia_dma */
};

static void
intio_attach(parent, self, aux)
	struct device *parent, *self;
	void *aux;		/* NULL */
{
	struct intio_softc *sc = (struct intio_softc *)self;
	struct intio_attach_args ia;

	if (self == NULL) {
		/* console only init */
		x68k_config_found(cfdata_intiobus, NULL, &initial_ia, NULL);
		return;
	}

	printf (" mapped at %08p\n", intiobase);

	sc->sc_map = extent_create("intiomap",
				  PHYS_INTIODEV,
				  PHYS_INTIODEV + 0x400000,
				  M_DEVBUF, NULL, NULL, EX_NOWAIT);
	intio_alloc_system_ports (sc);

	sc->sc_bst = &intio_bus;
	sc->sc_bst->x68k_bus_device = self;
	sc->sc_dmat = &intio_bus_dma;
	sc->sc_dmac = 0;

	bzero(iiv, sizeof (struct intio_interrupt_vector) * 256);

	ia.ia_bst = sc->sc_bst;
	ia.ia_dmat = sc->sc_dmat;

	config_search (intio_search, self, &ia);
}

static int
intio_search(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	struct intio_attach_args *ia = aux;
	struct intio_softc *sc = (struct intio_softc *)parent;

	ia->ia_bst = sc->sc_bst;
	ia->ia_dmat = sc->sc_dmat;
	ia->ia_name = cf->cf_driver->cd_name;
	ia->ia_addr = cf->cf_addr;
	ia->ia_intr = cf->cf_intr;
	ia->ia_errintr = cf->cf_errintr;
	ia->ia_dma = cf->cf_dma;

	if ((*cf->cf_attach->ca_match)(parent, cf, ia) > 0)
		config_attach(parent, cf, ia, intio_print);

	return (0);
}

static int
intio_print(aux, name)
	void *aux;
	const char *name;
{
	struct intio_attach_args *ia = aux;

/*	if (ia->ia_addr > 0)	*/
		printf (" addr 0x%06x", ia->ia_addr);
	if (ia->ia_intr > 0) {
		printf (" interrupting at 0x%02x", ia->ia_intr);
		if (ia->ia_errintr > 0)
			printf (" and 0x%02x", ia->ia_errintr);
	}
	if (ia->ia_dma >= 0)
		printf (" using DMA ch%d", ia->ia_dma);

	return (QUIET);
}

/*
 * intio memory map manager
 */

int
intio_map_allocate_region(parent, ia, flag)
	struct device *parent;
	struct intio_attach_args *ia;
	enum intio_map_flag flag; /* INTIO_MAP_TESTONLY or INTIO_MAP_ALLOCATE */
{
	struct intio_softc *sc = (struct intio_softc*) parent;
	struct extent *map = sc->sc_map;
	int r;

	r = extent_alloc_region (map, ia->ia_addr, ia->ia_size, 0);
#ifdef DEBUG
	extent_print (map);
#endif
	if (r == 0) {
		if (flag != INTIO_MAP_ALLOCATE)
		extent_free (map, ia->ia_addr, ia->ia_size, 0);
		return 0;
	} 

	return -1;
}

int
intio_map_free_region(parent, ia)
	struct device *parent;
	struct intio_attach_args *ia;
{
	struct intio_softc *sc = (struct intio_softc*) parent;
	struct extent *map = sc->sc_map;

	extent_free (map, ia->ia_addr, ia->ia_size, 0);
#ifdef DEBUG
	extent_print (map);
#endif
	return 0;
}

void
intio_alloc_system_ports(sc)
	struct intio_softc *sc;
{
	extent_alloc_region (sc->sc_map, INTIO_SYSPORT, 16, 0);
}


/*
 * intio bus space stuff.
 */
static int
intio_bus_space_map(t, bpa, size, flags, bshp)
	bus_space_tag_t t;
	bus_addr_t bpa;
	bus_size_t size;
	int flags;
	bus_space_handle_t *bshp;
{
	/*
	 * Intio bus is mapped permanently.
	 */
	*bshp = (bus_space_handle_t)
	  ((u_int) bpa - PHYS_INTIODEV + intiobase);

	return (0);
}

static void
intio_bus_space_unmap(t, bsh, size)
	bus_space_tag_t t;
	bus_space_handle_t bsh;
	bus_size_t size;
{
	return;
}

static int
intio_bus_space_subregion(t, bsh, offset, size, nbshp)
	bus_space_tag_t t;
	bus_space_handle_t bsh;
	bus_size_t offset, size;
	bus_space_handle_t *nbshp;
{

	*nbshp = bsh + offset;
	return (0);
}


/*
 * interrupt handler
 */
int
intio_intr_establish (vector, name, handler, arg)
	int vector;
	const char *name;	/* XXX */
	intio_intr_handler_t handler;
	void *arg;
{
	if (vector < 16)
		panic ("Invalid interrupt vector");
	if (iiv[vector].iiv_handler)
		return EBUSY;
	iiv[vector].iiv_handler = handler;
	iiv[vector].iiv_arg = arg;

	return 0;
}

int
intio_intr_disestablish (vector, arg)
	int vector;
	void *arg;
{
	if (iiv[vector].iiv_handler == 0 || iiv[vector].iiv_arg != arg)
		return EINVAL;
	iiv[vector].iiv_handler = 0;
	iiv[vector].iiv_arg = 0;

	return 0;
}

int
intio_intr (frame)
	struct frame *frame;
{
	int vector = frame->f_vector / 4;

	/* CAUTION: HERE WE ARE IN SPLHIGH() */
	/* LOWER TO APPROPRIATE IPL AT VERY FIRST IN THE HANDLER!! */

	if (iiv[vector].iiv_handler == 0) {
		printf ("Stray interrupt: %d type %x\n", vector, frame->f_format);
		return 0;
	}

	/* TODO: update the interrupt statics. */

	return (*(iiv[vector].iiv_handler)) (iiv[vector].iiv_arg);
}



/*
 * intio bus dma stuff.  stolen from arch/i386/isa/isa_machdep.c
 */

/*
 * Create an INTIO DMA map.
 */
int
_intio_bus_dmamap_create(t, size, nsegments, maxsegsz, boundary, flags, dmamp)
	bus_dma_tag_t t;
	bus_size_t size;
	int nsegments;
	bus_size_t maxsegsz;
	bus_size_t boundary;
	int flags;
	bus_dmamap_t *dmamp;
{
	struct intio_dma_cookie *cookie;
	bus_dmamap_t map;
	int error, cookieflags;
	void *cookiestore;
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
	 * a transfer will require is (maxxfer / NBPG) + 1.  If
	 * the caller can't handle that many segments (e.g. the
	 * DMAC), we may have to bounce it as well.
	 */
	if (avail_end <= t->_bounce_thresh)
		/* Bouncing not necessary due to memory size. */ 
		map->x68k_dm_bounce_thresh = 0;
	cookieflags = 0;
	if (map->x68k_dm_bounce_thresh != 0 ||
	    ((map->x68k_dm_size / NBPG) + 1) > map->x68k_dm_segcnt) {
		cookieflags |= ID_MIGHT_NEED_BOUNCE;
		cookiesize += (sizeof(bus_dma_segment_t) * map->x68k_dm_segcnt);
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
	cookie = (struct intio_dma_cookie *)cookiestore;
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
_intio_bus_dmamap_destroy(t, map)
	bus_dma_tag_t t;
	bus_dmamap_t map;
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
_intio_bus_dmamap_load(t, map, buf, buflen, p, flags)
	bus_dma_tag_t t;
	bus_dmamap_t map; 
	void *buf;
	bus_size_t buflen;
	struct proc *p;
	int flags;
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
_intio_bus_dmamap_load_mbuf(t, map, m0, flags)  
	bus_dma_tag_t t;
	bus_dmamap_t map;
	struct mbuf *m0;
	int flags;
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
_intio_bus_dmamap_load_uio(t, map, uio, flags)
	bus_dma_tag_t t;
	bus_dmamap_t map;
	struct uio *uio;
	int flags;
{
	panic("_intio_bus_dmamap_load_uio: not implemented");
}

/*
 * Like _intio_bus_dmamap_load(), but for raw memory allocated with
 * bus_dmamem_alloc().
 */
int
_intio_bus_dmamap_load_raw(t, map, segs, nsegs, size, flags)
	bus_dma_tag_t t;
	bus_dmamap_t map;
	bus_dma_segment_t *segs;
	int nsegs;
	bus_size_t size;
	int flags;
{

	panic("_intio_bus_dmamap_load_raw: not implemented");
}

/*
 * Unload an INTIO DMA map.
 */
void
_intio_bus_dmamap_unload(t, map)
	bus_dma_tag_t t;
	bus_dmamap_t map;
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
_intio_bus_dmamap_sync(t, map, offset, len, ops)
	bus_dma_tag_t t;
	bus_dmamap_t map;
	bus_addr_t offset;
	bus_size_t len;
	int ops;
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
_intio_bus_dmamem_alloc(t, size, alignment, boundary, segs, nsegs, rsegs, flags)
	bus_dma_tag_t t;
	bus_size_t size, alignment, boundary;
	bus_dma_segment_t *segs;
	int nsegs;
	int *rsegs;
	int flags;
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
_intio_dma_alloc_bouncebuf(t, map, size, flags)
	bus_dma_tag_t t;
	bus_dmamap_t map;
	bus_size_t size;
	int flags;
{
	struct intio_dma_cookie *cookie = map->x68k_dm_cookie;
	int error = 0;

	cookie->id_bouncebuflen = round_page(size);
	error = _intio_bus_dmamem_alloc(t, cookie->id_bouncebuflen,
	    NBPG, map->x68k_dm_boundary, cookie->id_bouncesegs,
	    map->x68k_dm_segcnt, &cookie->id_nbouncesegs, flags);
	if (error)
		goto out;
	error = x68k_bus_dmamem_map(t, cookie->id_bouncesegs,
	    cookie->id_nbouncesegs, cookie->id_bouncebuflen,
	    (caddr_t *)&cookie->id_bouncebuf, flags);

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
_intio_dma_free_bouncebuf(t, map)
	bus_dma_tag_t t;
	bus_dmamap_t map;
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
