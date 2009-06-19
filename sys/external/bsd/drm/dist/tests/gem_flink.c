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
 *    Eric Anholt <eric@anholt.net>
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <fcntl.h>
#include <inttypes.h>
#include <errno.h>
#include <sys/stat.h>
#include "drm.h"
#include "i915_drm.h"

static void
test_flink(int fd)
{
	struct drm_i915_gem_create create;
	struct drm_gem_flink flink;
	struct drm_gem_open open;
	int ret;

	printf("Testing flink and open.\n");

	memset(&create, 0, sizeof(create));
	create.size = 16 * 1024;
	ret = ioctl(fd, DRM_IOCTL_I915_GEM_CREATE, &create);
	assert(ret == 0);

	flink.handle = create.handle;
	ret = ioctl(fd, DRM_IOCTL_GEM_FLINK, &flink);
	assert(ret == 0);

	open.name = flink.name;
	ret = ioctl(fd, DRM_IOCTL_GEM_OPEN, &open);
	assert(ret == 0);
	assert(open.handle != 0);
}

static void
test_double_flink(int fd)
{
	struct drm_i915_gem_create create;
	struct drm_gem_flink flink;
	struct drm_gem_flink flink2;
	int ret;

	printf("Testing repeated flink.\n");

	memset(&create, 0, sizeof(create));
	create.size = 16 * 1024;
	ret = ioctl(fd, DRM_IOCTL_I915_GEM_CREATE, &create);
	assert(ret == 0);

	flink.handle = create.handle;
	ret = ioctl(fd, DRM_IOCTL_GEM_FLINK, &flink);
	assert(ret == 0);

	flink2.handle = create.handle;
	ret = ioctl(fd, DRM_IOCTL_GEM_FLINK, &flink2);
	assert(ret == 0);
	assert(flink2.name == flink.name);
}

static void
test_bad_flink(int fd)
{
	struct drm_gem_flink flink;
	int ret;

	printf("Testing error return on bad flink ioctl.\n");

	flink.handle = 0x10101010;
	ret = ioctl(fd, DRM_IOCTL_GEM_FLINK, &flink);
	assert(ret == -1 && errno == EBADF);
}

static void
test_bad_open(int fd)
{
	struct drm_gem_open open;
	int ret;

	printf("Testing error return on bad open ioctl.\n");

	open.name = 0x10101010;
	ret = ioctl(fd, DRM_IOCTL_GEM_OPEN, &open);

	assert(ret == -1 && errno == ENOENT);
}

int main(int argc, char **argv)
{
	int fd;

	fd = drm_open_matching("8086:*", 0);
	if (fd < 0) {
		fprintf(stderr, "failed to open intel drm device, skipping\n");
		return 0;
	}

	test_flink(fd);
	test_double_flink(fd);
	test_bad_flink(fd);
	test_bad_open(fd);

	return 0;
}
