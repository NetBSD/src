/*	$NetBSD: app.h,v 1.7.2.1 2024/02/25 15:47:19 martin Exp $	*/

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

/*! \file isc/app.h
 * \brief ISC Application Support
 *
 * Dealing with program termination can be difficult, especially in a
 * multithreaded program.  The routines in this module help coordinate
 * the shutdown process.  They are used as follows by the initial (main)
 * thread of the application:
 *
 *\li		isc_app_start();	Call very early in main(), before
 *					any other threads have been created.
 *
 *\li		isc_app_run();		This will post any on-run events,
 *					and then block until application
 *					shutdown is requested.  A shutdown
 *					request is made by calling
 *					isc_app_shutdown(), or by sending
 *					SIGINT or SIGTERM to the process.
 *					After isc_app_run() returns, the
 *					application should shutdown itself.
 *
 *\li		isc_app_finish();	Call very late in main().
 *
 * Applications that want to use SIGHUP/isc_app_reload() to trigger reloading
 * should check the result of isc_app_run() and call the reload routine if
 * the result is ISC_R_RELOAD.  They should then call isc_app_run() again
 * to resume waiting for reload or termination.
 *
 * Use of this module is not required.  In particular, isc_app_start() is
 * NOT an ISC library initialization routine.
 *
 * This module also supports per-thread 'application contexts'.  With this
 * mode, a thread-based application will have a separate context, in which
 * it uses other ISC library services such as tasks or timers.  Signals are
 * not caught in this mode, so that the application can handle the signals
 * in its preferred way.
 *
 * \li MP:
 *	Clients must ensure that isc_app_start(), isc_app_run(), and
 *	isc_app_finish() are called at most once.  isc_app_shutdown()
 *	is safe to use by any thread (provided isc_app_start() has been
 *	called previously).
 *
 *	The same note applies to isc_app_ctxXXX() functions, but in this case
 *	it's a per-thread restriction.  For example, a thread with an
 *	application context must ensure that isc_app_ctxstart() with the
 *	context is called at most once.
 *
 * \li Reliability:
 *	No anticipated impact.
 *
 * \li Resources:
 *	None.
 *
 * \li Security:
 *	No anticipated impact.
 *
 * \li Standards:
 *	None.
 */

#include <stdbool.h>

#include <isc/eventclass.h>
#include <isc/lang.h>
#include <isc/magic.h>
#include <isc/result.h>
#include <isc/types.h>

/***
 *** Types
 ***/

typedef isc_event_t isc_appevent_t;

#define ISC_APPEVENT_FIRSTEVENT (ISC_EVENTCLASS_APP + 0)
#define ISC_APPEVENT_SHUTDOWN	(ISC_EVENTCLASS_APP + 1)
#define ISC_APPEVENT_LASTEVENT	(ISC_EVENTCLASS_APP + 65535)

ISC_LANG_BEGINDECLS

isc_result_t
isc_app_ctxstart(isc_appctx_t *ctx);

isc_result_t
isc_app_start(void);
/*!<
 * \brief Start an ISC library application.
 *
 * Notes:
 *	This call should be made before any other ISC library call, and as
 *	close to the beginning of the application as possible.
 *
 * Requires:
 *\li	'ctx' is a valid application context (for app_ctxstart()).
 */

isc_result_t
isc_app_ctxonrun(isc_appctx_t *ctx, isc_mem_t *mctx, isc_task_t *task,
		 isc_taskaction_t action, void *arg);
isc_result_t
isc_app_onrun(isc_mem_t *mctx, isc_task_t *task, isc_taskaction_t action,
	      void *arg);
/*!<
 * \brief Request delivery of an event when the application is run.
 *
 * Requires:
 *\li	isc_app_start() has been called.
 *\li	'ctx' is a valid application context (for app_ctxonrun()).
 *
 * Returns:
 *	ISC_R_SUCCESS
 *	ISC_R_NOMEMORY
 */

isc_result_t
isc_app_ctxrun(isc_appctx_t *ctx);

isc_result_t
isc_app_run(void);
/*!<
 * \brief Run an ISC library application.
 *
 * Notes:
 *\li	The caller (typically the initial thread of an application) will
 *	block until shutdown is requested.  When the call returns, the
 *	caller should start shutting down the application.
 *
 * Requires:
 *\li	isc_app_[ctx]start() has been called.
 *
 * Ensures:
 *\li	Any events requested via isc_app_onrun() will have been posted (in
 *	FIFO order) before isc_app_run() blocks.
 *\li	'ctx' is a valid application context (for app_ctxrun()).
 *
 * Returns:
 *\li	ISC_R_SUCCESS			Shutdown has been requested.
 *\li	ISC_R_RELOAD			Reload has been requested.
 */

bool
isc_app_isrunning(void);
/*!<
 * \brief Return if the ISC library application is running.
 *
 * Returns:
 *\li	true    App is running.
 *\li	false   App is not running.
 */

void
isc_app_ctxshutdown(isc_appctx_t *ctx);

void
isc_app_shutdown(void);
/*!<
 * \brief Request application shutdown.
 *
 * Notes:
 *\li	It is safe to call isc_app_shutdown() multiple times.  Shutdown will
 *	only be triggered once.
 *
 * Requires:
 *\li	isc_app_[ctx]run() has been called.
 *\li	'ctx' is a valid application context (for app_ctxshutdown()).
 *
 * Returns:
 *\li	ISC_R_SUCCESS
 *\li	ISC_R_UNEXPECTED
 */

void
isc_app_ctxsuspend(isc_appctx_t *ctx);
/*!<
 * \brief This has the same behavior as isc_app_ctxsuspend().
 */

void
isc_app_reload(void);
/*!<
 * \brief Request application reload.
 *
 * Requires:
 *\li	isc_app_run() has been called.
 *
 * Returns:
 *\li	ISC_R_SUCCESS
 *\li	ISC_R_UNEXPECTED
 */

void
isc_app_ctxfinish(isc_appctx_t *ctx);

void
isc_app_finish(void);
/*!<
 * \brief Finish an ISC library application.
 *
 * Notes:
 *\li	This call should be made at or near the end of main().
 *
 * Requires:
 *\li	isc_app_start() has been called.
 *\li	'ctx' is a valid application context (for app_ctxfinish()).
 *
 * Ensures:
 *\li	Any resources allocated by isc_app_start() have been released.
 */

void
isc_app_block(void);
/*!<
 * \brief Indicate that a blocking operation will be performed.
 *
 * Notes:
 *\li	If a blocking operation is in process, a call to isc_app_shutdown()
 *	or an external signal will abort the program, rather than allowing
 *	clean shutdown.  This is primarily useful for reading user input.
 *
 * Requires:
 * \li	isc_app_start() has been called.
 * \li	No other blocking operations are in progress.
 */

void
isc_app_unblock(void);
/*!<
 * \brief Indicate that a blocking operation is complete.
 *
 * Notes:
 * \li	When a blocking operation has completed, return the program to a
 * 	state where a call to isc_app_shutdown() or an external signal will
 * 	shutdown normally.
 *
 * Requires:
 * \li	isc_app_start() has been called.
 * \li	isc_app_block() has been called by the same thread.
 */

isc_result_t
isc_appctx_create(isc_mem_t *mctx, isc_appctx_t **ctxp);
/*!<
 * \brief Create an application context.
 *
 * Requires:
 *\li	'mctx' is a valid memory context.
 *\li	'ctxp' != NULL && *ctxp == NULL.
 */

void
isc_appctx_destroy(isc_appctx_t **ctxp);
/*!<
 * \brief Destroy an application context.
 *
 * Requires:
 *\li	'*ctxp' is a valid application context.
 *
 * Ensures:
 *\li	*ctxp == NULL.
 */

ISC_LANG_ENDDECLS
