/* $NetBSD: mpls_interface.c,v 1.6.8.1 2013/02/25 00:30:43 tls Exp $ */

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
mpls_add_label(struct ldp_peer * p, struct rt_msg * inh_rg,
    struct sockaddr * addr, int len, int label, int rlookup)
{
	char            padd[20];
	int             kount = 0, rv;
	union sockunion *so_dest, *so_pref = NULL, *so_gate, *so_nexthop,
		*so_tag, *so_oldifa = NULL, *so_ifa;
	struct rt_msg   rg;
	struct rt_msg	*rgp = &rg;
	struct label	*lab;

	strlcpy(padd, satos(p->address), 20);
	debugp("Trying to add %s/%d as label %d to peer %s\n", satos(addr),
		len, label, padd);

	/* Check if we should accept default route */
	if (!len && no_default_route != 0)
		return LDP_E_BAD_AF;

	/* Is there a label mapping for this ? */
	if (ldp_peer_get_lm(p, addr, len) == NULL)
		return LDP_E_NOENT;

	if (!inh_rg || (inh_rg->m_rtm.rtm_addrs & RTA_IFA) == 0) {
		/*
		 * XXX: Check if we have a route for that.
		 * Why the hell isn't kernel inserting the route immediatly ?
		 * let's loop until we have it..
		 */

		if ((so_dest = make_inet_union(satos(addr))) == NULL) // XXX
			return LDP_E_MEMORY;
		if (len != 32 && (so_pref = from_cidr_to_union(len)) == NULL) {
			free(so_dest);
			return LDP_E_MEMORY;
		}
		do {
			if (kount == rlookup) {
				debugp("No route for this prefix\n");
				return LDP_E_NO_SUCH_ROUTE;
			}
			if (kount > 0)
				debugp("Route test hit: %d\n", kount);
			kount++;
			/* Last time give it a higher chance */
			if (kount == rlookup)
				usleep(5000);

			rv = get_route(rgp, so_dest, so_pref, 1);
			if (rv != LDP_E_OK && len == 32)
				/* Host maybe ? */
				rv = get_route(rgp, so_dest, NULL, 1);
		} while (rv != LDP_E_OK);

		free(so_dest);
		if (so_pref)
			free(so_pref);

	} else
		rgp = inh_rg;

	/* Check if it's an IPv4 route */

	so_gate = (union sockunion *) rgp->m_space;
	so_gate = (union sockunion *)((char*)so_gate +
	    RT_ROUNDUP(so_gate->sa.sa_len));
	if (rgp->m_rtm.rtm_addrs & RTA_IFA) {
		int li = 1;
		so_oldifa = so_gate;
		if (rgp->m_rtm.rtm_addrs & RTA_NETMASK)
			li++;
		if (rgp->m_rtm.rtm_addrs & RTA_GENMASK)
			li++;
		if (rgp->m_rtm.rtm_addrs & RTA_IFP)
			li++;
		for (int i = 0; i < li; i++)
			so_oldifa = (union sockunion *)((char*)so_oldifa +
			    RT_ROUNDUP(so_oldifa->sa.sa_len));
	}

	if (so_gate->sa.sa_family != AF_INET) {
		debugp("Failed at family check - only INET supoprted for now\n");
		return LDP_E_BAD_AF;
	}

	/* Check if the address is bounded to the peer */
	if (check_ifaddr(p, &so_gate->sa) == NULL) {
		debugp("Failed at next-hop check\n");
		return LDP_E_ROUTE_ERROR;
	}

	/* Verify if we have a binding for this prefix */
	lab = label_get_by_prefix(addr, len);

	/* And we should have one because we have a route for it */
	assert (lab);

	if (lab->binding == MPLS_LABEL_IMPLNULL) {
		change_local_label(lab, get_free_local_label());
		if (!lab->binding) {
			fatalp("No more free labels !!!\n");
			return LDP_E_TOO_MANY_LABELS;
		}
	}

	warnp("[mpls_add_label] Adding %s/%d as local binding %d, label %d"
	    " to peer %s\n",
		satos(addr), len, lab->binding, label, padd);

	/* Modify existing label */
	lab->label = label;
	lab->p = p;

	/* Add switching route */
	so_dest = make_mpls_union(lab->binding);
	so_nexthop = malloc(sizeof(*so_nexthop));
	if (!so_nexthop) {
		free(so_dest);
		fatalp("Out of memory\n");
		return LDP_E_MEMORY;
	}
	memcpy(so_nexthop, so_gate, so_gate->sa.sa_len);
	if ((so_tag = make_mpls_union(label)) == NULL) {
		free(so_dest);
		free(so_nexthop);
		fatalp("Out of memory\n");
		return LDP_E_MEMORY;
	}
	if (add_route(so_dest, NULL, so_nexthop, NULL, so_tag, FREESO, RTM_ADD) != LDP_E_OK)
		return LDP_E_ROUTE_ERROR;

	/* Now, let's add tag to IPv4 route and point it to mpls interface */
	if ((so_dest = make_inet_union(satos(addr))) == NULL) {	// XXX: grobian
		fatalp("Out of memory\n");
		return LDP_E_MEMORY;
	}

	/* if prefixlen == 32 check if it's inserted as host
 	* and treat it as host. It may also be set as /32 prefix
 	* (thanks mlelstv for heads-up about this)
 	*/
	if ((len == 32) && (rgp->m_rtm.rtm_flags & RTF_HOST))
		so_pref = NULL;
	else if ((so_pref = from_cidr_to_union(len)) == NULL) {
		free(so_dest);
		fatalp("Out of memory\n");
		return LDP_E_MEMORY;
	}

	/* Add tag to route */
	so_nexthop = malloc(sizeof(*so_nexthop));
	if (!so_nexthop) {
		free(so_dest);
		if (so_pref != NULL)
			free(so_pref);
		fatalp("Out of memory\n");
		return LDP_E_MEMORY;
	}
	memcpy(so_nexthop, so_gate, so_gate->sa.sa_len);
	so_tag = make_mpls_union(label);
	if (so_oldifa != NULL) {
		so_ifa = malloc(sizeof(*so_ifa));
		if (so_ifa == NULL) {
			free(so_dest);
			if (so_pref != NULL)
				free(so_pref);
			free(so_tag);
			free(so_nexthop);
			fatalp("Out of memory\n");
			return LDP_E_MEMORY;
		}
		memcpy(so_ifa, so_oldifa, so_oldifa->sa.sa_len);
	} else
		so_ifa = NULL;
	if (add_route(so_dest, so_pref, so_nexthop, so_ifa, so_tag, FREESO, RTM_CHANGE) != LDP_E_OK)
		return LDP_E_ROUTE_ERROR;

	debugp("Added %s/%d as label %d to peer %s\n", satos(addr), len,
	    label, padd);

	return LDP_E_OK;
}

int 
mpls_add_ldp_peer(struct ldp_peer * p)
{
	return LDP_E_OK;
}

int 
mpls_delete_ldp_peer(struct ldp_peer * p)
{

	/* Reput all the routes also to IPv4 */
	label_reattach_all_peer_labels(p, LDP_READD_CHANGE);

	return LDP_E_OK;
}

int 
mpls_start_ldp()
{
	ldp_peer_init();
	label_init();

	return LDP_E_OK;
}
