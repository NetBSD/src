/* $NetBSD: mpls_interface.c,v 1.6.8.2 2014/08/20 00:05:09 tls Exp $ */

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

#include <arpa/inet.h>
#include <netinet/in.h>
#include <netmpls/mpls.h>
#include <net/route.h>

#include <sys/param.h>

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include "ldp.h"
#include "ldp_peer.h"
#include "ldp_errors.h"
#include "tlv_stack.h"
#include "label.h"
#include "mpls_interface.h"
#include "mpls_routes.h"

extern int no_default_route;

int
mpls_add_label(struct label *lab)
{
	char            p_str[20];
	union sockunion *so_dest, *so_nexthop, *so_tag, so_ifa;
	uint8_t prefixlen;
	uint32_t oldbinding;
	struct peer_map *pm;

	assert(lab != NULL);

	oldbinding = lab->binding;
	prefixlen = from_union_to_cidr(&lab->so_pref);

	strlcpy(p_str, satos(lab->p->address), sizeof(p_str));
	warnp("Trying to add %s/%d as label %d (oldlocal %d) to peer %s\n",
	    satos(&lab->so_dest.sa), prefixlen, lab->label, lab->binding,p_str);

	/* Check if we should accept default route */
	if (prefixlen == 0 && no_default_route != 0)
		return LDP_E_BAD_AF;

	/* double check if there is a label mapping for this */
	if ((pm = ldp_test_mapping(&lab->so_dest.sa, prefixlen,
	    &lab->so_gate.sa)) == NULL || pm->peer != lab->p) {
		if (pm != NULL)
			free(pm);
		return LDP_E_NOENT;
	}
	free(pm);

	if (lab->so_gate.sa.sa_family != AF_INET &&
	    lab->so_gate.sa.sa_family != AF_INET6) {
		warnp("mpls_add_label: so_gate is not IP or IPv6\n");
		return LDP_E_BAD_AF;
	}

	/* Check if the address is bounded to the peer */
	if (check_ifaddr(lab->p, &lab->so_gate.sa) == NULL) {
		warnp("Failed at next-hop check\n");
		return LDP_E_ROUTE_ERROR;
	}

	/* if binding is implicit null we need to generate a new one */
	if (lab->binding == MPLS_LABEL_IMPLNULL) {
		lab->binding = get_free_local_label();
		if (!lab->binding) {
			fatalp("Label pool depleted\n");
			return LDP_E_TOO_MANY_LABELS;
		}
		announce_label_change(lab);
	}

	warnp("[mpls_add_label] Adding %s/%d as local binding %d (%d), label %d"
	    " to peer %s\n", satos(&lab->so_dest.sa), prefixlen, lab->binding,
	    oldbinding, lab->label, p_str);

	/* Add switching route */
	if ((so_dest = make_mpls_union(lab->binding)) == NULL)
		return LDP_E_MEMORY;
	if ((so_tag = make_mpls_union(lab->label)) == NULL) {
		free(so_dest);
		fatalp("Out of memory\n");
		return LDP_E_MEMORY;
	}
	if ((so_nexthop = malloc(lab->so_gate.sa.sa_len)) == NULL) {
		free(so_dest);
		free(so_tag);
		fatalp("Out of memory\n");
		return LDP_E_MEMORY;
	}
	memcpy(so_nexthop, &lab->so_gate, lab->so_gate.sa.sa_len);

	if (add_route(so_dest, NULL, so_nexthop, NULL, so_tag, FREESO,
	    oldbinding == MPLS_LABEL_IMPLNULL ? RTM_ADD : RTM_CHANGE)
	    != LDP_E_OK) {
		fatalp("[mpls_add_label] MPLS route error\n");
		return LDP_E_ROUTE_ERROR;
	}

	/* Now, let's add tag to IP route and point it to mpls interface */

	if (getsockname(lab->p->socket, &so_ifa.sa,
	    & (socklen_t) { sizeof(union sockunion) } )) {
		fatalp("[mpls_add_label]: getsockname\n");
                return LDP_E_ROUTE_ERROR;
        }

	if ((so_tag = make_mpls_union(lab->label)) == NULL) {
		fatalp("Out of memory\n");
		return LDP_E_MEMORY;
	}

	if (add_route(&lab->so_dest, lab->host ? NULL : &lab->so_pref,
	    &lab->so_gate, &so_ifa, so_tag, NO_FREESO, RTM_CHANGE) != LDP_E_OK){
		free(so_tag);
		fatalp("[mpls_add_label]: INET route failure\n");
		return LDP_E_ROUTE_ERROR;
	}
	free(so_tag);

	warnp("[mpls_add_label]: SUCCESS\n");

	return LDP_E_OK;
}

int 
mpls_add_ldp_peer(const struct ldp_peer * p)
{
	return LDP_E_OK;
}

int 
mpls_delete_ldp_peer(const struct ldp_peer * p)
{

	/* Reput all the routes also to IPv4 */
	label_reattach_all_peer_labels(p, REATT_INET_CHANGE);

	return LDP_E_OK;
}

int 
mpls_start_ldp()
{
	ldp_peer_init();
	label_init();

	return LDP_E_OK;
}
