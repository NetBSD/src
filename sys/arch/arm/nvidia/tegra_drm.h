/* $NetBSD: tegra_drm.h,v 1.8 2017/12/26 14:54:52 jmcneill Exp $ */

/*-
 * Copyright (c) 2015 Jared D. McNeill <jmcneill@invisible.ca>
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

#ifndef _ARM_TEGRA_DRM_H
#define _ARM_TEGRA_DRM_H

#include <drm/drm_fb_helper.h>
#include <drm/drm_gem_cma_helper.h>

#define DRIVER_AUTHOR		"Jared McNeill"

#define DRIVER_NAME		"tegra"
#define DRIVER_DESC		"NVIDIA Tegra K1"
#define DRIVER_DATE		"20151108"

#define DRIVER_MAJOR		0
#define DRIVER_MINOR		1
#define DRIVER_PATCHLEVEL	0

struct tegra_framebuffer;

struct tegra_drm_softc {
	device_t		sc_dev;
	struct drm_device	*sc_ddev;

	bus_space_tag_t		sc_bst;
	bus_dma_tag_t		sc_dmat;

	int			sc_phandle;

	struct clk		*sc_clk_host1x;
	struct fdtbus_reset	*sc_rst_host1x;

	struct clk		*sc_clk_dc[2];
	struct clk		*sc_clk_dc_parent[2];
	struct fdtbus_reset	*sc_rst_dc[2];

	struct clk		*sc_clk_hdmi;
	struct clk		*sc_clk_hdmi_parent;
	struct fdtbus_reset	*sc_rst_hdmi;

	i2c_tag_t		sc_ddc;
	struct fdtbus_gpio_pin	*sc_pin_hpd;

	bool			sc_force_dvi;

	uint32_t		sc_vbl_received[2];
};

struct tegra_drmfb_attach_args {
	struct drm_device	*tfa_drm_dev;
	struct drm_fb_helper	*tfa_fb_helper;
	struct drm_fb_helper_surface_size tfa_fb_sizes;
	bus_space_tag_t		tfa_fb_bst;
	bus_dma_tag_t		tfa_fb_dmat;
	uint32_t		tfa_fb_linebytes;
};

struct tegra_crtc {
	struct drm_crtc		base;
	bus_space_tag_t		bst;
	bus_space_handle_t	bsh;
	bus_size_t		size;
	int			intr;
	int			index;
	void			*ih;
	bool			enabled;
	struct clk		*clk_parent;

	struct drm_gem_cma_object	*cursor_obj;
	int			cursor_x;
	int			cursor_y;
};

struct tegra_encoder {
	struct drm_encoder	base;
	bus_space_tag_t		bst;
	bus_space_handle_t	bsh;
	bus_size_t		size;
};

struct tegra_connector {
	struct drm_connector	base;
	i2c_tag_t		ddc;
	struct i2c_adapter	*adapter;
	struct fdtbus_gpio_pin	*hpd;

	bool			has_hdmi_sink;
	bool			has_audio;
};

struct tegra_framebuffer {
	struct drm_framebuffer	base;
	struct drm_gem_cma_object *obj;
};

struct tegra_fbdev {
	struct drm_fb_helper	helper;
};

#define HDMI_READ(enc, reg)			\
    bus_space_read_4((enc)->bst, (enc)->bsh, (reg))
#define HDMI_WRITE(enc, reg, val)		\
    bus_space_write_4((enc)->bst, (enc)->bsh, (reg), (val))
#define HDMI_SET_CLEAR(enc, reg, set, clr)	\
    tegra_reg_set_clear((enc)->bst, (enc)->bsh, (reg), (set), (clr))

#define DC_READ(crtc, reg)			\
    bus_space_read_4((crtc)->bst, (crtc)->bsh, (reg))
#define DC_WRITE(crtc, reg, val)		\
    bus_space_write_4((crtc)->bst, (crtc)->bsh, (reg), (val))
#define DC_SET_CLEAR(crtc, reg, set, clr)	\
    tegra_reg_set_clear((crtc)->bst, (crtc)->bsh, (reg), (set), (clr))

#define TEGRA_DC_DEPTH		32

#define tegra_drm_private(ddev)	(ddev)->dev_private
#define to_tegra_crtc(x)	container_of(x, struct tegra_crtc, base)
#define to_tegra_encoder(x)	container_of(x, struct tegra_encoder, base)
#define to_tegra_connector(x)	container_of(x, struct tegra_connector, base)
#define to_tegra_framebuffer(x)	container_of(x, struct tegra_framebuffer, base)
#define to_tegra_fbdev(x)	container_of(x, struct tegra_fbdev, helper)

int	tegra_drm_mode_init(struct drm_device *);
int	tegra_drm_fb_init(struct drm_device *);
u32	tegra_drm_get_vblank_counter(struct drm_device *, int);
int	tegra_drm_enable_vblank(struct drm_device *, int);
void	tegra_drm_disable_vblank(struct drm_device *, int);
int	tegra_drm_framebuffer_init(struct drm_device *,
	    struct tegra_framebuffer *);

#endif /* _ARM_TEGRA_DRM_H */
