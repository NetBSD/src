/*	$NetBSD: qp.c,v 1.1.1.1 2025/01/26 16:12:38 christos Exp $	*/

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
#include <stdint.h>
#include <stdio.h>

#include <isc/buffer.h>
#include <isc/loop.h>
#include <isc/magic.h>
#include <isc/refcount.h>
#include <isc/rwlock.h>
#include <isc/urcu.h>
#include <isc/util.h>

#include <dns/fixedname.h>
#include <dns/name.h>
#include <dns/qp.h>
#include <dns/types.h>

#include "qp_p.h"

#include <tests/qp.h>

/***********************************************************************
 *
 *  key reverse conversions
 */

uint8_t
qp_test_bittoascii(dns_qpshift_t bit) {
	uint8_t byte = dns_qp_byte_for_bit[bit];
	if (bit == SHIFT_NOBYTE) {
		return '.';
	} else if (qp_common_character(byte)) {
		return byte;
	} else if (byte < '-') {
		return '#';
	} else if (byte < '_') {
		return '@';
	} else {
		return '~' - SHIFT_OFFSET + bit;
	}
}

const char *
qp_test_keytoascii(dns_qpkey_t key, size_t len) {
	for (size_t offset = 0; offset < len; offset++) {
		key[offset] = qp_test_bittoascii(key[offset]);
	}
	key[len] = '\0';
	return (const char *)key;
}

/***********************************************************************
 *
 *  trie properties
 */

static size_t
getheight(dns_qp_t *qp, dns_qpnode_t *n) {
	if (node_tag(n) == LEAF_TAG) {
		return 0;
	}
	size_t max_height = 0;
	dns_qpnode_t *twigs = branch_twigs(qp, n);
	dns_qpweight_t size = branch_twigs_size(n);
	for (dns_qpweight_t pos = 0; pos < size; pos++) {
		size_t height = getheight(qp, &twigs[pos]);
		max_height = ISC_MAX(max_height, height);
	}
	return max_height + 1;
}

size_t
qp_test_getheight(dns_qp_t *qp) {
	dns_qpnode_t *root = get_root(qp);
	return root == NULL ? 0 : getheight(qp, root);
}

static size_t
maxkeylen(dns_qp_t *qp, dns_qpnode_t *n) {
	if (node_tag(n) == LEAF_TAG) {
		dns_qpkey_t key;
		return leaf_qpkey(qp, n, key);
	}
	size_t max_len = 0;
	dns_qpnode_t *twigs = branch_twigs(qp, n);
	dns_qpweight_t size = branch_twigs_size(n);
	for (dns_qpweight_t pos = 0; pos < size; pos++) {
		size_t len = maxkeylen(qp, &twigs[pos]);
		max_len = ISC_MAX(max_len, len);
	}
	return max_len;
}

size_t
qp_test_maxkeylen(dns_qp_t *qp) {
	dns_qpnode_t *root = get_root(qp);
	return root == NULL ? 0 : maxkeylen(qp, root);
}

/***********************************************************************
 *
 *  dump to stdout
 */

static void
dumpread(dns_qpreadable_t qpr, const char *type, const char *tail) {
	dns_qpreader_t *qp = dns_qpreader(qpr);
	printf("%s %p root %u %u:%u base %p methods %p%s", type, qp,
	       qp->root_ref, ref_chunk(qp->root_ref), ref_cell(qp->root_ref),
	       qp->base, qp->methods, tail);
}

static void
dumpqp(dns_qp_t *qp, const char *type) {
	dumpread(qp, type, " mctx ");
	printf("%p\n", qp->mctx);
	printf("%s %p usage %p chunk_max %u bump %u fender %u\n", type, qp,
	       qp->usage, qp->chunk_max, qp->bump, qp->fender);
	printf("%s %p leaf %u live %u used %u free %u hold %u\n", type, qp,
	       qp->leaf_count, qp->used_count - qp->free_count, qp->used_count,
	       qp->free_count, qp->hold_count);
	printf("%s %p compact_all=%d transaction_mode=%d write_protect=%d\n",
	       type, qp, qp->compact_all, qp->transaction_mode,
	       qp->write_protect);
}

void
qp_test_dumpread(dns_qpreadable_t qp) {
	dumpread(qp, "qpread", "\n");
	fflush(stdout);
}

void
qp_test_dumpsnap(dns_qpsnap_t *qp) {
	dumpread(qp, "qpsnap", " whence ");
	printf("%p\n", qp->whence);
	fflush(stdout);
}

void
qp_test_dumpqp(dns_qp_t *qp) {
	dumpqp(qp, "qp");
	fflush(stdout);
}

void
qp_test_dumpmulti(dns_qpmulti_t *multi) {
	dns_qpreader_t qpr;
	dns_qpnode_t *reader = rcu_dereference(multi->reader);
	dns_qpmulti_t *whence = unpack_reader(&qpr, reader);
	dumpqp(&multi->writer, "qpmulti->writer");
	printf("qpmulti->reader %p root_ref %u %u:%u base %p\n", reader,
	       qpr.root_ref, ref_chunk(qpr.root_ref), ref_cell(qpr.root_ref),
	       qpr.base);
	printf("qpmulti->reader %p whence %p\n", reader, whence);
	unsigned int snapshots = 0;
	for (dns_qpsnap_t *snap = ISC_LIST_HEAD(multi->snapshots); //
	     snap != NULL; snap = ISC_LIST_NEXT(snap, link), snapshots++)
	{
	}
	printf("qpmulti %p snapshots %u\n", multi, snapshots);
	fflush(stdout);
}

void
qp_test_dumpchunks(dns_qp_t *qp) {
	dns_qpcell_t used = 0;
	dns_qpcell_t free = 0;
	dumpqp(qp, "qp");
	for (dns_qpchunk_t c = 0; c < qp->chunk_max; c++) {
		printf("qp %p chunk %u base %p "
		       "used %u free %u immutable %u discounted %u\n",
		       qp, c, qp->base->ptr[c], qp->usage[c].used,
		       qp->usage[c].free, qp->usage[c].immutable,
		       qp->usage[c].discounted);
		used += qp->usage[c].used;
		free += qp->usage[c].free;
	}
	printf("qp %p total used %u free %u\n", qp, used, free);
	fflush(stdout);
}

void
qp_test_dumptrie(dns_qpreadable_t qpr) {
	dns_qpreader_t *qp = dns_qpreader(qpr);
	struct {
		dns_qpref_t ref;
		dns_qpshift_t max, pos;
	} stack[512];
	size_t sp = 0;
	dns_qpcell_t leaf_count = 0;

	/*
	 * fake up a sentinel stack entry corresponding to the root
	 * node; the ref is deliberately out of bounds, and pos == max
	 * so we will immediately stop scanning it
	 */
	stack[sp].ref = INVALID_REF;
	stack[sp].max = 0;
	stack[sp].pos = 0;

	dns_qpnode_t *n = get_root(qp);
	if (n == NULL) {
		printf("%p EMPTY\n", n);
		fflush(stdout);
		return;
	} else {
		printf("%p ROOT qp %p base %p\n", n, qp, qp->base);
	}

	for (;;) {
		if (is_branch(n)) {
			dns_qpref_t ref = branch_twigs_ref(n);
			dns_qpweight_t max = branch_twigs_size(n);
			dns_qpnode_t *twigs = ref_ptr(qp, ref);

			/* brief list of twigs */
			dns_qpkey_t bits;
			size_t len = 0;
			for (dns_qpshift_t bit = SHIFT_NOBYTE;
			     bit < SHIFT_OFFSET; bit++)
			{
				if (branch_has_twig(n, bit)) {
					bits[len++] = bit;
				}
			}
			assert(len == max);
			qp_test_keytoascii(bits, len);
			printf("%*s%p BRANCH %p %u %u:%u %zu %s\n", (int)sp * 2,
			       "", n, twigs, ref, ref_chunk(ref), ref_cell(ref),
			       branch_key_offset(n), bits);

			++sp;
			stack[sp].ref = ref;
			stack[sp].max = max;
			stack[sp].pos = 0;
		} else {
			dns_qpkey_t key;
			qp_test_keytoascii(key, leaf_qpkey(qp, n, key));
			printf("%*s%p LEAF %p %d %s\n", (int)sp * 2, "", n,
			       leaf_pval(n), leaf_ival(n), key);
			leaf_count++;
		}

		while (stack[sp].pos == stack[sp].max) {
			if (sp == 0) {
				printf("LEAVES %d\n", leaf_count);
				fflush(stdout);
				return;
			}
			--sp;
		}

		stack[sp].pos++;
		fprintf(stderr, "pos %d/%d, ref+%d\n", stack[sp].pos,
			stack[sp].max, stack[sp].pos - 1);
		n = ref_ptr(qp, stack[sp].ref) + stack[sp].pos - 1;
	}
}

static void
dumpdot_name(dns_qpnode_t *n) {
	if (n == NULL) {
		printf("empty");
	} else if (is_branch(n)) {
		dns_qpref_t ref = branch_twigs_ref(n);
		printf("c%dn%d", ref_chunk(ref), ref_cell(ref));
	} else {
		printf("v%p", leaf_pval(n));
	}
}

static void
dumpdot_twig(dns_qp_t *qp, dns_qpnode_t *n) {
	if (n == NULL) {
		printf("empty [shape=oval, label=\"\\N EMPTY\"];\n");
	} else if (is_branch(n)) {
		dumpdot_name(n);
		printf(" [shape=record, label=\"{ \\N\\noff %zu | ",
		       branch_key_offset(n));
		char sep = '{';
		for (dns_qpshift_t bit = SHIFT_NOBYTE; bit < SHIFT_OFFSET;
		     bit++)
		{
			if (branch_has_twig(n, bit)) {
				printf("%c <t%d> %c ", sep,
				       branch_twig_pos(n, bit),
				       qp_test_bittoascii(bit));
				sep = '|';
			}
		}
		printf("}}\"];\n");

		dns_qpnode_t *twigs = branch_twigs(qp, n);
		dns_qpweight_t size = branch_twigs_size(n);

		for (dns_qpweight_t pos = 0; pos < size; pos++) {
			dumpdot_name(n);
			printf(":t%d:e -> ", pos);
			dumpdot_name(&twigs[pos]);
			printf(":w;\n");
		}

		for (dns_qpweight_t pos = 0; pos < size; pos++) {
			dumpdot_twig(qp, &twigs[pos]);
		}

	} else {
		dns_qpkey_t key;
		const char *str;
		str = qp_test_keytoascii(key, leaf_qpkey(qp, n, key));
		printf("v%p [shape=oval, label=\"\\N ival %d\\n%s\"];\n",
		       leaf_pval(n), leaf_ival(n), str);
	}
}

void
qp_test_dumpdot(dns_qp_t *qp) {
	REQUIRE(QP_VALID(qp));
	dns_qpnode_t *n = get_root(qp);
	printf("strict digraph {\nrankdir = \"LR\"; ranksep = 1.0;\n");
	printf("ROOT [shape=point]; ROOT -> ");
	dumpdot_name(n);
	printf(":w;\n");
	dumpdot_twig(qp, n);
	printf("}\n");
}

void
qp_test_printkey(const dns_qpkey_t key, size_t keylen) {
	dns_fixedname_t fn;
	dns_name_t *n = dns_fixedname_initname(&fn);
	char txt[DNS_NAME_FORMATSIZE];

	dns_qpkey_toname(key, keylen, n);
	dns_name_format(n, txt, sizeof(txt));
	printf("%s%s\n", txt, dns_name_isabsolute(n) ? "." : "");
}

/**********************************************************************/
