/*
 * Copyright © 2007 Intel Corporation
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
 *    Eric Anholt <eric@anholt.net>
 *
 */

#include "drmtest.h"

static void
set_draw_cliprects_empty(int fd, int drawable)
{
	int ret;
	struct drm_update_draw update;

	update.handle = drawable;
	update.type = DRM_DRAWABLE_CLIPRECTS;
	update.num = 0;
	update.data = 0;

	ret = ioctl(fd, DRM_IOCTL_UPDATE_DRAW, &update);
	assert(ret == 0);
}

static void
set_draw_cliprects_empty_fail(int fd, int drawable)
{
	int ret;
	struct drm_update_draw update;

	update.handle = drawable;
	update.type = DRM_DRAWABLE_CLIPRECTS;
	update.num = 0;
	update.data = 0;

	ret = ioctl(fd, DRM_IOCTL_UPDATE_DRAW, &update);
	assert(ret == -1 && errno == EINVAL);
}

static void
set_draw_cliprects_2(int fd, int drawable)
{
	int ret;
	struct drm_update_draw update;
	drm_clip_rect_t rects[2];

	rects[0].x1 = 0;
	rects[0].y1 = 0;
	rects[0].x2 = 10;
	rects[0].y2 = 10;

	rects[1].x1 = 10;
	rects[1].y1 = 10;
	rects[1].x2 = 20;
	rects[1].y2 = 20;

	update.handle = drawable;
	update.type = DRM_DRAWABLE_CLIPRECTS;
	update.num = 2;
	update.data = (unsigned long long)(uintptr_t)&rects;

	ret = ioctl(fd, DRM_IOCTL_UPDATE_DRAW, &update);
	assert(ret == 0);
}

static int add_drawable(int fd)
{
	drm_draw_t drawarg;
	int ret;

	/* Create a drawable.
	 * IOCTL_ADD_DRAW is RDWR, though it should really just be RD
	 */
	drawarg.handle = 0;
	ret = ioctl(fd, DRM_IOCTL_ADD_DRAW, &drawarg);
	assert(ret == 0);
	return drawarg.handle;
}

static int rm_drawable(int fd, int drawable, int fail)
{
	drm_draw_t drawarg;
	int ret;

	/* Create a drawable.
	 * IOCTL_ADD_DRAW is RDWR, though it should really just be RD
	 */
	drawarg.handle = drawable;
	ret = ioctl(fd, DRM_IOCTL_RM_DRAW, &drawarg);
	if (!fail)
		assert(ret == 0);
	else
		assert(ret == -1 && errno == EINVAL);

	return drawarg.handle;
}

/**
 * Tests drawable management: adding, removing, and updating the cliprects of
 * drawables.
 */
int main(int argc, char **argv)
{
	int fd, ret, d1, d2;

	if (getuid() != 0) {
		fprintf(stderr, "updatedraw test requires root, skipping\n");
		return 0;
	}

	fd = drm_open_any_master();

	d1 = add_drawable(fd);
	d2 = add_drawable(fd);
	/* Do a series of cliprect updates */
	set_draw_cliprects_empty(fd, d1);
	set_draw_cliprects_empty(fd, d2);
	set_draw_cliprects_2(fd, d1);
	set_draw_cliprects_empty(fd, d1);

	/* Remove our drawables */
	rm_drawable(fd, d1, 0);
	rm_drawable(fd, d2, 0);

	/* Check that removing an unknown drawable returns error */
	rm_drawable(fd, 0x7fffffff, 1);

	/* Attempt to set cliprects on a nonexistent drawable */
	set_draw_cliprects_empty_fail(fd, d1);

	close(fd);
	return 0;
}
