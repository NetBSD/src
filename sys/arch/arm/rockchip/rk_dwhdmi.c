/* $NetBSD: rk_dwhdmi.c,v 1.7 2021/12/19 11:01:10 riastradh Exp $ */

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
__KERNEL_RCSID(0, "$NetBSD: rk_dwhdmi.c,v 1.7 2021/12/19 11:01:10 riastradh Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/conf.h>
#include <sys/device.h>
#include <sys/intr.h>
#include <sys/kernel.h>
#include <sys/systm.h>

#include <dev/fdt/fdt_port.h>
#include <dev/fdt/fdtvar.h>
#include <dev/fdt/syscon.h>

#include <dev/ic/dw_hdmi.h>

#include <drm/drm_drv.h>
#include <drm/drm_crtc_helper.h>

#define	RK3399_GRF_SOC_CON20		0x6250
#define	 HDMI_LCDC_SEL			__BIT(6)

static const struct dwhdmi_mpll_config rk_dwhdmi_mpll_config[] = {
	{ 40000,	0x00b3, 0x0000, 0x0018 },
	{ 65000,	0x0072, 0x0001, 0x0028 },
	{ 66000,	0x013e, 0x0003, 0x0038 },
	{ 83500,	0x0072, 0x0001, 0x0028 },
	{ 146250,	0x0051, 0x0002, 0x0038 },
	{ 148500,	0x0051, 0x0003, 0x0000 },
	{ 272000,	0x0040, 0x0003, 0x0000 },
	{ 340000,	0x0040, 0x0003, 0x0000 },
	{ 0,		0x0051, 0x0003, 0x0000 },
};

static const struct dwhdmi_phy_config rk_dwhdmi_phy_config[] = {
	{ 74250,	0x8009, 0x0004, 0x0272 },
	{ 148500,	0x802b, 0x0004, 0x028d },
	{ 297000,	0x8039, 0x0005, 0x028d },
	{ 594000,	0x8039, 0x0000, 0x019d },
	{ 0,		0x0000, 0x0000, 0x0000 }
};

enum {
	DWHDMI_PORT_INPUT = 0,
	DWHDMI_PORT_OUTPUT = 1,
};

static const struct device_compatible_entry compat_data[] = {
	{ .compat = "rockchip,rk3399-dw-hdmi" },
	DEVICE_COMPAT_EOL
};

struct rk_dwhdmi_softc {
	struct dwhdmi_softc	sc_base;
	int			sc_phandle;
	struct clk		*sc_clk_vpll;

	struct fdt_device_ports	sc_ports;
	struct drm_display_mode	sc_curmode;
	struct drm_encoder	sc_encoder;
	struct syscon		*sc_grf;

	bool			sc_activated;
};

#define	to_rk_dwhdmi_softc(x)	container_of(x, struct rk_dwhdmi_softc, sc_base)
#define	to_rk_dwhdmi_encoder(x)	container_of(x, struct rk_dwhdmi_softc, sc_encoder)

static void
rk_dwhdmi_select_input(struct rk_dwhdmi_softc *sc, u_int crtc_index)
{
	const uint32_t write_mask = HDMI_LCDC_SEL << 16;
	const uint32_t write_val = crtc_index == 0 ? HDMI_LCDC_SEL : 0;

	syscon_lock(sc->sc_grf);
	syscon_write_4(sc->sc_grf, RK3399_GRF_SOC_CON20, write_mask | write_val);
	syscon_unlock(sc->sc_grf);
}

static bool
rk_dwhdmi_encoder_mode_fixup(struct drm_encoder *encoder,
    const struct drm_display_mode *mode, struct drm_display_mode *adjusted_mode)
{
	return true;
}

static void
rk_dwhdmi_encoder_mode_set(struct drm_encoder *encoder,
    struct drm_display_mode *mode, struct drm_display_mode *adjusted)
{
}

static void
rk_dwhdmi_encoder_enable(struct drm_encoder *encoder)
{
}

static void
rk_dwhdmi_encoder_disable(struct drm_encoder *encoder)
{
}

static void
rk_dwhdmi_encoder_prepare(struct drm_encoder *encoder)
{
	struct rk_dwhdmi_softc * const sc = to_rk_dwhdmi_encoder(encoder);
	const u_int crtc_index = drm_crtc_index(encoder->crtc);

	rk_dwhdmi_select_input(sc, crtc_index);
}

static void
rk_dwhdmi_encoder_commit(struct drm_encoder *encoder)
{
}

static const struct drm_encoder_funcs rk_dwhdmi_encoder_funcs = {
	.destroy = drm_encoder_cleanup,
};

static const struct drm_encoder_helper_funcs rk_dwhdmi_encoder_helper_funcs = {
	.prepare = rk_dwhdmi_encoder_prepare,
	.mode_fixup = rk_dwhdmi_encoder_mode_fixup,
	.mode_set = rk_dwhdmi_encoder_mode_set,
	.enable = rk_dwhdmi_encoder_enable,
	.disable = rk_dwhdmi_encoder_disable,
	.commit = rk_dwhdmi_encoder_commit,
};

static int
rk_dwhdmi_ep_activate(device_t dev, struct fdt_endpoint *ep, bool activate)
{
	struct rk_dwhdmi_softc * const sc = device_private(dev);
	struct fdt_endpoint *in_ep = fdt_endpoint_remote(ep);
	struct fdt_endpoint *out_ep, *out_rep;
	struct drm_crtc *crtc;
	int error;

	if (sc->sc_activated != false) {
		return 0;
	}

	if (!activate)
		return EINVAL;

	if (fdt_endpoint_port_index(ep) != DWHDMI_PORT_INPUT)
		return EINVAL;

	switch (fdt_endpoint_type(in_ep)) {
	case EP_DRM_CRTC:
		crtc = fdt_endpoint_get_data(in_ep);
		break;
	default:
		crtc = NULL;
		break;
	}

	if (crtc == NULL)
		return EINVAL;

	sc->sc_encoder.possible_crtcs = 3; // 1U << drm_crtc_index(crtc); /* XXX */
	drm_encoder_init(crtc->dev, &sc->sc_encoder, &rk_dwhdmi_encoder_funcs,
	    DRM_MODE_ENCODER_TMDS, NULL);
	drm_encoder_helper_add(&sc->sc_encoder, &rk_dwhdmi_encoder_helper_funcs);

	sc->sc_base.sc_connector.base.connector_type = DRM_MODE_CONNECTOR_HDMIA;
	error = dwhdmi_bind(&sc->sc_base, &sc->sc_encoder);
	if (error != 0)
		return error;
	sc->sc_activated = true;

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
rk_dwhdmi_ep_get_data(device_t dev, struct fdt_endpoint *ep)
{
	struct rk_dwhdmi_softc * const sc = device_private(dev);

	return &sc->sc_encoder;
}

static void
rk_dwhdmi_enable(struct dwhdmi_softc *dsc)
{

	dwhdmi_phy_enable(dsc);
}

static void
rk_dwhdmi_mode_set(struct dwhdmi_softc *dsc,
    const struct drm_display_mode *mode, const struct drm_display_mode *adjusted_mode)
{
	struct rk_dwhdmi_softc * const sc = to_rk_dwhdmi_softc(dsc);
	int error;

	if (sc->sc_clk_vpll != NULL) {
		error = clk_set_rate(sc->sc_clk_vpll, adjusted_mode->clock * 1000);
		if (error != 0)
			device_printf(dsc->sc_dev, "couldn't set pixel clock to %u Hz: %d\n",
			    adjusted_mode->clock * 1000, error);
	}

	dwhdmi_phy_mode_set(dsc, mode, adjusted_mode);
}

static audio_dai_tag_t
rk_dwhdmi_dai_get_tag(device_t dev, const void *data, size_t len)
{
	struct rk_dwhdmi_softc * const sc = device_private(dev);

	if (len != 4)
		return NULL;

	return &sc->sc_base.sc_dai;
}

static struct fdtbus_dai_controller_func rk_dwhdmi_dai_funcs = {
	.get_tag = rk_dwhdmi_dai_get_tag
};

static int
rk_dwhdmi_match(device_t parent, cfdata_t cf, void *aux)
{
	struct fdt_attach_args * const faa = aux;

	return of_compatible_match(faa->faa_phandle, compat_data);
}

static void
rk_dwhdmi_attach(device_t parent, device_t self, void *aux)
{
	struct rk_dwhdmi_softc * const sc = device_private(self);
	struct fdt_attach_args * const faa = aux;
	const int phandle = faa->faa_phandle;
	bus_addr_t addr;
	bus_size_t size;

	if (fdtbus_get_reg(phandle, 0, &addr, &size) != 0) {
		aprint_error(": couldn't get registers\n");
		return;
	}

	/* Required */
	if (fdtbus_clock_enable(phandle, "iahb", true) != 0) {
		aprint_error(": couldn't enable iahb clock\n");
		return;
	}

	/* Required */
	if (fdtbus_clock_enable(phandle, "isfr", true) != 0) {
		aprint_error(": couldn't enable isfr clock\n");
		return;
	}

	/* Optional */
	sc->sc_clk_vpll = fdtbus_clock_get(phandle, "vpll");
	if (sc->sc_clk_vpll != NULL && clk_enable(sc->sc_clk_vpll) != 0) {
		aprint_error(": couldn't enable vpll clock\n");
		return;
	}

	/* Optional */
	if (fdtbus_clock_enable(phandle, "grf", false) != 0) {
		aprint_error(": couldn't enable grf clock\n");
		return;
	}

	/* Optional */
	if (fdtbus_clock_enable(phandle, "cec", false) != 0) {
		aprint_error(": couldn't enable cec clock\n");
		return;
	}

	sc->sc_base.sc_dev = self;
	if (of_getprop_uint32(phandle, "reg-io-width", &sc->sc_base.sc_reg_width) != 0)
		sc->sc_base.sc_reg_width = 4;
	sc->sc_base.sc_bst = faa->faa_bst;
	if (bus_space_map(sc->sc_base.sc_bst, addr, size, 0, &sc->sc_base.sc_bsh) != 0) {
		aprint_error(": couldn't map registers\n");
		return;
	}
	sc->sc_phandle = faa->faa_phandle;
	sc->sc_grf = fdtbus_syscon_acquire(phandle, "rockchip,grf");
	if (sc->sc_grf == NULL) {
		aprint_error(": couldn't get grf syscon\n");
		return;
	}

	aprint_naive("\n");
	aprint_normal(": HDMI TX\n");

	sc->sc_base.sc_ic = fdtbus_i2c_acquire(phandle, "ddc-i2c-bus");
	if (of_hasprop(phandle, "ddc-i2c-bus") && sc->sc_base.sc_ic == NULL) {
		aprint_error_dev(self, "couldn't find external I2C master\n");
		return;
	}

	sc->sc_base.sc_flags |= DWHDMI_USE_INTERNAL_PHY;
	sc->sc_base.sc_detect = dwhdmi_phy_detect;
	sc->sc_base.sc_enable = rk_dwhdmi_enable;
	sc->sc_base.sc_disable = dwhdmi_phy_disable;
	sc->sc_base.sc_mode_set = rk_dwhdmi_mode_set;
	sc->sc_base.sc_mpll_config = rk_dwhdmi_mpll_config;
	sc->sc_base.sc_phy_config = rk_dwhdmi_phy_config;

	if (dwhdmi_attach(&sc->sc_base) != 0) {
		aprint_error_dev(self, "failed to attach driver\n");
		return;
	}

	sc->sc_ports.dp_ep_activate = rk_dwhdmi_ep_activate;
	sc->sc_ports.dp_ep_get_data = rk_dwhdmi_ep_get_data;
	fdt_ports_register(&sc->sc_ports, self, phandle, EP_DRM_ENCODER);

	fdtbus_register_dai_controller(self, phandle, &rk_dwhdmi_dai_funcs);
}

CFATTACH_DECL_NEW(rk_dwhdmi, sizeof(struct rk_dwhdmi_softc),
	rk_dwhdmi_match, rk_dwhdmi_attach, NULL, NULL);
