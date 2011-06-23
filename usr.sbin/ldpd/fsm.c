/* $NetBSD: fsm.c,v 1.3.4.1 2011/06/23 14:20:47 cherry Exp $ */

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
#include <arpa/inet.h>
#include <netinet/in.h>
#include <net/if.h>

#include <ifaddrs.h>
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>

#include "ldp.h"
#include "ldp_peer.h"
#include "socketops.h"
#include "ldp_errors.h"
#include "fsm.h"

char            my_ldp_id[20];
struct sockaddr	mplssockaddr;

/* Processing a hello */
void
run_ldp_hello(struct ldp_pdu * pduid, struct hello_tlv * ht,
    struct in_addr * padd, struct in_addr * ladd, int mysock)
{
	struct ldp_peer *peer = NULL;
	struct in_addr  peer_addr;
	struct transport_address_tlv *trtlv;
	struct hello_info *hi;

	if ((!pduid) || (!ht))
		return;

	debugp("Hello received for address: %s\n", inet_ntoa(*ladd));
	debugp("Hello: Type: 0x%.4X Length: %.2d ID: %.8X\n", ht->type,
	    ht->length, ht->messageid);

	/* Add it to hello list or just update timer */
	SLIST_FOREACH(hi, &hello_info_head, infos)
		if (hi->ldp_id.s_addr == pduid->ldp_id.s_addr)
			break;
	if (hi == NULL) {
		hi = malloc(sizeof(*hi));
		if (!hi) {
			fatalp("Cannot alloc a hello info structure");
			return;
		}
		hi->ldp_id.s_addr = pduid->ldp_id.s_addr;
		SLIST_INSERT_HEAD(&hello_info_head, hi, infos);
	} else
		/* Just update timer */
		hi->keepalive = LDP_HELLO_KEEP;

	if (ht->length <= 4)	/* Common hello parameters */
		return;
	ht->ch.type = ntohs(ht->ch.type);
	ht->ch.length = ntohs(ht->ch.length);
	ht->ch.holdtime = ntohs(ht->ch.holdtime);
	ht->ch.res = ntohs(ht->ch.res);
	debugp("Common hello Type: 0x%.4X Length: %.2d R:%d T:%d"
	    "Hold time: %d\n", ht->ch.type, ht->ch.length,
	    ht->ch.tr / 2, ht->ch.tr % 2, ht->ch.holdtime);
	if (ht->ch.holdtime != 0)
		hi->keepalive = ht->ch.holdtime;
	else {
		if (ht->ch.res >> 15 == 0)
			hi->keepalive = LDP_HELLO_KEEP;
		else
			hi->keepalive = LDP_THELLO_KEEP;
	}
	if (!get_ldp_peer(&pduid->ldp_id)) {
		/* First of all set peer_addr to announced LDP_ID */
		memcpy(&peer_addr, &pduid->ldp_id,
		    sizeof(struct in_addr));
		/*
		 * Now let's see if there is any transport TLV in
		 * there
		 */
		if (pduid->length - PDU_PAYLOAD_LENGTH -
		    sizeof(struct hello_tlv) > 3) {
			trtlv = (struct transport_address_tlv *) &ht[1];
			if (trtlv->type == TLV_IPV4_TRANSPORT)
				memcpy(&peer_addr, &trtlv->address,
				    sizeof(struct in_addr));
		} else
			trtlv = NULL;
		/*
		 * RFC says: If A1 > A2, LSR1 plays the active role;
		 * otherwise it is passive.
		 */
		if (ntohl(peer_addr.s_addr) < ntohl(ladd->s_addr)) {
#define	TRADDR (trtlv && trtlv->type == TLV_IPV4_TRANSPORT) ? &peer_addr : NULL
			peer = ldp_peer_new(&pduid->ldp_id, padd,
				TRADDR, ht->ch.holdtime, 0);
			if (!peer)
				return;
			if (peer && peer->state == LDP_PEER_CONNECTED)
				send_initialize(peer);
		}
	}
}

struct address_list_tlv *
build_address_list_tlv(void)
{
	struct address_list_tlv *t;
	struct ifaddrs *ifa, *ifb;
	struct sockaddr_in *sa;
	struct in_addr *ia;
	uint16_t       adrcount = 0;

	if (getifaddrs(&ifa) == -1)
		return NULL;

	/* Find out the number of addresses */
	/* Ignore loopback */
	for (ifb = ifa; ifb; ifb = ifb->ifa_next)
		if ((ifb->ifa_addr->sa_family == AF_INET) &&
		    (ifb->ifa_flags & IFF_UP)) {
			sa = (struct sockaddr_in *) ifb->ifa_addr;
			if (ntohl(sa->sin_addr.s_addr) >> 24 != IN_LOOPBACKNET)
				adrcount++;
		}
	t = malloc(sizeof(*t) + (adrcount - 1) * sizeof(struct in_addr));

	if (!t) {
		fatalp("build_address_list_tlv: malloc problem\n");
		freeifaddrs(ifa);
		return NULL;
	}

	t->type = htons(LDP_ADDRESS);
	t->length = htons(sizeof(struct address_list_tlv) - TLV_TYPE_LENGTH
			  + (adrcount - 1) * sizeof(struct in_addr));
	t->messageid = htonl(get_message_id());

	t->a_type = htons(TLV_ADDRESS_LIST);
	t->a_length = htons(sizeof(t->a_af) +
	    adrcount * sizeof(struct in_addr));
	t->a_af = htons(LDP_AF_INET);

	ia = &t->a_address;
	for (adrcount = 0, ifb = ifa; ifb; ifb = ifb->ifa_next) {
		if ((ifb->ifa_addr->sa_family != AF_INET) ||
		    (!(ifb->ifa_flags & IFF_UP)) ||
		    (ifb->ifa_flags & IFF_LOOPBACK))
			continue;
		sa = (struct sockaddr_in *) ifb->ifa_addr;
		memcpy(&ia[adrcount], &sa->sin_addr, sizeof(struct in_addr));
		adrcount++;
	}
	freeifaddrs(ifa);

	add_my_if_addrs(ia, adrcount);
	return t;
}

/*
 * Calculate LDP ID
 * Get also mpls pseudo-interface address
 */
int 
set_my_ldp_id()
{
	struct ifaddrs *ifa, *ifb;
	struct in_addr  a;
	struct sockaddr_in *sa;

	a.s_addr = 0;
	my_ldp_id[0] = '\0';
	mplssockaddr.sa_len = 0;

	if (getifaddrs(&ifa) == -1)
		return LDP_E_GENERIC;

	for (ifb = ifa; ifb; ifb = ifb->ifa_next)
		if(ifb->ifa_flags & IFF_UP) {
			if (strncmp("mpls", ifb->ifa_name, 4) == 0 &&
			    ifb->ifa_addr->sa_family == AF_LINK)
				memcpy(&mplssockaddr, ifb->ifa_addr,
				    ifb->ifa_addr->sa_len);
			
			if (ifb->ifa_addr->sa_family != AF_INET)
				continue;

			sa = (struct sockaddr_in *) ifb->ifa_addr;
			if (ntohl(sa->sin_addr.s_addr) >> 24 == IN_LOOPBACKNET)
				continue;	/* No 127/8 */
			if (ntohl(sa->sin_addr.s_addr) > ntohl(a.s_addr))
				a.s_addr = sa->sin_addr.s_addr;
		}
	freeifaddrs(ifa);
	debugp("LDP ID: %s\n", inet_ntoa(a));
	strlcpy(my_ldp_id, inet_ntoa(a), INET_ADDRSTRLEN);
	return LDP_E_OK;
}
