/*	$NetBSD: rdataslab.h,v 1.6.2.1 2024/02/25 15:46:58 martin Exp $	*/

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

/*! \file dns/rdataslab.h
 * \brief
 * Implements storage of rdatasets into slabs of memory.
 *
 * MP:
 *\li	Clients of this module must impose any required synchronization.
 *
 * Reliability:
 *\li	This module deals with low-level byte streams.  Errors in any of
 *	the functions are likely to crash the server or corrupt memory.
 *
 *\li	If the caller passes invalid memory references, these functions are
 *	likely to crash the server or corrupt memory.
 *
 * Resources:
 *\li	None.
 *
 * Security:
 *\li	None.
 *
 * Standards:
 *\li	None.
 */

/***
 *** Imports
 ***/

#include <stdbool.h>

#include <isc/lang.h>

#include <dns/types.h>

ISC_LANG_BEGINDECLS

#define DNS_RDATASLAB_FORCE 0x1
#define DNS_RDATASLAB_EXACT 0x2

#define DNS_RDATASLAB_OFFLINE 0x01 /* RRSIG is for offline DNSKEY */
#define DNS_RDATASLAB_WARNMASK          \
	0x0E /*%< RRSIG(DNSKEY) expired \
	      * warnings number mask. */
#define DNS_RDATASLAB_WARNSHIFT               \
	1 /*%< How many bits to shift to find \
	   * remaining expired warning number. */

/***
 *** Functions
 ***/

isc_result_t
dns_rdataslab_fromrdataset(dns_rdataset_t *rdataset, isc_mem_t *mctx,
			   isc_region_t *region, unsigned int reservelen);
/*%<
 * Slabify a rdataset.  The slab area will be allocated and returned
 * in 'region'.
 *
 * Requires:
 *\li	'rdataset' is valid.
 *
 * Ensures:
 *\li	'region' will have base pointing to the start of allocated memory,
 *	with the slabified region beginning at region->base + reservelen.
 *	region->length contains the total length allocated.
 *
 * Returns:
 *\li	ISC_R_SUCCESS		- successful completion
 *\li	ISC_R_NOMEMORY		- no memory.
 *\li	XXX others
 */

unsigned int
dns_rdataslab_size(unsigned char *slab, unsigned int reservelen);
/*%<
 * Return the total size of an rdataslab.
 *
 * Requires:
 *\li	'slab' points to a slab.
 *
 * Returns:
 *\li	The number of bytes in the slab, including the reservelen.
 */

unsigned int
dns_rdataslab_rdatasize(unsigned char *slab, unsigned int reservelen);
/*%<
 * Return the size of the rdata in an rdataslab.
 *
 * Requires:
 *\li	'slab' points to a slab.
 */

unsigned int
dns_rdataslab_count(unsigned char *slab, unsigned int reservelen);
/*%<
 * Return the number of records in the rdataslab
 *
 * Requires:
 *\li	'slab' points to a slab.
 *
 * Returns:
 *\li	The number of records in the slab.
 */

isc_result_t
dns_rdataslab_merge(unsigned char *oslab, unsigned char *nslab,
		    unsigned int reservelen, isc_mem_t *mctx,
		    dns_rdataclass_t rdclass, dns_rdatatype_t type,
		    unsigned int flags, unsigned char **tslabp);
/*%<
 * Merge 'oslab' and 'nslab'.
 */

isc_result_t
dns_rdataslab_subtract(unsigned char *mslab, unsigned char *sslab,
		       unsigned int reservelen, isc_mem_t *mctx,
		       dns_rdataclass_t rdclass, dns_rdatatype_t type,
		       unsigned int flags, unsigned char **tslabp);
/*%<
 * Subtract 'sslab' from 'mslab'.  If 'exact' is true then all elements
 * of 'sslab' must exist in 'mslab'.
 *
 * XXX
 * valid flags are DNS_RDATASLAB_EXACT
 */

bool
dns_rdataslab_equal(unsigned char *slab1, unsigned char *slab2,
		    unsigned int reservelen);
/*%<
 * Compare two rdataslabs for equality.  This does _not_ do a full
 * DNSSEC comparison.
 *
 * Requires:
 *\li	'slab1' and 'slab2' point to slabs.
 *
 * Returns:
 *\li	true if the slabs are equal, false otherwise.
 */
bool
dns_rdataslab_equalx(unsigned char *slab1, unsigned char *slab2,
		     unsigned int reservelen, dns_rdataclass_t rdclass,
		     dns_rdatatype_t type);
/*%<
 * Compare two rdataslabs for DNSSEC equality.
 *
 * Requires:
 *\li	'slab1' and 'slab2' point to slabs.
 *
 * Returns:
 *\li	true if the slabs are equal, #false otherwise.
 */

ISC_LANG_ENDDECLS
