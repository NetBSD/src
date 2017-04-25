/*	$NetBSD: task_api.c,v 1.2.6.1.4.2 2017/04/25 22:01:58 snj Exp $	*/

/*
 * Copyright (C) 2009-2012, 2014  Internet Systems Consortium, Inc. ("ISC")
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND ISC DISCLAIMS ALL WARRANTIES WITH
 * REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS.  IN NO EVENT SHALL ISC BE LIABLE FOR ANY SPECIAL, DIRECT,
 * INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
 * LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE
 * OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */

/* Id */

#include <config.h>

#include <unistd.h>

#include <isc/app.h>
#include <isc/magic.h>
#include <isc/mutex.h>
#include <isc/once.h>
#include <isc/task.h>
#include <isc/util.h>

static isc_mutex_t createlock;
static isc_once_t once = ISC_ONCE_INIT;
static isc_taskmgrcreatefunc_t taskmgr_createfunc = NULL;

static void
initialize(void) {
	RUNTIME_CHECK(isc_mutex_init(&createlock) == ISC_R_SUCCESS);
}

isc_result_t
isc_task_register(isc_taskmgrcreatefunc_t createfunc) {
	isc_result_t result = ISC_R_SUCCESS;

	RUNTIME_CHECK(isc_once_do(&once, initialize) == ISC_R_SUCCESS);

	LOCK(&createlock);
	if (taskmgr_createfunc == NULL)
		taskmgr_createfunc = createfunc;
	else
		result = ISC_R_EXISTS;
	UNLOCK(&createlock);

	return (result);
}

isc_result_t
isc_taskmgr_createinctx(isc_mem_t *mctx, isc_appctx_t *actx,
			unsigned int workers, unsigned int default_quantum,
			isc_taskmgr_t **managerp)
{
	isc_result_t result;

	LOCK(&createlock);

	REQUIRE(taskmgr_createfunc != NULL);
	result = (*taskmgr_createfunc)(mctx, workers, default_quantum,
				       managerp);

	UNLOCK(&createlock);

	if (result == ISC_R_SUCCESS)
		isc_appctx_settaskmgr(actx, *managerp);

	return (result);
}

isc_result_t
isc_taskmgr_create(isc_mem_t *mctx, unsigned int workers,
		   unsigned int default_quantum, isc_taskmgr_t **managerp)
{
	isc_result_t result;

	if (isc_bind9)
		return (isc__taskmgr_create(mctx, workers,
			 		    default_quantum, managerp));
	LOCK(&createlock);

	REQUIRE(taskmgr_createfunc != NULL);
	result = (*taskmgr_createfunc)(mctx, workers, default_quantum,
				       managerp);

	UNLOCK(&createlock);

	return (result);
}

void
isc_taskmgr_destroy(isc_taskmgr_t **managerp) {
	REQUIRE(managerp != NULL && ISCAPI_TASKMGR_VALID(*managerp));

	if (isc_bind9)
		isc__taskmgr_destroy(managerp);
	else
		(*managerp)->methods->destroy(managerp);

	ENSURE(*managerp == NULL);
}

void
isc_taskmgr_setmode(isc_taskmgr_t *manager, isc_taskmgrmode_t mode) {
	REQUIRE(ISCAPI_TASKMGR_VALID(manager));

	if (isc_bind9)
		isc__taskmgr_setmode(manager, mode);
	else
		manager->methods->setmode(manager, mode);
}

isc_taskmgrmode_t
isc_taskmgr_mode(isc_taskmgr_t *manager) {
	REQUIRE(ISCAPI_TASKMGR_VALID(manager));

	if (isc_bind9)
		return (isc__taskmgr_mode(manager));

	return (manager->methods->mode(manager));
}

isc_result_t
isc_task_create(isc_taskmgr_t *manager, unsigned int quantum,
		isc_task_t **taskp)
{
	REQUIRE(ISCAPI_TASKMGR_VALID(manager));
	REQUIRE(taskp != NULL && *taskp == NULL);

	if (isc_bind9)
		return (isc__task_create(manager, quantum, taskp));

	return (manager->methods->taskcreate(manager, quantum, taskp));
}

void
isc_task_attach(isc_task_t *source, isc_task_t **targetp) {
	REQUIRE(ISCAPI_TASK_VALID(source));
	REQUIRE(targetp != NULL && *targetp == NULL);

	if (isc_bind9)
		isc__task_attach(source, targetp);
	else
		source->methods->attach(source, targetp);

	ENSURE(*targetp == source);
}

void
isc_task_detach(isc_task_t **taskp) {
	REQUIRE(taskp != NULL && ISCAPI_TASK_VALID(*taskp));

	if (isc_bind9)
		isc__task_detach(taskp);
	else
		(*taskp)->methods->detach(taskp);

	ENSURE(*taskp == NULL);
}

void
isc_task_send(isc_task_t *task, isc_event_t **eventp) {
	REQUIRE(ISCAPI_TASK_VALID(task));
	REQUIRE(eventp != NULL && *eventp != NULL);

	if (isc_bind9)
		isc__task_send(task, eventp);
	else {
		task->methods->send(task, eventp);
		ENSURE(*eventp == NULL);
	}
}

void
isc_task_sendanddetach(isc_task_t **taskp, isc_event_t **eventp) {
	REQUIRE(taskp != NULL && ISCAPI_TASK_VALID(*taskp));
	REQUIRE(eventp != NULL && *eventp != NULL);

	if (isc_bind9)
		isc__task_sendanddetach(taskp, eventp);
	else {
		(*taskp)->methods->sendanddetach(taskp, eventp);
		ENSURE(*eventp == NULL);
	}

	ENSURE(*taskp == NULL);
}

unsigned int
isc_task_unsend(isc_task_t *task, void *sender, isc_eventtype_t type,
		void *tag, isc_eventlist_t *events)
{
	REQUIRE(ISCAPI_TASK_VALID(task));

	if (isc_bind9)
		return (isc__task_unsend(task, sender, type, tag, events));

	return (task->methods->unsend(task, sender, type, tag, events));
}

isc_result_t
isc_task_onshutdown(isc_task_t *task, isc_taskaction_t action, void *arg)
{
	REQUIRE(ISCAPI_TASK_VALID(task));

	if (isc_bind9)
		return (isc__task_onshutdown(task, action, arg));

	return (task->methods->onshutdown(task, action, arg));
}

void
isc_task_shutdown(isc_task_t *task) {
	REQUIRE(ISCAPI_TASK_VALID(task));

	if (isc_bind9)
		isc__task_shutdown(task);
	else
		task->methods->shutdown(task);
}

void
isc_task_setname(isc_task_t *task, const char *name, void *tag) {
	REQUIRE(ISCAPI_TASK_VALID(task));

	if (isc_bind9)
		isc__task_setname(task, name, tag);
	else
		task->methods->setname(task, name, tag);
}

unsigned int
isc_task_purge(isc_task_t *task, void *sender, isc_eventtype_t type, void *tag)
{
	REQUIRE(ISCAPI_TASK_VALID(task));

	if (isc_bind9)
		return (isc__task_purge(task, sender, type, tag));

	return (task->methods->purgeevents(task, sender, type, tag));
}

void
isc_taskmgr_setexcltask(isc_taskmgr_t *mgr, isc_task_t *task) {
	REQUIRE(ISCAPI_TASK_VALID(task));

	if (isc_bind9)
		isc__taskmgr_setexcltask(mgr, task);
	else
		mgr->methods->setexcltask(mgr, task);
}

isc_result_t
isc_taskmgr_excltask(isc_taskmgr_t *mgr, isc_task_t **taskp) {
	if (isc_bind9)
		return (isc__taskmgr_excltask(mgr, taskp));
	else
		return (mgr->methods->excltask(mgr, taskp));
}

isc_result_t
isc_task_beginexclusive(isc_task_t *task) {
	REQUIRE(ISCAPI_TASK_VALID(task));

	if (isc_bind9)
		return (isc__task_beginexclusive(task));

	return (task->methods->beginexclusive(task));
}

void
isc_task_endexclusive(isc_task_t *task) {
	REQUIRE(ISCAPI_TASK_VALID(task));

	if (isc_bind9)
		isc__task_endexclusive(task);
	else
		task->methods->endexclusive(task);
}

void
isc_task_setprivilege(isc_task_t *task, isc_boolean_t priv) {
	REQUIRE(ISCAPI_TASK_VALID(task));

	if (isc_bind9)
		isc__task_setprivilege(task, priv);
	else
		task->methods->setprivilege(task, priv);
}

isc_boolean_t
isc_task_privilege(isc_task_t *task) {
	REQUIRE(ISCAPI_TASK_VALID(task));

	if (isc_bind9)
		return (isc__task_privilege(task));

	return (task->methods->privilege(task));
}


/*%
 * This is necessary for libisc's internal timer implementation.  Other
 * implementation might skip implementing this.
 */
unsigned int
isc_task_purgerange(isc_task_t *task, void *sender, isc_eventtype_t first,
		    isc_eventtype_t last, void *tag)
{
	REQUIRE(ISCAPI_TASK_VALID(task));

	if (isc_bind9)
		return (isc__task_purgerange(task, sender, first, last, tag));

	return (task->methods->purgerange(task, sender, first, last, tag));
}

void
isc_task_getcurrenttime(isc_task_t *task, isc_stdtime_t *t) {
	if (!isc_bind9)
		return;

	isc__task_getcurrenttime(task, t);
}

void
isc_task_destroy(isc_task_t **taskp) {
	if (!isc_bind9)
		return;

	isc__task_destroy(taskp);
}

isc_boolean_t
isc_task_purgeevent(isc_task_t *task0, isc_event_t *event) {
	if (!isc_bind9)
		return (ISC_FALSE);

	return (isc__task_purgeevent(task0, event));
}

unsigned int
isc_task_unsendrange(isc_task_t *task, void *sender, isc_eventtype_t first, 
                      isc_eventtype_t last, void *tag,
                      isc_eventlist_t *events) {
	if (!isc_bind9)
		return (0);


	return (isc__task_unsendrange(task, sender, first, last, tag, events));
}

const char *
isc_task_getname(isc_task_t *task0) {
	if (!isc_bind9)
		return ("");

	return (isc__task_getname(task0));
}

void *
isc_task_gettag(isc_task_t *task0) {
	if (!isc_bind9)
		return NULL;

	return (isc__task_gettag(task0));
}

