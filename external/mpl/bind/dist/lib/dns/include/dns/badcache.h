/*	$NetBSD: badcache.h,v 1.2 2018/08/12 13:02:35 christos Exp $	*/

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

#ifndef DNS_BADCACHE_H
#define DNS_BADCACHE_H 1

/*****
 ***** Module Info
 *****/

/*! \file dns/badcache.h
 * \brief
 * Defines dns_badcache_t, the "bad cache" object.
 *
 * Notes:
 *\li 	A bad cache object is a hash table of name/type tuples,
 *	indicating whether a given tuple known to be "bad" in some
 *	sense (e.g., queries for that name and type have been
 *	returning SERVFAIL). This is used for both the "bad server
 *	cache" in the resolver and for the "servfail cache" in
 *	the view.
 *
 * Reliability:
 *
 * Resources:
 *
 * Security:
 *
 * Standards:
 */

/***
 ***	Imports
 ***/

#include <dns/types.h>

ISC_LANG_BEGINDECLS

/***
 ***	Functions
 ***/

isc_result_t
dns_badcache_init(isc_mem_t *mctx, unsigned int size, dns_badcache_t **bcp);
/*%
 * Allocate and initialize a badcache and store it in '*bcp'.
 *
 * Requires:
 * \li	mctx != NULL
 * \li	bcp != NULL
 * \li	*bcp == NULL
 */

void
dns_badcache_destroy(dns_badcache_t **bcp);
/*%
 * Flush and then free badcache in 'bcp'. '*bcp' is set to NULL on return.
 *
 * Requires:
 * \li	'*bcp' to be a valid badcache
 */

void
dns_badcache_add(dns_badcache_t *bc, const dns_name_t *name,
		 dns_rdatatype_t type, isc_boolean_t update,
		 isc_uint32_t flags, isc_time_t *expire);
/*%
 * Adds a badcache entry to the badcache 'bc' for name 'name' and
 * type 'type'.  If an entry already exists, then it will be updated if
 * 'update' is ISC_TRUE.  The entry will be stored with flags 'flags'
 * and expiration date 'expire'.
 *
 * Requires:
 * \li	bc to be a valid badcache.
 * \li	name != NULL
 * \li	expire != NULL
 */

isc_boolean_t
dns_badcache_find(dns_badcache_t *bc, const dns_name_t *name,
		  dns_rdatatype_t type, isc_uint32_t *flagp,
		  isc_time_t *now);
/*%
 * Returns ISC_TRUE if a record is found in the badcache 'bc' matching
 * 'name' and 'type', with an expiration date later than 'now'.
 * If 'flagp' is not NULL, then '*flagp' is updated to the flags
 * that were stored in the badcache entry.  Returns ISC_FALSE if
 * no matching record is found.
 *
 * Requires:
 * \li	bc to be a valid badcache.
 * \li	name != NULL
 * \li	now != NULL
 */

void
dns_badcache_flush(dns_badcache_t *bc);
/*%
 * Flush the entire bad cache.
 *
 * Requires:
 * \li	bc to be a valid badcache
 */

void
dns_badcache_flushname(dns_badcache_t *bc, const dns_name_t *name);
/*%
 * Flush the bad cache of all entries at 'name'.
 *
 * Requires:
 * \li	bc to be a valid badcache
 * \li	name != NULL
 */

void
dns_badcache_flushtree(dns_badcache_t *bc, const dns_name_t *name);
/*%
 * Flush the bad cache of all entries at or below 'name'.
 *
 * Requires:
 * \li	bc to be a valid badcache
 * \li	name != NULL
 */

void
dns_badcache_print(dns_badcache_t *bc, const char *cachename, FILE *fp);
/*%
 * Print the contents of badcache 'bc' (headed by the title 'cachename')
 * to file pointer 'fp'.
 *
 * Requires:
 * \li	bc to be a valid badcache
 * \li	cachename != NULL
 * \li	fp != NULL
 */

ISC_LANG_ENDDECLS

#endif /* DNS_BADCACHE_H */
