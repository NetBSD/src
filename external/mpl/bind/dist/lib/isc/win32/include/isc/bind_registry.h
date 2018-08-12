/*	$NetBSD: bind_registry.h,v 1.2 2018/08/12 13:02:40 christos Exp $	*/

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


#ifndef ISC_BINDREGISTRY_H
#define ISC_BINDREGISTRY_H

/*
 * BIND makes use of the following Registry keys in various places, especially
 * during startup and installation
 */

#define BIND_SUBKEY		"Software\\ISC\\BIND"
#define BIND_SESSION		"CurrentSession"
#define BIND_SESSION_SUBKEY	"Software\\ISC\\BIND\\CurrentSession"
#define BIND_UNINSTALL_SUBKEY	\
	"Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\ISC BIND"

#define EVENTLOG_APP_SUBKEY	\
	"SYSTEM\\CurrentControlSet\\Services\\EventLog\\Application"
#define BIND_MESSAGE_SUBKEY	\
	"SYSTEM\\CurrentControlSet\\Services\\EventLog\\Application\\named"
#define BIND_MESSAGE_NAME	"named"

#define BIND_SERVICE_SUBKEY	\
	"SYSTEM\\CurrentControlSet\\Services\\named"


#define BIND_CONFIGFILE		0
#define BIND_DEBUGLEVEL		1
#define BIND_QUERYLOG		2
#define BIND_FOREGROUND		3
#define BIND_PORT		4

#endif /* ISC_BINDREGISTRY_H */
