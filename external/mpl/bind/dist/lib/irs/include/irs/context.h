/*	$NetBSD: context.h,v 1.2 2018/08/12 13:02:37 christos Exp $	*/

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


#ifndef IRS_CONTEXT_H
#define IRS_CONTEXT_H 1

/*! \file
 *
 * \brief
 * The IRS context module provides an abstract interface to the DNS library
 * with an application.  An IRS context object initializes and holds various
 * resources used in the DNS library.
 */

#include <dns/types.h>
#include <irs/types.h>

ISC_LANG_BEGINDECLS

isc_result_t
irs_context_create(irs_context_t **contextp);
/*%<
 * Create an IRS context.  It internally initializes the ISC and DNS libraries
 * (if not yet), creates a DNS client object and initializes the client using
 * the configuration files parsed via the 'resconf' and 'dnsconf' IRS modules.
 * Some of the internally initialized objects can be used by the application
 * via irs_context_getxxx() functions (see below).
 *
 * Requires:
 *
 *\li	contextp != NULL && *contextp == NULL.
 */

isc_result_t
irs_context_get(irs_context_t **contextp);
/*%<
 * Return an IRS context for the calling thread.  If no IRS context is
 * associated to the thread, this function creates a new one by calling
 * irs_context_create(), and associates it with the thread as a thread specific
 * data value.  This function is provided for standard libraries that are
 * expected to be thread-safe but do not accept an appropriate IRS context
 * as a library parameter, e.g., getaddrinfo().
 *
 * Requires:
 *
 *\li	contextp != NULL && *contextp == NULL.
 */

void
irs_context_destroy(irs_context_t **contextp);
/*%<
 * Destroy an IRS context.
 *
 * Requires:
 *
 *\li	'*contextp' is a valid IRS context.
 *
 * Ensures:
 *\li	'*contextp' == NULL.
 */

isc_mem_t *
irs_context_getmctx(irs_context_t *context);
/*%<
 * Return the memory context held in the context.
 *
 * Requires:
 *
 *\li	'context' is a valid IRS context.
 */

isc_appctx_t *
irs_context_getappctx(irs_context_t *context);
/*%<
 * Return the application context held in the context.
 *
 * Requires:
 *
 *\li	'context' is a valid IRS context.
 */

isc_taskmgr_t *
irs_context_gettaskmgr(irs_context_t *context);
/*%<
 * Return the task manager held in the context.
 *
 * Requires:
 *
 *\li	'context' is a valid IRS context.
 */

isc_timermgr_t *
irs_context_gettimermgr(irs_context_t *context);
/*%<
 * Return the timer manager held in the context.
 *
 * Requires:
 *
 *\li	'context' is a valid IRS context.
 */

isc_task_t *
irs_context_gettask(irs_context_t *context);
/*%<
 * Return the task object held in the context.
 *
 * Requires:
 *
 *\li	'context' is a valid IRS context.
 */

dns_client_t *
irs_context_getdnsclient(irs_context_t *context);
/*%<
 * Return the DNS client object held in the context.
 *
 * Requires:
 *
 *\li	'context' is a valid IRS context.
 */

irs_resconf_t *
irs_context_getresconf(irs_context_t *context);
/*%<
 * Return the resolver configuration object held in the context.
 *
 * Requires:
 *
 *\li	'context' is a valid IRS context.
 */

irs_dnsconf_t *
irs_context_getdnsconf(irs_context_t *context);
/*%<
 * Return the advanced DNS configuration object held in the context.
 *
 * Requires:
 *
 *\li	'context' is a valid IRS context.
 */

ISC_LANG_ENDDECLS

#endif /* IRS_CONTEXT_H */
