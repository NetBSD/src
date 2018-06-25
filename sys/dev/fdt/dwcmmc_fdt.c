/* $NetBSD: dwcmmc_fdt.c,v 1.3.2.2 2018/06/25 07:25:49 pgoyette Exp $ */

/*-
 * Copyright (c) 2015-2018 Jared McNeill <jmcneill@invisible.ca>
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: dwcmmc_fdt.c,v 1.3.2.2 2018/06/25 07:25:49 pgoyette Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/intr.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/mutex.h>
#include <sys/condvar.h>
#include <sys/gpio.h>

#include <dev/ic/dwc_mmc_var.h>
#include <dev/fdt/fdtvar.h>

static int	dwcmmc_fdt_match(device_t, cfdata_t, void *);
static void	dwcmmc_fdt_attach(device_t, device_t, void *);

static int	dwcmmc_fdt_card_detect(struct dwc_mmc_softc *);
static int	dwcmmc_fdt_bus_clock(struct dwc_mmc_softc *, int);

struct dwcmmc_fdt_config {
	u_int		ciu_div;
};

static const struct dwcmmc_fdt_config dwcmmc_rk3328_config = {
	.ciu_div = 2,
};

static const struct of_compat_data compat_data[] = {
	{ "rockchip,rk3328-dw-mshc",	(uintptr_t)&dwcmmc_rk3328_config },
	{ NULL }
};

struct dwcmmc_fdt_softc {
	struct dwc_mmc_softc	sc;
	struct clk		*sc_clk_biu;
	struct clk		*sc_clk_ciu;
	struct fdtbus_gpio_pin	*sc_pin_cd;
	const struct dwcmmc_fdt_config *sc_conf;
	u_int			sc_ciu_div;
};

CFATTACH_DECL_NEW(dwcmmc_fdt, sizeof(struct dwc_mmc_softc),
	dwcmmc_fdt_match, dwcmmc_fdt_attach, NULL, NULL);

static int
dwcmmc_fdt_match(device_t parent, cfdata_t cf, void *aux)
{
	struct fdt_attach_args * const faa = aux;

	return of_match_compat_data(faa->faa_phandle, compat_data);
}

static void
dwcmmc_fdt_attach(device_t parent, device_t self, void *aux)
{
	struct dwcmmc_fdt_softc *esc = device_private(self);
	struct dwc_mmc_softc *sc = &esc->sc;
	struct fdt_attach_args * const faa = aux;
	const int phandle = faa->faa_phandle;
	char intrstr[128];
	u_int fifo_depth;
	bus_addr_t addr;
	bus_size_t size;
	int error;

	if (fdtbus_get_reg(phandle, 0, &addr, &size) != 0) {
		aprint_error(": couldn't get registers\n");
		return;
	}

	if (of_getprop_uint32(phandle, "fifo-depth", &fifo_depth))
		fifo_depth = 0;

	esc->sc_clk_biu = fdtbus_clock_get(phandle, "biu");
	if (esc->sc_clk_biu == NULL) {
		aprint_error(": couldn't get clock biu\n");
		return;
	}
	esc->sc_clk_ciu = fdtbus_clock_get(phandle, "ciu");
	if (esc->sc_clk_ciu == NULL) {
		aprint_error(": couldn't get clock ciu\n");
		return;
	}

	error = clk_enable(esc->sc_clk_biu);
	if (error) {
		aprint_error(": couldn't enable clock biu: %d\n", error);
		return;
	}
	error = clk_enable(esc->sc_clk_ciu);
	if (error) {
		aprint_error(": couldn't enable clock ciu: %d\n", error);
		return;
	}

	sc->sc_dev = self;
	sc->sc_bst = faa->faa_bst;
	sc->sc_dmat = faa->faa_dmat;
	error = bus_space_map(sc->sc_bst, addr, size, 0, &sc->sc_bsh);
	if (error) {
		aprint_error(": couldn't map %#llx: %d\n",
		    (uint64_t)addr, error);
		return;
	}
	esc->sc_conf = of_search_compatible(phandle, compat_data)->data;


	if (of_getprop_uint32(phandle, "max-frequency", &sc->sc_clock_freq) != 0)
		sc->sc_clock_freq = UINT_MAX;

	sc->sc_fifo_depth = fifo_depth;
	sc->sc_flags = DWC_MMC_F_USE_HOLD_REG | DWC_MMC_F_DMA;
	sc->sc_bus_clock = dwcmmc_fdt_bus_clock;

	esc->sc_pin_cd = fdtbus_gpio_acquire(phandle, "cd-gpios",
	    GPIO_PIN_INPUT);
	if (esc->sc_pin_cd)
		sc->sc_card_detect = dwcmmc_fdt_card_detect;

	aprint_naive("\n");
	aprint_normal(": DesignWare SD/MMC\n");

	if (!fdtbus_intr_str(phandle, 0, intrstr, sizeof(intrstr))) {
		aprint_error_dev(self, "failed to decode interrupt\n");
		return;
	}

	if (dwc_mmc_init(sc) != 0)
		return;

	sc->sc_ih = fdtbus_intr_establish(phandle, 0, IPL_BIO, 0,
	    dwc_mmc_intr, sc);
	if (sc->sc_ih == NULL) {
		aprint_error_dev(self, "couldn't establish interrupt on %s\n",
		    intrstr);
		return;
	}
	aprint_normal_dev(self, "interrupting on %s\n", intrstr);
}

static int
dwcmmc_fdt_card_detect(struct dwc_mmc_softc *sc)
{
	struct dwcmmc_fdt_softc *esc = device_private(sc->sc_dev);

	KASSERT(esc->sc_pin_cd != NULL);

	return fdtbus_gpio_read(esc->sc_pin_cd);
}

static int
dwcmmc_fdt_bus_clock(struct dwc_mmc_softc *sc, int rate)
{
	struct dwcmmc_fdt_softc *esc = device_private(sc->sc_dev);
        const u_int ciu_div = esc->sc_conf->ciu_div > 0 ? esc->sc_conf->ciu_div : 1;
	int error;

	error = clk_set_rate(esc->sc_clk_ciu, 1000 * rate * ciu_div);
	if (error != 0) {
		aprint_error_dev(sc->sc_dev, "failed to set rate to %u kHz: %d\n",
		    rate * ciu_div, error);
		return error;
	}

	sc->sc_clock_freq = (clk_get_rate(esc->sc_clk_ciu) / ciu_div) / 1000;

	aprint_debug_dev(sc->sc_dev, "set clock rate to %u kHz (target %u kHz)\n",
	    sc->sc_clock_freq, rate);

	return 0;
}
