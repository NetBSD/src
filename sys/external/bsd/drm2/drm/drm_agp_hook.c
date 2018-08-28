/*	$NetBSD: drm_agp_hook.c,v 1.1 2018/08/28 03:41:39 riastradh Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: drm_agp_hook.c,v 1.1 2018/08/28 03:41:39 riastradh Exp $");

#include <sys/types.h>
#include <sys/condvar.h>
#include <sys/errno.h>
#include <sys/mutex.h>
#include <sys/once.h>

#include <drm/drmP.h>
#include <drm/drm_internal.h>

static struct {
	kmutex_t			lock;
	kcondvar_t			cv;
	unsigned			refcnt; /* at most one per device */
	const struct drm_agp_hooks	*hooks;
} agp_hooks __cacheline_aligned;

void
drm_agp_hooks_init(void)
{

	mutex_init(&agp_hooks.lock, MUTEX_DEFAULT, IPL_NONE);
	cv_init(&agp_hooks.cv, "agphooks");
	agp_hooks.refcnt = 0;
	agp_hooks.hooks = NULL;
}

void
drm_agp_hooks_fini(void)
{

	KASSERT(agp_hooks.hooks == NULL);
	KASSERT(agp_hooks.refcnt == 0);
	cv_destroy(&agp_hooks.cv);
	mutex_destroy(&agp_hooks.lock);
}

int
drm_agp_register(const struct drm_agp_hooks *hooks)
{
	int error = 0;

	mutex_enter(&agp_hooks.lock);
	if (agp_hooks.refcnt) {
		KASSERT(agp_hooks.hooks);
		error = EBUSY;
	} else {
		agp_hooks.refcnt++;
		agp_hooks.hooks = hooks;
	}
	mutex_exit(&agp_hooks.lock);

	return error;
}

int
drm_agp_deregister(const struct drm_agp_hooks *hooks)
{
	int error;

	mutex_enter(&agp_hooks.lock);
	KASSERT(agp_hooks.hooks == hooks);
	if (agp_hooks.refcnt > 1) {
		error = EBUSY;
	} else {
		agp_hooks.refcnt = 0;
		agp_hooks.hooks = NULL;
	}
	mutex_exit(&agp_hooks.lock);

	return error;
}

static const struct drm_agp_hooks *
drm_agp_hooks_acquire(void)
{
	const struct drm_agp_hooks *hooks;

	mutex_enter(&agp_hooks.lock);
	if (agp_hooks.refcnt == 0) {
		hooks = NULL;
	} else {
		KASSERT(agp_hooks.refcnt < UINT_MAX);
		agp_hooks.refcnt++;
		hooks = agp_hooks.hooks;
	}
	mutex_exit(&agp_hooks.lock);

	return hooks;
}

static void
drm_agp_hooks_release(const struct drm_agp_hooks *hooks)
{

	mutex_enter(&agp_hooks.lock);
	KASSERT(agp_hooks.hooks == hooks);
	KASSERT(agp_hooks.refcnt);
	if (--agp_hooks.refcnt == 0)
		cv_broadcast(&agp_hooks.cv);
	mutex_exit(&agp_hooks.lock);
}

struct drm_agp_head *
drm_agp_init(struct drm_device *dev)
{
	const struct drm_agp_hooks *hooks;
	struct drm_agp_head *agp;

	if ((hooks = drm_agp_hooks_acquire()) == NULL)
		return NULL;
	agp = hooks->agph_init(dev);
	if (agp == NULL)
		drm_agp_hooks_release(hooks);

	return agp;
}

void
drm_agp_fini(struct drm_device *dev)
{

	if (dev->agp == NULL)
		return;
	dev->agp->hooks->agph_clear(dev);
	drm_agp_hooks_release(dev->agp->hooks);
	kfree(dev->agp);
	dev->agp = NULL;
}

void
drm_agp_clear(struct drm_device *dev)
{

	if (dev->agp == NULL)
		return;
	dev->agp->hooks->agph_clear(dev);
}

int
drm_agp_acquire(struct drm_device *dev)
{

	if (dev->agp == NULL)
		return -ENODEV;
	return dev->agp->hooks->agph_acquire(dev);
}

int
drm_agp_release(struct drm_device *dev)
{

	if (dev->agp == NULL)
		return -EINVAL;
	return dev->agp->hooks->agph_release(dev);
}

int
drm_agp_enable(struct drm_device *dev, struct drm_agp_mode mode)
{

	if (dev->agp == NULL)
		return -EINVAL;
	return dev->agp->hooks->agph_enable(dev, mode);
}

int
drm_agp_info(struct drm_device *dev, struct drm_agp_info *info)
{

	if (dev->agp == NULL)
		return -EINVAL;
	return dev->agp->hooks->agph_info(dev, info);
}

int
drm_agp_alloc(struct drm_device *dev, struct drm_agp_buffer *request)
{

	if (dev->agp == NULL)
		return -EINVAL;
	return dev->agp->hooks->agph_alloc(dev, request);
}

int
drm_agp_free(struct drm_device *dev, struct drm_agp_buffer *request)
{

	if (dev->agp == NULL)
		return -EINVAL;
	return dev->agp->hooks->agph_free(dev, request);
}

int
drm_agp_bind(struct drm_device *dev, struct drm_agp_binding *request)
{

	if (dev->agp == NULL)
		return -EINVAL;
	return dev->agp->hooks->agph_bind(dev, request);
}

int
drm_agp_unbind(struct drm_device *dev, struct drm_agp_binding *request)
{

	if (dev->agp == NULL)
		return -EINVAL;
	return dev->agp->hooks->agph_unbind(dev, request);
}

#define	DEFINE_AGP_HOOK_IOCTL(NAME, FIELD)				      \
int									      \
NAME(struct drm_device *dev, void *data, struct drm_file *file)		      \
{									      \
									      \
	if (dev->agp == NULL)						      \
		return -ENODEV;						      \
	return dev->agp->hooks->FIELD(dev, data, file);			      \
}

DEFINE_AGP_HOOK_IOCTL(drm_agp_acquire_ioctl, agph_acquire_ioctl)
DEFINE_AGP_HOOK_IOCTL(drm_agp_release_ioctl, agph_release_ioctl)
DEFINE_AGP_HOOK_IOCTL(drm_agp_enable_ioctl, agph_enable_ioctl)
DEFINE_AGP_HOOK_IOCTL(drm_agp_info_ioctl, agph_info_ioctl)
DEFINE_AGP_HOOK_IOCTL(drm_agp_alloc_ioctl, agph_alloc_ioctl)
DEFINE_AGP_HOOK_IOCTL(drm_agp_free_ioctl, agph_free_ioctl)
DEFINE_AGP_HOOK_IOCTL(drm_agp_bind_ioctl, agph_bind_ioctl)
DEFINE_AGP_HOOK_IOCTL(drm_agp_unbind_ioctl, agph_unbind_ioctl)

void __pci_iomem *
drm_agp_borrow(struct drm_device *dev, unsigned bar, bus_size_t size)
{
	const struct drm_agp_hooks *hooks;
	void __pci_iomem *iomem;

	if ((hooks = drm_agp_hooks_acquire()) == NULL)
		return NULL;
	iomem = hooks->agph_borrow(dev, bar, size);
	drm_agp_hooks_release(hooks);

	return iomem;
}

void
drm_agp_flush(void)
{
	const struct drm_agp_hooks *hooks;

	if ((hooks = drm_agp_hooks_acquire()) == NULL)
		return;
	hooks->agph_flush();
	drm_agp_hooks_release(hooks);
}
