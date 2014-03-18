/*	$NetBSD: intel_fb.c,v 1.2 2014/03/18 18:20:42 riastradh Exp $	*/

/*-
 * Copyright (c) 2013 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Taylor R. Campbell.
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
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/* intel_fb.c stubs */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: intel_fb.c,v 1.2 2014/03/18 18:20:42 riastradh Exp $");

#include <drm/drmP.h>
#include <drm/drm_fb_helper.h>

#include "i915_drv.h"
#include "intel_drv.h"

void
intel_fb_output_poll_changed(struct drm_device *dev __unused)
{
	struct drm_i915_private *const dev_priv = dev->dev_private;

	drm_fb_helper_hotplug_event(&dev_priv->fbdev->helper);
}

void
intel_fb_restore_mode(struct drm_device *dev __unused)
{
	struct drm_i915_private *const dev_priv = dev->dev_private;
	struct drm_plane *plane;
	int ret;

	mutex_lock(&dev->mode_config.mutex);

	ret = drm_fb_helper_restore_fbdev_mode(&dev_priv->fbdev->helper);
	if (ret)
		DRM_ERROR("failed to restore fbdev mode: %d\n", ret);

	list_for_each_entry(plane, &dev->mode_config.plane_list, head)
		(*plane->funcs->disable_plane)(plane);

	mutex_unlock(&dev->mode_config.mutex);
}
