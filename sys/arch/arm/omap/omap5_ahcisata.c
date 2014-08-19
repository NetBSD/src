/* $NetBSD: omap5_ahcisata.c,v 1.1.2.3 2014/08/20 00:02:47 tls Exp $ */

/*-
 * Copyright (c) 2013 The NetBSD Foundation, Inc.
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: omap5_ahcisata.c,v 1.1.2.3 2014/08/20 00:02:47 tls Exp $");

#include "locators.h"

#include "opt_omap.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/intr.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <sys/bus.h>
#include <sys/gpio.h>

#include <dev/ata/atavar.h>

#include <dev/ic/ahcisatavar.h>

#include <arm/omap/omap2_obioreg.h>
#include <arm/omap/omap2_obiovar.h>
#include <arm/omap/omap2_reg.h>

struct omap5_ahci_softc {
	struct ahci_softc	psc_sc;
	void		*psc_ih;
};

static int
omap5_ahcisata_match(device_t parent, cfdata_t match, void *aux)
{
#if defined(OMAP5)
	struct obio_attach_args *obio = aux;

	if (obio->obio_addr == AHCI1_BASE_OMAP5)
		return 1;
#endif

	return 0;
}

static void
omap5_ahcisata_attach(device_t parent, device_t self, void *aux)
{
	struct omap5_ahci_softc *psc = device_private(self);
	struct ahci_softc * const sc = &psc->psc_sc;
	struct obio_attach_args * const obio = aux;
	int rv;

	sc->sc_atac.atac_dev = self;

	aprint_naive("\n");
	aprint_normal(": OMAP AHCI controller\n");

#ifdef OMAP5
	{
		bus_space_handle_t ioh;
		rv = bus_space_map(obio->obio_iot, OMAP2_CM_BASE
		    + OMAP5_CM_L3INIT_CORE + OMAP5_CM_L3INIT_SATA_CLKCTRL,
		    4, 0, &ioh);
		KASSERT(rv == 0);
		uint32_t v = bus_space_read_4(obio->obio_iot, ioh, 0);
		v &= ~OMAP4_CM_L3INIT_SATA_CLKCTRL_MODULEMODE;
		v |= OMAP4_CM_L3INIT_SATA_CLKCTRL_MODULEMODE_HW;
		bus_space_write_4(obio->obio_iot, ioh, 0, v);
		bus_space_unmap(obio->obio_iot, ioh, 4);
	}
#endif

	sc->sc_ahci_ports = 1;
	sc->sc_dmat = obio->obio_dmat;
	sc->sc_ahcit = obio->obio_iot;
	sc->sc_ahcis = obio->obio_size;
	rv = bus_space_map(obio->obio_iot, obio->obio_addr, obio->obio_size,
	    0, &sc->sc_ahcih);
	if (rv) {
		aprint_error_dev(self, "couldn't map memory space\n");
		return;
	}

	psc->psc_ih = intr_establish(obio->obio_intr, IPL_BIO, IST_LEVEL,
	    ahci_intr, sc);

	ahci_attach(sc);
}

static int
omap5_ahcisata_detach(device_t self, int flags)
{
	struct omap5_ahci_softc * const psc = device_private(self);
	struct ahci_softc * const sc = &psc->psc_sc;
	int rv;

	rv = ahci_detach(sc, flags);
	if (rv)
		return rv;

	if (psc->psc_ih) {
		intr_disestablish(psc->psc_ih);
		psc->psc_ih = NULL;
	}

	if (sc->sc_ahcis) {
		bus_space_unmap(sc->sc_ahcit, sc->sc_ahcih, sc->sc_ahcis);
		sc->sc_ahcit = 0;
		sc->sc_ahcih = 0;
		sc->sc_ahcis = 0;
	}

	return 0;
}

CFATTACH_DECL_NEW(omap5_ahcisata, sizeof(struct omap5_ahci_softc),
    omap5_ahcisata_match,
    omap5_ahcisata_attach,
    omap5_ahcisata_detach,
    0
);
