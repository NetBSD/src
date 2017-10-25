/*	$NetBSD: event.c,v 1.15 2017/10/25 08:12:38 maya Exp $ */

/*
 * Copyright (c) 1992, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This software was developed by the Computer Systems Engineering group
 * at Lawrence Berkeley Laboratory under DARPA contract BG 91-66 and
 * contributed to Berkeley.
 *
 * All advertising materials mentioning features or use of this software
 * must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Lawrence Berkeley Laboratory.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	@(#)event.c	8.1 (Berkeley) 6/11/93
 */

/*
 * Internal `Firm_event' interface for the keyboard and mouse drivers.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: event.c,v 1.15 2017/10/25 08:12:38 maya Exp $");

#include <sys/param.h>
#include <sys/fcntl.h>
#include <sys/kmem.h>
#include <sys/proc.h>
#include <sys/systm.h>
#include <sys/vnode.h>
#include <sys/select.h>
#include <sys/poll.h>
#include <sys/mutex.h>
#include <sys/condvar.h>

#include <machine/vuid_event.h>
#include <x68k/dev/event_var.h>

/*
 * Initialize a firm_event queue.
 */
void
ev_init(struct evvar *ev, const char *name, kmutex_t *mtx)
{

	ev->ev_get = ev->ev_put = 0;
	ev->ev_q = kmem_zalloc((size_t)EV_QSIZE * sizeof(struct firm_event),
	    KM_SLEEP);
	selinit(&ev->ev_sel);
	ev->ev_lock = mtx;
	cv_init(&ev->ev_cv, name);
}

/*
 * Tear down a firm_event queue.
 */
void
ev_fini(struct evvar *ev)
{

	cv_destroy(&ev->ev_cv);
	seldestroy(&ev->ev_sel);
	kmem_free(ev->ev_q, (size_t)EV_QSIZE * sizeof(struct firm_event));
}

/*
 * User-level interface: read, select.
 * (User cannot write an event queue.)
 */
int
ev_read(struct evvar *ev, struct uio *uio, int flags)
{
	int n, cnt, put, error;

	/*
	 * Make sure we can return at least 1.
	 */
	if (uio->uio_resid < sizeof(struct firm_event))
		return (EMSGSIZE);	/* ??? */
	mutex_enter(ev->ev_lock);
	while (ev->ev_get == ev->ev_put) {
		if (flags & IO_NDELAY) {
			mutex_exit(ev->ev_lock);
			return (EWOULDBLOCK);
		}
		ev->ev_wanted = true;
		error = cv_wait_sig(&ev->ev_cv, ev->ev_lock);
		if (error != 0) {
			mutex_exit(ev->ev_lock);
			return (error);
		}
	}
	/*
	 * Move firm_events from tail end of queue (there is at least one
	 * there).
	 */
	if (ev->ev_put < ev->ev_get)
		cnt = EV_QSIZE - ev->ev_get;	/* events in [get..QSIZE) */
	else
		cnt = ev->ev_put - ev->ev_get;	/* events in [get..put) */
	put = ev->ev_put;
	mutex_exit(ev->ev_lock);
	n = howmany(uio->uio_resid, sizeof(struct firm_event));
	if (cnt > n)
		cnt = n;
	error = uiomove((void *)&ev->ev_q[ev->ev_get],
	    cnt * sizeof(struct firm_event), uio);
	n -= cnt;
	/*
	 * If we do not wrap to 0, used up all our space, or had an error,
	 * stop.  Otherwise move from front of queue to put index, if there
	 * is anything there to move.
	 */
	if ((ev->ev_get = (ev->ev_get + cnt) % EV_QSIZE) != 0 ||
	    n == 0 || error || (cnt = put) == 0)
		return (error);
	if (cnt > n)
		cnt = n;
	error = uiomove((void *)&ev->ev_q[0],
	    cnt * sizeof(struct firm_event), uio);
	ev->ev_get = cnt;
	return (error);
}

int
ev_poll(struct evvar *ev, int events, struct lwp *l)
{
	int revents = 0;

	mutex_enter(ev->ev_lock);
	if (events & (POLLIN | POLLRDNORM)) {
		if (ev->ev_get == ev->ev_put)
			selrecord(l, &ev->ev_sel);
		else
			revents |= events & (POLLIN | POLLRDNORM);
	}
	revents |= events & (POLLOUT | POLLWRNORM);
	mutex_exit(ev->ev_lock);
	return (revents);
}

void
ev_wakeup(struct evvar *ev)
{

	mutex_enter(ev->ev_lock);
	selnotify(&ev->ev_sel, 0, 0);
	if (ev->ev_wanted) {
		ev->ev_wanted = false;
		cv_signal(&ev->ev_cv);
	}
	mutex_exit(ev->ev_lock);

	if (ev->ev_async) {
		mutex_enter(proc_lock);
		psignal(ev->ev_io, SIGIO);
		mutex_exit(proc_lock);
	}
}

static void
filt_evrdetach(struct knote *kn)
{
	struct evvar *ev = kn->kn_hook;

	mutex_enter(ev->ev_lock);
	SLIST_REMOVE(&ev->ev_sel.sel_klist, kn, knote, kn_selnext);
	mutex_exit(ev->ev_lock);
}

static int
filt_evread(struct knote *kn, long hint)
{
	struct evvar *ev = kn->kn_hook;
	int rv = 1;

	mutex_enter(ev->ev_lock);
	if (ev->ev_get == ev->ev_put) {
		rv = 0;
	} else {
		if (ev->ev_get < ev->ev_put)
			kn->kn_data = ev->ev_put - ev->ev_get;
		else
			kn->kn_data = (EV_QSIZE - ev->ev_get) +
			    ev->ev_put;

		kn->kn_data *= sizeof(struct firm_event);
	}
	mutex_exit(ev->ev_lock);

	return rv;
}

static const struct filterops ev_filtops = {
	.f_isfd = 1,
	.f_attach = NULL,
	.f_detach = filt_evrdetach,
	.f_event = filt_evread,
};

int
ev_kqfilter(struct evvar *ev, struct knote *kn)
{
	struct klist *klist;

	switch (kn->kn_filter) {
	case EVFILT_READ:
		klist = &ev->ev_sel.sel_klist;
		kn->kn_fop = &ev_filtops;
		break;

	default:
		return (1);
	}

	kn->kn_hook = ev;

	mutex_enter(ev->ev_lock);
	SLIST_INSERT_HEAD(klist, kn, kn_selnext);
	mutex_exit(ev->ev_lock);

	return (0);
}
