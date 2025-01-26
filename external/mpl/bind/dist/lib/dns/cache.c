/*	$NetBSD: cache.c,v 1.13 2025/01/26 16:25:22 christos Exp $	*/

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

#include <isc/loop.h>
#include <isc/mem.h>
#include <isc/refcount.h>
#include <isc/result.h>
#include <isc/stats.h>
#include <isc/string.h>
#include <isc/time.h>
#include <isc/timer.h>
#include <isc/util.h>

#include <dns/cache.h>
#include <dns/db.h>
#include <dns/dbiterator.h>
#include <dns/log.h>
#include <dns/masterdump.h>
#include <dns/rdata.h>
#include <dns/rdataset.h>
#include <dns/rdatasetiter.h>
#include <dns/stats.h>

#ifdef HAVE_JSON_C
#include <json_object.h>
#endif /* HAVE_JSON_C */

#ifdef HAVE_LIBXML2
#include <libxml/xmlwriter.h>
#define ISC_XMLCHAR (const xmlChar *)
#endif /* HAVE_LIBXML2 */

#define CACHE_MAGIC	   ISC_MAGIC('$', '$', '$', '$')
#define VALID_CACHE(cache) ISC_MAGIC_VALID(cache, CACHE_MAGIC)

/*
 * DNS_CACHE_MINSIZE is how many bytes is the floor for
 * dns_cache_setcachesize().
 */
#define DNS_CACHE_MINSIZE 2097152U /*%< Bytes.  2097152 = 2 MB */

/***
 ***	Types
 ***/

/*%
 * The actual cache object.
 */

struct dns_cache {
	/* Unlocked. */
	unsigned int magic;
	isc_mutex_t lock;
	isc_mem_t *mctx;  /* Memory context for the dns_cache object */
	isc_mem_t *hmctx; /* Heap memory */
	isc_mem_t *tmctx; /* Tree memory */
	isc_loopmgr_t *loopmgr;
	char *name;
	isc_refcount_t references;

	/* Locked by 'lock'. */
	dns_rdataclass_t rdclass;
	dns_db_t *db;
	size_t size;
	dns_ttl_t serve_stale_ttl;
	dns_ttl_t serve_stale_refresh;
	isc_stats_t *stats;
	uint32_t maxrrperset;
	uint32_t maxtypepername;
};

/***
 ***	Functions
 ***/

static isc_result_t
cache_create_db(dns_cache_t *cache, dns_db_t **dbp, isc_mem_t **tmctxp,
		isc_mem_t **hmctxp) {
	isc_result_t result;
	char *argv[1] = { 0 };
	dns_db_t *db = NULL;
	isc_mem_t *tmctx = NULL, *hmctx = NULL;

	/*
	 * This will be the cache memory context, which is subject
	 * to cleaning when the configured memory limits are exceeded.
	 */
	isc_mem_create(&tmctx);
	isc_mem_setname(tmctx, "cache");

	/*
	 * This will be passed to RBTDB to use for heaps. This is separate
	 * from the main cache memory because it can grow quite large under
	 * heavy load and could otherwise cause the cache to be cleaned too
	 * aggressively.
	 */
	isc_mem_create(&hmctx);
	isc_mem_setname(hmctx, "cache_heap");

	/*
	 * For databases of type "qpcache" or "rbt" (which are the
	 * only cache implementations currently in existence) we pass
	 * hmctx to dns_db_create() via argv[0].
	 */
	argv[0] = (char *)hmctx;
	result = dns_db_create(tmctx, CACHEDB_DEFAULT, dns_rootname,
			       dns_dbtype_cache, cache->rdclass, 1, argv, &db);
	if (result != ISC_R_SUCCESS) {
		goto cleanup_mctx;
	}
	result = dns_db_setcachestats(db, cache->stats);
	if (result != ISC_R_SUCCESS) {
		goto cleanup_db;
	}

	dns_db_setservestalettl(db, cache->serve_stale_ttl);
	dns_db_setservestalerefresh(db, cache->serve_stale_refresh);
	dns_db_setmaxrrperset(db, cache->maxrrperset);
	dns_db_setmaxtypepername(db, cache->maxtypepername);

	/*
	 * XXX this is only used by the RBT cache, and can
	 * be removed when it is.
	 */
	dns_db_setloop(db, isc_loop_main(cache->loopmgr));

	*dbp = db;
	*hmctxp = hmctx;
	*tmctxp = tmctx;

	return ISC_R_SUCCESS;

cleanup_db:
	dns_db_detach(&db);
cleanup_mctx:
	isc_mem_detach(&hmctx);
	isc_mem_detach(&tmctx);

	return result;
}

static void
cache_destroy(dns_cache_t *cache) {
	isc_stats_detach(&cache->stats);
	isc_mutex_destroy(&cache->lock);
	isc_mem_free(cache->mctx, cache->name);
	if (cache->hmctx != NULL) {
		isc_mem_detach(&cache->hmctx);
	}
	if (cache->tmctx != NULL) {
		isc_mem_detach(&cache->tmctx);
	}
	isc_mem_putanddetach(&cache->mctx, cache, sizeof(*cache));
}

isc_result_t
dns_cache_create(isc_loopmgr_t *loopmgr, dns_rdataclass_t rdclass,
		 const char *cachename, isc_mem_t *mctx, dns_cache_t **cachep) {
	isc_result_t result;
	dns_cache_t *cache = NULL;

	REQUIRE(loopmgr != NULL);
	REQUIRE(cachename != NULL);
	REQUIRE(cachep != NULL && *cachep == NULL);

	cache = isc_mem_get(mctx, sizeof(*cache));
	*cache = (dns_cache_t){
		.rdclass = rdclass,
		.name = isc_mem_strdup(mctx, cachename),
		.loopmgr = loopmgr,
		.references = ISC_REFCOUNT_INITIALIZER(1),
		.magic = CACHE_MAGIC,
	};

	isc_mutex_init(&cache->lock);
	isc_mem_attach(mctx, &cache->mctx);

	isc_stats_create(mctx, &cache->stats, dns_cachestatscounter_max);

	/*
	 * Create the database
	 */
	result = cache_create_db(cache, &cache->db, &cache->tmctx,
				 &cache->hmctx);
	if (result != ISC_R_SUCCESS) {
		goto cleanup;
	}

	*cachep = cache;
	return ISC_R_SUCCESS;

cleanup:
	cache_destroy(cache);
	return result;
}

static void
cache_cleanup(dns_cache_t *cache) {
	REQUIRE(VALID_CACHE(cache));

	isc_refcount_destroy(&cache->references);
	cache->magic = 0;

	isc_mem_clearwater(cache->tmctx);
	dns_db_detach(&cache->db);

	cache_destroy(cache);
}

#if DNS_CACHE_TRACE
ISC_REFCOUNT_TRACE_IMPL(dns_cache, cache_cleanup);
#else
ISC_REFCOUNT_IMPL(dns_cache, cache_cleanup);
#endif

void
dns_cache_attachdb(dns_cache_t *cache, dns_db_t **dbp) {
	REQUIRE(VALID_CACHE(cache));
	REQUIRE(dbp != NULL && *dbp == NULL);
	REQUIRE(cache->db != NULL);

	LOCK(&cache->lock);
	dns_db_attach(cache->db, dbp);
	UNLOCK(&cache->lock);
}

const char *
dns_cache_getname(dns_cache_t *cache) {
	REQUIRE(VALID_CACHE(cache));

	return cache->name;
}

static void
updatewater(dns_cache_t *cache) {
	size_t hi = cache->size - (cache->size >> 3); /* ~ 7/8ths. */
	size_t lo = cache->size - (cache->size >> 2); /* ~ 3/4ths. */
	if (cache->size == 0U || hi == 0U || lo == 0U) {
		isc_mem_clearwater(cache->tmctx);
	} else {
		isc_mem_setwater(cache->tmctx, hi, lo);
	}
}

void
dns_cache_setcachesize(dns_cache_t *cache, size_t size) {
	REQUIRE(VALID_CACHE(cache));

	/*
	 * Impose a minimum cache size; pathological things happen if there
	 * is too little room.
	 */
	if (size != 0U && size < DNS_CACHE_MINSIZE) {
		size = DNS_CACHE_MINSIZE;
	}

	LOCK(&cache->lock);
	cache->size = size;
	updatewater(cache);
	UNLOCK(&cache->lock);
}

size_t
dns_cache_getcachesize(dns_cache_t *cache) {
	size_t size;

	REQUIRE(VALID_CACHE(cache));

	LOCK(&cache->lock);
	size = cache->size;
	UNLOCK(&cache->lock);

	return size;
}

void
dns_cache_setservestalettl(dns_cache_t *cache, dns_ttl_t ttl) {
	REQUIRE(VALID_CACHE(cache));

	LOCK(&cache->lock);
	cache->serve_stale_ttl = ttl;
	UNLOCK(&cache->lock);

	(void)dns_db_setservestalettl(cache->db, ttl);
}

dns_ttl_t
dns_cache_getservestalettl(dns_cache_t *cache) {
	dns_ttl_t ttl;
	isc_result_t result;

	REQUIRE(VALID_CACHE(cache));

	/*
	 * Could get it straight from the dns_cache_t, but use db
	 * to confirm the value that the db is really using.
	 */
	result = dns_db_getservestalettl(cache->db, &ttl);
	return result == ISC_R_SUCCESS ? ttl : 0;
}

void
dns_cache_setservestalerefresh(dns_cache_t *cache, dns_ttl_t interval) {
	REQUIRE(VALID_CACHE(cache));

	LOCK(&cache->lock);
	cache->serve_stale_refresh = interval;
	UNLOCK(&cache->lock);

	(void)dns_db_setservestalerefresh(cache->db, interval);
}

dns_ttl_t
dns_cache_getservestalerefresh(dns_cache_t *cache) {
	isc_result_t result;
	dns_ttl_t interval;

	REQUIRE(VALID_CACHE(cache));

	result = dns_db_getservestalerefresh(cache->db, &interval);
	return result == ISC_R_SUCCESS ? interval : 0;
}

isc_result_t
dns_cache_flush(dns_cache_t *cache) {
	dns_db_t *db = NULL, *olddb;
	isc_mem_t *tmctx = NULL, *oldtmctx;
	isc_mem_t *hmctx = NULL, *oldhmctx;
	isc_result_t result;

	result = cache_create_db(cache, &db, &tmctx, &hmctx);
	if (result != ISC_R_SUCCESS) {
		return result;
	}

	LOCK(&cache->lock);
	isc_mem_clearwater(cache->tmctx);
	oldhmctx = cache->hmctx;
	cache->hmctx = hmctx;
	oldtmctx = cache->tmctx;
	cache->tmctx = tmctx;
	updatewater(cache);
	olddb = cache->db;
	cache->db = db;
	UNLOCK(&cache->lock);

	dns_db_detach(&olddb);
	isc_mem_detach(&oldhmctx);
	isc_mem_detach(&oldtmctx);

	return ISC_R_SUCCESS;
}

static isc_result_t
clearnode(dns_db_t *db, dns_dbnode_t *node) {
	isc_result_t result;
	dns_rdatasetiter_t *iter = NULL;

	result = dns_db_allrdatasets(db, node, NULL, DNS_DB_STALEOK,
				     (isc_stdtime_t)0, &iter);
	if (result != ISC_R_SUCCESS) {
		return result;
	}

	for (result = dns_rdatasetiter_first(iter); result == ISC_R_SUCCESS;
	     result = dns_rdatasetiter_next(iter))
	{
		dns_rdataset_t rdataset;
		dns_rdataset_init(&rdataset);

		dns_rdatasetiter_current(iter, &rdataset);
		result = dns_db_deleterdataset(db, node, NULL, rdataset.type,
					       rdataset.covers);
		dns_rdataset_disassociate(&rdataset);
		if (result != ISC_R_SUCCESS && result != DNS_R_UNCHANGED) {
			break;
		}
	}

	if (result == ISC_R_NOMORE) {
		result = ISC_R_SUCCESS;
	}

	dns_rdatasetiter_destroy(&iter);
	return result;
}

static isc_result_t
cleartree(dns_db_t *db, const dns_name_t *name) {
	isc_result_t result, answer = ISC_R_SUCCESS;
	dns_dbiterator_t *iter = NULL;
	dns_dbnode_t *node = NULL, *top = NULL;
	dns_fixedname_t fnodename;
	dns_name_t *nodename;

	/*
	 * Create the node if it doesn't exist so dns_dbiterator_seek()
	 * can find it.  We will continue even if this fails.
	 */
	(void)dns_db_findnode(db, name, true, &top);

	nodename = dns_fixedname_initname(&fnodename);

	result = dns_db_createiterator(db, 0, &iter);
	if (result != ISC_R_SUCCESS) {
		goto cleanup;
	}

	result = dns_dbiterator_seek(iter, name);
	if (result == DNS_R_PARTIALMATCH) {
		result = dns_dbiterator_next(iter);
	}
	if (result != ISC_R_SUCCESS) {
		goto cleanup;
	}

	while (result == ISC_R_SUCCESS) {
		result = dns_dbiterator_current(iter, &node, nodename);
		if (result == DNS_R_NEWORIGIN) {
			result = ISC_R_SUCCESS;
		}
		if (result != ISC_R_SUCCESS) {
			goto cleanup;
		}
		/*
		 * Are we done?
		 */
		if (!dns_name_issubdomain(nodename, name)) {
			goto cleanup;
		}

		/*
		 * If clearnode fails record and move onto the next node.
		 */
		result = clearnode(db, node);
		if (result != ISC_R_SUCCESS && answer == ISC_R_SUCCESS) {
			answer = result;
		}
		dns_db_detachnode(db, &node);
		result = dns_dbiterator_next(iter);
	}

cleanup:
	if (result == ISC_R_NOMORE || result == ISC_R_NOTFOUND) {
		result = ISC_R_SUCCESS;
	}
	if (result != ISC_R_SUCCESS && answer == ISC_R_SUCCESS) {
		answer = result;
	}
	if (node != NULL) {
		dns_db_detachnode(db, &node);
	}
	if (iter != NULL) {
		dns_dbiterator_destroy(&iter);
	}
	if (top != NULL) {
		dns_db_detachnode(db, &top);
	}

	return answer;
}

isc_result_t
dns_cache_flushname(dns_cache_t *cache, const dns_name_t *name) {
	return dns_cache_flushnode(cache, name, false);
}

isc_result_t
dns_cache_flushnode(dns_cache_t *cache, const dns_name_t *name, bool tree) {
	isc_result_t result;
	dns_dbnode_t *node = NULL;
	dns_db_t *db = NULL;

	if (tree && dns_name_equal(name, dns_rootname)) {
		return dns_cache_flush(cache);
	}

	LOCK(&cache->lock);
	if (cache->db != NULL) {
		dns_db_attach(cache->db, &db);
	}
	UNLOCK(&cache->lock);
	if (db == NULL) {
		return ISC_R_SUCCESS;
	}

	if (tree) {
		result = cleartree(cache->db, name);
	} else {
		result = dns_db_findnode(cache->db, name, false, &node);
		if (result == ISC_R_NOTFOUND) {
			result = ISC_R_SUCCESS;
			goto cleanup_db;
		}
		if (result != ISC_R_SUCCESS) {
			goto cleanup_db;
		}
		result = clearnode(cache->db, node);
		dns_db_detachnode(cache->db, &node);
	}

cleanup_db:
	dns_db_detach(&db);
	return result;
}

isc_stats_t *
dns_cache_getstats(dns_cache_t *cache) {
	REQUIRE(VALID_CACHE(cache));
	return cache->stats;
}

void
dns_cache_updatestats(dns_cache_t *cache, isc_result_t result) {
	REQUIRE(VALID_CACHE(cache));
	if (cache->stats == NULL) {
		return;
	}

	switch (result) {
	case ISC_R_SUCCESS:
	case DNS_R_NCACHENXDOMAIN:
	case DNS_R_NCACHENXRRSET:
	case DNS_R_CNAME:
	case DNS_R_DNAME:
	case DNS_R_GLUE:
	case DNS_R_ZONECUT:
	case DNS_R_COVERINGNSEC:
		isc_stats_increment(cache->stats,
				    dns_cachestatscounter_queryhits);
		break;
	default:
		isc_stats_increment(cache->stats,
				    dns_cachestatscounter_querymisses);
	}
}

void
dns_cache_setmaxrrperset(dns_cache_t *cache, uint32_t value) {
	REQUIRE(VALID_CACHE(cache));

	cache->maxrrperset = value;
	if (cache->db != NULL) {
		dns_db_setmaxrrperset(cache->db, value);
	}
}

void
dns_cache_setmaxtypepername(dns_cache_t *cache, uint32_t value) {
	REQUIRE(VALID_CACHE(cache));

	cache->maxtypepername = value;
	if (cache->db != NULL) {
		dns_db_setmaxtypepername(cache->db, value);
	}
}

/*
 * XXX: Much of the following code has been copied in from statschannel.c.
 * We should refactor this into a generic function in stats.c that can be
 * called from both places.
 */
typedef struct cache_dumparg {
	isc_statsformat_t type;
	void *arg;		 /* type dependent argument */
	int ncounters;		 /* for general statistics */
	int *counterindices;	 /* for general statistics */
	uint64_t *countervalues; /* for general statistics */
	isc_result_t result;
} cache_dumparg_t;

static void
getcounter(isc_statscounter_t counter, uint64_t val, void *arg) {
	cache_dumparg_t *dumparg = arg;

	REQUIRE(counter < dumparg->ncounters);
	dumparg->countervalues[counter] = val;
}

static void
getcounters(isc_stats_t *stats, isc_statsformat_t type, int ncounters,
	    int *indices, uint64_t *values) {
	cache_dumparg_t dumparg;

	memset(values, 0, sizeof(values[0]) * ncounters);

	dumparg.type = type;
	dumparg.ncounters = ncounters;
	dumparg.counterindices = indices;
	dumparg.countervalues = values;

	isc_stats_dump(stats, getcounter, &dumparg, ISC_STATSDUMP_VERBOSE);
}

void
dns_cache_dumpstats(dns_cache_t *cache, FILE *fp) {
	int indices[dns_cachestatscounter_max];
	uint64_t values[dns_cachestatscounter_max];

	REQUIRE(VALID_CACHE(cache));

	getcounters(cache->stats, isc_statsformat_file,
		    dns_cachestatscounter_max, indices, values);

	fprintf(fp, "%20" PRIu64 " %s\n", values[dns_cachestatscounter_hits],
		"cache hits");
	fprintf(fp, "%20" PRIu64 " %s\n", values[dns_cachestatscounter_misses],
		"cache misses");
	fprintf(fp, "%20" PRIu64 " %s\n",
		values[dns_cachestatscounter_queryhits],
		"cache hits (from query)");
	fprintf(fp, "%20" PRIu64 " %s\n",
		values[dns_cachestatscounter_querymisses],
		"cache misses (from query)");
	fprintf(fp, "%20" PRIu64 " %s\n",
		values[dns_cachestatscounter_deletelru],
		"cache records deleted due to memory exhaustion");
	fprintf(fp, "%20" PRIu64 " %s\n",
		values[dns_cachestatscounter_deletettl],
		"cache records deleted due to TTL expiration");
	fprintf(fp, "%20" PRIu64 " %s\n",
		values[dns_cachestatscounter_coveringnsec],
		"covering nsec returned");
	fprintf(fp, "%20u %s\n", dns_db_nodecount(cache->db, dns_dbtree_main),
		"cache database nodes");
	fprintf(fp, "%20u %s\n", dns_db_nodecount(cache->db, dns_dbtree_nsec),
		"cache NSEC auxiliary database nodes");
	fprintf(fp, "%20" PRIu64 " %s\n", (uint64_t)dns_db_hashsize(cache->db),
		"cache database hash buckets");

	fprintf(fp, "%20" PRIu64 " %s\n", (uint64_t)isc_mem_inuse(cache->tmctx),
		"cache tree memory in use");

	fprintf(fp, "%20" PRIu64 " %s\n", (uint64_t)isc_mem_inuse(cache->hmctx),
		"cache heap memory in use");
}

#ifdef HAVE_LIBXML2
#define TRY0(a)                     \
	do {                        \
		xmlrc = (a);        \
		if (xmlrc < 0)      \
			goto error; \
	} while (0)
static int
renderstat(const char *name, uint64_t value, xmlTextWriterPtr writer) {
	int xmlrc;

	TRY0(xmlTextWriterStartElement(writer, ISC_XMLCHAR "counter"));
	TRY0(xmlTextWriterWriteAttribute(writer, ISC_XMLCHAR "name",
					 ISC_XMLCHAR name));
	TRY0(xmlTextWriterWriteFormatString(writer, "%" PRIu64 "", value));
	TRY0(xmlTextWriterEndElement(writer)); /* counter */

error:
	return xmlrc;
}

int
dns_cache_renderxml(dns_cache_t *cache, void *writer0) {
	int indices[dns_cachestatscounter_max];
	uint64_t values[dns_cachestatscounter_max];
	int xmlrc;
	xmlTextWriterPtr writer = (xmlTextWriterPtr)writer0;

	REQUIRE(VALID_CACHE(cache));

	getcounters(cache->stats, isc_statsformat_file,
		    dns_cachestatscounter_max, indices, values);
	TRY0(renderstat("CacheHits", values[dns_cachestatscounter_hits],
			writer));
	TRY0(renderstat("CacheMisses", values[dns_cachestatscounter_misses],
			writer));
	TRY0(renderstat("QueryHits", values[dns_cachestatscounter_queryhits],
			writer));
	TRY0(renderstat("QueryMisses",
			values[dns_cachestatscounter_querymisses], writer));
	TRY0(renderstat("DeleteLRU", values[dns_cachestatscounter_deletelru],
			writer));
	TRY0(renderstat("DeleteTTL", values[dns_cachestatscounter_deletettl],
			writer));
	TRY0(renderstat("CoveringNSEC",
			values[dns_cachestatscounter_coveringnsec], writer));

	TRY0(renderstat("CacheNodes",
			dns_db_nodecount(cache->db, dns_dbtree_main), writer));
	TRY0(renderstat("CacheNSECNodes",
			dns_db_nodecount(cache->db, dns_dbtree_nsec), writer));
	TRY0(renderstat("CacheBuckets", dns_db_hashsize(cache->db), writer));

	TRY0(renderstat("TreeMemInUse", isc_mem_inuse(cache->tmctx), writer));

	TRY0(renderstat("HeapMemInUse", isc_mem_inuse(cache->hmctx), writer));
error:
	return xmlrc;
}
#endif /* ifdef HAVE_LIBXML2 */

#ifdef HAVE_JSON_C
#define CHECKMEM(m)                              \
	do {                                     \
		if (m == NULL) {                 \
			result = ISC_R_NOMEMORY; \
			goto error;              \
		}                                \
	} while (0)

isc_result_t
dns_cache_renderjson(dns_cache_t *cache, void *cstats0) {
	isc_result_t result = ISC_R_SUCCESS;
	int indices[dns_cachestatscounter_max];
	uint64_t values[dns_cachestatscounter_max];
	json_object *obj;
	json_object *cstats = (json_object *)cstats0;

	REQUIRE(VALID_CACHE(cache));

	getcounters(cache->stats, isc_statsformat_file,
		    dns_cachestatscounter_max, indices, values);

	obj = json_object_new_int64(values[dns_cachestatscounter_hits]);
	CHECKMEM(obj);
	json_object_object_add(cstats, "CacheHits", obj);

	obj = json_object_new_int64(values[dns_cachestatscounter_misses]);
	CHECKMEM(obj);
	json_object_object_add(cstats, "CacheMisses", obj);

	obj = json_object_new_int64(values[dns_cachestatscounter_queryhits]);
	CHECKMEM(obj);
	json_object_object_add(cstats, "QueryHits", obj);

	obj = json_object_new_int64(values[dns_cachestatscounter_querymisses]);
	CHECKMEM(obj);
	json_object_object_add(cstats, "QueryMisses", obj);

	obj = json_object_new_int64(values[dns_cachestatscounter_deletelru]);
	CHECKMEM(obj);
	json_object_object_add(cstats, "DeleteLRU", obj);

	obj = json_object_new_int64(values[dns_cachestatscounter_deletettl]);
	CHECKMEM(obj);
	json_object_object_add(cstats, "DeleteTTL", obj);

	obj = json_object_new_int64(values[dns_cachestatscounter_coveringnsec]);
	CHECKMEM(obj);
	json_object_object_add(cstats, "CoveringNSEC", obj);

	obj = json_object_new_int64(
		dns_db_nodecount(cache->db, dns_dbtree_main));
	CHECKMEM(obj);
	json_object_object_add(cstats, "CacheNodes", obj);

	obj = json_object_new_int64(
		dns_db_nodecount(cache->db, dns_dbtree_nsec));
	CHECKMEM(obj);
	json_object_object_add(cstats, "CacheNSECNodes", obj);

	obj = json_object_new_int64(dns_db_hashsize(cache->db));
	CHECKMEM(obj);
	json_object_object_add(cstats, "CacheBuckets", obj);

	obj = json_object_new_int64(isc_mem_inuse(cache->tmctx));
	CHECKMEM(obj);
	json_object_object_add(cstats, "TreeMemInUse", obj);

	obj = json_object_new_int64(isc_mem_inuse(cache->hmctx));
	CHECKMEM(obj);
	json_object_object_add(cstats, "HeapMemInUse", obj);

	result = ISC_R_SUCCESS;
error:
	return result;
}
#endif /* ifdef HAVE_JSON_C */
