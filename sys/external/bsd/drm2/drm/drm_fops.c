/*	$NetBSD: drm_fops.c,v 1.14 2018/08/27 15:22:54 riastradh Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: drm_fops.c,v 1.14 2018/08/27 15:22:54 riastradh Exp $");

#include <sys/param.h>
#include <sys/select.h>

#include <drm/drmP.h>
#include <drm/drm_internal.h>
#include <drm/drm_legacy.h>
#include "../dist/drm/drm_legacy.h"

static int	drm_open_file_master(struct drm_file *);

static void	drm_master_release(struct drm_file *);
static void	drm_events_release(struct drm_file *);
static void	drm_close_file_master(struct drm_file *);

int
drm_open_file(struct drm_file *file, void *fp, struct drm_minor *minor)
{
	struct drm_device *const dev = minor->dev;
	int ret;

	file->authenticated = DRM_SUSER(); /* XXX */
	file->is_master = false;
	file->stereo_allowed = false;
	file->universal_planes = false;
	file->atomic = false;
	file->allowed_master = false;
	file->magic = 0;
	INIT_LIST_HEAD(&file->lhead);
	file->minor = minor;
	file->lock_count = 0;
	/* file->object_idr is initialized by drm_gem_open.  */
	/* file->table_lock is initialized by drm_gem_open.  */
	file->filp = fp;
	file->driver_priv = NULL;
	file->master = NULL;
	INIT_LIST_HEAD(&file->fbs);
	linux_mutex_init(&file->fbs_lock);
	INIT_LIST_HEAD(&file->blobs);
	DRM_INIT_WAITQUEUE(&file->event_wait, "drmevent");
	selinit(&file->event_selq);
	INIT_LIST_HEAD(&file->event_list);
	file->event_space = 0x1000; /* XXX cargo-culted from Linux */
	/* file->prime is initialized by drm_prime_init_file_private.  */

	if (drm_core_check_feature(dev, DRIVER_GEM))
		drm_gem_open(dev, file);
	if (drm_core_check_feature(dev, DRIVER_PRIME))
		drm_prime_init_file_private(&file->prime);

	if (dev->driver->open) {
		ret = (*dev->driver->open)(dev, file);
		if (ret)
			goto fail0;
	}

	ret = drm_open_file_master(file);
	if (ret)
		goto fail1;

        mutex_lock(&dev->struct_mutex);
        list_add(&file->lhead, &dev->filelist);
        mutex_unlock(&dev->struct_mutex);

	/* Success!  */
	return 0;

fail1:	/*
	 * XXX This error branch needs scrutiny, but Linux's error
	 * branches are incomprehensible and look wronger.
	 */
	if (dev->driver->preclose)
		(*dev->driver->preclose)(dev, file);
	if (dev->driver->postclose)
		(*dev->driver->postclose)(dev, file);
fail0:
	if (drm_core_check_feature(dev, DRIVER_PRIME))
		drm_prime_destroy_file_private(&file->prime);
	if (drm_core_check_feature(dev, DRIVER_GEM))
		drm_gem_release(dev, file);
	return ret;
}

int
drm_new_set_master(struct drm_device *dev, struct drm_file *file)
{
	struct drm_master *old_master;
	int ret;

	KASSERT(mutex_is_locked(&dev->master_mutex));
	KASSERT(file->minor->type == DRM_MINOR_LEGACY);
	KASSERT(file->minor->master == NULL);

	file->minor->master = drm_master_create(file->minor);
	if (file->minor->master == NULL) {
		ret = -ENOMEM;
		goto fail0;
	}

	/*
	 * Save the old master, to drop a reference later if all goes
	 * well, and get a reference to the new one.
	 */
	old_master = file->master;
	file->master = drm_master_get(file->minor->master);

	/* Invoke the driver callbacks master_create and master_set.  */
	if (dev->driver->master_create) {
		ret = (*dev->driver->master_create)(dev, file->minor->master);
		if (ret)
			goto fail1;
	}

	if (dev->driver->master_set) {
		ret = (*dev->driver->master_set)(dev, file, true);
		if (ret)
			goto fail1;
	}

	/*
	 * Mark ourselves as an authenticated master, and allowed to
	 * set a new master.
	 */
	file->is_master = 1;
	file->allowed_master = 1;
	file->authenticated = 1;

	/* If there was an old master, release it now.  */
	if (old_master)
		drm_master_put(&old_master);

	/* Success!  */
	return 0;

fail1:
	/* Release the master we just created.  */
	drm_master_put(&file->minor->master);
	KASSERT(file->minor->master == NULL);
	/* Release the reference we just added in the file.  */
	drm_master_put(&file->master);
	KASSERT(file->master == NULL);
	/* Restore the old master if there was one.  */
	file->master = old_master;
fail0:	KASSERT(ret);
	return ret;
}

static int
drm_open_file_master(struct drm_file *file)
{
	struct drm_device *const dev = file->minor->dev;
	int ret;

	/* If this is not the legacy device, there are no masters.  */
	if (file->minor->type != DRM_MINOR_LEGACY)
		return 0;

	mutex_lock(&dev->master_mutex);
	if (file->minor->master != NULL) {
		/*
		 * If the minor already has a master, get a reference
		 * to it.
		 */
		file->master = drm_master_get(file->minor->master);
		ret = 0;
	} else {
		/*
		 * Otherwise, automatically behave as though we had
		 * just done setmaster.
		 */
		ret = drm_new_set_master(dev, file);
	}
	mutex_unlock(&dev->master_mutex);

	return ret;
}

void
drm_close_file(struct drm_file *file)
{
	struct drm_minor *const minor = file->minor;
	struct drm_device *const dev = minor->dev;

	mutex_lock(&dev->struct_mutex);
	list_del(&file->lhead);
	if (file->magic)
		idr_remove(&file->master->magic_map, file->magic);
	mutex_unlock(&dev->struct_mutex);

	if (dev->driver->preclose)
		(*dev->driver->preclose)(dev, file);

	if (minor->master)
		drm_master_release(file);
	if (drm_core_check_feature(dev, DRIVER_HAVE_DMA))
		drm_legacy_reclaim_buffers(dev, file);
	drm_events_release(file);
	if (drm_core_check_feature(dev, DRIVER_MODESET)) {
		drm_fb_release(file);
		drm_property_destroy_user_blobs(dev, file);
	}
	if (drm_core_check_feature(dev, DRIVER_GEM))
		drm_gem_release(dev, file);
	drm_legacy_ctxbitmap_flush(dev, file);
	drm_close_file_master(file);

	if (dev->driver->postclose)
		(*dev->driver->postclose)(dev, file);

	if (drm_core_check_feature(dev, DRIVER_PRIME))
		drm_prime_destroy_file_private(&file->prime);

	seldestroy(&file->event_selq);
	DRM_DESTROY_WAITQUEUE(&file->event_wait);
	linux_mutex_destroy(&file->fbs_lock);
}

static void
drm_master_release(struct drm_file *file)
{

	/*
	 * XXX I think this locking concept is wrong -- we need to hold
	 * file->master->lock.spinlock across the two calls to
	 * drm_legacy_i_have_hw_lock and drm_legacy_lock_free.
	 */
	if (drm_legacy_i_have_hw_lock(file->minor->dev, file))
		drm_legacy_lock_free(&file->master->lock,
		    _DRM_LOCKING_CONTEXT(file->master->lock.hw_lock->lock));
}

static void
drm_events_release(struct drm_file *file)
{
	struct drm_device *const dev = file->minor->dev;
	struct drm_pending_vblank_event *vblank, *vblank_next;
	struct drm_pending_event *event, *event_next;
	unsigned long flags;

	spin_lock_irqsave(&dev->event_lock, flags);

	list_for_each_entry_safe(vblank, vblank_next, &dev->vblank_event_list,
	    base.link) {
		if (vblank->base.file_priv == file) {
			list_del(&vblank->base.link);
			drm_vblank_put(dev, vblank->pipe);
			(*vblank->base.destroy)(&vblank->base);
		}
	}
	list_for_each_entry_safe(event, event_next, &file->event_list, link) {
		(*event->destroy)(event);
	}

	spin_unlock_irqrestore(&dev->event_lock, flags);
}

static void
drm_close_file_master(struct drm_file *file)
{
	struct drm_device *const dev = file->minor->dev;

	mutex_lock(&dev->master_mutex);
	if (file->is_master) {
		if (file->minor->master == file->master) {
			if (dev->driver->master_drop)
				(*dev->driver->master_drop)(dev, file, true);
			drm_master_put(&file->minor->master);
		}
	}
	if (file->master != NULL)
		drm_master_put(&file->master);
	file->is_master = 0;
	mutex_unlock(&dev->master_mutex);
}
