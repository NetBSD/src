/*	$NetBSD: ntpaths.h,v 1.1.1.5 2022/09/23 12:09:23 christos Exp $	*/

/*
 * Copyright (C) Internet Systems Consortium, Inc. ("ISC")
 *
 * SPDX-License-Identifier: MPL-2.0
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0.  If a copy of the MPL was not distributed with this
 * file, you can obtain one at https://mozilla.org/MPL/2.0/.
 *
 * See the COPYRIGHT file distributed with this work for additional
 * information regarding copyright ownership.
 */

/*
 * Windows-specific path definitions
 * These routines are used to set up and return system-specific path
 * information about the files enumerated in NtPaths
 */

#ifndef ISC_NTPATHS_H
#define ISC_NTPATHS_H

#include <isc/lang.h>

/*
 * Index of paths needed
 */
enum NtPaths {
	NAMED_CONF_PATH,
	RESOLV_CONF_PATH,
	RNDC_CONF_PATH,
	NAMED_PID_PATH,
	NAMED_LOCK_PATH,
	LOCAL_STATE_DIR,
	SYS_CONF_DIR,
	RNDC_KEY_PATH,
	SESSION_KEY_PATH,
	BIND_KEYS_PATH
};

/*
 * Define macros to get the path of the config files
 */
#define NAMED_CONFFILE	isc_ntpaths_get(NAMED_CONF_PATH)
#define RNDC_CONFFILE	isc_ntpaths_get(RNDC_CONF_PATH)
#define RNDC_KEYFILE	isc_ntpaths_get(RNDC_KEY_PATH)
#define SESSION_KEYFILE isc_ntpaths_get(SESSION_KEY_PATH)
#define RESOLV_CONF	isc_ntpaths_get(RESOLV_CONF_PATH)

/*
 * Information about where the files are on disk
 */
#define NAMED_LOCALSTATEDIR "/dns/bin"
#define NAMED_SYSCONFDIR    "/dns/etc"

ISC_LANG_BEGINDECLS

void
isc_ntpaths_init(void);

char *
isc_ntpaths_get(int);

ISC_LANG_ENDDECLS

#endif /* ISC_NTPATHS_H */
