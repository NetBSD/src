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

#include <fcntl.h>
#include <fnmatch.h>
#include <sys/stat.h>
#include "drmtest.h"

#define LIBUDEV_I_KNOW_THE_API_IS_SUBJECT_TO_CHANGE
#include <libudev.h>

static int is_master(int fd)
{
	drm_client_t client;
	int ret;

	/* Check that we're the only opener and authed. */
	client.idx = 0;
	ret = ioctl(fd, DRM_IOCTL_GET_CLIENT, &client);
	assert (ret == 0);
	if (!client.auth)
		return 0;
	client.idx = 1;
	ret = ioctl(fd, DRM_IOCTL_GET_CLIENT, &client);
	if (ret != -1 || errno != EINVAL)
		return 0;

	return 1;
}

/** Open the first DRM device matching the criteria */
int drm_open_matching(const char *pci_glob, int flags)
{
	struct udev *udev;
	struct udev_enumerate *e;
	struct udev_device *device, *parent;
        struct udev_list_entry *entry;
	const char *pci_id, *path;
	int i, fd;

	udev = udev_new();
	if (udev == NULL) {
		fprintf(stderr, "failed to initialize udev context\n");
		abort();
	}

	fd = -1;
	e = udev_enumerate_new(udev);
	udev_enumerate_add_match_subsystem(e, "drm");
        udev_enumerate_scan_devices(e);
        udev_list_entry_foreach(entry, udev_enumerate_get_list_entry(e)) {
		path = udev_list_entry_get_name(entry);
		device = udev_device_new_from_syspath(udev, path);
		parent = udev_device_get_parent(device);
		/* Filter out KMS output devices. */
		if (strcmp(udev_device_get_subsystem(parent), "pci") != 0)
			continue;
		pci_id = udev_device_get_property_value(parent, "PCI_ID");
		if (fnmatch(pci_glob, pci_id, 0) != 0)
			continue;
		fd = open(udev_device_get_devnode(device), O_RDWR);
		if (fd < 0)
			continue;
		if ((flags & DRM_TEST_MASTER) && !is_master(fd)) {
			close(fd);
			fd = -1;
			continue;
		}

		break;
	}
        udev_enumerate_unref(e);
	udev_unref(udev);

	return fd;
}

int drm_open_any(void)
{
	int fd = drm_open_matching("*:*", 0);

	if (fd < 0) {
		fprintf(stderr, "failed to open any drm device\n");
		abort();
	}

	return fd;
}

/**
 * Open the first DRM device we can find where we end up being the master.
 */
int drm_open_any_master(void)
{
	int fd = drm_open_matching("*:*", DRM_TEST_MASTER);

	if (fd < 0) {
		fprintf(stderr, "failed to open any drm device\n");
		abort();
	}

	return fd;

}
