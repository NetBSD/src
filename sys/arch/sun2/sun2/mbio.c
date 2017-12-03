/*	$NetBSD: mbio.c,v 1.18.40.1 2017/12/03 11:36:45 jdolecek Exp $	*/

/*-
 * Copyright (c) 1996 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Adam Glass, Gordon W. Ross, and Matthew Fredette.
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
__KERNEL_RCSID(0, "$NetBSD: mbio.c,v 1.18.40.1 2017/12/03 11:36:45 jdolecek Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <uvm/uvm_extern.h>

#include <machine/autoconf.h>
#include <machine/pmap.h>

#include <sun2/sun2/control.h>
#include <sun2/sun2/machdep.h>
#include <sun2/sun2/mbio.h>

/* Does this machine have a Multibus? */
extern int cpu_has_multibus;
 
static int  mbio_match(device_t, cfdata_t, void *);
static void mbio_attach(device_t, device_t, void *);

struct mbio_softc {
	device_t	sc_dev;		/* base device */
	bus_space_tag_t	sc_bustag;	/* parent bus tag */
	bus_dma_tag_t	sc_dmatag;	/* parent bus dma tag */
};

CFATTACH_DECL_NEW(mbio, sizeof(struct mbio_softc),
    mbio_match, mbio_attach, NULL, NULL);

static int mbio_attached;

static	paddr_t mbio_bus_mmap(bus_space_tag_t, bus_type_t, bus_addr_t, off_t,
	    int, int);
static	int _mbio_bus_map(bus_space_tag_t, bus_type_t, bus_addr_t, bus_size_t,
	    int, vaddr_t, bus_space_handle_t *);

static struct sun68k_bus_space_tag mbio_space_tag = {
	NULL,				/* cookie */
	NULL,				/* parent bus tag */
	_mbio_bus_map,			/* bus_space_map */ 
	NULL,				/* bus_space_unmap */
	NULL,				/* bus_space_subregion */
	NULL,				/* bus_space_barrier */ 
	mbio_bus_mmap,			/* bus_space_mmap */ 
	NULL,				/* bus_intr_establish */
	NULL,				/* bus_space_peek_N */
	NULL				/* bus_space_poke_N */
};

static int 
mbio_match(device_t parent, cfdata_t cf, void *aux)
{
	struct mainbus_attach_args *ma = aux;

	if (mbio_attached)
		return 0;

	return (cpu_has_multibus &&
	    (ma->ma_name == NULL || strcmp(cf->cf_name, ma->ma_name) == 0));
}

static void 
mbio_attach(device_t parent, device_t self, void *aux)
{
	struct mainbus_attach_args *ma = aux;
	struct mbio_softc *sc = device_private(self);
	struct mbio_attach_args mba;
	const char *const *cpp;
	static const char *const special[] = {
		/* find these first */
		NULL
	};

	mbio_attached = 1;

	sc->sc_dev = self;
	aprint_normal("\n");

	sc->sc_bustag = ma->ma_bustag;
	sc->sc_dmatag = ma->ma_dmatag;

	mbio_space_tag.cookie = sc;
	mbio_space_tag.parent = sc->sc_bustag;

	/*
	 * Prepare the skeleton attach arguments for our devices.
	 * The values we give in the locators are indications to
	 * sun68k_bus_search about which locators must and must not
	 * be defined.
	 */
	mba = *ma;
	mba.mba_bustag = &mbio_space_tag;
	mba.mba_paddr = LOCATOR_REQUIRED;
	mba.mba_pri = LOCATOR_OPTIONAL;

	/* Find all `early' mbio devices */
	for (cpp = special; *cpp != NULL; cpp++) {
		mba.mba_name = *cpp;
		config_search_ia(sun68k_bus_search, self, "mbio", &mba);
	}

	/* Find all other mbio devices */
	mba.mba_name = NULL;
	config_search_ia(sun68k_bus_search, self, "mbio", &mba);
}

int
_mbio_bus_map(bus_space_tag_t t, bus_type_t btype, bus_addr_t paddr,
    bus_size_t size, int flags, vaddr_t vaddr, bus_space_handle_t *hp)
{
	struct mbio_softc *sc = t->cookie;

	if ((paddr + size) > MBIO_SIZE)
		panic("%s: out of range", __func__);

	return bus_space_map2(sc->sc_bustag, PMAP_MBIO, paddr,
	    size, flags | _SUN68K_BUS_MAP_USE_PROM, vaddr, hp);
}

paddr_t
mbio_bus_mmap(bus_space_tag_t t, bus_type_t btype, bus_addr_t paddr, off_t off,
    int prot, int flags)
{
	struct mbio_softc *sc = t->cookie;

	return bus_space_mmap2(sc->sc_bustag, PMAP_MBIO, paddr, off,
	    prot, flags);
}
