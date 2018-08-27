/*	$NetBSD: nouveau_nvkm_subdev_timer_base.c,v 1.2 2018/08/27 04:58:35 riastradh Exp $	*/

/*
 * Copyright 2012 Red Hat Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE COPYRIGHT HOLDER(S) OR AUTHOR(S) BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 * Authors: Ben Skeggs
 */
#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: nouveau_nvkm_subdev_timer_base.c,v 1.2 2018/08/27 04:58:35 riastradh Exp $");

#include "priv.h"

u64
nvkm_timer_read(struct nvkm_timer *tmr)
{
	return tmr->func->read(tmr);
}

void
nvkm_timer_alarm_trigger(struct nvkm_timer *tmr)
{
	struct nvkm_alarm *alarm, *atemp;
	unsigned long flags;
	struct list_head exec = LIST_HEAD_INIT(exec);

	/* Process pending alarms. */
	spin_lock_irqsave(&tmr->lock, flags);
	list_for_each_entry_safe(alarm, atemp, &tmr->alarms, head) {
		/* Have we hit the earliest alarm that hasn't gone off? */
		if (alarm->timestamp > nvkm_timer_read(tmr)) {
			/* Schedule it.  If we didn't race, we're done. */
			tmr->func->alarm_init(tmr, alarm->timestamp);
			if (alarm->timestamp > nvkm_timer_read(tmr))
				break;
		}

		/* Move to completed list.  We'll drop the lock before
		 * executing the callback so it can reschedule itself.
		 */
		list_del_init(&alarm->head);
		list_add(&alarm->exec, &exec);
	}

	/* Shut down interrupt if no more pending alarms. */
	if (list_empty(&tmr->alarms))
		tmr->func->alarm_fini(tmr);
	spin_unlock_irqrestore(&tmr->lock, flags);

	/* Execute completed callbacks. */
	list_for_each_entry_safe(alarm, atemp, &exec, exec) {
		list_del(&alarm->exec);
		alarm->func(alarm);
	}
}

void
nvkm_timer_alarm(struct nvkm_timer *tmr, u32 nsec, struct nvkm_alarm *alarm)
{
	struct nvkm_alarm *list;
	unsigned long flags;

	/* Remove alarm from pending list.
	 *
	 * This both protects against the corruption of the list,
	 * and implements alarm rescheduling/cancellation.
	 */
	spin_lock_irqsave(&tmr->lock, flags);
	list_del_init(&alarm->head);

	if (nsec) {
		/* Insert into pending list, ordered earliest to latest. */
		alarm->timestamp = nvkm_timer_read(tmr) + nsec;
		list_for_each_entry(list, &tmr->alarms, head) {
			if (list->timestamp > alarm->timestamp)
				break;
		}

		list_add_tail(&alarm->head, &list->head);

		/* Update HW if this is now the earliest alarm. */
		list = list_first_entry(&tmr->alarms, typeof(*list), head);
		if (list == alarm) {
			tmr->func->alarm_init(tmr, alarm->timestamp);
			/* This shouldn't happen if callers aren't stupid.
			 *
			 * Worst case scenario is that it'll take roughly
			 * 4 seconds for the next alarm to trigger.
			 */
			WARN_ON(alarm->timestamp <= nvkm_timer_read(tmr));
		}
	}
	spin_unlock_irqrestore(&tmr->lock, flags);
}

void
nvkm_timer_alarm_cancel(struct nvkm_timer *tmr, struct nvkm_alarm *alarm)
{
	unsigned long flags;
	spin_lock_irqsave(&tmr->lock, flags);
	list_del_init(&alarm->head);
	spin_unlock_irqrestore(&tmr->lock, flags);
}

static void
nvkm_timer_intr(struct nvkm_subdev *subdev)
{
	struct nvkm_timer *tmr = nvkm_timer(subdev);
	tmr->func->intr(tmr);
}

static int
nvkm_timer_fini(struct nvkm_subdev *subdev, bool suspend)
{
	struct nvkm_timer *tmr = nvkm_timer(subdev);
	tmr->func->alarm_fini(tmr);
	return 0;
}

static int
nvkm_timer_init(struct nvkm_subdev *subdev)
{
	struct nvkm_timer *tmr = nvkm_timer(subdev);
	if (tmr->func->init)
		tmr->func->init(tmr);
	tmr->func->time(tmr, ktime_to_ns(ktime_get()));
	nvkm_timer_alarm_trigger(tmr);
	return 0;
}

static void *
nvkm_timer_dtor(struct nvkm_subdev *subdev)
{
	return nvkm_timer(subdev);
}

static const struct nvkm_subdev_func
nvkm_timer = {
	.dtor = nvkm_timer_dtor,
	.init = nvkm_timer_init,
	.fini = nvkm_timer_fini,
	.intr = nvkm_timer_intr,
};

int
nvkm_timer_new_(const struct nvkm_timer_func *func, struct nvkm_device *device,
		int index, struct nvkm_timer **ptmr)
{
	struct nvkm_timer *tmr;

	if (!(tmr = *ptmr = kzalloc(sizeof(*tmr), GFP_KERNEL)))
		return -ENOMEM;

	nvkm_subdev_ctor(&nvkm_timer, device, index, 0, &tmr->subdev);
	tmr->func = func;
	INIT_LIST_HEAD(&tmr->alarms);
	spin_lock_init(&tmr->lock);
	return 0;
}
