/*	$NetBSD: task.c,v 1.17 2023/01/25 21:43:31 christos Exp $	*/

/*
 * Copyright (C) Internet Systems Consortium, Inc. ("ISC")
 *
 * SPDX-License-Identifier: MPL-2.0
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, you can obtain one at https://mozilla.org/MPL/2.0/.
 *
 * See the COPYRIGHT file distributed with this work for additional
 * information regarding copyright ownership.
 */

/*! \file */

/*
 * XXXRTH  Need to document the states a task can be in, and the rules
 * for changing states.
 */

#include <stdbool.h>
#include <unistd.h>

#include <isc/app.h>
#include <isc/atomic.h>
#include <isc/condition.h>
#include <isc/event.h>
#include <isc/log.h>
#include <isc/magic.h>
#include <isc/mem.h>
#include <isc/once.h>
#include <isc/platform.h>
#include <isc/print.h>
#include <isc/random.h>
#include <isc/refcount.h>
#include <isc/string.h>
#include <isc/task.h>
#include <isc/thread.h>
#include <isc/time.h>
#include <isc/util.h>

#ifdef HAVE_LIBXML2
#include <libxml/xmlwriter.h>
#define ISC_XMLCHAR (const xmlChar *)
#endif /* HAVE_LIBXML2 */

#ifdef HAVE_JSON_C
#include <json_object.h>
#endif /* HAVE_JSON_C */

#include "task_p.h"

/*
 * Task manager is built around 'as little locking as possible' concept.
 * Each thread has his own queue of tasks to be run, if a task is in running
 * state it will stay on the runner it's currently on, if a task is in idle
 * state it can be woken up on a specific runner with isc_task_sendto - that
 * helps with data locality on CPU.
 *
 * To make load even some tasks (from task pools) are bound to specific
 * queues using isc_task_create_bound. This way load balancing between
 * CPUs/queues happens on the higher layer.
 */

#ifdef ISC_TASK_TRACE
#define XTRACE(m) \
	fprintf(stderr, "task %p thread %zu: %s\n", task, isc_tid_v, (m))
#define XTTRACE(t, m) \
	fprintf(stderr, "task %p thread %zu: %s\n", (t), isc_tid_v, (m))
#define XTHREADTRACE(m) fprintf(stderr, "thread %zu: %s\n", isc_tid_v, (m))
#else /* ifdef ISC_TASK_TRACE */
#define XTRACE(m)
#define XTTRACE(t, m)
#define XTHREADTRACE(m)
#endif /* ifdef ISC_TASK_TRACE */

/***
 *** Types.
 ***/

typedef enum {
	task_state_idle,    /* not doing anything, events queue empty */
	task_state_ready,   /* waiting in worker's queue */
	task_state_paused,  /* not running, paused */
	task_state_pausing, /* running, waiting to be paused */
	task_state_running, /* actively processing events */
	task_state_done	    /* shutting down, no events or references */
} task_state_t;

#if defined(HAVE_LIBXML2) || defined(HAVE_JSON_C)
static const char *statenames[] = {
	"idle", "ready", "paused", "pausing", "running", "done",
};
#endif /* if defined(HAVE_LIBXML2) || defined(HAVE_JSON_C) */

#define TASK_MAGIC    ISC_MAGIC('T', 'A', 'S', 'K')
#define VALID_TASK(t) ISC_MAGIC_VALID(t, TASK_MAGIC)

struct isc_task {
	/* Not locked. */
	unsigned int magic;
	isc_taskmgr_t *manager;
	isc_mutex_t lock;
	/* Locked by task lock. */
	int threadid;
	task_state_t state;
	int pause_cnt;
	isc_refcount_t references;
	isc_refcount_t running;
	isc_eventlist_t events;
	isc_eventlist_t on_shutdown;
	unsigned int nevents;
	unsigned int quantum;
	isc_stdtime_t now;
	isc_time_t tnow;
	char name[16];
	void *tag;
	bool bound;
	/* Protected by atomics */
	atomic_bool shuttingdown;
	atomic_bool privileged;
	/* Locked by task manager lock. */
	LINK(isc_task_t) link;
};

#define TASK_SHUTTINGDOWN(t) (atomic_load_acquire(&(t)->shuttingdown))
#define TASK_PRIVILEGED(t)   (atomic_load_acquire(&(t)->privileged))

#define TASK_MANAGER_MAGIC ISC_MAGIC('T', 'S', 'K', 'M')
#define VALID_MANAGER(m)   ISC_MAGIC_VALID(m, TASK_MANAGER_MAGIC)

struct isc_taskmgr {
	/* Not locked. */
	unsigned int magic;
	isc_refcount_t references;
	isc_mem_t *mctx;
	isc_mutex_t lock;
	atomic_uint_fast32_t tasks_count;
	isc_nm_t *netmgr;

	/* Locked by task manager lock. */
	unsigned int default_quantum;
	LIST(isc_task_t) tasks;
	atomic_uint_fast32_t mode;
	atomic_bool exclusive_req;
	bool exiting;
	isc_task_t *excl;
};

#define DEFAULT_DEFAULT_QUANTUM 25

/*%
 * The following are intended for internal use (indicated by "isc__"
 * prefix) but are not declared as static, allowing direct access from
 * unit tests etc.
 */

bool
isc_task_purgeevent(isc_task_t *task, isc_event_t *event);
void
isc_taskmgr_setexcltask(isc_taskmgr_t *mgr, isc_task_t *task);
isc_result_t
isc_taskmgr_excltask(isc_taskmgr_t *mgr, isc_task_t **taskp);

/***
 *** Tasks.
 ***/

static void
task_finished(isc_task_t *task) {
	isc_taskmgr_t *manager = task->manager;
	isc_mem_t *mctx = manager->mctx;
	REQUIRE(EMPTY(task->events));
	REQUIRE(task->nevents == 0);
	REQUIRE(EMPTY(task->on_shutdown));
	REQUIRE(task->state == task_state_done);

	XTRACE("task_finished");

	isc_refcount_destroy(&task->running);
	isc_refcount_destroy(&task->references);

	LOCK(&manager->lock);
	UNLINK(manager->tasks, task, link);
	atomic_fetch_sub(&manager->tasks_count, 1);
	UNLOCK(&manager->lock);

	isc_mutex_destroy(&task->lock);
	task->magic = 0;
	isc_mem_put(mctx, task, sizeof(*task));

	isc_taskmgr_detach(&manager);
}

isc_result_t
isc_task_create(isc_taskmgr_t *manager, unsigned int quantum,
		isc_task_t **taskp) {
	return (isc_task_create_bound(manager, quantum, taskp, -1));
}

isc_result_t
isc_task_create_bound(isc_taskmgr_t *manager, unsigned int quantum,
		      isc_task_t **taskp, int threadid) {
	isc_task_t *task = NULL;
	bool exiting;

	REQUIRE(VALID_MANAGER(manager));
	REQUIRE(taskp != NULL && *taskp == NULL);

	XTRACE("isc_task_create");

	task = isc_mem_get(manager->mctx, sizeof(*task));
	*task = (isc_task_t){ 0 };

	isc_taskmgr_attach(manager, &task->manager);

	if (threadid == -1) {
		/*
		 * Task is not pinned to a queue, it's threadid will be
		 * chosen when first task will be sent to it - either
		 * randomly or specified by isc_task_sendto.
		 */
		task->bound = false;
		task->threadid = -1;
	} else {
		/*
		 * Task is pinned to a queue, it'll always be run
		 * by a specific thread.
		 */
		task->bound = true;
		task->threadid = threadid;
	}

	isc_mutex_init(&task->lock);
	task->state = task_state_idle;
	task->pause_cnt = 0;

	isc_refcount_init(&task->references, 1);
	isc_refcount_init(&task->running, 0);
	INIT_LIST(task->events);
	INIT_LIST(task->on_shutdown);
	task->nevents = 0;
	task->quantum = (quantum > 0) ? quantum : manager->default_quantum;
	atomic_init(&task->shuttingdown, false);
	atomic_init(&task->privileged, false);
	task->now = 0;
	isc_time_settoepoch(&task->tnow);
	memset(task->name, 0, sizeof(task->name));
	task->tag = NULL;
	INIT_LINK(task, link);
	task->magic = TASK_MAGIC;

	LOCK(&manager->lock);
	exiting = manager->exiting;
	if (!exiting) {
		APPEND(manager->tasks, task, link);
		atomic_fetch_add(&manager->tasks_count, 1);
	}
	UNLOCK(&manager->lock);

	if (exiting) {
		isc_refcount_destroy(&task->running);
		isc_refcount_decrement(&task->references);
		isc_refcount_destroy(&task->references);
		isc_mutex_destroy(&task->lock);
		isc_taskmgr_detach(&task->manager);
		isc_mem_put(manager->mctx, task, sizeof(*task));
		return (ISC_R_SHUTTINGDOWN);
	}

	*taskp = task;

	return (ISC_R_SUCCESS);
}

void
isc_task_attach(isc_task_t *source, isc_task_t **targetp) {
	/*
	 * Attach *targetp to source.
	 */

	REQUIRE(VALID_TASK(source));
	REQUIRE(targetp != NULL && *targetp == NULL);

	XTTRACE(source, "isc_task_attach");

	isc_refcount_increment(&source->references);

	*targetp = source;
}

static bool
task_shutdown(isc_task_t *task) {
	bool was_idle = false;
	isc_event_t *event, *prev;

	/*
	 * Caller must be holding the task's lock.
	 */

	XTRACE("task_shutdown");

	if (atomic_compare_exchange_strong(&task->shuttingdown,
					   &(bool){ false }, true))
	{
		XTRACE("shutting down");
		if (task->state == task_state_idle) {
			INSIST(EMPTY(task->events));
			task->state = task_state_ready;
			was_idle = true;
		}
		INSIST(task->state == task_state_ready ||
		       task->state == task_state_paused ||
		       task->state == task_state_pausing ||
		       task->state == task_state_running);

		/*
		 * Note that we post shutdown events LIFO.
		 */
		for (event = TAIL(task->on_shutdown); event != NULL;
		     event = prev)
		{
			prev = PREV(event, ev_link);
			DEQUEUE(task->on_shutdown, event, ev_link);
			ENQUEUE(task->events, event, ev_link);
			task->nevents++;
		}
	}

	return (was_idle);
}

/*
 * Moves a task onto the appropriate run queue.
 *
 * Caller must NOT hold queue lock.
 */
static void
task_ready(isc_task_t *task) {
	isc_taskmgr_t *manager = task->manager;
	REQUIRE(VALID_MANAGER(manager));

	XTRACE("task_ready");

	isc_refcount_increment0(&task->running);
	LOCK(&task->lock);
	isc_nm_task_enqueue(manager->netmgr, task, task->threadid);
	UNLOCK(&task->lock);
}

void
isc_task_ready(isc_task_t *task) {
	task_ready(task);
}

static bool
task_detach(isc_task_t *task) {
	/*
	 * Caller must be holding the task lock.
	 */

	XTRACE("detach");

	if (isc_refcount_decrement(&task->references) == 1 &&
	    task->state == task_state_idle)
	{
		INSIST(EMPTY(task->events));
		/*
		 * There are no references to this task, and no
		 * pending events.  We could try to optimize and
		 * either initiate shutdown or clean up the task,
		 * depending on its state, but it's easier to just
		 * make the task ready and allow run() or the event
		 * loop to deal with shutting down and termination.
		 */
		task->state = task_state_ready;
		return (true);
	}

	return (false);
}

void
isc_task_detach(isc_task_t **taskp) {
	isc_task_t *task;
	bool was_idle;

	/*
	 * Detach *taskp from its task.
	 */

	REQUIRE(taskp != NULL);
	task = *taskp;
	REQUIRE(VALID_TASK(task));

	XTRACE("isc_task_detach");

	LOCK(&task->lock);
	was_idle = task_detach(task);
	UNLOCK(&task->lock);

	if (was_idle) {
		task_ready(task);
	}

	*taskp = NULL;
}

static bool
task_send(isc_task_t *task, isc_event_t **eventp, int c) {
	bool was_idle = false;
	isc_event_t *event;

	/*
	 * Caller must be holding the task lock.
	 */

	REQUIRE(eventp != NULL);
	event = *eventp;
	*eventp = NULL;
	REQUIRE(event != NULL);
	REQUIRE(event->ev_type > 0);
	REQUIRE(task->state != task_state_done);
	REQUIRE(!ISC_LINK_LINKED(event, ev_ratelink));

	XTRACE("task_send");

	if (task->bound) {
		c = task->threadid;
	} else if (c < 0) {
		c = -1;
	}

	if (task->state == task_state_idle) {
		was_idle = true;
		task->threadid = c;
		INSIST(EMPTY(task->events));
		task->state = task_state_ready;
	}
	INSIST(task->state == task_state_ready ||
	       task->state == task_state_running ||
	       task->state == task_state_paused ||
	       task->state == task_state_pausing);
	ENQUEUE(task->events, event, ev_link);
	task->nevents++;

	return (was_idle);
}

void
isc_task_send(isc_task_t *task, isc_event_t **eventp) {
	isc_task_sendto(task, eventp, -1);
}

void
isc_task_sendanddetach(isc_task_t **taskp, isc_event_t **eventp) {
	isc_task_sendtoanddetach(taskp, eventp, -1);
}

void
isc_task_sendto(isc_task_t *task, isc_event_t **eventp, int c) {
	bool was_idle;

	/*
	 * Send '*event' to 'task'.
	 */

	REQUIRE(VALID_TASK(task));
	XTRACE("isc_task_send");

	/*
	 * We're trying hard to hold locks for as short a time as possible.
	 * We're also trying to hold as few locks as possible.  This is why
	 * some processing is deferred until after the lock is released.
	 */
	LOCK(&task->lock);
	was_idle = task_send(task, eventp, c);
	UNLOCK(&task->lock);

	if (was_idle) {
		/*
		 * We need to add this task to the ready queue.
		 *
		 * We've waited until now to do it because making a task
		 * ready requires locking the manager.  If we tried to do
		 * this while holding the task lock, we could deadlock.
		 *
		 * We've changed the state to ready, so no one else will
		 * be trying to add this task to the ready queue.  The
		 * only way to leave the ready state is by executing the
		 * task.  It thus doesn't matter if events are added,
		 * removed, or a shutdown is started in the interval
		 * between the time we released the task lock, and the time
		 * we add the task to the ready queue.
		 */
		task_ready(task);
	}
}

void
isc_task_sendtoanddetach(isc_task_t **taskp, isc_event_t **eventp, int c) {
	bool idle1, idle2;
	isc_task_t *task;

	/*
	 * Send '*event' to '*taskp' and then detach '*taskp' from its
	 * task.
	 */

	REQUIRE(taskp != NULL);
	task = *taskp;
	REQUIRE(VALID_TASK(task));
	XTRACE("isc_task_sendanddetach");

	LOCK(&task->lock);
	idle1 = task_send(task, eventp, c);
	idle2 = task_detach(task);
	UNLOCK(&task->lock);

	/*
	 * If idle1, then idle2 shouldn't be true as well since we're holding
	 * the task lock, and thus the task cannot switch from ready back to
	 * idle.
	 */
	INSIST(!(idle1 && idle2));

	if (idle1 || idle2) {
		task_ready(task);
	}

	*taskp = NULL;
}

#define PURGE_OK(event) (((event)->ev_attributes & ISC_EVENTATTR_NOPURGE) == 0)

static unsigned int
dequeue_events(isc_task_t *task, void *sender, isc_eventtype_t first,
	       isc_eventtype_t last, void *tag, isc_eventlist_t *events,
	       bool purging) {
	isc_event_t *event, *next_event;
	unsigned int count = 0;

	REQUIRE(VALID_TASK(task));
	REQUIRE(last >= first);

	XTRACE("dequeue_events");

	/*
	 * Events matching 'sender', whose type is >= first and <= last, and
	 * whose tag is 'tag' will be dequeued.  If 'purging', matching events
	 * which are marked as unpurgable will not be dequeued.
	 *
	 * sender == NULL means "any sender", and tag == NULL means "any tag".
	 */

	LOCK(&task->lock);

	for (event = HEAD(task->events); event != NULL; event = next_event) {
		next_event = NEXT(event, ev_link);
		if (event->ev_type >= first && event->ev_type <= last &&
		    (sender == NULL || event->ev_sender == sender) &&
		    (tag == NULL || event->ev_tag == tag) &&
		    (!purging || PURGE_OK(event)))
		{
			DEQUEUE(task->events, event, ev_link);
			task->nevents--;
			ENQUEUE(*events, event, ev_link);
			count++;
		}
	}

	UNLOCK(&task->lock);

	return (count);
}

unsigned int
isc_task_purgerange(isc_task_t *task, void *sender, isc_eventtype_t first,
		    isc_eventtype_t last, void *tag) {
	unsigned int count;
	isc_eventlist_t events;
	isc_event_t *event, *next_event;
	REQUIRE(VALID_TASK(task));

	/*
	 * Purge events from a task's event queue.
	 */

	XTRACE("isc_task_purgerange");

	ISC_LIST_INIT(events);

	count = dequeue_events(task, sender, first, last, tag, &events, true);

	for (event = HEAD(events); event != NULL; event = next_event) {
		next_event = NEXT(event, ev_link);
		ISC_LIST_UNLINK(events, event, ev_link);
		isc_event_free(&event);
	}

	/*
	 * Note that purging never changes the state of the task.
	 */

	return (count);
}

unsigned int
isc_task_purge(isc_task_t *task, void *sender, isc_eventtype_t type,
	       void *tag) {
	/*
	 * Purge events from a task's event queue.
	 */
	REQUIRE(VALID_TASK(task));

	XTRACE("isc_task_purge");

	return (isc_task_purgerange(task, sender, type, type, tag));
}

bool
isc_task_purgeevent(isc_task_t *task, isc_event_t *event) {
	isc_event_t *curr_event, *next_event;

	/*
	 * Purge 'event' from a task's event queue.
	 *
	 * XXXRTH:  WARNING:  This method may be removed before beta.
	 */

	REQUIRE(VALID_TASK(task));

	/*
	 * If 'event' is on the task's event queue, it will be purged,
	 * unless it is marked as unpurgeable.  'event' does not have to be
	 * on the task's event queue; in fact, it can even be an invalid
	 * pointer.  Purging only occurs if the event is actually on the task's
	 * event queue.
	 *
	 * Purging never changes the state of the task.
	 */

	LOCK(&task->lock);
	for (curr_event = HEAD(task->events); curr_event != NULL;
	     curr_event = next_event)
	{
		next_event = NEXT(curr_event, ev_link);
		if (curr_event == event && PURGE_OK(event)) {
			DEQUEUE(task->events, curr_event, ev_link);
			task->nevents--;
			break;
		}
	}
	UNLOCK(&task->lock);

	if (curr_event == NULL) {
		return (false);
	}

	isc_event_free(&curr_event);

	return (true);
}

unsigned int
isc_task_unsendrange(isc_task_t *task, void *sender, isc_eventtype_t first,
		     isc_eventtype_t last, void *tag, isc_eventlist_t *events) {
	/*
	 * Remove events from a task's event queue.
	 */
	REQUIRE(VALID_TASK(task));

	XTRACE("isc_task_unsendrange");

	return (dequeue_events(task, sender, first, last, tag, events, false));
}

unsigned int
isc_task_unsend(isc_task_t *task, void *sender, isc_eventtype_t type, void *tag,
		isc_eventlist_t *events) {
	/*
	 * Remove events from a task's event queue.
	 */

	XTRACE("isc_task_unsend");

	return (dequeue_events(task, sender, type, type, tag, events, false));
}

isc_result_t
isc_task_onshutdown(isc_task_t *task, isc_taskaction_t action, void *arg) {
	bool disallowed = false;
	isc_result_t result = ISC_R_SUCCESS;
	isc_event_t *event;

	/*
	 * Send a shutdown event with action 'action' and argument 'arg' when
	 * 'task' is shutdown.
	 */

	REQUIRE(VALID_TASK(task));
	REQUIRE(action != NULL);

	event = isc_event_allocate(task->manager->mctx, NULL,
				   ISC_TASKEVENT_SHUTDOWN, action, arg,
				   sizeof(*event));

	if (TASK_SHUTTINGDOWN(task)) {
		disallowed = true;
		result = ISC_R_SHUTTINGDOWN;
	} else {
		LOCK(&task->lock);
		ENQUEUE(task->on_shutdown, event, ev_link);
		UNLOCK(&task->lock);
	}

	if (disallowed) {
		isc_mem_put(task->manager->mctx, event, sizeof(*event));
	}

	return (result);
}

void
isc_task_shutdown(isc_task_t *task) {
	bool was_idle;

	/*
	 * Shutdown 'task'.
	 */

	REQUIRE(VALID_TASK(task));

	LOCK(&task->lock);
	was_idle = task_shutdown(task);
	UNLOCK(&task->lock);

	if (was_idle) {
		task_ready(task);
	}
}

void
isc_task_destroy(isc_task_t **taskp) {
	/*
	 * Destroy '*taskp'.
	 */

	REQUIRE(taskp != NULL);

	isc_task_shutdown(*taskp);
	isc_task_detach(taskp);
}

void
isc_task_setname(isc_task_t *task, const char *name, void *tag) {
	/*
	 * Name 'task'.
	 */

	REQUIRE(VALID_TASK(task));

	LOCK(&task->lock);
	strlcpy(task->name, name, sizeof(task->name));
	task->tag = tag;
	UNLOCK(&task->lock);
}

const char *
isc_task_getname(isc_task_t *task) {
	REQUIRE(VALID_TASK(task));

	return (task->name);
}

void *
isc_task_gettag(isc_task_t *task) {
	REQUIRE(VALID_TASK(task));

	return (task->tag);
}

void
isc_task_getcurrenttime(isc_task_t *task, isc_stdtime_t *t) {
	REQUIRE(VALID_TASK(task));
	REQUIRE(t != NULL);

	LOCK(&task->lock);
	*t = task->now;
	UNLOCK(&task->lock);
}

void
isc_task_getcurrenttimex(isc_task_t *task, isc_time_t *t) {
	REQUIRE(VALID_TASK(task));
	REQUIRE(t != NULL);

	LOCK(&task->lock);
	*t = task->tnow;
	UNLOCK(&task->lock);
}

isc_nm_t *
isc_task_getnetmgr(isc_task_t *task) {
	REQUIRE(VALID_TASK(task));

	return (task->manager->netmgr);
}

/***
 *** Task Manager.
 ***/

static isc_result_t
task_run(isc_task_t *task) {
	unsigned int dispatch_count = 0;
	bool finished = false;
	isc_event_t *event = NULL;
	isc_result_t result = ISC_R_SUCCESS;

	REQUIRE(VALID_TASK(task));

	LOCK(&task->lock);
	/*
	 * It is possible because that we have a paused task in the queue - it
	 * might have been paused in the meantime and we never hold both queue
	 * and task lock to avoid deadlocks, just bail then.
	 */
	if (task->state != task_state_ready) {
		goto done;
	}

	INSIST(task->state == task_state_ready);
	task->state = task_state_running;
	XTRACE("running");
	XTRACE(task->name);
	TIME_NOW(&task->tnow);
	task->now = isc_time_seconds(&task->tnow);

	while (true) {
		if (!EMPTY(task->events)) {
			event = HEAD(task->events);
			DEQUEUE(task->events, event, ev_link);
			task->nevents--;

			/*
			 * Execute the event action.
			 */
			XTRACE("execute action");
			XTRACE(task->name);
			if (event->ev_action != NULL) {
				UNLOCK(&task->lock);
				(event->ev_action)(task, event);
				LOCK(&task->lock);
			}
			XTRACE("execution complete");
			dispatch_count++;
		}

		if (isc_refcount_current(&task->references) == 0 &&
		    EMPTY(task->events) && !TASK_SHUTTINGDOWN(task))
		{
			/*
			 * There are no references and no pending events for
			 * this task, which means it will not become runnable
			 * again via an external action (such as sending an
			 * event or detaching).
			 *
			 * We initiate shutdown to prevent it from becoming a
			 * zombie.
			 *
			 * We do this here instead of in the "if
			 * EMPTY(task->events)" block below because:
			 *
			 *	If we post no shutdown events, we want the task
			 *	to finish.
			 *
			 *	If we did post shutdown events, will still want
			 *	the task's quantum to be applied.
			 */
			INSIST(!task_shutdown(task));
		}

		if (EMPTY(task->events)) {
			/*
			 * Nothing else to do for this task right now.
			 */
			XTRACE("empty");
			if (isc_refcount_current(&task->references) == 0 &&
			    TASK_SHUTTINGDOWN(task))
			{
				/*
				 * The task is done.
				 */
				XTRACE("done");
				task->state = task_state_done;
			} else {
				if (task->state == task_state_running) {
					XTRACE("idling");
					task->state = task_state_idle;
				} else if (task->state == task_state_pausing) {
					XTRACE("pausing");
					task->state = task_state_paused;
				}
			}
			break;
		} else if (task->state == task_state_pausing) {
			/*
			 * We got a pause request on this task, stop working on
			 * it and switch the state to paused.
			 */
			XTRACE("pausing");
			task->state = task_state_paused;
			break;
		} else if (dispatch_count >= task->quantum) {
			/*
			 * Our quantum has expired, but there is more work to be
			 * done.  We'll requeue it to the ready queue later.
			 *
			 * We don't check quantum until dispatching at least one
			 * event, so the minimum quantum is one.
			 */
			XTRACE("quantum");
			task->state = task_state_ready;
			result = ISC_R_QUOTA;
			break;
		}
	}

done:
	if (isc_refcount_decrement(&task->running) == 1 &&
	    task->state == task_state_done)
	{
		finished = true;
	}
	UNLOCK(&task->lock);

	if (finished) {
		task_finished(task);
	}

	return (result);
}

isc_result_t
isc_task_run(isc_task_t *task) {
	return (task_run(task));
}

static void
manager_free(isc_taskmgr_t *manager) {
	isc_refcount_destroy(&manager->references);
	isc_nm_detach(&manager->netmgr);

	isc_mutex_destroy(&manager->lock);
	manager->magic = 0;
	isc_mem_putanddetach(&manager->mctx, manager, sizeof(*manager));
}

void
isc_taskmgr_attach(isc_taskmgr_t *source, isc_taskmgr_t **targetp) {
	REQUIRE(VALID_MANAGER(source));
	REQUIRE(targetp != NULL && *targetp == NULL);

	isc_refcount_increment(&source->references);

	*targetp = source;
}

void
isc_taskmgr_detach(isc_taskmgr_t **managerp) {
	REQUIRE(managerp != NULL);
	REQUIRE(VALID_MANAGER(*managerp));

	isc_taskmgr_t *manager = *managerp;
	*managerp = NULL;

	if (isc_refcount_decrement(&manager->references) == 1) {
		manager_free(manager);
	}
}

isc_result_t
isc__taskmgr_create(isc_mem_t *mctx, unsigned int default_quantum, isc_nm_t *nm,
		    isc_taskmgr_t **managerp) {
	isc_taskmgr_t *manager;

	/*
	 * Create a new task manager.
	 */

	REQUIRE(managerp != NULL && *managerp == NULL);
	REQUIRE(nm != NULL);

	manager = isc_mem_get(mctx, sizeof(*manager));
	*manager = (isc_taskmgr_t){ .magic = TASK_MANAGER_MAGIC };

	isc_mutex_init(&manager->lock);

	if (default_quantum == 0) {
		default_quantum = DEFAULT_DEFAULT_QUANTUM;
	}
	manager->default_quantum = default_quantum;

	if (nm != NULL) {
		isc_nm_attach(nm, &manager->netmgr);
	}

	INIT_LIST(manager->tasks);
	atomic_init(&manager->mode, isc_taskmgrmode_normal);
	atomic_init(&manager->exclusive_req, false);
	atomic_init(&manager->tasks_count, 0);

	isc_mem_attach(mctx, &manager->mctx);

	isc_refcount_init(&manager->references, 1);

	*managerp = manager;

	return (ISC_R_SUCCESS);
}

void
isc__taskmgr_shutdown(isc_taskmgr_t *manager) {
	isc_task_t *task;

	REQUIRE(VALID_MANAGER(manager));

	XTHREADTRACE("isc_taskmgr_shutdown");
	/*
	 * Only one non-worker thread may ever call this routine.
	 * If a worker thread wants to initiate shutdown of the
	 * task manager, it should ask some non-worker thread to call
	 * isc_taskmgr_destroy(), e.g. by signalling a condition variable
	 * that the startup thread is sleeping on.
	 */

	/*
	 * Unlike elsewhere, we're going to hold this lock a long time.
	 * We need to do so, because otherwise the list of tasks could
	 * change while we were traversing it.
	 *
	 * This is also the only function where we will hold both the
	 * task manager lock and a task lock at the same time.
	 */
	LOCK(&manager->lock);
	if (manager->excl != NULL) {
		isc_task_detach((isc_task_t **)&manager->excl);
	}

	/*
	 * Make sure we only get called once.
	 */
	INSIST(manager->exiting == false);
	manager->exiting = true;

	/*
	 * Post shutdown event(s) to every task (if they haven't already been
	 * posted).
	 */
	for (task = HEAD(manager->tasks); task != NULL; task = NEXT(task, link))
	{
		bool was_idle;

		LOCK(&task->lock);
		was_idle = task_shutdown(task);
		if (was_idle) {
			task->threadid = 0;
		}
		UNLOCK(&task->lock);

		if (was_idle) {
			task_ready(task);
		}
	}

	UNLOCK(&manager->lock);
}

void
isc__taskmgr_destroy(isc_taskmgr_t **managerp) {
	REQUIRE(managerp != NULL && VALID_MANAGER(*managerp));
	XTHREADTRACE("isc_taskmgr_destroy");

#ifdef ISC_TASK_TRACE
	int counter = 0;
	while (isc_refcount_current(&(*managerp)->references) > 1 &&
	       counter++ < 1000)
	{
		usleep(10 * 1000);
	}
	INSIST(counter < 1000);
#else
	while (isc_refcount_current(&(*managerp)->references) > 1) {
		usleep(10 * 1000);
	}
#endif

	isc_taskmgr_detach(managerp);
}

void
isc_taskmgr_setexcltask(isc_taskmgr_t *mgr, isc_task_t *task) {
	REQUIRE(VALID_MANAGER(mgr));
	REQUIRE(VALID_TASK(task));

	LOCK(&task->lock);
	REQUIRE(task->threadid == 0);
	UNLOCK(&task->lock);

	LOCK(&mgr->lock);
	if (mgr->excl != NULL) {
		isc_task_detach(&mgr->excl);
	}
	isc_task_attach(task, &mgr->excl);
	UNLOCK(&mgr->lock);
}

isc_result_t
isc_taskmgr_excltask(isc_taskmgr_t *mgr, isc_task_t **taskp) {
	isc_result_t result;

	REQUIRE(VALID_MANAGER(mgr));
	REQUIRE(taskp != NULL && *taskp == NULL);

	LOCK(&mgr->lock);
	if (mgr->excl != NULL) {
		isc_task_attach(mgr->excl, taskp);
		result = ISC_R_SUCCESS;
	} else if (mgr->exiting) {
		result = ISC_R_SHUTTINGDOWN;
	} else {
		result = ISC_R_NOTFOUND;
	}
	UNLOCK(&mgr->lock);

	return (result);
}

isc_result_t
isc_task_beginexclusive(isc_task_t *task) {
	isc_taskmgr_t *manager;

	REQUIRE(VALID_TASK(task));

	manager = task->manager;

	REQUIRE(task->state == task_state_running);

	LOCK(&manager->lock);
	REQUIRE(task == manager->excl ||
		(manager->exiting && manager->excl == NULL));
	UNLOCK(&manager->lock);

	if (!atomic_compare_exchange_strong(&manager->exclusive_req,
					    &(bool){ false }, true))
	{
		return (ISC_R_LOCKBUSY);
	}

	if (isc_log_wouldlog(isc_lctx, ISC_LOG_DEBUG(1))) {
		isc_log_write(isc_lctx, ISC_LOGCATEGORY_GENERAL,
			      ISC_LOGMODULE_OTHER, ISC_LOG_DEBUG(1),
			      "exclusive task mode: %s", "starting");
	}

	isc_nm_pause(manager->netmgr);

	if (isc_log_wouldlog(isc_lctx, ISC_LOG_DEBUG(1))) {
		isc_log_write(isc_lctx, ISC_LOGCATEGORY_GENERAL,
			      ISC_LOGMODULE_OTHER, ISC_LOG_DEBUG(1),
			      "exclusive task mode: %s", "started");
	}

	return (ISC_R_SUCCESS);
}

void
isc_task_endexclusive(isc_task_t *task) {
	isc_taskmgr_t *manager;

	REQUIRE(VALID_TASK(task));
	REQUIRE(task->state == task_state_running);
	manager = task->manager;

	if (isc_log_wouldlog(isc_lctx, ISC_LOG_DEBUG(1))) {
		isc_log_write(isc_lctx, ISC_LOGCATEGORY_GENERAL,
			      ISC_LOGMODULE_OTHER, ISC_LOG_DEBUG(1),
			      "exclusive task mode: %s", "ending");
	}

	isc_nm_resume(manager->netmgr);

	if (isc_log_wouldlog(isc_lctx, ISC_LOG_DEBUG(1))) {
		isc_log_write(isc_lctx, ISC_LOGCATEGORY_GENERAL,
			      ISC_LOGMODULE_OTHER, ISC_LOG_DEBUG(1),
			      "exclusive task mode: %s", "ended");
	}

	REQUIRE(atomic_compare_exchange_strong(&manager->exclusive_req,
					       &(bool){ true }, false));
}

void
isc_task_pause(isc_task_t *task) {
	REQUIRE(VALID_TASK(task));

	LOCK(&task->lock);
	task->pause_cnt++;
	if (task->pause_cnt > 1) {
		/*
		 * Someone already paused this task, just increase
		 * the number of pausing clients.
		 */
		UNLOCK(&task->lock);
		return;
	}

	INSIST(task->state == task_state_idle ||
	       task->state == task_state_ready ||
	       task->state == task_state_running);
	if (task->state == task_state_running) {
		task->state = task_state_pausing;
	} else {
		task->state = task_state_paused;
	}
	UNLOCK(&task->lock);
}

void
isc_task_unpause(isc_task_t *task) {
	bool was_idle = false;

	REQUIRE(VALID_TASK(task));

	LOCK(&task->lock);
	task->pause_cnt--;
	INSIST(task->pause_cnt >= 0);
	if (task->pause_cnt > 0) {
		UNLOCK(&task->lock);
		return;
	}

	INSIST(task->state == task_state_paused ||
	       task->state == task_state_pausing);
	/* If the task was pausing we can't reschedule it */
	if (task->state == task_state_pausing) {
		task->state = task_state_running;
	} else {
		task->state = task_state_idle;
	}
	if (task->state == task_state_idle && !EMPTY(task->events)) {
		task->state = task_state_ready;
		was_idle = true;
	}
	UNLOCK(&task->lock);

	if (was_idle) {
		task_ready(task);
	}
}

void
isc_taskmgr_setmode(isc_taskmgr_t *manager, isc_taskmgrmode_t mode) {
	atomic_store(&manager->mode, mode);
}

isc_taskmgrmode_t
isc_taskmgr_mode(isc_taskmgr_t *manager) {
	return (atomic_load(&manager->mode));
}

void
isc_task_setprivilege(isc_task_t *task, bool priv) {
	REQUIRE(VALID_TASK(task));

	atomic_store_release(&task->privileged, priv);
}

bool
isc_task_getprivilege(isc_task_t *task) {
	REQUIRE(VALID_TASK(task));

	return (TASK_PRIVILEGED(task));
}

bool
isc_task_privileged(isc_task_t *task) {
	REQUIRE(VALID_TASK(task));

	return (isc_taskmgr_mode(task->manager) && TASK_PRIVILEGED(task));
}

bool
isc_task_exiting(isc_task_t *task) {
	REQUIRE(VALID_TASK(task));

	return (TASK_SHUTTINGDOWN(task));
}

#ifdef HAVE_LIBXML2
#define TRY0(a)                     \
	do {                        \
		xmlrc = (a);        \
		if (xmlrc < 0)      \
			goto error; \
	} while (0)
int
isc_taskmgr_renderxml(isc_taskmgr_t *mgr, void *writer0) {
	isc_task_t *task = NULL;
	int xmlrc;
	xmlTextWriterPtr writer = (xmlTextWriterPtr)writer0;

	LOCK(&mgr->lock);

	/*
	 * Write out the thread-model, and some details about each depending
	 * on which type is enabled.
	 */
	TRY0(xmlTextWriterStartElement(writer, ISC_XMLCHAR "thread-model"));
	TRY0(xmlTextWriterStartElement(writer, ISC_XMLCHAR "type"));
	TRY0(xmlTextWriterWriteString(writer, ISC_XMLCHAR "threaded"));
	TRY0(xmlTextWriterEndElement(writer)); /* type */

	TRY0(xmlTextWriterStartElement(writer, ISC_XMLCHAR "default-quantum"));
	TRY0(xmlTextWriterWriteFormatString(writer, "%d",
					    mgr->default_quantum));
	TRY0(xmlTextWriterEndElement(writer)); /* default-quantum */

	TRY0(xmlTextWriterEndElement(writer)); /* thread-model */

	TRY0(xmlTextWriterStartElement(writer, ISC_XMLCHAR "tasks"));
	task = ISC_LIST_HEAD(mgr->tasks);
	while (task != NULL) {
		LOCK(&task->lock);
		TRY0(xmlTextWriterStartElement(writer, ISC_XMLCHAR "task"));

		if (task->name[0] != 0) {
			TRY0(xmlTextWriterStartElement(writer,
						       ISC_XMLCHAR "name"));
			TRY0(xmlTextWriterWriteFormatString(writer, "%s",
							    task->name));
			TRY0(xmlTextWriterEndElement(writer)); /* name */
		}

		TRY0(xmlTextWriterStartElement(writer, ISC_XMLCHAR "reference"
								   "s"));
		TRY0(xmlTextWriterWriteFormatString(
			writer, "%" PRIuFAST32,
			isc_refcount_current(&task->references)));
		TRY0(xmlTextWriterEndElement(writer)); /* references */

		TRY0(xmlTextWriterStartElement(writer, ISC_XMLCHAR "id"));
		TRY0(xmlTextWriterWriteFormatString(writer, "%p", task));
		TRY0(xmlTextWriterEndElement(writer)); /* id */

		TRY0(xmlTextWriterStartElement(writer, ISC_XMLCHAR "state"));
		TRY0(xmlTextWriterWriteFormatString(writer, "%s",
						    statenames[task->state]));
		TRY0(xmlTextWriterEndElement(writer)); /* state */

		TRY0(xmlTextWriterStartElement(writer, ISC_XMLCHAR "quantum"));
		TRY0(xmlTextWriterWriteFormatString(writer, "%d",
						    task->quantum));
		TRY0(xmlTextWriterEndElement(writer)); /* quantum */

		TRY0(xmlTextWriterStartElement(writer, ISC_XMLCHAR "events"));
		TRY0(xmlTextWriterWriteFormatString(writer, "%d",
						    task->nevents));
		TRY0(xmlTextWriterEndElement(writer)); /* events */

		TRY0(xmlTextWriterEndElement(writer));

		UNLOCK(&task->lock);
		task = ISC_LIST_NEXT(task, link);
	}
	TRY0(xmlTextWriterEndElement(writer)); /* tasks */

error:
	if (task != NULL) {
		UNLOCK(&task->lock);
	}
	UNLOCK(&mgr->lock);

	return (xmlrc);
}
#endif /* HAVE_LIBXML2 */

#ifdef HAVE_JSON_C
#define CHECKMEM(m)                              \
	do {                                     \
		if (m == NULL) {                 \
			result = ISC_R_NOMEMORY; \
			goto error;              \
		}                                \
	} while (0)

isc_result_t
isc_taskmgr_renderjson(isc_taskmgr_t *mgr, void *tasks0) {
	isc_result_t result = ISC_R_SUCCESS;
	isc_task_t *task = NULL;
	json_object *obj = NULL, *array = NULL, *taskobj = NULL;
	json_object *tasks = (json_object *)tasks0;

	LOCK(&mgr->lock);

	/*
	 * Write out the thread-model, and some details about each depending
	 * on which type is enabled.
	 */
	obj = json_object_new_string("threaded");
	CHECKMEM(obj);
	json_object_object_add(tasks, "thread-model", obj);

	obj = json_object_new_int(mgr->default_quantum);
	CHECKMEM(obj);
	json_object_object_add(tasks, "default-quantum", obj);

	array = json_object_new_array();
	CHECKMEM(array);

	for (task = ISC_LIST_HEAD(mgr->tasks); task != NULL;
	     task = ISC_LIST_NEXT(task, link))
	{
		char buf[255];

		LOCK(&task->lock);

		taskobj = json_object_new_object();
		CHECKMEM(taskobj);
		json_object_array_add(array, taskobj);

		snprintf(buf, sizeof(buf), "%p", task);
		obj = json_object_new_string(buf);
		CHECKMEM(obj);
		json_object_object_add(taskobj, "id", obj);

		if (task->name[0] != 0) {
			obj = json_object_new_string(task->name);
			CHECKMEM(obj);
			json_object_object_add(taskobj, "name", obj);
		}

		obj = json_object_new_int(
			isc_refcount_current(&task->references));
		CHECKMEM(obj);
		json_object_object_add(taskobj, "references", obj);

		obj = json_object_new_string(statenames[task->state]);
		CHECKMEM(obj);
		json_object_object_add(taskobj, "state", obj);

		obj = json_object_new_int(task->quantum);
		CHECKMEM(obj);
		json_object_object_add(taskobj, "quantum", obj);

		obj = json_object_new_int(task->nevents);
		CHECKMEM(obj);
		json_object_object_add(taskobj, "events", obj);

		UNLOCK(&task->lock);
	}

	json_object_object_add(tasks, "tasks", array);
	array = NULL;
	result = ISC_R_SUCCESS;

error:
	if (array != NULL) {
		json_object_put(array);
	}

	if (task != NULL) {
		UNLOCK(&task->lock);
	}
	UNLOCK(&mgr->lock);

	return (result);
}
#endif /* ifdef HAVE_JSON_C */
