/*	$NetBSD: qp_test.c,v 1.1.1.1 2025/01/26 16:12:37 christos Exp $	*/

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
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define UNIT_TESTING
#include <cmocka.h>

#include <isc/random.h>
#include <isc/refcount.h>
#include <isc/result.h>
#include <isc/string.h>
#include <isc/urcu.h>
#include <isc/util.h>

#include <dns/name.h>
#include <dns/qp.h>

#include "qp_p.h"

#include <tests/dns.h>
#include <tests/qp.h>

bool verbose = false;

ISC_RUN_TEST_IMPL(qpkey_name) {
	struct {
		const char *namestr;
		uint8_t key[512];
		size_t len;
	} testcases[] = {
		{
			.namestr = "",
			.key = { 0x02 },
			.len = 0,
		},
		{
			.namestr = ".",
			.key = { 0x02, 0x02 },
			.len = 1,
		},
		{
			.namestr = "\\000",
			.key = { 0x03, 0x03, 0x02 },
			.len = 3,
		},
		{
			.namestr = "\\000\\009",
			.key = { 0x03, 0x03, 0x03, 0x0c, 0x02 },
			.len = 5,
		},
		{
			.namestr = "com",
			.key = { 0x16, 0x22, 0x20, 0x02 },
			.len = 4,
		},
		{
			.namestr = "com.",
			.key = { 0x02, 0x16, 0x22, 0x20, 0x02 },
			.len = 5,
		},
		{
			.namestr = "example.com.",
			.key = { 0x02, 0x16, 0x22, 0x20, 0x02, 0x18, 0x2b, 0x14,
				 0x20, 0x23, 0x1f, 0x18, 0x02 },
			.len = 13,
		},
		{
			.namestr = "example.com",
			.key = { 0x16, 0x22, 0x20, 0x02, 0x18, 0x2b, 0x14, 0x20,
				 0x23, 0x1f, 0x18, 0x02 },
			.len = 12,
		},
		{
			.namestr = "EXAMPLE.COM",
			.key = { 0x16, 0x22, 0x20, 0x02, 0x18, 0x2b, 0x14, 0x20,
				 0x23, 0x1f, 0x18, 0x02 },
			.len = 12,
		},
	};

	for (size_t i = 0; i < ARRAY_SIZE(testcases); i++) {
		size_t len;
		dns_qpkey_t key;
		dns_fixedname_t fn1, fn2;
		dns_name_t *in = NULL, *out = NULL;
		char namebuf[DNS_NAME_FORMATSIZE];

		in = dns_fixedname_initname(&fn1);
		if (testcases[i].len != 0) {
			dns_test_namefromstring(testcases[i].namestr, &fn1);
		}
		len = dns_qpkey_fromname(key, in);
		if (verbose) {
			qp_test_printkey(key, len);
		}

		assert_int_equal(testcases[i].len, len);
		assert_memory_equal(testcases[i].key, key, len);
		/* also check key correctness for empty name */
		if (len == 0) {
			assert_int_equal(testcases[i].key[0], ((char *)key)[0]);
		}

		out = dns_fixedname_initname(&fn2);
		dns_qpkey_toname(key, len, out);
		assert_true(dns_name_equal(in, out));
		/* check that 'out' is properly reset by dns_qpkey_toname */
		dns_qpkey_toname(key, len, out);
		dns_name_format(out, namebuf, sizeof(namebuf));
	}
}

ISC_RUN_TEST_IMPL(qpkey_sort) {
	struct {
		const char *namestr;
		dns_name_t *name;
		dns_fixedname_t fixed;
		size_t len;
		dns_qpkey_t key;
	} testcases[] = {
		{ .namestr = "." },
		{ .namestr = "\\000." },
		{ .namestr = "\\000.\\000." },
		{ .namestr = "\\000\\009." },
		{ .namestr = "\\007." },
		{ .namestr = "example.com." },
		{ .namestr = "EXAMPLE.COM." },
		{ .namestr = "www.example.com." },
		{ .namestr = "exam.com." },
		{ .namestr = "exams.com." },
		{ .namestr = "exam\\000.com." },
	};

	for (size_t i = 0; i < ARRAY_SIZE(testcases); i++) {
		dns_test_namefromstring(testcases[i].namestr,
					&testcases[i].fixed);
		testcases[i].name = dns_fixedname_name(&testcases[i].fixed);
		testcases[i].len = dns_qpkey_fromname(testcases[i].key,
						      testcases[i].name);
	}

	for (size_t i = 0; i < ARRAY_SIZE(testcases); i++) {
		for (size_t j = 0; j < ARRAY_SIZE(testcases); j++) {
			int namecmp = dns_name_compare(testcases[i].name,
						       testcases[j].name);
			size_t len = ISC_MIN(testcases[i].len,
					     testcases[j].len);
			/* include extra terminating NOBYTE */
			int keycmp = memcmp(testcases[i].key, testcases[j].key,
					    len + 1);
			assert_true((namecmp < 0) == (keycmp < 0));
			assert_true((namecmp == 0) == (keycmp == 0));
			assert_true((namecmp > 0) == (keycmp > 0));
		}
	}
}

#define ITER_ITEMS 100

static void
check_leaf(void *uctx, void *pval, uint32_t ival) {
	uint32_t *items = uctx;
	assert_in_range(ival, 1, ITER_ITEMS - 1);
	assert_ptr_equal(items + ival, pval);
}

static size_t
qpiter_makekey(dns_qpkey_t key, void *uctx, void *pval, uint32_t ival) {
	check_leaf(uctx, pval, ival);

	char str[8];
	snprintf(str, sizeof(str), "%03u", ival);

	size_t i = 0;
	while (str[i] != '\0') {
		key[i] = str[i] - '0' + SHIFT_BITMAP;
		i++;
	}
	key[i++] = SHIFT_NOBYTE;

	return i;
}

static void
getname(void *uctx, char *buf, size_t size) {
	strlcpy(buf, "test", size);
	UNUSED(uctx);
	UNUSED(size);
}

const dns_qpmethods_t qpiter_methods = {
	check_leaf,
	check_leaf,
	qpiter_makekey,
	getname,
};

ISC_RUN_TEST_IMPL(qpiter) {
	dns_qp_t *qp = NULL;
	uint32_t item[ITER_ITEMS] = { 0 };
	uint32_t order[ITER_ITEMS] = { 0 };
	dns_qpiter_t qpi;
	int inserted, n;
	uint32_t ival;
	void *pval = NULL;
	isc_result_t result;

	dns_qp_create(mctx, &qpiter_methods, item, &qp);
	for (size_t tests = 0; tests < 1234; tests++) {
		ival = isc_random_uniform(ITER_ITEMS - 1) + 1;
		pval = &item[ival];

		item[ival] = ival;

		inserted = n = 0;

		/* randomly insert or remove */
		dns_qpkey_t key;
		size_t len = qpiter_makekey(key, item, pval, ival);
		if (dns_qp_insert(qp, pval, ival) == ISC_R_EXISTS) {
			void *pvald = NULL;
			uint32_t ivald = 0;
			dns_qp_deletekey(qp, key, len, &pvald, &ivald);
			assert_ptr_equal(pval, pvald);
			assert_int_equal(ival, ivald);
			item[ival] = 0;
		}

		/* check that we see only valid items in the correct order */
		uint32_t prev = 0;
		dns_qpiter_init(qp, &qpi);
		while (dns_qpiter_next(&qpi, NULL, &pval, &ival) ==
		       ISC_R_SUCCESS)
		{
			assert_in_range(ival, prev + 1, ITER_ITEMS - 1);
			assert_int_equal(ival, item[ival]);
			assert_ptr_equal(pval, &item[ival]);
			order[inserted++] = ival;
			item[ival] = ~ival;
			prev = ival;
		}

		/* ensure we saw every item */
		for (ival = 0; ival < ITER_ITEMS; ival++) {
			if (item[ival] != 0) {
				assert_int_equal(item[ival], ~ival);
				item[ival] = ival;
			}
		}

		/* now iterate backward and check correctness */
		n = inserted;
		while (dns_qpiter_prev(&qpi, NULL, NULL, &ival) ==
		       ISC_R_SUCCESS)
		{
			--n;

			assert_int_equal(ival, order[n]);

			/* and check current iterator value as well */
			result = dns_qpiter_current(&qpi, NULL, NULL, &ival);
			assert_int_equal(result, ISC_R_SUCCESS);
			assert_int_equal(ival, order[n]);
		}

		assert_int_equal(n, 0);

		/* ...and forward again */
		while (dns_qpiter_next(&qpi, NULL, NULL, &ival) ==
		       ISC_R_SUCCESS)
		{
			assert_int_equal(ival, order[n]);

			/* and check current iterator value as well */
			result = dns_qpiter_current(&qpi, NULL, NULL, &ival);
			assert_int_equal(result, ISC_R_SUCCESS);
			assert_int_equal(ival, order[n]);

			n++;
		}

		assert_int_equal(n, inserted);

		/*
		 * if there are enough items inserted, try going
		 * forward a few steps, then back to the start,
		 * to confirm we can change directions while iterating.
		 */
		if (inserted > 3) {
			assert_int_equal(
				dns_qpiter_next(&qpi, NULL, NULL, &ival),
				ISC_R_SUCCESS);
			assert_int_equal(ival, order[0]);

			assert_int_equal(
				dns_qpiter_next(&qpi, NULL, NULL, &ival),
				ISC_R_SUCCESS);
			assert_int_equal(ival, order[1]);

			assert_int_equal(
				dns_qpiter_prev(&qpi, NULL, NULL, &ival),
				ISC_R_SUCCESS);
			assert_int_equal(ival, order[0]);

			assert_int_equal(
				dns_qpiter_next(&qpi, NULL, NULL, &ival),
				ISC_R_SUCCESS);
			assert_int_equal(ival, order[1]);

			assert_int_equal(
				dns_qpiter_prev(&qpi, NULL, NULL, &ival),
				ISC_R_SUCCESS);
			assert_int_equal(ival, order[0]);

			assert_int_equal(
				dns_qpiter_prev(&qpi, NULL, NULL, &ival),
				ISC_R_NOMORE);
		}
	}

	dns_qp_destroy(&qp);
}

static void
no_op(void *uctx, void *pval, uint32_t ival) {
	UNUSED(uctx);
	UNUSED(pval);
	UNUSED(ival);
}

static size_t
qpkey_fromstring(dns_qpkey_t key, void *uctx, void *pval, uint32_t ival) {
	dns_fixedname_t fixed;

	UNUSED(uctx);
	UNUSED(ival);
	if (*(char *)pval == '\0') {
		return 0;
	}
	dns_test_namefromstring(pval, &fixed);
	return dns_qpkey_fromname(key, dns_fixedname_name(&fixed));
}

const dns_qpmethods_t string_methods = {
	no_op,
	no_op,
	qpkey_fromstring,
	getname,
};

struct check_partialmatch {
	const char *query;
	isc_result_t result;
	const char *found;
};

static void
check_partialmatch(dns_qp_t *qp, struct check_partialmatch check[]) {
	for (int i = 0; check[i].query != NULL; i++) {
		isc_result_t result;
		dns_fixedname_t fn1, fn2;
		dns_name_t *name = dns_fixedname_initname(&fn1);
		dns_name_t *foundname = dns_fixedname_initname(&fn2);
		void *pval = NULL;

		dns_test_namefromstring(check[i].query, &fn1);
		result = dns_qp_lookup(qp, name, foundname, NULL, NULL, &pval,
				       NULL);

#if 0
		fprintf(stderr, "%s %s (expected %s) "
			"value \"%s\" (expected \"%s\")\n",
			check[i].query,
			isc_result_totext(result),
			isc_result_totext(check[i].result), (char *)pval,
			check[i].found);
#endif

		assert_int_equal(result, check[i].result);
		if (result == ISC_R_SUCCESS) {
			assert_true(dns_name_equal(name, foundname));
		} else if (result == DNS_R_PARTIALMATCH) {
			/*
			 * there are cases where we may have passed a
			 * query name that was relative to the zone apex,
			 * and gotten back an absolute name from the
			 * partial match. it's also possible for an
			 * absolute query to get a partial match on a
			 * node that had an empty name. in these cases,
			 * sanity checking the relations between name
			 * and foundname can trigger an assertion, so
			 * let's just skip them.
			 */
			if (dns_name_isabsolute(name) ==
			    dns_name_isabsolute(foundname))
			{
				assert_false(dns_name_equal(name, foundname));
				assert_true(
					dns_name_issubdomain(name, foundname));
			}
		}
		if (check[i].found == NULL) {
			assert_null(pval);
		} else {
			assert_string_equal(pval, check[i].found);
		}
	}
}

static void
insert_str(dns_qp_t *qp, const char *str) {
	isc_result_t result;
	uintptr_t pval = (uintptr_t)str;
	INSIST((pval & 3) == 0);
	result = dns_qp_insert(qp, (void *)pval, 0);
	assert_int_equal(result, ISC_R_SUCCESS);
}

ISC_RUN_TEST_IMPL(partialmatch) {
	isc_result_t result;
	dns_qp_t *qp = NULL;
	int i = 0;

	dns_qp_create(mctx, &string_methods, NULL, &qp);

	/*
	 * Fixed size strings [16] should ensure leaf-compatible alignment.
	 */
	const char insert[][16] = {
		"a.b.",	     "b.",	     "fo.bar.", "foo.bar.",
		"fooo.bar.", "web.foo.bar.", ".",	"",
	};

	/*
	 * omit the root node for now, otherwise we'll get "partial match"
	 * results when we want "not found".
	 */
	while (insert[i][0] != '.') {
		insert_str(qp, insert[i++]);
	}

	static struct check_partialmatch check1[] = {
		{ "a.b.", ISC_R_SUCCESS, "a.b." },
		{ "b.c.", ISC_R_NOTFOUND, NULL },
		{ "bar.", ISC_R_NOTFOUND, NULL },
		{ "f.bar.", ISC_R_NOTFOUND, NULL },
		{ "foo.bar.", ISC_R_SUCCESS, "foo.bar." },
		{ "foooo.bar.", ISC_R_NOTFOUND, NULL },
		{ "w.foo.bar.", DNS_R_PARTIALMATCH, "foo.bar." },
		{ "www.foo.bar.", DNS_R_PARTIALMATCH, "foo.bar." },
		{ "web.foo.bar.", ISC_R_SUCCESS, "web.foo.bar." },
		{ "webby.foo.bar.", DNS_R_PARTIALMATCH, "foo.bar." },
		{ "my.web.foo.bar.", DNS_R_PARTIALMATCH, "web.foo.bar." },
		{ "my.other.foo.bar.", DNS_R_PARTIALMATCH, "foo.bar." },
		{ NULL, 0, NULL },
	};
	check_partialmatch(qp, check1);

	/* what if the trie contains the root? */
	INSIST(insert[i][0] == '.');
	insert_str(qp, insert[i++]);

	static struct check_partialmatch check2[] = {
		{ "b.c.", DNS_R_PARTIALMATCH, "." },
		{ "bar.", DNS_R_PARTIALMATCH, "." },
		{ "foo.bar.", ISC_R_SUCCESS, "foo.bar." },
		{ "bar", ISC_R_NOTFOUND, NULL },
		{ NULL, 0, NULL },
	};
	check_partialmatch(qp, check2);

	/*
	 * what if entries in the trie are relative to the zone apex
	 * and there's no root node?
	 */
	dns_qpkey_t rootkey = { SHIFT_NOBYTE };
	result = dns_qp_deletekey(qp, rootkey, 1, NULL, NULL);
	assert_int_equal(result, ISC_R_SUCCESS);
	check_partialmatch(qp, (struct check_partialmatch[]){
				       { "bar", ISC_R_NOTFOUND, NULL },
				       { "bar.", ISC_R_NOTFOUND, NULL },
				       { NULL, 0, NULL },
			       });

	/* what if there's a root node with an empty key? */
	INSIST(insert[i][0] == '\0');
	insert_str(qp, insert[i++]);
	check_partialmatch(qp, (struct check_partialmatch[]){
				       { "bar", DNS_R_PARTIALMATCH, "" },
				       { "bar.", DNS_R_PARTIALMATCH, "" },
				       { NULL, 0, NULL },
			       });

	dns_qp_destroy(&qp);
}

struct check_qpchain {
	const char *query;
	isc_result_t result;
	unsigned int length;
	const char *names[10];
};

static void
check_qpchainiter(dns_qp_t *qp, struct check_qpchain check[],
		  dns_qpiter_t *iter) {
	for (int i = 0; check[i].query != NULL; i++) {
		isc_result_t result;
		dns_fixedname_t fn1;
		dns_name_t *name = dns_fixedname_initname(&fn1);
		dns_qpchain_t chain;

		dns_qpchain_init(qp, &chain);
		dns_test_namefromstring(check[i].query, &fn1);
		result = dns_qp_lookup(qp, name, NULL, iter, &chain, NULL,
				       NULL);
#if 0
		fprintf(stderr, "%s %s (expected %s), "
			"len %d (expected %d)\n", check[i].query,
			isc_result_totext(result),
			isc_result_totext(check[i].result),
			dns_qpchain_length(&chain), check[i].length);
#endif

		assert_int_equal(result, check[i].result);
		assert_int_equal(dns_qpchain_length(&chain), check[i].length);
		for (unsigned int j = 0; j < check[i].length; j++) {
			dns_fixedname_t fn2, fn3;
			dns_name_t *expected = dns_fixedname_initname(&fn2);
			dns_name_t *found = dns_fixedname_initname(&fn3);
			dns_test_namefromstring(check[i].names[j], &fn2);
			dns_qpchain_node(&chain, j, found, NULL, NULL);
#if 0
			char nb[DNS_NAME_FORMATSIZE];
			dns_name_format(found, nb, sizeof(nb));
			fprintf(stderr, "got %s, expected %s\n", nb,
				check[i].names[j]);
#endif
			assert_true(dns_name_equal(found, expected));
		}
	}
}

static void
check_qpchain(dns_qp_t *qp, struct check_qpchain check[]) {
	dns_qpiter_t iter;
	dns_qpiter_init(qp, &iter);
	check_qpchainiter(qp, check, NULL);
	check_qpchainiter(qp, check, &iter);
}

ISC_RUN_TEST_IMPL(qpchain) {
	dns_qp_t *qp = NULL;
	const char insert[][16] = { ".",      "a.",	    "b.",
				    "c.b.a.", "e.d.c.b.a.", "c.b.b.",
				    "c.d.",   "a.b.c.d.",   "a.b.c.d.e.",
				    "b.a.",   "x.k.c.d.",   "" };
	int i = 0;

	dns_qp_create(mctx, &string_methods, NULL, &qp);

	while (insert[i][0] != '\0') {
		insert_str(qp, insert[i++]);
	}

	static struct check_qpchain check1[] = {
		{ "b.", ISC_R_SUCCESS, 2, { ".", "b." } },
		{ "b.a.", ISC_R_SUCCESS, 3, { ".", "a.", "b.a." } },
		{ "c.", DNS_R_PARTIALMATCH, 1, { "." } },
		{ "e.d.c.b.a.",
		  ISC_R_SUCCESS,
		  5,
		  { ".", "a.", "b.a.", "c.b.a.", "e.d.c.b.a." } },
		{ "a.b.c.d.", ISC_R_SUCCESS, 3, { ".", "c.d.", "a.b.c.d." } },
		{ "b.c.d.", DNS_R_PARTIALMATCH, 2, { ".", "c.d." } },
		{ "z.x.k.c.d.",
		  DNS_R_PARTIALMATCH,
		  3,
		  { ".", "c.d.", "x.k.c.d." } },
		{ NULL, 0, 0, { NULL } },
	};

	check_qpchain(qp, check1);
	dns_qp_destroy(&qp);

	const char insert2[][16] = { "a.", "d.b.a.", "z.d.b.a.", "" };

	i = 0;

	dns_qp_create(mctx, &string_methods, NULL, &qp);

	while (insert2[i][0] != '\0') {
		insert_str(qp, insert2[i++]);
	}

	static struct check_qpchain check2[] = {
		{ "f.c.b.a.", DNS_R_PARTIALMATCH, 1, { "a." } },
		{ NULL, 0, 0, { NULL } },
	};

	check_qpchain(qp, check2);
	dns_qp_destroy(&qp);
}

struct check_predecessors {
	const char *query;
	const char *predecessor;
	isc_result_t result;
	int remaining;
};

static void
check_predecessors_withchain(dns_qp_t *qp, struct check_predecessors check[],
			     dns_qpchain_t *chain) {
	isc_result_t result;
	dns_fixedname_t fn1, fn2;
	dns_name_t *name = dns_fixedname_initname(&fn1);
	dns_name_t *pred = dns_fixedname_initname(&fn2);
	char *namestr = NULL;

	for (int i = 0; check[i].query != NULL; i++) {
		dns_qpiter_t it;

		dns_test_namefromstring(check[i].query, &fn1);

		/*
		 * normalize the expected predecessor name, in
		 * case it has escaped characters, so we can compare
		 * apples to apples.
		 */
		dns_fixedname_t fn3;
		dns_name_t *expred = dns_fixedname_initname(&fn3);
		char *predstr = NULL;
		dns_test_namefromstring(check[i].predecessor, &fn3);
		result = dns_name_tostring(expred, &predstr, mctx);
		assert_int_equal(result, ISC_R_SUCCESS);

		result = dns_qp_lookup(qp, name, NULL, &it, chain, NULL, NULL);
#if 0
		fprintf(stderr, "%s: expected %s got %s\n", check[i].query,
			isc_result_totext(check[i].result),
			isc_result_totext(result));
#endif
		assert_int_equal(result, check[i].result);

		if (result == ISC_R_SUCCESS) {
			/*
			 * we found an exact match; iterate to find
			 * the predecessor.
			 */
			result = dns_qpiter_prev(&it, pred, NULL, NULL);
			if (result == ISC_R_NOMORE) {
				result = dns_qpiter_prev(&it, pred, NULL, NULL);
			}
		} else {
			/*
			 * we didn't find a match, so the iterator should
			 * already be pointed at the predecessor node.
			 */
			result = dns_qpiter_current(&it, pred, NULL, NULL);
		}
		assert_int_equal(result, ISC_R_SUCCESS);

		result = dns_name_tostring(pred, &namestr, mctx);
#if 0
		fprintf(stderr, "... expected predecessor %s got %s\n",
			predstr, namestr);
#endif
		assert_int_equal(result, ISC_R_SUCCESS);
		assert_string_equal(namestr, predstr);

#if 0
		fprintf(stderr, "%d: remaining names after %s:\n", i, namestr);
#endif
		isc_mem_free(mctx, namestr);
		isc_mem_free(mctx, predstr);

		int j = 0;
		while (dns_qpiter_next(&it, name, NULL, NULL) == ISC_R_SUCCESS)
		{
#if 0
			result = dns_name_tostring(name, &namestr, mctx);
			assert_int_equal(result, ISC_R_SUCCESS);
			fprintf(stderr, "%s%s", j > 0 ? "->" : "", namestr);
			isc_mem_free(mctx, namestr);
#endif
			j++;
		}
#if 0
		fprintf(stderr, "\n...expected %d got %d\n",
			check[i].remaining, j);
#endif
		assert_int_equal(j, check[i].remaining);
	}
}

static void
check_predecessors(dns_qp_t *qp, struct check_predecessors check[]) {
	dns_qpchain_t chain;
	dns_qpchain_init(qp, &chain);
	check_predecessors_withchain(qp, check, NULL);
	check_predecessors_withchain(qp, check, &chain);
}

ISC_RUN_TEST_IMPL(predecessors) {
	dns_qp_t *qp = NULL;
	const char insert[][16] = {
		"a.",	  "b.",	      "c.b.a.",	  "e.d.c.b.a.",
		"c.b.b.", "c.d.",     "a.b.c.d.", "a.b.c.d.e.",
		"b.a.",	  "x.k.c.d.", "moog.",	  "mooker.",
		"mooko.", "moon.",    "moops.",	  ""
	};
	int i = 0;

	dns_qp_create(mctx, &string_methods, NULL, &qp);
	while (insert[i][0] != '\0') {
		insert_str(qp, insert[i++]);
	}

	/* first check: no root label in the database */
	static struct check_predecessors check1[] = {
		{ ".", "moops.", ISC_R_NOTFOUND, 0 },
		{ "a.", "moops.", ISC_R_SUCCESS, 0 },
		{ "b.a.", "a.", ISC_R_SUCCESS, 14 },
		{ "b.", "e.d.c.b.a.", ISC_R_SUCCESS, 11 },
		{ "aaa.a.", "a.", DNS_R_PARTIALMATCH, 14 },
		{ "ddd.a.", "e.d.c.b.a.", DNS_R_PARTIALMATCH, 11 },
		{ "d.c.", "c.b.b.", ISC_R_NOTFOUND, 9 },
		{ "1.2.c.b.a.", "c.b.a.", DNS_R_PARTIALMATCH, 12 },
		{ "a.b.c.e.f.", "a.b.c.d.e.", ISC_R_NOTFOUND, 5 },
		{ "z.y.x.", "moops.", ISC_R_NOTFOUND, 0 },
		{ "w.c.d.", "x.k.c.d.", DNS_R_PARTIALMATCH, 6 },
		{ "z.z.z.z.k.c.d.", "x.k.c.d.", DNS_R_PARTIALMATCH, 6 },
		{ "w.k.c.d.", "a.b.c.d.", DNS_R_PARTIALMATCH, 7 },
		{ "d.a.", "e.d.c.b.a.", DNS_R_PARTIALMATCH, 11 },
		{ "0.b.c.d.e.", "x.k.c.d.", ISC_R_NOTFOUND, 6 },
		{ "b.d.", "c.b.b.", ISC_R_NOTFOUND, 9 },
		{ "mon.", "a.b.c.d.e.", ISC_R_NOTFOUND, 5 },
		{ "moor.", "moops.", ISC_R_NOTFOUND, 0 },
		{ "mopbop.", "moops.", ISC_R_NOTFOUND, 0 },
		{ "moppop.", "moops.", ISC_R_NOTFOUND, 0 },
		{ "mopps.", "moops.", ISC_R_NOTFOUND, 0 },
		{ "mopzop.", "moops.", ISC_R_NOTFOUND, 0 },
		{ "mop.", "moops.", ISC_R_NOTFOUND, 0 },
		{ "monbop.", "a.b.c.d.e.", ISC_R_NOTFOUND, 5 },
		{ "monpop.", "a.b.c.d.e.", ISC_R_NOTFOUND, 5 },
		{ "monps.", "a.b.c.d.e.", ISC_R_NOTFOUND, 5 },
		{ "monzop.", "a.b.c.d.e.", ISC_R_NOTFOUND, 5 },
		{ "mon.", "a.b.c.d.e.", ISC_R_NOTFOUND, 5 },
		{ "moop.", "moon.", ISC_R_NOTFOUND, 1 },
		{ "moopser.", "moops.", ISC_R_NOTFOUND, 0 },
		{ "monky.", "a.b.c.d.e.", ISC_R_NOTFOUND, 5 },
		{ "monkey.", "a.b.c.d.e.", ISC_R_NOTFOUND, 5 },
		{ "monker.", "a.b.c.d.e.", ISC_R_NOTFOUND, 5 },
		{ NULL, NULL, 0, 0 }
	};

	check_predecessors(qp, check1);

	/* second check: add a root label and try again */
	const char root[16] = ".";
	insert_str(qp, root);

	static struct check_predecessors check2[] = {
		{ ".", "moops.", ISC_R_SUCCESS, 0 },
		{ "a.", ".", ISC_R_SUCCESS, 15 },
		{ "b.a.", "a.", ISC_R_SUCCESS, 14 },
		{ "b.", "e.d.c.b.a.", ISC_R_SUCCESS, 11 },
		{ "aaa.a.", "a.", DNS_R_PARTIALMATCH, 14 },
		{ "ddd.a.", "e.d.c.b.a.", DNS_R_PARTIALMATCH, 11 },
		{ "d.c.", "c.b.b.", DNS_R_PARTIALMATCH, 9 },
		{ "1.2.c.b.a.", "c.b.a.", DNS_R_PARTIALMATCH, 12 },
		{ "a.b.c.e.f.", "a.b.c.d.e.", DNS_R_PARTIALMATCH, 5 },
		{ "z.y.x.", "moops.", DNS_R_PARTIALMATCH, 0 },
		{ "w.c.d.", "x.k.c.d.", DNS_R_PARTIALMATCH, 6 },
		{ "z.z.z.z.k.c.d.", "x.k.c.d.", DNS_R_PARTIALMATCH, 6 },
		{ "w.k.c.d.", "a.b.c.d.", DNS_R_PARTIALMATCH, 7 },
		{ "d.a.", "e.d.c.b.a.", DNS_R_PARTIALMATCH, 11 },
		{ "0.b.c.d.e.", "x.k.c.d.", DNS_R_PARTIALMATCH, 6 },
		{ "mon.", "a.b.c.d.e.", DNS_R_PARTIALMATCH, 5 },
		{ "moor.", "moops.", DNS_R_PARTIALMATCH, 0 },
		{ "mopbop.", "moops.", DNS_R_PARTIALMATCH, 0 },
		{ "moppop.", "moops.", DNS_R_PARTIALMATCH, 0 },
		{ "mopps.", "moops.", DNS_R_PARTIALMATCH, 0 },
		{ "mopzop.", "moops.", DNS_R_PARTIALMATCH, 0 },
		{ "mop.", "moops.", DNS_R_PARTIALMATCH, 0 },
		{ "monbop.", "a.b.c.d.e.", DNS_R_PARTIALMATCH, 5 },
		{ "monpop.", "a.b.c.d.e.", DNS_R_PARTIALMATCH, 5 },
		{ "monps.", "a.b.c.d.e.", DNS_R_PARTIALMATCH, 5 },
		{ "monzop.", "a.b.c.d.e.", DNS_R_PARTIALMATCH, 5 },
		{ "mon.", "a.b.c.d.e.", DNS_R_PARTIALMATCH, 5 },
		{ "moop.", "moon.", DNS_R_PARTIALMATCH, 1 },
		{ "moopser.", "moops.", DNS_R_PARTIALMATCH, 0 },
		{ "monky.", "a.b.c.d.e.", DNS_R_PARTIALMATCH, 5 },
		{ "monkey.", "a.b.c.d.e.", DNS_R_PARTIALMATCH, 5 },
		{ "monker.", "a.b.c.d.e.", DNS_R_PARTIALMATCH, 5 },
		{ NULL, NULL, 0, 0 }
	};

	check_predecessors(qp, check2);

	dns_qp_destroy(&qp);
}

/*
 * this is a regression test for an infinite loop that could
 * previously occur in fix_iterator()
 */
ISC_RUN_TEST_IMPL(fixiterator) {
	dns_qp_t *qp = NULL;
	const char insert1[][32] = { "dynamic.",
				     "a.dynamic.",
				     "aaaa.dynamic.",
				     "cdnskey.dynamic.",
				     "cds.dynamic.",
				     "cname.dynamic.",
				     "dname.dynamic.",
				     "dnskey.dynamic.",
				     "ds.dynamic.",
				     "mx.dynamic.",
				     "ns.dynamic.",
				     "nsec.dynamic.",
				     "private-cdnskey.dynamic.",
				     "private-dnskey.dynamic.",
				     "rrsig.dynamic.",
				     "txt.dynamic.",
				     "trailing.",
				     "" };
	int i = 0;

	dns_qp_create(mctx, &string_methods, NULL, &qp);
	while (insert1[i][0] != '\0') {
		insert_str(qp, insert1[i++]);
	}

	static struct check_predecessors check1[] = {
		{ "newtext.dynamic.", "mx.dynamic.", DNS_R_PARTIALMATCH, 7 },
		{ "nsd.dynamic.", "ns.dynamic.", DNS_R_PARTIALMATCH, 6 },
		{ "nsf.dynamic.", "nsec.dynamic.", DNS_R_PARTIALMATCH, 5 },
		{ "d.", "trailing.", ISC_R_NOTFOUND, 0 },
		{ "absent.", "trailing.", ISC_R_NOTFOUND, 0 },
		{ "nonexistent.", "txt.dynamic.", ISC_R_NOTFOUND, 1 },
		{ "wayback.", "trailing.", ISC_R_NOTFOUND, 0 },
		{ NULL, NULL, 0, 0 }
	};

	check_predecessors(qp, check1);
	dns_qp_destroy(&qp);

	const char insert2[][64] = { ".", "abb.", "abc.", "" };
	i = 0;

	dns_qp_create(mctx, &string_methods, NULL, &qp);
	while (insert2[i][0] != '\0') {
		insert_str(qp, insert2[i++]);
	}

	static struct check_predecessors check2[] = {
		{ "acb.", "abc.", DNS_R_PARTIALMATCH, 0 },
		{ "acc.", "abc.", DNS_R_PARTIALMATCH, 0 },
		{ "abbb.", "abb.", DNS_R_PARTIALMATCH, 1 },
		{ "aab.", ".", DNS_R_PARTIALMATCH, 2 },
		{ NULL, NULL, 0, 0 }
	};

	check_predecessors(qp, check2);
	dns_qp_destroy(&qp);

	const char insert3[][64] = { "example.",
				     "key-is-13779.example.",
				     "key-is-14779.example.",
				     "key-not-13779.example.",
				     "key-not-14779.example.",
				     "" };
	i = 0;

	dns_qp_create(mctx, &string_methods, NULL, &qp);
	while (insert3[i][0] != '\0') {
		insert_str(qp, insert3[i++]);
	}

	static struct check_predecessors check3[] = { { "key-is-21556.example.",
							"key-is-14779.example.",
							DNS_R_PARTIALMATCH, 2 },
						      { NULL, NULL, 0, 0 } };

	check_predecessors(qp, check3);
	dns_qp_destroy(&qp);

	const char insert4[][64] = { ".", "\\000.", "\\000.\\000.",
				     "\\000\\009.", "" };
	i = 0;

	dns_qp_create(mctx, &string_methods, NULL, &qp);
	while (insert4[i][0] != '\0') {
		insert_str(qp, insert4[i++]);
	}

	static struct check_predecessors check4[] = {
		{ "\\007.", "\\000\\009.", DNS_R_PARTIALMATCH, 0 },
		{ "\\009.", "\\000\\009.", DNS_R_PARTIALMATCH, 0 },
		{ "\\045.", "\\000\\009.", DNS_R_PARTIALMATCH, 0 },
		{ "\\044.", "\\000\\009.", DNS_R_PARTIALMATCH, 0 },
		{ "\\000.", ".", ISC_R_SUCCESS, 3 },
		{ NULL, NULL, 0, 0 },
	};

	check_predecessors(qp, check4);
	dns_qp_destroy(&qp);
}

ISC_TEST_LIST_START
ISC_TEST_ENTRY(qpkey_name)
ISC_TEST_ENTRY(qpkey_sort)
ISC_TEST_ENTRY(qpiter)
ISC_TEST_ENTRY(partialmatch)
ISC_TEST_ENTRY(qpchain)
ISC_TEST_ENTRY(predecessors)
ISC_TEST_ENTRY(fixiterator)
ISC_TEST_LIST_END

ISC_TEST_MAIN
