/*	$NetBSD: imx6_spi.c,v 1.8 2023/05/04 13:29:33 bouyer Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: imx6_spi.c,v 1.8 2023/05/04 13:29:33 bouyer Exp $");

#include "opt_imxspi.h"

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/kmem.h>
#include <sys/gpio.h>

#include <arm/imx/imxspivar.h>

#include <dev/fdt/fdtvar.h>

struct imxspi_fdt_softc {
	struct imxspi_softc sc_imxspi; /* Must be first */

	struct spi_chipset_tag sc_tag;
	struct clk *sc_clk;

	struct fdtbus_gpio_pin **sc_pin_cs;
};

struct imx_spi_config {
	bool enhanced;
	enum imxspi_type type;
};

static const struct imx_spi_config imx6q_spi_config = {
	.enhanced = true,
	.type = IMX51_ECSPI,
};

static const struct device_compatible_entry compat_data[] = {
	{ .compat = "fsl,imx6q-ecspi",	.data = &imx6q_spi_config },
	{ .compat = "fsl,imx6sx-ecspi",	.data = &imx6q_spi_config },
	DEVICE_COMPAT_EOL
};

CFATTACH_DECL_NEW(imxspi_fdt, sizeof(struct imxspi_fdt_softc),
    imxspi_match, imxspi_attach, NULL, NULL);

static int
imxspi_cs_enable(void *arg, int slave)
{
	struct imxspi_fdt_softc * const sc = arg;
	fdtbus_gpio_write(sc->sc_pin_cs[slave], 1);
	return 0;
}

static int
imxspi_cs_disable(void *arg, int slave)
{
	struct imxspi_fdt_softc * const sc = arg;
	fdtbus_gpio_write(sc->sc_pin_cs[slave], 0);
	return 0;
}

int
imxspi_match(device_t parent, cfdata_t cf, void *aux)
{
	struct fdt_attach_args * const faa = aux;

	return of_compatible_match(faa->faa_phandle, compat_data);
}

void
imxspi_attach(device_t parent, device_t self, void *aux)
{
	struct imxspi_fdt_softc * const ifsc = device_private(self);
	struct imxspi_softc * const sc = &ifsc->sc_imxspi;
	struct fdt_attach_args * const faa = aux;
	char intrstr[128];
	const int phandle = faa->faa_phandle;
	bus_addr_t addr;
	bus_size_t size;
	int error;

	aprint_naive("\n");
	aprint_normal(": SPI\n");

	u_int nslaves;
	error = of_getprop_uint32(phandle, "fsl,spi-num-chipselects", &nslaves);
	if (error)
		nslaves = 4;

	ifsc->sc_pin_cs = kmem_alloc(sizeof(struct fdtbus_gpio_pin *) * nslaves, KM_SLEEP);

	for (int i = 0; i < nslaves; i++) {
		ifsc->sc_pin_cs[i] = fdtbus_gpio_acquire_index(phandle, "cs-gpios", i,
		    GPIO_PIN_OUTPUT);
	}

	ifsc->sc_clk = fdtbus_clock_get_index(phandle, 0);
	if (ifsc->sc_clk == NULL) {
		aprint_error(": couldn't get clock\n");
		return;
	}

	error = clk_enable(ifsc->sc_clk);
	if (error) {
		aprint_error_dev(sc->sc_dev, "couldn't enable: %d\n", error);
		return;
	}

	ifsc->sc_tag.cookie = ifsc;
	ifsc->sc_tag.spi_cs_enable = imxspi_cs_enable;
	ifsc->sc_tag.spi_cs_disable = imxspi_cs_disable;

	sc->sc_phandle = phandle;
	sc->sc_iot = faa->faa_bst;

	const struct imx_spi_config *config =
	    of_compatible_lookup(phandle, compat_data)->data;
	sc->sc_enhanced = config->enhanced;
	sc->sc_type = config->type;

	sc->sc_nslaves = nslaves;
	sc->sc_freq = clk_get_rate(ifsc->sc_clk);
	sc->sc_tag = &ifsc->sc_tag;

	if (fdtbus_get_reg(phandle, 0, &addr, &size) != 0) {
		aprint_error(": couldn't get iomux registers\n");
		return;
	}

	if (bus_space_map(sc->sc_iot, addr, size, 0, &sc->sc_ioh)) {
		aprint_error_dev(sc->sc_dev, "couldn't map registers\n");
		return;
	}

	if (!fdtbus_intr_str(faa->faa_phandle, 0, intrstr, sizeof(intrstr))) {
		aprint_error_dev(self, "failed to decode interrupt\n");
		return;
	}

	sc->sc_ih = fdtbus_intr_establish_xname(phandle, 0, IPL_VM,
	    0, imxspi_intr, &ifsc->sc_imxspi, device_xname(self));
	if (sc->sc_ih == NULL) {
		aprint_error_dev(self, "couldn't establish interrupt on %s\n",
		    intrstr);
		return;
	}
	aprint_normal_dev(self, "interrupting on %s\n", intrstr);

	imxspi_attach_common(self);
}
