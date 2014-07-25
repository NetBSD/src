/*	$NetBSD: drm_drv.c,v 1.8 2014/07/25 08:10:39 dholland Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: drm_drv.c,v 1.8 2014/07/25 08:10:39 dholland Exp $");

#include <sys/param.h>
#include <sys/types.h>
#include <sys/conf.h>
#include <sys/device.h>
#include <sys/file.h>
#include <sys/filedesc.h>
#include <sys/ioccom.h>
#include <sys/kauth.h>
#ifndef _MODULE
/* XXX Mega-kludge because modules are broken.  */
#include <sys/once.h>
#endif
#include <sys/pmf.h>
#include <sys/poll.h>
#ifndef _MODULE
#include <sys/reboot.h>		/* XXX drm_init kludge */
#endif
#include <sys/select.h>

#include <uvm/uvm_extern.h>

#include <linux/err.h>

#include <linux/pm.h>

#include <drm/drmP.h>

static dev_type_open(drm_open);

static int	drm_firstopen(struct drm_device *);

static int	drm_close(struct file *);
static int	drm_read(struct file *, off_t *, struct uio *, kauth_cred_t,
		    int);
static int	drm_dequeue_event(struct drm_file *, size_t,
		    struct drm_pending_event **, int);
static int	drm_poll(struct file *, int);
static int	drm_kqfilter(struct file *, struct knote *);
static int	drm_stat(struct file *, struct stat *);
static int	drm_ioctl(struct file *, unsigned long, void *);
static int	drm_version_string(char *, size_t *, const char *);
static paddr_t	drm_mmap(dev_t, off_t, int);

static drm_ioctl_t	drm_version;
static drm_ioctl_t	drm_mmap_ioctl;

#define	DRM_IOCTL_DEF(IOCTL, FUNC, FLAGS)				\
	[DRM_IOCTL_NR(IOCTL)] = {					\
		.cmd = (IOCTL),						\
		.flags = (FLAGS),					\
		.func = (FUNC),						\
		.cmd_drv = 0,						\
	}

/* XXX Kludge for AGP.  */
static drm_ioctl_t	drm_agp_acquire_hook_ioctl;
static drm_ioctl_t	drm_agp_release_hook_ioctl;
static drm_ioctl_t	drm_agp_enable_hook_ioctl;
static drm_ioctl_t	drm_agp_info_hook_ioctl;
static drm_ioctl_t	drm_agp_alloc_hook_ioctl;
static drm_ioctl_t	drm_agp_free_hook_ioctl;
static drm_ioctl_t	drm_agp_bind_hook_ioctl;
static drm_ioctl_t	drm_agp_unbind_hook_ioctl;

#define	drm_agp_acquire_ioctl	drm_agp_acquire_hook_ioctl
#define	drm_agp_release_ioctl	drm_agp_release_hook_ioctl
#define	drm_agp_enable_ioctl	drm_agp_enable_hook_ioctl
#define	drm_agp_info_ioctl	drm_agp_info_hook_ioctl
#define	drm_agp_alloc_ioctl	drm_agp_alloc_hook_ioctl
#define	drm_agp_free_ioctl	drm_agp_free_hook_ioctl
#define	drm_agp_bind_ioctl	drm_agp_bind_hook_ioctl
#define	drm_agp_unbind_ioctl	drm_agp_unbind_hook_ioctl

/* Table copied verbatim from dist/drm/drm_drv.c.  */
static const struct drm_ioctl_desc drm_ioctls[] = {
	DRM_IOCTL_DEF(DRM_IOCTL_VERSION, drm_version, DRM_UNLOCKED|DRM_RENDER_ALLOW),
	DRM_IOCTL_DEF(DRM_IOCTL_GET_UNIQUE, drm_getunique, 0),
	DRM_IOCTL_DEF(DRM_IOCTL_GET_MAGIC, drm_getmagic, 0),
	DRM_IOCTL_DEF(DRM_IOCTL_IRQ_BUSID, drm_irq_by_busid, DRM_MASTER|DRM_ROOT_ONLY),
	DRM_IOCTL_DEF(DRM_IOCTL_GET_MAP, drm_getmap, DRM_UNLOCKED),
	DRM_IOCTL_DEF(DRM_IOCTL_GET_CLIENT, drm_getclient, DRM_UNLOCKED),
	DRM_IOCTL_DEF(DRM_IOCTL_GET_STATS, drm_getstats, DRM_UNLOCKED),
	DRM_IOCTL_DEF(DRM_IOCTL_GET_CAP, drm_getcap, DRM_UNLOCKED|DRM_RENDER_ALLOW),
	DRM_IOCTL_DEF(DRM_IOCTL_SET_CLIENT_CAP, drm_setclientcap, 0),
	DRM_IOCTL_DEF(DRM_IOCTL_SET_VERSION, drm_setversion, DRM_MASTER),

	DRM_IOCTL_DEF(DRM_IOCTL_SET_UNIQUE, drm_setunique, DRM_AUTH|DRM_MASTER|DRM_ROOT_ONLY),
	DRM_IOCTL_DEF(DRM_IOCTL_BLOCK, drm_noop, DRM_AUTH|DRM_MASTER|DRM_ROOT_ONLY),
	DRM_IOCTL_DEF(DRM_IOCTL_UNBLOCK, drm_noop, DRM_AUTH|DRM_MASTER|DRM_ROOT_ONLY),
	DRM_IOCTL_DEF(DRM_IOCTL_AUTH_MAGIC, drm_authmagic, DRM_AUTH|DRM_MASTER),

	DRM_IOCTL_DEF(DRM_IOCTL_ADD_MAP, drm_addmap_ioctl, DRM_AUTH|DRM_MASTER|DRM_ROOT_ONLY),
	DRM_IOCTL_DEF(DRM_IOCTL_RM_MAP, drm_rmmap_ioctl, DRM_AUTH),

	DRM_IOCTL_DEF(DRM_IOCTL_SET_SAREA_CTX, drm_setsareactx, DRM_AUTH|DRM_MASTER|DRM_ROOT_ONLY),
	DRM_IOCTL_DEF(DRM_IOCTL_GET_SAREA_CTX, drm_getsareactx, DRM_AUTH),

	DRM_IOCTL_DEF(DRM_IOCTL_SET_MASTER, drm_setmaster_ioctl, DRM_ROOT_ONLY),
	DRM_IOCTL_DEF(DRM_IOCTL_DROP_MASTER, drm_dropmaster_ioctl, DRM_ROOT_ONLY),

	DRM_IOCTL_DEF(DRM_IOCTL_ADD_CTX, drm_addctx, DRM_AUTH|DRM_ROOT_ONLY),
	DRM_IOCTL_DEF(DRM_IOCTL_RM_CTX, drm_rmctx, DRM_AUTH|DRM_MASTER|DRM_ROOT_ONLY),
	DRM_IOCTL_DEF(DRM_IOCTL_MOD_CTX, drm_noop, DRM_AUTH|DRM_MASTER|DRM_ROOT_ONLY),
	DRM_IOCTL_DEF(DRM_IOCTL_GET_CTX, drm_getctx, DRM_AUTH),
	DRM_IOCTL_DEF(DRM_IOCTL_SWITCH_CTX, drm_switchctx, DRM_AUTH|DRM_MASTER|DRM_ROOT_ONLY),
	DRM_IOCTL_DEF(DRM_IOCTL_NEW_CTX, drm_newctx, DRM_AUTH|DRM_MASTER|DRM_ROOT_ONLY),
	DRM_IOCTL_DEF(DRM_IOCTL_RES_CTX, drm_resctx, DRM_AUTH),

	DRM_IOCTL_DEF(DRM_IOCTL_ADD_DRAW, drm_noop, DRM_AUTH|DRM_MASTER|DRM_ROOT_ONLY),
	DRM_IOCTL_DEF(DRM_IOCTL_RM_DRAW, drm_noop, DRM_AUTH|DRM_MASTER|DRM_ROOT_ONLY),

	DRM_IOCTL_DEF(DRM_IOCTL_LOCK, drm_lock, DRM_AUTH),
	DRM_IOCTL_DEF(DRM_IOCTL_UNLOCK, drm_unlock, DRM_AUTH),

	DRM_IOCTL_DEF(DRM_IOCTL_FINISH, drm_noop, DRM_AUTH),

	DRM_IOCTL_DEF(DRM_IOCTL_ADD_BUFS, drm_addbufs, DRM_AUTH|DRM_MASTER|DRM_ROOT_ONLY),
	DRM_IOCTL_DEF(DRM_IOCTL_MARK_BUFS, drm_markbufs, DRM_AUTH|DRM_MASTER|DRM_ROOT_ONLY),
	DRM_IOCTL_DEF(DRM_IOCTL_INFO_BUFS, drm_infobufs, DRM_AUTH),
	DRM_IOCTL_DEF(DRM_IOCTL_MAP_BUFS, drm_mapbufs, DRM_AUTH),
	DRM_IOCTL_DEF(DRM_IOCTL_FREE_BUFS, drm_freebufs, DRM_AUTH),
	DRM_IOCTL_DEF(DRM_IOCTL_DMA, drm_dma_ioctl, DRM_AUTH),

	DRM_IOCTL_DEF(DRM_IOCTL_CONTROL, drm_control, DRM_AUTH|DRM_MASTER|DRM_ROOT_ONLY),

#if __OS_HAS_AGP
	DRM_IOCTL_DEF(DRM_IOCTL_AGP_ACQUIRE, drm_agp_acquire_ioctl, DRM_AUTH|DRM_MASTER|DRM_ROOT_ONLY),
	DRM_IOCTL_DEF(DRM_IOCTL_AGP_RELEASE, drm_agp_release_ioctl, DRM_AUTH|DRM_MASTER|DRM_ROOT_ONLY),
	DRM_IOCTL_DEF(DRM_IOCTL_AGP_ENABLE, drm_agp_enable_ioctl, DRM_AUTH|DRM_MASTER|DRM_ROOT_ONLY),
	DRM_IOCTL_DEF(DRM_IOCTL_AGP_INFO, drm_agp_info_ioctl, DRM_AUTH),
	DRM_IOCTL_DEF(DRM_IOCTL_AGP_ALLOC, drm_agp_alloc_ioctl, DRM_AUTH|DRM_MASTER|DRM_ROOT_ONLY),
	DRM_IOCTL_DEF(DRM_IOCTL_AGP_FREE, drm_agp_free_ioctl, DRM_AUTH|DRM_MASTER|DRM_ROOT_ONLY),
	DRM_IOCTL_DEF(DRM_IOCTL_AGP_BIND, drm_agp_bind_ioctl, DRM_AUTH|DRM_MASTER|DRM_ROOT_ONLY),
	DRM_IOCTL_DEF(DRM_IOCTL_AGP_UNBIND, drm_agp_unbind_ioctl, DRM_AUTH|DRM_MASTER|DRM_ROOT_ONLY),
#endif

	DRM_IOCTL_DEF(DRM_IOCTL_SG_ALLOC, drm_sg_alloc, DRM_AUTH|DRM_MASTER|DRM_ROOT_ONLY),
	DRM_IOCTL_DEF(DRM_IOCTL_SG_FREE, drm_sg_free, DRM_AUTH|DRM_MASTER|DRM_ROOT_ONLY),

	DRM_IOCTL_DEF(DRM_IOCTL_WAIT_VBLANK, drm_wait_vblank, DRM_UNLOCKED),

	DRM_IOCTL_DEF(DRM_IOCTL_MODESET_CTL, drm_modeset_ctl, 0),

	DRM_IOCTL_DEF(DRM_IOCTL_UPDATE_DRAW, drm_noop, DRM_AUTH|DRM_MASTER|DRM_ROOT_ONLY),

	DRM_IOCTL_DEF(DRM_IOCTL_GEM_CLOSE, drm_gem_close_ioctl, DRM_UNLOCKED|DRM_RENDER_ALLOW),
	DRM_IOCTL_DEF(DRM_IOCTL_GEM_FLINK, drm_gem_flink_ioctl, DRM_AUTH|DRM_UNLOCKED),
	DRM_IOCTL_DEF(DRM_IOCTL_GEM_OPEN, drm_gem_open_ioctl, DRM_AUTH|DRM_UNLOCKED),

	DRM_IOCTL_DEF(DRM_IOCTL_MODE_GETRESOURCES, drm_mode_getresources, DRM_CONTROL_ALLOW|DRM_UNLOCKED),

#ifndef __NetBSD__		/* XXX drm prime */
	DRM_IOCTL_DEF(DRM_IOCTL_PRIME_HANDLE_TO_FD, drm_prime_handle_to_fd_ioctl, DRM_AUTH|DRM_UNLOCKED|DRM_RENDER_ALLOW),
	DRM_IOCTL_DEF(DRM_IOCTL_PRIME_FD_TO_HANDLE, drm_prime_fd_to_handle_ioctl, DRM_AUTH|DRM_UNLOCKED|DRM_RENDER_ALLOW),
#endif

	DRM_IOCTL_DEF(DRM_IOCTL_MODE_GETPLANERESOURCES, drm_mode_getplane_res, DRM_CONTROL_ALLOW|DRM_UNLOCKED),
	DRM_IOCTL_DEF(DRM_IOCTL_MODE_GETCRTC, drm_mode_getcrtc, DRM_CONTROL_ALLOW|DRM_UNLOCKED),
	DRM_IOCTL_DEF(DRM_IOCTL_MODE_SETCRTC, drm_mode_setcrtc, DRM_MASTER|DRM_CONTROL_ALLOW|DRM_UNLOCKED),
	DRM_IOCTL_DEF(DRM_IOCTL_MODE_GETPLANE, drm_mode_getplane, DRM_CONTROL_ALLOW|DRM_UNLOCKED),
	DRM_IOCTL_DEF(DRM_IOCTL_MODE_SETPLANE, drm_mode_setplane, DRM_MASTER|DRM_CONTROL_ALLOW|DRM_UNLOCKED),
	DRM_IOCTL_DEF(DRM_IOCTL_MODE_CURSOR, drm_mode_cursor_ioctl, DRM_MASTER|DRM_CONTROL_ALLOW|DRM_UNLOCKED),
	DRM_IOCTL_DEF(DRM_IOCTL_MODE_GETGAMMA, drm_mode_gamma_get_ioctl, DRM_UNLOCKED),
	DRM_IOCTL_DEF(DRM_IOCTL_MODE_SETGAMMA, drm_mode_gamma_set_ioctl, DRM_MASTER|DRM_UNLOCKED),
	DRM_IOCTL_DEF(DRM_IOCTL_MODE_GETENCODER, drm_mode_getencoder, DRM_CONTROL_ALLOW|DRM_UNLOCKED),
	DRM_IOCTL_DEF(DRM_IOCTL_MODE_GETCONNECTOR, drm_mode_getconnector, DRM_CONTROL_ALLOW|DRM_UNLOCKED),
	DRM_IOCTL_DEF(DRM_IOCTL_MODE_ATTACHMODE, drm_noop, DRM_MASTER|DRM_CONTROL_ALLOW|DRM_UNLOCKED),
	DRM_IOCTL_DEF(DRM_IOCTL_MODE_DETACHMODE, drm_noop, DRM_MASTER|DRM_CONTROL_ALLOW|DRM_UNLOCKED),
	DRM_IOCTL_DEF(DRM_IOCTL_MODE_GETPROPERTY, drm_mode_getproperty_ioctl, DRM_CONTROL_ALLOW|DRM_UNLOCKED),
	DRM_IOCTL_DEF(DRM_IOCTL_MODE_SETPROPERTY, drm_mode_connector_property_set_ioctl, DRM_MASTER|DRM_CONTROL_ALLOW|DRM_UNLOCKED),
	DRM_IOCTL_DEF(DRM_IOCTL_MODE_GETPROPBLOB, drm_mode_getblob_ioctl, DRM_CONTROL_ALLOW|DRM_UNLOCKED),
	DRM_IOCTL_DEF(DRM_IOCTL_MODE_GETFB, drm_mode_getfb, DRM_CONTROL_ALLOW|DRM_UNLOCKED),
	DRM_IOCTL_DEF(DRM_IOCTL_MODE_ADDFB, drm_mode_addfb, DRM_CONTROL_ALLOW|DRM_UNLOCKED),
	DRM_IOCTL_DEF(DRM_IOCTL_MODE_ADDFB2, drm_mode_addfb2, DRM_CONTROL_ALLOW|DRM_UNLOCKED),
	DRM_IOCTL_DEF(DRM_IOCTL_MODE_RMFB, drm_mode_rmfb, DRM_CONTROL_ALLOW|DRM_UNLOCKED),
	DRM_IOCTL_DEF(DRM_IOCTL_MODE_PAGE_FLIP, drm_mode_page_flip_ioctl, DRM_MASTER|DRM_CONTROL_ALLOW|DRM_UNLOCKED),
	DRM_IOCTL_DEF(DRM_IOCTL_MODE_DIRTYFB, drm_mode_dirtyfb_ioctl, DRM_MASTER|DRM_CONTROL_ALLOW|DRM_UNLOCKED),
	DRM_IOCTL_DEF(DRM_IOCTL_MODE_CREATE_DUMB, drm_mode_create_dumb_ioctl, DRM_CONTROL_ALLOW|DRM_UNLOCKED),
	DRM_IOCTL_DEF(DRM_IOCTL_MODE_MAP_DUMB, drm_mode_mmap_dumb_ioctl, DRM_CONTROL_ALLOW|DRM_UNLOCKED),
	DRM_IOCTL_DEF(DRM_IOCTL_MODE_DESTROY_DUMB, drm_mode_destroy_dumb_ioctl, DRM_CONTROL_ALLOW|DRM_UNLOCKED),
	DRM_IOCTL_DEF(DRM_IOCTL_MODE_OBJ_GETPROPERTIES, drm_mode_obj_get_properties_ioctl, DRM_CONTROL_ALLOW|DRM_UNLOCKED),
	DRM_IOCTL_DEF(DRM_IOCTL_MODE_OBJ_SETPROPERTY, drm_mode_obj_set_property_ioctl, DRM_MASTER|DRM_CONTROL_ALLOW|DRM_UNLOCKED),
	DRM_IOCTL_DEF(DRM_IOCTL_MODE_CURSOR2, drm_mode_cursor2_ioctl, DRM_MASTER|DRM_CONTROL_ALLOW|DRM_UNLOCKED),

#ifdef __NetBSD__
	DRM_IOCTL_DEF(DRM_IOCTL_MMAP, drm_mmap_ioctl, DRM_UNLOCKED),
#endif
};

const struct cdevsw drm_cdevsw = {
	.d_open = drm_open,
	.d_close = noclose,
	.d_read = noread,
	.d_write = nowrite,
	.d_ioctl = noioctl,
	.d_stop = nostop,
	.d_tty = notty,
	.d_poll = nopoll,
	.d_mmap = drm_mmap,
	.d_kqfilter = nokqfilter,
	.d_discard = nodiscard,
	/* XXX was D_TTY | D_NEGOFFSAFE */
	/* XXX Add D_MPSAFE some day... */
	.d_flag = D_NEGOFFSAFE,
};

static const struct fileops drm_fileops = {
	.fo_read = drm_read,
	.fo_write = fbadop_write,
	.fo_ioctl = drm_ioctl,
	.fo_fcntl = fnullop_fcntl,
	.fo_poll = drm_poll,
	.fo_stat = drm_stat,
	.fo_close = drm_close,
	.fo_kqfilter = drm_kqfilter,
	.fo_restart = fnullop_restart,
};

static int
drm_open(dev_t d, int flags, int fmt, struct lwp *l)
{
	struct drm_minor *dminor;
	struct drm_device *dev;
	bool firstopen, lastclose;
	int fd;
	struct file *fp;
	int error;
	extern int drm_guarantee_initialized(void);

	error = drm_guarantee_initialized();
	if (error)
		goto fail0;

	if (flags & O_EXCL) {
		error = EBUSY;
		goto fail0;
	}

	dminor = drm_minor_acquire(minor(d));
	if (IS_ERR(dminor)) {
		/* XXX errno Linux->NetBSD */
		error = -PTR_ERR(dminor);
		goto fail0;
	}
	dev = dminor->dev;
	if (dev->switch_power_state != DRM_SWITCH_POWER_ON) {
		error = EINVAL;
		goto fail1;
	}

	spin_lock(&dev->count_lock);
	if (dev->open_count == INT_MAX) {
		spin_unlock(&dev->count_lock);
		error = EBUSY;
		goto fail1;
	}
	firstopen = (dev->open_count == 0);
	dev->open_count++;
	spin_unlock(&dev->count_lock);

	if (firstopen) {
		/* XXX errno Linux->NetBSD */
		error = drm_firstopen(dev);
		if (error)
			goto fail2;
	}

	error = fd_allocfile(&fp, &fd);
	if (error)
		goto fail2;

	struct drm_file *const file = kmem_zalloc(sizeof(*file), KM_SLEEP);
	/* XXX errno Linux->NetBSD */
	error = -drm_open_file(file, fp, dminor);
	if (error)
		goto fail3;

	error = fd_clone(fp, fd, flags, &drm_fileops, file);
	KASSERT(error == EMOVEFD); /* XXX */

	/* Success!  (But error has to be EMOVEFD, not 0.)  */
	return error;

fail3:	kmem_free(file, sizeof(*file));
	fd_abort(curproc, fp, fd);
fail2:	spin_lock(&dev->count_lock);
	KASSERT(0 < dev->open_count);
	--dev->open_count;
	lastclose = (dev->open_count == 0);
	spin_unlock(&dev->count_lock);
	if (lastclose)
		(void)drm_lastclose(dev);
fail1:	drm_minor_release(dminor);
fail0:	KASSERT(error);
	return error;
}

static int
drm_close(struct file *fp)
{
	struct drm_file *const file = fp->f_data;
	struct drm_minor *const dminor = file->minor;
	struct drm_device *const dev = dminor->dev;
	bool lastclose;

	drm_close_file(file);
	kmem_free(file, sizeof(*file));

	spin_lock(&dev->count_lock);
	KASSERT(0 < dev->open_count);
	--dev->open_count;
	lastclose = (dev->open_count == 0);
	spin_unlock(&dev->count_lock);

	if (lastclose)
		(void)drm_lastclose(dev);

	drm_minor_release(dminor);

	return 0;
}

static int
drm_firstopen(struct drm_device *dev)
{
	int ret;

	if (drm_core_check_feature(dev, DRIVER_MODESET))
		return 0;

	if (dev->driver->firstopen) {
		ret = (*dev->driver->firstopen)(dev);
		if (ret)
			goto fail0;
	}

	ret = drm_legacy_dma_setup(dev);
	if (ret)
		goto fail1;

	return 0;

fail2: __unused
	drm_legacy_dma_takedown(dev);
fail1:	if (dev->driver->lastclose)
		(*dev->driver->lastclose)(dev);
fail0:	KASSERT(ret);
	return ret;
}

int
drm_lastclose(struct drm_device *dev)
{
	struct drm_vma_entry *vma, *vma_temp;

	/* XXX Order is sketchy here...  */
	if (dev->driver->lastclose)
		(*dev->driver->lastclose)(dev);
	if (dev->irq_enabled && !drm_core_check_feature(dev, DRIVER_MODESET))
		drm_irq_uninstall(dev);

	mutex_lock(&dev->struct_mutex);
	drm_agp_clear(dev);
	drm_legacy_sg_cleanup(dev);
	list_for_each_entry_safe(vma, vma_temp, &dev->vmalist, head) {
		list_del(&vma->head);
		kfree(vma);
	}
	drm_legacy_dma_takedown(dev);
	mutex_unlock(&dev->struct_mutex);

	if (!drm_core_check_feature(dev, DRIVER_MODESET)) {
		dev->sigdata.lock = NULL;
		dev->context_flag = 0;
		dev->last_context = 0;
		dev->if_version = 0;
	}

	return 0;
}

static int
drm_read(struct file *fp, off_t *off, struct uio *uio, kauth_cred_t cred,
    int flags)
{
	struct drm_file *const file = fp->f_data;
	struct drm_pending_event *event;
	bool first;
	int error = 0;

	for (first = true; ; first = false) {
		int f = 0;

		if (!first || ISSET(fp->f_flag, FNONBLOCK))
			f |= FNONBLOCK;

		/* XXX errno Linux->NetBSD */
		error = -drm_dequeue_event(file, uio->uio_resid, &event, f);
		if (error) {
			if ((error == EWOULDBLOCK) && !first)
				error = 0;
			break;
		}
		if (event == NULL)
			break;
		error = uiomove(event->event, event->event->length, uio);
		if (error)	/* XXX Requeue the event?  */
			break;
		(*event->destroy)(event);
	}

	/* Success!  */
	return error;
}

static int
drm_dequeue_event(struct drm_file *file, size_t max_length,
    struct drm_pending_event **eventp, int flags)
{
	struct drm_device *const dev = file->minor->dev;
	struct drm_pending_event *event = NULL;
	unsigned long irqflags;
	int ret = 0;

	spin_lock_irqsave(&dev->event_lock, irqflags);

	if (ISSET(flags, FNONBLOCK)) {
		if (list_empty(&file->event_list))
			ret = -EWOULDBLOCK;
	} else {
		DRM_SPIN_WAIT_UNTIL(ret, &file->event_wait, &dev->event_lock,
		    !list_empty(&file->event_list));
	}
	if (ret)
		goto out;

	event = list_first_entry(&file->event_list, struct drm_pending_event,
	    link);
	if (event->event->length > max_length) {
		ret = 0;
		goto out;
	}

	file->event_space += event->event->length;
	list_del(&event->link);

out:	spin_unlock_irqrestore(&dev->event_lock, irqflags);
	*eventp = event;
	return ret;
}

static int
drm_poll(struct file *fp __unused, int events __unused)
{
	struct drm_file *const file = fp->f_data;
	struct drm_device *const dev = file->minor->dev;
	int revents = 0;
	unsigned long irqflags;

	if (!ISSET(events, (POLLIN | POLLRDNORM)))
		return 0;

	spin_lock_irqsave(&dev->event_lock, irqflags);
	if (list_empty(&file->event_list))
		selrecord(curlwp, &file->event_selq);
	else
		revents |= (events & (POLLIN | POLLRDNORM));
	spin_unlock_irqrestore(&dev->event_lock, irqflags);

	return revents;
}

static void	filt_drm_detach(struct knote *);
static int	filt_drm_event(struct knote *, long);

static const struct filterops drm_filtops =
	{ 1, NULL, filt_drm_detach, filt_drm_event };

static int
drm_kqfilter(struct file *fp, struct knote *kn)
{
	struct drm_file *const file = fp->f_data;
	struct drm_device *const dev = file->minor->dev;
	unsigned long irqflags;

	switch (kn->kn_filter) {
	case EVFILT_READ:
		kn->kn_fop = &drm_filtops;
		kn->kn_hook = file;
		spin_lock_irqsave(&dev->event_lock, irqflags);
		SLIST_INSERT_HEAD(&file->event_selq.sel_klist, kn, kn_selnext);
		spin_unlock_irqrestore(&dev->event_lock, irqflags);
		return 0;
	case EVFILT_WRITE:
	default:
		return EINVAL;
	}
}

static void
filt_drm_detach(struct knote *kn)
{
	struct drm_file *const file = kn->kn_hook;
	struct drm_device *const dev = file->minor->dev;
	unsigned long irqflags;

	spin_lock_irqsave(&dev->event_lock, irqflags);
	SLIST_REMOVE(&file->event_selq.sel_klist, kn, knote, kn_selnext);
	spin_unlock_irqrestore(&dev->event_lock, irqflags);
}

static int
filt_drm_event(struct knote *kn, long hint)
{
	struct drm_file *const file = kn->kn_hook;
	struct drm_device *const dev = file->minor->dev;
	unsigned long irqflags;
	int ret;

	if (hint == NOTE_SUBMIT)
		KASSERT(spin_is_locked(&dev->event_lock));
	else
		spin_lock_irqsave(&dev->event_lock, irqflags);
	if (list_empty(&file->event_list)) {
		ret = 0;
	} else {
		struct drm_pending_event *const event =
		    list_first_entry(&file->event_list,
			struct drm_pending_event, link);
		kn->kn_data = event->event->length;
		ret = 1;
	}
	if (hint == NOTE_SUBMIT)
		KASSERT(spin_is_locked(&dev->event_lock));
	else
		spin_unlock_irqrestore(&dev->event_lock, irqflags);

	return ret;
}

static int
drm_stat(struct file *fp, struct stat *st)
{
	struct drm_file *const file = fp->f_data;
	struct drm_minor *const dminor = file->minor;
	const dev_t devno = makedev(cdevsw_lookup_major(&drm_cdevsw),
	    64*dminor->index + dminor->type);

	(void)memset(st, 0, sizeof(*st));

	st->st_dev = devno;
	st->st_ino = 0;		/* XXX (dev,ino) uniqueness bleh */
	st->st_uid = kauth_cred_geteuid(fp->f_cred);
	st->st_gid = kauth_cred_getegid(fp->f_cred);
	st->st_mode = S_IFCHR;	/* XXX what? */
	st->st_rdev = devno;
	/* XXX what else? */

	return 0;
}

static int
drm_ioctl(struct file *fp, unsigned long cmd, void *data)
{
	struct drm_file *const file = fp->f_data;
	const unsigned int nr = DRM_IOCTL_NR(cmd);
	int error;

	switch (cmd) {
	case FIONBIO:
	case FIOASYNC:
		return 0;

#if 0				/* XXX why? */
	case SIOCSPGRP:
	case TIOCSPGRP:
	case FIOSETOWN:
		return fsetown(&dev->buf_pgid, cmd, data);

	case SIOCGPGRP:
	case TIOCGPGRP:
	case FIOGETOWN:
		return fgetown(&dev->buf_pgid, cmd, data);
#endif

	default:
		break;
	}

	if (IOCGROUP(cmd) != DRM_IOCTL_BASE)
		return EINVAL;

	KASSERT(file != NULL);
	KASSERT(file->minor != NULL);
	KASSERT(file->minor->dev != NULL);
	struct drm_device *const dev = file->minor->dev;
	const struct drm_ioctl_desc *ioctl;

	if ((DRM_COMMAND_BASE <= nr) && (nr < DRM_COMMAND_END)) {
		const unsigned int driver_nr = nr - DRM_COMMAND_BASE;
		if (driver_nr >= dev->driver->num_ioctls)
			return EINVAL;
		ioctl = &dev->driver->ioctls[driver_nr];
	} else if (nr < __arraycount(drm_ioctls)) {
		ioctl = &drm_ioctls[nr];
	} else {
		ioctl = NULL;
	}

	if ((ioctl == NULL) || (ioctl->func == NULL))
		return EINVAL;

	if (ISSET(ioctl->flags, DRM_ROOT_ONLY) && !DRM_SUSER())
		return EACCES;

	if (ISSET(ioctl->flags, DRM_AUTH) && !file->authenticated)
		return EACCES;

	if (ISSET(ioctl->flags, DRM_MASTER) && (file->master == NULL))
		return EACCES;

	if (!ISSET(ioctl->flags, DRM_CONTROL_ALLOW) &&
	    (file->minor->type == DRM_MINOR_CONTROL))
		return EACCES;

	if (!ISSET(ioctl->flags, DRM_UNLOCKED))
		mutex_lock(&drm_global_mutex);

	/* XXX errno Linux->NetBSD */
	error = -(*ioctl->func)(dev, data, file);

	if (!ISSET(ioctl->flags, DRM_UNLOCKED))
		mutex_unlock(&drm_global_mutex);

	return error;
}

static int
drm_version_string(char *target, size_t *lenp, const char *source)
{
	const size_t len = strlen(source);
	const size_t trunc_len = MIN(len, *lenp);

	*lenp = len;
	if ((trunc_len > 0) && (target != NULL))
		/* copyoutstr takes a buffer size, not a string length.  */
		/* XXX errno NetBSD->Linux */
		return -copyoutstr(source, target, trunc_len+1, NULL);

	return 0;
}

static int
drm_version(struct drm_device *dev, void *data, struct drm_file *file)
{
	struct drm_version *v = data;
	int error;

	v->version_major = dev->driver->major;
	v->version_minor = dev->driver->minor;
	v->version_patchlevel = dev->driver->patchlevel;

	error = drm_version_string(v->name, &v->name_len, dev->driver->name);
	if (error)
		goto out;
	error = drm_version_string(v->date, &v->date_len, dev->driver->date);
	if (error)
		goto out;
	error = drm_version_string(v->desc, &v->desc_len, dev->driver->desc);
	if (error)
		goto out;

out:	return error;
}

static paddr_t
drm_mmap(dev_t d, off_t offset, int prot)
{
	struct drm_minor *dminor;
	paddr_t paddr;

	dminor = drm_minor_acquire(minor(d));
	if (IS_ERR(dminor))
		return (paddr_t)-1;

	paddr = drm_mmap_paddr(dminor->dev, offset, prot);

	drm_minor_release(dminor);
	return paddr;
}

static int
drm_mmap_ioctl(struct drm_device *dev, void *data, struct drm_file *file)
{
	struct drm_mmap *const args = data;
	void *addr = args->dnm_addr;
	const size_t size = args->dnm_size;
	const int prot = args->dnm_prot;
	const int flags = args->dnm_flags;
	const off_t offset = args->dnm_offset;
	struct uvm_object *uobj;
	voff_t uoffset;
	const vm_prot_t vm_maxprot = (VM_PROT_READ | VM_PROT_WRITE);
	vm_prot_t vm_prot;
	int uvmflag;
	vaddr_t align, vaddr;
	int ret;

	/* XXX Copypasta from drm_gem_mmap.  */
	if (drm_device_is_unplugged(dev))
		return -ENODEV;

	if (prot != (prot & (PROT_READ | PROT_WRITE)))
		return -EACCES;
	if (flags != MAP_SHARED)
		return -EINVAL;
	if (offset != (offset & ~(PAGE_SIZE-1)))
		return -EINVAL;
	if (size != (size & ~(PAGE_SIZE-1)))
		return -EINVAL;
	(void)addr;		/* XXX ignore -- no MAP_FIXED for now */

	/* Try a GEM object mapping first.  */
	ret = drm_gem_mmap_object(dev, offset, size, prot, &uobj, &uoffset);
	if (ret)
		return ret;
	if (uobj != NULL)
		goto map;

	/* Try a traditional DRM mapping second.  */
	ret = drm_mmap_object(dev, offset, size, prot, &uobj, &uoffset);
	if (ret)
		return ret;
	if (uobj != NULL)
		goto map;

	/* Fail.  */
	return ret;

map:	vm_prot = ((ISSET(prot, PROT_READ)? VM_PROT_READ : 0) |
	    (ISSET(prot, PROT_WRITE)? VM_PROT_WRITE : 0));
	KASSERT(vm_prot == (vm_prot & vm_maxprot));
	uvmflag = UVM_MAPFLAG(vm_prot, vm_maxprot, UVM_INH_COPY,
	    UVM_ADV_RANDOM, 0);

	align = 0;		/* XXX */
	vaddr = (*curproc->p_emul->e_vm_default_addr)(curproc,
	    (vaddr_t)curproc->p_vmspace->vm_daddr, size);
	/* XXX errno NetBSD->Linux */
	ret = -uvm_map(&curproc->p_vmspace->vm_map, &vaddr, size, uobj,
	    uoffset, align, uvmflag);
	if (ret) {
		(*uobj->pgops->pgo_detach)(uobj);
		return ret;
	}

	/* Success!  */
	args->dnm_addr = (void *)vaddr;
	return 0;
}

static const struct drm_agp_hooks *volatile drm_current_agp_hooks;

int
drm_agp_register(const struct drm_agp_hooks *hooks)
{

	membar_producer();
	if (atomic_cas_ptr(&drm_current_agp_hooks, NULL, __UNCONST(hooks))
	    != NULL)
		return EBUSY;

	return 0;
}

void
drm_agp_deregister(const struct drm_agp_hooks *hooks)
{

	if (atomic_cas_ptr(&drm_current_agp_hooks, __UNCONST(hooks), NULL)
	    != hooks)
		panic("%s: wrong hooks: %p != %p", __func__,
		    hooks, drm_current_agp_hooks);
}

int
drm_agp_release_hook(struct drm_device *dev)
{
	const struct drm_agp_hooks *const hooks = drm_current_agp_hooks;

	if (hooks == NULL) {
		if ((dev != NULL) &&
		    (dev->control != NULL) &&
		    (dev->control->kdev != NULL))
			panic("drm_agp_release(%s): no agp loaded",
			    device_xname(dev->control->kdev));
		else
			panic("drm_agp_release(drm_device %p): no agp loaded",
			    dev);
	}
	membar_consumer();
	return (*hooks->agph_release)(dev);
}

#define	DEFINE_AGP_HOOK_IOCTL(NAME, HOOK)				      \
static int								      \
NAME(struct drm_device *dev, void *data, struct drm_file *file)		      \
{									      \
	const struct drm_agp_hooks *const hooks = drm_current_agp_hooks;      \
									      \
	if (hooks == NULL)						      \
		return -ENODEV;						      \
	membar_consumer();						      \
	return (*hooks->HOOK)(dev, data, file);				      \
}

DEFINE_AGP_HOOK_IOCTL(drm_agp_acquire_hook_ioctl, agph_acquire_ioctl)
DEFINE_AGP_HOOK_IOCTL(drm_agp_release_hook_ioctl, agph_release_ioctl)
DEFINE_AGP_HOOK_IOCTL(drm_agp_enable_hook_ioctl, agph_enable_ioctl)
DEFINE_AGP_HOOK_IOCTL(drm_agp_info_hook_ioctl, agph_info_ioctl)
DEFINE_AGP_HOOK_IOCTL(drm_agp_alloc_hook_ioctl, agph_alloc_ioctl)
DEFINE_AGP_HOOK_IOCTL(drm_agp_free_hook_ioctl, agph_free_ioctl)
DEFINE_AGP_HOOK_IOCTL(drm_agp_bind_hook_ioctl, agph_bind_ioctl)
DEFINE_AGP_HOOK_IOCTL(drm_agp_unbind_hook_ioctl, agph_unbind_ioctl)
