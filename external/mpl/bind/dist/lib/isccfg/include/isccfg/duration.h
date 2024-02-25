/*	$NetBSD: duration.h,v 1.2.2.2 2024/02/25 15:47:33 martin Exp $	*/


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

#include <inttypes.h>
#include <stdbool.h>

#include <isc/lang.h>
#include <isc/types.h>

ISC_LANG_BEGINDECLS

#define CFG_DURATION_MAXLEN 80

/*%
 * A configuration object to store ISO 8601 durations.
 */
typedef struct isccfg_duration {
	/*
	 * The duration is stored in multiple parts:
	 * [0] Years
	 * [1] Months
	 * [2] Weeks
	 * [3] Days
	 * [4] Hours
	 * [5] Minutes
	 * [6] Seconds
	 */
	uint32_t parts[7];
	bool	 iso8601;
	bool	 unlimited;
} isccfg_duration_t;

isc_result_t
isccfg_duration_fromtext(isc_textregion_t *source, isccfg_duration_t *duration);
/*%<
 * Converts an ISO 8601 duration style value.
 *
 * Returns:
 *\li	ISC_R_SUCCESS
 *\li	DNS_R_BADNUMBER
 */

isc_result_t
isccfg_parse_duration(isc_textregion_t *source, isccfg_duration_t *duration);
/*%<
 * Converts a duration string to a ISO 8601 duration.
 * If the string does not start with a P (or p), fall back to TTL-style value.
 * In that case the duration will be treated in seconds only.
 *
 * Returns:
 *\li	ISC_R_SUCCESS
 *\li	DNS_R_BADNUMBER
 *\li	DNS_R_BADTTL
 */

uint32_t
isccfg_duration_toseconds(const isccfg_duration_t *duration);
/*%<
 * Converts an ISO 8601 duration to seconds.
 * The conversion is approximate:
 * - Months will be treated as 31 days.
 * - Years will be treated as 365 days.
 *
 * Notes:
 *\li	If the duration in seconds is greater than UINT32_MAX, the return value
 * 	will be UINT32_MAX.
 *
 * Returns:
 *\li	The duration in seconds.
 */

ISC_LANG_ENDDECLS
