/*-
 * Copyright (c) 2011 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Matt Thomas of 3am Software Foundry.
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

#include "opt_flash.h"
#include "locators.h"

#include <sys/cdefs.h>

__KERNEL_RCSID(1, "$NetBSD: rmixl_cfi_xlnor.c,v 1.1.2.2 2011/12/28 05:36:11 matt Exp $");

#include <sys/param.h>
#include <sys/device.h>
#include <sys/bus.h>

#include <dev/nor/nor.h>
#include <dev/nor/cfi.h>

#include <mips/rmi/rmixl_iobusvar.h>

static int  cfi_xlnor_match(device_t, cfdata_t, void *);
static void cfi_xlnor_attach(device_t, device_t, void *);
static int  cfi_xlnor_detach(device_t, int);

struct cfi_xlnor_softc {
	device_t		sc_dev;
	device_t		sc_nordev;
	struct cfi		sc_cfi;
	bus_addr_t		sc_addr;
	bus_size_t		sc_size;
	struct nor_interface	sc_nor_if;
};

CFATTACH_DECL_NEW(cfi_xlnor, sizeof(struct cfi_xlnor_softc),
    cfi_xlnor_match, cfi_xlnor_attach, cfi_xlnor_detach, 0);

static int
cfi_xlnor_match(device_t parent, cfdata_t cf, void *aux)
{
	struct rmixl_iobus_attach_args *ia = aux;
	bus_size_t tmpsize = CFI_QRY_MIN_MAP_SIZE;
	bus_addr_t addr = ia->ia_iobus_addr;
	struct cfi cfi;
	int rv;

	KASSERT(ia->ia_iobus_bst != NULL);
	KASSERT(ia->ia_iobus_addr == 0);
	KASSERT(ia->ia_iobus_size >= tmpsize);
	KASSERT(ia->ia_obio_bst == NULL);

	if (cf->cf_loc[XLIOBUSCF_CS] != ia->ia_cs
	    && cf->cf_loc[XLIOBUSCF_CS] != XLIOBUSCF_CS_DEFAULT)
		return 0;

	cfi.cfi_bst = ia->ia_iobus_bst;
	int error = bus_space_map(cfi.cfi_bst, addr, tmpsize, 0, &cfi.cfi_bsh);
	if (error != 0) {
		aprint_error("%s: cannot map %#"PRIxBUSSIZE" at offset %#"
		    PRIxBUSADDR": error %d\n", __func__, tmpsize, addr, error);
		return 0;
	}

	if (!cfi_probe(&cfi)) {
		aprint_debug("%s: probe addr %#"PRIxBUSADDR", CFI not found\n",
			__func__, addr);
		rv = 0;
	} else {
		rv = 1;
	}

	bus_space_unmap(cfi.cfi_bst, cfi.cfi_bsh, tmpsize);

	return rv;
}

static void
cfi_xlnor_attach(device_t parent, device_t self, void *aux)
{
	struct cfi_xlnor_softc *sc = device_private(self);
	struct rmixl_iobus_attach_args *ia = aux;
	struct cfi_query_data * const qryp = &sc->sc_cfi.cfi_qry_data;
	const bus_size_t tmpsize = CFI_QRY_MIN_MAP_SIZE;
	bool found;
	int error;

	aprint_normal("\n");

	sc->sc_dev = self;
	sc->sc_cfi.cfi_bst = ia->ia_iobus_bst;
	sc->sc_addr = ia->ia_iobus_addr;

	KASSERT(ia->ia_iobus_addr == 0);

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
		0, &sc->sc_cfi.cfi_bsh);
	if (error != 0) {
		aprint_error_dev(self, "could not map error %d\n", error);
		return;
	}

	if (! pmf_device_register1(self, NULL, NULL, NULL))
		aprint_error_dev(self, "couldn't establish power handler\n");

	sc->sc_nordev = nor_attach_mi(&sc->sc_nor_if, self);

}

static int
cfi_xlnor_detach(device_t self, int flags)
{
	struct cfi_xlnor_softc *sc = device_private(self);
	int rv = 0;

	pmf_device_deregister(self);

	if (sc->sc_nordev != NULL)
		rv = config_detach(sc->sc_nordev, flags);

	bus_space_unmap(sc->sc_cfi.cfi_bst, sc->sc_cfi.cfi_bsh, sc->sc_size);

	return rv;
}
