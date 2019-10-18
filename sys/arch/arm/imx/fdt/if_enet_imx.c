/*	$NetBSD: if_enet_imx.c,v 1.4 2019/10/18 12:53:08 hkenken Exp $	*/
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
__KERNEL_RCSID(0, "$NetBSD: if_enet_imx.c,v 1.4 2019/10/18 12:53:08 hkenken Exp $");

#include "opt_fdt.h"

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>

#include <arm/imx/imx6var.h>
#include <arm/imx/imx6_reg.h>
#include <arm/imx/if_enetreg.h>
#include <arm/imx/if_enetvar.h>

#include <dev/fdt/fdtvar.h>

struct enet_fdt_softc {
	struct enet_softc sc_enet;

	struct fdtbus_gpio_pin *sc_pin_reset;
};

CFATTACH_DECL_NEW(enet_fdt, sizeof(struct enet_fdt_softc),
    enet_match, enet_attach, NULL, NULL);

static const char * const compatible[] = {
	"fsl,imx6q-fec",
	NULL
};

static int enet_init_clocks(struct enet_softc *);
static void enet_phy_reset(struct enet_fdt_softc *, const int);

int
enet_match(device_t parent, cfdata_t cf, void *aux)
{
	struct fdt_attach_args * const faa = aux;

	return of_match_compatible(faa->faa_phandle, compatible);
}

void
enet_attach(device_t parent, device_t self, void *aux)
{
	struct enet_fdt_softc * const efsc = device_private(self);
	struct enet_softc *sc = &efsc->sc_enet;
	struct fdt_attach_args * const faa = aux;
	prop_dictionary_t prop = device_properties(self);
	const int phandle = faa->faa_phandle;
	bus_space_tag_t bst = faa->faa_bst;
	bus_space_handle_t bsh;
	bus_addr_t addr;
	bus_size_t size;
	int error;

	if (fdtbus_get_reg(phandle, 0, &addr, &size) != 0) {
		aprint_error(": couldn't get enet registers\n");
		return;
	}

	error = bus_space_map(bst, addr, size, 0, &bsh);
	if (error) {
		aprint_error(": couldn't map enet registers: %d\n", error);
		return;
	}

	sc->sc_clk_enet = fdtbus_clock_get(phandle, "ahb");
	if (sc->sc_clk_enet == NULL) {
		aprint_error(": couldn't get clock ahb\n");
		goto failure;
	}
	sc->sc_clk_enet_ref= fdtbus_clock_get(phandle, "ptp");
	if (sc->sc_clk_enet_ref == NULL) {
		aprint_error(": couldn't get clock ptp\n");
		goto failure;
	}

	aprint_naive("\n");
	aprint_normal(": Gigabit Ethernet Controller\n");

	sc->sc_dev = self;
	sc->sc_iot = bst;
	sc->sc_ioh = bsh;
	sc->sc_dmat = faa->faa_dmat;

	sc->sc_imxtype = 6;	/* i.MX6 */
	sc->sc_unit = 0;

	const char *phy_mode = fdtbus_get_string(phandle, "phy-mode");
	if (phy_mode == NULL) {
		aprint_error(": missing 'phy-mode' property\n");
		goto failure;
	}

	if (strcmp(phy_mode, "rgmii-txid") == 0) {
		prop_dictionary_set_bool(prop, "tx_internal_delay", true);
		sc->sc_rgmii = 1;
	} else if (strcmp(phy_mode, "rgmii-rxid") == 0) {
		prop_dictionary_set_bool(prop, "rx_internal_delay", true);
		sc->sc_rgmii = 1;
	} else if (strcmp(phy_mode, "rgmii-id") == 0) {
		prop_dictionary_set_bool(prop, "tx_internal_delay", true);
		prop_dictionary_set_bool(prop, "rx_internal_delay", true);
		sc->sc_rgmii = 1;
	} else if (strcmp(phy_mode, "rgmii") == 0) {
		sc->sc_rgmii = 1;
	} else {
		sc->sc_rgmii = 0;
	}

	char intrstr[128];
	if (!fdtbus_intr_str(phandle, 0, intrstr, sizeof(intrstr))) {
		aprint_error_dev(self, "failed to decode interrupt\n");
		goto failure;
	}
	sc->sc_ih = fdtbus_intr_establish(phandle, 0, IPL_NET,
	    FDT_INTR_MPSAFE, enet_intr, sc);
	if (sc->sc_ih == NULL) {
		aprint_error_dev(self, "failed to establish interrupt on %s\n",
		    intrstr);
		goto failure;
	}
	aprint_normal_dev(self, "interrupting on %s\n", intrstr);

	enet_init_clocks(sc);
	sc->sc_pllclock = clk_get_rate(sc->sc_clk_enet_ref);

	enet_phy_reset(efsc, phandle);

	if (enet_attach_common(self) != 0)
		goto failure;

	return;

failure:
	bus_space_unmap(sc->sc_iot, sc->sc_ioh, size);
	return;
}

static int
enet_init_clocks(struct enet_softc *sc)
{
	int error;

	error = clk_enable(sc->sc_clk_enet);
	if (error) {
		aprint_error_dev(sc->sc_dev, "couldn't enable enet: %d\n", error);
		return error;
	}
	error = clk_enable(sc->sc_clk_enet_ref);
	if (error) {
		aprint_error_dev(sc->sc_dev, "couldn't enable enet_ref: %d\n", error);
		return error;
	}

	return 0;
}

static void
enet_phy_reset(struct enet_fdt_softc *sc, const int phandle)
{
	int error;

	sc->sc_pin_reset = fdtbus_gpio_acquire(phandle, "phy-reset-gpios", GPIO_PIN_OUTPUT);
	if (sc->sc_pin_reset == NULL)
		return;

	u_int msec;
	error = of_getprop_uint32(phandle, "phy-reset-duration", &msec);
	if (error)
		msec = 1;

	/* Reset */
	fdtbus_gpio_write(sc->sc_pin_reset, 1);
	delay(msec * 1000);
	fdtbus_gpio_write(sc->sc_pin_reset, 0);
}
