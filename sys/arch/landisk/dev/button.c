/*	$NetBSD: button.c,v 1.1.12.4 2008/01/21 09:37:20 yamt Exp $	*/

/*
 * Copyright (c) 2003 Wasabi Systems, Inc.
 * All rights reserved.
 *
 * Written by Jason R. Thorpe for Wasabi Systems, Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed for the NetBSD Project by
 *	Wasabi Systems, Inc.
 * 4. The name of Wasabi Systems, Inc. may not be used to endorse
 *    or promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY WASABI SYSTEMS, INC. ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL WASABI SYSTEMS, INC
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: button.c,v 1.1.12.4 2008/01/21 09:37:20 yamt Exp $");

#include <sys/param.h>
#include <sys/conf.h>
#include <sys/systm.h>
#include <sys/malloc.h>
#include <sys/simplelock.h>
#include <sys/queue.h>
#include <sys/proc.h>
#include <sys/kthread.h>
#include <sys/errno.h>
#include <sys/fcntl.h>
#include <sys/callout.h>
#include <sys/kernel.h>
#include <sys/poll.h>
#include <sys/select.h>
#include <sys/vnode.h>

#include <machine/button.h>

#include <landisk/dev/buttonvar.h>

/*
 * event handler
 */
static LIST_HEAD(, btn_event) btn_event_list =
    LIST_HEAD_INITIALIZER(btn_event_list);
static struct simplelock btn_event_list_slock =
    SIMPLELOCK_INITIALIZER;

static struct lwp *btn_daemon;

#define	BTN_MAX_EVENTS		32

static struct simplelock btn_event_queue_slock =
    SIMPLELOCK_INITIALIZER;
static button_event_t btn_event_queue[BTN_MAX_EVENTS];
static int btn_event_queue_head;
static int btn_event_queue_tail;
static int btn_event_queue_count;
static int btn_event_queue_flags;
static struct selinfo btn_event_queue_selinfo;

static char btn_type[32];

#define	BEVQ_F_WAITING		0x01	/* daemon waiting for event */

#define	BTN_NEXT_EVENT(x)	(((x) + 1) / BTN_MAX_EVENTS)

dev_type_open(btnopen);
dev_type_close(btnclose);
dev_type_ioctl(btnioctl);
dev_type_read(btnread);
dev_type_poll(btnpoll);
dev_type_kqfilter(btnkqfilter);

const struct cdevsw button_cdevsw = {
	btnopen, btnclose, btnread, nowrite, btnioctl,
	nostop, notty, btnpoll, nommap, btnkqfilter,
};

static int
btn_queue_event(button_event_t *bev)
{

	if (btn_event_queue_count == BTN_MAX_EVENTS)
		return (0);

	btn_event_queue[btn_event_queue_head] = *bev;
	btn_event_queue_head = BTN_NEXT_EVENT(btn_event_queue_head);
	btn_event_queue_count++;

	return (1);
}

static int
btn_get_event(button_event_t *bev)
{

	if (btn_event_queue_count == 0)
		return (0);

	*bev = btn_event_queue[btn_event_queue_tail];
	btn_event_queue_tail = BTN_NEXT_EVENT(btn_event_queue_tail);
	btn_event_queue_count--;

	return (1);
}

static void
btn_event_queue_flush(void)
{

	btn_event_queue_head = 0;
	btn_event_queue_tail = 0;
	btn_event_queue_count = 0;
	btn_event_queue_flags = 0;
}

int
btnopen(dev_t dev, int flag, int mode, struct lwp *l)
{
	int error;

	if (minor(dev) != 0) {
		return (ENODEV);
	}

	simple_lock(&btn_event_queue_slock);
	if (btn_daemon != NULL) {
		error = EBUSY;
	} else {
		error = 0;
		btn_daemon = l;
		btn_event_queue_flush();
	}
	simple_unlock(&btn_event_queue_slock);

	return (error);
}

int
btnclose(dev_t dev, int flag, int mode, struct lwp *l)
{
	int count;

	if (minor(dev) != 0) {
		return (ENODEV);
	}

	simple_lock(&btn_event_queue_slock);
	count = btn_event_queue_count;
	btn_daemon = NULL;
	btn_event_queue_flush();
	simple_unlock(&btn_event_queue_slock);

	if (count) {
		printf("WARNING: %d events lost by exiting daemon\n", count);
	}

	return (0);
}

int
btnioctl(dev_t dev, u_long cmd, void *data, int flag, struct lwp *l)
{
	int error = 0;

	if (minor(dev) != 0) {
		return (ENODEV);
	}

	switch (cmd) {
	case BUTTON_IOC_GET_TYPE:
	    {
		struct button_type *button_type = (void *)data;
		strcpy(button_type->button_type, btn_type);
		break;
	    }

	default:
		error = ENOTTY;
		break;
	}

	return (error);
}

int
btnread(dev_t dev, struct uio *uio, int flags)
{
	button_event_t bev;
	int error;

	if (minor(dev) != 0) {
		return (ENODEV);
	}

	if (uio->uio_resid != BUTTON_EVENT_MSG_SIZE) {
		return (EINVAL);
	}

	simple_lock(&btn_event_queue_slock);
	for (;;) {
		if (btn_get_event(&bev)) {
			simple_unlock(&btn_event_queue_slock);
			return (uiomove(&bev, BUTTON_EVENT_MSG_SIZE, uio));
		}

		if (flags & IO_NDELAY) {
			simple_unlock(&btn_event_queue_slock);
			return (EWOULDBLOCK);
		}

		btn_event_queue_flags |= BEVQ_F_WAITING;
		error = ltsleep(&btn_event_queue_count,
		    (PRIBIO|PCATCH), "btnread", 0, &btn_event_queue_slock);
		if (error) {
			simple_unlock(&btn_event_queue_slock);
			return (error);
		}
	}
}

int
btnpoll(dev_t dev, int events, struct lwp *l)
{
	int revents;

	if (minor(dev) != 0) {
		return (ENODEV);
	}

	revents = events & (POLLOUT | POLLWRNORM);

	/* Attempt to save some work. */
	if ((events & (POLLIN | POLLRDNORM)) == 0)
		return (revents);

	simple_lock(&btn_event_queue_slock);
	if (btn_event_queue_count) {
		revents |= events & (POLLIN | POLLRDNORM);
	} else {
		selrecord(l, &btn_event_queue_selinfo);
	}
	simple_unlock(&btn_event_queue_slock);

	return (revents);
}

static void
filt_btn_rdetach(struct knote *kn)
{

	simple_lock(&btn_event_queue_slock);
	SLIST_REMOVE(&btn_event_queue_selinfo.sel_klist,
	    kn, knote, kn_selnext);
	simple_unlock(&btn_event_queue_slock);
}

static int
filt_btn_read(struct knote *kn, long hint)
{

	simple_lock(&btn_event_queue_slock);
	kn->kn_data = btn_event_queue_count;
	simple_unlock(&btn_event_queue_slock);

	return (kn->kn_data > 0);
}

static const struct filterops btn_read_filtops =
    { 1, NULL, filt_btn_rdetach, filt_btn_read };

static const struct filterops btn_write_filtops =
    { 1, NULL, filt_btn_rdetach, filt_seltrue };

int
btnkqfilter(dev_t dev, struct knote *kn)
{
	struct klist *klist;

	if (minor(dev) != 0) {
		return (ENODEV);
	}

	switch (kn->kn_filter) {
	case EVFILT_READ:
		klist = &btn_event_queue_selinfo.sel_klist;
		kn->kn_fop = &btn_read_filtops;
		break;

	case EVFILT_WRITE:
		klist = &btn_event_queue_selinfo.sel_klist;
		kn->kn_fop = &btn_write_filtops;
		break;

	default:
		return (1);
	}

	simple_lock(&btn_event_queue_slock);
	SLIST_INSERT_HEAD(klist, kn, kn_selnext);
	simple_unlock(&btn_event_queue_slock);

	return (0);
}

void
btn_settype(const char *type)
{

	/*
	 * Don't bother locking this; it's going to be set
	 * during autoconfiguration, and then only read from
	 * then on.
	 */
	strlcpy(btn_type, type, sizeof(btn_type));
}

int
btn_event_register(struct btn_event *bev)
{

	simple_lock(&btn_event_list_slock);
	LIST_INSERT_HEAD(&btn_event_list, bev, bev_list);
	simple_unlock(&btn_event_list_slock);

	return (0);
}

void
btn_event_unregister(struct btn_event *bev)
{

	simple_lock(&btn_event_list_slock);
	LIST_REMOVE(bev, bev_list);
	simple_unlock(&btn_event_list_slock);
}

void
btn_event_send(struct btn_event *bev, int event)
{
	button_event_t btnev;
	int rv;

	simple_lock(&btn_event_queue_slock);
	if (btn_daemon != NULL) {
		btnev.bev_type = BUTTON_EVENT_STATE_CHANGE;
		btnev.bev_event.bs_state = event;
		strcpy(btnev.bev_event.bs_name, bev->bev_name);

		rv = btn_queue_event(&btnev);
		if (rv == 0) {
			simple_unlock(&btn_event_queue_slock);
			printf("%s: WARNING: state change event %d lost; "
			    "queue full\n", bev->bev_name, btnev.bev_type);
		} else {
			if (btn_event_queue_flags & BEVQ_F_WAITING) {
				btn_event_queue_flags &= ~BEVQ_F_WAITING;
				simple_unlock(&btn_event_queue_slock);
				wakeup(&btn_event_queue_count);
			} else {
				simple_unlock(&btn_event_queue_slock);
			}
			selnotify(&btn_event_queue_selinfo, 0);
		}
		return;
	}
	simple_unlock(&btn_event_queue_slock);

	printf("%s: btn_event_send can't handle me.\n", bev->bev_name);
}
