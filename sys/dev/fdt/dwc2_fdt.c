/*	$NetBSD: dwc2_fdt.c,v 1.3 2018/08/15 07:46:15 skrll Exp $	*/

/*-
 * Copyright (c) 2013 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Nick Hudson
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
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: dwc2_fdt.c,v 1.3 2018/08/15 07:46:15 skrll Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/mutex.h>
#include <sys/bus.h>
#include <sys/workqueue.h>

#include <dev/fdt/fdtvar.h>

#include <dev/usb/usb.h>
#include <dev/usb/usbdi.h>
#include <dev/usb/usbdivar.h>
#include <dev/usb/usb_mem.h>

#include <dwc2/dwc2var.h>

#include <dwc2/dwc2.h>
#include "dwc2_core.h"

struct dwc2_fdt_softc {
	struct dwc2_softc	sc_dwc2;

	struct dwc2_core_params	sc_params;

	void			*sc_ih;
	int			sc_phandle;
};

static int dwc2_fdt_match(device_t, struct cfdata *, void *);
static void dwc2_fdt_attach(device_t, device_t, void *);
static void dwc2_fdt_deferred(device_t);

static void dwc2_fdt_rockchip_params(struct dwc2_fdt_softc *, struct dwc2_core_params *);

struct dwc2_fdt_config {
	void	(*params)(struct dwc2_fdt_softc *, struct dwc2_core_params *);
};

static const struct dwc2_fdt_config dwc2_fdt_rk3066_config = {
	.params = dwc2_fdt_rockchip_params,
};

static const struct dwc2_fdt_config dwc2_fdt_generic_config = {
};

static const struct of_compat_data compat_data[] = {
	{ "rockchip,rk3066-usb",	(uintptr_t)&dwc2_fdt_rk3066_config },
	{ "snps,dwc2",			(uintptr_t)&dwc2_fdt_generic_config },
	{ NULL }
};

CFATTACH_DECL_NEW(dwc2_fdt, sizeof(struct dwc2_fdt_softc),
    dwc2_fdt_match, dwc2_fdt_attach, NULL, NULL);

/* ARGSUSED */
static int
dwc2_fdt_match(device_t parent, struct cfdata *match, void *aux)
{
	struct fdt_attach_args * const faa = aux;

	return of_match_compat_data(faa->faa_phandle, compat_data);
}

/* ARGSUSED */
static void
dwc2_fdt_attach(device_t parent, device_t self, void *aux)
{
	struct dwc2_fdt_softc *sc = device_private(self);
	struct fdt_attach_args * const faa = aux;
	const int phandle = faa->faa_phandle;
	const struct dwc2_fdt_config *conf =
	    (void *)of_search_compatible(phandle, compat_data)->data;
	char intrstr[128];
	struct fdtbus_phy *phy;
	struct clk *clk;
	bus_addr_t addr;
	bus_size_t size;
	int error;

	const char *dr_mode = fdtbus_get_string(phandle, "dr_mode");
	if (dr_mode == NULL || strcmp(dr_mode, "host") != 0) {
		aprint_error(": mode '%s' not supported\n", dr_mode);
		return;
	}

	if (fdtbus_get_reg(phandle, 0, &addr, &size) != 0) {
		aprint_error(": couldn't get registers\n");
		return;
	}

	clk = fdtbus_clock_get(phandle, "otg");
	if (clk == NULL || clk_enable(clk) != 0) {
		aprint_error(": couldn't enable otg clock\n");
		return;
	}

	/* Enable optional phy */
	phy = fdtbus_phy_get(phandle, "usb2-phy");
	if (phy && fdtbus_phy_enable(phy, true) != 0) {
		aprint_error(": couldn't enable phy\n");
		return;
	}

	sc->sc_phandle = phandle;
	sc->sc_dwc2.sc_dev = self;
	sc->sc_dwc2.sc_iot = faa->faa_bst;
	sc->sc_dwc2.sc_bus.ub_dmatag = faa->faa_dmat;

	error = bus_space_map(faa->faa_bst, addr, size, 0, &sc->sc_dwc2.sc_ioh);
	if (error) {
		aprint_error(": couldn't map device\n");
		return;
	}

	if (conf->params) {
		conf->params(sc, &sc->sc_params);
		sc->sc_dwc2.sc_params = &sc->sc_params;
	}

	if (!fdtbus_intr_str(phandle, 0, intrstr, sizeof(intrstr))) {
		aprint_error(": failed to decode interrupt\n");
		return;
	}

	aprint_naive("\n");
	aprint_normal(": DesignWare USB2 OTG\n");

	sc->sc_ih = fdtbus_intr_establish(phandle, 0, IPL_VM, FDT_INTR_MPSAFE,
	    dwc2_intr, &sc->sc_dwc2);

	if (sc->sc_ih == NULL) {
		aprint_error_dev(self, "failed to establish interrupt %s\n",
		    intrstr);
		goto fail;
	}
	aprint_normal_dev(self, "interrupting on %s\n", intrstr);
	config_interrupts(self, dwc2_fdt_deferred);

	return;

fail:
	if (sc->sc_ih) {
		fdtbus_intr_disestablish(sc->sc_phandle, sc->sc_ih);
		sc->sc_ih = NULL;
	}
	bus_space_unmap(sc->sc_dwc2.sc_iot, sc->sc_dwc2.sc_ioh, size);
}

static void
dwc2_fdt_deferred(device_t self)
{
	struct dwc2_fdt_softc *sc = device_private(self);
	int error;

	error = dwc2_init(&sc->sc_dwc2);
	if (error != 0) {
		aprint_error_dev(self, "couldn't initialize host, error=%d\n",
		    error);
		return;
	}
	sc->sc_dwc2.sc_child = config_found(sc->sc_dwc2.sc_dev,
	    &sc->sc_dwc2.sc_bus, usbctlprint);
}

static void
dwc2_fdt_rockchip_params(struct dwc2_fdt_softc *sc, struct dwc2_core_params *params)
{
	dwc2_set_all_params(params, -1);

	params->otg_cap = DWC2_CAP_PARAM_NO_HNP_SRP_CAPABLE;
	params->host_rx_fifo_size = 525;
	params->host_nperio_tx_fifo_size = 128;
	params->host_perio_tx_fifo_size = 256;
	params->ahbcfg = GAHBCFG_HBSTLEN_INCR16 << GAHBCFG_HBSTLEN_SHIFT;
}
