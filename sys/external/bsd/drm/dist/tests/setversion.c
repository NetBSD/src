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

#include <limits.h>
#include "drmtest.h"

/**
 * Checks DRM_IOCTL_SET_VERSION.
 *
 * This tests that we can get the actual version out, and that setting invalid
 * major/minor numbers fails appropriately.  It does not check the actual
 * behavior differenses resulting from an increased DI version.
 */
int main(int argc, char **argv)
{
	int fd, ret;
	drm_set_version_t sv, version;

	if (getuid() != 0) {
		fprintf(stderr, "setversion test requires root, skipping\n");
		return 0;
	}

	fd = drm_open_any_master();

	/* First, check that we can get the DD/DI versions. */
	memset(&version, 0, sizeof(version));
	version.drm_di_major = -1;
	version.drm_di_minor = -1;
	version.drm_dd_major = -1;
	version.drm_dd_minor = -1;
	ret = ioctl(fd, DRM_IOCTL_SET_VERSION, &version);
	assert(ret == 0);
	assert(version.drm_di_major != -1);
	assert(version.drm_di_minor != -1);
	assert(version.drm_dd_major != -1);
	assert(version.drm_dd_minor != -1);

	/* Check that an invalid DI major fails */
	sv = version;
	sv.drm_di_major++;
	ret = ioctl(fd, DRM_IOCTL_SET_VERSION, &sv);
	assert(ret == -1 && errno == EINVAL);

	/* Check that an invalid DI minor fails */
	sv = version;
	sv.drm_di_major++;
	ret = ioctl(fd, DRM_IOCTL_SET_VERSION, &sv);
	assert(ret == -1 && errno == EINVAL);

	/* Check that an invalid DD major fails */
	sv = version;
	sv.drm_dd_major++;
	ret = ioctl(fd, DRM_IOCTL_SET_VERSION, &sv);
	assert(ret == -1 && errno == EINVAL);

	/* Check that an invalid DD minor fails */
	sv = version;
	sv.drm_dd_minor++;
	ret = ioctl(fd, DRM_IOCTL_SET_VERSION, &sv);
	assert(ret == -1 && errno == EINVAL);

	close(fd);
	return 0;
}
