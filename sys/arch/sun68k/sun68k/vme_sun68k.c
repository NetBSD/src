/*	$NetBSD: vme_sun68k.c,v 1.15 2009/11/27 03:23:14 rmind Exp $	*/

/*-
 * Copyright (c) 1997, 1998, 2001 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Paul Kranenburg and Matthew Fredette.
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
__KERNEL_RCSID(0, "$NetBSD: vme_sun68k.c,v 1.15 2009/11/27 03:23:14 rmind Exp $");

#include <sys/param.h>
#include <sys/extent.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/malloc.h>
#include <sys/errno.h>

#include <sys/proc.h>
#include <sys/syslog.h>

#include <uvm/uvm_extern.h>

#define _SUN68K_BUS_DMA_PRIVATE
#include <machine/bus.h>
#include <machine/autoconf.h>
#include <machine/pmap.h>
#include <machine/dvma.h>

#include <dev/vme/vmereg.h>
#include <dev/vme/vmevar.h>

#include <sun68k/sun68k/vme_sun68k.h>

struct sun68kvme_softc {
	device_t	 sc_dev;	/* base device */
	bus_space_tag_t	 sc_bustag;
	bus_dma_tag_t	 sc_dmatag;
};
struct  sun68kvme_softc *sun68kvme_sc;/*XXX*/

/* autoconfiguration driver */
static int	sun68kvme_match(device_t, cfdata_t, void *);
static void	sun68kvme_attach(device_t, device_t, void *);

static int	sun68k_vme_probe(void *, vme_addr_t, vme_size_t, vme_am_t,
	vme_datasize_t,
	int (*)(void *, bus_space_tag_t, bus_space_handle_t), void *);
static int	sun68k_vme_map(void *, vme_addr_t, vme_size_t, vme_am_t,
	vme_datasize_t, vme_swap_t, bus_space_tag_t *, bus_space_handle_t *,
	vme_mapresc_t *);
static void	sun68k_vme_unmap(void *, vme_mapresc_t);
static int	sun68k_vme_intr_map(void *, int, int, vme_intr_handle_t *);
static const struct evcnt *sun68k_vme_intr_evcnt(void *, vme_intr_handle_t);
static void *	sun68k_vme_intr_establish(void *, vme_intr_handle_t, int,
	int (*)(void *), void *);
static void	sun68k_vme_intr_disestablish(void *, void *);

/*
 * DMA functions.
 */
static void	sun68k_vct_dmamap_destroy(void *, bus_dmamap_t);

static int	sun68k_vct_dmamap_create(void *, vme_size_t, vme_am_t,
		    vme_datasize_t, vme_swap_t, int, vme_size_t, vme_addr_t,
		    int, bus_dmamap_t *);
static int	sun68k_vme_dmamap_load(bus_dma_tag_t, bus_dmamap_t, void *,
		    bus_size_t, struct proc *, int);
static int	sun68k_vme_dmamap_load_raw(bus_dma_tag_t, bus_dmamap_t,
		    bus_dma_segment_t *, int, bus_size_t, int);

paddr_t sun68k_vme_mmap_cookie(vme_addr_t, vme_am_t, bus_space_handle_t *);

CFATTACH_DECL_NEW(sun68kvme, sizeof(struct sun68kvme_softc),
    sun68kvme_match, sun68kvme_attach, NULL, NULL);

static int sun68kvme_attached;

/*
 * The VME bus logic on sun68k machines maps DMA requests in the first MB
 * of VME space to the last MB of DVMA space.  The base bus_dma code
 * in machdep.c manages DVMA space; all we must do is adjust the DMA
 * addresses returned by bus_dmamap_load*() by ANDing them with 
 * DVMA_VME_SLAVE_MASK.
 */

struct vme_chipset_tag sun68k_vme_chipset_tag = {
	NULL,
	sun68k_vme_map,
	sun68k_vme_unmap,
	sun68k_vme_probe,
	sun68k_vme_intr_map,
	sun68k_vme_intr_evcnt,
	sun68k_vme_intr_establish,
	sun68k_vme_intr_disestablish,
	sun68k_vct_dmamap_create,
	sun68k_vct_dmamap_destroy
};

struct sun68k_bus_dma_tag sun68k_vme_dma_tag;

/* Does this machine have a VME bus? */
extern int cpu_has_vme;

/*
 * Probe the VME bus.
 */
int 
sun68kvme_match(device_t parent, cfdata_t cf, void *aux)
{
        struct mainbus_attach_args *ma = aux;

	if (sun68kvme_attached)
		return 0;

        return cpu_has_vme &&
	    (ma->ma_name == NULL || strcmp(cf->cf_name, ma->ma_name) == 0);
}

/*
 * Attach the VME bus.
 */
void 
sun68kvme_attach(device_t parent, device_t self, void *aux)
{
	struct mainbus_attach_args *ma = aux;
	struct sun68kvme_softc *sc = device_private(self);
	struct vmebus_attach_args vba;

	sun68kvme_attached = 1;

	sun68kvme_sc = sc;

	sc->sc_dev = self;
	sc->sc_bustag = ma->ma_bustag;
	sc->sc_dmatag = ma->ma_dmatag;

	sun68k_vme_chipset_tag.cookie = sc;
	sun68k_vme_dma_tag = *ma->ma_dmatag;
	sun68k_vme_dma_tag._cookie = sc;
	sun68k_vme_dma_tag._dmamap_load = sun68k_vme_dmamap_load;
	sun68k_vme_dma_tag._dmamap_load_raw = sun68k_vme_dmamap_load_raw;

	vba.va_vct = &sun68k_vme_chipset_tag;
	vba.va_bdt = &sun68k_vme_dma_tag;
	vba.va_slaveconfig = 0;

	aprint_normal("\n");
	(void)config_found(self, &vba, 0);
}

/*
 * Probes for a device on the VME bus.
 * Returns zero on success.
 */
int
sun68k_vme_probe(void *cookie, vme_addr_t addr, vme_size_t len, vme_am_t mod,
    vme_datasize_t datasize,
    int (*callback)(void *, bus_space_tag_t, bus_space_handle_t), void *arg)
{
	struct sun68kvme_softc *sc = cookie;
	bus_type_t iospace;
	bus_addr_t paddr;
	bus_space_handle_t handle;
	bus_size_t size;
	bus_size_t off, max_off;
	int error;

	/* Map in the space. */
	error = vmebus_translate(mod, addr, &iospace, &paddr);
	if (error == 0)
		error = bus_space_map2(sc->sc_bustag, iospace, paddr, len, 
		    0, 0, &handle);
	if (error)
		return error;

	/* Probe the space. */
	size = (datasize == VME_D8 ? 1 : (datasize == VME_D16 ? 2 : 4));
	max_off = (callback ? size : len);
	for (off = 0; off < max_off; off += size) {
		error = _bus_space_peek(sc->sc_bustag, handle, off, size, NULL);
		if (error)
			break;
	}
	if (error == 0 && callback)
		error = (*callback)(arg, sc->sc_bustag, handle);

	/* Unmap the space. */
	bus_space_unmap(sc->sc_bustag, handle, len);

	return error;
}

/*
 * Maps in a device on the VME bus.
 */
int
sun68k_vme_map(void *cookie, vme_addr_t addr, vme_size_t size, vme_am_t mod,
    vme_datasize_t datasize, vme_swap_t swap, bus_space_tag_t *tp,
    bus_space_handle_t *hp, vme_mapresc_t *rp)
{
	struct sun68kvme_softc *sc = cookie;
	bus_type_t iospace;
	bus_addr_t paddr;
	int error;

	error = vmebus_translate(mod, addr, &iospace, &paddr);
	if (error != 0)
		return error;

	*tp = sc->sc_bustag;
	return bus_space_map2(sc->sc_bustag, iospace, paddr, size, 0, 0, hp);
}

/*
 * Assists in mmap'ing a device on the VME bus.
 */
paddr_t
sun68k_vme_mmap_cookie(vme_addr_t addr, vme_am_t mod, bus_space_handle_t *hp)
{
	struct sun68kvme_softc *sc = sun68kvme_sc;
	bus_type_t iospace;
	bus_addr_t paddr;
	int error;

	error = vmebus_translate(mod, addr, &iospace, &paddr);
	if (error != 0)
		return error;

	return bus_space_mmap2(sc->sc_bustag, iospace, paddr, 0, 0, 0);
}

struct sun68k_vme_intr_handle {
	int	vec;		/* VME interrupt vector */
	int	pri;		/* VME interrupt priority */
};

/*
 * This maps a VME interrupt level and vector pair into
 * a data structure that can subsequently be used to 
 * establish an interrupt handler.
 */
int
sun68k_vme_intr_map(void *cookie, int level, int vec, vme_intr_handle_t *ihp)
{
	struct sun68k_vme_intr_handle *svih;

	svih = malloc(sizeof(struct sun68k_vme_intr_handle),
	    M_DEVBUF, M_NOWAIT);
	svih->pri = level;
	svih->vec = vec;
	*ihp = svih;
	return 0;
}

const struct evcnt *
sun68k_vme_intr_evcnt(void *cookie, vme_intr_handle_t vih)
{

	/* XXX for now, no evcnt parent reported */
	return NULL;
}

/*
 * Establish a VME bus interrupt.
 */
void *
sun68k_vme_intr_establish(void *cookie, vme_intr_handle_t vih, int pri,
    int (*func)(void *), void *arg)
{
	struct sun68k_vme_intr_handle *svih =
	    (struct sun68k_vme_intr_handle *)vih;

	/* Install interrupt handler. */
	isr_add_vectored(func, arg, svih->pri, svih->vec);

	return NULL;
}

void
sun68k_vme_unmap(void *cookie, vme_mapresc_t resc)
{

	/* Not implemented */
	panic("%s: not implemented", __func__);
}

void 
sun68k_vme_intr_disestablish(void *cookie, void *a)
{

	/* Not implemented */
	panic("%s: not implemented", __func__);
}

/*
 * VME DMA functions.
 */

static void 
sun68k_vct_dmamap_destroy(void *cookie, bus_dmamap_t map)
{
	struct sun68kvme_softc *sc = cookie;

	bus_dmamap_destroy(sc->sc_dmatag, map);
}

static int
sun68k_vct_dmamap_create(void *cookie, vme_size_t size, vme_am_t am,
    vme_datasize_t datasize, vme_swap_t swap, int nsegments,
    vme_size_t maxsegsz, vme_addr_t boundary, int flags, bus_dmamap_t *dmamp)
{
	struct sun68kvme_softc *sc = cookie;

	/* Allocate a base map through parent bus ops */
	return bus_dmamap_create(sc->sc_dmatag, size, nsegments, maxsegsz,
	    boundary, flags, dmamp);
}

int 
sun68k_vme_dmamap_load(bus_dma_tag_t t, bus_dmamap_t map, void *buf,
    bus_size_t buflen, struct proc *p, int flags)
{
	int error;

	error = _bus_dmamap_load(t, map, buf, buflen, p, flags);
	if (error == 0)
		map->dm_segs[0].ds_addr &= DVMA_VME_SLAVE_MASK;
	return error;
}

int 
sun68k_vme_dmamap_load_raw(bus_dma_tag_t t, bus_dmamap_t map,
    bus_dma_segment_t *segs, int nsegs, bus_size_t size, int flags)
{  
	int error;

	error = _bus_dmamap_load_raw(t, map, segs, nsegs, size, flags);
	if (error == 0)  
		map->dm_segs[0].ds_addr &= DVMA_VME_SLAVE_MASK;
	return error;
}
