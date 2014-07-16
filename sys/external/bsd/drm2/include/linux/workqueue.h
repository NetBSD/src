/*	$NetBSD: workqueue.h,v 1.4 2014/07/16 20:59:58 riastradh Exp $	*/

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

#include <sys/types.h>
#include <sys/callout.h>
#include <sys/queue.h>
#include <sys/workqueue.h>

#include <linux/kernel.h>

#define	INIT_DELAYED_WORK		linux_INIT_DELAYED_WORK
#define	INIT_WORK			linux_INIT_WORK
#define	alloc_ordered_workqueue		linux_alloc_ordered_workqueue
#define	cancel_delayed_work		linux_cancel_delayed_work
#define	cancel_delayed_work_sync	linux_cancel_delayed_work_sync
#define	cancel_work			linux_cancel_work
#define	cancel_work_sync		linux_cancel_work_sync
#define	destroy_workqueue		linux_destroy_workqueue
#define	flush_work			linux_flush_work
#define	flush_workqueue			linux_flush_workqueue
#define	queue_delayed_work		linux_queue_delayed_work
#define	mod_delayed_work		linux_mod_delayed_work
#define	queue_work			linux_queue_work
#define	schedule_delayed_work		linux_schedule_delayed_work
#define	schedule_work			linux_schedule_work
#define	system_wq			linux_system_wq
#define	to_delayed_work			linux_to_delayed_work

struct workqueue_struct;

struct work_struct {
	struct work		w_wk;
	__cpu_simple_lock_t	w_lock; /* XXX */
	enum {
		WORK_IDLE,
		WORK_DELAYED,
		WORK_PENDING,
		WORK_INVOKED,
		WORK_CANCELLED,
		WORK_DELAYED_CANCELLED,
	}			w_state;
	struct workqueue_struct	*w_wq;
	void			(*w_fn)(struct work_struct *);
};

struct delayed_work {
	/* Not dw_work; name must match Linux.  */
	struct work_struct		work;
	struct callout			dw_callout;
	TAILQ_ENTRY(delayed_work)	dw_entry;
};

static inline struct delayed_work *
to_delayed_work(struct work_struct *work)
{
	return container_of(work, struct delayed_work, work);
}

extern struct workqueue_struct	*system_wq;

int	linux_workqueue_init(void);
void	linux_workqueue_fini(void);

#define	create_singlethread_workqueue(name)				      \
	alloc_ordered_workqueue((name), 0)

struct workqueue_struct *
	alloc_ordered_workqueue(const char *, int);
void	destroy_workqueue(struct workqueue_struct *);
void	flush_workqueue(struct workqueue_struct *);
void	flush_scheduled_work(void);

void	INIT_WORK(struct work_struct *, void (*)(struct work_struct *));
bool	schedule_work(struct work_struct *);
bool	queue_work(struct workqueue_struct *, struct work_struct *);
bool	cancel_work_sync(struct work_struct *);
void	flush_work(struct work_struct *);

void	INIT_DELAYED_WORK(struct delayed_work *,
	    void (*)(struct work_struct *));
bool	schedule_delayed_work(struct delayed_work *, unsigned long);
bool	queue_delayed_work(struct workqueue_struct *, struct delayed_work *,
	    unsigned long ticks);
bool	mod_delayed_work(struct workqueue_struct *, struct delayed_work *,
	    unsigned long ticks);
bool	cancel_delayed_work(struct delayed_work *);
bool	cancel_delayed_work_sync(struct delayed_work *);

#endif  /* _LINUX_WORKQUEUE_H_ */
