/*	$NetBSD: adb.c,v 1.13 2025/01/26 16:25:21 christos Exp $	*/

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
#include <limits.h>
#include <stdbool.h>

#include <isc/async.h>
#include <isc/atomic.h>
#include <isc/hash.h>
#include <isc/hashmap.h>
#include <isc/list.h>
#include <isc/loop.h>
#include <isc/mutex.h>
#include <isc/netaddr.h>
#include <isc/random.h>
#include <isc/result.h>
#include <isc/rwlock.h>
#include <isc/stats.h>
#include <isc/string.h>
#include <isc/tid.h>
#include <isc/util.h>

#include <dns/adb.h>
#include <dns/db.h>
#include <dns/log.h>
#include <dns/rdata.h>
#include <dns/rdataset.h>
#include <dns/rdatastruct.h>
#include <dns/rdatatype.h>
#include <dns/resolver.h>
#include <dns/stats.h>
#include <dns/transport.h>

#define DNS_ADB_MAGIC		 ISC_MAGIC('D', 'a', 'd', 'b')
#define DNS_ADB_VALID(x)	 ISC_MAGIC_VALID(x, DNS_ADB_MAGIC)
#define DNS_ADBNAME_MAGIC	 ISC_MAGIC('a', 'd', 'b', 'N')
#define DNS_ADBNAME_VALID(x)	 ISC_MAGIC_VALID(x, DNS_ADBNAME_MAGIC)
#define DNS_ADBNAMEHOOK_MAGIC	 ISC_MAGIC('a', 'd', 'N', 'H')
#define DNS_ADBNAMEHOOK_VALID(x) ISC_MAGIC_VALID(x, DNS_ADBNAMEHOOK_MAGIC)
#define DNS_ADBENTRY_MAGIC	 ISC_MAGIC('a', 'd', 'b', 'E')
#define DNS_ADBENTRY_VALID(x)	 ISC_MAGIC_VALID(x, DNS_ADBENTRY_MAGIC)
#define DNS_ADBFETCH_MAGIC	 ISC_MAGIC('a', 'd', 'F', '4')
#define DNS_ADBFETCH_VALID(x)	 ISC_MAGIC_VALID(x, DNS_ADBFETCH_MAGIC)
#define DNS_ADBFETCH6_MAGIC	 ISC_MAGIC('a', 'd', 'F', '6')
#define DNS_ADBFETCH6_VALID(x)	 ISC_MAGIC_VALID(x, DNS_ADBFETCH6_MAGIC)

/*!
 * For type 3 negative cache entries, we will remember that the address is
 * broken for this long.  XXXMLG This is also used for actual addresses, too.
 * The intent is to keep us from constantly asking about A/AAAA records
 * if the zone has extremely low TTLs.
 */
#define ADB_CACHE_MINIMUM 10	/*%< seconds */
#define ADB_CACHE_MAXIMUM 86400 /*%< seconds (86400 = 24 hours) */
#define ADB_ENTRY_WINDOW  60	/*%< seconds */

#ifndef ADB_HASH_BITS
#define ADB_HASH_BITS 12
#endif /* ifndef ADB_HASH_BITS */

/*%
 * The period in seconds after which an ADB name entry is regarded as stale
 * and forced to be cleaned up.
 * TODO: This should probably be configurable at run-time.
 */
#ifndef ADB_STALE_MARGIN
#define ADB_STALE_MARGIN 1800
#endif /* ifndef ADB_STALE_MARGIN */

#define DNS_ADB_MINADBSIZE (1024U * 1024U) /*%< 1 Megabyte */

typedef ISC_LIST(dns_adbname_t) dns_adbnamelist_t;
typedef struct dns_adbnamehook dns_adbnamehook_t;
typedef ISC_LIST(dns_adbnamehook_t) dns_adbnamehooklist_t;
typedef ISC_LIST(dns_adbentry_t) dns_adbentrylist_t;
typedef struct dns_adbfetch dns_adbfetch_t;
typedef struct dns_adbfetch6 dns_adbfetch6_t;

/*% dns adb structure */
struct dns_adb {
	unsigned int magic;

	isc_mutex_t lock;
	isc_mem_t *mctx;
	isc_mem_t *hmctx;
	dns_view_t *view;
	dns_resolver_t *res;

	isc_refcount_t references;

	dns_adbnamelist_t names_lru;
	isc_stdtime_t names_last_update;
	isc_hashmap_t *names;
	isc_rwlock_t names_lock;

	dns_adbentrylist_t entries_lru;
	isc_stdtime_t entries_last_update;
	isc_hashmap_t *entries;
	isc_rwlock_t entries_lock;

	isc_stats_t *stats;

	atomic_bool exiting;

	uint32_t quota;
	uint32_t atr_freq;
	double atr_low;
	double atr_high;
	double atr_discount;
};

/*%
 * dns_adbname structure:
 *
 * This is the structure representing a nameserver name; it can be looked
 * up via the adb->names hash table. It holds references to fetches
 * for A and AAAA records while they are ongoing (fetch_a, fetch_aaaa), and
 * lists of records pointing to address information when the fetches are
 * complete (v4, v6).
 */
struct dns_adbname {
	unsigned int magic;
	isc_refcount_t references;
	dns_adb_t *adb;
	dns_fixedname_t fname;
	dns_name_t *name;
	unsigned int partial_result;
	unsigned int flags;
	dns_name_t target;
	isc_stdtime_t expire_target;
	isc_stdtime_t expire_v4;
	isc_stdtime_t expire_v6;
	dns_adbnamehooklist_t v4;
	dns_adbnamehooklist_t v6;
	dns_adbfetch_t *fetch_a;
	dns_adbfetch_t *fetch_aaaa;
	unsigned int fetch_err;
	unsigned int fetch6_err;
	dns_adbfindlist_t finds;
	isc_mutex_t lock;
	isc_stdtime_t last_used;
	/* for LRU-based management */

	ISC_LINK(dns_adbname_t) link;
};

#if DNS_ADB_TRACE
#define dns_adbname_ref(ptr) dns_adbname__ref(ptr, __func__, __FILE__, __LINE__)
#define dns_adbname_unref(ptr) \
	dns_adbname__unref(ptr, __func__, __FILE__, __LINE__)
#define dns_adbname_attach(ptr, ptrp) \
	dns_adbname__attach(ptr, ptrp, __func__, __FILE__, __LINE__)
#define dns_adbname_detach(ptrp) \
	dns_adbname__detach(ptrp, __func__, __FILE__, __LINE__)
ISC_REFCOUNT_TRACE_DECL(dns_adbname);
#else
ISC_REFCOUNT_DECL(dns_adbname);
#endif

/*%
 * dns_adbfetch structure:
 * Stores the state for an ongoing A or AAAA fetch.
 */
struct dns_adbfetch {
	unsigned int magic;
	dns_fetch_t *fetch;
	dns_rdataset_t rdataset;
	unsigned int depth;
};

/*%
 * dns_adbnamehook structure:
 *
 * This is a small widget that dangles off a dns_adbname_t.  It contains a
 * pointer to the address information about this host, and a link to the next
 * namehook that will contain the next address this host has.
 */
struct dns_adbnamehook {
	unsigned int magic;
	dns_adbentry_t *entry;
	ISC_LINK(dns_adbnamehook_t) name_link;
	ISC_LINK(dns_adbnamehook_t) entry_link;
};

/*%
 * dns_adbentry structure:
 *
 * This is the structure representing a nameserver address; it can be looked
 * up via the adb->entries hash table. Also, each dns_adbnamehook and
 * and dns_adbaddrinfo object will contain a pointer to one of these.
 *
 * The structure holds quite a bit of information about addresses,
 * including edns state (in "flags"), RTT, and of course the address of
 * the host.
 */
struct dns_adbentry {
	unsigned int magic;

	dns_adb_t *adb;

	isc_mutex_t lock;
	isc_stdtime_t last_used;

	isc_refcount_t references;
	dns_adbnamehooklist_t nhs;

	atomic_uint flags;
	atomic_uint srtt;
	unsigned int completed;
	unsigned int timeouts;
	unsigned char plain;
	unsigned char plainto;
	unsigned char edns;
	unsigned char ednsto;
	uint16_t udpsize;

	uint8_t mode;
	atomic_uint_fast32_t quota;
	atomic_uint_fast32_t active;
	double atr;

	isc_sockaddr_t sockaddr;
	unsigned char *cookie;
	uint16_t cookielen;

	isc_stdtime_t expires;
	_Atomic(isc_stdtime_t) lastage;
	/*%<
	 * A nonzero 'expires' field indicates that the entry should
	 * persist until that time.  This allows entries found
	 * using dns_adb_findaddrinfo() to persist for a limited time
	 * even though they are not necessarily associated with a
	 * entry.
	 */

	ISC_LINK(dns_adbentry_t) link;
};

#if DNS_ADB_TRACE
#define dns_adbentry_ref(ptr) \
	dns_adbentry__ref(ptr, __func__, __FILE__, __LINE__)
#define dns_adbentry_unref(ptr) \
	dns_adbentry__unref(ptr, __func__, __FILE__, __LINE__)
#define dns_adbentry_attach(ptr, ptrp) \
	dns_adbentry__attach(ptr, ptrp, __func__, __FILE__, __LINE__)
#define dns_adbentry_detach(ptrp) \
	dns_adbentry__detach(ptrp, __func__, __FILE__, __LINE__)
ISC_REFCOUNT_TRACE_DECL(dns_adbentry);
#else
ISC_REFCOUNT_DECL(dns_adbentry);
#endif

/*
 * Internal functions (and prototypes).
 */
static dns_adbname_t *
new_adbname(dns_adb_t *adb, const dns_name_t *, unsigned int flags);
static void
destroy_adbname(dns_adbname_t *);
static bool
match_adbname(void *node, const void *key);
static uint32_t
hash_adbname(const dns_adbname_t *adbname);
static dns_adbnamehook_t *
new_adbnamehook(dns_adb_t *adb);
static void
free_adbnamehook(dns_adb_t *adb, dns_adbnamehook_t **namehookp);
static dns_adbentry_t *
new_adbentry(dns_adb_t *adb, const isc_sockaddr_t *addr, isc_stdtime_t now);
static void
destroy_adbentry(dns_adbentry_t *entry);
static bool
match_adbentry(void *node, const void *key);
static dns_adbfind_t *
new_adbfind(dns_adb_t *, in_port_t);
static void
free_adbfind(dns_adbfind_t **);
static dns_adbaddrinfo_t *
new_adbaddrinfo(dns_adb_t *, dns_adbentry_t *, in_port_t);
static dns_adbfetch_t *
new_adbfetch(dns_adb_t *);
static void
free_adbfetch(dns_adb_t *, dns_adbfetch_t **);
static void
purge_stale_names(dns_adb_t *adb, isc_stdtime_t now);
static dns_adbname_t *
get_attached_and_locked_name(dns_adb_t *, const dns_name_t *,
			     unsigned int flags, isc_stdtime_t now);
static void
purge_stale_entries(dns_adb_t *adb, isc_stdtime_t now);
static dns_adbentry_t *
get_attached_and_locked_entry(dns_adb_t *adb, isc_stdtime_t now,
			      const isc_sockaddr_t *addr);
static void
dump_adb(dns_adb_t *, FILE *, bool debug, isc_stdtime_t);
static void
print_namehook_list(FILE *, const char *legend, dns_adb_t *adb,
		    dns_adbnamehooklist_t *list, bool debug, isc_stdtime_t now);
static void
print_find_list(FILE *, dns_adbname_t *);
static void
print_fetch_list(FILE *, dns_adbname_t *);
static void
clean_namehooks(dns_adb_t *, dns_adbnamehooklist_t *);
static void
clean_target(dns_adb_t *, dns_name_t *);
static void
clean_finds_at_name(dns_adbname_t *, dns_adbstatus_t, unsigned int);
static void
maybe_expire_namehooks(dns_adbname_t *, isc_stdtime_t);
static bool
maybe_expire_name(dns_adbname_t *adbname, isc_stdtime_t now);
static void
expire_name(dns_adbname_t *adbname, dns_adbstatus_t astat);
static bool
entry_expired(dns_adbentry_t *adbentry, isc_stdtime_t now);
static bool
maybe_expire_entry(dns_adbentry_t *adbentry, isc_stdtime_t now);
static void
expire_entry(dns_adbentry_t *adbentry);
static isc_result_t
dbfind_name(dns_adbname_t *, isc_stdtime_t, dns_rdatatype_t);
static isc_result_t
fetch_name(dns_adbname_t *, bool, unsigned int, isc_counter_t *qc,
	   dns_rdatatype_t);
static void
destroy(dns_adb_t *);
static void
shutdown_names(dns_adb_t *);
static void
shutdown_entries(dns_adb_t *);
static void
dump_entry(FILE *, dns_adb_t *, dns_adbentry_t *, bool, isc_stdtime_t);
static void
adjustsrtt(dns_adbaddrinfo_t *addr, unsigned int rtt, unsigned int factor,
	   isc_stdtime_t now);
static void
log_quota(dns_adbentry_t *entry, const char *fmt, ...) ISC_FORMAT_PRINTF(2, 3);

static bool
adbentry_overquota(dns_adbentry_t *entry);

/*
 * Private flag(s) for adbfind objects. These are used internally and
 * are not meant to be seen or used by the caller; however, we use the
 * same flags field as for DNS_ADBFIND_xxx flags, so we must be careful
 * that there is no overlap between these values and those. To make it
 * easier, we will number these starting from the most significant bit
 * instead of the least significant.
 */
enum {
	FIND_EVENT_SENT = 1 << 31,
};
#define FIND_EVENTSENT(h) (((h)->flags & FIND_EVENT_SENT) != 0)

/*
 * Private flag(s) for adbname objects.
 */
enum {
	NAME_IS_DEAD = 1 << 31,
};
#define NAME_DEAD(n) (((n)->flags & NAME_IS_DEAD) != 0)

/*
 * Private flag(s) for adbentry objects.  Note that these will also
 * be used for addrinfo flags, and in resolver.c we'll use the same
 * field for FCTX_ADDRINFO_xxx flags to store information about remote
 * servers, so we must be careful that there is no overlap between
 * these values and those. To make it easier, we will number these
 * starting from the most significant bit instead of the least
 * significant.
 */
enum {
	ENTRY_IS_DEAD = 1 << 31,
};
#define ENTRY_DEAD(e) ((atomic_load(&(e)->flags) & ENTRY_IS_DEAD) != 0)

/*
 * To the name, address classes are all that really exist.  If it has a
 * V6 address it doesn't care if it came from a AAAA query.
 */
#define NAME_HAS_V4(n) (!ISC_LIST_EMPTY((n)->v4))
#define NAME_HAS_V6(n) (!ISC_LIST_EMPTY((n)->v6))

/*
 * Fetches are broken out into A and AAAA types.  In some cases,
 * however, it makes more sense to test for a particular class of fetches,
 * like V4 or V6 above.
 */
#define NAME_FETCH_A(n)	   ((n)->fetch_a != NULL)
#define NAME_FETCH_AAAA(n) ((n)->fetch_aaaa != NULL)
#define NAME_FETCH(n)	   (NAME_FETCH_A(n) || NAME_FETCH_AAAA(n))

/*
 * Find options and tests to see if there are addresses on the list.
 */
#define FIND_WANTEVENT(fn)	(((fn)->options & DNS_ADBFIND_WANTEVENT) != 0)
#define FIND_WANTEMPTYEVENT(fn) (((fn)->options & DNS_ADBFIND_EMPTYEVENT) != 0)
#define FIND_AVOIDFETCHES(fn)	(((fn)->options & DNS_ADBFIND_AVOIDFETCHES) != 0)
#define FIND_STARTATZONE(fn)	(((fn)->options & DNS_ADBFIND_STARTATZONE) != 0)
#define FIND_STATICSTUB(fn)	(((fn)->options & DNS_ADBFIND_STATICSTUB) != 0)
#define FIND_HAS_ADDRS(fn)	(!ISC_LIST_EMPTY((fn)->list))
#define FIND_NOFETCH(fn)	(((fn)->options & DNS_ADBFIND_NOFETCH) != 0)

#define ADBNAME_FLAGS_MASK (DNS_ADBFIND_STARTATZONE | DNS_ADBFIND_STATICSTUB)

/*
 * These are currently used on simple unsigned ints, so they are
 * not really associated with any particular type.
 */
#define WANT_INET(x)  (((x) & DNS_ADBFIND_INET) != 0)
#define WANT_INET6(x) (((x) & DNS_ADBFIND_INET6) != 0)

#define EXPIRE_OK(exp, now) ((exp == INT_MAX) || (exp < now))

/*
 * Find out if the flags on a name (nf) indicate if it is a hint or
 * glue, and compare this to the appropriate bits set in o, to see if
 * this is ok.
 */
#define STARTATZONE_MATCHES(nf, o)                  \
	(((nf)->flags & DNS_ADBFIND_STARTATZONE) == \
	 ((o) & DNS_ADBFIND_STARTATZONE))

#define ENTER_LEVEL  ISC_LOG_DEBUG(50)
#define CLEAN_LEVEL  ISC_LOG_DEBUG(100)
#define DEF_LEVEL    ISC_LOG_DEBUG(5)
#define NCACHE_LEVEL ISC_LOG_DEBUG(20)

#define NCACHE_RESULT(r) \
	((r) == DNS_R_NCACHENXDOMAIN || (r) == DNS_R_NCACHENXRRSET)
#define AUTH_NX(r) ((r) == DNS_R_NXDOMAIN || (r) == DNS_R_NXRRSET)

/*
 * Due to the ttlclamp(), the TTL is never 0 unless the trust is ultimate,
 * in which case we need to set the expiration to have immediate effect.
 */
#define ADJUSTED_EXPIRE(expire, now, ttl)                                      \
	((ttl != 0)                                                            \
		 ? ISC_MIN(expire, ISC_MAX(now + ADB_ENTRY_WINDOW, now + ttl)) \
		 : INT_MAX)

/*
 * Error states.
 */
enum {
	FIND_ERR_SUCCESS = 0,
	FIND_ERR_CANCELED,
	FIND_ERR_FAILURE,
	FIND_ERR_NXDOMAIN,
	FIND_ERR_NXRRSET,
	FIND_ERR_UNEXPECTED,
	FIND_ERR_NOTFOUND,
};

static const char *errnames[] = { "success",  "canceled", "failure",
				  "nxdomain", "nxrrset",  "unexpected",
				  "not_found" };

static isc_result_t find_err_map[] = {
	ISC_R_SUCCESS, ISC_R_CANCELED,	 ISC_R_FAILURE, DNS_R_NXDOMAIN,
	DNS_R_NXRRSET, ISC_R_UNEXPECTED, ISC_R_NOTFOUND /* not YET found */
};

static void
DP(int level, const char *format, ...) ISC_FORMAT_PRINTF(2, 3);

static void
DP(int level, const char *format, ...) {
	va_list args;

	va_start(args, format);
	isc_log_vwrite(dns_lctx, DNS_LOGCATEGORY_DATABASE, DNS_LOGMODULE_ADB,
		       level, format, args);
	va_end(args);
}

/*%
 * Increment resolver-related statistics counters.
 */
static void
inc_resstats(dns_adb_t *adb, isc_statscounter_t counter) {
	if (adb->res != NULL) {
		dns_resolver_incstats(adb->res, counter);
	}
}

/*%
 * Set adb-related statistics counters.
 */
static void
set_adbstat(dns_adb_t *adb, uint64_t val, isc_statscounter_t counter) {
	if (adb->stats != NULL) {
		isc_stats_set(adb->stats, val, counter);
	}
}

static void
dec_adbstats(dns_adb_t *adb, isc_statscounter_t counter) {
	if (adb->stats != NULL) {
		isc_stats_decrement(adb->stats, counter);
	}
}

static void
inc_adbstats(dns_adb_t *adb, isc_statscounter_t counter) {
	if (adb->stats != NULL) {
		isc_stats_increment(adb->stats, counter);
	}
}

static dns_ttl_t
ttlclamp(dns_ttl_t ttl) {
	if (ttl < ADB_CACHE_MINIMUM) {
		ttl = ADB_CACHE_MINIMUM;
	}
	if (ttl > ADB_CACHE_MAXIMUM) {
		ttl = ADB_CACHE_MAXIMUM;
	}

	return ttl;
}

/*
 * Requires the name to be locked and that no entries to be locked.
 *
 * This code handles A and AAAA rdatasets only.
 */
static isc_result_t
import_rdataset(dns_adbname_t *adbname, dns_rdataset_t *rdataset,
		isc_stdtime_t now) {
	isc_result_t result;
	dns_adb_t *adb = NULL;
	dns_rdatatype_t rdtype;

	REQUIRE(DNS_ADBNAME_VALID(adbname));

	adb = adbname->adb;

	REQUIRE(DNS_ADB_VALID(adb));

	rdtype = rdataset->type;

	switch (rdataset->trust) {
	case dns_trust_glue:
	case dns_trust_additional:
		rdataset->ttl = ADB_CACHE_MINIMUM;
		break;
	case dns_trust_ultimate:
		rdataset->ttl = 0;
		break;
	default:
		rdataset->ttl = ttlclamp(rdataset->ttl);
	}

	REQUIRE(rdtype == dns_rdatatype_a || rdtype == dns_rdatatype_aaaa);

	for (result = dns_rdataset_first(rdataset); result == ISC_R_SUCCESS;
	     result = dns_rdataset_next(rdataset))
	{
		/* FIXME: Move to a separate function */
		dns_adbnamehooklist_t *hookhead = NULL;
		dns_adbentry_t *entry = NULL;
		dns_adbnamehook_t *nh = NULL;
		dns_rdata_t rdata = DNS_RDATA_INIT;
		isc_sockaddr_t sockaddr;
		struct in_addr ina;
		struct in6_addr in6a;

		dns_rdataset_current(rdataset, &rdata);
		switch (rdtype) {
		case dns_rdatatype_a:
			INSIST(rdata.length == 4);
			memmove(&ina.s_addr, rdata.data, 4);
			isc_sockaddr_fromin(&sockaddr, &ina, 0);
			hookhead = &adbname->v4;
			break;
		case dns_rdatatype_aaaa:
			INSIST(rdata.length == 16);
			memmove(in6a.s6_addr, rdata.data, 16);
			isc_sockaddr_fromin6(&sockaddr, &in6a, 0);
			hookhead = &adbname->v6;
			break;
		default:
			UNREACHABLE();
		}

		entry = get_attached_and_locked_entry(adb, now, &sockaddr);
		INSIST(!ENTRY_DEAD(entry));

		dns_adbnamehook_t *anh = NULL;
		for (anh = ISC_LIST_HEAD(*hookhead); anh != NULL;
		     anh = ISC_LIST_NEXT(anh, name_link))
		{
			if (anh->entry == entry) {
				break;
			}
		}
		if (anh == NULL) {
			nh = new_adbnamehook(adb);
			dns_adbentry_attach(entry, &nh->entry);
			ISC_LIST_APPEND(*hookhead, nh, name_link);
			ISC_LIST_APPEND(entry->nhs, nh, entry_link);
		}
		UNLOCK(&entry->lock);
		dns_adbentry_detach(&entry);
	}
	if (result == ISC_R_NOMORE) {
		result = ISC_R_SUCCESS;
	}
	INSIST(result == ISC_R_SUCCESS);

	switch (rdtype) {
	case dns_rdatatype_a:
		adbname->expire_v4 = ADJUSTED_EXPIRE(adbname->expire_v4, now,
						     rdataset->ttl);
		DP(NCACHE_LEVEL, "expire_v4 set to %u import_rdataset",
		   adbname->expire_v4);
		break;
	case dns_rdatatype_aaaa:
		adbname->expire_v6 = ADJUSTED_EXPIRE(adbname->expire_v6, now,
						     rdataset->ttl);
		DP(NCACHE_LEVEL, "expire_v6 set to %u import_rdataset",
		   adbname->expire_v6);
		break;
	default:
		UNREACHABLE();
	}

	return ISC_R_SUCCESS;
}

static bool
match_ptr(void *node, const void *key) {
	return node == key;
}

/*
 * Requires the name to be locked.
 */
static void
expire_name(dns_adbname_t *adbname, dns_adbstatus_t astat) {
	isc_result_t result;

	REQUIRE(DNS_ADBNAME_VALID(adbname));

	dns_adb_t *adb = adbname->adb;

	REQUIRE(DNS_ADB_VALID(adb));

	DP(DEF_LEVEL, "killing name %p", adbname);

	/*
	 * Clean up the name's various contents.  These functions
	 * are destructive in that they will always empty the lists
	 * of finds and namehooks.
	 */
	clean_finds_at_name(adbname, astat, DNS_ADBFIND_ADDRESSMASK);
	clean_namehooks(adb, &adbname->v4);
	clean_namehooks(adb, &adbname->v6);
	clean_target(adb, &adbname->target);

	if (NAME_FETCH_A(adbname)) {
		dns_resolver_cancelfetch(adbname->fetch_a->fetch);
	}

	if (NAME_FETCH_AAAA(adbname)) {
		dns_resolver_cancelfetch(adbname->fetch_aaaa->fetch);
	}

	adbname->flags |= NAME_IS_DEAD;

	/*
	 * Remove the adbname from the hashtable...
	 */
	result = isc_hashmap_delete(adb->names, hash_adbname(adbname),
				    match_ptr, adbname);
	RUNTIME_CHECK(result == ISC_R_SUCCESS);
	/* ... and LRU list */
	ISC_LIST_UNLINK(adb->names_lru, adbname, link);

	dns_adbname_unref(adbname);
}

/*
 * Requires the name to be locked and no entries to be locked.
 */
static void
maybe_expire_namehooks(dns_adbname_t *adbname, isc_stdtime_t now) {
	REQUIRE(DNS_ADBNAME_VALID(adbname));
	REQUIRE(DNS_ADB_VALID(adbname->adb));

	dns_adb_t *adb = adbname->adb;

	/*
	 * Check to see if we need to remove the v4 addresses
	 */
	if (!NAME_FETCH_A(adbname) && EXPIRE_OK(adbname->expire_v4, now)) {
		if (NAME_HAS_V4(adbname)) {
			DP(DEF_LEVEL, "expiring v4 for name %p", adbname);
			clean_namehooks(adb, &adbname->v4);
			adbname->partial_result &= ~DNS_ADBFIND_INET;
		}
		adbname->expire_v4 = INT_MAX;
		adbname->fetch_err = FIND_ERR_UNEXPECTED;
	}

	/*
	 * Check to see if we need to remove the v6 addresses
	 */
	if (!NAME_FETCH_AAAA(adbname) && EXPIRE_OK(adbname->expire_v6, now)) {
		if (NAME_HAS_V6(adbname)) {
			DP(DEF_LEVEL, "expiring v6 for name %p", adbname);
			clean_namehooks(adb, &adbname->v6);
			adbname->partial_result &= ~DNS_ADBFIND_INET6;
		}
		adbname->expire_v6 = INT_MAX;
		adbname->fetch6_err = FIND_ERR_UNEXPECTED;
	}

	/*
	 * Check to see if we need to remove the alias target.
	 */
	if (EXPIRE_OK(adbname->expire_target, now)) {
		clean_target(adb, &adbname->target);
		adbname->expire_target = INT_MAX;
	}
}

static void
shutdown_names(dns_adb_t *adb) {
	dns_adbname_t *next = NULL;

	RWLOCK(&adb->names_lock, isc_rwlocktype_write);
	for (dns_adbname_t *name = ISC_LIST_HEAD(adb->names_lru); name != NULL;
	     name = next)
	{
		next = ISC_LIST_NEXT(name, link);
		dns_adbname_ref(name);
		LOCK(&name->lock);
		/*
		 * Run through the list.  For each name, clean up finds
		 * found there, and cancel any fetches running.  When
		 * all the fetches are canceled, the name will destroy
		 * itself.
		 */
		expire_name(name, DNS_ADB_SHUTTINGDOWN);
		UNLOCK(&name->lock);
		dns_adbname_detach(&name);
	}
	RWUNLOCK(&adb->names_lock, isc_rwlocktype_write);
}

static void
shutdown_entries(dns_adb_t *adb) {
	dns_adbentry_t *next = NULL;
	RWLOCK(&adb->entries_lock, isc_rwlocktype_write);
	for (dns_adbentry_t *adbentry = ISC_LIST_HEAD(adb->entries_lru);
	     adbentry != NULL; adbentry = next)
	{
		next = ISC_LIST_NEXT(adbentry, link);
		expire_entry(adbentry);
	}
	RWUNLOCK(&adb->entries_lock, isc_rwlocktype_write);
}

/*
 * The name containing the 'namehooks' list must be locked.
 */
static void
clean_namehooks(dns_adb_t *adb, dns_adbnamehooklist_t *namehooks) {
	dns_adbnamehook_t *namehook = NULL;

	namehook = ISC_LIST_HEAD(*namehooks);
	while (namehook != NULL) {
		INSIST(DNS_ADBNAMEHOOK_VALID(namehook));
		INSIST(DNS_ADBENTRY_VALID(namehook->entry));

		dns_adbentry_t *adbentry = namehook->entry;
		namehook->entry = NULL;

		/*
		 * Free the namehook
		 */
		ISC_LIST_UNLINK(*namehooks, namehook, name_link);

		LOCK(&adbentry->lock);
		ISC_LIST_UNLINK(adbentry->nhs, namehook, entry_link);
		UNLOCK(&adbentry->lock);
		dns_adbentry_detach(&adbentry);

		free_adbnamehook(adb, &namehook);

		namehook = ISC_LIST_HEAD(*namehooks);
	}
}

static void
clean_target(dns_adb_t *adb, dns_name_t *target) {
	if (dns_name_countlabels(target) > 0) {
		dns_name_free(target, adb->mctx);
		dns_name_init(target, NULL);
	}
}

static isc_result_t
set_target(dns_adb_t *adb, const dns_name_t *name, const dns_name_t *fname,
	   dns_rdataset_t *rdataset, dns_name_t *target) {
	isc_result_t result;
	dns_rdata_t rdata = DNS_RDATA_INIT;

	REQUIRE(dns_name_countlabels(target) == 0);

	if (rdataset->type == dns_rdatatype_cname) {
		dns_rdata_cname_t cname;

		/*
		 * Copy the CNAME's target into the target name.
		 */
		result = dns_rdataset_first(rdataset);
		if (result != ISC_R_SUCCESS) {
			return result;
		}
		dns_rdataset_current(rdataset, &rdata);
		result = dns_rdata_tostruct(&rdata, &cname, NULL);
		if (result != ISC_R_SUCCESS) {
			return result;
		}
		dns_name_dup(&cname.cname, adb->mctx, target);
		dns_rdata_freestruct(&cname);
	} else {
		dns_fixedname_t fixed1, fixed2;
		dns_name_t *prefix = NULL, *new_target = NULL;
		dns_rdata_dname_t dname;
		dns_namereln_t namereln;
		unsigned int nlabels;
		int order;

		INSIST(rdataset->type == dns_rdatatype_dname);
		namereln = dns_name_fullcompare(name, fname, &order, &nlabels);
		INSIST(namereln == dns_namereln_subdomain);

		/*
		 * Get the target name of the DNAME.
		 */
		result = dns_rdataset_first(rdataset);
		if (result != ISC_R_SUCCESS) {
			return result;
		}
		dns_rdataset_current(rdataset, &rdata);
		result = dns_rdata_tostruct(&rdata, &dname, NULL);
		if (result != ISC_R_SUCCESS) {
			return result;
		}

		/*
		 * Construct the new target name.
		 */
		prefix = dns_fixedname_initname(&fixed1);
		new_target = dns_fixedname_initname(&fixed2);
		dns_name_split(name, nlabels, prefix, NULL);
		result = dns_name_concatenate(prefix, &dname.dname, new_target,
					      NULL);
		dns_rdata_freestruct(&dname);
		if (result != ISC_R_SUCCESS) {
			return result;
		}
		dns_name_dup(new_target, adb->mctx, target);
	}

	return ISC_R_SUCCESS;
}

/*
 * The name must be locked.
 */
static void
clean_finds_at_name(dns_adbname_t *name, dns_adbstatus_t astat,
		    unsigned int addrs) {
	dns_adbfind_t *find = NULL, *next = NULL;

	DP(ENTER_LEVEL,
	   "ENTER clean_finds_at_name, name %p, astat %08x, addrs %08x", name,
	   astat, addrs);

	for (find = ISC_LIST_HEAD(name->finds); find != NULL; find = next) {
		bool process = false;
		unsigned int wanted, notify;

		LOCK(&find->lock);
		next = ISC_LIST_NEXT(find, plink);

		wanted = find->flags & DNS_ADBFIND_ADDRESSMASK;
		notify = wanted & addrs;

		switch (astat) {
		case DNS_ADB_MOREADDRESSES:
			DP(ISC_LOG_DEBUG(3), "more addresses");
			if ((notify) != 0) {
				find->flags &= ~addrs;
				process = true;
			}
			break;
		case DNS_ADB_NOMOREADDRESSES:
			DP(ISC_LOG_DEBUG(3), "no more addresses");
			find->flags &= ~addrs;
			wanted = find->flags & DNS_ADBFIND_ADDRESSMASK;
			if (wanted == 0) {
				process = true;
			}
			break;
		default:
			find->flags &= ~addrs;
			process = true;
		}

		if (process) {
			DP(DEF_LEVEL, "cfan: processing find %p", find);

			/*
			 * Unlink the find from the name, letting the caller
			 * call dns_adb_destroyfind() on it to clean it up
			 * later.
			 */
			ISC_LIST_UNLINK(name->finds, find, plink);
			find->adbname = NULL;

			INSIST(!FIND_EVENTSENT(find));

			atomic_store(&find->status, astat);

			DP(DEF_LEVEL, "cfan: sending find %p to caller", find);

			isc_async_run(find->loop, find->cb, find);
			find->flags |= FIND_EVENT_SENT;
		} else {
			DP(DEF_LEVEL, "cfan: skipping find %p", find);
		}

		UNLOCK(&find->lock);
	}
	DP(ENTER_LEVEL, "EXIT clean_finds_at_name, name %p", name);
}

static dns_adbname_t *
new_adbname(dns_adb_t *adb, const dns_name_t *dnsname, unsigned int flags) {
	dns_adbname_t *name = NULL;

	name = isc_mem_get(adb->mctx, sizeof(*name));
	*name = (dns_adbname_t){
		.adb = dns_adb_ref(adb),
		.expire_v4 = INT_MAX,
		.expire_v6 = INT_MAX,
		.expire_target = INT_MAX,
		.fetch_err = FIND_ERR_UNEXPECTED,
		.fetch6_err = FIND_ERR_UNEXPECTED,
		.v4 = ISC_LIST_INITIALIZER,
		.v6 = ISC_LIST_INITIALIZER,
		.finds = ISC_LIST_INITIALIZER,
		.link = ISC_LINK_INITIALIZER,
		.flags = flags & ADBNAME_FLAGS_MASK,
		.magic = DNS_ADBNAME_MAGIC,
	};

#if DNS_ADB_TRACE
	fprintf(stderr, "dns_adbname__init:%s:%s:%d:%p->references = 1\n",
		__func__, __FILE__, __LINE__ + 1, name);
#endif
	isc_refcount_init(&name->references, 1);

	isc_mutex_init(&name->lock);

	name->name = dns_fixedname_initname(&name->fname);
	dns_name_copy(dnsname, name->name);
	dns_name_init(&name->target, NULL);

	inc_adbstats(adb, dns_adbstats_namescnt);
	return name;
}

#if DNS_ADB_TRACE
ISC_REFCOUNT_TRACE_IMPL(dns_adbname, destroy_adbname);
#else
ISC_REFCOUNT_IMPL(dns_adbname, destroy_adbname);
#endif

static void
destroy_adbname(dns_adbname_t *name) {
	REQUIRE(DNS_ADBNAME_VALID(name));

	dns_adb_t *adb = name->adb;

	REQUIRE(!NAME_HAS_V4(name));
	REQUIRE(!NAME_HAS_V6(name));
	REQUIRE(!NAME_FETCH(name));
	REQUIRE(ISC_LIST_EMPTY(name->finds));
	REQUIRE(!ISC_LINK_LINKED(name, link));

	name->magic = 0;

	isc_mutex_destroy(&name->lock);

	isc_mem_put(adb->mctx, name, sizeof(*name));

	dec_adbstats(adb, dns_adbstats_namescnt);
	dns_adb_detach(&adb);
}

static dns_adbnamehook_t *
new_adbnamehook(dns_adb_t *adb) {
	dns_adbnamehook_t *nh = isc_mem_get(adb->mctx, sizeof(*nh));
	*nh = (dns_adbnamehook_t){
		.name_link = ISC_LINK_INITIALIZER,
		.entry_link = ISC_LINK_INITIALIZER,
		.magic = DNS_ADBNAMEHOOK_MAGIC,
	};

	return nh;
}

static void
free_adbnamehook(dns_adb_t *adb, dns_adbnamehook_t **namehook) {
	dns_adbnamehook_t *nh = NULL;

	REQUIRE(namehook != NULL && DNS_ADBNAMEHOOK_VALID(*namehook));

	nh = *namehook;
	*namehook = NULL;

	REQUIRE(nh->entry == NULL);
	REQUIRE(!ISC_LINK_LINKED(nh, name_link));
	REQUIRE(!ISC_LINK_LINKED(nh, entry_link));

	nh->magic = 0;

	isc_mem_put(adb->mctx, nh, sizeof(*nh));
}

static dns_adbentry_t *
new_adbentry(dns_adb_t *adb, const isc_sockaddr_t *addr, isc_stdtime_t now) {
	dns_adbentry_t *entry = NULL;

	entry = isc_mem_get(adb->mctx, sizeof(*entry));
	*entry = (dns_adbentry_t){
		.srtt = isc_random_uniform(0x1f) + 1,
		.sockaddr = *addr,
		.link = ISC_LINK_INITIALIZER,
		.quota = adb->quota,
		.references = ISC_REFCOUNT_INITIALIZER(1),
		.adb = dns_adb_ref(adb),
		.expires = now + ADB_ENTRY_WINDOW,
		.magic = DNS_ADBENTRY_MAGIC,
	};

#if DNS_ADB_TRACE
	fprintf(stderr, "dns_adbentry__init:%s:%s:%d:%p->references = 1\n",
		__func__, __FILE__, __LINE__ + 1, entry);
#endif
	isc_mutex_init(&entry->lock);

	inc_adbstats(adb, dns_adbstats_entriescnt);

	return entry;
}

static void
destroy_adbentry(dns_adbentry_t *entry) {
	REQUIRE(DNS_ADBENTRY_VALID(entry));

	dns_adb_t *adb = entry->adb;
	uint_fast32_t active;

	entry->magic = 0;

	INSIST(!ISC_LINK_LINKED(entry, link));

	INSIST(ISC_LIST_EMPTY(entry->nhs));

	active = atomic_load_acquire(&entry->active);
	INSIST(active == 0);

	if (entry->cookie != NULL) {
		isc_mem_put(adb->mctx, entry->cookie, entry->cookielen);
	}

	isc_mutex_destroy(&entry->lock);
	isc_mem_put(adb->mctx, entry, sizeof(*entry));

	dec_adbstats(adb, dns_adbstats_entriescnt);

	dns_adb_detach(&adb);
}

#if DNS_ADB_TRACE
ISC_REFCOUNT_TRACE_IMPL(dns_adbentry, destroy_adbentry);
#else
ISC_REFCOUNT_IMPL(dns_adbentry, destroy_adbentry);
#endif

static dns_adbfind_t *
new_adbfind(dns_adb_t *adb, in_port_t port) {
	dns_adbfind_t *find = NULL;

	find = isc_mem_get(adb->mctx, sizeof(*find));
	*find = (dns_adbfind_t){
		.port = port,
		.result_v4 = ISC_R_UNEXPECTED,
		.result_v6 = ISC_R_UNEXPECTED,
		.publink = ISC_LINK_INITIALIZER,
		.plink = ISC_LINK_INITIALIZER,
		.list = ISC_LIST_INITIALIZER,
	};

	dns_adb_attach(adb, &find->adb);
	isc_mutex_init(&find->lock);

	find->magic = DNS_ADBFIND_MAGIC;

	return find;
}

static void
free_adbfind(dns_adbfind_t **findp) {
	dns_adb_t *adb = NULL;
	dns_adbfind_t *find = NULL;

	REQUIRE(findp != NULL && DNS_ADBFIND_VALID(*findp));

	find = *findp;
	*findp = NULL;

	adb = find->adb;

	REQUIRE(!FIND_HAS_ADDRS(find));
	REQUIRE(!ISC_LINK_LINKED(find, publink));
	REQUIRE(!ISC_LINK_LINKED(find, plink));
	REQUIRE(find->adbname == NULL);

	find->magic = 0;

	isc_mutex_destroy(&find->lock);

	isc_mem_put(adb->mctx, find, sizeof(*find));
	dns_adb_detach(&adb);
}

static dns_adbfetch_t *
new_adbfetch(dns_adb_t *adb) {
	dns_adbfetch_t *fetch = NULL;

	fetch = isc_mem_get(adb->mctx, sizeof(*fetch));
	*fetch = (dns_adbfetch_t){ 0 };
	dns_rdataset_init(&fetch->rdataset);

	fetch->magic = DNS_ADBFETCH_MAGIC;

	return fetch;
}

static void
free_adbfetch(dns_adb_t *adb, dns_adbfetch_t **fetchp) {
	dns_adbfetch_t *fetch = NULL;

	REQUIRE(fetchp != NULL && DNS_ADBFETCH_VALID(*fetchp));

	fetch = *fetchp;
	*fetchp = NULL;

	fetch->magic = 0;

	if (dns_rdataset_isassociated(&fetch->rdataset)) {
		dns_rdataset_disassociate(&fetch->rdataset);
	}

	isc_mem_put(adb->mctx, fetch, sizeof(*fetch));
}

/*
 * Copy bits from an adbentry into a newly allocated adb_addrinfo structure.
 * The entry must be locked, and its reference count must be incremented.
 */
static dns_adbaddrinfo_t *
new_adbaddrinfo(dns_adb_t *adb, dns_adbentry_t *entry, in_port_t port) {
	dns_adbaddrinfo_t *ai = NULL;

	ai = isc_mem_get(adb->mctx, sizeof(*ai));
	*ai = (dns_adbaddrinfo_t){
		.srtt = atomic_load(&entry->srtt),
		.flags = atomic_load(&entry->flags),
		.publink = ISC_LINK_INITIALIZER,
		.sockaddr = entry->sockaddr,
		.entry = dns_adbentry_ref(entry),
		.magic = DNS_ADBADDRINFO_MAGIC,
	};

	isc_sockaddr_setport(&ai->sockaddr, port);

	return ai;
}

static void
free_adbaddrinfo(dns_adb_t *adb, dns_adbaddrinfo_t **ainfo) {
	dns_adbaddrinfo_t *ai = NULL;

	REQUIRE(ainfo != NULL && DNS_ADBADDRINFO_VALID(*ainfo));

	ai = *ainfo;
	*ainfo = NULL;

	REQUIRE(!ISC_LINK_LINKED(ai, publink));

	ai->magic = 0;

	if (ai->transport != NULL) {
		dns_transport_detach(&ai->transport);
	}
	dns_adbentry_detach(&ai->entry);

	isc_mem_put(adb->mctx, ai, sizeof(*ai));
}

static bool
match_adbname(void *node, const void *key) {
	const dns_adbname_t *adbname0 = node;
	const dns_adbname_t *adbname1 = key;

	if ((adbname0->flags & ADBNAME_FLAGS_MASK) !=
	    (adbname1->flags & ADBNAME_FLAGS_MASK))
	{
		return false;
	}

	return dns_name_equal(adbname0->name, adbname1->name);
}

static uint32_t
hash_adbname(const dns_adbname_t *adbname) {
	isc_hash32_t hash;
	unsigned int flags = adbname->flags & ADBNAME_FLAGS_MASK;

	isc_hash32_init(&hash);
	isc_hash32_hash(&hash, adbname->name->ndata, adbname->name->length,
			false);
	isc_hash32_hash(&hash, &flags, sizeof(flags), true);
	return isc_hash32_finalize(&hash);
}

/*
 * Search for the name in the hash table.
 */
static dns_adbname_t *
get_attached_and_locked_name(dns_adb_t *adb, const dns_name_t *name,
			     unsigned int flags, isc_stdtime_t now) {
	isc_result_t result;
	dns_adbname_t *adbname = NULL;
	isc_time_t timenow;
	isc_stdtime_t last_update;
	dns_adbname_t key = {
		.name = UNCONST(name),
		.flags = flags & ADBNAME_FLAGS_MASK,
	};
	uint32_t hashval = hash_adbname(&key);
	isc_rwlocktype_t locktype = isc_rwlocktype_read;

	isc_time_set(&timenow, now, 0);

	RWLOCK(&adb->names_lock, locktype);
	last_update = adb->names_last_update;

	if (last_update + ADB_STALE_MARGIN >= now ||
	    isc_mem_isovermem(adb->mctx))
	{
		last_update = now;
		UPGRADELOCK(&adb->names_lock, locktype);
		purge_stale_names(adb, now);
		adb->names_last_update = last_update;
	}

	result = isc_hashmap_find(adb->names, hashval, match_adbname,
				  (void *)&key, (void **)&adbname);
	switch (result) {
	case ISC_R_NOTFOUND:
		UPGRADELOCK(&adb->names_lock, locktype);

		/* Allocate a new name and add it to the hash table. */
		adbname = new_adbname(adb, name, key.flags);

		void *found = NULL;
		result = isc_hashmap_add(adb->names, hashval, match_adbname,
					 (void *)&key, adbname, &found);
		if (result == ISC_R_EXISTS) {
			destroy_adbname(adbname);
			adbname = found;
			result = ISC_R_SUCCESS;
			ISC_LIST_UNLINK(adb->names_lru, adbname, link);
		}
		INSIST(result == ISC_R_SUCCESS);

		break;
	case ISC_R_SUCCESS:
		if (locktype == isc_rwlocktype_write) {
			ISC_LIST_UNLINK(adb->names_lru, adbname, link);
		}
		break;
	default:
		UNREACHABLE();
	}

	dns_adbname_ref(adbname);

	LOCK(&adbname->lock); /* Must be unlocked by the caller */
	if (adbname->last_used + ADB_CACHE_MINIMUM <= last_update) {
		adbname->last_used = now;
	}
	if (locktype == isc_rwlocktype_write) {
		ISC_LIST_PREPEND(adb->names_lru, adbname, link);
	}

	/*
	 * The refcount is now 2 and the final detach will happen in
	 * expire_name() - the unused adbname stored in the hashtable and lru
	 * has always refcount == 1
	 */
	RWUNLOCK(&adb->names_lock, locktype);

	return adbname;
}

static void
upgrade_entries_lock(dns_adb_t *adb, isc_rwlocktype_t *locktypep,
		     isc_stdtime_t now) {
	if (*locktypep == isc_rwlocktype_read) {
		UPGRADELOCK(&adb->entries_lock, *locktypep);
		purge_stale_entries(adb, now);
		adb->entries_last_update = now;
	}
}

static bool
match_adbentry(void *node, const void *key) {
	dns_adbentry_t *adbentry = node;

	return isc_sockaddr_equal(&adbentry->sockaddr, key);
}

/*
 * Find the entry in the adb->entries hashtable.
 */
static dns_adbentry_t *
get_attached_and_locked_entry(dns_adb_t *adb, isc_stdtime_t now,
			      const isc_sockaddr_t *addr) {
	isc_result_t result;
	dns_adbentry_t *adbentry = NULL;
	isc_time_t timenow;
	isc_stdtime_t last_update;
	uint32_t hashval = isc_sockaddr_hash(addr, true);
	isc_rwlocktype_t locktype = isc_rwlocktype_read;

	isc_time_set(&timenow, now, 0);

	RWLOCK(&adb->entries_lock, locktype);
	last_update = adb->entries_last_update;

	if (now - last_update > ADB_STALE_MARGIN ||
	    isc_mem_isovermem(adb->mctx))
	{
		last_update = now;

		upgrade_entries_lock(adb, &locktype, now);
	}

	result = isc_hashmap_find(adb->entries, hashval, match_adbentry,
				  (const unsigned char *)addr,
				  (void **)&adbentry);
	if (result == ISC_R_NOTFOUND) {
		upgrade_entries_lock(adb, &locktype, now);

	create:
		INSIST(locktype == isc_rwlocktype_write);

		/* Allocate a new entry and add it to the hash table. */
		adbentry = new_adbentry(adb, addr, now);

		void *found = NULL;
		result = isc_hashmap_add(adb->entries, hashval, match_adbentry,
					 &adbentry->sockaddr, adbentry, &found);
		if (result == ISC_R_SUCCESS) {
			ISC_LIST_PREPEND(adb->entries_lru, adbentry, link);
		} else if (result == ISC_R_EXISTS) {
			dns_adbentry_detach(&adbentry);
			adbentry = found;
			result = ISC_R_SUCCESS;
		}
	}
	INSIST(result == ISC_R_SUCCESS);

	/*
	 * The dns_adbentry_ref() must stay here before trying to expire
	 * the ADB entry, so it is not destroyed under the lock.
	 */
	dns_adbentry_ref(adbentry);
	LOCK(&adbentry->lock); /* Must be unlocked by the caller */
	switch (locktype) {
	case isc_rwlocktype_read:
		if (!entry_expired(adbentry, now)) {
			break;
		}

		/* We need to upgrade the LRU lock */
		UNLOCK(&adbentry->lock);
		upgrade_entries_lock(adb, &locktype, now);
		LOCK(&adbentry->lock);
		FALLTHROUGH;
	case isc_rwlocktype_write:
		if (ENTRY_DEAD(adbentry) || maybe_expire_entry(adbentry, now)) {
			UNLOCK(&adbentry->lock);
			dns_adbentry_detach(&adbentry);
			goto create;
		}
		break;
	default:
		UNREACHABLE();
	}

	/* Did enough time pass to update the LRU? */
	if (adbentry->last_used + ADB_CACHE_MINIMUM <= last_update) {
		adbentry->last_used = now;
		if (locktype == isc_rwlocktype_write) {
			ISC_LIST_UNLINK(adb->entries_lru, adbentry, link);
			ISC_LIST_PREPEND(adb->entries_lru, adbentry, link);
		}
	}

	RWUNLOCK(&adb->entries_lock, locktype);

	return adbentry;
}

static void
log_quota(dns_adbentry_t *entry, const char *fmt, ...) {
	va_list ap;
	char msgbuf[2048];
	char addrbuf[ISC_NETADDR_FORMATSIZE];
	isc_netaddr_t netaddr;

	va_start(ap, fmt);
	vsnprintf(msgbuf, sizeof(msgbuf), fmt, ap);
	va_end(ap);

	isc_netaddr_fromsockaddr(&netaddr, &entry->sockaddr);
	isc_netaddr_format(&netaddr, addrbuf, sizeof(addrbuf));

	isc_log_write(dns_lctx, DNS_LOGCATEGORY_DATABASE, DNS_LOGMODULE_ADB,
		      ISC_LOG_INFO,
		      "adb: quota %s (%" PRIuFAST32 "/%" PRIuFAST32 "): %s",
		      addrbuf, atomic_load_relaxed(&entry->active),
		      atomic_load_relaxed(&entry->quota), msgbuf);
}

static void
copy_namehook_lists(dns_adb_t *adb, dns_adbfind_t *find, dns_adbname_t *name) {
	dns_adbnamehook_t *namehook = NULL;
	dns_adbentry_t *entry = NULL;

	if ((find->options & DNS_ADBFIND_INET) != 0) {
		namehook = ISC_LIST_HEAD(name->v4);
		while (namehook != NULL) {
			dns_adbaddrinfo_t *addrinfo = NULL;
			entry = namehook->entry;

			if ((find->options & DNS_ADBFIND_QUOTAEXEMPT) == 0 &&
			    adbentry_overquota(entry))
			{
				find->options |= DNS_ADBFIND_OVERQUOTA;
				goto nextv4;
			}

			addrinfo = new_adbaddrinfo(adb, entry, find->port);

			/*
			 * Found a valid entry.  Add it to the find's list.
			 */
			ISC_LIST_APPEND(find->list, addrinfo, publink);
		nextv4:
			namehook = ISC_LIST_NEXT(namehook, name_link);
		}
	}

	if ((find->options & DNS_ADBFIND_INET6) != 0) {
		namehook = ISC_LIST_HEAD(name->v6);
		while (namehook != NULL) {
			dns_adbaddrinfo_t *addrinfo = NULL;
			entry = namehook->entry;

			if ((find->options & DNS_ADBFIND_QUOTAEXEMPT) == 0 &&
			    adbentry_overquota(entry))
			{
				find->options |= DNS_ADBFIND_OVERQUOTA;
				goto nextv6;
			}

			addrinfo = new_adbaddrinfo(adb, entry, find->port);

			/*
			 * Found a valid entry.  Add it to the find's list.
			 */
			ISC_LIST_APPEND(find->list, addrinfo, publink);
		nextv6:
			namehook = ISC_LIST_NEXT(namehook, name_link);
		}
	}
}

/*
 * The name must be locked and write lock on adb->names_lock must be held.
 */
static bool
maybe_expire_name(dns_adbname_t *adbname, isc_stdtime_t now) {
	REQUIRE(DNS_ADBNAME_VALID(adbname));

	/* Leave this name alone if it still has active namehooks... */
	if (NAME_HAS_V4(adbname) || NAME_HAS_V6(adbname)) {
		return false;
	}

	/* ...an active fetch in progres... */
	if (NAME_FETCH(adbname)) {
		return false;
	}

	/* ... or is not yet expired. */
	if (!EXPIRE_OK(adbname->expire_v4, now) ||
	    !EXPIRE_OK(adbname->expire_v6, now) ||
	    !EXPIRE_OK(adbname->expire_target, now))
	{
		return false;
	}

	expire_name(adbname, DNS_ADB_EXPIRED);

	return true;
}

static void
expire_entry(dns_adbentry_t *adbentry) {
	isc_result_t result;
	dns_adb_t *adb = adbentry->adb;

	if (!ENTRY_DEAD(adbentry)) {
		(void)atomic_fetch_or(&adbentry->flags, ENTRY_IS_DEAD);

		result = isc_hashmap_delete(
			adb->entries,
			isc_sockaddr_hash(&adbentry->sockaddr, true), match_ptr,
			adbentry);
		RUNTIME_CHECK(result == ISC_R_SUCCESS);
		ISC_LIST_UNLINK(adb->entries_lru, adbentry, link);
	}

	dns_adbentry_detach(&adbentry);
}

static bool
entry_expired(dns_adbentry_t *adbentry, isc_stdtime_t now) {
	if (!ISC_LIST_EMPTY(adbentry->nhs)) {
		return false;
	}

	if (!EXPIRE_OK(adbentry->expires, now)) {
		return false;
	}

	return true;
}

static bool
maybe_expire_entry(dns_adbentry_t *adbentry, isc_stdtime_t now) {
	REQUIRE(DNS_ADBENTRY_VALID(adbentry));

	if (entry_expired(adbentry, now)) {
		expire_entry(adbentry);
		return true;
	}

	return false;
}

/*%
 * Examine the tail entry of the LRU list to see if it expires or is stale
 * (unused for some period); if so, the name entry will be freed.  If the ADB
 * is in the overmem condition, the tail and the next to tail entries
 * will be unconditionally removed (unless they have an outstanding fetch).
 * We don't care about a race on 'overmem' at the risk of causing some
 * collateral damage or a small delay in starting cleanup.
 *
 * adb->names_lock MUST be write locked
 */
static void
purge_stale_names(dns_adb_t *adb, isc_stdtime_t now) {
	bool overmem = isc_mem_isovermem(adb->mctx);
	int max_removed = overmem ? 2 : 1;
	int scans = 0, removed = 0;
	dns_adbname_t *prev = NULL;

	/*
	 * We limit the number of scanned entries to 10 (arbitrary choice)
	 * in order to avoid examining too many entries when there are many
	 * tail entries that have fetches (this should be rare, but could
	 * happen).
	 */

	for (dns_adbname_t *adbname = ISC_LIST_TAIL(adb->names_lru);
	     adbname != NULL && removed < max_removed && scans < 10;
	     adbname = prev)
	{
		prev = ISC_LIST_PREV(adbname, link);

		dns_adbname_ref(adbname);
		LOCK(&adbname->lock);

		scans++;

		/*
		 * Remove the name if it's expired or unused,
		 * has no address data.
		 */
		maybe_expire_namehooks(adbname, now);
		if (maybe_expire_name(adbname, now)) {
			removed++;
			goto next;
		}

		/*
		 * Make sure that we are not purging ADB names that has been
		 * just created.
		 */
		if (adbname->last_used + ADB_CACHE_MINIMUM >= now) {
			prev = NULL;
			goto next;
		}

		if (overmem) {
			expire_name(adbname, DNS_ADB_CANCELED);
			removed++;
			goto next;
		}

		if (adbname->last_used + ADB_STALE_MARGIN < now) {
			expire_name(adbname, DNS_ADB_CANCELED);
			removed++;
			goto next;
		}

		/*
		 * We won't expire anything on the LRU list as the
		 * .last_used + ADB_STALE_MARGIN will always be bigger
		 * than `now` for all previous entries, so we just stop
		 * the scanning.
		 */
		prev = NULL;
	next:
		UNLOCK(&adbname->lock);
		dns_adbname_detach(&adbname);
	}
}

static void
cleanup_names(dns_adb_t *adb, isc_stdtime_t now) {
	dns_adbname_t *next = NULL;

	RWLOCK(&adb->names_lock, isc_rwlocktype_write);
	for (dns_adbname_t *adbname = ISC_LIST_HEAD(adb->names_lru);
	     adbname != NULL; adbname = next)
	{
		next = ISC_LIST_NEXT(adbname, link);

		dns_adbname_ref(adbname);
		LOCK(&adbname->lock);
		/*
		 * Name hooks expire after the address record's TTL
		 * or 30 minutes, whichever is shorter. If after cleaning
		 * those up there are no name hooks left, and no active
		 * fetches, we can remove this name from the bucket.
		 */
		maybe_expire_namehooks(adbname, now);
		(void)maybe_expire_name(adbname, now);
		UNLOCK(&adbname->lock);
		dns_adbname_detach(&adbname);
	}
	RWUNLOCK(&adb->names_lock, isc_rwlocktype_write);
}

/*%
 * Examine the tail entry of the LRU list to see if it expires or is stale
 * (unused for some period); if so, the name entry will be freed.  If the ADB
 * is in the overmem condition, the tail and the next to tail entries
 * will be unconditionally removed (unless they have an outstanding fetch).
 * We don't care about a race on 'overmem' at the risk of causing some
 * collateral damage or a small delay in starting cleanup.
 *
 * adb->entries_lock MUST be write locked
 */
static void
purge_stale_entries(dns_adb_t *adb, isc_stdtime_t now) {
	bool overmem = isc_mem_isovermem(adb->mctx);
	int max_removed = overmem ? 2 : 1;
	int scans = 0, removed = 0;
	dns_adbentry_t *prev = NULL;

	/*
	 * We limit the number of scanned entries to 10 (arbitrary choice)
	 * in order to avoid examining too many entries when there are many
	 * tail entries that have fetches (this should be rare, but could
	 * happen).
	 */

	for (dns_adbentry_t *adbentry = ISC_LIST_TAIL(adb->entries_lru);
	     adbentry != NULL && removed < max_removed && scans < 10;
	     adbentry = prev)
	{
		prev = ISC_LIST_PREV(adbentry, link);

		dns_adbentry_ref(adbentry);
		LOCK(&adbentry->lock);

		scans++;

		/*
		 * Remove the entry if it's expired and unused.
		 */
		if (maybe_expire_entry(adbentry, now)) {
			removed++;
			goto next;
		}

		/*
		 * Make sure that we are not purging ADB entry that has been
		 * just created.
		 */
		if (adbentry->last_used + ADB_CACHE_MINIMUM >= now) {
			prev = NULL;
			goto next;
		}

		if (overmem) {
			maybe_expire_entry(adbentry, INT_MAX);
			removed++;
			goto next;
		}

		if (adbentry->last_used + ADB_STALE_MARGIN < now) {
			maybe_expire_entry(adbentry, INT_MAX);
			removed++;
			goto next;
		}

		/*
		 * We won't expire anything on the LRU list as the
		 * .last_used + ADB_STALE_MARGIN will always be bigger
		 * than `now` for all previous entries, so we just stop
		 * the scanning
		 */
		prev = NULL;
	next:
		UNLOCK(&adbentry->lock);
		dns_adbentry_detach(&adbentry);
	}
}

static void
cleanup_entries(dns_adb_t *adb, isc_stdtime_t now) {
	dns_adbentry_t *next = NULL;

	RWLOCK(&adb->entries_lock, isc_rwlocktype_write);
	for (dns_adbentry_t *adbentry = ISC_LIST_HEAD(adb->entries_lru);
	     adbentry != NULL; adbentry = next)
	{
		next = ISC_LIST_NEXT(adbentry, link);

		dns_adbentry_ref(adbentry);
		LOCK(&adbentry->lock);
		maybe_expire_entry(adbentry, now);
		UNLOCK(&adbentry->lock);
		dns_adbentry_detach(&adbentry);
	}
	RWUNLOCK(&adb->entries_lock, isc_rwlocktype_write);
}

static void
destroy(dns_adb_t *adb) {
	DP(DEF_LEVEL, "destroying ADB %p", adb);

	adb->magic = 0;

	RWLOCK(&adb->names_lock, isc_rwlocktype_write);
	INSIST(isc_hashmap_count(adb->names) == 0);
	isc_hashmap_destroy(&adb->names);
	RWUNLOCK(&adb->names_lock, isc_rwlocktype_write);
	isc_rwlock_destroy(&adb->names_lock);

	RWLOCK(&adb->entries_lock, isc_rwlocktype_write);
	/* There are no unassociated entries */
	INSIST(isc_hashmap_count(adb->entries) == 0);
	isc_hashmap_destroy(&adb->entries);
	RWUNLOCK(&adb->entries_lock, isc_rwlocktype_write);
	isc_rwlock_destroy(&adb->entries_lock);

	isc_mem_detach(&adb->hmctx);

	isc_mutex_destroy(&adb->lock);

	isc_stats_detach(&adb->stats);
	dns_resolver_detach(&adb->res);
	dns_view_weakdetach(&adb->view);
	isc_mem_putanddetach(&adb->mctx, adb, sizeof(dns_adb_t));
}

#if DNS_ADB_TRACE
ISC_REFCOUNT_TRACE_IMPL(dns_adb, destroy);
#else
ISC_REFCOUNT_IMPL(dns_adb, destroy);
#endif

/*
 * Public functions.
 */

void
dns_adb_create(isc_mem_t *mem, dns_view_t *view, dns_adb_t **newadb) {
	dns_adb_t *adb = NULL;

	REQUIRE(mem != NULL);
	REQUIRE(view != NULL);
	REQUIRE(newadb != NULL && *newadb == NULL);

	adb = isc_mem_get(mem, sizeof(dns_adb_t));
	*adb = (dns_adb_t){
		.names_lru = ISC_LIST_INITIALIZER,
		.entries_lru = ISC_LIST_INITIALIZER,
	};

	/*
	 * Initialize things here that cannot fail, and especially things
	 * that must be NULL for the error return to work properly.
	 */
#if DNS_ADB_TRACE
	fprintf(stderr, "dns_adb__init:%s:%s:%d:%p->references = 1\n", __func__,
		__FILE__, __LINE__ + 1, adb);
#endif
	isc_refcount_init(&adb->references, 1);
	dns_view_weakattach(view, &adb->view);
	dns_resolver_attach(view->resolver, &adb->res);
	isc_mem_attach(mem, &adb->mctx);

	isc_mem_create(&adb->hmctx);
	isc_mem_setname(adb->hmctx, "ADB_hashmaps");

	isc_hashmap_create(adb->hmctx, ADB_HASH_BITS, &adb->names);
	isc_rwlock_init(&adb->names_lock);

	isc_hashmap_create(adb->hmctx, ADB_HASH_BITS, &adb->entries);
	isc_rwlock_init(&adb->entries_lock);

	isc_mutex_init(&adb->lock);

	isc_stats_create(adb->mctx, &adb->stats, dns_adbstats_max);

	set_adbstat(adb, 0, dns_adbstats_nnames);
	set_adbstat(adb, 0, dns_adbstats_nentries);

	/*
	 * Normal return.
	 */
	adb->magic = DNS_ADB_MAGIC;
	*newadb = adb;
}

void
dns_adb_shutdown(dns_adb_t *adb) {
	if (!atomic_compare_exchange_strong(&adb->exiting, &(bool){ false },
					    true))
	{
		return;
	}

	DP(DEF_LEVEL, "shutting down ADB %p", adb);

	isc_mem_clearwater(adb->mctx);

	shutdown_names(adb);
	shutdown_entries(adb);
}

/*
 * Look up the name in our internal database.
 *
 * There are three possibilities. Note that these are not always exclusive.
 *
 * - No name found.  In this case, allocate a new name header and
 *   an initial namehook or two.
 *
 * - Name found, valid addresses present.  Allocate one addrinfo
 *   structure for each found and append it to the linked list
 *   of addresses for this header.
 *
 * - Name found, queries pending.  In this case, if a loop was
 *   passed in, allocate a job id, attach it to the name's job
 *   list and remember to tell the caller that there will be
 *   more info coming later.
 */
isc_result_t
dns_adb_createfind(dns_adb_t *adb, isc_loop_t *loop, isc_job_cb cb, void *cbarg,
		   const dns_name_t *name, const dns_name_t *qname,
		   dns_rdatatype_t qtype ISC_ATTR_UNUSED, unsigned int options,
		   isc_stdtime_t now, dns_name_t *target, in_port_t port,
		   unsigned int depth, isc_counter_t *qc,
		   dns_adbfind_t **findp) {
	isc_result_t result = ISC_R_UNEXPECTED;
	dns_adbfind_t *find = NULL;
	dns_adbname_t *adbname = NULL;
	bool want_event = true;
	bool start_at_zone = false;
	bool alias = false;
	bool have_address = false;
	unsigned int wanted_addresses = (options & DNS_ADBFIND_ADDRESSMASK);
	unsigned int wanted_fetches = 0;
	unsigned int query_pending = 0;
	char namebuf[DNS_NAME_FORMATSIZE] = { 0 };

	REQUIRE(DNS_ADB_VALID(adb));
	if (loop != NULL) {
		REQUIRE(cb != NULL);
	}
	REQUIRE(name != NULL);
	REQUIRE(qname != NULL);
	REQUIRE(findp != NULL && *findp == NULL);
	REQUIRE(target == NULL || dns_name_hasbuffer(target));

	REQUIRE((options & DNS_ADBFIND_ADDRESSMASK) != 0);

	if (atomic_load(&adb->exiting)) {
		DP(DEF_LEVEL, "dns_adb_createfind: returning "
			      "ISC_R_SHUTTINGDOWN");

		return ISC_R_SHUTTINGDOWN;
	}

	if (now == 0) {
		now = isc_stdtime_now();
	}

	/*
	 * If STATICSTUB is set we always want to have STARTATZONE set.
	 */
	if (options & DNS_ADBFIND_STATICSTUB) {
		options |= DNS_ADBFIND_STARTATZONE;
	}

	/*
	 * Remember what types of addresses we are interested in.
	 */
	find = new_adbfind(adb, port);
	find->options = options;
	find->flags |= wanted_addresses;
	if (FIND_WANTEVENT(find)) {
		REQUIRE(loop != NULL);
	}

	if (isc_log_wouldlog(dns_lctx, DEF_LEVEL)) {
		dns_name_format(name, namebuf, sizeof(namebuf));
	}

again:
	/* Try to see if we know anything about this name at all. */
	adbname = get_attached_and_locked_name(adb, name, find->options, now);

	if (NAME_DEAD(adbname)) {
		UNLOCK(&adbname->lock);
		dns_adbname_detach(&adbname);
		goto again;
	}

	/*
	 * Name hooks expire after the address record's TTL or 30 minutes,
	 * whichever is shorter. If there are expired name hooks, remove
	 * them so we'll send a new fetch.
	 */
	maybe_expire_namehooks(adbname, now);

	/*
	 * Do we know that the name is an alias?
	 */
	if (!EXPIRE_OK(adbname->expire_target, now)) {
		/* Yes, it is. */
		DP(DEF_LEVEL,
		   "dns_adb_createfind: name %s (%p) is an alias (cached)",
		   namebuf, adbname);
		alias = true;
		goto post_copy;
	}

	/*
	 * Try to populate the name from the database and/or
	 * start fetches.  First try looking for an A record
	 * in the database.
	 */
	if (!NAME_HAS_V4(adbname) && EXPIRE_OK(adbname->expire_v4, now) &&
	    WANT_INET(wanted_addresses))
	{
		result = dbfind_name(adbname, now, dns_rdatatype_a);
		switch (result) {
		case ISC_R_SUCCESS:
			/* Found an A; now we proceed to check for AAAA */
			DP(DEF_LEVEL,
			   "dns_adb_createfind: found A for name %s (%p) in db",
			   namebuf, adbname);
			break;

		case DNS_R_ALIAS:
			/* Got a CNAME or DNAME. */
			DP(DEF_LEVEL,
			   "dns_adb_createfind: name %s (%p) is an alias",
			   namebuf, adbname);
			alias = true;
			goto post_copy;

		case DNS_R_NXDOMAIN:
		case DNS_R_NCACHENXDOMAIN:
			/*
			 * If the name doesn't exist at all, don't bother with
			 * v6 queries; they won't work.
			 */
			goto fetch;

		case DNS_R_NXRRSET:
		case DNS_R_NCACHENXRRSET:
		case DNS_R_HINTNXRRSET:
			/*
			 * The name does exist but we didn't get our data, go
			 * ahead and try AAAA.
			 */
			break;

		default:
			/*
			 * Any other result, start a fetch for A, then fall
			 * through to AAAA.
			 */
			if (!NAME_FETCH_A(adbname) && !FIND_STATICSTUB(find)) {
				wanted_fetches |= DNS_ADBFIND_INET;
			}
			break;
		}
	}

	/*
	 * Now look up or start fetches for AAAA.
	 */
	if (!NAME_HAS_V6(adbname) && EXPIRE_OK(adbname->expire_v6, now) &&
	    WANT_INET6(wanted_addresses))
	{
		result = dbfind_name(adbname, now, dns_rdatatype_aaaa);
		switch (result) {
		case ISC_R_SUCCESS:
			DP(DEF_LEVEL,
			   "dns_adb_createfind: found AAAA for name %s (%p)",
			   namebuf, adbname);
			break;

		case DNS_R_ALIAS:
			/* Got a CNAME or DNAME. */
			DP(DEF_LEVEL,
			   "dns_adb_createfind: name %s (%p) is an alias",
			   namebuf, adbname);
			alias = true;
			goto post_copy;

		case DNS_R_NXDOMAIN:
		case DNS_R_NCACHENXDOMAIN:
		case DNS_R_NXRRSET:
		case DNS_R_NCACHENXRRSET:
			/*
			 * Name doens't exist or was found in the negative
			 * cache to have no AAAA, don't bother fetching.
			 */
			break;

		default:
			/*
			 * Any other result, start a fetch for AAAA.
			 */
			if (!NAME_FETCH_AAAA(adbname) && !FIND_STATICSTUB(find))
			{
				wanted_fetches |= DNS_ADBFIND_INET6;
			}
			break;
		}
	}

fetch:
	if ((WANT_INET(wanted_addresses) && NAME_HAS_V4(adbname)) ||
	    (WANT_INET6(wanted_addresses) && NAME_HAS_V6(adbname)))
	{
		have_address = true;
	} else {
		have_address = false;
	}
	if (wanted_fetches != 0 && !(FIND_AVOIDFETCHES(find) && have_address) &&
	    !FIND_NOFETCH(find))
	{
		/*
		 * We're missing at least one address family.  Either the
		 * caller hasn't instructed us to avoid fetches, or we don't
		 * know anything about any of the address families that would
		 * be acceptable so we have to launch fetches.
		 */

		if (FIND_STARTATZONE(find)) {
			start_at_zone = true;
		}

		/*
		 * Start V4.
		 */
		if (WANT_INET(wanted_fetches) &&
		    fetch_name(adbname, start_at_zone, depth, qc,
			       dns_rdatatype_a) == ISC_R_SUCCESS)
		{
			DP(DEF_LEVEL,
			   "dns_adb_createfind: "
			   "started A fetch for name %s (%p)",
			   namebuf, adbname);
		}

		/*
		 * Start V6.
		 */
		if (WANT_INET6(wanted_fetches) &&
		    fetch_name(adbname, start_at_zone, depth, qc,
			       dns_rdatatype_aaaa) == ISC_R_SUCCESS)
		{
			DP(DEF_LEVEL,
			   "dns_adb_createfind: "
			   "started AAAA fetch for name %s (%p)",
			   namebuf, adbname);
		}
	}

	/*
	 * Run through the name and copy out the bits we are
	 * interested in.
	 */
	copy_namehook_lists(adb, find, adbname);

post_copy:
	if (NAME_FETCH_A(adbname)) {
		query_pending |= DNS_ADBFIND_INET;
	}
	if (NAME_FETCH_AAAA(adbname)) {
		query_pending |= DNS_ADBFIND_INET6;
	}

	/*
	 * Attach to the name's query list if there are queries
	 * already running, and we have been asked to.
	 */
	if (!FIND_WANTEVENT(find)) {
		want_event = false;
	}
	if (FIND_WANTEMPTYEVENT(find) && FIND_HAS_ADDRS(find)) {
		want_event = false;
	}
	if ((wanted_addresses & query_pending) == 0) {
		want_event = false;
	}
	if (alias) {
		want_event = false;
	}
	if (want_event) {
		bool empty;

		find->adbname = adbname;
		empty = ISC_LIST_EMPTY(adbname->finds);
		ISC_LIST_APPEND(adbname->finds, find, plink);
		find->query_pending = (query_pending & wanted_addresses);
		find->flags &= ~DNS_ADBFIND_ADDRESSMASK;
		find->flags |= (find->query_pending & DNS_ADBFIND_ADDRESSMASK);
		DP(DEF_LEVEL, "createfind: attaching find %p to adbname %p %d",
		   find, adbname, empty);
	} else {
		/*
		 * Remove the flag so the caller knows there will never
		 * be an event, and set internal flags to fake that
		 * the event was sent and freed, so dns_adb_destroyfind() will
		 * do the right thing.
		 */
		find->query_pending = (query_pending & wanted_addresses);
		find->options &= ~DNS_ADBFIND_WANTEVENT;
		find->flags |= FIND_EVENT_SENT;
		find->flags &= ~DNS_ADBFIND_ADDRESSMASK;
	}

	find->partial_result |= (adbname->partial_result & wanted_addresses);
	if (alias) {
		if (target != NULL) {
			dns_name_copy(&adbname->target, target);
		}
		result = DNS_R_ALIAS;
	} else {
		result = ISC_R_SUCCESS;
	}

	/*
	 * Copy out error flags from the name structure into the find.
	 */
	find->result_v4 = find_err_map[adbname->fetch_err];
	find->result_v6 = find_err_map[adbname->fetch6_err];

	if (want_event) {
		INSIST((find->flags & DNS_ADBFIND_ADDRESSMASK) != 0);
		find->loop = loop;
		atomic_store(&find->status, DNS_ADB_UNSET);
		find->cb = cb;
		find->cbarg = cbarg;
	}

	*findp = find;

	UNLOCK(&adbname->lock);
	dns_adbname_detach(&adbname);

	return result;
}

void
dns_adb_destroyfind(dns_adbfind_t **findp) {
	dns_adbfind_t *find = NULL;
	dns_adbaddrinfo_t *ai = NULL;
	dns_adb_t *adb = NULL;

	REQUIRE(findp != NULL && DNS_ADBFIND_VALID(*findp));

	find = *findp;
	*findp = NULL;

	DP(DEF_LEVEL, "dns_adb_destroyfind on find %p", find);

	adb = find->adb;

	LOCK(&find->lock);

	REQUIRE(find->adbname == NULL);

	/*
	 * Free the addrinfo objects on the find's list. Note that
	 * we also need to decrement the reference counter in the
	 * associated adbentry every time we remove one from the list.
	 */
	ai = ISC_LIST_HEAD(find->list);
	while (ai != NULL) {
		ISC_LIST_UNLINK(find->list, ai, publink);
		free_adbaddrinfo(adb, &ai);
		ai = ISC_LIST_HEAD(find->list);
	}
	UNLOCK(&find->lock);

	free_adbfind(&find);
}

/*
 * Caller must hold find lock.
 */
static void
find_sendevent(dns_adbfind_t *find) {
	if (!FIND_EVENTSENT(find)) {
		atomic_store(&find->status, DNS_ADB_CANCELED);

		DP(DEF_LEVEL, "sending find %p to caller", find);

		isc_async_run(find->loop, find->cb, find);
	}
}

void
dns_adb_cancelfind(dns_adbfind_t *find) {
	dns_adbname_t *adbname = NULL;

	DP(DEF_LEVEL, "dns_adb_cancelfind on find %p", find);

	REQUIRE(DNS_ADBFIND_VALID(find));
	REQUIRE(DNS_ADB_VALID(find->adb));

	LOCK(&find->lock);
	REQUIRE(FIND_WANTEVENT(find));

	adbname = find->adbname;

	if (adbname == NULL) {
		find_sendevent(find);
		UNLOCK(&find->lock);
	} else {
		/*
		 * Release the find lock, then acquire the name and find
		 * locks in that order, to match locking hierarchy
		 * elsewhere.
		 */
		dns_adbname_ref(adbname);
		UNLOCK(&find->lock);

		/*
		 * Other thread could cancel the find between the unlock and
		 * lock, so we need to recheck whether the adbname is still
		 * valid and reference the adbname, so it does not vanish before
		 * we have a chance to lock it again.
		 */

		LOCK(&adbname->lock);
		LOCK(&find->lock);

		if (find->adbname != NULL) {
			ISC_LIST_UNLINK(find->adbname->finds, find, plink);
			find->adbname = NULL;
		}

		find_sendevent(find);

		UNLOCK(&find->lock);
		UNLOCK(&adbname->lock);
		dns_adbname_detach(&adbname);
	}
}

unsigned int
dns_adb_findstatus(dns_adbfind_t *find) {
	REQUIRE(DNS_ADBFIND_VALID(find));

	return atomic_load(&find->status);
}

void
dns_adb_dump(dns_adb_t *adb, FILE *f) {
	isc_stdtime_t now = isc_stdtime_now();

	REQUIRE(DNS_ADB_VALID(adb));
	REQUIRE(f != NULL);

	if (atomic_load(&adb->exiting)) {
		return;
	}

	cleanup_names(adb, now);
	cleanup_entries(adb, now);
	dump_adb(adb, f, false, now);
}

static void
dump_ttl(FILE *f, const char *legend, isc_stdtime_t value, isc_stdtime_t now) {
	if (value == INT_MAX) {
		return;
	}
	fprintf(f, " [%s TTL %d]", legend, (int)(value - now));
}

/*
 * Both rwlocks for the hash tables need to be held by the caller.
 */
static void
dump_adb(dns_adb_t *adb, FILE *f, bool debug, isc_stdtime_t now) {
	fprintf(f, ";\n; Address database dump\n;\n");
	fprintf(f, "; [edns success/timeout]\n");
	fprintf(f, "; [plain success/timeout]\n;\n");
	if (debug) {
		fprintf(f, "; addr %p, references %" PRIuFAST32 "\n", adb,
			isc_refcount_current(&adb->references));
	}

	/*
	 * Ensure this operation is applied to both hash tables at once.
	 */
	RWLOCK(&adb->names_lock, isc_rwlocktype_write);

	for (dns_adbname_t *name = ISC_LIST_HEAD(adb->names_lru); name != NULL;
	     name = ISC_LIST_NEXT(name, link))
	{
		LOCK(&name->lock);
		/*
		 * Dump the names
		 */
		if (debug) {
			fprintf(f, "; name %p (flags %08x)\n", name,
				name->flags);
		}
		fprintf(f, "; ");
		dns_name_print(name->name, f);
		if (dns_name_countlabels(&name->target) > 0) {
			fprintf(f, " alias ");
			dns_name_print(&name->target, f);
		}

		dump_ttl(f, "v4", name->expire_v4, now);
		dump_ttl(f, "v6", name->expire_v6, now);
		dump_ttl(f, "target", name->expire_target, now);

		fprintf(f, " [v4 %s] [v6 %s]", errnames[name->fetch_err],
			errnames[name->fetch6_err]);

		fprintf(f, "\n");

		print_namehook_list(f, "v4", adb, &name->v4, debug, now);
		print_namehook_list(f, "v6", adb, &name->v6, debug, now);

		if (debug) {
			print_fetch_list(f, name);
			print_find_list(f, name);
		}
		UNLOCK(&name->lock);
	}

	RWLOCK(&adb->entries_lock, isc_rwlocktype_write);
	fprintf(f, ";\n; Unassociated entries\n;\n");
	for (dns_adbentry_t *adbentry = ISC_LIST_HEAD(adb->entries_lru);
	     adbentry != NULL; adbentry = ISC_LIST_NEXT(adbentry, link))
	{
		LOCK(&adbentry->lock);
		if (ISC_LIST_EMPTY(adbentry->nhs)) {
			dump_entry(f, adb, adbentry, debug, now);
		}
		UNLOCK(&adbentry->lock);
	}

	RWUNLOCK(&adb->entries_lock, isc_rwlocktype_write);
	RWUNLOCK(&adb->names_lock, isc_rwlocktype_write);
}

static void
dump_entry(FILE *f, dns_adb_t *adb, dns_adbentry_t *entry, bool debug,
	   isc_stdtime_t now) {
	char addrbuf[ISC_NETADDR_FORMATSIZE];
	isc_netaddr_t netaddr;

	isc_netaddr_fromsockaddr(&netaddr, &entry->sockaddr);
	isc_netaddr_format(&netaddr, addrbuf, sizeof(addrbuf));

	if (debug) {
		fprintf(f, ";\t%p: refcnt %" PRIuFAST32 "\n", entry,
			isc_refcount_current(&entry->references));
	}

	fprintf(f,
		";\t%s [srtt %u] [flags %08x] [edns %u/%u] "
		"[plain %u/%u]",
		addrbuf, atomic_load(&entry->srtt), atomic_load(&entry->flags),
		entry->edns, entry->ednsto, entry->plain, entry->plainto);
	if (entry->udpsize != 0U) {
		fprintf(f, " [udpsize %u]", entry->udpsize);
	}
	if (entry->cookie != NULL) {
		unsigned int i;
		fprintf(f, " [cookie=");
		for (i = 0; i < entry->cookielen; i++) {
			fprintf(f, "%02x", entry->cookie[i]);
		}
		fprintf(f, "]");
	}
	fprintf(f, " [ttl %d]", entry->expires - now);

	if (adb != NULL && adb->quota != 0 && adb->atr_freq != 0) {
		uint_fast32_t quota = atomic_load_relaxed(&entry->quota);
		fprintf(f, " [atr %0.2f] [quota %" PRIuFAST32 "]", entry->atr,
			quota);
	}

	fprintf(f, "\n");
}

static void
dumpfind(dns_adbfind_t *find, FILE *f) {
	char tmp[512];
	const char *tmpp = NULL;
	dns_adbaddrinfo_t *ai = NULL;
	isc_sockaddr_t *sa = NULL;

	/*
	 * Not used currently, in the API Just In Case we
	 * want to dump out the name and/or entries too.
	 */

	LOCK(&find->lock);

	fprintf(f, ";Find %p\n", find);
	fprintf(f, ";\tqpending %08x partial %08x options %08x flags %08x\n",
		find->query_pending, find->partial_result, find->options,
		find->flags);
	fprintf(f, ";\tname %p\n", find->adbname);

	ai = ISC_LIST_HEAD(find->list);
	if (ai != NULL) {
		fprintf(f, "\tAddresses:\n");
	}
	while (ai != NULL) {
		sa = &ai->sockaddr;
		switch (sa->type.sa.sa_family) {
		case AF_INET:
			tmpp = inet_ntop(AF_INET, &sa->type.sin.sin_addr, tmp,
					 sizeof(tmp));
			break;
		case AF_INET6:
			tmpp = inet_ntop(AF_INET6, &sa->type.sin6.sin6_addr,
					 tmp, sizeof(tmp));
			break;
		default:
			tmpp = "UnkFamily";
		}

		if (tmpp == NULL) {
			tmpp = "BadAddress";
		}

		fprintf(f,
			"\t\tentry %p, flags %08x"
			" srtt %u addr %s\n",
			ai->entry, ai->flags, ai->srtt, tmpp);

		ai = ISC_LIST_NEXT(ai, publink);
	}

	UNLOCK(&find->lock);
}

static void
print_namehook_list(FILE *f, const char *legend, dns_adb_t *adb,
		    dns_adbnamehooklist_t *list, bool debug,
		    isc_stdtime_t now) {
	dns_adbnamehook_t *nh = NULL;

	for (nh = ISC_LIST_HEAD(*list); nh != NULL;
	     nh = ISC_LIST_NEXT(nh, name_link))
	{
		if (debug) {
			fprintf(f, ";\tHook(%s) %p\n", legend, nh);
		}
		LOCK(&nh->entry->lock);
		dump_entry(f, adb, nh->entry, debug, now);
		UNLOCK(&nh->entry->lock);
	}
}

static void
print_fetch(FILE *f, dns_adbfetch_t *ft, const char *type) {
	fprintf(f, "\t\tFetch(%s): %p -> { fetch %p }\n", type, ft, ft->fetch);
}

static void
print_fetch_list(FILE *f, dns_adbname_t *n) {
	if (NAME_FETCH_A(n)) {
		print_fetch(f, n->fetch_a, "A");
	}
	if (NAME_FETCH_AAAA(n)) {
		print_fetch(f, n->fetch_aaaa, "AAAA");
	}
}

static void
print_find_list(FILE *f, dns_adbname_t *name) {
	dns_adbfind_t *find = NULL;

	find = ISC_LIST_HEAD(name->finds);
	while (find != NULL) {
		dumpfind(find, f);
		find = ISC_LIST_NEXT(find, plink);
	}
}

static isc_result_t
putstr(isc_buffer_t **b, const char *str) {
	isc_result_t result;

	result = isc_buffer_reserve(*b, strlen(str));
	if (result != ISC_R_SUCCESS) {
		return result;
	}

	isc_buffer_putstr(*b, str);
	return ISC_R_SUCCESS;
}

isc_result_t
dns_adb_dumpquota(dns_adb_t *adb, isc_buffer_t **buf) {
	REQUIRE(DNS_ADB_VALID(adb));

	isc_hashmap_iter_t *it = NULL;
	isc_result_t result;

	RWLOCK(&adb->entries_lock, isc_rwlocktype_read);
	isc_hashmap_iter_create(adb->entries, &it);
	for (result = isc_hashmap_iter_first(it); result == ISC_R_SUCCESS;
	     result = isc_hashmap_iter_next(it))
	{
		dns_adbentry_t *entry = NULL;
		isc_hashmap_iter_current(it, (void **)&entry);

		LOCK(&entry->lock);
		char addrbuf[ISC_NETADDR_FORMATSIZE];
		char text[ISC_NETADDR_FORMATSIZE + BUFSIZ];
		isc_netaddr_t netaddr;

		if (entry->atr == 0.0 && entry->quota == adb->quota) {
			goto unlock;
		}

		isc_netaddr_fromsockaddr(&netaddr, &entry->sockaddr);
		isc_netaddr_format(&netaddr, addrbuf, sizeof(addrbuf));

		snprintf(text, sizeof(text),
			 "\n- quota %s (%" PRIuFAST32 "/%d) atr %0.2f", addrbuf,
			 atomic_load_relaxed(&entry->quota), adb->quota,
			 entry->atr);
		putstr(buf, text);
	unlock:
		UNLOCK(&entry->lock);
	}
	isc_hashmap_iter_destroy(&it);
	RWUNLOCK(&adb->entries_lock, isc_rwlocktype_read);

	return ISC_R_SUCCESS;
}

static isc_result_t
dbfind_name(dns_adbname_t *adbname, isc_stdtime_t now, dns_rdatatype_t rdtype) {
	isc_result_t result;
	dns_rdataset_t rdataset;
	dns_adb_t *adb = NULL;
	dns_fixedname_t foundname;
	dns_name_t *fname = NULL;

	REQUIRE(DNS_ADBNAME_VALID(adbname));

	adb = adbname->adb;

	REQUIRE(DNS_ADB_VALID(adb));
	REQUIRE(rdtype == dns_rdatatype_a || rdtype == dns_rdatatype_aaaa);

	fname = dns_fixedname_initname(&foundname);
	dns_rdataset_init(&rdataset);

	if (rdtype == dns_rdatatype_a) {
		adbname->fetch_err = FIND_ERR_UNEXPECTED;
	} else {
		adbname->fetch6_err = FIND_ERR_UNEXPECTED;
	}

	/*
	 * We need to specify whether to search static-stub zones (if
	 * configured) depending on whether this is a "start at zone" lookup,
	 * i.e., whether it's a "bailiwick" glue.  If it's bailiwick (in which
	 * case DNS_ADBFIND_STARTATZONE is set) we need to stop the search at
	 * any matching static-stub zone without looking into the cache to honor
	 * the configuration on which server we should send queries to.
	 */
	result =
		dns_view_find(adb->view, adbname->name, rdtype, now,
			      DNS_DBFIND_GLUEOK | DNS_DBFIND_ADDITIONALOK, true,
			      ((adbname->flags & DNS_ADBFIND_STARTATZONE) != 0),
			      NULL, NULL, fname, &rdataset, NULL);

	switch (result) {
	case DNS_R_GLUE:
	case DNS_R_HINT:
	case ISC_R_SUCCESS:
		/*
		 * Found in the database.  Even if we can't copy out
		 * any information, return success, or else a fetch
		 * will be made, which will only make things worse.
		 */
		if (rdtype == dns_rdatatype_a) {
			adbname->fetch_err = FIND_ERR_SUCCESS;
		} else {
			adbname->fetch6_err = FIND_ERR_SUCCESS;
		}
		result = import_rdataset(adbname, &rdataset, now);
		break;
	case DNS_R_NXDOMAIN:
	case DNS_R_NXRRSET:
		/*
		 * We're authoritative and the data doesn't exist.
		 * Make up a negative cache entry so we don't ask again
		 * for a while.
		 *
		 * XXXRTH  What time should we use?  I'm putting in 30 seconds
		 * for now.
		 */
		if (rdtype == dns_rdatatype_a) {
			adbname->expire_v4 = now + 30;
			DP(NCACHE_LEVEL,
			   "adb name %p: Caching auth negative entry for A",
			   adbname);
			if (result == DNS_R_NXDOMAIN) {
				adbname->fetch_err = FIND_ERR_NXDOMAIN;
			} else {
				adbname->fetch_err = FIND_ERR_NXRRSET;
			}
		} else {
			DP(NCACHE_LEVEL,
			   "adb name %p: Caching auth negative entry for AAAA",
			   adbname);
			adbname->expire_v6 = now + 30;
			if (result == DNS_R_NXDOMAIN) {
				adbname->fetch6_err = FIND_ERR_NXDOMAIN;
			} else {
				adbname->fetch6_err = FIND_ERR_NXRRSET;
			}
		}
		break;
	case DNS_R_NCACHENXDOMAIN:
	case DNS_R_NCACHENXRRSET:
		/*
		 * We found a negative cache entry.  Pull the TTL from it
		 * so we won't ask again for a while.
		 */
		rdataset.ttl = ttlclamp(rdataset.ttl);
		if (rdtype == dns_rdatatype_a) {
			adbname->expire_v4 = rdataset.ttl + now;
			if (result == DNS_R_NCACHENXDOMAIN) {
				adbname->fetch_err = FIND_ERR_NXDOMAIN;
			} else {
				adbname->fetch_err = FIND_ERR_NXRRSET;
			}
			DP(NCACHE_LEVEL,
			   "adb name %p: Caching negative entry for A (ttl %u)",
			   adbname, rdataset.ttl);
		} else {
			DP(NCACHE_LEVEL,
			   "adb name %p: Caching negative entry for AAAA (ttl "
			   "%u)",
			   adbname, rdataset.ttl);
			adbname->expire_v6 = rdataset.ttl + now;
			if (result == DNS_R_NCACHENXDOMAIN) {
				adbname->fetch6_err = FIND_ERR_NXDOMAIN;
			} else {
				adbname->fetch6_err = FIND_ERR_NXRRSET;
			}
		}
		break;
	case DNS_R_CNAME:
	case DNS_R_DNAME:
		rdataset.ttl = ttlclamp(rdataset.ttl);
		clean_target(adb, &adbname->target);
		adbname->expire_target = INT_MAX;
		result = set_target(adb, adbname->name, fname, &rdataset,
				    &adbname->target);
		if (result == ISC_R_SUCCESS) {
			result = DNS_R_ALIAS;
			DP(NCACHE_LEVEL, "adb name %p: caching alias target",
			   adbname);
			adbname->expire_target = ADJUSTED_EXPIRE(
				adbname->expire_target, now, rdataset.ttl);
		}
		if (rdtype == dns_rdatatype_a) {
			adbname->fetch_err = FIND_ERR_SUCCESS;
		} else {
			adbname->fetch6_err = FIND_ERR_SUCCESS;
		}
		break;
	default:
		break;
	}

	if (dns_rdataset_isassociated(&rdataset)) {
		dns_rdataset_disassociate(&rdataset);
	}

	return result;
}

static void
fetch_callback(void *arg) {
	dns_fetchresponse_t *resp = (dns_fetchresponse_t *)arg;
	dns_adbname_t *name = resp->arg;
	dns_adb_t *adb = NULL;
	dns_adbfetch_t *fetch = NULL;
	dns_adbstatus_t astat = DNS_ADB_NOMOREADDRESSES;
	isc_stdtime_t now;
	isc_result_t result;
	unsigned int address_type;

	REQUIRE(DNS_ADBNAME_VALID(name));
	dns_adb_attach(name->adb, &adb);

	REQUIRE(DNS_ADB_VALID(adb));

	LOCK(&name->lock);

	INSIST(NAME_FETCH_A(name) || NAME_FETCH_AAAA(name));
	address_type = 0;
	if (NAME_FETCH_A(name) && (name->fetch_a->fetch == resp->fetch)) {
		address_type = DNS_ADBFIND_INET;
		fetch = name->fetch_a;
		name->fetch_a = NULL;
	} else if (NAME_FETCH_AAAA(name) &&
		   (name->fetch_aaaa->fetch == resp->fetch))
	{
		address_type = DNS_ADBFIND_INET6;
		fetch = name->fetch_aaaa;
		name->fetch_aaaa = NULL;
	} else {
		fetch = NULL;
	}

	INSIST(address_type != 0 && fetch != NULL);

	/*
	 * Cleanup things we don't care about.
	 */
	if (resp->node != NULL) {
		dns_db_detachnode(resp->db, &resp->node);
	}
	if (resp->db != NULL) {
		dns_db_detach(&resp->db);
	}

	/*
	 * If this name is marked as dead, clean up, throwing away
	 * potentially good data.
	 */
	if (NAME_DEAD(name)) {
		astat = DNS_ADB_CANCELED;
		goto out;
	}

	now = isc_stdtime_now();

	/*
	 * If we got a negative cache response, remember it.
	 */
	if (NCACHE_RESULT(resp->result)) {
		resp->rdataset->ttl = ttlclamp(resp->rdataset->ttl);
		if (address_type == DNS_ADBFIND_INET) {
			name->expire_v4 = ADJUSTED_EXPIRE(name->expire_v4, now,
							  resp->rdataset->ttl);
			DP(NCACHE_LEVEL,
			   "adb fetch name %p: "
			   "caching negative entry for A (ttl %u)",
			   name, name->expire_v4);
			if (resp->result == DNS_R_NCACHENXDOMAIN) {
				name->fetch_err = FIND_ERR_NXDOMAIN;
			} else {
				name->fetch_err = FIND_ERR_NXRRSET;
			}
			inc_resstats(adb, dns_resstatscounter_gluefetchv4fail);
		} else {
			name->expire_v6 = ADJUSTED_EXPIRE(name->expire_v6, now,
							  resp->rdataset->ttl);
			DP(NCACHE_LEVEL,
			   "adb fetch name %p: "
			   "caching negative entry for AAAA (ttl %u)",
			   name, name->expire_v6);
			if (resp->result == DNS_R_NCACHENXDOMAIN) {
				name->fetch6_err = FIND_ERR_NXDOMAIN;
			} else {
				name->fetch6_err = FIND_ERR_NXRRSET;
			}
			inc_resstats(adb, dns_resstatscounter_gluefetchv6fail);
		}
		goto out;
	}

	/*
	 * Handle CNAME/DNAME.
	 */
	if (resp->result == DNS_R_CNAME || resp->result == DNS_R_DNAME) {
		resp->rdataset->ttl = ttlclamp(resp->rdataset->ttl);
		clean_target(adb, &name->target);
		name->expire_target = INT_MAX;
		result = set_target(adb, name->name, resp->foundname,
				    resp->rdataset, &name->target);
		if (result == ISC_R_SUCCESS) {
			DP(NCACHE_LEVEL,
			   "adb fetch name %p: caching alias target", name);
			name->expire_target = ADJUSTED_EXPIRE(
				name->expire_target, now, resp->rdataset->ttl);
		}
		goto check_result;
	}

	/*
	 * Did we get back junk?  If so, and there are no more fetches
	 * sitting out there, tell all the finds about it.
	 */
	if (resp->result != ISC_R_SUCCESS) {
		char buf[DNS_NAME_FORMATSIZE];

		dns_name_format(name->name, buf, sizeof(buf));
		DP(DEF_LEVEL, "adb: fetch of '%s' %s failed: %s", buf,
		   address_type == DNS_ADBFIND_INET ? "A" : "AAAA",
		   isc_result_totext(resp->result));
		/*
		 * Don't record a failure unless this is the initial
		 * fetch of a chain.
		 */
		if (fetch->depth > 1) {
			goto out;
		}
		/* XXXMLG Don't pound on bad servers. */
		if (address_type == DNS_ADBFIND_INET) {
			name->expire_v4 = ISC_MIN(name->expire_v4, now + 10);
			name->fetch_err = FIND_ERR_FAILURE;
			inc_resstats(adb, dns_resstatscounter_gluefetchv4fail);
		} else {
			name->expire_v6 = ISC_MIN(name->expire_v6, now + 10);
			name->fetch6_err = FIND_ERR_FAILURE;
			inc_resstats(adb, dns_resstatscounter_gluefetchv6fail);
		}
		goto out;
	}

	/*
	 * We got something potentially useful.
	 */
	result = import_rdataset(name, &fetch->rdataset, now);

check_result:
	if (result == ISC_R_SUCCESS) {
		astat = DNS_ADB_MOREADDRESSES;
		if (address_type == DNS_ADBFIND_INET) {
			name->fetch_err = FIND_ERR_SUCCESS;
		} else {
			name->fetch6_err = FIND_ERR_SUCCESS;
		}
	}

out:
	dns_resolver_destroyfetch(&fetch->fetch);
	free_adbfetch(adb, &fetch);
	isc_mem_putanddetach(&resp->mctx, resp, sizeof(*resp));
	if (astat != DNS_ADB_CANCELED) {
		clean_finds_at_name(name, astat, address_type);
	}
	UNLOCK(&name->lock);
	dns_adbname_detach(&name);
	dns_adb_detach(&adb);
}

static isc_result_t
fetch_name(dns_adbname_t *adbname, bool start_at_zone, unsigned int depth,
	   isc_counter_t *qc, dns_rdatatype_t type) {
	isc_result_t result;
	dns_adbfetch_t *fetch = NULL;
	dns_adb_t *adb = NULL;
	dns_fixedname_t fixed;
	dns_name_t *name = NULL;
	dns_rdataset_t rdataset;
	dns_rdataset_t *nameservers = NULL;
	unsigned int options;

	REQUIRE(DNS_ADBNAME_VALID(adbname));

	adb = adbname->adb;

	REQUIRE(DNS_ADB_VALID(adb));

	REQUIRE((type == dns_rdatatype_a && !NAME_FETCH_A(adbname)) ||
		(type == dns_rdatatype_aaaa && !NAME_FETCH_AAAA(adbname)));

	adbname->fetch_err = FIND_ERR_NOTFOUND;

	dns_rdataset_init(&rdataset);

	options = DNS_FETCHOPT_NOVALIDATE;

	if (start_at_zone) {
		DP(ENTER_LEVEL, "fetch_name: starting at zone for name %p",
		   adbname);
		name = dns_fixedname_initname(&fixed);
		result = dns_view_findzonecut(adb->view, adbname->name, name,
					      NULL, 0, 0, true, false,
					      &rdataset, NULL);
		if (result != ISC_R_SUCCESS && result != DNS_R_HINT) {
			goto cleanup;
		}
		nameservers = &rdataset;
		options |= DNS_FETCHOPT_UNSHARED;
	} else if (adb->view->qminimization) {
		options |= DNS_FETCHOPT_QMINIMIZE | DNS_FETCHOPT_QMIN_SKIP_IP6A;
		if (adb->view->qmin_strict) {
			options |= DNS_FETCHOPT_QMIN_STRICT;
		}
	}

	fetch = new_adbfetch(adb);
	fetch->depth = depth;

	/*
	 * We're not minimizing this query, as nothing user-related should
	 * be leaked here.
	 * However, if we'd ever want to change it we'd have to modify
	 * createfetch to find deepest cached name when we're providing
	 * domain and nameservers.
	 */
	result = dns_resolver_createfetch(
		adb->res, adbname->name, type, name, nameservers, NULL, NULL, 0,
		options, depth, qc, isc_loop(), fetch_callback, adbname,
		&fetch->rdataset, NULL, &fetch->fetch);
	if (result != ISC_R_SUCCESS) {
		DP(ENTER_LEVEL, "fetch_name: createfetch failed with %s",
		   isc_result_totext(result));
		goto cleanup;
	}

	dns_adbname_ref(adbname);

	if (type == dns_rdatatype_a) {
		adbname->fetch_a = fetch;
		inc_resstats(adb, dns_resstatscounter_gluefetchv4);
	} else {
		adbname->fetch_aaaa = fetch;
		inc_resstats(adb, dns_resstatscounter_gluefetchv6);
	}
	fetch = NULL; /* Keep us from cleaning this up below. */

cleanup:
	if (fetch != NULL) {
		free_adbfetch(adb, &fetch);
	}
	if (dns_rdataset_isassociated(&rdataset)) {
		dns_rdataset_disassociate(&rdataset);
	}

	return result;
}

void
dns_adb_adjustsrtt(dns_adb_t *adb, dns_adbaddrinfo_t *addr, unsigned int rtt,
		   unsigned int factor) {
	REQUIRE(DNS_ADB_VALID(adb));
	REQUIRE(DNS_ADBADDRINFO_VALID(addr));
	REQUIRE(factor <= 10);

	isc_stdtime_t now = 0;
	if (factor == DNS_ADB_RTTADJAGE) {
		now = isc_stdtime_now();
	}

	adjustsrtt(addr, rtt, factor, now);
}

void
dns_adb_agesrtt(dns_adb_t *adb, dns_adbaddrinfo_t *addr, isc_stdtime_t now) {
	REQUIRE(DNS_ADB_VALID(adb));
	REQUIRE(DNS_ADBADDRINFO_VALID(addr));

	adjustsrtt(addr, 0, DNS_ADB_RTTADJAGE, now);
}

static void
adjustsrtt(dns_adbaddrinfo_t *addr, unsigned int rtt, unsigned int factor,
	   isc_stdtime_t now) {
	unsigned int new_srtt;

	if (factor == DNS_ADB_RTTADJAGE) {
		if (atomic_load(&addr->entry->lastage) != now) {
			new_srtt = (uint64_t)atomic_load(&addr->entry->srtt) *
				   98 / 100;
			atomic_store(&addr->entry->lastage, now);
			atomic_store(&addr->entry->srtt, new_srtt);
			addr->srtt = new_srtt;
		}
	} else {
		new_srtt = ((uint64_t)atomic_load(&addr->entry->srtt) / 10 *
			    factor) +
			   ((uint64_t)rtt / 10 * (10 - factor));
		atomic_store(&addr->entry->srtt, new_srtt);
		addr->srtt = new_srtt;
	}
}

void
dns_adb_changeflags(dns_adb_t *adb, dns_adbaddrinfo_t *addr, unsigned int bits,
		    unsigned int mask) {
	REQUIRE(DNS_ADB_VALID(adb));
	REQUIRE(DNS_ADBADDRINFO_VALID(addr));

	dns_adbentry_t *entry = addr->entry;

	unsigned int flags = atomic_load(&entry->flags);
	while (!atomic_compare_exchange_strong(&entry->flags, &flags,
					       (flags & ~mask) | (bits & mask)))
	{
		/* repeat */
	}

	/*
	 * Note that we do not update the other bits in addr->flags with
	 * the most recent values from addr->entry->flags.
	 */
	addr->flags = (addr->flags & ~mask) | (bits & mask);
}

/*
 * The polynomial backoff curve (10000 / ((10 + n) / 10)^(3/2)) <0..99> drops
 * fairly aggressively at first, then slows down and tails off at around 2-3%.
 *
 * These will be used to make quota adjustments.
 */
static int quota_adj[] = {
	10000, 8668, 7607, 6747, 6037, 5443, 4941, 4512, 4141, 3818, 3536,
	3286,  3065, 2867, 2690, 2530, 2385, 2254, 2134, 2025, 1925, 1832,
	1747,  1668, 1595, 1527, 1464, 1405, 1350, 1298, 1250, 1205, 1162,
	1121,  1083, 1048, 1014, 981,  922,  894,  868,	 843,  820,  797,
	775,   755,  735,  716,	 698,  680,  664,  648,	 632,  618,  603,
	590,   577,  564,  552,	 540,  529,  518,  507,	 497,  487,  477,
	468,   459,  450,  442,	 434,  426,  418,  411,	 404,  397,  390,
	383,   377,  370,  364,	 358,  353,  347,  342,	 336,  331,  326,
	321,   316,  312,  307,	 303,  298,  294,  290,	 286,  282,  278
};

#define QUOTA_ADJ_SIZE (sizeof(quota_adj) / sizeof(quota_adj[0]))

/*
 * The adb entry associated with 'addr' must be locked.
 */
static void
maybe_adjust_quota(dns_adb_t *adb, dns_adbaddrinfo_t *addr, bool timeout) {
	double tr;

	UNUSED(adb);

	if (adb->quota == 0 || adb->atr_freq == 0) {
		return;
	}

	if (timeout) {
		addr->entry->timeouts++;
	}

	if (addr->entry->completed++ <= adb->atr_freq) {
		return;
	}

	/*
	 * Calculate an exponential rolling average of the timeout ratio
	 *
	 * XXX: Integer arithmetic might be better than floating point
	 */
	tr = (double)addr->entry->timeouts / addr->entry->completed;
	addr->entry->timeouts = addr->entry->completed = 0;
	INSIST(addr->entry->atr >= 0.0);
	INSIST(addr->entry->atr <= 1.0);
	INSIST(adb->atr_discount >= 0.0);
	INSIST(adb->atr_discount <= 1.0);
	addr->entry->atr *= 1.0 - adb->atr_discount;
	addr->entry->atr += tr * adb->atr_discount;
	addr->entry->atr = ISC_CLAMP(addr->entry->atr, 0.0, 1.0);

	if (addr->entry->atr < adb->atr_low && addr->entry->mode > 0) {
		uint_fast32_t new_quota =
			adb->quota * quota_adj[--addr->entry->mode] / 10000;
		atomic_store_release(&addr->entry->quota,
				     ISC_MAX(1, new_quota));
		log_quota(addr->entry,
			  "atr %0.2f, quota increased to %" PRIuFAST32,
			  addr->entry->atr, new_quota);
	} else if (addr->entry->atr > adb->atr_high &&
		   addr->entry->mode < (QUOTA_ADJ_SIZE - 1))
	{
		uint_fast32_t new_quota =
			adb->quota * quota_adj[++addr->entry->mode] / 10000;
		atomic_store_release(&addr->entry->quota,
				     ISC_MAX(1, new_quota));
		log_quota(addr->entry,
			  "atr %0.2f, quota decreased to %" PRIuFAST32,
			  addr->entry->atr, new_quota);
	}
}

#define EDNSTOS 3U

void
dns_adb_plainresponse(dns_adb_t *adb, dns_adbaddrinfo_t *addr) {
	REQUIRE(DNS_ADB_VALID(adb));
	REQUIRE(DNS_ADBADDRINFO_VALID(addr));

	dns_adbentry_t *entry = addr->entry;
	LOCK(&entry->lock);

	maybe_adjust_quota(adb, addr, false);

	entry->plain++;
	if (entry->plain == 0xff) {
		entry->edns >>= 1;
		entry->ednsto >>= 1;
		entry->plain >>= 1;
		entry->plainto >>= 1;
	}
	UNLOCK(&entry->lock);
}

void
dns_adb_timeout(dns_adb_t *adb, dns_adbaddrinfo_t *addr) {
	REQUIRE(DNS_ADB_VALID(adb));
	REQUIRE(DNS_ADBADDRINFO_VALID(addr));

	dns_adbentry_t *entry = addr->entry;
	LOCK(&entry->lock);

	maybe_adjust_quota(adb, addr, true);

	addr->entry->plainto++;
	if (addr->entry->plainto == 0xff) {
		addr->entry->edns >>= 1;
		addr->entry->ednsto >>= 1;
		addr->entry->plain >>= 1;
		addr->entry->plainto >>= 1;
	}
	UNLOCK(&entry->lock);
}

void
dns_adb_ednsto(dns_adb_t *adb, dns_adbaddrinfo_t *addr) {
	REQUIRE(DNS_ADB_VALID(adb));
	REQUIRE(DNS_ADBADDRINFO_VALID(addr));

	dns_adbentry_t *entry = addr->entry;
	LOCK(&entry->lock);

	maybe_adjust_quota(adb, addr, true);

	entry->ednsto++;
	if (addr->entry->ednsto == 0xff) {
		entry->edns >>= 1;
		entry->ednsto >>= 1;
		entry->plain >>= 1;
		entry->plainto >>= 1;
	}
	UNLOCK(&entry->lock);
}

void
dns_adb_setudpsize(dns_adb_t *adb, dns_adbaddrinfo_t *addr, unsigned int size) {
	REQUIRE(DNS_ADB_VALID(adb));
	REQUIRE(DNS_ADBADDRINFO_VALID(addr));

	dns_adbentry_t *entry = addr->entry;

	LOCK(&entry->lock);
	if (size < 512U) {
		size = 512U;
	}
	if (size > addr->entry->udpsize) {
		addr->entry->udpsize = size;
	}

	maybe_adjust_quota(adb, addr, false);

	entry->edns++;
	if (entry->edns == 0xff) {
		entry->edns >>= 1;
		entry->ednsto >>= 1;
		entry->plain >>= 1;
		entry->plainto >>= 1;
	}
	UNLOCK(&entry->lock);
}

unsigned int
dns_adb_getudpsize(dns_adb_t *adb, dns_adbaddrinfo_t *addr) {
	REQUIRE(DNS_ADB_VALID(adb));
	REQUIRE(DNS_ADBADDRINFO_VALID(addr));

	unsigned int size;
	dns_adbentry_t *entry = addr->entry;

	LOCK(&entry->lock);
	size = entry->udpsize;
	UNLOCK(&entry->lock);

	return size;
}

void
dns_adb_setcookie(dns_adb_t *adb, dns_adbaddrinfo_t *addr,
		  const unsigned char *cookie, size_t len) {
	REQUIRE(DNS_ADB_VALID(adb));
	REQUIRE(DNS_ADBADDRINFO_VALID(addr));

	dns_adbentry_t *entry = addr->entry;

	LOCK(&entry->lock);

	if (entry->cookie != NULL &&
	    (cookie == NULL || len != entry->cookielen))
	{
		isc_mem_put(adb->mctx, entry->cookie, entry->cookielen);
		entry->cookie = NULL;
		entry->cookielen = 0;
	}

	if (entry->cookie == NULL && cookie != NULL && len != 0U) {
		entry->cookie = isc_mem_get(adb->mctx, len);
		entry->cookielen = (uint16_t)len;
	}

	if (entry->cookie != NULL) {
		memmove(entry->cookie, cookie, len);
	}
	UNLOCK(&entry->lock);
}

size_t
dns_adb_getcookie(dns_adbaddrinfo_t *addr, unsigned char *cookie, size_t len) {
	REQUIRE(DNS_ADBADDRINFO_VALID(addr));

	dns_adbentry_t *entry = addr->entry;

	LOCK(&entry->lock);
	if (entry->cookie == NULL) {
		len = 0;
		goto unlock;
	}
	if (cookie != NULL) {
		if (len < entry->cookielen) {
			len = 0;
			goto unlock;
		}
		memmove(cookie, entry->cookie, entry->cookielen);
	}
	len = entry->cookielen;

unlock:
	UNLOCK(&entry->lock);

	return len;
}

isc_result_t
dns_adb_findaddrinfo(dns_adb_t *adb, const isc_sockaddr_t *sa,
		     dns_adbaddrinfo_t **addrp, isc_stdtime_t now) {
	REQUIRE(DNS_ADB_VALID(adb));
	REQUIRE(addrp != NULL && *addrp == NULL);
	UNUSED(now);

	isc_result_t result = ISC_R_SUCCESS;
	dns_adbentry_t *entry = NULL;
	dns_adbaddrinfo_t *addr = NULL;
	in_port_t port;

	if (atomic_load(&adb->exiting)) {
		return ISC_R_SHUTTINGDOWN;
	}

	entry = get_attached_and_locked_entry(adb, now, sa);

	UNLOCK(&entry->lock);

	port = isc_sockaddr_getport(sa);
	addr = new_adbaddrinfo(adb, entry, port);
	*addrp = addr;

	dns_adbentry_detach(&entry);

	return result;
}

void
dns_adb_freeaddrinfo(dns_adb_t *adb, dns_adbaddrinfo_t **addrp) {
	dns_adbaddrinfo_t *addr = NULL;
	dns_adbentry_t *entry = NULL;

	REQUIRE(DNS_ADB_VALID(adb));
	REQUIRE(addrp != NULL);

	addr = *addrp;
	*addrp = NULL;

	REQUIRE(DNS_ADBADDRINFO_VALID(addr));

	entry = addr->entry;

	REQUIRE(DNS_ADBENTRY_VALID(entry));

	free_adbaddrinfo(adb, &addr);
}

void
dns_adb_flush(dns_adb_t *adb) {
	REQUIRE(DNS_ADB_VALID(adb));

	if (atomic_load(&adb->exiting)) {
		return;
	}

	cleanup_names(adb, INT_MAX);
	cleanup_entries(adb, INT_MAX);
#ifdef DUMP_ADB_AFTER_CLEANING
	dump_adb(adb, stdout, true, INT_MAX);
#endif /* ifdef DUMP_ADB_AFTER_CLEANING */
}

void
dns_adb_flushname(dns_adb_t *adb, const dns_name_t *name) {
	dns_adbname_t *adbname = NULL;
	isc_result_t result;
	bool start_at_zone = false;
	bool static_stub = false;
	dns_adbname_t key = { .name = UNCONST(name) };

	REQUIRE(DNS_ADB_VALID(adb));
	REQUIRE(name != NULL);

	if (atomic_load(&adb->exiting)) {
		return;
	}

	RWLOCK(&adb->names_lock, isc_rwlocktype_write);
again:
	/*
	 * Delete all entries - with and without DNS_ADBFIND_STARTATZONE set
	 * and with and without DNS_ADBFIND_STATICSTUB set.
	 */
	key.flags = ((static_stub) ? DNS_ADBFIND_STATICSTUB : 0) |
		    ((start_at_zone) ? DNS_ADBFIND_STARTATZONE : 0);

	result = isc_hashmap_find(adb->names, hash_adbname(&key), match_adbname,
				  (void *)&key, (void **)&adbname);
	if (result == ISC_R_SUCCESS) {
		dns_adbname_ref(adbname);
		LOCK(&adbname->lock);
		if (dns_name_equal(name, adbname->name)) {
			expire_name(adbname, DNS_ADB_CANCELED);
		}
		UNLOCK(&adbname->lock);
		dns_adbname_detach(&adbname);
	}
	if (!start_at_zone) {
		start_at_zone = true;
		goto again;
	}
	if (!static_stub) {
		static_stub = true;
		goto again;
	}
	RWUNLOCK(&adb->names_lock, isc_rwlocktype_write);
}

void
dns_adb_flushnames(dns_adb_t *adb, const dns_name_t *name) {
	dns_adbname_t *next = NULL;

	REQUIRE(DNS_ADB_VALID(adb));
	REQUIRE(name != NULL);

	if (atomic_load(&adb->exiting)) {
		return;
	}

	RWLOCK(&adb->names_lock, isc_rwlocktype_write);
	for (dns_adbname_t *adbname = ISC_LIST_HEAD(adb->names_lru);
	     adbname != NULL; adbname = next)
	{
		next = ISC_LIST_NEXT(adbname, link);
		dns_adbname_ref(adbname);
		LOCK(&adbname->lock);
		if (dns_name_issubdomain(adbname->name, name)) {
			expire_name(adbname, DNS_ADB_CANCELED);
		}
		UNLOCK(&adbname->lock);
		dns_adbname_detach(&adbname);
	}
	RWUNLOCK(&adb->names_lock, isc_rwlocktype_write);
}

void
dns_adb_setadbsize(dns_adb_t *adb, size_t size) {
	size_t hiwater, lowater;

	REQUIRE(DNS_ADB_VALID(adb));

	if (size != 0U && size < DNS_ADB_MINADBSIZE) {
		size = DNS_ADB_MINADBSIZE;
	}

	hiwater = size - (size >> 3); /* Approximately 7/8ths. */
	lowater = size - (size >> 2); /* Approximately 3/4ths. */

	if (size == 0U || hiwater == 0U || lowater == 0U) {
		isc_mem_clearwater(adb->mctx);
	} else {
		isc_mem_setwater(adb->mctx, hiwater, lowater);
	}
}

void
dns_adb_setquota(dns_adb_t *adb, uint32_t quota, uint32_t freq, double low,
		 double high, double discount) {
	REQUIRE(DNS_ADB_VALID(adb));

	adb->quota = quota;
	adb->atr_freq = freq;
	adb->atr_low = low;
	adb->atr_high = high;
	adb->atr_discount = discount;
}

void
dns_adb_getquota(dns_adb_t *adb, uint32_t *quotap, uint32_t *freqp,
		 double *lowp, double *highp, double *discountp) {
	REQUIRE(DNS_ADB_VALID(adb));

	SET_IF_NOT_NULL(quotap, adb->quota);

	SET_IF_NOT_NULL(freqp, adb->atr_freq);

	SET_IF_NOT_NULL(lowp, adb->atr_low);

	SET_IF_NOT_NULL(highp, adb->atr_high);

	SET_IF_NOT_NULL(discountp, adb->atr_discount);
}

static bool
adbentry_overquota(dns_adbentry_t *entry) {
	REQUIRE(DNS_ADBENTRY_VALID(entry));

	uint_fast32_t quota = atomic_load_relaxed(&entry->quota);
	uint_fast32_t active = atomic_load_acquire(&entry->active);

	return quota != 0 && active >= quota;
}

bool
dns_adb_overquota(dns_adb_t *adb ISC_ATTR_UNUSED, dns_adbaddrinfo_t *addrinfo) {
	REQUIRE(DNS_ADBADDRINFO_VALID(addrinfo));

	return adbentry_overquota(addrinfo->entry);
}

void
dns_adb_beginudpfetch(dns_adb_t *adb, dns_adbaddrinfo_t *addr) {
	uint_fast32_t active;

	REQUIRE(DNS_ADB_VALID(adb));
	REQUIRE(DNS_ADBADDRINFO_VALID(addr));

	active = atomic_fetch_add_relaxed(&addr->entry->active, 1);
	INSIST(active != UINT32_MAX);
}

void
dns_adb_endudpfetch(dns_adb_t *adb, dns_adbaddrinfo_t *addr) {
	uint_fast32_t active;

	REQUIRE(DNS_ADB_VALID(adb));
	REQUIRE(DNS_ADBADDRINFO_VALID(addr));

	active = atomic_fetch_sub_release(&addr->entry->active, 1);
	INSIST(active != 0);
}

isc_stats_t *
dns_adb_getstats(dns_adb_t *adb) {
	REQUIRE(DNS_ADB_VALID(adb));

	return adb->stats;
}
