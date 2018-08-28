/*	$NetBSD: i915_gem_debug.c,v 1.2 2018/08/27 04:58:23 riastradh Exp $	*/

/*
 * Copyright © 2008 Intel Corporation
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 *
 * Authors:
 *    Keith Packard <keithp@keithp.com>
 *
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: i915_gem_debug.c,v 1.2 2018/08/27 04:58:23 riastradh Exp $");

#include <drm/drmP.h>
#include <drm/i915_drm.h>
#include "i915_drv.h"

#if WATCH_LISTS
int
i915_verify_lists(struct drm_device *dev)
{
	static int warned;
	struct drm_i915_private *dev_priv = to_i915(dev);
	struct drm_i915_gem_object *obj;
	struct intel_engine_cs *ring;
	int err = 0;
	int i;

	if (warned)
		return 0;

	for_each_ring(ring, dev_priv, i) {
		list_for_each_entry(obj, &ring->active_list, ring_list[ring->id]) {
			if (obj->base.dev != dev ||
			    !atomic_read(&obj->base.refcount.refcount)) {
				DRM_ERROR("%s: freed active obj %p\n",
					  ring->name, obj);
				err++;
				break;
			} else if (!obj->active ||
				   obj->last_read_req[ring->id] == NULL) {
				DRM_ERROR("%s: invalid active obj %p\n",
					  ring->name, obj);
				err++;
			} else if (obj->base.write_domain) {
				DRM_ERROR("%s: invalid write obj %p (w %x)\n",
					  ring->name,
					  obj, obj->base.write_domain);
				err++;
			}
		}
	}

	return warned = err;
}
#endif /* WATCH_LIST */
