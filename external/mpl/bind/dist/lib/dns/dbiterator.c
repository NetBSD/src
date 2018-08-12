/*	$NetBSD: dbiterator.c,v 1.2 2018/08/12 13:02:35 christos Exp $	*/

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


/*! \file */

#include <config.h>

#include <isc/util.h>

#include <dns/dbiterator.h>
#include <dns/name.h>

void
dns_dbiterator_destroy(dns_dbiterator_t **iteratorp) {
	/*
	 * Destroy '*iteratorp'.
	 */

	REQUIRE(iteratorp != NULL);
	REQUIRE(DNS_DBITERATOR_VALID(*iteratorp));

	(*iteratorp)->methods->destroy(iteratorp);

	ENSURE(*iteratorp == NULL);
}

isc_result_t
dns_dbiterator_first(dns_dbiterator_t *iterator) {
	/*
	 * Move the node cursor to the first node in the database (if any).
	 */

	REQUIRE(DNS_DBITERATOR_VALID(iterator));

	return (iterator->methods->first(iterator));
}

isc_result_t
dns_dbiterator_last(dns_dbiterator_t *iterator) {
	/*
	 * Move the node cursor to the first node in the database (if any).
	 */

	REQUIRE(DNS_DBITERATOR_VALID(iterator));

	return (iterator->methods->last(iterator));
}

isc_result_t
dns_dbiterator_seek(dns_dbiterator_t *iterator, const dns_name_t *name) {
	/*
	 * Move the node cursor to the node with name 'name'.
	 */

	REQUIRE(DNS_DBITERATOR_VALID(iterator));

	return (iterator->methods->seek(iterator, name));
}

isc_result_t
dns_dbiterator_prev(dns_dbiterator_t *iterator) {
	/*
	 * Move the node cursor to the previous node in the database (if any).
	 */

	REQUIRE(DNS_DBITERATOR_VALID(iterator));

	return (iterator->methods->prev(iterator));
}

isc_result_t
dns_dbiterator_next(dns_dbiterator_t *iterator) {
	/*
	 * Move the node cursor to the next node in the database (if any).
	 */

	REQUIRE(DNS_DBITERATOR_VALID(iterator));

	return (iterator->methods->next(iterator));
}

isc_result_t
dns_dbiterator_current(dns_dbiterator_t *iterator, dns_dbnode_t **nodep,
		       dns_name_t *name)
{
	/*
	 * Return the current node.
	 */

	REQUIRE(DNS_DBITERATOR_VALID(iterator));
	REQUIRE(nodep != NULL && *nodep == NULL);
	REQUIRE(name == NULL || dns_name_hasbuffer(name));

	return (iterator->methods->current(iterator, nodep, name));
}

isc_result_t
dns_dbiterator_pause(dns_dbiterator_t *iterator) {
	/*
	 * Pause iteration.
	 */

	REQUIRE(DNS_DBITERATOR_VALID(iterator));

	return (iterator->methods->pause(iterator));
}

isc_result_t
dns_dbiterator_origin(dns_dbiterator_t *iterator, dns_name_t *name) {

	/*
	 * Return the origin to which returned node names are relative.
	 */

	REQUIRE(DNS_DBITERATOR_VALID(iterator));
	REQUIRE(iterator->relative_names);
	REQUIRE(dns_name_hasbuffer(name));

	return (iterator->methods->origin(iterator, name));
}

void
dns_dbiterator_setcleanmode(dns_dbiterator_t *iterator, isc_boolean_t mode) {
	REQUIRE(DNS_DBITERATOR_VALID(iterator));

	iterator->cleaning = mode;
}
