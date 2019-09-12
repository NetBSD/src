/*	$NetBSD: stats.c,v 1.4.4.1 2019/09/12 19:18:16 martin Exp $	*/

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

#include <inttypes.h>
#include <string.h>

#include <isc/atomic.h>
#include <isc/buffer.h>
#include <isc/magic.h>
#include <isc/mem.h>
#include <isc/platform.h>
#include <isc/print.h>
#include <isc/refcount.h>
#include <isc/stats.h>
#include <isc/util.h>

#define ISC_STATS_MAGIC			ISC_MAGIC('S', 't', 'a', 't')
#define ISC_STATS_VALID(x)		ISC_MAGIC_VALID(x, ISC_STATS_MAGIC)

#if (defined(_WIN32) && !defined(_WIN64)) || !defined(_LP64)
typedef atomic_int_fast32_t isc_stat_t;
#else
typedef atomic_int_fast64_t isc_stat_t;
#endif

struct isc_stats {
	unsigned int		magic;
	isc_mem_t		*mctx;
	isc_refcount_t		refs;
	int			ncounters;
	isc_stat_t		*counters;
};

static isc_result_t
create_stats(isc_mem_t *mctx, int ncounters, isc_stats_t **statsp) {
	isc_stats_t *stats;

	REQUIRE(statsp != NULL && *statsp == NULL);

	stats = isc_mem_get(mctx, sizeof(*stats));
	stats->counters = isc_mem_get(mctx, sizeof(isc_stat_t) * ncounters);
	isc_refcount_init(&stats->refs, 1);
	memset(stats->counters, 0, sizeof(isc_stat_t) * ncounters);
	stats->mctx = NULL;
	isc_mem_attach(mctx, &stats->mctx);
	stats->ncounters = ncounters;
	stats->magic = ISC_STATS_MAGIC;
	*statsp = stats;

	return (ISC_R_SUCCESS);
}

void
isc_stats_attach(isc_stats_t *stats, isc_stats_t **statsp) {
	REQUIRE(ISC_STATS_VALID(stats));
	REQUIRE(statsp != NULL && *statsp == NULL);

	isc_refcount_increment(&stats->refs);
	*statsp = stats;
}

void
isc_stats_detach(isc_stats_t **statsp) {
	isc_stats_t *stats;

	REQUIRE(statsp != NULL && ISC_STATS_VALID(*statsp));

	stats = *statsp;
	*statsp = NULL;

	if (isc_refcount_decrement(&stats->refs) == 1) {
		isc_mem_put(stats->mctx, stats->counters,
			    sizeof(isc_stat_t) * stats->ncounters);
		isc_mem_putanddetach(&stats->mctx, stats, sizeof(*stats));
	}
}

int
isc_stats_ncounters(isc_stats_t *stats) {
	REQUIRE(ISC_STATS_VALID(stats));

	return (stats->ncounters);
}

isc_result_t
isc_stats_create(isc_mem_t *mctx, isc_stats_t **statsp, int ncounters) {
	REQUIRE(statsp != NULL && *statsp == NULL);

	return (create_stats(mctx, ncounters, statsp));
}

void
isc_stats_increment(isc_stats_t *stats, isc_statscounter_t counter) {
	REQUIRE(ISC_STATS_VALID(stats));
	REQUIRE(counter < stats->ncounters);

	atomic_fetch_add_explicit(&stats->counters[counter], 1,
				  memory_order_relaxed);
}

void
isc_stats_decrement(isc_stats_t *stats, isc_statscounter_t counter) {
	REQUIRE(ISC_STATS_VALID(stats));
	REQUIRE(counter < stats->ncounters);

	atomic_fetch_sub_explicit(&stats->counters[counter], 1,
				  memory_order_relaxed);
}

void
isc_stats_dump(isc_stats_t *stats, isc_stats_dumper_t dump_fn,
	       void *arg, unsigned int options)
{
	int i;

	REQUIRE(ISC_STATS_VALID(stats));

	for (i = 0; i < stats->ncounters; i++) {
		uint32_t counter = atomic_load_explicit(&stats->counters[i],
							memory_order_relaxed);
		if ((options & ISC_STATSDUMP_VERBOSE) == 0 && counter == 0) {
			continue;
		}
		dump_fn((isc_statscounter_t)i, counter, arg);
	}
}

void
isc_stats_set(isc_stats_t *stats, uint64_t val,
	      isc_statscounter_t counter)
{
	REQUIRE(ISC_STATS_VALID(stats));
	REQUIRE(counter < stats->ncounters);

	atomic_store_explicit(&stats->counters[counter], val,
			      memory_order_relaxed);
}
