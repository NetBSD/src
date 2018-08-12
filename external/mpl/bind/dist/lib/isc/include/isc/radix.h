/*	$NetBSD: radix.h,v 1.2 2018/08/12 13:02:38 christos Exp $	*/

/*
 * Copyright (C) Internet Systems Consortium, Inc. ("ISC")
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * See the COPYRIGHT file distributed with this work for additional
 * information regarding copyright ownership.
 */

#include <isc/magic.h>
#include <isc/types.h>
#include <isc/mutex.h>
#include <isc/net.h>
#include <isc/refcount.h>

#include <string.h>

#ifndef _RADIX_H
#define _RADIX_H

#define NETADDR_TO_PREFIX_T(na,pt,bits,is_ecs)	\
	do { \
		const void *p = na; \
		memset(&(pt), 0, sizeof(pt)); \
		if (p != NULL) { \
			(pt).family = (na)->family; \
			(pt).bitlen = (bits); \
			if ((pt).family == AF_INET6) { \
				memmove(&(pt).add.sin6, &(na)->type.in6, \
				       ((bits)+7)/8); \
			} else \
				memmove(&(pt).add.sin, &(na)->type.in, \
				       ((bits)+7)/8); \
		} else { \
			(pt).family = AF_UNSPEC; \
			(pt).bitlen = 0; \
		} \
		(pt).ecs = is_ecs; \
		isc_refcount_init(&(pt).refcount, 0); \
	} while(/*CONSTCOND*/0)

typedef struct isc_prefix {
	isc_mem_t *mctx;
	unsigned int family;	/* AF_INET | AF_INET6, or AF_UNSPEC for "any" */
	unsigned int bitlen;	/* 0 for "any" */
	isc_boolean_t ecs;	/* ISC_TRUE for an EDNS client subnet address */
	isc_refcount_t refcount;
	union {
		struct in_addr sin;
		struct in6_addr sin6;
	} add;
} isc_prefix_t;

typedef void (*isc_radix_destroyfunc_t)(void *);
typedef void (*isc_radix_processfunc_t)(isc_prefix_t *, void **);

#define isc_prefix_tochar(prefix) ((char *)&(prefix)->add.sin)
#define isc_prefix_touchar(prefix) ((u_char *)&(prefix)->add.sin)

#define BIT_TEST(f, b)  ((f) & (b))

/*
 * We need "first match" when we search the radix tree to preserve
 * compatibility with the existing ACL implementation. Radix trees
 * naturally lend themselves to "best match". In order to get "first match"
 * behavior, we keep track of the order in which entries are added to the
 * tree--and when a search is made, we find all matching entries, and
 * return the one that was added first.
 *
 * An IPv4 prefix and an IPv6 prefix may share a radix tree node if they
 * have the same length and bit pattern (e.g., 127/8 and 7f::/8).  Also,
 * a node that matches a client address may also match an EDNS client
 * subnet address.  To disambiguate between these, node_num and data
 * are four-element arrays;
 *
 *   - node_num[0] and data[0] are used for IPv4 client addresses
 *   - node_num[1] and data[1] for IPv4 client subnet addresses
 *   - node_num[2] and data[2] are used for IPv6 client addresses
 *   - node_num[3] and data[3] for IPv6 client subnet addresses
 *
 * A prefix of 0/0 (aka "any" or "none"), is always stored as IPv4,
 * but matches IPv6 addresses too, as well as all client subnet
 * addresses.
 */

#define RADIX_NOECS 0
#define RADIX_ECS 2
#define RADIX_V4 0
#define RADIX_V6 1
#define RADIX_V4_ECS 2
#define RADIX_V6_ECS 3
#define RADIX_FAMILIES 4

#define ISC_RADIX_FAMILY(p) \
	((((p)->family == AF_INET6) ? RADIX_V6 : RADIX_V4) + \
	 ((p)->ecs ? RADIX_ECS : RADIX_NOECS))

typedef struct isc_radix_node {
	isc_mem_t *mctx;
	isc_uint32_t bit;		/* bit length of the prefix */
	isc_prefix_t *prefix;		/* who we are in radix tree */
	struct isc_radix_node *l, *r;	/* left and right children */
	struct isc_radix_node *parent;	/* may be used */
	void *data[RADIX_FAMILIES];	/* pointers to IPv4 and IPV6 data */
	int node_num[RADIX_FAMILIES];	/* which node this was in the tree,
					   or -1 for glue nodes */
} isc_radix_node_t;

#define RADIX_TREE_MAGIC         ISC_MAGIC('R','d','x','T');
#define RADIX_TREE_VALID(a)      ISC_MAGIC_VALID(a, RADIX_TREE_MAGIC);

typedef struct isc_radix_tree {
	unsigned int magic;
	isc_mem_t *mctx;
	isc_radix_node_t *head;
	isc_uint32_t maxbits;		/* for IP, 32 bit addresses */
	int num_active_node;		/* for debugging purposes */
	int num_added_node;		/* total number of nodes */
} isc_radix_tree_t;

isc_result_t
isc_radix_search(isc_radix_tree_t *radix, isc_radix_node_t **target,
		 isc_prefix_t *prefix);
/*%<
 * Search 'radix' for the best match to 'prefix'.
 * Return the node found in '*target'.
 *
 * Requires:
 * \li	'radix' to be valid.
 * \li	'target' is not NULL and "*target" is NULL.
 * \li	'prefix' to be valid.
 *
 * Returns:
 * \li	ISC_R_NOTFOUND
 * \li	ISC_R_SUCCESS
 */

isc_result_t
isc_radix_insert(isc_radix_tree_t *radix, isc_radix_node_t **target,
		 isc_radix_node_t *source, isc_prefix_t *prefix);
/*%<
 * Insert 'source' or 'prefix' into the radix tree 'radix'.
 * Return the node added in 'target'.
 *
 * Requires:
 * \li	'radix' to be valid.
 * \li	'target' is not NULL and "*target" is NULL.
 * \li	'prefix' to be valid or 'source' to be non NULL and contain
 *	a valid prefix.
 *
 * Returns:
 * \li	ISC_R_NOMEMORY
 * \li	ISC_R_SUCCESS
 */

void
isc_radix_remove(isc_radix_tree_t *radix, isc_radix_node_t *node);
/*%<
 * Remove the node 'node' from the radix tree 'radix'.
 *
 * Requires:
 * \li	'radix' to be valid.
 * \li	'node' to be valid.
 */

isc_result_t
isc_radix_create(isc_mem_t *mctx, isc_radix_tree_t **target, int maxbits);
/*%<
 * Create a radix tree with a maximum depth of 'maxbits';
 *
 * Requires:
 * \li	'mctx' to be valid.
 * \li	'target' to be non NULL and '*target' to be NULL.
 * \li	'maxbits' to be less than or equal to RADIX_MAXBITS.
 *
 * Returns:
 * \li	ISC_R_NOMEMORY
 * \li	ISC_R_SUCCESS
 */

void
isc_radix_destroy(isc_radix_tree_t *radix, isc_radix_destroyfunc_t func);
/*%<
 * Destroy a radix tree optionally calling 'func' to clean up node data.
 *
 * Requires:
 * \li	'radix' to be valid.
 */

void
isc_radix_process(isc_radix_tree_t *radix, isc_radix_processfunc_t func);
/*%<
 * Walk a radix tree calling 'func' to process node data.
 *
 * Requires:
 * \li	'radix' to be valid.
 * \li	'func' to point to a function.
 */

#define RADIX_MAXBITS 128
#define RADIX_NBIT(x)        (0x80 >> ((x) & 0x7f))
#define RADIX_NBYTE(x)       ((x) >> 3)

#define RADIX_WALK(Xhead, Xnode) \
    do { \
	isc_radix_node_t *Xstack[RADIX_MAXBITS+1]; \
	isc_radix_node_t **Xsp = Xstack; \
	isc_radix_node_t *Xrn = (Xhead); \
	while ((Xnode = Xrn)) { \
	    if (Xnode->prefix)

#define RADIX_WALK_END \
	    if (Xrn->l) { \
		if (Xrn->r) { \
		    *Xsp++ = Xrn->r; \
		} \
		Xrn = Xrn->l; \
	    } else if (Xrn->r) { \
		Xrn = Xrn->r; \
	    } else if (Xsp != Xstack) { \
		Xrn = *(--Xsp); \
	    } else { \
		Xrn = (isc_radix_node_t *) 0; \
	    } \
	} \
    } while (/*CONSTCOND*/0)

#endif /* _RADIX_H */
