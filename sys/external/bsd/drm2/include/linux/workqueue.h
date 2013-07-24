/*	$NetBSD: workqueue.h,v 1.1.2.2 2013/07/24 01:52:24 riastradh Exp $	*/

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

#ifndef _LINUX_WORKQUEUE_H_
#define _LINUX_WORKQUEUE_H_

#include <sys/callout.h>

#include <linux/kernel.h>

struct work_struct {
	struct callout ws_callout;
};

struct delayed_work {
	struct work_struct dw_work;
};

static inline void
INIT_DELAYED_WORK(struct delayed_work *dw, void (*fn)(struct delayed_work *))
{

	/* XXX This cast business is sketchy.  */
	callout_setfunc(&dw->dw_work.ws_callout, (void (*)(void *))fn,
	    &dw->dw_work);
}

static inline struct delayed_work *
to_delayed_work(struct work_struct *work)
{
	return container_of(work, struct delayed_work, dw_work);
}

static inline void
schedule_delayed_work(struct delayed_work *dw, unsigned long ticks)
{
	KASSERT(ticks < INT_MAX);
	callout_schedule(&dw->dw_work.ws_callout, (int)ticks);
}

static inline void
cancel_delayed_work_sync(struct delayed_work *dw)
{
	callout_stop(&dw->dw_work.ws_callout);
}

#endif  /* _LINUX_WORKQUEUE_H_ */
