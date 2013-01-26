/* $NetBSD: label.c,v 1.4 2013/01/26 17:29:55 kefren Exp $ */

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
#include <stdlib.h>
#include <string.h>

#include "ldp.h"
#include "tlv_stack.h"
#include "mpls_routes.h"
#include "label.h"
#include "ldp_errors.h"

int	min_label = MIN_LABEL, max_label = MAX_LABEL;

void 
label_init()
{
	SLIST_INIT(&label_head);
}

/*
 * if binding == 0 it receives a free one
 */
struct label   *
label_add(union sockunion * so_dest, union sockunion * so_pref,
	  union sockunion * so_gate, uint32_t binding, struct ldp_peer * p,
	  uint32_t label)
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

	memcpy(&l->so_dest, so_dest, sizeof(union sockunion));
	memcpy(&l->so_pref, so_pref, sizeof(union sockunion));

	if (so_gate)
		memcpy(&l->so_gate, so_gate, sizeof(union sockunion));
	if (binding)
		l->binding = binding;
	else
		l->binding = get_free_local_label();
	l->p = p;
	l->label = label;

	SLIST_INSERT_HEAD(&label_head, l, labels);

	strlcpy(spreftmp, union_ntoa(so_pref), INET_ADDRSTRLEN);
	warnp("[label_add] added binding %d for %s/%s\n", l->binding,
	    union_ntoa(so_dest), spreftmp);

	send_label_tlv_to_all(&(so_dest->sa),
	    from_union_to_cidr(so_pref), l->binding);
	return l;
}

/* Unlink a label */
void 
label_del(struct label * l)
{
	warnp("[label_del] deleted binding %d for %s\n", l->binding,
	   union_ntoa(&l->so_dest));
	SLIST_REMOVE(&label_head, l, label, labels);
	free(l);
}

/*
 * Delete or Reuse the old IPv4 route, delete MPLS route (if any)
 */
void
label_reattach_route(struct label *l, int readd)
{
	union sockunion *u;
	union sockunion emptysu;
	struct rt_msg rg;
	int oldbinding = l->binding;

	warnp("[label_reattach_route] binding %d deleted\n",
		l->binding);

	l->p = NULL;
	l->binding = MPLS_LABEL_IMPLNULL;

	/* No gateway ? */
	memset(&emptysu, 0, sizeof (union sockunion));
	if (memcmp(&l->so_gate, &emptysu, sizeof(union sockunion)) == 0)
		return;

	if (l->label != MPLS_LABEL_IMPLNULL && readd == LDP_READD_CHANGE) {
	/* Delete and re-add IPv4 route */
		if (get_route(&rg, &l->so_dest, &l->so_pref, 1) == LDP_E_OK) {
			delete_route(&l->so_dest, &l->so_pref, NO_FREESO);
			add_route(&l->so_dest, &l->so_pref, &l->so_gate, NULL, NULL,
			    NO_FREESO, RTM_READD);
		} else if (from_union_to_cidr(&l->so_pref) == 32 &&
		    l->so_dest.sa.sa_family == AF_INET &&
		    get_route(&rg, &l->so_dest, NULL, 1) == LDP_E_OK) {
			delete_route(&l->so_dest, NULL, NO_FREESO);
			add_route(&l->so_dest, NULL, &l->so_gate, NULL, NULL,
			    NO_FREESO, RTM_READD);
		} else
			add_route(&l->so_dest, &l->so_pref,
			    &l->so_gate, NULL, NULL, NO_FREESO, RTM_READD);
	} else
		if (readd != LDP_READD_NODEL)
			delete_route(&l->so_dest, &l->so_pref, NO_FREESO);

	l->label = 0;

	/* Deletes pure MPLS route */
	if (oldbinding >= min_label) {
		u = make_mpls_union(oldbinding);
		delete_route(u, NULL, FREESO);
	}
}
/*
 * Get a label by dst and pref
 */
struct label*
label_get(union sockunion *sodest, union sockunion *sopref)
{
	struct label *l;

	SLIST_FOREACH (l, &label_head, labels)
	    if (sodest->sin.sin_addr.s_addr ==
		    l->so_dest.sin.sin_addr.s_addr &&
		sopref->sin.sin_addr.s_addr ==
		    l->so_pref.sin.sin_addr.s_addr)
			return l;
	return NULL;
}

/*
 * Find all labels that points to a peer
 * and reattach them to IPv4
 */
void
label_reattach_all_peer_labels(struct ldp_peer *p, int readd)
{
	struct label   *l;

	SLIST_FOREACH(l, &label_head, labels)
		if (l->p == p)
			label_reattach_route(l, readd);
}

/*
 * Find all labels that points to a peer
 * and delete them
 */
void 
del_all_peer_labels(struct ldp_peer * p, int readd)
{
	struct label   *l, *lnext;

	SLIST_FOREACH_SAFE(l, &label_head, labels, lnext) {
		if(l->p != p)
			continue;
		label_reattach_route(l, readd);
		label_del(l);
		SLIST_REMOVE(&label_head, l, label, labels);
	}
}

/*
 * Finds a label by its binding and deletes it
 */
void 
label_del_by_binding(uint32_t binding, int readd)
{
	struct label   *l;

	SLIST_FOREACH(l, &label_head, labels)
		if ((uint32_t)l->binding == binding) {
			label_reattach_route(l, readd);
			label_del(l);
			SLIST_REMOVE(&label_head, l, label, labels);
			break;
		}
}

/*
 * For Compatibility with old bindinds code
 */
struct label*
label_get_by_prefix(const struct sockaddr *a, int prefixlen)
{
	union sockunion *so_dest, *so_pref;
	struct label *l;

	so_dest = make_inet_union(satos(a));	// XXX: grobian
	so_pref = from_cidr_to_union(prefixlen);

	l = label_get(so_dest, so_pref);

	free(so_dest);
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
		SLIST_FOREACH(l, &label_head, labels)
			if (l->binding == lbl)
				break;
		if (l == NULL)
			return lbl;
	}
	return 0;
}

/*
 * Change local binding
 */
void
change_local_label(struct label *l, uint32_t newbind)
{
	send_withdraw_tlv_to_all(&(l->so_dest.sa),
		from_union_to_cidr(&(l->so_pref)));
	l->binding = newbind;
	send_label_tlv_to_all(&(l->so_dest.sa),
		from_union_to_cidr(&(l->so_pref)),
		l->binding);
}
