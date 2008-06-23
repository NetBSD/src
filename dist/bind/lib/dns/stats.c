/*	$NetBSD: stats.c,v 1.1.1.4.12.1 2008/06/23 04:28:06 wrstuden Exp $	*/

/*
 * Copyright (C) 2004, 2005, 2007, 2008  Internet Systems Consortium, Inc. ("ISC")
 * Copyright (C) 2000, 2001  Internet Software Consortium.
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND ISC DISCLAIMS ALL WARRANTIES WITH
 * REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS.  IN NO EVENT SHALL ISC BE LIABLE FOR ANY SPECIAL, DIRECT,
 * INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
 * LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE
 * OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */

/* Id: stats.c,v 1.12.128.4 2008/04/03 06:20:34 tbox Exp */

/*! \file */

#include <config.h>

#include <string.h>

#include <isc/atomic.h>
#include <isc/buffer.h>
#include <isc/magic.h>
#include <isc/mem.h>
#include <isc/platform.h>
#include <isc/print.h>
#include <isc/rwlock.h>
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
	dns_statstype_opcode = 3
} dns_statstype_t;

/*%
 * It doesn't make sense to have 2^16 counters for all possible types since
 * most of them won't be used.  We have counters for the first 256 types and
 * those explicitly supported in the rdata implementation.
 * XXXJT: this introduces tight coupling with the rdata implementation.
 * Ideally, we should have rdata handle this type of details.
 */
enum {
	/* For 0-255, we use the rdtype value as counter indices */
	rdtypecounter_dlv = 256,	/* for dns_rdatatype_dlv */
	rdtypecounter_others = 257,	/* anything else */
	rdtypecounter_max = 258,
	/* The following are used for rdataset */
	rdtypenxcounter_max = rdtypecounter_max * 2,
	rdtypecounter_nxdomain = rdtypenxcounter_max,
	rdatasettypecounter_max = rdtypecounter_nxdomain + 1
};

#ifndef DNS_STATS_USEMULTIFIELDS
#if defined(ISC_RWLOCK_USEATOMIC) && defined(ISC_PLATFORM_HAVEXADD) && !defined(ISC_PLATFORM_HAVEXADDQ)
#define DNS_STATS_USEMULTIFIELDS 1
#else
#define DNS_STATS_USEMULTIFIELDS 0
#endif
#endif	/* DNS_STATS_USEMULTIFIELDS */

#if DNS_STATS_USEMULTIFIELDS
typedef struct {
	isc_uint32_t hi;
	isc_uint32_t lo;
} dns_stat_t;
#else
typedef isc_uint64_t dns_stat_t;
#endif

struct dns_stats {
	/*% Unlocked */
	unsigned int	magic;
	dns_statstype_t	type;
	isc_mem_t	*mctx;
	int		ncounters;

	isc_mutex_t	lock;
	unsigned int	references; /* locked by lock */

	/*%
	 * Locked by counterlock or unlocked if efficient rwlock is not
	 * available.
	 */
#ifdef ISC_RWLOCK_USEATOMIC
	isc_rwlock_t	counterlock;
#endif
	dns_stat_t	*counters;

	/*%
	 * We don't want to lock the counters while we are dumping, so we first
	 * copy the current counter values into a local array.  This buffer
	 * will be used as the copy destination.  It's allocated on creation
	 * of the stats structure so that the dump operation won't fail due
	 * to memory allocation failure.
	 * XXX: this approach is weird for non-threaded build because the
	 * additional memory and the copy overhead could be avoided.  We prefer
	 * simplicity here, however, under the assumption that this function
	 * should be only rarely called.
	 */
	isc_uint64_t	*copiedcounters;
};

static isc_result_t
create_stats(isc_mem_t *mctx, dns_statstype_t type, int ncounters,
	     dns_stats_t **statsp)
{
	dns_stats_t *stats;
	isc_result_t result = ISC_R_SUCCESS;

	REQUIRE(statsp != NULL && *statsp == NULL);

	stats = isc_mem_get(mctx, sizeof(*stats));
	if (stats == NULL)
		return (ISC_R_NOMEMORY);

	result = isc_mutex_init(&stats->lock);
	if (result != ISC_R_SUCCESS)
		goto clean_stats;

	stats->counters = isc_mem_get(mctx, sizeof(dns_stat_t) * ncounters);
	if (stats->counters == NULL) {
		result = ISC_R_NOMEMORY;
		goto clean_mutex;
	}
	stats->copiedcounters = isc_mem_get(mctx,
					    sizeof(isc_uint64_t) * ncounters);
	if (stats->copiedcounters == NULL) {
		result = ISC_R_NOMEMORY;
		goto clean_counters;
	}

#ifdef ISC_RWLOCK_USEATOMIC
	result = isc_rwlock_init(&stats->counterlock, 0, 0);
	if (result != ISC_R_SUCCESS)
		goto clean_copiedcounters;
#endif

	stats->type = type;
	stats->references = 1;
	memset(stats->counters, 0, sizeof(dns_stat_t) * ncounters);
	stats->mctx = NULL;
	isc_mem_attach(mctx, &stats->mctx);
	stats->ncounters = ncounters;
	stats->magic = DNS_STATS_MAGIC;

	*statsp = stats;

	return (result);

clean_counters:
	isc_mem_put(mctx, stats->counters, sizeof(dns_stat_t) * ncounters);

#ifdef ISC_RWLOCK_USEATOMIC
clean_copiedcounters:
	isc_mem_put(mctx, stats->copiedcounters,
		    sizeof(dns_stat_t) * ncounters);
#endif

clean_mutex:
	DESTROYLOCK(&stats->lock);

clean_stats:
	isc_mem_put(mctx, stats, sizeof(*stats));

	return (result);
}

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
		isc_mem_put(stats->mctx, stats->copiedcounters,
			    sizeof(dns_stat_t) * stats->ncounters);
		isc_mem_put(stats->mctx, stats->counters,
			    sizeof(dns_stat_t) * stats->ncounters);
		DESTROYLOCK(&stats->lock);
#ifdef ISC_RWLOCK_USEATOMIC
		isc_rwlock_destroy(&stats->counterlock);
#endif
		isc_mem_putanddetach(&stats->mctx, stats, sizeof(*stats));
	}
}

static inline void
incrementcounter(dns_stats_t *stats, int counter) {
	isc_int32_t prev;

#ifdef ISC_RWLOCK_USEATOMIC
	/*
	 * We use a "read" lock to prevent other threads from reading the
	 * counter while we "writing" a counter field.  The write access itself
	 * is protected by the atomic operation.
	 */
	isc_rwlock_lock(&stats->counterlock, isc_rwlocktype_read);
#endif

#if DNS_STATS_USEMULTIFIELDS
	prev = isc_atomic_xadd((isc_int32_t *)&stats->counters[counter].lo, 1);
	/*
	 * If the lower 32-bit field overflows, increment the higher field.
	 * Note that it's *theoretically* possible that the lower field
	 * overlaps again before the higher field is incremented.  It doesn't
	 * matter, however, because we don't read the value until
	 * dns_stats_copy() is called where the whole process is protected
	 * by the write (exclusive) lock.
	 */
	if (prev == (isc_int32_t)0xffffffff)
		isc_atomic_xadd((isc_int32_t *)&stats->counters[counter].hi, 1);
#elif defined(ISC_PLATFORM_HAVEXADDQ)
	UNUSED(prev);
	isc_atomic_xaddq((isc_int64_t *)&stats->counters[counter], 1);
#else
	UNUSED(prev);
	stats->counters[counter]++;
#endif

#ifdef ISC_RWLOCK_USEATOMIC
	isc_rwlock_unlock(&stats->counterlock, isc_rwlocktype_read);
#endif
}

static inline void
decrementcounter(dns_stats_t *stats, int counter) {
	isc_int32_t prev;

#ifdef ISC_RWLOCK_USEATOMIC
	isc_rwlock_lock(&stats->counterlock, isc_rwlocktype_read);
#endif

#if DNS_STATS_USEMULTIFIELDS
	prev = isc_atomic_xadd((isc_int32_t *)&stats->counters[counter].lo, -1);
	if (prev == 0)
		isc_atomic_xadd((isc_int32_t *)&stats->counters[counter].hi,
				-1);
#elif defined(ISC_PLATFORM_HAVEXADDQ)
	UNUSED(prev);
	isc_atomic_xaddq((isc_int64_t *)&stats->counters[counter], -1);
#else
	UNUSED(prev);
	stats->counters[counter]--;
#endif

#ifdef ISC_RWLOCK_USEATOMIC
	isc_rwlock_unlock(&stats->counterlock, isc_rwlocktype_read);
#endif
}

static void
copy_counters(dns_stats_t *stats) {
	int i;

#ifdef ISC_RWLOCK_USEATOMIC
	/*
	 * We use a "write" lock before "reading" the statistics counters as
	 * an exclusive lock.
	 */
	isc_rwlock_lock(&stats->counterlock, isc_rwlocktype_write);
#endif

#if DNS_STATS_USEMULTIFIELDS
	for (i = 0; i < stats->ncounters; i++) {
		stats->copiedcounters[i] =
				(isc_uint64_t)(stats->counters[i].hi) << 32 |
				stats->counters[i].lo;
	}
#else
	UNUSED(i);
	memcpy(stats->copiedcounters, stats->counters,
	       stats->ncounters * sizeof(dns_stat_t));
#endif

#ifdef ISC_RWLOCK_USEATOMIC
	isc_rwlock_unlock(&stats->counterlock, isc_rwlocktype_write);
#endif
}

/*%
 * Create methods
 */
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
			     (rdtypecounter_max * 2) + 1, statsp));
}

isc_result_t
dns_opcodestats_create(isc_mem_t *mctx, dns_stats_t **statsp) {
	REQUIRE(statsp != NULL && *statsp == NULL);

	return (create_stats(mctx, dns_statstype_opcode, 16, statsp));
}

/*%
 * Increment/Decrement methods
 */
void
dns_generalstats_increment(dns_stats_t *stats, dns_statscounter_t counter) {
	REQUIRE(DNS_STATS_VALID(stats) && stats->type == dns_statstype_general);
	REQUIRE(counter < stats->ncounters);

	incrementcounter(stats, (int)counter);
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

	incrementcounter(stats, counter);
}

static inline void
update_rdatasetstats(dns_stats_t *stats, dns_rdatastatstype_t rrsettype,
		     isc_boolean_t increment)
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

	if (increment)
		incrementcounter(stats, counter);
	else
		decrementcounter(stats, counter);
}

void
dns_rdatasetstats_increment(dns_stats_t *stats, dns_rdatastatstype_t rrsettype)
{
	REQUIRE(DNS_STATS_VALID(stats) &&
		stats->type == dns_statstype_rdataset);

	update_rdatasetstats(stats, rrsettype, ISC_TRUE);
}

void
dns_rdatasetstats_decrement(dns_stats_t *stats, dns_rdatastatstype_t rrsettype)
{
	REQUIRE(DNS_STATS_VALID(stats) &&
		stats->type == dns_statstype_rdataset);

	update_rdatasetstats(stats, rrsettype, ISC_FALSE);
}
void
dns_opcodestats_increment(dns_stats_t *stats, dns_opcode_t code) {
	REQUIRE(DNS_STATS_VALID(stats) && stats->type == dns_statstype_opcode);

	incrementcounter(stats, (int)code);
}

/*%
 * Dump methods
 */
void
dns_generalstats_dump(dns_stats_t *stats, dns_generalstats_dumper_t dump_fn,
		      void *arg, unsigned int options)
{
	int i;

	REQUIRE(DNS_STATS_VALID(stats) && stats->type == dns_statstype_general);

	copy_counters(stats);

	for (i = 0; i < stats->ncounters; i++) {
		if ((options & DNS_STATSDUMP_VERBOSE) == 0 &&
		    stats->copiedcounters[i] == 0)
				continue;
		dump_fn(i, stats->copiedcounters[i], arg);
	}
}

static void
dump_rdentry(dns_stats_t *stats, int counter, int rdcounter,
	     dns_rdatastatstype_t attributes,
	     dns_rdatatypestats_dumper_t dump_fn, void * arg,
	     unsigned int options)
{
	dns_rdatatype_t rdtype = dns_rdatatype_none; /* sentinel */
	dns_rdatastatstype_t type;

	if ((options & DNS_STATSDUMP_VERBOSE) == 0 &&
	    stats->copiedcounters[counter] == 0)
		return;
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
	dump_fn(type, stats->copiedcounters[counter], arg);
}

void
dns_rdatatypestats_dump(dns_stats_t *stats, dns_rdatatypestats_dumper_t dump_fn,
			void *arg, unsigned int options)
{
	int i;

	REQUIRE(DNS_STATS_VALID(stats) && stats->type == dns_statstype_rdtype);

	copy_counters(stats);

	for (i = 0; i < stats->ncounters; i++)
		dump_rdentry(stats, i, i, 0, dump_fn, arg, options);
}

void
dns_rdatasetstats_dump(dns_stats_t *stats, dns_rdatatypestats_dumper_t dump_fn,
		       void *arg, unsigned int options)
{
	int i;

	REQUIRE(DNS_STATS_VALID(stats) &&
		stats->type == dns_statstype_rdataset);

	copy_counters(stats);

	for (i = 0; i < rdtypecounter_max; i++)
		dump_rdentry(stats, i, i, 0, dump_fn, arg, options);
	for (i = rdtypecounter_max; i < rdtypenxcounter_max; i++) {
		dump_rdentry(stats, i, i - rdtypecounter_max,
			     DNS_RDATASTATSTYPE_ATTR_NXRRSET,
			     dump_fn, arg, options);
	}
	dump_rdentry(stats, rdtypecounter_nxdomain, 0,
		     DNS_RDATASTATSTYPE_ATTR_NXDOMAIN, dump_fn, arg, options);

	INSIST(i < stats->ncounters);
}

void
dns_opcodestats_dump(dns_stats_t *stats, dns_opcodestats_dumper_t dump_fn,
		     void *arg, unsigned int options)
{
	int i;

	REQUIRE(DNS_STATS_VALID(stats) && stats->type == dns_statstype_opcode);

	copy_counters(stats);

	for (i = 0; i < stats->ncounters; i++) {
		if ((options & DNS_STATSDUMP_VERBOSE) == 0 &&
		    stats->copiedcounters[i] == 0)
				continue;
		dump_fn((dns_opcode_t)i, stats->copiedcounters[i], arg);
	}
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
dns_stats_alloccounters(isc_mem_t *mctx, isc_uint64_t **ctrp) {
	int i;
	isc_uint64_t *p =
		isc_mem_get(mctx, DNS_STATS_NCOUNTERS * sizeof(isc_uint64_t));
	if (p == NULL)
		return (ISC_R_NOMEMORY);
	for (i = 0; i < DNS_STATS_NCOUNTERS; i++)
		p[i] = 0;
	*ctrp = p;
	return (ISC_R_SUCCESS);
}

void
dns_stats_freecounters(isc_mem_t *mctx, isc_uint64_t **ctrp) {
	isc_mem_put(mctx, *ctrp, DNS_STATS_NCOUNTERS * sizeof(isc_uint64_t));
	*ctrp = NULL;
}
