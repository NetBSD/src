/*	$NetBSD: qp.h,v 1.2 2025/01/26 16:25:49 christos Exp $	*/

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

#pragma once

/*
 * Cheap but inaccurate conversion from bit numbers to ascii: this is
 * adequate for displaying the trie structure. It lacks any context
 * that would be necessary for handling escapes, so unusual characters
 * are fudged.
 */
uint8_t
qp_test_bittoascii(uint8_t bit);

/*
 * Simple and incorrect key conversion for display purposes.
 * Overwrites the key in place, and returns a pointer to the converted key.
 */
const char *
qp_test_keytoascii(dns_qpkey_t key, size_t len);

/*
 * The maximum height of the trie
 */
size_t
qp_test_getheight(dns_qp_t *qp);

/*
 * The maximum length of any key in the trie (for comparison with the trie's
 * height)
 */
size_t
qp_test_maxkeylen(dns_qp_t *qp);

/*
 * Print dns_qp_t metadata to stdout
 */
void
qp_test_dumpqp(dns_qp_t *qp);

/*
 * Print dns_qpread_t metadata to stdout
 */
void
qp_test_dumpread(dns_qpreadable_t qp);

/*
 * Print dns_qpsnap_t metadata to stdout
 */
void
qp_test_dumpsnap(dns_qpsnap_t *qps);

/*
 * Print dns_qpmulti_t metadata to stdout
 */
void
qp_test_dumpmulti(dns_qpmulti_t *multi);

/*
 * Print dns_qp_t chunk arrays to stdout
 */
void
qp_test_dumpchunks(dns_qp_t *qp);

/*
 * Print out the trie structure to stdout in an ad-hoc text format
 * that uses indentation to indicate depth
 */
void
qp_test_dumptrie(dns_qpreadable_t qp);

/*
 * Print out the trie structure to stdout in graphviz dot format
 */
void
qp_test_dumpdot(dns_qp_t *qp);

/*
 * Print the name encoded in a QP key.
 */
void
qp_test_printkey(const dns_qpkey_t key, size_t keylen);
