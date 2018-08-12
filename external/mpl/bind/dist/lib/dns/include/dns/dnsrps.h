/*	$NetBSD: dnsrps.h,v 1.2 2018/08/12 13:02:35 christos Exp $	*/

/*
 * Copyright (C) Internet Systems Consortium, Inc. ("ISC")
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * See the COPYRIGHT file distributed with this work for additional
 * information regarding copyright ownership.
 */

#ifndef DNS_DNSRPS_H
#define DNS_DNSRPS_H

#include <isc/lang.h>
#include <dns/types.h>

#include <config.h>

#ifdef USE_DNSRPS

#include <dns/librpz.h>
#include <dns/rpz.h>

/*
 * Error message if dlopen(librpz) failed.
 */
extern librpz_emsg_t librpz_lib_open_emsg;


/*
 * These shim BIND9 database, node, and rdataset are handles on RRs from librpz.
 *
 * All of these structures are used by a single thread and so need no locks.
 *
 * rpsdb_t holds the state for a set of RPZ queries.
 *
 * rpsnode_t is a link to the rpsdb_t for the set of  RPZ queries
 * and a flag saying whether it is pretending to be a node with RRs for
 * the qname or the node with the SOA for the zone containing the rewritten
 * RRs or justifying NXDOMAIN.
 */
typedef struct {
	uint8_t			unused;
} rpsnode_t;
typedef struct rpsdb {
	dns_db_t		common;
	int			ref_cnt;
	librpz_result_id_t	hit_id;
	librpz_result_t		result;
	librpz_rsp_t*		rsp;
	librpz_domain_buf_t	origin_buf;
	const dns_name_t	*qname;
	rpsnode_t		origin_node;
	rpsnode_t		data_node;
} rpsdb_t;


/*
 * Convert a dnsrps policy to a classic BIND9 RPZ policy.
 */
dns_rpz_policy_t dns_dnsrps_2policy(librpz_policy_t rps_policy);

/*
 * Convert a dnsrps trigger to a classic BIND9 RPZ rewrite or trigger type.
 */
dns_rpz_type_t dns_dnsrps_trig2type(librpz_trig_t trig);

/*
 * Convert a classic BIND9 RPZ rewrite or trigger type to a librpz trigger type.
 */
librpz_trig_t dns_dnsrps_type2trig(dns_rpz_type_t type);

/*
 * Start dnsrps for the entire server.
 */
isc_result_t dns_dnsrps_server_create(void);

/*
 * Stop dnsrps for the entire server.
 */
void dns_dnsrps_server_destroy(void);

/*
 * Ready dnsrps for a view.
 */
isc_result_t dns_dnsrps_view_init(dns_rpz_zones_t *new, char *rps_cstr);

/*
 * Connect to and start the dnsrps daemon, dnsrpzd.
 */
isc_result_t dns_dnsrps_connect(dns_rpz_zones_t *rpzs);

/*
 * Get ready to try dnsrps rewriting.
 */
isc_result_t dns_dnsrps_rewrite_init(librpz_emsg_t *emsg, dns_rpz_st_t *st,
				      dns_rpz_zones_t *rpzs,
				      const dns_name_t *qname, isc_mem_t *mctx,
				      isc_boolean_t have_rd);

#endif /* USE_DNSRPS */

ISC_LANG_ENDDECLS

#endif /* DNS_DNSRPS_H */
