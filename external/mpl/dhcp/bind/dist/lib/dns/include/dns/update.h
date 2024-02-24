/*	$NetBSD: update.h,v 1.1.2.2 2024/02/24 13:07:08 martin Exp $	*/

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

#ifndef DNS_UPDATE_H
#define DNS_UPDATE_H 1

/*! \file dns/update.h */

/***
 ***	Imports
 ***/

#include <inttypes.h>

#include <isc/lang.h>

#include <dns/diff.h>
#include <dns/types.h>

typedef struct {
	void (*func)(void *arg, dns_zone_t *zone, int level,
		     const char *message);
	void *arg;
} dns_update_log_t;

ISC_LANG_BEGINDECLS

/***
 ***	Functions
 ***/

uint32_t
dns_update_soaserial(uint32_t serial, dns_updatemethod_t method,
		     dns_updatemethod_t *used);
/*%<
 * Return the next serial number after 'serial', depending on the
 * update method 'method':
 *
 *\li	* dns_updatemethod_increment increments the serial number by one
 *\li	* dns_updatemethod_date sets the serial number to YYYYMMDD00
 *\li	* dns_updatemethod_unixtime sets the serial number to the current
 *	  time (seconds since UNIX epoch)
 *\li	* dns_updatemethod_none just returns the given serial
 *
 * NOTE: The dns_updatemethod_increment will be used if dns_updatemethod_date or
 * dns_updatemethod_unixtime is used and the new serial number would be lower
 * than current serial number.
 *
 * Sets *used to the method that was used.
 */

isc_result_t
dns_update_signatures(dns_update_log_t *log, dns_zone_t *zone, dns_db_t *db,
		      dns_dbversion_t *oldver, dns_dbversion_t *newver,
		      dns_diff_t *diff, uint32_t sigvalidityinterval);

isc_result_t
dns_update_signaturesinc(dns_update_log_t *log, dns_zone_t *zone, dns_db_t *db,
			 dns_dbversion_t *oldver, dns_dbversion_t *newver,
			 dns_diff_t *diff, uint32_t sigvalidityinterval,
			 dns_update_state_t **state);

ISC_LANG_ENDDECLS

#endif /* DNS_UPDATE_H */
