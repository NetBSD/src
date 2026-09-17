/*	$NetBSD: dnssecsignstats_test.c,v 1.1.1.1 2026/09/17 17:45:09 christos Exp $	*/

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

#include <inttypes.h>
#include <sched.h> /* IWYU pragma: keep */
#include <setjmp.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>

#define UNIT_TESTING
#include <cmocka.h>

#include <isc/atomic.h>
#include <isc/stats.h>
#include <isc/thread.h>
#include <isc/urcu.h>
#include <isc/util.h>

#include <dns/stats.h>

#include <tests/dns.h>

#define DNSSEC_KEY(alg, id) ((uint32_t)(alg) << 16 | (id))

typedef struct {
	uint32_t key;
	uint64_t value;
} dumped_entry_t;

typedef struct {
	dumped_entry_t entries[64];
	size_t count;
} dump_t;

static void
collect(uint32_t key, uint64_t value, void *arg) {
	dump_t *dump = arg;

	assert_true(dump->count < ARRAY_SIZE(dump->entries));
	dump->entries[dump->count++] = (dumped_entry_t){
		.key = key,
		.value = value,
	};
}

static bool
find_value(const dump_t *dump, uint32_t key, uint64_t *valuep) {
	for (size_t i = 0; i < dump->count; i++) {
		if (dump->entries[i].key == key) {
			*valuep = dump->entries[i].value;
			return true;
		}
	}

	return false;
}

ISC_RUN_TEST_IMPL(basic) {
	dns_stats_t *stats = NULL;
	dump_t dump = { 0 };
	uint64_t value = 0;

	UNUSED(state);

	dns_dnssecsignstats_create(mctx, &stats);
	for (dns_keytag_t id = 1; id <= 40; id++) {
		dns_dnssecsignstats_increment(stats, id, 8,
					      dns_dnssecsignstats_sign);
	}
	dns_dnssecsignstats_increment(stats, 1, 8, dns_dnssecsignstats_sign);
	dns_dnssecsignstats_increment(stats, 1, 8, dns_dnssecsignstats_refresh);
	dns_dnssecsignstats_increment(stats, 1, 13, dns_dnssecsignstats_sign);

	dns_dnssecsignstats_dump(stats, dns_dnssecsignstats_sign, collect,
				 &dump, 0);
	assert_int_equal(dump.count, 41);
	assert_true(find_value(&dump, DNSSEC_KEY(8, 1), &value));
	assert_int_equal(value, 2);
	assert_true(find_value(&dump, DNSSEC_KEY(13, 1), &value));
	assert_int_equal(value, 1);

	dump = (dump_t){ 0 };
	dns_dnssecsignstats_dump(stats, dns_dnssecsignstats_refresh, collect,
				 &dump, 0);
	assert_int_equal(dump.count, 1);
	assert_true(find_value(&dump, DNSSEC_KEY(8, 1), &value));
	assert_int_equal(value, 1);

	dump = (dump_t){ 0 };
	dns_dnssecsignstats_dump(stats, dns_dnssecsignstats_refresh, collect,
				 &dump, ISC_STATSDUMP_VERBOSE);
	assert_int_equal(dump.count, 41);
	assert_true(find_value(&dump, DNSSEC_KEY(13, 1), &value));
	assert_int_equal(value, 0);

	dns_dnssecsignstats_clear(stats, 1, 8);
	dump = (dump_t){ 0 };
	dns_dnssecsignstats_dump(stats, dns_dnssecsignstats_sign, collect,
				 &dump, 0);
	assert_int_equal(dump.count, 40);
	assert_false(find_value(&dump, DNSSEC_KEY(8, 1), &value));

	dns_dnssecsignstats_increment(stats, 1, 8, dns_dnssecsignstats_refresh);
	dump = (dump_t){ 0 };
	dns_dnssecsignstats_dump(stats, dns_dnssecsignstats_refresh, collect,
				 &dump, 0);
	assert_true(find_value(&dump, DNSSEC_KEY(8, 1), &value));
	assert_int_equal(value, 1);

	dns_stats_detach(&stats);
	rcu_barrier();
}

#define STRESS_READER_THREADS 4
#define STRESS_ITERATIONS     20000
#define STRESS_KEY_COUNT      64

typedef struct {
	dns_stats_t *stats;
	atomic_uint_fast32_t readers_ready;
	atomic_uint_fast64_t dumps;
	atomic_bool invalid;
	atomic_bool stop;
} stress_test_t;

static void
stress_dump(uint32_t key, uint64_t value, void *arg) {
	stress_test_t *test = arg;
	uint8_t alg = key >> 16;
	dns_keytag_t id = key;

	if ((alg != 8 && alg != 13) || id == 0 || id > STRESS_KEY_COUNT ||
	    value != 1)
	{
		atomic_store_relaxed(&test->invalid, true);
	}
}

static void *
stress_reader(void *arg) {
	stress_test_t *test = arg;

	atomic_fetch_add_relaxed(&test->readers_ready, 1);
	while (!atomic_load_acquire(&test->stop)) {
		dns_dnssecsignstats_dump(test->stats, dns_dnssecsignstats_sign,
					 stress_dump, test, 0);
		dns_dnssecsignstats_dump(test->stats,
					 dns_dnssecsignstats_refresh,
					 stress_dump, test, 0);
		atomic_fetch_add_relaxed(&test->dumps, 1);
	}

	return NULL;
}

ISC_RUN_TEST_IMPL(concurrent_clear_dump) {
	dns_stats_t *stats = NULL;
	stress_test_t test = { 0 };
	isc_thread_t threads[STRESS_READER_THREADS];

	UNUSED(state);

	dns_dnssecsignstats_create(mctx, &stats);
	test.stats = stats;
	atomic_init(&test.readers_ready, 0);
	atomic_init(&test.dumps, 0);
	atomic_init(&test.invalid, false);
	atomic_init(&test.stop, false);

	for (size_t i = 0; i < ARRAY_SIZE(threads); i++) {
		isc_thread_create(stress_reader, &test, &threads[i]);
	}
	while (atomic_load_acquire(&test.readers_ready) < ARRAY_SIZE(threads)) {
		isc_thread_yield();
	}

	for (size_t i = 0; i < STRESS_ITERATIONS; i++) {
		dns_keytag_t id = i % STRESS_KEY_COUNT + 1;
		uint8_t alg = i % 2 == 0 ? 8 : 13;

		dns_dnssecsignstats_increment(stats, id, alg,
					      dns_dnssecsignstats_sign);
		dns_dnssecsignstats_increment(stats, id, alg,
					      dns_dnssecsignstats_refresh);
		dns_dnssecsignstats_clear(stats, id, alg);
	}

	atomic_store_release(&test.stop, true);
	for (size_t i = 0; i < ARRAY_SIZE(threads); i++) {
		isc_thread_join(threads[i], NULL);
	}

	assert_false(atomic_load_relaxed(&test.invalid));
	assert_true(atomic_load_relaxed(&test.dumps) > 0);

	dns_stats_detach(&stats);
	rcu_barrier();
}

ISC_TEST_LIST_START
ISC_TEST_ENTRY(basic)
ISC_TEST_ENTRY(concurrent_clear_dump)
ISC_TEST_LIST_END

ISC_TEST_MAIN
