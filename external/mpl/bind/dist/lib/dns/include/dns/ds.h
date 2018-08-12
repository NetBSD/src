/*	$NetBSD: ds.h,v 1.1.1.1 2018/08/12 12:08:20 christos Exp $	*/

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


#ifndef DNS_DS_H
#define DNS_DS_H 1

#include <isc/lang.h>

#include <dns/types.h>

#define DNS_DSDIGEST_SHA1 (1)
#define DNS_DSDIGEST_SHA256 (2)
#define DNS_DSDIGEST_GOST (3)
#define DNS_DSDIGEST_SHA384 (4)

/*
 * Assuming SHA-384 digest type.
 */
#define DNS_DS_BUFFERSIZE (52)

ISC_LANG_BEGINDECLS

isc_result_t
dns_ds_buildrdata(dns_name_t *owner, dns_rdata_t *key,
		  unsigned int digest_type, unsigned char *buffer,
		  dns_rdata_t *rdata);
/*%<
 * Build the rdata of a DS record.
 *
 * Requires:
 *\li	key	Points to a valid DNS KEY record.
 *\li	buffer	Points to a temporary buffer of at least
 * 		#DNS_DS_BUFFERSIZE bytes.
 *\li	rdata	Points to an initialized dns_rdata_t.
 *
 * Ensures:
 *  \li    *rdata	Contains a valid DS rdata.  The 'data' member refers
 *		to 'buffer'.
 */

ISC_LANG_ENDDECLS

#endif /* DNS_DS_H */
