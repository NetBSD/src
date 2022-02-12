/*	$NetBSD: linux_sync_file.c,v 1.2 2022/02/12 15:51:29 thorpej Exp $	*/

/*-
 * Copyright (c) 2020 The NetBSD Foundation, Inc.
 * All rights reserved.
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
__KERNEL_RCSID(0, "$NetBSD: linux_sync_file.c,v 1.2 2022/02/12 15:51:29 thorpej Exp $");

#include <sys/event.h>
#include <sys/fcntl.h>
#include <sys/file.h>
#include <sys/filedesc.h>
#include <sys/kmem.h>
#include <sys/mutex.h>
#include <sys/poll.h>
#include <sys/select.h>
#include <sys/queue.h>

#include <linux/dma-fence.h>
#include <linux/sync_file.h>

static const struct fileops sync_file_ops;

struct sync_file *
sync_file_create(struct dma_fence *fence, struct file *fp)
{
	struct sync_file *sf;

	sf = kmem_zalloc(sizeof(*sf), KM_SLEEP);
	sf->file = fp;
	sf->sf_fence = dma_fence_get(fence);
	mutex_init(&sf->sf_lock, MUTEX_DEFAULT, IPL_VM);
	selinit(&sf->sf_selq);
	sf->sf_polling = false;
	sf->sf_signalled = false;

	fp->f_type = DTYPE_MISC;
	fp->f_flag = FREAD | FWRITE;
	fp->f_ops = &sync_file_ops;

	return sf;
}

static int
sync_file_close(struct file *fp)
{
	struct sync_file *sf = fp->f_data;

	if (sf->sf_polling)
		dma_fence_remove_callback(sf->sf_fence, &sf->sf_fcb);
	dma_fence_put(sf->sf_fence);
	sf->sf_fence = NULL;

	kmem_free(sf, sizeof(*sf));

	return 0;
}

static void
sync_file_fence_cb(struct dma_fence *fence, struct dma_fence_cb *fcb)
{
	struct sync_file *sf = container_of(fcb, struct sync_file, sf_fcb);

	mutex_enter(&sf->sf_lock);
	sf->sf_signalled = true;
	selnotify(&sf->sf_selq, POLLIN, NOTE_SUBMIT);
	mutex_exit(&sf->sf_lock);
}

static int
sync_file_poll(struct file *fp, int events)
{
	struct sync_file *sf = fp->f_data;
	int revents = 0;
	int ret;

	if ((events & POLLIN) == 0)
		return 0;

	mutex_enter(&sf->sf_lock);
	if (sf->sf_signalled) {
		revents |= POLLIN;
	} else if (sf->sf_polling) {
		selrecord(curlwp, &sf->sf_selq);
	} else {
		sf->sf_polling = true;
		mutex_exit(&sf->sf_lock);
		ret = dma_fence_add_callback(sf->sf_fence, &sf->sf_fcb,
		    sync_file_fence_cb);
		mutex_enter(&sf->sf_lock);
		if (ret < 0) {
			sf->sf_signalled = true;
			selnotify(&sf->sf_selq, POLLIN, NOTE_SUBMIT);
			revents |= POLLIN;
		} else {
			selrecord(curlwp, &sf->sf_selq);
		}
	}
	mutex_exit(&sf->sf_lock);

	return revents;
}

static const struct filterops sync_file_filtops;

static int
sync_file_kqfilter(struct file *fp, struct knote *kn)
{
	struct sync_file *sf = fp->f_data;

	switch (kn->kn_filter) {
	case EVFILT_READ:
		kn->kn_fop = &sync_file_filtops;
		kn->kn_hook = sf;
		mutex_enter(&sf->sf_lock);
		klist_insert(&sf->sf_selq.sel_klist, kn);
		mutex_exit(&sf->sf_lock);
		return 0;
	default:
		return EINVAL;
	}
}

static void
filt_sync_file_detach(struct knote *kn)
{
	struct sync_file *sf = kn->kn_hook;

	mutex_enter(&sf->sf_lock);
	klist_remove(&sf->sf_selq.sel_klist, kn);
	mutex_exit(&sf->sf_lock);
}

static int
filt_sync_file_event(struct knote *kn, long hint)
{
	struct sync_file *sf = kn->kn_hook;
	int ret;

	if (hint == NOTE_SUBMIT)
		KASSERT(mutex_owned(&sf->sf_lock));
	else
		mutex_enter(&sf->sf_lock);

	if (sf->sf_signalled) {
		kn->kn_data = 0; /* XXX Does this work??  */
		ret = 1;
	} else if (sf->sf_polling) {
		ret = 0;
	} else {
		sf->sf_polling = true;
		mutex_exit(&sf->sf_lock);
		ret = dma_fence_add_callback(sf->sf_fence, &sf->sf_fcb,
		    sync_file_fence_cb);
		mutex_enter(&sf->sf_lock);
		if (ret < 0) {
			sf->sf_signalled = true;
			selnotify(&sf->sf_selq, POLLIN, NOTE_SUBMIT);
			kn->kn_data = 0;
			ret = 1;
		} else {
			selrecord(curlwp, &sf->sf_selq);
			ret = 0;
		}
	}

	if (hint == NOTE_SUBMIT)
		KASSERT(mutex_owned(&sf->sf_lock));
	else
		mutex_exit(&sf->sf_lock);

	return ret;
}

static const struct filterops sync_file_filtops = {
	.f_flags = FILTEROP_ISFD,
	.f_attach = NULL,
	.f_detach = filt_sync_file_detach,
	.f_event = filt_sync_file_event,
};

struct dma_fence *
sync_file_get_fence(int fd)
{
	struct file *fp;
	struct sync_file *sf;
	struct dma_fence *fence;

	if ((fp = fd_getfile(fd)) == NULL)
		return NULL;
	sf = fp->f_data;
	fence = dma_fence_get(sf->sf_fence);
	fd_putfile(fd);

	return fence;
}

static const struct fileops sync_file_ops = {
	.fo_name = "linux_sync_file",
	.fo_read = fbadop_read,
	.fo_write = fbadop_write,
	.fo_ioctl = fbadop_ioctl,
	.fo_fcntl = fnullop_fcntl,
	.fo_poll = sync_file_poll,
	.fo_stat = fbadop_stat,	/* XXX */
	.fo_close = sync_file_close,
	.fo_kqfilter = sync_file_kqfilter,
	.fo_restart = fnullop_restart,
	.fo_mmap = NULL,
};
