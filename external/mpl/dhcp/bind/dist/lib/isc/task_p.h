/*	$NetBSD: task_p.h,v 1.1.2.2 2024/02/24 13:07:22 martin Exp $	*/

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

#include <isc/mem.h>
#include <isc/result.h>
#include <isc/task.h>

isc_result_t
isc__taskmgr_create(isc_mem_t *mctx, unsigned int default_quantum, isc_nm_t *nm,
		    isc_taskmgr_t **managerp);
/*%<
 * Create a new task manager.
 *
 * Notes:
 *
 *\li	If 'default_quantum' is non-zero, then it will be used as the default
 *	quantum value when tasks are created.  If zero, then an implementation
 *	defined default quantum will be used.
 *
 *\li	If 'nm' is set then netmgr is paused when an exclusive task mode
 *	is requested.
 *
 * Requires:
 *
 *\li      'mctx' is a valid memory context.
 *
 *\li	managerp != NULL && *managerp == NULL
 *
 * Ensures:
 *
 *\li	On success, '*managerp' will be attached to the newly created task
 *	manager.
 *
 * Returns:
 *
 *\li	#ISC_R_SUCCESS
 *\li	#ISC_R_NOMEMORY
 *\li	#ISC_R_NOTHREADS		No threads could be created.
 *\li	#ISC_R_UNEXPECTED		An unexpected error occurred.
 *\li	#ISC_R_SHUTTINGDOWN		The non-threaded, shared, task
 *					manager shutting down.
 */

void
isc__taskmgr_destroy(isc_taskmgr_t **managerp);
/*%<
 * Destroy '*managerp'.
 *
 * Notes:
 *
 *\li	Calling isc_taskmgr_destroy() will shutdown all tasks managed by
 *	*managerp that haven't already been shutdown.  The call will block
 *	until all tasks have entered the done state.
 *
 *\li	isc_taskmgr_destroy() must not be called by a task event action,
 *	because it would block forever waiting for the event action to
 *	complete.  An event action that wants to cause task manager shutdown
 *	should request some non-event action thread of execution to do the
 *	shutdown, e.g. by signaling a condition variable or using
 *	isc_app_shutdown().
 *
 *\li	Task manager references are not reference counted, so the caller
 *	must ensure that no attempt will be made to use the manager after
 *	isc_taskmgr_destroy() returns.
 *
 * Requires:
 *
 *\li	'*managerp' is a valid task manager.
 *
 *\li   'isc__taskmgr_shutdown()' and isc__netmgr_shutdown() have been
 *	called.
 */

void
isc__taskmgr_shutdown(isc_taskmgr_t *manager);
/*%>
 * Shutdown 'manager'.
 *
 * Notes:
 *
 *\li	Calling isc__taskmgr_shutdown() will shut down all tasks managed by
 *	*managerp that haven't already been shut down.
 *
 * Requires:
 *
 *\li   'manager' is a valid task manager.
 *
 *\li	isc_taskmgr_destroy() has not be called previously on '*managerp'.
 *
 * Ensures:
 *
 *\li	All resources used by the task manager, and any tasks it managed,
 *	have been freed.
 */
