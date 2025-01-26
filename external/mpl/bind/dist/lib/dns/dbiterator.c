/*	$NetBSD: dbiterator.c,v 1.7 2025/01/26 16:25:22 christos Exp $	*/

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

/*! \file */

#include <stdbool.h>

#include <isc/util.h>

#include <dns/dbiterator.h>
#include <dns/name.h>

void
dns__dbiterator_destroy(dns_dbiterator_t **iteratorp DNS__DB_FLARG) {
	/*
	 * Destroy '*iteratorp'.
	 */

	REQUIRE(iteratorp != NULL);
	REQUIRE(DNS_DBITERATOR_VALID(*iteratorp));

	(*iteratorp)->methods->destroy(iteratorp DNS__DB_FLARG_PASS);

	ENSURE(*iteratorp == NULL);
}

isc_result_t
dns__dbiterator_first(dns_dbiterator_t *iterator DNS__DB_FLARG) {
	/*
	 * Move the node cursor to the first node in the database (if any).
	 */

	REQUIRE(DNS_DBITERATOR_VALID(iterator));

	return iterator->methods->first(iterator DNS__DB_FLARG_PASS);
}

isc_result_t
dns__dbiterator_last(dns_dbiterator_t *iterator DNS__DB_FLARG) {
	/*
	 * Move the node cursor to the first node in the database (if any).
	 */

	REQUIRE(DNS_DBITERATOR_VALID(iterator));

	return iterator->methods->last(iterator DNS__DB_FLARG_PASS);
}

isc_result_t
dns__dbiterator_seek(dns_dbiterator_t *iterator,
		     const dns_name_t *name DNS__DB_FLARG) {
	/*
	 * Move the node cursor to the node with name 'name'.
	 */

	REQUIRE(DNS_DBITERATOR_VALID(iterator));

	return iterator->methods->seek(iterator, name DNS__DB_FLARG_PASS);
}

isc_result_t
dns__dbiterator_prev(dns_dbiterator_t *iterator DNS__DB_FLARG) {
	/*
	 * Move the node cursor to the previous node in the database (if any).
	 */

	REQUIRE(DNS_DBITERATOR_VALID(iterator));

	return iterator->methods->prev(iterator DNS__DB_FLARG_PASS);
}

isc_result_t
dns__dbiterator_next(dns_dbiterator_t *iterator DNS__DB_FLARG) {
	/*
	 * Move the node cursor to the next node in the database (if any).
	 */

	REQUIRE(DNS_DBITERATOR_VALID(iterator));

	return iterator->methods->next(iterator DNS__DB_FLARG_PASS);
}

isc_result_t
dns__dbiterator_current(dns_dbiterator_t *iterator, dns_dbnode_t **nodep,
			dns_name_t *name DNS__DB_FLARG) {
	/*
	 * Return the current node.
	 */

	REQUIRE(DNS_DBITERATOR_VALID(iterator));
	REQUIRE(nodep != NULL && *nodep == NULL);
	REQUIRE(name == NULL || dns_name_hasbuffer(name));

	return iterator->methods->current(iterator, nodep,
					  name DNS__DB_FLARG_PASS);
}

isc_result_t
dns_dbiterator_pause(dns_dbiterator_t *iterator) {
	/*
	 * Pause iteration.
	 */

	REQUIRE(DNS_DBITERATOR_VALID(iterator));

	return iterator->methods->pause(iterator);
}

isc_result_t
dns_dbiterator_origin(dns_dbiterator_t *iterator, dns_name_t *name) {
	/*
	 * Return the origin to which returned node names are relative.
	 */

	REQUIRE(DNS_DBITERATOR_VALID(iterator));
	REQUIRE(iterator->relative_names);
	REQUIRE(dns_name_hasbuffer(name));

	return iterator->methods->origin(iterator, name);
}
