/*	$NetBSD: stats.c,v 1.8 2023/01/25 21:43:30 christos Exp $	*/

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

#include <isc/magic.h>
#include <isc/mem.h>
#include <isc/refcount.h>
#include <isc/stats.h>
#include <isc/util.h>

#include <dns/log.h>
#include <dns/opcode.h>
#include <dns/rdatatype.h>
#include <dns/stats.h>

#define DNS_STATS_MAGIC	   ISC_MAGIC('D', 's', 't', 't')
#define DNS_STATS_VALID(x) ISC_MAGIC_VALID(x, DNS_STATS_MAGIC)

/*%
 * Statistics types.
 */
typedef enum {
	dns_statstype_general = 0,
	dns_statstype_rdtype = 1,
	dns_statstype_rdataset = 2,
	dns_statstype_opcode = 3,
	dns_statstype_rcode = 4,
	dns_statstype_dnssec = 5
} dns_statstype_t;

/*%
 * It doesn't make sense to have 2^16 counters for all possible types since
 * most of them won't be used.  We have counters for the first 256 types.
 *
 * A rdtypecounter is now 8 bits for RRtypes and 3 bits for flags:
 *
 *       0  1  2  3  4  5  6  7  8  9 10 11 12 13 14 15
 *     +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
 *     |  |  |  |  |  |  S  |NX|         RRType        |
 *     +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
 *
 * If the 8 bits for RRtype are all zero, this is an Other RRtype.
 */
#define RDTYPECOUNTER_MAXTYPE 0x00ff

/*
 *
 * Bit 7 is the NXRRSET (NX) flag and indicates whether this is a
 * positive (0) or a negative (1) RRset.
 */
#define RDTYPECOUNTER_NXRRSET 0x0100

/*
 * Then bit 5 and 6 mostly tell you if this counter is for an active,
 * stale, or ancient RRtype:
 *
 *     S = 0 (0b00) means Active
 *     S = 1 (0b01) means Stale
 *     S = 2 (0b10) means Ancient
 *
 * Since a counter cannot be stale and ancient at the same time, we
 * treat S = 0b11 as a special case to deal with NXDOMAIN counters.
 */
#define RDTYPECOUNTER_STALE    (1 << 9)
#define RDTYPECOUNTER_ANCIENT  (1 << 10)
#define RDTYPECOUNTER_NXDOMAIN ((1 << 9) | (1 << 10))

/*
 * S = 0b11 indicates an NXDOMAIN counter and in this case the RRtype
 * field signals the expiry of this cached item:
 *
 *     RRType = 0 (0b00) means Active
 *     RRType = 1 (0b01) means Stale
 *     RRType = 2 (0b02) means Ancient
 *
 */
#define RDTYPECOUNTER_NXDOMAIN_STALE   1
#define RDTYPECOUNTER_NXDOMAIN_ANCIENT 2

/*
 * The maximum value for rdtypecounter is for an ancient NXDOMAIN.
 */
#define RDTYPECOUNTER_MAXVAL 0x0602

/*
 * DNSSEC sign statistics.
 *
 * Per key we maintain 3 counters. The first is actually no counter but
 * a key id reference. The second is the number of signatures the key created.
 * The third is the number of signatures refreshed by the key.
 */

/* Maximum number of keys to keep track of for DNSSEC signing statistics. */
static int dnssecsign_num_keys = 4;
static int dnssecsign_block_size = 3;
/* Key id mask */
#define DNSSECSIGNSTATS_KEY_ID_MASK 0x0000FFFF

struct dns_stats {
	unsigned int magic;
	dns_statstype_t type;
	isc_mem_t *mctx;
	isc_stats_t *counters;
	isc_refcount_t references;
};

typedef struct rdatadumparg {
	dns_rdatatypestats_dumper_t fn;
	void *arg;
} rdatadumparg_t;

typedef struct opcodedumparg {
	dns_opcodestats_dumper_t fn;
	void *arg;
} opcodedumparg_t;

typedef struct rcodedumparg {
	dns_rcodestats_dumper_t fn;
	void *arg;
} rcodedumparg_t;
typedef struct dnssecsigndumparg {
	dns_dnssecsignstats_dumper_t fn;
	void *arg;
} dnssecsigndumparg_t;

void
dns_stats_attach(dns_stats_t *stats, dns_stats_t **statsp) {
	REQUIRE(DNS_STATS_VALID(stats));
	REQUIRE(statsp != NULL && *statsp == NULL);

	isc_refcount_increment(&stats->references);

	*statsp = stats;
}

void
dns_stats_detach(dns_stats_t **statsp) {
	dns_stats_t *stats;

	REQUIRE(statsp != NULL && DNS_STATS_VALID(*statsp));

	stats = *statsp;
	*statsp = NULL;

	if (isc_refcount_decrement(&stats->references) == 1) {
		isc_refcount_destroy(&stats->references);
		isc_stats_detach(&stats->counters);
		isc_mem_putanddetach(&stats->mctx, stats, sizeof(*stats));
	}
}

/*%
 * Create methods
 */
static isc_result_t
create_stats(isc_mem_t *mctx, dns_statstype_t type, int ncounters,
	     dns_stats_t **statsp) {
	dns_stats_t *stats;
	isc_result_t result;

	stats = isc_mem_get(mctx, sizeof(*stats));

	stats->counters = NULL;
	isc_refcount_init(&stats->references, 1);

	result = isc_stats_create(mctx, &stats->counters, ncounters);
	if (result != ISC_R_SUCCESS) {
		goto clean_mutex;
	}

	stats->magic = DNS_STATS_MAGIC;
	stats->type = type;
	stats->mctx = NULL;
	isc_mem_attach(mctx, &stats->mctx);
	*statsp = stats;

	return (ISC_R_SUCCESS);

clean_mutex:
	isc_mem_put(mctx, stats, sizeof(*stats));

	return (result);
}

isc_result_t
dns_generalstats_create(isc_mem_t *mctx, dns_stats_t **statsp, int ncounters) {
	REQUIRE(statsp != NULL && *statsp == NULL);

	return (create_stats(mctx, dns_statstype_general, ncounters, statsp));
}

isc_result_t
dns_rdatatypestats_create(isc_mem_t *mctx, dns_stats_t **statsp) {
	REQUIRE(statsp != NULL && *statsp == NULL);

	/*
	 * Create rdtype statistics for the first 255 RRtypes,
	 * plus one additional for other RRtypes.
	 */
	return (create_stats(mctx, dns_statstype_rdtype,
			     (RDTYPECOUNTER_MAXTYPE + 1), statsp));
}

isc_result_t
dns_rdatasetstats_create(isc_mem_t *mctx, dns_stats_t **statsp) {
	REQUIRE(statsp != NULL && *statsp == NULL);

	return (create_stats(mctx, dns_statstype_rdataset,
			     (RDTYPECOUNTER_MAXVAL + 1), statsp));
}

isc_result_t
dns_opcodestats_create(isc_mem_t *mctx, dns_stats_t **statsp) {
	REQUIRE(statsp != NULL && *statsp == NULL);

	return (create_stats(mctx, dns_statstype_opcode, 16, statsp));
}

isc_result_t
dns_rcodestats_create(isc_mem_t *mctx, dns_stats_t **statsp) {
	REQUIRE(statsp != NULL && *statsp == NULL);

	return (create_stats(mctx, dns_statstype_rcode, dns_rcode_badcookie + 1,
			     statsp));
}

isc_result_t
dns_dnssecsignstats_create(isc_mem_t *mctx, dns_stats_t **statsp) {
	REQUIRE(statsp != NULL && *statsp == NULL);

	/*
	 * Create two counters per key, one is the key id, the other two are
	 * the actual counters for creating and refreshing signatures.
	 */
	return (create_stats(mctx, dns_statstype_dnssec,
			     dnssecsign_num_keys * dnssecsign_block_size,
			     statsp));
}

/*%
 * Increment/Decrement methods
 */
void
dns_generalstats_increment(dns_stats_t *stats, isc_statscounter_t counter) {
	REQUIRE(DNS_STATS_VALID(stats) && stats->type == dns_statstype_general);

	isc_stats_increment(stats->counters, counter);
}

static isc_statscounter_t
rdatatype2counter(dns_rdatatype_t type) {
	if (type > (dns_rdatatype_t)RDTYPECOUNTER_MAXTYPE) {
		return (0);
	}
	return ((isc_statscounter_t)type);
}

void
dns_rdatatypestats_increment(dns_stats_t *stats, dns_rdatatype_t type) {
	isc_statscounter_t counter;

	REQUIRE(DNS_STATS_VALID(stats) && stats->type == dns_statstype_rdtype);

	counter = rdatatype2counter(type);
	isc_stats_increment(stats->counters, counter);
}

static void
update_rdatasetstats(dns_stats_t *stats, dns_rdatastatstype_t rrsettype,
		     bool increment) {
	isc_statscounter_t counter;

	if ((DNS_RDATASTATSTYPE_ATTR(rrsettype) &
	     DNS_RDATASTATSTYPE_ATTR_NXDOMAIN) != 0)
	{
		counter = RDTYPECOUNTER_NXDOMAIN;

		/*
		 * This is an NXDOMAIN counter, save the expiry value
		 * (active, stale, or ancient) value in the RRtype part.
		 */
		if ((DNS_RDATASTATSTYPE_ATTR(rrsettype) &
		     DNS_RDATASTATSTYPE_ATTR_ANCIENT) != 0)
		{
			counter |= RDTYPECOUNTER_NXDOMAIN_ANCIENT;
		} else if ((DNS_RDATASTATSTYPE_ATTR(rrsettype) &
			    DNS_RDATASTATSTYPE_ATTR_STALE) != 0)
		{
			counter += RDTYPECOUNTER_NXDOMAIN_STALE;
		}
	} else {
		counter = rdatatype2counter(DNS_RDATASTATSTYPE_BASE(rrsettype));

		if ((DNS_RDATASTATSTYPE_ATTR(rrsettype) &
		     DNS_RDATASTATSTYPE_ATTR_NXRRSET) != 0)
		{
			counter |= RDTYPECOUNTER_NXRRSET;
		}

		if ((DNS_RDATASTATSTYPE_ATTR(rrsettype) &
		     DNS_RDATASTATSTYPE_ATTR_ANCIENT) != 0)
		{
			counter |= RDTYPECOUNTER_ANCIENT;
		} else if ((DNS_RDATASTATSTYPE_ATTR(rrsettype) &
			    DNS_RDATASTATSTYPE_ATTR_STALE) != 0)
		{
			counter |= RDTYPECOUNTER_STALE;
		}
	}

	if (increment) {
		isc_stats_increment(stats->counters, counter);
	} else {
		isc_stats_decrement(stats->counters, counter);
	}
}

void
dns_rdatasetstats_increment(dns_stats_t *stats,
			    dns_rdatastatstype_t rrsettype) {
	REQUIRE(DNS_STATS_VALID(stats) &&
		stats->type == dns_statstype_rdataset);

	update_rdatasetstats(stats, rrsettype, true);
}

void
dns_rdatasetstats_decrement(dns_stats_t *stats,
			    dns_rdatastatstype_t rrsettype) {
	REQUIRE(DNS_STATS_VALID(stats) &&
		stats->type == dns_statstype_rdataset);

	update_rdatasetstats(stats, rrsettype, false);
}

void
dns_opcodestats_increment(dns_stats_t *stats, dns_opcode_t code) {
	REQUIRE(DNS_STATS_VALID(stats) && stats->type == dns_statstype_opcode);

	isc_stats_increment(stats->counters, (isc_statscounter_t)code);
}

void
dns_rcodestats_increment(dns_stats_t *stats, dns_rcode_t code) {
	REQUIRE(DNS_STATS_VALID(stats) && stats->type == dns_statstype_rcode);

	if (code <= dns_rcode_badcookie) {
		isc_stats_increment(stats->counters, (isc_statscounter_t)code);
	}
}

void
dns_dnssecsignstats_increment(dns_stats_t *stats, dns_keytag_t id, uint8_t alg,
			      dnssecsignstats_type_t operation) {
	uint32_t kval;
	int num_keys = isc_stats_ncounters(stats->counters) /
		       dnssecsign_block_size;

	REQUIRE(DNS_STATS_VALID(stats) && stats->type == dns_statstype_dnssec);

	/* Shift algorithm in front of key tag, which is 16 bits */
	kval = (uint32_t)(alg << 16 | id);

	/* Look up correct counter. */
	for (int i = 0; i < num_keys; i++) {
		int idx = i * dnssecsign_block_size;
		uint32_t counter = isc_stats_get_counter(stats->counters, idx);
		if (counter == kval) {
			/* Match */
			isc_stats_increment(stats->counters, (idx + operation));
			return;
		}
	}

	/* No match found. Store key in unused slot. */
	for (int i = 0; i < num_keys; i++) {
		int idx = i * dnssecsign_block_size;
		uint32_t counter = isc_stats_get_counter(stats->counters, idx);
		if (counter == 0) {
			isc_stats_set(stats->counters, kval, idx);
			isc_stats_increment(stats->counters, (idx + operation));
			return;
		}
	}

	/* No room, grow stats storage. */
	isc_stats_resize(&stats->counters,
			 (num_keys * dnssecsign_block_size * 2));

	/* Reset counters for new key (new index, nidx). */
	int nidx = num_keys * dnssecsign_block_size;
	isc_stats_set(stats->counters, kval, nidx);
	isc_stats_set(stats->counters, 0, (nidx + dns_dnssecsignstats_sign));
	isc_stats_set(stats->counters, 0, (nidx + dns_dnssecsignstats_refresh));

	/* And increment the counter for the given operation. */
	isc_stats_increment(stats->counters, (nidx + operation));
}

void
dns_dnssecsignstats_clear(dns_stats_t *stats, dns_keytag_t id, uint8_t alg) {
	uint32_t kval;
	int num_keys = isc_stats_ncounters(stats->counters) /
		       dnssecsign_block_size;

	REQUIRE(DNS_STATS_VALID(stats) && stats->type == dns_statstype_dnssec);

	/* Shift algorithm in front of key tag, which is 16 bits */
	kval = (uint32_t)(alg << 16 | id);

	/* Look up correct counter. */
	for (int i = 0; i < num_keys; i++) {
		int idx = i * dnssecsign_block_size;
		uint32_t counter = isc_stats_get_counter(stats->counters, idx);
		if (counter == kval) {
			/* Match */
			isc_stats_set(stats->counters, 0, idx);
			isc_stats_set(stats->counters, 0,
				      (idx + dns_dnssecsignstats_sign));
			isc_stats_set(stats->counters, 0,
				      (idx + dns_dnssecsignstats_refresh));
			return;
		}
	}
}

/*%
 * Dump methods
 */
void
dns_generalstats_dump(dns_stats_t *stats, dns_generalstats_dumper_t dump_fn,
		      void *arg, unsigned int options) {
	REQUIRE(DNS_STATS_VALID(stats) && stats->type == dns_statstype_general);

	isc_stats_dump(stats->counters, (isc_stats_dumper_t)dump_fn, arg,
		       options);
}

static void
dump_rdentry(int rdcounter, uint64_t value, dns_rdatastatstype_t attributes,
	     dns_rdatatypestats_dumper_t dump_fn, void *arg) {
	dns_rdatatype_t rdtype = dns_rdatatype_none; /* sentinel */
	dns_rdatastatstype_t type;

	if ((rdcounter & RDTYPECOUNTER_MAXTYPE) == 0) {
		attributes |= DNS_RDATASTATSTYPE_ATTR_OTHERTYPE;
	} else {
		rdtype = (dns_rdatatype_t)(rdcounter & RDTYPECOUNTER_MAXTYPE);
	}
	type = DNS_RDATASTATSTYPE_VALUE((dns_rdatastatstype_t)rdtype,
					attributes);
	dump_fn(type, value, arg);
}

static void
rdatatype_dumpcb(isc_statscounter_t counter, uint64_t value, void *arg) {
	rdatadumparg_t *rdatadumparg = arg;

	dump_rdentry(counter, value, 0, rdatadumparg->fn, rdatadumparg->arg);
}

void
dns_rdatatypestats_dump(dns_stats_t *stats, dns_rdatatypestats_dumper_t dump_fn,
			void *arg0, unsigned int options) {
	rdatadumparg_t arg;
	REQUIRE(DNS_STATS_VALID(stats) && stats->type == dns_statstype_rdtype);

	arg.fn = dump_fn;
	arg.arg = arg0;
	isc_stats_dump(stats->counters, rdatatype_dumpcb, &arg, options);
}

static void
rdataset_dumpcb(isc_statscounter_t counter, uint64_t value, void *arg) {
	rdatadumparg_t *rdatadumparg = arg;
	unsigned int attributes = 0;

	if ((counter & RDTYPECOUNTER_NXDOMAIN) == RDTYPECOUNTER_NXDOMAIN) {
		attributes |= DNS_RDATASTATSTYPE_ATTR_NXDOMAIN;

		/*
		 * This is an NXDOMAIN counter, check the RRtype part for the
		 * expiry value (active, stale, or ancient).
		 */
		if ((counter & RDTYPECOUNTER_MAXTYPE) ==
		    RDTYPECOUNTER_NXDOMAIN_STALE)
		{
			attributes |= DNS_RDATASTATSTYPE_ATTR_STALE;
		} else if ((counter & RDTYPECOUNTER_MAXTYPE) ==
			   RDTYPECOUNTER_NXDOMAIN_ANCIENT)
		{
			attributes |= DNS_RDATASTATSTYPE_ATTR_ANCIENT;
		}
	} else {
		if ((counter & RDTYPECOUNTER_MAXTYPE) == 0) {
			attributes |= DNS_RDATASTATSTYPE_ATTR_OTHERTYPE;
		}
		if ((counter & RDTYPECOUNTER_NXRRSET) != 0) {
			attributes |= DNS_RDATASTATSTYPE_ATTR_NXRRSET;
		}

		if ((counter & RDTYPECOUNTER_STALE) != 0) {
			attributes |= DNS_RDATASTATSTYPE_ATTR_STALE;
		} else if ((counter & RDTYPECOUNTER_ANCIENT) != 0) {
			attributes |= DNS_RDATASTATSTYPE_ATTR_ANCIENT;
		}
	}

	dump_rdentry(counter, value, attributes, rdatadumparg->fn,
		     rdatadumparg->arg);
}

void
dns_rdatasetstats_dump(dns_stats_t *stats, dns_rdatatypestats_dumper_t dump_fn,
		       void *arg0, unsigned int options) {
	rdatadumparg_t arg;

	REQUIRE(DNS_STATS_VALID(stats) &&
		stats->type == dns_statstype_rdataset);

	arg.fn = dump_fn;
	arg.arg = arg0;
	isc_stats_dump(stats->counters, rdataset_dumpcb, &arg, options);
}

static void
dnssec_dumpcb(isc_statscounter_t counter, uint64_t value, void *arg) {
	dnssecsigndumparg_t *dnssecarg = arg;

	dnssecarg->fn((dns_keytag_t)counter, value, dnssecarg->arg);
}

static void
dnssec_statsdump(isc_stats_t *stats, dnssecsignstats_type_t operation,
		 isc_stats_dumper_t dump_fn, void *arg, unsigned int options) {
	int i, num_keys;

	num_keys = isc_stats_ncounters(stats) / dnssecsign_block_size;
	for (i = 0; i < num_keys; i++) {
		int idx = dnssecsign_block_size * i;
		uint32_t kval, val;
		dns_keytag_t id;

		kval = isc_stats_get_counter(stats, idx);
		if (kval == 0) {
			continue;
		}

		val = isc_stats_get_counter(stats, (idx + operation));
		if ((options & ISC_STATSDUMP_VERBOSE) == 0 && val == 0) {
			continue;
		}

		id = (dns_keytag_t)kval & DNSSECSIGNSTATS_KEY_ID_MASK;

		dump_fn((isc_statscounter_t)id, val, arg);
	}
}

void
dns_dnssecsignstats_dump(dns_stats_t *stats, dnssecsignstats_type_t operation,
			 dns_dnssecsignstats_dumper_t dump_fn, void *arg0,
			 unsigned int options) {
	dnssecsigndumparg_t arg;

	REQUIRE(DNS_STATS_VALID(stats) && stats->type == dns_statstype_dnssec);

	arg.fn = dump_fn;
	arg.arg = arg0;

	dnssec_statsdump(stats->counters, operation, dnssec_dumpcb, &arg,
			 options);
}

static void
opcode_dumpcb(isc_statscounter_t counter, uint64_t value, void *arg) {
	opcodedumparg_t *opcodearg = arg;

	opcodearg->fn((dns_opcode_t)counter, value, opcodearg->arg);
}

static void
rcode_dumpcb(isc_statscounter_t counter, uint64_t value, void *arg) {
	rcodedumparg_t *rcodearg = arg;

	rcodearg->fn((dns_rcode_t)counter, value, rcodearg->arg);
}

void
dns_opcodestats_dump(dns_stats_t *stats, dns_opcodestats_dumper_t dump_fn,
		     void *arg0, unsigned int options) {
	opcodedumparg_t arg;

	REQUIRE(DNS_STATS_VALID(stats) && stats->type == dns_statstype_opcode);

	arg.fn = dump_fn;
	arg.arg = arg0;
	isc_stats_dump(stats->counters, opcode_dumpcb, &arg, options);
}

void
dns_rcodestats_dump(dns_stats_t *stats, dns_rcodestats_dumper_t dump_fn,
		    void *arg0, unsigned int options) {
	rcodedumparg_t arg;

	REQUIRE(DNS_STATS_VALID(stats) && stats->type == dns_statstype_rcode);

	arg.fn = dump_fn;
	arg.arg = arg0;
	isc_stats_dump(stats->counters, rcode_dumpcb, &arg, options);
}

/***
 *** Obsolete variables and functions follow:
 ***/
LIBDNS_EXTERNAL_DATA const char *dns_statscounter_names[DNS_STATS_NCOUNTERS] = {
	"success",   "referral", "nxrrset",   "nxdomain",
	"recursion", "failure",	 "duplicate", "dropped"
};

isc_result_t
dns_stats_alloccounters(isc_mem_t *mctx, uint64_t **ctrp) {
	int i;
	uint64_t *p = isc_mem_get(mctx, DNS_STATS_NCOUNTERS * sizeof(uint64_t));
	if (p == NULL) {
		return (ISC_R_NOMEMORY);
	}
	for (i = 0; i < DNS_STATS_NCOUNTERS; i++) {
		p[i] = 0;
	}
	*ctrp = p;
	return (ISC_R_SUCCESS);
}

void
dns_stats_freecounters(isc_mem_t *mctx, uint64_t **ctrp) {
	isc_mem_put(mctx, *ctrp, DNS_STATS_NCOUNTERS * sizeof(uint64_t));
	*ctrp = NULL;
}
