/*	$NetBSD: nametree.h,v 1.1.1.1 2025/01/26 16:12:35 christos Exp $	*/

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

/*****
***** Module Info
*****/

/*! \file
 * \brief
 * A nametree module is a tree of DNS names containing boolean values
 * or bitfields, allowing a quick lookup to see whether a name is included
 * in or excluded from some policy.
 */

#include <stdbool.h>

#include <isc/lang.h>
#include <isc/magic.h>
#include <isc/refcount.h>
#include <isc/rwlock.h>
#include <isc/stdtime.h>

#include <dns/rdatastruct.h>
#include <dns/types.h>

#include <dst/dst.h>

/* Add -DDNS_NAMETREE_TRACE=1 to CFLAGS for detailed reference tracing */

typedef enum {
	DNS_NAMETREE_BOOL,
	DNS_NAMETREE_BITS,
	DNS_NAMETREE_COUNT
} dns_nametree_type_t;

ISC_LANG_BEGINDECLS

void
dns_nametree_create(isc_mem_t *mctx, dns_nametree_type_t type, const char *name,
		    dns_nametree_t **ntp);
/*%<
 * Create a nametree.
 *
 * If 'name' is not NULL, it will be saved as the name of the QP trie
 * for debugging purposes.
 *
 * 'type' indicates whether the tree will be used for storing boolean
 * values (DNS_NAMETREE_BOOL), bitfields (DNS_NAMETREE_BITS), or counters
 * (DNS_NAMETREE_COUNT).
 *
 * Requires:
 *
 *\li	'mctx' is a valid memory context.
 *\li	ntp != NULL && *ntp == NULL
 */

isc_result_t
dns_nametree_add(dns_nametree_t *nametree, const dns_name_t *name,
		 uint32_t value);
/*%<
 * Add a node to 'nametree'.
 *
 * If the nametree type was set to DNS_NAMETREE_BOOL, then 'value'
 * represents a single boolean value, true or false. If the name already
 * exists within the tree, then return ISC_R_EXISTS.
 *
 * If the nametree type was set to DNS_NAMETREE_COUNT, then 'value'
 * can only be true. Each time the same name is added to the tree,
 * ISC_R_SUCCESS is returned and a counter is incremented.
 * dns_nametree_delete() must be deleted the same number of times
 * as dns_nametree_add() before the name is removed from the tree.
 *
 * If the nametree type was set to DNS_NAMETREE_BITS, then 'value' is
 * a bit number within a bit field, which is sized to accomodate at least
 * 'value' bits. If the name already exists, then that bit will be set
 * in the bitfield, other bits will be retained, and ISC_R_SUCCESS will be
 * returned. If 'value' excees the number of bits in the existing bit
 * field, the field will be expanded.
 *
 * Requires:
 *
 *\li	'nametree' points to a valid nametree.
 *
 * Returns:
 *
 *\li	ISC_R_SUCCESS
 *\li	ISC_R_EXISTS
 *
 *\li	Any other result indicates failure.
 */

isc_result_t
dns_nametree_delete(dns_nametree_t *nametree, const dns_name_t *name);
/*%<
 * Delete 'name' from 'nametree'.
 *
 * If the nametree type was set to DNS_NAMETREE_COUNT, then this must
 * be called for each name the same number of times as dns_nametree_add()
 * was called before the name is removed.
 *
 * Requires:
 *
 *\li	'nametree' points to a valid nametree.
 *\li	'name' is not NULL
 *
 * Returns:
 *
 *\li	ISC_R_SUCCESS
 *
 *\li	Any other result indicates failure.
 */

isc_result_t
dns_nametree_find(dns_nametree_t *nametree, const dns_name_t *name,
		  dns_ntnode_t **ntp);
/*%<
 * Retrieve the node that exactly matches 'name' from 'nametree'.
 *
 * Requires:
 *
 *\li	'nametree' is a valid nametree.
 *
 *\li	'name' is a valid name.
 *
 *\li	ntp != NULL && *ntp == NULL
 *
 * Returns:
 *
 *\li	ISC_R_SUCCESS
 *\li	ISC_R_NOTFOUND
 *
 *\li	Any other result indicates an error.
 */

bool
dns_nametree_covered(dns_nametree_t *nametree, const dns_name_t *name,
		     dns_name_t *found, uint32_t bit);
/*%<
 * Indicates whether a 'name' (with optional 'bit' value) is covered by
 * 'nametree'.
 *
 * In DNS_NAMETREE_BOOL nametrees, this returns true if 'name' has a match
 * or a closest ancestor in 'nametree' with its value set to 'true'.
 * 'bit' is ignored.
 *
 * In DNS_NAMETREE_BITS trees, this returns true if 'name' has a match or
 * a closest ancestor in 'nametree' with the 'bit' set in its bitfield.
 *
 * If a name is not found, the default return value is false.
 *
 * If 'found' is not NULL, the name or ancestor name that was found in
 * the tree is copied into it.
 *
 * Requires:
 *
 *\li	'nametree' is a valid nametree, or is NULL.
 */

#if DNS_NAMETREE_TRACE
#define dns_nametree_ref(ptr) \
	dns_nametree__ref(ptr, __func__, __FILE__, __LINE__)
#define dns_nametree_unref(ptr) \
	dns_nametree__unref(ptr, __func__, __FILE__, __LINE__)
#define dns_nametree_attach(ptr, ptrp) \
	dns_nametree__attach(ptr, ptrp, __func__, __FILE__, __LINE__)
#define dns_nametree_detach(ptrp) \
	dns_nametree__detach(ptrp, __func__, __FILE__, __LINE__)
#define dns_ntnode_ref(ptr) dns_ntnode__ref(ptr, __func__, __FILE__, __LINE__)
#define dns_ntnode_unref(ptr) \
	dns_ntnode__unref(ptr, __func__, __FILE__, __LINE__)
#define dns_ntnode_attach(ptr, ptrp) \
	dns_ntnode__attach(ptr, ptrp, __func__, __FILE__, __LINE__)
#define dns_ntnode_detach(ptrp) \
	dns_ntnode__detach(ptrp, __func__, __FILE__, __LINE__)
ISC_REFCOUNT_TRACE_DECL(dns_nametree);
ISC_REFCOUNT_TRACE_DECL(dns_ntnode);
#else
ISC_REFCOUNT_DECL(dns_nametree);
ISC_REFCOUNT_DECL(dns_ntnode);
#endif
ISC_LANG_ENDDECLS
