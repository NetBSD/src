/*	$NetBSD: nametree.c,v 1.1.1.1 2025/01/26 16:12:33 christos Exp $	*/

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

/*! \file */

#include <stdbool.h>

#include <isc/mem.h>
#include <isc/refcount.h>
#include <isc/result.h>
#include <isc/string.h>
#include <isc/urcu.h>
#include <isc/util.h>

#include <dns/fixedname.h>
#include <dns/nametree.h>
#include <dns/qp.h>

#define NAMETREE_MAGIC	   ISC_MAGIC('N', 'T', 'r', 'e')
#define VALID_NAMETREE(kt) ISC_MAGIC_VALID(kt, NAMETREE_MAGIC)

struct dns_nametree {
	unsigned int magic;
	isc_mem_t *mctx;
	isc_refcount_t references;
	dns_nametree_type_t type;
	dns_qpmulti_t *table;
	char name[64];
};

struct dns_ntnode {
	isc_mem_t *mctx;
	isc_refcount_t references;
	dns_name_t name;
	bool set;
	uint8_t *bits;
};

/* QP trie methods */
static void
qp_attach(void *uctx, void *pval, uint32_t ival);
static void
qp_detach(void *uctx, void *pval, uint32_t ival);
static size_t
qp_makekey(dns_qpkey_t key, void *uctx, void *pval, uint32_t ival);
static void
qp_triename(void *uctx, char *buf, size_t size);

static dns_qpmethods_t qpmethods = {
	qp_attach,
	qp_detach,
	qp_makekey,
	qp_triename,
};

static void
destroy_ntnode(dns_ntnode_t *node) {
	if (node->bits != NULL) {
		isc_mem_cput(node->mctx, node->bits, node->bits[0],
			     sizeof(char));
	}
	dns_name_free(&node->name, node->mctx);
	isc_mem_putanddetach(&node->mctx, node, sizeof(dns_ntnode_t));
}

#if DNS_NAMETREE_TRACE
ISC_REFCOUNT_TRACE_IMPL(dns_ntnode, destroy_ntnode);
#else
ISC_REFCOUNT_IMPL(dns_ntnode, destroy_ntnode);
#endif

void
dns_nametree_create(isc_mem_t *mctx, dns_nametree_type_t type, const char *name,
		    dns_nametree_t **ntp) {
	dns_nametree_t *nametree = NULL;

	REQUIRE(ntp != NULL && *ntp == NULL);

	nametree = isc_mem_get(mctx, sizeof(*nametree));
	*nametree = (dns_nametree_t){
		.magic = NAMETREE_MAGIC,
		.type = type,
	};
	isc_mem_attach(mctx, &nametree->mctx);
	isc_refcount_init(&nametree->references, 1);

	if (name != NULL) {
		strlcpy(nametree->name, name, sizeof(nametree->name));
	}

	dns_qpmulti_create(mctx, &qpmethods, nametree, &nametree->table);
	*ntp = nametree;
}

static void
destroy_nametree(dns_nametree_t *nametree) {
	nametree->magic = 0;

	dns_qpmulti_destroy(&nametree->table);

	isc_mem_putanddetach(&nametree->mctx, nametree, sizeof(*nametree));
}

#if DNS_NAMETREE_TRACE
ISC_REFCOUNT_TRACE_IMPL(dns_nametree, destroy_nametree);
#else
ISC_REFCOUNT_IMPL(dns_nametree, destroy_nametree);
#endif

static dns_ntnode_t *
newnode(isc_mem_t *mctx, const dns_name_t *name) {
	dns_ntnode_t *node = isc_mem_get(mctx, sizeof(*node));
	*node = (dns_ntnode_t){
		.name = DNS_NAME_INITEMPTY,
	};
	isc_mem_attach(mctx, &node->mctx);
	isc_refcount_init(&node->references, 1);

	dns_name_dupwithoffsets(name, mctx, &node->name);

	return node;
}

static bool
matchbit(unsigned char *bits, uint32_t val) {
	unsigned int len = val / 8 + 2;
	unsigned int mask = 1 << (val % 8);

	if (len <= bits[0] && (bits[len - 1] & mask) != 0) {
		return true;
	}
	return false;
}

isc_result_t
dns_nametree_add(dns_nametree_t *nametree, const dns_name_t *name,
		 uint32_t value) {
	isc_result_t result;
	dns_qp_t *qp = NULL;
	uint32_t size, pos, mask, count = 0;
	dns_ntnode_t *old = NULL, *new = NULL;

	REQUIRE(VALID_NAMETREE(nametree));
	REQUIRE(name != NULL);

	dns_qpmulti_write(nametree->table, &qp);

	switch (nametree->type) {
	case DNS_NAMETREE_BOOL:
		new = newnode(nametree->mctx, name);
		new->set = value;
		break;

	case DNS_NAMETREE_COUNT:
		new = newnode(nametree->mctx, name);
		new->set = true;
		result = dns_qp_deletename(qp, name, (void **)&old, &count);
		if (result == ISC_R_SUCCESS) {
			count += 1;
		}
		break;

	case DNS_NAMETREE_BITS:
		result = dns_qp_getname(qp, name, (void **)&old, NULL);
		if (result == ISC_R_SUCCESS && matchbit(old->bits, value)) {
			goto out;
		}

		size = pos = value / 8 + 2;
		mask = 1 << (value % 8);

		if (old != NULL && old->bits[0] > pos) {
			size = old->bits[0];
		}

		new = newnode(nametree->mctx, name);
		new->bits = isc_mem_cget(nametree->mctx, size, sizeof(char));
		if (result == ISC_R_SUCCESS) {
			memmove(new->bits, old->bits, old->bits[0]);
			result = dns_qp_deletename(qp, name, NULL, NULL);
			INSIST(result == ISC_R_SUCCESS);
		}

		new->bits[pos - 1] |= mask;
		new->bits[0] = size;
		break;
	default:
		UNREACHABLE();
	}

	result = dns_qp_insert(qp, new, count);
	/*
	 * We detach the node here, so any dns_qp_deletename() will
	 * destroy the node directly.
	 */
	dns_ntnode_detach(&new);

out:
	dns_qp_compact(qp, DNS_QPGC_MAYBE);
	dns_qpmulti_commit(nametree->table, &qp);
	return result;
}

isc_result_t
dns_nametree_delete(dns_nametree_t *nametree, const dns_name_t *name) {
	isc_result_t result;
	dns_qp_t *qp = NULL;
	dns_ntnode_t *old = NULL;
	uint32_t count;

	REQUIRE(VALID_NAMETREE(nametree));
	REQUIRE(name != NULL);

	dns_qpmulti_write(nametree->table, &qp);
	result = dns_qp_deletename(qp, name, (void **)&old, &count);
	switch (nametree->type) {
	case DNS_NAMETREE_BOOL:
	case DNS_NAMETREE_BITS:
		break;

	case DNS_NAMETREE_COUNT:
		if (result == ISC_R_SUCCESS && count-- != 0) {
			dns_ntnode_t *new = newnode(nametree->mctx, name);
			new->set = true;
			result = dns_qp_insert(qp, new, count);
			INSIST(result == ISC_R_SUCCESS);
			dns_ntnode_detach(&new);
		}
		break;
	default:
		UNREACHABLE();
	}
	dns_qp_compact(qp, DNS_QPGC_MAYBE);
	dns_qpmulti_commit(nametree->table, &qp);

	return result;
}

isc_result_t
dns_nametree_find(dns_nametree_t *nametree, const dns_name_t *name,
		  dns_ntnode_t **ntnodep) {
	isc_result_t result;
	dns_ntnode_t *node = NULL;
	dns_qpread_t qpr;

	REQUIRE(VALID_NAMETREE(nametree));
	REQUIRE(name != NULL);
	REQUIRE(ntnodep != NULL && *ntnodep == NULL);

	dns_qpmulti_query(nametree->table, &qpr);
	result = dns_qp_getname(&qpr, name, (void **)&node, NULL);
	if (result == ISC_R_SUCCESS) {
		dns_ntnode_attach(node, ntnodep);
	}
	dns_qpread_destroy(nametree->table, &qpr);

	return result;
}

bool
dns_nametree_covered(dns_nametree_t *nametree, const dns_name_t *name,
		     dns_name_t *found, uint32_t bit) {
	isc_result_t result;
	dns_qpread_t qpr;
	dns_ntnode_t *node = NULL;
	bool ret = false;

	REQUIRE(VALID_NAMETREE(nametree));

	dns_qpmulti_query(nametree->table, &qpr);
	result = dns_qp_lookup(&qpr, name, NULL, NULL, NULL, (void **)&node,
			       NULL);
	if (result == ISC_R_SUCCESS || result == DNS_R_PARTIALMATCH) {
		if (found != NULL) {
			dns_name_copy(&node->name, found);
		}
		switch (nametree->type) {
		case DNS_NAMETREE_BOOL:
			ret = node->set;
			break;
		case DNS_NAMETREE_COUNT:
			ret = true;
			break;
		case DNS_NAMETREE_BITS:
			ret = matchbit(node->bits, bit);
			break;
		}
	}

	dns_qpread_destroy(nametree->table, &qpr);
	return ret;
}

static void
qp_attach(void *uctx ISC_ATTR_UNUSED, void *pval,
	  uint32_t ival ISC_ATTR_UNUSED) {
	dns_ntnode_t *ntnode = pval;
	dns_ntnode_ref(ntnode);
}

static void
qp_detach(void *uctx ISC_ATTR_UNUSED, void *pval,
	  uint32_t ival ISC_ATTR_UNUSED) {
	dns_ntnode_t *ntnode = pval;
	dns_ntnode_detach(&ntnode);
}

static size_t
qp_makekey(dns_qpkey_t key, void *uctx ISC_ATTR_UNUSED, void *pval,
	   uint32_t ival ISC_ATTR_UNUSED) {
	dns_ntnode_t *ntnode = pval;
	return dns_qpkey_fromname(key, &ntnode->name);
}

static void
qp_triename(void *uctx, char *buf, size_t size) {
	dns_nametree_t *nametree = uctx;
	snprintf(buf, size, "%s nametree", nametree->name);
}
