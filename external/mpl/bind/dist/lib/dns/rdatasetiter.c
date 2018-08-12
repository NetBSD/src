/*	$NetBSD: rdatasetiter.c,v 1.1.1.1 2018/08/12 12:08:14 christos Exp $	*/

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

#include <stddef.h>

#include <isc/util.h>

#include <dns/rdataset.h>
#include <dns/rdatasetiter.h>

void
dns_rdatasetiter_destroy(dns_rdatasetiter_t **iteratorp) {
	/*
	 * Destroy '*iteratorp'.
	 */

	REQUIRE(iteratorp != NULL);
	REQUIRE(DNS_RDATASETITER_VALID(*iteratorp));

	(*iteratorp)->methods->destroy(iteratorp);

	ENSURE(*iteratorp == NULL);
}

isc_result_t
dns_rdatasetiter_first(dns_rdatasetiter_t *iterator) {
	/*
	 * Move the rdataset cursor to the first rdataset at the node (if any).
	 */

	REQUIRE(DNS_RDATASETITER_VALID(iterator));

	return (iterator->methods->first(iterator));
}

isc_result_t
dns_rdatasetiter_next(dns_rdatasetiter_t *iterator) {
	/*
	 * Move the rdataset cursor to the next rdataset at the node (if any).
	 */

	REQUIRE(DNS_RDATASETITER_VALID(iterator));

	return (iterator->methods->next(iterator));
}

void
dns_rdatasetiter_current(dns_rdatasetiter_t *iterator,
			 dns_rdataset_t *rdataset)
{
	/*
	 * Return the current rdataset.
	 */

	REQUIRE(DNS_RDATASETITER_VALID(iterator));
	REQUIRE(DNS_RDATASET_VALID(rdataset));
	REQUIRE(! dns_rdataset_isassociated(rdataset));

	iterator->methods->current(iterator, rdataset);
}
