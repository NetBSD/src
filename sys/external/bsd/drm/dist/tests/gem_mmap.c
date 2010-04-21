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

#define OBJECT_SIZE 16384

int do_read(int fd, int handle, void *buf, int offset, int size)
{
	struct drm_i915_gem_pread read;

	/* Ensure that we don't have any convenient data in buf in case
	 * we fail.
	 */
	memset(buf, 0xd0, size);

	memset(&read, 0, sizeof(read));
	read.handle = handle;
	read.data_ptr = (uintptr_t)buf;
	read.size = size;
	read.offset = offset;

	return ioctl(fd, DRM_IOCTL_I915_GEM_PREAD, &read);
}

int do_write(int fd, int handle, void *buf, int offset, int size)
{
	struct drm_i915_gem_pwrite write;

	memset(&write, 0, sizeof(write));
	write.handle = handle;
	write.data_ptr = (uintptr_t)buf;
	write.size = size;
	write.offset = offset;

	return ioctl(fd, DRM_IOCTL_I915_GEM_PWRITE, &write);
}

int main(int argc, char **argv)
{
	int fd;
	struct drm_i915_gem_create create;
	struct drm_i915_gem_mmap mmap;
	struct drm_gem_close unref;
	uint8_t expected[OBJECT_SIZE];
	uint8_t buf[OBJECT_SIZE];
	uint8_t *addr;
	int ret;
	int handle;

	fd = drm_open_matching("8086:*", 0);
	if (fd < 0) {
		fprintf(stderr, "failed to open intel drm device, skipping\n");
		return 0;
	}

	memset(&mmap, 0, sizeof(mmap));
	mmap.handle = 0x10101010;
	mmap.offset = 0;
	mmap.size = 4096;
	printf("Testing mmaping of bad object.\n");
	ret = ioctl(fd, DRM_IOCTL_I915_GEM_MMAP, &mmap);
	assert(ret == -1 && errno == EBADF);

	memset(&create, 0, sizeof(create));
	create.size = OBJECT_SIZE;
	ret = ioctl(fd, DRM_IOCTL_I915_GEM_CREATE, &create);
	assert(ret == 0);
	handle = create.handle;

	printf("Testing mmaping of newly created object.\n");
	mmap.handle = handle;
	mmap.offset = 0;
	mmap.size = OBJECT_SIZE;
	ret = ioctl(fd, DRM_IOCTL_I915_GEM_MMAP, &mmap);
	assert(ret == 0);
	addr = (uint8_t *)(uintptr_t)mmap.addr_ptr;

	printf("Testing contents of newly created object.\n");
	memset(expected, 0, sizeof(expected));
	assert(memcmp(addr, expected, sizeof(expected)) == 0);

	printf("Testing coherency of writes and mmap reads.\n");
	memset(buf, 0, sizeof(buf));
	memset(buf + 1024, 0x01, 1024);
	memset(expected + 1024, 0x01, 1024);
	ret = do_write(fd, handle, buf, 0, OBJECT_SIZE);
	assert(ret == 0);
	assert(memcmp(buf, addr, sizeof(buf)) == 0);

	printf("Testing that mapping stays after close\n");
	unref.handle = handle;
	ret = ioctl(fd, DRM_IOCTL_GEM_CLOSE, &unref);
	assert(ret == 0);
	assert(memcmp(buf, addr, sizeof(buf)) == 0);

	printf("Testing unmapping\n");
	munmap(addr, OBJECT_SIZE);

	close(fd);

	return 0;
}
