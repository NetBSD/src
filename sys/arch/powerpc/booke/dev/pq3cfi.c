/*	$NetBSD: pq3cfi.c,v 1.4.2.1 2012/10/30 17:20:10 yamt Exp $	*/
/*-
 * Copyright (c) 2011 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Cliff Neighbors.
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
 * NOR CFI driver support for booke
 */

#include "opt_flash.h"
#include "locators.h"

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: pq3cfi.c,v 1.4.2.1 2012/10/30 17:20:10 yamt Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/cdefs.h>
#include <sys/device.h>
#include <sys/endian.h>

#include <sys/bus.h>

#include <powerpc/booke/cpuvar.h>

#include <dev/nor/nor.h>
#include <dev/nor/cfi.h>


static int  pq3cfi_match(device_t, cfdata_t, void *);
static void pq3cfi_attach(device_t, device_t, void *);
static int  pq3cfi_detach(device_t, int);

struct pq3cfi_softc {
	device_t		sc_dev;
	device_t		sc_nordev;
	struct cfi			sc_cfi;
	bus_addr_t		sc_addr;
	bus_size_t		sc_size;
	struct nor_interface	sc_nor_if;
};

CFATTACH_DECL_NEW(pq3cfi, sizeof(struct pq3cfi_softc), pq3cfi_match,
    pq3cfi_attach, pq3cfi_detach, NULL);

static int
pq3cfi_match(device_t parent, cfdata_t match, void *aux)
{
	struct generic_attach_args *ga = aux;
	bus_size_t tmpsize = CFI_QRY_MIN_MAP_SIZE;
	bus_addr_t addr;
	struct cfi cfi;
	int rv;

	KASSERT(ga->ga_bst != NULL);

	addr = ga->ga_addr;
	if (addr == OBIOCF_ADDR_DEFAULT) {
		aprint_error("%s: no base address\n", __func__);
		return 0;
	}

	cfi.cfi_bst = ga->ga_bst;
	int error = bus_space_map(cfi.cfi_bst, addr, tmpsize, 0, &cfi.cfi_bsh);
	if (error != 0) {
		aprint_error("%s: cannot map %d at offset %#x, error %d\n",				__func__, tmpsize, addr, error);
		return false;
	}

	if (! cfi_probe(&cfi)) {
		aprint_debug("%s: probe addr %#x, CFI not found\n",
			__func__, addr);
		rv = 0;
	} else {
		rv = 1;
	}

	bus_space_unmap(cfi.cfi_bst, cfi.cfi_bsh, tmpsize);

	return rv;
}

static void
pq3cfi_attach(device_t parent, device_t self, void *aux)
{
	struct pq3cfi_softc *sc = device_private(self);
	struct generic_attach_args *ga = aux;
	struct cfi_query_data * const qryp = &sc->sc_cfi.cfi_qry_data;
	const bus_size_t tmpsize = CFI_QRY_MIN_MAP_SIZE;
	bool found;
	int error;

	aprint_normal("\n");

	sc->sc_dev = self;
	sc->sc_cfi.cfi_bst = ga->ga_bst;
	sc->sc_addr = ga->ga_addr;

	/* map enough to identify, remap later when size is known */
	error = bus_space_map(sc->sc_cfi.cfi_bst, sc->sc_addr, tmpsize,
		0, &sc->sc_cfi.cfi_bsh);
	if (error != 0) {
		aprint_error_dev(self, "could not map error %d\n", error);
		return;
	}

	found = cfi_identify(&sc->sc_cfi);

	bus_space_unmap(sc->sc_cfi.cfi_bst, sc->sc_cfi.cfi_bsh, tmpsize);

	if (! found) {
		/* should not happen, we already probed OK in match */
		aprint_error_dev(self, "could not map error %d\n", error);
		return;
	}

	sc->sc_size = 1 << qryp->device_size;

	sc->sc_nor_if = nor_interface_cfi;
	sc->sc_nor_if.private = &sc->sc_cfi;
	sc->sc_nor_if.access_width = (1 << sc->sc_cfi.cfi_portwidth);

	cfi_print(self, &sc->sc_cfi);

	error = bus_space_map(sc->sc_cfi.cfi_bst, sc->sc_addr, sc->sc_size,
		BUS_SPACE_MAP_PREFETCHABLE, &sc->sc_cfi.cfi_bsh);
	if (error != 0) {
		aprint_error_dev(self, "could not map error %d\n", error);
		return;
	}

	if (! pmf_device_register1(self, NULL, NULL, NULL))
		aprint_error_dev(self, "couldn't establish power handler\n");

	sc->sc_nordev = nor_attach_mi(&sc->sc_nor_if, self);

}

static int
pq3cfi_detach(device_t self, int flags)
{
	struct pq3cfi_softc *sc = device_private(self);
	int rv = 0;

	pmf_device_deregister(self);

	if (sc->sc_nordev != NULL)
		rv = config_detach(sc->sc_nordev, flags);

	bus_space_unmap(sc->sc_cfi.cfi_bst, sc->sc_cfi.cfi_bsh, sc->sc_size);

	return rv;
}
