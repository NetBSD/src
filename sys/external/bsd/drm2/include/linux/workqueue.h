/*	$NetBSD: workqueue.h,v 1.1.2.9 2013/09/08 15:58:24 riastradh Exp $	*/

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

#include <asm/bug.h>
#include <linux/kernel.h>

/*
 * XXX This implementation is a load of bollocks -- callouts are
 * expedient, but wrong, if for no reason other than that we never call
 * callout_destroy.
 */

struct work_struct {
	struct callout ws_callout;
};

struct delayed_work {
	struct work_struct work;
};

static inline void
INIT_WORK(struct work_struct *work, void (*fn)(struct work_struct *))
{

	callout_init(&work->ws_callout, 0);

	/* XXX This cast business is sketchy.  */
	callout_setfunc(&work->ws_callout, (void (*)(void *))fn, work);
}

static inline void
INIT_DELAYED_WORK(struct delayed_work *dw, void (*fn)(struct work_struct *))
{
	INIT_WORK(&dw->work, fn);
}

static inline struct delayed_work *
to_delayed_work(struct work_struct *work)
{
	return container_of(work, struct delayed_work, work);
}

static inline void
schedule_work(struct work_struct *work)
{
	callout_schedule(&work->ws_callout, 0);
}

static inline void
schedule_delayed_work(struct delayed_work *dw, unsigned long ticks)
{
	KASSERT(ticks < INT_MAX);
	callout_schedule(&dw->work.ws_callout, (int)ticks);
}

static inline bool
cancel_work(struct work_struct *work)
{
	return !callout_stop(&work->ws_callout);
}

static inline bool
cancel_work_sync(struct work_struct *work)
{
	return !callout_halt(&work->ws_callout, NULL);
}

static inline bool
cancel_delayed_work(struct delayed_work *dw)
{
	return cancel_work(&dw->work);
}

static inline bool
cancel_delayed_work_sync(struct delayed_work *dw)
{
	return cancel_work_sync(&dw->work);
}

/*
 * XXX Bogus stubs for Linux work queues.
 */

struct workqueue_struct;

static inline struct workqueue_struct *
alloc_ordered_workqueue(const char *name __unused, int flags __unused)
{
	return (void *)(uintptr_t)0xdeadbeef;
}

static inline void
destroy_workqueue(struct workqueue_struct *wq __unused)
{
	KASSERT(wq == (void *)(uintptr_t)0xdeadbeef);
}

#define	flush_workqueue(wq)		WARN(true, "Can't flush workqueues!");
#define	flush_scheduled_work(wq)	WARN(true, "Can't flush workqueues!");

static inline void
queue_work(struct workqueue_struct *wq __unused, struct work_struct *work)
{
	KASSERT(wq == (void *)(uintptr_t)0xdeadbeef);
	schedule_work(work);
}

static inline void
queue_delayed_work(struct workqueue_struct *wq __unused,
    struct delayed_work *dw,
    unsigned int ticks)
{
	KASSERT(wq == (void *)(uintptr_t)0xdeadbeef);
	schedule_delayed_work(dw, ticks);
}

#endif  /* _LINUX_WORKQUEUE_H_ */
