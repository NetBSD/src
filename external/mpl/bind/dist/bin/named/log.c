/*	$NetBSD: log.c,v 1.6 2022/09/23 12:15:21 christos Exp $	*/

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

#include <dns/log.h>

#include <isccfg/log.h>

#include <ns/log.h>

#include <named/log.h>

#ifndef ISC_FACILITY
#define ISC_FACILITY LOG_DAEMON
#endif /* ifndef ISC_FACILITY */

/*%
 * When adding a new category, be sure to add the appropriate
 * \#define to <named/log.h> and to update the list in
 * bin/check/check-tool.c.
 */
static isc_logcategory_t categories[] = { { "", 0 },
					  { "unmatched", 0 },
					  { NULL, 0 } };

/*%
 * When adding a new module, be sure to add the appropriate
 * \#define to <dns/log.h>.
 */
static isc_logmodule_t modules[] = {
	{ "main", 0 }, { "server", 0 }, { "control", 0 }, { NULL, 0 }
};

isc_result_t
named_log_init(bool safe) {
	isc_result_t result;
	isc_logconfig_t *lcfg = NULL;

	named_g_categories = categories;
	named_g_modules = modules;

	/*
	 * Setup a logging context.
	 */
	isc_log_create(named_g_mctx, &named_g_lctx, &lcfg);

	/*
	 * named-checktool.c:setup_logging() needs to be kept in sync.
	 */
	isc_log_registercategories(named_g_lctx, named_g_categories);
	isc_log_registermodules(named_g_lctx, named_g_modules);
	isc_log_setcontext(named_g_lctx);
	dns_log_init(named_g_lctx);
	dns_log_setcontext(named_g_lctx);
	cfg_log_init(named_g_lctx);
	ns_log_init(named_g_lctx);
	ns_log_setcontext(named_g_lctx);

	if (safe) {
		named_log_setsafechannels(lcfg);
	} else {
		named_log_setdefaultchannels(lcfg);
	}

	result = named_log_setdefaultcategory(lcfg);
	if (result != ISC_R_SUCCESS) {
		goto cleanup;
	}

	return (ISC_R_SUCCESS);

cleanup:
	isc_log_destroy(&named_g_lctx);
	isc_log_setcontext(NULL);
	dns_log_setcontext(NULL);

	return (result);
}

void
named_log_setdefaultchannels(isc_logconfig_t *lcfg) {
	isc_logdestination_t destination;

	/*
	 * By default, the logging library makes "default_debug" log to
	 * stderr.  In BIND, we want to override this and log to named.run
	 * instead, unless the -g option was given.
	 */
	if (!named_g_logstderr) {
		destination.file.stream = NULL;
		destination.file.name = "named.run";
		destination.file.versions = ISC_LOG_ROLLNEVER;
		destination.file.maximum_size = 0;
		isc_log_createchannel(lcfg, "default_debug", ISC_LOG_TOFILE,
				      ISC_LOG_DYNAMIC, &destination,
				      ISC_LOG_PRINTTIME | ISC_LOG_DEBUGONLY);
	}

	if (named_g_logfile != NULL) {
		destination.file.stream = NULL;
		destination.file.name = named_g_logfile;
		destination.file.versions = ISC_LOG_ROLLNEVER;
		destination.file.maximum_size = 0;
		isc_log_createchannel(lcfg, "default_logfile", ISC_LOG_TOFILE,
				      ISC_LOG_DYNAMIC, &destination,
				      ISC_LOG_PRINTTIME |
					      ISC_LOG_PRINTCATEGORY |
					      ISC_LOG_PRINTLEVEL);
	}

#if ISC_FACILITY != LOG_DAEMON
	destination.facility = ISC_FACILITY;
	isc_log_createchannel(lcfg, "default_syslog", ISC_LOG_TOSYSLOG,
			      ISC_LOG_INFO, &destination, 0);
#endif /* if ISC_FACILITY != LOG_DAEMON */

	/*
	 * Set the initial debug level.
	 */
	isc_log_setdebuglevel(named_g_lctx, named_g_debuglevel);
}

void
named_log_setsafechannels(isc_logconfig_t *lcfg) {
	isc_logdestination_t destination;

	if (!named_g_logstderr) {
		isc_log_createchannel(lcfg, "default_debug", ISC_LOG_TONULL,
				      ISC_LOG_DYNAMIC, NULL, 0);

		/*
		 * Setting the debug level to zero should get the output
		 * discarded a bit faster.
		 */
		isc_log_setdebuglevel(named_g_lctx, 0);
	} else {
		isc_log_setdebuglevel(named_g_lctx, named_g_debuglevel);
	}

	if (named_g_logfile != NULL) {
		destination.file.stream = NULL;
		destination.file.name = named_g_logfile;
		destination.file.versions = ISC_LOG_ROLLNEVER;
		destination.file.maximum_size = 0;
		isc_log_createchannel(lcfg, "default_logfile", ISC_LOG_TOFILE,
				      ISC_LOG_DYNAMIC, &destination,
				      ISC_LOG_PRINTTIME |
					      ISC_LOG_PRINTCATEGORY |
					      ISC_LOG_PRINTLEVEL);
	}

#if ISC_FACILITY != LOG_DAEMON
	destination.facility = ISC_FACILITY;
	isc_log_createchannel(lcfg, "default_syslog", ISC_LOG_TOSYSLOG,
			      ISC_LOG_INFO, &destination, 0);
#endif /* if ISC_FACILITY != LOG_DAEMON */
}

isc_result_t
named_log_setdefaultcategory(isc_logconfig_t *lcfg) {
	isc_result_t result = ISC_R_SUCCESS;

	result = isc_log_usechannel(lcfg, "default_debug",
				    ISC_LOGCATEGORY_DEFAULT, NULL);
	if (result != ISC_R_SUCCESS) {
		goto cleanup;
	}

	if (!named_g_logstderr) {
		if (named_g_logfile != NULL) {
			result = isc_log_usechannel(lcfg, "default_logfile",
						    ISC_LOGCATEGORY_DEFAULT,
						    NULL);
		} else if (!named_g_nosyslog) {
			result = isc_log_usechannel(lcfg, "default_syslog",
						    ISC_LOGCATEGORY_DEFAULT,
						    NULL);
		}
	}

cleanup:
	return (result);
}

isc_result_t
named_log_setunmatchedcategory(isc_logconfig_t *lcfg) {
	isc_result_t result;

	result = isc_log_usechannel(lcfg, "null", NAMED_LOGCATEGORY_UNMATCHED,
				    NULL);
	return (result);
}

void
named_log_shutdown(void) {
	isc_log_destroy(&named_g_lctx);
	isc_log_setcontext(NULL);
	dns_log_setcontext(NULL);
}
