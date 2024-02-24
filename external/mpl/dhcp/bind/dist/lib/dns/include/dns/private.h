/*	$NetBSD: private.h,v 1.1.2.2 2024/02/24 13:07:06 martin Exp $	*/

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

#include <stdbool.h>

#include <isc/lang.h>
#include <isc/types.h>

#include <dns/db.h>
#include <dns/types.h>

#ifndef DNS_PRIVATE_H
#define DNS_PRIVATE_H

ISC_LANG_BEGINDECLS

isc_result_t
dns_private_chains(dns_db_t *db, dns_dbversion_t *ver,
		   dns_rdatatype_t privatetype, bool *build_nsec,
		   bool *build_nsec3);
/*%<
 * Examine the NSEC, NSEC3PARAM and privatetype RRsets at the apex of the
 * database to determine which of NSEC or NSEC3 chains we are currently
 * maintaining.  In normal operations only one of NSEC or NSEC3 is being
 * maintained but when we are transitiong between NSEC and NSEC3 we need
 * to update both sets of chains.  If 'privatetype' is zero then the
 * privatetype RRset will not be examined.
 *
 * Requires:
 * \li	'db' is valid.
 * \li	'version' is valid or NULL.
 * \li	'build_nsec' is a pointer to a bool or NULL.
 * \li	'build_nsec3' is a pointer to a bool or NULL.
 *
 * Returns:
 * \li 	ISC_R_SUCCESS, 'build_nsec' and 'build_nsec3' will be valid.
 * \li	other on error
 */

isc_result_t
dns_private_totext(dns_rdata_t *privaterdata, isc_buffer_t *buffer);
/*%<
 * Convert a private-type RR 'privaterdata' to human-readable form,
 * and place the result in 'buffer'.  The text should indicate
 * which action the private-type record specifies and whether the
 * action has been completed.
 *
 * Requires:
 * \li	'privaterdata' is a valid rdata containing at least five bytes
 * \li	'buffer' is a valid buffer
 *
 * Returns:
 * \li 	ISC_R_SUCCESS
 * \li	other on error
 */

ISC_LANG_ENDDECLS

#endif /* ifndef DNS_PRIVATE_H */
