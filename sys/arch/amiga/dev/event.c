/*	$NetBSD: event.c,v 1.17 2021/09/26 16:36:18 thorpej Exp $ */

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
 *
 * Header: event.c,v 1.5 92/11/26 01:10:44 torek Exp  (LBL)
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: event.c,v 1.17 2021/09/26 16:36:18 thorpej Exp $");

/*
 * Internal `Firm_event' interface for the keyboard and mouse drivers.
 */

#include <sys/param.h>
#include <sys/fcntl.h>
#include <sys/malloc.h>
#include <sys/poll.h>
#include <sys/proc.h>
#include <sys/select.h>
#include <sys/systm.h>
#include <sys/vnode.h>

#include <amiga/dev/vuid_event.h>
#include <amiga/dev/event_var.h>

/*
 * Initialize a firm_event queue.
 */
void
ev_init(register struct evvar *ev)
{

	ev->ev_get = ev->ev_put = 0;
	ev->ev_q = malloc((u_long)EV_QSIZE * sizeof(struct firm_event),
	    M_DEVBUF, M_WAITOK|M_ZERO);
	selinit(&ev->ev_sel);
}

/*
 * Tear down a firm_event queue.
 */
void
ev_fini(register struct evvar *ev)
{

	seldestroy(&ev->ev_sel);
	free(ev->ev_q, M_DEVBUF);
}

/*
 * User-level interface: read, poll.
 * (User cannot write an event queue.)
 */
int
ev_read(register struct evvar *ev, struct uio *uio, int flags)
{
	int s, n, cnt, error;

	/*
	 * Make sure we can return at least 1.
	 */
	if (uio->uio_resid < sizeof(struct firm_event))
		return (EMSGSIZE);	/* ??? */
	s = splev();
	while (ev->ev_get == ev->ev_put) {
		if (flags & IO_NDELAY) {
			splx(s);
			return (EWOULDBLOCK);
		}
		ev->ev_wanted = 1;
		error = tsleep((void *)ev, PEVENT | PCATCH, "firm_event", 0);
		if (error) {
			splx(s);
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
	splx(s);
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
	    n == 0 || error || (cnt = ev->ev_put) == 0)
		return (error);
	if (cnt > n)
		cnt = n;
	error = uiomove((void *)&ev->ev_q[0],
	    cnt * sizeof(struct firm_event), uio);
	ev->ev_get = cnt;
	return (error);
}

int
ev_poll(register struct evvar *ev, int events, struct lwp *l)
{
	int s = splev();
	int revents = 0;

	if (events & (POLLIN | POLLRDNORM))
		/* succeed if there is something to read */
		if (ev->ev_get != ev->ev_put)
			revents |= events & (POLLIN | POLLRDNORM);
	if (revents == 0)
		if (events & (POLLIN | POLLRDNORM))
			selrecord(l, &ev->ev_sel);
	splx(s);
	return (revents);
}

static void
filt_evrdetach(struct knote *kn)
{
	struct evvar *ev = kn->kn_hook;
	int s;

	s = splev();
	selremove_knote(&ev->ev_sel, kn);
	splx(s);
}

static int
filt_evread(struct knote *kn, long hint)
{
	struct evvar *ev = kn->kn_hook;

	if (ev->ev_get == ev->ev_put)
		return (0);

	if (ev->ev_get < ev->ev_put)
		kn->kn_data = ev->ev_put - ev->ev_get;
	else
		kn->kn_data = (EV_QSIZE - ev->ev_get) +
		    ev->ev_put;

	kn->kn_data *= sizeof(struct firm_event);

	return (1);
}

static const struct filterops ev_filtops = {
	.f_flags = FILTEROP_ISFD,
	.f_attach = NULL,
	.f_detach = filt_evrdetach,
	.f_event = filt_evread,
};

int
ev_kqfilter(struct evvar *ev, struct knote *kn)
{
	int s;

	switch (kn->kn_filter) {
	case EVFILT_READ:
		kn->kn_fop = &ev_filtops;
		break;

	default:
		return (EINVAL);
	}

	kn->kn_hook = ev;

	s = splev();
	selrecord_knote(&ev->ev_sel, kn);
	splx(s);

	return (0);
}
