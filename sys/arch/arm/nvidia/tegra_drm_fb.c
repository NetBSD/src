/* $NetBSD: tegra_drm_fb.c,v 1.6 2017/12/26 14:54:52 jmcneill Exp $ */

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
__KERNEL_RCSID(0, "$NetBSD: tegra_drm_fb.c,v 1.6 2017/12/26 14:54:52 jmcneill Exp $");

#include <drm/drmP.h>
#include <drm/drm_crtc.h>
#include <drm/drm_fb_helper.h>
#include <drm/drm_crtc_helper.h>

#include <arm/nvidia/tegra_drm.h>

static int	tegra_fb_init(struct drm_device *, struct drm_framebuffer *,
		    struct drm_fb_helper_surface_size *);

static int	tegra_fb_probe(struct drm_fb_helper *,
		    struct drm_fb_helper_surface_size *);

static struct drm_fb_helper_funcs tegra_fb_helper_funcs = {
	.fb_probe = tegra_fb_probe
};

int
tegra_drm_fb_init(struct drm_device *ddev)
{
	struct tegra_fbdev *fbdev;
	int error;

	fbdev = kmem_zalloc(sizeof(*fbdev), KM_SLEEP);
	fbdev->helper.funcs = &tegra_fb_helper_funcs;

	error = drm_fb_helper_init(ddev, &fbdev->helper, 2, 1);
	if (error) {
		kmem_free(fbdev, sizeof(*fbdev));
		return error;
	}

	/*
	 * This might seem silly, but it saves us from having to allocate
	 * enough memory for the largest possible framebuffer (around 35MB).
	 *
	 * Allocate the fb_helper's framebuffer here but don't initialize
	 * it yet. The fb helper won't configure a CRTC unless this field is
	 * set, and we don't know the preferred size at this time.
	 *
	 * The fb_probe callback will eventually be called with the preferred
	 * surface size, so defer allocating the buffer object until then.
	 */
	fbdev->helper.fb =
	    kmem_zalloc(sizeof(struct tegra_framebuffer), KM_SLEEP);

	drm_fb_helper_single_add_all_connectors(&fbdev->helper);

	drm_helper_disable_unused_functions(ddev);

	drm_fb_helper_initial_config(&fbdev->helper, 32);

	return 0;
}

static int
tegra_fb_probe(struct drm_fb_helper *helper,
    struct drm_fb_helper_surface_size *sizes)
{
	struct tegra_drm_softc * const sc = tegra_drm_private(helper->dev);
	struct drm_device *ddev = helper->dev;
	struct tegra_drmfb_attach_args tfa;

	if (tegra_fb_init(ddev, helper->fb, sizes) != 0) {
		DRM_ERROR("failed to initialize framebuffer\n");
		return -ENOMEM;
	}

	memset(&tfa, 0, sizeof(tfa));
	tfa.tfa_drm_dev = ddev;
	tfa.tfa_fb_helper = helper;
	tfa.tfa_fb_sizes = *sizes;
	tfa.tfa_fb_bst = sc->sc_bst;
	tfa.tfa_fb_dmat = sc->sc_dmat;
	tfa.tfa_fb_linebytes = helper->fb->pitches[0];

	helper->fbdev = config_found_ia(ddev->dev, "tegrafbbus", &tfa, NULL);
	if (helper->fbdev == NULL) {
		DRM_ERROR("unable to attach tegrafb\n");
		return -ENXIO;
	}

	return 0;
}

static int
tegra_fb_init(struct drm_device *ddev, struct drm_framebuffer *fb,
    struct drm_fb_helper_surface_size *sizes)
{
	struct tegra_framebuffer *tegra_fb = to_tegra_framebuffer(fb);
	const u_int width = sizes->surface_width;
	const u_int height = sizes->surface_height;
	const u_int pitch = width * (32 / 8);

	const size_t size = roundup(height * pitch, PAGE_SIZE);

	tegra_fb->obj = drm_gem_cma_create(ddev, size);
	if (tegra_fb->obj == NULL)
		return -ENOMEM;

        fb->pitches[0] = pitch;
        fb->offsets[0] = 0;
        fb->width = width;
        fb->height = height;
        fb->pixel_format = DRM_FORMAT_XRGB8888;
        drm_fb_get_bpp_depth(fb->pixel_format, &fb->depth,
            &fb->bits_per_pixel);
	tegra_drm_framebuffer_init(ddev, tegra_fb);

	return 0;
}
