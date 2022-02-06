/*	$NetBSD: imx_sdhc.c,v 1.7 2022/02/06 15:52:20 jmcneill Exp $	*/

/*-
 * Copyright (c) 2019 Genetec Corporation.  All rights reserved.
 * Written by Hashimoto Kenichi for Genetec Corporation.
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
__KERNEL_RCSID(0, "$NetBSD: imx_sdhc.c,v 1.7 2022/02/06 15:52:20 jmcneill Exp $");

#include "opt_fdt.h"

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/intr.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/gpio.h>

#include <dev/sdmmc/sdhcreg.h>
#include <dev/sdmmc/sdhcvar.h>
#include <dev/sdmmc/sdmmcvar.h>

#include <dev/fdt/fdtvar.h>

static int imx_sdhc_match(device_t, cfdata_t, void *);
static void imx_sdhc_attach(device_t, device_t, void *);

static int imx_sdhc_card_detect(struct sdhc_softc *);
static int imx_sdhc_write_protect(struct sdhc_softc *);

struct imx_sdhc_softc {
	struct sdhc_softc sc_sdhc;

	bus_space_tag_t		sc_bst;
	bus_space_handle_t	sc_bsh;
	bus_size_t		sc_bsz;

	struct sdhc_host	*sc_host;
	void			*sc_ih;

	struct clk		*sc_clk_per;
	struct fdtbus_regulator	*sc_vmmc_supply;

	struct fdtbus_gpio_pin	*sc_pin_cd;
	struct fdtbus_gpio_pin	*sc_pin_wp;
};

CFATTACH_DECL_NEW(imx_sdhc, sizeof(struct imx_sdhc_softc),
	imx_sdhc_match, imx_sdhc_attach, NULL, NULL);

struct imx6_sdhc_config {
	uint32_t		flags;
};

static const struct imx6_sdhc_config imx6q_config = {
	.flags = SDHC_FLAG_BROKEN_ADMA2_ZEROLEN |
		 SDHC_FLAG_NO_BUSY_INTR,
};

static const struct imx6_sdhc_config imx7d_config = {
	.flags = 0
};

static const struct device_compatible_entry compat_data[] = {
	{ .compat = "fsl,imx6q-usdhc",	.data = &imx6q_config },
	{ .compat = "fsl,imx7d-usdhc",	.data = &imx7d_config },
	DEVICE_COMPAT_EOL
};

static int
imx_sdhc_match(device_t parent, cfdata_t cf, void *aux)
{
	struct fdt_attach_args * const faa = aux;

	return of_compatible_match(faa->faa_phandle, compat_data);
}

static void
imx_sdhc_attach(device_t parent, device_t self, void *aux)
{
	struct imx_sdhc_softc * const sc = device_private(self);
	struct fdt_attach_args * const faa = aux;
	const int phandle = faa->faa_phandle;
	const struct imx6_sdhc_config *conf;
	char intrstr[128];
	bus_addr_t addr;
	bus_size_t size;
	u_int bus_width;
	int error;

	fdtbus_clock_assign(phandle);

	if (fdtbus_get_reg(phandle, 0, &addr, &size) != 0) {
		aprint_error(": couldn't get registers\n");
		return;
	}

	sc->sc_clk_per = fdtbus_clock_get(phandle, "per");
	if (sc->sc_clk_per == NULL) {
		aprint_error(": couldn't get clock\n");
		return;
	}

	if (of_getprop_uint32(phandle, "bus-width", &bus_width))
		bus_width = 4;

	sc->sc_vmmc_supply = fdtbus_regulator_acquire(phandle, "vmmc-supply");

	conf = of_compatible_lookup(phandle, compat_data)->data;

	sc->sc_sdhc.sc_dev = self;
	sc->sc_sdhc.sc_dmat = faa->faa_dmat;

	sc->sc_sdhc.sc_clkbase = clk_get_rate(sc->sc_clk_per) / 1000;
	sc->sc_sdhc.sc_flags =
	    SDHC_FLAG_USE_DMA |
	    SDHC_FLAG_NO_PWR0 |
	    SDHC_FLAG_HAVE_DVS |
	    SDHC_FLAG_32BIT_ACCESS |
	    SDHC_FLAG_USDHC;
	sc->sc_sdhc.sc_flags |= conf->flags;

	if (bus_width == 8)
		sc->sc_sdhc.sc_flags |= SDHC_FLAG_8BIT_MODE;
	if (of_hasprop(phandle, "no-1-8-v"))
		sc->sc_sdhc.sc_flags |= SDHC_FLAG_NO_1_8_V;

	sc->sc_sdhc.sc_host = &sc->sc_host;

	sc->sc_bst = faa->faa_bst;
	error = bus_space_map(sc->sc_bst, addr, size, 0, &sc->sc_bsh);
	if (error) {
		aprint_error(": couldn't map %#" PRIxBUSADDR ": %d", addr, error);
		return;
	}
	sc->sc_bsz = size;

	sc->sc_pin_cd = fdtbus_gpio_acquire(phandle,
	    "cd-gpios", GPIO_PIN_INPUT);
	if (sc->sc_pin_cd) {
		sc->sc_sdhc.sc_vendor_card_detect = imx_sdhc_card_detect;
		sc->sc_sdhc.sc_flags |= SDHC_FLAG_POLL_CARD_DET;
	}

	sc->sc_pin_wp = fdtbus_gpio_acquire(phandle,
	    "wp-gpios", GPIO_PIN_INPUT);
	if (sc->sc_pin_wp) {
		sc->sc_sdhc.sc_vendor_write_protect = imx_sdhc_write_protect;
	}

	error = clk_enable(sc->sc_clk_per);
	if (error) {
		aprint_error(": couldn't enable clock: %d\n", error);
		return;
	}

	if (sc->sc_vmmc_supply != NULL) {
		error = fdtbus_regulator_enable(sc->sc_vmmc_supply);
		if (error) {
			aprint_error(": couldn't enable vmmc supply: %d\n", error);
			return;
		}
	}

	aprint_naive("\n");
	aprint_normal(": SDMMC (%u kHz)\n", sc->sc_sdhc.sc_clkbase);

	if (sc->sc_sdhc.sc_clkbase == 0) {
		aprint_error_dev(self, "couldn't determine frequency\n");
		return;
	}

	if (!fdtbus_intr_str(phandle, 0, intrstr, sizeof(intrstr))) {
		aprint_error_dev(self, "failed to decode interrupt\n");
		return;
	}

	sc->sc_ih = fdtbus_intr_establish_xname(phandle, 0, IPL_SDMMC,
	    FDT_INTR_MPSAFE, sdhc_intr, &sc->sc_sdhc, device_xname(self));
	if (sc->sc_ih == NULL) {
		aprint_error_dev(self, "couldn't establish interrupt on %s\n",
		    intrstr);
		return;
	}
	aprint_normal_dev(self, "interrupting on %s\n", intrstr);

	error = sdhc_host_found(&sc->sc_sdhc, sc->sc_bst, sc->sc_bsh, sc->sc_bsz);
	if (error) {
		aprint_error_dev(self, "couldn't initialize host, error = %d\n",
		    error);
		fdtbus_intr_disestablish(phandle, sc->sc_ih);
		sc->sc_ih = NULL;
		return;
	}
}

static int
imx_sdhc_card_detect(struct sdhc_softc *ssc)
{
	struct imx_sdhc_softc *sc = device_private(ssc->sc_dev);

	KASSERT(sc->sc_pin_cd != NULL);

	return fdtbus_gpio_read(sc->sc_pin_cd);
}

static int
imx_sdhc_write_protect(struct sdhc_softc *ssc)
{
	struct imx_sdhc_softc *sc = device_private(ssc->sc_dev);

	KASSERT(sc->sc_pin_wp != NULL);

	return fdtbus_gpio_read(sc->sc_pin_wp);
}

