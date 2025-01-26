/*	$NetBSD: dbiterator.h,v 1.8 2025/01/26 16:25:26 christos Exp $	*/

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

/*! \file dns/dbiterator.h
 * \brief
 * The DNS DB Iterator interface allows iteration of all of the nodes in a
 * database.
 *
 * The dns_dbiterator_t type is like a "virtual class".  To actually use
 * it, an implementation of the class is required.  This implementation is
 * supplied by the database.
 *
 * It is the client's responsibility to call dns_db_detachnode() on all
 * nodes returned.
 *
 * XXX &lt;more&gt; XXX
 *
 * MP:
 *\li	The iterator itself is not locked.  The caller must ensure
 *	synchronization.
 *
 *\li	The iterator methods ensure appropriate database locking.
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

/*****
***** Imports
*****/

#include <stdbool.h>

#include <isc/lang.h>
#include <isc/magic.h>

#include <dns/types.h>

ISC_LANG_BEGINDECLS

/*****
***** Types
*****/

typedef struct dns_dbiteratormethods {
	void (*destroy)(dns_dbiterator_t **iteratorp DNS__DB_FLARG);
	isc_result_t (*first)(dns_dbiterator_t *iterator DNS__DB_FLARG);
	isc_result_t (*last)(dns_dbiterator_t *iterator DNS__DB_FLARG);
	isc_result_t (*seek)(dns_dbiterator_t	   *iterator,
			     const dns_name_t *name DNS__DB_FLARG);
	isc_result_t (*prev)(dns_dbiterator_t *iterator DNS__DB_FLARG);
	isc_result_t (*next)(dns_dbiterator_t *iterator DNS__DB_FLARG);
	isc_result_t (*current)(dns_dbiterator_t *iterator,
				dns_dbnode_t	**nodep,
				dns_name_t *name  DNS__DB_FLARG);
	isc_result_t (*pause)(dns_dbiterator_t *iterator);
	isc_result_t (*origin)(dns_dbiterator_t *iterator, dns_name_t *name);
} dns_dbiteratormethods_t;

#define DNS_DBITERATOR_MAGIC	  ISC_MAGIC('D', 'N', 'S', 'I')
#define DNS_DBITERATOR_VALID(dbi) ISC_MAGIC_VALID(dbi, DNS_DBITERATOR_MAGIC)
/*%
 * This structure is actually just the common prefix of a DNS db
 * implementation's version of a dns_dbiterator_t.
 *
 * Clients may use the 'db' field of this structure.  Except for that field,
 * direct use of this structure by clients is forbidden.  DB implementations
 * may change the structure.  'magic' must be DNS_DBITERATOR_MAGIC for any of
 * the dns_dbiterator routines to work.  DB iterator implementations must
 * maintain all DB iterator invariants.
 */
struct dns_dbiterator {
	/* Unlocked. */
	unsigned int		 magic;
	dns_dbiteratormethods_t *methods;
	dns_db_t		*db;
	bool			 relative_names;
};

#define dns_dbiterator_destroy(iteratorp) \
	dns__dbiterator_destroy(iteratorp DNS__DB_FILELINE)
void
dns__dbiterator_destroy(dns_dbiterator_t **iteratorp DNS__DB_FLARG);
/*%<
 * Destroy '*iteratorp'.
 *
 * Requires:
 *
 *\li	'*iteratorp' is a valid iterator.
 *
 * Ensures:
 *
 *\li	All resources used by the iterator are freed.
 *
 *\li	*iteratorp == NULL.
 */

#define dns_dbiterator_first(iterator) \
	dns__dbiterator_first(iterator DNS__DB_FILELINE)
isc_result_t
dns__dbiterator_first(dns_dbiterator_t *iterator DNS__DB_FLARG);
/*%<
 * Move the node cursor to the first node in the database (if any).
 *
 * Requires:
 *\li	'iterator' is a valid iterator.
 *
 * Returns:
 *\li	#ISC_R_SUCCESS
 *\li	#ISC_R_NOMORE			There are no nodes in the database.
 *
 *\li	Other results are possible, depending on the DB implementation.
 */

#define dns_dbiterator_last(iterator) \
	dns__dbiterator_last(iterator DNS__DB_FILELINE)
isc_result_t
dns__dbiterator_last(dns_dbiterator_t *iterator DNS__DB_FLARG);
/*%<
 * Move the node cursor to the last node in the database (if any).
 *
 * Requires:
 *\li	'iterator' is a valid iterator.
 *
 * Returns:
 *\li	#ISC_R_SUCCESS
 *\li	#ISC_R_NOMORE			There are no nodes in the database.
 *
 *\li	Other results are possible, depending on the DB implementation.
 */

#define dns_dbiterator_seek(iterator, name) \
	dns__dbiterator_seek(iterator, name DNS__DB_FILELINE)
isc_result_t
dns__dbiterator_seek(dns_dbiterator_t	   *iterator,
		     const dns_name_t *name DNS__DB_FLARG);
/*%<
 * Move the node cursor to the node with name 'name'.
 *
 * Requires:
 *\li	'iterator' is a valid iterator.
 *
 *\li	'name' is a valid name.
 *
 * Returns:
 *\li	#ISC_R_SUCCESS
 *\li	#ISC_R_NOTFOUND
 *\li	#DNS_R_PARTIALMATCH
 *	(node is at name above requested named when name has children)
 *
 *\li	Other results are possible, depending on the DB implementation.
 */

#define dns_dbiterator_prev(iterator) \
	dns__dbiterator_prev(iterator DNS__DB_FILELINE)
isc_result_t
dns__dbiterator_prev(dns_dbiterator_t *iterator DNS__DB_FLARG);
/*%<
 * Move the node cursor to the previous node in the database (if any).
 *
 * Requires:
 *\li	'iterator' is a valid iterator.
 *
 * Returns:
 *\li	#ISC_R_SUCCESS
 *\li	#ISC_R_NOMORE			There are no more nodes in the
 *					database.
 *
 *\li	Other results are possible, depending on the DB implementation.
 */

#define dns_dbiterator_next(iterator) \
	dns__dbiterator_next(iterator DNS__DB_FILELINE)
isc_result_t
dns__dbiterator_next(dns_dbiterator_t *iterator DNS__DB_FLARG);
/*%<
 * Move the node cursor to the next node in the database (if any).
 *
 * Requires:
 *\li	'iterator' is a valid iterator.
 *
 * Returns:
 *\li	#ISC_R_SUCCESS
 *\li	#ISC_R_NOMORE			There are no more nodes in the
 *					database.
 *
 *\li	Other results are possible, depending on the DB implementation.
 */

#define dns_dbiterator_current(iterator, nodep, name) \
	dns__dbiterator_current(iterator, nodep, name DNS__DB_FILELINE)
isc_result_t
dns__dbiterator_current(dns_dbiterator_t *iterator, dns_dbnode_t **nodep,
			dns_name_t *name DNS__DB_FLARG);
/*%<
 * Return the current node.
 *
 * Notes:
 *\li	If 'name' is not NULL, it will be set to the name of the node.
 *
 * Requires:
 *\li	'iterator' is a valid iterator.
 *
 *\li	nodep != NULL && *nodep == NULL
 *
 *\li	The node cursor of 'iterator' is at a valid location (i.e. the
 *	result of last call to a cursor movement command was ISC_R_SUCCESS).
 *
 *\li	'name' is NULL, or is a valid name with a dedicated buffer.
 *
 * Returns:
 *
 *\li	#ISC_R_SUCCESS
 *\li	#DNS_R_NEWORIGIN
 *      If this iterator was created with 'relative_names' set to true,
 *      then #DNS_R_NEWORIGIN will be returned when there is a change in
 *      origin to which the names are relative.  This result can occur only
 *      when 'name' is not NULL.  This is also a successful result.
 *
 *\li	Other results are possible, depending on the DB implementation.
 */

isc_result_t
dns_dbiterator_pause(dns_dbiterator_t *iterator);
/*%<
 * Pause iteration.
 *
 * Calling a cursor movement method or dns_dbiterator_current() may cause
 * database locks to be acquired.  Rather than reacquire these locks every
 * time one of these routines is called, the locks may simply be held.
 * Calling dns_dbiterator_pause() releases any such locks.  Iterator clients
 * should call this routine any time they are not going to execute another
 * iterator method in the immediate future.
 *
 * Requires:
 *\li	'iterator' is a valid iterator.
 *
 * Ensures:
 *\li	Any database locks being held for efficiency of iterator access are
 *	released.
 *
 * Returns:
 *\li	#ISC_R_SUCCESS
 *
 *\li	Other results are possible, depending on the DB implementation.
 */

isc_result_t
dns_dbiterator_origin(dns_dbiterator_t *iterator, dns_name_t *name);
/*%<
 * Return the origin to which returned node names are relative.
 *
 * Requires:
 *
 *\li	'iterator' is a valid relative_names iterator.
 *
 *\li	'name' is a valid name with a dedicated buffer.
 *
 * Returns:
 *
 *\li	#ISC_R_SUCCESS
 *\li	#ISC_R_NOSPACE
 *
 *\li	Other results are possible, depending on the DB implementation.
 */

ISC_LANG_ENDDECLS
