/*	$NetBSD: task.c,v 1.4 2019/02/24 20:01:31 christos Exp $	*/

/*
 * Copyright (C) Internet Systems Consortium, Inc. ("ISC")
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * See the COPYRIGHT file distributed with this work for additional
 * information regarding copyright ownership.
 */

/*! \file */

/*
 * XXXRTH  Need to document the states a task can be in, and the rules
 * for changing states.
 */

#include <config.h>

#include <stdbool.h>

#include <isc/app.h>
#include <isc/atomic.h>
#include <isc/condition.h>
#include <isc/event.h>
#include <isc/json.h>
#include <isc/magic.h>
#include <isc/mem.h>
#include <isc/once.h>
#include <isc/platform.h>
#include <isc/print.h>
#include <isc/string.h>
#include <isc/random.h>
#include <isc/task.h>
#include <isc/thread.h>
#include <isc/time.h>
#include <isc/util.h>
#include <isc/xml.h>

#ifdef OPENSSL_LEAKS
#include <openssl/err.h>
#endif

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
#define XTRACE(m)		fprintf(stderr, "task %p thread %lu: %s\n", \
				       task, isc_thread_self(), (m))
#define XTTRACE(t, m)		fprintf(stderr, "task %p thread %lu: %s\n", \
				       (t), isc_thread_self(), (m))
#define XTHREADTRACE(m)		fprintf(stderr, "thread %lu: %s\n", \
				       isc_thread_self(), (m))
#else
#define XTRACE(m)
#define XTTRACE(t, m)
#define XTHREADTRACE(m)
#endif

/***
 *** Types.
 ***/

typedef enum {
	task_state_idle, task_state_ready, task_state_running,
	task_state_done
} task_state_t;

#if defined(HAVE_LIBXML2) || defined(HAVE_JSON)
static const char *statenames[] = {
	"idle", "ready", "running", "done",
};
#endif

#define TASK_MAGIC			ISC_MAGIC('T', 'A', 'S', 'K')
#define VALID_TASK(t)			ISC_MAGIC_VALID(t, TASK_MAGIC)

typedef struct isc__task isc__task_t;
typedef struct isc__taskmgr isc__taskmgr_t;
typedef struct isc__taskqueue isc__taskqueue_t;

struct isc__task {
	/* Not locked. */
	isc_task_t			common;
	isc__taskmgr_t *		manager;
	isc_mutex_t			lock;
	/* Locked by task lock. */
	task_state_t			state;
	unsigned int			references;
	isc_eventlist_t			events;
	isc_eventlist_t			on_shutdown;
	unsigned int			nevents;
	unsigned int			quantum;
	unsigned int			flags;
	isc_stdtime_t			now;
	isc_time_t			tnow;
	char				name[16];
	void *				tag;
	unsigned int			threadid;
	bool				bound;
	/* Locked by task manager lock. */
	LINK(isc__task_t)		link;
	LINK(isc__task_t)		ready_link;
	LINK(isc__task_t)		ready_priority_link;
};

#define TASK_F_SHUTTINGDOWN		0x01
#define TASK_F_PRIVILEGED		0x02

#define TASK_SHUTTINGDOWN(t)		(((t)->flags & TASK_F_SHUTTINGDOWN) \
					 != 0)

#define TASK_MANAGER_MAGIC		ISC_MAGIC('T', 'S', 'K', 'M')
#define VALID_MANAGER(m)		ISC_MAGIC_VALID(m, TASK_MANAGER_MAGIC)

typedef ISC_LIST(isc__task_t)	isc__tasklist_t;

struct isc__taskqueue {
	/* Everything locked by lock */
	isc_mutex_t			lock;
	isc__tasklist_t			ready_tasks;
	isc__tasklist_t			ready_priority_tasks;
	isc_condition_t			work_available;
	isc_thread_t			thread;
	unsigned int			threadid;
	isc__taskmgr_t			*manager;
};

struct isc__taskmgr {
	/* Not locked. */
	isc_taskmgr_t			common;
	isc_mem_t *			mctx;
	isc_mutex_t			lock;
	isc_mutex_t			halt_lock;
	isc_condition_t			halt_cond;
	unsigned int			workers;
	atomic_uint_fast32_t		tasks_running;
	atomic_uint_fast32_t		tasks_ready;
	atomic_uint_fast32_t		curq;
	atomic_uint_fast32_t		tasks_count;
	isc__taskqueue_t		*queues;

	/* Locked by task manager lock. */
	unsigned int			default_quantum;
	LIST(isc__task_t)		tasks;
	atomic_uint_fast32_t		mode;
	atomic_bool			pause_req;
	atomic_bool			exclusive_req;
	atomic_bool			exiting;

	/* Locked by halt_lock */
	unsigned int			halted;

	/*
	 * Multiple threads can read/write 'excl' at the same time, so we need
	 * to protect the access.  We can't use 'lock' since isc_task_detach()
	 * will try to acquire it.
	 */
	isc_mutex_t			excl_lock;
	isc__task_t			*excl;
};

void
isc__taskmgr_pause(isc_taskmgr_t *manager0);
void
isc__taskmgr_resume(isc_taskmgr_t *manager0);


#define DEFAULT_DEFAULT_QUANTUM		25
#define FINISHED(m)	(atomic_load_relaxed(&((m)->exiting)) == true && \
			 atomic_load(&(m)->tasks_count) == 0)

/*%
 * The following are intended for internal use (indicated by "isc__"
 * prefix) but are not declared as static, allowing direct access from
 * unit tests etc.
 */

bool
isc_task_purgeevent(isc_task_t *task0, isc_event_t *event);
void
isc_taskmgr_setexcltask(isc_taskmgr_t *mgr0, isc_task_t *task0);
isc_result_t
isc_taskmgr_excltask(isc_taskmgr_t *mgr0, isc_task_t **taskp);
static inline bool
empty_readyq(isc__taskmgr_t *manager, int c);

static inline isc__task_t *
pop_readyq(isc__taskmgr_t *manager, int c);

static inline void
push_readyq(isc__taskmgr_t *manager, isc__task_t *task, int c);

static inline void
wake_all_queues(isc__taskmgr_t *manager);

/***
 *** Tasks.
 ***/

static inline void
wake_all_queues(isc__taskmgr_t *manager) {
	for (unsigned int i = 0; i < manager->workers; i++) {
		LOCK(&manager->queues[i].lock);
		BROADCAST(&manager->queues[i].work_available);
		UNLOCK(&manager->queues[i].lock);
	}
}

static void
task_finished(isc__task_t *task) {
	isc__taskmgr_t *manager = task->manager;
	REQUIRE(EMPTY(task->events));
	REQUIRE(task->nevents == 0);
	REQUIRE(EMPTY(task->on_shutdown));
	REQUIRE(task->references == 0);
	REQUIRE(task->state == task_state_done);

	XTRACE("task_finished");

	LOCK(&manager->lock);
	UNLINK(manager->tasks, task, link);
	atomic_fetch_sub(&manager->tasks_count, 1);
	UNLOCK(&manager->lock);
	if (FINISHED(manager)) {
		/*
		 * All tasks have completed and the
		 * task manager is exiting.  Wake up
		 * any idle worker threads so they
		 * can exit.
		 */
		wake_all_queues(manager);
	}
	isc_mutex_destroy(&task->lock);
	task->common.impmagic = 0;
	task->common.magic = 0;
	isc_mem_put(manager->mctx, task, sizeof(*task));
}

isc_result_t
isc_task_create(isc_taskmgr_t *manager0, unsigned int quantum,
		isc_task_t **taskp)
{
	return (isc_task_create_bound(manager0, quantum, taskp, -1));
}

isc_result_t
isc_task_create_bound(isc_taskmgr_t *manager0, unsigned int quantum,
		      isc_task_t **taskp, int threadid)
{
	isc__taskmgr_t *manager = (isc__taskmgr_t *)manager0;
	isc__task_t *task;
	bool exiting;

	REQUIRE(VALID_MANAGER(manager));
	REQUIRE(taskp != NULL && *taskp == NULL);

	task = isc_mem_get(manager->mctx, sizeof(*task));
	if (task == NULL)
		return (ISC_R_NOMEMORY);
	XTRACE("isc_task_create");
	task->manager = manager;

	if (threadid == -1) {
		/*
		 * Task is not pinned to a queue, it's threadid will be
		 * choosen when first task will be sent to it - either
		 * randomly or specified by isc_task_sendto.
		 */
		task->bound = false;
		task->threadid = 0;
	} else {
		/*
		 * Task is pinned to a queue, it'll always be run
		 * by a specific thread.
		 */
		task->bound = true;
		task->threadid = threadid % manager->workers;
	}

	isc_mutex_init(&task->lock);
	task->state = task_state_idle;
	task->references = 1;
	INIT_LIST(task->events);
	INIT_LIST(task->on_shutdown);
	task->nevents = 0;
	task->quantum = (quantum > 0) ? quantum : manager->default_quantum;
	task->flags = 0;
	task->now = 0;
	isc_time_settoepoch(&task->tnow);
	memset(task->name, 0, sizeof(task->name));
	task->tag = NULL;
	INIT_LINK(task, link);
	INIT_LINK(task, ready_link);
	INIT_LINK(task, ready_priority_link);

	exiting = false;
	LOCK(&manager->lock);
	if (!atomic_load_relaxed(&manager->exiting)) {
		APPEND(manager->tasks, task, link);
		atomic_fetch_add(&manager->tasks_count, 1);
	} else {
		exiting = true;
	}
	UNLOCK(&manager->lock);

	if (exiting) {
		isc_mutex_destroy(&task->lock);
		isc_mem_put(manager->mctx, task, sizeof(*task));
		return (ISC_R_SHUTTINGDOWN);
	}

	task->common.magic = ISCAPI_TASK_MAGIC;
	task->common.impmagic = TASK_MAGIC;
	*taskp = (isc_task_t *)task;

	return (ISC_R_SUCCESS);
}

void
isc_task_attach(isc_task_t *source0, isc_task_t **targetp) {
	isc__task_t *source = (isc__task_t *)source0;

	/*
	 * Attach *targetp to source.
	 */

	REQUIRE(VALID_TASK(source));
	REQUIRE(targetp != NULL && *targetp == NULL);

	XTTRACE(source, "isc_task_attach");

	LOCK(&source->lock);
	source->references++;
	UNLOCK(&source->lock);

	*targetp = (isc_task_t *)source;
}

static inline bool
task_shutdown(isc__task_t *task) {
	bool was_idle = false;
	isc_event_t *event, *prev;

	/*
	 * Caller must be holding the task's lock.
	 */

	XTRACE("task_shutdown");

	if (! TASK_SHUTTINGDOWN(task)) {
		XTRACE("shutting down");
		task->flags |= TASK_F_SHUTTINGDOWN;
		if (task->state == task_state_idle) {
			INSIST(EMPTY(task->events));
			task->state = task_state_ready;
			was_idle = true;
		}
		INSIST(task->state == task_state_ready ||
		       task->state == task_state_running);

		/*
		 * Note that we post shutdown events LIFO.
		 */
		for (event = TAIL(task->on_shutdown);
		     event != NULL;
		     event = prev) {
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
 * Caller must NOT hold manager lock.
 */
static inline void
task_ready(isc__task_t *task) {
	isc__taskmgr_t *manager = task->manager;
	bool has_privilege = isc_task_privilege((isc_task_t *) task);

	REQUIRE(VALID_MANAGER(manager));
	REQUIRE(task->state == task_state_ready);

	XTRACE("task_ready");
	LOCK(&manager->queues[task->threadid].lock);
	push_readyq(manager, task, task->threadid);
	if (atomic_load(&manager->mode) == isc_taskmgrmode_normal ||
	    has_privilege) {
		SIGNAL(&manager->queues[task->threadid].work_available);
	}
	UNLOCK(&manager->queues[task->threadid].lock);
}

static inline bool
task_detach(isc__task_t *task) {

	/*
	 * Caller must be holding the task lock.
	 */

	REQUIRE(task->references > 0);

	XTRACE("detach");

	task->references--;
	if (task->references == 0 && task->state == task_state_idle) {
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
	isc__task_t *task;
	bool was_idle;

	/*
	 * Detach *taskp from its task.
	 */

	REQUIRE(taskp != NULL);
	task = (isc__task_t *)*taskp;
	REQUIRE(VALID_TASK(task));

	XTRACE("isc_task_detach");

	LOCK(&task->lock);
	was_idle = task_detach(task);
	UNLOCK(&task->lock);

	if (was_idle)
		task_ready(task);

	*taskp = NULL;
}

static inline bool
task_send(isc__task_t *task, isc_event_t **eventp, int c) {
	bool was_idle = false;
	isc_event_t *event;

	/*
	 * Caller must be holding the task lock.
	 */

	REQUIRE(eventp != NULL);
	event = *eventp;
	REQUIRE(event != NULL);
	REQUIRE(event->ev_type > 0);
	REQUIRE(task->state != task_state_done);
	REQUIRE(!ISC_LINK_LINKED(event, ev_ratelink));

	XTRACE("task_send");

	if (task->state == task_state_idle) {
		was_idle = true;
		task->threadid = c;
		INSIST(EMPTY(task->events));
		task->state = task_state_ready;
	}
	INSIST(task->state == task_state_ready ||
	       task->state == task_state_running);
	ENQUEUE(task->events, event, ev_link);
	task->nevents++;
	*eventp = NULL;

	return (was_idle);
}

void
isc_task_send(isc_task_t *task0, isc_event_t **eventp) {
	isc_task_sendto(task0, eventp, -1);
}

void
isc_task_sendanddetach(isc_task_t **taskp, isc_event_t **eventp) {
	isc_task_sendtoanddetach(taskp, eventp, -1);
}

void
isc_task_sendto(isc_task_t *task0, isc_event_t **eventp, int c) {
	isc__task_t *task = (isc__task_t *)task0;
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
	/* If task is bound ignore provided cpu. */
	if (task->bound) {
		c = task->threadid;
	} else if (c < 0) {
		c = atomic_fetch_add_explicit(&task->manager->curq, 1,
					      memory_order_relaxed);
	}
	c %= task->manager->workers;
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
	isc__task_t *task;

	/*
	 * Send '*event' to '*taskp' and then detach '*taskp' from its
	 * task.
	 */

	REQUIRE(taskp != NULL);
	task = (isc__task_t *)*taskp;
	REQUIRE(VALID_TASK(task));
	XTRACE("isc_task_sendanddetach");

	LOCK(&task->lock);
	if (task->bound) {
		c = task->threadid;
	} else if (c < 0) {
		c = atomic_fetch_add_explicit(&task->manager->curq, 1,
					      memory_order_relaxed);
	}
	c %= task->manager->workers;
	idle1 = task_send(task, eventp, c);
	idle2 = task_detach(task);
	UNLOCK(&task->lock);

	/*
	 * If idle1, then idle2 shouldn't be true as well since we're holding
	 * the task lock, and thus the task cannot switch from ready back to
	 * idle.
	 */
	INSIST(!(idle1 && idle2));

	if (idle1 || idle2)
		task_ready(task);

	*taskp = NULL;
}

#define PURGE_OK(event)	(((event)->ev_attributes & ISC_EVENTATTR_NOPURGE) == 0)

static unsigned int
dequeue_events(isc__task_t *task, void *sender, isc_eventtype_t first,
	       isc_eventtype_t last, void *tag,
	       isc_eventlist_t *events, bool purging)
{
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
		    (!purging || PURGE_OK(event))) {
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
isc_task_purgerange(isc_task_t *task0, void *sender, isc_eventtype_t first,
		     isc_eventtype_t last, void *tag)
{
	isc__task_t *task = (isc__task_t *)task0;
	unsigned int count;
	isc_eventlist_t events;
	isc_event_t *event, *next_event;
	REQUIRE(VALID_TASK(task));

	/*
	 * Purge events from a task's event queue.
	 */

	XTRACE("isc_task_purgerange");

	ISC_LIST_INIT(events);

	count = dequeue_events(task, sender, first, last, tag, &events,
			       true);

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
		void *tag)
{
	/*
	 * Purge events from a task's event queue.
	 */
	REQUIRE(VALID_TASK(task));

	XTRACE("isc_task_purge");

	return (isc_task_purgerange(task, sender, type, type, tag));
}

bool
isc_task_purgeevent(isc_task_t *task0, isc_event_t *event) {
	isc__task_t *task = (isc__task_t *)task0;
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
	for (curr_event = HEAD(task->events);
	     curr_event != NULL;
	     curr_event = next_event) {
		next_event = NEXT(curr_event, ev_link);
		if (curr_event == event && PURGE_OK(event)) {
			DEQUEUE(task->events, curr_event, ev_link);
			task->nevents--;
			break;
		}
	}
	UNLOCK(&task->lock);

	if (curr_event == NULL)
		return (false);

	isc_event_free(&curr_event);

	return (true);
}

unsigned int
isc_task_unsendrange(isc_task_t *task, void *sender, isc_eventtype_t first,
		      isc_eventtype_t last, void *tag,
		      isc_eventlist_t *events)
{
	/*
	 * Remove events from a task's event queue.
	 */
	REQUIRE(VALID_TASK(task));

	XTRACE("isc_task_unsendrange");

	return (dequeue_events((isc__task_t *)task, sender, first,
			       last, tag, events, false));
}

unsigned int
isc_task_unsend(isc_task_t *task, void *sender, isc_eventtype_t type,
		 void *tag, isc_eventlist_t *events)
{
	/*
	 * Remove events from a task's event queue.
	 */

	XTRACE("isc_task_unsend");

	return (dequeue_events((isc__task_t *)task, sender, type,
			       type, tag, events, false));
}

isc_result_t
isc_task_onshutdown(isc_task_t *task0, isc_taskaction_t action,
		     void *arg)
{
	isc__task_t *task = (isc__task_t *)task0;
	bool disallowed = false;
	isc_result_t result = ISC_R_SUCCESS;
	isc_event_t *event;

	/*
	 * Send a shutdown event with action 'action' and argument 'arg' when
	 * 'task' is shutdown.
	 */

	REQUIRE(VALID_TASK(task));
	REQUIRE(action != NULL);

	event = isc_event_allocate(task->manager->mctx,
				   NULL,
				   ISC_TASKEVENT_SHUTDOWN,
				   action,
				   arg,
				   sizeof(*event));
	if (event == NULL)
		return (ISC_R_NOMEMORY);

	LOCK(&task->lock);
	if (TASK_SHUTTINGDOWN(task)) {
		disallowed = true;
		result = ISC_R_SHUTTINGDOWN;
	} else
		ENQUEUE(task->on_shutdown, event, ev_link);
	UNLOCK(&task->lock);

	if (disallowed)
		isc_mem_put(task->manager->mctx, event, sizeof(*event));

	return (result);
}

void
isc_task_shutdown(isc_task_t *task0) {
	isc__task_t *task = (isc__task_t *)task0;
	bool was_idle;

	/*
	 * Shutdown 'task'.
	 */

	REQUIRE(VALID_TASK(task));

	LOCK(&task->lock);
	was_idle = task_shutdown(task);
	UNLOCK(&task->lock);

	if (was_idle)
		task_ready(task);
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
isc_task_setname(isc_task_t *task0, const char *name, void *tag) {
	isc__task_t *task = (isc__task_t *)task0;

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
isc_task_getname(isc_task_t *task0) {
	isc__task_t *task = (isc__task_t *)task0;

	REQUIRE(VALID_TASK(task));

	return (task->name);
}

void *
isc_task_gettag(isc_task_t *task0) {
	isc__task_t *task = (isc__task_t *)task0;

	REQUIRE(VALID_TASK(task));

	return (task->tag);
}

void
isc_task_getcurrenttime(isc_task_t *task0, isc_stdtime_t *t) {
	isc__task_t *task = (isc__task_t *)task0;

	REQUIRE(VALID_TASK(task));
	REQUIRE(t != NULL);

	LOCK(&task->lock);
	*t = task->now;
	UNLOCK(&task->lock);
}

void
isc_task_getcurrenttimex(isc_task_t *task0, isc_time_t *t) {
	isc__task_t *task = (isc__task_t *)task0;

	REQUIRE(VALID_TASK(task));
	REQUIRE(t != NULL);

	LOCK(&task->lock);
	*t = task->tnow;
	UNLOCK(&task->lock);
}

/***
 *** Task Manager.
 ***/

/*
 * Return true if the current ready list for the manager, which is
 * either ready_tasks or the ready_priority_tasks, depending on whether
 * the manager is currently in normal or privileged execution mode.
 *
 * Caller must hold the task manager lock.
 */
static inline bool
empty_readyq(isc__taskmgr_t *manager, int c) {
	isc__tasklist_t queue;

	if (atomic_load_relaxed(&manager->mode) == isc_taskmgrmode_normal) {
		queue = manager->queues[c].ready_tasks;
	} else {
		queue = manager->queues[c].ready_priority_tasks;
	}
	return (EMPTY(queue));
}

/*
 * Dequeue and return a pointer to the first task on the current ready
 * list for the manager.
 * If the task is privileged, dequeue it from the other ready list
 * as well.
 *
 * Caller must hold the task manager lock.
 */
static inline isc__task_t *
pop_readyq(isc__taskmgr_t *manager, int c) {
	isc__task_t *task;

	if (atomic_load_relaxed(&manager->mode) == isc_taskmgrmode_normal) {
		task = HEAD(manager->queues[c].ready_tasks);
	} else {
		task = HEAD(manager->queues[c].ready_priority_tasks);
	}

	if (task != NULL) {
		DEQUEUE(manager->queues[c].ready_tasks, task, ready_link);
		if (ISC_LINK_LINKED(task, ready_priority_link)) {
			DEQUEUE(manager->queues[c].ready_priority_tasks, task,
				ready_priority_link);
		}
	}

	return (task);
}

/*
 * Push 'task' onto the ready_tasks queue.  If 'task' has the privilege
 * flag set, then also push it onto the ready_priority_tasks queue.
 *
 * Caller must hold the task manager lock.
 */
static inline void
push_readyq(isc__taskmgr_t *manager, isc__task_t *task, int c) {
	ENQUEUE(manager->queues[c].ready_tasks, task, ready_link);
	if ((task->flags & TASK_F_PRIVILEGED) != 0) {
		ENQUEUE(manager->queues[c].ready_priority_tasks, task,
			ready_priority_link);
	}
	atomic_fetch_add_explicit(&manager->tasks_ready, 1,
				  memory_order_acquire);
}

static void
dispatch(isc__taskmgr_t *manager, unsigned int threadid) {
	isc__task_t *task;

	REQUIRE(VALID_MANAGER(manager));

	/* Wait for everything to initialize */
	LOCK(&manager->lock);
	UNLOCK(&manager->lock);

	/*
	 * Again we're trying to hold the lock for as short a time as possible
	 * and to do as little locking and unlocking as possible.
	 *
	 * In both while loops, the appropriate lock must be held before the
	 * while body starts.  Code which acquired the lock at the top of
	 * the loop would be more readable, but would result in a lot of
	 * extra locking.  Compare:
	 *
	 * Straightforward:
	 *
	 *	LOCK();
	 *	...
	 *	UNLOCK();
	 *	while (expression) {
	 *		LOCK();
	 *		...
	 *		UNLOCK();
	 *
	 *	       	Unlocked part here...
	 *
	 *		LOCK();
	 *		...
	 *		UNLOCK();
	 *	}
	 *
	 * Note how if the loop continues we unlock and then immediately lock.
	 * For N iterations of the loop, this code does 2N+1 locks and 2N+1
	 * unlocks.  Also note that the lock is not held when the while
	 * condition is tested, which may or may not be important, depending
	 * on the expression.
	 *
	 * As written:
	 *
	 *	LOCK();
	 *	while (expression) {
	 *		...
	 *		UNLOCK();
	 *
	 *	       	Unlocked part here...
	 *
	 *		LOCK();
	 *		...
	 *	}
	 *	UNLOCK();
	 *
	 * For N iterations of the loop, this code does N+1 locks and N+1
	 * unlocks.  The while expression is always protected by the lock.
	 */
	LOCK(&manager->queues[threadid].lock);

	while (!FINISHED(manager)) {
		/*
		 * For reasons similar to those given in the comment in
		 * isc_task_send() above, it is safe for us to dequeue
		 * the task while only holding the manager lock, and then
		 * change the task to running state while only holding the
		 * task lock.
		 *
		 * If a pause has been requested, don't do any work
		 * until it's been released.
		 */
		while ((empty_readyq(manager, threadid) &&
			!atomic_load_relaxed(&manager->pause_req) &&
			!atomic_load_relaxed(&manager->exclusive_req)) &&
		       !FINISHED(manager))
		{
			XTHREADTRACE("wait");
			XTHREADTRACE(atomic_load_relaxed(&manager->pause_req)
				     ? "paused"
				     : "notpaused");
			XTHREADTRACE(atomic_load_relaxed(&manager->exclusive_req)
				     ? "excreq"
				     : "notexcreq");
			WAIT(&manager->queues[threadid].work_available,
			     &manager->queues[threadid].lock);
			XTHREADTRACE("awake");
		}
		XTHREADTRACE("working");

		if (atomic_load_relaxed(&manager->pause_req) ||
		    atomic_load_relaxed(&manager->exclusive_req)) {
			UNLOCK(&manager->queues[threadid].lock);
			XTHREADTRACE("halting");

			/*
			 * Switching to exclusive mode is done as a
			 * 2-phase-lock, checking if we have to switch is
			 * done without any locks on pause_req and
			 * exclusive_req to save time - the worst
			 * thing that can happen is that we'll launch one
			 * task more and exclusive task will be postponed a
			 * bit.
			 *
			 * Broadcasting on halt_cond seems suboptimal, but
			 * exclusive tasks are rare enought that we don't
			 * care.
			 */
			LOCK(&manager->halt_lock);
			manager->halted++;
			BROADCAST(&manager->halt_cond);
			while (atomic_load_relaxed(&manager->pause_req) ||
			       atomic_load_relaxed(&manager->exclusive_req))
			{
				WAIT(&manager->halt_cond, &manager->halt_lock);
			}
			manager->halted--;
			SIGNAL(&manager->halt_cond);
			UNLOCK(&manager->halt_lock);

			LOCK(&manager->queues[threadid].lock);
			/* Restart the loop after */
			continue;
		}

		task = pop_readyq(manager, threadid);
		if (task != NULL) {
			unsigned int dispatch_count = 0;
			bool done = false;
			bool requeue = false;
			bool finished = false;
			isc_event_t *event;

			INSIST(VALID_TASK(task));

			/*
			 * Note we only unlock the queue lock if we actually
			 * have a task to do.  We must reacquire the queue
			 * lock before exiting the 'if (task != NULL)' block.
			 */
			UNLOCK(&manager->queues[threadid].lock);
			RUNTIME_CHECK(
			      atomic_fetch_sub_explicit(&manager->tasks_ready,
						1, memory_order_release) > 0);
			atomic_fetch_add_explicit(&manager->tasks_running, 1,
						  memory_order_acquire);

			LOCK(&task->lock);
			INSIST(task->state == task_state_ready);
			task->state = task_state_running;
			XTRACE("running");
			XTRACE(task->name);
			TIME_NOW(&task->tnow);
			task->now = isc_time_seconds(&task->tnow);
			do {
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
						(event->ev_action)(
							(isc_task_t *)task,
							event);
						LOCK(&task->lock);
					}
					dispatch_count++;
				}

				if (task->references == 0 &&
				    EMPTY(task->events) &&
				    !TASK_SHUTTINGDOWN(task)) {
					bool was_idle;

					/*
					 * There are no references and no
					 * pending events for this task,
					 * which means it will not become
					 * runnable again via an external
					 * action (such as sending an event
					 * or detaching).
					 *
					 * We initiate shutdown to prevent
					 * it from becoming a zombie.
					 *
					 * We do this here instead of in
					 * the "if EMPTY(task->events)" block
					 * below because:
					 *
					 *	If we post no shutdown events,
					 *	we want the task to finish.
					 *
					 *	If we did post shutdown events,
					 *	will still want the task's
					 *	quantum to be applied.
					 */
					was_idle = task_shutdown(task);
					INSIST(!was_idle);
				}

				if (EMPTY(task->events)) {
					/*
					 * Nothing else to do for this task
					 * right now.
					 */
					XTRACE("empty");
					if (task->references == 0 &&
					    TASK_SHUTTINGDOWN(task)) {
						/*
						 * The task is done.
						 */
						XTRACE("done");
						finished = true;
						task->state = task_state_done;
					} else
						task->state = task_state_idle;
					done = true;
				} else if (dispatch_count >= task->quantum) {
					/*
					 * Our quantum has expired, but
					 * there is more work to be done.
					 * We'll requeue it to the ready
					 * queue later.
					 *
					 * We don't check quantum until
					 * dispatching at least one event,
					 * so the minimum quantum is one.
					 */
					XTRACE("quantum");
					task->state = task_state_ready;
					requeue = true;
					done = true;
				}
			} while (!done);
			UNLOCK(&task->lock);

			if (finished)
				task_finished(task);

			RUNTIME_CHECK(
			      atomic_fetch_sub_explicit(&manager->tasks_running,
						1, memory_order_release) > 0);
			LOCK(&manager->queues[threadid].lock);
			if (requeue) {
				/*
				 * We know we're awake, so we don't have
				 * to wakeup any sleeping threads if the
				 * ready queue is empty before we requeue.
				 *
				 * A possible optimization if the queue is
				 * empty is to 'goto' the 'if (task != NULL)'
				 * block, avoiding the ENQUEUE of the task
				 * and the subsequent immediate DEQUEUE
				 * (since it is the only executable task).
				 * We don't do this because then we'd be
				 * skipping the exit_requested check.  The
				 * cost of ENQUEUE is low anyway, especially
				 * when you consider that we'd have to do
				 * an extra EMPTY check to see if we could
				 * do the optimization.  If the ready queue
				 * were usually nonempty, the 'optimization'
				 * might even hurt rather than help.
				 */
				push_readyq(manager, task, threadid);
			}
		}

		/*
		 * If we are in privileged execution mode and there are no
		 * tasks remaining on the current ready queue, then
		 * we're stuck.  Automatically drop privileges at that
		 * point and continue with the regular ready queue.
		 */
		if (manager->mode != isc_taskmgrmode_normal &&
		    atomic_load_explicit(&manager->tasks_running,
					 memory_order_acquire) == 0)
		{
			UNLOCK(&manager->queues[threadid].lock);
			LOCK(&manager->lock);
			/*
			 * Check once again, under lock. Mode can only
			 * change from privileged to normal anyway, and
			 * if we enter this loop twice at the same time
			 * we'll end up in a deadlock over queue locks.
			 *
			 */
			if (manager->mode != isc_taskmgrmode_normal &&
			    atomic_load_explicit(&manager->tasks_running,
						 memory_order_acquire) == 0)
			{
				bool empty = true;
				unsigned int i;
				for (i = 0; i < manager->workers && empty; i++)
				{
					LOCK(&manager->queues[i].lock);
					empty &= empty_readyq(manager, i);
					UNLOCK(&manager->queues[i].lock);
				}
				if (empty) {
					atomic_store(&manager->mode,
						     isc_taskmgrmode_normal);
					wake_all_queues(manager);
				}
			}
			UNLOCK(&manager->lock);
			LOCK(&manager->queues[threadid].lock);
		}
	}
	UNLOCK(&manager->queues[threadid].lock);
	/*
	 * There might be other dispatchers waiting on empty tasks,
	 * wake them up.
	 */
	wake_all_queues(manager);
}

static isc_threadresult_t
#ifdef _WIN32
WINAPI
#endif
run(void *queuep) {
	isc__taskqueue_t *tq = queuep;
	isc__taskmgr_t *manager = tq->manager;
	int threadid = tq->threadid;
	isc_thread_setaffinity(threadid);

	XTHREADTRACE("starting");

	dispatch(manager, threadid);

	XTHREADTRACE("exiting");

#ifdef OPENSSL_LEAKS
	ERR_remove_state(0);
#endif

	return ((isc_threadresult_t)0);
}

static void
manager_free(isc__taskmgr_t *manager) {
	for (unsigned int i = 0; i < manager->workers; i++) {
		isc_mutex_destroy(&manager->queues[i].lock);
	}
	isc_mutex_destroy(&manager->lock);
	isc_mutex_destroy(&manager->halt_lock);
	isc_mem_put(manager->mctx, manager->queues,
		    manager->workers * sizeof(isc__taskqueue_t));
	manager->common.impmagic = 0;
	manager->common.magic = 0;
	isc_mem_putanddetach(&manager->mctx, manager, sizeof(*manager));
}

isc_result_t
isc_taskmgr_create(isc_mem_t *mctx, unsigned int workers,
		    unsigned int default_quantum, isc_taskmgr_t **managerp)
{
	unsigned int i;
	isc__taskmgr_t *manager;

	/*
	 * Create a new task manager.
	 */

	REQUIRE(workers > 0);
	REQUIRE(managerp != NULL && *managerp == NULL);

	manager = isc_mem_get(mctx, sizeof(*manager));
	RUNTIME_CHECK(manager != NULL);
	manager->common.impmagic = TASK_MANAGER_MAGIC;
	manager->common.magic = ISCAPI_TASKMGR_MAGIC;
	atomic_store(&manager->mode, isc_taskmgrmode_normal);
	manager->mctx = NULL;
	isc_mutex_init(&manager->lock);
	isc_mutex_init(&manager->excl_lock);

	isc_mutex_init(&manager->halt_lock);
	isc_condition_init(&manager->halt_cond);

	manager->workers = workers;

	if (default_quantum == 0) {
		default_quantum = DEFAULT_DEFAULT_QUANTUM;
	}
	manager->default_quantum = default_quantum;
	INIT_LIST(manager->tasks);
	atomic_store(&manager->tasks_count, 0);
	manager->queues = isc_mem_get(mctx, workers * sizeof(isc__taskqueue_t));
	RUNTIME_CHECK(manager->queues != NULL);

	manager->tasks_running = 0;
	manager->tasks_ready = 0;
	manager->curq = 0;
	manager->exiting = false;
	manager->excl = NULL;
	manager->halted = 0;
	atomic_store_relaxed(&manager->exclusive_req, false);
	atomic_store_relaxed(&manager->pause_req, false);

	isc_mem_attach(mctx, &manager->mctx);

	LOCK(&manager->lock);
	/*
	 * Start workers.
	 */
	for (i = 0; i < workers; i++) {
		INIT_LIST(manager->queues[i].ready_tasks);
		INIT_LIST(manager->queues[i].ready_priority_tasks);
		isc_mutex_init(&manager->queues[i].lock);
		isc_condition_init(&manager->queues[i].work_available);

		manager->queues[i].manager = manager;
		manager->queues[i].threadid = i;
		RUNTIME_CHECK(isc_thread_create(run, &manager->queues[i],
						&manager->queues[i].thread)
			      == ISC_R_SUCCESS);
		char name[16];
		snprintf(name, sizeof(name), "isc-worker%04u", i);
		isc_thread_setname(manager->queues[i].thread, name);
	}
	UNLOCK(&manager->lock);

	isc_thread_setconcurrency(workers);

	*managerp = (isc_taskmgr_t *)manager;

	return (ISC_R_SUCCESS);
}

void
isc_taskmgr_destroy(isc_taskmgr_t **managerp) {
	isc__taskmgr_t *manager;
	isc__task_t *task;
	unsigned int i;
	bool exiting;

	/*
	 * Destroy '*managerp'.
	 */

	REQUIRE(managerp != NULL);
	manager = (isc__taskmgr_t *)*managerp;
	REQUIRE(VALID_MANAGER(manager));

	XTHREADTRACE("isc_taskmgr_destroy");
	/*
	 * Only one non-worker thread may ever call this routine.
	 * If a worker thread wants to initiate shutdown of the
	 * task manager, it should ask some non-worker thread to call
	 * isc_taskmgr_destroy(), e.g. by signalling a condition variable
	 * that the startup thread is sleeping on.
	 */

	/*
	 * Detach the exclusive task before acquiring the manager lock
	 */
	LOCK(&manager->excl_lock);
	if (manager->excl != NULL)
		isc_task_detach((isc_task_t **) &manager->excl);
	UNLOCK(&manager->excl_lock);

	/*
	 * Unlike elsewhere, we're going to hold this lock a long time.
	 * We need to do so, because otherwise the list of tasks could
	 * change while we were traversing it.
	 *
	 * This is also the only function where we will hold both the
	 * task manager lock and a task lock at the same time.
	 */

	LOCK(&manager->lock);

	/*
	 * Make sure we only get called once.
	 */
	exiting = false;

	INSIST(!!atomic_compare_exchange_strong(&manager->exiting,
						&exiting, true));

	/*
	 * If privileged mode was on, turn it off.
	 */
	atomic_store(&manager->mode, isc_taskmgrmode_normal);

	/*
	 * Post shutdown event(s) to every task (if they haven't already been
	 * posted). To make things easier post idle tasks to worker 0.
	 */
	LOCK(&manager->queues[0].lock);
	for (task = HEAD(manager->tasks);
	     task != NULL;
	     task = NEXT(task, link)) {
		LOCK(&task->lock);
		if (task_shutdown(task)) {
			task->threadid = 0;
			push_readyq(manager, task, 0);
		}
		UNLOCK(&task->lock);
	}
	UNLOCK(&manager->queues[0].lock);

	/*
	 * Wake up any sleeping workers.  This ensures we get work done if
	 * there's work left to do, and if there are already no tasks left
	 * it will cause the workers to see manager->exiting.
	 */
	wake_all_queues(manager);
	UNLOCK(&manager->lock);

	/*
	 * Wait for all the worker threads to exit.
	 */
	for (i = 0; i < manager->workers; i++)
		(void)isc_thread_join(manager->queues[i].thread, NULL);

	manager_free(manager);

	*managerp = NULL;
}

void
isc_taskmgr_setprivilegedmode(isc_taskmgr_t *manager0) {
	isc__taskmgr_t *manager = (isc__taskmgr_t *)manager0;

	atomic_store(&manager->mode, isc_taskmgrmode_privileged);
}

isc_taskmgrmode_t
isc_taskmgr_mode(isc_taskmgr_t *manager0) {
	isc__taskmgr_t *manager = (isc__taskmgr_t *)manager0;
	return (atomic_load(&manager->mode));
}

void
isc__taskmgr_pause(isc_taskmgr_t *manager0) {
	isc__taskmgr_t *manager = (isc__taskmgr_t *)manager0;

	LOCK(&manager->halt_lock);
	while (atomic_load_relaxed(&manager->exclusive_req) ||
	       atomic_load_relaxed(&manager->pause_req)) {
		UNLOCK(&manager->halt_lock);
		/* This is ugly but pause is used EXCLUSIVELY in tests */
		isc_thread_yield();
		LOCK(&manager->halt_lock);
	}

	atomic_store_relaxed(&manager->pause_req, true);
	while (manager->halted < manager->workers) {
		wake_all_queues(manager);
		WAIT(&manager->halt_cond, &manager->halt_lock);
	}
	UNLOCK(&manager->halt_lock);
}

void
isc__taskmgr_resume(isc_taskmgr_t *manager0) {
	isc__taskmgr_t *manager = (isc__taskmgr_t *)manager0;
	LOCK(&manager->halt_lock);
	if (manager->pause_req) {
		manager->pause_req = false;
		while (manager->halted > 0) {
			BROADCAST(&manager->halt_cond);
			WAIT(&manager->halt_cond, &manager->halt_lock);
		}
	}
	UNLOCK(&manager->halt_lock);
}

void
isc_taskmgr_setexcltask(isc_taskmgr_t *mgr0, isc_task_t *task0) {
	isc__taskmgr_t *mgr = (isc__taskmgr_t *) mgr0;
	isc__task_t *task = (isc__task_t *) task0;

	REQUIRE(VALID_MANAGER(mgr));
	REQUIRE(VALID_TASK(task));
	LOCK(&mgr->excl_lock);
	if (mgr->excl != NULL)
		isc_task_detach((isc_task_t **) &mgr->excl);
	isc_task_attach(task0, (isc_task_t **) &mgr->excl);
	UNLOCK(&mgr->excl_lock);
}

isc_result_t
isc_taskmgr_excltask(isc_taskmgr_t *mgr0, isc_task_t **taskp) {
	isc__taskmgr_t *mgr = (isc__taskmgr_t *) mgr0;
	isc_result_t result = ISC_R_SUCCESS;

	REQUIRE(VALID_MANAGER(mgr));
	REQUIRE(taskp != NULL && *taskp == NULL);

	LOCK(&mgr->excl_lock);
	if (mgr->excl != NULL)
		isc_task_attach((isc_task_t *) mgr->excl, taskp);
	else
		result = ISC_R_NOTFOUND;
	UNLOCK(&mgr->excl_lock);

	return (result);
}

isc_result_t
isc_task_beginexclusive(isc_task_t *task0) {
	isc__task_t *task = (isc__task_t *)task0;
	isc__taskmgr_t *manager = task->manager;

	REQUIRE(VALID_TASK(task));

	REQUIRE(task->state == task_state_running);

	LOCK(&manager->excl_lock);
	REQUIRE(task == task->manager->excl ||
		(atomic_load_relaxed(&task->manager->exiting) &&
				     task->manager->excl == NULL));
	UNLOCK(&manager->excl_lock);

	if (atomic_load_relaxed(&manager->exclusive_req) ||
	    atomic_load_relaxed(&manager->pause_req)) {
		return (ISC_R_LOCKBUSY);
	}

	LOCK(&manager->halt_lock);
	INSIST(!atomic_load_relaxed(&manager->exclusive_req) &&
	       !atomic_load_relaxed(&manager->pause_req));
	atomic_store_relaxed(&manager->exclusive_req, true);
	while (manager->halted + 1 < manager->workers) {
		wake_all_queues(manager);
		WAIT(&manager->halt_cond, &manager->halt_lock);
	}
	UNLOCK(&manager->halt_lock);
	return (ISC_R_SUCCESS);
}

void
isc_task_endexclusive(isc_task_t *task0) {
	isc__task_t *task = (isc__task_t *)task0;
	isc__taskmgr_t *manager = task->manager;

	REQUIRE(VALID_TASK(task));
	REQUIRE(task->state == task_state_running);
	LOCK(&manager->halt_lock);
	REQUIRE(atomic_load_relaxed(&manager->exclusive_req) == true);
	atomic_store_relaxed(&manager->exclusive_req, false);
	while (manager->halted > 0) {
		BROADCAST(&manager->halt_cond);
		WAIT(&manager->halt_cond, &manager->halt_lock);
	}
	UNLOCK(&manager->halt_lock);
}

void
isc_task_setprivilege(isc_task_t *task0, bool priv) {
	REQUIRE(ISCAPI_TASK_VALID(task0));
	isc__task_t *task = (isc__task_t *)task0;
	isc__taskmgr_t *manager = task->manager;
	bool oldpriv;

	LOCK(&task->lock);
	oldpriv = ((task->flags & TASK_F_PRIVILEGED) != 0);
	if (priv)
		task->flags |= TASK_F_PRIVILEGED;
	else
		task->flags &= ~TASK_F_PRIVILEGED;
	UNLOCK(&task->lock);

	if (priv == oldpriv)
		return;

	LOCK(&manager->queues[task->threadid].lock);
	if (priv && ISC_LINK_LINKED(task, ready_link))
		ENQUEUE(manager->queues[task->threadid].ready_priority_tasks,
			task, ready_priority_link);
	else if (!priv && ISC_LINK_LINKED(task, ready_priority_link))
		DEQUEUE(manager->queues[task->threadid].ready_priority_tasks,
			task, ready_priority_link);
	UNLOCK(&manager->queues[task->threadid].lock);
}

bool
isc_task_privilege(isc_task_t *task0) {
	isc__task_t *task = (isc__task_t *)task0;
	bool priv;
	REQUIRE(VALID_TASK(task));

	LOCK(&task->lock);
	priv = ((task->flags & TASK_F_PRIVILEGED) != 0);
	UNLOCK(&task->lock);
	return (priv);
}

bool
isc_task_exiting(isc_task_t *t) {
	isc__task_t *task = (isc__task_t *)t;

	REQUIRE(VALID_TASK(task));
	return (TASK_SHUTTINGDOWN(task));
}


#ifdef HAVE_LIBXML2
#define TRY0(a) do { xmlrc = (a); if (xmlrc < 0) goto error; } while(/*CONSTCOND*/0)
int
isc_taskmgr_renderxml(isc_taskmgr_t *mgr0, xmlTextWriterPtr writer) {
	isc__taskmgr_t *mgr = (isc__taskmgr_t *)mgr0;
	isc__task_t *task = NULL;
	int xmlrc;

	LOCK(&mgr->lock);

	/*
	 * Write out the thread-model, and some details about each depending
	 * on which type is enabled.
	 */
	TRY0(xmlTextWriterStartElement(writer, ISC_XMLCHAR "thread-model"));
	TRY0(xmlTextWriterStartElement(writer, ISC_XMLCHAR "type"));
	TRY0(xmlTextWriterWriteString(writer, ISC_XMLCHAR "threaded"));
	TRY0(xmlTextWriterEndElement(writer)); /* type */

	TRY0(xmlTextWriterStartElement(writer, ISC_XMLCHAR "worker-threads"));
	TRY0(xmlTextWriterWriteFormatString(writer, "%d", mgr->workers));
	TRY0(xmlTextWriterEndElement(writer)); /* worker-threads */

	TRY0(xmlTextWriterStartElement(writer, ISC_XMLCHAR "default-quantum"));
	TRY0(xmlTextWriterWriteFormatString(writer, "%d",
					    mgr->default_quantum));
	TRY0(xmlTextWriterEndElement(writer)); /* default-quantum */

	TRY0(xmlTextWriterStartElement(writer, ISC_XMLCHAR "tasks-count"));
	TRY0(xmlTextWriterWriteFormatString(writer, "%d",
			       (int) atomic_load_relaxed(&mgr->tasks_count)));
	TRY0(xmlTextWriterEndElement(writer)); /* tasks-count */

	TRY0(xmlTextWriterStartElement(writer, ISC_XMLCHAR "tasks-running"));
	TRY0(xmlTextWriterWriteFormatString(writer, "%d",
			     (int) atomic_load_relaxed(&mgr->tasks_running)));
	TRY0(xmlTextWriterEndElement(writer)); /* tasks-running */

	TRY0(xmlTextWriterStartElement(writer, ISC_XMLCHAR "tasks-ready"));
	TRY0(xmlTextWriterWriteFormatString(writer, "%d",
			       (int) atomic_load_relaxed(&mgr->tasks_ready)));
	TRY0(xmlTextWriterEndElement(writer)); /* tasks-ready */

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

		TRY0(xmlTextWriterStartElement(writer,
					       ISC_XMLCHAR "references"));
		TRY0(xmlTextWriterWriteFormatString(writer, "%d",
						    task->references));
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
	if (task != NULL)
		UNLOCK(&task->lock);
	UNLOCK(&mgr->lock);

	return (xmlrc);
}
#endif /* HAVE_LIBXML2 */

#ifdef HAVE_JSON
#define CHECKMEM(m) do { \
	if (m == NULL) { \
		result = ISC_R_NOMEMORY;\
		goto error;\
	} \
} while(/*CONSTCOND*/0)

isc_result_t
isc_taskmgr_renderjson(isc_taskmgr_t *mgr0, json_object *tasks) {
	isc_result_t result = ISC_R_SUCCESS;
	isc__taskmgr_t *mgr = (isc__taskmgr_t *)mgr0;
	isc__task_t *task = NULL;
	json_object *obj = NULL, *array = NULL, *taskobj = NULL;

	LOCK(&mgr->lock);

	/*
	 * Write out the thread-model, and some details about each depending
	 * on which type is enabled.
	 */
	obj = json_object_new_string("threaded");
	CHECKMEM(obj);
	json_object_object_add(tasks, "thread-model", obj);

	obj = json_object_new_int(mgr->workers);
	CHECKMEM(obj);
	json_object_object_add(tasks, "worker-threads", obj);

	obj = json_object_new_int(mgr->default_quantum);
	CHECKMEM(obj);
	json_object_object_add(tasks, "default-quantum", obj);

	obj = json_object_new_int(atomic_load_relaxed(&mgr->tasks_count));
	CHECKMEM(obj);
	json_object_object_add(tasks, "tasks-count", obj);

	obj = json_object_new_int(atomic_load_relaxed(&mgr->tasks_running));
	CHECKMEM(obj);
	json_object_object_add(tasks, "tasks-running", obj);

	obj = json_object_new_int(atomic_load_relaxed(&mgr->tasks_ready));
	CHECKMEM(obj);
	json_object_object_add(tasks, "tasks-ready", obj);

	array = json_object_new_array();
	CHECKMEM(array);

	for (task = ISC_LIST_HEAD(mgr->tasks);
	     task != NULL;
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

		obj = json_object_new_int(task->references);
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
	if (array != NULL)
		json_object_put(array);

	if (task != NULL)
		UNLOCK(&task->lock);
	UNLOCK(&mgr->lock);

	return (result);
}
#endif


isc_result_t
isc_taskmgr_createinctx(isc_mem_t *mctx, isc_appctx_t *actx,
			unsigned int workers, unsigned int default_quantum,
			isc_taskmgr_t **managerp)
{
	isc_result_t result;

	result = isc_taskmgr_create(mctx, workers, default_quantum,
				       managerp);

	if (result == ISC_R_SUCCESS)
		isc_appctx_settaskmgr(actx, *managerp);

	return (result);
}
