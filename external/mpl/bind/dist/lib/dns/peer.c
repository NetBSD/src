/*	$NetBSD: peer.c,v 1.12 2025/01/26 16:25:24 christos Exp $	*/

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

#include <inttypes.h>
#include <stdbool.h>

#include <isc/mem.h>
#include <isc/sockaddr.h>
#include <isc/string.h>
#include <isc/util.h>

#include <dns/bit.h>
#include <dns/fixedname.h>
#include <dns/name.h>
#include <dns/peer.h>

/***
 *** Types
 ***/

struct dns_peerlist {
	unsigned int magic;
	isc_refcount_t refs;

	isc_mem_t *mem;

	ISC_LIST(dns_peer_t) elements;
};

struct dns_peer {
	unsigned int magic;
	isc_refcount_t refs;

	isc_mem_t *mem;

	isc_netaddr_t address;
	unsigned int prefixlen;
	bool bogus;
	dns_transfer_format_t transfer_format;
	uint32_t transfers;
	bool support_ixfr;
	bool provide_ixfr;
	bool request_ixfr;
	bool support_edns;
	bool request_nsid;
	bool send_cookie;
	bool require_cookie;
	bool request_expire;
	bool force_tcp;
	bool tcp_keepalive;
	bool check_axfr_id;
	dns_name_t *key;
	isc_sockaddr_t *transfer_source;
	isc_sockaddr_t *notify_source;
	isc_sockaddr_t *query_source;
	uint16_t udpsize;    /* receive size */
	uint16_t maxudp;     /* transmit size */
	uint16_t padding;    /* pad block size */
	uint8_t ednsversion; /* edns version */

	uint32_t bitflags;

	ISC_LINK(dns_peer_t) next;
};

/*%
 * Bit positions in the dns_peer_t structure flags field
 */
#define BOGUS_BIT		   0
#define SERVER_TRANSFER_FORMAT_BIT 1
#define TRANSFERS_BIT		   2
#define PROVIDE_IXFR_BIT	   3
#define REQUEST_IXFR_BIT	   4
#define SUPPORT_EDNS_BIT	   5
#define SERVER_UDPSIZE_BIT	   6
#define SERVER_MAXUDP_BIT	   7
#define REQUEST_NSID_BIT	   8
#define SEND_COOKIE_BIT		   9
#define REQUEST_EXPIRE_BIT	   10
#define EDNS_VERSION_BIT	   11
#define FORCE_TCP_BIT		   12
#define SERVER_PADDING_BIT	   13
#define REQUEST_TCP_KEEPALIVE_BIT  14
#define REQUIRE_COOKIE_BIT	   15

static void
peerlist_delete(dns_peerlist_t **list);

static void
peer_delete(dns_peer_t **peer);

isc_result_t
dns_peerlist_new(isc_mem_t *mem, dns_peerlist_t **list) {
	dns_peerlist_t *l;

	REQUIRE(list != NULL);

	l = isc_mem_get(mem, sizeof(*l));

	ISC_LIST_INIT(l->elements);
	l->mem = mem;
	isc_refcount_init(&l->refs, 1);
	l->magic = DNS_PEERLIST_MAGIC;

	*list = l;

	return ISC_R_SUCCESS;
}

void
dns_peerlist_attach(dns_peerlist_t *source, dns_peerlist_t **target) {
	REQUIRE(DNS_PEERLIST_VALID(source));
	REQUIRE(target != NULL);
	REQUIRE(*target == NULL);

	isc_refcount_increment(&source->refs);

	*target = source;
}

void
dns_peerlist_detach(dns_peerlist_t **list) {
	dns_peerlist_t *plist;

	REQUIRE(list != NULL);
	REQUIRE(*list != NULL);
	REQUIRE(DNS_PEERLIST_VALID(*list));

	plist = *list;
	*list = NULL;

	if (isc_refcount_decrement(&plist->refs) == 1) {
		peerlist_delete(&plist);
	}
}

static void
peerlist_delete(dns_peerlist_t **list) {
	dns_peerlist_t *l;
	dns_peer_t *server, *stmp;

	REQUIRE(list != NULL);
	REQUIRE(DNS_PEERLIST_VALID(*list));

	l = *list;
	*list = NULL;

	isc_refcount_destroy(&l->refs);

	server = ISC_LIST_HEAD(l->elements);
	while (server != NULL) {
		stmp = ISC_LIST_NEXT(server, next);
		ISC_LIST_UNLINK(l->elements, server, next);
		dns_peer_detach(&server);
		server = stmp;
	}

	l->magic = 0;
	isc_mem_put(l->mem, l, sizeof(*l));
}

void
dns_peerlist_addpeer(dns_peerlist_t *peers, dns_peer_t *peer) {
	dns_peer_t *p = NULL;

	dns_peer_attach(peer, &p);

	/*
	 * More specifics to front of list.
	 */
	for (p = ISC_LIST_HEAD(peers->elements); p != NULL;
	     p = ISC_LIST_NEXT(p, next))
	{
		if (p->prefixlen < peer->prefixlen) {
			break;
		}
	}

	if (p != NULL) {
		ISC_LIST_INSERTBEFORE(peers->elements, p, peer, next);
	} else {
		ISC_LIST_APPEND(peers->elements, peer, next);
	}
}

isc_result_t
dns_peerlist_peerbyaddr(dns_peerlist_t *servers, const isc_netaddr_t *addr,
			dns_peer_t **retval) {
	dns_peer_t *server;
	isc_result_t res;

	REQUIRE(retval != NULL);
	REQUIRE(DNS_PEERLIST_VALID(servers));

	server = ISC_LIST_HEAD(servers->elements);
	while (server != NULL) {
		if (isc_netaddr_eqprefix(addr, &server->address,
					 server->prefixlen))
		{
			break;
		}

		server = ISC_LIST_NEXT(server, next);
	}

	if (server != NULL) {
		*retval = server;
		res = ISC_R_SUCCESS;
	} else {
		res = ISC_R_NOTFOUND;
	}

	return res;
}

isc_result_t
dns_peerlist_currpeer(dns_peerlist_t *peers, dns_peer_t **retval) {
	dns_peer_t *p = NULL;

	p = ISC_LIST_TAIL(peers->elements);

	dns_peer_attach(p, retval);

	return ISC_R_SUCCESS;
}

isc_result_t
dns_peer_new(isc_mem_t *mem, const isc_netaddr_t *addr, dns_peer_t **peerptr) {
	unsigned int prefixlen = 0;

	REQUIRE(peerptr != NULL);
	switch (addr->family) {
	case AF_INET:
		prefixlen = 32;
		break;
	case AF_INET6:
		prefixlen = 128;
		break;
	default:
		UNREACHABLE();
	}

	return dns_peer_newprefix(mem, addr, prefixlen, peerptr);
}

isc_result_t
dns_peer_newprefix(isc_mem_t *mem, const isc_netaddr_t *addr,
		   unsigned int prefixlen, dns_peer_t **peerptr) {
	dns_peer_t *peer;

	REQUIRE(peerptr != NULL && *peerptr == NULL);

	peer = isc_mem_get(mem, sizeof(*peer));

	*peer = (dns_peer_t){
		.magic = DNS_PEER_MAGIC,
		.address = *addr,
		.prefixlen = prefixlen,
		.mem = mem,
		.transfer_format = dns_one_answer,
	};

	isc_refcount_init(&peer->refs, 1);

	ISC_LINK_INIT(peer, next);

	*peerptr = peer;

	return ISC_R_SUCCESS;
}

void
dns_peer_attach(dns_peer_t *source, dns_peer_t **target) {
	REQUIRE(DNS_PEER_VALID(source));
	REQUIRE(target != NULL);
	REQUIRE(*target == NULL);

	isc_refcount_increment(&source->refs);

	*target = source;
}

void
dns_peer_detach(dns_peer_t **peer) {
	dns_peer_t *p;

	REQUIRE(peer != NULL);
	REQUIRE(*peer != NULL);
	REQUIRE(DNS_PEER_VALID(*peer));

	p = *peer;
	*peer = NULL;

	if (isc_refcount_decrement(&p->refs) == 1) {
		peer_delete(&p);
	}
}

static void
peer_delete(dns_peer_t **peer) {
	dns_peer_t *p;
	isc_mem_t *mem;

	REQUIRE(peer != NULL);
	REQUIRE(DNS_PEER_VALID(*peer));

	p = *peer;
	*peer = NULL;

	isc_refcount_destroy(&p->refs);

	mem = p->mem;
	p->mem = NULL;
	p->magic = 0;

	if (p->key != NULL) {
		dns_name_free(p->key, mem);
		isc_mem_put(mem, p->key, sizeof(dns_name_t));
	}

	if (p->query_source != NULL) {
		isc_mem_put(mem, p->query_source, sizeof(*p->query_source));
	}

	if (p->notify_source != NULL) {
		isc_mem_put(mem, p->notify_source, sizeof(*p->notify_source));
	}

	if (p->transfer_source != NULL) {
		isc_mem_put(mem, p->transfer_source,
			    sizeof(*p->transfer_source));
	}

	isc_mem_put(mem, p, sizeof(*p));
}

#define ACCESS_OPTION(name, macro, type, element)                        \
	isc_result_t dns_peer_get##name(dns_peer_t *peer, type *value) { \
		REQUIRE(DNS_PEER_VALID(peer));                           \
		REQUIRE(value != NULL);                                  \
		if (DNS_BIT_CHECK(macro, &peer->bitflags)) {             \
			*value = peer->element;                          \
			return (ISC_R_SUCCESS);                          \
		} else {                                                 \
			return (ISC_R_NOTFOUND);                         \
		}                                                        \
	}                                                                \
	isc_result_t dns_peer_set##name(dns_peer_t *peer, type value) {  \
		bool existed;                                            \
		REQUIRE(DNS_PEER_VALID(peer));                           \
		existed = DNS_BIT_CHECK(macro, &peer->bitflags);         \
		peer->element = value;                                   \
		DNS_BIT_SET(macro, &peer->bitflags);                     \
		return (existed ? ISC_R_EXISTS : ISC_R_SUCCESS);         \
	}

ACCESS_OPTION(bogus, BOGUS_BIT, bool, bogus)
ACCESS_OPTION(forcetcp, FORCE_TCP_BIT, bool, force_tcp)
ACCESS_OPTION(maxudp, SERVER_MAXUDP_BIT, uint16_t, maxudp)
ACCESS_OPTION(provideixfr, PROVIDE_IXFR_BIT, bool, provide_ixfr)
ACCESS_OPTION(requestexpire, REQUEST_EXPIRE_BIT, bool, request_expire)
ACCESS_OPTION(requestixfr, REQUEST_IXFR_BIT, bool, request_ixfr)
ACCESS_OPTION(requestnsid, REQUEST_NSID_BIT, bool, request_nsid)
ACCESS_OPTION(requirecookie, REQUIRE_COOKIE_BIT, bool, require_cookie)
ACCESS_OPTION(sendcookie, SEND_COOKIE_BIT, bool, send_cookie)
ACCESS_OPTION(supportedns, SUPPORT_EDNS_BIT, bool, support_edns)
ACCESS_OPTION(tcpkeepalive, REQUEST_TCP_KEEPALIVE_BIT, bool, tcp_keepalive)
ACCESS_OPTION(transferformat, SERVER_TRANSFER_FORMAT_BIT, dns_transfer_format_t,
	      transfer_format)
ACCESS_OPTION(transfers, TRANSFERS_BIT, uint32_t, transfers)
ACCESS_OPTION(udpsize, SERVER_UDPSIZE_BIT, uint16_t, udpsize)

#define ACCESS_OPTIONMAX(name, macro, type, element, max)                \
	isc_result_t dns_peer_get##name(dns_peer_t *peer, type *value) { \
		REQUIRE(DNS_PEER_VALID(peer));                           \
		REQUIRE(value != NULL);                                  \
		if (DNS_BIT_CHECK(macro, &peer->bitflags)) {             \
			*value = peer->element;                          \
			return (ISC_R_SUCCESS);                          \
		} else {                                                 \
			return (ISC_R_NOTFOUND);                         \
		}                                                        \
	}                                                                \
	isc_result_t dns_peer_set##name(dns_peer_t *peer, type value) {  \
		bool existed;                                            \
		REQUIRE(DNS_PEER_VALID(peer));                           \
		existed = DNS_BIT_CHECK(macro, &peer->bitflags);         \
		if (value > max) {                                       \
			value = max;                                     \
		}                                                        \
		peer->element = value;                                   \
		DNS_BIT_SET(macro, &peer->bitflags);                     \
		return (existed ? ISC_R_EXISTS : ISC_R_SUCCESS);         \
	}

ACCESS_OPTIONMAX(padding, SERVER_PADDING_BIT, uint16_t, padding, 512)

#define ACCESS_SOCKADDR(name, element)                                       \
	isc_result_t dns_peer_get##name(dns_peer_t *peer,                    \
					isc_sockaddr_t *value) {             \
		REQUIRE(DNS_PEER_VALID(peer));                               \
		REQUIRE(value != NULL);                                      \
		if (peer->element == NULL) {                                 \
			return (ISC_R_NOTFOUND);                             \
		}                                                            \
		*value = *peer->element;                                     \
		return (ISC_R_SUCCESS);                                      \
	}                                                                    \
	isc_result_t dns_peer_set##name(dns_peer_t *peer,                    \
					const isc_sockaddr_t *value) {       \
		REQUIRE(DNS_PEER_VALID(peer));                               \
		if (peer->element != NULL) {                                 \
			isc_mem_put(peer->mem, peer->element,                \
				    sizeof(*peer->element));                 \
			peer->element = NULL;                                \
		}                                                            \
		if (value != NULL) {                                         \
			peer->element = isc_mem_get(peer->mem,               \
						    sizeof(*peer->element)); \
			*peer->element = *value;                             \
		}                                                            \
		return (ISC_R_SUCCESS);                                      \
	}

ACCESS_SOCKADDR(notifysource, notify_source)
ACCESS_SOCKADDR(querysource, query_source)
ACCESS_SOCKADDR(transfersource, transfer_source)

#define ACCESS_OPTION_OVERWRITE(name, macro, type, element)              \
	isc_result_t dns_peer_get##name(dns_peer_t *peer, type *value) { \
		REQUIRE(DNS_PEER_VALID(peer));                           \
		REQUIRE(value != NULL);                                  \
		if (DNS_BIT_CHECK(macro, &peer->bitflags)) {             \
			*value = peer->element;                          \
			return (ISC_R_SUCCESS);                          \
		} else {                                                 \
			return (ISC_R_NOTFOUND);                         \
		}                                                        \
	}                                                                \
	isc_result_t dns_peer_set##name(dns_peer_t *peer, type value) {  \
		REQUIRE(DNS_PEER_VALID(peer));                           \
		peer->element = value;                                   \
		DNS_BIT_SET(macro, &peer->bitflags);                     \
		return (ISC_R_SUCCESS);                                  \
	}

ACCESS_OPTION_OVERWRITE(ednsversion, EDNS_VERSION_BIT, uint8_t, ednsversion)

isc_result_t
dns_peer_getkey(dns_peer_t *peer, dns_name_t **retval) {
	REQUIRE(DNS_PEER_VALID(peer));
	REQUIRE(retval != NULL);

	if (peer->key != NULL) {
		*retval = peer->key;
	}

	return peer->key == NULL ? ISC_R_NOTFOUND : ISC_R_SUCCESS;
}

isc_result_t
dns_peer_setkey(dns_peer_t *peer, dns_name_t **keyval) {
	bool exists = false;

	if (peer->key != NULL) {
		dns_name_free(peer->key, peer->mem);
		isc_mem_put(peer->mem, peer->key, sizeof(dns_name_t));
		exists = true;
	}

	peer->key = *keyval;
	*keyval = NULL;

	return exists ? ISC_R_EXISTS : ISC_R_SUCCESS;
}

isc_result_t
dns_peer_setkeybycharp(dns_peer_t *peer, const char *keyval) {
	isc_buffer_t b;
	dns_fixedname_t fname;
	dns_name_t *name;
	isc_result_t result;

	dns_fixedname_init(&fname);
	isc_buffer_constinit(&b, keyval, strlen(keyval));
	isc_buffer_add(&b, strlen(keyval));
	result = dns_name_fromtext(dns_fixedname_name(&fname), &b, dns_rootname,
				   0, NULL);
	if (result != ISC_R_SUCCESS) {
		return result;
	}

	name = isc_mem_get(peer->mem, sizeof(dns_name_t));

	dns_name_init(name, NULL);
	dns_name_dup(dns_fixedname_name(&fname), peer->mem, name);

	result = dns_peer_setkey(peer, &name);
	if (result != ISC_R_SUCCESS) {
		isc_mem_put(peer->mem, name, sizeof(dns_name_t));
	}

	return result;
}
