/* $NetBSD: rk_vop.c,v 1.11 2021/12/19 11:00:46 riastradh Exp $ */

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
__KERNEL_RCSID(0, "$NetBSD: rk_vop.c,v 1.11 2021/12/19 11:00:46 riastradh Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/intr.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/conf.h>
#include <sys/sysctl.h>

#include <drm/drm_drv.h>
#include <drm/drm_crtc.h>
#include <drm/drm_crtc_helper.h>
#include <drm/drm_fourcc.h>
#include <drm/drm_plane_helper.h>

#include <dev/fdt/fdtvar.h>
#include <dev/fdt/fdt_port.h>

#include <arm/rockchip/rk_drm.h>

#define	VOP_REG_CFG_DONE		0x0000
#define	 REG_LOAD_EN			__BIT(0)
#define	VOP_SYS_CTRL			0x0008
#define	 VOP_STANDBY_EN			__BIT(22)
#define	 MIPI_OUT_EN			__BIT(15)
#define	 EDP_OUT_EN			__BIT(14)
#define	 HDMI_OUT_EN			__BIT(13)
#define	 RGB_OUT_EN			__BIT(12)
#define	VOP_DSP_CTRL0			0x0010
#define	 DSP_OUT_MODE			__BITS(3,0)
#define	  DSP_OUT_MODE_RGB888		0
#define	  DSP_OUT_MODE_RGBaaa		15
#define	VOP_DSP_CTRL1			0x0014
#define	VOP_WIN0_CTRL			0x0030
#define	 WIN0_LB_MODE			__BITS(7,5)
#define	  WIN0_LB_MODE_RGB_3840X2	2
#define	  WIN0_LB_MODE_RGB_2560X4	3
#define	  WIN0_LB_MODE_RGB_1920X5	4
#define	  WIN0_LB_MODE_RGB_1280X8	5
#define	 WIN0_DATA_FMT			__BITS(3,1)
#define	  WIN0_DATA_FMT_ARGB888		0
#define	 WIN0_EN			__BIT(0)
#define	VOP_WIN0_COLOR_KEY		0x0038
#define	VOP_WIN0_VIR			0x003c
#define	 WIN0_VIR_STRIDE		__BITS(13,0)
#define	VOP_WIN0_YRGB_MST		0x0040
#define	VOP_WIN0_ACT_INFO		0x0048
#define	 WIN0_ACT_HEIGHT		__BITS(28,16)
#define	 WIN0_ACT_WIDTH			__BITS(12,0)
#define	VOP_WIN0_DSP_INFO		0x004c
#define	 WIN0_DSP_HEIGHT		__BITS(27,16)
#define	 WIN0_DSP_WIDTH			__BITS(11,0)
#define	VOP_WIN0_DSP_ST			0x0050
#define	 WIN0_DSP_YST			__BITS(28,16)
#define	 WIN0_DSP_XST			__BITS(12,0)
#define	VOP_POST_DSP_HACT_INFO		0x0170
#define	 DSP_HACT_ST_POST		__BITS(28,16)
#define	 DSP_HACT_END_POST		__BITS(12,0)
#define	VOP_POST_DSP_VACT_INFO		0x0174
#define	 DSP_VACT_ST_POST		__BITS(28,16)
#define	 DSP_VACT_END_POST		__BITS(12,0)
#define	VOP_DSP_HTOTAL_HS_END		0x0188
#define	 DSP_HS_END			__BITS(28,16)
#define	 DSP_HTOTAL			__BITS(12,0)
#define	VOP_DSP_HACT_ST_END		0x018c
#define	 DSP_HACT_ST			__BITS(28,16)
#define	 DSP_HACT_END			__BITS(12,0)
#define	VOP_DSP_VTOTAL_VS_END		0x0190
#define	 DSP_VS_END			__BITS(28,16)
#define	 DSP_VTOTAL			__BITS(12,0)
#define	VOP_DSP_VACT_ST_END		0x0194
#define	 DSP_VACT_ST			__BITS(28,16)
#define	 DSP_VACT_END			__BITS(12,0)

/*
 * Polarity fields are in different locations depending on SoC and output type,
 * but always in the same order.
 */
#define	DSP_DCLK_POL			__BIT(3)
#define	DSP_DEN_POL			__BIT(2)
#define	DSP_VSYNC_POL			__BIT(1)
#define	DSP_HSYNC_POL			__BIT(0)

enum vop_ep_type {
	VOP_EP_MIPI,
	VOP_EP_EDP,
	VOP_EP_HDMI,
	VOP_EP_MIPI1,
	VOP_EP_DP,
	VOP_NEP
};

struct rk_vop_softc;
struct rk_vop_config;

struct rk_vop_crtc {
	struct drm_crtc		base;
	struct rk_vop_softc	*sc;
};

struct rk_vop_softc {
	device_t		sc_dev;
	bus_space_tag_t		sc_bst;
	bus_space_handle_t	sc_bsh;
	int			sc_phandle;

	struct clk		*sc_dclk;

	struct rk_vop_crtc	sc_crtc;

	struct fdt_device_ports	sc_ports;

	const struct rk_vop_config *sc_conf;
};

#define	to_rk_vop_crtc(x)	container_of(x, struct rk_vop_crtc, base)

#define	RD4(sc, reg)				\
	bus_space_read_4((sc)->sc_bst, (sc)->sc_bsh, (reg))
#define	WR4(sc, reg, val)			\
	bus_space_write_4((sc)->sc_bst, (sc)->sc_bsh, (reg), (val))

struct rk_vop_config {
	const char		*descr;
	u_int			out_mode;
	void			(*init)(struct rk_vop_softc *);
	void			(*set_polarity)(struct rk_vop_softc *,
						enum vop_ep_type, uint32_t);
};

#define	RK3399_VOP_MIPI_POL	__BITS(31,28)
#define	RK3399_VOP_EDP_POL	__BITS(27,24)
#define	RK3399_VOP_HDMI_POL	__BITS(23,20)
#define	RK3399_VOP_DP_POL	__BITS(19,16)

#define	RK3399_VOP_SYS_CTRL_ENABLE	__BIT(11)

static void
rk3399_vop_set_polarity(struct rk_vop_softc *sc, enum vop_ep_type ep_type, uint32_t pol)
{
	uint32_t mask, val;

	switch (ep_type) {
	case VOP_EP_MIPI:
	case VOP_EP_MIPI1:
		mask = RK3399_VOP_MIPI_POL;
		break;
	case VOP_EP_EDP:
		mask = RK3399_VOP_EDP_POL;
		break;
	case VOP_EP_HDMI:
		mask = RK3399_VOP_HDMI_POL;
		break;
	case VOP_EP_DP:
		mask = RK3399_VOP_DP_POL;
		break;
	default:
		return;
	}

	val = RD4(sc, VOP_DSP_CTRL1);
	val &= ~mask;
	val |= __SHIFTIN(pol, mask);
	WR4(sc, VOP_DSP_CTRL1, val);
}

static void
rk3399_vop_init(struct rk_vop_softc *sc)
{
	uint32_t val;

	val = RD4(sc, VOP_SYS_CTRL);
	val |= RK3399_VOP_SYS_CTRL_ENABLE;
	WR4(sc, VOP_SYS_CTRL, val);
}

static const struct rk_vop_config rk3399_vop_lit_config = {
	.descr = "RK3399 VOPL",
	.out_mode = DSP_OUT_MODE_RGB888,
	.init = rk3399_vop_init,
	.set_polarity = rk3399_vop_set_polarity,
};

static const struct rk_vop_config rk3399_vop_big_config = {
	.descr = "RK3399 VOPB",
	.out_mode = DSP_OUT_MODE_RGBaaa,
	.init = rk3399_vop_init,
	.set_polarity = rk3399_vop_set_polarity,
};

static const struct device_compatible_entry compat_data[] = {
	{ .compat = "rockchip,rk3399-vop-big",
	  .data = &rk3399_vop_big_config },
	{ .compat = "rockchip,rk3399-vop-lit",
	  .data = &rk3399_vop_lit_config },

	DEVICE_COMPAT_EOL
};

static int
rk_vop_mode_do_set_base(struct drm_crtc *crtc, struct drm_framebuffer *fb,
    int x, int y, int atomic)
{
	struct rk_vop_crtc *mixer_crtc = to_rk_vop_crtc(crtc);
	struct rk_vop_softc * const sc = mixer_crtc->sc;
	struct rk_drm_framebuffer *sfb = atomic?
	    to_rk_drm_framebuffer(fb) :
	    to_rk_drm_framebuffer(crtc->primary->fb);

	uint64_t paddr = (uint64_t)sfb->obj->dmamap->dm_segs[0].ds_addr;


	paddr += y * sfb->base.pitches[0];
	paddr += x * sfb->base.format->cpp[0];

	KASSERT((paddr & ~0xffffffff) == 0);

	const uint32_t vir = __SHIFTIN(sfb->base.pitches[0] / 4,
	    WIN0_VIR_STRIDE);
	WR4(sc, VOP_WIN0_VIR, vir);

	/* Framebuffer start address */
	WR4(sc, VOP_WIN0_YRGB_MST, (uint32_t)paddr);

	return 0;
}

static void
rk_vop_destroy(struct drm_crtc *crtc)
{
	drm_crtc_cleanup(crtc);
}

static const struct drm_crtc_funcs rk_vop_crtc_funcs = {
	.set_config = drm_crtc_helper_set_config,
	.destroy = rk_vop_destroy,
};

static void
rk_vop_dpms(struct drm_crtc *crtc, int mode)
{
	struct rk_vop_crtc *mixer_crtc = to_rk_vop_crtc(crtc);
	struct rk_vop_softc * const sc = mixer_crtc->sc;
	uint32_t val;

	val = RD4(sc, VOP_SYS_CTRL);

	switch (mode) {
	case DRM_MODE_DPMS_ON:
		val &= ~VOP_STANDBY_EN;
		break;
	case DRM_MODE_DPMS_STANDBY:
	case DRM_MODE_DPMS_SUSPEND:
	case DRM_MODE_DPMS_OFF:
		val |= VOP_STANDBY_EN;
		break;
	}

	WR4(sc, VOP_SYS_CTRL, val);

	/* Commit settings */
	WR4(sc, VOP_REG_CFG_DONE, REG_LOAD_EN);
}

static bool
rk_vop_mode_fixup(struct drm_crtc *crtc,
    const struct drm_display_mode *mode, struct drm_display_mode *adjusted_mode)
{
	return true;
}

static int
rk_vop_mode_set(struct drm_crtc *crtc, struct drm_display_mode *mode,
    struct drm_display_mode *adjusted_mode, int x, int y,
    struct drm_framebuffer *old_fb)
{
	struct rk_vop_crtc *mixer_crtc = to_rk_vop_crtc(crtc);
	struct rk_vop_softc * const sc = mixer_crtc->sc;
	uint32_t val;
	u_int lb_mode;
	int error;
	u_int pol;
	int connector_type = 0;
	struct drm_connector *connector;
	struct drm_connector_list_iter conn_iter;

	const u_int hactive = adjusted_mode->hdisplay;
	const u_int hsync_len = adjusted_mode->hsync_end - adjusted_mode->hsync_start;
	const u_int hback_porch = adjusted_mode->htotal - adjusted_mode->hsync_end;
	const u_int hfront_porch = adjusted_mode->hsync_start - adjusted_mode->hdisplay;

	const u_int vactive = adjusted_mode->vdisplay;
	const u_int vsync_len = adjusted_mode->vsync_end - adjusted_mode->vsync_start;
	const u_int vback_porch = adjusted_mode->vtotal - adjusted_mode->vsync_end;
	const u_int vfront_porch = adjusted_mode->vsync_start - adjusted_mode->vdisplay;

	error = clk_set_rate(sc->sc_dclk, adjusted_mode->clock * 1000);
	if (error != 0)
		DRM_ERROR("couldn't set pixel clock: %d\n", error);

	val = __SHIFTIN(hactive - 1, WIN0_ACT_WIDTH) |
	      __SHIFTIN(vactive - 1, WIN0_ACT_HEIGHT);
	WR4(sc, VOP_WIN0_ACT_INFO, val);

	val = __SHIFTIN(hactive - 1, WIN0_DSP_WIDTH) |
	      __SHIFTIN(vactive - 1, WIN0_DSP_HEIGHT);
	WR4(sc, VOP_WIN0_DSP_INFO, val);

	val = __SHIFTIN(hsync_len + hback_porch, WIN0_DSP_XST) |
	      __SHIFTIN(vsync_len + vback_porch, WIN0_DSP_YST);
	WR4(sc, VOP_WIN0_DSP_ST, val);

	WR4(sc, VOP_WIN0_COLOR_KEY, 0);

	if (adjusted_mode->hdisplay > 2560)
		lb_mode = WIN0_LB_MODE_RGB_3840X2;
	else if (adjusted_mode->hdisplay > 1920)
		lb_mode = WIN0_LB_MODE_RGB_2560X4;
	else if (adjusted_mode->hdisplay > 1280)
		lb_mode = WIN0_LB_MODE_RGB_1920X5;
	else
		lb_mode = WIN0_LB_MODE_RGB_1280X8;

	val = __SHIFTIN(lb_mode, WIN0_LB_MODE) |
	      __SHIFTIN(WIN0_DATA_FMT_ARGB888, WIN0_DATA_FMT) |
	      WIN0_EN;
	WR4(sc, VOP_WIN0_CTRL, val);

	rk_vop_mode_do_set_base(crtc, old_fb, x, y, 0);

	pol = DSP_DCLK_POL;
	if ((adjusted_mode->flags & DRM_MODE_FLAG_PHSYNC) != 0)
		pol |= DSP_HSYNC_POL;
	if ((adjusted_mode->flags & DRM_MODE_FLAG_PVSYNC) != 0)
		pol |= DSP_VSYNC_POL;

	drm_connector_list_iter_begin(crtc->dev, &conn_iter);
	drm_for_each_connector_iter(connector, &conn_iter) {
		if (connector->encoder == NULL)
			continue;
		if (connector->encoder->crtc == crtc) {
			connector_type = connector->connector_type;
			break;
		}
	}
	drm_connector_list_iter_end(&conn_iter);

	switch (connector_type) {
	case DRM_MODE_CONNECTOR_HDMIA:
		sc->sc_conf->set_polarity(sc, VOP_EP_HDMI, pol);
		break;
	case DRM_MODE_CONNECTOR_eDP:
		sc->sc_conf->set_polarity(sc, VOP_EP_EDP, pol);
		break;
	}

	val = RD4(sc, VOP_SYS_CTRL);
	val &= ~VOP_STANDBY_EN;
	val &= ~(MIPI_OUT_EN|EDP_OUT_EN|HDMI_OUT_EN|RGB_OUT_EN);

	switch (connector_type) {
	case DRM_MODE_CONNECTOR_HDMIA:
		val |= HDMI_OUT_EN;
		break;
	case DRM_MODE_CONNECTOR_eDP:
		val |= EDP_OUT_EN;
		break;
	}
	WR4(sc, VOP_SYS_CTRL, val);

	val = RD4(sc, VOP_DSP_CTRL0);
	val &= ~DSP_OUT_MODE;
	val |= __SHIFTIN(sc->sc_conf->out_mode, DSP_OUT_MODE);
	WR4(sc, VOP_DSP_CTRL0, val);

	val = __SHIFTIN(hsync_len + hback_porch, DSP_HACT_ST_POST) |
	      __SHIFTIN(hsync_len + hback_porch + hactive, DSP_HACT_END_POST);
	WR4(sc, VOP_POST_DSP_HACT_INFO, val);

	val = __SHIFTIN(hsync_len + hback_porch, DSP_HACT_ST) |
	      __SHIFTIN(hsync_len + hback_porch + hactive, DSP_HACT_END);
	WR4(sc, VOP_DSP_HACT_ST_END, val);

	val = __SHIFTIN(hsync_len, DSP_HTOTAL) |
	      __SHIFTIN(hsync_len + hback_porch + hactive + hfront_porch, DSP_HS_END);
	WR4(sc, VOP_DSP_HTOTAL_HS_END, val);

	val = __SHIFTIN(vsync_len + vback_porch, DSP_VACT_ST_POST) |
	      __SHIFTIN(vsync_len + vback_porch + vactive, DSP_VACT_END_POST);
	WR4(sc, VOP_POST_DSP_VACT_INFO, val);

	val = __SHIFTIN(vsync_len + vback_porch, DSP_VACT_ST) |
	      __SHIFTIN(vsync_len + vback_porch + vactive, DSP_VACT_END);
	WR4(sc, VOP_DSP_VACT_ST_END, val);

	val = __SHIFTIN(vsync_len, DSP_VTOTAL) |
	      __SHIFTIN(vsync_len + vback_porch + vactive + vfront_porch, DSP_VS_END);
	WR4(sc, VOP_DSP_VTOTAL_VS_END, val);

	return 0;
}

static int
rk_vop_mode_set_base(struct drm_crtc *crtc, int x, int y,
    struct drm_framebuffer *old_fb)
{
	struct rk_vop_crtc *mixer_crtc = to_rk_vop_crtc(crtc);
	struct rk_vop_softc * const sc = mixer_crtc->sc;

	rk_vop_mode_do_set_base(crtc, old_fb, x, y, 0);

	/* Commit settings */
	WR4(sc, VOP_REG_CFG_DONE, REG_LOAD_EN);

	return 0;
}

static int
rk_vop_mode_set_base_atomic(struct drm_crtc *crtc, struct drm_framebuffer *fb,
    int x, int y, enum mode_set_atomic state)
{
	struct rk_vop_crtc *mixer_crtc = to_rk_vop_crtc(crtc);
	struct rk_vop_softc * const sc = mixer_crtc->sc;

	rk_vop_mode_do_set_base(crtc, fb, x, y, 1);

	/* Commit settings */
	WR4(sc, VOP_REG_CFG_DONE, REG_LOAD_EN);

	return 0;
}

static void
rk_vop_disable(struct drm_crtc *crtc)
{
}

static void
rk_vop_prepare(struct drm_crtc *crtc)
{
}

static void
rk_vop_commit(struct drm_crtc *crtc)
{
	struct rk_vop_crtc *mixer_crtc = to_rk_vop_crtc(crtc);
	struct rk_vop_softc * const sc = mixer_crtc->sc;

	/* Commit settings */
	WR4(sc, VOP_REG_CFG_DONE, REG_LOAD_EN);
}

static const struct drm_crtc_helper_funcs rk_vop_crtc_helper_funcs = {
	.dpms = rk_vop_dpms,
	.mode_fixup = rk_vop_mode_fixup,
	.mode_set = rk_vop_mode_set,
	.mode_set_base = rk_vop_mode_set_base,
	.mode_set_base_atomic = rk_vop_mode_set_base_atomic,
	.disable = rk_vop_disable,
	.prepare = rk_vop_prepare,
	.commit = rk_vop_commit,
};

static int
rk_vop_ep_activate(device_t dev, struct fdt_endpoint *ep, bool activate)
{
	struct rk_vop_softc * const sc = device_private(dev);
	struct drm_device *ddev;

	if (!activate)
		return EINVAL;

	ddev = rk_drm_port_device(&sc->sc_ports);
	if (ddev == NULL) {
		DRM_ERROR("couldn't find DRM device\n");
		return ENXIO;
	}

	if (sc->sc_crtc.sc == NULL) {
		sc->sc_crtc.sc = sc;

		drm_crtc_init(ddev, &sc->sc_crtc.base, &rk_vop_crtc_funcs);
		drm_crtc_helper_add(&sc->sc_crtc.base, &rk_vop_crtc_helper_funcs);

		aprint_debug_dev(dev, "using CRTC %d for %s\n",
		    drm_crtc_index(&sc->sc_crtc.base), sc->sc_conf->descr);
	}

	const u_int ep_index = fdt_endpoint_index(ep);
	if (ep_index >= VOP_NEP) {
		DRM_ERROR("endpoint index %d out of range\n", ep_index);
		return ENXIO;
	}

	return fdt_endpoint_activate(ep, activate);
}

static void *
rk_vop_ep_get_data(device_t dev, struct fdt_endpoint *ep)
{
	struct rk_vop_softc * const sc = device_private(dev);

	return &sc->sc_crtc.base;
}

static int
rk_vop_match(device_t parent, cfdata_t cf, void *aux)
{
	struct fdt_attach_args * const faa = aux;

	return of_compatible_match(faa->faa_phandle, compat_data);
}

static void
rk_vop_attach(device_t parent, device_t self, void *aux)
{
	struct rk_vop_softc * const sc = device_private(self);
	struct fdt_attach_args * const faa = aux;
	const int phandle = faa->faa_phandle;
	const char * const reset_names[] = { "axi", "ahb", "dclk" };
	const char * const clock_names[] = { "aclk_vop", "hclk_vop" };
	struct fdtbus_reset *rst;
	bus_addr_t addr;
	bus_size_t size;
	u_int n;

	if (fdtbus_get_reg(phandle, 0, &addr, &size) != 0) {
		aprint_error(": couldn't get registers\n");
		return;
	}

	fdtbus_clock_assign(phandle);

	for (n = 0; n < __arraycount(reset_names); n++) {
		rst = fdtbus_reset_get(phandle, reset_names[n]);
		if (rst == NULL || fdtbus_reset_deassert(rst) != 0) {
			aprint_error(": couldn't de-assert reset %s\n", reset_names[n]);
			return;
		}
	}
	for (n = 0; n < __arraycount(clock_names); n++) {
		if (fdtbus_clock_enable(phandle, clock_names[n], true) != 0) {
			aprint_error(": couldn't enable clock %s\n", clock_names[n]);
			return;
		}
	}
	sc->sc_dclk = fdtbus_clock_get(phandle, "dclk_vop");
	if (sc->sc_dclk == NULL || clk_enable(sc->sc_dclk) != 0) {
		aprint_error(": couldn't enable clock %s\n", "dclk_vop");
		return;
	}

	sc->sc_dev = self;
	sc->sc_bst = faa->faa_bst;
	if (bus_space_map(sc->sc_bst, addr, size, 0, &sc->sc_bsh) != 0) {
		aprint_error(": couldn't map registers\n");
		return;
	}
	sc->sc_phandle = faa->faa_phandle;
	sc->sc_conf = of_compatible_lookup(phandle, compat_data)->data;

	aprint_naive("\n");
	aprint_normal(": %s\n", sc->sc_conf->descr);

	if (sc->sc_conf->init != NULL)
		sc->sc_conf->init(sc);

	sc->sc_ports.dp_ep_activate = rk_vop_ep_activate;
	sc->sc_ports.dp_ep_get_data = rk_vop_ep_get_data;
	fdt_ports_register(&sc->sc_ports, self, phandle, EP_DRM_CRTC);

	const int port_phandle = of_find_firstchild_byname(phandle, "port");
	if (port_phandle > 0)
		rk_drm_register_port(port_phandle, &sc->sc_ports);
}

CFATTACH_DECL_NEW(rk_vop, sizeof(struct rk_vop_softc),
	rk_vop_match, rk_vop_attach, NULL, NULL);
