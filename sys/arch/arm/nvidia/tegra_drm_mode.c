/* $NetBSD: tegra_drm_mode.c,v 1.15.8.2 2017/12/03 11:35:54 jdolecek Exp $ */

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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: tegra_drm_mode.c,v 1.15.8.2 2017/12/03 11:35:54 jdolecek Exp $");

#include <drm/drmP.h>
#include <drm/drm_crtc.h>
#include <drm/drm_crtc_helper.h>
#include <drm/drm_edid.h>

#include <dev/i2c/ddcvar.h>

#include <arm/nvidia/tegra_reg.h>
#include <arm/nvidia/tegra_var.h>
#include <arm/nvidia/tegra_intr.h>
#include <arm/nvidia/tegra_pmcreg.h>
#include <arm/nvidia/tegra_dcreg.h>
#include <arm/nvidia/tegra_hdmireg.h>
#include <arm/nvidia/tegra_drm.h>

#include <dev/fdt/fdtvar.h>

static struct drm_framebuffer *tegra_fb_create(struct drm_device *,
		    struct drm_file *, struct drm_mode_fb_cmd2 *);

static const struct drm_mode_config_funcs tegra_mode_config_funcs = {
	.fb_create = tegra_fb_create
};

static int	tegra_framebuffer_create_handle(struct drm_framebuffer *,
		    struct drm_file *, unsigned int *);
static void	tegra_framebuffer_destroy(struct drm_framebuffer *);

static const struct drm_framebuffer_funcs tegra_framebuffer_funcs = {
	.create_handle = tegra_framebuffer_create_handle,
	.destroy = tegra_framebuffer_destroy
};

static int	tegra_crtc_init(struct drm_device *, int);
static void	tegra_crtc_destroy(struct drm_crtc *);
static int	tegra_crtc_cursor_set(struct drm_crtc *, struct drm_file *,
		    uint32_t, uint32_t, uint32_t);
static int	tegra_crtc_cursor_move(struct drm_crtc *, int, int);
static int	tegra_crtc_intr(void *);

static const struct drm_crtc_funcs tegra_crtc_funcs = {
	.cursor_set = tegra_crtc_cursor_set,
	.cursor_move = tegra_crtc_cursor_move,
	.set_config = drm_crtc_helper_set_config,
	.destroy = tegra_crtc_destroy
};

static void	tegra_crtc_dpms(struct drm_crtc *, int);
static bool	tegra_crtc_mode_fixup(struct drm_crtc *,
		    const struct drm_display_mode *,
		    struct drm_display_mode *);
static int	tegra_crtc_mode_set(struct drm_crtc *,
		    struct drm_display_mode *, struct drm_display_mode *,
		    int, int, struct drm_framebuffer *);
static int	tegra_crtc_mode_set_base(struct drm_crtc *,
		    int, int, struct drm_framebuffer *);
static int	tegra_crtc_mode_set_base_atomic(struct drm_crtc *,
		    struct drm_framebuffer *, int, int, enum mode_set_atomic);
static void	tegra_crtc_disable(struct drm_crtc *);
static void	tegra_crtc_prepare(struct drm_crtc *);
static void	tegra_crtc_commit(struct drm_crtc *);

static int	tegra_crtc_do_set_base(struct drm_crtc *,
		    struct drm_framebuffer *, int, int, int);

static const struct drm_crtc_helper_funcs tegra_crtc_helper_funcs = {
	.dpms = tegra_crtc_dpms,
	.mode_fixup = tegra_crtc_mode_fixup,
	.mode_set = tegra_crtc_mode_set,
	.mode_set_base = tegra_crtc_mode_set_base,
	.mode_set_base_atomic = tegra_crtc_mode_set_base_atomic,
	.disable = tegra_crtc_disable,
	.prepare = tegra_crtc_prepare,
	.commit = tegra_crtc_commit
};

static int	tegra_encoder_init(struct drm_device *);
static void	tegra_encoder_destroy(struct drm_encoder *);

static const struct drm_encoder_funcs tegra_encoder_funcs = {
	.destroy = tegra_encoder_destroy
};

static void	tegra_encoder_dpms(struct drm_encoder *, int);
static bool	tegra_encoder_mode_fixup(struct drm_encoder *,
		    const struct drm_display_mode *, struct drm_display_mode *);
static void	tegra_encoder_mode_set(struct drm_encoder *,
		    struct drm_display_mode *, struct drm_display_mode *);
static void	tegra_encoder_prepare(struct drm_encoder *);
static void	tegra_encoder_commit(struct drm_encoder *);

static const struct drm_encoder_helper_funcs tegra_encoder_helper_funcs = {
	.dpms = tegra_encoder_dpms,
	.mode_fixup = tegra_encoder_mode_fixup,
	.prepare = tegra_encoder_prepare,
	.commit = tegra_encoder_commit,
	.mode_set = tegra_encoder_mode_set
};

static int	tegra_connector_init(struct drm_device *, struct drm_encoder *);
static void	tegra_connector_destroy(struct drm_connector *);
static enum drm_connector_status tegra_connector_detect(struct drm_connector *,
		    bool);

static const struct drm_connector_funcs tegra_connector_funcs = {
	.dpms = drm_helper_connector_dpms,
	.detect = tegra_connector_detect,
	.fill_modes = drm_helper_probe_single_connector_modes,
	.destroy = tegra_connector_destroy
};

static int	tegra_connector_mode_valid(struct drm_connector *,
		    struct drm_display_mode *);
static int	tegra_connector_get_modes(struct drm_connector *);
static struct drm_encoder *tegra_connector_best_encoder(struct drm_connector *);

static const struct drm_connector_helper_funcs tegra_connector_helper_funcs = {
	.mode_valid = tegra_connector_mode_valid,
	.get_modes = tegra_connector_get_modes,
	.best_encoder = tegra_connector_best_encoder
};

static const struct tegra_hdmi_tmds_config {
	u_int		dot_clock;
	uint32_t	sor_pll0;
	uint32_t	sor_pll1;
	uint32_t	sor_lane_drive_current;
	uint32_t	pe_current;
	uint32_t	sor_io_peak_current;
	uint32_t	sor_pad_ctls0;
	uint32_t	car_plld_misc;	/* XXX unused? */
} tegra_hdmi_tmds_config[] = {
	/* 480p */
	{ 27000,      0x01003010, 0x00301b00, 0x1f1f1f1f,
	  0x00000000, 0x03030303, 0x800034bb, 0x40400820 },
	/* 720p / 1080i */
	{ 74250,      0x01003110, 0x00301500, 0x2c2c2c2c,
	  0x00000000, 0x07070707, 0x800034bb, 0x40400820 },
	/* 1080p */
	{ 148500,     0x01003310, 0x00301500, 0x2d2d2d2d,
	  0x00000000, 0x05050505, 0x800034bb, 0x40400820 },
	/* 2160p */
	{ 297000,     0x01003f10, 0x00300f00, 0x37373737,
	  0x00000000, 0x17171717, 0x800036bb, 0x40400f20 },
};

int
tegra_drm_mode_init(struct drm_device *ddev)
{
	int error;

	drm_mode_config_init(ddev);
	ddev->mode_config.min_width = 0;
	ddev->mode_config.min_height = 0;
	ddev->mode_config.max_width = 4096;
	ddev->mode_config.max_height = 2160;
	ddev->mode_config.funcs = &tegra_mode_config_funcs;

	error = tegra_crtc_init(ddev, 0);
	if (error)
		return error;

	error = tegra_crtc_init(ddev, 1);
	if (error)
		return error;

	error = tegra_encoder_init(ddev);
	if (error)
		return error;

	error = drm_vblank_init(ddev, 2);
	if (error)
		return error;

	return 0;
}

int
tegra_drm_framebuffer_init(struct drm_device *ddev,
    struct tegra_framebuffer *fb)
{
	return drm_framebuffer_init(ddev, &fb->base, &tegra_framebuffer_funcs);
}

static struct drm_framebuffer *
tegra_fb_create(struct drm_device *ddev, struct drm_file *file,
    struct drm_mode_fb_cmd2 *cmd)
{
	struct tegra_framebuffer *fb;
	struct drm_gem_object *gem_obj;
	int error;

	if (cmd->flags)
		return NULL;
	if (cmd->pixel_format != DRM_FORMAT_ARGB8888 &&
	    cmd->pixel_format != DRM_FORMAT_XRGB8888) {
		return NULL;
	}

	gem_obj = drm_gem_object_lookup(ddev, file, cmd->handles[0]);
	if (gem_obj == NULL)
		return NULL;

	fb = kmem_zalloc(sizeof(*fb), KM_SLEEP);
	fb->obj = to_tegra_gem_obj(gem_obj);
	fb->base.pitches[0] = cmd->pitches[0];
	fb->base.offsets[0] = cmd->offsets[0];
	fb->base.width = cmd->width;
	fb->base.height = cmd->height;
	fb->base.pixel_format = cmd->pixel_format;
	drm_fb_get_bpp_depth(cmd->pixel_format, &fb->base.depth,
	    &fb->base.bits_per_pixel);

	error = tegra_drm_framebuffer_init(ddev, fb);
	if (error)
		goto dealloc;

	return &fb->base;

	drm_framebuffer_cleanup(&fb->base);
dealloc:
	kmem_free(fb, sizeof(*fb));
	drm_gem_object_unreference_unlocked(gem_obj);

	return NULL;
}

static int
tegra_framebuffer_create_handle(struct drm_framebuffer *fb,
    struct drm_file *file, unsigned int *handle)
{
	struct tegra_framebuffer *tegra_fb = to_tegra_framebuffer(fb);

	return drm_gem_handle_create(file, &tegra_fb->obj->base, handle);
}

static void
tegra_framebuffer_destroy(struct drm_framebuffer *fb)
{
	struct tegra_framebuffer *tegra_fb = to_tegra_framebuffer(fb);

	drm_framebuffer_cleanup(fb);
	drm_gem_object_unreference_unlocked(&tegra_fb->obj->base);
	kmem_free(tegra_fb, sizeof(*tegra_fb));
}

static int
tegra_crtc_init(struct drm_device *ddev, int index)
{
	struct tegra_drm_softc * const sc = tegra_drm_private(ddev);
	struct tegra_crtc *crtc;
	bus_addr_t offset;
	bus_size_t size;
	u_int intr;
	int error;

	if (sc->sc_clk_dc[index] == NULL ||
	    sc->sc_clk_hdmi_parent == NULL ||
	    sc->sc_rst_dc[index] == NULL) {
		DRM_ERROR("no clocks configured for crtc %d\n", index);
		return -EIO;
	}

	switch (index) {
	case 0:
		offset = TEGRA_GHOST_BASE + TEGRA_DISPLAYA_OFFSET;
		size = TEGRA_DISPLAYA_SIZE;
		intr = TEGRA_INTR_DISPLAYA;
		break;
	case 1:
		offset = TEGRA_GHOST_BASE + TEGRA_DISPLAYB_OFFSET;
		size = TEGRA_DISPLAYB_SIZE;
		intr = TEGRA_INTR_DISPLAYB;
		break;
	default:
		return -EINVAL;
	}

	crtc = kmem_zalloc(sizeof(*crtc), KM_SLEEP);
	crtc->index = index;
	crtc->bst = sc->sc_bst;
	error = bus_space_map(crtc->bst, offset, size, 0, &crtc->bsh);
	if (error) {
		kmem_free(crtc, sizeof(*crtc));
		return -error;
	}
	crtc->size = size;
	crtc->intr = intr;
	crtc->ih = intr_establish(intr, IPL_VM, IST_LEVEL | IST_MPSAFE,
	    tegra_crtc_intr, crtc);
	if (crtc->ih == NULL) {
		DRM_ERROR("failed to establish interrupt for crtc %d\n", index);
	}
	const size_t cursor_size = 256 * 256 * 4;
	crtc->cursor_obj = tegra_drm_obj_alloc(ddev, cursor_size);
	if (crtc->cursor_obj == NULL) {
		kmem_free(crtc, sizeof(*crtc));
		return -ENOMEM;
	}

	/* Enter reset */
	fdtbus_reset_assert(sc->sc_rst_dc[index]);

	/* Turn on power to display partition */
	const u_int pmc_partid = index == 0 ? PMC_PARTID_DIS : PMC_PARTID_DISB;
	tegra_pmc_power(pmc_partid, true);
	tegra_pmc_remove_clamping(pmc_partid);

	/* Set parent clock to the HDMI parent (ignoring DC parent in DT!) */
	error = clk_set_parent(sc->sc_clk_dc[index],
	    sc->sc_clk_hdmi_parent);
	if (error) {
		DRM_ERROR("failed to set crtc %d clock parent: %d\n",
		    index, error);
		return -error;
	}

	/* Enable DC clock */
	error = clk_enable(sc->sc_clk_dc[index]);
	if (error) {
		DRM_ERROR("failed to enable crtc %d clock: %d\n", index, error);
		return -error;
	}

	/* Leave reset */
	fdtbus_reset_deassert(sc->sc_rst_dc[index]);

	crtc->clk_parent = sc->sc_clk_hdmi_parent;

	DC_WRITE(crtc, DC_CMD_INT_ENABLE_REG, DC_CMD_INT_V_BLANK);

	drm_crtc_init(ddev, &crtc->base, &tegra_crtc_funcs);
	drm_crtc_helper_add(&crtc->base, &tegra_crtc_helper_funcs);

	return 0;
}

static int
tegra_crtc_cursor_set(struct drm_crtc *crtc, struct drm_file *file_priv,
    uint32_t handle, uint32_t width, uint32_t height)
{
	struct tegra_crtc *tegra_crtc = to_tegra_crtc(crtc);
	struct drm_gem_object *gem_obj = NULL;
	struct tegra_gem_object *obj;
	uint32_t cfg, opt;
	int error;

	if (tegra_crtc->enabled == false)
		return 0;

	if (handle == 0) {
		/* hide cursor */
		opt = DC_READ(tegra_crtc, DC_DISP_DISP_WIN_OPTIONS_REG);
		if ((opt & DC_DISP_DISP_WIN_OPTIONS_CURSOR_ENABLE) != 0) {
			opt &= ~DC_DISP_DISP_WIN_OPTIONS_CURSOR_ENABLE;
			DC_WRITE(tegra_crtc, DC_DISP_DISP_WIN_OPTIONS_REG, opt);
			/* Commit settings */
			DC_WRITE(tegra_crtc, DC_CMD_STATE_CONTROL_REG,
			    DC_CMD_STATE_CONTROL_GENERAL_UPDATE);
			DC_WRITE(tegra_crtc, DC_CMD_STATE_CONTROL_REG,
			    DC_CMD_STATE_CONTROL_GENERAL_ACT_REQ);
		}
		error = 0;
		goto done;
	}

	if ((width != height) ||
	    (width != 32 && width != 64 && width != 128 && width != 256)) {
		DRM_ERROR("Cursor dimension %ux%u not supported\n",
		    width, height);
		error = -EINVAL;
		goto done;
	}

	gem_obj = drm_gem_object_lookup(crtc->dev, file_priv, handle);
	if (gem_obj == NULL) {
		DRM_ERROR("Cannot find cursor object %#x for crtc %d\n",
		    handle, tegra_crtc->index);
		error = -ENOENT;
		goto done;
	}
	obj = to_tegra_gem_obj(gem_obj);

	if (obj->base.size < width * height * 4) {
		DRM_ERROR("Cursor buffer is too small\n");
		error = -ENOMEM;
		goto done;
	}

	cfg = __SHIFTIN(DC_DISP_CURSOR_START_ADDR_CLIPPING_DISPLAY,
			DC_DISP_CURSOR_START_ADDR_CLIPPING);
	switch (width) {
	case 32:
		cfg |= __SHIFTIN(DC_DISP_CURSOR_START_ADDR_SIZE_32,
				 DC_DISP_CURSOR_START_ADDR_SIZE);
		break;
	case 64:
		cfg |= __SHIFTIN(DC_DISP_CURSOR_START_ADDR_SIZE_64,
				 DC_DISP_CURSOR_START_ADDR_SIZE);
		break;
	case 128:
		cfg |= __SHIFTIN(DC_DISP_CURSOR_START_ADDR_SIZE_128,
				 DC_DISP_CURSOR_START_ADDR_SIZE);
		break;
	case 256:
		cfg |= __SHIFTIN(DC_DISP_CURSOR_START_ADDR_SIZE_256,
				 DC_DISP_CURSOR_START_ADDR_SIZE);
		break;
	}

	/* copy cursor (argb -> rgba) */
	struct tegra_gem_object *cursor_obj = tegra_crtc->cursor_obj;
	uint32_t off, *cp = obj->dmap, *crtc_cp = cursor_obj->dmap;
	for (off = 0; off < width * height; off++) {
		crtc_cp[off] = (cp[off] << 8) | (cp[off] >> 24);
	}

	const uint64_t paddr = cursor_obj->dmasegs[0].ds_addr;

	DC_WRITE(tegra_crtc, DC_DISP_CURSOR_START_ADDR_HI_REG,
	    (paddr >> 32) & 3);
	cfg |= __SHIFTIN((paddr >> 10) & 0x3fffff,
			 DC_DISP_CURSOR_START_ADDR_ADDRESS_LO);
	const uint32_t ocfg =
	    DC_READ(tegra_crtc, DC_DISP_CURSOR_START_ADDR_REG);
	if (cfg != ocfg) {
		DC_WRITE(tegra_crtc, DC_DISP_CURSOR_START_ADDR_REG, cfg);
	}

	cfg = DC_READ(tegra_crtc, DC_DISP_BLEND_CURSOR_CONTROL_REG);
	cfg &= ~DC_DISP_BLEND_CURSOR_CONTROL_DST_BLEND_FACTOR_SEL;
	cfg |= __SHIFTIN(2, DC_DISP_BLEND_CURSOR_CONTROL_DST_BLEND_FACTOR_SEL);
	cfg &= ~DC_DISP_BLEND_CURSOR_CONTROL_SRC_BLEND_FACTOR_SEL;
	cfg |= __SHIFTIN(1, DC_DISP_BLEND_CURSOR_CONTROL_SRC_BLEND_FACTOR_SEL);
	cfg &= ~DC_DISP_BLEND_CURSOR_CONTROL_ALPHA;
	cfg |= __SHIFTIN(255, DC_DISP_BLEND_CURSOR_CONTROL_ALPHA);
	cfg |= DC_DISP_BLEND_CURSOR_CONTROL_MODE_SEL;
	DC_WRITE(tegra_crtc, DC_DISP_BLEND_CURSOR_CONTROL_REG, cfg);

	/* set cursor position */
	DC_WRITE(tegra_crtc, DC_DISP_CURSOR_POSITION_REG,
	    __SHIFTIN(tegra_crtc->cursor_x, DC_DISP_CURSOR_POSITION_H) |
	    __SHIFTIN(tegra_crtc->cursor_y, DC_DISP_CURSOR_POSITION_V));

	/* Commit settings */
	DC_WRITE(tegra_crtc, DC_CMD_STATE_CONTROL_REG,
	    DC_CMD_STATE_CONTROL_CURSOR_UPDATE);
	DC_WRITE(tegra_crtc, DC_CMD_STATE_CONTROL_REG,
	    DC_CMD_STATE_CONTROL_CURSOR_ACT_REQ);

	/* show cursor */
	opt = DC_READ(tegra_crtc, DC_DISP_DISP_WIN_OPTIONS_REG);
	if ((opt & DC_DISP_DISP_WIN_OPTIONS_CURSOR_ENABLE) == 0) {
		opt |= DC_DISP_DISP_WIN_OPTIONS_CURSOR_ENABLE;
		DC_WRITE(tegra_crtc, DC_DISP_DISP_WIN_OPTIONS_REG, opt);

		/* Commit settings */
		DC_WRITE(tegra_crtc, DC_CMD_STATE_CONTROL_REG,
		    DC_CMD_STATE_CONTROL_GENERAL_UPDATE);
		DC_WRITE(tegra_crtc, DC_CMD_STATE_CONTROL_REG,
		    DC_CMD_STATE_CONTROL_GENERAL_ACT_REQ);
	}

	error = 0;

done:
	if (error == 0) {
		/* Wait for activation request to complete */
		while (DC_READ(tegra_crtc, DC_CMD_STATE_CONTROL_REG) &
		    DC_CMD_STATE_CONTROL_GENERAL_ACT_REQ)
			;
	}

	if (gem_obj) {
		drm_gem_object_unreference_unlocked(gem_obj);
	}

	return error;
}

static int
tegra_crtc_cursor_move(struct drm_crtc *crtc, int x, int y)
{
	struct tegra_crtc *tegra_crtc = to_tegra_crtc(crtc);

	tegra_crtc->cursor_x = x & 0x3fff;
	tegra_crtc->cursor_y = y & 0x3fff;

	DC_WRITE(tegra_crtc, DC_DISP_CURSOR_POSITION_REG,
	    __SHIFTIN(x & 0x3fff, DC_DISP_CURSOR_POSITION_H) |
	    __SHIFTIN(y & 0x3fff, DC_DISP_CURSOR_POSITION_V));

	/* Commit settings */
	DC_WRITE(tegra_crtc, DC_CMD_STATE_CONTROL_REG,
	    DC_CMD_STATE_CONTROL_CURSOR_UPDATE);
	DC_WRITE(tegra_crtc, DC_CMD_STATE_CONTROL_REG,
	    DC_CMD_STATE_CONTROL_CURSOR_ACT_REQ);

	return 0;
}

static void
tegra_crtc_destroy(struct drm_crtc *crtc)
{
	struct tegra_crtc *tegra_crtc = to_tegra_crtc(crtc);
	drm_crtc_cleanup(crtc);
	if (tegra_crtc->ih) {
		intr_disestablish(tegra_crtc->ih);
	}
	tegra_drm_obj_free(tegra_crtc->cursor_obj);
	bus_space_unmap(tegra_crtc->bst, tegra_crtc->bsh, tegra_crtc->size);
	kmem_free(tegra_crtc, sizeof(*tegra_crtc));
}

static void
tegra_crtc_dpms(struct drm_crtc *crtc, int mode)
{
	struct tegra_crtc *tegra_crtc = to_tegra_crtc(crtc);

	switch (mode) {
	case DRM_MODE_DPMS_ON:
	case DRM_MODE_DPMS_STANDBY:
	case DRM_MODE_DPMS_SUSPEND:
		DC_SET_CLEAR(tegra_crtc, DC_WINC_A_WIN_OPTIONS_REG,
		    DC_WINC_A_WIN_OPTIONS_WIN_ENABLE, 0);
		break;
	case DRM_MODE_DPMS_OFF:
		DC_SET_CLEAR(tegra_crtc, DC_WINC_A_WIN_OPTIONS_REG,
		    0, DC_WINC_A_WIN_OPTIONS_WIN_ENABLE);
		break;
	}
}

static bool
tegra_crtc_mode_fixup(struct drm_crtc *crtc,
    const struct drm_display_mode *mode, struct drm_display_mode *adjusted_mode)
{
	return true;
}

static int
tegra_crtc_mode_set(struct drm_crtc *crtc, struct drm_display_mode *mode,
    struct drm_display_mode *adjusted_mode, int x, int y,
    struct drm_framebuffer *old_fb)
{
	struct tegra_crtc *tegra_crtc = to_tegra_crtc(crtc);
	const u_int hspw = mode->crtc_hsync_end - mode->crtc_hsync_start;
	const u_int hbp = mode->crtc_htotal - mode->crtc_hsync_end;
	const u_int hfp = mode->crtc_hsync_start - mode->crtc_hdisplay;
	const u_int vspw = mode->crtc_vsync_end - mode->crtc_vsync_start;
	const u_int vbp = mode->crtc_vtotal - mode->crtc_vsync_end;
	const u_int vfp = mode->crtc_vsync_start - mode->crtc_vdisplay;

	/* Set colour depth to ARGB8888 */
	DC_WRITE(tegra_crtc, DC_WINC_A_COLOR_DEPTH_REG,
	    __SHIFTIN(DC_WINC_A_COLOR_DEPTH_DEPTH_T_A8R8G8B8,
	    DC_WINC_A_COLOR_DEPTH_DEPTH));

	/* Disable byte swapping */
	DC_WRITE(tegra_crtc, DC_WINC_A_BYTE_SWAP_REG,
	    __SHIFTIN(DC_WINC_A_BYTE_SWAP_SWAP_NOSWAP,
	    DC_WINC_A_BYTE_SWAP_SWAP));

	/* Initial DDA */
	DC_WRITE(tegra_crtc, DC_WINC_A_H_INITIAL_DDA_REG, 0);
	DC_WRITE(tegra_crtc, DC_WINC_A_V_INITIAL_DDA_REG, 0);
	DC_WRITE(tegra_crtc, DC_WINC_A_DDA_INCREMENT_REG, 0x10001000);

	/* Window position, size, stride */
	DC_WRITE(tegra_crtc, DC_WINC_A_POSITION_REG,
	    __SHIFTIN(0, DC_WINC_A_POSITION_V) |
	    __SHIFTIN(0, DC_WINC_A_POSITION_H));
	DC_WRITE(tegra_crtc, DC_WINC_A_SIZE_REG,
	    __SHIFTIN(mode->crtc_vdisplay, DC_WINC_A_SIZE_V) |
	    __SHIFTIN(mode->crtc_hdisplay, DC_WINC_A_SIZE_H));
	DC_WRITE(tegra_crtc, DC_WINC_A_PRESCALED_SIZE_REG,
	    __SHIFTIN(mode->crtc_vdisplay, DC_WINC_A_PRESCALED_SIZE_V) |
	    __SHIFTIN(mode->crtc_hdisplay * (TEGRA_DC_DEPTH / 8),
		      DC_WINC_A_PRESCALED_SIZE_H));
	DC_WRITE(tegra_crtc, DC_WINC_A_LINE_STRIDE_REG,
	    __SHIFTIN(mode->crtc_hdisplay * (TEGRA_DC_DEPTH / 8),
	    DC_WINC_A_LINE_STRIDE_LINE_STRIDE));

	tegra_crtc_do_set_base(crtc, old_fb, x, y, 0);

	/* Enable window A */
	DC_WRITE(tegra_crtc, DC_WINC_A_WIN_OPTIONS_REG,
	    DC_WINC_A_WIN_OPTIONS_WIN_ENABLE);

	/* Timing and signal options */
	DC_WRITE(tegra_crtc, DC_DISP_DISP_TIMING_OPTIONS_REG,
	    __SHIFTIN(1, DC_DISP_DISP_TIMING_OPTIONS_VSYNC_POS));
	DC_WRITE(tegra_crtc, DC_DISP_DISP_COLOR_CONTROL_REG,
	    __SHIFTIN(DC_DISP_DISP_COLOR_CONTROL_BASE_COLOR_SIZE_888,
		      DC_DISP_DISP_COLOR_CONTROL_BASE_COLOR_SIZE));
	DC_WRITE(tegra_crtc, DC_DISP_DISP_SIGNAL_OPTIONS0_REG,
	    DC_DISP_DISP_SIGNAL_OPTIONS0_H_PULSE2_ENABLE);
	DC_WRITE(tegra_crtc, DC_DISP_H_PULSE2_CONTROL_REG,
	    __SHIFTIN(DC_DISP_H_PULSE2_CONTROL_V_QUAL_VACTIVE,
		      DC_DISP_H_PULSE2_CONTROL_V_QUAL) |
	    __SHIFTIN(DC_DISP_H_PULSE2_CONTROL_LAST_END_A,
		      DC_DISP_H_PULSE2_CONTROL_LAST));

	const u_int pulse_start = 1 + hspw + hbp - 10;
	DC_WRITE(tegra_crtc, DC_DISP_H_PULSE2_POSITION_A_REG,
	    __SHIFTIN(pulse_start, DC_DISP_H_PULSE2_POSITION_A_START) |
	    __SHIFTIN(pulse_start + 8, DC_DISP_H_PULSE2_POSITION_A_END));

	/* Pixel clock */
	const u_int parent_rate = clk_get_rate(tegra_crtc->clk_parent);
	const u_int div = (parent_rate * 2) / (mode->crtc_clock * 1000) - 2;
	DC_WRITE(tegra_crtc, DC_DISP_DISP_CLOCK_CONTROL_REG,
	    __SHIFTIN(0, DC_DISP_DISP_CLOCK_CONTROL_PIXEL_CLK_DIVIDER) |
	    __SHIFTIN(div, DC_DISP_DISP_CLOCK_CONTROL_SHIFT_CLK_DIVIDER));

	/* Mode timings */
	DC_WRITE(tegra_crtc, DC_DISP_REF_TO_SYNC_REG,
	    __SHIFTIN(1, DC_DISP_REF_TO_SYNC_V) |
	    __SHIFTIN(1, DC_DISP_REF_TO_SYNC_H));
	DC_WRITE(tegra_crtc, DC_DISP_SYNC_WIDTH_REG,
	    __SHIFTIN(vspw, DC_DISP_SYNC_WIDTH_V) |
	    __SHIFTIN(hspw, DC_DISP_SYNC_WIDTH_H));
	DC_WRITE(tegra_crtc, DC_DISP_BACK_PORCH_REG,
	    __SHIFTIN(vbp, DC_DISP_BACK_PORCH_V) |
	    __SHIFTIN(hbp, DC_DISP_BACK_PORCH_H));
	DC_WRITE(tegra_crtc, DC_DISP_FRONT_PORCH_REG,
	    __SHIFTIN(vfp, DC_DISP_FRONT_PORCH_V) |
	    __SHIFTIN(hfp, DC_DISP_FRONT_PORCH_H));
	DC_WRITE(tegra_crtc, DC_DISP_DISP_ACTIVE_REG,
	    __SHIFTIN(mode->crtc_vdisplay, DC_DISP_DISP_ACTIVE_V) |
	    __SHIFTIN(mode->crtc_hdisplay, DC_DISP_DISP_ACTIVE_H));

	return 0;
}

static int
tegra_crtc_do_set_base(struct drm_crtc *crtc, struct drm_framebuffer *fb,
    int x, int y, int atomic)
{
	struct tegra_crtc *tegra_crtc = to_tegra_crtc(crtc);
	struct tegra_framebuffer *tegra_fb = atomic ?
	    to_tegra_framebuffer(fb) :
	    to_tegra_framebuffer(crtc->primary->fb);

	uint64_t paddr = (uint64_t)tegra_fb->obj->dmamap->dm_segs[0].ds_addr;

	/* Framebuffer start address */
	DC_WRITE(tegra_crtc, DC_WINBUF_A_START_ADDR_HI_REG, (paddr >> 32) & 3);
	DC_WRITE(tegra_crtc, DC_WINBUF_A_START_ADDR_REG, paddr & 0xffffffff);

	/* Offsets */
	DC_WRITE(tegra_crtc, DC_WINBUF_A_ADDR_H_OFFSET_REG, x);
	DC_WRITE(tegra_crtc, DC_WINBUF_A_ADDR_V_OFFSET_REG, y);

	/* Surface kind */
	DC_WRITE(tegra_crtc, DC_WINBUF_A_SURFACE_KIND_REG,
	    __SHIFTIN(DC_WINBUF_A_SURFACE_KIND_SURFACE_KIND_PITCH,
	    DC_WINBUF_A_SURFACE_KIND_SURFACE_KIND));

	return 0;
}

static int
tegra_crtc_mode_set_base(struct drm_crtc *crtc, int x, int y,
    struct drm_framebuffer *old_fb)
{
	struct tegra_crtc *tegra_crtc = to_tegra_crtc(crtc);
	
	tegra_crtc_do_set_base(crtc, old_fb, x, y, 0);

	/* Commit settings */
	DC_WRITE(tegra_crtc, DC_CMD_STATE_CONTROL_REG,
	    DC_CMD_STATE_CONTROL_WIN_A_UPDATE);
	DC_WRITE(tegra_crtc, DC_CMD_STATE_CONTROL_REG,
	    DC_CMD_STATE_CONTROL_WIN_A_ACT_REQ);

	return 0;
}

static int
tegra_crtc_mode_set_base_atomic(struct drm_crtc *crtc,
    struct drm_framebuffer *fb, int x, int y, enum mode_set_atomic state)
{
	struct tegra_crtc *tegra_crtc = to_tegra_crtc(crtc);

	tegra_crtc_do_set_base(crtc, fb, x, y, 1);

	/* Commit settings */
	DC_WRITE(tegra_crtc, DC_CMD_STATE_CONTROL_REG,
	    DC_CMD_STATE_CONTROL_WIN_A_UPDATE);
	DC_WRITE(tegra_crtc, DC_CMD_STATE_CONTROL_REG,
	    DC_CMD_STATE_CONTROL_WIN_A_ACT_REQ);

	return 0;
}

static void
tegra_crtc_disable(struct drm_crtc *crtc)
{
}

static void
tegra_crtc_prepare(struct drm_crtc *crtc)
{
	struct tegra_crtc *tegra_crtc = to_tegra_crtc(crtc);

	/* Access control */
	DC_WRITE(tegra_crtc, DC_CMD_STATE_ACCESS_REG, 0);

	/* Enable window A programming */
	DC_WRITE(tegra_crtc, DC_CMD_DISPLAY_WINDOW_HEADER_REG,
	    DC_CMD_DISPLAY_WINDOW_HEADER_WINDOW_A_SELECT);
}

static void
tegra_crtc_commit(struct drm_crtc *crtc)
{
	struct tegra_crtc *tegra_crtc = to_tegra_crtc(crtc);

	/* Enable continuous display mode */
	DC_WRITE(tegra_crtc, DC_CMD_DISPLAY_COMMAND_REG,
	    __SHIFTIN(DC_CMD_DISPLAY_COMMAND_DISPLAY_CTRL_MODE_C_DISPLAY,
	    DC_CMD_DISPLAY_COMMAND_DISPLAY_CTRL_MODE));

	/* Enable power */
	DC_SET_CLEAR(tegra_crtc, DC_CMD_DISPLAY_POWER_CONTROL_REG,
	    DC_CMD_DISPLAY_POWER_CONTROL_PM1_ENABLE |
	    DC_CMD_DISPLAY_POWER_CONTROL_PM0_ENABLE |
	    DC_CMD_DISPLAY_POWER_CONTROL_PW4_ENABLE |
	    DC_CMD_DISPLAY_POWER_CONTROL_PW3_ENABLE |
	    DC_CMD_DISPLAY_POWER_CONTROL_PW2_ENABLE |
	    DC_CMD_DISPLAY_POWER_CONTROL_PW1_ENABLE |
	    DC_CMD_DISPLAY_POWER_CONTROL_PW0_ENABLE,
	    0);

	/* Commit settings */
	DC_WRITE(tegra_crtc, DC_CMD_STATE_CONTROL_REG,
	    DC_CMD_STATE_CONTROL_GENERAL_UPDATE |
	    DC_CMD_STATE_CONTROL_WIN_A_UPDATE);
	DC_WRITE(tegra_crtc, DC_CMD_STATE_CONTROL_REG,
	    DC_CMD_STATE_CONTROL_GENERAL_ACT_REQ |
	    DC_CMD_STATE_CONTROL_WIN_A_ACT_REQ);

	tegra_crtc->enabled = true;
}

static int
tegra_encoder_init(struct drm_device *ddev)
{
	struct tegra_drm_softc * const sc = tegra_drm_private(ddev);
	struct tegra_encoder *encoder;
	int error;

	if (sc->sc_clk_hdmi == NULL ||
	    sc->sc_clk_hdmi_parent == NULL ||
	    sc->sc_rst_hdmi == NULL) {
		DRM_ERROR("no clocks configured for hdmi\n");
		DRM_ERROR("clk: hdmi %p parent %p\n", sc->sc_clk_hdmi, sc->sc_clk_hdmi_parent);
		DRM_ERROR("rst: hdmi %p\n", sc->sc_rst_hdmi);
		return -EIO;
	}

	const bus_addr_t offset = TEGRA_GHOST_BASE + TEGRA_HDMI_OFFSET;
	const bus_size_t size = TEGRA_HDMI_SIZE;

	encoder = kmem_zalloc(sizeof(*encoder), KM_SLEEP);
	encoder->bst = sc->sc_bst;
	error = bus_space_map(encoder->bst, offset, size, 0, &encoder->bsh);
	if (error) {
		kmem_free(encoder, sizeof(*encoder));
		return -error;
	}
	encoder->size = size;

	tegra_pmc_hdmi_enable();

	/* Enable parent PLL */
	error = clk_set_rate(sc->sc_clk_hdmi_parent, 594000000);
	if (error) {
		DRM_ERROR("couldn't set hdmi parent PLL rate: %d\n", error);
		return -error;
	}
	error = clk_enable(sc->sc_clk_hdmi_parent);
	if (error) {
		DRM_ERROR("couldn't enable hdmi parent PLL: %d\n", error);
		return -error;
	}

	drm_encoder_init(ddev, &encoder->base, &tegra_encoder_funcs,
	    DRM_MODE_ENCODER_TMDS);
	drm_encoder_helper_add(&encoder->base, &tegra_encoder_helper_funcs);

	encoder->base.possible_crtcs = (1 << 0) | (1 << 1);

	return tegra_connector_init(ddev, &encoder->base);
}

static void
tegra_encoder_destroy(struct drm_encoder *encoder)
{
	struct tegra_encoder *tegra_encoder = to_tegra_encoder(encoder);
	drm_encoder_cleanup(encoder);
	kmem_free(tegra_encoder, sizeof(*tegra_encoder));
}

static void
tegra_encoder_dpms(struct drm_encoder *encoder, int mode)
{
	struct tegra_encoder *tegra_encoder = to_tegra_encoder(encoder);

	if (encoder->crtc == NULL)
		return;

	switch (mode) {
	case DRM_MODE_DPMS_ON:
	case DRM_MODE_DPMS_STANDBY:
	case DRM_MODE_DPMS_SUSPEND:
		HDMI_SET_CLEAR(tegra_encoder, HDMI_NV_PDISP_SOR_BLANK_REG,
		    0, HDMI_NV_PDISP_SOR_BLANK_OVERRIDE);
		break;
	case DRM_MODE_DPMS_OFF:
		HDMI_SET_CLEAR(tegra_encoder, HDMI_NV_PDISP_SOR_BLANK_REG,
		    HDMI_NV_PDISP_SOR_BLANK_OVERRIDE, 0);
		break;
	}
}

static bool
tegra_encoder_mode_fixup(struct drm_encoder *encoder,
    const struct drm_display_mode *mode, struct drm_display_mode *adjusted_mode)
{
	return true;
}

static int
tegra_encoder_hdmi_set_clock(struct drm_encoder *encoder, u_int rate)
{
	struct drm_device *ddev = encoder->dev;
	struct tegra_drm_softc * const sc = tegra_drm_private(ddev);
	int error;

	/* Enter reset */
	fdtbus_reset_assert(sc->sc_rst_hdmi);

	/* Set HDMI parent clock */
	error = clk_set_parent(sc->sc_clk_hdmi, sc->sc_clk_hdmi_parent);
	if (error) {
		DRM_ERROR("couldn't set hdmi parent: %d\n", error);
		return -error;
	}

	/* Set dot clock frequency */
	error = clk_set_rate(sc->sc_clk_hdmi, rate);
	if (error) {
		DRM_ERROR("couldn't set hdmi clock: %d\n", error);
		return -error;
	}
	error = clk_enable(sc->sc_clk_hdmi);
	if (error) {
		DRM_ERROR("couldn't enable hdmi clock: %d\n", error);
		return -error;
	}

	/* Leave reset */
	fdtbus_reset_deassert(sc->sc_rst_hdmi);

	return 0;
}

static void
tegra_encoder_mode_set(struct drm_encoder *encoder,
    struct drm_display_mode *mode, struct drm_display_mode *adjusted_mode)
{
	struct drm_device *ddev = encoder->dev;
	struct tegra_encoder *tegra_encoder = to_tegra_encoder(encoder);
	struct tegra_crtc *tegra_crtc = to_tegra_crtc(encoder->crtc);
	struct tegra_connector *tegra_connector = NULL;
	struct drm_connector *connector;
	const struct tegra_hdmi_tmds_config *tmds = NULL;
	uint32_t input_ctrl;
	int retry;
	u_int i;

	tegra_encoder_hdmi_set_clock(encoder, mode->crtc_clock * 1000);

	/* find the connector for this encoder */
	list_for_each_entry(connector, &ddev->mode_config.connector_list, head) {
		if (connector->encoder == encoder) {
			tegra_connector = to_tegra_connector(connector);
			break;
		}
	}

	for (i = 0; i < __arraycount(tegra_hdmi_tmds_config); i++) {
		if (tegra_hdmi_tmds_config[i].dot_clock >= mode->crtc_clock) {
			break;
		}
	}
	if (i < __arraycount(tegra_hdmi_tmds_config)) {
		tmds = &tegra_hdmi_tmds_config[i];
	} else {
		tmds = &tegra_hdmi_tmds_config[__arraycount(tegra_hdmi_tmds_config) - 1];
	}

	HDMI_WRITE(tegra_encoder, HDMI_NV_PDISP_SOR_PLL0_REG, tmds->sor_pll0);
	HDMI_WRITE(tegra_encoder, HDMI_NV_PDISP_SOR_PLL1_REG, tmds->sor_pll1);
	HDMI_WRITE(tegra_encoder, HDMI_NV_PDISP_SOR_LANE_DRIVE_CURRENT_REG,
	    tmds->sor_lane_drive_current);
	HDMI_WRITE(tegra_encoder, HDMI_NV_PDISP_PE_CURRENT_REG,
	    tmds->pe_current);
	HDMI_WRITE(tegra_encoder, HDMI_NV_PDISP_SOR_IO_PEAK_CURRENT_REG,
	    tmds->sor_io_peak_current);
	HDMI_WRITE(tegra_encoder, HDMI_NV_PDISP_SOR_PAD_CTLS0_REG,
	    tmds->sor_pad_ctls0);

	const u_int div = (mode->crtc_clock / 1000) * 4;
	HDMI_WRITE(tegra_encoder, HDMI_NV_PDISP_SOR_REFCLK_REG,
	    __SHIFTIN(div >> 2, HDMI_NV_PDISP_SOR_REFCLK_DIV_INT) |
	    __SHIFTIN(div & 3, HDMI_NV_PDISP_SOR_REFCLK_DIV_FRAC));

	HDMI_SET_CLEAR(tegra_encoder, HDMI_NV_PDISP_SOR_CSTM_REG,
	    __SHIFTIN(HDMI_NV_PDISP_SOR_CSTM_MODE_TMDS,
		      HDMI_NV_PDISP_SOR_CSTM_MODE) |
	    __SHIFTIN(2, HDMI_NV_PDISP_SOR_CSTM_ROTCLK) |
	    HDMI_NV_PDISP_SOR_CSTM_PLLDIV,
	    HDMI_NV_PDISP_SOR_CSTM_MODE |
	    HDMI_NV_PDISP_SOR_CSTM_ROTCLK |
	    HDMI_NV_PDISP_SOR_CSTM_LVDS_EN);

	const uint32_t inst =
	    HDMI_NV_PDISP_SOR_SEQ_INST_DRIVE_PWM_OUT_LO |
	    HDMI_NV_PDISP_SOR_SEQ_INST_HALT |
	    __SHIFTIN(2, HDMI_NV_PDISP_SOR_SEQ_INST_WAIT_UNITS) |
	    __SHIFTIN(1, HDMI_NV_PDISP_SOR_SEQ_INST_WAIT_TIME);
	HDMI_WRITE(tegra_encoder, HDMI_NV_PDISP_SOR_SEQ_INST0_REG, inst);
	HDMI_WRITE(tegra_encoder, HDMI_NV_PDISP_SOR_SEQ_INST8_REG, inst);

	input_ctrl = __SHIFTIN(tegra_crtc->index,
			       HDMI_NV_PDISP_INPUT_CONTROL_HDMI_SRC_SELECT);
	if (mode->crtc_hdisplay != 640 || mode->crtc_vdisplay != 480)
		input_ctrl |= HDMI_NV_PDISP_INPUT_CONTROL_ARM_VIDEO_RANGE;
	HDMI_WRITE(tegra_encoder, HDMI_NV_PDISP_INPUT_CONTROL_REG, input_ctrl);

	/* Start SOR */
	HDMI_SET_CLEAR(tegra_encoder, HDMI_NV_PDISP_SOR_PLL0_REG,
	    0,
	    HDMI_NV_PDISP_SOR_PLL0_PWR |
	    HDMI_NV_PDISP_SOR_PLL0_VCOPD |
	    HDMI_NV_PDISP_SOR_PLL0_PULLDOWN);
	delay(10);
	HDMI_SET_CLEAR(tegra_encoder, HDMI_NV_PDISP_SOR_PLL0_REG,
	    0,
	    HDMI_NV_PDISP_SOR_PLL0_PDBG);

	HDMI_WRITE(tegra_encoder, HDMI_NV_PDISP_SOR_PWR_REG,
	    HDMI_NV_PDISP_SOR_PWR_NORMAL_STATE |
	    HDMI_NV_PDISP_SOR_PWR_SETTING_NEW);

	HDMI_WRITE(tegra_encoder, HDMI_NV_PDISP_SOR_PWR_REG,
	    HDMI_NV_PDISP_SOR_PWR_NORMAL_STATE);

	for (retry = 10000; retry > 0; retry--) {
		const uint32_t pwr = HDMI_READ(tegra_encoder,
		    HDMI_NV_PDISP_SOR_PWR_REG);
		if ((pwr & HDMI_NV_PDISP_SOR_PWR_SETTING_NEW) == 0)
			break;
		delay(10);
	}
	if (retry == 0) {
		DRM_ERROR("timeout enabling SOR power\n");
	}

	uint32_t state2 =
	    __SHIFTIN(1, HDMI_NV_PDISP_SOR_STATE2_ASY_OWNER) |
	    __SHIFTIN(3, HDMI_NV_PDISP_SOR_STATE2_ASY_SUBOWNER) |
	    __SHIFTIN(1, HDMI_NV_PDISP_SOR_STATE2_ASY_CRCMODE) |
	    __SHIFTIN(1, HDMI_NV_PDISP_SOR_STATE2_ASY_PROTOCOL);
	if (mode->flags & DRM_MODE_FLAG_NHSYNC)
		state2 |= HDMI_NV_PDISP_SOR_STATE2_ASY_HSYNCPOL;
	if (mode->flags & DRM_MODE_FLAG_NVSYNC)
		state2 |= HDMI_NV_PDISP_SOR_STATE2_ASY_VSYNCPOL;
	HDMI_WRITE(tegra_encoder, HDMI_NV_PDISP_SOR_STATE2_REG, state2);

	HDMI_WRITE(tegra_encoder, HDMI_NV_PDISP_SOR_STATE1_REG,
	    __SHIFTIN(HDMI_NV_PDISP_SOR_STATE1_ASY_HEAD_OPMODE_AWAKE,
		      HDMI_NV_PDISP_SOR_STATE1_ASY_HEAD_OPMODE) |
	    HDMI_NV_PDISP_SOR_STATE1_ASY_ORMODE);

	HDMI_WRITE(tegra_encoder, HDMI_NV_PDISP_SOR_STATE0_REG, 0);

	HDMI_WRITE(tegra_encoder, HDMI_NV_PDISP_SOR_STATE0_REG,
	    HDMI_NV_PDISP_SOR_STATE0_UPDATE);

	HDMI_SET_CLEAR(tegra_encoder, HDMI_NV_PDISP_SOR_STATE1_REG,
	    HDMI_NV_PDISP_SOR_STATE1_ATTACHED, 0);

	HDMI_WRITE(tegra_encoder, HDMI_NV_PDISP_SOR_STATE0_REG, 0);

	const u_int rekey = 56;
	const u_int hspw = mode->hsync_end - mode->hsync_start;
	const u_int hbp = mode->htotal - mode->hsync_end;
	const u_int hfp = mode->hsync_start - mode->hdisplay;
	const u_int max_ac_packet = (hspw + hbp + hfp - rekey - 18) / 32;
	uint32_t ctrl =
	    __SHIFTIN(rekey, HDMI_NV_PDISP_HDMI_CTRL_REKEY) |
	    __SHIFTIN(max_ac_packet, HDMI_NV_PDISP_HDMI_CTRL_MAX_AC_PACKET);
	if (tegra_connector && tegra_connector->has_hdmi_sink) {
		ctrl |= HDMI_NV_PDISP_HDMI_CTRL_ENABLE; /* HDMI ENABLE */
	}
	HDMI_WRITE(tegra_encoder, HDMI_NV_PDISP_HDMI_CTRL_REG, ctrl);

	if (tegra_connector && tegra_connector->has_hdmi_sink &&
	    tegra_connector->has_audio) {
		struct hdmi_audio_infoframe ai;
		struct hdmi_avi_infoframe avii;
		uint8_t aibuf[HDMI_INFOFRAME_HEADER_SIZE + HDMI_AUDIO_INFOFRAME_SIZE];
		uint8_t aviibuf[HDMI_INFOFRAME_HEADER_SIZE + HDMI_AVI_INFOFRAME_SIZE];
		const u_int n = 6144;	/* 48 kHz */
		const u_int cts = ((mode->crtc_clock * 10) * (n / 128)) / 480;

		HDMI_WRITE(tegra_encoder, HDMI_NV_PDISP_SOR_AUDIO_CNTRL0_REG,
		    __SHIFTIN(HDMI_NV_PDISP_SOR_AUDIO_CNTRL0_SOURCE_SELECT_AUTO,
			      HDMI_NV_PDISP_SOR_AUDIO_CNTRL0_SOURCE_SELECT) |
		    HDMI_NV_PDISP_SOR_AUDIO_CNTRL0_INJECT_NULLSMPL);
		HDMI_WRITE(tegra_encoder, HDMI_NV_PDISP_AUDIO_N_REG,
		    HDMI_NV_PDISP_AUDIO_N_RESETF |
		    HDMI_NV_PDISP_AUDIO_N_GENERATE |
		    __SHIFTIN(n - 1, HDMI_NV_PDISP_AUDIO_N_VALUE));

		HDMI_WRITE(tegra_encoder, HDMI_NV_PDISP_HDMI_SPARE_REG,
		    HDMI_NV_PDISP_HDMI_SPARE_HW_CTS |
		    HDMI_NV_PDISP_HDMI_SPARE_FORCE_SW_CTS |
		    __SHIFTIN(1, HDMI_NV_PDISP_HDMI_SPARE_CTS_RESET_VAL));

		/*
		 * When HW_CTS=1 and FORCE_SW_CTS=1, the CTS is programmed by
		 * software in the 44.1 kHz register regardless of chosen rate.
		 */
		HDMI_WRITE(tegra_encoder,
		    HDMI_NV_PDISP_HDMI_ACR_0441_SUBPACK_LOW_REG,
		    cts << 8);
		HDMI_WRITE(tegra_encoder,
		    HDMI_NV_PDISP_HDMI_ACR_0441_SUBPACK_HIGH_REG,
		    0x80000000 | n);

		HDMI_SET_CLEAR(tegra_encoder, HDMI_NV_PDISP_AUDIO_N_REG, 0,
		    HDMI_NV_PDISP_AUDIO_N_RESETF);

		HDMI_WRITE(tegra_encoder,
		    HDMI_NV_PDISP_SOR_AUDIO_AVAL_0480_REG, 24000);

		hdmi_audio_infoframe_init(&ai);
		ai.channels = 2;
		hdmi_audio_infoframe_pack(&ai, aibuf, sizeof(aibuf));
		HDMI_WRITE(tegra_encoder,
		    HDMI_NV_PDISP_HDMI_AUDIO_INFOFRAME_HEADER_REG,
		    aibuf[0] | (aibuf[1] << 8) | (aibuf[2] << 16));
		HDMI_WRITE(tegra_encoder,
		    HDMI_NV_PDISP_HDMI_AUDIO_INFOFRAME_SUBPACK0_LOW_REG,
		    aibuf[3] | (aibuf[4] << 8) |
		    (aibuf[5] << 16) | (aibuf[6] << 24));
		HDMI_WRITE(tegra_encoder,
		    HDMI_NV_PDISP_HDMI_AUDIO_INFOFRAME_SUBPACK0_HIGH_REG,
		    aibuf[7] | (aibuf[8] << 8) | (aibuf[9] << 16));

		hdmi_avi_infoframe_init(&avii);
		drm_hdmi_avi_infoframe_from_display_mode(&avii, mode);
		hdmi_avi_infoframe_pack(&avii, aviibuf, sizeof(aviibuf));
		HDMI_WRITE(tegra_encoder,
		    HDMI_NV_PDISP_HDMI_AVI_INFOFRAME_HEADER_REG,
		    aviibuf[0] | (aviibuf[1] << 8) | (aviibuf[2] << 16));
		HDMI_WRITE(tegra_encoder,
		    HDMI_NV_PDISP_HDMI_AVI_INFOFRAME_SUBPACK0_LOW_REG,
		    aviibuf[3] | (aviibuf[4] << 8) |
		    (aviibuf[5] << 16) | (aviibuf[6] << 24));
		HDMI_WRITE(tegra_encoder,
		    HDMI_NV_PDISP_HDMI_AVI_INFOFRAME_SUBPACK0_HIGH_REG,
		    aviibuf[7] | (aviibuf[8] << 8) | (aviibuf[9] << 16));
		HDMI_WRITE(tegra_encoder,
		    HDMI_NV_PDISP_HDMI_AVI_INFOFRAME_SUBPACK1_LOW_REG,
		    aviibuf[10] | (aviibuf[11] << 8) |
		    (aviibuf[12] << 16) | (aviibuf[13] << 24));
		HDMI_WRITE(tegra_encoder,
		    HDMI_NV_PDISP_HDMI_AVI_INFOFRAME_SUBPACK1_HIGH_REG,
		    aviibuf[14] | (aviibuf[15] << 8) | (aviibuf[16] << 16));
		
		HDMI_WRITE(tegra_encoder, HDMI_NV_PDISP_HDMI_GENERIC_CTRL_REG,
		    HDMI_NV_PDISP_HDMI_GENERIC_CTRL_AUDIO);
		HDMI_WRITE(tegra_encoder,
		    HDMI_NV_PDISP_HDMI_AVI_INFOFRAME_CTRL_REG,
		    HDMI_NV_PDISP_HDMI_AVI_INFOFRAME_CTRL_ENABLE);
		HDMI_WRITE(tegra_encoder,
		    HDMI_NV_PDISP_HDMI_AUDIO_INFOFRAME_CTRL_REG,
		    HDMI_NV_PDISP_HDMI_AUDIO_INFOFRAME_CTRL_ENABLE);
		HDMI_WRITE(tegra_encoder, HDMI_NV_PDISP_HDMI_ACR_CTRL_REG, 0);
	} else {
		HDMI_WRITE(tegra_encoder,
		    HDMI_NV_PDISP_HDMI_GENERIC_CTRL_REG, 0);
		HDMI_WRITE(tegra_encoder,
		    HDMI_NV_PDISP_HDMI_AVI_INFOFRAME_CTRL_REG, 0);
		HDMI_WRITE(tegra_encoder,
		    HDMI_NV_PDISP_HDMI_AUDIO_INFOFRAME_CTRL_REG, 0);
		HDMI_WRITE(tegra_encoder, HDMI_NV_PDISP_HDMI_ACR_CTRL_REG, 0);
	}

	/* Enable DC output to HDMI */
	DC_SET_CLEAR(tegra_crtc, DC_DISP_DISP_WIN_OPTIONS_REG,
	    DC_DISP_DISP_WIN_OPTIONS_HDMI_ENABLE, 0);

	/* Commit settings */
	DC_WRITE(tegra_crtc, DC_CMD_STATE_CONTROL_REG,
	    DC_CMD_STATE_CONTROL_GENERAL_UPDATE |
	    DC_CMD_STATE_CONTROL_WIN_A_UPDATE);
	DC_WRITE(tegra_crtc, DC_CMD_STATE_CONTROL_REG,
	    DC_CMD_STATE_CONTROL_GENERAL_ACT_REQ |
	    DC_CMD_STATE_CONTROL_WIN_A_ACT_REQ);
}

static void
tegra_encoder_prepare(struct drm_encoder *encoder)
{
}

static void
tegra_encoder_commit(struct drm_encoder *encoder)
{
}

static int
tegra_connector_init(struct drm_device *ddev, struct drm_encoder *encoder)
{
	struct tegra_drm_softc * const sc = tegra_drm_private(ddev);
	struct tegra_connector *connector;

	connector = kmem_zalloc(sizeof(*connector), KM_SLEEP);

	drm_connector_init(ddev, &connector->base, &tegra_connector_funcs,
	    DRM_MODE_CONNECTOR_HDMIA);
	drm_connector_helper_add(&connector->base,
	    &tegra_connector_helper_funcs);

	connector->base.interlace_allowed = 0;
	connector->base.doublescan_allowed = 0;

	drm_sysfs_connector_add(&connector->base);

	connector->base.polled =
	    DRM_CONNECTOR_POLL_CONNECT | DRM_CONNECTOR_POLL_DISCONNECT;

	drm_mode_connector_attach_encoder(&connector->base, encoder);

	connector->hpd = sc->sc_pin_hpd;
	if (!connector->hpd)
		DRM_ERROR("failed to find hpd pin for connector\n");

	connector->ddc = sc->sc_ddc;
	if (!connector->ddc)
		DRM_ERROR("failed to find ddc device for connector\n");

	return 0;
}

static void
tegra_connector_destroy(struct drm_connector *connector)
{
	struct tegra_connector *tegra_connector = to_tegra_connector(connector);

	drm_sysfs_connector_remove(connector);
	drm_connector_cleanup(connector);
	kmem_free(tegra_connector, sizeof(*tegra_connector));
}

static enum drm_connector_status
tegra_connector_detect(struct drm_connector *connector, bool force)
{
	struct tegra_connector *tegra_connector = to_tegra_connector(connector);
	bool con;


	if (!tegra_connector->hpd) {
		tegra_connector->has_hdmi_sink = false;
		tegra_connector->has_audio = false;
		return connector_status_connected;
	}

	con = fdtbus_gpio_read(tegra_connector->hpd);
	if (con) {
		return connector_status_connected;
	} else {
		prop_dictionary_t prop = device_properties(connector->dev->dev);
		prop_dictionary_remove(prop, "physical-address");
		tegra_connector->has_hdmi_sink = false;
		tegra_connector->has_audio = false;
		return connector_status_disconnected;
	}
}

static int
tegra_connector_mode_valid(struct drm_connector *connector,
    struct drm_display_mode *mode)
{
	return MODE_OK;
}

static int
tegra_connector_get_modes(struct drm_connector *connector)
{
	struct tegra_connector *tegra_connector = to_tegra_connector(connector);
	struct tegra_drm_softc * const sc = tegra_drm_private(connector->dev);
	prop_dictionary_t prop = device_properties(connector->dev->dev);
	char edid[EDID_LENGTH * 4];
	struct edid *pedid = NULL;
	int error, block;

	if (tegra_connector->ddc) {
		memset(edid, 0, sizeof(edid));
		for (block = 0; block < 4; block++) {
			error = ddc_read_edid_block(tegra_connector->ddc,
			    &edid[block * EDID_LENGTH], EDID_LENGTH, block);
			if (error)
				break;
			if (block == 0) {
				pedid = (struct edid *)edid;
				if (edid[0x7e] == 0)
					break;
			}
		}
	}

	if (pedid) {
		if (sc->sc_force_dvi) {
			tegra_connector->has_hdmi_sink = false;
			tegra_connector->has_audio = false;
		} else {
			tegra_connector->has_hdmi_sink =
			    drm_detect_hdmi_monitor(pedid);
			tegra_connector->has_audio =
			    drm_detect_monitor_audio(pedid);
		}
		drm_mode_connector_update_edid_property(connector, pedid);
		error = drm_add_edid_modes(connector, pedid);
		drm_edid_to_eld(connector, pedid);
		if (drm_detect_hdmi_monitor(pedid)) {
			prop_dictionary_set_uint16(prop, "physical-address",
			    connector->physical_address);
		}
		return error;
	} else {
		drm_mode_connector_update_edid_property(connector, NULL);
		return 0;
	}
}

static struct drm_encoder *
tegra_connector_best_encoder(struct drm_connector *connector)
{
	int enc_id = connector->encoder_ids[0];
	struct drm_mode_object *obj;
	struct drm_encoder *encoder = NULL;

	if (enc_id) {
		obj = drm_mode_object_find(connector->dev, enc_id,
		    DRM_MODE_OBJECT_ENCODER);
		if (obj == NULL)
			return NULL;
		encoder = obj_to_encoder(obj);
	}

	return encoder;
}

static int
tegra_crtc_intr(void *priv)
{
	struct tegra_crtc *tegra_crtc = priv;
	struct drm_device *ddev = tegra_crtc->base.dev;
	struct tegra_drm_softc * const sc = tegra_drm_private(ddev);
	int rv = 0;

	const uint32_t status = DC_READ(tegra_crtc, DC_CMD_INT_STATUS_REG);

	if (status & DC_CMD_INT_V_BLANK) {
		DC_WRITE(tegra_crtc, DC_CMD_INT_STATUS_REG, DC_CMD_INT_V_BLANK);
		atomic_inc_32(&sc->sc_vbl_received[tegra_crtc->index]);
		drm_handle_vblank(ddev, tegra_crtc->index);
		rv = 1;
	}

	return rv;
}

u32
tegra_drm_get_vblank_counter(struct drm_device *ddev, int crtc)
{
	struct tegra_drm_softc * const sc = tegra_drm_private(ddev);

	if (crtc > 1)
		return 0;

	return sc->sc_vbl_received[crtc];
}

int
tegra_drm_enable_vblank(struct drm_device *ddev, int crtc)
{
	struct tegra_crtc *tegra_crtc = NULL;
	struct drm_crtc *iter;

	list_for_each_entry(iter, &ddev->mode_config.crtc_list, head) {
		if (to_tegra_crtc(iter)->index == crtc) {
			tegra_crtc = to_tegra_crtc(iter);
			break;
		}
	}
	if (tegra_crtc == NULL)
		return -EINVAL;

	DC_SET_CLEAR(tegra_crtc, DC_CMD_INT_MASK_REG, DC_CMD_INT_V_BLANK, 0);

	return 0;
}

void
tegra_drm_disable_vblank(struct drm_device *ddev, int crtc)
{
	struct tegra_crtc *tegra_crtc = NULL;
	struct drm_crtc *iter;

	list_for_each_entry(iter, &ddev->mode_config.crtc_list, head) {
		if (to_tegra_crtc(iter)->index == crtc) {
			tegra_crtc = to_tegra_crtc(iter);
			break;
		}
	}
	if (tegra_crtc == NULL)
		return;

	DC_SET_CLEAR(tegra_crtc, DC_CMD_INT_MASK_REG, 0, DC_CMD_INT_V_BLANK);
	DC_WRITE(tegra_crtc, DC_CMD_INT_STATUS_REG, DC_CMD_INT_V_BLANK);
}
