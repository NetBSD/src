/*	$NetBSD: vme.c,v 1.21.40.1 2014/08/20 00:03:26 tls Exp $	*/

/*-
 * Copyright (c) 1996 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Gordon W. Ross.
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
__KERNEL_RCSID(0, "$NetBSD: vme.c,v 1.21.40.1 2014/08/20 00:03:26 tls Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <uvm/uvm_extern.h>

#define _SUN68K_BUS_DMA_PRIVATE
#include <machine/autoconf.h>
#include <machine/bus.h>
#include <machine/dvma.h>
#include <machine/pmap.h>

#include <sun3/sun3/vme.h>

/* Does this machine have a VME bus? */
extern int cpu_has_vme;

/*
 * Convert vme unit number to bus type,
 * but only for supported bus types.
 * (See autoconf.h and vme.h)
 */
#define VME_UNITS	6
static const struct {
	int bustype;
	const char *name;
	int pmtype;
	vaddr_t base;
	vaddr_t mask;
} vme_info[VME_UNITS] = {
	{ BUS_VME16D16, "A16/D16", PMAP_VME16, VME16_BASE, VME16_MASK },
	{ BUS_VME16D32, "A16/D32", PMAP_VME32, VME16_BASE, VME16_MASK },
	{ BUS_VME24D16, "A24/D16", PMAP_VME16, VME24_BASE, VME24_MASK },
	{ BUS_VME24D32, "A24/D32", PMAP_VME32, VME24_BASE, VME24_MASK },
	{ BUS_VME32D16, "A32/D16", PMAP_VME16, VME32_BASE, VME32_MASK },
	{ BUS_VME32D32, "A32/D32", PMAP_VME32, VME32_BASE, VME32_MASK },
};

static int  vme_match(device_t , cfdata_t, void *);
static void vme_attach(device_t , device_t, void *);

struct vme_softc {
	device_t	sc_dev;
	bus_space_tag_t	sc_bustag;
	bus_dma_tag_t	sc_dmatag;
	int		sc_bustype;
};

CFATTACH_DECL_NEW(vme, sizeof(struct vme_softc),
    vme_match, vme_attach, NULL, NULL);

static int vme_bus_map(bus_space_tag_t, bus_type_t, bus_addr_t, bus_size_t,
    int, vaddr_t, bus_space_handle_t *);
static paddr_t vme_bus_mmap(bus_space_tag_t, bus_type_t, bus_addr_t,
    off_t, int, int);
static int vme_dmamap_load(bus_dma_tag_t, bus_dmamap_t, void *, bus_size_t,
    struct proc *, int);

static struct sun68k_bus_space_tag vme_space_tag = {
	NULL,				/* cookie */
	NULL,				/* parent bus space tag */
	vme_bus_map,			/* bus_space_map */
	NULL,				/* bus_space_unmap */
	NULL,				/* bus_space_subregion */
	NULL,				/* bus_space_barrier */
	vme_bus_mmap,			/* bus_space_mmap */
	NULL,				/* bus_intr_establish */
	NULL,				/* bus_space_peek_N */
	NULL				/* bus_space_poke_N */
};

static struct sun68k_bus_dma_tag vme_dma_tag;

static int
vme_match(device_t parent, cfdata_t cf, void *aux)
{
	struct confargs *ca = aux;
	int unit;

	if (cpu_has_vme == 0)
		return 0;

	unit = cf->cf_unit;
	if (unit >= VME_UNITS)
		return 0;

	if (ca->ca_bustype != vme_info[unit].bustype)
		return 0;

	return 1;
}

static void
vme_attach(device_t parent, device_t self, void *aux)
{
	struct confargs *ca = aux;
	struct vme_softc *sc = device_private(self);
	struct confargs vmea;
	int unit;

	sc->sc_dev = self;

	unit = device_unit(self);
	aprint_normal(": (%s)\n", vme_info[unit].name);

	sc->sc_bustag = ca->ca_bustag;
	sc->sc_dmatag = ca->ca_dmatag;
	sc->sc_bustype = unit;

	vme_space_tag.cookie = sc;
	vme_space_tag.parent = sc->sc_bustag;

	vme_dma_tag = *sc->sc_dmatag;
	vme_dma_tag._cookie = sc;
	vme_dma_tag._dmamap_load = vme_dmamap_load;

	vmea = *ca;
	vmea.ca_bustag = &vme_space_tag;
	vmea.ca_dmatag = &vme_dma_tag;

	/* We know ca_bustype == BUS_VMExx */
	config_search_ia(bus_scan, self, "vme", &vmea);
}

int
vme_bus_map(bus_space_tag_t t, bus_type_t btype, bus_addr_t paddr,
    bus_size_t size, int flags, vaddr_t vaddr, bus_space_handle_t *hp)
{
	struct vme_softc *sc = t->cookie;
	paddr_t pa;
	int bustype, pmtype;

	bustype = sc->sc_bustype;
	pa = paddr;
	pa &= vme_info[bustype].mask;
	pa |= vme_info[bustype].base;
	pmtype = vme_info[bustype].pmtype;

	return bus_space_map2(sc->sc_bustag, pmtype, pa, size,
	    flags | _SUN68K_BUS_MAP_USE_PROM, vaddr, hp);
}

paddr_t
vme_bus_mmap(bus_space_tag_t t, bus_type_t btype, bus_addr_t paddr, off_t off,
    int prot, int flags)
{
	struct vme_softc *sc = t->cookie;
	paddr_t pa;
	int bustype, pmtype;

	bustype = sc->sc_bustype;
	pa = paddr;
	pa &= vme_info[bustype].mask;
	pa |= vme_info[bustype].base;
	pmtype = vme_info[bustype].pmtype;

	return bus_space_mmap2(sc->sc_bustag, pmtype, pa, off, prot, flags);
}

static int
vme_dmamap_load(bus_dma_tag_t t, bus_dmamap_t map, void *buf,
    bus_size_t buflen, struct proc *p, int flags)
{
	int error;

	error = _bus_dmamap_load(t, map, buf, buflen, p, flags);
	if (error == 0)
		map->dm_segs[0].ds_addr &= DVMA_VME_SLAVE_MASK;
	return error;
}
