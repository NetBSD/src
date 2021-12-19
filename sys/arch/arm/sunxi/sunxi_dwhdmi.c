/* $NetBSD: sunxi_dwhdmi.c,v 1.11 2021/12/19 11:01:10 riastradh Exp $ */

/*-
 * Copyright (c) 2019 Jared D. McNeill <jmcneill@invisible.ca>
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
__KERNEL_RCSID(0, "$NetBSD: sunxi_dwhdmi.c,v 1.11 2021/12/19 11:01:10 riastradh Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/conf.h>
#include <sys/device.h>
#include <sys/intr.h>
#include <sys/kernel.h>
#include <sys/systm.h>

#include <dev/fdt/fdt_port.h>
#include <dev/fdt/fdtvar.h>

#include <dev/ic/dw_hdmi.h>

#include <arm/sunxi/sunxi_hdmiphy.h>

#include <drm/drm_drv.h>

enum {
	DWHDMI_PORT_INPUT = 0,
	DWHDMI_PORT_OUTPUT = 1,
};

static const struct device_compatible_entry compat_data[] = {
	{ .compat = "allwinner,sun8i-h3-dw-hdmi" },
	{ .compat = "allwinner,sun50i-a64-dw-hdmi" },
	DEVICE_COMPAT_EOL
};

struct sunxi_dwhdmi_softc {
	struct dwhdmi_softc	sc_base;
	int			sc_phandle;
	struct fdtbus_phy	*sc_phy;
	struct fdtbus_regulator	*sc_regulator;
	struct clk		*sc_clk;

	struct fdt_device_ports	sc_ports;
	struct drm_display_mode	sc_curmode;
};

#define	to_sunxi_dwhdmi_softc(x)	container_of(x, struct sunxi_dwhdmi_softc, sc_base)

static int
sunxi_dwhdmi_ep_activate(device_t dev, struct fdt_endpoint *ep, bool activate)
{
	struct sunxi_dwhdmi_softc * const sc = device_private(dev);
	struct fdt_endpoint *in_ep = fdt_endpoint_remote(ep);
	struct fdt_endpoint *out_ep, *out_rep;
	struct drm_encoder *encoder;
	struct drm_bridge *bridge;
	int error;

	if (!activate)
		return EINVAL;

	if (fdt_endpoint_port_index(ep) != DWHDMI_PORT_INPUT)
		return EINVAL;

	switch (fdt_endpoint_type(in_ep)) {
	case EP_DRM_ENCODER:
		encoder = fdt_endpoint_get_data(in_ep);
		break;
	case EP_DRM_BRIDGE:
		bridge = fdt_endpoint_get_data(in_ep);
		encoder = bridge->encoder;
		break;
	default:
		encoder = NULL;
		break;
	}

	if (encoder == NULL)
		return EINVAL;

	if (sc->sc_regulator != NULL) {
		error = fdtbus_regulator_enable(sc->sc_regulator);
		if (error != 0) {
			device_printf(dev, "couldn't enable supply\n");
			return error;
		}
	}

	error = dwhdmi_bind(&sc->sc_base, encoder);
	if (error != 0)
		return error;

	out_ep = fdt_endpoint_get_from_index(&sc->sc_ports, DWHDMI_PORT_OUTPUT, 0);
	if (out_ep != NULL) {
		/* Ignore downstream connectors, we have our own. */
		out_rep = fdt_endpoint_remote(out_ep);
		if (out_rep != NULL && fdt_endpoint_type(out_rep) == EP_DRM_CONNECTOR)
			return 0;

		error = fdt_endpoint_activate(out_ep, activate);
		if (error != 0)
			return error;
	}

	return 0;
}

static void *
sunxi_dwhdmi_ep_get_data(device_t dev, struct fdt_endpoint *ep)
{
	struct sunxi_dwhdmi_softc * const sc = device_private(dev);

	return &sc->sc_base.sc_bridge;
}

static enum drm_connector_status
sunxi_dwhdmi_detect(struct dwhdmi_softc *dsc, bool force)
{
	struct sunxi_dwhdmi_softc * const sc = to_sunxi_dwhdmi_softc(dsc);

	KASSERT(sc->sc_phy != NULL);

	if (sunxi_hdmiphy_detect(sc->sc_phy, force))
		return connector_status_connected;
	else
		return connector_status_disconnected;
}

static void
sunxi_dwhdmi_enable(struct dwhdmi_softc *dsc)
{
	struct sunxi_dwhdmi_softc * const sc = to_sunxi_dwhdmi_softc(dsc);
	int error;

	KASSERT(sc->sc_phy != NULL);

	error = fdtbus_phy_enable(sc->sc_phy, true);
	if (error != 0) {
		device_printf(dsc->sc_dev, "failed to enable phy: %d\n", error);
		return;
	}

	error = sunxi_hdmiphy_config(sc->sc_phy, &sc->sc_curmode);
	if (error != 0)
		device_printf(dsc->sc_dev, "failed to configure phy: %d\n", error);
}

static void
sunxi_dwhdmi_disable(struct dwhdmi_softc *dsc)
{
	struct sunxi_dwhdmi_softc * const sc = to_sunxi_dwhdmi_softc(dsc);
	int error;

	KASSERT(sc->sc_phy != NULL);

	error = fdtbus_phy_enable(sc->sc_phy, false);
	if (error != 0)
		device_printf(dsc->sc_dev, "failed to disable phy\n");
}

static void
sunxi_dwhdmi_mode_set(struct dwhdmi_softc *dsc,
    const struct drm_display_mode *mode,
    const struct drm_display_mode *adjusted_mode)
{
	struct sunxi_dwhdmi_softc * const sc = to_sunxi_dwhdmi_softc(dsc);
	int error;

	if (sc->sc_clk != NULL) {
		error = clk_set_rate(sc->sc_clk, adjusted_mode->clock * 1000);
		if (error != 0)
			device_printf(sc->sc_base.sc_dev,
			    "couldn't set pixel clock to %u Hz: %d\n",
			    adjusted_mode->clock * 1000, error);
	}

	sc->sc_curmode = *adjusted_mode;
}

static audio_dai_tag_t
sunxi_dwhdmi_dai_get_tag(device_t dev, const void *data, size_t len)
{
	struct sunxi_dwhdmi_softc * const sc = device_private(dev);

	if (len != 4)
		return NULL;

	return &sc->sc_base.sc_dai;
}

static struct fdtbus_dai_controller_func sunxi_dwhdmi_dai_funcs = {
	.get_tag = sunxi_dwhdmi_dai_get_tag
};

static int
sunxi_dwhdmi_match(device_t parent, cfdata_t cf, void *aux)
{
	struct fdt_attach_args * const faa = aux;

	return of_compatible_match(faa->faa_phandle, compat_data);
}

static void
sunxi_dwhdmi_attach(device_t parent, device_t self, void *aux)
{
	struct sunxi_dwhdmi_softc * const sc = device_private(self);
	struct fdt_attach_args * const faa = aux;
	prop_dictionary_t prop = device_properties(self);
	const int phandle = faa->faa_phandle;
	struct clk *clk_iahb, *clk_isfr, *clk_tmds;
	struct fdtbus_reset *rst;
	bool is_disabled;
	bus_addr_t addr;
	bus_size_t size;

	if (prop_dictionary_get_bool(prop, "disabled", &is_disabled) && is_disabled) {
		aprint_naive("\n");
		aprint_normal(": HDMI TX (disabled)\n");
		return;
	}

	if (fdtbus_get_reg(phandle, 0, &addr, &size) != 0) {
		aprint_error(": couldn't get registers\n");
		return;
	}

	clk_iahb = fdtbus_clock_get(phandle, "iahb");
	if (clk_iahb == NULL || clk_enable(clk_iahb) != 0) {
		aprint_error(": couldn't enable iahb clock\n");
		return;
	}

	clk_isfr = fdtbus_clock_get(phandle, "isfr");
	if (clk_isfr == NULL || clk_enable(clk_isfr) != 0) {
		aprint_error(": couldn't enable isfr clock\n");
		return;
	}

	clk_tmds = fdtbus_clock_get(phandle, "tmds");
	if (clk_tmds == NULL || clk_enable(clk_tmds) != 0) {
		aprint_error(": couldn't enable tmds clock\n");
		return;
	}

	sc->sc_base.sc_dev = self;
	sc->sc_base.sc_reg_width = 1;
	sc->sc_base.sc_bst = faa->faa_bst;
	if (bus_space_map(sc->sc_base.sc_bst, addr, size, 0, &sc->sc_base.sc_bsh) != 0) {
		aprint_error(": couldn't map registers\n");
		return;
	}
	sc->sc_base.sc_detect = sunxi_dwhdmi_detect;
	sc->sc_base.sc_enable = sunxi_dwhdmi_enable;
	sc->sc_base.sc_disable = sunxi_dwhdmi_disable;
	sc->sc_base.sc_mode_set = sunxi_dwhdmi_mode_set;
	sc->sc_base.sc_scl_hcnt = 0xd8;
	sc->sc_base.sc_scl_lcnt = 0xfe;
	sc->sc_phandle = faa->faa_phandle;
	sc->sc_clk = clk_tmds;

	aprint_naive("\n");
	aprint_normal(": HDMI TX\n");

	sc->sc_regulator = fdtbus_regulator_acquire(sc->sc_phandle, "hvcc-supply");

	sc->sc_phy = fdtbus_phy_get(sc->sc_phandle, "hdmi-phy");
	if (sc->sc_phy == NULL)
		sc->sc_phy = fdtbus_phy_get(sc->sc_phandle, "phy");
	if (sc->sc_phy == NULL) {
		device_printf(self, "couldn't find PHY\n");
		return;
	}

	rst = fdtbus_reset_get(phandle, "ctrl");
	if (rst == NULL || fdtbus_reset_deassert(rst) != 0) {
		aprint_error_dev(self, "couldn't de-assert reset\n");
		return;
	}

	sunxi_hdmiphy_init(sc->sc_phy);

	if (dwhdmi_attach(&sc->sc_base) != 0) {
		aprint_error_dev(self, "failed to attach driver\n");
		return;
	}

	sc->sc_ports.dp_ep_activate = sunxi_dwhdmi_ep_activate;
	sc->sc_ports.dp_ep_get_data = sunxi_dwhdmi_ep_get_data;
	fdt_ports_register(&sc->sc_ports, self, phandle, EP_DRM_BRIDGE);

	fdtbus_register_dai_controller(self, phandle, &sunxi_dwhdmi_dai_funcs);
}

CFATTACH_DECL_NEW(sunxi_dwhdmi, sizeof(struct sunxi_dwhdmi_softc),
	sunxi_dwhdmi_match, sunxi_dwhdmi_attach, NULL, NULL);
