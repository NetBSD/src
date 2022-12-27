/*	$NetBSD: if_enet_imx.c,v 1.7 2022/12/27 18:55:06 mrg Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: if_enet_imx.c,v 1.7 2022/12/27 18:55:06 mrg Exp $");

#include "opt_fdt.h"

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>

#include <arm/imx/if_enetreg.h>
#include <arm/imx/if_enetvar.h>

#include <dev/fdt/fdtvar.h>

struct enet_fdt_softc {
	struct enet_softc sc_enet;

	struct fdtbus_gpio_pin *sc_pin_reset;
};

CFATTACH_DECL_NEW(enet_fdt, sizeof(struct enet_fdt_softc),
    enet_match, enet_attach, NULL, NULL);

static const struct device_compatible_entry compat_data[] = {
	/* compatible			imxtype */
	{ .compat = "fsl,imx6q-fec",	.value = 6 },
	{ .compat = "fsl,imx6sx-fec",	.value = 7 },
	DEVICE_COMPAT_EOL
};

static int enet_init_clocks(struct enet_softc *);
static void enet_phy_reset(struct enet_fdt_softc *, const int);
static int enet_phy_id(struct enet_softc *, const int);
static void *enet_intr_establish(struct enet_softc *, int, u_int);

int
enet_match(device_t parent, cfdata_t cf, void *aux)
{
	struct fdt_attach_args * const faa = aux;

	return of_compatible_match(faa->faa_phandle, compat_data);
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

	sc->sc_clk_ipg = fdtbus_clock_get(phandle, "ipg");
	if (sc->sc_clk_ipg == NULL) {
		aprint_error(": couldn't get clock ipg\n");
		goto failure;
	}
	sc->sc_clk_enet = fdtbus_clock_get(phandle, "ahb");
	if (sc->sc_clk_enet == NULL) {
		aprint_error(": couldn't get clock ahb\n");
		goto failure;
	}
	sc->sc_clk_enet_ref = fdtbus_clock_get(phandle, "ptp");
	if (sc->sc_clk_enet_ref == NULL) {
		aprint_error(": couldn't get clock ptp\n");
		goto failure;
	}

	if (fdtbus_clock_enable(phandle, "enet_clk_ref", false) != 0) {
		aprint_error(": couldn't enable clock enet_clk_ref\n");
		goto failure;
	}
	if (fdtbus_clock_enable(phandle, "enet_out", false) != 0) {
		aprint_error(": couldn't enable clock enet_out\n");
		goto failure;
	}

	aprint_naive("\n");
	aprint_normal(": Gigabit Ethernet Controller\n");

	sc->sc_dev = self;
	sc->sc_iot = bst;
	sc->sc_ioh = bsh;
	sc->sc_dmat = faa->faa_dmat;

	sc->sc_imxtype = of_compatible_lookup(phandle, compat_data)->value;
	sc->sc_unit = 0;
	sc->sc_phyid = enet_phy_id(sc, phandle);

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

	sc->sc_ih = enet_intr_establish(sc, phandle, 0);
	if (sc->sc_ih == NULL)
		goto failure;

	if (sc->sc_imxtype == 7) {
		sc->sc_ih2 = enet_intr_establish(sc, phandle, 1);
		sc->sc_ih3 = enet_intr_establish(sc, phandle, 2);
		if (sc->sc_ih2 == NULL || sc->sc_ih3 == NULL)
			goto failure;
	}

	enet_init_clocks(sc);
	sc->sc_clock = clk_get_rate(sc->sc_clk_ipg);

	enet_phy_reset(efsc, phandle);

	if (enet_attach_common(self) != 0)
		goto failure;

	return;

failure:
	bus_space_unmap(bst, bsh, size);
	return;
}

static void *
enet_intr_establish(struct enet_softc *sc, int phandle, u_int index)
{
	char intrstr[128];
	char xname[16];
	void *ih;

	if (!fdtbus_intr_str(phandle, index, intrstr, sizeof(intrstr))) {
		aprint_error_dev(sc->sc_dev, "failed to decode interrupt %d\n",
		    index);
		return NULL;
	}

	snprintf(xname, sizeof(xname), "%s #%u", device_xname(sc->sc_dev),
	    index);
	ih = fdtbus_intr_establish_xname(phandle, index, IPL_NET, 0,
	    enet_intr, sc, xname);
	if (ih == NULL) {
		aprint_error_dev(sc->sc_dev, "failed to establish interrupt on %s\n",
		    intrstr);
		return NULL;
	}
	aprint_normal_dev(sc->sc_dev, "interrupting on %s\n", intrstr);

	return ih;
}

static int
enet_init_clocks(struct enet_softc *sc)
{
	int error;

	error = clk_enable(sc->sc_clk_ipg);
	if (error) {
		aprint_error_dev(sc->sc_dev, "couldn't enable ipg: %d\n", error);
		return error;
	}
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
	u_int msec;

	sc->sc_pin_reset = fdtbus_gpio_acquire(phandle, "phy-reset-gpios", GPIO_PIN_OUTPUT);
	if (sc->sc_pin_reset == NULL) {
		aprint_error_dev(sc->sc_enet.sc_dev, "couldn't find phy reset gpios\n");
		return;
	}

	if (of_getprop_uint32(phandle, "phy-reset-duration", &msec))
		msec = 1;

	/* Reset */
	fdtbus_gpio_write(sc->sc_pin_reset, 1);
	delay(msec * 1000);
	fdtbus_gpio_write(sc->sc_pin_reset, 0);

	/* Post delay */
	if (of_getprop_uint32(phandle, "phy-reset-post-delay", &msec))
		msec = 0;

	delay(msec * 1000);
}

static int
enet_phy_id(struct enet_softc *sc, const int phandle)
{
	int phy_phandle;
	bus_addr_t addr;

	phy_phandle = fdtbus_get_phandle(phandle, "phy-handle");
	if (phy_phandle == -1)
		return MII_PHY_ANY;

	if (fdtbus_get_reg(phy_phandle, 0, &addr, NULL) != 0)
		return MII_PHY_ANY;

	return (int)addr;
}
