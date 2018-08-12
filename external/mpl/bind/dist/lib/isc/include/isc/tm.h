/*	$NetBSD: tm.h,v 1.1.1.1 2018/08/12 12:08:26 christos Exp $	*/

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

#ifndef ISC_TM_H
#define ISC_TM_H 1

/*! \file isc/tm.h
 * Provides portable conversion routines for struct tm.
 */
#include <time.h>

#include <isc/lang.h>
#include <isc/types.h>


ISC_LANG_BEGINDECLS

time_t
isc_tm_timegm(struct tm *tm);
/*
 * Convert a tm structure to time_t, using UTC rather than the local
 * time zone.
 */

char *
isc_tm_strptime(const char *buf, const char *fmt, struct tm *tm);
/*
 * Parse a formatted date string into struct tm.
 */

ISC_LANG_ENDDECLS

#endif /* ISC_TIMER_H */
