/* $NetBSD: rk_anxdp.c,v 1.6 2021/12/19 12:43:37 riastradh Exp $ */

/*-
 * Copyright (c) 2019 Jonathan A. Kollasch <jakllsch@kollasch.net>
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
__KERNEL_RCSID(0, "$NetBSD: rk_anxdp.c,v 1.6 2021/12/19 12:43:37 riastradh Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/intr.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/conf.h>

#include <drm/drm_drv.h>
#include <drm/drm_crtc_helper.h>

#include <dev/fdt/fdtvar.h>
#include <dev/fdt/fdt_port.h>
#include <dev/fdt/syscon.h>

#include <dev/ic/anx_dp.h>

#define	RK3399_GRF_SOC_CON20		0x6250
#define  EDP_LCDC_SEL			__BIT(5)

enum {
	ANXDP_PORT_INPUT = 0,
	ANXDP_PORT_OUTPUT = 1,
};

static const struct device_compatible_entry compat_data[] = {
	{ .compat = "rockchip,rk3399-edp" },
	DEVICE_COMPAT_EOL
};

struct rk_anxdp_softc {
	struct anxdp_softc	sc_base;
	int			sc_phandle;

	struct fdt_device_ports	sc_ports;
	struct drm_encoder	sc_encoder;
	struct drm_display_mode	sc_curmode;
	struct syscon		*sc_grf;

	bool			sc_activated;
};

#define	to_rk_anxdp_softc(x)	container_of(x, struct rk_anxdp_softc, sc_base)
#define	to_rk_anxdp_encoder(x)	container_of(x, struct rk_anxdp_softc, sc_encoder)

static void
rk_anxdp_select_input(struct rk_anxdp_softc *sc, u_int crtc_index)
{
	const uint32_t write_mask = EDP_LCDC_SEL << 16;
	const uint32_t write_val = crtc_index == 0 ? EDP_LCDC_SEL : 0;

	syscon_lock(sc->sc_grf);
	syscon_write_4(sc->sc_grf, RK3399_GRF_SOC_CON20, write_mask | write_val);
	syscon_unlock(sc->sc_grf);
}

static bool
rk_anxdp_encoder_mode_fixup(struct drm_encoder *encoder,
    const struct drm_display_mode *mode, struct drm_display_mode *adjusted_mode)
{
	return true;
}

static void
rk_anxdp_encoder_mode_set(struct drm_encoder *encoder,
    struct drm_display_mode *mode, struct drm_display_mode *adjusted)
{
}

static void
rk_anxdp_encoder_enable(struct drm_encoder *encoder)
{
}

static void
rk_anxdp_encoder_disable(struct drm_encoder *encoder)
{
}

static void
rk_anxdp_encoder_prepare(struct drm_encoder *encoder)
{
	struct rk_anxdp_softc * const sc = to_rk_anxdp_encoder(encoder);
	const u_int crtc_index = drm_crtc_index(encoder->crtc);

	rk_anxdp_select_input(sc, crtc_index);
}

static const struct drm_encoder_funcs rk_anxdp_encoder_funcs = {
	.destroy = drm_encoder_cleanup,
};

static const struct drm_encoder_helper_funcs rk_anxdp_encoder_helper_funcs = {
	.prepare = rk_anxdp_encoder_prepare,
	.mode_fixup = rk_anxdp_encoder_mode_fixup,
	.mode_set = rk_anxdp_encoder_mode_set,
	.enable = rk_anxdp_encoder_enable,
	.disable = rk_anxdp_encoder_disable,
};

static int
rk_anxdp_ep_activate(device_t dev, struct fdt_endpoint *ep, bool activate)
{
	struct rk_anxdp_softc * const sc = device_private(dev);
	struct fdt_endpoint *in_ep = fdt_endpoint_remote(ep);
	struct fdt_endpoint *out_ep, *out_rep;
	struct drm_crtc *crtc;
	int error;

	if (sc->sc_activated != false) {
		return 0;
	}

	if (!activate)
		return EINVAL;

	if (fdt_endpoint_port_index(ep) != ANXDP_PORT_INPUT)
		return EINVAL;

	switch (fdt_endpoint_type(in_ep)) {
	case EP_DRM_CRTC:
		crtc = fdt_endpoint_get_data(in_ep);
		break;
	default:
		return EINVAL;
		break;
	}

	sc->sc_encoder.possible_crtcs = 0x2; /* VOPB only */
	drm_encoder_init(crtc->dev, &sc->sc_encoder, &rk_anxdp_encoder_funcs,
	    DRM_MODE_ENCODER_TMDS, NULL);
	drm_encoder_helper_add(&sc->sc_encoder, &rk_anxdp_encoder_helper_funcs);

	out_ep = fdt_endpoint_get_from_index(&sc->sc_ports, ANXDP_PORT_OUTPUT, 0);
	if (out_ep != NULL) {
		out_rep = fdt_endpoint_remote(out_ep);
		if (out_rep != NULL && fdt_endpoint_type(out_rep) == EP_DRM_PANEL)
			sc->sc_base.sc_panel = fdt_endpoint_get_data(out_rep);
	}

        sc->sc_base.sc_connector.base.connector_type = DRM_MODE_CONNECTOR_eDP;                                                
	error = anxdp_bind(&sc->sc_base, &sc->sc_encoder);
	if (error != 0)
		return error;
	sc->sc_activated = true;

	if (out_ep != NULL) {
		/* Ignore downstream connectors, we have our own. */
		if (out_rep != NULL && fdt_endpoint_type(out_rep) == EP_DRM_CONNECTOR)
			return 0;
		error = fdt_endpoint_activate(out_ep, activate);
		if (error != 0)
			return error;
	}

	return 0;
}

static void *
rk_anxdp_ep_get_data(device_t dev, struct fdt_endpoint *ep)
{
	struct rk_anxdp_softc * const sc = device_private(dev);

	return &sc->sc_encoder;
}

#if ANXDP_AUDIO
static audio_dai_tag_t
rk_anxdp_dai_get_tag(device_t dev, const void *data, size_t len)
{
	struct rk_anxdp_softc * const sc = device_private(dev);

	if (len != 4)
		return NULL;

	return &sc->sc_base.sc_dai;
}

static struct fdtbus_dai_controller_func rk_anxdp_dai_funcs = {
	.get_tag = rk_anxdp_dai_get_tag
};
#endif

static int
rk_anxdp_match(device_t parent, cfdata_t cf, void *aux)
{
	struct fdt_attach_args * const faa = aux;

	return of_compatible_match(faa->faa_phandle, compat_data);
}

static void
rk_anxdp_attach(device_t parent, device_t self, void *aux)
{
	struct rk_anxdp_softc * const sc = device_private(self);
	struct fdt_attach_args * const faa = aux;
	const int phandle = faa->faa_phandle;
	bus_addr_t addr;
	bus_size_t size;

	if (fdtbus_get_reg(phandle, 0, &addr, &size) != 0) {
		aprint_error(": couldn't get registers\n");
		return;
	}

	/* Required */
	if (fdtbus_clock_enable(phandle, "pclk", true) != 0) {
		aprint_error(": couldn't enable pclk clock\n");
		return;
	}

	/* Required */
	if (fdtbus_clock_enable(phandle, "dp", true) != 0) {
		aprint_error(": couldn't enable dp clock\n");
		return;
	}

	/* Optional */
	if (fdtbus_clock_enable(phandle, "grf", false) != 0) {
		aprint_error(": couldn't enable grf clock\n");
		return;
	}
	
	/* TODO: Optional phy */

	sc->sc_base.sc_dev = self;
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
	aprint_normal(": eDP TX\n");

	sc->sc_base.sc_flags |= ANXDP_FLAG_ROCKCHIP;

	if (anxdp_attach(&sc->sc_base) != 0) {
		aprint_error_dev(self, "failed to attach driver\n");
		return;
	}

	sc->sc_ports.dp_ep_activate = rk_anxdp_ep_activate;
	sc->sc_ports.dp_ep_get_data = rk_anxdp_ep_get_data;
	fdt_ports_register(&sc->sc_ports, self, phandle, EP_DRM_ENCODER);

#if ANXDP_AUDIO
	fdtbus_register_dai_controller(self, phandle, &rk_anxdp_dai_funcs);
#endif
}

CFATTACH_DECL_NEW(rk_anxdp, sizeof(struct rk_anxdp_softc),
	rk_anxdp_match, rk_anxdp_attach, NULL, NULL);
