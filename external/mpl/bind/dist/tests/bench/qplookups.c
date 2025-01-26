/*	$NetBSD: qplookups.c,v 1.2 2025/01/26 16:25:47 christos Exp $	*/

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

#include <isc/commandline.h>
#include <isc/file.h>
#include <isc/ht.h>
#include <isc/rwlock.h>
#include <isc/time.h>
#include <isc/util.h>

#include <dns/fixedname.h>
#include <dns/qp.h>
#include <dns/rbt.h>
#include <dns/types.h>

#include <tests/dns.h>
#include <tests/qp.h>

static inline size_t
smallname_length(void *pval, uint32_t ival) {
	UNUSED(pval);
	return ival & 0xff;
}

static inline size_t
smallname_labels(void *pval, uint32_t ival) {
	UNUSED(pval);
	return ival >> 8;
}

static inline isc_refcount_t *
smallname_refcount(void *pval, uint32_t ival) {
	UNUSED(ival);
	return pval;
}

static inline uint8_t *
smallname_ndata(void *pval, uint32_t ival) {
	return (uint8_t *)(smallname_refcount(pval, ival) + 1);
}

static inline uint8_t *
smallname_offsets(void *pval, uint32_t ival) {
	return smallname_ndata(pval, ival) + smallname_length(pval, ival);
}

static void
smallname_from_name(const dns_name_t *name, void **valp, uint32_t *ctxp) {
	size_t size = sizeof(isc_refcount_t) + name->length + name->labels;
	*valp = isc_mem_get(mctx, size);
	*ctxp = name->labels << 8 | name->length;
	isc_refcount_init(smallname_refcount(*valp, *ctxp), 0);
	memmove(smallname_ndata(*valp, *ctxp), name->ndata, name->length);
	memmove(smallname_offsets(*valp, *ctxp), name->offsets, name->labels);
}

static void
smallname_free(void *pval, uint32_t ival) {
	size_t size = sizeof(isc_refcount_t);
	size += smallname_length(pval, ival) + smallname_labels(pval, ival);
	isc_mem_put(mctx, pval, size);
}

static void
name_from_smallname(dns_name_t *name, void *pval, uint32_t ival) {
	dns_name_reset(name);
	name->ndata = smallname_ndata(pval, ival);
	name->length = smallname_length(pval, ival);
	name->labels = smallname_labels(pval, ival);
	name->offsets = smallname_offsets(pval, ival);
	name->attributes.readonly = true;
	if (name->ndata[name->offsets[name->labels - 1]] == '\0') {
		name->attributes.absolute = true;
	}
}

static size_t
qpkey_from_smallname(dns_qpkey_t key, void *ctx, void *pval, uint32_t ival) {
	UNUSED(ctx);
	dns_name_t name = DNS_NAME_INITEMPTY;
	name_from_smallname(&name, pval, ival);
	return dns_qpkey_fromname(key, &name);
}

static void
smallname_attach(void *ctx, void *pval, uint32_t ival) {
	UNUSED(ctx);
	isc_refcount_increment0(smallname_refcount(pval, ival));
}

static void
smallname_detach(void *ctx, void *pval, uint32_t ival) {
	if (isc_refcount_decrement(smallname_refcount(pval, ival)) == 1) {
		isc_mem_free(ctx, pval);
	}
}

static void
testname(void *ctx, char *buf, size_t size) {
	REQUIRE(ctx == NULL);
	strlcpy(buf, "test", size);
}

const dns_qpmethods_t methods = {
	smallname_attach,
	smallname_detach,
	qpkey_from_smallname,
	testname,
};

static void
usage(void) {
	fprintf(stderr, "usage: lookups <filename>\n");
	exit(EXIT_FAILURE);
}

static size_t
load_qp(dns_qp_t *qp, const char *filename) {
	isc_result_t result;
	char *filetext = NULL;
	size_t filesize, names = 0;
	char *pos = NULL, *file_end = NULL;
	off_t fileoff;
	FILE *fp = NULL;

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
		void *pval = NULL;
		uint32_t ival = 0;
		dns_fixedname_t fixed;
		dns_name_t *name = dns_fixedname_initname(&fixed);
		isc_buffer_t buffer;
		char *newline = NULL, *domain = pos;
		size_t len;

		pos += strcspn(pos, "\r\n");
		newline = pos;
		pos += strspn(pos, "\r\n");

		len = newline - domain;
		domain[len] = '\0';

		isc_buffer_init(&buffer, domain, len);
		isc_buffer_add(&buffer, len);
		result = dns_name_fromtext(name, &buffer, dns_rootname, 0,
					   NULL);
		if (result == ISC_R_SUCCESS) {
			smallname_from_name(name, &pval, &ival);
			result = dns_qp_insert(qp, pval, ival);
		}
		if (result == ISC_R_EXISTS && pval != NULL) {
			smallname_free(pval, ival);
			continue;
		}
		if (result != ISC_R_SUCCESS) {
			fprintf(stderr, "%s:%zu: %s %s\n", filename, names,
				domain, isc_result_totext(result));
			exit(EXIT_FAILURE);
		}

		names++;
	}

	return names;
}

int
main(int argc, char **argv) {
	dns_qp_t *qp = NULL;
	isc_nanosecs_t start, stop;
	dns_fixedname_t *items = NULL;
	dns_qpiter_t it = { 0 };
	dns_name_t *name = NULL;
	size_t i = 0, n = 0;
	char buf[BUFSIZ];

	if (argc != 2) {
		usage();
	}

	isc_mem_create(&mctx);

	dns_qp_create(mctx, &methods, NULL, &qp);

	start = isc_time_monotonic();
	n = load_qp(qp, argv[1]);
	dns_qp_compact(qp, DNS_QPGC_ALL);
	stop = isc_time_monotonic();

	snprintf(buf, sizeof(buf), "load %zd names:", n);
	printf("%-57s%7.3fsec\n", buf, (stop - start) / (double)NS_PER_SEC);

	items = isc_mem_cget(mctx, n, sizeof(dns_fixedname_t));
	dns_qpiter_init(qp, &it);

	start = isc_time_monotonic();
	for (i = 0;; i++) {
		name = dns_fixedname_initname(&items[i]);
		if (dns_qpiter_next(&it, name, NULL, NULL) != ISC_R_SUCCESS) {
			break;
		}
	}
	stop = isc_time_monotonic();

	snprintf(buf, sizeof(buf), "iterate %zd names:", n);
	printf("%-57s%7.3fsec\n", buf, (stop - start) / (double)NS_PER_SEC);

	n = i;
	start = isc_time_monotonic();
	for (i = 0; i < n; i++) {
		name = dns_fixedname_name(&items[i]);
		dns_qp_getname(qp, name, NULL, NULL);
	}
	stop = isc_time_monotonic();

	snprintf(buf, sizeof(buf), "look up %zd names (dns_qp_getname):", n);
	printf("%-57s%7.3fsec\n", buf, (stop - start) / (double)NS_PER_SEC);

	start = isc_time_monotonic();
	for (i = 0; i < n; i++) {
		name = dns_fixedname_name(&items[i]);
		dns_qp_lookup(qp, name, 0, NULL, NULL, NULL, NULL);
	}
	stop = isc_time_monotonic();

	snprintf(buf, sizeof(buf), "look up %zd names (dns_qp_lookup):", n);
	printf("%-57s%7.3fsec\n", buf, (stop - start) / (double)NS_PER_SEC);

	start = isc_time_monotonic();
	for (i = 0; i < n; i++) {
		/*
		 * copy the name, and modify the first letter before
		 * searching; that way it probably won't be found in
		 * the QP trie. (though it might, if for example the trie
		 * contains both "x." and "y.". for best results,
		 * use input data where this isn't an issue.)
		 */
		dns_fixedname_t sf;
		dns_name_t *search = dns_fixedname_initname(&sf);

		name = dns_fixedname_name(&items[i]);
		dns_name_copy(name, search);
		if (search->ndata[1] != 0) {
			++search->ndata[1];
		}

		dns_qp_lookup(qp, search, 0, NULL, NULL, NULL, NULL);
	}
	stop = isc_time_monotonic();

	snprintf(buf, sizeof(buf),
		 "look up %zd wrong names (dns_qp_lookup):", n);
	printf("%-57s%7.3fsec\n", buf, (stop - start) / (double)NS_PER_SEC);

	isc_mem_cput(mctx, items, n, sizeof(dns_fixedname_t));
	return 0;
}
