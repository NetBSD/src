/*	$NetBSD: rdatalist.h,v 1.1.1.1 2018/08/12 12:08:20 christos Exp $	*/

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


#ifndef DNS_RDATALIST_H
#define DNS_RDATALIST_H 1

/*****
 ***** Module Info
 *****/

/*! \file dns/rdatalist.h
 * \brief
 * A DNS rdatalist is a list of rdata of a common type and class.
 *
 * MP:
 *\li	Clients of this module must impose any required synchronization.
 *
 * Reliability:
 *\li	No anticipated impact.
 *
 * Resources:
 *\li	TBS
 *
 * Security:
 *\li	No anticipated impact.
 *
 * Standards:
 *\li	None.
 */

#include <isc/lang.h>

#include <dns/types.h>

/*%
 * Clients may use this type directly.
 */
struct dns_rdatalist {
	dns_rdataclass_t		rdclass;
	dns_rdatatype_t			type;
	dns_rdatatype_t			covers;
	dns_ttl_t			ttl;
	ISC_LIST(dns_rdata_t)		rdata;
	ISC_LINK(dns_rdatalist_t)	link;
	/*%<
	 * Case vector.  If the bit is set then the corresponding
	 * character in the owner name needs to be AND'd with 0x20,
	 * rendering that character upper case.
	 */
	unsigned char			upper[32];
};

ISC_LANG_BEGINDECLS

void
dns_rdatalist_init(dns_rdatalist_t *rdatalist);
/*%<
 * Initialize rdatalist.
 *
 * Ensures:
 *\li	All fields of rdatalist have been initialized to their default
 *	values.
 */

isc_result_t
dns_rdatalist_tordataset(dns_rdatalist_t *rdatalist,
			 dns_rdataset_t *rdataset);
/*%<
 * Make 'rdataset' refer to the rdata in 'rdatalist'.
 *
 * Note:
 *\li	The caller must ensure that 'rdatalist' remains valid and unchanged
 *	while 'rdataset' is associated with it.
 *
 * Requires:
 *
 *\li	'rdatalist' is a valid rdatalist.
 *
 *\li	'rdataset' is a valid rdataset that is not currently associated with
 *	any rdata.
 *
 * Ensures,
 *	on success,
 *
 *\li		'rdataset' is associated with the rdata in rdatalist.
 *
 * Returns:
 *\li	#ISC_R_SUCCESS
 */

isc_result_t
dns_rdatalist_fromrdataset(dns_rdataset_t *rdataset,
			   dns_rdatalist_t **rdatalist);
/*%<
 * Point 'rdatalist' to the rdatalist in 'rdataset'.
 *
 * Requires:
 *
 *\li	'rdatalist' is a pointer to a NULL dns_rdatalist_t pointer.
 *
 *\li	'rdataset' is a valid rdataset associated with an rdatalist.
 *
 * Ensures,
 *	on success,
 *
 *\li		'rdatalist' is pointed to the rdatalist in rdataset.
 *
 * Returns:
 *\li	#ISC_R_SUCCESS
 */

ISC_LANG_ENDDECLS

#endif /* DNS_RDATALIST_H */
