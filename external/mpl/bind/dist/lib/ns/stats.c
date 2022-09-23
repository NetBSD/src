/*	$NetBSD: stats.c,v 1.7 2022/09/23 12:15:36 christos Exp $	*/

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

#include <isc/magic.h>
#include <isc/mem.h>
#include <isc/refcount.h>
#include <isc/stats.h>
#include <isc/util.h>

#include <ns/stats.h>

#define NS_STATS_MAGIC	  ISC_MAGIC('N', 's', 't', 't')
#define NS_STATS_VALID(x) ISC_MAGIC_VALID(x, NS_STATS_MAGIC)

struct ns_stats {
	/*% Unlocked */
	unsigned int magic;
	isc_mem_t *mctx;
	isc_stats_t *counters;
	isc_refcount_t references;
};

void
ns_stats_attach(ns_stats_t *stats, ns_stats_t **statsp) {
	REQUIRE(NS_STATS_VALID(stats));
	REQUIRE(statsp != NULL && *statsp == NULL);

	isc_refcount_increment(&stats->references);

	*statsp = stats;
}

void
ns_stats_detach(ns_stats_t **statsp) {
	ns_stats_t *stats;

	REQUIRE(statsp != NULL && NS_STATS_VALID(*statsp));

	stats = *statsp;
	*statsp = NULL;

	if (isc_refcount_decrement(&stats->references) == 1) {
		isc_stats_detach(&stats->counters);
		isc_refcount_destroy(&stats->references);
		isc_mem_putanddetach(&stats->mctx, stats, sizeof(*stats));
	}
}

isc_result_t
ns_stats_create(isc_mem_t *mctx, int ncounters, ns_stats_t **statsp) {
	ns_stats_t *stats;
	isc_result_t result;

	REQUIRE(statsp != NULL && *statsp == NULL);

	stats = isc_mem_get(mctx, sizeof(*stats));
	stats->counters = NULL;

	isc_refcount_init(&stats->references, 1);

	result = isc_stats_create(mctx, &stats->counters, ncounters);
	if (result != ISC_R_SUCCESS) {
		goto clean_mem;
	}

	stats->magic = NS_STATS_MAGIC;
	stats->mctx = NULL;
	isc_mem_attach(mctx, &stats->mctx);
	*statsp = stats;

	return (ISC_R_SUCCESS);

clean_mem:
	isc_mem_put(mctx, stats, sizeof(*stats));

	return (result);
}

/*%
 * Increment/Decrement methods
 */
void
ns_stats_increment(ns_stats_t *stats, isc_statscounter_t counter) {
	REQUIRE(NS_STATS_VALID(stats));

	isc_stats_increment(stats->counters, counter);
}

void
ns_stats_decrement(ns_stats_t *stats, isc_statscounter_t counter) {
	REQUIRE(NS_STATS_VALID(stats));

	isc_stats_decrement(stats->counters, counter);
}

isc_stats_t *
ns_stats_get(ns_stats_t *stats) {
	REQUIRE(NS_STATS_VALID(stats));

	return (stats->counters);
}

void
ns_stats_update_if_greater(ns_stats_t *stats, isc_statscounter_t counter,
			   isc_statscounter_t value) {
	REQUIRE(NS_STATS_VALID(stats));

	isc_stats_update_if_greater(stats->counters, counter, value);
}

isc_statscounter_t
ns_stats_get_counter(ns_stats_t *stats, isc_statscounter_t counter) {
	REQUIRE(NS_STATS_VALID(stats));

	return (isc_stats_get_counter(stats->counters, counter));
}
