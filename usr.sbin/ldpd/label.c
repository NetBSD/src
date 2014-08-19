/* $NetBSD: label.c,v 1.3.12.2 2014/08/20 00:05:09 tls Exp $ */

/*-
 * Copyright (c) 2010 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Mihai Chelaru <kefren@NetBSD.org>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <netmpls/mpls.h>

#include <assert.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "ldp.h"
#include "tlv_stack.h"
#include "mpls_routes.h"
#include "label.h"
#include "ldp_errors.h"

static int labels_compare(void*, const void*, const void*);

int min_label = MIN_LABEL, max_label = MAX_LABEL;

static rb_tree_t labels_tree;
static const rb_tree_ops_t tree_ops = {
	.rbto_compare_nodes = labels_compare,
	.rbto_compare_key = labels_compare,
	.rbto_node_offset = offsetof(struct label, lbtree),
	.rbto_context = NULL
};

void 
label_init()
{

	rb_tree_init(&labels_tree, &tree_ops);
}

static int
labels_compare(void *context, const void *node1, const void *node2)
{
	const struct label *l1 = node1, *l2 = node2;
	int ret;

	if (__predict_false(l1->so_dest.sa.sa_family !=
	    l2->so_dest.sa.sa_family))
		return l1->so_dest.sa.sa_family > l2->so_dest.sa.sa_family ?
		    1 : -1;

	assert(l1->so_dest.sa.sa_len == l2->so_dest.sa.sa_len);
	assert(l1->so_pref.sa.sa_len == l2->so_pref.sa.sa_len);

	if ((ret = memcmp(&l1->so_dest.sa, &l2->so_dest.sa,
	    l1->so_dest.sa.sa_len)) != 0)
		return ret;
	else
		return memcmp(&l1->so_pref.sa, &l2->so_pref.sa,
		    l1->so_pref.sa.sa_len);
}

/*
 * if binding == 0 it receives a free one
 */
struct label   *
label_add(const union sockunion * so_dest, const union sockunion * so_pref,
	  const union sockunion * so_gate, uint32_t binding,
	  const struct ldp_peer * p, uint32_t label, bool host)
{
	struct label   *l;
	char	spreftmp[INET_ADDRSTRLEN];

	l = calloc(1, sizeof(*l));

	if (!l) {
		fatalp("label_add: malloc problem\n");
		return NULL;
	}

	assert(so_dest);
	assert(so_pref);
	assert(so_dest->sa.sa_family == so_pref->sa.sa_family);
	assert(label_get(so_dest, so_pref) == NULL);

	memcpy(&l->so_dest, so_dest, so_dest->sa.sa_len);
	memcpy(&l->so_pref, so_pref, so_pref->sa.sa_len);

	if (so_gate)
		memcpy(&l->so_gate, so_gate, so_gate->sa.sa_len);
	if (binding)
		l->binding = binding;
	else
		l->binding = get_free_local_label();
	l->p = p;
	l->label = label;
	l->host = host;

	if (rb_tree_insert_node(&labels_tree, l) != l)
		fatalp("label already in tree");

	strlcpy(spreftmp, satos(&so_pref->sa), INET_ADDRSTRLEN);
	warnp("[label_add] added binding %d for %s/%s\n", l->binding,
	    satos(&so_dest->sa), spreftmp);

	send_label_tlv_to_all(&(so_dest->sa),
	    from_union_to_cidr(so_pref), l->binding);
	return l;
}

/* Unlink a label */
void 
label_del(struct label * l)
{
	warnp("[label_del] deleted binding %d for %s\n", l->binding,
	   satos(&l->so_dest.sa));
	rb_tree_remove_node(&labels_tree, l);
	free(l);
}

/*
 * Delete or Reuse the old IPv4 route, delete MPLS route
 * readd = REATT_INET_CHANGE -> delete and recreate the INET route
 * readd = REATT_INET_DEL -> deletes INET route
 * readd = REATT_INET_NODEL -> doesn't touch the INET route
 */
void
label_reattach_route(struct label *l, int readd)
{

	warnp("[label_reattach_route] binding %d deleted\n",
		l->binding);

	/* No gateway ? */
	if (l->so_gate.sa.sa_len == 0)
		return;

	if (readd == REATT_INET_CHANGE) {
		/* Delete the tagged route and re-add IPv4 route */
		delete_route(&l->so_dest,
		    l->host ? NULL : &l->so_pref, NO_FREESO);
		add_route(&l->so_dest,
		    l->host ? NULL : &l->so_pref, &l->so_gate,
		    NULL, NULL, NO_FREESO, RTM_READD);
	} else if (readd == REATT_INET_DEL)
		delete_route(&l->so_dest, l->host ? NULL : &l->so_pref,
		    NO_FREESO);

	/* Deletes the MPLS route */
	if (l->binding >= min_label)
		delete_route(make_mpls_union(l->binding), NULL, FREESO);

	l->binding = MPLS_LABEL_IMPLNULL;
	l->p = NULL;
	l->label = 0;
}
/*
 * Get a label by dst and pref
 */
struct label*
label_get(const union sockunion *sodest, const union sockunion *sopref)
{
	struct label l;

	memset(&l, 0, sizeof(l));
	memcpy(&l.so_dest, sodest, sodest->sa.sa_len);
	memcpy(&l.so_pref, sopref, sopref->sa.sa_len);

	return rb_tree_find_node(&labels_tree, &l);
}

/*
 * Find all labels that points to a peer
 * and reattach them to IPv4
 */
void
label_reattach_all_peer_labels(const struct ldp_peer *p, int readd)
{
	struct label   *l;

	RB_TREE_FOREACH(l, &labels_tree)
		if (l->p == p)
			label_reattach_route(l, readd);
}

/*
 * Find all labels that points to a peer
 * and delete them
 */
void 
del_all_peer_labels(const struct ldp_peer * p, int readd)
{
	struct label   *l, *lnext;

	RB_TREE_FOREACH(l, &labels_tree) {
back_delete:
		if(l->p != p)
			continue;
		label_reattach_route(l, readd);
		lnext = rb_tree_iterate(&labels_tree, l, RB_DIR_RIGHT);
		label_del(l);
		if (lnext == NULL)
			break;
		l = lnext;
		goto back_delete;
	}
}

/*
 * Finds a label by its binding and deletes it
 */
void 
label_del_by_binding(uint32_t binding, int readd)
{
	struct label   *l;

	RB_TREE_FOREACH(l, &labels_tree)
		if ((uint32_t)l->binding == binding) {
			label_reattach_route(l, readd);
			label_del(l);
			break;
		}
}

/*
 * For Compatibility with old bindinds code
 */
struct label*
label_get_by_prefix(const struct sockaddr *a, int prefixlen)
{
	const union sockunion *so_dest;
	union sockunion *so_pref;
	struct label *l;

	so_dest = (const union sockunion *)a;
	so_pref = from_cidr_to_union(prefixlen);

	l = label_get(so_dest, so_pref);

	free(so_pref);

	return l;
}

/*
 * Get a free binding
 */
uint32_t
get_free_local_label()
{
	struct label *l;
	int lbl;
 
	for (lbl = min_label; lbl <= max_label; lbl++) {
		RB_TREE_FOREACH(l, &labels_tree)
			if (l->binding == lbl)
				break;
		if (l == NULL)
			return lbl;
	}
	return 0;
}

/*
 * Announce peers that a label has changed its binding
 * by withdrawing it and reannouncing it
 */
void
announce_label_change(struct label *l)
{
	send_withdraw_tlv_to_all(&(l->so_dest.sa),
		from_union_to_cidr(&(l->so_pref)));
	send_label_tlv_to_all(&(l->so_dest.sa),
		from_union_to_cidr(&(l->so_pref)),
		l->binding);
}

void
label_check_assoc(struct ldp_peer *p)
{
	struct label *l;
	struct ldp_peer_address *wp;

	RB_TREE_FOREACH(l, &labels_tree)
		if (l->p == NULL && l->so_gate.sa.sa_family != 0)
			SLIST_FOREACH(wp, &p->ldp_peer_address_head, addresses)
				if (sockaddr_cmp(&l->so_gate.sa,
				    &wp->address.sa) == 0) {
					l->p = p;
					l->label = MPLS_LABEL_IMPLNULL;
					break;
				}
}

struct label *
label_get_right(struct label *l)
{
	if (l == NULL)
		return RB_TREE_MIN(&labels_tree);
	else
		return rb_tree_iterate(&labels_tree, l, RB_DIR_RIGHT);
}
