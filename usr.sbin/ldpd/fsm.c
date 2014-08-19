/* $NetBSD: fsm.c,v 1.5.8.3 2014/08/20 00:05:09 tls Exp $ */

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

/* Process a hello */
void
run_ldp_hello(const struct ldp_pdu * pduid, const struct hello_tlv * ht,
    const struct sockaddr * padd, const struct in_addr * ladd, int mysock,
    bool may_connect)
{
	struct ldp_peer *peer = NULL;
	const struct transport_address_tlv *trtlv;
	struct hello_info *hi = NULL;
	union sockunion traddr;

	if ((!pduid) || (!ht))
		return;

	debugp("Hello received for address: %s\n", inet_ntoa(*ladd));
	debugp("Hello: Type: 0x%.4X Length: %.2d ID: %.8X\n", ht->type,
	    ht->length, ht->messageid);

	if (ht->length <= 4)	/* Common hello parameters */
		return;
	debugp("Common hello Type: 0x%.4X Length: %.2d"
	    " Hold time: %d\n", ntohs(ht->ch.type), ntohs(ht->ch.length),
	    ht->ch.holdtime);

	memset(&traddr, 0, sizeof(traddr));
	/* Check transport TLV */
	if (pduid->length - PDU_PAYLOAD_LENGTH -
	    sizeof(struct hello_tlv) >= 8) {
		trtlv = (const struct transport_address_tlv *)(ht + 1);
		if (trtlv->type == htons(TLV_IPV4_TRANSPORT)) {
			traddr.sin.sin_family = AF_INET;
			traddr.sin.sin_len = sizeof(struct sockaddr_in);
			memcpy(&traddr.sin.sin_addr,
			    &trtlv->address, sizeof(struct in_addr));
		} else if (trtlv->type == htons(TLV_IPV6_TRANSPORT)) {
			traddr.sin6.sin6_family = AF_INET6;
			traddr.sin6.sin6_len = sizeof(struct sockaddr_in6);
			memcpy(&traddr.sin6.sin6_addr,
			    &trtlv->address, sizeof(struct in6_addr));
		} else
			warnp("Unknown AF %x for transport address\n",
			    ntohs(trtlv->type));
	} else {
		/* Use LDP ID as transport address */
		traddr.sin.sin_family = AF_INET;
		traddr.sin.sin_len = sizeof(struct sockaddr_in);
		memcpy(&traddr.sin.sin_addr,
		    &pduid->ldp_id, sizeof(struct in_addr));
	}
	/* Add it to hello list or just update timer */
	SLIST_FOREACH(hi, &hello_info_head, infos)
		if (hi->ldp_id.s_addr == pduid->ldp_id.s_addr &&
		    sockaddr_cmp(&hi->transport_address.sa, &traddr.sa) == 0)
			break;
	if (hi == NULL) {
		hi = calloc(1, sizeof(*hi));
		if (!hi) {
			fatalp("Cannot alloc a hello info structure");
			return;
		}
		hi->ldp_id.s_addr = pduid->ldp_id.s_addr;
		memcpy(&hi->transport_address, &traddr, traddr.sa.sa_len);
		SLIST_INSERT_HEAD(&hello_info_head, hi, infos);
		may_connect = false;
	}

	/* Update expire timer */
	if (ht->ch.holdtime != 0)
		hi->keepalive = ntohs(ht->ch.holdtime);
	else {
		if (ntohs(ht->ch.res) >> 15 == 0)
			hi->keepalive = LDP_HELLO_KEEP;
		else
			hi->keepalive = LDP_THELLO_KEEP;
	}

	if (!get_ldp_peer_by_id(&pduid->ldp_id)) {
		/*
		 * RFC 5036 2.5.2: If A1 > A2, LSR1 plays the active role;
		 * otherwise it is passive.
		 */
		if (may_connect == true &&
		    (hi->transport_address.sa.sa_family == AF_INET &&
		    ntohl(hi->transport_address.sin.sin_addr.s_addr) <
		    ntohl(ladd->s_addr))) {
			peer = ldp_peer_new(&pduid->ldp_id, padd,
				&hi->transport_address.sa,
				ntohs(ht->ch.holdtime), 0);
			if (peer == NULL)
				return;
			if (peer->state == LDP_PEER_CONNECTED)
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
		    (!(ifb->ifa_flags & IFF_UP)))
			continue;
		sa = (struct sockaddr_in *) ifb->ifa_addr;
		if (ntohl(sa->sin_addr.s_addr) >> 24 == IN_LOOPBACKNET)
			continue;
		memcpy(&ia[adrcount], &sa->sin_addr, sizeof(struct in_addr));
		adrcount++;
	}
	freeifaddrs(ifa);

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
	setproctitle("LDP ID: %s", my_ldp_id);
	return LDP_E_OK;
}
