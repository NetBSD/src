/*	$NetBSD: log.h,v 1.6.2.1 2024/02/25 15:43:07 martin Exp $	*/

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

/*! \file */

#include <isc/log.h>
#include <isc/types.h>

#include <dns/log.h>

#include <named/globals.h> /* Required for named_g_(categories|modules). */

/* Unused slot 0. */
#define NAMED_LOGCATEGORY_UNMATCHED (&named_g_categories[1])

/*
 * Backwards compatibility.
 */
#define NAMED_LOGCATEGORY_GENERAL ISC_LOGCATEGORY_GENERAL

#define NAMED_LOGMODULE_MAIN	(&named_g_modules[0])
#define NAMED_LOGMODULE_SERVER	(&named_g_modules[1])
#define NAMED_LOGMODULE_CONTROL (&named_g_modules[2])

isc_result_t
named_log_init(bool safe);
/*%
 * Initialize the logging system and set up an initial default
 * logging default configuration that will be used until the
 * config file has been read.
 *
 * If 'safe' is true, use a default configuration that refrains
 * from opening files.  This is to avoid creating log files
 * as root.
 */

void
named_log_setdefaultchannels(isc_logconfig_t *lcfg);
/*%
 * Set up logging channels according to the named defaults, which
 * may differ from the logging library defaults.  Currently,
 * this just means setting up default_debug.
 */

void
named_log_setsafechannels(isc_logconfig_t *lcfg);
/*%
 * Like named_log_setdefaultchannels(), but omits any logging to files.
 */

void
named_log_setdefaultsslkeylogfile(isc_logconfig_t *lcfg);
/*%
 * If the SSLKEYLOGFILE environment variable is set, sets up a default
 * logging channel for writing TLS pre-master secrets to the path stored
 * in that environment variable (for debugging purposes).
 */

isc_result_t
named_log_setdefaultcategory(isc_logconfig_t *lcfg);
/*%
 * Set up "category default" to go to the right places.
 */

isc_result_t
named_log_setunmatchedcategory(isc_logconfig_t *lcfg);
/*%
 * Set up "category unmatched" to go to the right places.
 */

void
named_log_shutdown(void);
