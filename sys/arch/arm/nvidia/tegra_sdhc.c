/* $NetBSD: tegra_sdhc.c,v 1.1.2.2 2015/04/06 15:17:53 skrll Exp $ */

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
__KERNEL_RCSID(0, "$NetBSD: tegra_sdhc.c,v 1.1.2.2 2015/04/06 15:17:53 skrll Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/intr.h>
#include <sys/systm.h>
#include <sys/kernel.h>

#include <dev/sdmmc/sdhcreg.h>
#include <dev/sdmmc/sdhcvar.h>
#include <dev/sdmmc/sdmmcvar.h>

#include <arm/nvidia/tegra_var.h>

/* 8-bit eMMC is supported on SDMMC2 and SDMMC4 */
#define SDMMC_8BIT_P(port)	((port) == 1 || (port) == 3)

static int	tegra_sdhc_match(device_t, cfdata_t, void *);
static void	tegra_sdhc_attach(device_t, device_t, void *);

static void	tegra_sdhc_attach_i(device_t);

struct tegra_sdhc_softc {
	struct sdhc_softc	sc;

	bus_space_tag_t		sc_bst;
	bus_space_handle_t	sc_bsh;
	bus_size_t		sc_bsz;
	struct sdhc_host	*sc_host;
	void			*sc_ih;
};

CFATTACH_DECL_NEW(tegra_sdhc, sizeof(struct tegra_sdhc_softc),
	tegra_sdhc_match, tegra_sdhc_attach, NULL, NULL);

static int
tegra_sdhc_match(device_t parent, cfdata_t cf, void *aux)
{
	return 1;
}

static void
tegra_sdhc_attach(device_t parent, device_t self, void *aux)
{
	struct tegra_sdhc_softc * const sc = device_private(self);
	struct tegraio_attach_args * const tio = aux;
	const struct tegra_locators * const loc = &tio->tio_loc;

	sc->sc.sc_dev = self;
	sc->sc.sc_dmat = tio->tio_dmat;
	sc->sc.sc_flags = SDHC_FLAG_32BIT_ACCESS |
			  SDHC_FLAG_USE_DMA;
	if (SDMMC_8BIT_P(loc->loc_port)) {
		sc->sc.sc_flags |= SDHC_FLAG_8BIT_MODE;
	}
	sc->sc.sc_host = &sc->sc_host;

	sc->sc_bst = tio->tio_bst;
	bus_space_subregion(tio->tio_bst, tio->tio_bsh,
	    loc->loc_offset, loc->loc_size, &sc->sc_bsh);
	sc->sc_bsz = loc->loc_size;

#if notyet
	sc->sc.sc_clkbase = tegra_sdhc_get_freq(loc->loc_port) / 1000;
#else
	sc->sc.sc_clkbase = 0;
#endif

	aprint_naive("\n");
	aprint_normal(": SDMMC%d\n", loc->loc_port + 1);

	if (sc->sc.sc_clkbase == 0) {
		aprint_error_dev(self, "couldn't determine frequency\n");
		return;
	}

	sc->sc_ih = intr_establish(loc->loc_intr, IPL_SDMMC, IST_LEVEL,
	    sdhc_intr, &sc->sc);
	if (sc->sc_ih == NULL) {
		aprint_error_dev(self, "couldn't establish interrupt %d\n",
		    loc->loc_intr);
		return;
	}
	aprint_normal_dev(self, "interrupting on irq %d\n", loc->loc_intr);

	config_interrupts(self, tegra_sdhc_attach_i);
}

static void
tegra_sdhc_attach_i(device_t self)
{
	struct tegra_sdhc_softc * const sc = device_private(self);
	int error;

	error = sdhc_host_found(&sc->sc, sc->sc_bst, sc->sc_bsh, sc->sc_bsz);
	if (error) {
		aprint_error_dev(self, "couldn't initialize host, error = %d\n",
		    error);
		intr_disestablish(sc->sc_ih);
		sc->sc_ih = NULL;
		return;
	}
}
