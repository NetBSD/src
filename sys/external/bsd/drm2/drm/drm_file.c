/*	$NetBSD: drm_file.c,v 1.4 2021/12/19 09:52:00 riastradh Exp $	*/

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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: drm_file.c,v 1.4 2021/12/19 09:52:00 riastradh Exp $");

#include <sys/param.h>
#include <sys/select.h>

#include <linux/capability.h>

#include <drm/drm_drv.h>
#include <drm/drm_legacy.h>
#include <drm/drm_file.h>

#include "../dist/drm/drm_crtc_internal.h"
#include "../dist/drm/drm_internal.h"
#include "../dist/drm/drm_legacy.h"

static void	drm_events_release(struct drm_file *);

int
drm_open_file(struct drm_file *file, void *fp, struct drm_minor *minor)
{
	/*
	 * XXX Synchronize with dist/drm/drm_file.c: drm_file_alloc,
	 * drm_open_helper.
	 */
	struct drm_device *const dev = minor->dev;
	int ret;
	bool postclose = false;

	file->authenticated = capable(CAP_SYS_ADMIN); /* XXX */
	file->is_master = false;
	file->stereo_allowed = false;
	file->universal_planes = false;
	file->atomic = false;
	file->aspect_ratio_allowed = false;
	file->writeback_connectors = false;
	file->is_master = false;
	file->master = NULL;
	file->magic = 0;

	INIT_LIST_HEAD(&file->lhead);
	file->minor = minor;
	/* file->object_idr is initialized by drm_gem_open.  */
	/* file->table_lock is initialized by drm_gem_open.  */
	/* file->syncobj_idr is initialized by drm_syncobj_open.  */
	/* file->syncobj_table_lock is initialized by drm_syncobj_open.  */
	file->filp = fp;
	file->driver_priv = NULL;
	INIT_LIST_HEAD(&file->fbs);
	linux_mutex_init(&file->fbs_lock);
	INIT_LIST_HEAD(&file->blobs);
	DRM_INIT_WAITQUEUE(&file->event_wait, "drmevent");
	selinit(&file->event_selq);
	INIT_LIST_HEAD(&file->event_list);
	file->event_space = 0x1000; /* XXX cargo-culted from Linux */
	/* file->prime is initialized by drm_prime_init_file_private.  */
	file->event_read_lock = NULL;
	DRM_INIT_WAITQUEUE(&file->event_read_wq, "drmevtrd");

	if (drm_core_check_feature(dev, DRIVER_GEM))
		drm_gem_open(dev, file);
	if (drm_core_check_feature(dev, DRIVER_SYNCOBJ))
		drm_syncobj_open(file);
	drm_prime_init_file_private(&file->prime);

	if (dev->driver->open) {
		ret = (*dev->driver->open)(dev, file);
		if (ret)
			goto fail0;
	}

	if (drm_is_primary_client(file)) {
		ret = drm_master_open(file);
		if (ret)
			goto fail1;
	}

        mutex_lock(&dev->struct_mutex);
        list_add(&file->lhead, &dev->filelist);
        mutex_unlock(&dev->struct_mutex);

	/* Success!  */
	return 0;

fail1:
	postclose = true;
	if (drm_core_check_feature(dev, DRIVER_LEGACY) &&
	    dev->driver->preclose)
		(*dev->driver->preclose)(dev, file);
fail0:
	drm_prime_destroy_file_private(&file->prime);
	if (drm_core_check_feature(dev, DRIVER_SYNCOBJ))
		drm_syncobj_release(file);
	if (drm_core_check_feature(dev, DRIVER_GEM))
		drm_gem_release(dev, file);
	if (postclose &&
	    drm_core_check_feature(dev, DRIVER_LEGACY) &&
	    dev->driver->postclose)
		(*dev->driver->postclose)(dev, file);
	KASSERT(file->event_read_lock == NULL);
	DRM_DESTROY_WAITQUEUE(&file->event_read_wq);
	seldestroy(&file->event_selq);
	DRM_DESTROY_WAITQUEUE(&file->event_wait);
	linux_mutex_destroy(&file->fbs_lock);
	return ret;
}

void
drm_close_file(struct drm_file *file)
{
	/* XXX Synchronize with dist/drm/drm_file.c, drm_file_free.  */
	struct drm_minor *const minor = file->minor;
	struct drm_device *const dev = minor->dev;

	if (drm_core_check_feature(dev, DRIVER_LEGACY) &&
	    dev->driver->preclose)
		(*dev->driver->preclose)(dev, file);

	if (drm_core_check_feature(dev, DRIVER_LEGACY))
		drm_legacy_lock_release(dev, file->filp);

	if (drm_core_check_feature(dev, DRIVER_HAVE_DMA))
		drm_legacy_reclaim_buffers(dev, file);

	drm_events_release(file);

	if (drm_core_check_feature(dev, DRIVER_MODESET)) {
		drm_fb_release(file);
		drm_property_destroy_user_blobs(dev, file);
	}

	if (drm_core_check_feature(dev, DRIVER_SYNCOBJ))
		drm_syncobj_release(file);

	if (drm_core_check_feature(dev, DRIVER_GEM))
		drm_gem_release(dev, file);

	drm_legacy_ctxbitmap_flush(dev, file);

	if (drm_is_primary_client(file))
		drm_master_release(file);

	if (dev->driver->postclose)
		dev->driver->postclose(dev, file);

	drm_prime_destroy_file_private(&file->prime);

	KASSERT(list_empty(&file->event_list));

	seldestroy(&file->event_selq);
	DRM_DESTROY_WAITQUEUE(&file->event_wait);
	linux_mutex_destroy(&file->fbs_lock);
}

static void
drm_events_release(struct drm_file *file)
{
	/* XXX Synchronize with dist/drm/drm_file.c, drm_events_release.  */
	struct drm_device *const dev = file->minor->dev;
	struct drm_pending_event *event, *event_next;

	spin_lock(&dev->event_lock);
	list_for_each_entry_safe(event, event_next, &file->pending_event_list,
	    pending_link) {
		KASSERT(event->file_priv == file);
		list_del(&event->pending_link);
		event->file_priv = NULL;
	}
	list_for_each_entry_safe(event, event_next, &file->event_list, link) {
		list_del(&event->link);
		kfree(event);
	}
	spin_unlock(&dev->event_lock);
}
