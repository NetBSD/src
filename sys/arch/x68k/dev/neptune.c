/*	$NetBSD: neptune.c,v 1.20 2014/03/26 08:17:59 christos Exp $	*/

/*-
 * Copyright (c) 1998 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Minoura Makoto.
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
 * Neptune-X -- X68k-ISA Bus Bridge
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: neptune.c,v 1.20 2014/03/26 08:17:59 christos Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/malloc.h>

#include <machine/bus.h>
#include <machine/cpu.h>
#include <machine/frame.h>

#include <arch/x68k/dev/intiovar.h>
#include <arch/x68k/dev/neptunevar.h>

/* bus_space stuff */
static int neptune_bus_space_map(bus_space_tag_t, bus_addr_t, bus_size_t,
	int, bus_space_handle_t *);
static void neptune_bus_space_unmap(bus_space_tag_t, bus_space_handle_t,
	bus_size_t);
static int neptune_bus_space_subregion(bus_space_tag_t, bus_space_handle_t,
	bus_size_t, bus_size_t, bus_space_handle_t *);

static struct x68k_bus_space neptune_bus = {
#if 0
	X68K_NEPUTUNE_BUS,
#endif
	neptune_bus_space_map, neptune_bus_space_unmap,
	neptune_bus_space_subregion,
	x68k_bus_space_alloc, x68k_bus_space_free,
};


static int neptune_match(device_t, cfdata_t, void *);
static void neptune_attach(device_t, device_t, void *);
static int neptune_search(device_t, cfdata_t, const int *, void *);
static int neptune_print(void *, const char *);

CFATTACH_DECL_NEW(neptune, sizeof(struct neptune_softc),
    neptune_match, neptune_attach, NULL, NULL);

static int
neptune_match(device_t parent, cfdata_t cf, void *aux)
{
	struct intio_attach_args *ia = aux;

	if (strcmp(ia->ia_name, "neptune") != 0)
		return 0;

	ia->ia_size = 0x400;
	if (intio_map_allocate_region(parent, ia, INTIO_MAP_TESTONLY))
		return 0;

	/* Neptune is a virtual device.  Always there. */

	return (1);
}


static void
neptune_attach(device_t parent, device_t self, void *aux)
{
	struct neptune_softc *sc = device_private(self);
	struct intio_attach_args *ia = aux;
	struct neptune_attach_args na;
	int r __diagused;
	cfdata_t cf;

	ia->ia_size = 0x400;
	r = intio_map_allocate_region(parent, ia, INTIO_MAP_ALLOCATE);
#ifdef DIAGNOSTIC
	if (r)
		panic("IO map for Neptune corruption??");
#endif

	sc->sc_bst = malloc(sizeof(struct x68k_bus_space), M_DEVBUF, M_NOWAIT);
	if (sc->sc_bst == NULL)
		panic("neptune_attach: can't allocate bus_space structure");
	*sc->sc_bst = neptune_bus;
	sc->sc_bst->x68k_bus_device = self;

	sc->sc_addr = (vaddr_t)IIOV(ia->ia_addr);

	na.na_bst = sc->sc_bst;
	na.na_intr = ia->ia_intr;

	cf = config_search_ia(neptune_search, self, "neptune", &na);
	if (cf) {
		aprint_normal(": Neptune-X ISA bridge\n");
		config_attach(self, cf, &na, neptune_print);
	} else {
		aprint_normal(": no device found.\n");
		intio_map_free_region(parent, ia);
	}
}

static int
neptune_search(device_t parent, cfdata_t cf, const int *ldesc, void *aux)
{
	struct neptune_attach_args *na = aux;

	na->na_addr = cf->neptune_cf_addr;

	return config_match(parent, cf, na);
}

static int
neptune_print(void *aux, const char *name)
{
	struct neptune_attach_args *na = aux;

/*	if (na->na_addr > 0)	*/
		aprint_normal(" addr 0x%06x", na->na_addr);

	return (QUIET);
}


/*
 * neptune bus space stuff.
 */
static int
neptune_bus_space_map(bus_space_tag_t t, bus_addr_t bpa, bus_size_t size,
    int flags, bus_space_handle_t *bshp)
{
	struct neptune_softc *sc = device_private(t->x68k_bus_device);
	vaddr_t start = sc->sc_addr;

	/*
	 * Neptune bus is mapped permanently.
	 */
	*bshp = (bus_space_handle_t) ((u_int)start + ((u_int)bpa - 0x200) * 2);

	if (badaddr((void *)*bshp)) {
		return 1;
	}

	*bshp |= 0x80000000;	/* higher byte (= even address) only */

	return (0);
}

static void
neptune_bus_space_unmap(bus_space_tag_t t, bus_space_handle_t bsh,
    bus_size_t size)
{
	return;
}

static int
neptune_bus_space_subregion(bus_space_tag_t t, bus_space_handle_t bsh,
    bus_size_t offset, bus_size_t size, bus_space_handle_t *nbshp)
{

	*nbshp = bsh + offset*2;
	return (0);
}
