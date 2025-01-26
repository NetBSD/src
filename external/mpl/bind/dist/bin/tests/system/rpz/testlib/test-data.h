/*	$NetBSD: test-data.h,v 1.2 2025/01/26 16:25:00 christos Exp $	*/

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

/*
 * Limited implementation of the DNSRPS API for testing purposes.
 *
 * Copyright (c) 2016-2017 Farsight Security, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *	http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#define LIBRPZ_LIB_OPEN 2
#include <dns/librpz.h>

#if __NAMESER < 19991006
/*
 * If the new API is not available define the values we use.
 */

#define ns_c_in 1

#define ns_t_invalid 0
#define ns_t_a	     1
#define ns_t_cname   5
#define ns_t_txt     16
#define ns_t_aaaa    28
#define ns_t_dname   39

#endif

#include "trpz.h"

#define NODE_FLAG_IPV6_ADDRESS 0x1
#define NODE_FLAG_STATIC_DATA  0x2

#define ZOPT_POLICY_PASSTHRU 0x0001
#define ZOPT_POLICY_DROP     0x0002
#define ZOPT_POLICY_TCP_ONLY 0x0004
#define ZOPT_POLICY_NXDOMAIN 0x0008
#define ZOPT_POLICY_NODATA   0x0010
#define ZOPT_POLICY_GIVEN    0x0020
#define ZOPT_POLICY_DISABLED 0x0040

#define ZOPT_RECURSIVE_ONLY	0x0100
#define ZOPT_NOT_RECURSIVE_ONLY 0x0200
#define ZOPT_QNAME_AS_NS	0x0400
#define ZOPT_IP_AS_NS		0x0800

#define ZOPT_QNAME_WAIT_RECURSE	   0x1000
#define ZOPT_NO_QNAME_WAIT_RECURSE 0x2000
#define ZOPT_NO_NSIP_WAIT_RECURSE  0x4000

typedef struct {
	char name[256];
	uint32_t serial;
	int has_update;
	size_t rollback;
	int has_triggers[2][LIBRPZ_TRIG_NSIP + 1];
	bool forgotten;
	bool qname_as_ns, ip_as_ns;
	bool not_recursive_only;
	bool no_qname_wait_recurse, no_nsip_wait_recurse;
	unsigned long flags;
} trpz_zone_t;

typedef struct {
	uint16_t type;
	uint16_t class;
	uint32_t ttl;
	uint16_t rdlength;
	uint8_t *rdata;
	unsigned int rrn;
} trpz_rr_t;

typedef struct {
	char *canonical;
	char *dname;
	librpz_result_t result;
	uint32_t ttl;
	trpz_rr_t *rrs;
	size_t nrrs, rridx;
	librpz_policy_t poverride, hidden_policy;
	unsigned long flags;
	librpz_trig_t match_trig;
} trpz_result_t;

#define DECL_NODE(canon, name, policy, znum, trig) \
	{ canon, name, { 0, 0, policy, policy, znum, znum, trig, true } },

#define NUM_ZONES_SNAPSHOT1 20

extern const rpz_soa_t g_soa_record;

#define WDNS_PRESLEN_NAME 1025
extern size_t
wdns_domain_to_str(const uint8_t *src, size_t src_len, char *dst);
extern int
wdns_str_to_name(const char *str, uint8_t **pbuf, bool downcase);

extern void
reverse_labels(const char *str, char *pbuf);

extern rpz_soa_t *
parse_serial(unsigned char *rdata, size_t rdlen);

extern int
load_all_updates(const char *fname, trpz_result_t **presults, size_t *pnresults,
		 trpz_zone_t **pzones, size_t *pnzones, char **errp);
extern int
apply_update(const char *updstr, trpz_result_t **presults, size_t *pnresults,
	     trpz_zone_t **pzones, size_t *pnzones, int is_static,
	     unsigned long flags, char **errp);
extern int
sanity_check_data_file(const char *fname, char **errp);

extern unsigned long
parse_zone_options(const char *str);

extern int
get_address_info(const char *astr, int *pfamily, char *pbuf,
		 const char *optname, char **errp);
