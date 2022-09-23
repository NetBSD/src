/*	$NetBSD: syslog.h,v 1.1.1.4 2022/09/23 12:09:23 christos Exp $	*/

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

#ifndef _SYSLOG_H
#define _SYSLOG_H

#include <stdio.h>

/* Constant definitions for openlog() */
#define LOG_PID	 1
#define LOG_CONS 2
/* NT event log does not support facility level */
#define LOG_KERN   0
#define LOG_USER   0
#define LOG_MAIL   0
#define LOG_DAEMON 0
#define LOG_AUTH   0
#define LOG_SYSLOG 0
#define LOG_LPR	   0
#define LOG_LOCAL0 0
#define LOG_LOCAL1 0
#define LOG_LOCAL2 0
#define LOG_LOCAL3 0
#define LOG_LOCAL4 0
#define LOG_LOCAL5 0
#define LOG_LOCAL6 0
#define LOG_LOCAL7 0

#define LOG_EMERG   0 /* system is unusable */
#define LOG_ALERT   1 /* action must be taken immediately */
#define LOG_CRIT    2 /* critical conditions */
#define LOG_ERR	    3 /* error conditions */
#define LOG_WARNING 4 /* warning conditions */
#define LOG_NOTICE  5 /* normal but signification condition */
#define LOG_INFO    6 /* informational */
#define LOG_DEBUG   7 /* debug-level messages */

void
syslog(int level, const char *fmt, ...);

void
openlog(const char *, int, ...);

void
closelog(void);

void
ModifyLogLevel(int level);

void
InitNTLogging(FILE *, int);

void
NTReportError(const char *, const char *);
/*
 * Include the event codes required for logging.
 */
#include <isc/bindevt.h>

#endif /* ifndef _SYSLOG_H */
