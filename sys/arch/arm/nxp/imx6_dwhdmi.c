/* $NetBSD: imx6_dwhdmi.c,v 1.3 2021/12/19 11:25:32 riastradh Exp $ */
/*-
 * Copyright (c) 2020 Genetec Corporation.  All rights reserved.
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
__KERNEL_RCSID(0, "$NetBSD: imx6_dwhdmi.c,v 1.3 2021/12/19 11:25:32 riastradh Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/intr.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/conf.h>

#include <drm/drm_crtc_helper.h>

#include <dev/fdt/fdtvar.h>
#include <dev/fdt/fdt_port.h>
#include <dev/fdt/syscon.h>

#include <dev/ic/dw_hdmi.h>

static const struct dwhdmi_mpll_config imx6_dwhdmi_mpll_config[] = {
	{ 45250,	0x01e0, 0x0000, 0x091c },
	{ 92500,	0x0072, 0x0001, 0x06dc },
	{ 148500,	0x0051, 0x0002, 0x091c },
	{ 216000,	0x00a0, 0x000a, 0x06dc },
	{ 0,		0x0000, 0x0000, 0x0000 },
};

static const struct dwhdmi_phy_config imx6_dwhdmi_phy_config[] = {
	{ 216000,	0x800d, 0x000a, 0x01ad },
	{ 0,		0x0000, 0x0000, 0x0000 }
};

enum {
	DWHDMI_PORT_INPUT = 0,
	DWHDMI_PORT_OUTPUT = 1,
};

static const struct device_compatible_entry compat_data[] = {
	{ .compat = "fsl,imx6dl-hdmi" },
	{ .compat = "fsl,imx6q-hdmi" },
	DEVICE_COMPAT_EOL
};

struct imx6_dwhdmi_softc {
	struct dwhdmi_softc	sc_base;
	int			sc_phandle;

	struct fdt_device_ports	sc_ports;
	struct drm_display_mode	sc_curmode;
	struct drm_encoder	sc_encoder;

	bool			sc_activated;
};

#define	to_imx6_dwhdmi_softc(x)	container_of(x, struct imx6_dwhdmi_softc, sc_base)
#define	to_imx6_dwhdmi_encoder(x)	container_of(x, struct imx6_dwhdmi_softc, sc_encoder)

static bool
imx6_dwhdmi_encoder_mode_fixup(struct drm_encoder *encoder,
    const struct drm_display_mode *mode, struct drm_display_mode *adjusted_mode)
{
	return true;
}

static void
imx6_dwhdmi_encoder_mode_set(struct drm_encoder *encoder,
    struct drm_display_mode *mode, struct drm_display_mode *adjusted)
{
}

static void
imx6_dwhdmi_encoder_enable(struct drm_encoder *encoder)
{
}

static void
imx6_dwhdmi_encoder_disable(struct drm_encoder *encoder)
{
}

static void
imx6_dwhdmi_encoder_prepare(struct drm_encoder *encoder)
{
}

static void
imx6_dwhdmi_encoder_commit(struct drm_encoder *encoder)
{
}

static const struct drm_encoder_funcs imx6_dwhdmi_encoder_funcs = {
	.destroy = drm_encoder_cleanup,
};

static const struct drm_encoder_helper_funcs imx6_dwhdmi_encoder_helper_funcs = {
	.prepare = imx6_dwhdmi_encoder_prepare,
	.mode_fixup = imx6_dwhdmi_encoder_mode_fixup,
	.mode_set = imx6_dwhdmi_encoder_mode_set,
	.enable = imx6_dwhdmi_encoder_enable,
	.disable = imx6_dwhdmi_encoder_disable,
	.commit = imx6_dwhdmi_encoder_commit,
};

static int
imx6_dwhdmi_ep_activate(device_t dev, struct fdt_endpoint *ep, bool activate)
{
	struct imx6_dwhdmi_softc * const sc = device_private(dev);
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
	drm_encoder_init(crtc->dev, &sc->sc_encoder, &imx6_dwhdmi_encoder_funcs,
	    DRM_MODE_ENCODER_TMDS);
	drm_encoder_helper_add(&sc->sc_encoder, &imx6_dwhdmi_encoder_helper_funcs);

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
imx6_dwhdmi_ep_get_data(device_t dev, struct fdt_endpoint *ep)
{
	struct imx6_dwhdmi_softc * const sc = device_private(dev);

	return &sc->sc_encoder;
}

static audio_dai_tag_t
imx6_dwhdmi_dai_get_tag(device_t dev, const void *data, size_t len)
{
	struct imx6_dwhdmi_softc * const sc = device_private(dev);

	if (len != 4)
		return NULL;

	return &sc->sc_base.sc_dai;
}

static struct fdtbus_dai_controller_func imx6_dwhdmi_dai_funcs = {
	.get_tag = imx6_dwhdmi_dai_get_tag
};

static int
imx6_dwhdmi_match(device_t parent, cfdata_t cf, void *aux)
{
	struct fdt_attach_args * const faa = aux;

	return of_compatible_match(faa->faa_phandle, compat_data);
}

static void
imx6_dwhdmi_attach(device_t parent, device_t self, void *aux)
{
	struct imx6_dwhdmi_softc * const sc = device_private(self);
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

	sc->sc_base.sc_dev = self;
	sc->sc_base.sc_reg_width = 1;
	sc->sc_base.sc_bst = faa->faa_bst;
	if (bus_space_map(sc->sc_base.sc_bst, addr, size, 0, &sc->sc_base.sc_bsh) != 0) {
		aprint_error(": couldn't map registers\n");
		return;
	}
	sc->sc_phandle = faa->faa_phandle;

	aprint_naive("\n");
	aprint_normal(": HDMI TX\n");

	sc->sc_base.sc_ic = fdtbus_i2c_acquire(phandle, "ddc-i2c-bus");
	if (of_hasprop(phandle, "ddc-i2c-bus") && sc->sc_base.sc_ic == NULL) {
		aprint_error_dev(self, "couldn't find external I2C master\n");
		return;
	}

	sc->sc_base.sc_flags |= DWHDMI_USE_INTERNAL_PHY;
	sc->sc_base.sc_detect = dwhdmi_phy_detect;
	sc->sc_base.sc_enable = dwhdmi_phy_enable;
	sc->sc_base.sc_disable = dwhdmi_phy_disable;
	sc->sc_base.sc_mode_set = dwhdmi_phy_mode_set;
	sc->sc_base.sc_mpll_config = imx6_dwhdmi_mpll_config;
	sc->sc_base.sc_phy_config = imx6_dwhdmi_phy_config;

	if (dwhdmi_attach(&sc->sc_base) != 0) {
		aprint_error_dev(self, "failed to attach driver\n");
		return;
	}

	sc->sc_ports.dp_ep_activate = imx6_dwhdmi_ep_activate;
	sc->sc_ports.dp_ep_get_data = imx6_dwhdmi_ep_get_data;
	fdt_ports_register(&sc->sc_ports, self, phandle, EP_DRM_ENCODER);

	fdtbus_register_dai_controller(self, phandle, &imx6_dwhdmi_dai_funcs);
}

CFATTACH_DECL_NEW(imx6_dwhdmi, sizeof(struct imx6_dwhdmi_softc),
	imx6_dwhdmi_match, imx6_dwhdmi_attach, NULL, NULL);
