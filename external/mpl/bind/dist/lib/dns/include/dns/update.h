/*	$NetBSD: update.h,v 1.1.1.1 2018/08/12 12:08:18 christos Exp $	*/

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


#ifndef DNS_UPDATE_H
#define DNS_UPDATE_H 1

/*! \file dns/update.h */

/***
 ***	Imports
 ***/

#include <isc/lang.h>

#include <dns/types.h>
#include <dns/diff.h>

typedef struct {
	void (*func)(void *arg, dns_zone_t *zone, int level,
		     const char *message);
	void *arg;
} dns_update_log_t;

ISC_LANG_BEGINDECLS

/***
 ***	Functions
 ***/

isc_uint32_t
dns_update_soaserial(isc_uint32_t serial, dns_updatemethod_t method);
/*%<
 * Return the next serial number after 'serial', depending on the
 * update method 'method':
 *
 *\li	* dns_updatemethod_increment increments the serial number by one
 *\li	* dns_updatemethod_unixtime sets the serial number to the current
 *	  time (seconds since UNIX epoch) if possible, or increments by one
 *	  if not.
 */

isc_result_t
dns_update_signatures(dns_update_log_t *log, dns_zone_t *zone, dns_db_t *db,
		      dns_dbversion_t *oldver, dns_dbversion_t *newver,
		      dns_diff_t *diff, isc_uint32_t sigvalidityinterval);

isc_result_t
dns_update_signaturesinc(dns_update_log_t *log, dns_zone_t *zone, dns_db_t *db,
			 dns_dbversion_t *oldver, dns_dbversion_t *newver,
			 dns_diff_t *diff, isc_uint32_t sigvalidityinterval,
			 dns_update_state_t **state);

ISC_LANG_ENDDECLS

#endif /* DNS_UPDATE_H */
