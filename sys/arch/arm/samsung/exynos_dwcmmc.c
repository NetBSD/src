/* $NetBSD: exynos_dwcmmc.c,v 1.2.2.3 2016/07/09 20:24:50 skrll Exp $ */

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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: exynos_dwcmmc.c,v 1.2.2.3 2016/07/09 20:24:50 skrll Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/intr.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/mutex.h>
#include <sys/condvar.h>

#include <arm/samsung/exynos_var.h>

#include <dev/ic/dwc_mmc_var.h>
#include <dev/fdt/fdtvar.h>

static int	exynos_dwcmmc_match(device_t, cfdata_t, void *);
static void	exynos_dwcmmc_attach(device_t, device_t, void *);

static void	exynos_dwcmmc_attach_i(device_t);

static int	exynos_dwcmmc_card_detect(struct dwc_mmc_softc *);

struct exynos_dwcmmc_softc {
	struct dwc_mmc_softc	sc;
	struct clk		*sc_clk_biu;
	struct clk		*sc_clk_ciu;
	struct fdtbus_gpio_pin	*sc_pin_cd;
};

CFATTACH_DECL_NEW(exynos_dwcmmc, sizeof(struct dwc_mmc_softc),
	exynos_dwcmmc_match, exynos_dwcmmc_attach, NULL, NULL);

static const char * const exynos_dwcmmc_compat[] = {
	"samsung,exynos5420-dw-mshc-smu",
	"samsung,exynos5420-dw-mshc",
	NULL
};

static int
exynos_dwcmmc_match(device_t parent, cfdata_t cf, void *aux)
{
	struct fdt_attach_args * const faa = aux;

	return of_match_compatible(faa->faa_phandle, exynos_dwcmmc_compat);
}

static void
exynos_dwcmmc_attach(device_t parent, device_t self, void *aux)
{
	struct exynos_dwcmmc_softc *esc = device_private(self);
	struct dwc_mmc_softc *sc = &esc->sc;
	struct fdt_attach_args * const faa = aux;
	const int phandle = faa->faa_phandle;
	char intrstr[128];
	bus_addr_t addr;
	bus_size_t size;
	u_int ciu_div, fifo_depth;
	int error;

	if (fdtbus_get_reg(phandle, 0, &addr, &size) != 0) {
		aprint_error(": couldn't get registers\n");
		return;
	}

	//if (of_getprop_uint32(phandle, "bus-width", &bus_width)) {
	//	bus_width = 4;
	//}
	if (of_getprop_uint32(phandle, "fifo-depth", &fifo_depth)) {
		fifo_depth = 64;
	}
	if (of_getprop_uint32(phandle, "samsung,dw-mshc-ciu-div", &ciu_div)) {
		aprint_error(": missing samsung,dw-mshc-ciu-div property\n");
		return;
	}

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

	sc->sc_clock_freq = clk_get_rate(esc->sc_clk_ciu) / (ciu_div + 1);
	sc->sc_fifo_depth = fifo_depth;

	esc->sc_pin_cd = fdtbus_gpio_acquire(phandle, "cd-gpios",
	    GPIO_PIN_INPUT);
	if (esc->sc_pin_cd) {
		sc->sc_card_detect = exynos_dwcmmc_card_detect;
	}

	aprint_naive("\n");
	aprint_normal(": MHS (%u Hz)\n", sc->sc_clock_freq);

	if (!fdtbus_intr_str(phandle, 0, intrstr, sizeof(intrstr))) {
		aprint_error_dev(self, "failed to decode interrupt\n");
		return;
	}

	sc->sc_ih = fdtbus_intr_establish(phandle, 0, IPL_BIO, 0,
	    dwc_mmc_intr, sc);
	if (sc->sc_ih == NULL) {
		aprint_error_dev(self, "couldn't establish interrupt on %s\n",
		    intrstr);
		return;
	}
	aprint_normal_dev(self, "interrupting on %s\n", intrstr);

	config_interrupts(self, exynos_dwcmmc_attach_i);
}

static void
exynos_dwcmmc_attach_i(device_t self)
{
	struct exynos_dwcmmc_softc *esc = device_private(self);
	struct dwc_mmc_softc *sc = &esc->sc;

	dwc_mmc_init(sc);
}

static int
exynos_dwcmmc_card_detect(struct dwc_mmc_softc *sc)
{
	struct exynos_dwcmmc_softc *esc = device_private(sc->sc_dev);

	KASSERT(esc->sc_pin_cd != NULL);

	return fdtbus_gpio_read(esc->sc_pin_cd);
}
