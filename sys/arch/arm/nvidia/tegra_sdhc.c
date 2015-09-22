/* $NetBSD: tegra_sdhc.c,v 1.1.2.4 2015/09/22 12:05:38 skrll Exp $ */

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
__KERNEL_RCSID(0, "$NetBSD: tegra_sdhc.c,v 1.1.2.4 2015/09/22 12:05:38 skrll Exp $");

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

static int	tegra_sdhc_card_detect(struct sdhc_softc *);
static int	tegra_sdhc_write_protect(struct sdhc_softc *);

struct tegra_sdhc_softc {
	struct sdhc_softc	sc;

	u_int			sc_port;

	bus_space_tag_t		sc_bst;
	bus_space_handle_t	sc_bsh;
	bus_size_t		sc_bsz;
	struct sdhc_host	*sc_host;
	void			*sc_ih;

	struct tegra_gpio_pin	*sc_pin_cd;
	struct tegra_gpio_pin	*sc_pin_power;
	struct tegra_gpio_pin	*sc_pin_wp;
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
	prop_dictionary_t prop = device_properties(self);
	const char *pin;
	int error;

	sc->sc.sc_dev = self;
	sc->sc.sc_dmat = tio->tio_dmat;
	sc->sc.sc_flags = SDHC_FLAG_32BIT_ACCESS |
			  SDHC_FLAG_NO_PWR0 |
			  SDHC_FLAG_NO_CLKBASE |
			  SDHC_FLAG_NO_TIMEOUT |
			  SDHC_FLAG_SINGLE_POWER_WRITE |
			  SDHC_FLAG_USE_DMA |
			  SDHC_FLAG_USE_ADMA2;
	if (SDMMC_8BIT_P(loc->loc_port)) {
		sc->sc.sc_flags |= SDHC_FLAG_8BIT_MODE;
	}
	sc->sc.sc_host = &sc->sc_host;

	sc->sc_bst = tio->tio_bst;
	bus_space_subregion(tio->tio_bst, tio->tio_bsh,
	    loc->loc_offset, loc->loc_size, &sc->sc_bsh);
	sc->sc_bsz = loc->loc_size;
	sc->sc_port = loc->loc_port;

	if (prop_dictionary_get_cstring_nocopy(prop, "power-gpio", &pin)) {
		sc->sc_pin_power = tegra_gpio_acquire(pin, GPIO_PIN_OUTPUT);
		if (sc->sc_pin_power)
			tegra_gpio_write(sc->sc_pin_power, 1);
	}

	if (prop_dictionary_get_cstring_nocopy(prop, "cd-gpio", &pin))
		sc->sc_pin_cd = tegra_gpio_acquire(pin, GPIO_PIN_INPUT);
	if (prop_dictionary_get_cstring_nocopy(prop, "wp-gpio", &pin))
		sc->sc_pin_wp = tegra_gpio_acquire(pin, GPIO_PIN_INPUT);

	if (sc->sc_pin_cd) {
		sc->sc.sc_vendor_card_detect = tegra_sdhc_card_detect;
		sc->sc.sc_flags |= SDHC_FLAG_POLL_CARD_DET;
	}
	if (sc->sc_pin_wp)
		sc->sc.sc_vendor_write_protect = tegra_sdhc_write_protect;

	tegra_car_periph_sdmmc_set_rate(sc->sc_port, 204000000);
	sc->sc.sc_clkbase = tegra_car_periph_sdmmc_rate(sc->sc_port) / 1000;

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

	error = sdhc_host_found(&sc->sc, sc->sc_bst, sc->sc_bsh, sc->sc_bsz);
	if (error) {
		aprint_error_dev(self, "couldn't initialize host, error = %d\n",
		    error);
		intr_disestablish(sc->sc_ih);
		sc->sc_ih = NULL;
		return;
	}
}

static int
tegra_sdhc_card_detect(struct sdhc_softc *ssc)
{
	struct tegra_sdhc_softc *sc = device_private(ssc->sc_dev);

	KASSERT(sc->sc_pin_cd != NULL);

	return !tegra_gpio_read(sc->sc_pin_cd);
}

static int
tegra_sdhc_write_protect(struct sdhc_softc *ssc)
{
	struct tegra_sdhc_softc *sc = device_private(ssc->sc_dev);

	KASSERT(sc->sc_pin_wp != NULL);

	return tegra_gpio_read(sc->sc_pin_wp);
}
