/*	$NetBSD: nsec.h,v 1.1.1.1 2018/08/12 12:08:20 christos Exp $	*/

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


#ifndef DNS_NSEC_H
#define DNS_NSEC_H 1

/*! \file dns/nsec.h */

#include <isc/lang.h>

#include <dns/types.h>
#include <dns/name.h>

#define DNS_NSEC_BUFFERSIZE (DNS_NAME_MAXWIRE + 8192 + 512)

ISC_LANG_BEGINDECLS

isc_result_t
dns_nsec_buildrdata(dns_db_t *db, dns_dbversion_t *version,
		    dns_dbnode_t *node, const dns_name_t *target,
		    unsigned char *buffer, dns_rdata_t *rdata);
/*%<
 * Build the rdata of a NSEC record.
 *
 * Requires:
 *\li	buffer	Points to a temporary buffer of at least
 * 		DNS_NSEC_BUFFERSIZE bytes.
 *\li	rdata	Points to an initialized dns_rdata_t.
 *
 * Ensures:
 *  \li    *rdata	Contains a valid NSEC rdata.  The 'data' member refers
 *		to 'buffer'.
 */

isc_result_t
dns_nsec_build(dns_db_t *db, dns_dbversion_t *version, dns_dbnode_t *node,
	       const dns_name_t *target, dns_ttl_t ttl);
/*%<
 * Build a NSEC record and add it to a database.
 */

isc_boolean_t
dns_nsec_typepresent(dns_rdata_t *nsec, dns_rdatatype_t type);
/*%<
 * Determine if a type is marked as present in an NSEC record.
 *
 * Requires:
 *\li	'nsec' points to a valid rdataset of type NSEC
 */

isc_result_t
dns_nsec_nseconly(dns_db_t *db, dns_dbversion_t *version,
		  isc_boolean_t *answer);
/*
 * Report whether the DNSKEY RRset has a NSEC only algorithm.  Unknown
 * algorithms are assumed to support NSEC3.  If DNSKEY is not found,
 * *answer is set to ISC_FALSE, and ISC_R_NOTFOUND is returned.
 *
 * Requires:
 * 	'answer' to be non NULL.
 */

unsigned int
dns_nsec_compressbitmap(unsigned char *map, const unsigned char *raw,
			unsigned int max_type);
/*%<
 * Convert a raw bitmap into a compressed windowed bit map.  'map' and 'raw'
 * may overlap.
 *
 * Returns the length of the compressed windowed bit map.
 */

void
dns_nsec_setbit(unsigned char *array, unsigned int type, unsigned int bit);
/*%<
 * Set type bit in raw 'array' to 'bit'.
 */

isc_boolean_t
dns_nsec_isset(const unsigned char *array, unsigned int type);
/*%<
 * Test if the corresponding 'type' bit is set in 'array'.
 */

isc_result_t
dns_nsec_noexistnodata(dns_rdatatype_t type, const dns_name_t *name,
		       const dns_name_t *nsecname, dns_rdataset_t *nsecset,
		       isc_boolean_t *exists, isc_boolean_t *data,
		       dns_name_t *wild, dns_nseclog_t log, void *arg);
/*%
 * Return ISC_R_SUCCESS if we can determine that the name doesn't exist
 * or we can determine whether there is data or not at the name.
 * If the name does not exist return the wildcard name.
 *
 * Return ISC_R_IGNORE when the NSEC is not the appropriate one.
 */

ISC_LANG_ENDDECLS

#endif /* DNS_NSEC_H */
