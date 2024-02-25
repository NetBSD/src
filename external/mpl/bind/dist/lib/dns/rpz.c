/*	$NetBSD: rpz.c,v 1.11.2.2 2024/02/25 15:46:52 martin Exp $	*/

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
#include <stdlib.h>

#include <isc/buffer.h>
#include <isc/magic.h>
#include <isc/mem.h>
#include <isc/net.h>
#include <isc/netaddr.h>
#include <isc/print.h>
#include <isc/result.h>
#include <isc/rwlock.h>
#include <isc/string.h>
#include <isc/task.h>
#include <isc/util.h>

#include <dns/db.h>
#include <dns/dbiterator.h>
#include <dns/dnsrps.h>
#include <dns/events.h>
#include <dns/fixedname.h>
#include <dns/log.h>
#include <dns/rbt.h>
#include <dns/rdata.h>
#include <dns/rdataset.h>
#include <dns/rdatasetiter.h>
#include <dns/rdatastruct.h>
#include <dns/rpz.h>
#include <dns/view.h>

#define DNS_RPZ_ZONE_MAGIC  ISC_MAGIC('r', 'p', 'z', ' ')
#define DNS_RPZ_ZONES_MAGIC ISC_MAGIC('r', 'p', 'z', 's')

#define DNS_RPZ_ZONE_VALID(rpz)	  ISC_MAGIC_VALID(rpz, DNS_RPZ_ZONE_MAGIC)
#define DNS_RPZ_ZONES_VALID(rpzs) ISC_MAGIC_VALID(rpzs, DNS_RPZ_ZONES_MAGIC)

/*
 * Parallel radix trees for databases of response policy IP addresses
 *
 * The radix or patricia trees are somewhat specialized to handle response
 * policy addresses by representing the two sets of IP addresses and name
 * server IP addresses in a single tree.  One set of IP addresses is
 * for rpz-ip policies or policies triggered by addresses in A or
 * AAAA records in responses.
 * The second set is for rpz-nsip policies or policies triggered by addresses
 * in A or AAAA records for NS records that are authorities for responses.
 *
 * Each leaf indicates that an IP address is listed in the IP address or the
 * name server IP address policy sub-zone (or both) of the corresponding
 * response policy zone.  The policy data such as a CNAME or an A record
 * is kept in the policy zone.  After an IP address has been found in a radix
 * tree, the node in the policy zone's database is found by converting
 * the IP address to a domain name in a canonical form.
 *
 *
 * The response policy zone canonical form of an IPv6 address is one of:
 *	prefix.W.W.W.W.W.W.W.W
 *	prefix.WORDS.zz
 *	prefix.WORDS.zz.WORDS
 *	prefix.zz.WORDS
 *  where
 *	prefix	is the prefix length of the IPv6 address between 1 and 128
 *	W	is a number between 0 and 65535
 *	WORDS	is one or more numbers W separated with "."
 *	zz	corresponds to :: in the standard IPv6 text representation
 *
 * The canonical form of IPv4 addresses is:
 *	prefix.B.B.B.B
 *  where
 *	prefix	is the prefix length of the address between 1 and 32
 *	B	is a number between 0 and 255
 *
 * Names for IPv4 addresses are distinguished from IPv6 addresses by having
 * 5 labels all of which are numbers, and a prefix between 1 and 32.
 */

/*
 * Nodes hashtable calculation parameters
 */
#define DNS_RPZ_HTSIZE_MAX 24
#define DNS_RPZ_HTSIZE_DIV 3

static isc_result_t
dns__rpz_shuttingdown(dns_rpz_zones_t *rpzs);
static void
dns__rpz_timer_cb(isc_task_t *task, isc_event_t *event);

/*
 * Use a private definition of IPv6 addresses because s6_addr32 is not
 * always defined and our IPv6 addresses are in non-standard byte order
 */
typedef uint32_t dns_rpz_cidr_word_t;
#define DNS_RPZ_CIDR_WORD_BITS ((int)sizeof(dns_rpz_cidr_word_t) * 8)
#define DNS_RPZ_CIDR_KEY_BITS  ((int)sizeof(dns_rpz_cidr_key_t) * 8)
#define DNS_RPZ_CIDR_WORDS     (128 / DNS_RPZ_CIDR_WORD_BITS)
typedef struct {
	dns_rpz_cidr_word_t w[DNS_RPZ_CIDR_WORDS];
} dns_rpz_cidr_key_t;

#define ADDR_V4MAPPED 0xffff
#define KEY_IS_IPV4(prefix, ip)                                  \
	((prefix) >= 96 && (ip)->w[0] == 0 && (ip)->w[1] == 0 && \
	 (ip)->w[2] == ADDR_V4MAPPED)

#define DNS_RPZ_WORD_MASK(b)                   \
	((b) == 0 ? (dns_rpz_cidr_word_t)(-1)  \
		  : ((dns_rpz_cidr_word_t)(-1) \
		     << (DNS_RPZ_CIDR_WORD_BITS - (b))))

/*
 * Get bit #n from the array of words of an IP address.
 */
#define DNS_RPZ_IP_BIT(ip, n)                          \
	(1 & ((ip)->w[(n) / DNS_RPZ_CIDR_WORD_BITS] >> \
	      (DNS_RPZ_CIDR_WORD_BITS - 1 - ((n) % DNS_RPZ_CIDR_WORD_BITS))))

/*
 * A triplet of arrays of bits flagging the existence of
 * client-IP, IP, and NSIP policy triggers.
 */
typedef struct dns_rpz_addr_zbits dns_rpz_addr_zbits_t;
struct dns_rpz_addr_zbits {
	dns_rpz_zbits_t client_ip;
	dns_rpz_zbits_t ip;
	dns_rpz_zbits_t nsip;
};

/*
 * A CIDR or radix tree node.
 */
struct dns_rpz_cidr_node {
	dns_rpz_cidr_node_t *parent;
	dns_rpz_cidr_node_t *child[2];
	dns_rpz_cidr_key_t ip;
	dns_rpz_prefix_t prefix;
	dns_rpz_addr_zbits_t set;
	dns_rpz_addr_zbits_t sum;
};

/*
 * A pair of arrays of bits flagging the existence of
 * QNAME and NSDNAME policy triggers.
 */
typedef struct dns_rpz_nm_zbits dns_rpz_nm_zbits_t;
struct dns_rpz_nm_zbits {
	dns_rpz_zbits_t qname;
	dns_rpz_zbits_t ns;
};

/*
 * The data in a RBT node has two pairs of bits for policy zones.
 * One pair is for the corresponding name of the node such as example.com
 * and the other pair is for a wildcard child such as *.example.com.
 */
typedef struct dns_rpz_nm_data dns_rpz_nm_data_t;
struct dns_rpz_nm_data {
	dns_rpz_nm_zbits_t set;
	dns_rpz_nm_zbits_t wild;
};

static isc_result_t
rpz_add(dns_rpz_zone_t *rpz, const dns_name_t *src_name);
static void
rpz_del(dns_rpz_zone_t *rpz, const dns_name_t *src_name);

const char *
dns_rpz_type2str(dns_rpz_type_t type) {
	switch (type) {
	case DNS_RPZ_TYPE_CLIENT_IP:
		return ("CLIENT-IP");
	case DNS_RPZ_TYPE_QNAME:
		return ("QNAME");
	case DNS_RPZ_TYPE_IP:
		return ("IP");
	case DNS_RPZ_TYPE_NSIP:
		return ("NSIP");
	case DNS_RPZ_TYPE_NSDNAME:
		return ("NSDNAME");
	case DNS_RPZ_TYPE_BAD:
		break;
	}
	FATAL_ERROR("impossible rpz type %d", type);
	return ("impossible");
}

dns_rpz_policy_t
dns_rpz_str2policy(const char *str) {
	static struct {
		const char *str;
		dns_rpz_policy_t policy;
	} tbl[] = {
		{ "given", DNS_RPZ_POLICY_GIVEN },
		{ "disabled", DNS_RPZ_POLICY_DISABLED },
		{ "passthru", DNS_RPZ_POLICY_PASSTHRU },
		{ "drop", DNS_RPZ_POLICY_DROP },
		{ "tcp-only", DNS_RPZ_POLICY_TCP_ONLY },
		{ "nxdomain", DNS_RPZ_POLICY_NXDOMAIN },
		{ "nodata", DNS_RPZ_POLICY_NODATA },
		{ "cname", DNS_RPZ_POLICY_CNAME },
		{ "no-op", DNS_RPZ_POLICY_PASSTHRU }, /* old passthru */
	};
	unsigned int n;

	if (str == NULL) {
		return (DNS_RPZ_POLICY_ERROR);
	}
	for (n = 0; n < sizeof(tbl) / sizeof(tbl[0]); ++n) {
		if (!strcasecmp(tbl[n].str, str)) {
			return (tbl[n].policy);
		}
	}
	return (DNS_RPZ_POLICY_ERROR);
}

const char *
dns_rpz_policy2str(dns_rpz_policy_t policy) {
	const char *str = NULL;

	switch (policy) {
	case DNS_RPZ_POLICY_PASSTHRU:
		str = "PASSTHRU";
		break;
	case DNS_RPZ_POLICY_DROP:
		str = "DROP";
		break;
	case DNS_RPZ_POLICY_TCP_ONLY:
		str = "TCP-ONLY";
		break;
	case DNS_RPZ_POLICY_NXDOMAIN:
		str = "NXDOMAIN";
		break;
	case DNS_RPZ_POLICY_NODATA:
		str = "NODATA";
		break;
	case DNS_RPZ_POLICY_RECORD:
		str = "Local-Data";
		break;
	case DNS_RPZ_POLICY_CNAME:
	case DNS_RPZ_POLICY_WILDCNAME:
		str = "CNAME";
		break;
	case DNS_RPZ_POLICY_MISS:
		str = "MISS";
		break;
	case DNS_RPZ_POLICY_DNS64:
		str = "DNS64";
		break;
	case DNS_RPZ_POLICY_ERROR:
		str = "ERROR";
		break;
	default:
		UNREACHABLE();
	}
	return (str);
}

/*
 * Return the bit number of the highest set bit in 'zbit'.
 * (for example, 0x01 returns 0, 0xFF returns 7, etc.)
 */
static int
zbit_to_num(dns_rpz_zbits_t zbit) {
	dns_rpz_num_t rpz_num;

	REQUIRE(zbit != 0);
	rpz_num = 0;
	if ((zbit & 0xffffffff00000000ULL) != 0) {
		zbit >>= 32;
		rpz_num += 32;
	}
	if ((zbit & 0xffff0000) != 0) {
		zbit >>= 16;
		rpz_num += 16;
	}
	if ((zbit & 0xff00) != 0) {
		zbit >>= 8;
		rpz_num += 8;
	}
	if ((zbit & 0xf0) != 0) {
		zbit >>= 4;
		rpz_num += 4;
	}
	if ((zbit & 0xc) != 0) {
		zbit >>= 2;
		rpz_num += 2;
	}
	if ((zbit & 2) != 0) {
		++rpz_num;
	}
	return (rpz_num);
}

/*
 * Make a set of bit masks given one or more bits and their type.
 */
static void
make_addr_set(dns_rpz_addr_zbits_t *tgt_set, dns_rpz_zbits_t zbits,
	      dns_rpz_type_t type) {
	switch (type) {
	case DNS_RPZ_TYPE_CLIENT_IP:
		tgt_set->client_ip = zbits;
		tgt_set->ip = 0;
		tgt_set->nsip = 0;
		break;
	case DNS_RPZ_TYPE_IP:
		tgt_set->client_ip = 0;
		tgt_set->ip = zbits;
		tgt_set->nsip = 0;
		break;
	case DNS_RPZ_TYPE_NSIP:
		tgt_set->client_ip = 0;
		tgt_set->ip = 0;
		tgt_set->nsip = zbits;
		break;
	default:
		UNREACHABLE();
	}
}

static void
make_nm_set(dns_rpz_nm_zbits_t *tgt_set, dns_rpz_num_t rpz_num,
	    dns_rpz_type_t type) {
	switch (type) {
	case DNS_RPZ_TYPE_QNAME:
		tgt_set->qname = DNS_RPZ_ZBIT(rpz_num);
		tgt_set->ns = 0;
		break;
	case DNS_RPZ_TYPE_NSDNAME:
		tgt_set->qname = 0;
		tgt_set->ns = DNS_RPZ_ZBIT(rpz_num);
		break;
	default:
		UNREACHABLE();
	}
}

/*
 * Mark a node and all of its parents as having client-IP, IP, or NSIP data
 */
static void
set_sum_pair(dns_rpz_cidr_node_t *cnode) {
	dns_rpz_addr_zbits_t sum;

	do {
		dns_rpz_cidr_node_t *child = cnode->child[0];
		sum = cnode->set;

		if (child != NULL) {
			sum.client_ip |= child->sum.client_ip;
			sum.ip |= child->sum.ip;
			sum.nsip |= child->sum.nsip;
		}

		child = cnode->child[1];
		if (child != NULL) {
			sum.client_ip |= child->sum.client_ip;
			sum.ip |= child->sum.ip;
			sum.nsip |= child->sum.nsip;
		}

		if (cnode->sum.client_ip == sum.client_ip &&
		    cnode->sum.ip == sum.ip && cnode->sum.nsip == sum.nsip)
		{
			break;
		}
		cnode->sum = sum;
		cnode = cnode->parent;
	} while (cnode != NULL);
}

/* Caller must hold rpzs->maint_lock */
static void
fix_qname_skip_recurse(dns_rpz_zones_t *rpzs) {
	dns_rpz_zbits_t mask;

	/*
	 * qname_wait_recurse and qname_skip_recurse are used to
	 * implement the "qname-wait-recurse" config option.
	 *
	 * When "qname-wait-recurse" is yes, no processing happens without
	 * recursion. In this case, qname_wait_recurse is true, and
	 * qname_skip_recurse (a bit field indicating which policy zones
	 * can be processed without recursion) is set to all 0's by
	 * fix_qname_skip_recurse().
	 *
	 * When "qname-wait-recurse" is no, qname_skip_recurse may be
	 * set to a non-zero value by fix_qname_skip_recurse(). The mask
	 * has to have bits set for the policy zones for which
	 * processing may continue without recursion, and bits cleared
	 * for the rest.
	 *
	 * (1) The ARM says:
	 *
	 *   The "qname-wait-recurse no" option overrides that default
	 *   behavior when recursion cannot change a non-error
	 *   response. The option does not affect QNAME or client-IP
	 *   triggers in policy zones listed after other zones
	 *   containing IP, NSIP and NSDNAME triggers, because those may
	 *   depend on the A, AAAA, and NS records that would be found
	 *   during recursive resolution.
	 *
	 * Let's consider the following:
	 *
	 *     zbits_req = (rpzs->have.ipv4 | rpzs->have.ipv6 |
	 *		    rpzs->have.nsdname |
	 *		    rpzs->have.nsipv4 | rpzs->have.nsipv6);
	 *
	 * zbits_req now contains bits set for zones which require
	 * recursion.
	 *
	 * But going by the description in the ARM, if the first policy
	 * zone requires recursion, then all zones after that (higher
	 * order bits) have to wait as well.  If the Nth zone requires
	 * recursion, then (N+1)th zone onwards all need to wait.
	 *
	 * So mapping this, examples:
	 *
	 * zbits_req = 0b000  mask = 0xffffffff (no zones have to wait for
	 *					 recursion)
	 * zbits_req = 0b001  mask = 0x00000000 (all zones have to wait)
	 * zbits_req = 0b010  mask = 0x00000001 (the first zone doesn't have to
	 *					 wait, second zone onwards need
	 *					 to wait)
	 * zbits_req = 0b011  mask = 0x00000000 (all zones have to wait)
	 * zbits_req = 0b100  mask = 0x00000011 (the 1st and 2nd zones don't
	 *					 have to wait, third zone
	 *					 onwards need to wait)
	 *
	 * More generally, we have to count the number of trailing 0
	 * bits in zbits_req and only these can be processed without
	 * recursion. All the rest need to wait.
	 *
	 * (2) The ARM says that "qname-wait-recurse no" option
	 * overrides the default behavior when recursion cannot change a
	 * non-error response. So, in the order of listing of policy
	 * zones, within the first policy zone where recursion may be
	 * required, we should first allow CLIENT-IP and QNAME policy
	 * records to be attempted without recursion.
	 */

	/*
	 * Get a mask covering all policy zones that are not subordinate to
	 * other policy zones containing triggers that require that the
	 * qname be resolved before they can be checked.
	 */
	rpzs->have.client_ip = rpzs->have.client_ipv4 | rpzs->have.client_ipv6;
	rpzs->have.ip = rpzs->have.ipv4 | rpzs->have.ipv6;
	rpzs->have.nsip = rpzs->have.nsipv4 | rpzs->have.nsipv6;

	if (rpzs->p.qname_wait_recurse) {
		mask = 0;
	} else {
		dns_rpz_zbits_t zbits_req;
		dns_rpz_zbits_t zbits_notreq;
		dns_rpz_zbits_t mask2;
		dns_rpz_zbits_t req_mask;

		/*
		 * Get the masks of zones with policies that
		 * do/don't require recursion
		 */

		zbits_req = (rpzs->have.ipv4 | rpzs->have.ipv6 |
			     rpzs->have.nsdname | rpzs->have.nsipv4 |
			     rpzs->have.nsipv6);
		zbits_notreq = (rpzs->have.client_ip | rpzs->have.qname);

		if (zbits_req == 0) {
			mask = DNS_RPZ_ALL_ZBITS;
			goto set;
		}

		/*
		 * req_mask is a mask covering used bits in
		 * zbits_req. (For instance, 0b1 => 0b1, 0b101 => 0b111,
		 * 0b11010101 => 0b11111111).
		 */
		req_mask = zbits_req;
		req_mask |= req_mask >> 1;
		req_mask |= req_mask >> 2;
		req_mask |= req_mask >> 4;
		req_mask |= req_mask >> 8;
		req_mask |= req_mask >> 16;
		req_mask |= req_mask >> 32;

		/*
		 * There's no point in skipping recursion for a later
		 * zone if it is required in a previous zone.
		 */
		if ((zbits_notreq & req_mask) == 0) {
			mask = 0;
			goto set;
		}

		/*
		 * This bit arithmetic creates a mask of zones in which
		 * it is okay to skip recursion. After the first zone
		 * that has to wait for recursion, all the others have
		 * to wait as well, so we want to create a mask in which
		 * all the trailing zeroes in zbits_req are are 1, and
		 * more significant bits are 0. (For instance,
		 * 0x0700 => 0x00ff, 0x0007 => 0x0000)
		 */
		mask = ~(zbits_req | ((~zbits_req) + 1));

		/*
		 * As mentioned in (2) above, the zone corresponding to
		 * the least significant zero could have its CLIENT-IP
		 * and QNAME policies checked before recursion, if it
		 * has any of those policies.  So if it does, we
		 * can set its 0 to 1.
		 *
		 * Locate the least significant 0 bit in the mask (for
		 * instance, 0xff => 0x100)...
		 */
		mask2 = (mask << 1) & ~mask;

		/*
		 * Also set the bit for zone 0, because if it's in
		 * zbits_notreq then it's definitely okay to attempt to
		 * skip recursion for zone 0...
		 */
		mask2 |= 1;

		/* Clear any bits *not* in zbits_notreq... */
		mask2 &= zbits_notreq;

		/* And merge the result into the skip-recursion mask */
		mask |= mask2;
	}

set:
	isc_log_write(dns_lctx, DNS_LOGCATEGORY_RPZ, DNS_LOGMODULE_RBTDB,
		      DNS_RPZ_DEBUG_QUIET,
		      "computed RPZ qname_skip_recurse mask=0x%" PRIx64,
		      (uint64_t)mask);
	rpzs->have.qname_skip_recurse = mask;
}

static void
adj_trigger_cnt(dns_rpz_zone_t *rpz, dns_rpz_type_t rpz_type,
		const dns_rpz_cidr_key_t *tgt_ip, dns_rpz_prefix_t tgt_prefix,
		bool inc) {
	dns_rpz_trigger_counter_t *cnt = NULL;
	dns_rpz_zbits_t *have = NULL;

	switch (rpz_type) {
	case DNS_RPZ_TYPE_CLIENT_IP:
		REQUIRE(tgt_ip != NULL);
		if (KEY_IS_IPV4(tgt_prefix, tgt_ip)) {
			cnt = &rpz->rpzs->triggers[rpz->num].client_ipv4;
			have = &rpz->rpzs->have.client_ipv4;
		} else {
			cnt = &rpz->rpzs->triggers[rpz->num].client_ipv6;
			have = &rpz->rpzs->have.client_ipv6;
		}
		break;
	case DNS_RPZ_TYPE_QNAME:
		cnt = &rpz->rpzs->triggers[rpz->num].qname;
		have = &rpz->rpzs->have.qname;
		break;
	case DNS_RPZ_TYPE_IP:
		REQUIRE(tgt_ip != NULL);
		if (KEY_IS_IPV4(tgt_prefix, tgt_ip)) {
			cnt = &rpz->rpzs->triggers[rpz->num].ipv4;
			have = &rpz->rpzs->have.ipv4;
		} else {
			cnt = &rpz->rpzs->triggers[rpz->num].ipv6;
			have = &rpz->rpzs->have.ipv6;
		}
		break;
	case DNS_RPZ_TYPE_NSDNAME:
		cnt = &rpz->rpzs->triggers[rpz->num].nsdname;
		have = &rpz->rpzs->have.nsdname;
		break;
	case DNS_RPZ_TYPE_NSIP:
		REQUIRE(tgt_ip != NULL);
		if (KEY_IS_IPV4(tgt_prefix, tgt_ip)) {
			cnt = &rpz->rpzs->triggers[rpz->num].nsipv4;
			have = &rpz->rpzs->have.nsipv4;
		} else {
			cnt = &rpz->rpzs->triggers[rpz->num].nsipv6;
			have = &rpz->rpzs->have.nsipv6;
		}
		break;
	default:
		UNREACHABLE();
	}

	if (inc) {
		if (++*cnt == 1U) {
			*have |= DNS_RPZ_ZBIT(rpz->num);
			fix_qname_skip_recurse(rpz->rpzs);
		}
	} else {
		REQUIRE(*cnt != 0U);
		if (--*cnt == 0U) {
			*have &= ~DNS_RPZ_ZBIT(rpz->num);
			fix_qname_skip_recurse(rpz->rpzs);
		}
	}
}

static dns_rpz_cidr_node_t *
new_node(dns_rpz_zones_t *rpzs, const dns_rpz_cidr_key_t *ip,
	 dns_rpz_prefix_t prefix, const dns_rpz_cidr_node_t *child) {
	dns_rpz_cidr_node_t *node = NULL;
	int i, words, wlen;

	node = isc_mem_get(rpzs->mctx, sizeof(*node));
	*node = (dns_rpz_cidr_node_t){
		.prefix = prefix,
	};

	if (child != NULL) {
		node->sum = child->sum;
	}

	words = prefix / DNS_RPZ_CIDR_WORD_BITS;
	wlen = prefix % DNS_RPZ_CIDR_WORD_BITS;
	i = 0;
	while (i < words) {
		node->ip.w[i] = ip->w[i];
		++i;
	}
	if (wlen != 0) {
		node->ip.w[i] = ip->w[i] & DNS_RPZ_WORD_MASK(wlen);
		++i;
	}
	while (i < DNS_RPZ_CIDR_WORDS) {
		node->ip.w[i++] = 0;
	}

	return (node);
}

static void
badname(int level, const dns_name_t *name, const char *str1, const char *str2) {
	/*
	 * bin/tests/system/rpz/tests.sh looks for "invalid rpz".
	 */
	if (level < DNS_RPZ_DEBUG_QUIET && isc_log_wouldlog(dns_lctx, level)) {
		char namebuf[DNS_NAME_FORMATSIZE];
		dns_name_format(name, namebuf, sizeof(namebuf));
		isc_log_write(dns_lctx, DNS_LOGCATEGORY_RPZ,
			      DNS_LOGMODULE_RBTDB, level,
			      "invalid rpz IP address \"%s\"%s%s", namebuf,
			      str1, str2);
	}
}

/*
 * Convert an IP address from radix tree binary (host byte order) to
 * to its canonical response policy domain name without the origin of the
 * policy zone.
 *
 * Generate a name for an IPv6 address that fits RFC 5952, except that our
 * reversed format requires that when the length of the consecutive 16-bit
 * 0 fields are equal (e.g., 1.0.0.1.0.0.db8.2001 corresponding to
 * 2001:db8:0:0:1:0:0:1), we shorted the last instead of the first
 * (e.g., 1.0.0.1.zz.db8.2001 corresponding to 2001:db8::1:0:0:1).
 */
static isc_result_t
ip2name(const dns_rpz_cidr_key_t *tgt_ip, dns_rpz_prefix_t tgt_prefix,
	const dns_name_t *base_name, dns_name_t *ip_name) {
#ifndef INET6_ADDRSTRLEN
#define INET6_ADDRSTRLEN 46
#endif /* ifndef INET6_ADDRSTRLEN */
	char str[1 + 8 + 1 + INET6_ADDRSTRLEN + 1];
	isc_buffer_t buffer;
	isc_result_t result;
	int len;

	if (KEY_IS_IPV4(tgt_prefix, tgt_ip)) {
		len = snprintf(str, sizeof(str), "%u.%u.%u.%u.%u",
			       tgt_prefix - 96U, tgt_ip->w[3] & 0xffU,
			       (tgt_ip->w[3] >> 8) & 0xffU,
			       (tgt_ip->w[3] >> 16) & 0xffU,
			       (tgt_ip->w[3] >> 24) & 0xffU);
		if (len < 0 || (size_t)len >= sizeof(str)) {
			return (ISC_R_FAILURE);
		}
	} else {
		int w[DNS_RPZ_CIDR_WORDS * 2];
		int best_first, best_len, cur_first, cur_len;

		len = snprintf(str, sizeof(str), "%d", tgt_prefix);
		if (len < 0 || (size_t)len >= sizeof(str)) {
			return (ISC_R_FAILURE);
		}

		for (int n = 0; n < DNS_RPZ_CIDR_WORDS; n++) {
			w[n * 2 + 1] =
				((tgt_ip->w[DNS_RPZ_CIDR_WORDS - 1 - n] >> 16) &
				 0xffff);
			w[n * 2] = tgt_ip->w[DNS_RPZ_CIDR_WORDS - 1 - n] &
				   0xffff;
		}
		/*
		 * Find the start and length of the first longest sequence
		 * of zeros in the address.
		 */
		best_first = -1;
		best_len = 0;
		cur_first = -1;
		cur_len = 0;
		for (int n = 0; n <= 7; ++n) {
			if (w[n] != 0) {
				cur_len = 0;
				cur_first = -1;
			} else {
				++cur_len;
				if (cur_first < 0) {
					cur_first = n;
				} else if (cur_len >= best_len) {
					best_first = cur_first;
					best_len = cur_len;
				}
			}
		}

		for (int n = 0; n <= 7; ++n) {
			int i;

			INSIST(len > 0 && (size_t)len < sizeof(str));
			if (n == best_first) {
				i = snprintf(str + len, sizeof(str) - len,
					     ".zz");
				n += best_len - 1;
			} else {
				i = snprintf(str + len, sizeof(str) - len,
					     ".%x", w[n]);
			}
			if (i < 0 || (size_t)i >= (size_t)(sizeof(str) - len)) {
				return (ISC_R_FAILURE);
			}
			len += i;
		}
	}

	isc_buffer_init(&buffer, str, sizeof(str));
	isc_buffer_add(&buffer, len);
	result = dns_name_fromtext(ip_name, &buffer, base_name, 0, NULL);
	return (result);
}

/*
 * Determine the type of a name in a response policy zone.
 */
static dns_rpz_type_t
type_from_name(const dns_rpz_zones_t *rpzs, dns_rpz_zone_t *rpz,
	       const dns_name_t *name) {
	if (dns_name_issubdomain(name, &rpz->ip)) {
		return (DNS_RPZ_TYPE_IP);
	}

	if (dns_name_issubdomain(name, &rpz->client_ip)) {
		return (DNS_RPZ_TYPE_CLIENT_IP);
	}

	if ((rpzs->p.nsip_on & DNS_RPZ_ZBIT(rpz->num)) != 0 &&
	    dns_name_issubdomain(name, &rpz->nsip))
	{
		return (DNS_RPZ_TYPE_NSIP);
	}

	if ((rpzs->p.nsdname_on & DNS_RPZ_ZBIT(rpz->num)) != 0 &&
	    dns_name_issubdomain(name, &rpz->nsdname))
	{
		return (DNS_RPZ_TYPE_NSDNAME);
	}

	return (DNS_RPZ_TYPE_QNAME);
}

/*
 * Convert an IP address from canonical response policy domain name form
 * to radix tree binary (host byte order) for adding or deleting IP or NSIP
 * data.
 */
static isc_result_t
name2ipkey(int log_level, dns_rpz_zone_t *rpz, dns_rpz_type_t rpz_type,
	   const dns_name_t *src_name, dns_rpz_cidr_key_t *tgt_ip,
	   dns_rpz_prefix_t *tgt_prefix, dns_rpz_addr_zbits_t *new_set) {
	char ip_str[DNS_NAME_FORMATSIZE];
	dns_offsets_t ip_name_offsets;
	dns_fixedname_t ip_name2f;
	dns_name_t ip_name;
	const char *prefix_str = NULL, *cp = NULL, *end = NULL;
	char *cp2;
	int ip_labels;
	dns_rpz_prefix_t prefix;
	unsigned long prefix_num, l;
	isc_result_t result;
	int i;

	REQUIRE(rpz != NULL);
	REQUIRE(rpz->rpzs != NULL && rpz->num < rpz->rpzs->p.num_zones);

	make_addr_set(new_set, DNS_RPZ_ZBIT(rpz->num), rpz_type);

	ip_labels = dns_name_countlabels(src_name);
	if (rpz_type == DNS_RPZ_TYPE_QNAME) {
		ip_labels -= dns_name_countlabels(&rpz->origin);
	} else {
		ip_labels -= dns_name_countlabels(&rpz->nsdname);
	}
	if (ip_labels < 2) {
		badname(log_level, src_name, "; too short", "");
		return (ISC_R_FAILURE);
	}
	dns_name_init(&ip_name, ip_name_offsets);
	dns_name_getlabelsequence(src_name, 0, ip_labels, &ip_name);

	/*
	 * Get text for the IP address
	 */
	dns_name_format(&ip_name, ip_str, sizeof(ip_str));
	end = &ip_str[strlen(ip_str) + 1];
	prefix_str = ip_str;

	prefix_num = strtoul(prefix_str, &cp2, 10);
	if (*cp2 != '.') {
		badname(log_level, src_name, "; invalid leading prefix length",
			"");
		return (ISC_R_FAILURE);
	}
	/*
	 * Patch in trailing nul character to print just the length
	 * label (for various cases below).
	 */
	*cp2 = '\0';
	if (prefix_num < 1U || prefix_num > 128U) {
		badname(log_level, src_name, "; invalid prefix length of ",
			prefix_str);
		return (ISC_R_FAILURE);
	}
	cp = cp2 + 1;

	if (--ip_labels == 4 && !strchr(cp, 'z')) {
		/*
		 * Convert an IPv4 address
		 * from the form "prefix.z.y.x.w"
		 */
		if (prefix_num > 32U) {
			badname(log_level, src_name,
				"; invalid IPv4 prefix length of ", prefix_str);
			return (ISC_R_FAILURE);
		}
		prefix_num += 96;
		*tgt_prefix = (dns_rpz_prefix_t)prefix_num;
		tgt_ip->w[0] = 0;
		tgt_ip->w[1] = 0;
		tgt_ip->w[2] = ADDR_V4MAPPED;
		tgt_ip->w[3] = 0;
		for (i = 0; i < 32; i += 8) {
			l = strtoul(cp, &cp2, 10);
			if (l > 255U || (*cp2 != '.' && *cp2 != '\0')) {
				if (*cp2 == '.') {
					*cp2 = '\0';
				}
				badname(log_level, src_name,
					"; invalid IPv4 octet ", cp);
				return (ISC_R_FAILURE);
			}
			tgt_ip->w[3] |= l << i;
			cp = cp2 + 1;
		}
	} else {
		/*
		 * Convert a text IPv6 address.
		 */
		*tgt_prefix = (dns_rpz_prefix_t)prefix_num;
		for (i = 0; ip_labels > 0 && i < DNS_RPZ_CIDR_WORDS * 2;
		     ip_labels--)
		{
			if (cp[0] == 'z' && cp[1] == 'z' &&
			    (cp[2] == '.' || cp[2] == '\0') && i <= 6)
			{
				do {
					if ((i & 1) == 0) {
						tgt_ip->w[3 - i / 2] = 0;
					}
					++i;
				} while (ip_labels + i <= 8);
				cp += 3;
			} else {
				l = strtoul(cp, &cp2, 16);
				if (l > 0xffffu ||
				    (*cp2 != '.' && *cp2 != '\0'))
				{
					if (*cp2 == '.') {
						*cp2 = '\0';
					}
					badname(log_level, src_name,
						"; invalid IPv6 word ", cp);
					return (ISC_R_FAILURE);
				}
				if ((i & 1) == 0) {
					tgt_ip->w[3 - i / 2] = l;
				} else {
					tgt_ip->w[3 - i / 2] |= l << 16;
				}
				i++;
				cp = cp2 + 1;
			}
		}
	}
	if (cp != end) {
		badname(log_level, src_name, "", "");
		return (ISC_R_FAILURE);
	}

	/*
	 * Check for 1s after the prefix length.
	 */
	prefix = (dns_rpz_prefix_t)prefix_num;
	while (prefix < DNS_RPZ_CIDR_KEY_BITS) {
		dns_rpz_cidr_word_t aword;

		i = prefix % DNS_RPZ_CIDR_WORD_BITS;
		aword = tgt_ip->w[prefix / DNS_RPZ_CIDR_WORD_BITS];
		if ((aword & ~DNS_RPZ_WORD_MASK(i)) != 0) {
			badname(log_level, src_name,
				"; too small prefix length of ", prefix_str);
			return (ISC_R_FAILURE);
		}
		prefix -= i;
		prefix += DNS_RPZ_CIDR_WORD_BITS;
	}

	/*
	 * Complain about bad names but be generous and accept them.
	 */
	if (log_level < DNS_RPZ_DEBUG_QUIET &&
	    isc_log_wouldlog(dns_lctx, log_level))
	{
		/*
		 * Convert the address back to a canonical domain name
		 * to ensure that the original name is in canonical form.
		 */
		dns_name_t *ip_name2 = dns_fixedname_initname(&ip_name2f);
		result = ip2name(tgt_ip, (dns_rpz_prefix_t)prefix_num, NULL,
				 ip_name2);
		if (result != ISC_R_SUCCESS ||
		    !dns_name_equal(&ip_name, ip_name2))
		{
			char ip2_str[DNS_NAME_FORMATSIZE];
			dns_name_format(ip_name2, ip2_str, sizeof(ip2_str));
			isc_log_write(dns_lctx, DNS_LOGCATEGORY_RPZ,
				      DNS_LOGMODULE_RBTDB, log_level,
				      "rpz IP address \"%s\""
				      " is not the canonical \"%s\"",
				      ip_str, ip2_str);
		}
	}

	return (ISC_R_SUCCESS);
}

/*
 * Get trigger name and data bits for adding or deleting summary NSDNAME
 * or QNAME data.
 */
static void
name2data(dns_rpz_zone_t *rpz, dns_rpz_type_t rpz_type,
	  const dns_name_t *src_name, dns_name_t *trig_name,
	  dns_rpz_nm_data_t *new_data) {
	dns_offsets_t tmp_name_offsets;
	dns_name_t tmp_name;
	unsigned int prefix_len, n;

	REQUIRE(rpz != NULL);
	REQUIRE(rpz->rpzs != NULL && rpz->num < rpz->rpzs->p.num_zones);

	/*
	 * Handle wildcards by putting only the parent into the
	 * summary RBT.  The summary database only causes a check of the
	 * real policy zone where wildcards will be handled.
	 */
	if (dns_name_iswildcard(src_name)) {
		prefix_len = 1;
		memset(&new_data->set, 0, sizeof(new_data->set));
		make_nm_set(&new_data->wild, rpz->num, rpz_type);
	} else {
		prefix_len = 0;
		make_nm_set(&new_data->set, rpz->num, rpz_type);
		memset(&new_data->wild, 0, sizeof(new_data->wild));
	}

	dns_name_init(&tmp_name, tmp_name_offsets);
	n = dns_name_countlabels(src_name);
	n -= prefix_len;
	if (rpz_type == DNS_RPZ_TYPE_QNAME) {
		n -= dns_name_countlabels(&rpz->origin);
	} else {
		n -= dns_name_countlabels(&rpz->nsdname);
	}
	dns_name_getlabelsequence(src_name, prefix_len, n, &tmp_name);
	(void)dns_name_concatenate(&tmp_name, dns_rootname, trig_name, NULL);
}

#ifndef HAVE_BUILTIN_CLZ
/**
 * \brief Count Leading Zeros: Find the location of the left-most set
 * bit.
 */
static unsigned int
clz(dns_rpz_cidr_word_t w) {
	unsigned int bit;

	bit = DNS_RPZ_CIDR_WORD_BITS - 1;

	if ((w & 0xffff0000) != 0) {
		w >>= 16;
		bit -= 16;
	}

	if ((w & 0xff00) != 0) {
		w >>= 8;
		bit -= 8;
	}

	if ((w & 0xf0) != 0) {
		w >>= 4;
		bit -= 4;
	}

	if ((w & 0xc) != 0) {
		w >>= 2;
		bit -= 2;
	}

	if ((w & 2) != 0) {
		--bit;
	}

	return (bit);
}
#endif /* ifndef HAVE_BUILTIN_CLZ */

/*
 * Find the first differing bit in two keys (IP addresses).
 */
static int
diff_keys(const dns_rpz_cidr_key_t *key1, dns_rpz_prefix_t prefix1,
	  const dns_rpz_cidr_key_t *key2, dns_rpz_prefix_t prefix2) {
	dns_rpz_cidr_word_t delta;
	dns_rpz_prefix_t maxbit, bit;
	int i;

	bit = 0;
	maxbit = ISC_MIN(prefix1, prefix2);

	/*
	 * find the first differing words
	 */
	for (i = 0; bit < maxbit; i++, bit += DNS_RPZ_CIDR_WORD_BITS) {
		delta = key1->w[i] ^ key2->w[i];
		if (delta != 0) {
#ifdef HAVE_BUILTIN_CLZ
			bit += __builtin_clz(delta);
#else  /* ifdef HAVE_BUILTIN_CLZ */
			bit += clz(delta);
#endif /* ifdef HAVE_BUILTIN_CLZ */
			break;
		}
	}
	return (ISC_MIN(bit, maxbit));
}

/*
 * Given a hit while searching the radix trees,
 * clear all bits for higher numbered zones.
 */
static dns_rpz_zbits_t
trim_zbits(dns_rpz_zbits_t zbits, dns_rpz_zbits_t found) {
	dns_rpz_zbits_t x;

	/*
	 * Isolate the first or smallest numbered hit bit.
	 * Make a mask of that bit and all smaller numbered bits.
	 */
	x = zbits & found;
	x &= (~x + 1);
	x = (x << 1) - 1;
	zbits &= x;
	return (zbits);
}

/*
 * Search a radix tree for an IP address for ordinary lookup
 *	or for a CIDR block adding or deleting an entry
 *
 * Return ISC_R_SUCCESS, DNS_R_PARTIALMATCH, ISC_R_NOTFOUND,
 *	    and *found=longest match node
 *	or with create==true, ISC_R_EXISTS or ISC_R_NOMEMORY
 */
static isc_result_t
search(dns_rpz_zones_t *rpzs, const dns_rpz_cidr_key_t *tgt_ip,
       dns_rpz_prefix_t tgt_prefix, const dns_rpz_addr_zbits_t *tgt_set,
       bool create, dns_rpz_cidr_node_t **found) {
	dns_rpz_cidr_node_t *cur = NULL, *parent = NULL, *child = NULL;
	dns_rpz_cidr_node_t *new_parent = NULL, *sibling = NULL;
	dns_rpz_addr_zbits_t set;
	int cur_num, child_num;
	isc_result_t find_result;

	set = *tgt_set;
	find_result = ISC_R_NOTFOUND;
	*found = NULL;
	cur = rpzs->cidr;
	parent = NULL;
	cur_num = 0;
	for (;;) {
		dns_rpz_prefix_t dbit;
		if (cur == NULL) {
			/*
			 * No child so we cannot go down.
			 * Quit with whatever we already found
			 * or add the target as a child of the current parent.
			 */
			if (!create) {
				return (find_result);
			}
			child = new_node(rpzs, tgt_ip, tgt_prefix, NULL);
			if (child == NULL) {
				return (ISC_R_NOMEMORY);
			}
			if (parent == NULL) {
				rpzs->cidr = child;
			} else {
				parent->child[cur_num] = child;
			}
			child->parent = parent;
			child->set.client_ip |= tgt_set->client_ip;
			child->set.ip |= tgt_set->ip;
			child->set.nsip |= tgt_set->nsip;
			set_sum_pair(child);
			*found = child;
			return (ISC_R_SUCCESS);
		}

		if ((cur->sum.client_ip & set.client_ip) == 0 &&
		    (cur->sum.ip & set.ip) == 0 &&
		    (cur->sum.nsip & set.nsip) == 0)
		{
			/*
			 * This node has no relevant data
			 * and is in none of the target trees.
			 * Pretend it does not exist if we are not adding.
			 *
			 * If we are adding, continue down to eventually add
			 * a node and mark/put this node in the correct tree.
			 */
			if (!create) {
				return (find_result);
			}
		}

		dbit = diff_keys(tgt_ip, tgt_prefix, &cur->ip, cur->prefix);
		/*
		 * dbit <= tgt_prefix and dbit <= cur->prefix always.
		 * We are finished searching if we matched all of the target.
		 */
		if (dbit == tgt_prefix) {
			if (tgt_prefix == cur->prefix) {
				/*
				 * The node's key matches the target exactly.
				 */
				if ((cur->set.client_ip & set.client_ip) != 0 ||
				    (cur->set.ip & set.ip) != 0 ||
				    (cur->set.nsip & set.nsip) != 0)
				{
					/*
					 * It is the answer if it has data.
					 */
					*found = cur;
					if (create) {
						find_result = ISC_R_EXISTS;
					} else {
						find_result = ISC_R_SUCCESS;
					}
				} else if (create) {
					/*
					 * The node lacked relevant data,
					 * but will have it now.
					 */
					cur->set.client_ip |=
						tgt_set->client_ip;
					cur->set.ip |= tgt_set->ip;
					cur->set.nsip |= tgt_set->nsip;
					set_sum_pair(cur);
					*found = cur;
					find_result = ISC_R_SUCCESS;
				}
				return (find_result);
			}

			/*
			 * We know tgt_prefix < cur->prefix which means that
			 * the target is shorter than the current node.
			 * Add the target as the current node's parent.
			 */
			if (!create) {
				return (find_result);
			}

			new_parent = new_node(rpzs, tgt_ip, tgt_prefix, cur);
			if (new_parent == NULL) {
				return (ISC_R_NOMEMORY);
			}
			new_parent->parent = parent;
			if (parent == NULL) {
				rpzs->cidr = new_parent;
			} else {
				parent->child[cur_num] = new_parent;
			}
			child_num = DNS_RPZ_IP_BIT(&cur->ip, tgt_prefix);
			new_parent->child[child_num] = cur;
			cur->parent = new_parent;
			new_parent->set = *tgt_set;
			set_sum_pair(new_parent);
			*found = new_parent;
			return (ISC_R_SUCCESS);
		}

		if (dbit == cur->prefix) {
			if ((cur->set.client_ip & set.client_ip) != 0 ||
			    (cur->set.ip & set.ip) != 0 ||
			    (cur->set.nsip & set.nsip) != 0)
			{
				/*
				 * We have a partial match between of all of the
				 * current node but only part of the target.
				 * Continue searching for other hits in the
				 * same or lower numbered trees.
				 */
				find_result = DNS_R_PARTIALMATCH;
				*found = cur;
				set.client_ip = trim_zbits(set.client_ip,
							   cur->set.client_ip);
				set.ip = trim_zbits(set.ip, cur->set.ip);
				set.nsip = trim_zbits(set.nsip, cur->set.nsip);
			}
			parent = cur;
			cur_num = DNS_RPZ_IP_BIT(tgt_ip, dbit);
			cur = cur->child[cur_num];
			continue;
		}

		/*
		 * dbit < tgt_prefix and dbit < cur->prefix,
		 * so we failed to match both the target and the current node.
		 * Insert a fork of a parent above the current node and
		 * add the target as a sibling of the current node
		 */
		if (!create) {
			return (find_result);
		}

		sibling = new_node(rpzs, tgt_ip, tgt_prefix, NULL);
		if (sibling == NULL) {
			return (ISC_R_NOMEMORY);
		}
		new_parent = new_node(rpzs, tgt_ip, dbit, cur);
		if (new_parent == NULL) {
			isc_mem_put(rpzs->mctx, sibling, sizeof(*sibling));
			return (ISC_R_NOMEMORY);
		}
		new_parent->parent = parent;
		if (parent == NULL) {
			rpzs->cidr = new_parent;
		} else {
			parent->child[cur_num] = new_parent;
		}
		child_num = DNS_RPZ_IP_BIT(tgt_ip, dbit);
		new_parent->child[child_num] = sibling;
		new_parent->child[1 - child_num] = cur;
		cur->parent = new_parent;
		sibling->parent = new_parent;
		sibling->set = *tgt_set;
		set_sum_pair(sibling);
		*found = sibling;
		return (ISC_R_SUCCESS);
	}
}

/*
 * Add an IP address to the radix tree.
 */
static isc_result_t
add_cidr(dns_rpz_zone_t *rpz, dns_rpz_type_t rpz_type,
	 const dns_name_t *src_name) {
	dns_rpz_cidr_key_t tgt_ip;
	dns_rpz_prefix_t tgt_prefix;
	dns_rpz_addr_zbits_t set;
	dns_rpz_cidr_node_t *found = NULL;
	isc_result_t result;

	result = name2ipkey(DNS_RPZ_ERROR_LEVEL, rpz, rpz_type, src_name,
			    &tgt_ip, &tgt_prefix, &set);
	/*
	 * Log complaints about bad owner names but let the zone load.
	 */
	if (result != ISC_R_SUCCESS) {
		return (ISC_R_SUCCESS);
	}

	result = search(rpz->rpzs, &tgt_ip, tgt_prefix, &set, true, &found);
	if (result != ISC_R_SUCCESS) {
		char namebuf[DNS_NAME_FORMATSIZE];

		/*
		 * Do not worry if the radix tree already exists,
		 * because diff_apply() likes to add nodes before deleting.
		 */
		if (result == ISC_R_EXISTS) {
			return (ISC_R_SUCCESS);
		}

		/*
		 * bin/tests/system/rpz/tests.sh looks for "rpz.*failed".
		 */
		dns_name_format(src_name, namebuf, sizeof(namebuf));
		isc_log_write(dns_lctx, DNS_LOGCATEGORY_RPZ,
			      DNS_LOGMODULE_RBTDB, DNS_RPZ_ERROR_LEVEL,
			      "rpz add_cidr(%s) failed: %s", namebuf,
			      isc_result_totext(result));
		return (result);
	}

	adj_trigger_cnt(rpz, rpz_type, &tgt_ip, tgt_prefix, true);
	return (result);
}

static isc_result_t
add_nm(dns_rpz_zones_t *rpzs, dns_name_t *trig_name,
       const dns_rpz_nm_data_t *new_data) {
	dns_rbtnode_t *nmnode = NULL;
	dns_rpz_nm_data_t *nm_data = NULL;
	isc_result_t result;

	nmnode = NULL;
	result = dns_rbt_addnode(rpzs->rbt, trig_name, &nmnode);
	switch (result) {
	case ISC_R_SUCCESS:
	case ISC_R_EXISTS:
		nm_data = nmnode->data;
		if (nm_data == NULL) {
			nm_data = isc_mem_get(rpzs->mctx, sizeof(*nm_data));
			*nm_data = *new_data;
			nmnode->data = nm_data;
			return (ISC_R_SUCCESS);
		}
		break;
	default:
		return (result);
	}

	/*
	 * Do not count bits that are already present
	 */
	if ((nm_data->set.qname & new_data->set.qname) != 0 ||
	    (nm_data->set.ns & new_data->set.ns) != 0 ||
	    (nm_data->wild.qname & new_data->wild.qname) != 0 ||
	    (nm_data->wild.ns & new_data->wild.ns) != 0)
	{
		return (ISC_R_EXISTS);
	}

	nm_data->set.qname |= new_data->set.qname;
	nm_data->set.ns |= new_data->set.ns;
	nm_data->wild.qname |= new_data->wild.qname;
	nm_data->wild.ns |= new_data->wild.ns;
	return (ISC_R_SUCCESS);
}

static isc_result_t
add_name(dns_rpz_zone_t *rpz, dns_rpz_type_t rpz_type,
	 const dns_name_t *src_name) {
	dns_rpz_nm_data_t new_data;
	dns_fixedname_t trig_namef;
	dns_name_t *trig_name = NULL;
	isc_result_t result;

	/*
	 * We need a summary database of names even with 1 policy zone,
	 * because wildcard triggers are handled differently.
	 */

	trig_name = dns_fixedname_initname(&trig_namef);
	name2data(rpz, rpz_type, src_name, trig_name, &new_data);

	result = add_nm(rpz->rpzs, trig_name, &new_data);

	/*
	 * Do not worry if the node already exists,
	 * because diff_apply() likes to add nodes before deleting.
	 */
	if (result == ISC_R_EXISTS) {
		return (ISC_R_SUCCESS);
	}
	if (result == ISC_R_SUCCESS) {
		adj_trigger_cnt(rpz, rpz_type, NULL, 0, true);
	}
	return (result);
}

/*
 * Callback to free the data for a node in the summary RBT database.
 */
static void
rpz_node_deleter(void *nm_data, void *mctx) {
	isc_mem_put(mctx, nm_data, sizeof(dns_rpz_nm_data_t));
}

/*
 * Get ready for a new set of policy zones for a view.
 */
isc_result_t
dns_rpz_new_zones(isc_mem_t *mctx, isc_taskmgr_t *taskmgr,
		  isc_timermgr_t *timermgr, char *rps_cstr,
		  size_t rps_cstr_size, dns_rpz_zones_t **rpzsp) {
	dns_rpz_zones_t *rpzs = NULL;
	isc_result_t result = ISC_R_SUCCESS;

	REQUIRE(rpzsp != NULL && *rpzsp == NULL);

	rpzs = isc_mem_get(mctx, sizeof(*rpzs));
	*rpzs = (dns_rpz_zones_t){
		.rps_cstr = rps_cstr,
		.rps_cstr_size = rps_cstr_size,
		.taskmgr = taskmgr,
		.timermgr = timermgr,
		.magic = DNS_RPZ_ZONES_MAGIC,
	};

	isc_rwlock_init(&rpzs->search_lock, 0, 0);
	isc_mutex_init(&rpzs->maint_lock);
	isc_refcount_init(&rpzs->references, 1);

#ifdef USE_DNSRPS
	if (rps_cstr != NULL) {
		result = dns_dnsrps_view_init(rpzs, rps_cstr);
		if (result != ISC_R_SUCCESS) {
			goto cleanup_rbt;
		}
	}
#else  /* ifdef USE_DNSRPS */
	INSIST(!rpzs->p.dnsrps_enabled);
#endif /* ifdef USE_DNSRPS */
	if (!rpzs->p.dnsrps_enabled) {
		result = dns_rbt_create(mctx, rpz_node_deleter, mctx,
					&rpzs->rbt);
	}

	if (result != ISC_R_SUCCESS) {
		goto cleanup_rbt;
	}

	result = isc_taskmgr_excltask(taskmgr, &rpzs->updater);
	if (result != ISC_R_SUCCESS) {
		goto cleanup_task;
	}

	isc_mem_attach(mctx, &rpzs->mctx);

	*rpzsp = rpzs;
	return (ISC_R_SUCCESS);

cleanup_task:
	dns_rbt_destroy(&rpzs->rbt);

cleanup_rbt:
	isc_refcount_decrementz(&rpzs->references);
	isc_refcount_destroy(&rpzs->references);
	isc_mutex_destroy(&rpzs->maint_lock);
	isc_rwlock_destroy(&rpzs->search_lock);
	isc_mem_put(mctx, rpzs, sizeof(*rpzs));

	return (result);
}

isc_result_t
dns_rpz_new_zone(dns_rpz_zones_t *rpzs, dns_rpz_zone_t **rpzp) {
	isc_result_t result;
	dns_rpz_zone_t *rpz = NULL;

	REQUIRE(DNS_RPZ_ZONES_VALID(rpzs));
	REQUIRE(rpzp != NULL && *rpzp == NULL);

	if (rpzs->p.num_zones >= DNS_RPZ_MAX_ZONES) {
		return (ISC_R_NOSPACE);
	}

	result = dns__rpz_shuttingdown(rpzs);
	if (result != ISC_R_SUCCESS) {
		return (result);
	}

	rpz = isc_mem_get(rpzs->mctx, sizeof(*rpz));
	*rpz = (dns_rpz_zone_t){
		.addsoa = true,
		.magic = DNS_RPZ_ZONE_MAGIC,
		.rpzs = rpzs,
	};

	result = isc_timer_create(rpzs->timermgr, isc_timertype_inactive, NULL,
				  NULL, rpzs->updater, dns__rpz_timer_cb, rpz,
				  &rpz->updatetimer);
	if (result != ISC_R_SUCCESS) {
		goto cleanup_timer;
	}

	/*
	 * This will never be used, but costs us nothing and
	 * simplifies dns__rpz_timer_cb().
	 */

	isc_ht_init(&rpz->nodes, rpzs->mctx, 1, ISC_HT_CASE_SENSITIVE);

	dns_name_init(&rpz->origin, NULL);
	dns_name_init(&rpz->client_ip, NULL);
	dns_name_init(&rpz->ip, NULL);
	dns_name_init(&rpz->nsdname, NULL);
	dns_name_init(&rpz->nsip, NULL);
	dns_name_init(&rpz->passthru, NULL);
	dns_name_init(&rpz->drop, NULL);
	dns_name_init(&rpz->tcp_only, NULL);
	dns_name_init(&rpz->cname, NULL);

	isc_time_settoepoch(&rpz->lastupdated);

	ISC_EVENT_INIT(&rpz->updateevent, sizeof(rpz->updateevent), 0, NULL, 0,
		       NULL, NULL, NULL, NULL, NULL);

	rpz->num = rpzs->p.num_zones++;
	rpzs->zones[rpz->num] = rpz;

	*rpzp = rpz;

	return (ISC_R_SUCCESS);

cleanup_timer:
	isc_mem_put(rpzs->mctx, rpz, sizeof(*rpz));

	return (result);
}

isc_result_t
dns_rpz_dbupdate_callback(dns_db_t *db, void *fn_arg) {
	dns_rpz_zone_t *rpz = (dns_rpz_zone_t *)fn_arg;
	isc_time_t now;
	isc_result_t result = ISC_R_SUCCESS;
	char dname[DNS_NAME_FORMATSIZE];

	REQUIRE(DNS_DB_VALID(db));
	REQUIRE(DNS_RPZ_ZONE_VALID(rpz));

	LOCK(&rpz->rpzs->maint_lock);

	if (rpz->rpzs->shuttingdown) {
		result = ISC_R_SHUTTINGDOWN;
		goto unlock;
	}

	/* New zone came as AXFR */
	if (rpz->db != NULL && rpz->db != db) {
		/* We need to clean up the old DB */
		if (rpz->dbversion != NULL) {
			dns_db_closeversion(rpz->db, &rpz->dbversion, false);
		}
		dns_db_updatenotify_unregister(rpz->db,
					       dns_rpz_dbupdate_callback, rpz);
		dns_db_detach(&rpz->db);
	}

	if (rpz->db == NULL) {
		RUNTIME_CHECK(rpz->dbversion == NULL);
		dns_db_attach(db, &rpz->db);
	}

	dns_name_format(&rpz->origin, dname, DNS_NAME_FORMATSIZE);

	if (!rpz->updatepending && !rpz->updaterunning) {
		uint64_t tdiff;

		rpz->updatepending = true;

		isc_time_now(&now);
		tdiff = isc_time_microdiff(&now, &rpz->lastupdated) / 1000000;
		if (tdiff < rpz->min_update_interval) {
			uint64_t defer = rpz->min_update_interval - tdiff;
			isc_interval_t interval;
			isc_log_write(dns_lctx, DNS_LOGCATEGORY_GENERAL,
				      DNS_LOGMODULE_MASTER, ISC_LOG_INFO,
				      "rpz: %s: new zone version came "
				      "too soon, deferring update for "
				      "%" PRIu64 " seconds",
				      dname, defer);
			isc_interval_set(&interval, (unsigned int)defer, 0);
			dns_db_currentversion(rpz->db, &rpz->dbversion);
			(void)isc_timer_reset(rpz->updatetimer,
					      isc_timertype_once, NULL,
					      &interval, true);
		} else {
			isc_event_t *event = NULL;

			dns_db_currentversion(rpz->db, &rpz->dbversion);
			INSIST(!ISC_LINK_LINKED(&rpz->updateevent, ev_link));
			ISC_EVENT_INIT(&rpz->updateevent,
				       sizeof(rpz->updateevent), 0, NULL,
				       DNS_EVENT_RPZUPDATED, dns__rpz_timer_cb,
				       rpz, rpz, NULL, NULL);
			event = &rpz->updateevent;
			isc_task_send(rpz->rpzs->updater, &event);
		}
	} else {
		rpz->updatepending = true;
		isc_log_write(dns_lctx, DNS_LOGCATEGORY_GENERAL,
			      DNS_LOGMODULE_MASTER, ISC_LOG_DEBUG(3),
			      "rpz: %s: update already queued or running",
			      dname);
		if (rpz->dbversion != NULL) {
			dns_db_closeversion(rpz->db, &rpz->dbversion, false);
		}
		dns_db_currentversion(rpz->db, &rpz->dbversion);
	}

unlock:
	UNLOCK(&rpz->rpzs->maint_lock);

	return (result);
}

static void
update_rpz_done_cb(void *data, isc_result_t result) {
	dns_rpz_zone_t *rpz = (dns_rpz_zone_t *)data;
	char dname[DNS_NAME_FORMATSIZE];

	REQUIRE(DNS_RPZ_ZONE_VALID(rpz));

	if (result == ISC_R_SUCCESS && rpz->updateresult != ISC_R_SUCCESS) {
		result = rpz->updateresult;
	}

	LOCK(&rpz->rpzs->maint_lock);
	rpz->updaterunning = false;

	dns_name_format(&rpz->origin, dname, DNS_NAME_FORMATSIZE);

	/* If there's no update pending, or if shutting down, finish. */
	if (!rpz->updatepending || rpz->rpzs->shuttingdown) {
		goto done;
	}

	/* If there's an update pending, schedule it */
	if (rpz->min_update_interval > 0) {
		uint64_t defer = rpz->min_update_interval;
		isc_interval_t interval;

		isc_log_write(dns_lctx, DNS_LOGCATEGORY_GENERAL,
			      DNS_LOGMODULE_MASTER, ISC_LOG_INFO,
			      "rpz: %s: new zone version came "
			      "too soon, deferring update for "
			      "%" PRIu64 " seconds",
			      dname, defer);
		isc_interval_set(&interval, (unsigned int)defer, 0);
		(void)isc_timer_reset(rpz->updatetimer, isc_timertype_once,
				      NULL, &interval, true);
	} else {
		isc_event_t *event = NULL;
		INSIST(!ISC_LINK_LINKED(&rpz->updateevent, ev_link));
		ISC_EVENT_INIT(&rpz->updateevent, sizeof(rpz->updateevent), 0,
			       NULL, DNS_EVENT_RPZUPDATED, dns__rpz_timer_cb,
			       rpz, rpz, NULL, NULL);
		event = &rpz->updateevent;
		isc_task_send(rpz->rpzs->updater, &event);
	}

done:
	dns_db_closeversion(rpz->updb, &rpz->updbversion, false);
	dns_db_detach(&rpz->updb);

	UNLOCK(&rpz->rpzs->maint_lock);

	isc_log_write(dns_lctx, DNS_LOGCATEGORY_GENERAL, DNS_LOGMODULE_MASTER,
		      ISC_LOG_INFO, "rpz: %s: reload done: %s", dname,
		      isc_result_totext(result));

	dns_rpz_unref_rpzs(rpz->rpzs);
}

static isc_result_t
update_nodes(dns_rpz_zone_t *rpz, isc_ht_t *newnodes) {
	isc_result_t result;
	dns_dbiterator_t *updbit = NULL;
	dns_name_t *name = NULL;
	dns_fixedname_t fixname;
	char domain[DNS_NAME_FORMATSIZE];

	dns_name_format(&rpz->origin, domain, DNS_NAME_FORMATSIZE);

	name = dns_fixedname_initname(&fixname);

	result = dns_db_createiterator(rpz->updb, DNS_DB_NONSEC3, &updbit);
	if (result != ISC_R_SUCCESS) {
		isc_log_write(dns_lctx, DNS_LOGCATEGORY_GENERAL,
			      DNS_LOGMODULE_MASTER, ISC_LOG_ERROR,
			      "rpz: %s: failed to create DB iterator - %s",
			      domain, isc_result_totext(result));
		return (result);
	}

	result = dns_dbiterator_first(updbit);
	if (result != ISC_R_SUCCESS && result != ISC_R_NOMORE) {
		isc_log_write(dns_lctx, DNS_LOGCATEGORY_GENERAL,
			      DNS_LOGMODULE_MASTER, ISC_LOG_ERROR,
			      "rpz: %s: failed to get db iterator - %s", domain,
			      isc_result_totext(result));
		goto cleanup;
	}

	while (result == ISC_R_SUCCESS) {
		char namebuf[DNS_NAME_FORMATSIZE];
		dns_rdatasetiter_t *rdsiter = NULL;
		dns_dbnode_t *node = NULL;

		result = dns__rpz_shuttingdown(rpz->rpzs);
		if (result != ISC_R_SUCCESS) {
			goto cleanup;
		}

		result = dns_dbiterator_current(updbit, &node, name);
		if (result != ISC_R_SUCCESS) {
			isc_log_write(dns_lctx, DNS_LOGCATEGORY_GENERAL,
				      DNS_LOGMODULE_MASTER, ISC_LOG_ERROR,
				      "rpz: %s: failed to get dbiterator - %s",
				      domain, isc_result_totext(result));
			goto cleanup;
		}

		result = dns_dbiterator_pause(updbit);
		RUNTIME_CHECK(result == ISC_R_SUCCESS);

		result = dns_db_allrdatasets(rpz->updb, node, rpz->updbversion,
					     0, 0, &rdsiter);
		if (result != ISC_R_SUCCESS) {
			isc_log_write(dns_lctx, DNS_LOGCATEGORY_GENERAL,
				      DNS_LOGMODULE_MASTER, ISC_LOG_ERROR,
				      "rpz: %s: failed to fetch "
				      "rrdatasets - %s",
				      domain, isc_result_totext(result));
			dns_db_detachnode(rpz->updb, &node);
			goto cleanup;
		}

		result = dns_rdatasetiter_first(rdsiter);

		dns_rdatasetiter_destroy(&rdsiter);
		dns_db_detachnode(rpz->updb, &node);

		if (result != ISC_R_SUCCESS) { /* skip empty non-terminal */
			if (result != ISC_R_NOMORE) {
				isc_log_write(
					dns_lctx, DNS_LOGCATEGORY_GENERAL,
					DNS_LOGMODULE_MASTER, ISC_LOG_ERROR,
					"rpz: %s: error %s while creating "
					"rdatasetiter",
					domain, isc_result_totext(result));
			}
			goto next;
		}

		dns_name_downcase(name, name, NULL);

		/* Add entry to the new nodes table */
		result = isc_ht_add(newnodes, name->ndata, name->length, rpz);
		if (result != ISC_R_SUCCESS) {
			dns_name_format(name, namebuf, sizeof(namebuf));
			isc_log_write(dns_lctx, DNS_LOGCATEGORY_GENERAL,
				      DNS_LOGMODULE_MASTER, ISC_LOG_ERROR,
				      "rpz: %s, adding node %s to HT error %s",
				      domain, namebuf,
				      isc_result_totext(result));
			goto next;
		}

		/* Does the entry exist in the old nodes table? */
		result = isc_ht_find(rpz->nodes, name->ndata, name->length,
				     NULL);
		if (result == ISC_R_SUCCESS) { /* found */
			isc_ht_delete(rpz->nodes, name->ndata, name->length);
			goto next;
		}

		/*
		 * Only the single rpz updates are serialized, so we need to
		 * lock here because we can be processing more updates to
		 * different rpz zones at the same time
		 */
		LOCK(&rpz->rpzs->maint_lock);
		result = rpz_add(rpz, name);
		UNLOCK(&rpz->rpzs->maint_lock);

		if (result != ISC_R_SUCCESS) {
			dns_name_format(name, namebuf, sizeof(namebuf));
			isc_log_write(dns_lctx, DNS_LOGCATEGORY_GENERAL,
				      DNS_LOGMODULE_MASTER, ISC_LOG_ERROR,
				      "rpz: %s: adding node %s "
				      "to RPZ error %s",
				      domain, namebuf,
				      isc_result_totext(result));
		} else if (isc_log_wouldlog(dns_lctx, ISC_LOG_DEBUG(3))) {
			dns_name_format(name, namebuf, sizeof(namebuf));
			isc_log_write(dns_lctx, DNS_LOGCATEGORY_GENERAL,
				      DNS_LOGMODULE_MASTER, ISC_LOG_DEBUG(3),
				      "rpz: %s: adding node %s", domain,
				      namebuf);
		}

	next:
		result = dns_dbiterator_next(updbit);
	}
	INSIST(result != ISC_R_SUCCESS);
	if (result == ISC_R_NOMORE) {
		result = ISC_R_SUCCESS;
	}

cleanup:
	dns_dbiterator_destroy(&updbit);

	return (result);
}

static isc_result_t
cleanup_nodes(dns_rpz_zone_t *rpz) {
	isc_result_t result;
	isc_ht_iter_t *iter = NULL;
	dns_name_t *name = NULL;
	dns_fixedname_t fixname;

	name = dns_fixedname_initname(&fixname);

	isc_ht_iter_create(rpz->nodes, &iter);

	for (result = isc_ht_iter_first(iter); result == ISC_R_SUCCESS;
	     result = isc_ht_iter_delcurrent_next(iter))
	{
		isc_region_t region;
		unsigned char *key = NULL;
		size_t keysize;

		result = dns__rpz_shuttingdown(rpz->rpzs);
		if (result != ISC_R_SUCCESS) {
			break;
		}

		isc_ht_iter_currentkey(iter, &key, &keysize);
		region.base = key;
		region.length = (unsigned int)keysize;
		dns_name_fromregion(name, &region);

		LOCK(&rpz->rpzs->maint_lock);
		rpz_del(rpz, name);
		UNLOCK(&rpz->rpzs->maint_lock);
	}
	INSIST(result != ISC_R_SUCCESS);
	if (result == ISC_R_NOMORE) {
		result = ISC_R_SUCCESS;
	}

	isc_ht_iter_destroy(&iter);

	return (result);
}

static isc_result_t
dns__rpz_shuttingdown(dns_rpz_zones_t *rpzs) {
	bool shuttingdown = false;

	LOCK(&rpzs->maint_lock);
	shuttingdown = rpzs->shuttingdown;
	UNLOCK(&rpzs->maint_lock);

	if (shuttingdown) {
		return (ISC_R_SHUTTINGDOWN);
	}

	return (ISC_R_SUCCESS);
}

static void
update_rpz_cb(void *data) {
	dns_rpz_zone_t *rpz = (dns_rpz_zone_t *)data;
	isc_result_t result = ISC_R_SUCCESS;
	isc_ht_t *newnodes = NULL;

	REQUIRE(rpz->nodes != NULL);

	result = dns__rpz_shuttingdown(rpz->rpzs);
	if (result != ISC_R_SUCCESS) {
		goto shuttingdown;
	}

	isc_ht_init(&newnodes, rpz->rpzs->mctx, 1, ISC_HT_CASE_SENSITIVE);

	result = update_nodes(rpz, newnodes);
	if (result != ISC_R_SUCCESS) {
		goto cleanup;
	}

	result = cleanup_nodes(rpz);
	if (result != ISC_R_SUCCESS) {
		goto cleanup;
	}

	/* Finalize the update */
	ISC_SWAP(rpz->nodes, newnodes);

cleanup:
	isc_ht_destroy(&newnodes);

shuttingdown:
	rpz->updateresult = result;
}

static void
dns__rpz_timer_cb(isc_task_t *task, isc_event_t *event) {
	char domain[DNS_NAME_FORMATSIZE];
	isc_result_t result;
	dns_rpz_zone_t *rpz = NULL;

	UNUSED(task);
	REQUIRE(event != NULL);
	REQUIRE(event->ev_arg != NULL);

	rpz = (dns_rpz_zone_t *)event->ev_arg;
	isc_event_free(&event);

	REQUIRE(isc_nm_tid() >= 0);
	REQUIRE(DNS_RPZ_ZONE_VALID(rpz));

	LOCK(&rpz->rpzs->maint_lock);

	if (rpz->rpzs->shuttingdown) {
		goto unlock;
	}

	rpz->updatepending = false;
	rpz->updaterunning = true;
	rpz->updateresult = ISC_R_UNSET;

	INSIST(rpz->updb == NULL);
	INSIST(rpz->updbversion == NULL);
	INSIST(rpz->dbversion != NULL);
	INSIST(DNS_DB_VALID(rpz->db));
	dns_db_attach(rpz->db, &rpz->updb);
	rpz->updbversion = rpz->dbversion;
	rpz->dbversion = NULL;

	dns_name_format(&rpz->origin, domain, DNS_NAME_FORMATSIZE);
	isc_log_write(dns_lctx, DNS_LOGCATEGORY_GENERAL, DNS_LOGMODULE_MASTER,
		      ISC_LOG_INFO, "rpz: %s: reload start", domain);

	dns_rpz_ref_rpzs(rpz->rpzs);
	isc_nm_work_offload(isc_task_getnetmgr(rpz->rpzs->updater),
			    update_rpz_cb, update_rpz_done_cb, rpz);

	result = isc_time_now(&rpz->lastupdated);
	RUNTIME_CHECK(result == ISC_R_SUCCESS);

unlock:
	UNLOCK(&rpz->rpzs->maint_lock);
}

/*
 * Free the radix tree of a response policy database.
 */
static void
cidr_free(dns_rpz_zones_t *rpzs) {
	dns_rpz_cidr_node_t *cur = NULL, *child = NULL, *parent = NULL;

	cur = rpzs->cidr;
	while (cur != NULL) {
		/* Depth first. */
		child = cur->child[0];
		if (child != NULL) {
			cur = child;
			continue;
		}
		child = cur->child[1];
		if (child != NULL) {
			cur = child;
			continue;
		}

		/* Delete this leaf and go up. */
		parent = cur->parent;
		if (parent == NULL) {
			rpzs->cidr = NULL;
		} else {
			parent->child[parent->child[1] == cur] = NULL;
		}
		isc_mem_put(rpzs->mctx, cur, sizeof(*cur));
		cur = parent;
	}
}

static void
dns__rpz_shutdown(dns_rpz_zone_t *rpz) {
	/* maint_lock must be locked */
	if (rpz->updatetimer != NULL) {
		isc_result_t result;

		/* Don't wait for timer to trigger for shutdown */
		result = isc_timer_reset(rpz->updatetimer,
					 isc_timertype_inactive, NULL, NULL,
					 true);
		RUNTIME_CHECK(result == ISC_R_SUCCESS);
	}
}

static void
dns_rpz_zone_destroy(dns_rpz_zone_t **rpzp) {
	dns_rpz_zone_t *rpz = NULL;
	dns_rpz_zones_t *rpzs;

	rpz = *rpzp;
	*rpzp = NULL;

	rpzs = rpz->rpzs;
	rpz->rpzs = NULL;

	if (dns_name_dynamic(&rpz->origin)) {
		dns_name_free(&rpz->origin, rpzs->mctx);
	}
	if (dns_name_dynamic(&rpz->client_ip)) {
		dns_name_free(&rpz->client_ip, rpzs->mctx);
	}
	if (dns_name_dynamic(&rpz->ip)) {
		dns_name_free(&rpz->ip, rpzs->mctx);
	}
	if (dns_name_dynamic(&rpz->nsdname)) {
		dns_name_free(&rpz->nsdname, rpzs->mctx);
	}
	if (dns_name_dynamic(&rpz->nsip)) {
		dns_name_free(&rpz->nsip, rpzs->mctx);
	}
	if (dns_name_dynamic(&rpz->passthru)) {
		dns_name_free(&rpz->passthru, rpzs->mctx);
	}
	if (dns_name_dynamic(&rpz->drop)) {
		dns_name_free(&rpz->drop, rpzs->mctx);
	}
	if (dns_name_dynamic(&rpz->tcp_only)) {
		dns_name_free(&rpz->tcp_only, rpzs->mctx);
	}
	if (dns_name_dynamic(&rpz->cname)) {
		dns_name_free(&rpz->cname, rpzs->mctx);
	}
	if (rpz->db != NULL) {
		if (rpz->dbversion != NULL) {
			dns_db_closeversion(rpz->db, &rpz->dbversion, false);
		}
		dns_db_updatenotify_unregister(rpz->db,
					       dns_rpz_dbupdate_callback, rpz);
		dns_db_detach(&rpz->db);
	}
	INSIST(!rpz->updaterunning);

	isc_timer_reset(rpz->updatetimer, isc_timertype_inactive, NULL, NULL,
			true);
	isc_timer_destroy(&rpz->updatetimer);

	isc_ht_destroy(&rpz->nodes);

	isc_mem_put(rpzs->mctx, rpz, sizeof(*rpz));
}

static void
dns__rpz_zones_destroy(dns_rpz_zones_t *rpzs) {
	REQUIRE(rpzs->shuttingdown);

	isc_refcount_destroy(&rpzs->references);

	for (dns_rpz_num_t rpz_num = 0; rpz_num < DNS_RPZ_MAX_ZONES; ++rpz_num)
	{
		if (rpzs->zones[rpz_num] == NULL) {
			continue;
		}

		dns_rpz_zone_destroy(&rpzs->zones[rpz_num]);
	}

	if (rpzs->rps_cstr_size != 0) {
#ifdef USE_DNSRPS
		librpz->client_detach(&rpzs->rps_client);
#endif /* ifdef USE_DNSRPS */
		isc_mem_put(rpzs->mctx, rpzs->rps_cstr, rpzs->rps_cstr_size);
	}

	cidr_free(rpzs);
	if (rpzs->rbt != NULL) {
		dns_rbt_destroy(&rpzs->rbt);
	}
	isc_task_detach(&rpzs->updater);
	isc_mutex_destroy(&rpzs->maint_lock);
	isc_rwlock_destroy(&rpzs->search_lock);
	isc_mem_putanddetach(&rpzs->mctx, rpzs, sizeof(*rpzs));
}

void
dns_rpz_zones_shutdown(dns_rpz_zones_t *rpzs) {
	REQUIRE(DNS_RPZ_ZONES_VALID(rpzs));
	/*
	 * Forget the last of the view's rpz machinery when shutting down.
	 */

	LOCK(&rpzs->maint_lock);
	if (rpzs->shuttingdown) {
		UNLOCK(&rpzs->maint_lock);
		return;
	}

	rpzs->shuttingdown = true;

	for (dns_rpz_num_t rpz_num = 0; rpz_num < DNS_RPZ_MAX_ZONES; ++rpz_num)
	{
		if (rpzs->zones[rpz_num] == NULL) {
			continue;
		}

		dns__rpz_shutdown(rpzs->zones[rpz_num]);
	}
	UNLOCK(&rpzs->maint_lock);
}

#ifdef DNS_RPZ_TRACE
ISC_REFCOUNT_TRACE_IMPL(dns_rpz_zones, dns__rpz_zones_destroy);
#else
ISC_REFCOUNT_IMPL(dns_rpz_zones, dns__rpz_zones_destroy);
#endif

/*
 * Add an IP address to the radix tree or a name to the summary database.
 */
static isc_result_t
rpz_add(dns_rpz_zone_t *rpz, const dns_name_t *src_name) {
	dns_rpz_type_t rpz_type;
	isc_result_t result = ISC_R_FAILURE;
	dns_rpz_zones_t *rpzs = NULL;
	dns_rpz_num_t rpz_num;

	REQUIRE(rpz != NULL);

	rpzs = rpz->rpzs;
	rpz_num = rpz->num;

	REQUIRE(rpzs != NULL && rpz_num < rpzs->p.num_zones);

	RWLOCK(&rpzs->search_lock, isc_rwlocktype_write);

	rpz_type = type_from_name(rpzs, rpz, src_name);

	switch (rpz_type) {
	case DNS_RPZ_TYPE_QNAME:
	case DNS_RPZ_TYPE_NSDNAME:
		result = add_name(rpz, rpz_type, src_name);
		break;
	case DNS_RPZ_TYPE_CLIENT_IP:
	case DNS_RPZ_TYPE_IP:
	case DNS_RPZ_TYPE_NSIP:
		result = add_cidr(rpz, rpz_type, src_name);
		break;
	case DNS_RPZ_TYPE_BAD:
		break;
	}
	RWUNLOCK(&rpzs->search_lock, isc_rwlocktype_write);

	return (result);
}

/*
 * Remove an IP address from the radix tree.
 */
static void
del_cidr(dns_rpz_zone_t *rpz, dns_rpz_type_t rpz_type,
	 const dns_name_t *src_name) {
	isc_result_t result;
	dns_rpz_cidr_key_t tgt_ip;
	dns_rpz_prefix_t tgt_prefix;
	dns_rpz_addr_zbits_t tgt_set;
	dns_rpz_cidr_node_t *tgt = NULL, *parent = NULL, *child = NULL;

	/*
	 * Do not worry about invalid rpz IP address names.  If we
	 * are here, then something relevant was added and so was
	 * valid.  Invalid names here are usually internal RBTDB nodes.
	 */
	result = name2ipkey(DNS_RPZ_DEBUG_QUIET, rpz, rpz_type, src_name,
			    &tgt_ip, &tgt_prefix, &tgt_set);
	if (result != ISC_R_SUCCESS) {
		return;
	}

	result = search(rpz->rpzs, &tgt_ip, tgt_prefix, &tgt_set, false, &tgt);
	if (result != ISC_R_SUCCESS) {
		INSIST(result == ISC_R_NOTFOUND ||
		       result == DNS_R_PARTIALMATCH);
		/*
		 * Do not worry about missing summary RBT nodes that probably
		 * correspond to RBTDB nodes that were implicit RBT nodes
		 * that were later added for (often empty) wildcards
		 * and then to the RBTDB deferred cleanup list.
		 */
		return;
	}

	/*
	 * Mark the node and its parents to reflect the deleted IP address.
	 * Do not count bits that are already clear for internal RBTDB nodes.
	 */
	tgt_set.client_ip &= tgt->set.client_ip;
	tgt_set.ip &= tgt->set.ip;
	tgt_set.nsip &= tgt->set.nsip;
	tgt->set.client_ip &= ~tgt_set.client_ip;
	tgt->set.ip &= ~tgt_set.ip;
	tgt->set.nsip &= ~tgt_set.nsip;
	set_sum_pair(tgt);

	adj_trigger_cnt(rpz, rpz_type, &tgt_ip, tgt_prefix, false);

	/*
	 * We might need to delete 2 nodes.
	 */
	do {
		/*
		 * The node is now useless if it has no data of its own
		 * and 0 or 1 children.  We are finished if it is not
		 * useless.
		 */
		if ((child = tgt->child[0]) != NULL) {
			if (tgt->child[1] != NULL) {
				break;
			}
		} else {
			child = tgt->child[1];
		}
		if (tgt->set.client_ip != 0 || tgt->set.ip != 0 ||
		    tgt->set.nsip != 0)
		{
			break;
		}

		/*
		 * Replace the pointer to this node in the parent with
		 * the remaining child or NULL.
		 */
		parent = tgt->parent;
		if (parent == NULL) {
			rpz->rpzs->cidr = child;
		} else {
			parent->child[parent->child[1] == tgt] = child;
		}

		/*
		 * If the child exists fix up its parent pointer.
		 */
		if (child != NULL) {
			child->parent = parent;
		}
		isc_mem_put(rpz->rpzs->mctx, tgt, sizeof(*tgt));

		tgt = parent;
	} while (tgt != NULL);
}

static void
del_name(dns_rpz_zone_t *rpz, dns_rpz_type_t rpz_type,
	 const dns_name_t *src_name) {
	char namebuf[DNS_NAME_FORMATSIZE];
	dns_fixedname_t trig_namef;
	dns_name_t *trig_name = NULL;
	dns_rbtnode_t *nmnode = NULL;
	dns_rpz_nm_data_t *nm_data = NULL;
	dns_rpz_nm_data_t del_data;
	isc_result_t result;
	bool exists;

	/*
	 * We need a summary database of names even with 1 policy zone,
	 * because wildcard triggers are handled differently.
	 */

	trig_name = dns_fixedname_initname(&trig_namef);
	name2data(rpz, rpz_type, src_name, trig_name, &del_data);

	nmnode = NULL;
	result = dns_rbt_findnode(rpz->rpzs->rbt, trig_name, NULL, &nmnode,
				  NULL, 0, NULL, NULL);
	if (result != ISC_R_SUCCESS) {
		/*
		 * Do not worry about missing summary RBT nodes that probably
		 * correspond to RBTDB nodes that were implicit RBT nodes
		 * that were later added for (often empty) wildcards
		 * and then to the RBTDB deferred cleanup list.
		 */
		if (result == ISC_R_NOTFOUND || result == DNS_R_PARTIALMATCH) {
			return;
		}
		dns_name_format(src_name, namebuf, sizeof(namebuf));
		isc_log_write(dns_lctx, DNS_LOGCATEGORY_RPZ,
			      DNS_LOGMODULE_RBTDB, DNS_RPZ_ERROR_LEVEL,
			      "rpz del_name(%s) node search failed: %s",
			      namebuf, isc_result_totext(result));
		return;
	}

	nm_data = nmnode->data;
	INSIST(nm_data != NULL);

	/*
	 * Do not count bits that next existed for RBT nodes that would we
	 * would not have found in a summary for a single RBTDB tree.
	 */
	del_data.set.qname &= nm_data->set.qname;
	del_data.set.ns &= nm_data->set.ns;
	del_data.wild.qname &= nm_data->wild.qname;
	del_data.wild.ns &= nm_data->wild.ns;

	exists = (del_data.set.qname != 0 || del_data.set.ns != 0 ||
		  del_data.wild.qname != 0 || del_data.wild.ns != 0);

	nm_data->set.qname &= ~del_data.set.qname;
	nm_data->set.ns &= ~del_data.set.ns;
	nm_data->wild.qname &= ~del_data.wild.qname;
	nm_data->wild.ns &= ~del_data.wild.ns;

	if (nm_data->set.qname == 0 && nm_data->set.ns == 0 &&
	    nm_data->wild.qname == 0 && nm_data->wild.ns == 0)
	{
		result = dns_rbt_deletenode(rpz->rpzs->rbt, nmnode, false);
		if (result != ISC_R_SUCCESS) {
			/*
			 * bin/tests/system/rpz/tests.sh looks for
			 * "rpz.*failed".
			 */
			dns_name_format(src_name, namebuf, sizeof(namebuf));
			isc_log_write(dns_lctx, DNS_LOGCATEGORY_RPZ,
				      DNS_LOGMODULE_RBTDB, DNS_RPZ_ERROR_LEVEL,
				      "rpz del_name(%s) node delete "
				      "failed: %s",
				      namebuf, isc_result_totext(result));
		}
	}

	if (exists) {
		adj_trigger_cnt(rpz, rpz_type, NULL, 0, false);
	}
}

/*
 * Remove an IP address from the radix tree or a name from the summary database.
 */
static void
rpz_del(dns_rpz_zone_t *rpz, const dns_name_t *src_name) {
	dns_rpz_type_t rpz_type;
	dns_rpz_zones_t *rpzs = NULL;
	dns_rpz_num_t rpz_num;

	REQUIRE(rpz != NULL);

	rpzs = rpz->rpzs;
	rpz_num = rpz->num;

	REQUIRE(rpzs != NULL && rpz_num < rpzs->p.num_zones);

	RWLOCK(&rpzs->search_lock, isc_rwlocktype_write);

	rpz_type = type_from_name(rpzs, rpz, src_name);

	switch (rpz_type) {
	case DNS_RPZ_TYPE_QNAME:
	case DNS_RPZ_TYPE_NSDNAME:
		del_name(rpz, rpz_type, src_name);
		break;
	case DNS_RPZ_TYPE_CLIENT_IP:
	case DNS_RPZ_TYPE_IP:
	case DNS_RPZ_TYPE_NSIP:
		del_cidr(rpz, rpz_type, src_name);
		break;
	case DNS_RPZ_TYPE_BAD:
		break;
	}

	RWUNLOCK(&rpzs->search_lock, isc_rwlocktype_write);
}

/*
 * Search the summary radix tree to get a relative owner name in a
 * policy zone relevant to a triggering IP address.
 *	rpz_type and zbits limit the search for IP address netaddr
 *	return the policy zone's number or DNS_RPZ_INVALID_NUM
 *	ip_name is the relative owner name found and
 *	*prefixp is its prefix length.
 */
dns_rpz_num_t
dns_rpz_find_ip(dns_rpz_zones_t *rpzs, dns_rpz_type_t rpz_type,
		dns_rpz_zbits_t zbits, const isc_netaddr_t *netaddr,
		dns_name_t *ip_name, dns_rpz_prefix_t *prefixp) {
	dns_rpz_cidr_key_t tgt_ip;
	dns_rpz_addr_zbits_t tgt_set;
	dns_rpz_cidr_node_t *found = NULL;
	isc_result_t result;
	dns_rpz_num_t rpz_num = 0;
	dns_rpz_have_t have;
	int i;

	RWLOCK(&rpzs->search_lock, isc_rwlocktype_read);
	have = rpzs->have;
	RWUNLOCK(&rpzs->search_lock, isc_rwlocktype_read);

	/*
	 * Convert IP address to CIDR tree key.
	 */
	if (netaddr->family == AF_INET) {
		tgt_ip.w[0] = 0;
		tgt_ip.w[1] = 0;
		tgt_ip.w[2] = ADDR_V4MAPPED;
		tgt_ip.w[3] = ntohl(netaddr->type.in.s_addr);
		switch (rpz_type) {
		case DNS_RPZ_TYPE_CLIENT_IP:
			zbits &= have.client_ipv4;
			break;
		case DNS_RPZ_TYPE_IP:
			zbits &= have.ipv4;
			break;
		case DNS_RPZ_TYPE_NSIP:
			zbits &= have.nsipv4;
			break;
		default:
			UNREACHABLE();
			break;
		}
	} else if (netaddr->family == AF_INET6) {
		dns_rpz_cidr_key_t src_ip6;

		/*
		 * Given the int aligned struct in_addr member of netaddr->type
		 * one could cast netaddr->type.in6 to dns_rpz_cidr_key_t *,
		 * but some people object.
		 */
		memmove(src_ip6.w, &netaddr->type.in6, sizeof(src_ip6.w));
		for (i = 0; i < 4; i++) {
			tgt_ip.w[i] = ntohl(src_ip6.w[i]);
		}
		switch (rpz_type) {
		case DNS_RPZ_TYPE_CLIENT_IP:
			zbits &= have.client_ipv6;
			break;
		case DNS_RPZ_TYPE_IP:
			zbits &= have.ipv6;
			break;
		case DNS_RPZ_TYPE_NSIP:
			zbits &= have.nsipv6;
			break;
		default:
			UNREACHABLE();
			break;
		}
	} else {
		return (DNS_RPZ_INVALID_NUM);
	}

	if (zbits == 0) {
		return (DNS_RPZ_INVALID_NUM);
	}
	make_addr_set(&tgt_set, zbits, rpz_type);

	RWLOCK(&rpzs->search_lock, isc_rwlocktype_read);
	result = search(rpzs, &tgt_ip, 128, &tgt_set, false, &found);
	if (result == ISC_R_NOTFOUND) {
		/*
		 * There are no eligible zones for this IP address.
		 */
		RWUNLOCK(&rpzs->search_lock, isc_rwlocktype_read);
		return (DNS_RPZ_INVALID_NUM);
	}

	/*
	 * Construct the trigger name for the longest matching trigger
	 * in the first eligible zone with a match.
	 */
	*prefixp = found->prefix;
	switch (rpz_type) {
	case DNS_RPZ_TYPE_CLIENT_IP:
		rpz_num = zbit_to_num(found->set.client_ip & tgt_set.client_ip);
		break;
	case DNS_RPZ_TYPE_IP:
		rpz_num = zbit_to_num(found->set.ip & tgt_set.ip);
		break;
	case DNS_RPZ_TYPE_NSIP:
		rpz_num = zbit_to_num(found->set.nsip & tgt_set.nsip);
		break;
	default:
		UNREACHABLE();
	}
	result = ip2name(&found->ip, found->prefix, dns_rootname, ip_name);
	RWUNLOCK(&rpzs->search_lock, isc_rwlocktype_read);
	if (result != ISC_R_SUCCESS) {
		/*
		 * bin/tests/system/rpz/tests.sh looks for "rpz.*failed".
		 */
		isc_log_write(dns_lctx, DNS_LOGCATEGORY_RPZ,
			      DNS_LOGMODULE_RBTDB, DNS_RPZ_ERROR_LEVEL,
			      "rpz ip2name() failed: %s",
			      isc_result_totext(result));
		return (DNS_RPZ_INVALID_NUM);
	}
	return (rpz_num);
}

/*
 * Search the summary radix tree for policy zones with triggers matching
 * a name.
 */
dns_rpz_zbits_t
dns_rpz_find_name(dns_rpz_zones_t *rpzs, dns_rpz_type_t rpz_type,
		  dns_rpz_zbits_t zbits, dns_name_t *trig_name) {
	char namebuf[DNS_NAME_FORMATSIZE];
	dns_rbtnode_t *nmnode = NULL;
	const dns_rpz_nm_data_t *nm_data = NULL;
	dns_rpz_zbits_t found_zbits;
	dns_rbtnodechain_t chain;
	isc_result_t result;
	int i;

	if (zbits == 0) {
		return (0);
	}

	found_zbits = 0;

	dns_rbtnodechain_init(&chain);

	RWLOCK(&rpzs->search_lock, isc_rwlocktype_read);

	nmnode = NULL;
	result = dns_rbt_findnode(rpzs->rbt, trig_name, NULL, &nmnode, &chain,
				  DNS_RBTFIND_EMPTYDATA, NULL, NULL);

	switch (result) {
	case ISC_R_SUCCESS:
		nm_data = nmnode->data;
		if (nm_data != NULL) {
			if (rpz_type == DNS_RPZ_TYPE_QNAME) {
				found_zbits = nm_data->set.qname;
			} else {
				found_zbits = nm_data->set.ns;
			}
		}
		FALLTHROUGH;

	case DNS_R_PARTIALMATCH:
		i = chain.level_matches;
		nmnode = chain.levels[chain.level_matches];

		/*
		 * Whenever an exact match is found by dns_rbt_findnode(),
		 * the highest level node in the chain will not be put into
		 * chain->levels[] array, but instead the chain->end
		 * pointer will be adjusted to point to that node.
		 *
		 * Suppose we have the following entries in a rpz zone:
		 *   example.com     CNAME rpz-passthru.
		 *   *.example.com   CNAME rpz-passthru.
		 *
		 * A query for www.example.com would result in the
		 * following chain object returned by dns_rbt_findnode():
		 *   chain->level_count = 2
		 *   chain->level_matches = 2
		 *   chain->levels[0] = .
		 *   chain->levels[1] = example.com
		 *   chain->levels[2] = NULL
		 *   chain->end = www
		 *
		 * Since exact matches only care for testing rpz set bits,
		 * we need to test for rpz wild bits through iterating the
		 * nodechain, and that includes testing the rpz wild bits
		 * in the highest level node found. In the case of an exact
		 * match, chain->levels[chain->level_matches] will be NULL,
		 * to address that we must use chain->end as the start
		 * point, then iterate over the remaining levels in the
		 * chain.
		 */
		if (nmnode == NULL) {
			--i;
			nmnode = chain.end;
		}

		while (nmnode != NULL) {
			nm_data = nmnode->data;
			if (nm_data != NULL) {
				if (rpz_type == DNS_RPZ_TYPE_QNAME) {
					found_zbits |= nm_data->wild.qname;
				} else {
					found_zbits |= nm_data->wild.ns;
				}
			}

			if (i >= 0) {
				nmnode = chain.levels[i];
				--i;
			} else {
				break;
			}
		}
		break;

	case ISC_R_NOTFOUND:
		break;

	default:
		/*
		 * bin/tests/system/rpz/tests.sh looks for "rpz.*failed".
		 */
		dns_name_format(trig_name, namebuf, sizeof(namebuf));
		isc_log_write(dns_lctx, DNS_LOGCATEGORY_RPZ,
			      DNS_LOGMODULE_RBTDB, DNS_RPZ_ERROR_LEVEL,
			      "dns_rpz_find_name(%s) failed: %s", namebuf,
			      isc_result_totext(result));
		break;
	}

	RWUNLOCK(&rpzs->search_lock, isc_rwlocktype_read);

	dns_rbtnodechain_invalidate(&chain);

	return (zbits & found_zbits);
}

/*
 * Translate CNAME rdata to a QNAME response policy action.
 */
dns_rpz_policy_t
dns_rpz_decode_cname(dns_rpz_zone_t *rpz, dns_rdataset_t *rdataset,
		     dns_name_t *selfname) {
	dns_rdata_t rdata = DNS_RDATA_INIT;
	dns_rdata_cname_t cname;
	isc_result_t result;

	result = dns_rdataset_first(rdataset);
	INSIST(result == ISC_R_SUCCESS);
	dns_rdataset_current(rdataset, &rdata);
	result = dns_rdata_tostruct(&rdata, &cname, NULL);
	INSIST(result == ISC_R_SUCCESS);
	dns_rdata_reset(&rdata);

	/*
	 * CNAME . means NXDOMAIN
	 */
	if (dns_name_equal(&cname.cname, dns_rootname)) {
		return (DNS_RPZ_POLICY_NXDOMAIN);
	}

	if (dns_name_iswildcard(&cname.cname)) {
		/*
		 * CNAME *. means NODATA
		 */
		if (dns_name_countlabels(&cname.cname) == 2) {
			return (DNS_RPZ_POLICY_NODATA);
		}

		/*
		 * A qname of www.evil.com and a policy of
		 *	*.evil.com    CNAME   *.garden.net
		 * gives a result of
		 *	evil.com    CNAME   evil.com.garden.net
		 */
		if (dns_name_countlabels(&cname.cname) > 2) {
			return (DNS_RPZ_POLICY_WILDCNAME);
		}
	}

	/*
	 * CNAME rpz-tcp-only. means "send truncated UDP responses."
	 */
	if (dns_name_equal(&cname.cname, &rpz->tcp_only)) {
		return (DNS_RPZ_POLICY_TCP_ONLY);
	}

	/*
	 * CNAME rpz-drop. means "do not respond."
	 */
	if (dns_name_equal(&cname.cname, &rpz->drop)) {
		return (DNS_RPZ_POLICY_DROP);
	}

	/*
	 * CNAME rpz-passthru. means "do not rewrite."
	 */
	if (dns_name_equal(&cname.cname, &rpz->passthru)) {
		return (DNS_RPZ_POLICY_PASSTHRU);
	}

	/*
	 * 128.1.0.127.rpz-ip CNAME  128.1.0.0.127. is obsolete PASSTHRU
	 */
	if (selfname != NULL && dns_name_equal(&cname.cname, selfname)) {
		return (DNS_RPZ_POLICY_PASSTHRU);
	}

	/*
	 * Any other rdata gives a response consisting of the rdata.
	 */
	return (DNS_RPZ_POLICY_RECORD);
}
