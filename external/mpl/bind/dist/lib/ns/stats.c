/*	$NetBSD: stats.c,v 1.1.1.1 2018/08/12 12:08:07 christos Exp $	*/

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

#include <isc/magic.h>
#include <isc/mem.h>
#include <isc/stats.h>
#include <isc/util.h>

#include <ns/stats.h>

#define NS_STATS_MAGIC			ISC_MAGIC('N', 's', 't', 't')
#define NS_STATS_VALID(x)		ISC_MAGIC_VALID(x, NS_STATS_MAGIC)

struct ns_stats {
	/*% Unlocked */
	unsigned int	magic;
	isc_mem_t	*mctx;
	isc_mutex_t	lock;
	isc_stats_t	*counters;

	/*%  Locked by lock */
	unsigned int	references;
};

void
ns_stats_attach(ns_stats_t *stats, ns_stats_t **statsp) {
	REQUIRE(NS_STATS_VALID(stats));
	REQUIRE(statsp != NULL && *statsp == NULL);

	LOCK(&stats->lock);
	stats->references++;
	UNLOCK(&stats->lock);

	*statsp = stats;
}

void
ns_stats_detach(ns_stats_t **statsp) {
	ns_stats_t *stats;

	REQUIRE(statsp != NULL && NS_STATS_VALID(*statsp));

	stats = *statsp;
	*statsp = NULL;

	LOCK(&stats->lock);
	stats->references--;
	UNLOCK(&stats->lock);

	if (stats->references == 0) {
		isc_stats_detach(&stats->counters);
		DESTROYLOCK(&stats->lock);
		isc_mem_putanddetach(&stats->mctx, stats, sizeof(*stats));
	}
}

isc_result_t
ns_stats_create(isc_mem_t *mctx, int ncounters, ns_stats_t **statsp) {
	ns_stats_t *stats;
	isc_result_t result;

	REQUIRE(statsp != NULL && *statsp == NULL);

	stats = isc_mem_get(mctx, sizeof(*stats));
	if (stats == NULL)
		return (ISC_R_NOMEMORY);

	stats->counters = NULL;
	stats->references = 1;

	result = isc_mutex_init(&stats->lock);
	if (result != ISC_R_SUCCESS)
		goto clean_stats;

	result = isc_stats_create(mctx, &stats->counters, ncounters);
	if (result != ISC_R_SUCCESS)
		goto clean_mutex;

	stats->magic = NS_STATS_MAGIC;
	stats->mctx = NULL;
	isc_mem_attach(mctx, &stats->mctx);
	*statsp = stats;

	return (ISC_R_SUCCESS);

  clean_mutex:
	DESTROYLOCK(&stats->lock);
  clean_stats:
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
