/*	$NetBSD: load-names.c,v 1.2 2025/01/26 16:25:47 christos Exp $	*/

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
#include <stdlib.h>

#include <isc/barrier.h>
#include <isc/file.h>
#include <isc/hashmap.h>
#include <isc/ht.h>
#include <isc/list.h>
#include <isc/rwlock.h>
#include <isc/thread.h>
#include <isc/urcu.h>
#include <isc/util.h>

#include <dns/fixedname.h>
#include <dns/qp.h>
#include <dns/rbt.h>
#include <dns/types.h>

#include "qp_p.h"

#include <tests/dns.h>
#include <tests/qp.h>

struct item_s {
	const char *text;
	dns_fixedname_t fixed;
	struct cds_lfht_node ht_node;
} item[1024 * 1024];

isc_barrier_t barrier;
isc_rwlock_t rwl;

struct thread_s {
	isc_thread_t thread;
	struct fun *fun;
	void *map;
	size_t start;
	size_t end;
	uint64_t d0;
	uint64_t d1;
} threads[1024];

static void
item_check(void *ctx, void *pval, uint32_t ival) {
	UNUSED(ctx);
	assert(pval == &item[ival]);
}

static size_t
item_makekey(dns_qpkey_t key, void *ctx, void *pval, uint32_t ival) {
	UNUSED(ctx);
	assert(pval == &item[ival]);
	return dns_qpkey_fromname(key, &item[ival].fixed.name);
}

static void
testname(void *ctx, char *buf, size_t size) {
	REQUIRE(ctx == NULL);
	strlcpy(buf, "test", size);
}

const dns_qpmethods_t qpmethods = {
	item_check,
	item_check,
	item_makekey,
	testname,
};

#define CHECK(count, result)                                        \
	do {                                                        \
		if (result != ISC_R_SUCCESS) {                      \
			dns_name_t *name = &item[count].fixed.name; \
			char buf[DNS_NAME_MAXTEXT] = { 0 };         \
			dns_name_format(name, buf, sizeof(buf));    \
			fprintf(stderr, "%s: %s\n", buf,            \
				isc_result_totext(result));         \
			exit(EXIT_FAILURE);                         \
		}                                                   \
	} while (0)

struct fun {
	const char *name;
	void *(*new)(isc_mem_t *mem);
	isc_threadfunc_t thread;
};

/*
 * cds_lfht
 */

static void *
new_lfht(isc_mem_t *mem ISC_ATTR_UNUSED) {
	struct cds_lfht *lfht = cds_lfht_new(
		1, 1, 0, CDS_LFHT_AUTO_RESIZE | CDS_LFHT_ACCOUNTING, NULL);
	return lfht;
}

static int
lfht_match(struct cds_lfht_node *ht_node, const void *_key) {
	const struct item_s *i = caa_container_of(ht_node, struct item_s,
						  ht_node);
	const dns_name_t *key = _key;

	return dns_name_equal(key, &i->fixed.name);
}

static isc_result_t
add_lfht(void *lfht, size_t count) {
	unsigned long hash = dns_name_hash(&item[count].fixed.name);

	struct cds_lfht_node *ht_node = cds_lfht_add_unique(
		lfht, hash, lfht_match, &item[count].fixed.name,
		&item[count].ht_node);

	if (ht_node != &item[count].ht_node) {
		return ISC_R_EXISTS;
	}

	return ISC_R_SUCCESS;
}

static isc_result_t
get_lfht(void *lfht, size_t count, void **pval) {
	unsigned long hash = dns_name_hash(&item[count].fixed.name);

	struct cds_lfht_iter iter;
	cds_lfht_lookup(lfht, hash, lfht_match, &item[count].fixed.name, &iter);

	struct cds_lfht_node *ht_node = cds_lfht_iter_get_node(&iter);
	if (ht_node == NULL) {
		return ISC_R_NOTFOUND;
	}

	*pval = caa_container_of(ht_node, struct item_s, ht_node);
	return ISC_R_SUCCESS;
}

static void *
thread_lfht(void *arg0) {
	struct thread_s *arg = arg0;

	isc_barrier_wait(&barrier);

	isc_time_t t0 = isc_time_now_hires();
	for (size_t n = arg->start; n < arg->end; n++) {
		isc_result_t result = add_lfht(arg->map, n);
		CHECK(n, result);
	}

	isc_time_t t1 = isc_time_now_hires();
	for (size_t n = arg->start; n < arg->end; n++) {
		void *pval = NULL;
		isc_result_t result = get_lfht(arg->map, n, &pval);
		CHECK(n, result);
		assert(pval == &item[n]);
	}

	isc_time_t t2 = isc_time_now_hires();

	arg->d0 = isc_time_microdiff(&t1, &t0);
	arg->d1 = isc_time_microdiff(&t2, &t1);

	return NULL;
}

/*
 * hashmap
 */

static void *
new_hashmap(isc_mem_t *mem) {
	isc_hashmap_t *hashmap = NULL;
	isc_hashmap_create(mem, 1, &hashmap);

	return hashmap;
}

static bool
name_match(void *node, const void *key) {
	const struct item_s *i = node;
	return dns_name_equal(&i->fixed.name, key);
}

static isc_result_t
add_hashmap(void *hashmap, size_t count) {
	isc_result_t result = isc_hashmap_add(
		hashmap, dns_name_hash(&item[count].fixed.name), name_match,
		&item[count].fixed.name, &item[count], NULL);
	return result;
}

static isc_result_t
get_hashmap(void *hashmap, size_t count, void **pval) {
	isc_result_t result = isc_hashmap_find(
		hashmap, dns_name_hash(&item[count].fixed.name), name_match,
		&item[count].fixed.name, pval);
	return result;
}

static void *
thread_hashmap(void *arg0) {
	struct thread_s *arg = arg0;

	isc_barrier_wait(&barrier);

	isc_time_t t0 = isc_time_now_hires();
	WRLOCK(&rwl);
	for (size_t n = arg->start; n < arg->end; n++) {
		isc_result_t result = add_hashmap(arg->map, n);
		CHECK(n, result);
	}
	WRUNLOCK(&rwl);

	isc_time_t t1 = isc_time_now_hires();
	RDLOCK(&rwl);
	for (size_t n = arg->start; n < arg->end; n++) {
		void *pval = NULL;
		isc_result_t result = get_hashmap(arg->map, n, &pval);
		CHECK(n, result);
		assert(pval == &item[n]);
	}
	RDUNLOCK(&rwl);
	isc_time_t t2 = isc_time_now_hires();

	arg->d0 = isc_time_microdiff(&t1, &t0);
	arg->d1 = isc_time_microdiff(&t2, &t1);

	return NULL;
}

/*
 * ht
 */

static void *
new_ht(isc_mem_t *mem) {
	isc_ht_t *ht = NULL;
	isc_ht_init(&ht, mem, 1, 0);
	return ht;
}

static isc_result_t
add_ht(void *ht, size_t count) {
	isc_result_t result = isc_ht_add(ht, item[count].fixed.name.ndata,
					 item[count].fixed.name.length,
					 &item[count]);
	return result;
}

static isc_result_t
get_ht(void *ht, size_t count, void **pval) {
	isc_result_t result = isc_ht_find(ht, item[count].fixed.name.ndata,
					  item[count].fixed.name.length, pval);
	return result;
}

static void *
thread_ht(void *arg0) {
	struct thread_s *arg = arg0;

	isc_barrier_wait(&barrier);

	isc_time_t t0 = isc_time_now_hires();
	WRLOCK(&rwl);
	for (size_t n = arg->start; n < arg->end; n++) {
		isc_result_t result = add_ht(arg->map, n);
		CHECK(n, result);
	}
	WRUNLOCK(&rwl);

	isc_time_t t1 = isc_time_now_hires();
	RDLOCK(&rwl);
	for (size_t n = arg->start; n < arg->end; n++) {
		void *pval = NULL;
		isc_result_t result = get_ht(arg->map, n, &pval);
		CHECK(n, result);
		assert(pval == &item[n]);
	}
	RDUNLOCK(&rwl);
	isc_time_t t2 = isc_time_now_hires();

	arg->d0 = isc_time_microdiff(&t1, &t0);
	arg->d1 = isc_time_microdiff(&t2, &t1);

	return NULL;
}

/*
 * rbt
 */

static void *
new_rbt(isc_mem_t *mem) {
	dns_rbt_t *rbt = NULL;
	(void)dns_rbt_create(mem, NULL, NULL, &rbt);
	return rbt;
}

static isc_result_t
add_rbt(void *rbt, size_t count) {
	isc_result_t result;
	dns_rbtnode_t *node = NULL;

	result = dns_rbt_addnode(rbt, &item[count].fixed.name, &node);
	if (result == ISC_R_SUCCESS ||
	    (result == ISC_R_EXISTS && node->data == NULL))
	{
		node->data = &item[count];
		result = ISC_R_SUCCESS;
	}

	return result;
}

static isc_result_t
get_rbt(void *rbt, size_t count, void **pval) {
	isc_result_t result;
	dns_rbtnode_t *node = NULL;

	result = dns_rbt_findnode(rbt, &item[count].fixed.name, NULL, &node,
				  NULL, 0, NULL, NULL);
	if (result == ISC_R_SUCCESS) {
		*pval = node->data;
	}
	return result;
}

static void *
thread_rbt(void *arg0) {
	struct thread_s *arg = arg0;

	isc_barrier_wait(&barrier);

	isc_time_t t0 = isc_time_now_hires();
	WRLOCK(&rwl);
	for (size_t n = arg->start; n < arg->end; n++) {
		isc_result_t result = add_rbt(arg->map, n);
		CHECK(n, result);
	}
	WRUNLOCK(&rwl);

	isc_time_t t1 = isc_time_now_hires();
	RDLOCK(&rwl);
	for (size_t n = arg->start; n < arg->end; n++) {
		void *pval = NULL;
		isc_result_t result = get_rbt(arg->map, n, &pval);
		CHECK(n, result);
		assert(pval == &item[n]);
	}
	RDUNLOCK(&rwl);

	isc_time_t t2 = isc_time_now_hires();

	arg->d0 = isc_time_microdiff(&t1, &t0);
	arg->d1 = isc_time_microdiff(&t2, &t1);

	return NULL;
}

/*
 * qp
 */

static void *
new_qp(isc_mem_t *mem) {
	dns_qpmulti_t *qpmulti = NULL;
	dns_qpmulti_create(mem, &qpmethods, NULL, &qpmulti);
	return qpmulti;
}

static isc_result_t
add_qp(void *qp, size_t count) {
	isc_result_t result = dns_qp_insert(qp, &item[count], count);
	return result;
}

static void
sqz_qp(void *qp) {
	dns_qp_compact(qp, DNS_QPGC_MAYBE);
}

static isc_result_t
get_qp(void *qp, size_t count, void **pval) {
	return dns_qp_getname(qp, &item[count].fixed.name, pval, NULL);
}

static void *
_thread_qp(void *arg0, bool sqz, bool brr) {
	struct thread_s *arg = arg0;

	isc_barrier_wait(&barrier);

	dns_qp_t *qp = NULL;
	dns_qpmulti_write(arg->map, &qp);

	isc_time_t t0 = isc_time_now_hires();
	for (size_t n = arg->start; n < arg->end; n++) {
		isc_result_t result = add_qp(qp, n);
		CHECK(n, result);
	}
	if (sqz) {
		sqz_qp(qp);
	}
	dns_qpmulti_commit(arg->map, &qp);
	if (brr) {
		rcu_barrier();
	}

	isc_time_t t1 = isc_time_now_hires();

	dns_qpread_t qpr;
	dns_qpmulti_query(arg->map, &qpr);

	for (size_t n = arg->start; n < arg->end; n++) {
		void *pval = NULL;
		isc_result_t result = get_qp(&qpr, n, &pval);
		CHECK(n, result);
		assert(pval == &item[n]);
	}

	dns_qpread_destroy(arg->map, &qpr);

	isc_time_t t2 = isc_time_now_hires();

	arg->d0 = isc_time_microdiff(&t1, &t0);
	arg->d1 = isc_time_microdiff(&t2, &t1);

	return NULL;
}

static void *
thread_qp(void *arg0) {
	return _thread_qp(arg0, true, false);
}

static void *
thread_qp_nosqz(void *arg0) {
	return _thread_qp(arg0, false, false);
}

static void *
thread_qp_brr(void *arg0) {
	return _thread_qp(arg0, true, true);
}

/*
 * fun table
 */
static struct fun fun_list[] = {
	{ "lfht", new_lfht, thread_lfht },
	{ "ht", new_ht, thread_ht },
	{ "hashmap", new_hashmap, thread_hashmap },
	{ "rbt", new_rbt, thread_rbt },
	{ "qp", new_qp, thread_qp },
	{ "qp+nosqz", new_qp, thread_qp_nosqz },
	{ "qp+barrier", new_qp, thread_qp_brr },
	{ NULL, NULL, NULL },
};

#define FILE_CHECK(check, msg)                                                 \
	do {                                                                   \
		if (!(check)) {                                                \
			fprintf(stderr, "%s:%zu: %s\n", filename, lines, msg); \
			exit(EXIT_FAILURE);                                    \
		}                                                              \
	} while (0)

int
main(int argc, char *argv[]) {
	isc_result_t result;
	const char *filename = NULL;
	char *filetext = NULL;
	off_t fileoff;
	FILE *fp = NULL;
	size_t filesize, lines = 0, wirebytes = 0, labels = 0;
	char *pos = NULL, *file_end = NULL;

	isc_rwlock_init(&rwl);

	isc_mem_create(&mctx);

	if (argc != 2) {
		fprintf(stderr,
			"usage: load-names <filename.csv> <nthreads>\n");
		exit(EXIT_FAILURE);
	}

	filename = argv[1];
	result = isc_file_getsize(filename, &fileoff);
	if (result != ISC_R_SUCCESS) {
		fprintf(stderr, "stat(%s): %s\n", filename,
			isc_result_totext(result));
		exit(EXIT_FAILURE);
	}
	filesize = (size_t)fileoff;

	filetext = isc_mem_get(mctx, filesize + 1);
	fp = fopen(filename, "r");
	if (fp == NULL || fread(filetext, 1, filesize, fp) < filesize) {
		fprintf(stderr, "read(%s): %s\n", filename, strerror(errno));
		exit(EXIT_FAILURE);
	}
	fclose(fp);
	filetext[filesize] = '\0';

	pos = filetext;
	file_end = pos + filesize;
	while (pos < file_end) {
		char *domain = NULL, *newline = NULL;
		size_t len;

		FILE_CHECK(lines < ARRAY_SIZE(item), "too many lines");
		pos += strspn(pos, "0123456789");

		FILE_CHECK(*pos++ == ',', "missing comma");

		domain = pos;
		pos += strcspn(pos, "\r\n");
		FILE_CHECK(*pos != '\0', "missing newline");
		newline = pos;
		pos += strspn(pos, "\r\n");
		len = newline - domain;

		item[lines].text = domain;
		domain[len] = '\0';

		dns_name_t *name = dns_fixedname_initname(&item[lines].fixed);
		isc_buffer_t buffer;
		isc_buffer_init(&buffer, domain, len);
		isc_buffer_add(&buffer, len);
		result = dns_name_fromtext(name, &buffer, dns_rootname, 0,
					   NULL);
		FILE_CHECK(result == ISC_R_SUCCESS, isc_result_totext(result));

		wirebytes += name->length;
		labels += name->labels;
		lines++;
	}

	printf("names %g MB labels %g MB\n\n", (double)wirebytes / 1048576.0,
	       (double)labels / 1048576.0);

	printf("%10s | %10s | %10s | %10s | %10s | %10s | %10s |\n",
	       "algorithm", "threads", "load", "query", "dirty MB", "total",
	       "final MB");

	for (size_t nthreads = 128; nthreads > 0; nthreads /= 2) {
		printf("---------- | ---------- | ---------- | ---------- | "
		       "---------- | ---------- | ---------- |\n");

		for (struct fun *fun = fun_list; fun->name != NULL; fun++) {
			isc_mem_t *mem = NULL;
			void *map = NULL;

			isc_mem_create(&mem);
			map = fun->new(mem);

			size_t nitems = lines / (nthreads + 1);

			isc_barrier_init(&barrier, nthreads);

			isc_time_t t0 = isc_time_now_hires();
			size_t m0 = isc_mem_inuse(mem);

			for (size_t i = 0; i < nthreads; i++) {
				threads[i] = (struct thread_s){
					.fun = fun,
					.map = map,
					.start = nitems * i,
					.end = nitems * i + nitems,
				};
				isc_thread_create(fun->thread, &threads[i],
						  &threads[i].thread);
			}

			uint64_t d0 = 0;
			uint64_t d1 = 0;

			for (size_t i = 0; i < nthreads; i++) {
				isc_thread_join(threads[i].thread, NULL);
				d0 += threads[i].d0;
				d1 += threads[i].d1;
			}

			size_t m1 = isc_mem_inuse(mem);

			rcu_barrier();

			isc_time_t t1 = isc_time_now_hires();
			uint64_t d3 = isc_time_microdiff(&t1, &t0);
			size_t m2 = isc_mem_inuse(mem);

			printf("%10s | %10zu | %10.4f | %10.4f | %10.4f | "
			       "%10.4f | %10.4f |\n",
			       fun->name, nthreads,
			       (double)(d0 / nthreads) / (1000.0 * 1000.0),
			       (double)(d1 / nthreads) / (1000.0 * 1000.0),
			       (double)(m1 - m0) / (1024.0 * 1024.0),
			       (double)d3 / (1000.0 * 1000.0),
			       (double)(m2 - m0) / (1024.0 * 1024.0)

			);
		}
	}

	printf("---------- | ---------- | ---------- | ---------- | "
	       "---------- | ---------- | ---------- |\n");
}
