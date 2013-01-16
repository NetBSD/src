/* $NetBSD: ldp_peer.c,v 1.3.6.1 2013/01/16 05:34:09 yamt Exp $ */

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

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <netmpls/mpls.h>
#include <arpa/inet.h>

#include <stdlib.h>
#include <strings.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>

#include "conffile.h"
#include "socketops.h"
#include "ldp_errors.h"
#include "ldp.h"
#include "tlv_stack.h"
#include "mpls_interface.h"
#include "notifications.h"
#include "ldp_peer.h"

extern int ldp_holddown_time;

struct in_addr *myaddresses;

void 
ldp_peer_init(void)
{
	SLIST_INIT(&ldp_peer_head);
	myaddresses = NULL;
}

/*
 * soc should be > 1 if there is already a TCP socket for this else we'll
 * initiate a new one
 */
struct ldp_peer *
ldp_peer_new(struct in_addr * ldp_id, struct in_addr * a,
	     struct in_addr * tradd, struct in6_addr * tradd6,
	     uint16_t holdtime, int soc)
{
	struct ldp_peer *p;
	int s = soc;
	struct sockaddr_in sa;
	struct conf_neighbour *cn;

	if (s < 1) {
		s = socket(PF_INET, SOCK_STREAM, 0);
		memset(&sa, 0, sizeof(sa));
		sa.sin_len = sizeof(sa);
		sa.sin_family = AF_INET;

		if (tradd)
			memcpy(&sa.sin_addr, tradd,
			    sizeof(struct in_addr));
		else
			memcpy(&sa.sin_addr, a,
			    sizeof(struct in_addr));
		sa.sin_port = htons(LDP_PORT);

		set_ttl(s);
	}

	/* MD5 authentication needed ? */
	SLIST_FOREACH(cn, &conei_head, neilist)
		if (cn->authenticate != 0 && (a->s_addr == cn->address.s_addr ||
		    (tradd && tradd->s_addr == cn->address.s_addr))) {
			if (setsockopt(s, IPPROTO_TCP, TCP_MD5SIG, &(int){1},
			    sizeof(int)) != 0)
				fatalp("setsockopt TCP_MD5SIG: %s\n",
				    strerror(errno));
			break;
		}

	/* Set the peer in CONNECTING/CONNECTED state */
	p = calloc(1, sizeof(*p));

	if (!p) {
		fatalp("ldp_peer_new: calloc problem\n");
		return NULL;
	}

	SLIST_INSERT_HEAD(&ldp_peer_head, p, peers);
	memcpy(&p->address, a, sizeof(struct in_addr));
	memcpy(&p->ldp_id, ldp_id, sizeof(struct in_addr));
	if (tradd)
		memcpy(&p->transport_address, tradd,
		    sizeof(struct in_addr));
	else
		memcpy(&p->transport_address, a,
		    sizeof(struct in_addr));
	p->holdtime = holdtime > ldp_holddown_time ? holdtime : ldp_holddown_time;
	p->socket = s;
	if (soc < 1) {
		p->state = LDP_PEER_CONNECTING;
		p->master = 1;
	} else {
		p->state = LDP_PEER_CONNECTED;
		p->master = 0;
		set_ttl(p->socket);
	}
	SLIST_INIT(&p->ldp_peer_address_head);
	SLIST_INIT(&p->label_mapping_head);
	p->timeout = p->holdtime;

	/* And connect to peer */
	if (soc < 1)
		if (connect(s, (struct sockaddr *) & sa, sizeof(sa)) == -1) {
			if (errno == EINTR) {
				return p;	/* We take care of this in
						 * big_loop */
			}
			warnp("connect to %s failed: %s\n",
			    inet_ntoa(sa.sin_addr), strerror(errno));
			ldp_peer_holddown(p);
			return NULL;
		}
	p->state = LDP_PEER_CONNECTED;
	return p;
}

void 
ldp_peer_holddown(struct ldp_peer * p)
{
	if (!p)
		return;
	if (p->state == LDP_PEER_ESTABLISHED)
		mpls_delete_ldp_peer(p);
	p->state = LDP_PEER_HOLDDOWN;
	p->timeout = ldp_holddown_time;
	shutdown(p->socket, SHUT_RDWR);
	ldp_peer_delete_all_mappings(p);
	del_all_ifaddr(p);
	fatalp("LDP Neighbour %s is DOWN\n", inet_ntoa(p->ldp_id));
}

void
ldp_peer_holddown_all()
{
	struct ldp_peer *p;

	SLIST_FOREACH(p, &ldp_peer_head, peers) {
		if ((p->state == LDP_PEER_ESTABLISHED) ||
		    (p->state == LDP_PEER_CONNECTED))
			send_notification(p, get_message_id(), NOTIF_SHUTDOWN);
		ldp_peer_holddown(p);
	}
}

void 
ldp_peer_delete(struct ldp_peer * p)
{

	if (!p)
		return;

	SLIST_REMOVE(&ldp_peer_head, p, ldp_peer, peers);
	close(p->socket);
	warnp("LDP Neighbor %s holddown timer expired\n", inet_ntoa(p->ldp_id));
	free(p);
}

struct ldp_peer *
get_ldp_peer(struct in_addr * a)
{
	struct ldp_peer *p;

	SLIST_FOREACH(p, &ldp_peer_head, peers) {
		if (!memcmp((void *) a, (void *) &p->ldp_id,
		    sizeof(struct in_addr)))
			return p;
		if (!memcmp((void *) a, (void *) &p->address,
		    sizeof(struct in_addr)))
			return p;
		if (check_ifaddr(p, a))
			return p;
	}
	return NULL;
}

struct ldp_peer *
get_ldp_peer_by_socket(int s)
{
	struct ldp_peer *p;

	SLIST_FOREACH(p, &ldp_peer_head, peers)
		if (p->socket == s)
			return p;
	return NULL;
}

/*
 * Adds address list bounded to a specific peer
 * Returns the number of addresses inserted successfuly
 */
int 
add_ifaddresses(struct ldp_peer * p, struct al_tlv * a)
{
	int             i, c, n;
	struct in_addr *ia;

	/*
	 * Check if tlv is Address type, if it's correct size (at least one
	 * address) and if it's IPv4
	 */

	if ((ntohs(a->type) != TLV_ADDRESS_LIST) ||
	    (ntohs(a->length) < sizeof(a->af) + sizeof(struct in_addr)) ||
	    (ntohs(a->af) != LDP_AF_INET))
		return 0;

	/* Number of addresses to insert */
	n = (ntohs(a->length) - sizeof(a->af)) / sizeof(struct in_addr);

	debugp("Trying to add %d addresses to peer %s ... \n", n,
	    inet_ntoa(p->ldp_id));

	for (ia = (struct in_addr *) & a->address, c = 0, i = 0; i < n; i++) {
		if (add_ifaddr(p, &ia[i]) == LDP_E_OK)
			c++;
	}

	debugp("Added %d addresses\n", c);

	return c;
}

int 
del_ifaddresses(struct ldp_peer * p, struct al_tlv * a)
{
	int             i, c, n;
	struct in_addr *ia;

	/*
	 * Check if tlv is Address type, if it's correct size (at least one
	 * address) and if it's IPv4
	 */

	if (ntohs(a->type) != TLV_ADDRESS_LIST ||
	    ntohs(a->length) > sizeof(a->af) + sizeof(struct in_addr) ||
	    ntohs(a->af) != LDP_AF_INET)
		return -1;

	n = (ntohs(a->length) - sizeof(a->af)) / sizeof(struct in_addr);

	debugp("Trying to delete %d addresses from peer %s ... \n", n,
	    inet_ntoa(p->ldp_id));

	for (ia = (struct in_addr *) & a[1], c = 0, i = 0; i < n; i++) {
		if (del_ifaddr(p, &ia[i]) == LDP_E_OK)
			c++;
	}

	debugp("Deleted %d addresses\n", c);

	return c;
}


/* Adds a _SINGLE_ address to a specific peer */
int 
add_ifaddr(struct ldp_peer * p, struct in_addr * a)
{
	struct ldp_peer_address *lpa;

	/* Is it already there ? */
	if (check_ifaddr(p, a))
		return LDP_E_ALREADY_DONE;

	lpa = calloc(1, sizeof(*lpa));

	if (!lpa) {
		fatalp("add_ifaddr: malloc problem\n");
		return LDP_E_MEMORY;
	}

	memcpy(&lpa->address, a, sizeof(struct in_addr));

	SLIST_INSERT_HEAD(&p->ldp_peer_address_head, lpa, addresses);
	return LDP_E_OK;
}

/* Deletes an address bounded to a specific peer */
int 
del_ifaddr(struct ldp_peer * p, struct in_addr * a)
{
	struct ldp_peer_address *wp;

	wp = check_ifaddr(p, a);
	if (!wp)
		return LDP_E_NOENT;

	SLIST_REMOVE(&p->ldp_peer_address_head, wp, ldp_peer_address,
	    addresses);
	free(wp);
	return LDP_E_OK;
}

/* Checks if an address is already bounded */
struct ldp_peer_address *
check_ifaddr(struct ldp_peer * p, struct in_addr * a)
{
	struct ldp_peer_address *wp;

	SLIST_FOREACH(wp, &p->ldp_peer_address_head, addresses)
		if (memcmp(a, &wp->address, sizeof(struct in_addr)) == 0)
			return wp;
	return NULL;
}

void 
del_all_ifaddr(struct ldp_peer * p)
{
	struct ldp_peer_address *wp;

	while (!SLIST_EMPTY(&p->ldp_peer_address_head)) {
		wp = SLIST_FIRST(&p->ldp_peer_address_head);
		SLIST_REMOVE_HEAD(&p->ldp_peer_address_head, addresses);
		free(wp);
	}
}

void 
print_bounded_addresses(struct ldp_peer * p)
{
	struct ldp_peer_address *wp;
	char abuf[512];

	snprintf(abuf, sizeof(abuf), "Addresses bounded to peer %s: ",
		inet_ntoa(p->address));
	SLIST_FOREACH(wp, &p->ldp_peer_address_head, addresses) {
		strncat(abuf, inet_ntoa(wp->address), sizeof(abuf) -1);
		strncat(abuf, " ", sizeof(abuf) -1);
	}
	warnp("%s\n", abuf);
}

void 
add_my_if_addrs(struct in_addr * a, int count)
{
	myaddresses = calloc((count + 1), sizeof(*myaddresses));

	if (!myaddresses) {
		fatalp("add_my_if_addrs: malloc problem\n");
		return;
	}
	memcpy(myaddresses, a, count * sizeof(struct in_addr));
	myaddresses[count].s_addr = 0;
}

/* Adds a label and a prefix to a specific peer */
int 
ldp_peer_add_mapping(struct ldp_peer * p, struct in_addr * a, int prefix,
    int label)
{
	struct label_mapping *lma;

	if (!p)
		return -1;
	if (ldp_peer_get_lm(p, a, prefix))
		return LDP_E_ALREADY_DONE;

	lma = malloc(sizeof(*lma));

	if (!lma) {
		fatalp("ldp_peer_add_mapping: malloc problem\n");
		return LDP_E_MEMORY;
	}

	memcpy(&lma->address, a, sizeof(struct in_addr));
	lma->prefix = prefix;
	lma->label = label;

	SLIST_INSERT_HEAD(&p->label_mapping_head, lma, mappings);

	return LDP_E_OK;
}

int 
ldp_peer_delete_mapping(struct ldp_peer * p, struct in_addr * a, int prefix)
{
	struct label_mapping *lma;

	if (!a)
		return ldp_peer_delete_all_mappings(p);

	lma = ldp_peer_get_lm(p, a, prefix);
	if (!lma)
		return LDP_E_NOENT;

	SLIST_REMOVE(&p->label_mapping_head, lma, label_mapping, mappings);
	free(lma);

	return LDP_E_OK;
}

struct label_mapping *
ldp_peer_get_lm(struct ldp_peer * p, struct in_addr * a, int prefix)
{
	struct label_mapping *rv;

	if (!p)
		return NULL;

	SLIST_FOREACH(rv, &p->label_mapping_head, mappings)
		if ((rv->prefix == prefix) && (!memcmp(a, &rv->address,
		    sizeof(struct in_addr))))
			break;

	return rv;

}

int 
ldp_peer_delete_all_mappings(struct ldp_peer * p)
{
	struct label_mapping *lma;

	while(!SLIST_EMPTY(&p->label_mapping_head)) {
		lma = SLIST_FIRST(&p->label_mapping_head);
		SLIST_REMOVE_HEAD(&p->label_mapping_head, mappings);
		free(lma);
	}

	return LDP_E_OK;
}

/* returns a mapping and its peer */
struct peer_map *
ldp_test_mapping(struct in_addr * a, int prefix, struct in_addr * gate)
{
	struct ldp_peer *lpeer;
	struct peer_map *rv = NULL;
	struct label_mapping *lm = NULL;

	/* Checks if it's LPDID, else checks if it's an interface */

	lpeer = get_ldp_peer(gate);
	if (!lpeer) {
		debugp("Gateway %s is not an LDP peer\n", inet_ntoa(*gate));
		return NULL;
	}
	if (lpeer->state != LDP_PEER_ESTABLISHED) {
		warnp("ldp_test_mapping: peer is down ?!\n");
		return NULL;
	}
	lm = ldp_peer_get_lm(lpeer, a, prefix);

	if (!lm) {
		debugp("Cannot match that prefix to the specified peer\n");
		return NULL;
	}
	rv = malloc(sizeof(*rv));

	if (!rv) {
		fatalp("ldp_test_mapping: malloc problem\n");
		return NULL;
	}

	rv->lm = lm;
	rv->peer = lpeer;

	return rv;
}

/* Name from state */
const char * ldp_state_to_name(int state)
{
	switch(state) {
		case LDP_PEER_CONNECTING:
			return "CONNECTING";
		case LDP_PEER_CONNECTED:
			return "CONNECTED";
		case LDP_PEER_ESTABLISHED:
			return "ESTABLISHED";
		case LDP_PEER_HOLDDOWN:
			return "HOLDDOWN";
	}
	return "UNKNOWN";
}
