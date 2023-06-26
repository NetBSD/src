/*	$NetBSD: task.h,v 1.8 2023/06/26 22:03:01 christos Exp $	*/

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

#pragma once

/*****
 ***** Module Info
 *****/

/*! \file isc/task.h
 * \brief The task system provides a lightweight execution context, which is
 * basically an event queue.
 *
 * When a task's event queue is non-empty, the
 * task is runnable.  A small work crew of threads, typically one per CPU,
 * execute runnable tasks by dispatching the events on the tasks' event
 * queues.  Context switching between tasks is fast.
 *
 * \li MP:
 *	The module ensures appropriate synchronization of data structures it
 *	creates and manipulates.
 *	The caller must ensure that isc_taskmgr_destroy() is called only
 *	once for a given manager.
 *
 * \li Reliability:
 *	No anticipated impact.
 *
 * \li Resources:
 *	TBS
 *
 * \li Security:
 *	No anticipated impact.
 *
 * \li Standards:
 *	None.
 *
 * \section purge Purging and Unsending
 *
 * Events which have been queued for a task but not delivered may be removed
 * from the task's event queue by purging or unsending.
 *
 * With both types, the caller specifies a matching pattern that selects
 * events based upon their sender, type, and tag.
 *
 * Purging calls isc_event_free() on the matching events.
 *
 * Unsending returns a list of events that matched the pattern.
 * The caller is then responsible for them.
 *
 * Consumers of events should purge, not unsend.
 *
 * Producers of events often want to remove events when the caller indicates
 * it is no longer interested in the object, e.g. by canceling a timer.
 * Sometimes this can be done by purging, but for some event types, the
 * calls to isc_event_free() cause deadlock because the event free routine
 * wants to acquire a lock the caller is already holding.  Unsending instead
 * of purging solves this problem.  As a general rule, producers should only
 * unsend events which they have sent.
 */

/***
 *** Imports.
 ***/

#include <stdbool.h>

#include <isc/eventclass.h>
#include <isc/lang.h>
#include <isc/netmgr.h>
#include <isc/stdtime.h>
#include <isc/types.h>

#define ISC_TASKEVENT_FIRSTEVENT (ISC_EVENTCLASS_TASK + 0)
#define ISC_TASKEVENT_SHUTDOWN	 (ISC_EVENTCLASS_TASK + 1)
#define ISC_TASKEVENT_TEST	 (ISC_EVENTCLASS_TASK + 1)
#define ISC_TASKEVENT_LASTEVENT	 (ISC_EVENTCLASS_TASK + 65535)

/*****
 ***** Tasks.
 *****/

ISC_LANG_BEGINDECLS

/***
 *** Types
 ***/

typedef enum {
	isc_taskmgrmode_normal = 0,
	isc_taskmgrmode_privileged
} isc_taskmgrmode_t;

isc_result_t
isc_task_create(isc_taskmgr_t *manager, unsigned int quantum,
		isc_task_t **taskp);
isc_result_t
isc_task_create_bound(isc_taskmgr_t *manager, unsigned int quantum,
		      isc_task_t **taskp, int threadid);
/*%<
 * Create a task, optionally bound to a particular threadid.
 *
 * Notes:
 *
 *\li	If 'quantum' is non-zero, then only that many events can be dispatched
 *	before the task must yield to other tasks waiting to execute.  If
 *	quantum is zero, then the default quantum of the task manager will
 *	be used.
 *
 *\li	The 'quantum' option may be removed from isc_task_create() in the
 *	future.  If this happens, isc_task_getquantum() and
 *	isc_task_setquantum() will be provided.
 *
 * Requires:
 *
 *\li	'manager' is a valid task manager.
 *
 *\li	taskp != NULL && *taskp == NULL
 *
 * Ensures:
 *
 *\li	On success, '*taskp' is bound to the new task.
 *
 * Returns:
 *
 *\li   #ISC_R_SUCCESS
 *\li	#ISC_R_NOMEMORY
 *\li	#ISC_R_UNEXPECTED
 *\li	#ISC_R_SHUTTINGDOWN
 */

void
isc_task_ready(isc_task_t *task);
/*%<
 * Enqueue the task onto netmgr queue.
 */

isc_result_t
isc_task_run(isc_task_t *task);
/*%<
 * Run all the queued events for the 'task', returning
 * when the queue is empty or the number of events executed
 * exceeds the 'quantum' specified when the task was created.
 *
 * Requires:
 *
 *\li	'task' is a valid task.
 *
 * Returns:
 *
 *\li	#ISC_R_SUCCESS
 *\li	#ISC_R_QUOTA
 */

void
isc_task_attach(isc_task_t *source, isc_task_t **targetp);
/*%<
 * Attach *targetp to source.
 *
 * Requires:
 *
 *\li	'source' is a valid task.
 *
 *\li	'targetp' points to a NULL isc_task_t *.
 *
 * Ensures:
 *
 *\li	*targetp is attached to source.
 */

void
isc_task_detach(isc_task_t **taskp);
/*%<
 * Detach *taskp from its task.
 *
 * Requires:
 *
 *\li	'*taskp' is a valid task.
 *
 * Ensures:
 *
 *\li	*taskp is NULL.
 *
 *\li	If '*taskp' is the last reference to the task, the task is idle (has
 *	an empty event queue), and has not been shutdown, the task will be
 *	shutdown.
 *
 *\li	If '*taskp' is the last reference to the task and
 *	the task has been shutdown,
 *		all resources used by the task will be freed.
 */

void
isc_task_send(isc_task_t *task, isc_event_t **eventp);

void
isc_task_sendto(isc_task_t *task, isc_event_t **eventp, int c);
/*%<
 * Send '*event' to 'task', if task is idle try starting it on cpu 'c'
 * If 'c' is smaller than 0 then cpu is selected randomly.
 *
 * Requires:
 *
 *\li	'task' is a valid task.
 *\li	eventp != NULL && *eventp != NULL.
 *
 * Ensures:
 *
 *\li	*eventp == NULL.
 */

void
isc_task_sendtoanddetach(isc_task_t **taskp, isc_event_t **eventp, int c);

void
isc_task_sendanddetach(isc_task_t **taskp, isc_event_t **eventp);
/*%<
 * Send '*event' to '*taskp' and then detach '*taskp' from its
 * task. If task is idle try starting it on cpu 'c'
 * If 'c' is smaller than 0 then cpu is selected randomly.
 *
 * Requires:
 *
 *\li	'*taskp' is a valid task.
 *\li	eventp != NULL && *eventp != NULL.
 *
 * Ensures:
 *
 *\li	*eventp == NULL.
 *
 *\li	*taskp == NULL.
 *
 *\li	If '*taskp' is the last reference to the task, the task is
 *	idle (has an empty event queue), and has not been shutdown,
 *	the task will be shutdown.
 *
 *\li	If '*taskp' is the last reference to the task and
 *	the task has been shutdown,
 *		all resources used by the task will be freed.
 */

unsigned int
isc_task_purgerange(isc_task_t *task, void *sender, isc_eventtype_t first,
		    isc_eventtype_t last, void *tag);
/*%<
 * Purge events from a task's event queue.
 *
 * Requires:
 *
 *\li	'task' is a valid task.
 *
 *\li	last >= first
 *
 * Ensures:
 *
 *\li	Events in the event queue of 'task' whose sender is 'sender', whose
 *	type is >= first and <= last, and whose tag is 'tag' will be purged,
 *	unless they are marked as unpurgable.
 *
 *\li	A sender of NULL will match any sender.  A NULL tag matches any
 *	tag.
 *
 * Returns:
 *
 *\li	The number of events purged.
 */

unsigned int
isc_task_purge(isc_task_t *task, void *sender, isc_eventtype_t type, void *tag);
/*%<
 * Purge events from a task's event queue.
 *
 * Notes:
 *
 *\li	This function is equivalent to
 *
 *\code
 *		isc_task_purgerange(task, sender, type, type, tag);
 *\endcode
 *
 * Requires:
 *
 *\li	'task' is a valid task.
 *
 * Ensures:
 *
 *\li	Events in the event queue of 'task' whose sender is 'sender', whose
 *	type is 'type', and whose tag is 'tag' will be purged, unless they
 *	are marked as unpurgable.
 *
 *\li	A sender of NULL will match any sender.  A NULL tag matches any
 *	tag.
 *
 * Returns:
 *
 *\li	The number of events purged.
 */

bool
isc_task_purgeevent(isc_task_t *task, isc_event_t *event);
/*%<
 * Purge 'event' from a task's event queue.
 *
 * XXXRTH:  WARNING:  This method may be removed before beta.
 *
 * Notes:
 *
 *\li	If 'event' is on the task's event queue, it will be purged,
 * 	unless it is marked as unpurgeable.  'event' does not have to be
 *	on the task's event queue; in fact, it can even be an invalid
 *	pointer.  Purging only occurs if the event is actually on the task's
 *	event queue.
 *
 * \li	Purging never changes the state of the task.
 *
 * Requires:
 *
 *\li	'task' is a valid task.
 *
 * Ensures:
 *
 *\li	'event' is not in the event queue for 'task'.
 *
 * Returns:
 *
 *\li	#true			The event was purged.
 *\li	#false			The event was not in the event queue,
 *					or was marked unpurgeable.
 */

unsigned int
isc_task_unsendrange(isc_task_t *task, void *sender, isc_eventtype_t first,
		     isc_eventtype_t last, void *tag, isc_eventlist_t *events);
/*%<
 * Remove events from a task's event queue.
 *
 * Requires:
 *
 *\li	'task' is a valid task.
 *
 *\li	last >= first.
 *
 *\li	*events is a valid list.
 *
 * Ensures:
 *
 *\li	Events in the event queue of 'task' whose sender is 'sender', whose
 *	type is >= first and <= last, and whose tag is 'tag' will be dequeued
 *	and appended to *events.
 *
 *\li	A sender of NULL will match any sender.  A NULL tag matches any
 *	tag.
 *
 * Returns:
 *
 *\li	The number of events unsent.
 */

unsigned int
isc_task_unsend(isc_task_t *task, void *sender, isc_eventtype_t type, void *tag,
		isc_eventlist_t *events);
/*%<
 * Remove events from a task's event queue.
 *
 * Notes:
 *
 *\li  This function is equivalent to
 *
 *\code
 *             isc_task_unsendrange(task, sender, type, type, tag, events);
 *\endcode
 *
 * Requires:
 *
 *\li  'task' is a valid task.
 *
 *\li  *events is a valid list.
 *
 * Ensures:
 *
 *\li  Events in the event queue of 'task' whose sender is 'sender', whose
 *     type is 'type', and whose tag is 'tag' will be dequeued and appended
 *     to *events.
 *
 * Returns:
 *
 *\li  The number of events unsent.
 */

isc_result_t
isc_task_onshutdown(isc_task_t *task, isc_taskaction_t action, void *arg);
/*%<
 * Send a shutdown event with action 'action' and argument 'arg' when
 * 'task' is shutdown.
 *
 * Notes:
 *
 *\li	Shutdown events are posted in LIFO order.
 *
 * Requires:
 *
 *\li	'task' is a valid task.
 *
 *\li	'action' is a valid task action.
 *
 * Ensures:
 *
 *\li	When the task is shutdown, shutdown events requested with
 *	isc_task_onshutdown() will be appended to the task's event queue.
 *
 *
 * Returns:
 *
 *\li	#ISC_R_SUCCESS
 *\li	#ISC_R_NOMEMORY
 *\li	#ISC_R_SHUTTINGDOWN			Task is shutting down.
 */

void
isc_task_shutdown(isc_task_t *task);
/*%<
 * Shutdown 'task'.
 *
 * Notes:
 *
 *\li	Shutting down a task causes any shutdown events requested with
 *	isc_task_onshutdown() to be posted (in LIFO order).  The task
 *	moves into a "shutting down" mode which prevents further calls
 *	to isc_task_onshutdown().
 *
 *\li	Trying to shutdown a task that has already been shutdown has no
 *	effect.
 *
 * Requires:
 *
 *\li	'task' is a valid task.
 *
 * Ensures:
 *
 *\li	Any shutdown events requested with isc_task_onshutdown() have been
 *	posted (in LIFO order).
 */

void
isc_task_destroy(isc_task_t **taskp);
/*%<
 * Destroy '*taskp'.
 *
 * Notes:
 *
 *\li	This call is equivalent to:
 *
 *\code
 *		isc_task_shutdown(*taskp);
 *		isc_task_detach(taskp);
 *\endcode
 *
 * Requires:
 *
 *	'*taskp' is a valid task.
 *
 * Ensures:
 *
 *\li	Any shutdown events requested with isc_task_onshutdown() have been
 *	posted (in LIFO order).
 *
 *\li	*taskp == NULL
 *
 *\li	If '*taskp' is the last reference to the task,
 *		all resources used by the task will be freed.
 */

void
isc_task_setname(isc_task_t *task, const char *name, void *tag);
/*%<
 * Name 'task'.
 *
 * Notes:
 *
 *\li	Only the first 15 characters of 'name' will be copied.
 *
 *\li	Naming a task is currently only useful for debugging purposes.
 *
 * Requires:
 *
 *\li	'task' is a valid task.
 */

const char *
isc_task_getname(isc_task_t *task);
/*%<
 * Get the name of 'task', as previously set using isc_task_setname().
 *
 * Notes:
 *\li	This function is for debugging purposes only.
 *
 * Requires:
 *\li	'task' is a valid task.
 *
 * Returns:
 *\li	A non-NULL pointer to a null-terminated string.
 * 	If the task has not been named, the string is
 * 	empty.
 *
 */

isc_nm_t *
isc_task_getnetmgr(isc_task_t *task);

void *
isc_task_gettag(isc_task_t *task);
/*%<
 * Get the tag value for  'task', as previously set using isc_task_settag().
 *
 * Notes:
 *\li	This function is for debugging purposes only.
 *
 * Requires:
 *\li	'task' is a valid task.
 */

void
isc_task_setquantum(isc_task_t *task, unsigned int quantum);
/*%<
 * Set future 'task' quantum to 'quantum'.  The current 'task' quantum will be
 * kept for the current isc_task_run() loop, and will be changed for the next
 * run.  Therefore, the function is safe to use from the event callback as it
 * will not affect the current event loop processing.
 */

isc_result_t
isc_task_beginexclusive(isc_task_t *task);
/*%<
 * Request exclusive access for 'task', which must be the calling
 * task.  Waits for any other concurrently executing tasks to finish their
 * current event, and prevents any new events from executing in any of the
 * tasks sharing a task manager with 'task'.
 * It also pauses processing of network events in netmgr if it was provided
 * when taskmgr was created.
 *
 * The exclusive access must be relinquished by calling
 * isc_task_endexclusive() before returning from the current event handler.
 *
 * Requires:
 *\li	'task' is the calling task.
 *
 * Returns:
 *\li	#ISC_R_SUCCESS		The current task now has exclusive access.
 *\li	#ISC_R_LOCKBUSY		Another task has already requested exclusive
 *				access.
 */

void
isc_task_endexclusive(isc_task_t *task);
/*%<
 * Relinquish the exclusive access obtained by isc_task_beginexclusive(),
 * allowing other tasks to execute.
 *
 * Requires:
 *\li	'task' is the calling task, and has obtained
 *		exclusive access by calling isc_task_spl().
 */

void
isc_task_pause(isc_task_t *task0);
void
isc_task_unpause(isc_task_t *task0);
/*%<
 * Pause/unpause this task. Pausing a task removes it from the ready
 * queue if it is present there; this ensures that the task will not
 * run again until unpaused. This is necessary when the libuv network
 * thread executes a function which schedules task manager events; this
 * prevents the task manager from executing the next event in a task
 * before the network thread has finished.
 *
 * Requires:
 *\li	'task' is a valid task, and is not already paused or shutting down.
 */

void
isc_task_getcurrenttime(isc_task_t *task, isc_stdtime_t *t);
void
isc_task_getcurrenttimex(isc_task_t *task, isc_time_t *t);
/*%<
 * Provide the most recent timestamp on the task.  The timestamp is considered
 * as the "current time".
 *
 * isc_task_getcurrentime() returns the time in one-second granularity;
 * isc_task_getcurrentimex() returns it in nanosecond granularity.
 *
 * Requires:
 *\li	'task' is a valid task.
 *\li	't' is a valid non NULL pointer.
 *
 * Ensures:
 *\li	'*t' has the "current time".
 */

bool
isc_task_exiting(isc_task_t *t);
/*%<
 * Returns true if the task is in the process of shutting down,
 * false otherwise.
 *
 * Requires:
 *\li	'task' is a valid task.
 */

void
isc_task_setprivilege(isc_task_t *task, bool priv);
/*%<
 * Set or unset the task's "privileged" flag depending on the value of
 * 'priv'.
 *
 * Under normal circumstances this flag has no effect on the task behavior,
 * but when the task manager has been set to privileged execution mode via
 * isc_taskmgr_setmode(), only tasks with the flag set will be executed,
 * and all other tasks will wait until they're done.  Once all privileged
 * tasks have finished executing, the task manager resumes running
 * non-privileged tasks.
 *
 * Requires:
 *\li	'task' is a valid task.
 */

bool
isc_task_getprivilege(isc_task_t *task);
/*%<
 * Returns the current value of the task's privilege flag.
 *
 * Requires:
 *\li	'task' is a valid task.
 */

bool
isc_task_privileged(isc_task_t *task);
/*%<
 * Returns true if the task's privilege flag is set *and* the task
 * manager is currently in privileged mode.
 *
 * Requires:
 *\li	'task' is a valid task.
 */

/*****
 ***** Task Manager.
 *****/

void
isc_taskmgr_attach(isc_taskmgr_t *, isc_taskmgr_t **);
void
isc_taskmgr_detach(isc_taskmgr_t **);
/*%<
 * Attach/detach the task manager.
 */

void
isc_taskmgr_setmode(isc_taskmgr_t *manager, isc_taskmgrmode_t mode);
isc_taskmgrmode_t
isc_taskmgr_mode(isc_taskmgr_t *manager);
/*%<
 * Set/get the operating mode of the task manager. Valid modes are:
 *
 *\li	isc_taskmgrmode_normal
 *\li	isc_taskmgrmode_privileged
 *
 * In privileged execution mode, only tasks that have had the "privilege"
 * flag set via isc_task_setprivilege() can be executed. When all such
 * tasks are complete, non-privileged tasks resume running. The task calling
 * this function should be in task-exclusive mode; the privileged tasks
 * will be run after isc_task_endexclusive() is called.
 *
 * Requires:
 *
 *\li	'manager' is a valid task manager.
 */

void
isc_taskmgr_setexcltask(isc_taskmgr_t *mgr, isc_task_t *task);
/*%<
 * Set a task which will be used for all task-exclusive operations.
 *
 * Requires:
 *\li	'manager' is a valid task manager.
 *
 *\li	'task' is a valid task.
 */

isc_result_t
isc_taskmgr_excltask(isc_taskmgr_t *mgr, isc_task_t **taskp);
/*%<
 * Attach '*taskp' to the task set by isc_taskmgr_getexcltask().
 * This task should be used whenever running in task-exclusive mode,
 * so as to prevent deadlock between two exclusive tasks.
 *
 * Requires:
 *\li	'manager' is a valid task manager.
 *
 *\li	taskp != NULL && *taskp == NULL
 */

#ifdef HAVE_LIBXML2
int
isc_taskmgr_renderxml(isc_taskmgr_t *mgr, void *writer0);
#endif /* ifdef HAVE_LIBXML2 */

#ifdef HAVE_JSON_C
isc_result_t
isc_taskmgr_renderjson(isc_taskmgr_t *mgr, void *tasksobj0);
#endif /* HAVE_JSON_C */

ISC_LANG_ENDDECLS
