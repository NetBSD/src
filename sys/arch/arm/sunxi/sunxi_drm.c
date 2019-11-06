/* $NetBSD: sunxi_drm.c,v 1.7.6.1 2019/11/06 09:48:31 martin Exp $ */

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
__KERNEL_RCSID(0, "$NetBSD: sunxi_drm.c,v 1.7.6.1 2019/11/06 09:48:31 martin Exp $");

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
#include <drm/drm_crtc_helper.h>
#include <drm/drm_fb_helper.h>

#include <dev/fdt/fdtvar.h>
#include <dev/fdt/fdt_port.h>

#include <arm/sunxi/sunxi_drm.h>

#define	SUNXI_DRM_MAX_WIDTH	3840
#define	SUNXI_DRM_MAX_HEIGHT	2160

static TAILQ_HEAD(, sunxi_drm_endpoint) sunxi_drm_endpoints =
    TAILQ_HEAD_INITIALIZER(sunxi_drm_endpoints);

static const char * const compatible[] = {
	"allwinner,sun8i-h3-display-engine",
	"allwinner,sun50i-a64-display-engine",
	NULL
};

static const char * fb_compatible[] = {
	"allwinner,simple-framebuffer",
	NULL
};

static int	sunxi_drm_match(device_t, cfdata_t, void *);
static void	sunxi_drm_attach(device_t, device_t, void *);

static void	sunxi_drm_init(device_t);
static vmem_t	*sunxi_drm_alloc_cma_pool(struct drm_device *, size_t);

static int	sunxi_drm_set_busid(struct drm_device *, struct drm_master *);

static uint32_t	sunxi_drm_get_vblank_counter(struct drm_device *, unsigned int);
static int	sunxi_drm_enable_vblank(struct drm_device *, unsigned int);
static void	sunxi_drm_disable_vblank(struct drm_device *, unsigned int);

static int	sunxi_drm_load(struct drm_device *, unsigned long);
static int	sunxi_drm_unload(struct drm_device *);

static struct drm_driver sunxi_drm_driver = {
	.driver_features = DRIVER_MODESET | DRIVER_GEM | DRIVER_PRIME,
	.dev_priv_size = 0,
	.load = sunxi_drm_load,
	.unload = sunxi_drm_unload,

	.gem_free_object = drm_gem_cma_free_object,
	.mmap_object = drm_gem_or_legacy_mmap_object,
	.gem_uvm_ops = &drm_gem_cma_uvm_ops,

	.dumb_create = drm_gem_cma_dumb_create,
	.dumb_map_offset = drm_gem_cma_dumb_map_offset,
	.dumb_destroy = drm_gem_dumb_destroy,

	.get_vblank_counter = sunxi_drm_get_vblank_counter,
	.enable_vblank = sunxi_drm_enable_vblank,
	.disable_vblank = sunxi_drm_disable_vblank,

	.name = DRIVER_NAME,
	.desc = DRIVER_DESC,
	.date = DRIVER_DATE,
	.major = DRIVER_MAJOR,
	.minor = DRIVER_MINOR,
	.patchlevel = DRIVER_PATCHLEVEL,

	.set_busid = sunxi_drm_set_busid,
};

CFATTACH_DECL_NEW(sunxi_drm, sizeof(struct sunxi_drm_softc),
	sunxi_drm_match, sunxi_drm_attach, NULL, NULL);

static int
sunxi_drm_match(device_t parent, cfdata_t cf, void *aux)
{
	struct fdt_attach_args * const faa = aux;

	return of_match_compatible(faa->faa_phandle, compatible);
}

static void
sunxi_drm_attach(device_t parent, device_t self, void *aux)
{
	struct sunxi_drm_softc * const sc = device_private(self);
	struct fdt_attach_args * const faa = aux;
	struct drm_driver * const driver = &sunxi_drm_driver;
	prop_dictionary_t dict = device_properties(self);
	bool is_disabled;

	sc->sc_dev = self;
	sc->sc_dmat = faa->faa_dmat;
	sc->sc_bst = faa->faa_bst;
	sc->sc_phandle = faa->faa_phandle;

	aprint_naive("\n");

	if (prop_dictionary_get_bool(dict, "disabled", &is_disabled) && is_disabled) {
		aprint_normal(": Display Engine Pipeline (disabled)\n");
		return;
	}

	aprint_normal(": Display Engine Pipeline\n");

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

	fdt_remove_bycompat(fb_compatible);

	config_defer(self, sunxi_drm_init);
}

static void
sunxi_drm_init(device_t dev)
{
	struct sunxi_drm_softc * const sc = device_private(dev);
	struct drm_driver * const driver = &sunxi_drm_driver;
	int error;

	error = -drm_dev_register(sc->sc_ddev, 0);
	if (error) {
		drm_dev_unref(sc->sc_ddev);
		aprint_error_dev(dev, "couldn't register DRM device: %d\n",
		    error);
		return;
	}

	aprint_normal_dev(dev, "initialized %s %d.%d.%d %s on minor %d\n",
	    driver->name, driver->major, driver->minor, driver->patchlevel,
	    driver->date, sc->sc_ddev->primary->index);
}

static vmem_t *
sunxi_drm_alloc_cma_pool(struct drm_device *ddev, size_t cma_size)
{
	struct sunxi_drm_softc * const sc = sunxi_drm_private(ddev);
	bus_dma_segment_t segs[1];
	int nsegs;
	int error;

	error = bus_dmamem_alloc(sc->sc_dmat, cma_size, PAGE_SIZE, 0,
	    segs, 1, &nsegs, BUS_DMA_NOWAIT);
	if (error) {
		aprint_error_dev(sc->sc_dev, "couldn't allocate CMA pool\n");
		return NULL;
	}

	return vmem_create("sunxidrm", segs[0].ds_addr, segs[0].ds_len,
	    PAGE_SIZE, NULL, NULL, NULL, 0, VM_SLEEP, IPL_NONE);
}

static int
sunxi_drm_set_busid(struct drm_device *ddev, struct drm_master *master)
{
	struct sunxi_drm_softc * const sc = sunxi_drm_private(ddev);
	char id[32];

	snprintf(id, sizeof(id), "platform:sunxi:%u", device_unit(sc->sc_dev));

	master->unique = kzalloc(strlen(id) + 1, GFP_KERNEL);
	if (master->unique == NULL)
		return -ENOMEM;
	strcpy(master->unique, id);
	master->unique_len = strlen(master->unique);

	return 0;
}

static int
sunxi_drm_fb_create_handle(struct drm_framebuffer *fb,
    struct drm_file *file, unsigned int *handle)
{
	struct sunxi_drm_framebuffer *sfb = to_sunxi_drm_framebuffer(fb);

	return drm_gem_handle_create(file, &sfb->obj->base, handle);
}

static void
sunxi_drm_fb_destroy(struct drm_framebuffer *fb)
{
	struct sunxi_drm_framebuffer *sfb = to_sunxi_drm_framebuffer(fb);

	drm_framebuffer_cleanup(fb);
	drm_gem_object_unreference_unlocked(&sfb->obj->base);
	kmem_free(sfb, sizeof(*sfb));
}

static const struct drm_framebuffer_funcs sunxi_drm_framebuffer_funcs = {
	.create_handle = sunxi_drm_fb_create_handle,
	.destroy = sunxi_drm_fb_destroy,
};

static struct drm_framebuffer *
sunxi_drm_fb_create(struct drm_device *ddev, struct drm_file *file,
    struct drm_mode_fb_cmd2 *cmd)
{
	struct sunxi_drm_framebuffer *fb;
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
	case DRM_FORMAT_ARGB8888:
		fb->base.depth = 32;
		break;
	default:
		break;
	}

	error = drm_framebuffer_init(ddev, &fb->base, &sunxi_drm_framebuffer_funcs);
	if (error != 0)
		goto dealloc;

	return &fb->base;

dealloc:
	drm_framebuffer_cleanup(&fb->base);
	kmem_free(fb, sizeof(*fb));
	drm_gem_object_unreference_unlocked(gem_obj);

	return NULL;
}

static struct drm_mode_config_funcs sunxi_drm_mode_config_funcs = {
	.fb_create = sunxi_drm_fb_create,
};

static int
sunxi_drm_fb_probe(struct drm_fb_helper *helper, struct drm_fb_helper_surface_size *sizes)
{
	struct sunxi_drm_softc * const sc = sunxi_drm_private(helper->dev);
	struct drm_device *ddev = helper->dev;
	struct sunxi_drm_framebuffer *sfb = to_sunxi_drm_framebuffer(helper->fb);
	struct drm_framebuffer *fb = helper->fb;
	struct sunxi_drmfb_attach_args sfa;
	size_t cma_size;
	int error;

	const u_int width = sizes->surface_width;
	const u_int height = sizes->surface_height;
	const u_int pitch = width * (32 / 8);

	const size_t size = roundup(height * pitch, PAGE_SIZE);

	/* Reserve enough memory for the FB console plus a 4K plane, rounded to 1MB */
	cma_size = size;
	cma_size += (SUNXI_DRM_MAX_WIDTH * SUNXI_DRM_MAX_HEIGHT * 4);
	cma_size = roundup(cma_size, 1024 * 1024);
	sc->sc_ddev->cma_pool = sunxi_drm_alloc_cma_pool(sc->sc_ddev, cma_size);
	if (sc->sc_ddev->cma_pool != NULL)
		aprint_normal_dev(sc->sc_dev, "reserved %u MB DRAM for CMA\n",
		    (u_int)(cma_size / (1024 * 1024)));

	sfb->obj = drm_gem_cma_create(ddev, size);
	if (sfb->obj == NULL) {
		DRM_ERROR("failed to allocate memory for framebuffer\n");
		return -ENOMEM;
	}

	fb->pitches[0] = pitch;
	fb->offsets[0] = 0;
	fb->width = width;
	fb->height = height;
	fb->pixel_format = DRM_FORMAT_XRGB8888;
	drm_fb_get_bpp_depth(fb->pixel_format, &fb->depth, &fb->bits_per_pixel);

	error = drm_framebuffer_init(ddev, fb, &sunxi_drm_framebuffer_funcs);
	if (error != 0) {
		DRM_ERROR("failed to initialize framebuffer\n");
		return error;
	}

	memset(&sfa, 0, sizeof(sfa));
	sfa.sfa_drm_dev = ddev;
	sfa.sfa_fb_helper = helper;
	sfa.sfa_fb_sizes = *sizes;
	sfa.sfa_fb_bst = sc->sc_bst;
	sfa.sfa_fb_dmat = sc->sc_dmat;
	sfa.sfa_fb_linebytes = helper->fb->pitches[0];

	helper->fbdev = config_found_ia(ddev->dev, "sunxifbbus", &sfa, NULL);
	if (helper->fbdev == NULL) {
		DRM_ERROR("unable to attach framebuffer\n");
		return -ENXIO;
	}

	return 0;
}

static struct drm_fb_helper_funcs sunxi_drm_fb_helper_funcs = {
	.fb_probe = sunxi_drm_fb_probe,
};

static int
sunxi_drm_load(struct drm_device *ddev, unsigned long flags)
{
	struct sunxi_drm_softc * const sc = sunxi_drm_private(ddev);
	struct sunxi_drm_endpoint *sep;
	struct sunxi_drm_fbdev *fbdev;
	const u_int *data;
	int datalen, error, num_crtc;

	drm_mode_config_init(ddev);
	ddev->mode_config.min_width = 0;
	ddev->mode_config.min_height = 0;
	ddev->mode_config.max_width = SUNXI_DRM_MAX_WIDTH;
	ddev->mode_config.max_height = SUNXI_DRM_MAX_HEIGHT;
	ddev->mode_config.funcs = &sunxi_drm_mode_config_funcs;

	num_crtc = 0;
	data = fdtbus_get_prop(sc->sc_phandle, "allwinner,pipelines", &datalen);
	while (datalen >= 4) {
		const int crtc_phandle = fdtbus_get_phandle_from_native(be32dec(data));

		TAILQ_FOREACH(sep, &sunxi_drm_endpoints, entries)
			if (sep->phandle == crtc_phandle && sep->ddev == NULL) {
				sep->ddev = ddev;
				error = fdt_endpoint_activate_direct(sep->ep, true);
				if (error != 0) {
					aprint_error_dev(sc->sc_dev, "failed to activate endpoint: %d\n",
					    error);
				}
				if (fdt_endpoint_type(sep->ep) == EP_DRM_CRTC)
					num_crtc++;
			}

		datalen -= 4;
		data++;
	}

	if (num_crtc == 0) {
		aprint_error_dev(sc->sc_dev, "no pipelines configured\n");
		return ENXIO;
	}

	fbdev = kmem_zalloc(sizeof(*fbdev), KM_SLEEP);

	drm_fb_helper_prepare(ddev, &fbdev->helper, &sunxi_drm_fb_helper_funcs);

	error = drm_fb_helper_init(ddev, &fbdev->helper, num_crtc, num_crtc);
	if (error)
		goto drmerr;

	fbdev->helper.fb = kmem_zalloc(sizeof(struct sunxi_drm_framebuffer), KM_SLEEP);

	drm_fb_helper_single_add_all_connectors(&fbdev->helper);

	drm_helper_disable_unused_functions(ddev);

	drm_fb_helper_initial_config(&fbdev->helper, 32);

	/* XXX */
	ddev->irq_enabled = true;
	drm_vblank_init(ddev, num_crtc);

	return 0;

drmerr:
	drm_mode_config_cleanup(ddev);
	kmem_free(fbdev, sizeof(*fbdev));

	return error;
}

static uint32_t
sunxi_drm_get_vblank_counter(struct drm_device *ddev, unsigned int crtc)
{
	struct sunxi_drm_softc * const sc = sunxi_drm_private(ddev);

	if (crtc >= __arraycount(sc->sc_vbl))
		return 0;

	if (sc->sc_vbl[crtc].get_vblank_counter == NULL)
		return 0;

	return sc->sc_vbl[crtc].get_vblank_counter(sc->sc_vbl[crtc].priv);
}

static int
sunxi_drm_enable_vblank(struct drm_device *ddev, unsigned int crtc)
{
	struct sunxi_drm_softc * const sc = sunxi_drm_private(ddev);

	if (crtc >= __arraycount(sc->sc_vbl))
		return 0;

	if (sc->sc_vbl[crtc].enable_vblank == NULL)
		return 0;

	sc->sc_vbl[crtc].enable_vblank(sc->sc_vbl[crtc].priv);

	return 0;
}

static void
sunxi_drm_disable_vblank(struct drm_device *ddev, unsigned int crtc)
{
	struct sunxi_drm_softc * const sc = sunxi_drm_private(ddev);

	if (crtc >= __arraycount(sc->sc_vbl))
		return;

	if (sc->sc_vbl[crtc].disable_vblank == NULL)
		return;

	sc->sc_vbl[crtc].disable_vblank(sc->sc_vbl[crtc].priv);
}

static int
sunxi_drm_unload(struct drm_device *ddev)
{
	drm_mode_config_cleanup(ddev);

	return 0;
}

int
sunxi_drm_register_endpoint(int phandle, struct fdt_endpoint *ep)
{
	struct sunxi_drm_endpoint *sep;

	sep = kmem_zalloc(sizeof(*sep), KM_SLEEP);
	sep->phandle = phandle;
	sep->ep = ep;
	sep->ddev = NULL;
	TAILQ_INSERT_TAIL(&sunxi_drm_endpoints, sep, entries);

	return 0;
}

struct drm_device *
sunxi_drm_endpoint_device(struct fdt_endpoint *ep)
{
	struct sunxi_drm_endpoint *sep;

	TAILQ_FOREACH(sep, &sunxi_drm_endpoints, entries)
		if (sep->ep == ep)
			return sep->ddev;

	return NULL;
}
