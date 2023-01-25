/*	$NetBSD: peer.c,v 1.10 2023/01/25 21:43:30 christos Exp $	*/

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
#define NOTIFY_DSCP_BIT		   10
#define TRANSFER_DSCP_BIT	   11
#define QUERY_DSCP_BIT		   12
#define REQUEST_EXPIRE_BIT	   13
#define EDNS_VERSION_BIT	   14
#define FORCE_TCP_BIT		   15
#define SERVER_PADDING_BIT	   16
#define REQUEST_TCP_KEEPALIVE_BIT  17

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

	return (ISC_R_SUCCESS);
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

	return (res);
}

isc_result_t
dns_peerlist_currpeer(dns_peerlist_t *peers, dns_peer_t **retval) {
	dns_peer_t *p = NULL;

	p = ISC_LIST_TAIL(peers->elements);

	dns_peer_attach(p, retval);

	return (ISC_R_SUCCESS);
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

	return (dns_peer_newprefix(mem, addr, prefixlen, peerptr));
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

	return (ISC_R_SUCCESS);
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

isc_result_t
dns_peer_setbogus(dns_peer_t *peer, bool newval) {
	bool existed;

	REQUIRE(DNS_PEER_VALID(peer));

	existed = DNS_BIT_CHECK(BOGUS_BIT, &peer->bitflags);

	peer->bogus = newval;
	DNS_BIT_SET(BOGUS_BIT, &peer->bitflags);

	return (existed ? ISC_R_EXISTS : ISC_R_SUCCESS);
}

isc_result_t
dns_peer_getbogus(dns_peer_t *peer, bool *retval) {
	REQUIRE(DNS_PEER_VALID(peer));
	REQUIRE(retval != NULL);

	if (DNS_BIT_CHECK(BOGUS_BIT, &peer->bitflags)) {
		*retval = peer->bogus;
		return (ISC_R_SUCCESS);
	} else {
		return (ISC_R_NOTFOUND);
	}
}

isc_result_t
dns_peer_setprovideixfr(dns_peer_t *peer, bool newval) {
	bool existed;

	REQUIRE(DNS_PEER_VALID(peer));

	existed = DNS_BIT_CHECK(PROVIDE_IXFR_BIT, &peer->bitflags);

	peer->provide_ixfr = newval;
	DNS_BIT_SET(PROVIDE_IXFR_BIT, &peer->bitflags);

	return (existed ? ISC_R_EXISTS : ISC_R_SUCCESS);
}

isc_result_t
dns_peer_getprovideixfr(dns_peer_t *peer, bool *retval) {
	REQUIRE(DNS_PEER_VALID(peer));
	REQUIRE(retval != NULL);

	if (DNS_BIT_CHECK(PROVIDE_IXFR_BIT, &peer->bitflags)) {
		*retval = peer->provide_ixfr;
		return (ISC_R_SUCCESS);
	} else {
		return (ISC_R_NOTFOUND);
	}
}

isc_result_t
dns_peer_setrequestixfr(dns_peer_t *peer, bool newval) {
	bool existed;

	REQUIRE(DNS_PEER_VALID(peer));

	existed = DNS_BIT_CHECK(REQUEST_IXFR_BIT, &peer->bitflags);

	peer->request_ixfr = newval;
	DNS_BIT_SET(REQUEST_IXFR_BIT, &peer->bitflags);

	return (existed ? ISC_R_EXISTS : ISC_R_SUCCESS);
}

isc_result_t
dns_peer_getrequestixfr(dns_peer_t *peer, bool *retval) {
	REQUIRE(DNS_PEER_VALID(peer));
	REQUIRE(retval != NULL);

	if (DNS_BIT_CHECK(REQUEST_IXFR_BIT, &peer->bitflags)) {
		*retval = peer->request_ixfr;
		return (ISC_R_SUCCESS);
	} else {
		return (ISC_R_NOTFOUND);
	}
}

isc_result_t
dns_peer_setsupportedns(dns_peer_t *peer, bool newval) {
	bool existed;

	REQUIRE(DNS_PEER_VALID(peer));

	existed = DNS_BIT_CHECK(SUPPORT_EDNS_BIT, &peer->bitflags);

	peer->support_edns = newval;
	DNS_BIT_SET(SUPPORT_EDNS_BIT, &peer->bitflags);

	return (existed ? ISC_R_EXISTS : ISC_R_SUCCESS);
}

isc_result_t
dns_peer_getsupportedns(dns_peer_t *peer, bool *retval) {
	REQUIRE(DNS_PEER_VALID(peer));
	REQUIRE(retval != NULL);

	if (DNS_BIT_CHECK(SUPPORT_EDNS_BIT, &peer->bitflags)) {
		*retval = peer->support_edns;
		return (ISC_R_SUCCESS);
	} else {
		return (ISC_R_NOTFOUND);
	}
}

isc_result_t
dns_peer_setrequestnsid(dns_peer_t *peer, bool newval) {
	bool existed;

	REQUIRE(DNS_PEER_VALID(peer));

	existed = DNS_BIT_CHECK(REQUEST_NSID_BIT, &peer->bitflags);

	peer->request_nsid = newval;
	DNS_BIT_SET(REQUEST_NSID_BIT, &peer->bitflags);

	return (existed ? ISC_R_EXISTS : ISC_R_SUCCESS);
}

isc_result_t
dns_peer_getrequestnsid(dns_peer_t *peer, bool *retval) {
	REQUIRE(DNS_PEER_VALID(peer));
	REQUIRE(retval != NULL);

	if (DNS_BIT_CHECK(REQUEST_NSID_BIT, &peer->bitflags)) {
		*retval = peer->request_nsid;
		return (ISC_R_SUCCESS);
	} else {
		return (ISC_R_NOTFOUND);
	}
}

isc_result_t
dns_peer_setsendcookie(dns_peer_t *peer, bool newval) {
	bool existed;

	REQUIRE(DNS_PEER_VALID(peer));

	existed = DNS_BIT_CHECK(SEND_COOKIE_BIT, &peer->bitflags);

	peer->send_cookie = newval;
	DNS_BIT_SET(SEND_COOKIE_BIT, &peer->bitflags);

	return (existed ? ISC_R_EXISTS : ISC_R_SUCCESS);
}

isc_result_t
dns_peer_getsendcookie(dns_peer_t *peer, bool *retval) {
	REQUIRE(DNS_PEER_VALID(peer));
	REQUIRE(retval != NULL);

	if (DNS_BIT_CHECK(SEND_COOKIE_BIT, &peer->bitflags)) {
		*retval = peer->send_cookie;
		return (ISC_R_SUCCESS);
	} else {
		return (ISC_R_NOTFOUND);
	}
}

isc_result_t
dns_peer_setrequestexpire(dns_peer_t *peer, bool newval) {
	bool existed;

	REQUIRE(DNS_PEER_VALID(peer));

	existed = DNS_BIT_CHECK(REQUEST_EXPIRE_BIT, &peer->bitflags);

	peer->request_expire = newval;
	DNS_BIT_SET(REQUEST_EXPIRE_BIT, &peer->bitflags);

	return (existed ? ISC_R_EXISTS : ISC_R_SUCCESS);
}

isc_result_t
dns_peer_getrequestexpire(dns_peer_t *peer, bool *retval) {
	REQUIRE(DNS_PEER_VALID(peer));
	REQUIRE(retval != NULL);

	if (DNS_BIT_CHECK(REQUEST_EXPIRE_BIT, &peer->bitflags)) {
		*retval = peer->request_expire;
		return (ISC_R_SUCCESS);
	} else {
		return (ISC_R_NOTFOUND);
	}
}

isc_result_t
dns_peer_setforcetcp(dns_peer_t *peer, bool newval) {
	bool existed;

	REQUIRE(DNS_PEER_VALID(peer));

	existed = DNS_BIT_CHECK(FORCE_TCP_BIT, &peer->bitflags);

	peer->force_tcp = newval;
	DNS_BIT_SET(FORCE_TCP_BIT, &peer->bitflags);

	return (existed ? ISC_R_EXISTS : ISC_R_SUCCESS);
}

isc_result_t
dns_peer_getforcetcp(dns_peer_t *peer, bool *retval) {
	REQUIRE(DNS_PEER_VALID(peer));
	REQUIRE(retval != NULL);

	if (DNS_BIT_CHECK(FORCE_TCP_BIT, &peer->bitflags)) {
		*retval = peer->force_tcp;
		return (ISC_R_SUCCESS);
	} else {
		return (ISC_R_NOTFOUND);
	}
}

isc_result_t
dns_peer_settcpkeepalive(dns_peer_t *peer, bool newval) {
	bool existed;

	REQUIRE(DNS_PEER_VALID(peer));

	existed = DNS_BIT_CHECK(REQUEST_TCP_KEEPALIVE_BIT, &peer->bitflags);

	peer->tcp_keepalive = newval;
	DNS_BIT_SET(REQUEST_TCP_KEEPALIVE_BIT, &peer->bitflags);

	return (existed ? ISC_R_EXISTS : ISC_R_SUCCESS);
}

isc_result_t
dns_peer_gettcpkeepalive(dns_peer_t *peer, bool *retval) {
	REQUIRE(DNS_PEER_VALID(peer));
	REQUIRE(retval != NULL);

	if (DNS_BIT_CHECK(REQUEST_TCP_KEEPALIVE_BIT, &peer->bitflags)) {
		*retval = peer->tcp_keepalive;
		return (ISC_R_SUCCESS);
	} else {
		return (ISC_R_NOTFOUND);
	}
}

isc_result_t
dns_peer_settransfers(dns_peer_t *peer, uint32_t newval) {
	bool existed;

	REQUIRE(DNS_PEER_VALID(peer));

	existed = DNS_BIT_CHECK(TRANSFERS_BIT, &peer->bitflags);

	peer->transfers = newval;
	DNS_BIT_SET(TRANSFERS_BIT, &peer->bitflags);

	return (existed ? ISC_R_EXISTS : ISC_R_SUCCESS);
}

isc_result_t
dns_peer_gettransfers(dns_peer_t *peer, uint32_t *retval) {
	REQUIRE(DNS_PEER_VALID(peer));
	REQUIRE(retval != NULL);

	if (DNS_BIT_CHECK(TRANSFERS_BIT, &peer->bitflags)) {
		*retval = peer->transfers;
		return (ISC_R_SUCCESS);
	} else {
		return (ISC_R_NOTFOUND);
	}
}

isc_result_t
dns_peer_settransferformat(dns_peer_t *peer, dns_transfer_format_t newval) {
	bool existed;

	REQUIRE(DNS_PEER_VALID(peer));

	existed = DNS_BIT_CHECK(SERVER_TRANSFER_FORMAT_BIT, &peer->bitflags);

	peer->transfer_format = newval;
	DNS_BIT_SET(SERVER_TRANSFER_FORMAT_BIT, &peer->bitflags);

	return (existed ? ISC_R_EXISTS : ISC_R_SUCCESS);
}

isc_result_t
dns_peer_gettransferformat(dns_peer_t *peer, dns_transfer_format_t *retval) {
	REQUIRE(DNS_PEER_VALID(peer));
	REQUIRE(retval != NULL);

	if (DNS_BIT_CHECK(SERVER_TRANSFER_FORMAT_BIT, &peer->bitflags)) {
		*retval = peer->transfer_format;
		return (ISC_R_SUCCESS);
	} else {
		return (ISC_R_NOTFOUND);
	}
}

isc_result_t
dns_peer_getkey(dns_peer_t *peer, dns_name_t **retval) {
	REQUIRE(DNS_PEER_VALID(peer));
	REQUIRE(retval != NULL);

	if (peer->key != NULL) {
		*retval = peer->key;
	}

	return (peer->key == NULL ? ISC_R_NOTFOUND : ISC_R_SUCCESS);
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

	return (exists ? ISC_R_EXISTS : ISC_R_SUCCESS);
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
		return (result);
	}

	name = isc_mem_get(peer->mem, sizeof(dns_name_t));

	dns_name_init(name, NULL);
	dns_name_dup(dns_fixedname_name(&fname), peer->mem, name);

	result = dns_peer_setkey(peer, &name);
	if (result != ISC_R_SUCCESS) {
		isc_mem_put(peer->mem, name, sizeof(dns_name_t));
	}

	return (result);
}

isc_result_t
dns_peer_settransfersource(dns_peer_t *peer,
			   const isc_sockaddr_t *transfer_source) {
	REQUIRE(DNS_PEER_VALID(peer));

	if (peer->transfer_source != NULL) {
		isc_mem_put(peer->mem, peer->transfer_source,
			    sizeof(*peer->transfer_source));
		peer->transfer_source = NULL;
	}
	if (transfer_source != NULL) {
		peer->transfer_source =
			isc_mem_get(peer->mem, sizeof(*peer->transfer_source));

		*peer->transfer_source = *transfer_source;
	}
	return (ISC_R_SUCCESS);
}

isc_result_t
dns_peer_gettransfersource(dns_peer_t *peer, isc_sockaddr_t *transfer_source) {
	REQUIRE(DNS_PEER_VALID(peer));
	REQUIRE(transfer_source != NULL);

	if (peer->transfer_source == NULL) {
		return (ISC_R_NOTFOUND);
	}
	*transfer_source = *peer->transfer_source;
	return (ISC_R_SUCCESS);
}

isc_result_t
dns_peer_setnotifysource(dns_peer_t *peer,
			 const isc_sockaddr_t *notify_source) {
	REQUIRE(DNS_PEER_VALID(peer));

	if (peer->notify_source != NULL) {
		isc_mem_put(peer->mem, peer->notify_source,
			    sizeof(*peer->notify_source));
		peer->notify_source = NULL;
	}
	if (notify_source != NULL) {
		peer->notify_source = isc_mem_get(peer->mem,
						  sizeof(*peer->notify_source));

		*peer->notify_source = *notify_source;
	}
	return (ISC_R_SUCCESS);
}

isc_result_t
dns_peer_getnotifysource(dns_peer_t *peer, isc_sockaddr_t *notify_source) {
	REQUIRE(DNS_PEER_VALID(peer));
	REQUIRE(notify_source != NULL);

	if (peer->notify_source == NULL) {
		return (ISC_R_NOTFOUND);
	}
	*notify_source = *peer->notify_source;
	return (ISC_R_SUCCESS);
}

isc_result_t
dns_peer_setquerysource(dns_peer_t *peer, const isc_sockaddr_t *query_source) {
	REQUIRE(DNS_PEER_VALID(peer));

	if (peer->query_source != NULL) {
		isc_mem_put(peer->mem, peer->query_source,
			    sizeof(*peer->query_source));
		peer->query_source = NULL;
	}
	if (query_source != NULL) {
		peer->query_source = isc_mem_get(peer->mem,
						 sizeof(*peer->query_source));

		*peer->query_source = *query_source;
	}
	return (ISC_R_SUCCESS);
}

isc_result_t
dns_peer_getquerysource(dns_peer_t *peer, isc_sockaddr_t *query_source) {
	REQUIRE(DNS_PEER_VALID(peer));
	REQUIRE(query_source != NULL);

	if (peer->query_source == NULL) {
		return (ISC_R_NOTFOUND);
	}
	*query_source = *peer->query_source;
	return (ISC_R_SUCCESS);
}

isc_result_t
dns_peer_setudpsize(dns_peer_t *peer, uint16_t udpsize) {
	bool existed;

	REQUIRE(DNS_PEER_VALID(peer));

	existed = DNS_BIT_CHECK(SERVER_UDPSIZE_BIT, &peer->bitflags);

	peer->udpsize = udpsize;
	DNS_BIT_SET(SERVER_UDPSIZE_BIT, &peer->bitflags);

	return (existed ? ISC_R_EXISTS : ISC_R_SUCCESS);
}

isc_result_t
dns_peer_getudpsize(dns_peer_t *peer, uint16_t *udpsize) {
	REQUIRE(DNS_PEER_VALID(peer));
	REQUIRE(udpsize != NULL);

	if (DNS_BIT_CHECK(SERVER_UDPSIZE_BIT, &peer->bitflags)) {
		*udpsize = peer->udpsize;
		return (ISC_R_SUCCESS);
	} else {
		return (ISC_R_NOTFOUND);
	}
}

isc_result_t
dns_peer_setmaxudp(dns_peer_t *peer, uint16_t maxudp) {
	bool existed;

	REQUIRE(DNS_PEER_VALID(peer));

	existed = DNS_BIT_CHECK(SERVER_MAXUDP_BIT, &peer->bitflags);

	peer->maxudp = maxudp;
	DNS_BIT_SET(SERVER_MAXUDP_BIT, &peer->bitflags);

	return (existed ? ISC_R_EXISTS : ISC_R_SUCCESS);
}

isc_result_t
dns_peer_getmaxudp(dns_peer_t *peer, uint16_t *maxudp) {
	REQUIRE(DNS_PEER_VALID(peer));
	REQUIRE(maxudp != NULL);

	if (DNS_BIT_CHECK(SERVER_MAXUDP_BIT, &peer->bitflags)) {
		*maxudp = peer->maxudp;
		return (ISC_R_SUCCESS);
	} else {
		return (ISC_R_NOTFOUND);
	}
}

isc_result_t
dns_peer_setpadding(dns_peer_t *peer, uint16_t padding) {
	bool existed;

	REQUIRE(DNS_PEER_VALID(peer));

	existed = DNS_BIT_CHECK(SERVER_PADDING_BIT, &peer->bitflags);

	if (padding > 512) {
		padding = 512;
	}
	peer->padding = padding;
	DNS_BIT_SET(SERVER_PADDING_BIT, &peer->bitflags);

	return (existed ? ISC_R_EXISTS : ISC_R_SUCCESS);
}

isc_result_t
dns_peer_getpadding(dns_peer_t *peer, uint16_t *padding) {
	REQUIRE(DNS_PEER_VALID(peer));
	REQUIRE(padding != NULL);

	if (DNS_BIT_CHECK(SERVER_PADDING_BIT, &peer->bitflags)) {
		*padding = peer->padding;
		return (ISC_R_SUCCESS);
	} else {
		return (ISC_R_NOTFOUND);
	}
}

isc_result_t
dns_peer_setnotifydscp(dns_peer_t *peer, isc_dscp_t dscp) {
	REQUIRE(DNS_PEER_VALID(peer));
	REQUIRE(dscp < 64);

	peer->notify_dscp = dscp;
	DNS_BIT_SET(NOTIFY_DSCP_BIT, &peer->bitflags);
	return (ISC_R_SUCCESS);
}

isc_result_t
dns_peer_getnotifydscp(dns_peer_t *peer, isc_dscp_t *dscpp) {
	REQUIRE(DNS_PEER_VALID(peer));
	REQUIRE(dscpp != NULL);

	if (DNS_BIT_CHECK(NOTIFY_DSCP_BIT, &peer->bitflags)) {
		*dscpp = peer->notify_dscp;
		return (ISC_R_SUCCESS);
	}
	return (ISC_R_NOTFOUND);
}

isc_result_t
dns_peer_settransferdscp(dns_peer_t *peer, isc_dscp_t dscp) {
	REQUIRE(DNS_PEER_VALID(peer));
	REQUIRE(dscp < 64);

	peer->transfer_dscp = dscp;
	DNS_BIT_SET(TRANSFER_DSCP_BIT, &peer->bitflags);
	return (ISC_R_SUCCESS);
}

isc_result_t
dns_peer_gettransferdscp(dns_peer_t *peer, isc_dscp_t *dscpp) {
	REQUIRE(DNS_PEER_VALID(peer));
	REQUIRE(dscpp != NULL);

	if (DNS_BIT_CHECK(TRANSFER_DSCP_BIT, &peer->bitflags)) {
		*dscpp = peer->transfer_dscp;
		return (ISC_R_SUCCESS);
	}
	return (ISC_R_NOTFOUND);
}

isc_result_t
dns_peer_setquerydscp(dns_peer_t *peer, isc_dscp_t dscp) {
	REQUIRE(DNS_PEER_VALID(peer));
	REQUIRE(dscp < 64);

	peer->query_dscp = dscp;
	DNS_BIT_SET(QUERY_DSCP_BIT, &peer->bitflags);
	return (ISC_R_SUCCESS);
}

isc_result_t
dns_peer_getquerydscp(dns_peer_t *peer, isc_dscp_t *dscpp) {
	REQUIRE(DNS_PEER_VALID(peer));
	REQUIRE(dscpp != NULL);

	if (DNS_BIT_CHECK(QUERY_DSCP_BIT, &peer->bitflags)) {
		*dscpp = peer->query_dscp;
		return (ISC_R_SUCCESS);
	}
	return (ISC_R_NOTFOUND);
}

isc_result_t
dns_peer_setednsversion(dns_peer_t *peer, uint8_t ednsversion) {
	REQUIRE(DNS_PEER_VALID(peer));

	peer->ednsversion = ednsversion;
	DNS_BIT_SET(EDNS_VERSION_BIT, &peer->bitflags);

	return (ISC_R_SUCCESS);
}

isc_result_t
dns_peer_getednsversion(dns_peer_t *peer, uint8_t *ednsversion) {
	REQUIRE(DNS_PEER_VALID(peer));
	REQUIRE(ednsversion != NULL);

	if (DNS_BIT_CHECK(EDNS_VERSION_BIT, &peer->bitflags)) {
		*ednsversion = peer->ednsversion;
		return (ISC_R_SUCCESS);
	} else {
		return (ISC_R_NOTFOUND);
	}
}
