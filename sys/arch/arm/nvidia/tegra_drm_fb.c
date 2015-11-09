/* $NetBSD: tegra_drm_fb.c,v 1.1 2015/11/09 23:05:58 jmcneill Exp $ */

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
__KERNEL_RCSID(0, "$NetBSD: tegra_drm_fb.c,v 1.1 2015/11/09 23:05:58 jmcneill Exp $");

#include <drm/drmP.h>
#include <drm/drm_crtc.h>
#include <drm/drm_fb_helper.h>
#include <drm/drm_crtc_helper.h>

#include <arm/nvidia/tegra_drm.h>

static int	tegra_fb_probe(struct drm_fb_helper *,
		    struct drm_fb_helper_surface_size *);

static struct drm_fb_helper_funcs tegra_fb_helper_funcs = {
	.fb_probe = tegra_fb_probe
};

int
tegra_drm_fb_init(struct drm_device *ddev)
{
	struct tegra_fbdev *fbdev;
	struct drm_framebuffer *fb;
	struct drm_mode_fb_cmd2 cmd;
	int error;

	fbdev = kmem_zalloc(sizeof(*fbdev), KM_SLEEP);
	if (fbdev == NULL)
		return -ENOMEM;
	fbdev->helper.funcs = &tegra_fb_helper_funcs;

	error = drm_fb_helper_init(ddev, &fbdev->helper, 2, 1);
	if (error) {
		kmem_free(fbdev, sizeof(*fbdev));
		return error;
	}

	memset(&cmd, 0, sizeof(cmd));
	cmd.width = 4096;
	cmd.height = 2160;
	cmd.pixel_format = DRM_FORMAT_ARGB8888;
	cmd.pitches[0] = cmd.width * (32 / 8);

	fb = ddev->mode_config.funcs->fb_create(ddev, NULL, &cmd);
	if (fb == NULL) {
		DRM_ERROR("couldn't create framebuffer\n");
		return -EIO;
	}
	fbdev->helper.fb = fb;

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

	memset(&tfa, 0, sizeof(tfa));
	tfa.tfa_drm_dev = ddev;
	tfa.tfa_fb_helper = helper;
	tfa.tfa_fb_sizes = *sizes;
	tfa.tfa_fb_bst = sc->sc_bst;
	tfa.tfa_fb_dmat = sc->sc_dmat;

	helper->fbdev = config_found_ia(ddev->dev, "tegrafbbus", &tfa, NULL);
	if (helper->fbdev == NULL) {
		DRM_ERROR("unable to attach tegrafb\n");
		return -ENXIO;
	}

	return 0;
}
