/*	$NetBSD: stats.c,v 1.1.1.11 2026/09/17 17:45:07 christos Exp $	*/

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

#include <isc/atomic.h>
#include <isc/magic.h>
#include <isc/mem.h>
#include <isc/refcount.h>
#include <isc/stats.h>
#include <isc/urcu.h>
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
 * DNSSEC signing counters are a small RCU-protected list keyed by the DNSKEY
 * algorithm and key tag. The key is immutable after publication; the counters
 * remain atomic for concurrent dumps. List mutations are serialized by the
 * caller.
 */
typedef struct dns_dnssecsignstat {
	isc_mem_t *mctx;
	uint32_t key;
	isc_atomic_statscounter_t signatures;
	isc_atomic_statscounter_t refreshes;
	struct cds_list_head link;
	struct rcu_head rcu_head;
} dns_dnssecsignstat_t;

typedef struct dns_dnssecsignstats {
	struct cds_list_head keys;
} dns_dnssecsignstats_t;

struct dns_stats {
	unsigned int magic;
	dns_statstype_t type;
	isc_mem_t *mctx;
	union {
		isc_stats_t *counters;
		dns_dnssecsignstats_t *dnssec;
	};
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

static void
dns_dnssecsignstat_destroy(struct rcu_head *rcu_head) {
	dns_dnssecsignstat_t *entry =
		caa_container_of(rcu_head, dns_dnssecsignstat_t, rcu_head);

	isc_mem_putanddetach(&entry->mctx, entry, sizeof(*entry));
}

static void
dns_dnssecsignstats_destroy(dns_stats_t *stats) {
	dns_dnssecsignstats_t *dnssec = stats->dnssec;

	dns_dnssecsignstat_t *entry, *next;
	cds_list_for_each_entry_safe(entry, next, &dnssec->keys, link) {
		cds_list_del_rcu(&entry->link);
		call_rcu(&entry->rcu_head, dns_dnssecsignstat_destroy);
	}

	isc_mem_put(stats->mctx, dnssec, sizeof(*dnssec));
}

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
		if (stats->type == dns_statstype_dnssec) {
			dns_dnssecsignstats_destroy(stats);
		} else {
			isc_stats_detach(&stats->counters);
		}
		isc_mem_putanddetach(&stats->mctx, stats, sizeof(*stats));
	}
}

/*%
 * Create methods
 */
static dns_stats_t *
allocate_stats(isc_mem_t *mctx, dns_statstype_t type) {
	dns_stats_t *stats = isc_mem_get(mctx, sizeof(*stats));

	*stats = (dns_stats_t){
		.magic = DNS_STATS_MAGIC,
		.type = type,
		.references = ISC_REFCOUNT_INITIALIZER(1),
	};
	isc_mem_attach(mctx, &stats->mctx);

	return stats;
}

static void
create_stats(isc_mem_t *mctx, dns_statstype_t type, int ncounters,
	     dns_stats_t **statsp) {
	dns_stats_t *stats = allocate_stats(mctx, type);

	isc_stats_create(mctx, &stats->counters, ncounters);
	*statsp = stats;
}

void
dns_generalstats_create(isc_mem_t *mctx, dns_stats_t **statsp, int ncounters) {
	REQUIRE(statsp != NULL && *statsp == NULL);

	create_stats(mctx, dns_statstype_general, ncounters, statsp);
}

void
dns_rdatatypestats_create(isc_mem_t *mctx, dns_stats_t **statsp) {
	REQUIRE(statsp != NULL && *statsp == NULL);

	/*
	 * Create rdtype statistics for the first 255 RRtypes,
	 * plus one additional for other RRtypes.
	 */
	create_stats(mctx, dns_statstype_rdtype, RDTYPECOUNTER_MAXTYPE + 1,
		     statsp);
}

void
dns_rdatasetstats_create(isc_mem_t *mctx, dns_stats_t **statsp) {
	REQUIRE(statsp != NULL && *statsp == NULL);

	create_stats(mctx, dns_statstype_rdataset, RDTYPECOUNTER_MAXVAL + 1,
		     statsp);
}

void
dns_opcodestats_create(isc_mem_t *mctx, dns_stats_t **statsp) {
	REQUIRE(statsp != NULL && *statsp == NULL);

	create_stats(mctx, dns_statstype_opcode, 16, statsp);
}

void
dns_rcodestats_create(isc_mem_t *mctx, dns_stats_t **statsp) {
	REQUIRE(statsp != NULL && *statsp == NULL);

	create_stats(mctx, dns_statstype_rcode, dns_rcode_badcookie + 1,
		     statsp);
}

void
dns_dnssecsignstats_create(isc_mem_t *mctx, dns_stats_t **statsp) {
	REQUIRE(statsp != NULL && *statsp == NULL);

	dns_stats_t *stats = allocate_stats(mctx, dns_statstype_dnssec);

	dns_dnssecsignstats_t *dnssec = isc_mem_get(mctx, sizeof(*dnssec));
	*dnssec = (dns_dnssecsignstats_t){
		.keys = CDS_LIST_HEAD_INIT(dnssec->keys),
	};

	stats->dnssec = dnssec;

	*statsp = stats;
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
		return 0;
	}
	return (isc_statscounter_t)type;
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

static void
dnssecsignstat_new(dns_stats_t *stats, uint32_t key,
		   dnssecsignstats_type_t operation) {
	dns_dnssecsignstats_t *dnssec = stats->dnssec;

	dns_dnssecsignstat_t *entry = isc_mem_get(stats->mctx, sizeof(*entry));
	*entry = (dns_dnssecsignstat_t){
		.key = key,
		.link = CDS_LIST_HEAD_INIT(entry->link),
	};
	isc_mem_attach(stats->mctx, &entry->mctx);

	switch (operation) {
	case dns_dnssecsignstats_sign:
		atomic_init(&entry->signatures, 1);
		break;
	case dns_dnssecsignstats_refresh:
		atomic_init(&entry->refreshes, 1);
		break;
	default:
		UNREACHABLE();
	}

	cds_list_add_rcu(&entry->link, &dnssec->keys);
}

static isc_result_t
dnssecsignstat_increment(dns_stats_t *stats, uint32_t key,
			 dnssecsignstats_type_t operation) {
	dns_dnssecsignstats_t *dnssec = stats->dnssec;
	dns_dnssecsignstat_t *entry;
	cds_list_for_each_entry_rcu(entry, &dnssec->keys, link) {
		if (entry->key != key) {
			continue;
		}

		switch (operation) {
		case dns_dnssecsignstats_sign:
			atomic_fetch_add_relaxed(&entry->signatures, 1);
			break;
		case dns_dnssecsignstats_refresh:
			atomic_fetch_add_relaxed(&entry->refreshes, 1);
			break;
		default:
			UNREACHABLE();
		}
		return ISC_R_SUCCESS;
	}

	return ISC_R_NOTFOUND;
}

void
dns_dnssecsignstats_increment(dns_stats_t *stats, dns_keytag_t id, uint8_t alg,
			      dnssecsignstats_type_t operation) {
	REQUIRE(DNS_STATS_VALID(stats) && stats->type == dns_statstype_dnssec);

	isc_result_t result;
	uint32_t key = (uint32_t)alg << 16 | id;

	rcu_read_lock();
	result = dnssecsignstat_increment(stats, key, operation);
	rcu_read_unlock();
	if (result == ISC_R_SUCCESS) {
		return;
	}

	dnssecsignstat_new(stats, key, operation);
}

void
dns_dnssecsignstats_clear(dns_stats_t *stats, dns_keytag_t id, uint8_t alg) {
	REQUIRE(DNS_STATS_VALID(stats) && stats->type == dns_statstype_dnssec);

	dns_dnssecsignstats_t *dnssec = stats->dnssec;
	uint32_t key = (uint32_t)alg << 16 | id;

	dns_dnssecsignstat_t *entry;
	cds_list_for_each_entry(entry, &dnssec->keys, link) {
		if (entry->key != key) {
			continue;
		}

		cds_list_del_rcu(&entry->link);
		call_rcu(&entry->rcu_head, dns_dnssecsignstat_destroy);
		return;
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

void
dns_dnssecsignstats_dump(dns_stats_t *stats, dnssecsignstats_type_t operation,
			 dns_dnssecsignstats_dumper_t dump_fn, void *arg,
			 unsigned int options) {
	REQUIRE(DNS_STATS_VALID(stats) && stats->type == dns_statstype_dnssec);

	dns_dnssecsignstats_t *dnssec = stats->dnssec;

	rcu_read_lock();
	dns_dnssecsignstat_t *entry;
	cds_list_for_each_entry_rcu(entry, &dnssec->keys, link) {
		isc_statscounter_t value;

		switch (operation) {
		case dns_dnssecsignstats_sign:
			value = atomic_load_acquire(&entry->signatures);
			break;
		case dns_dnssecsignstats_refresh:
			value = atomic_load_acquire(&entry->refreshes);
			break;
		default:
			UNREACHABLE();
		}

		if ((options & ISC_STATSDUMP_VERBOSE) == 0 && value == 0) {
			continue;
		}
		dump_fn(entry->key, value, arg);
	}
	rcu_read_unlock();
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
