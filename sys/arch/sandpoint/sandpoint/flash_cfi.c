/* $NetBSD: flash_cfi.c,v 1.3.28.1 2015/09/22 12:05:50 skrll Exp $ */

/*-
 * Copyright (c) 2011 Frank Wille.
 * All rights reserved.
 *
 * Written by Frank Wille for The NetBSD Project.
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
 * NOR CFI driver support for sandpoint
 */
#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: flash_cfi.c,v 1.3.28.1 2015/09/22 12:05:50 skrll Exp $");

#include <sys/param.h>
#include <sys/device.h>

#include <machine/autoconf.h>

#include <dev/nor/nor.h>
#include <dev/nor/cfi.h>


static int  sandpointcfi_probe(device_t, cfdata_t, void *);
static void sandpointcfi_attach(device_t, device_t, void *);
static int  sandpointcfi_detach(device_t, int);

struct sandpointcfi_softc {
	device_t		sc_dev;
	device_t		sc_nordev;
	struct cfi		sc_cfi;
	bus_size_t		sc_size;
	struct nor_interface	sc_nor_if;
};

CFATTACH_DECL_NEW(sandpointcfi, sizeof(struct sandpointcfi_softc),
    sandpointcfi_probe, sandpointcfi_attach, sandpointcfi_detach, NULL);

static int
sandpointcfi_probe(device_t parent, cfdata_t cf, void *aux)
{
	extern struct cfdriver cfi_cd;
	struct mainbus_attach_args *ma = aux;
	const bus_size_t qrysize = CFI_QRY_MIN_MAP_SIZE;
	struct cfi cfi;
	int error, rv;

	if (strcmp(ma->ma_name, cfi_cd.cd_name) != 0)
		return 0;

	KASSERT(ma->ma_bst != NULL);

	cfi.cfi_bst = ma->ma_bst;

	/* flash should be at least 2 MiB in size at 0xffe00000 */
	error = bus_space_map(cfi.cfi_bst, 0xffe00000, qrysize, 0,
	    &cfi.cfi_bsh);
	if (error != 0) {
		aprint_error("%s: cannot map %d at offset %#x, error %d\n",
		    __func__, qrysize, ma->ma_addr, error);
		return 0;
	}

	/* probe for NOR flash */
	if (!cfi_probe(&cfi)) {
		aprint_debug("%s: probe addr %#x, CFI not found\n",
		    __func__, ma->ma_addr);
		rv = 0;
	} else
		rv = 1;

	bus_space_unmap(cfi.cfi_bst, cfi.cfi_bsh, qrysize);
	return rv;
}

static void
sandpointcfi_attach(device_t parent, device_t self, void *aux)
{
	struct mainbus_attach_args *ma = aux;
	struct sandpointcfi_softc *sc;
	const bus_size_t qrysize = CFI_QRY_MIN_MAP_SIZE;
	bus_addr_t addr;
	bool found;
	int error;

	aprint_naive("\n");
	aprint_normal("\n");

	sc = device_private(self);
	sc->sc_dev = self;
	sc->sc_cfi.cfi_bst = ma->ma_bst;

	/* map enough to identify, remap later when size is known */
	error = bus_space_map(sc->sc_cfi.cfi_bst, 0xffe00000, qrysize, 0,
	    &sc->sc_cfi.cfi_bsh);
	if (error != 0) {
		aprint_error_dev(self, "could not map error %d\n", error);
		return;
	}

	/* identify the NOR flash */
	found = cfi_identify(&sc->sc_cfi);

	bus_space_unmap(sc->sc_cfi.cfi_bst, sc->sc_cfi.cfi_bsh, qrysize);
	if (!found) {
		/* should not happen, we already probed OK in match */
		aprint_error_dev(self, "could not map error %d\n", error);
		return;
	}

	/* get size of flash in bytes */
	sc->sc_size = 1 << sc->sc_cfi.cfi_qry_data.device_size;

	/* determine real base address */
	addr = (0xffffffff - sc->sc_size) + 1;

	sc->sc_nor_if = nor_interface_cfi;
	sc->sc_nor_if.private = &sc->sc_cfi;
	sc->sc_nor_if.access_width = (1 << sc->sc_cfi.cfi_portwidth);

	cfi_print(self, &sc->sc_cfi);

	error = bus_space_map(sc->sc_cfi.cfi_bst, addr, sc->sc_size, 0,
		&sc->sc_cfi.cfi_bsh);
	if (error != 0) {
		aprint_error_dev(self, "could not map error %d\n", error);
		return;
	}

	if (!pmf_device_register1(self, NULL, NULL, NULL))
		aprint_error_dev(self, "couldn't establish power handler\n");

	sc->sc_nordev = nor_attach_mi(&sc->sc_nor_if, self);
}

static int
sandpointcfi_detach(device_t self, int flags)
{
	struct sandpointcfi_softc *sc;
	int rv;

	pmf_device_deregister(self);
	sc = device_private(self);
	rv = 0;

	if (sc->sc_nordev != NULL)
		rv = config_detach(sc->sc_nordev, flags);

	bus_space_unmap(sc->sc_cfi.cfi_bst, sc->sc_cfi.cfi_bsh, sc->sc_size);
	return rv;
}
