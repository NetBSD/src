/*	$NetBSD: rdataslab.h,v 1.9 2025/01/26 16:25:28 christos Exp $	*/

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

#include <isc/atomic.h>
#include <isc/heap.h>
#include <isc/lang.h>
#include <isc/stdtime.h>
#include <isc/urcu.h>

#include <dns/name.h>
#include <dns/rdataset.h>
#include <dns/types.h>

ISC_LANG_BEGINDECLS

#define DNS_RDATASLAB_FORCE 0x1
#define DNS_RDATASLAB_EXACT 0x2

#define DNS_RDATASLAB_OFFLINE 0x01 /* RRSIG is for offline DNSKEY */

struct dns_slabheader_proof {
	dns_name_t	name;
	void	       *neg;
	void	       *negsig;
	dns_rdatatype_t type;
};

struct dns_slabheader {
	/*%
	 * Locked by the owning node's lock.
	 */
	uint32_t	      serial;
	dns_ttl_t	      ttl;
	dns_typepair_t	      type;
	atomic_uint_least16_t attributes;
	dns_trust_t	      trust;

	unsigned int heap_index;
	/*%<
	 * Used for TTL-based cache cleaning.
	 */

	isc_stdtime_t resign;
	unsigned int  resign_lsb : 1;

	atomic_uint_fast16_t count;
	/*%<
	 * Monotonically increased every time this rdataset is bound so that
	 * it is used as the base of the starting point in DNS responses
	 * when the "cyclic" rrset-order is required.
	 */

	atomic_uint_fast32_t last_refresh_fail_ts;

	dns_slabheader_proof_t *noqname;
	dns_slabheader_proof_t *closest;
	/*%<
	 * We don't use the LIST macros, because the LIST structure has
	 * both head and tail pointers, and is doubly linked.
	 */

	struct dns_slabheader *next;
	/*%<
	 * If this is the top header for an rdataset, 'next' points
	 * to the top header for the next rdataset (i.e., the next type).
	 * Otherwise, it points up to the header whose down pointer points
	 * at this header.
	 */

	struct dns_slabheader *down;
	/*%<
	 * Points to the header for the next older version of
	 * this rdataset.
	 */

	dns_db_t     *db;
	dns_dbnode_t *node;
	/*%<
	 * The database and database node objects containing
	 * this rdataset, if any.
	 */

	isc_stdtime_t last_used;
	ISC_LINK(struct dns_slabheader) link;

	/*%
	 * Case vector.  If the bit is set then the corresponding
	 * character in the owner name needs to be AND'd with 0x20,
	 * rendering that character upper case.
	 */
	unsigned char upper[32];

	isc_heap_t *heap;
};

enum {
	DNS_SLABHEADERATTR_NONEXISTENT = 1 << 0,
	DNS_SLABHEADERATTR_STALE = 1 << 1,
	DNS_SLABHEADERATTR_IGNORE = 1 << 2,
	DNS_SLABHEADERATTR_NXDOMAIN = 1 << 3,
	DNS_SLABHEADERATTR_RESIGN = 1 << 4,
	DNS_SLABHEADERATTR_STATCOUNT = 1 << 5,
	DNS_SLABHEADERATTR_OPTOUT = 1 << 6,
	DNS_SLABHEADERATTR_NEGATIVE = 1 << 7,
	DNS_SLABHEADERATTR_PREFETCH = 1 << 8,
	DNS_SLABHEADERATTR_CASESET = 1 << 9,
	DNS_SLABHEADERATTR_ZEROTTL = 1 << 10,
	DNS_SLABHEADERATTR_CASEFULLYLOWER = 1 << 11,
	DNS_SLABHEADERATTR_ANCIENT = 1 << 12,
	DNS_SLABHEADERATTR_STALE_WINDOW = 1 << 13,
};

#define DNS_SLABHEADER_GETATTR(header, attribute) \
	(atomic_load_acquire(&(header)->attributes) & attribute)
#define DNS_SLABHEADER_SETATTR(header, attribute) \
	atomic_fetch_or_release(&(header)->attributes, attribute)
#define DNS_SLABHEADER_CLRATTR(header, attribute) \
	atomic_fetch_and_release(&(header)->attributes, ~(attribute))

extern dns_rdatasetmethods_t dns_rdataslab_rdatasetmethods;

/***
 *** Functions
 ***/

isc_result_t
dns_rdataslab_fromrdataset(dns_rdataset_t *rdataset, isc_mem_t *mctx,
			   isc_region_t *region, unsigned int reservelen,
			   uint32_t limit);
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
		    unsigned int flags, uint32_t maxrrperset,
		    unsigned char **tslabp);
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

dns_slabheader_t *
dns_slabheader_fromrdataset(const dns_rdataset_t *rdataset);
/*%
 * Returns the address of the slab header for a slab-type rdataset.
 */

void *
dns_slabheader_raw(dns_slabheader_t *header);
/*%
 * Returns the address of the raw memory following a dns_slabheader.
 */

void
dns_slabheader_setownercase(dns_slabheader_t *header, const dns_name_t *name);
/*%<
 * Store the casing of 'name', into a bitfield in 'header'.
 */

void
dns_slabheader_copycase(dns_slabheader_t *dest, dns_slabheader_t *src);
/*%<
 * Copy the casing of 'src', into 'dest'.
 */

void
dns_slabheader_reset(dns_slabheader_t *h, dns_db_t *db, dns_dbnode_t *node);
/*%<
 * Reset an rdataslab header 'h' so it can be used to store data in
 * database 'db' and node 'node'.
 */

dns_slabheader_t *
dns_slabheader_new(dns_db_t *db, dns_dbnode_t *node);
/*%<
 * Allocate memory for an rdataslab header and initialize it for use
 * in database 'db'/node 'node'.
 */

void
dns_slabheader_destroy(dns_slabheader_t **headerp);
/*%<
 * Free all memory associated with '*headerp'.
 */

void
dns_slabheader_freeproof(isc_mem_t *mctx, dns_slabheader_proof_t **proof);
/*%<
 * Free all memory associated with a nonexistence proof.
 */
ISC_LANG_ENDDECLS
