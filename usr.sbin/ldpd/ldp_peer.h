/* $NetBSD: ldp_peer.h,v 1.3 2013/01/26 17:29:55 kefren Exp $ */

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

#ifndef _LDP_PEER_H_
#define _LDP_PEER_H_

#include "sys/types.h"
#include "sys/queue.h"
#include "netinet/in.h"

#include "mpls_routes.h"
#include "tlv.h"

struct ldp_peer_address {
	union sockunion address;
	SLIST_ENTRY(ldp_peer_address) addresses;
};

struct label_mapping {
	union sockunion address;
	uint	prefix;
	uint	label;
	SLIST_ENTRY(label_mapping) mappings;
};

struct ldp_peer {
	/*
	 * I add routes to address.
	 * I maintain LDP TCP connection on transport_address.
	 * I use ldp_id as peer identificator.
	 */
	struct sockaddr *address, *transport_address;
	/* According to draft-ietf-mpls-ldp-ipv6-07 ID is IPv4 only for now */
	struct in_addr	ldp_id;
	/* TCP socket */
	int             socket;
	/* Rest of the peer parameters */
	uint16_t	holdtime, timeout;
	int		master;	/* 0 if we're passive */
	int		state;	/* see below for possible states */
	time_t		established_t;	/* time when it did connect */
	/* Here I maintain all the addresses announced by a peer */
	SLIST_HEAD(,ldp_peer_address) ldp_peer_address_head;
	SLIST_HEAD(,label_mapping) label_mapping_head;

	SLIST_ENTRY(ldp_peer) peers;
};
SLIST_HEAD(,ldp_peer) ldp_peer_head;

struct peer_map {
	struct ldp_peer *peer;
	struct label_mapping *lm;
};

/* LDP Peers States */
#define	LDP_PEER_CONNECTING	0
#define	LDP_PEER_CONNECTED	1
#define	LDP_PEER_ESTABLISHED	2
#define	LDP_PEER_HOLDDOWN	3

void            ldp_peer_init(void);
struct ldp_peer *	ldp_peer_new(const struct in_addr *, struct sockaddr *,
				struct sockaddr *, uint16_t, int);
void            ldp_peer_holddown(struct ldp_peer *);
void            ldp_peer_delete(struct ldp_peer *);
struct ldp_peer * get_ldp_peer(const struct sockaddr *);
struct ldp_peer * get_ldp_peer_by_id(const struct in_addr *);
struct ldp_peer * get_ldp_peer_by_socket(int);
int add_ifaddresses(struct ldp_peer *, struct al_tlv *);
int add_ifaddr(struct ldp_peer *, struct sockaddr *);
int del_ifaddr(struct ldp_peer *, struct sockaddr *);
struct ldp_peer_address * check_ifaddr(struct ldp_peer *,
	const struct sockaddr *);
void print_bounded_addresses(struct ldp_peer *);
void del_all_ifaddr(struct ldp_peer *);
int del_ifaddresses(struct ldp_peer *, struct al_tlv *);
void add_my_if_addrs(struct in_addr *, int);

int ldp_peer_add_mapping(struct ldp_peer *, struct sockaddr *, int, int);
int ldp_peer_delete_mapping(struct ldp_peer *, struct sockaddr *, int);
struct label_mapping * ldp_peer_get_lm(struct ldp_peer *, struct sockaddr *,
				uint);
int ldp_peer_delete_all_mappings(struct ldp_peer *);
void ldp_peer_holddown_all(void);

struct peer_map * ldp_test_mapping(struct sockaddr *, int, struct sockaddr *);

const char *	ldp_state_to_name(int);

#endif	/* !_LDP_PEER_H_ */
