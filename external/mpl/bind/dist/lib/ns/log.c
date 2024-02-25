/*	$NetBSD: log.c,v 1.6.2.1 2024/02/25 15:47:34 martin Exp $	*/

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

#include <isc/result.h>
#include <isc/util.h>

#include <ns/log.h>

#ifndef ISC_FACILITY
#define ISC_FACILITY LOG_DAEMON
#endif /* ifndef ISC_FACILITY */

/*%
 * When adding a new category, be sure to add the appropriate
 * \#define to <ns/log.h>
 */
isc_logcategory_t ns_categories[] = { { "client", 0 },
				      { "network", 0 },
				      { "update", 0 },
				      { "queries", 0 },
				      { "update-security", 0 },
				      { "query-errors", 0 },
				      { "trust-anchor-telemetry", 0 },
				      { "serve-stale", 0 },
				      { NULL, 0 } };

/*%
 * When adding a new module, be sure to add the appropriate
 * \#define to <ns/log.h>.
 */
isc_logmodule_t ns_modules[] = {
	{ "ns/client", 0 }, { "ns/query", 0 },	 { "ns/interfacemgr", 0 },
	{ "ns/update", 0 }, { "ns/xfer-in", 0 }, { "ns/xfer-out", 0 },
	{ "ns/notify", 0 }, { "ns/hooks", 0 },	 { NULL, 0 }
};

isc_log_t *ns_lctx = NULL;

void
ns_log_init(isc_log_t *lctx) {
	REQUIRE(lctx != NULL);

	isc_log_registercategories(lctx, ns_categories);
	isc_log_registermodules(lctx, ns_modules);
}

void
ns_log_setcontext(isc_log_t *lctx) {
	ns_lctx = lctx;
}
