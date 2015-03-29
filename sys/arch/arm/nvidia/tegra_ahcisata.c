/* $NetBSD: tegra_ahcisata.c,v 1.1 2015/03/29 10:41:59 jmcneill Exp $ */

/*-
 * Copyright (c) 2015 Jared D. McNeill <jmcneill@invisible.ca>
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include "locators.h"

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: tegra_ahcisata.c,v 1.1 2015/03/29 10:41:59 jmcneill Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/intr.h>
#include <sys/systm.h>
#include <sys/kernel.h>

#include <dev/ata/atavar.h>
#include <dev/ic/ahcisatavar.h>

#include <arm/nvidia/tegra_var.h>

static int	tegra_ahcisata_match(device_t, cfdata_t, void *);
static void	tegra_ahcisata_attach(device_t, device_t, void *);

struct tegra_ahcisata_softc {
	struct ahci_softc	sc;
	void			*sc_ih;
};

CFATTACH_DECL_NEW(tegra_ahcisata, sizeof(struct tegra_ahcisata_softc),
	tegra_ahcisata_match, tegra_ahcisata_attach, NULL, NULL);

static int
tegra_ahcisata_match(device_t parent, cfdata_t cf, void *aux)
{
	return 1;
}

static void
tegra_ahcisata_attach(device_t parent, device_t self, void *aux)
{
	struct tegra_ahcisata_softc * const sc = device_private(self);
	struct tegraio_attach_args * const tio = aux;
	const struct tegra_locators * const loc = &tio->tio_loc;

	sc->sc.sc_atac.atac_dev = self;
	sc->sc.sc_dmat = tio->tio_dmat;
	sc->sc.sc_ahcit = tio->tio_bst;
	bus_space_subregion(tio->tio_bst, tio->tio_bsh,
	    loc->loc_offset, loc->loc_size, &sc->sc.sc_ahcis);
	sc->sc.sc_ahci_ports = 1;

	aprint_naive("\n");
	aprint_normal(": SATA\n");

	sc->sc_ih = intr_establish(loc->loc_intr, IPL_BIO, IST_LEVEL,
	    ahci_intr, &sc->sc);
	if (sc->sc_ih == NULL) {
		aprint_error_dev(self, "couldn't establish interrupt %d\n",
		    loc->loc_intr);
		return;
	}
	aprint_normal_dev(self, "interrupting on irq %d\n", loc->loc_intr);

	ahci_attach(&sc->sc);
}
