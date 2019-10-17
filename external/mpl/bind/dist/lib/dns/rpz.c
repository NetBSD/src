/*	$NetBSD: rpz.c,v 1.4.4.2 2019/10/17 19:34:20 martin Exp $	*/

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

/*! \file */

#include <config.h>

#include <inttypes.h>
#include <stdbool.h>
#include <stdlib.h>

#include <isc/buffer.h>
#include <isc/mem.h>
#include <isc/net.h>
#include <isc/netaddr.h>
#include <isc/print.h>
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
#include <dns/rdata.h>
#include <dns/rdataset.h>
#include <dns/rdatastruct.h>
#include <dns/rdatasetiter.h>
#include <dns/result.h>
#include <dns/rbt.h>
#include <dns/rpz.h>
#include <dns/view.h>


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
#define DNS_RPZ_HTSIZE_MAX	24
#define DNS_RPZ_HTSIZE_DIV	3

/*
 * Maximum number of nodes to process per quantum
 */
#define DNS_RPZ_QUANTUM 1024

static void
dns_rpz_update_from_db(dns_rpz_zone_t *rpz);

static void
dns_rpz_update_taskaction(isc_task_t *task, isc_event_t *event);

/*
 * Use a private definition of IPv6 addresses because s6_addr32 is not
 * always defined and our IPv6 addresses are in non-standard byte order
 */
typedef uint32_t		dns_rpz_cidr_word_t;
#define DNS_RPZ_CIDR_WORD_BITS	((int)sizeof(dns_rpz_cidr_word_t)*8)
#define DNS_RPZ_CIDR_KEY_BITS	((int)sizeof(dns_rpz_cidr_key_t)*8)
#define DNS_RPZ_CIDR_WORDS	(128/DNS_RPZ_CIDR_WORD_BITS)
typedef struct {
	dns_rpz_cidr_word_t	w[DNS_RPZ_CIDR_WORDS];
} dns_rpz_cidr_key_t;

#define ADDR_V4MAPPED		0xffff
#define KEY_IS_IPV4(prefix,ip) ((prefix) >= 96 && (ip)->w[0] == 0 &&	\
				(ip)->w[1] == 0 && (ip)->w[2] == ADDR_V4MAPPED)

#define DNS_RPZ_WORD_MASK(b) ((b) == 0 ? (dns_rpz_cidr_word_t)(-1)	\
			      : ((dns_rpz_cidr_word_t)(-1)		\
				 << (DNS_RPZ_CIDR_WORD_BITS - (b))))

/*
 * Get bit #n from the array of words of an IP address.
 */
#define DNS_RPZ_IP_BIT(ip, n) (1 & ((ip)->w[(n)/DNS_RPZ_CIDR_WORD_BITS] >>  \
				    (DNS_RPZ_CIDR_WORD_BITS		    \
				     - 1 - ((n) % DNS_RPZ_CIDR_WORD_BITS))))

/*
 * A triplet of arrays of bits flagging the existence of
 * client-IP, IP, and NSIP policy triggers.
 */
typedef struct dns_rpz_addr_zbits dns_rpz_addr_zbits_t;
struct dns_rpz_addr_zbits {
	dns_rpz_zbits_t		client_ip;
	dns_rpz_zbits_t		ip;
	dns_rpz_zbits_t		nsip;
};

/*
 * A CIDR or radix tree node.
 */
struct dns_rpz_cidr_node {
	dns_rpz_cidr_node_t	*parent;
	dns_rpz_cidr_node_t	*child[2];
	dns_rpz_cidr_key_t	ip;
	dns_rpz_prefix_t	prefix;
	dns_rpz_addr_zbits_t	set;
	dns_rpz_addr_zbits_t	sum;
};

/*
 * A pair of arrays of bits flagging the existence of
 * QNAME and NSDNAME policy triggers.
 */
typedef struct dns_rpz_nm_zbits dns_rpz_nm_zbits_t;
struct dns_rpz_nm_zbits {
	dns_rpz_zbits_t		qname;
	dns_rpz_zbits_t		ns;
};

/*
 * The data in a RBT node has two pairs of bits for policy zones.
 * One pair is for the corresponding name of the node such as example.com
 * and the other pair is for a wildcard child such as *.example.com.
 */
typedef struct dns_rpz_nm_data dns_rpz_nm_data_t;
struct dns_rpz_nm_data {
	dns_rpz_nm_zbits_t	set;
	dns_rpz_nm_zbits_t	wild;
};

static void
rpz_detach(dns_rpz_zone_t **rpzp);

static void
rpz_detach_rpzs(dns_rpz_zones_t **rpzsp);

#if 0
/*
 * Catch a name while debugging.
 */
static void
catch_name(const dns_name_t *src_name, const char *tgt, const char *str) {
	dns_fixedname_t tgt_namef;
	dns_name_t *tgt_name;

	tgt_name = dns_fixedname_initname(&tgt_namef);
	dns_name_fromstring(tgt_name, tgt, DNS_NAME_DOWNCASE, NULL);
	if (dns_name_equal(src_name, tgt_name)) {
		isc_log_write(dns_lctx, DNS_LOGCATEGORY_RPZ,
			      DNS_LOGMODULE_RBTDB, DNS_RPZ_ERROR_LEVEL,
			      "rpz hit failed: %s %s", str, tgt);
	}
}
#endif

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
	FATAL_ERROR(__FILE__, __LINE__, "impossible rpz type %d", type);
	return ("impossible");
}

dns_rpz_policy_t
dns_rpz_str2policy(const char *str) {
	static struct {
		const char *str;
		dns_rpz_policy_t policy;
	} tbl[] = {
		{"given",	DNS_RPZ_POLICY_GIVEN},
		{"disabled",	DNS_RPZ_POLICY_DISABLED},
		{"passthru",	DNS_RPZ_POLICY_PASSTHRU},
		{"drop",	DNS_RPZ_POLICY_DROP},
		{"tcp-only",	DNS_RPZ_POLICY_TCP_ONLY},
		{"nxdomain",	DNS_RPZ_POLICY_NXDOMAIN},
		{"nodata",	DNS_RPZ_POLICY_NODATA},
		{"cname",	DNS_RPZ_POLICY_CNAME},
		{"no-op",	DNS_RPZ_POLICY_PASSTHRU},   /* old passthru */
	};
	unsigned int n;

	if (str == NULL)
		return (DNS_RPZ_POLICY_ERROR);
	for (n = 0; n < sizeof(tbl)/sizeof(tbl[0]); ++n) {
		if (!strcasecmp(tbl[n].str, str))
			return (tbl[n].policy);
	}
	return (DNS_RPZ_POLICY_ERROR);
}

const char *
dns_rpz_policy2str(dns_rpz_policy_t policy) {
	const char *str;

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
	default:
		INSIST(0);
		ISC_UNREACHABLE();
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
	if ((zbit & 2) != 0)
		++rpz_num;
	return (rpz_num);
}

/*
 * Make a set of bit masks given one or more bits and their type.
 */
static void
make_addr_set(dns_rpz_addr_zbits_t *tgt_set, dns_rpz_zbits_t zbits,
	      dns_rpz_type_t type)
{
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
		INSIST(0);
		ISC_UNREACHABLE();
	}
}

static void
make_nm_set(dns_rpz_nm_zbits_t *tgt_set,
	    dns_rpz_num_t rpz_num, dns_rpz_type_t type)
{
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
		INSIST(0);
		ISC_UNREACHABLE();
	}
}

/*
 * Mark a node and all of its parents as having client-IP, IP, or NSIP data
 */
static void
set_sum_pair(dns_rpz_cidr_node_t *cnode) {
	dns_rpz_cidr_node_t *child;
	dns_rpz_addr_zbits_t sum;

	do {
		sum = cnode->set;

		child = cnode->child[0];
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
		    cnode->sum.ip == sum.ip &&
		    cnode->sum.nsip == sum.nsip)
			break;
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
			     rpzs->have.nsdname |
			     rpzs->have.nsipv4 | rpzs->have.nsipv6);
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
		      (uint64_t) mask);
	rpzs->have.qname_skip_recurse = mask;
}

static void
adj_trigger_cnt(dns_rpz_zones_t *rpzs, dns_rpz_num_t rpz_num,
		dns_rpz_type_t rpz_type,
		const dns_rpz_cidr_key_t *tgt_ip, dns_rpz_prefix_t tgt_prefix,
		bool inc)
{
	dns_rpz_trigger_counter_t *cnt = NULL;
	dns_rpz_zbits_t *have = NULL;

	switch (rpz_type) {
	case DNS_RPZ_TYPE_CLIENT_IP:
		REQUIRE(tgt_ip != NULL);
		if (KEY_IS_IPV4(tgt_prefix, tgt_ip)) {
			cnt = &rpzs->triggers[rpz_num].client_ipv4;
			have = &rpzs->have.client_ipv4;
		} else {
			cnt = &rpzs->triggers[rpz_num].client_ipv6;
			have = &rpzs->have.client_ipv6;
		}
		break;
	case DNS_RPZ_TYPE_QNAME:
		cnt = &rpzs->triggers[rpz_num].qname;
		have = &rpzs->have.qname;
		break;
	case DNS_RPZ_TYPE_IP:
		REQUIRE(tgt_ip != NULL);
		if (KEY_IS_IPV4(tgt_prefix, tgt_ip)) {
			cnt = &rpzs->triggers[rpz_num].ipv4;
			have = &rpzs->have.ipv4;
		} else {
			cnt = &rpzs->triggers[rpz_num].ipv6;
			have = &rpzs->have.ipv6;
		}
		break;
	case DNS_RPZ_TYPE_NSDNAME:
		cnt = &rpzs->triggers[rpz_num].nsdname;
		have = &rpzs->have.nsdname;
		break;
	case DNS_RPZ_TYPE_NSIP:
		REQUIRE(tgt_ip != NULL);
		if (KEY_IS_IPV4(tgt_prefix, tgt_ip)) {
			cnt = &rpzs->triggers[rpz_num].nsipv4;
			have = &rpzs->have.nsipv4;
		} else {
			cnt = &rpzs->triggers[rpz_num].nsipv6;
			have = &rpzs->have.nsipv6;
		}
		break;
	default:
		INSIST(0);
		ISC_UNREACHABLE();
	}

	if (inc) {
		if (++*cnt == 1U) {
			*have |= DNS_RPZ_ZBIT(rpz_num);
			fix_qname_skip_recurse(rpzs);
		}
	} else {
		REQUIRE(*cnt != 0U);
		if (--*cnt == 0U) {
			*have &= ~DNS_RPZ_ZBIT(rpz_num);
			fix_qname_skip_recurse(rpzs);
		}
	}
}

static dns_rpz_cidr_node_t *
new_node(dns_rpz_zones_t *rpzs,
	 const dns_rpz_cidr_key_t *ip, dns_rpz_prefix_t prefix,
	 const dns_rpz_cidr_node_t *child)
{
	dns_rpz_cidr_node_t *node;
	int i, words, wlen;

	node = isc_mem_get(rpzs->mctx, sizeof(*node));
	if (node == NULL)
		return (NULL);
	memset(node, 0, sizeof(*node));

	if (child != NULL)
		node->sum = child->sum;

	node->prefix = prefix;
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
	while (i < DNS_RPZ_CIDR_WORDS)
		node->ip.w[i++] = 0;

	return (node);
}

static void
badname(int level, const dns_name_t *name, const char *str1, const char *str2) {
	char namebuf[DNS_NAME_FORMATSIZE];

	/*
	 * bin/tests/system/rpz/tests.sh looks for "invalid rpz".
	 */
	if (level < DNS_RPZ_DEBUG_QUIET &&
	    isc_log_wouldlog(dns_lctx, level)) {
		dns_name_format(name, namebuf, sizeof(namebuf));
		isc_log_write(dns_lctx, DNS_LOGCATEGORY_RPZ,
			      DNS_LOGMODULE_RBTDB, level,
			      "invalid rpz IP address \"%s\"%s%s",
			      namebuf, str1, str2);
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
	const dns_name_t *base_name, dns_name_t *ip_name)
{
#ifndef INET6_ADDRSTRLEN
#define INET6_ADDRSTRLEN 46
#endif
	int w[DNS_RPZ_CIDR_WORDS*2];
	char str[1+8+1+INET6_ADDRSTRLEN+1];
	isc_buffer_t buffer;
	isc_result_t result;
	int best_first, best_len, cur_first, cur_len;
	int i, n, len;

	if (KEY_IS_IPV4(tgt_prefix, tgt_ip)) {
		len = snprintf(str, sizeof(str), "%u.%u.%u.%u.%u",
			       tgt_prefix - 96U,
			       tgt_ip->w[3] & 0xffU,
			       (tgt_ip->w[3]>>8) & 0xffU,
			       (tgt_ip->w[3]>>16) & 0xffU,
			       (tgt_ip->w[3]>>24) & 0xffU);
		if (len < 0 || len > (int)sizeof(str)) {
			return (ISC_R_FAILURE);
		}
	} else {
		len = snprintf(str, sizeof(str), "%d", tgt_prefix);
		if (len == -1) {
			return (ISC_R_FAILURE);
		}

		for (i = 0; i < DNS_RPZ_CIDR_WORDS; i++) {
			w[i*2+1] = ((tgt_ip->w[DNS_RPZ_CIDR_WORDS-1-i] >> 16)
				    & 0xffff);
			w[i*2] = tgt_ip->w[DNS_RPZ_CIDR_WORDS-1-i] & 0xffff;
		}
		/*
		 * Find the start and length of the first longest sequence
		 * of zeros in the address.
		 */
		best_first = -1;
		best_len = 0;
		cur_first = -1;
		cur_len = 0;
		for (n = 0; n <=7; ++n) {
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

		for (n = 0; n <= 7; ++n) {
			INSIST(len < (int)sizeof(str));
			if (n == best_first) {
				len += snprintf(str + len, sizeof(str) - len,
						".zz");
				n += best_len - 1;
			} else {
				len += snprintf(str + len, sizeof(str) - len,
						".%x", w[n]);
			}
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
type_from_name(const dns_rpz_zones_t *rpzs,
	       dns_rpz_zone_t *rpz, const dns_name_t *name)
{
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
name2ipkey(int log_level,
	   const dns_rpz_zones_t *rpzs, dns_rpz_num_t rpz_num,
	   dns_rpz_type_t rpz_type, const dns_name_t *src_name,
	   dns_rpz_cidr_key_t *tgt_ip, dns_rpz_prefix_t *tgt_prefix,
	   dns_rpz_addr_zbits_t *new_set)
{
	dns_rpz_zone_t *rpz;
	char ip_str[DNS_NAME_FORMATSIZE], ip2_str[DNS_NAME_FORMATSIZE];
	dns_offsets_t ip_name_offsets;
	dns_fixedname_t ip_name2f;
	dns_name_t ip_name, *ip_name2;
	const char *prefix_str, *cp, *end;
	char *cp2;
	int ip_labels;
	dns_rpz_prefix_t prefix;
	unsigned long prefix_num, l;
	isc_result_t result;
	int i;

	REQUIRE(rpzs != NULL && rpz_num < rpzs->p.num_zones);
	rpz = rpzs->zones[rpz_num];
	REQUIRE(rpz != NULL);

	make_addr_set(new_set, DNS_RPZ_ZBIT(rpz_num), rpz_type);

	ip_labels = dns_name_countlabels(src_name);
	if (rpz_type == DNS_RPZ_TYPE_QNAME)
		ip_labels -= dns_name_countlabels(&rpz->origin);
	else
		ip_labels -= dns_name_countlabels(&rpz->nsdname);
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
	end = &ip_str[strlen(ip_str)+1];
	prefix_str = ip_str;

	prefix_num = strtoul(prefix_str, &cp2, 10);
	if (*cp2 != '.') {
		badname(log_level, src_name,
			"; invalid leading prefix length", "");
		return (ISC_R_FAILURE);
	}
	/*
	 * Patch in trailing nul character to print just the length
	 * label (for various cases below).
	 */
	*cp2 = '\0';
	if (prefix_num < 1U || prefix_num > 128U) {
		badname(log_level, src_name,
			"; invalid prefix length of ", prefix_str);
		return (ISC_R_FAILURE);
	}
	cp = cp2+1;

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
				if (*cp2 == '.')
					*cp2 = '\0';
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
		for (i = 0;
		     ip_labels > 0 && i < DNS_RPZ_CIDR_WORDS * 2;
		     ip_labels--) {
			if (cp[0] == 'z' && cp[1] == 'z' &&
			    (cp[2] == '.' || cp[2] == '\0') &&
			    i <= 6) {
				do {
					if ((i & 1) == 0)
					    tgt_ip->w[3-i/2] = 0;
					++i;
				} while (ip_labels + i <= 8);
				cp += 3;
			} else {
				l = strtoul(cp, &cp2, 16);
				if (l > 0xffffu ||
				    (*cp2 != '.' && *cp2 != '\0')) {
					if (*cp2 == '.')
					    *cp2 = '\0';
					badname(log_level, src_name,
						"; invalid IPv6 word ", cp);
					return (ISC_R_FAILURE);
				}
				if ((i & 1) == 0)
					tgt_ip->w[3-i/2] = l;
				else
					tgt_ip->w[3-i/2] |= l << 16;
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
	    isc_log_wouldlog(dns_lctx, log_level)) {
		/*
		 * Convert the address back to a canonical domain name
		 * to ensure that the original name is in canonical form.
		 */
		ip_name2 = dns_fixedname_initname(&ip_name2f);
		result = ip2name(tgt_ip, (dns_rpz_prefix_t)prefix_num,
				 NULL, ip_name2);
		if (result != ISC_R_SUCCESS ||
		    !dns_name_equal(&ip_name, ip_name2)) {
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
name2data(dns_rpz_zones_t *rpzs, dns_rpz_num_t rpz_num,
	  dns_rpz_type_t rpz_type, const dns_name_t *src_name,
	  dns_name_t *trig_name, dns_rpz_nm_data_t *new_data)
{
	dns_rpz_zone_t *rpz;
	dns_offsets_t tmp_name_offsets;
	dns_name_t tmp_name;
	unsigned int prefix_len, n;

	REQUIRE(rpzs != NULL && rpz_num < rpzs->p.num_zones);
	rpz = rpzs->zones[rpz_num];
	REQUIRE(rpz != NULL);

	/*
	 * Handle wildcards by putting only the parent into the
	 * summary RBT.  The summary database only causes a check of the
	 * real policy zone where wildcards will be handled.
	 */
	if (dns_name_iswildcard(src_name)) {
		prefix_len = 1;
		memset(&new_data->set, 0, sizeof(new_data->set));
		make_nm_set(&new_data->wild, rpz_num, rpz_type);
	} else {
		prefix_len = 0;
		make_nm_set(&new_data->set, rpz_num, rpz_type);
		memset(&new_data->wild, 0, sizeof(new_data->wild));
	}

	dns_name_init(&tmp_name, tmp_name_offsets);
	n = dns_name_countlabels(src_name);
	n -= prefix_len;
	if (rpz_type == DNS_RPZ_TYPE_QNAME)
		n -= dns_name_countlabels(&rpz->origin);
	else
		n -= dns_name_countlabels(&rpz->nsdname);
	dns_name_getlabelsequence(src_name, prefix_len, n, &tmp_name);
	(void)dns_name_concatenate(&tmp_name, dns_rootname, trig_name, NULL);
}

#ifndef HAVE_BUILTIN_CLZ
/**
 * \brief Count Leading Zeros: Find the location of the left-most set
 * bit.
 */
static inline unsigned int
clz(dns_rpz_cidr_word_t w) {
	unsigned int bit;

	bit = DNS_RPZ_CIDR_WORD_BITS-1;

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

	if ((w & 2) != 0)
		--bit;

	return (bit);
}
#endif

/*
 * Find the first differing bit in two keys (IP addresses).
 */
static int
diff_keys(const dns_rpz_cidr_key_t *key1, dns_rpz_prefix_t prefix1,
	  const dns_rpz_cidr_key_t *key2, dns_rpz_prefix_t prefix2)
{
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
		if (ISC_UNLIKELY(delta != 0)) {
#ifdef HAVE_BUILTIN_CLZ
			bit += __builtin_clz(delta);
#else
			bit += clz(delta);
#endif
			break;
		}
	}
	return (ISC_MIN(bit, maxbit));
}

/*
 * Given a hit while searching the radix trees,
 * clear all bits for higher numbered zones.
 */
static inline dns_rpz_zbits_t
trim_zbits(dns_rpz_zbits_t zbits, dns_rpz_zbits_t found) {
	dns_rpz_zbits_t x;

	/*
	 * Isolate the first or smallest numbered hit bit.
	 * Make a mask of that bit and all smaller numbered bits.
	 */
	x = zbits & found;
	x &= (~x + 1);
	x = (x << 1) - 1;
	return (zbits &= x);
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
search(dns_rpz_zones_t *rpzs,
       const dns_rpz_cidr_key_t *tgt_ip, dns_rpz_prefix_t tgt_prefix,
       const dns_rpz_addr_zbits_t *tgt_set, bool create,
       dns_rpz_cidr_node_t **found)
{
	dns_rpz_cidr_node_t *cur, *parent, *child, *new_parent, *sibling;
	dns_rpz_addr_zbits_t set;
	int cur_num, child_num;
	dns_rpz_prefix_t dbit;
	isc_result_t find_result;

	set = *tgt_set;
	find_result = ISC_R_NOTFOUND;
	*found = NULL;
	cur = rpzs->cidr;
	parent = NULL;
	cur_num = 0;
	for (;;) {
		if (cur == NULL) {
			/*
			 * No child so we cannot go down.
			 * Quit with whatever we already found
			 * or add the target as a child of the current parent.
			 */
			if (!create)
				return (find_result);
			child = new_node(rpzs, tgt_ip, tgt_prefix, NULL);
			if (child == NULL)
				return (ISC_R_NOMEMORY);
			if (parent == NULL)
				rpzs->cidr = child;
			else
				parent->child[cur_num] = child;
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
		    (cur->sum.nsip & set.nsip) == 0) {
			/*
			 * This node has no relevant data
			 * and is in none of the target trees.
			 * Pretend it does not exist if we are not adding.
			 *
			 * If we are adding, continue down to eventually add
			 * a node and mark/put this node in the correct tree.
			 */
			if (!create)
				return (find_result);
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
				    (cur->set.nsip & set.nsip) != 0) {
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
			if (!create)
				return (find_result);

			new_parent = new_node(rpzs, tgt_ip, tgt_prefix, cur);
			if (new_parent == NULL)
				return (ISC_R_NOMEMORY);
			new_parent->parent = parent;
			if (parent == NULL)
				rpzs->cidr = new_parent;
			else
				parent->child[cur_num] = new_parent;
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
			    (cur->set.nsip & set.nsip) != 0) {
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
				set.ip = trim_zbits(set.ip,
						    cur->set.ip);
				set.nsip = trim_zbits(set.nsip,
						      cur->set.nsip);
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
		if (!create)
			return (find_result);

		sibling = new_node(rpzs, tgt_ip, tgt_prefix, NULL);
		if (sibling == NULL)
			return (ISC_R_NOMEMORY);
		new_parent = new_node(rpzs, tgt_ip, dbit, cur);
		if (new_parent == NULL) {
			isc_mem_put(rpzs->mctx, sibling, sizeof(*sibling));
			return (ISC_R_NOMEMORY);
		}
		new_parent->parent = parent;
		if (parent == NULL)
			rpzs->cidr = new_parent;
		else
			parent->child[cur_num] = new_parent;
		child_num = DNS_RPZ_IP_BIT(tgt_ip, dbit);
		new_parent->child[child_num] = sibling;
		new_parent->child[1-child_num] = cur;
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
add_cidr(dns_rpz_zones_t *rpzs, dns_rpz_num_t rpz_num,
	 dns_rpz_type_t rpz_type, const dns_name_t *src_name)
{
	dns_rpz_cidr_key_t tgt_ip;
	dns_rpz_prefix_t tgt_prefix;
	dns_rpz_addr_zbits_t set;
	dns_rpz_cidr_node_t *found;
	isc_result_t result;

	result = name2ipkey(DNS_RPZ_ERROR_LEVEL, rpzs, rpz_num, rpz_type,
			    src_name, &tgt_ip, &tgt_prefix, &set);
	/*
	 * Log complaints about bad owner names but let the zone load.
	 */
	if (result != ISC_R_SUCCESS)
		return (ISC_R_SUCCESS);

	result = search(rpzs, &tgt_ip, tgt_prefix, &set, true, &found);
	if (result != ISC_R_SUCCESS) {
		char namebuf[DNS_NAME_FORMATSIZE];

		/*
		 * Do not worry if the radix tree already exists,
		 * because diff_apply() likes to add nodes before deleting.
		 */
		if (result == ISC_R_EXISTS)
			return (ISC_R_SUCCESS);

		/*
		 * bin/tests/system/rpz/tests.sh looks for "rpz.*failed".
		 */
		dns_name_format(src_name, namebuf, sizeof(namebuf));
		isc_log_write(dns_lctx, DNS_LOGCATEGORY_RPZ,
			      DNS_LOGMODULE_RBTDB, DNS_RPZ_ERROR_LEVEL,
			      "rpz add_cidr(%s) failed: %s",
			      namebuf, isc_result_totext(result));
		return (result);
	}

	adj_trigger_cnt(rpzs, rpz_num, rpz_type, &tgt_ip, tgt_prefix, true);
	return (result);
}

static isc_result_t
add_nm(dns_rpz_zones_t *rpzs, dns_name_t *trig_name,
	 const dns_rpz_nm_data_t *new_data)
{
	dns_rbtnode_t *nmnode;
	dns_rpz_nm_data_t *nm_data;
	isc_result_t result;

	nmnode = NULL;
	result = dns_rbt_addnode(rpzs->rbt, trig_name, &nmnode);
	switch (result) {
	case ISC_R_SUCCESS:
	case ISC_R_EXISTS:
		nm_data = nmnode->data;
		if (nm_data == NULL) {
			nm_data = isc_mem_get(rpzs->mctx, sizeof(*nm_data));
			if (nm_data == NULL)
				return (ISC_R_NOMEMORY);
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
		return (ISC_R_EXISTS);

	nm_data->set.qname |= new_data->set.qname;
	nm_data->set.ns |= new_data->set.ns;
	nm_data->wild.qname |= new_data->wild.qname;
	nm_data->wild.ns |= new_data->wild.ns;
	return (ISC_R_SUCCESS);
}

static isc_result_t
add_name(dns_rpz_zones_t *rpzs, dns_rpz_num_t rpz_num,
	 dns_rpz_type_t rpz_type, const dns_name_t *src_name)
{
	dns_rpz_nm_data_t new_data;
	dns_fixedname_t trig_namef;
	dns_name_t *trig_name;
	isc_result_t result;

	/*
	 * We need a summary database of names even with 1 policy zone,
	 * because wildcard triggers are handled differently.
	 */

	trig_name = dns_fixedname_initname(&trig_namef);
	name2data(rpzs, rpz_num, rpz_type, src_name, trig_name, &new_data);

	result = add_nm(rpzs, trig_name, &new_data);

	/*
	 * Do not worry if the node already exists,
	 * because diff_apply() likes to add nodes before deleting.
	 */
	if (result == ISC_R_EXISTS)
		return (ISC_R_SUCCESS);
	if (result == ISC_R_SUCCESS)
		adj_trigger_cnt(rpzs, rpz_num, rpz_type, NULL, 0, true);
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
dns_rpz_new_zones(dns_rpz_zones_t **rpzsp, char *rps_cstr,
		  size_t rps_cstr_size, isc_mem_t *mctx,
		  isc_taskmgr_t *taskmgr, isc_timermgr_t *timermgr)
{
	dns_rpz_zones_t *zones;
	isc_result_t result;

	REQUIRE(rpzsp != NULL && *rpzsp == NULL);

	zones = isc_mem_get(mctx, sizeof(*zones));
	if (zones == NULL)
		return (ISC_R_NOMEMORY);
	memset(zones, 0, sizeof(*zones));

	result = isc_rwlock_init(&zones->search_lock, 0, 0);
	if (result != ISC_R_SUCCESS)
		goto cleanup_rwlock;

	isc_mutex_init(&zones->maint_lock);
	isc_refcount_init(&zones->refs, 1);
	isc_refcount_init(&zones->irefs, 1);

	zones->rps_cstr = rps_cstr;
	zones->rps_cstr_size = rps_cstr_size;
#ifdef USE_DNSRPS
	if (rps_cstr != NULL) {
		result = dns_dnsrps_view_init(zones, rps_cstr);
	}
#else
	INSIST(!zones->p.dnsrps_enabled);
#endif
	if (result == ISC_R_SUCCESS && !zones->p.dnsrps_enabled) {
		result = dns_rbt_create(mctx, rpz_node_deleter,
					mctx, &zones->rbt);
	}

	if (result != ISC_R_SUCCESS)
		goto cleanup_rbt;

	result = isc_task_create(taskmgr, 0, &zones->updater);
	if (result != ISC_R_SUCCESS)
		goto cleanup_task;

	isc_mem_attach(mctx, &zones->mctx);
	zones->timermgr = timermgr;
	zones->taskmgr = taskmgr;

	*rpzsp = zones;
	return (ISC_R_SUCCESS);

cleanup_task:
	dns_rbt_destroy(&zones->rbt);

cleanup_rbt:
	INSIST(isc_refcount_decrement(&zones->irefs) == 1);
	isc_refcount_destroy(&zones->irefs);
	INSIST(isc_refcount_decrement(&zones->refs) == 1);
	isc_refcount_destroy(&zones->refs);

	isc_mutex_destroy(&zones->maint_lock);

	isc_rwlock_destroy(&zones->search_lock);

cleanup_rwlock:
	isc_mem_put(mctx, zones, sizeof(*zones));

	return (result);
}

isc_result_t
dns_rpz_new_zone(dns_rpz_zones_t *rpzs, dns_rpz_zone_t **rpzp) {
	dns_rpz_zone_t *zone;
	isc_result_t result;

	REQUIRE(rpzp != NULL && *rpzp == NULL);
	REQUIRE(rpzs != NULL);
	if (rpzs->p.num_zones >= DNS_RPZ_MAX_ZONES) {
		return (ISC_R_NOSPACE);
	}

	zone = isc_mem_get(rpzs->mctx, sizeof(*zone));
	if (zone == NULL) {
		return (ISC_R_NOMEMORY);
	}

	memset(zone, 0, sizeof(*zone));
	isc_refcount_init(&zone->refs, 1);

	result = isc_timer_create(rpzs->timermgr, isc_timertype_inactive,
				  NULL, NULL, rpzs->updater,
				  dns_rpz_update_taskaction,
				  zone, &zone->updatetimer);
	if (result != ISC_R_SUCCESS)
		goto cleanup_timer;

	/*
	 * This will never be used, but costs us nothing and
	 * simplifies update_from_db
	 */

	result = isc_ht_init(&zone->nodes, rpzs->mctx, 1);
	if (result != ISC_R_SUCCESS)
		goto cleanup_ht;

	dns_name_init(&zone->origin, NULL);
	dns_name_init(&zone->client_ip, NULL);
	dns_name_init(&zone->ip, NULL);
	dns_name_init(&zone->nsdname, NULL);
	dns_name_init(&zone->nsip, NULL);
	dns_name_init(&zone->passthru, NULL);
	dns_name_init(&zone->drop, NULL);
	dns_name_init(&zone->tcp_only, NULL);
	dns_name_init(&zone->cname, NULL);

	isc_time_settoepoch(&zone->lastupdated);
	zone->updatepending = false;
	zone->updaterunning = false;
	zone->db = NULL;
	zone->dbversion = NULL;
	zone->updb = NULL;
	zone->updbversion = NULL;
	zone->updbit = NULL;
	isc_refcount_increment(&rpzs->irefs);
	zone->rpzs = rpzs;
	zone->db_registered = false;
	zone->addsoa = true;
	ISC_EVENT_INIT(&zone->updateevent, sizeof(zone->updateevent),
		       0, NULL, 0, NULL, NULL, NULL, NULL, NULL);

	zone->num = rpzs->p.num_zones++;
	rpzs->zones[zone->num] = zone;

	*rpzp = zone;

	return (ISC_R_SUCCESS);

cleanup_ht:
	isc_timer_detach(&zone->updatetimer);

cleanup_timer:
	INSIST(isc_refcount_decrement(&zone->refs) > 0);
	isc_refcount_destroy(&zone->refs);

	isc_mem_put(rpzs->mctx, zone, sizeof(*zone));

	return (result);
}

isc_result_t
dns_rpz_dbupdate_callback(dns_db_t *db, void *fn_arg) {
	dns_rpz_zone_t *zone = (dns_rpz_zone_t *) fn_arg;
	isc_time_t now;
	uint64_t tdiff;
	isc_result_t result = ISC_R_SUCCESS;
	char dname[DNS_NAME_FORMATSIZE];

	REQUIRE(DNS_DB_VALID(db));
	REQUIRE(zone != NULL);

	LOCK(&zone->rpzs->maint_lock);

	/* New zone came as AXFR */
	if (zone->db != NULL && zone->db != db) {
		/* We need to clean up the old DB */
		if (zone->dbversion != NULL)
			dns_db_closeversion(zone->db, &zone->dbversion,
					    false);
		dns_db_updatenotify_unregister(zone->db,
					       dns_rpz_dbupdate_callback,
					       zone);
		dns_db_detach(&zone->db);
	}

	if (zone->db == NULL) {
		RUNTIME_CHECK(zone->dbversion == NULL);
		dns_db_attach(db, &zone->db);
	}

	if (!zone->updatepending && !zone->updaterunning) {
		zone->updatepending = true;
		isc_time_now(&now);
		tdiff = isc_time_microdiff(&now, &zone->lastupdated) / 1000000;
		if (tdiff < zone->min_update_interval) {
			uint64_t defer = zone->min_update_interval - tdiff;
			isc_interval_t interval;
			dns_name_format(&zone->origin, dname,
					DNS_NAME_FORMATSIZE);
			isc_log_write(dns_lctx, DNS_LOGCATEGORY_GENERAL,
				      DNS_LOGMODULE_MASTER, ISC_LOG_INFO,
				      "rpz: %s: new zone version came "
				      "too soon, deferring update for "
				      "%" PRIu64 " seconds", dname, defer);
			isc_interval_set(&interval, (unsigned int)defer, 0);
			dns_db_currentversion(zone->db, &zone->dbversion);
			result = isc_timer_reset(zone->updatetimer,
						 isc_timertype_once,
						 NULL, &interval, true);
			if (result != ISC_R_SUCCESS)
				goto cleanup;
		} else {
			isc_event_t *event;

			dns_db_currentversion(zone->db, &zone->dbversion);
			INSIST(!ISC_LINK_LINKED(&zone->updateevent, ev_link));
			ISC_EVENT_INIT(&zone->updateevent,
				       sizeof(zone->updateevent), 0, NULL,
				       DNS_EVENT_RPZUPDATED,
				       dns_rpz_update_taskaction,
				       zone, zone, NULL, NULL);
			event = &zone->updateevent;
			isc_task_send(zone->rpzs->updater, &event);
		}
	} else {
		zone->updatepending = true;
		dns_name_format(&zone->origin, dname, DNS_NAME_FORMATSIZE);
		isc_log_write(dns_lctx, DNS_LOGCATEGORY_GENERAL,
			      DNS_LOGMODULE_MASTER, ISC_LOG_DEBUG(3),
			      "rpz: %s: update already queued or running",
			      dname);
		if (zone->dbversion != NULL) {
			dns_db_closeversion(zone->db, &zone->dbversion,
					    false);
		}
		dns_db_currentversion(zone->db, &zone->dbversion);
	}

  cleanup:
	UNLOCK(&zone->rpzs->maint_lock);

	return (result);
}

static void
dns_rpz_update_taskaction(isc_task_t *task, isc_event_t *event) {
	isc_result_t result;
	dns_rpz_zone_t *zone;

	REQUIRE(event != NULL);
	REQUIRE(event->ev_arg != NULL);

	UNUSED(task);
	zone = (dns_rpz_zone_t *) event->ev_arg;
	isc_event_free(&event);
	LOCK(&zone->rpzs->maint_lock);
	zone->updatepending = false;
	zone->updaterunning = true;
	dns_rpz_update_from_db(zone);
	result = isc_timer_reset(zone->updatetimer, isc_timertype_inactive,
				 NULL, NULL, true);
	RUNTIME_CHECK(result == ISC_R_SUCCESS);
	result = isc_time_now(&zone->lastupdated);
	RUNTIME_CHECK(result == ISC_R_SUCCESS);
	UNLOCK(&zone->rpzs->maint_lock);
}

static isc_result_t
setup_update(dns_rpz_zone_t *rpz) {
	isc_result_t result;
	char domain[DNS_NAME_FORMATSIZE];
	unsigned int nodecount;
	uint32_t hashsize;

	dns_name_format(&rpz->origin, domain, DNS_NAME_FORMATSIZE);
	isc_log_write(dns_lctx, DNS_LOGCATEGORY_GENERAL,
		      DNS_LOGMODULE_MASTER, ISC_LOG_INFO,
		      "rpz: %s: reload start", domain);

	nodecount = dns_db_nodecount(rpz->updb);
	hashsize = 1;
	while (nodecount != 0 &&
	       hashsize <= (DNS_RPZ_HTSIZE_MAX + DNS_RPZ_HTSIZE_DIV))
	{
		hashsize++;
		nodecount >>=1;
	}

	if (hashsize > DNS_RPZ_HTSIZE_DIV)
		hashsize -= DNS_RPZ_HTSIZE_DIV;

	isc_log_write(dns_lctx, DNS_LOGCATEGORY_GENERAL,
			      DNS_LOGMODULE_MASTER, ISC_LOG_DEBUG(1),
			      "rpz: %s: using hashtable size %d",
			      domain, hashsize);

	result = isc_ht_init(&rpz->newnodes, rpz->rpzs->mctx, hashsize);
	if (result != ISC_R_SUCCESS) {
		isc_log_write(dns_lctx, DNS_LOGCATEGORY_GENERAL,
			      DNS_LOGMODULE_MASTER, ISC_LOG_ERROR,
			      "rpz: %s: failed to initialize hashtable - %s",
			      domain, isc_result_totext(result));
		goto cleanup;
	}

	result = dns_db_createiterator(rpz->updb, DNS_DB_NONSEC3, &rpz->updbit);
	if (result != ISC_R_SUCCESS) {
		isc_log_write(dns_lctx, DNS_LOGCATEGORY_GENERAL,
			      DNS_LOGMODULE_MASTER, ISC_LOG_ERROR,
			      "rpz: %s: failed to create DB iterator - %s",
			      domain, isc_result_totext(result));
		goto cleanup;
	}

	result = dns_dbiterator_first(rpz->updbit);
	if (result != ISC_R_SUCCESS) {
		isc_log_write(dns_lctx, DNS_LOGCATEGORY_GENERAL,
			      DNS_LOGMODULE_MASTER, ISC_LOG_ERROR,
			      "rpz: %s: failed to get db iterator - %s",
			      domain, isc_result_totext(result));
		goto cleanup;
	}

	result = dns_dbiterator_pause(rpz->updbit);
	if (result != ISC_R_SUCCESS) {
		isc_log_write(dns_lctx, DNS_LOGCATEGORY_GENERAL,
			      DNS_LOGMODULE_MASTER, ISC_LOG_ERROR,
			      "rpz: %s: failed to pause db iterator - %s",
			      domain, isc_result_totext(result));
		goto cleanup;
	}


 cleanup:
	if (result != ISC_R_SUCCESS) {
		if (rpz->updbit != NULL)
			dns_dbiterator_destroy(&rpz->updbit);
		if (rpz->newnodes != NULL)
			isc_ht_destroy(&rpz->newnodes);
		dns_db_closeversion(rpz->updb, &rpz->updbversion, false);
	}

	return (result);
}

static void
finish_update(dns_rpz_zone_t *rpz) {
	isc_result_t result;
	isc_ht_t *tmpht = NULL;
	isc_ht_iter_t *iter = NULL;
	dns_fixedname_t fname;
	char dname[DNS_NAME_FORMATSIZE];
	dns_name_t *name;

	/*
	 * Iterate over old ht with existing nodes deleted to delete
	 * deleted nodes from RPZ
	 */
	result = isc_ht_iter_create(rpz->nodes, &iter);
	if (result != ISC_R_SUCCESS) {
		char domain[DNS_NAME_FORMATSIZE];

		dns_name_format(&rpz->origin, domain, DNS_NAME_FORMATSIZE);
		isc_log_write(dns_lctx, DNS_LOGCATEGORY_GENERAL,
			      DNS_LOGMODULE_MASTER, ISC_LOG_ERROR,
			      "rpz: %s: failed to create HT iterator - %s",
			      domain, isc_result_totext(result));
		goto cleanup;
	}

	name = dns_fixedname_initname(&fname);

	for (result = isc_ht_iter_first(iter);
	     result == ISC_R_SUCCESS;
	     result = isc_ht_iter_delcurrent_next(iter))
	{
		isc_region_t region;
		unsigned char *key = NULL;
		size_t keysize;

		isc_ht_iter_currentkey(iter, &key, &keysize);
		region.base = key;
		region.length = (unsigned int)keysize;
		dns_name_fromregion(name, &region);
		dns_rpz_delete(rpz->rpzs, rpz->num, name);
	}

	tmpht = rpz->nodes;
	rpz->nodes = rpz->newnodes;
	rpz->newnodes = tmpht;

	LOCK(&rpz->rpzs->maint_lock);
	rpz->updaterunning = false;
	/*
	 * If there's an update pending schedule it
	 */
	if (rpz->updatepending == true) {
		if (rpz->min_update_interval > 0) {
			uint64_t defer = rpz->min_update_interval;
			isc_interval_t interval;
			dns_name_format(&rpz->origin, dname,
					DNS_NAME_FORMATSIZE);
			isc_log_write(dns_lctx, DNS_LOGCATEGORY_GENERAL,
				      DNS_LOGMODULE_MASTER, ISC_LOG_INFO,
				      "rpz: %s: new zone version came "
				      "too soon, deferring update for "
				      "%" PRIu64 " seconds", dname, defer);
			isc_interval_set(&interval, (unsigned int)defer, 0);
			isc_timer_reset(rpz->updatetimer, isc_timertype_once,
					NULL, &interval, true);
		} else {
			isc_event_t *event;
			INSIST(!ISC_LINK_LINKED(&rpz->updateevent, ev_link));
			ISC_EVENT_INIT(&rpz->updateevent,
				       sizeof(rpz->updateevent), 0, NULL,
				       DNS_EVENT_RPZUPDATED,
				       dns_rpz_update_taskaction,
				       rpz, rpz, NULL, NULL);
			event = &rpz->updateevent;
			isc_task_send(rpz->rpzs->updater, &event);
		}
	}
	UNLOCK(&rpz->rpzs->maint_lock);

cleanup:
	if (iter != NULL)
		isc_ht_iter_destroy(&iter);
}

static void
update_quantum(isc_task_t *task, isc_event_t *event) {
	isc_result_t result = ISC_R_SUCCESS;
	dns_dbnode_t *node = NULL;
	dns_rpz_zone_t *rpz;
	char domain[DNS_NAME_FORMATSIZE];
	dns_fixedname_t fixname;
	dns_name_t *name;
	int count = 0;

	UNUSED(task);

	REQUIRE(event != NULL);
	REQUIRE(event->ev_arg != NULL);

	rpz = (dns_rpz_zone_t *) event->ev_arg;
	isc_event_free(&event);

	REQUIRE(rpz->updbit != NULL);
	REQUIRE(rpz->newnodes != NULL);

	name = dns_fixedname_initname(&fixname);

	dns_name_format(&rpz->origin, domain, DNS_NAME_FORMATSIZE);

	while (result == ISC_R_SUCCESS && count++ < DNS_RPZ_QUANTUM) {
		char namebuf[DNS_NAME_FORMATSIZE];
		dns_rdatasetiter_t *rdsiter = NULL;

		result = dns_dbiterator_current(rpz->updbit, &node, name);
		if (result != ISC_R_SUCCESS) {
			isc_log_write(dns_lctx, DNS_LOGCATEGORY_GENERAL,
				      DNS_LOGMODULE_MASTER, ISC_LOG_ERROR,
				      "rpz: %s: failed to get dbiterator - %s",
				      domain, isc_result_totext(result));
			dns_db_detachnode(rpz->updb, &node);
			break;
		}

		result = dns_db_allrdatasets(rpz->updb, node, rpz->updbversion,
					     0, &rdsiter);
		if (result != ISC_R_SUCCESS) {
			isc_log_write(dns_lctx, DNS_LOGCATEGORY_GENERAL,
				      DNS_LOGMODULE_MASTER, ISC_LOG_ERROR,
				      "rpz: %s: failed to fetch "
				      "rrdatasets - %s",
				      domain, isc_result_totext(result));
			dns_db_detachnode(rpz->updb, &node);
			break;
		}

		result = dns_rdatasetiter_first(rdsiter);
		dns_rdatasetiter_destroy(&rdsiter);
		if (result != ISC_R_SUCCESS) { /* empty non-terminal */
			if (result != ISC_R_NOMORE)
				isc_log_write(dns_lctx, DNS_LOGCATEGORY_GENERAL,
				      DNS_LOGMODULE_MASTER, ISC_LOG_ERROR,
				      "rpz: %s: error %s while creating "
				      "rdatasetiter",
				      domain, isc_result_totext(result));
			dns_db_detachnode(rpz->updb, &node);
			result = dns_dbiterator_next(rpz->updbit);
			continue;
		}

		result = isc_ht_add(rpz->newnodes, name->ndata,
				    name->length, rpz);
		if (result != ISC_R_SUCCESS) {
			dns_name_format(name, namebuf, sizeof(namebuf));
			isc_log_write(dns_lctx, DNS_LOGCATEGORY_GENERAL,
				      DNS_LOGMODULE_MASTER, ISC_LOG_ERROR,
				      "rpz: %s, adding node %s to HT error %s",
				      domain, namebuf,
				      isc_result_totext(result));
			dns_db_detachnode(rpz->updb, &node);
			result = dns_dbiterator_next(rpz->updbit);
			continue;
		}

		result = isc_ht_find(rpz->nodes, name->ndata,
				     name->length, NULL);
		if (result == ISC_R_SUCCESS) {
			isc_ht_delete(rpz->nodes, name->ndata, name->length);
		} else { /* not found */
			result = dns_rpz_add(rpz->rpzs, rpz->num, name);
			if (result != ISC_R_SUCCESS) {
				dns_name_format(name, namebuf, sizeof(namebuf));
				isc_log_write(dns_lctx,
					      DNS_LOGCATEGORY_GENERAL,
					      DNS_LOGMODULE_MASTER,
					      ISC_LOG_ERROR,
					      "rpz: %s: adding node %s "
					      "to RPZ error %s",
					      domain, namebuf,
					      isc_result_totext(result));
			} else {
				dns_name_format(name, namebuf, sizeof(namebuf));
				isc_log_write(dns_lctx,
					      DNS_LOGCATEGORY_GENERAL,
					      DNS_LOGMODULE_MASTER,
					      ISC_LOG_DEBUG(3),
					      "rpz: %s: adding node %s",
					      domain, namebuf);
			}
		}

		dns_db_detachnode(rpz->updb, &node);
		result = dns_dbiterator_next(rpz->updbit);
	}

	if (result == ISC_R_SUCCESS) {
		isc_event_t *nevent;
		/*
		 * Pause the iterator so that the DB is not locked
		 */
		dns_dbiterator_pause(rpz->updbit);
		/*
		 * We finished a quantum; trigger the next one and return
		 */
		INSIST(!ISC_LINK_LINKED(&rpz->updateevent, ev_link));
		ISC_EVENT_INIT(&rpz->updateevent,
			       sizeof(rpz->updateevent), 0, NULL,
			       DNS_EVENT_RPZUPDATED,
			       update_quantum,
			       rpz, rpz, NULL, NULL);
		nevent = &rpz->updateevent;
		isc_task_send(rpz->rpzs->updater, &nevent);
		return;
	} else if (result == ISC_R_NOMORE) {
		/*
		 * All done.
		 */
		finish_update(rpz);
		isc_log_write(dns_lctx, DNS_LOGCATEGORY_GENERAL,
			      DNS_LOGMODULE_MASTER, ISC_LOG_INFO,
			      "rpz: %s: reload done", domain);
	}

	/*
	 * If we're here, we've either finished or something went wrong,
	 * so clean up.
	 */
	if (rpz->updbit != NULL)
		dns_dbiterator_destroy(&rpz->updbit);
	if (rpz->newnodes != NULL)
		isc_ht_destroy(&rpz->newnodes);
	dns_db_closeversion(rpz->updb, &rpz->updbversion, false);
	dns_db_detach(&rpz->updb);
	rpz_detach(&rpz);
}

static void
dns_rpz_update_from_db(dns_rpz_zone_t *rpz) {
	isc_result_t result;
	isc_event_t *event;

	REQUIRE(rpz != NULL);
	REQUIRE(DNS_DB_VALID(rpz->db));
	REQUIRE(rpz->updb == NULL);
	REQUIRE(rpz->updbversion == NULL);
	REQUIRE(rpz->updbit == NULL);
	REQUIRE(rpz->newnodes == NULL);

	isc_refcount_increment(&rpz->refs);
	dns_db_attach(rpz->db, &rpz->updb);
	rpz->updbversion = rpz->dbversion;
	rpz->dbversion = NULL;

	result = setup_update(rpz);
	if (result != ISC_R_SUCCESS) {
		goto cleanup;
	}

	event = &rpz->updateevent;
	INSIST(!ISC_LINK_LINKED(&rpz->updateevent, ev_link));
	ISC_EVENT_INIT(&rpz->updateevent, sizeof(rpz->updateevent),
		       0, NULL, DNS_EVENT_RPZUPDATED,
		       update_quantum, rpz, rpz, NULL, NULL);
	isc_task_send(rpz->rpzs->updater, &event);
	return;

 cleanup:
	if (rpz->updbit != NULL)
		dns_dbiterator_destroy(&rpz->updbit);
	if (rpz->newnodes != NULL)
		isc_ht_destroy(&rpz->newnodes);
	dns_db_closeversion(rpz->updb, &rpz->updbversion, false);
	dns_db_detach(&rpz->updb);
	rpz_detach(&rpz);
}

/*
 * Free the radix tree of a response policy database.
 */
static void
cidr_free(dns_rpz_zones_t *rpzs) {
	dns_rpz_cidr_node_t *cur, *child, *parent;

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
		if (parent == NULL)
			rpzs->cidr = NULL;
		else
			parent->child[parent->child[1] == cur] = NULL;
		isc_mem_put(rpzs->mctx, cur, sizeof(*cur));
		cur = parent;
	}
}

/*
 * Discard a response policy zone blob
 * before discarding the overall rpz structure.
 */
static void
rpz_detach(dns_rpz_zone_t **rpzp) {
	dns_rpz_zone_t *rpz;
	dns_rpz_zones_t *rpzs;

	REQUIRE(rpzp != NULL && *rpzp != NULL);

	rpz = *rpzp;
	*rpzp = NULL;

	if (isc_refcount_decrement(&rpz->refs) != 1) {
		return;
	}

	isc_refcount_destroy(&rpz->refs);

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
	if (rpz->dbversion != NULL) {
		dns_db_closeversion(rpz->db, &rpz->dbversion, false);
	}
	if (rpz->db != NULL) {
		dns_db_updatenotify_unregister(
			rpz->db, dns_rpz_dbupdate_callback, rpz);
		dns_db_detach(&rpz->db);
	}
	if (rpz->updaterunning) {
		isc_task_purgeevent(rpzs->updater, &rpz->updateevent);
		if (rpz->updbit != NULL) {
			dns_dbiterator_destroy(&rpz->updbit);
		}
		if (rpz->newnodes != NULL) {
			isc_ht_destroy(&rpz->newnodes);
		}
		dns_db_closeversion(rpz->updb, &rpz->updbversion, false);
		dns_db_detach(&rpz->updb);
	}

	isc_timer_reset(rpz->updatetimer, isc_timertype_inactive,
			NULL, NULL, true);
	isc_timer_detach(&rpz->updatetimer);

	isc_ht_destroy(&rpz->nodes);

	isc_mem_put(rpzs->mctx, rpz, sizeof(*rpz));
	rpz_detach_rpzs(&rpzs);
}

void
dns_rpz_attach_rpzs(dns_rpz_zones_t *rpzs, dns_rpz_zones_t **rpzsp) {
	REQUIRE(rpzsp != NULL && *rpzsp == NULL);
	isc_refcount_increment(&rpzs->refs);
	*rpzsp = rpzs;
}

/*
 * Forget a view's policy zones.
 */
void
dns_rpz_detach_rpzs(dns_rpz_zones_t **rpzsp) {
	REQUIRE(rpzsp != NULL && *rpzsp != NULL);
	dns_rpz_zones_t *rpzs = *rpzsp;
	*rpzsp = NULL;

	if (isc_refcount_decrement(&rpzs->refs) == 1) {

		isc_task_destroy(&rpzs->updater);

		/*
		 * Forget the last of view's rpz machinery after
		 * the last reference.
		 */
		for (dns_rpz_num_t rpz_num = 0;
		     rpz_num < DNS_RPZ_MAX_ZONES;
		     ++rpz_num)
		{
			dns_rpz_zone_t *rpz = rpzs->zones[rpz_num];
			rpzs->zones[rpz_num] = NULL;
			if (rpz != NULL) {
				rpz_detach(&rpz);
			}
		}
		rpz_detach_rpzs(&rpzs);
	}
}

static void
rpz_detach_rpzs(dns_rpz_zones_t **rpzsp) {
	REQUIRE(rpzsp != NULL && *rpzsp != NULL);
	dns_rpz_zones_t *rpzs = *rpzsp;
	*rpzsp = NULL;

	if (isc_refcount_decrement(&rpzs->irefs) == 1) {
		if (rpzs->rps_cstr_size != 0) {
#ifdef USE_DNSRPS
			librpz->client_detach(&rpzs->rps_client);
#endif
			isc_mem_put(rpzs->mctx, rpzs->rps_cstr,
				    rpzs->rps_cstr_size);
		}

		cidr_free(rpzs);
		if (rpzs->rbt != NULL) {
			dns_rbt_destroy(&rpzs->rbt);
		}
		isc_mutex_destroy(&rpzs->maint_lock);
		isc_rwlock_destroy(&rpzs->search_lock);
		isc_refcount_destroy(&rpzs->refs);
		isc_mem_putanddetach(&rpzs->mctx, rpzs, sizeof(*rpzs));
	}
}

/*
 * Deprecated and removed.
 */
isc_result_t
dns_rpz_beginload(dns_rpz_zones_t **load_rpzsp,
		  dns_rpz_zones_t *rpzs, dns_rpz_num_t rpz_num)
{
	UNUSED(load_rpzsp);
	UNUSED(rpzs);
	UNUSED(rpz_num);

	return (ISC_R_NOTIMPLEMENTED);
}

/*
 * Deprecated and removed.
 */
isc_result_t
dns_rpz_ready(dns_rpz_zones_t *rpzs,
	      dns_rpz_zones_t **load_rpzsp, dns_rpz_num_t rpz_num)
{
	UNUSED(rpzs);
	UNUSED(load_rpzsp);
	UNUSED(rpz_num);

	return (ISC_R_NOTIMPLEMENTED);
}

/*
 * Add an IP address to the radix tree or a name to the summary database.
 */
isc_result_t
dns_rpz_add(dns_rpz_zones_t *rpzs, dns_rpz_num_t rpz_num,
	    const dns_name_t *src_name)
{
	dns_rpz_zone_t *rpz;
	dns_rpz_type_t rpz_type;
	isc_result_t result = ISC_R_FAILURE;

	REQUIRE(rpzs != NULL && rpz_num < rpzs->p.num_zones);
	rpz = rpzs->zones[rpz_num];
	REQUIRE(rpz != NULL);
	RWLOCK(&rpzs->search_lock, isc_rwlocktype_write);

	rpz_type = type_from_name(rpzs, rpz, src_name);


	switch (rpz_type) {
	case DNS_RPZ_TYPE_QNAME:
	case DNS_RPZ_TYPE_NSDNAME:
		result = add_name(rpzs, rpz_num, rpz_type, src_name);
		break;
	case DNS_RPZ_TYPE_CLIENT_IP:
	case DNS_RPZ_TYPE_IP:
	case DNS_RPZ_TYPE_NSIP:
		result = add_cidr(rpzs, rpz_num, rpz_type, src_name);
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
del_cidr(dns_rpz_zones_t *rpzs, dns_rpz_num_t rpz_num,
	 dns_rpz_type_t rpz_type, const dns_name_t *src_name)
{
	isc_result_t result;
	dns_rpz_cidr_key_t tgt_ip;
	dns_rpz_prefix_t tgt_prefix;
	dns_rpz_addr_zbits_t tgt_set;
	dns_rpz_cidr_node_t *tgt, *parent, *child;

	/*
	 * Do not worry about invalid rpz IP address names.  If we
	 * are here, then something relevant was added and so was
	 * valid.  Invalid names here are usually internal RBTDB nodes.
	 */
	result = name2ipkey(DNS_RPZ_DEBUG_QUIET, rpzs, rpz_num, rpz_type,
			    src_name, &tgt_ip, &tgt_prefix, &tgt_set);
	if (result != ISC_R_SUCCESS)
		return;

	result = search(rpzs, &tgt_ip, tgt_prefix, &tgt_set, false, &tgt);
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

	adj_trigger_cnt(rpzs, rpz_num, rpz_type, &tgt_ip, tgt_prefix,
			false);

	/*
	 * We might need to delete 2 nodes.
	 */
	do {
		/*
		 * The node is now useless if it has no data of its own
		 * and 0 or 1 children.  We are finished if it is not useless.
		 */
		if ((child = tgt->child[0]) != NULL) {
			if (tgt->child[1] != NULL)
				break;
		} else {
			child = tgt->child[1];
		}
		if (tgt->set.client_ip != 0 ||
		    tgt->set.ip != 0 ||
		    tgt->set.nsip != 0)
			break;

		/*
		 * Replace the pointer to this node in the parent with
		 * the remaining child or NULL.
		 */
		parent = tgt->parent;
		if (parent == NULL) {
			rpzs->cidr = child;
		} else {
			parent->child[parent->child[1] == tgt] = child;
		}
		/*
		 * If the child exists fix up its parent pointer.
		 */
		if (child != NULL)
			child->parent = parent;
		isc_mem_put(rpzs->mctx, tgt, sizeof(*tgt));

		tgt = parent;
	} while (tgt != NULL);
}

static void
del_name(dns_rpz_zones_t *rpzs, dns_rpz_num_t rpz_num,
	 dns_rpz_type_t rpz_type, const dns_name_t *src_name)
{
	char namebuf[DNS_NAME_FORMATSIZE];
	dns_fixedname_t trig_namef;
	dns_name_t *trig_name;
	dns_rbtnode_t *nmnode;
	dns_rpz_nm_data_t *nm_data, del_data;
	isc_result_t result;
	bool exists;

	/*
	 * We need a summary database of names even with 1 policy zone,
	 * because wildcard triggers are handled differently.
	 */

	trig_name = dns_fixedname_initname(&trig_namef);
	name2data(rpzs, rpz_num, rpz_type, src_name, trig_name, &del_data);

	nmnode = NULL;
	result = dns_rbt_findnode(rpzs->rbt, trig_name, NULL, &nmnode, NULL, 0,
				  NULL, NULL);
	if (result != ISC_R_SUCCESS) {
		/*
		 * Do not worry about missing summary RBT nodes that probably
		 * correspond to RBTDB nodes that were implicit RBT nodes
		 * that were later added for (often empty) wildcards
		 * and then to the RBTDB deferred cleanup list.
		 */
		if (result == ISC_R_NOTFOUND ||
		    result == DNS_R_PARTIALMATCH)
			return;
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
	    nm_data->wild.qname == 0 && nm_data->wild.ns == 0) {
		result = dns_rbt_deletenode(rpzs->rbt, nmnode, false);
		if (result != ISC_R_SUCCESS) {
			/*
			 * bin/tests/system/rpz/tests.sh looks for
			 * "rpz.*failed".
			 */
			dns_name_format(src_name, namebuf, sizeof(namebuf));
			isc_log_write(dns_lctx, DNS_LOGCATEGORY_RPZ,
				      DNS_LOGMODULE_RBTDB, DNS_RPZ_ERROR_LEVEL,
				      "rpz del_name(%s) node delete failed: %s",
				      namebuf, isc_result_totext(result));
		}
	}

	if (exists)
		adj_trigger_cnt(rpzs, rpz_num, rpz_type, NULL, 0, false);
}

/*
 * Remove an IP address from the radix tree or a name from the summary database.
 */
void
dns_rpz_delete(dns_rpz_zones_t *rpzs, dns_rpz_num_t rpz_num,
	       const dns_name_t *src_name)
{
	dns_rpz_zone_t *rpz;
	dns_rpz_type_t rpz_type;

	REQUIRE(rpzs != NULL && rpz_num < rpzs->p.num_zones);
	rpz = rpzs->zones[rpz_num];
	REQUIRE(rpz != NULL);

	RWLOCK(&rpzs->search_lock, isc_rwlocktype_write);

	rpz_type = type_from_name(rpzs, rpz, src_name);

	switch (rpz_type) {
	case DNS_RPZ_TYPE_QNAME:
	case DNS_RPZ_TYPE_NSDNAME:
		del_name(rpzs, rpz_num, rpz_type, src_name);
		break;
	case DNS_RPZ_TYPE_CLIENT_IP:
	case DNS_RPZ_TYPE_IP:
	case DNS_RPZ_TYPE_NSIP:
		del_cidr(rpzs, rpz_num, rpz_type, src_name);
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
		dns_name_t *ip_name, dns_rpz_prefix_t *prefixp)
{
	dns_rpz_cidr_key_t tgt_ip;
	dns_rpz_addr_zbits_t tgt_set;
	dns_rpz_cidr_node_t *found;
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
			INSIST(0);
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
			INSIST(0);
			break;
		}
	} else {
		return (DNS_RPZ_INVALID_NUM);
	}

	if (zbits == 0)
		return (DNS_RPZ_INVALID_NUM);
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
		INSIST(0);
		ISC_UNREACHABLE();
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
		  dns_rpz_zbits_t zbits, dns_name_t *trig_name)
{
	char namebuf[DNS_NAME_FORMATSIZE];
	dns_rbtnode_t *nmnode;
	const dns_rpz_nm_data_t *nm_data;
	dns_rpz_zbits_t found_zbits;
	dns_rbtnodechain_t chain;
	isc_result_t result;
	int i;

	if (zbits == 0) {
		return (0);
	}

	found_zbits = 0;

	dns_rbtnodechain_init(&chain, NULL);

	RWLOCK(&rpzs->search_lock, isc_rwlocktype_read);

	nmnode = NULL;
	result = dns_rbt_findnode(rpzs->rbt, trig_name, NULL, &nmnode,
				  &chain, DNS_RBTFIND_EMPTYDATA, NULL, NULL);
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
		/* FALLTHROUGH */

	case DNS_R_PARTIALMATCH:
		i = chain.level_matches;
		while (i >= 0 && (nmnode = chain.levels[i]) != NULL) {
			nm_data = nmnode->data;
			if (nm_data != NULL) {
				if (rpz_type == DNS_RPZ_TYPE_QNAME) {
					found_zbits |= nm_data->wild.qname;
				} else {
					found_zbits |= nm_data->wild.ns;
				}
			}
			i--;
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
			      "dns_rpz_find_name(%s) failed: %s",
			      namebuf, isc_result_totext(result));
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
		     dns_name_t *selfname)
{
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
	if (dns_name_equal(&cname.cname, dns_rootname))
		return (DNS_RPZ_POLICY_NXDOMAIN);

	if (dns_name_iswildcard(&cname.cname)) {
		/*
		 * CNAME *. means NODATA
		 */
		if (dns_name_countlabels(&cname.cname) == 2)
			return (DNS_RPZ_POLICY_NODATA);

		/*
		 * A qname of www.evil.com and a policy of
		 *	*.evil.com    CNAME   *.garden.net
		 * gives a result of
		 *	evil.com    CNAME   evil.com.garden.net
		 */
		if (dns_name_countlabels(&cname.cname) > 2)
			return (DNS_RPZ_POLICY_WILDCNAME);
	}

	/*
	 * CNAME rpz-tcp-only. means "send truncated UDP responses."
	 */
	if (dns_name_equal(&cname.cname, &rpz->tcp_only))
		return (DNS_RPZ_POLICY_TCP_ONLY);

	/*
	 * CNAME rpz-drop. means "do not respond."
	 */
	if (dns_name_equal(&cname.cname, &rpz->drop))
		return (DNS_RPZ_POLICY_DROP);

	/*
	 * CNAME rpz-passthru. means "do not rewrite."
	 */
	if (dns_name_equal(&cname.cname, &rpz->passthru))
		return (DNS_RPZ_POLICY_PASSTHRU);

	/*
	 * 128.1.0.127.rpz-ip CNAME  128.1.0.0.127. is obsolete PASSTHRU
	 */
	if (selfname != NULL && dns_name_equal(&cname.cname, selfname))
		return (DNS_RPZ_POLICY_PASSTHRU);

	/*
	 * Any other rdata gives a response consisting of the rdata.
	 */
	return (DNS_RPZ_POLICY_RECORD);
}
