/* $NetBSD: sunxi_lcdc.c,v 1.5 2019/02/18 02:42:27 jakllsch Exp $ */

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
__KERNEL_RCSID(0, "$NetBSD: sunxi_lcdc.c,v 1.5 2019/02/18 02:42:27 jakllsch Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/intr.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/conf.h>

#include <drm/drmP.h>
#include <drm/drm_crtc_helper.h>

#include <dev/fdt/fdtvar.h>
#include <dev/fdt/fdt_port.h>

#include <arm/sunxi/sunxi_drm.h>

#define	TCON_GCTL_REG		0x000
#define	 TCON_GCTL_TCON_EN			__BIT(31)
#define	 TCON_GCTL_GAMMA_EN			__BIT(30)
#define	 TCON_GCTL_IO_MAP_SEL			__BIT(0)
#define	TCON_GINT0_REG		0x004
#define	 TCON_GINT0_TCON0_VB_INT_EN		__BIT(31)
#define	 TCON_GINT0_TCON1_VB_INT_EN		__BIT(30)
#define	 TCON_GINT0_TCON0_VB_INT_FLAG		__BIT(15)
#define	 TCON_GINT0_TCON1_VB_INT_FLAG		__BIT(14)
#define	TCON_GINT1_REG		0x008
#define	 TCON_GINT1_TCON1_LINE_INT_NUM		__BITS(11,0)

#define	TCON0_CTL_REG		0x040
#define	 TCON0_CTL_TCON0_EN			__BIT(31)
#define	 TCON0_CTL_START_DELAY			__BITS(8,4)
#define	 TCON0_CTL_TCON0_SRC_SEL		__BITS(2,0)
#define	TCON0_DCLK_REG		0x044
#define	 TCON0_DCLK_EN				__BITS(31,28)
#define	 TCON0_DCLK_DIV				__BITS(6,0)
#define	TCON0_BASIC0_REG	0x048
#define	TCON0_BASIC1_REG	0x04c
#define	TCON0_BASIC2_REG	0x050
#define	TCON0_BASIC3_REG	0x054
#define	TCON0_IO_POL_REG	0x088
#define	 TCON0_IO_POL_IO_OUTPUT_SEL		__BIT(31)
#define	 TCON0_IO_POL_DCLK_SEL			__BITS(30,28)
#define	 TCON0_IO_POL_IO3_INV			__BIT(27)
#define	 TCON0_IO_POL_IO2_INV			__BIT(26)
#define	 TCON0_IO_POL_IO1_INV			__BIT(25)
#define	 TCON0_IO_POL_IO0_INV			__BIT(24)
#define	 TCON0_IO_POL_DATA_INV			__BITS(23,0)
#define	TCON0_IO_TRI_REG	0x08c

#define	TCON1_CTL_REG		0x090
#define	 TCON1_CTL_TCON1_EN			__BIT(31)
#define	 TCON1_CTL_START_DELAY			__BITS(8,4)
#define	 TCON1_CTL_TCON1_SRC_SEL		__BIT(1)
#define	TCON1_BASIC0_REG	0x094
#define	TCON1_BASIC1_REG	0x098
#define	TCON1_BASIC2_REG	0x09c
#define	TCON1_BASIC3_REG	0x0a0
#define	TCON1_BASIC4_REG	0x0a4
#define	TCON1_BASIC5_REG	0x0a8
#define	TCON1_IO_POL_REG	0x0f0
#define	 TCON1_IO_POL_IO3_INV			__BIT(27)
#define	 TCON1_IO_POL_IO2_INV			__BIT(26)
#define	 TCON1_IO_POL_IO1_INV			__BIT(25)
#define	 TCON1_IO_POL_IO0_INV			__BIT(24)
#define	 TCON1_IO_POL_DATA_INV			__BITS(23,0)
#define	TCON1_IO_TRI_REG	0x0f4

enum {
	TCON_PORT_INPUT = 0,
	TCON_PORT_OUTPUT = 1,
};

enum tcon_type {
	TYPE_TCON0,
	TYPE_TCON1,
};

static const struct of_compat_data compat_data[] = {
	{ "allwinner,sun8i-h3-tcon-tv",		TYPE_TCON1 },
	{ "allwinner,sun50i-a64-tcon-lcd",	TYPE_TCON0 },
	{ "allwinner,sun50i-a64-tcon-tv",	TYPE_TCON1 },
	{ NULL }
};

struct sunxi_lcdc_softc;

struct sunxi_lcdc_encoder {
	struct drm_encoder	base;
	struct sunxi_lcdc_softc *sc;
	struct drm_display_mode	curmode;
};

struct sunxi_lcdc_softc {
	device_t		sc_dev;
	bus_space_tag_t		sc_bst;
	bus_space_handle_t	sc_bsh;
	int			sc_phandle;

	enum tcon_type		sc_type;

	struct clk		*sc_clk_ch[2];

	struct sunxi_lcdc_encoder sc_encoder;
	struct drm_connector	sc_connector;

	struct fdt_device_ports	sc_ports;

	uint32_t		sc_vbl_counter;
};

#define	to_sunxi_lcdc_encoder(x)	container_of(x, struct sunxi_lcdc_encoder, base)

#define	TCON_READ(sc, reg)				\
	bus_space_read_4((sc)->sc_bst, (sc)->sc_bsh, (reg))
#define	TCON_WRITE(sc, reg, val)			\
	bus_space_write_4((sc)->sc_bst, (sc)->sc_bsh, (reg), (val))

static void
sunxi_lcdc_destroy(struct drm_encoder *encoder)
{
}

static const struct drm_encoder_funcs sunxi_lcdc_funcs = {
	.destroy = sunxi_lcdc_destroy,
};

static void
sunxi_lcdc_tcon_dpms(struct drm_encoder *encoder, int mode)
{
}

static bool
sunxi_lcdc_tcon_mode_fixup(struct drm_encoder *encoder,
    const struct drm_display_mode *mode, struct drm_display_mode *adjusted_mode)
{
	return true;
}

static void
sunxi_lcdc_tcon_mode_set(struct drm_encoder *encoder,
    struct drm_display_mode *mode, struct drm_display_mode *adjusted_mode)
{
	struct sunxi_lcdc_encoder *lcdc_encoder = to_sunxi_lcdc_encoder(encoder);

	lcdc_encoder->curmode = *adjusted_mode;
}

static void
sunxi_lcdc_tcon0_prepare(struct drm_encoder *encoder)
{
	struct sunxi_lcdc_encoder *lcdc_encoder = to_sunxi_lcdc_encoder(encoder);
	struct sunxi_lcdc_softc * const sc = lcdc_encoder->sc;
	uint32_t val;

	val = TCON_READ(sc, TCON_GCTL_REG);
	val |= TCON_GCTL_TCON_EN;
	TCON_WRITE(sc, TCON_GCTL_REG, val);

	TCON_WRITE(sc, TCON0_IO_TRI_REG, 0);
}

static void
sunxi_lcdc_tcon1_prepare(struct drm_encoder *encoder)
{
	struct sunxi_lcdc_encoder *lcdc_encoder = to_sunxi_lcdc_encoder(encoder);
	struct sunxi_lcdc_softc * const sc = lcdc_encoder->sc;
	uint32_t val;

	val = TCON_READ(sc, TCON_GCTL_REG);
	val |= TCON_GCTL_TCON_EN;
	TCON_WRITE(sc, TCON_GCTL_REG, val);

	TCON_WRITE(sc, TCON1_IO_POL_REG, 0);
	TCON_WRITE(sc, TCON1_IO_TRI_REG, 0xffffffff);
}

static void
sunxi_lcdc_tcon0_commit(struct drm_encoder *encoder)
{
	struct sunxi_lcdc_encoder *lcdc_encoder = to_sunxi_lcdc_encoder(encoder);
	struct sunxi_lcdc_softc * const sc = lcdc_encoder->sc;
	struct drm_display_mode *mode = &lcdc_encoder->curmode;
	uint32_t val;
	int error;

	const u_int interlace_p = (mode->flags & DRM_MODE_FLAG_INTERLACE) != 0;
	const u_int hspw = mode->hsync_end - mode->hsync_start;
	const u_int hbp = mode->htotal - mode->hsync_start;
	const u_int vspw = mode->vsync_end - mode->vsync_start;
	const u_int vbp = mode->vtotal - mode->vsync_start;
	const u_int vblank_len =
	    ((mode->vtotal << interlace_p) >> 1) - mode->vdisplay - 2;
	const u_int start_delay =
	    vblank_len >= 32 ? 30 : vblank_len - 2;

	val = TCON0_CTL_TCON0_EN |
	      __SHIFTIN(start_delay, TCON0_CTL_START_DELAY);
	TCON_WRITE(sc, TCON0_CTL_REG, val);

	TCON_WRITE(sc, TCON0_BASIC0_REG, ((mode->hdisplay - 1) << 16) | (mode->vdisplay - 1));
	TCON_WRITE(sc, TCON0_BASIC1_REG, ((mode->htotal - 1) << 16) | (hbp - 1));
	TCON_WRITE(sc, TCON0_BASIC2_REG, ((mode->vtotal * 2) << 16) | (vbp - 1));
	TCON_WRITE(sc, TCON0_BASIC3_REG, ((hspw - 1) << 16) | (vspw - 1));

	val = TCON_READ(sc, TCON0_IO_POL_REG);
	val &= ~(TCON0_IO_POL_IO3_INV|TCON0_IO_POL_IO2_INV|
		 TCON0_IO_POL_IO1_INV|TCON0_IO_POL_IO0_INV|
		 TCON0_IO_POL_DATA_INV);
	if ((mode->flags & DRM_MODE_FLAG_PHSYNC) == 0)
		val |= TCON0_IO_POL_IO1_INV;
	if ((mode->flags & DRM_MODE_FLAG_PVSYNC) == 0)
		val |= TCON0_IO_POL_IO0_INV;
	TCON_WRITE(sc, TCON0_IO_POL_REG, val);

	if (sc->sc_clk_ch[0] != NULL) {
		error = clk_set_rate(sc->sc_clk_ch[0], mode->crtc_clock * 1000);
		if (error != 0) {
			device_printf(sc->sc_dev, "failed to set CH0 PLL rate to %u Hz: %d\n",
			    mode->crtc_clock * 1000, error);
			return;
		}
		error = clk_enable(sc->sc_clk_ch[0]);
		if (error != 0) {
			device_printf(sc->sc_dev, "failed to enable CH0 PLL: %d\n", error);
			return;
		}
	} else {
		device_printf(sc->sc_dev, "no CH0 PLL configured\n");
	}
}

static void
sunxi_lcdc_tcon1_commit(struct drm_encoder *encoder)
{
	struct sunxi_lcdc_encoder *lcdc_encoder = to_sunxi_lcdc_encoder(encoder);
	struct sunxi_lcdc_softc * const sc = lcdc_encoder->sc;
	struct drm_display_mode *mode = &lcdc_encoder->curmode;
	uint32_t val;
	int error;

	const u_int interlace_p = (mode->flags & DRM_MODE_FLAG_INTERLACE) != 0;
	const u_int hspw = mode->hsync_end - mode->hsync_start;
	const u_int hbp = mode->htotal - mode->hsync_start;
	const u_int vspw = mode->vsync_end - mode->vsync_start;
	const u_int vbp = mode->vtotal - mode->vsync_start;
	const u_int vblank_len =
	    ((mode->vtotal << interlace_p) >> 1) - mode->vdisplay - 2;
	const u_int start_delay =
	    vblank_len >= 32 ? 30 : vblank_len - 2;

	val = TCON1_CTL_TCON1_EN |
	      __SHIFTIN(start_delay, TCON1_CTL_START_DELAY);
	TCON_WRITE(sc, TCON1_CTL_REG, val);

	TCON_WRITE(sc, TCON1_BASIC0_REG, ((mode->hdisplay - 1) << 16) | (mode->vdisplay - 1));
	TCON_WRITE(sc, TCON1_BASIC1_REG, ((mode->hdisplay - 1) << 16) | (mode->vdisplay - 1));
	TCON_WRITE(sc, TCON1_BASIC2_REG, ((mode->hdisplay - 1) << 16) | (mode->vdisplay - 1));
	TCON_WRITE(sc, TCON1_BASIC3_REG, ((mode->htotal - 1) << 16) | (hbp - 1));
	TCON_WRITE(sc, TCON1_BASIC4_REG, ((mode->vtotal * 2) << 16) | (vbp - 1));
	TCON_WRITE(sc, TCON1_BASIC5_REG, ((hspw - 1) << 16) | (vspw - 1));

	TCON_WRITE(sc, TCON_GINT1_REG,
	    __SHIFTIN(start_delay + 2, TCON_GINT1_TCON1_LINE_INT_NUM));

	if (sc->sc_clk_ch[1] != NULL) {
		error = clk_set_rate(sc->sc_clk_ch[1], mode->crtc_clock * 1000);
		if (error != 0) {
			device_printf(sc->sc_dev, "failed to set CH1 PLL rate to %u Hz: %d\n",
			    mode->crtc_clock * 1000, error);
			return;
		}
		error = clk_enable(sc->sc_clk_ch[1]);
		if (error != 0) {
			device_printf(sc->sc_dev, "failed to enable CH1 PLL: %d\n", error);
			return;
		}
	} else {
		device_printf(sc->sc_dev, "no CH1 PLL configured\n");
	}
}

static const struct drm_encoder_helper_funcs sunxi_lcdc_tcon0_helper_funcs = {
	.dpms = sunxi_lcdc_tcon_dpms,
	.mode_fixup = sunxi_lcdc_tcon_mode_fixup,
	.prepare = sunxi_lcdc_tcon0_prepare,
	.commit = sunxi_lcdc_tcon0_commit,
	.mode_set = sunxi_lcdc_tcon_mode_set,
};

static const struct drm_encoder_helper_funcs sunxi_lcdc_tcon1_helper_funcs = {
	.dpms = sunxi_lcdc_tcon_dpms,
	.mode_fixup = sunxi_lcdc_tcon_mode_fixup,
	.prepare = sunxi_lcdc_tcon1_prepare,
	.commit = sunxi_lcdc_tcon1_commit,
	.mode_set = sunxi_lcdc_tcon_mode_set,
};

static int
sunxi_lcdc_encoder_mode(struct fdt_endpoint *out_ep)
{
	struct fdt_endpoint *remote_ep = fdt_endpoint_remote(out_ep);

	if (remote_ep == NULL)
		return DRM_MODE_ENCODER_NONE;

	switch (fdt_endpoint_type(remote_ep)) {
	case EP_DRM_BRIDGE:
		return DRM_MODE_ENCODER_TMDS;
	case EP_DRM_PANEL:
		return DRM_MODE_ENCODER_LVDS;
	default:
		return DRM_MODE_ENCODER_NONE;
	}
}

static uint32_t
sunxi_lcdc_get_vblank_counter(void *priv)
{
	struct sunxi_lcdc_softc * const sc = priv;

	return sc->sc_vbl_counter;
}

static void
sunxi_lcdc_enable_vblank(void *priv)
{
	struct sunxi_lcdc_softc * const sc = priv;
        const int crtc_index = ffs32(sc->sc_encoder.base.possible_crtcs) - 1;

	if (crtc_index == 0)
		TCON_WRITE(sc, TCON_GINT0_REG, TCON_GINT0_TCON0_VB_INT_EN);
	else
		TCON_WRITE(sc, TCON_GINT0_REG, TCON_GINT0_TCON1_VB_INT_EN);
}

static void
sunxi_lcdc_disable_vblank(void *priv)
{
	struct sunxi_lcdc_softc * const sc = priv;

	TCON_WRITE(sc, TCON_GINT0_REG, 0);
}

static void
sunxi_lcdc_setup_vblank(struct sunxi_lcdc_softc *sc)
{
        const int crtc_index = ffs32(sc->sc_encoder.base.possible_crtcs) - 1;
	struct drm_device *ddev = sc->sc_encoder.base.dev;
	struct sunxi_drm_softc *drm_sc;

	KASSERT(ddev != NULL);

	drm_sc = device_private(ddev->dev);
	drm_sc->sc_vbl[crtc_index].priv = sc;
	drm_sc->sc_vbl[crtc_index].get_vblank_counter = sunxi_lcdc_get_vblank_counter;
	drm_sc->sc_vbl[crtc_index].enable_vblank = sunxi_lcdc_enable_vblank;
	drm_sc->sc_vbl[crtc_index].disable_vblank = sunxi_lcdc_disable_vblank;
}

static int
sunxi_lcdc_ep_activate(device_t dev, struct fdt_endpoint *ep, bool activate)
{
	struct sunxi_lcdc_softc * const sc = device_private(dev);
	struct fdt_endpoint *in_ep = fdt_endpoint_remote(ep);
	struct fdt_endpoint *out_ep;
	struct drm_crtc *crtc;

	if (!activate)
		return EINVAL;

	if (fdt_endpoint_port_index(ep) != TCON_PORT_INPUT)
		return EINVAL;

	if (fdt_endpoint_type(in_ep) != EP_DRM_CRTC)
		return EINVAL;

	crtc = fdt_endpoint_get_data(in_ep);

	sc->sc_encoder.sc = sc;
	sc->sc_encoder.base.possible_crtcs = 1 << drm_crtc_index(crtc);

	out_ep = fdt_endpoint_get_from_index(&sc->sc_ports, TCON_PORT_OUTPUT, 0);
	if (out_ep != NULL) {
		drm_encoder_init(crtc->dev, &sc->sc_encoder.base, &sunxi_lcdc_funcs,
		    sunxi_lcdc_encoder_mode(out_ep));
		drm_encoder_helper_add(&sc->sc_encoder.base, &sunxi_lcdc_tcon0_helper_funcs);

		sunxi_lcdc_setup_vblank(sc);

		return fdt_endpoint_activate(out_ep, activate);
	}

	out_ep = fdt_endpoint_get_from_index(&sc->sc_ports, TCON_PORT_OUTPUT, 1);
	if (out_ep != NULL) {
		drm_encoder_init(crtc->dev, &sc->sc_encoder.base, &sunxi_lcdc_funcs,
		    sunxi_lcdc_encoder_mode(out_ep));
		drm_encoder_helper_add(&sc->sc_encoder.base, &sunxi_lcdc_tcon1_helper_funcs);

		sunxi_lcdc_setup_vblank(sc);

		return fdt_endpoint_activate(out_ep, activate);
	}

	return ENXIO;
}

static void *
sunxi_lcdc_ep_get_data(device_t dev, struct fdt_endpoint *ep)
{
	struct sunxi_lcdc_softc * const sc = device_private(dev);

	return &sc->sc_encoder;
}

static int
sunxi_lcdc_intr(void *priv)
{
	struct sunxi_lcdc_softc * const sc = priv;
	uint32_t val;
	int rv = 0;

	const int crtc_index = ffs32(sc->sc_encoder.base.possible_crtcs) - 1;
	const uint32_t status_mask = crtc_index == 0 ?
	    TCON_GINT0_TCON0_VB_INT_FLAG : TCON_GINT0_TCON1_VB_INT_FLAG;

	val = TCON_READ(sc, TCON_GINT0_REG);
	if ((val & status_mask) != 0) {
		TCON_WRITE(sc, TCON_GINT0_REG, val & ~status_mask);
		atomic_inc_32(&sc->sc_vbl_counter);
		drm_handle_vblank(sc->sc_encoder.base.dev, crtc_index);
		rv = 1;
	}

	return rv;
}

static int
sunxi_lcdc_match(device_t parent, cfdata_t cf, void *aux)
{
	struct fdt_attach_args * const faa = aux;

	return of_match_compat_data(faa->faa_phandle, compat_data);
}

static void
sunxi_lcdc_attach(device_t parent, device_t self, void *aux)
{
	struct sunxi_lcdc_softc * const sc = device_private(self);
	struct fdt_attach_args * const faa = aux;
	const int phandle = faa->faa_phandle;
	struct fdtbus_reset *rst;
	char intrstr[128];
	struct clk *clk;
	bus_addr_t addr;
	bus_size_t size;
	void *ih;

	if (fdtbus_get_reg(phandle, 0, &addr, &size) != 0) {
		aprint_error(": couldn't get registers\n");
		return;
	}

	if (!fdtbus_intr_str(phandle, 0, intrstr, sizeof(intrstr))) {
		aprint_error(": couldn't decode interrupt\n");
		return;
	}

	rst = fdtbus_reset_get(phandle, "lcd");
	if (rst == NULL || fdtbus_reset_deassert(rst) != 0) {
		aprint_error(": couldn't de-assert reset\n");
		return;
	}

	clk = fdtbus_clock_get(phandle, "ahb");
	if (clk == NULL || clk_enable(clk) != 0) {
		aprint_error(": couldn't enable bus clock\n");
		return;
	}

	sc->sc_dev = self;
	sc->sc_bst = faa->faa_bst;
	if (bus_space_map(sc->sc_bst, addr, size, 0, &sc->sc_bsh) != 0) {
		aprint_error(": couldn't map registers\n");
		return;
	}
	sc->sc_phandle = faa->faa_phandle;
	sc->sc_type = of_search_compatible(phandle, compat_data)->data;
	sc->sc_clk_ch[0] = fdtbus_clock_get(phandle, "tcon-ch0");
	sc->sc_clk_ch[1] = fdtbus_clock_get(phandle, "tcon-ch1");

	aprint_naive("\n");
	switch (sc->sc_type) {
	case TYPE_TCON0:
		aprint_normal(": TCON0\n");
		break;
	case TYPE_TCON1:
		aprint_normal(": TCON1\n");
		break;
	}

	sc->sc_ports.dp_ep_activate = sunxi_lcdc_ep_activate;
	sc->sc_ports.dp_ep_get_data = sunxi_lcdc_ep_get_data;
	fdt_ports_register(&sc->sc_ports, self, phandle, EP_DRM_ENCODER);

	ih = fdtbus_intr_establish(phandle, 0, IPL_VM, FDT_INTR_MPSAFE,
	    sunxi_lcdc_intr, sc);
	if (ih == NULL) {
		aprint_error_dev(self, "couldn't establish interrupt on %s\n",
		    intrstr);
		return;
	}
	aprint_normal_dev(self, "interrupting on %s\n", intrstr);
}

CFATTACH_DECL_NEW(sunxi_lcdc, sizeof(struct sunxi_lcdc_softc),
	sunxi_lcdc_match, sunxi_lcdc_attach, NULL, NULL);
