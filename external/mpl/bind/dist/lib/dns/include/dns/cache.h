/*	$NetBSD: cache.h,v 1.1.1.7 2024/09/22 00:06:18 christos Exp $	*/

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

/*****
***** Module Info
*****/

/*! \file dns/cache.h
 * \brief
 * Defines dns_cache_t, the cache object.
 *
 * Notes:
 *\li 	A cache object contains DNS data of a single class.
 *	Multiple classes will be handled by creating multiple
 *	views, each with a different class and its own cache.
 *
 * MP:
 *\li	See notes at the individual functions.
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

#include <stdbool.h>

#include <isc/lang.h>
#include <isc/stats.h>
#include <isc/stdtime.h>

#include <dns/types.h>

ISC_LANG_BEGINDECLS

/***
 ***	Functions
 ***/
isc_result_t
dns_cache_create(isc_mem_t *mctx, isc_taskmgr_t *taskmgr,
		 isc_timermgr_t *timermgr, dns_rdataclass_t rdclass,
		 const char *cachename, const char *db_type,
		 unsigned int db_argc, char **db_argv, dns_cache_t **cachep);
/*%<
 * Create a new named DNS cache using two separate memory contexts, one for
 * cache data which can be cleaned and a separate one for memory allocated for
 * the heap (which can grow without an upper limit and has no mechanism for
 * shrinking).
 *
 * Requires:
 *
 *\li	'mctx' is a valid memory context.
 *
 *\li	'taskmgr' is a valid task manager (if 'db_type' is "rbt").
 *
 *\li	'taskmgr' is a valid task manager and 'timermgr' is a valid timer
 * 	manager, or both are NULL (if 'db_type' is not "rbt").  If NULL, no
 * 	periodic cleaning of the cache will take place.
 *
 *\li	'cachename' is a valid string.  This must not be NULL.

 *\li	'mctx' is a valid memory context.
 *
 *\li	'cachep' is a valid pointer, and *cachep == NULL
 *
 * Ensures:
 *
 *\li	'*cachep' is attached to the newly created cache
 *
 * Returns:
 *
 *\li	#ISC_R_SUCCESS
 *\li	#ISC_R_NOMEMORY
 */

void
dns_cache_attach(dns_cache_t *cache, dns_cache_t **targetp);
/*%<
 * Attach *targetp to cache.
 *
 * Requires:
 *
 *\li	'cache' is a valid cache.
 *
 *\li	'targetp' points to a NULL dns_cache_t *.
 *
 * Ensures:
 *
 *\li	*targetp is attached to cache.
 */

void
dns_cache_detach(dns_cache_t **cachep);
/*%<
 * Detach *cachep from its cache.
 *
 * Requires:
 *
 *\li	'cachep' points to a valid cache.
 *
 * Ensures:
 *
 *\li	*cachep is NULL.
 *
 *\li	If '*cachep' is the last reference to the cache,
 *		all resources used by the cache will be freed
 */

void
dns_cache_attachdb(dns_cache_t *cache, dns_db_t **dbp);
/*%<
 * Attach *dbp to the cache's database.
 *
 * Notes:
 *
 *\li	This may be used to get a reference to the database for
 *	the purpose of cache lookups (XXX currently it is also
 * 	the way to add data to the cache, but having a
 * 	separate dns_cache_add() interface instead would allow
 * 	more control over memory usage).
 *	The caller should call dns_db_detach() on the reference
 *	when it is no longer needed.
 *
 * Requires:
 *
 *\li	'cache' is a valid cache.
 *
 *\li	'dbp' points to a NULL dns_db *.
 *
 * Ensures:
 *
 *\li	*dbp is attached to the database.
 */

isc_result_t
dns_cache_clean(dns_cache_t *cache, isc_stdtime_t now);
/*%<
 * Force immediate cleaning of the cache, freeing all rdatasets
 * whose TTL has expired as of 'now' and that have no pending
 * references.
 */

const char *
dns_cache_getname(dns_cache_t *cache);
/*%<
 * Get the cache name.
 */

void
dns_cache_setcachesize(dns_cache_t *cache, size_t size);
/*%<
 * Set the maximum cache size.  0 means unlimited.
 */

size_t
dns_cache_getcachesize(dns_cache_t *cache);
/*%<
 * Get the maximum cache size.
 */

void
dns_cache_setservestalettl(dns_cache_t *cache, dns_ttl_t ttl);
/*%<
 * Sets the maximum length of time that cached answers may be retained
 * past their normal TTL.  Default value for the library is 0, disabling
 * the use of stale data.
 *
 * Requires:
 *\li	'cache' to be valid.
 */

dns_ttl_t
dns_cache_getservestalettl(dns_cache_t *cache);
/*%<
 * Gets the maximum length of time that cached answers may be kept past
 * normal expiry.
 *
 * Requires:
 *\li	'cache' to be valid.
 */

void
dns_cache_setservestalerefresh(dns_cache_t *cache, dns_ttl_t interval);
/*%<
 * Sets the length of time to wait before attempting to refresh a rrset
 * if a previous attempt in doing so has failed.
 * During this time window if stale rrset are available in cache they
 * will be directly returned to client.
 *
 * Requires:
 *\li	'cache' to be valid.
 */

dns_ttl_t
dns_cache_getservestalerefresh(dns_cache_t *cache);
/*%<
 * Gets the 'stale-refresh-time' value, set by a previous call to
 * 'dns_cache_setservestalerefresh'.
 *
 * Requires:
 *\li	'cache' to be valid.
 */

isc_result_t
dns_cache_flush(dns_cache_t *cache);
/*%<
 * Flushes all data from the cache.
 *
 * Returns:
 *\li	#ISC_R_SUCCESS
 *\li	#ISC_R_NOMEMORY
 */

isc_result_t
dns_cache_flushnode(dns_cache_t *cache, const dns_name_t *name, bool tree);
/*
 * Flush a given name from the cache.  If 'tree' is true, then
 * also flush all names under 'name'.
 *
 * Requires:
 *\li	'cache' to be valid.
 *\li	'name' to be valid.
 *
 * Returns:
 *\li	#ISC_R_SUCCESS
 *\li	#ISC_R_NOMEMORY
 *\li	other error returns.
 */

isc_result_t
dns_cache_flushname(dns_cache_t *cache, const dns_name_t *name);
/*
 * Flush a given name from the cache.  Equivalent to
 * dns_cache_flushpartial(cache, name, false).
 *
 * Requires:
 *\li	'cache' to be valid.
 *\li	'name' to be valid.
 *
 * Returns:
 *\li	#ISC_R_SUCCESS
 *\li	#ISC_R_NOMEMORY
 *\li	other error returns.
 */

isc_stats_t *
dns_cache_getstats(dns_cache_t *cache);
/*
 * Return a pointer to the stats collection object for 'cache'
 */

void
dns_cache_dumpstats(dns_cache_t *cache, FILE *fp);
/*
 * Dump cache statistics and status in text to 'fp'
 */

void
dns_cache_updatestats(dns_cache_t *cache, isc_result_t result);
/*
 * Update cache statistics based on result code in 'result'
 */

void
dns_cache_setmaxrrperset(dns_cache_t *cache, uint32_t value);
/*%<
 * Set the maximum resource records per RRSet that can be cached.
 */

void
dns_cache_setmaxtypepername(dns_cache_t *cache, uint32_t value);
/*%<
 * Set the maximum resource record types per owner name that can be cached.
 */

#ifdef HAVE_LIBXML2
int
dns_cache_renderxml(dns_cache_t *cache, void *writer0);
/*
 * Render cache statistics and status in XML for 'writer'.
 */
#endif /* HAVE_LIBXML2 */

#ifdef HAVE_JSON_C
isc_result_t
dns_cache_renderjson(dns_cache_t *cache, void *cstats0);
/*
 * Render cache statistics and status in JSON
 */
#endif /* HAVE_JSON_C */

ISC_LANG_ENDDECLS
