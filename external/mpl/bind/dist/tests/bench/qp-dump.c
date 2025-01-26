/*	$NetBSD: qp-dump.c,v 1.2 2025/01/26 16:25:47 christos Exp $	*/

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
	fprintf(stderr,
		"usage: qp_dump [-dt] <filename>\n"
		"	-d	output in graphviz dot format\n"
		"	-t	output in ad-hoc indented text format\n");
}

int
main(int argc, char *argv[]) {
	isc_result_t result;
	dns_qp_t *qp = NULL;
	const char *filename = NULL;
	char *filetext = NULL;
	size_t filesize;
	off_t fileoff;
	FILE *fp = NULL;
	size_t wirebytes = 0, labels = 0, names = 0;
	char *pos = NULL, *file_end = NULL;
	bool dumpdot = false, dumptxt = false;
	int opt;

	while ((opt = isc_commandline_parse(argc, argv, "dt")) != -1) {
		switch (opt) {
		case 'd':
			dumpdot = true;
			continue;
		case 't':
			dumptxt = true;
			continue;
		default:
			usage();
			exit(EXIT_FAILURE);
			continue;
		}
	}
	argc -= isc_commandline_index;
	argv += isc_commandline_index;

	if (argc != 1) {
		/* must exit 0 to appease test runner */
		usage();
		exit(EXIT_SUCCESS);
	}

	isc_mem_create(&mctx);

	filename = argv[0];
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

	dns_qp_create(mctx, &methods, NULL, &qp);

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

		wirebytes += name->length;
		labels += name->labels;
		names += 1;
	}
	dns_qp_compact(qp, DNS_QPGC_ALL);

#define print_megabytes(label, value) \
	printf("%6.2f MiB - " label "\n", (double)(value) / 1048576.0)

	if (!dumptxt && !dumpdot) {
		size_t smallbytes = wirebytes + labels +
				    names * sizeof(isc_refcount_t);
		dns_qp_memusage_t memusage = dns_qp_memusage(qp);
		uint64_t compaction_us, recovery_us, rollback_us;
		dns_qp_gctime(&compaction_us, &recovery_us, &rollback_us);

		printf("leaves %zu\n"
		       " nodes %zu\n"
		       "  used %zu\n"
		       "  free %zu\n"
		       "   cow %zu\n"
		       "chunks %zu\n"
		       " bytes %zu\n",
		       memusage.leaves, memusage.live, memusage.used,
		       memusage.free, memusage.hold, memusage.chunk_count,
		       memusage.bytes);

		printf("%f compaction\n", (double)compaction_us / 1000000);
		printf("%f recovery\n", (double)recovery_us / 1000000);
		printf("%f rollback\n", (double)rollback_us / 1000000);

		size_t bytes = memusage.bytes;
		print_megabytes("file size", filesize);
		print_megabytes("names", wirebytes);
		print_megabytes("labels", labels);
		print_megabytes("names + labels", wirebytes + labels);
		print_megabytes("smallnames", smallbytes);
		print_megabytes("qp-trie", bytes);
		print_megabytes("qp-trie + smallnames", bytes + smallbytes);
		print_megabytes("calculated", bytes + smallbytes + filesize);
		print_megabytes("allocated", isc_mem_inuse(mctx));
		printf("%6zu - height\n", qp_test_getheight(qp));
		printf("%6zu - max key len\n", qp_test_maxkeylen(qp));
	}

	if (dumptxt) {
		qp_test_dumptrie(qp);
	}
	if (dumpdot) {
		qp_test_dumpdot(qp);
	}

	return 0;
}
