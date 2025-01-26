/*	$NetBSD: qpmulti.c,v 1.2 2025/01/26 16:25:47 christos Exp $	*/

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

#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <unistd.h>

#include <isc/async.h>
#include <isc/barrier.h>
#include <isc/list.h>
#include <isc/log.h>
#include <isc/loop.h>
#include <isc/magic.h>
#include <isc/mem.h>
#include <isc/mutex.h>
#include <isc/os.h>
#include <isc/random.h>
#include <isc/refcount.h>
#include <isc/rwlock.h>
#include <isc/thread.h>
#include <isc/tid.h>
#include <isc/time.h>
#include <isc/timer.h>
#include <isc/urcu.h>
#include <isc/util.h>
#include <isc/uv.h>

#include <dns/log.h>
#include <dns/qp.h>
#include <dns/types.h>

#include "loop_p.h"
#include "qp_p.h"

#include <tests/qp.h>

#define ITEM_COUNT	 ((size_t)1000000)
#define RUNTIME		 (0.25 * NS_PER_SEC)
#define MAX_OPS_PER_LOOP (1 << 10)

#define VERBOSE 0
#define ZIPF	0

#if VERBOSE
#define TRACE(fmt, ...)                                                     \
	isc_log_write(dns_lctx, DNS_LOGCATEGORY_DATABASE, DNS_LOGMODULE_QP, \
		      ISC_LOG_DEBUG(7), "%s:%d:%s():t%d: " fmt, __FILE__,   \
		      __LINE__, __func__, isc_tid(), ##__VA_ARGS__)
#else
#define TRACE(...)
#endif

#if ZIPF
/*
 * Zipf rejection sampling derived from code by Jason Crease
 * https://jasoncrease.medium.com/rejection-sampling-the-zipf-distribution-6b359792cffa
 */
static uint32_t
rand_zipf(uint32_t max, double skew) {
	double s = skew;
	double t = (pow(max, 1 - s) - s) / (1 - s);
	for (;;) {
		double p = t * (double)isc_random32() / UINT32_MAX;
		double invB = p <= 1 ? p : pow(p * (1 - s) + s, 1 / (1 - s));
		uint32_t sample = (uint32_t)(invB + 1);
		double ratio = sample <= 1 ? pow(sample, -s)
					   : pow(sample, -s) / pow(invB, -s);
		if (ratio > (double)isc_random32() / UINT32_MAX) {
			return sample - 1;
		}
	}
}
#endif

static struct {
	size_t len;
	bool present;
	dns_qpkey_t key;
} *item;

static void
item_refcount(void *ctx, void *pval, uint32_t ival) {
	UNUSED(ctx);
	UNUSED(pval);
	UNUSED(ival);
}

static size_t
item_makekey(dns_qpkey_t key, void *ctx, void *pval, uint32_t ival) {
	UNUSED(ctx);
	UNUSED(pval);
	memmove(key, item[ival].key, item[ival].len);
	return item[ival].len;
}

static void
benchname(void *ctx, char *buf, size_t size) {
	UNUSED(ctx);
	strlcpy(buf, "bench", size);
}

const dns_qpmethods_t item_methods = {
	item_refcount,
	item_refcount,
	item_makekey,
	benchname,
};

static uint8_t
random_byte(void) {
	return isc_random_uniform(SHIFT_OFFSET - SHIFT_NOBYTE) + SHIFT_NOBYTE;
}

static void
init_items(isc_mem_t *mctx) {
	dns_qp_t *qp = NULL;
	uint64_t start;

	start = isc_time_monotonic();
	item = isc_mem_callocate(mctx, ITEM_COUNT, sizeof(*item));

	/* ensure there are no duplicate names */
	dns_qp_create(mctx, &item_methods, NULL, &qp);
	for (size_t i = 0; i < ITEM_COUNT; i++) {
		do {
			size_t len = isc_random_uniform(16) + 4;
			item[i].len = len;
			for (size_t off = 0; off < len; off++) {
				item[i].key[off] = random_byte();
			}
			item[i].key[len] = SHIFT_NOBYTE;
		} while (dns_qp_getkey(qp, item[i].key, item[i].len, NULL,
				       NULL) == ISC_R_SUCCESS);
		INSIST(dns_qp_insert(qp, &item[i], i) == ISC_R_SUCCESS);
	}
	dns_qp_destroy(&qp);

	double time = (double)(isc_time_monotonic() - start) / NS_PER_SEC;
	printf("%f sec to create %zu items, %f/sec %zu bytes\n", time,
	       ITEM_COUNT, ITEM_COUNT / time, ITEM_COUNT * sizeof(*item));
}

static void
init_logging(isc_mem_t *mctx) {
	isc_result_t result;
	isc_logdestination_t destination;
	isc_logconfig_t *logconfig = NULL;
	isc_log_t *lctx = NULL;

	isc_log_create(mctx, &lctx, &logconfig);
	isc_log_setcontext(lctx);
	dns_log_init(lctx);
	dns_log_setcontext(lctx);

	destination.file.stream = stderr;
	destination.file.name = NULL;
	destination.file.versions = ISC_LOG_ROLLNEVER;
	destination.file.maximum_size = 0;
	isc_log_createchannel(logconfig, "stderr", ISC_LOG_TOFILEDESC,
			      ISC_LOG_DYNAMIC, &destination,
			      ISC_LOG_PRINTPREFIX | ISC_LOG_PRINTTIME |
				      ISC_LOG_ISO8601);
#if VERBOSE
	isc_log_setdebuglevel(lctx, 7);
#endif

	result = isc_log_usechannel(logconfig, "stderr",
				    ISC_LOGCATEGORY_DEFAULT, NULL);
	INSIST(result == ISC_R_SUCCESS);
}

static void
collect(void *);

struct thread_args {
	struct bench_state *bctx; /* (in) */
	isc_barrier_t *barrier;	  /* (in) */
	isc_loopmgr_t *loopmgr;	  /* (in) */
	isc_job_t job;		  /* (in) */
	isc_job_cb cb;		  /* (in) */
	dns_qpmulti_t *multi;	  /* (in) */
	double zipf_skew;	  /* (in) */
	uint32_t max_item;	  /* (in) */
	uint32_t ops_per_tx;	  /* (in) */
	uint32_t tx_per_loop;	  /* (in) */
	uint32_t absent;	  /* items not found or inserted (out) */
	uint32_t present;	  /* items found or deleted (out) */
	uint32_t compactions;	  /* (out) */
	uint64_t transactions;	  /* (out) */
	isc_nanosecs_t worked;	  /* (out) */
	isc_nanosecs_t start;	  /* (out) */
	isc_nanosecs_t stop;	  /* (out) */
};

static void
first_loop(void *varg) {
	struct thread_args *args = varg;
	isc_loop_t *loop = isc_loop();

	isc_job_run(loop, &args->job, args->cb, args);

	isc_barrier_wait(args->barrier);
	args->start = isc_time_monotonic();
}

static void
next_loop(struct thread_args *args, isc_nanosecs_t start) {
	isc_nanosecs_t stop = isc_time_monotonic();

	args->worked += stop - start;
	args->stop = stop;
	if (args->stop - args->start < RUNTIME) {
		isc_job_run(isc_loop(), &args->job, args->cb, args);
		return;
	}
	isc_async_run(isc_loop_main(args->loopmgr), collect, args);
}

#if ZIPF
static void
read_zipf(void *varg) {
	struct thread_args *args = varg;
	isc_nanosecs_t start;

	/* outside time because it is v slow */
	uint32_t r[args->tx_per_loop][args->ops_per_tx];
	for (uint32_t tx = 0; tx < args->tx_per_loop; tx++) {
		for (uint32_t op = 0; op < args->ops_per_tx; op++) {
			r[tx][op] = rand_zipf(args->max_item, args->zipf_skew);
		}
	}

	start = isc_time_monotonic();
	for (uint32_t tx = 0; tx < args->tx_per_loop; tx++) {
		args->transactions++;
		dns_qpread_t qp;
		dns_qpmulti_query(args->multi, &qp);
		for (uint32_t op = 0; op < args->ops_per_tx; op++) {
			uint32_t i = r[tx][op];
			isc_result_t result = dns_qp_getkey(
				&qp, item[i].key, item[i].len, NULL, NULL);
			if (result == ISC_R_SUCCESS) {
				args->present++;
			} else {
				args->absent++;
			}
		}
		dns_qpread_destroy(args->multi, &qp);
	}
	next_loop(args, start);
}
#else
#define read_zipf read_transactions
#endif

static void
read_transactions(void *varg) {
	struct thread_args *args = varg;
	isc_nanosecs_t start = isc_time_monotonic();

	for (uint32_t tx = 0; tx < args->tx_per_loop; tx++) {
		args->transactions++;
		dns_qpread_t qp;
		dns_qpmulti_query(args->multi, &qp);
		for (uint32_t op = 0; op < args->ops_per_tx; op++) {
			uint32_t i = isc_random_uniform(args->max_item);
			isc_result_t result = dns_qp_getkey(
				&qp, item[i].key, item[i].len, NULL, NULL);
			if (result == ISC_R_SUCCESS) {
				args->present++;
			} else {
				args->absent++;
			}
		}
		dns_qpread_destroy(args->multi, &qp);
	}
	next_loop(args, start);
}

static void
mutate_transactions(void *varg) {
	struct thread_args *args = varg;
	isc_nanosecs_t start = isc_time_monotonic();

	for (uint32_t tx = 0; tx < args->tx_per_loop; tx++) {
		dns_qp_t *qp = NULL;
		dns_qpmulti_write(args->multi, &qp);
		for (uint32_t op = 0; op < args->ops_per_tx; op++) {
			uint32_t i = isc_random_uniform(args->max_item);
			if (item[i].present) {
				isc_result_t result = dns_qp_deletekey(
					qp, item[i].key, item[i].len, NULL,
					NULL);
				INSIST(result == ISC_R_SUCCESS);
				item[i].present = false;
				args->present++;
			} else {
				isc_result_t result =
					dns_qp_insert(qp, &item[i], i);
				INSIST(result == ISC_R_SUCCESS);
				item[i].present = true;
				args->absent++;
			}
		}
		/*
		 * We would normally use DNS_QPGC_MAYBE, but here we do the
		 * fragmented check ourself so we can count compactions
		 */
		if (dns_qp_memusage(qp).fragmented) {
			dns_qp_compact(qp, DNS_QPGC_NOW);
			args->compactions++;
		}
		dns_qpmulti_commit(args->multi, &qp);
		args->transactions++;
	}
	next_loop(args, start);
}

enum benchmode {
	init,
	vary_max_items_rw,
	vary_max_items_ro,
	vary_mut_read,
	vary_read_only,
	vary_mut_ops_per_tx,
	vary_mut_tx_per_loop,
	vary_read_ops_per_tx_rw,
	vary_read_ops_per_tx_ro,
	vary_read_tx_per_loop_rw,
	vary_read_tx_per_loop_ro,
	vary_zipf_skew,
};

struct bench_state {
	isc_mem_t *mctx;
	isc_barrier_t barrier;
	isc_loopmgr_t *loopmgr;
	dns_qpmulti_t *multi;
	enum benchmode mode;
	size_t bytes;
	size_t qp_bytes;
	size_t qp_items;
	isc_nanosecs_t load_time;
	uint32_t nloops;
	uint32_t waiting;
	uint32_t max_item;
	uint32_t mutate;
	uint32_t mut_ops_per_tx;
	uint32_t mut_tx_per_loop;
	uint32_t readers;
	uint32_t read_ops_per_tx;
	uint32_t read_tx_per_loop;
	double zipf_skew;
	struct thread_args thread[];
};

static void
load_multi(struct bench_state *bctx) {
	dns_qp_t *qp = NULL;
	size_t count = 0;
	uint64_t start;

	dns_qpmulti_create(bctx->mctx, &item_methods, NULL, &bctx->multi);

	/* initial contents of the trie */
	start = isc_time_monotonic();
	dns_qpmulti_update(bctx->multi, &qp);
	for (size_t i = 0; i < bctx->max_item; i++) {
		if (isc_random_uniform(2) == 0) {
			item[i].present = false;
			continue;
		}
		INSIST(dns_qp_insert(qp, &item[i], i) == ISC_R_SUCCESS);
		item[i].present = true;
		count++;
	}
	dns_qp_compact(qp, DNS_QPGC_ALL);
	dns_qpmulti_commit(bctx->multi, &qp);

	bctx->load_time = isc_time_monotonic() - start;
	bctx->qp_bytes = dns_qpmulti_memusage(bctx->multi).bytes;
	bctx->qp_items = count;
}

static void
tsv_header(void) {
	printf("runtime\t");
	printf("elapsed\t");
	printf(" load s\t");
	printf(" B/item\t");
	printf("  items\t");

	printf("    mut\t");
	printf("tx/loop\t");
	printf(" ops/tx\t");
	printf("     gc\t");
	printf("   txns\t");
	printf("    ops\t");
	printf(" work s\t");
	printf("txns/us\t");
	printf(" ops/us\t");

	printf("   read\t");
	printf("tx/loop\t");
	printf(" ops/tx\t");
	printf("  Ktxns\t");
	printf("   Kops\t");
	printf(" work s\t");
	printf("txns/us\t");
	printf(" ops/us\t");
	printf("    raw\t");
	printf("   loop\n");
}

/*
 * This function sets up the parameters for each benchmark run and
 * dispatches the work to the event loops. Each run is part of a
 * series, where most of the parameters are fixed and one parameter is
 * varied. The layout here is somewhat eccentric, in order to keep
 * each series together.
 *
 * A series starts with an `init` block, which sets up the constant
 * parameters and the variable parameter for the first run. Following
 * the `init` block is a `case` label which adjusts the variable
 * parameter for each subsequent run in the series, and checks when
 * the series is finished. At the end of the series, we `goto` the
 * `init` label for the next series.
 */
static void
dispatch(struct bench_state *bctx) {
	switch (bctx->mode) {
	case init:
		goto init_max_items_rw;

	fini:;
		isc_loopmgr_t *loopmgr = bctx->loopmgr;
		dns_qpmulti_destroy(&bctx->multi);
		isc_mem_putanddetach(&bctx->mctx, bctx, bctx->bytes);
		isc_loopmgr_shutdown(loopmgr);
		return;

	init_max_items_rw:
		bctx->mode = vary_max_items_rw;
		printf("\n");
		printf("vary size of trie\n");
		tsv_header();
		bctx->mutate = 1;
		bctx->readers = bctx->nloops - 1;
		bctx->mut_ops_per_tx = 4;
		bctx->mut_tx_per_loop = 4;
		bctx->read_ops_per_tx = 32;
		bctx->read_tx_per_loop = 32;
		bctx->max_item = 10;
		load_multi(bctx);
		break;

	case vary_max_items_rw:
		if (bctx->max_item == ITEM_COUNT) {
			goto init_max_items_ro;
		} else {
			dns_qpmulti_destroy(&bctx->multi);
			bctx->max_item *= 10;
			load_multi(bctx);
		}
		break;

	init_max_items_ro:
		bctx->mode = vary_max_items_ro;
		printf("\n");
		printf("vary size of trie (readonly)\n");
		tsv_header();
		bctx->mutate = 0;
		bctx->readers = bctx->nloops;
		bctx->mut_ops_per_tx = 4;
		bctx->mut_tx_per_loop = 4;
		bctx->read_ops_per_tx = 32;
		bctx->read_tx_per_loop = 32;
		dns_qpmulti_destroy(&bctx->multi);
		bctx->max_item = 10;
		load_multi(bctx);
		break;
	case vary_max_items_ro:
		if (bctx->max_item == ITEM_COUNT) {
			goto init_zipf_skew;
		} else {
			dns_qpmulti_destroy(&bctx->multi);
			bctx->max_item *= 10;
			load_multi(bctx);
		}
		break;

	init_zipf_skew:
		bctx->mode = vary_zipf_skew;
		printf("\n");
		printf("vary zipf skew (readonly) "
		       " [ cache friendliness? ]\n");
		tsv_header();
		bctx->mutate = 0;
		bctx->readers = 0;
		bctx->mut_ops_per_tx = 4;
		bctx->mut_tx_per_loop = 4;
		bctx->read_ops_per_tx = 32;
		bctx->read_tx_per_loop = 32;
		bctx->zipf_skew = 0.01;
		/* dumb hack */
		bctx->load_time = bctx->zipf_skew * NS_PER_SEC;
		break;
	case vary_zipf_skew:
		bctx->zipf_skew += 0.1;
		bctx->load_time = bctx->zipf_skew * NS_PER_SEC;
		if (bctx->zipf_skew >= 1.0) {
			bctx->zipf_skew = 0.0;
			bctx->load_time = 0;
			goto init_mut_read;
		}
		break;

	init_mut_read:
		bctx->mode = vary_mut_read;
		printf("\n");
		printf("vary mutate / read threads "
		       "[ read perf per thread should be flat ]\n");
		tsv_header();
		bctx->mutate = bctx->nloops - 1;
		bctx->readers = 1;
		bctx->mut_ops_per_tx = 4;
		bctx->mut_tx_per_loop = 4;
		bctx->read_ops_per_tx = 32;
		bctx->read_tx_per_loop = 32;
		break;
	case vary_mut_read:
		if (bctx->mutate == 1) {
			goto init_read_only;
		} else {
			bctx->mutate--;
			bctx->readers++;
		}
		break;

	init_read_only:
		bctx->mode = vary_read_only;
		printf("\n");
		printf("vary read threads "
		       "[ read perf per thread should be flat ]\n");
		tsv_header();
		bctx->mutate = 0;
		bctx->readers = 1;
		bctx->mut_ops_per_tx = 4;
		bctx->mut_tx_per_loop = 4;
		bctx->read_ops_per_tx = 32;
		bctx->read_tx_per_loop = 32;
		break;
	case vary_read_only:
		if (bctx->readers == bctx->nloops) {
			goto init_mut_ops_per_tx;
		} else {
			bctx->readers++;
		}
		break;

	init_mut_ops_per_tx:
		bctx->mode = vary_mut_ops_per_tx;
		printf("\n");
		printf("vary mutate operations per transaction "
		       "[ mutate activity affects read perf? ]\n");
		tsv_header();
		bctx->mutate = 1;
		bctx->readers = bctx->nloops - 1;
		bctx->mut_ops_per_tx = 1;
		bctx->mut_tx_per_loop = 1;
		bctx->read_ops_per_tx = 32;
		bctx->read_tx_per_loop = 32;
		break;
	case vary_mut_ops_per_tx:
		if (bctx->mut_ops_per_tx * bctx->mut_tx_per_loop ==
		    MAX_OPS_PER_LOOP)
		{
			goto init_mut_tx_per_loop;
		} else {
			bctx->mut_ops_per_tx *= 2;
		}
		break;

	init_mut_tx_per_loop:
		bctx->mode = vary_mut_tx_per_loop;
		printf("\n");
		printf("vary mutate transactions per loop "
		       "[ mutate activity affects read perf? ]\n");
		tsv_header();
		bctx->mutate = 1;
		bctx->readers = bctx->nloops - 1;
		bctx->mut_ops_per_tx = 4;
		bctx->mut_tx_per_loop = 1;
		bctx->read_ops_per_tx = 32;
		bctx->read_tx_per_loop = 32;
		break;
	case vary_mut_tx_per_loop:
		if (bctx->mut_ops_per_tx * bctx->mut_tx_per_loop ==
		    MAX_OPS_PER_LOOP)
		{
			goto init_read_tx_per_loop_rw;
		} else {
			bctx->mut_tx_per_loop *= 2;
		}
		break;

	init_read_tx_per_loop_rw:
		bctx->mode = vary_read_tx_per_loop_rw;
		printf("\n");
		printf("vary read transactions per loop "
		       "[ loop overhead? ]\n");
		tsv_header();
		bctx->mutate = 1;
		bctx->readers = bctx->nloops - 1;
		bctx->mut_ops_per_tx = 4;
		bctx->mut_tx_per_loop = 4;
		bctx->read_ops_per_tx = 4;
		bctx->read_tx_per_loop = 1;
		break;
	case vary_read_tx_per_loop_rw:
		if (bctx->read_ops_per_tx * bctx->read_tx_per_loop ==
		    MAX_OPS_PER_LOOP)
		{
			goto init_read_tx_per_loop_ro;
		} else {
			bctx->read_tx_per_loop *= 2;
		}
		break;

	init_read_tx_per_loop_ro:
		bctx->mode = vary_read_tx_per_loop_ro;
		printf("\n");
		printf("vary read transactions per loop (readonly) "
		       "[ loop overhead? ]\n");
		tsv_header();
		bctx->mutate = 0;
		bctx->readers = bctx->nloops;
		bctx->mut_ops_per_tx = 4;
		bctx->mut_tx_per_loop = 4;
		bctx->read_ops_per_tx = 4;
		bctx->read_tx_per_loop = 1;
		break;
	case vary_read_tx_per_loop_ro:
		if (bctx->read_ops_per_tx * bctx->read_tx_per_loop ==
		    MAX_OPS_PER_LOOP)
		{
			goto init_read_ops_per_tx_rw;
		} else {
			bctx->read_tx_per_loop *= 2;
		}
		break;

	init_read_ops_per_tx_rw:
		bctx->mode = vary_read_ops_per_tx_rw;
		printf("\n");
		printf("vary read operations per transaction "
		       " [ transaction overhead should be small ]\n");
		tsv_header();
		bctx->mutate = 1;
		bctx->readers = bctx->nloops - 1;
		bctx->mut_ops_per_tx = 4;
		bctx->mut_tx_per_loop = 4;
		bctx->read_ops_per_tx = 1;
		bctx->read_tx_per_loop = MAX_OPS_PER_LOOP;
		break;
	case vary_read_ops_per_tx_rw:
		if (bctx->read_ops_per_tx == MAX_OPS_PER_LOOP) {
			goto init_read_ops_per_tx_ro;
		} else {
			bctx->read_ops_per_tx *= 2;
			bctx->read_tx_per_loop /= 2;
		}
		break;

	init_read_ops_per_tx_ro:
		bctx->mode = vary_read_ops_per_tx_ro;
		printf("\n");
		printf("vary read operations per transaction (readonly) "
		       " [ transaction overhead should be small ]\n");
		tsv_header();
		bctx->mutate = 0;
		bctx->readers = bctx->nloops;
		bctx->mut_ops_per_tx = 0;
		bctx->mut_tx_per_loop = 0;
		bctx->read_ops_per_tx = 1;
		bctx->read_tx_per_loop = MAX_OPS_PER_LOOP;
		break;
	case vary_read_ops_per_tx_ro:
		if (bctx->read_ops_per_tx == MAX_OPS_PER_LOOP) {
			goto fini;
		} else {
			bctx->read_ops_per_tx *= 2;
			bctx->read_tx_per_loop /= 2;
		}
		break;
	}

	/* dispatch a benchmark run */

	bool zipf = bctx->mutate == 0 && bctx->readers == 0;
	bctx->waiting = zipf ? bctx->nloops : bctx->readers + bctx->mutate;
	isc_barrier_init(&bctx->barrier, bctx->waiting);
	for (uint32_t t = 0; t < bctx->waiting; t++) {
		bool mut = t < bctx->mutate;
		bctx->thread[t] = (struct thread_args){
			.bctx = bctx,
			.barrier = &bctx->barrier,
			.loopmgr = bctx->loopmgr,
			.multi = bctx->multi,
			.max_item = bctx->max_item,
			.zipf_skew = bctx->zipf_skew,
			.cb = zipf  ? read_zipf
			      : mut ? mutate_transactions
				    : read_transactions,
			.job = ISC_JOB_INITIALIZER,
			.ops_per_tx = mut ? bctx->mut_ops_per_tx
					  : bctx->read_ops_per_tx,
			.tx_per_loop = mut ? bctx->mut_tx_per_loop
					   : bctx->read_tx_per_loop,
		};
		isc_async_run(isc_loop_get(bctx->loopmgr, t), first_loop,
			      &bctx->thread[t]);
	}
}

static void
collect(void *varg) {
	struct thread_args *args = varg;
	struct bench_state *bctx = args->bctx;
	struct thread_args *thread = bctx->thread;
	struct {
		uint64_t worked, txns, ops, compactions;
	} stats[2] = {};
	double load_time = bctx->load_time;
	double elapsed = 0, mut_work, readers, read_work, elapsed_ms;
	uint32_t nloops;
	bool zipf;

	TRACE("collect");

	bctx->waiting--;
	if (bctx->waiting > 0) {
		return;
	}
	isc_barrier_destroy(&bctx->barrier);

	load_time = load_time > 0 ? load_time / (double)NS_PER_SEC : NAN;

	zipf = bctx->mutate == 0 && bctx->readers == 0;
	nloops = zipf ? bctx->nloops : bctx->readers + bctx->mutate;
	for (uint32_t t = 0; t < nloops; t++) {
		struct thread_args *tp = &thread[t];
		elapsed = ISC_MAX(elapsed, (tp->stop - tp->start));
		bool mut = t < bctx->mutate;

		stats[mut].worked += tp->worked;
		stats[mut].txns += tp->transactions;
		stats[mut].ops += tp->transactions * tp->ops_per_tx;
		stats[mut].compactions += tp->compactions;
	}

	printf("%7.3f\t", RUNTIME / (double)NS_PER_SEC);
	printf("%7.3f\t", elapsed / (double)NS_PER_SEC);
	printf("%7.3f\t", load_time);
	printf("%7.2f\t", (double)bctx->qp_bytes / bctx->qp_items);
	printf("%7u\t", bctx->max_item);

	mut_work = stats[1].worked / (double)US_PER_MS;
	printf("%7u\t", bctx->mutate);
	printf("%7u\t", bctx->mut_tx_per_loop);
	printf("%7u\t", bctx->mut_ops_per_tx);
	printf("%7llu\t", (unsigned long long)stats[1].compactions);
	printf("%7llu\t", (unsigned long long)stats[1].txns);
	printf("%7llu\t", (unsigned long long)stats[1].ops);
	printf("%7.2f\t", stats[1].worked / (double)NS_PER_SEC);
	printf("%7.2f\t", stats[1].txns / mut_work);
	printf("%7.2f\t", stats[1].ops / mut_work);

	readers = zipf ? bctx->nloops - bctx->mutate : bctx->readers;
	read_work = stats[0].worked / (double)US_PER_MS;
	elapsed_ms = elapsed / (double)US_PER_MS;
	printf("%7u\t", bctx->readers);
	printf("%7u\t", bctx->read_tx_per_loop);
	printf("%7u\t", bctx->read_ops_per_tx);
	printf("%7llu\t", (unsigned long long)stats[0].txns / 1000);
	printf("%7llu\t", (unsigned long long)stats[0].ops / 1000);
	printf("%7.2f\t", stats[0].worked / (double)NS_PER_SEC);
	printf("%7.2f\t", stats[0].txns / read_work);
	printf("%7.2f\t", stats[0].ops / read_work);
	printf("%7.2f\t", stats[0].ops * readers / read_work);
	printf("%7.2f\n", stats[0].ops / elapsed_ms);

	dispatch(bctx);
}

static void
startup(void *arg) {
	isc_loopmgr_t *loopmgr = arg;
	isc_loop_t *loop = isc_loop();
	isc_mem_t *mctx = isc_loop_getmctx(loop);
	uint32_t nloops = isc_loopmgr_nloops(loopmgr);
	size_t bytes = sizeof(struct bench_state) +
		       sizeof(struct thread_args) * nloops;
	struct bench_state *bctx = isc_mem_cget(mctx, 1, bytes);

	*bctx = (struct bench_state){
		.loopmgr = loopmgr,
		.bytes = bytes,
		.nloops = nloops,
	};
	isc_mem_attach(mctx, &bctx->mctx);

	dispatch(bctx);
}

struct ticker {
	isc_loopmgr_t *loopmgr;
	isc_mem_t *mctx;
	isc_timer_t *timer;
};

static void
tick(void *varg) {
	/* just make the loop cycle */
	UNUSED(varg);
}

static void
start_ticker(void *varg) {
	struct ticker *ticker = varg;
	isc_loop_t *loop = isc_loop();

	isc_timer_create(loop, tick, NULL, &ticker->timer);
	isc_timer_start(ticker->timer, isc_timertype_ticker,
			&(isc_interval_t){
				.seconds = 0,
				.nanoseconds = 1 * NS_PER_MS,
			});
}

static void
stop_ticker(void *varg) {
	struct ticker *ticker = varg;

	isc_timer_stop(ticker->timer);
	isc_timer_destroy(&ticker->timer);
	isc_mem_putanddetach(&ticker->mctx, ticker, sizeof(*ticker));
}

static void
setup_tickers(isc_mem_t *mctx, isc_loopmgr_t *loopmgr) {
	uint32_t nloops = isc_loopmgr_nloops(loopmgr);
	for (uint32_t i = 0; i < nloops; i++) {
		isc_loop_t *loop = isc_loop_get(loopmgr, i);
		struct ticker *ticker = isc_mem_get(mctx, sizeof(*ticker));
		*ticker = (struct ticker){
			.loopmgr = loopmgr,
		};
		isc_mem_attach(mctx, &ticker->mctx);
		isc_loop_setup(loop, start_ticker, ticker);
		isc_loop_teardown(loop, stop_ticker, ticker);
	}
}

int
main(void) {
	isc_loopmgr_t *loopmgr = NULL;
	isc_mem_t *mctx = NULL;

	setlinebuf(stdout);

	uint32_t nloops;
	const char *env_workers = getenv("ISC_TASK_WORKERS");

	if (env_workers != NULL) {
		nloops = atoi(env_workers);
	} else {
		nloops = isc_os_ncpus();
	}
	INSIST(nloops > 1);

	isc_mem_create(&mctx);
	isc_mem_setdestroycheck(mctx, true);
	init_logging(mctx);
	init_items(mctx);

	isc_loopmgr_create(mctx, nloops, &loopmgr);
	setup_tickers(mctx, loopmgr);
	isc_loop_setup(isc_loop_main(loopmgr), startup, loopmgr);
	isc_loopmgr_run(loopmgr);
	isc_loopmgr_destroy(&loopmgr);

	isc_log_destroy(&dns_lctx);
	isc_mem_free(mctx, item);
	isc_mem_checkdestroyed(stdout);
	isc_mem_destroy(&mctx);

	return 0;
}
