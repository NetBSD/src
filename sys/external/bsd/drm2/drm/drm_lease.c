/*	$NetBSD: drm_lease.c,v 1.5 2021/12/19 11:08:47 riastradh Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: drm_lease.c,v 1.5 2021/12/19 11:08:47 riastradh Exp $");

#include <sys/types.h>
#include <sys/errno.h>
#include <sys/systm.h>

#include <linux/mutex.h>

#include <drm/drm_auth.h>
#include <drm/drm_device.h>
#include <drm/drm_file.h>
#include <drm/drm_lease.h>

/*
 * drm_lease_owner(master)
 *
 *	Return the root of the lease tree, following parent pointers up
 *	from master.
 */
struct drm_master *
drm_lease_owner(struct drm_master *master)
{
	struct drm_master *lessor;

	while ((lessor = master->lessor) != NULL)
		master = lessor;

	return master;
}

/*
 * drm_lease_held(file, id)
 *
 *	XXX
 */
bool
drm_lease_held(struct drm_file *file, int id)
{
	struct drm_master *master;
	bool held;

	if (file == NULL || (master = file->master) == NULL)
		return true;

	mutex_lock(&master->dev->mode_config.idr_mutex);
	held = _drm_lease_held(file, id);
	mutex_unlock(&master->dev->mode_config.idr_mutex);

	return held;
}

/*
 * _drm_lease_held(file, id)
 *
 *	Same as drm_lease_held but caller must hold file's root
 *	master's dev->mode_config.idr_mutex.
 */
bool
_drm_lease_held(struct drm_file *file, int id)
{
	return true;
}

/*
 * drm_lease_revoke(master)
 *
 *	XXX
 */
void
drm_lease_revoke(struct drm_master *master)
{
}

/*
 * drm_lease_filter_crtcs(file, crtcs)
 *
 *	XXX
 */
uint32_t
drm_lease_filter_crtcs(struct drm_file *file, uint32_t crtcs)
{
	return crtcs;
}

/*
 * drm_mode_create_lease_ioctl(dev, data, file)
 *
 *	DRM_IOCTL_MODE_CREATE_LEASE(struct drm_mode_create_lease) ioctl
 *	implementation.
 */
int
drm_mode_create_lease_ioctl(struct drm_device *dev, void *data,
    struct drm_file *file)
{
	struct drm_mode_create_lease *args __unused = data;

	/* XXX not yet implemented */
	return -ENODEV;
}

/*
 * drm_mode_get_lease_ioctl(dev, data, file)
 *
 *	DRM_IOCTL_MODE_GET_LEASE(struct drm_mode_get_lease) ioctl
 *	implementation.
 */
int
drm_mode_get_lease_ioctl(struct drm_device *dev, void *data,
    struct drm_file *file)
{
	struct drm_mode_get_lease *args __unused = data;

	/* XXX not yet implemented */
	return -ENODEV;
}

/*
 * drm_mode_list_lessees_ioctl(dev, data, file)
 *
 *	DRM_IOCTL_MODE_LIST_LESSEES(struct drm_mode_list_lessees) ioctl
 *	implementation.
 */
int
drm_mode_list_lessees_ioctl(struct drm_device *dev, void *data,
    struct drm_file *file)
{
	struct drm_mode_list_lessees *args __unused = data;

	/* XXX not yet implemented */
	return -ENODEV;
}

/*
 * drm_mode_revoke_lease_ioctl(dev, data, file)
 *
 *	DRM_IOCTL_MODE_REVOKE_LEASE(struct drm_mode_revoke_lease) ioctl
 *	implementation.
 */
int
drm_mode_revoke_lease_ioctl(struct drm_device *dev, void *data,
    struct drm_file *file)
{
	struct drm_mode_revoke_lease *args __unused = data;

	/* XXX not yet implemented */
	return -ENODEV;
}

/*
 * drm_lease_destroy(master)
 *
 *	Notify lessees that master is being destroyed.
 */
void
drm_lease_destroy(struct drm_master *master)
{
}
