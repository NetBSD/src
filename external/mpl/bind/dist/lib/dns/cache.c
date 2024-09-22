/*	$NetBSD: cache.c,v 1.1.1.9 2024/09/22 00:06:16 christos Exp $	*/

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

#include <isc/mem.h>
#include <isc/print.h>
#include <isc/refcount.h>
#include <isc/result.h>
#include <isc/stats.h>
#include <isc/string.h>
#include <isc/task.h>
#include <isc/time.h>
#include <isc/timer.h>
#include <isc/util.h>

#include <dns/cache.h>
#include <dns/db.h>
#include <dns/dbiterator.h>
#include <dns/events.h>
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

#include "rbtdb.h"

#define CACHE_MAGIC	   ISC_MAGIC('$', '$', '$', '$')
#define VALID_CACHE(cache) ISC_MAGIC_VALID(cache, CACHE_MAGIC)

/*!
 * Control incremental cleaning.
 * DNS_CACHE_MINSIZE is how many bytes is the floor for
 * dns_cache_setcachesize(). See also DNS_CACHE_CLEANERINCREMENT
 */
#define DNS_CACHE_MINSIZE 2097152U /*%< Bytes.  2097152 = 2 MB */
/*!
 * Control incremental cleaning.
 * CLEANERINCREMENT is how many nodes are examined in one pass.
 * See also DNS_CACHE_MINSIZE
 */
#define DNS_CACHE_CLEANERINCREMENT 1000U /*%< Number of nodes. */

/***
 ***	Types
 ***/

/*
 * A cache_cleaner_t encapsulates the state of the periodic
 * cache cleaning.
 */

typedef struct cache_cleaner cache_cleaner_t;

typedef enum {
	cleaner_s_idle, /*%< Waiting for cleaning interval to expire. */
	cleaner_s_busy, /*%< Currently cleaning. */
	cleaner_s_done	/*%< Freed enough memory after being overmem. */
} cleaner_state_t;

/*
 * Convenience macros for comprehensive assertion checking.
 */
#define CLEANER_IDLE(c) \
	((c)->state == cleaner_s_idle && (c)->resched_event != NULL)
#define CLEANER_BUSY(c)                                           \
	((c)->state == cleaner_s_busy && (c)->iterator != NULL && \
	 (c)->resched_event == NULL)

/*%
 * Accesses to a cache cleaner object are synchronized through
 * task/event serialization, or locked from the cache object.
 */
struct cache_cleaner {
	isc_mutex_t lock;
	/*%<
	 * Locks overmem_event, overmem.  Note: never allocate memory
	 * while holding this lock - that could lead to deadlock since
	 * the lock is take by water() which is called from the memory
	 * allocator.
	 */

	dns_cache_t *cache;
	isc_task_t *task;
	isc_event_t *resched_event; /*% Sent by cleaner task to
				     * itself to reschedule */
	isc_event_t *overmem_event;

	dns_dbiterator_t *iterator;
	unsigned int increment; /*% Number of names to
				 * clean in one increment */
	cleaner_state_t state;	/*% Idle/Busy. */
	bool overmem;		/*% The cache is in an overmem state.
				 * */
	bool replaceiterator;
};

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
	isc_taskmgr_t *taskmgr;
	char *name;
	isc_refcount_t references;
	isc_refcount_t live_tasks;

	/* Locked by 'lock'. */
	dns_rdataclass_t rdclass;
	dns_db_t *db;
	cache_cleaner_t cleaner;
	char *db_type;
	int db_argc;
	char **db_argv;
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
cache_cleaner_init(dns_cache_t *cache, isc_taskmgr_t *taskmgr,
		   isc_timermgr_t *timermgr, cache_cleaner_t *cleaner);

static void
incremental_cleaning_action(isc_task_t *task, isc_event_t *event);

static void
cleaner_shutdown_action(isc_task_t *task, isc_event_t *event);

static void
overmem_cleaning_action(isc_task_t *task, isc_event_t *event);

static void
water(void *arg, int mark);

static isc_result_t
cache_create_db(dns_cache_t *cache, dns_db_t **dbp, isc_mem_t **tmctxp,
		isc_mem_t **hmctxp) {
	isc_result_t result;
	isc_task_t *dbtask = NULL;
	isc_task_t *prunetask = NULL;
	dns_db_t *db = NULL;
	isc_mem_t *tmctx = NULL;
	isc_mem_t *hmctx = NULL;

	REQUIRE(dbp != NULL && *dbp == NULL);
	REQUIRE(tmctxp != NULL && *tmctxp == NULL);
	REQUIRE(hmctxp != NULL && *hmctxp == NULL);

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

	if (strcmp(cache->db_type, "rbt") == 0) {
		cache->db_argv[0] = (char *)hmctx;
	}
	result = dns_db_create(tmctx, cache->db_type, dns_rootname,
			       dns_dbtype_cache, cache->rdclass, cache->db_argc,
			       cache->db_argv, &db);
	if (result != ISC_R_SUCCESS) {
		goto cleanup_mctx;
	}

	dns_db_setservestalettl(db, cache->serve_stale_ttl);
	dns_db_setservestalerefresh(db, cache->serve_stale_refresh);
	dns_db_setmaxrrperset(db, cache->maxrrperset);
	dns_db_setmaxtypepername(db, cache->maxtypepername);

	if (cache->taskmgr == NULL) {
		*dbp = db;
		*tmctxp = tmctx;
		*hmctxp = hmctx;
		return (ISC_R_SUCCESS);
	}

	result = isc_task_create(cache->taskmgr, 1, &dbtask);
	if (result != ISC_R_SUCCESS) {
		goto cleanup_db;
	}
	isc_task_setname(dbtask, "cache_dbtask", NULL);

	result = isc_task_create(cache->taskmgr, UINT_MAX, &prunetask);
	if (result != ISC_R_SUCCESS) {
		goto cleanup_dbtask;
	}
	isc_task_setname(prunetask, "cache_prunetask", NULL);

	dns_db_settask(db, dbtask, prunetask);

	isc_task_detach(&prunetask);
	isc_task_detach(&dbtask);

	*dbp = db;
	*tmctxp = tmctx;
	*hmctxp = hmctx;

	return (ISC_R_SUCCESS);

cleanup_dbtask:
	isc_task_detach(&dbtask);
cleanup_db:
	dns_db_detach(&db);
cleanup_mctx:
	isc_mem_detach(&tmctx);
	isc_mem_detach(&hmctx);

	return (result);
}

static void
cache_free(dns_cache_t *cache) {
	REQUIRE(VALID_CACHE(cache));

	isc_refcount_destroy(&cache->references);
	isc_refcount_destroy(&cache->live_tasks);

	isc_mem_clearwater(cache->tmctx);

	if (cache->cleaner.task != NULL) {
		isc_task_detach(&cache->cleaner.task);
	}

	if (cache->cleaner.overmem_event != NULL) {
		isc_event_free(&cache->cleaner.overmem_event);
	}

	if (cache->cleaner.resched_event != NULL) {
		isc_event_free(&cache->cleaner.resched_event);
	}

	if (cache->cleaner.iterator != NULL) {
		dns_dbiterator_destroy(&cache->cleaner.iterator);
	}

	isc_mutex_destroy(&cache->cleaner.lock);

	if (cache->db != NULL) {
		dns_db_detach(&cache->db);
	}

	if (cache->db_argv != NULL) {
		/*
		 * We don't free db_argv[0] in "rbt" cache databases
		 * as it's a pointer to hmctx
		 */
		int extra = 0;
		if (strcmp(cache->db_type, "rbt") == 0) {
			extra = 1;
			cache->db_argv[0] = NULL;
		}
		for (int i = extra; i < cache->db_argc; i++) {
			if (cache->db_argv[i] != NULL) {
				isc_mem_free(cache->mctx, cache->db_argv[i]);
			}
		}
		isc_mem_put(cache->mctx, cache->db_argv,
			    cache->db_argc * sizeof(char *));
	}

	if (cache->db_type != NULL) {
		isc_mem_free(cache->mctx, cache->db_type);
	}

	if (cache->name != NULL) {
		isc_mem_free(cache->mctx, cache->name);
	}

	if (cache->stats != NULL) {
		isc_stats_detach(&cache->stats);
	}

	if (cache->taskmgr != NULL) {
		isc_taskmgr_detach(&cache->taskmgr);
	}

	isc_mutex_destroy(&cache->lock);

	cache->magic = 0;
	if (cache->hmctx != NULL) {
		isc_mem_detach(&cache->hmctx);
	}
	if (cache->tmctx != NULL) {
		isc_mem_detach(&cache->tmctx);
	}
	isc_mem_putanddetach(&cache->mctx, cache, sizeof(*cache));
}

isc_result_t
dns_cache_create(isc_mem_t *mctx, isc_taskmgr_t *taskmgr,
		 isc_timermgr_t *timermgr, dns_rdataclass_t rdclass,
		 const char *cachename, const char *db_type,
		 unsigned int db_argc, char **db_argv, dns_cache_t **cachep) {
	isc_result_t result;
	dns_cache_t *cache;
	int i, extra = 0;

	REQUIRE(cachep != NULL);
	REQUIRE(*cachep == NULL);
	REQUIRE(mctx != NULL);
	REQUIRE(taskmgr != NULL || strcmp(db_type, "rbt") != 0);
	REQUIRE(cachename != NULL);

	cache = isc_mem_get(mctx, sizeof(*cache));
	*cache = (dns_cache_t){
		.db_type = isc_mem_strdup(mctx, db_type),
		.rdclass = rdclass,
		.db_argc = db_argc,
		.name = cachename == NULL ? NULL
					  : isc_mem_strdup(mctx, cachename),
		.magic = CACHE_MAGIC,
	};

	isc_mutex_init(&cache->lock);
	isc_mem_attach(mctx, &cache->mctx);

	if (taskmgr != NULL) {
		isc_taskmgr_attach(taskmgr, &cache->taskmgr);
	}

	isc_refcount_init(&cache->references, 1);
	isc_refcount_init(&cache->live_tasks, 1);

	result = isc_stats_create(mctx, &cache->stats,
				  dns_cachestatscounter_max);
	if (result != ISC_R_SUCCESS) {
		goto cleanup;
	}

	/*
	 * For databases of type "rbt" we pass hmctx to dns_db_create()
	 * via cache->db_argv, followed by the rest of the arguments in
	 * db_argv (of which there really shouldn't be any).
	 */
	if (strcmp(cache->db_type, "rbt") == 0) {
		extra = 1;
		cache->db_argc++;
	}

	if (cache->db_argc != 0) {
		cache->db_argv = isc_mem_get(mctx,
					     cache->db_argc * sizeof(char *));

		for (i = 0; i < cache->db_argc; i++) {
			cache->db_argv[i] = NULL;
		}

		for (i = extra; i < cache->db_argc; i++) {
			cache->db_argv[i] = isc_mem_strdup(mctx,
							   db_argv[i - extra]);
		}
	}

	/*
	 * Create the database
	 */
	result = cache_create_db(cache, &cache->db, &cache->tmctx,
				 &cache->hmctx);
	if (result != ISC_R_SUCCESS) {
		goto cleanup;
	}

	/*
	 * RBT-type cache DB has its own mechanism of cache cleaning and doesn't
	 * need the control of the generic cleaner.
	 */
	if (strcmp(db_type, "rbt") == 0) {
		result = cache_cleaner_init(cache, NULL, NULL, &cache->cleaner);
	} else {
		result = cache_cleaner_init(cache, taskmgr, timermgr,
					    &cache->cleaner);
	}
	if (result != ISC_R_SUCCESS) {
		goto cleanup;
	}

	result = dns_db_setcachestats(cache->db, cache->stats);
	if (result != ISC_R_SUCCESS) {
		goto cleanup;
	}

	*cachep = cache;
	return (ISC_R_SUCCESS);

cleanup:
	cache_free(cache);
	return (result);
}

void
dns_cache_attach(dns_cache_t *cache, dns_cache_t **targetp) {
	REQUIRE(VALID_CACHE(cache));
	REQUIRE(targetp != NULL && *targetp == NULL);

	isc_refcount_increment(&cache->references);

	*targetp = cache;
}

void
dns_cache_detach(dns_cache_t **cachep) {
	dns_cache_t *cache;

	REQUIRE(cachep != NULL);
	cache = *cachep;
	*cachep = NULL;
	REQUIRE(VALID_CACHE(cache));

	if (isc_refcount_decrement(&cache->references) == 1) {
		cache->cleaner.overmem = false;

		/*
		 * If the cleaner task exists, let it free the cache.
		 */
		if (isc_refcount_decrement(&cache->live_tasks) > 1) {
			isc_task_shutdown(cache->cleaner.task);
		} else {
			cache_free(cache);
		}
	}
}

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

	return (cache->name);
}

/*
 * Initialize the cache cleaner object at *cleaner.
 * Space for the object must be allocated by the caller.
 */

static isc_result_t
cache_cleaner_init(dns_cache_t *cache, isc_taskmgr_t *taskmgr,
		   isc_timermgr_t *timermgr, cache_cleaner_t *cleaner) {
	isc_result_t result;

	isc_mutex_init(&cleaner->lock);

	cleaner->increment = DNS_CACHE_CLEANERINCREMENT;
	cleaner->state = cleaner_s_idle;
	cleaner->cache = cache;
	cleaner->iterator = NULL;
	cleaner->overmem = false;
	cleaner->replaceiterator = false;

	cleaner->task = NULL;
	cleaner->resched_event = NULL;
	cleaner->overmem_event = NULL;

	result = dns_db_createiterator(cleaner->cache->db, false,
				       &cleaner->iterator);
	if (result != ISC_R_SUCCESS) {
		goto cleanup;
	}

	if (taskmgr != NULL && timermgr != NULL) {
		result = isc_task_create(taskmgr, 1, &cleaner->task);
		if (result != ISC_R_SUCCESS) {
			UNEXPECTED_ERROR("isc_task_create() failed: %s",
					 isc_result_totext(result));
			result = ISC_R_UNEXPECTED;
			goto cleanup;
		}
		isc_refcount_increment(&cleaner->cache->live_tasks);
		isc_task_setname(cleaner->task, "cachecleaner", cleaner);

		result = isc_task_onshutdown(cleaner->task,
					     cleaner_shutdown_action, cache);
		if (result != ISC_R_SUCCESS) {
			isc_refcount_decrement0(&cleaner->cache->live_tasks);
			UNEXPECTED_ERROR("cache cleaner: "
					 "isc_task_onshutdown() failed: %s",
					 isc_result_totext(result));
			goto cleanup;
		}

		cleaner->resched_event = isc_event_allocate(
			cache->mctx, cleaner, DNS_EVENT_CACHECLEAN,
			incremental_cleaning_action, cleaner,
			sizeof(isc_event_t));

		cleaner->overmem_event = isc_event_allocate(
			cache->mctx, cleaner, DNS_EVENT_CACHEOVERMEM,
			overmem_cleaning_action, cleaner, sizeof(isc_event_t));
	}

	return (ISC_R_SUCCESS);

cleanup:
	if (cleaner->overmem_event != NULL) {
		isc_event_free(&cleaner->overmem_event);
	}
	if (cleaner->resched_event != NULL) {
		isc_event_free(&cleaner->resched_event);
	}
	if (cleaner->task != NULL) {
		isc_task_detach(&cleaner->task);
	}
	if (cleaner->iterator != NULL) {
		dns_dbiterator_destroy(&cleaner->iterator);
	}
	isc_mutex_destroy(&cleaner->lock);

	return (result);
}

static void
begin_cleaning(cache_cleaner_t *cleaner) {
	isc_result_t result = ISC_R_SUCCESS;

	REQUIRE(CLEANER_IDLE(cleaner));

	/*
	 * Create an iterator, if it does not already exist, and
	 * position it at the beginning of the cache.
	 */
	if (cleaner->iterator == NULL) {
		result = dns_db_createiterator(cleaner->cache->db, false,
					       &cleaner->iterator);
	}
	if (result != ISC_R_SUCCESS) {
		isc_log_write(dns_lctx, DNS_LOGCATEGORY_DATABASE,
			      DNS_LOGMODULE_CACHE, ISC_LOG_WARNING,
			      "cache cleaner could not create "
			      "iterator: %s",
			      isc_result_totext(result));
	} else {
		dns_dbiterator_setcleanmode(cleaner->iterator, true);
		result = dns_dbiterator_first(cleaner->iterator);
	}
	if (result != ISC_R_SUCCESS) {
		/*
		 * If the result is ISC_R_NOMORE, the database is empty,
		 * so there is nothing to be cleaned.
		 */
		if (result != ISC_R_NOMORE && cleaner->iterator != NULL) {
			UNEXPECTED_ERROR("cache cleaner: "
					 "dns_dbiterator_first() failed: %s",
					 isc_result_totext(result));
			dns_dbiterator_destroy(&cleaner->iterator);
		} else if (cleaner->iterator != NULL) {
			result = dns_dbiterator_pause(cleaner->iterator);
			RUNTIME_CHECK(result == ISC_R_SUCCESS);
		}
	} else {
		/*
		 * Pause the iterator to free its lock.
		 */
		result = dns_dbiterator_pause(cleaner->iterator);
		RUNTIME_CHECK(result == ISC_R_SUCCESS);

		isc_log_write(
			dns_lctx, DNS_LOGCATEGORY_DATABASE, DNS_LOGMODULE_CACHE,
			ISC_LOG_DEBUG(1), "begin cache cleaning, mem inuse %lu",
			(unsigned long)isc_mem_inuse(cleaner->cache->mctx));
		cleaner->state = cleaner_s_busy;
		isc_task_send(cleaner->task, &cleaner->resched_event);
	}

	return;
}

static void
end_cleaning(cache_cleaner_t *cleaner, isc_event_t *event) {
	isc_result_t result;

	REQUIRE(CLEANER_BUSY(cleaner));
	REQUIRE(event != NULL);

	result = dns_dbiterator_pause(cleaner->iterator);
	if (result != ISC_R_SUCCESS) {
		dns_dbiterator_destroy(&cleaner->iterator);
	}

	isc_log_write(dns_lctx, DNS_LOGCATEGORY_DATABASE, DNS_LOGMODULE_CACHE,
		      ISC_LOG_DEBUG(1), "end cache cleaning, mem inuse %lu",
		      (unsigned long)isc_mem_inuse(cleaner->cache->mctx));

	cleaner->state = cleaner_s_idle;
	cleaner->resched_event = event;
}

/*
 * This is called when the cache either surpasses its upper limit
 * or shrinks beyond its lower limit.
 */
static void
overmem_cleaning_action(isc_task_t *task, isc_event_t *event) {
	cache_cleaner_t *cleaner = event->ev_arg;
	bool want_cleaning = false;

	UNUSED(task);

	INSIST(task == cleaner->task);
	INSIST(event->ev_type == DNS_EVENT_CACHEOVERMEM);
	INSIST(cleaner->overmem_event == NULL);

	isc_log_write(dns_lctx, DNS_LOGCATEGORY_DATABASE, DNS_LOGMODULE_CACHE,
		      ISC_LOG_DEBUG(1),
		      "overmem_cleaning_action called, "
		      "overmem = %d, state = %d",
		      cleaner->overmem, cleaner->state);

	LOCK(&cleaner->lock);

	if (cleaner->overmem) {
		if (cleaner->state == cleaner_s_idle) {
			want_cleaning = true;
		}
	} else {
		if (cleaner->state == cleaner_s_busy) {
			/*
			 * end_cleaning() can't be called here because
			 * then both cleaner->overmem_event and
			 * cleaner->resched_event will point to this
			 * event.  Set the state to done, and then
			 * when the incremental_cleaning_action() event
			 * is posted, it will handle the end_cleaning.
			 */
			cleaner->state = cleaner_s_done;
		}
	}

	cleaner->overmem_event = event;

	UNLOCK(&cleaner->lock);

	if (want_cleaning) {
		begin_cleaning(cleaner);
	}
}

/*
 * Do incremental cleaning.
 */
static void
incremental_cleaning_action(isc_task_t *task, isc_event_t *event) {
	cache_cleaner_t *cleaner = event->ev_arg;
	isc_result_t result;
	unsigned int n_names;
	isc_time_t start;

	UNUSED(task);

	INSIST(task == cleaner->task);
	INSIST(event->ev_type == DNS_EVENT_CACHECLEAN);

	if (cleaner->state == cleaner_s_done) {
		cleaner->state = cleaner_s_busy;
		end_cleaning(cleaner, event);
		LOCK(&cleaner->cache->lock);
		LOCK(&cleaner->lock);
		if (cleaner->replaceiterator) {
			dns_dbiterator_destroy(&cleaner->iterator);
			(void)dns_db_createiterator(cleaner->cache->db, false,
						    &cleaner->iterator);
			cleaner->replaceiterator = false;
		}
		UNLOCK(&cleaner->lock);
		UNLOCK(&cleaner->cache->lock);
		return;
	}

	INSIST(CLEANER_BUSY(cleaner));

	n_names = cleaner->increment;

	REQUIRE(DNS_DBITERATOR_VALID(cleaner->iterator));

	isc_time_now(&start);
	while (n_names-- > 0) {
		dns_dbnode_t *node = NULL;

		result = dns_dbiterator_current(cleaner->iterator, &node, NULL);
		if (result != ISC_R_SUCCESS) {
			UNEXPECTED_ERROR("cache cleaner: "
					 "dns_dbiterator_current() failed: %s",
					 isc_result_totext(result));

			end_cleaning(cleaner, event);
			return;
		}

		/*
		 * The node was not needed, but was required by
		 * dns_dbiterator_current().  Give up its reference.
		 */
		dns_db_detachnode(cleaner->cache->db, &node);

		/*
		 * Step to the next node.
		 */
		result = dns_dbiterator_next(cleaner->iterator);

		if (result != ISC_R_SUCCESS) {
			/*
			 * Either the end was reached (ISC_R_NOMORE) or
			 * some error was signaled.  If the cache is still
			 * overmem and no error was encountered,
			 * keep trying to clean it, otherwise stop cleaning.
			 */
			if (result != ISC_R_NOMORE) {
				UNEXPECTED_ERROR("cache cleaner: "
						 "dns_dbiterator_next() "
						 "failed: %s",
						 isc_result_totext(result));
			} else if (cleaner->overmem) {
				result =
					dns_dbiterator_first(cleaner->iterator);
				if (result == ISC_R_SUCCESS) {
					isc_log_write(dns_lctx,
						      DNS_LOGCATEGORY_DATABASE,
						      DNS_LOGMODULE_CACHE,
						      ISC_LOG_DEBUG(1),
						      "cache cleaner: "
						      "still overmem, "
						      "reset and try again");
					continue;
				}
			}

			end_cleaning(cleaner, event);
			return;
		}
	}

	/*
	 * We have successfully performed a cleaning increment but have
	 * not gone through the entire cache.  Free the iterator locks
	 * and reschedule another batch.  If it fails, just try to continue
	 * anyway.
	 */
	result = dns_dbiterator_pause(cleaner->iterator);
	RUNTIME_CHECK(result == ISC_R_SUCCESS);

	isc_log_write(dns_lctx, DNS_LOGCATEGORY_DATABASE, DNS_LOGMODULE_CACHE,
		      ISC_LOG_DEBUG(1),
		      "cache cleaner: checked %u nodes, "
		      "mem inuse %lu, sleeping",
		      cleaner->increment,
		      (unsigned long)isc_mem_inuse(cleaner->cache->mctx));

	isc_task_send(task, &event);
	INSIST(CLEANER_BUSY(cleaner));
	return;
}

/*
 * Do immediate cleaning.
 */
isc_result_t
dns_cache_clean(dns_cache_t *cache, isc_stdtime_t now) {
	isc_result_t result;
	dns_dbiterator_t *iterator = NULL;

	REQUIRE(VALID_CACHE(cache));

	result = dns_db_createiterator(cache->db, 0, &iterator);
	if (result != ISC_R_SUCCESS) {
		return (result);
	}

	result = dns_dbiterator_first(iterator);

	while (result == ISC_R_SUCCESS) {
		dns_dbnode_t *node = NULL;
		result = dns_dbiterator_current(iterator, &node,
						(dns_name_t *)NULL);
		if (result != ISC_R_SUCCESS) {
			break;
		}

		/*
		 * Check TTLs, mark expired rdatasets stale.
		 */
		result = dns_db_expirenode(cache->db, node, now);
		if (result != ISC_R_SUCCESS) {
			UNEXPECTED_ERROR("cache cleaner: dns_db_expirenode() "
					 "failed: %s",
					 isc_result_totext(result));
			/*
			 * Continue anyway.
			 */
		}

		/*
		 * This is where the actual freeing takes place.
		 */
		dns_db_detachnode(cache->db, &node);

		result = dns_dbiterator_next(iterator);
	}

	dns_dbiterator_destroy(&iterator);

	if (result == ISC_R_NOMORE) {
		result = ISC_R_SUCCESS;
	}

	return (result);
}

static void
water(void *arg, int mark) {
	dns_cache_t *cache = arg;
	bool overmem = (mark == ISC_MEM_HIWATER);

	REQUIRE(VALID_CACHE(cache));

	LOCK(&cache->cleaner.lock);

	if (overmem != cache->cleaner.overmem) {
		dns_db_overmem(cache->db, overmem);
		cache->cleaner.overmem = overmem;
		isc_mem_waterack(cache->tmctx, mark);
	}

	if (cache->cleaner.overmem_event != NULL) {
		isc_task_send(cache->cleaner.task,
			      &cache->cleaner.overmem_event);
	}

	UNLOCK(&cache->cleaner.lock);
}

static void
updatewater(dns_cache_t *cache) {
	size_t hi = cache->size - (cache->size >> 3); /* ~ 7/8ths. */
	size_t lo = cache->size - (cache->size >> 2); /* ~ 3/4ths. */
	if (cache->size == 0U || hi == 0U || lo == 0U) {
		isc_mem_clearwater(cache->tmctx);
	} else {
		isc_mem_setwater(cache->tmctx, water, cache, hi, lo);
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

	return (size);
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
	return (result == ISC_R_SUCCESS ? ttl : 0);
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
	return (result == ISC_R_SUCCESS ? interval : 0);
}

/*
 * The cleaner task is shutting down; do the necessary cleanup.
 */
static void
cleaner_shutdown_action(isc_task_t *task, isc_event_t *event) {
	dns_cache_t *cache = event->ev_arg;

	UNUSED(task);

	INSIST(task == cache->cleaner.task);
	INSIST(event->ev_type == ISC_TASKEVENT_SHUTDOWN);

	if (CLEANER_BUSY(&cache->cleaner)) {
		end_cleaning(&cache->cleaner, event);
	} else {
		isc_event_free(&event);
	}

	/* Make sure we don't reschedule anymore. */
	(void)isc_task_purge(task, NULL, DNS_EVENT_CACHECLEAN, NULL);

	isc_refcount_decrementz(&cache->live_tasks);

	cache_free(cache);
}

isc_result_t
dns_cache_flush(dns_cache_t *cache) {
	dns_db_t *db = NULL, *olddb;
	dns_dbiterator_t *dbiterator = NULL, *olddbiterator = NULL;
	isc_result_t result;
	isc_mem_t *tmctx = NULL, *oldtmctx;
	isc_mem_t *hmctx = NULL, *oldhmctx;

	result = cache_create_db(cache, &db, &tmctx, &hmctx);
	if (result != ISC_R_SUCCESS) {
		return (result);
	}

	result = dns_db_createiterator(db, false, &dbiterator);
	if (result != ISC_R_SUCCESS) {
		dns_db_detach(&db);
		isc_mem_detach(&tmctx);
		isc_mem_detach(&hmctx);
		return (result);
	}

	LOCK(&cache->lock);
	LOCK(&cache->cleaner.lock);
	isc_mem_clearwater(cache->tmctx);
	if (cache->cleaner.state == cleaner_s_idle) {
		olddbiterator = cache->cleaner.iterator;
		cache->cleaner.iterator = dbiterator;
		dbiterator = NULL;
	} else {
		if (cache->cleaner.state == cleaner_s_busy) {
			cache->cleaner.state = cleaner_s_done;
		}
		cache->cleaner.replaceiterator = true;
	}
	oldhmctx = cache->hmctx;
	cache->hmctx = hmctx;
	oldtmctx = cache->tmctx;
	cache->tmctx = tmctx;
	updatewater(cache);
	olddb = cache->db;
	cache->db = db;
	dns_db_setcachestats(cache->db, cache->stats);
	UNLOCK(&cache->cleaner.lock);
	UNLOCK(&cache->lock);

	if (dbiterator != NULL) {
		dns_dbiterator_destroy(&dbiterator);
	}
	if (olddbiterator != NULL) {
		dns_dbiterator_destroy(&olddbiterator);
	}
	dns_db_detach(&olddb);
	isc_mem_detach(&oldhmctx);
	isc_mem_detach(&oldtmctx);

	return (ISC_R_SUCCESS);
}

static isc_result_t
clearnode(dns_db_t *db, dns_dbnode_t *node) {
	isc_result_t result;
	dns_rdatasetiter_t *iter = NULL;

	result = dns_db_allrdatasets(db, node, NULL, DNS_DB_STALEOK,
				     (isc_stdtime_t)0, &iter);
	if (result != ISC_R_SUCCESS) {
		return (result);
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
	return (result);
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

	return (answer);
}

isc_result_t
dns_cache_flushname(dns_cache_t *cache, const dns_name_t *name) {
	return (dns_cache_flushnode(cache, name, false));
}

isc_result_t
dns_cache_flushnode(dns_cache_t *cache, const dns_name_t *name, bool tree) {
	isc_result_t result;
	dns_dbnode_t *node = NULL;
	dns_db_t *db = NULL;

	if (tree && dns_name_equal(name, dns_rootname)) {
		return (dns_cache_flush(cache));
	}

	LOCK(&cache->lock);
	if (cache->db != NULL) {
		dns_db_attach(cache->db, &db);
	}
	UNLOCK(&cache->lock);
	if (db == NULL) {
		return (ISC_R_SUCCESS);
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
	return (result);
}

isc_stats_t *
dns_cache_getstats(dns_cache_t *cache) {
	REQUIRE(VALID_CACHE(cache));
	return (cache->stats);
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

	fprintf(fp, "%20" PRIu64 " %s\n", (uint64_t)isc_mem_total(cache->tmctx),
		"cache tree memory total");
	fprintf(fp, "%20" PRIu64 " %s\n", (uint64_t)isc_mem_inuse(cache->tmctx),
		"cache tree memory in use");
	fprintf(fp, "%20" PRIu64 " %s\n",
		(uint64_t)isc_mem_maxinuse(cache->tmctx),
		"cache tree highest memory in use");

	fprintf(fp, "%20" PRIu64 " %s\n", (uint64_t)isc_mem_total(cache->hmctx),
		"cache heap memory total");
	fprintf(fp, "%20" PRIu64 " %s\n", (uint64_t)isc_mem_inuse(cache->hmctx),
		"cache heap memory in use");
	fprintf(fp, "%20" PRIu64 " %s\n",
		(uint64_t)isc_mem_maxinuse(cache->hmctx),
		"cache heap highest memory in use");
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
	return (xmlrc);
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

	TRY0(renderstat("TreeMemTotal", isc_mem_total(cache->tmctx), writer));
	TRY0(renderstat("TreeMemInUse", isc_mem_inuse(cache->tmctx), writer));
	TRY0(renderstat("TreeMemMax", isc_mem_maxinuse(cache->tmctx), writer));

	TRY0(renderstat("HeapMemTotal", isc_mem_total(cache->hmctx), writer));
	TRY0(renderstat("HeapMemInUse", isc_mem_inuse(cache->hmctx), writer));
	TRY0(renderstat("HeapMemMax", isc_mem_maxinuse(cache->hmctx), writer));
error:
	return (xmlrc);
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

	obj = json_object_new_int64(isc_mem_total(cache->tmctx));
	CHECKMEM(obj);
	json_object_object_add(cstats, "TreeMemTotal", obj);

	obj = json_object_new_int64(isc_mem_inuse(cache->tmctx));
	CHECKMEM(obj);
	json_object_object_add(cstats, "TreeMemInUse", obj);

	obj = json_object_new_int64(isc_mem_maxinuse(cache->tmctx));
	CHECKMEM(obj);
	json_object_object_add(cstats, "TreeMemMax", obj);

	obj = json_object_new_int64(isc_mem_total(cache->hmctx));
	CHECKMEM(obj);
	json_object_object_add(cstats, "HeapMemTotal", obj);

	obj = json_object_new_int64(isc_mem_inuse(cache->hmctx));
	CHECKMEM(obj);
	json_object_object_add(cstats, "HeapMemInUse", obj);

	obj = json_object_new_int64(isc_mem_maxinuse(cache->hmctx));
	CHECKMEM(obj);
	json_object_object_add(cstats, "HeapMemMax", obj);

	result = ISC_R_SUCCESS;
error:
	return (result);
}
#endif /* ifdef HAVE_JSON_C */
