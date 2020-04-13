/*	$NetBSD: stats.c,v 1.3.2.3 2020/04/13 08:02:57 martin Exp $	*/

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

#include <isc/magic.h>
#include <isc/mem.h>
#include <isc/stats.h>
#include <isc/util.h>

#include <dns/opcode.h>
#include <dns/rdatatype.h>
#include <dns/stats.h>

#define DNS_STATS_MAGIC			ISC_MAGIC('D', 's', 't', 't')
#define DNS_STATS_VALID(x)		ISC_MAGIC_VALID(x, DNS_STATS_MAGIC)

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
 * most of them won't be used.  We have counters for the first 256 types and
 * those explicitly supported in the rdata implementation.
 * XXXJT: this introduces tight coupling with the rdata implementation.
 * Ideally, we should have rdata handle this type of details.
 */
/*
 * types, !types, nxdomain, stale types, stale !types, stale nxdomain,
 * ancient types, ancient !types, ancient nxdomain
 */
enum {
	/* For 0-255, we use the rdtype value as counter indices */
	rdtypecounter_dlv = 256,	/* for dns_rdatatype_dlv */
	rdtypecounter_others = 257,	/* anything else */
	rdtypecounter_max = 258,
	/* The following are used for nxrrset rdataset */
	rdtypenxcounter_max = rdtypecounter_max * 2,
	/* nxdomain counter */
	rdtypecounter_nxdomain = rdtypenxcounter_max,
	/* stale counters offset */
	rdtypecounter_stale = rdtypecounter_nxdomain + 1,
	rdtypecounter_stale_max = rdtypecounter_stale + rdtypecounter_max,
	rdtypenxcounter_stale_max = rdtypecounter_stale_max + rdtypecounter_max,
	rdtypecounter_stale_nxdomain = rdtypenxcounter_stale_max,
	/* ancient counters offset */
	rdtypecounter_ancient = rdtypecounter_stale_nxdomain + 1,
	rdtypecounter_ancient_max = rdtypecounter_ancient + rdtypecounter_max,
	rdtypenxcounter_ancient_max = rdtypecounter_ancient_max +
				      rdtypecounter_max,
	rdtypecounter_ancient_nxdomain = rdtypenxcounter_ancient_max,
	/* limit of number counter types */
	rdatasettypecounter_max = rdtypecounter_ancient_nxdomain + 1,
};

/* dnssec maximum key id */
static int dnssec_keyid_max = 65535;

struct dns_stats {
	/*% Unlocked */
	unsigned int	magic;
	dns_statstype_t	type;
	isc_mem_t	*mctx;
	isc_mutex_t	lock;
	isc_stats_t	*counters;

	/*%  Locked by lock */
	unsigned int	references;
};

typedef struct rdatadumparg {
	dns_rdatatypestats_dumper_t	fn;
	void				*arg;
} rdatadumparg_t;

typedef struct opcodedumparg {
	dns_opcodestats_dumper_t	fn;
	void				*arg;
} opcodedumparg_t;

typedef struct rcodedumparg {
	dns_rcodestats_dumper_t		fn;
	void				*arg;
} rcodedumparg_t;
typedef struct dnssecsigndumparg {
	dns_dnssecsignstats_dumper_t	fn;
	void				*arg;
} dnssecsigndumparg_t;

void
dns_stats_attach(dns_stats_t *stats, dns_stats_t **statsp) {
	REQUIRE(DNS_STATS_VALID(stats));
	REQUIRE(statsp != NULL && *statsp == NULL);

	LOCK(&stats->lock);
	stats->references++;
	UNLOCK(&stats->lock);

	*statsp = stats;
}

void
dns_stats_detach(dns_stats_t **statsp) {
	dns_stats_t *stats;

	REQUIRE(statsp != NULL && DNS_STATS_VALID(*statsp));

	stats = *statsp;
	*statsp = NULL;

	LOCK(&stats->lock);
	stats->references--;
	UNLOCK(&stats->lock);

	if (stats->references == 0) {
		isc_stats_detach(&stats->counters);
		isc_mutex_destroy(&stats->lock);
		isc_mem_putanddetach(&stats->mctx, stats, sizeof(*stats));
	}
}

/*%
 * Create methods
 */
static isc_result_t
create_stats(isc_mem_t *mctx, dns_statstype_t type, int ncounters,
	     dns_stats_t **statsp)
{
	dns_stats_t *stats;
	isc_result_t result;

	stats = isc_mem_get(mctx, sizeof(*stats));
	if (stats == NULL)
		return (ISC_R_NOMEMORY);

	stats->counters = NULL;
	stats->references = 1;

	isc_mutex_init(&stats->lock);

	result = isc_stats_create(mctx, &stats->counters, ncounters);
	if (result != ISC_R_SUCCESS)
		goto clean_mutex;

	stats->magic = DNS_STATS_MAGIC;
	stats->type = type;
	stats->mctx = NULL;
	isc_mem_attach(mctx, &stats->mctx);
	*statsp = stats;

	return (ISC_R_SUCCESS);

  clean_mutex:
	isc_mutex_destroy(&stats->lock);
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

	return (create_stats(mctx, dns_statstype_rdtype, rdtypecounter_max,
			     statsp));
}

isc_result_t
dns_rdatasetstats_create(isc_mem_t *mctx, dns_stats_t **statsp) {
	REQUIRE(statsp != NULL && *statsp == NULL);

	return (create_stats(mctx, dns_statstype_rdataset,
			     rdatasettypecounter_max, statsp));
}

isc_result_t
dns_opcodestats_create(isc_mem_t *mctx, dns_stats_t **statsp) {
	REQUIRE(statsp != NULL && *statsp == NULL);

	return (create_stats(mctx, dns_statstype_opcode, 16, statsp));
}

isc_result_t
dns_rcodestats_create(isc_mem_t *mctx, dns_stats_t **statsp) {
	REQUIRE(statsp != NULL && *statsp == NULL);

	return (create_stats(mctx, dns_statstype_rcode,
			     dns_rcode_badcookie + 1, statsp));
}

isc_result_t
dns_dnssecsignstats_create(isc_mem_t *mctx, dns_stats_t **statsp) {
	REQUIRE(statsp != NULL && *statsp == NULL);

	return (create_stats(mctx, dns_statstype_dnssec,
			     dnssec_keyid_max, statsp));
}

/*%
 * Increment/Decrement methods
 */
void
dns_generalstats_increment(dns_stats_t *stats, isc_statscounter_t counter) {
	REQUIRE(DNS_STATS_VALID(stats) && stats->type == dns_statstype_general);

	isc_stats_increment(stats->counters, counter);
}

void
dns_rdatatypestats_increment(dns_stats_t *stats, dns_rdatatype_t type) {
	int counter;

	REQUIRE(DNS_STATS_VALID(stats) && stats->type == dns_statstype_rdtype);

	if (type == dns_rdatatype_dlv)
		counter = rdtypecounter_dlv;
	else if (type > dns_rdatatype_any)
		counter = rdtypecounter_others;
	else
		counter = (int)type;

	isc_stats_increment(stats->counters, (isc_statscounter_t)counter);
}

static inline void
update_rdatasetstats(dns_stats_t *stats, dns_rdatastatstype_t rrsettype,
		     bool increment)
{
	int counter;
	dns_rdatatype_t rdtype;

	if ((DNS_RDATASTATSTYPE_ATTR(rrsettype) &
	     DNS_RDATASTATSTYPE_ATTR_NXDOMAIN) != 0) {
		counter = rdtypecounter_nxdomain;
	} else {
		rdtype = DNS_RDATASTATSTYPE_BASE(rrsettype);
		if (rdtype == dns_rdatatype_dlv)
			counter = (int)rdtypecounter_dlv;
		else if (rdtype > dns_rdatatype_any)
			counter = (int)rdtypecounter_others;
		else
			counter = (int)rdtype;

		if ((DNS_RDATASTATSTYPE_ATTR(rrsettype) &
		     DNS_RDATASTATSTYPE_ATTR_NXRRSET) != 0)
			counter += rdtypecounter_max;
	}

	if ((DNS_RDATASTATSTYPE_ATTR(rrsettype) &
	     DNS_RDATASTATSTYPE_ATTR_ANCIENT) != 0) {
		counter += rdtypecounter_ancient;
	} else if ((DNS_RDATASTATSTYPE_ATTR(rrsettype) &
	     DNS_RDATASTATSTYPE_ATTR_STALE) != 0) {
		counter += rdtypecounter_stale;
	}

	if (increment) {
		isc_stats_increment(stats->counters, counter);
	} else {
		isc_stats_decrement(stats->counters, counter);
	}
}

void
dns_rdatasetstats_increment(dns_stats_t *stats, dns_rdatastatstype_t rrsettype)
{
	REQUIRE(DNS_STATS_VALID(stats) &&
		stats->type == dns_statstype_rdataset);

	update_rdatasetstats(stats, rrsettype, true);
}

void
dns_rdatasetstats_decrement(dns_stats_t *stats, dns_rdatastatstype_t rrsettype)
{
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

	if (code <= dns_rcode_badcookie)
		isc_stats_increment(stats->counters, (isc_statscounter_t)code);
}

void
dns_dnssecsignstats_increment(dns_stats_t *stats, dns_keytag_t id) {
	REQUIRE(DNS_STATS_VALID(stats) && stats->type == dns_statstype_dnssec);

	isc_stats_increment(stats->counters, (isc_statscounter_t)id);
}

/*%
 * Dump methods
 */
void
dns_generalstats_dump(dns_stats_t *stats, dns_generalstats_dumper_t dump_fn,
		      void *arg, unsigned int options)
{
	REQUIRE(DNS_STATS_VALID(stats) && stats->type == dns_statstype_general);

	isc_stats_dump(stats->counters, (isc_stats_dumper_t)dump_fn,
		       arg, options);
}

static void
dump_rdentry(int rdcounter, uint64_t value, dns_rdatastatstype_t attributes,
	     dns_rdatatypestats_dumper_t dump_fn, void * arg)
{
	dns_rdatatype_t rdtype = dns_rdatatype_none; /* sentinel */
	dns_rdatastatstype_t type;

	if (rdcounter == rdtypecounter_others)
		attributes |= DNS_RDATASTATSTYPE_ATTR_OTHERTYPE;
	else {
		if (rdcounter == rdtypecounter_dlv)
			rdtype = dns_rdatatype_dlv;
		else
			rdtype = (dns_rdatatype_t)rdcounter;
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
			void *arg0, unsigned int options)
{
	rdatadumparg_t arg;
	REQUIRE(DNS_STATS_VALID(stats) && stats->type == dns_statstype_rdtype);

	arg.fn = dump_fn;
	arg.arg = arg0;
	isc_stats_dump(stats->counters, rdatatype_dumpcb, &arg, options);
}

static void
rdataset_dumpcb(isc_statscounter_t counter, uint64_t value, void *arg) {
	rdatadumparg_t *rdatadumparg = arg;
	unsigned int attributes;
	bool dump = true;

	if (counter < rdtypecounter_max) {
		attributes = 0;
	} else if (counter < rdtypenxcounter_max) {
		counter -= rdtypecounter_max;
		attributes = DNS_RDATASTATSTYPE_ATTR_NXRRSET;
	} else if (counter == rdtypecounter_nxdomain) {
		counter = 0;
		attributes = DNS_RDATASTATSTYPE_ATTR_NXDOMAIN;
	} else if (counter < rdtypecounter_stale_max) {
		counter -= rdtypecounter_stale;
		attributes = DNS_RDATASTATSTYPE_ATTR_STALE;
	} else if (counter < rdtypenxcounter_stale_max) {
		counter -= rdtypecounter_stale_max;
		attributes = DNS_RDATASTATSTYPE_ATTR_NXRRSET |
			     DNS_RDATASTATSTYPE_ATTR_STALE;
	} else if (counter == rdtypecounter_stale_nxdomain) {
		counter = 0;
		attributes = DNS_RDATASTATSTYPE_ATTR_NXDOMAIN |
			     DNS_RDATASTATSTYPE_ATTR_STALE;
	} else if (counter < rdtypecounter_ancient_max) {
		counter -= rdtypecounter_ancient;
		attributes = DNS_RDATASTATSTYPE_ATTR_ANCIENT;
	} else if (counter < rdtypenxcounter_ancient_max) {
		counter -= rdtypecounter_ancient_max;
		attributes = DNS_RDATASTATSTYPE_ATTR_NXRRSET |
			     DNS_RDATASTATSTYPE_ATTR_ANCIENT;
	} else if (counter == rdtypecounter_ancient_nxdomain) {
		counter = 0;
		attributes = DNS_RDATASTATSTYPE_ATTR_NXDOMAIN |
			     DNS_RDATASTATSTYPE_ATTR_ANCIENT;
	} else {
		/* Out of bounds, do not dump entry. */
		dump = false;
	}

	if (dump) {
		dump_rdentry(counter, value, attributes, rdatadumparg->fn,
		     rdatadumparg->arg);
	}
}

void
dns_rdatasetstats_dump(dns_stats_t *stats, dns_rdatatypestats_dumper_t dump_fn,
		       void *arg0, unsigned int options)
{
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

void
dns_dnssecsignstats_dump(dns_stats_t *stats,
			 dns_dnssecsignstats_dumper_t dump_fn,
			 void *arg0, unsigned int options)
{
	dnssecsigndumparg_t arg;

	REQUIRE(DNS_STATS_VALID(stats) &&
		stats->type == dns_statstype_dnssec);

	arg.fn = dump_fn;
	arg.arg = arg0;
	isc_stats_dump(stats->counters, dnssec_dumpcb, &arg, options);
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
		     void *arg0, unsigned int options)
{
	opcodedumparg_t arg;

	REQUIRE(DNS_STATS_VALID(stats) && stats->type == dns_statstype_opcode);

	arg.fn = dump_fn;
	arg.arg = arg0;
	isc_stats_dump(stats->counters, opcode_dumpcb, &arg, options);
}

void
dns_rcodestats_dump(dns_stats_t *stats, dns_rcodestats_dumper_t dump_fn,
		     void *arg0, unsigned int options)
{
	rcodedumparg_t arg;

	REQUIRE(DNS_STATS_VALID(stats) && stats->type == dns_statstype_rcode);

	arg.fn = dump_fn;
	arg.arg = arg0;
	isc_stats_dump(stats->counters, rcode_dumpcb, &arg, options);
}

/***
 *** Obsolete variables and functions follow:
 ***/
LIBDNS_EXTERNAL_DATA const char *dns_statscounter_names[DNS_STATS_NCOUNTERS] =
	{
	"success",
	"referral",
	"nxrrset",
	"nxdomain",
	"recursion",
	"failure",
	"duplicate",
	"dropped"
	};

isc_result_t
dns_stats_alloccounters(isc_mem_t *mctx, uint64_t **ctrp) {
	int i;
	uint64_t *p =
		isc_mem_get(mctx, DNS_STATS_NCOUNTERS * sizeof(uint64_t));
	if (p == NULL)
		return (ISC_R_NOMEMORY);
	for (i = 0; i < DNS_STATS_NCOUNTERS; i++)
		p[i] = 0;
	*ctrp = p;
	return (ISC_R_SUCCESS);
}

void
dns_stats_freecounters(isc_mem_t *mctx, uint64_t **ctrp) {
	isc_mem_put(mctx, *ctrp, DNS_STATS_NCOUNTERS * sizeof(uint64_t));
	*ctrp = NULL;
}
