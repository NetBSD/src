/*	$NetBSD: drm_gem_framebuffer_helper.c,v 1.3 2021/12/19 09:49:17 riastradh Exp $	*/

/*-
 * Copyright (c) 2018 The NetBSD Foundation, Inc.
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: drm_gem_framebuffer_helper.c,v 1.3 2021/12/19 09:49:17 riastradh Exp $");

#include <linux/err.h>
#include <linux/slab.h>

#include <drm/drm_print.h>
#include <drm/drm_fourcc.h>
#include <drm/drm_framebuffer.h>
#include <drm/drm_gem.h>
#include <drm/drm_gem_framebuffer_helper.h>
#include <drm/drm_modeset_helper.h>

#include <uapi/drm/drm_mode.h>

/*
 * drm_gem_fb_destroy(fb)
 *
 *	Release the objects in, clean up and, kfree a framebuffer
 *	allocated with drm_gem_fb_create_with_funcs.
 *
 *	Fit for use as struct drm_framebuffer_funcs::destroy.  Caller
 *	must guarantee that the struct drm_framebuffer is allocated
 *	with kmalloc.
 */
void
drm_gem_fb_destroy(struct drm_framebuffer *fb)
{
	unsigned plane;

	for (plane = 0; plane < __arraycount(fb->obj); plane++)
		drm_gem_object_put_unlocked(fb->obj[plane]);
	drm_framebuffer_cleanup(fb);
	kfree(fb);
}

/*
 * drm_gem_fb_create_handle(fb, file, handlep)
 *
 *	Create a GEM handle for the object of the first plane (plane=0)
 *	of fb in the specified drm file namespace, and store it in
 *	*handlep.
 *
 *	Returns 0 on success, negative error on failure.
 */
int
drm_gem_fb_create_handle(struct drm_framebuffer *fb, struct drm_file *file,
    unsigned *handlep)
{

	return drm_gem_handle_create(file, fb->obj[0], handlep);
}

/*
 * drm_gem_fb_create_with_funcs(dev, file, mode_cmd, funcs)
 *
 *	Create a framebuffer in the specified drm device from the given
 *	mode command, resolving mode_cmd's handles in the specified drm
 *	file.
 *
 *	Returns pointer on success, ERR_PTR on failure.
 *
 *	ENOENT	missing handle
 *	EINVAL	wrong size object, invalid mode format
 */
struct drm_framebuffer *
drm_gem_fb_create_with_funcs(struct drm_device *dev, struct drm_file *file,
    const struct drm_mode_fb_cmd2 *mode_cmd,
    const struct drm_framebuffer_funcs *funcs)
{
	struct drm_framebuffer *fb;
	unsigned plane;
	int ret;

	/* Allocate a framebuffer object with kmalloc.  */
	fb = kmalloc(sizeof(*fb), GFP_KERNEL);
	if (fb == NULL) {
		ret = -ENOMEM;
		goto fail0;
	}

	/*
	 * Fill framebuffer parameters from mode_cmd.  If they're not
	 * valid, fail with EINVAL.
	 */
	drm_helper_mode_fill_fb_struct(dev, fb, mode_cmd);
	if (fb->format == NULL) {
		ret = -EINVAL;
		goto fail1;
	}

	/* Get the object for each plane.  */
	for (plane = 0; plane < fb->format->num_planes; plane++) {
		unsigned vsub = (plane > 0 ? fb->format->vsub : 1); /* XXX ? */
		unsigned hsub = (plane > 0 ? fb->format->hsub : 1); /* XXX ? */
		unsigned handle = mode_cmd->handles[plane];
		unsigned size;

		/* Look up the object for this plane.  */
		fb->obj[plane] = drm_gem_object_lookup(file, handle);
		if (fb->obj[plane] == NULL) {
			ret = -ENOENT;
			goto fail2;
		}

		/* Confirm the object is large enough to handle it.  */
		size = (mode_cmd->height/vsub - 1)*mode_cmd->pitches[plane]
		    + (mode_cmd->width/hsub)*fb->format->cpp[plane]
		    + mode_cmd->offsets[plane];
		if (fb->obj[plane]->size < size) {
			ret = -EINVAL;
			plane++; /* free this one too */
			goto fail2;
		}
	}

	/* Initialize the framebuffer.  */
	ret = drm_framebuffer_init(dev, fb, funcs);
	if (ret)
		goto fail2;

	/* Success!  */
	return fb;

fail2:	while (plane --> 0)
		drm_gem_object_put_unlocked(fb->obj[plane]);
fail1:	kmem_free(fb, sizeof(*fb));
fail0:	KASSERT(ret);
	return ERR_PTR(ret);
}
