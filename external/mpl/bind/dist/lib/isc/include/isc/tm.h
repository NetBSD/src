/*	$NetBSD: tm.h,v 1.2.6.1 2024/02/29 12:35:12 martin Exp $	*/

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
