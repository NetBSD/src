/* $NetBSD: ti_lcdc.c,v 1.3.2.2 2019/11/27 13:46:44 martin Exp $ */

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
__KERNEL_RCSID(0, "$NetBSD: ti_lcdc.c,v 1.3.2.2 2019/11/27 13:46:44 martin Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/intr.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/conf.h>

#include <uvm/uvm_extern.h>
#include <uvm/uvm_object.h>
#include <uvm/uvm_device.h>

#include <drm/drmP.h>
#include <drm/drm_crtc.h>
#include <drm/drm_crtc_helper.h>
#include <drm/drm_plane_helper.h>
#include <drm/drm_fb_helper.h>

#include <dev/fdt/fdtvar.h>
#include <dev/fdt/fdt_port.h>

#include <arm/ti/ti_prcm.h>
#include <arm/ti/ti_lcdc.h>
#include <arm/ti/ti_lcdcreg.h>

static const char * const compatible[] = {
	"ti,am33xx-tilcdc",
	NULL
};

enum {
	TILCDC_PORT_OUTPUT = 0,
};

static int	tilcdc_match(device_t, cfdata_t, void *);
static void	tilcdc_attach(device_t, device_t, void *);

static int	tilcdc_set_busid(struct drm_device *, struct drm_master *);

static int	tilcdc_load(struct drm_device *, unsigned long);
static int	tilcdc_unload(struct drm_device *);

static struct drm_driver tilcdc_driver = {
	.driver_features = DRIVER_MODESET | DRIVER_GEM | DRIVER_PRIME,
	.dev_priv_size = 0,
	.load = tilcdc_load,
	.unload = tilcdc_unload,

	.gem_free_object = drm_gem_cma_free_object,
	.mmap_object = drm_gem_or_legacy_mmap_object,
	.gem_uvm_ops = &drm_gem_cma_uvm_ops,

	.dumb_create = drm_gem_cma_dumb_create,
	.dumb_map_offset = drm_gem_cma_dumb_map_offset,
	.dumb_destroy = drm_gem_dumb_destroy,

	.name = DRIVER_NAME,
	.desc = DRIVER_DESC,
	.date = DRIVER_DATE,
	.major = DRIVER_MAJOR,
	.minor = DRIVER_MINOR,
	.patchlevel = DRIVER_PATCHLEVEL,

	.set_busid = tilcdc_set_busid,
};

CFATTACH_DECL_NEW(ti_lcdc, sizeof(struct tilcdc_softc),
	tilcdc_match, tilcdc_attach, NULL, NULL);

static int
tilcdc_mode_do_set_base(struct drm_crtc *crtc, struct drm_framebuffer *fb,
    int x, int y, int atomic)
{
	struct tilcdc_crtc *mixer_crtc = to_tilcdc_crtc(crtc);
	struct tilcdc_softc * const sc = mixer_crtc->sc;
	struct tilcdc_framebuffer *sfb = atomic?
	    to_tilcdc_framebuffer(fb) :
	    to_tilcdc_framebuffer(crtc->primary->fb);

	const uint32_t paddr = (uint32_t)sfb->obj->dmamap->dm_segs[0].ds_addr;
	const u_int psize = sfb->obj->dmamap->dm_segs[0].ds_len;

	/* Framebuffer start address */
	WR4(sc, LCD_LCDDMA_FB0_BASE, paddr);
	WR4(sc, LCD_LCDDMA_FB0_CEILING, paddr + psize - 1);

	return 0;
}

static void
tilcdc_destroy(struct drm_crtc *crtc)
{
	drm_crtc_cleanup(crtc);
}

static const struct drm_crtc_funcs tilcdc_crtc_funcs = {
	.set_config = drm_crtc_helper_set_config,
	.destroy = tilcdc_destroy,
};

static void
tilcdc_dpms(struct drm_crtc *crtc, int mode)
{
}

static bool
tilcdc_mode_fixup(struct drm_crtc *crtc,
    const struct drm_display_mode *mode, struct drm_display_mode *adjusted_mode)
{
#if 0
	adjusted_mode->hskew = mode->hsync_end - mode->hsync_start;
	adjusted_mode->flags |= DRM_MODE_FLAG_HSKEW;

	adjusted_mode->flags &= ~(DRM_MODE_FLAG_NHSYNC | DRM_MODE_FLAG_PHSYNC);
	if (mode->flags & DRM_MODE_FLAG_NHSYNC)
		adjusted_mode->flags |= DRM_MODE_FLAG_PHSYNC;
	else
		adjusted_mode->flags |= DRM_MODE_FLAG_NHSYNC;
#endif

	return true;
}

static int
tilcdc_mode_set(struct drm_crtc *crtc, struct drm_display_mode *mode,
    struct drm_display_mode *adjusted_mode, int x, int y,
    struct drm_framebuffer *old_fb)
{
	struct tilcdc_crtc *mixer_crtc = to_tilcdc_crtc(crtc);
	struct tilcdc_softc * const sc = mixer_crtc->sc;
	int clk_div, div, diff, best_diff;
	uint32_t val;

	const u_int hspw = adjusted_mode->crtc_hsync_end - adjusted_mode->crtc_hsync_start;
	const u_int hbp = adjusted_mode->crtc_htotal - adjusted_mode->crtc_hsync_end;
	const u_int hfp = adjusted_mode->crtc_hsync_start - adjusted_mode->crtc_hdisplay;
	const u_int vspw = adjusted_mode->crtc_vsync_end - adjusted_mode->crtc_vsync_start;
	const u_int vbp = adjusted_mode->crtc_vtotal - adjusted_mode->crtc_vsync_end;
	const u_int vfp = adjusted_mode->crtc_vsync_start - adjusted_mode->crtc_vdisplay;

	const u_int rate = clk_get_rate(sc->sc_clk);

	clk_div = 255;
	best_diff = -1;
	for (div = 2; div < 255; div++) {
		const int pixel_clock = (rate / div) / 1000;
		diff = abs(adjusted_mode->crtc_clock - pixel_clock);
		if (best_diff == -1 || diff < best_diff) {
			best_diff = diff;
			clk_div = div;
		}
	}
	if (clk_div == 255) {
		device_printf(sc->sc_dev, "couldn't configure pixel clock (%u)\n",
		    adjusted_mode->crtc_clock);
		return ERANGE;
	}

	val = CTRL_RASTER_MODE |
	      (clk_div << CTRL_DIV_SHIFT);
	WR4(sc, LCD_CTRL, val);

	val = RASTER_TIMING_0_HFP(hfp) |
	      RASTER_TIMING_0_HBP(hbp) |
	      RASTER_TIMING_0_HSW(hspw) |
	      RASTER_TIMING_0_PPL(adjusted_mode->hdisplay);
	WR4(sc, LCD_RASTER_TIMING_0, val);

	val = RASTER_TIMING_1_VFP(vfp) |
	      RASTER_TIMING_1_VBP(vbp) |
	      RASTER_TIMING_1_VSW(vspw) |
	      RASTER_TIMING_1_LPP(adjusted_mode->vdisplay);
	WR4(sc, LCD_RASTER_TIMING_1, val);

	val = RASTER_TIMING_2_HFP(hfp) |
	      RASTER_TIMING_2_HBP(hbp) |
	      RASTER_TIMING_2_HSW(hspw) |
	      RASTER_TIMING_2_LPP(adjusted_mode->vdisplay);
	/* XXX TDA HDMI TX */
	val |= RASTER_TIMING_2_IPC;
	val |= RASTER_TIMING_2_PHSVS;
	val |= RASTER_TIMING_2_PHSVS_RISE;
	val |= RASTER_TIMING_2_ACB(255);
	val |= RASTER_TIMING_2_ACBI(0);
	WR4(sc, LCD_RASTER_TIMING_2, val);

	val = (4 << LCDDMA_CTRL_BURST_SIZE_SHIFT) |
	      (0 << LCDDMA_CTRL_TH_FIFO_RDY_SHIFT) |
	      LCDDMA_CTRL_FB0_ONLY;
	WR4(sc, LCD_LCDDMA_CTRL, val);

	/* XXX TDA HDMI TX */
	val = RASTER_CTRL_LCDTFT |
	      RASTER_CTRL_TFT24 |
	      RASTER_CTRL_TFT24_UNPACKED |
	      RASTER_CTRL_REQDLY(0x80) |
	      RASTER_CTRL_PALMODE_DATA_ONLY;
	WR4(sc, LCD_RASTER_CTRL, val);

	tilcdc_mode_do_set_base(crtc, old_fb, x, y, 0);

	return 0;
}

static int
tilcdc_mode_set_base(struct drm_crtc *crtc, int x, int y,
    struct drm_framebuffer *old_fb)
{
	tilcdc_mode_do_set_base(crtc, old_fb, x, y, 0);

	return 0;
}

static int
tilcdc_mode_set_base_atomic(struct drm_crtc *crtc, struct drm_framebuffer *fb,
    int x, int y, enum mode_set_atomic state)
{
	tilcdc_mode_do_set_base(crtc, fb, x, y, 1);

	return 0;
}

static void
tilcdc_disable(struct drm_crtc *crtc)
{
}

static void
tilcdc_prepare(struct drm_crtc *crtc)
{
}

static void
tilcdc_commit(struct drm_crtc *crtc)
{
	struct tilcdc_crtc *mixer_crtc = to_tilcdc_crtc(crtc);
	struct tilcdc_softc * const sc = mixer_crtc->sc;
	uint32_t val;

	WR4(sc, LCD_CLKC_ENABLE, CLKC_ENABLE_DMA | CLKC_ENABLE_CORE);
	WR4(sc, LCD_CLKC_RESET, CLKC_RESET_MAIN);
	delay(100);
	WR4(sc, LCD_CLKC_RESET, 0);

	val = RD4(sc, LCD_RASTER_CTRL);
	WR4(sc, LCD_RASTER_CTRL, val | RASTER_CTRL_LCDEN);
}

static const struct drm_crtc_helper_funcs tilcdc_crtc_helper_funcs = {
	.dpms = tilcdc_dpms,
	.mode_fixup = tilcdc_mode_fixup,
	.mode_set = tilcdc_mode_set,
	.mode_set_base = tilcdc_mode_set_base,
	.mode_set_base_atomic = tilcdc_mode_set_base_atomic,
	.disable = tilcdc_disable,
	.prepare = tilcdc_prepare,
	.commit = tilcdc_commit,
};

static void
tilcdc_encoder_destroy(struct drm_encoder *encoder)
{
}

static const struct drm_encoder_funcs tilcdc_encoder_funcs = {
	.destroy = tilcdc_encoder_destroy,
};

static void
tilcdc_encoder_dpms(struct drm_encoder *encoder, int mode)
{
}

static bool
tilcdc_encoder_mode_fixup(struct drm_encoder *encoder,
    const struct drm_display_mode *mode, struct drm_display_mode *adjusted_mode)
{
	return true;
}

static void
tilcdc_encoder_mode_set(struct drm_encoder *encoder,
    struct drm_display_mode *mode, struct drm_display_mode *adjusted_mode)
{
}

static void
tilcdc_encoder_prepare(struct drm_encoder *encoder)
{
}

static void
tilcdc_encoder_commit(struct drm_encoder *encoder)
{
}

static const struct drm_encoder_helper_funcs tilcdc_encoder_helper_funcs = {
	.dpms = tilcdc_encoder_dpms,
	.mode_fixup = tilcdc_encoder_mode_fixup,
	.prepare = tilcdc_encoder_prepare,
	.commit = tilcdc_encoder_commit,
	.mode_set = tilcdc_encoder_mode_set,
};

static int
tilcdc_ep_activate(device_t dev, struct fdt_endpoint *ep, bool activate)
{
	struct tilcdc_softc * const sc = device_private(dev);
	struct drm_device *ddev = sc->sc_ddev;

	if (!activate)
		return EINVAL;

	sc->sc_crtc.sc = sc;

	WR4(sc, LCD_SYSCONFIG, SYSCONFIG_STANDBY_SMART | SYSCONFIG_IDLE_SMART);

	drm_crtc_init(ddev, &sc->sc_crtc.base, &tilcdc_crtc_funcs);
	drm_crtc_helper_add(&sc->sc_crtc.base, &tilcdc_crtc_helper_funcs);

	sc->sc_encoder.sc = sc;
	sc->sc_encoder.base.possible_crtcs = 1 << drm_crtc_index(&sc->sc_crtc.base);

	drm_encoder_init(ddev, &sc->sc_encoder.base, &tilcdc_encoder_funcs,
	    DRM_MODE_ENCODER_TMDS);
	drm_encoder_helper_add(&sc->sc_encoder.base, &tilcdc_encoder_helper_funcs);

	return fdt_endpoint_activate(ep, activate);
}

static void *
tilcdc_ep_get_data(device_t dev, struct fdt_endpoint *ep)
{
	struct tilcdc_softc * const sc = device_private(dev);

	return &sc->sc_encoder.base;
}

static int
tilcdc_match(device_t parent, cfdata_t cf, void *aux)
{
	struct fdt_attach_args * const faa = aux;

	return of_match_compatible(faa->faa_phandle, compatible);
}

static void
tilcdc_attach(device_t parent, device_t self, void *aux)
{
	struct tilcdc_softc * const sc = device_private(self);
	struct fdt_attach_args * const faa = aux;
	const int phandle = faa->faa_phandle;
	struct drm_driver * const driver = &tilcdc_driver;
	prop_dictionary_t dict = device_properties(self);
	bool is_disabled;
	bus_addr_t addr;
	bus_size_t size;
	int error;

	if (prop_dictionary_get_bool(dict, "disabled", &is_disabled) && is_disabled) {
		aprint_normal(": TI LCDC (disabled)\n");
		return;
	}

	if (fdtbus_get_reg(phandle, 0, &addr, &size) != 0) {
		aprint_error(": couldn't get registers\n");
		return;
	}

	sc->sc_dev = self;
	sc->sc_phandle = faa->faa_phandle;
	sc->sc_dmat = faa->faa_dmat;
	sc->sc_bst = faa->faa_bst;
	if (bus_space_map(sc->sc_bst, addr, size, 0, &sc->sc_bsh) != 0) {
		aprint_error(": couldn't map registers\n");
		return;
	}
	sc->sc_clk = ti_prcm_get_hwmod(phandle, 0);
	if (sc->sc_clk == NULL || clk_enable(sc->sc_clk) != 0) {
		aprint_error(": couldn't enable module\n");
		return;
	}

	aprint_naive("\n");
	aprint_normal(": TI LCDC\n");

	sc->sc_ports.dp_ep_activate = tilcdc_ep_activate;
	sc->sc_ports.dp_ep_get_data = tilcdc_ep_get_data;
	fdt_ports_register(&sc->sc_ports, self, phandle, EP_DRM_ENCODER);

	sc->sc_ddev = drm_dev_alloc(driver, sc->sc_dev);
	if (sc->sc_ddev == NULL) {
		aprint_error_dev(self, "couldn't allocate DRM device\n");
		return;
	}
	sc->sc_ddev->dev_private = sc;
	sc->sc_ddev->bst = sc->sc_bst;
	sc->sc_ddev->bus_dmat = sc->sc_dmat;
	sc->sc_ddev->dmat = sc->sc_ddev->bus_dmat;
	sc->sc_ddev->dmat_subregion_p = false;

	error = -drm_dev_register(sc->sc_ddev, 0);
	if (error) {
		drm_dev_unref(sc->sc_ddev);
		aprint_error_dev(self, "couldn't register DRM device: %d\n",
		    error);
		return;
	}

	aprint_normal_dev(self, "initialized %s %d.%d.%d %s on minor %d\n",
	    driver->name, driver->major, driver->minor, driver->patchlevel,
	    driver->date, sc->sc_ddev->primary->index);
}

static int
tilcdc_set_busid(struct drm_device *ddev, struct drm_master *master)
{
	struct tilcdc_softc * const sc = tilcdc_private(ddev);
	char id[32];

	snprintf(id, sizeof(id), "platform:tilcdc:%u", device_unit(sc->sc_dev));

	master->unique = kzalloc(strlen(id) + 1, GFP_KERNEL);
	if (master->unique == NULL)
		return -ENOMEM;
	strcpy(master->unique, id);
	master->unique_len = strlen(master->unique);

	return 0;
}

static int
tilcdc_fb_create_handle(struct drm_framebuffer *fb,
    struct drm_file *file, unsigned int *handle)
{
	struct tilcdc_framebuffer *sfb = to_tilcdc_framebuffer(fb);

	return drm_gem_handle_create(file, &sfb->obj->base, handle);
}

static void
tilcdc_fb_destroy(struct drm_framebuffer *fb)
{
	struct tilcdc_framebuffer *sfb = to_tilcdc_framebuffer(fb);

	drm_framebuffer_cleanup(fb);
	drm_gem_object_unreference_unlocked(&sfb->obj->base);
	kmem_free(sfb, sizeof(*sfb));
}

static const struct drm_framebuffer_funcs tilcdc_framebuffer_funcs = {
	.create_handle = tilcdc_fb_create_handle,
	.destroy = tilcdc_fb_destroy,
};

static struct drm_framebuffer *
tilcdc_fb_create(struct drm_device *ddev, struct drm_file *file,
    struct drm_mode_fb_cmd2 *cmd)
{
	struct tilcdc_framebuffer *fb;
	struct drm_gem_object *gem_obj;
	int error;

	if (cmd->flags)
		return NULL;

	gem_obj = drm_gem_object_lookup(ddev, file, cmd->handles[0]);
	if (gem_obj == NULL)
		return NULL;

	fb = kmem_zalloc(sizeof(*fb), KM_SLEEP);
	fb->obj = to_drm_gem_cma_obj(gem_obj);
	fb->base.pitches[0] = cmd->pitches[0];
	fb->base.pitches[1] = cmd->pitches[1];
	fb->base.pitches[2] = cmd->pitches[2];
	fb->base.offsets[0] = cmd->offsets[0];
	fb->base.offsets[1] = cmd->offsets[2];
	fb->base.offsets[2] = cmd->offsets[1];
	fb->base.width = cmd->width;
	fb->base.height = cmd->height;
	fb->base.pixel_format = cmd->pixel_format;
	fb->base.bits_per_pixel = drm_format_plane_cpp(fb->base.pixel_format, 0) * 8;

	switch (fb->base.pixel_format) {
	case DRM_FORMAT_XRGB8888:
	case DRM_FORMAT_XBGR8888:
		fb->base.depth = 32;
		break;
	default:
		break;
	}

	error = drm_framebuffer_init(ddev, &fb->base, &tilcdc_framebuffer_funcs);
	if (error != 0)
		goto dealloc;

	return &fb->base;

dealloc:
	drm_framebuffer_cleanup(&fb->base);
	kmem_free(fb, sizeof(*fb));
	drm_gem_object_unreference_unlocked(gem_obj);

	return NULL;
}

static struct drm_mode_config_funcs tilcdc_mode_config_funcs = {
	.fb_create = tilcdc_fb_create,
};

static int
tilcdc_fb_probe(struct drm_fb_helper *helper, struct drm_fb_helper_surface_size *sizes)
{
	struct tilcdc_softc * const sc = tilcdc_private(helper->dev);
	struct drm_device *ddev = helper->dev;
	struct tilcdc_framebuffer *sfb = to_tilcdc_framebuffer(helper->fb);
	struct drm_framebuffer *fb = helper->fb;
	struct tilcdcfb_attach_args tfa;
	const char *br_wiring;
	uint32_t pixel_format;
	int error;

	const u_int width = sizes->surface_width;
	const u_int height = sizes->surface_height;
	const u_int pitch = width * (32 / 8);

	br_wiring = fdtbus_get_string(sc->sc_phandle, "blue-and-red-wiring");
	if (br_wiring && strcmp(br_wiring, "straight") == 0) {
		pixel_format = DRM_FORMAT_XBGR8888;
	} else {
		pixel_format = DRM_FORMAT_XRGB8888;
	}

	const size_t size = roundup(height * pitch, PAGE_SIZE);

	sfb->obj = drm_gem_cma_create(ddev, size);
	if (sfb->obj == NULL) {
		DRM_ERROR("failed to allocate memory for framebuffer\n");
		return -ENOMEM;
	}

	fb->pitches[0] = pitch;
	fb->offsets[0] = 0;
	fb->width = width;
	fb->height = height;
	fb->pixel_format = pixel_format;
	drm_fb_get_bpp_depth(fb->pixel_format, &fb->depth, &fb->bits_per_pixel);

	error = drm_framebuffer_init(ddev, fb, &tilcdc_framebuffer_funcs);
	if (error != 0) {
		DRM_ERROR("failed to initialize framebuffer\n");
		return error;
	}

	memset(&tfa, 0, sizeof(tfa));
	tfa.tfa_drm_dev = ddev;
	tfa.tfa_fb_helper = helper;
	tfa.tfa_fb_sizes = *sizes;
	tfa.tfa_fb_bst = sc->sc_bst;
	tfa.tfa_fb_dmat = sc->sc_dmat;
	tfa.tfa_fb_linebytes = helper->fb->pitches[0];

	helper->fbdev = config_found_ia(ddev->dev, "tilcdcfbbus", &tfa, NULL);
	if (helper->fbdev == NULL) {
		DRM_ERROR("unable to attach framebuffer\n");
		return -ENXIO;
	}

	return 0;
}

static struct drm_fb_helper_funcs tilcdc_fb_helper_funcs = {
	.fb_probe = tilcdc_fb_probe,
};

static int
tilcdc_load(struct drm_device *ddev, unsigned long flags)
{
	struct tilcdc_softc * const sc = tilcdc_private(ddev);
	struct tilcdc_fbdev *fbdev;
	struct fdt_endpoint *ep;
	int error;

	drm_mode_config_init(ddev);
	ddev->mode_config.min_width = 0;
	ddev->mode_config.min_height = 0;
	ddev->mode_config.max_width = 2048;
	ddev->mode_config.max_height = 2048;
	ddev->mode_config.funcs = &tilcdc_mode_config_funcs;

	ep = fdt_endpoint_get_from_index(&sc->sc_ports, TILCDC_PORT_OUTPUT, 0);
	if (ep == NULL) {
		aprint_error_dev(sc->sc_dev, "couldn't find endpoint\n");
		return ENXIO;
	}
	error = fdt_endpoint_activate_direct(ep, true);
	if (error != 0) {
		aprint_error_dev(sc->sc_dev, "couldn't activate endpoint: %d\n", error);
		return error;
	}

	fbdev = kmem_zalloc(sizeof(*fbdev), KM_SLEEP);

	drm_fb_helper_prepare(ddev, &fbdev->helper, &tilcdc_fb_helper_funcs);

	error = drm_fb_helper_init(ddev, &fbdev->helper, 1, 1);
	if (error)
		goto drmerr;

	fbdev->helper.fb = kmem_zalloc(sizeof(struct tilcdc_framebuffer), KM_SLEEP);

	drm_fb_helper_single_add_all_connectors(&fbdev->helper);

	drm_helper_disable_unused_functions(ddev);

	drm_fb_helper_initial_config(&fbdev->helper, 32);

	return 0;

drmerr:
	drm_mode_config_cleanup(ddev);
	kmem_free(fbdev, sizeof(*fbdev));

	return error;
}

static int
tilcdc_unload(struct drm_device *ddev)
{
	drm_mode_config_cleanup(ddev);

	return 0;
}
