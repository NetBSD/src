/*	$NetBSD: qpmulti_test.c,v 1.1.1.1 2025/01/26 16:12:37 christos Exp $	*/

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

#include <assert.h>
#include <inttypes.h>
#include <sched.h> /* IWYU pragma: keep */
#include <setjmp.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdlib.h>

#define UNIT_TESTING
#include <cmocka.h>

#include <isc/assertions.h>
#include <isc/log.h>
#include <isc/loop.h>
#include <isc/magic.h>
#include <isc/mem.h>
#include <isc/random.h>
#include <isc/refcount.h>
#include <isc/rwlock.h>
#include <isc/urcu.h>
#include <isc/util.h>

#include <dns/log.h>
#include <dns/qp.h>
#include <dns/types.h>

#include "qp_p.h"

#include <tests/isc.h>
#include <tests/qp.h>

#define VERBOSE		  0
#define ITEM_COUNT	  12345
#define TRANSACTION_SIZE  123
#define TRANSACTION_COUNT 1234

#if VERBOSE
#define TRACE(fmt, ...)                                                     \
	isc_log_write(dns_lctx, DNS_LOGCATEGORY_DATABASE, DNS_LOGMODULE_QP, \
		      ISC_LOG_DEBUG(7), "%s:%d:%s(): " fmt, __FILE__,       \
		      __LINE__, __func__, ##__VA_ARGS__)
#else
#define TRACE(...)
#endif

#if VERBOSE
#define ASSERT(p)                       \
	if (!(p)) {                     \
		TRACE("%s failed", #p); \
		ok = false;             \
	} else
#else
#define ASSERT(p) assert_true(p)
#endif

static void
setup_logging(void) {
	isc_result_t result;
	isc_logdestination_t destination;
	isc_logconfig_t *logconfig = NULL;

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
	assert_int_equal(result, ISC_R_SUCCESS);
}

static struct {
	uint32_t refcount;
	bool in_ro;
	bool in_rw;
	uint8_t len;
	dns_qpkey_t key;
	dns_qpkey_t ascii;
} item[ITEM_COUNT];

static void
item_attach(void *ctx, void *pval, uint32_t ival) {
	INSIST(ctx == NULL);
	INSIST(pval == &item[ival]);
	item[ival].refcount++;
}

static void
item_detach(void *ctx, void *pval, uint32_t ival) {
	assert_null(ctx);
	assert_ptr_equal(pval, &item[ival]);
	assert_int_not_equal(item[ival].refcount, 0);
	item[ival].refcount--;
}

static size_t
item_makekey(dns_qpkey_t key, void *ctx, void *pval, uint32_t ival) {
	INSIST(ctx == NULL);
	uintptr_t ip = (uintptr_t)pval;
	uintptr_t lo = (uintptr_t)item;
	uintptr_t hi = sizeof(item) + lo;
	if (!(ival < ARRAY_SIZE(item) && lo <= ip && ip < hi &&
	      pval == &item[ival]))
	{
		ISC_INSIST(ival < ARRAY_SIZE(item));
		ISC_INSIST(pval != NULL);
		ISC_INSIST(ip >= lo);
		ISC_INSIST(ip < hi);
		ISC_INSIST(pval == &item[ival]);
	}
	memmove(key, item[ival].key, item[ival].len);
	return item[ival].len;
}

static void
testname(void *ctx, char *buf, size_t size) {
	REQUIRE(ctx == NULL);
	strlcpy(buf, "test", size);
}

const dns_qpmethods_t test_methods = {
	item_attach,
	item_detach,
	item_makekey,
	testname,
};

static uint8_t
random_byte(void) {
	return isc_random_uniform(SHIFT_OFFSET - SHIFT_NOBYTE) + SHIFT_NOBYTE;
}

static void
setup_items(void) {
	void *pval = NULL;
	dns_qp_t *qp = NULL;
	dns_qp_create(mctx, &test_methods, NULL, &qp);
	for (size_t i = 0; i < ARRAY_SIZE(item); i++) {
		do {
			size_t len = isc_random_uniform(16) + 4;
			item[i].len = len;
			for (size_t off = 0; off < len; off++) {
				item[i].key[off] = random_byte();
			}
			memmove(item[i].ascii, item[i].key, len);
			qp_test_keytoascii(item[i].ascii, len);
		} while (dns_qp_getkey(qp, item[i].key, item[i].len, &pval,
				       NULL) == ISC_R_SUCCESS);
		assert_int_equal(dns_qp_insert(qp, &item[i], i), ISC_R_SUCCESS);
	}
	dns_qp_destroy(&qp);
}

static bool
checkkey(dns_qpreadable_t qpr, size_t i, bool exists, const char *rubric) {
	bool ok = true;
	void *pval = NULL;
	uint32_t ival = ~0U;
	isc_result_t result;
	result = dns_qp_getkey(qpr, item[i].key, item[i].len, &pval, &ival);
	if (result == ISC_R_SUCCESS) {
		assert_true(exists);
		assert_ptr_equal(pval, &item[i]);
		assert_int_equal(ival, i);
	} else if (result == ISC_R_NOTFOUND) {
		assert_false(exists);
		assert_null(pval);
		assert_int_equal(ival, ~0U);
	} else {
		UNREACHABLE();
	}
	if (!ok) {
		TRACE("checkkey %p %zu %s %s %s", qpr.qpr, i,
		      exists ? "exists" : "missing", isc_result_totext(result),
		      rubric);
		UNUSED(rubric);
	}
	return ok;
}

static bool
checkallro(dns_qpreadable_t qp) {
	bool ok = true;
	for (size_t i = 0; i < ARRAY_SIZE(item); i++) {
		ASSERT(checkkey(qp, i, item[i].in_ro, "checkall ro"));
	}
	if (!ok) {
		qp_test_dumptrie(qp);
		TRACE("checkallro failed");
	}
	return ok;
}

static bool
checkallrw(dns_qpreadable_t qp) {
	bool ok = true;
	for (size_t i = 0; i < ARRAY_SIZE(item); i++) {
		ASSERT(checkkey(qp, i, item[i].in_rw, "checkall rw"));
	}
	if (!ok) {
		qp_test_dumptrie(qp);
		TRACE("checkallrw failed");
	}
	return ok;
}

static void
one_transaction(dns_qpmulti_t *qpm) {
	isc_result_t result;
	bool ok = true;

	dns_qpreader_t *qpo = NULL;
	dns_qpsnap_t *qps = NULL;
	dns_qpread_t qpr = { 0 };
	dns_qp_t *qpw = NULL;

	bool snap = isc_random_uniform(2) == 0;
	bool update = isc_random_uniform(2) != 0;
	bool rollback = update && isc_random_uniform(4) == 0;
	size_t count = isc_random_uniform(TRANSACTION_SIZE);

	TRACE("transaction %s %s %s size %zu", snap ? "snapshot" : "query",
	      update ? "update" : "write", rollback ? "rollback" : "commit",
	      count);

	/*
	 * We need to take care to avoid lock order inversion:
	 * The write mutex must be the outermost lock if it is held.
	 * The mutex must not be taken while the rwlock is held.
	 */

	/* briefly take and drop mutex */
	if (snap) {
		dns_qpmulti_snapshot(qpm, &qps);
		qpo = (dns_qpreader_t *)qps;
	}

	/* take mutex */
	if (update) {
		dns_qpmulti_update(qpm, &qpw);
	} else {
		dns_qpmulti_write(qpm, &qpw);
	}

	if (!snap) {
		dns_qpmulti_query(qpm, &qpr);
		qpo = (dns_qpreader_t *)&qpr;
	}

	for (size_t n = 0; n < count; n++) {
		size_t i = isc_random_uniform(ARRAY_SIZE(item));

		ASSERT(checkkey(qpo, i, item[i].in_ro, "before ro"));
		ASSERT(checkkey(qpw, i, item[i].in_rw, "before rw"));

		if (item[i].in_rw) {
			/* TRACE("delete %zu %.*s", i,
				 item[i].len, item[i].ascii); */
			void *pvald = NULL;
			uint32_t ivald = 0;
			result = dns_qp_deletekey(qpw, item[i].key, item[i].len,
						  &pvald, &ivald);
			ASSERT(result == ISC_R_SUCCESS);
			ASSERT(pvald == &item[i]);
			ASSERT(ivald == i);
			item[i].in_rw = false;
		} else {
			/* TRACE("insert %zu %.*s", i,
				 item[i].len, item[i].ascii); */
			result = dns_qp_insert(qpw, &item[i], i);
			ASSERT(result == ISC_R_SUCCESS);
			item[i].in_rw = true;
		}

		ASSERT(checkkey(qpo, i, item[i].in_ro, "after ro"));
		ASSERT(checkkey(qpw, i, item[i].in_rw, "after rw"));

		if (!ok) {
			TRACE("mutate %zu/%zu failed", n, count);
			qp_test_dumptrie(qpo);
			qp_test_dumptrie(qpw);
		}
		assert_true(ok);
	}

	assert_true(checkallro(qpo));
	assert_true(checkallrw(qpw));

	if (!snap) {
		dns_qpread_destroy(qpm, &qpr);
	}

	if (rollback) {
		TRACE("transaction rollback");
		dns_qpmulti_rollback(qpm, &qpw);
		/* mutex is now dropped */
		dns_qpmulti_query(qpm, &qpr);
		for (size_t i = 0; i < ARRAY_SIZE(item); i++) {
			if (snap) {
				ASSERT(checkkey(qps, i, item[i].in_ro,
						"rollback ro"));
			}
			item[i].in_rw = item[i].in_ro;
			ASSERT(checkkey(&qpr, i, item[i].in_rw, "rollback rw"));
		}
		dns_qpread_destroy(qpm, &qpr);
	} else {
		TRACE("transaction commit");
		dns_qpmulti_commit(qpm, &qpw);
		/* mutex is now dropped */
		dns_qpmulti_query(qpm, &qpr);
		for (size_t i = 0; i < ARRAY_SIZE(item); i++) {
			if (snap) {
				ASSERT(checkkey(qps, i, item[i].in_ro,
						"commit ro"));
			}
			item[i].in_ro = item[i].in_rw;
			ASSERT(checkkey(&qpr, i, item[i].in_rw, "commit rw"));
		}
		dns_qpread_destroy(qpm, &qpr);
	}

	if (snap) {
		TRACE("snapshot destroy");
		/* takes mutex briefly */
		dns_qpsnap_destroy(qpm, &qps);
	}

	TRACE("completed %s %s %s size %zu", snap ? "snapshot" : "query",
	      update ? "update" : "write", rollback ? "rollback" : "commit",
	      count);

	if (!ok) {
		TRACE("transaction failed");
		dns_qpmulti_query(qpm, &qpr);
		qp_test_dumptrie(&qpr);
		dns_qpread_destroy(qpm, &qpr);
	}
	assert_true(ok);
}

static void
many_transactions(void *arg) {
	UNUSED(arg);

	dns_qpmulti_t *qpm = NULL;
	dns_qpmulti_create(mctx, &test_methods, NULL, &qpm);
	qpm->writer.write_protect = true;

	for (size_t n = 0; n < TRANSACTION_COUNT; n++) {
		TRACE("transaction %zu", n);
		one_transaction(qpm);
		rcu_quiescent_state();
	}

	dns_qpmulti_destroy(&qpm);
	isc_loopmgr_shutdown(loopmgr);
}

ISC_RUN_TEST_IMPL(qpmulti) {
	setup_loopmgr(NULL);
	setup_logging();
	setup_items();
	isc_loop_setup(isc_loop_main(loopmgr), many_transactions, NULL);
	isc_loopmgr_run(loopmgr);
	rcu_barrier();
	isc_loopmgr_destroy(&loopmgr);
	isc_log_destroy(&dns_lctx);
}

ISC_TEST_LIST_START
ISC_TEST_ENTRY(qpmulti)
ISC_TEST_LIST_END

ISC_TEST_MAIN
