/*	$NetBSD: drm_cdevsw.c,v 1.9 2018/08/27 07:54:07 riastradh Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: drm_cdevsw.c,v 1.9 2018/08/27 07:54:07 riastradh Exp $");

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
#include <drm/drm_internal.h>
#include "../dist/drm/drm_legacy.h"

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
static int	drm_fop_mmap(struct file *, off_t *, size_t, int, int *, int *,
			     struct uvm_object **, int *);
static paddr_t	drm_legacy_mmap(dev_t, off_t, int);

const struct cdevsw drm_cdevsw = {
	.d_open = drm_open,
	.d_close = noclose,
	.d_read = noread,
	.d_write = nowrite,
	.d_ioctl = noioctl,
	.d_stop = nostop,
	.d_tty = notty,
	.d_poll = nopoll,
	.d_mmap = drm_legacy_mmap,
	.d_kqfilter = nokqfilter,
	.d_discard = nodiscard,
	/* XXX was D_TTY | D_NEGOFFSAFE */
	/* XXX Add D_MPSAFE some day... */
	.d_flag = D_NEGOFFSAFE,
};

static const struct fileops drm_fileops = {
	.fo_name = "drm",
	.fo_read = drm_read,
	.fo_write = fbadop_write,
	.fo_ioctl = drm_ioctl,
	.fo_fcntl = fnullop_fcntl,
	.fo_poll = drm_poll,
	.fo_stat = drm_stat,
	.fo_close = drm_close,
	.fo_kqfilter = drm_kqfilter,
	.fo_restart = fnullop_restart,
	.fo_mmap = drm_fop_mmap,
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

	mutex_lock(&drm_global_mutex);
	if (dev->open_count == INT_MAX) {
		mutex_unlock(&drm_global_mutex);
		error = EBUSY;
		goto fail1;
	}
	firstopen = (dev->open_count == 0);
	dev->open_count++;
	mutex_lock(&drm_global_mutex);

	if (firstopen) {
		/* XXX errno Linux->NetBSD */
		error = -drm_firstopen(dev);
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
fail2:	mutex_lock(&drm_global_mutex);
	KASSERT(0 < dev->open_count);
	--dev->open_count;
	lastclose = (dev->open_count == 0);
	mutex_unlock(&drm_global_mutex);
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

	mutex_lock(&drm_global_mutex);
	KASSERT(0 < dev->open_count);
	--dev->open_count;
	lastclose = (dev->open_count == 0);
	mutex_unlock(&drm_global_mutex);

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

	/* XXX Order is sketchy here...  */
	if (dev->driver->lastclose)
		(*dev->driver->lastclose)(dev);
	if (dev->irq_enabled && !drm_core_check_feature(dev, DRIVER_MODESET))
		drm_irq_uninstall(dev);

	mutex_lock(&dev->struct_mutex);
	if (dev->agp)
		drm_agp_clear_hook(dev);
	drm_legacy_sg_cleanup(dev);
	drm_legacy_dma_takedown(dev);
	mutex_unlock(&dev->struct_mutex);

	/* XXX Synchronize with drm_legacy_dev_reinit.  */
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
		/* Event is too large, can't return it.  */
		event = NULL;
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

static const struct filterops drm_filtops = {
	.f_isfd = 1,
	.f_attach = NULL,
	.f_detach = filt_drm_detach,
	.f_event = filt_drm_event,
};

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
	    64*dminor->type + dminor->index);

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
drm_fop_mmap(struct file *fp, off_t *offp, size_t len, int prot, int *flagsp,
	     int *advicep, struct uvm_object **uobjp, int *maxprotp)
{
	struct drm_file *const file = fp->f_data;
	struct drm_device *const dev = file->minor->dev;
	int error;

	KASSERT(fp == file->filp);
	error = (*dev->driver->mmap_object)(dev, *offp, len, prot, uobjp,
	    offp, file->filp);
	*maxprotp = prot;
	*advicep = UVM_ADV_RANDOM;
	return -error;
}

static paddr_t
drm_legacy_mmap(dev_t d, off_t offset, int prot)
{
	struct drm_minor *dminor;
	paddr_t paddr;

	dminor = drm_minor_acquire(minor(d));
	if (IS_ERR(dminor))
		return (paddr_t)-1;

	paddr = drm_legacy_mmap_paddr(dminor->dev, offset, prot);

	drm_minor_release(dminor);
	return paddr;
}
