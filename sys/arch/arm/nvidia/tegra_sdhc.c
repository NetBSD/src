/* $NetBSD: tegra_sdhc.c,v 1.1.2.5 2015/12/27 12:09:31 skrll Exp $ */

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
__KERNEL_RCSID(0, "$NetBSD: tegra_sdhc.c,v 1.1.2.5 2015/12/27 12:09:31 skrll Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/intr.h>
#include <sys/systm.h>
#include <sys/kernel.h>

#include <dev/sdmmc/sdhcreg.h>
#include <dev/sdmmc/sdhcvar.h>
#include <dev/sdmmc/sdmmcvar.h>

#include <arm/nvidia/tegra_reg.h>
#include <arm/nvidia/tegra_var.h>

#include <dev/fdt/fdtvar.h>

static int	tegra_sdhc_match(device_t, cfdata_t, void *);
static void	tegra_sdhc_attach(device_t, device_t, void *);

static int	tegra_sdhc_card_detect(struct sdhc_softc *);
static int	tegra_sdhc_write_protect(struct sdhc_softc *);

struct tegra_sdhc_softc {
	struct sdhc_softc	sc;

	struct clk		*sc_clk;
	struct fdtbus_reset	*sc_rst;

	bus_space_tag_t		sc_bst;
	bus_space_handle_t	sc_bsh;
	bus_size_t		sc_bsz;
	struct sdhc_host	*sc_host;
	void			*sc_ih;

	struct fdtbus_gpio_pin	*sc_pin_cd;
	struct fdtbus_gpio_pin	*sc_pin_power;
	struct fdtbus_gpio_pin	*sc_pin_wp;
};

CFATTACH_DECL_NEW(tegra_sdhc, sizeof(struct tegra_sdhc_softc),
	tegra_sdhc_match, tegra_sdhc_attach, NULL, NULL);

static int
tegra_sdhc_match(device_t parent, cfdata_t cf, void *aux)
{
	const char * const compatible[] = { "nvidia,tegra124-sdhci", NULL };
	struct fdt_attach_args * const faa = aux;

	return of_match_compatible(faa->faa_phandle, compatible);
}

static void
tegra_sdhc_attach(device_t parent, device_t self, void *aux)
{
	struct tegra_sdhc_softc * const sc = device_private(self);
	struct fdt_attach_args * const faa = aux;
	char intrstr[128];
	bus_addr_t addr;
	bus_size_t size;
	u_int bus_width;
	int error;

	if (fdtbus_get_reg(faa->faa_phandle, 0, &addr, &size) != 0) {
		aprint_error(": couldn't get registers\n");
		return;
	}

	if (of_getprop_uint32(faa->faa_phandle, "bus-width", &bus_width))
		bus_width = 4;

	sc->sc.sc_dev = self;
	sc->sc.sc_dmat = faa->faa_dmat;
	sc->sc.sc_flags = SDHC_FLAG_32BIT_ACCESS |
			  SDHC_FLAG_NO_PWR0 |
			  SDHC_FLAG_NO_CLKBASE |
			  SDHC_FLAG_NO_TIMEOUT |
			  SDHC_FLAG_SINGLE_POWER_WRITE |
			  SDHC_FLAG_USE_DMA |
			  SDHC_FLAG_USE_ADMA2;
	if (bus_width == 8) {
		sc->sc.sc_flags |= SDHC_FLAG_8BIT_MODE;
	}
	sc->sc.sc_host = &sc->sc_host;

	sc->sc_bst = faa->faa_bst;
	error = bus_space_map(sc->sc_bst, addr, size, 0, &sc->sc_bsh);
	if (error) {
		aprint_error(": couldn't map %#llx: %d", (uint64_t)addr, error);
		return;
	}
	sc->sc_bsz = size;

	sc->sc_pin_power = fdtbus_gpio_acquire(faa->faa_phandle,
	    "power-gpios", GPIO_PIN_OUTPUT);
	if (sc->sc_pin_power)
		fdtbus_gpio_write(sc->sc_pin_power, 1);

	sc->sc_pin_cd = fdtbus_gpio_acquire(faa->faa_phandle, 
	    "cd-gpios", GPIO_PIN_INPUT);
	sc->sc_pin_wp = fdtbus_gpio_acquire(faa->faa_phandle,
	    "wp-gpios", GPIO_PIN_INPUT);

	if (sc->sc_pin_cd) {
		sc->sc.sc_vendor_card_detect = tegra_sdhc_card_detect;
		sc->sc.sc_flags |= SDHC_FLAG_POLL_CARD_DET;
	}
	if (sc->sc_pin_wp) {
		sc->sc.sc_vendor_write_protect = tegra_sdhc_write_protect;
	}

	sc->sc_clk = fdtbus_clock_get_index(faa->faa_phandle, 0);
	if (sc->sc_clk == NULL) {
		aprint_error(": couldn't get clock\n");
		return;
	}
	sc->sc_rst = fdtbus_reset_get(faa->faa_phandle, "sdhci");
	if (sc->sc_rst == NULL) {
		aprint_error(": couldn't get reset\n");
		return;
	}

	fdtbus_reset_assert(sc->sc_rst);
	error = clk_set_rate(sc->sc_clk, 204000000);
	if (error) {
		aprint_error(": couldn't set frequency: %d\n", error);
		return;
	}
	error = clk_enable(sc->sc_clk);
	if (error) {
		aprint_error(": couldn't enable clock: %d\n", error);
		return;
	}
	fdtbus_reset_deassert(sc->sc_rst);

	sc->sc.sc_clkbase = clk_get_rate(sc->sc_clk) / 1000;

	aprint_naive("\n");
	aprint_normal(": SDMMC\n");

	if (sc->sc.sc_clkbase == 0) {
		aprint_error_dev(self, "couldn't determine frequency\n");
		return;
	}

	if (!fdtbus_intr_str(faa->faa_phandle, 0, intrstr, sizeof(intrstr))) {
		aprint_error_dev(self, "failed to decode interrupt\n");
		return;
	}

	sc->sc_ih = fdtbus_intr_establish(faa->faa_phandle, 0, IPL_SDMMC, 0,
	    sdhc_intr, &sc->sc);
	if (sc->sc_ih == NULL) {
		aprint_error_dev(self, "couldn't establish interrupt on %s\n",
		    intrstr);
		return;
	}
	aprint_normal_dev(self, "interrupting on %s\n", intrstr);

	error = sdhc_host_found(&sc->sc, sc->sc_bst, sc->sc_bsh, sc->sc_bsz);
	if (error) {
		aprint_error_dev(self, "couldn't initialize host, error = %d\n",
		    error);
		fdtbus_intr_disestablish(faa->faa_phandle, sc->sc_ih);
		sc->sc_ih = NULL;
		return;
	}
}

static int
tegra_sdhc_card_detect(struct sdhc_softc *ssc)
{
	struct tegra_sdhc_softc *sc = device_private(ssc->sc_dev);

	KASSERT(sc->sc_pin_cd != NULL);

	return fdtbus_gpio_read(sc->sc_pin_cd);
}

static int
tegra_sdhc_write_protect(struct sdhc_softc *ssc)
{
	struct tegra_sdhc_softc *sc = device_private(ssc->sc_dev);

	KASSERT(sc->sc_pin_wp != NULL);

	return fdtbus_gpio_read(sc->sc_pin_wp);
}
