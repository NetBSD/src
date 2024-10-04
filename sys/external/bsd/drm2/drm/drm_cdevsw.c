/*	$NetBSD: drm_cdevsw.c,v 1.30.4.1 2024/10/04 11:40:51 martin Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: drm_cdevsw.c,v 1.30.4.1 2024/10/04 11:40:51 martin Exp $");

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

#include <drm/drm_agpsupport.h>
#include <drm/drm_device.h>
#include <drm/drm_drv.h>
#include <drm/drm_file.h>
#include <drm/drm_irq.h>
#include <drm/drm_legacy.h>

#include "../dist/drm/drm_internal.h"
#include "../dist/drm/drm_legacy.h"

static dev_type_open(drm_open);

static int	drm_close(struct file *);
static int	drm_read(struct file *, off_t *, struct uio *, kauth_cred_t,
		    int);
static int	drm_dequeue_event(struct drm_file *, size_t,
		    struct drm_pending_event **, int);
static int	drm_ioctl_shim(struct file *, unsigned long, void *);
static int	drm_poll(struct file *, int);
static int	drm_kqfilter(struct file *, struct knote *);
static int	drm_stat(struct file *, struct stat *);
static int	drm_fop_mmap(struct file *, off_t *, size_t, int, int *, int *,
			     struct uvm_object **, int *);
static void	drm_requeue_event(struct drm_file *, struct drm_pending_event *);

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

const struct fileops drm_fileops = {
	.fo_name = "drm",
	.fo_read = drm_read,
	.fo_write = fbadop_write,
	.fo_ioctl = drm_ioctl_shim,
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
	bool lastclose;
	int fd;
	struct file *fp;
	struct drm_file *priv;
	int need_setup = 0;
	int error;

	error = drm_guarantee_initialized();
	if (error)
		goto fail0;

	/* Synchronize with drm_file.c, drm_open and drm_open_helper.  */

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
	if (dev->open_count++ == 0)
		need_setup = 1;
	mutex_unlock(&drm_global_mutex);

	error = fd_allocfile(&fp, &fd);
	if (error)
		goto fail2;

	priv = drm_file_alloc(dminor);
	if (IS_ERR(priv)) {
		/* XXX errno Linux->NetBSD */
		error = -PTR_ERR(priv);
		goto fail3;
	}

	if (drm_is_primary_client(priv)) {
		/* XXX errno Linux->NetBSD */
		error = -drm_master_open(priv);
		if (error)
			goto fail4;
	}
	priv->filp = fp;

	mutex_lock(&dev->filelist_mutex);
	list_add(&priv->lhead, &dev->filelist);
	mutex_unlock(&dev->filelist_mutex);
	/* XXX Alpha hose?  */

	if (need_setup) {
		/* XXX errno Linux->NetBSD */
		error = -drm_legacy_setup(dev);
		if (error)
			goto fail5;
	}

	error = fd_clone(fp, fd, flags, &drm_fileops, priv);
	KASSERT(error == EMOVEFD); /* XXX */

	/* Success!  (But error has to be EMOVEFD, not 0.)  */
	return error;

fail5:	mutex_lock(&dev->filelist_mutex);
	list_del(&priv->lhead);
	mutex_unlock(&dev->filelist_mutex);
fail4:	drm_file_free(priv);
fail3:	fd_abort(curproc, fp, fd);
fail2:	mutex_lock(&drm_global_mutex);
	KASSERT(0 < dev->open_count);
	--dev->open_count;
	lastclose = (dev->open_count == 0);
	mutex_unlock(&drm_global_mutex);
	if (lastclose)
		drm_lastclose(dev);
fail1:	drm_minor_release(dminor);
fail0:	KASSERT(error);
	if (error == ERESTARTSYS)
		error = ERESTART;
	return error;
}

static int
drm_close(struct file *fp)
{
	struct drm_file *const priv = fp->f_data;
	struct drm_minor *const dminor = priv->minor;
	struct drm_device *const dev = dminor->dev;
	bool lastclose;

	/* Synchronize with drm_file.c, drm_release.  */

	mutex_lock(&dev->filelist_mutex);
	list_del(&priv->lhead);
	mutex_unlock(&dev->filelist_mutex);

	drm_file_free(priv);

	mutex_lock(&drm_global_mutex);
	KASSERT(0 < dev->open_count);
	--dev->open_count;
	lastclose = (dev->open_count == 0);
	mutex_unlock(&drm_global_mutex);

	if (lastclose)
		drm_lastclose(dev);

	drm_minor_release(dminor);

	return 0;
}

static int
drm_read(struct file *fp, off_t *off, struct uio *uio, kauth_cred_t cred,
    int flags)
{
	struct drm_file *const file = fp->f_data;
	struct drm_device *const dev = file->minor->dev;
	struct drm_pending_event *event;
	bool first;
	int ret = 0;

	/*
	 * Only one event reader at a time, so that if copyout faults
	 * after dequeueing one event and we have to put the event
	 * back, another reader won't see out-of-order events.
	 */
	spin_lock(&dev->event_lock);
	DRM_SPIN_WAIT_NOINTR_UNTIL(ret, &file->event_read_wq, &dev->event_lock,
	    file->event_read_lock == NULL);
	if (ret) {
		spin_unlock(&dev->event_lock);
		/* XXX errno Linux->NetBSD */
		return -ret;
	}
	file->event_read_lock = curlwp;
	spin_unlock(&dev->event_lock);

	for (first = true; ; first = false) {
		int f = 0;
		off_t offset;
		size_t resid;

		if (!first || ISSET(fp->f_flag, FNONBLOCK))
			f |= FNONBLOCK;

		ret = drm_dequeue_event(file, uio->uio_resid, &event, f);
		if (ret) {
			if ((ret == -EWOULDBLOCK) && !first)
				ret = 0;
			break;
		}
		if (event == NULL)
			break;

		offset = uio->uio_offset;
		resid = uio->uio_resid;
		/* XXX errno NetBSD->Linux */
		ret = -uiomove(event->event, event->event->length, uio);
		if (ret) {
			/*
			 * Faulted on copyout.  Put the event back and
			 * stop here.
			 */
			if (!first) {
				/*
				 * Already transferred some events.
				 * Rather than back them all out, just
				 * say we succeeded at returning those.
				 */
				ret = 0;
			}
			uio->uio_offset = offset;
			uio->uio_resid = resid;
			drm_requeue_event(file, event);
			break;
		}
		kfree(event);
	}

	/* Release the event read lock.  */
	spin_lock(&dev->event_lock);
	KASSERT(file->event_read_lock == curlwp);
	file->event_read_lock = NULL;
	DRM_SPIN_WAKEUP_ONE(&file->event_read_wq, &dev->event_lock);
	spin_unlock(&dev->event_lock);

	/* XXX errno Linux->NetBSD */

	/* Success!  */
	if (ret == ERESTARTSYS)
		ret = ERESTART;
	return -ret;
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

static void
drm_requeue_event(struct drm_file *file, struct drm_pending_event *event)
{
	struct drm_device *const dev = file->minor->dev;
	unsigned long irqflags;

	spin_lock_irqsave(&dev->event_lock, irqflags);
	list_add(&event->link, &file->event_list);
	KASSERT(file->event_space >= event->event->length);
	file->event_space -= event->event->length;
	spin_unlock_irqrestore(&dev->event_lock, irqflags);
}

static int
drm_ioctl_shim(struct file *fp, unsigned long cmd, void *data)
{
	struct drm_file *file = fp->f_data;
	struct drm_driver *driver = file->minor->dev->driver;
	int error;

	if (driver->ioctl_override)
		error = driver->ioctl_override(fp, cmd, data);
	else
		error = drm_ioctl(fp, cmd, data);
	if (error == ERESTARTSYS)
		error = ERESTART;

	return error;
}

static int
drm_poll(struct file *fp, int events)
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
	.f_flags = FILTEROP_ISFD,
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
		selrecord_knote(&file->event_selq, kn);
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
	selremove_knote(&file->event_selq, kn);
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
	    dminor->index);

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
	KASSERT(len > 0);

	/* XXX errno Linux->NetBSD */
	error = -(*dev->driver->mmap_object)(dev, *offp, len, prot, uobjp,
	    offp, file->filp);
	*maxprotp = prot;
	*advicep = UVM_ADV_RANDOM;
	if (error == ERESTARTSYS)
		error = ERESTART;
	return error;
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
