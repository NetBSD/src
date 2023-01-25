/*	$NetBSD: view.c,v 1.13 2023/01/25 21:43:30 christos Exp $	*/

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

#ifdef HAVE_LMDB
#include <lmdb.h>

#include <dns/lmdb.h>
#endif /* HAVE_LMDB */

#include <isc/atomic.h>
#include <isc/file.h>
#include <isc/hash.h>
#include <isc/lex.h>
#include <isc/print.h>
#include <isc/stats.h>
#include <isc/string.h> /* Required for HP/UX (and others?) */
#include <isc/task.h>
#include <isc/util.h>

#include <dns/acl.h>
#include <dns/adb.h>
#include <dns/badcache.h>
#include <dns/cache.h>
#include <dns/db.h>
#include <dns/dispatch.h>
#include <dns/dlz.h>
#include <dns/dns64.h>
#include <dns/dnssec.h>
#include <dns/events.h>
#include <dns/forward.h>
#include <dns/keytable.h>
#include <dns/keyvalues.h>
#include <dns/master.h>
#include <dns/masterdump.h>
#include <dns/nta.h>
#include <dns/order.h>
#include <dns/peer.h>
#include <dns/rbt.h>
#include <dns/rdataset.h>
#include <dns/request.h>
#include <dns/resolver.h>
#include <dns/result.h>
#include <dns/rpz.h>
#include <dns/rrl.h>
#include <dns/stats.h>
#include <dns/time.h>
#include <dns/tsig.h>
#include <dns/zone.h>
#include <dns/zt.h>

#define CHECK(op)                            \
	do {                                 \
		result = (op);               \
		if (result != ISC_R_SUCCESS) \
			goto cleanup;        \
	} while (0)

#define RESSHUTDOWN(v) \
	((atomic_load(&(v)->attributes) & DNS_VIEWATTR_RESSHUTDOWN) != 0)
#define ADBSHUTDOWN(v) \
	((atomic_load(&(v)->attributes) & DNS_VIEWATTR_ADBSHUTDOWN) != 0)
#define REQSHUTDOWN(v) \
	((atomic_load(&(v)->attributes) & DNS_VIEWATTR_REQSHUTDOWN) != 0)

#define DNS_VIEW_DELONLYHASH   111
#define DNS_VIEW_FAILCACHESIZE 1021

static void
resolver_shutdown(isc_task_t *task, isc_event_t *event);
static void
adb_shutdown(isc_task_t *task, isc_event_t *event);
static void
req_shutdown(isc_task_t *task, isc_event_t *event);

isc_result_t
dns_view_create(isc_mem_t *mctx, dns_rdataclass_t rdclass, const char *name,
		dns_view_t **viewp) {
	dns_view_t *view;
	isc_result_t result;
	char buffer[1024];

	/*
	 * Create a view.
	 */

	REQUIRE(name != NULL);
	REQUIRE(viewp != NULL && *viewp == NULL);

	view = isc_mem_get(mctx, sizeof(*view));

	view->nta_file = NULL;
	view->mctx = NULL;
	isc_mem_attach(mctx, &view->mctx);
	view->name = isc_mem_strdup(mctx, name);

	result = isc_file_sanitize(NULL, view->name, "nta", buffer,
				   sizeof(buffer));
	if (result != ISC_R_SUCCESS) {
		goto cleanup_name;
	}
	view->nta_file = isc_mem_strdup(mctx, buffer);

	isc_mutex_init(&view->lock);

	view->zonetable = NULL;
	result = dns_zt_create(mctx, rdclass, &view->zonetable);
	if (result != ISC_R_SUCCESS) {
		UNEXPECTED_ERROR(__FILE__, __LINE__,
				 "dns_zt_create() failed: %s",
				 isc_result_totext(result));
		result = ISC_R_UNEXPECTED;
		goto cleanup_mutex;
	}

	view->secroots_priv = NULL;
	view->ntatable_priv = NULL;
	view->fwdtable = NULL;
	result = dns_fwdtable_create(mctx, &view->fwdtable);
	if (result != ISC_R_SUCCESS) {
		UNEXPECTED_ERROR(__FILE__, __LINE__,
				 "dns_fwdtable_create() failed: %s",
				 isc_result_totext(result));
		result = ISC_R_UNEXPECTED;
		goto cleanup_zt;
	}

	view->cache = NULL;
	view->cachedb = NULL;
	ISC_LIST_INIT(view->dlz_searched);
	ISC_LIST_INIT(view->dlz_unsearched);
	view->hints = NULL;
	view->resolver = NULL;
	view->adb = NULL;
	view->requestmgr = NULL;
	view->rdclass = rdclass;
	view->frozen = false;
	view->task = NULL;
	isc_refcount_init(&view->references, 1);
	isc_refcount_init(&view->weakrefs, 1);
	atomic_init(&view->attributes,
		    (DNS_VIEWATTR_RESSHUTDOWN | DNS_VIEWATTR_ADBSHUTDOWN |
		     DNS_VIEWATTR_REQSHUTDOWN));
	view->statickeys = NULL;
	view->dynamickeys = NULL;
	view->matchclients = NULL;
	view->matchdestinations = NULL;
	view->matchrecursiveonly = false;
	result = dns_tsigkeyring_create(view->mctx, &view->dynamickeys);
	if (result != ISC_R_SUCCESS) {
		goto cleanup_weakrefs;
	}
	view->peers = NULL;
	view->order = NULL;
	view->delonly = NULL;
	view->rootdelonly = false;
	view->rootexclude = NULL;
	view->adbstats = NULL;
	view->resstats = NULL;
	view->resquerystats = NULL;
	view->cacheshared = false;
	ISC_LIST_INIT(view->dns64);
	view->dns64cnt = 0;

	/*
	 * Initialize configuration data with default values.
	 */
	view->recursion = true;
	view->qminimization = false;
	view->qmin_strict = false;
	view->auth_nxdomain = false; /* Was true in BIND 8 */
	view->enablevalidation = true;
	view->acceptexpired = false;
	view->use_glue_cache = false;
	view->minimal_any = false;
	view->minimalresponses = dns_minimal_no;
	view->transfer_format = dns_one_answer;
	view->cacheacl = NULL;
	view->cacheonacl = NULL;
	view->checknames = false;
	view->queryacl = NULL;
	view->queryonacl = NULL;
	view->recursionacl = NULL;
	view->recursiononacl = NULL;
	view->sortlist = NULL;
	view->transferacl = NULL;
	view->notifyacl = NULL;
	view->updateacl = NULL;
	view->upfwdacl = NULL;
	view->denyansweracl = NULL;
	view->nocasecompress = NULL;
	view->msgcompression = true;
	view->answeracl_exclude = NULL;
	view->denyanswernames = NULL;
	view->answernames_exclude = NULL;
	view->rrl = NULL;
	view->provideixfr = true;
	view->maxcachettl = 7 * 24 * 3600;
	view->maxncachettl = 3 * 3600;
	view->mincachettl = 0;
	view->minncachettl = 0;
	view->nta_lifetime = 0;
	view->nta_recheck = 0;
	view->prefetch_eligible = 0;
	view->prefetch_trigger = 0;
	view->dstport = 53;
	view->preferred_glue = 0;
	view->flush = false;
	view->maxudp = 0;
	view->staleanswerttl = 1;
	view->staleanswersok = dns_stale_answer_conf;
	view->staleanswersenable = false;
	view->nocookieudp = 0;
	view->padding = 0;
	view->pad_acl = NULL;
	view->maxbits = 0;
	view->rpzs = NULL;
	view->catzs = NULL;
	view->managed_keys = NULL;
	view->redirect = NULL;
	view->redirectzone = NULL;
	dns_fixedname_init(&view->redirectfixed);
	view->requestnsid = false;
	view->sendcookie = true;
	view->requireservercookie = false;
	view->synthfromdnssec = true;
	view->trust_anchor_telemetry = true;
	view->root_key_sentinel = true;
	view->new_zone_dir = NULL;
	view->new_zone_file = NULL;
	view->new_zone_db = NULL;
	view->new_zone_dbenv = NULL;
	view->new_zone_mapsize = 0ULL;
	view->new_zone_config = NULL;
	view->cfg_destroy = NULL;
	view->fail_ttl = 0;
	view->failcache = NULL;
	result = dns_badcache_init(view->mctx, DNS_VIEW_FAILCACHESIZE,
				   &view->failcache);
	if (result != ISC_R_SUCCESS) {
		goto cleanup_dynkeys;
	}
	view->v6bias = 0;
	view->dtenv = NULL;
	view->dttypes = 0;

	view->plugins = NULL;
	view->plugins_free = NULL;
	view->hooktable = NULL;
	view->hooktable_free = NULL;

	isc_mutex_init(&view->new_zone_lock);

	result = dns_order_create(view->mctx, &view->order);
	if (result != ISC_R_SUCCESS) {
		goto cleanup_new_zone_lock;
	}

	result = dns_peerlist_new(view->mctx, &view->peers);
	if (result != ISC_R_SUCCESS) {
		goto cleanup_order;
	}

	result = dns_aclenv_init(view->mctx, &view->aclenv);
	if (result != ISC_R_SUCCESS) {
		goto cleanup_peerlist;
	}

	ISC_LINK_INIT(view, link);
	ISC_EVENT_INIT(&view->resevent, sizeof(view->resevent), 0, NULL,
		       DNS_EVENT_VIEWRESSHUTDOWN, resolver_shutdown, view, NULL,
		       NULL, NULL);
	ISC_EVENT_INIT(&view->adbevent, sizeof(view->adbevent), 0, NULL,
		       DNS_EVENT_VIEWADBSHUTDOWN, adb_shutdown, view, NULL,
		       NULL, NULL);
	ISC_EVENT_INIT(&view->reqevent, sizeof(view->reqevent), 0, NULL,
		       DNS_EVENT_VIEWREQSHUTDOWN, req_shutdown, view, NULL,
		       NULL, NULL);
	view->viewlist = NULL;
	view->magic = DNS_VIEW_MAGIC;

	*viewp = view;

	return (ISC_R_SUCCESS);

cleanup_peerlist:
	if (view->peers != NULL) {
		dns_peerlist_detach(&view->peers);
	}

cleanup_order:
	if (view->order != NULL) {
		dns_order_detach(&view->order);
	}

cleanup_new_zone_lock:
	isc_mutex_destroy(&view->new_zone_lock);

	dns_badcache_destroy(&view->failcache);

cleanup_dynkeys:
	if (view->dynamickeys != NULL) {
		dns_tsigkeyring_detach(&view->dynamickeys);
	}

cleanup_weakrefs:
	isc_refcount_decrementz(&view->weakrefs);
	isc_refcount_destroy(&view->weakrefs);

	isc_refcount_decrementz(&view->references);
	isc_refcount_destroy(&view->references);

	if (view->fwdtable != NULL) {
		dns_fwdtable_destroy(&view->fwdtable);
	}

cleanup_zt:
	if (view->zonetable != NULL) {
		dns_zt_detach(&view->zonetable);
	}

cleanup_mutex:
	isc_mutex_destroy(&view->lock);

	if (view->nta_file != NULL) {
		isc_mem_free(mctx, view->nta_file);
	}

cleanup_name:
	isc_mem_free(mctx, view->name);
	isc_mem_putanddetach(&view->mctx, view, sizeof(*view));

	return (result);
}

static void
destroy(dns_view_t *view) {
	dns_dns64_t *dns64;
	dns_dlzdb_t *dlzdb;

	REQUIRE(!ISC_LINK_LINKED(view, link));
	REQUIRE(RESSHUTDOWN(view));
	REQUIRE(ADBSHUTDOWN(view));
	REQUIRE(REQSHUTDOWN(view));

	isc_refcount_destroy(&view->references);
	isc_refcount_destroy(&view->weakrefs);

	if (view->order != NULL) {
		dns_order_detach(&view->order);
	}
	if (view->peers != NULL) {
		dns_peerlist_detach(&view->peers);
	}

	if (view->dynamickeys != NULL) {
		isc_result_t result;
		char template[PATH_MAX];
		char keyfile[PATH_MAX];
		FILE *fp = NULL;

		result = isc_file_mktemplate(NULL, template, sizeof(template));
		if (result == ISC_R_SUCCESS) {
			(void)isc_file_openuniqueprivate(template, &fp);
		}
		if (fp == NULL) {
			dns_tsigkeyring_detach(&view->dynamickeys);
		} else {
			result = dns_tsigkeyring_dumpanddetach(
				&view->dynamickeys, fp);
			if (result == ISC_R_SUCCESS) {
				if (fclose(fp) == 0) {
					result = isc_file_sanitize(
						NULL, view->name, "tsigkeys",
						keyfile, sizeof(keyfile));
					if (result == ISC_R_SUCCESS) {
						result = isc_file_rename(
							template, keyfile);
					}
				}
				if (result != ISC_R_SUCCESS) {
					(void)remove(template);
				}
			} else {
				(void)fclose(fp);
				(void)remove(template);
			}
		}
	}
	if (view->statickeys != NULL) {
		dns_tsigkeyring_detach(&view->statickeys);
	}
	if (view->adb != NULL) {
		dns_adb_detach(&view->adb);
	}
	if (view->resolver != NULL) {
		dns_resolver_detach(&view->resolver);
	}
	dns_rrl_view_destroy(view);
	if (view->rpzs != NULL) {
		dns_rpz_detach_rpzs(&view->rpzs);
	}
	if (view->catzs != NULL) {
		dns_catz_catzs_detach(&view->catzs);
	}
	for (dlzdb = ISC_LIST_HEAD(view->dlz_searched); dlzdb != NULL;
	     dlzdb = ISC_LIST_HEAD(view->dlz_searched))
	{
		ISC_LIST_UNLINK(view->dlz_searched, dlzdb, link);
		dns_dlzdestroy(&dlzdb);
	}
	for (dlzdb = ISC_LIST_HEAD(view->dlz_unsearched); dlzdb != NULL;
	     dlzdb = ISC_LIST_HEAD(view->dlz_unsearched))
	{
		ISC_LIST_UNLINK(view->dlz_unsearched, dlzdb, link);
		dns_dlzdestroy(&dlzdb);
	}
	if (view->requestmgr != NULL) {
		dns_requestmgr_detach(&view->requestmgr);
	}
	if (view->task != NULL) {
		isc_task_detach(&view->task);
	}
	if (view->hints != NULL) {
		dns_db_detach(&view->hints);
	}
	if (view->cachedb != NULL) {
		dns_db_detach(&view->cachedb);
	}
	if (view->cache != NULL) {
		dns_cache_detach(&view->cache);
	}
	if (view->nocasecompress != NULL) {
		dns_acl_detach(&view->nocasecompress);
	}
	if (view->matchclients != NULL) {
		dns_acl_detach(&view->matchclients);
	}
	if (view->matchdestinations != NULL) {
		dns_acl_detach(&view->matchdestinations);
	}
	if (view->cacheacl != NULL) {
		dns_acl_detach(&view->cacheacl);
	}
	if (view->cacheonacl != NULL) {
		dns_acl_detach(&view->cacheonacl);
	}
	if (view->queryacl != NULL) {
		dns_acl_detach(&view->queryacl);
	}
	if (view->queryonacl != NULL) {
		dns_acl_detach(&view->queryonacl);
	}
	if (view->recursionacl != NULL) {
		dns_acl_detach(&view->recursionacl);
	}
	if (view->recursiononacl != NULL) {
		dns_acl_detach(&view->recursiononacl);
	}
	if (view->sortlist != NULL) {
		dns_acl_detach(&view->sortlist);
	}
	if (view->transferacl != NULL) {
		dns_acl_detach(&view->transferacl);
	}
	if (view->notifyacl != NULL) {
		dns_acl_detach(&view->notifyacl);
	}
	if (view->updateacl != NULL) {
		dns_acl_detach(&view->updateacl);
	}
	if (view->upfwdacl != NULL) {
		dns_acl_detach(&view->upfwdacl);
	}
	if (view->denyansweracl != NULL) {
		dns_acl_detach(&view->denyansweracl);
	}
	if (view->pad_acl != NULL) {
		dns_acl_detach(&view->pad_acl);
	}
	if (view->answeracl_exclude != NULL) {
		dns_rbt_destroy(&view->answeracl_exclude);
	}
	if (view->denyanswernames != NULL) {
		dns_rbt_destroy(&view->denyanswernames);
	}
	if (view->answernames_exclude != NULL) {
		dns_rbt_destroy(&view->answernames_exclude);
	}
	if (view->delonly != NULL) {
		dns_name_t *name;
		int i;

		for (i = 0; i < DNS_VIEW_DELONLYHASH; i++) {
			name = ISC_LIST_HEAD(view->delonly[i]);
			while (name != NULL) {
				ISC_LIST_UNLINK(view->delonly[i], name, link);
				dns_name_free(name, view->mctx);
				isc_mem_put(view->mctx, name, sizeof(*name));
				name = ISC_LIST_HEAD(view->delonly[i]);
			}
		}
		isc_mem_put(view->mctx, view->delonly,
			    sizeof(dns_namelist_t) * DNS_VIEW_DELONLYHASH);
		view->delonly = NULL;
	}
	if (view->rootexclude != NULL) {
		dns_name_t *name;
		int i;

		for (i = 0; i < DNS_VIEW_DELONLYHASH; i++) {
			name = ISC_LIST_HEAD(view->rootexclude[i]);
			while (name != NULL) {
				ISC_LIST_UNLINK(view->rootexclude[i], name,
						link);
				dns_name_free(name, view->mctx);
				isc_mem_put(view->mctx, name, sizeof(*name));
				name = ISC_LIST_HEAD(view->rootexclude[i]);
			}
		}
		isc_mem_put(view->mctx, view->rootexclude,
			    sizeof(dns_namelist_t) * DNS_VIEW_DELONLYHASH);
		view->rootexclude = NULL;
	}
	if (view->adbstats != NULL) {
		isc_stats_detach(&view->adbstats);
	}
	if (view->resstats != NULL) {
		isc_stats_detach(&view->resstats);
	}
	if (view->resquerystats != NULL) {
		dns_stats_detach(&view->resquerystats);
	}
	if (view->secroots_priv != NULL) {
		dns_keytable_detach(&view->secroots_priv);
	}
	if (view->ntatable_priv != NULL) {
		dns_ntatable_detach(&view->ntatable_priv);
	}
	for (dns64 = ISC_LIST_HEAD(view->dns64); dns64 != NULL;
	     dns64 = ISC_LIST_HEAD(view->dns64))
	{
		dns_dns64_unlink(&view->dns64, dns64);
		dns_dns64_destroy(&dns64);
	}
	if (view->managed_keys != NULL) {
		dns_zone_detach(&view->managed_keys);
	}
	if (view->redirect != NULL) {
		dns_zone_detach(&view->redirect);
	}
#ifdef HAVE_DNSTAP
	if (view->dtenv != NULL) {
		dns_dt_detach(&view->dtenv);
	}
#endif /* HAVE_DNSTAP */
	dns_view_setnewzones(view, false, NULL, NULL, 0ULL);
	if (view->new_zone_file != NULL) {
		isc_mem_free(view->mctx, view->new_zone_file);
		view->new_zone_file = NULL;
	}
	if (view->new_zone_dir != NULL) {
		isc_mem_free(view->mctx, view->new_zone_dir);
		view->new_zone_dir = NULL;
	}
#ifdef HAVE_LMDB
	if (view->new_zone_dbenv != NULL) {
		mdb_env_close((MDB_env *)view->new_zone_dbenv);
		view->new_zone_dbenv = NULL;
	}
	if (view->new_zone_db != NULL) {
		isc_mem_free(view->mctx, view->new_zone_db);
		view->new_zone_db = NULL;
	}
#endif /* HAVE_LMDB */
	dns_fwdtable_destroy(&view->fwdtable);
	dns_aclenv_destroy(&view->aclenv);
	if (view->failcache != NULL) {
		dns_badcache_destroy(&view->failcache);
	}
	isc_mutex_destroy(&view->new_zone_lock);
	isc_mutex_destroy(&view->lock);
	isc_refcount_destroy(&view->references);
	isc_refcount_destroy(&view->weakrefs);
	isc_mem_free(view->mctx, view->nta_file);
	isc_mem_free(view->mctx, view->name);
	if (view->hooktable != NULL && view->hooktable_free != NULL) {
		view->hooktable_free(view->mctx, &view->hooktable);
	}
	if (view->plugins != NULL && view->plugins_free != NULL) {
		view->plugins_free(view->mctx, &view->plugins);
	}
	isc_mem_putanddetach(&view->mctx, view, sizeof(*view));
}

void
dns_view_attach(dns_view_t *source, dns_view_t **targetp) {
	REQUIRE(DNS_VIEW_VALID(source));
	REQUIRE(targetp != NULL && *targetp == NULL);

	isc_refcount_increment(&source->references);

	*targetp = source;
}

static void
view_flushanddetach(dns_view_t **viewp, bool flush) {
	REQUIRE(viewp != NULL && DNS_VIEW_VALID(*viewp));
	dns_view_t *view = *viewp;
	*viewp = NULL;

	if (flush) {
		view->flush = flush;
	}

	if (isc_refcount_decrement(&view->references) == 1) {
		dns_zone_t *mkzone = NULL, *rdzone = NULL;

		isc_refcount_destroy(&view->references);
		if (!RESSHUTDOWN(view)) {
			dns_resolver_shutdown(view->resolver);
		}
		if (!ADBSHUTDOWN(view)) {
			dns_adb_shutdown(view->adb);
		}
		if (!REQSHUTDOWN(view)) {
			dns_requestmgr_shutdown(view->requestmgr);
		}
		LOCK(&view->lock);
		if (view->zonetable != NULL) {
			if (view->flush) {
				dns_zt_flushanddetach(&view->zonetable);
			} else {
				dns_zt_detach(&view->zonetable);
			}
		}
		if (view->managed_keys != NULL) {
			mkzone = view->managed_keys;
			view->managed_keys = NULL;
			if (view->flush) {
				dns_zone_flush(mkzone);
			}
		}
		if (view->redirect != NULL) {
			rdzone = view->redirect;
			view->redirect = NULL;
			if (view->flush) {
				dns_zone_flush(rdzone);
			}
		}
		if (view->catzs != NULL) {
			dns_catz_catzs_detach(&view->catzs);
		}
		if (view->ntatable_priv != NULL) {
			dns_ntatable_shutdown(view->ntatable_priv);
		}
		UNLOCK(&view->lock);

		/* Need to detach zones outside view lock */
		if (mkzone != NULL) {
			dns_zone_detach(&mkzone);
		}

		if (rdzone != NULL) {
			dns_zone_detach(&rdzone);
		}

		dns_view_weakdetach(&view);
	}
}

void
dns_view_flushanddetach(dns_view_t **viewp) {
	view_flushanddetach(viewp, true);
}

void
dns_view_detach(dns_view_t **viewp) {
	view_flushanddetach(viewp, false);
}

static isc_result_t
dialup(dns_zone_t *zone, void *dummy) {
	UNUSED(dummy);
	dns_zone_dialup(zone);
	return (ISC_R_SUCCESS);
}

void
dns_view_dialup(dns_view_t *view) {
	REQUIRE(DNS_VIEW_VALID(view));
	REQUIRE(view->zonetable != NULL);

	(void)dns_zt_apply(view->zonetable, isc_rwlocktype_read, false, NULL,
			   dialup, NULL);
}

void
dns_view_weakattach(dns_view_t *source, dns_view_t **targetp) {
	REQUIRE(DNS_VIEW_VALID(source));
	REQUIRE(targetp != NULL && *targetp == NULL);

	isc_refcount_increment(&source->weakrefs);

	*targetp = source;
}

void
dns_view_weakdetach(dns_view_t **viewp) {
	dns_view_t *view;

	REQUIRE(viewp != NULL);
	view = *viewp;
	*viewp = NULL;
	REQUIRE(DNS_VIEW_VALID(view));

	if (isc_refcount_decrement(&view->weakrefs) == 1) {
		destroy(view);
	}
}

static void
resolver_shutdown(isc_task_t *task, isc_event_t *event) {
	dns_view_t *view = event->ev_arg;

	REQUIRE(event->ev_type == DNS_EVENT_VIEWRESSHUTDOWN);
	REQUIRE(DNS_VIEW_VALID(view));
	REQUIRE(view->task == task);

	UNUSED(task);

	isc_event_free(&event);

	atomic_fetch_or(&view->attributes, DNS_VIEWATTR_RESSHUTDOWN);
	dns_view_weakdetach(&view);
}

static void
adb_shutdown(isc_task_t *task, isc_event_t *event) {
	dns_view_t *view = event->ev_arg;

	REQUIRE(event->ev_type == DNS_EVENT_VIEWADBSHUTDOWN);
	REQUIRE(DNS_VIEW_VALID(view));
	REQUIRE(view->task == task);

	UNUSED(task);

	isc_event_free(&event);

	atomic_fetch_or(&view->attributes, DNS_VIEWATTR_ADBSHUTDOWN);

	dns_view_weakdetach(&view);
}

static void
req_shutdown(isc_task_t *task, isc_event_t *event) {
	dns_view_t *view = event->ev_arg;

	REQUIRE(event->ev_type == DNS_EVENT_VIEWREQSHUTDOWN);
	REQUIRE(DNS_VIEW_VALID(view));
	REQUIRE(view->task == task);

	UNUSED(task);

	isc_event_free(&event);

	atomic_fetch_or(&view->attributes, DNS_VIEWATTR_REQSHUTDOWN);

	dns_view_weakdetach(&view);
}

isc_result_t
dns_view_createzonetable(dns_view_t *view) {
	REQUIRE(DNS_VIEW_VALID(view));
	REQUIRE(!view->frozen);
	REQUIRE(view->zonetable == NULL);

	return (dns_zt_create(view->mctx, view->rdclass, &view->zonetable));
}

isc_result_t
dns_view_createresolver(dns_view_t *view, isc_taskmgr_t *taskmgr,
			unsigned int ntasks, unsigned int ndisp,
			isc_socketmgr_t *socketmgr, isc_timermgr_t *timermgr,
			unsigned int options, dns_dispatchmgr_t *dispatchmgr,
			dns_dispatch_t *dispatchv4,
			dns_dispatch_t *dispatchv6) {
	isc_result_t result;
	isc_event_t *event;
	isc_mem_t *mctx = NULL;

	REQUIRE(DNS_VIEW_VALID(view));
	REQUIRE(!view->frozen);
	REQUIRE(view->resolver == NULL);

	result = isc_task_create(taskmgr, 0, &view->task);
	if (result != ISC_R_SUCCESS) {
		return (result);
	}
	isc_task_setname(view->task, "view", view);

	result = dns_resolver_create(view, taskmgr, ntasks, ndisp, socketmgr,
				     timermgr, options, dispatchmgr, dispatchv4,
				     dispatchv6, &view->resolver);
	if (result != ISC_R_SUCCESS) {
		isc_task_detach(&view->task);
		return (result);
	}
	event = &view->resevent;
	dns_resolver_whenshutdown(view->resolver, view->task, &event);
	atomic_fetch_and(&view->attributes, ~DNS_VIEWATTR_RESSHUTDOWN);
	isc_refcount_increment(&view->weakrefs);

	isc_mem_create(&mctx);

	result = dns_adb_create(mctx, view, timermgr, taskmgr, &view->adb);
	isc_mem_setname(mctx, "ADB", NULL);
	isc_mem_detach(&mctx);
	if (result != ISC_R_SUCCESS) {
		dns_resolver_shutdown(view->resolver);
		return (result);
	}
	event = &view->adbevent;
	dns_adb_whenshutdown(view->adb, view->task, &event);
	atomic_fetch_and(&view->attributes, ~DNS_VIEWATTR_ADBSHUTDOWN);
	isc_refcount_increment(&view->weakrefs);

	result = dns_requestmgr_create(view->mctx, timermgr, socketmgr,
				       dns_resolver_taskmgr(view->resolver),
				       dns_resolver_dispatchmgr(view->resolver),
				       dispatchv4, dispatchv6,
				       &view->requestmgr);
	if (result != ISC_R_SUCCESS) {
		dns_adb_shutdown(view->adb);
		dns_resolver_shutdown(view->resolver);
		return (result);
	}
	event = &view->reqevent;
	dns_requestmgr_whenshutdown(view->requestmgr, view->task, &event);
	atomic_fetch_and(&view->attributes, ~DNS_VIEWATTR_REQSHUTDOWN);
	isc_refcount_increment(&view->weakrefs);

	return (ISC_R_SUCCESS);
}

void
dns_view_setcache(dns_view_t *view, dns_cache_t *cache, bool shared) {
	REQUIRE(DNS_VIEW_VALID(view));
	REQUIRE(!view->frozen);

	view->cacheshared = shared;
	if (view->cache != NULL) {
		dns_db_detach(&view->cachedb);
		dns_cache_detach(&view->cache);
	}
	dns_cache_attach(cache, &view->cache);
	dns_cache_attachdb(cache, &view->cachedb);
	INSIST(DNS_DB_VALID(view->cachedb));
}

bool
dns_view_iscacheshared(dns_view_t *view) {
	REQUIRE(DNS_VIEW_VALID(view));

	return (view->cacheshared);
}

void
dns_view_sethints(dns_view_t *view, dns_db_t *hints) {
	REQUIRE(DNS_VIEW_VALID(view));
	REQUIRE(!view->frozen);
	REQUIRE(view->hints == NULL);
	REQUIRE(dns_db_iszone(hints));

	dns_db_attach(hints, &view->hints);
}

void
dns_view_setkeyring(dns_view_t *view, dns_tsig_keyring_t *ring) {
	REQUIRE(DNS_VIEW_VALID(view));
	REQUIRE(ring != NULL);
	if (view->statickeys != NULL) {
		dns_tsigkeyring_detach(&view->statickeys);
	}
	dns_tsigkeyring_attach(ring, &view->statickeys);
}

void
dns_view_setdynamickeyring(dns_view_t *view, dns_tsig_keyring_t *ring) {
	REQUIRE(DNS_VIEW_VALID(view));
	REQUIRE(ring != NULL);
	if (view->dynamickeys != NULL) {
		dns_tsigkeyring_detach(&view->dynamickeys);
	}
	dns_tsigkeyring_attach(ring, &view->dynamickeys);
}

void
dns_view_getdynamickeyring(dns_view_t *view, dns_tsig_keyring_t **ringp) {
	REQUIRE(DNS_VIEW_VALID(view));
	REQUIRE(ringp != NULL && *ringp == NULL);
	if (view->dynamickeys != NULL) {
		dns_tsigkeyring_attach(view->dynamickeys, ringp);
	}
}

void
dns_view_restorekeyring(dns_view_t *view) {
	FILE *fp;
	char keyfile[PATH_MAX];
	isc_result_t result;

	REQUIRE(DNS_VIEW_VALID(view));

	if (view->dynamickeys != NULL) {
		result = isc_file_sanitize(NULL, view->name, "tsigkeys",
					   keyfile, sizeof(keyfile));
		if (result == ISC_R_SUCCESS) {
			fp = fopen(keyfile, "r");
			if (fp != NULL) {
				dns_keyring_restore(view->dynamickeys, fp);
				(void)fclose(fp);
			}
		}
	}
}

void
dns_view_setdstport(dns_view_t *view, in_port_t dstport) {
	REQUIRE(DNS_VIEW_VALID(view));
	view->dstport = dstport;
}

void
dns_view_freeze(dns_view_t *view) {
	REQUIRE(DNS_VIEW_VALID(view));
	REQUIRE(!view->frozen);

	if (view->resolver != NULL) {
		INSIST(view->cachedb != NULL);
		dns_resolver_freeze(view->resolver);
	}
	view->frozen = true;
}

void
dns_view_thaw(dns_view_t *view) {
	REQUIRE(DNS_VIEW_VALID(view));
	REQUIRE(view->frozen);

	view->frozen = false;
}

isc_result_t
dns_view_addzone(dns_view_t *view, dns_zone_t *zone) {
	isc_result_t result;

	REQUIRE(DNS_VIEW_VALID(view));
	REQUIRE(!view->frozen);
	REQUIRE(view->zonetable != NULL);

	result = dns_zt_mount(view->zonetable, zone);

	return (result);
}

isc_result_t
dns_view_findzone(dns_view_t *view, const dns_name_t *name,
		  dns_zone_t **zonep) {
	isc_result_t result;

	REQUIRE(DNS_VIEW_VALID(view));

	LOCK(&view->lock);
	if (view->zonetable != NULL) {
		result = dns_zt_find(view->zonetable, name, 0, NULL, zonep);
		if (result == DNS_R_PARTIALMATCH) {
			dns_zone_detach(zonep);
			result = ISC_R_NOTFOUND;
		}
	} else {
		result = ISC_R_NOTFOUND;
	}
	UNLOCK(&view->lock);

	return (result);
}

isc_result_t
dns_view_find(dns_view_t *view, const dns_name_t *name, dns_rdatatype_t type,
	      isc_stdtime_t now, unsigned int options, bool use_hints,
	      bool use_static_stub, dns_db_t **dbp, dns_dbnode_t **nodep,
	      dns_name_t *foundname, dns_rdataset_t *rdataset,
	      dns_rdataset_t *sigrdataset) {
	isc_result_t result;
	dns_db_t *db, *zdb;
	dns_dbnode_t *node, *znode;
	bool is_cache, is_staticstub_zone;
	dns_rdataset_t zrdataset, zsigrdataset;
	dns_zone_t *zone;

	/*
	 * Find an rdataset whose owner name is 'name', and whose type is
	 * 'type'.
	 */

	REQUIRE(DNS_VIEW_VALID(view));
	REQUIRE(view->frozen);
	REQUIRE(type != dns_rdatatype_rrsig);
	REQUIRE(rdataset != NULL); /* XXXBEW - remove this */
	REQUIRE(nodep == NULL || *nodep == NULL);

	/*
	 * Initialize.
	 */
	dns_rdataset_init(&zrdataset);
	dns_rdataset_init(&zsigrdataset);
	zdb = NULL;
	znode = NULL;

	/*
	 * Find a database to answer the query.
	 */
	db = NULL;
	node = NULL;
	is_staticstub_zone = false;
	zone = NULL;
	LOCK(&view->lock);
	if (view->zonetable != NULL) {
		result = dns_zt_find(view->zonetable, name, DNS_ZTFIND_MIRROR,
				     NULL, &zone);
	} else {
		result = ISC_R_NOTFOUND;
	}
	UNLOCK(&view->lock);
	if (zone != NULL && dns_zone_gettype(zone) == dns_zone_staticstub &&
	    !use_static_stub)
	{
		result = ISC_R_NOTFOUND;
	}
	if (result == ISC_R_SUCCESS || result == DNS_R_PARTIALMATCH) {
		result = dns_zone_getdb(zone, &db);
		if (result != ISC_R_SUCCESS && view->cachedb != NULL) {
			dns_db_attach(view->cachedb, &db);
		} else if (result != ISC_R_SUCCESS) {
			goto cleanup;
		}
		if (dns_zone_gettype(zone) == dns_zone_staticstub &&
		    dns_name_equal(name, dns_zone_getorigin(zone)))
		{
			is_staticstub_zone = true;
		}
	} else if (result == ISC_R_NOTFOUND && view->cachedb != NULL) {
		dns_db_attach(view->cachedb, &db);
	} else {
		goto cleanup;
	}

	is_cache = dns_db_iscache(db);

db_find:
	/*
	 * Now look for an answer in the database.
	 */
	result = dns_db_find(db, name, NULL, type, options, now, &node,
			     foundname, rdataset, sigrdataset);

	if (result == DNS_R_DELEGATION || result == ISC_R_NOTFOUND) {
		if (dns_rdataset_isassociated(rdataset)) {
			dns_rdataset_disassociate(rdataset);
		}
		if (sigrdataset != NULL &&
		    dns_rdataset_isassociated(sigrdataset))
		{
			dns_rdataset_disassociate(sigrdataset);
		}
		if (node != NULL) {
			dns_db_detachnode(db, &node);
		}
		if (!is_cache) {
			dns_db_detach(&db);
			if (view->cachedb != NULL && !is_staticstub_zone) {
				/*
				 * Either the answer is in the cache, or we
				 * don't know it.
				 * Note that if the result comes from a
				 * static-stub zone we stop the search here
				 * (see the function description in view.h).
				 */
				is_cache = true;
				dns_db_attach(view->cachedb, &db);
				goto db_find;
			}
		} else {
			/*
			 * We don't have the data in the cache.  If we've got
			 * glue from the zone, use it.
			 */
			if (dns_rdataset_isassociated(&zrdataset)) {
				dns_rdataset_clone(&zrdataset, rdataset);
				if (sigrdataset != NULL &&
				    dns_rdataset_isassociated(&zsigrdataset))
				{
					dns_rdataset_clone(&zsigrdataset,
							   sigrdataset);
				}
				result = DNS_R_GLUE;
				if (db != NULL) {
					dns_db_detach(&db);
				}
				dns_db_attach(zdb, &db);
				dns_db_attachnode(db, znode, &node);
				goto cleanup;
			}
		}
		/*
		 * We don't know the answer.
		 */
		result = ISC_R_NOTFOUND;
	} else if (result == DNS_R_GLUE) {
		if (view->cachedb != NULL && !is_staticstub_zone) {
			/*
			 * We found an answer, but the cache may be better.
			 * Remember what we've got and go look in the cache.
			 */
			is_cache = true;
			dns_rdataset_clone(rdataset, &zrdataset);
			dns_rdataset_disassociate(rdataset);
			if (sigrdataset != NULL &&
			    dns_rdataset_isassociated(sigrdataset))
			{
				dns_rdataset_clone(sigrdataset, &zsigrdataset);
				dns_rdataset_disassociate(sigrdataset);
			}
			dns_db_attach(db, &zdb);
			dns_db_attachnode(zdb, node, &znode);
			dns_db_detachnode(db, &node);
			dns_db_detach(&db);
			dns_db_attach(view->cachedb, &db);
			goto db_find;
		}
		/*
		 * Otherwise, the glue is the best answer.
		 */
		result = ISC_R_SUCCESS;
	}

	if (result == ISC_R_NOTFOUND && use_hints && view->hints != NULL) {
		if (dns_rdataset_isassociated(rdataset)) {
			dns_rdataset_disassociate(rdataset);
		}
		if (sigrdataset != NULL &&
		    dns_rdataset_isassociated(sigrdataset))
		{
			dns_rdataset_disassociate(sigrdataset);
		}
		if (db != NULL) {
			if (node != NULL) {
				dns_db_detachnode(db, &node);
			}
			dns_db_detach(&db);
		}
		result = dns_db_find(view->hints, name, NULL, type, options,
				     now, &node, foundname, rdataset,
				     sigrdataset);
		if (result == ISC_R_SUCCESS || result == DNS_R_GLUE) {
			/*
			 * We just used a hint.  Let the resolver know it
			 * should consider priming.
			 */
			dns_resolver_prime(view->resolver);
			dns_db_attach(view->hints, &db);
			result = DNS_R_HINT;
		} else if (result == DNS_R_NXRRSET) {
			dns_db_attach(view->hints, &db);
			result = DNS_R_HINTNXRRSET;
		} else if (result == DNS_R_NXDOMAIN) {
			result = ISC_R_NOTFOUND;
		}

		/*
		 * Cleanup if non-standard hints are used.
		 */
		if (db == NULL && node != NULL) {
			dns_db_detachnode(view->hints, &node);
		}
	}

cleanup:
	if (dns_rdataset_isassociated(&zrdataset)) {
		dns_rdataset_disassociate(&zrdataset);
		if (dns_rdataset_isassociated(&zsigrdataset)) {
			dns_rdataset_disassociate(&zsigrdataset);
		}
	}

	if (zdb != NULL) {
		if (znode != NULL) {
			dns_db_detachnode(zdb, &znode);
		}
		dns_db_detach(&zdb);
	}

	if (db != NULL) {
		if (node != NULL) {
			if (nodep != NULL) {
				*nodep = node;
			} else {
				dns_db_detachnode(db, &node);
			}
		}
		if (dbp != NULL) {
			*dbp = db;
		} else {
			dns_db_detach(&db);
		}
	} else {
		INSIST(node == NULL);
	}

	if (zone != NULL) {
		dns_zone_detach(&zone);
	}

	return (result);
}

isc_result_t
dns_view_simplefind(dns_view_t *view, const dns_name_t *name,
		    dns_rdatatype_t type, isc_stdtime_t now,
		    unsigned int options, bool use_hints,
		    dns_rdataset_t *rdataset, dns_rdataset_t *sigrdataset) {
	isc_result_t result;
	dns_fixedname_t foundname;

	dns_fixedname_init(&foundname);
	result = dns_view_find(view, name, type, now, options, use_hints, false,
			       NULL, NULL, dns_fixedname_name(&foundname),
			       rdataset, sigrdataset);
	if (result == DNS_R_NXDOMAIN) {
		/*
		 * The rdataset and sigrdataset of the relevant NSEC record
		 * may be returned, but the caller cannot use them because
		 * foundname is not returned by this simplified API.  We
		 * disassociate them here to prevent any misuse by the caller.
		 */
		if (dns_rdataset_isassociated(rdataset)) {
			dns_rdataset_disassociate(rdataset);
		}
		if (sigrdataset != NULL &&
		    dns_rdataset_isassociated(sigrdataset))
		{
			dns_rdataset_disassociate(sigrdataset);
		}
	} else if (result != ISC_R_SUCCESS && result != DNS_R_GLUE &&
		   result != DNS_R_HINT && result != DNS_R_NCACHENXDOMAIN &&
		   result != DNS_R_NCACHENXRRSET && result != DNS_R_NXRRSET &&
		   result != DNS_R_HINTNXRRSET && result != ISC_R_NOTFOUND)
	{
		if (dns_rdataset_isassociated(rdataset)) {
			dns_rdataset_disassociate(rdataset);
		}
		if (sigrdataset != NULL &&
		    dns_rdataset_isassociated(sigrdataset))
		{
			dns_rdataset_disassociate(sigrdataset);
		}
		result = ISC_R_NOTFOUND;
	}

	return (result);
}

isc_result_t
dns_view_findzonecut(dns_view_t *view, const dns_name_t *name,
		     dns_name_t *fname, dns_name_t *dcname, isc_stdtime_t now,
		     unsigned int options, bool use_hints, bool use_cache,
		     dns_rdataset_t *rdataset, dns_rdataset_t *sigrdataset) {
	isc_result_t result;
	dns_db_t *db;
	bool is_cache, use_zone, try_hints;
	dns_zone_t *zone;
	dns_name_t *zfname;
	dns_rdataset_t zrdataset, zsigrdataset;
	dns_fixedname_t zfixedname;
	unsigned int ztoptions = DNS_ZTFIND_MIRROR;

	REQUIRE(DNS_VIEW_VALID(view));
	REQUIRE(view->frozen);

	db = NULL;
	use_zone = false;
	try_hints = false;
	zfname = NULL;

	/*
	 * Initialize.
	 */
	dns_fixedname_init(&zfixedname);
	dns_rdataset_init(&zrdataset);
	dns_rdataset_init(&zsigrdataset);

	/*
	 * Find the right database.
	 */
	zone = NULL;
	LOCK(&view->lock);
	if (view->zonetable != NULL) {
		if ((options & DNS_DBFIND_NOEXACT) != 0) {
			ztoptions |= DNS_ZTFIND_NOEXACT;
		}
		result = dns_zt_find(view->zonetable, name, ztoptions, NULL,
				     &zone);
	} else {
		result = ISC_R_NOTFOUND;
	}
	UNLOCK(&view->lock);
	if (result == ISC_R_SUCCESS || result == DNS_R_PARTIALMATCH) {
		result = dns_zone_getdb(zone, &db);
	}
	if (result == ISC_R_NOTFOUND) {
		/*
		 * We're not directly authoritative for this query name, nor
		 * is it a subdomain of any zone for which we're
		 * authoritative.
		 */
		if (use_cache && view->cachedb != NULL) {
			/*
			 * We have a cache; try it.
			 */
			dns_db_attach(view->cachedb, &db);
		} else if (use_hints && view->hints != NULL) {
			/*
			 * Maybe we have hints...
			 */
			try_hints = true;
			goto finish;
		} else {
			result = DNS_R_NXDOMAIN;
			goto cleanup;
		}
	} else if (result != ISC_R_SUCCESS) {
		/*
		 * Something is broken.
		 */
		goto cleanup;
	}
	is_cache = dns_db_iscache(db);

db_find:
	/*
	 * Look for the zonecut.
	 */
	if (!is_cache) {
		result = dns_db_find(db, name, NULL, dns_rdatatype_ns, options,
				     now, NULL, fname, rdataset, sigrdataset);
		if (result == DNS_R_DELEGATION) {
			result = ISC_R_SUCCESS;
		} else if (result != ISC_R_SUCCESS) {
			goto cleanup;
		}

		if (use_cache && view->cachedb != NULL && db != view->hints) {
			/*
			 * We found an answer, but the cache may be better.
			 */
			zfname = dns_fixedname_name(&zfixedname);
			dns_name_copynf(fname, zfname);
			dns_rdataset_clone(rdataset, &zrdataset);
			dns_rdataset_disassociate(rdataset);
			if (sigrdataset != NULL &&
			    dns_rdataset_isassociated(sigrdataset))
			{
				dns_rdataset_clone(sigrdataset, &zsigrdataset);
				dns_rdataset_disassociate(sigrdataset);
			}
			dns_db_detach(&db);
			dns_db_attach(view->cachedb, &db);
			is_cache = true;
			goto db_find;
		}
	} else {
		result = dns_db_findzonecut(db, name, options, now, NULL, fname,
					    dcname, rdataset, sigrdataset);
		if (result == ISC_R_SUCCESS) {
			if (zfname != NULL &&
			    (!dns_name_issubdomain(fname, zfname) ||
			     (dns_zone_gettype(zone) == dns_zone_staticstub &&
			      dns_name_equal(fname, zfname))))
			{
				/*
				 * We found a zonecut in the cache, but our
				 * zone delegation is better.
				 */
				use_zone = true;
			}
		} else if (result == ISC_R_NOTFOUND) {
			if (zfname != NULL) {
				/*
				 * We didn't find anything in the cache, but we
				 * have a zone delegation, so use it.
				 */
				use_zone = true;
				result = ISC_R_SUCCESS;
			} else if (use_hints && view->hints != NULL) {
				/*
				 * Maybe we have hints...
				 */
				try_hints = true;
				result = ISC_R_SUCCESS;
			} else {
				result = DNS_R_NXDOMAIN;
			}
		} else {
			/*
			 * Something bad happened.
			 */
			goto cleanup;
		}
	}

finish:
	if (use_zone) {
		if (dns_rdataset_isassociated(rdataset)) {
			dns_rdataset_disassociate(rdataset);
			if (sigrdataset != NULL &&
			    dns_rdataset_isassociated(sigrdataset))
			{
				dns_rdataset_disassociate(sigrdataset);
			}
		}
		dns_name_copynf(zfname, fname);
		if (dcname != NULL) {
			dns_name_copynf(zfname, dcname);
		}
		dns_rdataset_clone(&zrdataset, rdataset);
		if (sigrdataset != NULL &&
		    dns_rdataset_isassociated(&zrdataset))
		{
			dns_rdataset_clone(&zsigrdataset, sigrdataset);
		}
	} else if (try_hints) {
		/*
		 * We've found nothing so far, but we have hints.
		 */
		result = dns_db_find(view->hints, dns_rootname, NULL,
				     dns_rdatatype_ns, 0, now, NULL, fname,
				     rdataset, NULL);
		if (result != ISC_R_SUCCESS) {
			/*
			 * We can't even find the hints for the root
			 * nameservers!
			 */
			if (dns_rdataset_isassociated(rdataset)) {
				dns_rdataset_disassociate(rdataset);
			}
			result = ISC_R_NOTFOUND;
		} else if (dcname != NULL) {
			dns_name_copynf(fname, dcname);
		}
	}

cleanup:
	if (dns_rdataset_isassociated(&zrdataset)) {
		dns_rdataset_disassociate(&zrdataset);
		if (dns_rdataset_isassociated(&zsigrdataset)) {
			dns_rdataset_disassociate(&zsigrdataset);
		}
	}
	if (db != NULL) {
		dns_db_detach(&db);
	}
	if (zone != NULL) {
		dns_zone_detach(&zone);
	}

	return (result);
}

isc_result_t
dns_viewlist_find(dns_viewlist_t *list, const char *name,
		  dns_rdataclass_t rdclass, dns_view_t **viewp) {
	dns_view_t *view;

	REQUIRE(list != NULL);

	for (view = ISC_LIST_HEAD(*list); view != NULL;
	     view = ISC_LIST_NEXT(view, link))
	{
		if (strcmp(view->name, name) == 0 && view->rdclass == rdclass) {
			break;
		}
	}
	if (view == NULL) {
		return (ISC_R_NOTFOUND);
	}

	dns_view_attach(view, viewp);

	return (ISC_R_SUCCESS);
}

isc_result_t
dns_viewlist_findzone(dns_viewlist_t *list, const dns_name_t *name,
		      bool allclasses, dns_rdataclass_t rdclass,
		      dns_zone_t **zonep) {
	dns_view_t *view;
	isc_result_t result;
	dns_zone_t *zone1 = NULL, *zone2 = NULL;
	dns_zone_t **zp = NULL;

	REQUIRE(list != NULL);
	REQUIRE(zonep != NULL && *zonep == NULL);

	for (view = ISC_LIST_HEAD(*list); view != NULL;
	     view = ISC_LIST_NEXT(view, link))
	{
		if (!allclasses && view->rdclass != rdclass) {
			continue;
		}

		/*
		 * If the zone is defined in more than one view,
		 * treat it as not found.
		 */
		zp = (zone1 == NULL) ? &zone1 : &zone2;
		LOCK(&view->lock);
		if (view->zonetable != NULL) {
			result = dns_zt_find(view->zonetable, name, 0, NULL,
					     zp);
		} else {
			result = ISC_R_NOTFOUND;
		}
		UNLOCK(&view->lock);
		INSIST(result == ISC_R_SUCCESS || result == ISC_R_NOTFOUND ||
		       result == DNS_R_PARTIALMATCH);

		/* Treat a partial match as no match */
		if (result == DNS_R_PARTIALMATCH) {
			dns_zone_detach(zp);
			result = ISC_R_NOTFOUND;
			POST(result);
		}

		if (zone2 != NULL) {
			dns_zone_detach(&zone1);
			dns_zone_detach(&zone2);
			return (ISC_R_MULTIPLE);
		}
	}

	if (zone1 != NULL) {
		dns_zone_attach(zone1, zonep);
		dns_zone_detach(&zone1);
		return (ISC_R_SUCCESS);
	}

	return (ISC_R_NOTFOUND);
}

isc_result_t
dns_view_load(dns_view_t *view, bool stop, bool newonly) {
	REQUIRE(DNS_VIEW_VALID(view));
	REQUIRE(view->zonetable != NULL);

	return (dns_zt_load(view->zonetable, stop, newonly));
}

isc_result_t
dns_view_asyncload(dns_view_t *view, bool newonly, dns_zt_allloaded_t callback,
		   void *arg) {
	REQUIRE(DNS_VIEW_VALID(view));
	REQUIRE(view->zonetable != NULL);

	return (dns_zt_asyncload(view->zonetable, newonly, callback, arg));
}

isc_result_t
dns_view_gettsig(dns_view_t *view, const dns_name_t *keyname,
		 dns_tsigkey_t **keyp) {
	isc_result_t result;
	REQUIRE(keyp != NULL && *keyp == NULL);

	result = dns_tsigkey_find(keyp, keyname, NULL, view->statickeys);
	if (result == ISC_R_NOTFOUND) {
		result = dns_tsigkey_find(keyp, keyname, NULL,
					  view->dynamickeys);
	}
	return (result);
}

isc_result_t
dns_view_getpeertsig(dns_view_t *view, const isc_netaddr_t *peeraddr,
		     dns_tsigkey_t **keyp) {
	isc_result_t result;
	dns_name_t *keyname = NULL;
	dns_peer_t *peer = NULL;

	result = dns_peerlist_peerbyaddr(view->peers, peeraddr, &peer);
	if (result != ISC_R_SUCCESS) {
		return (result);
	}

	result = dns_peer_getkey(peer, &keyname);
	if (result != ISC_R_SUCCESS) {
		return (result);
	}

	result = dns_view_gettsig(view, keyname, keyp);
	return ((result == ISC_R_NOTFOUND) ? ISC_R_FAILURE : result);
}

isc_result_t
dns_view_checksig(dns_view_t *view, isc_buffer_t *source, dns_message_t *msg) {
	REQUIRE(DNS_VIEW_VALID(view));
	REQUIRE(source != NULL);

	return (dns_tsig_verify(source, msg, view->statickeys,
				view->dynamickeys));
}

isc_result_t
dns_view_dumpdbtostream(dns_view_t *view, FILE *fp) {
	isc_result_t result;

	REQUIRE(DNS_VIEW_VALID(view));

	(void)fprintf(fp, ";\n; Cache dump of view '%s'\n;\n", view->name);
	result = dns_master_dumptostream(view->mctx, view->cachedb, NULL,
					 &dns_master_style_cache,
					 dns_masterformat_text, NULL, fp);
	if (result != ISC_R_SUCCESS) {
		return (result);
	}
	dns_adb_dump(view->adb, fp);
	dns_resolver_printbadcache(view->resolver, fp);
	dns_badcache_print(view->failcache, "SERVFAIL cache", fp);
	return (ISC_R_SUCCESS);
}

isc_result_t
dns_view_flushcache(dns_view_t *view, bool fixuponly) {
	isc_result_t result;

	REQUIRE(DNS_VIEW_VALID(view));

	if (view->cachedb == NULL) {
		return (ISC_R_SUCCESS);
	}
	if (!fixuponly) {
		result = dns_cache_flush(view->cache);
		if (result != ISC_R_SUCCESS) {
			return (result);
		}
	}
	dns_db_detach(&view->cachedb);
	dns_cache_attachdb(view->cache, &view->cachedb);
	if (view->resolver != NULL) {
		dns_resolver_flushbadcache(view->resolver, NULL);
	}
	if (view->failcache != NULL) {
		dns_badcache_flush(view->failcache);
	}

	dns_adb_flush(view->adb);
	return (ISC_R_SUCCESS);
}

isc_result_t
dns_view_flushname(dns_view_t *view, const dns_name_t *name) {
	return (dns_view_flushnode(view, name, false));
}

isc_result_t
dns_view_flushnode(dns_view_t *view, const dns_name_t *name, bool tree) {
	isc_result_t result = ISC_R_SUCCESS;

	REQUIRE(DNS_VIEW_VALID(view));

	if (tree) {
		if (view->adb != NULL) {
			dns_adb_flushnames(view->adb, name);
		}
		if (view->resolver != NULL) {
			dns_resolver_flushbadnames(view->resolver, name);
		}
		if (view->failcache != NULL) {
			dns_badcache_flushtree(view->failcache, name);
		}
	} else {
		if (view->adb != NULL) {
			dns_adb_flushname(view->adb, name);
		}
		if (view->resolver != NULL) {
			dns_resolver_flushbadcache(view->resolver, name);
		}
		if (view->failcache != NULL) {
			dns_badcache_flushname(view->failcache, name);
		}
	}

	if (view->cache != NULL) {
		result = dns_cache_flushnode(view->cache, name, tree);
	}

	return (result);
}

isc_result_t
dns_view_adddelegationonly(dns_view_t *view, const dns_name_t *name) {
	dns_name_t *item;
	unsigned int hash;

	REQUIRE(DNS_VIEW_VALID(view));

	if (view->delonly == NULL) {
		view->delonly = isc_mem_get(view->mctx,
					    sizeof(dns_namelist_t) *
						    DNS_VIEW_DELONLYHASH);
		for (hash = 0; hash < DNS_VIEW_DELONLYHASH; hash++) {
			ISC_LIST_INIT(view->delonly[hash]);
		}
	}
	hash = dns_name_hash(name, false) % DNS_VIEW_DELONLYHASH;
	item = ISC_LIST_HEAD(view->delonly[hash]);
	while (item != NULL && !dns_name_equal(item, name)) {
		item = ISC_LIST_NEXT(item, link);
	}
	if (item != NULL) {
		return (ISC_R_SUCCESS);
	}
	item = isc_mem_get(view->mctx, sizeof(*item));
	dns_name_init(item, NULL);
	dns_name_dup(name, view->mctx, item);
	ISC_LIST_APPEND(view->delonly[hash], item, link);
	return (ISC_R_SUCCESS);
}

isc_result_t
dns_view_excludedelegationonly(dns_view_t *view, const dns_name_t *name) {
	dns_name_t *item;
	unsigned int hash;

	REQUIRE(DNS_VIEW_VALID(view));

	if (view->rootexclude == NULL) {
		view->rootexclude = isc_mem_get(view->mctx,
						sizeof(dns_namelist_t) *
							DNS_VIEW_DELONLYHASH);
		for (hash = 0; hash < DNS_VIEW_DELONLYHASH; hash++) {
			ISC_LIST_INIT(view->rootexclude[hash]);
		}
	}
	hash = dns_name_hash(name, false) % DNS_VIEW_DELONLYHASH;
	item = ISC_LIST_HEAD(view->rootexclude[hash]);
	while (item != NULL && !dns_name_equal(item, name)) {
		item = ISC_LIST_NEXT(item, link);
	}
	if (item != NULL) {
		return (ISC_R_SUCCESS);
	}
	item = isc_mem_get(view->mctx, sizeof(*item));
	dns_name_init(item, NULL);
	dns_name_dup(name, view->mctx, item);
	ISC_LIST_APPEND(view->rootexclude[hash], item, link);
	return (ISC_R_SUCCESS);
}

bool
dns_view_isdelegationonly(dns_view_t *view, const dns_name_t *name) {
	dns_name_t *item;
	unsigned int hash;

	REQUIRE(DNS_VIEW_VALID(view));

	if (!view->rootdelonly && view->delonly == NULL) {
		return (false);
	}

	hash = dns_name_hash(name, false) % DNS_VIEW_DELONLYHASH;
	if (view->rootdelonly && dns_name_countlabels(name) <= 2) {
		if (view->rootexclude == NULL) {
			return (true);
		}
		item = ISC_LIST_HEAD(view->rootexclude[hash]);
		while (item != NULL && !dns_name_equal(item, name)) {
			item = ISC_LIST_NEXT(item, link);
		}
		if (item == NULL) {
			return (true);
		}
	}

	if (view->delonly == NULL) {
		return (false);
	}

	item = ISC_LIST_HEAD(view->delonly[hash]);
	while (item != NULL && !dns_name_equal(item, name)) {
		item = ISC_LIST_NEXT(item, link);
	}
	if (item == NULL) {
		return (false);
	}
	return (true);
}

void
dns_view_setrootdelonly(dns_view_t *view, bool value) {
	REQUIRE(DNS_VIEW_VALID(view));
	view->rootdelonly = value;
}

bool
dns_view_getrootdelonly(dns_view_t *view) {
	REQUIRE(DNS_VIEW_VALID(view));
	return (view->rootdelonly);
}

isc_result_t
dns_view_freezezones(dns_view_t *view, bool value) {
	REQUIRE(DNS_VIEW_VALID(view));
	REQUIRE(view->zonetable != NULL);

	return (dns_zt_freezezones(view->zonetable, view, value));
}

void
dns_view_setadbstats(dns_view_t *view, isc_stats_t *stats) {
	REQUIRE(DNS_VIEW_VALID(view));
	REQUIRE(!view->frozen);
	REQUIRE(view->adbstats == NULL);

	isc_stats_attach(stats, &view->adbstats);
}

void
dns_view_getadbstats(dns_view_t *view, isc_stats_t **statsp) {
	REQUIRE(DNS_VIEW_VALID(view));
	REQUIRE(statsp != NULL && *statsp == NULL);

	if (view->adbstats != NULL) {
		isc_stats_attach(view->adbstats, statsp);
	}
}

void
dns_view_setresstats(dns_view_t *view, isc_stats_t *stats) {
	REQUIRE(DNS_VIEW_VALID(view));
	REQUIRE(!view->frozen);
	REQUIRE(view->resstats == NULL);

	isc_stats_attach(stats, &view->resstats);
}

void
dns_view_getresstats(dns_view_t *view, isc_stats_t **statsp) {
	REQUIRE(DNS_VIEW_VALID(view));
	REQUIRE(statsp != NULL && *statsp == NULL);

	if (view->resstats != NULL) {
		isc_stats_attach(view->resstats, statsp);
	}
}

void
dns_view_setresquerystats(dns_view_t *view, dns_stats_t *stats) {
	REQUIRE(DNS_VIEW_VALID(view));
	REQUIRE(!view->frozen);
	REQUIRE(view->resquerystats == NULL);

	dns_stats_attach(stats, &view->resquerystats);
}

void
dns_view_getresquerystats(dns_view_t *view, dns_stats_t **statsp) {
	REQUIRE(DNS_VIEW_VALID(view));
	REQUIRE(statsp != NULL && *statsp == NULL);

	if (view->resquerystats != NULL) {
		dns_stats_attach(view->resquerystats, statsp);
	}
}

isc_result_t
dns_view_initntatable(dns_view_t *view, isc_taskmgr_t *taskmgr,
		      isc_timermgr_t *timermgr) {
	REQUIRE(DNS_VIEW_VALID(view));
	if (view->ntatable_priv != NULL) {
		dns_ntatable_detach(&view->ntatable_priv);
	}
	return (dns_ntatable_create(view, taskmgr, timermgr,
				    &view->ntatable_priv));
}

isc_result_t
dns_view_getntatable(dns_view_t *view, dns_ntatable_t **ntp) {
	REQUIRE(DNS_VIEW_VALID(view));
	REQUIRE(ntp != NULL && *ntp == NULL);
	if (view->ntatable_priv == NULL) {
		return (ISC_R_NOTFOUND);
	}
	dns_ntatable_attach(view->ntatable_priv, ntp);
	return (ISC_R_SUCCESS);
}

isc_result_t
dns_view_initsecroots(dns_view_t *view, isc_mem_t *mctx) {
	REQUIRE(DNS_VIEW_VALID(view));
	if (view->secroots_priv != NULL) {
		dns_keytable_detach(&view->secroots_priv);
	}
	return (dns_keytable_create(mctx, &view->secroots_priv));
}

isc_result_t
dns_view_getsecroots(dns_view_t *view, dns_keytable_t **ktp) {
	REQUIRE(DNS_VIEW_VALID(view));
	REQUIRE(ktp != NULL && *ktp == NULL);
	if (view->secroots_priv == NULL) {
		return (ISC_R_NOTFOUND);
	}
	dns_keytable_attach(view->secroots_priv, ktp);
	return (ISC_R_SUCCESS);
}

bool
dns_view_ntacovers(dns_view_t *view, isc_stdtime_t now, const dns_name_t *name,
		   const dns_name_t *anchor) {
	REQUIRE(DNS_VIEW_VALID(view));

	if (view->ntatable_priv == NULL) {
		return (false);
	}

	return (dns_ntatable_covered(view->ntatable_priv, now, name, anchor));
}

isc_result_t
dns_view_issecuredomain(dns_view_t *view, const dns_name_t *name,
			isc_stdtime_t now, bool checknta, bool *ntap,
			bool *secure_domain) {
	isc_result_t result;
	bool secure = false;
	dns_fixedname_t fn;
	dns_name_t *anchor;

	REQUIRE(DNS_VIEW_VALID(view));

	if (view->secroots_priv == NULL) {
		return (ISC_R_NOTFOUND);
	}

	anchor = dns_fixedname_initname(&fn);

	result = dns_keytable_issecuredomain(view->secroots_priv, name, anchor,
					     &secure);
	if (result != ISC_R_SUCCESS) {
		return (result);
	}

	if (ntap != NULL) {
		*ntap = false;
	}
	if (checknta && secure && view->ntatable_priv != NULL &&
	    dns_ntatable_covered(view->ntatable_priv, now, name, anchor))
	{
		if (ntap != NULL) {
			*ntap = true;
		}
		secure = false;
	}

	*secure_domain = secure;
	return (ISC_R_SUCCESS);
}

void
dns_view_untrust(dns_view_t *view, const dns_name_t *keyname,
		 dns_rdata_dnskey_t *dnskey) {
	isc_result_t result;
	dns_keytable_t *sr = NULL;

	REQUIRE(DNS_VIEW_VALID(view));
	REQUIRE(keyname != NULL);
	REQUIRE(dnskey != NULL);

	/*
	 * Clear the revoke bit, if set, so that the key will match what's
	 * in secroots now.
	 */
	dnskey->flags &= ~DNS_KEYFLAG_REVOKE;

	result = dns_view_getsecroots(view, &sr);
	if (result != ISC_R_SUCCESS) {
		return;
	}

	result = dns_keytable_deletekey(sr, keyname, dnskey);
	if (result == ISC_R_SUCCESS) {
		/*
		 * If key was found in secroots, then it was a
		 * configured trust anchor, and we want to fail
		 * secure. If there are no other configured keys,
		 * then leave a null key so that we can't validate
		 * anymore.
		 */
		dns_keytable_marksecure(sr, keyname);
	}

	dns_keytable_detach(&sr);
}

/*
 * Create path to a directory and a filename constructed from viewname.
 * This is a front-end to isc_file_sanitize(), allowing backward
 * compatibility to older versions when a file couldn't be expected
 * to be in the specified directory but might be in the current working
 * directory instead.
 *
 * It first tests for the existence of a file <viewname>.<suffix> in
 * 'directory'. If the file does not exist, it checks again in the
 * current working directory. If it does not exist there either,
 * return the path inside the directory.
 *
 * Returns ISC_R_SUCCESS if a path to an existing file is found or
 * a new path is created; returns ISC_R_NOSPACE if the path won't
 * fit in 'buflen'.
 */

static isc_result_t
nz_legacy(const char *directory, const char *viewname, const char *suffix,
	  char *buffer, size_t buflen) {
	isc_result_t result;
	char newbuf[PATH_MAX];

	result = isc_file_sanitize(directory, viewname, suffix, buffer, buflen);
	if (result != ISC_R_SUCCESS) {
		return (result);
	} else if (directory == NULL || isc_file_exists(buffer)) {
		return (ISC_R_SUCCESS);
	} else {
		/* Save buffer */
		strlcpy(newbuf, buffer, sizeof(newbuf));
	}

	/*
	 * It isn't in the specified directory; check CWD.
	 */
	result = isc_file_sanitize(NULL, viewname, suffix, buffer, buflen);
	if (result != ISC_R_SUCCESS || isc_file_exists(buffer)) {
		return (result);
	}

	/*
	 * File does not exist in either 'directory' or CWD,
	 * so use the path in 'directory'.
	 */
	strlcpy(buffer, newbuf, buflen);
	return (ISC_R_SUCCESS);
}

isc_result_t
dns_view_setnewzones(dns_view_t *view, bool allow, void *cfgctx,
		     void (*cfg_destroy)(void **), uint64_t mapsize) {
	isc_result_t result = ISC_R_SUCCESS;
	char buffer[1024];
#ifdef HAVE_LMDB
	MDB_env *env = NULL;
	int status;
#endif /* ifdef HAVE_LMDB */

#ifndef HAVE_LMDB
	UNUSED(mapsize);
#endif /* ifndef HAVE_LMDB */

	REQUIRE(DNS_VIEW_VALID(view));
	REQUIRE((cfgctx != NULL && cfg_destroy != NULL) || !allow);

	if (view->new_zone_file != NULL) {
		isc_mem_free(view->mctx, view->new_zone_file);
		view->new_zone_file = NULL;
	}

#ifdef HAVE_LMDB
	if (view->new_zone_dbenv != NULL) {
		mdb_env_close((MDB_env *)view->new_zone_dbenv);
		view->new_zone_dbenv = NULL;
	}

	if (view->new_zone_db != NULL) {
		isc_mem_free(view->mctx, view->new_zone_db);
		view->new_zone_db = NULL;
	}
#endif /* HAVE_LMDB */

	if (view->new_zone_config != NULL) {
		view->cfg_destroy(&view->new_zone_config);
		view->cfg_destroy = NULL;
	}

	if (!allow) {
		return (ISC_R_SUCCESS);
	}

	CHECK(nz_legacy(view->new_zone_dir, view->name, "nzf", buffer,
			sizeof(buffer)));

	view->new_zone_file = isc_mem_strdup(view->mctx, buffer);

#ifdef HAVE_LMDB
	CHECK(nz_legacy(view->new_zone_dir, view->name, "nzd", buffer,
			sizeof(buffer)));

	view->new_zone_db = isc_mem_strdup(view->mctx, buffer);

	status = mdb_env_create(&env);
	if (status != MDB_SUCCESS) {
		isc_log_write(dns_lctx, DNS_LOGCATEGORY_GENERAL,
			      ISC_LOGMODULE_OTHER, ISC_LOG_ERROR,
			      "mdb_env_create failed: %s",
			      mdb_strerror(status));
		CHECK(ISC_R_FAILURE);
	}

	if (mapsize != 0ULL) {
		status = mdb_env_set_mapsize(env, mapsize);
		if (status != MDB_SUCCESS) {
			isc_log_write(dns_lctx, DNS_LOGCATEGORY_GENERAL,
				      ISC_LOGMODULE_OTHER, ISC_LOG_ERROR,
				      "mdb_env_set_mapsize failed: %s",
				      mdb_strerror(status));
			CHECK(ISC_R_FAILURE);
		}
		view->new_zone_mapsize = mapsize;
	}

	status = mdb_env_open(env, view->new_zone_db, DNS_LMDB_FLAGS, 0600);
	if (status != MDB_SUCCESS) {
		isc_log_write(dns_lctx, DNS_LOGCATEGORY_GENERAL,
			      ISC_LOGMODULE_OTHER, ISC_LOG_ERROR,
			      "mdb_env_open of '%s' failed: %s",
			      view->new_zone_db, mdb_strerror(status));
		CHECK(ISC_R_FAILURE);
	}

	view->new_zone_dbenv = env;
	env = NULL;
#endif /* HAVE_LMDB */

	view->new_zone_config = cfgctx;
	view->cfg_destroy = cfg_destroy;

cleanup:
	if (result != ISC_R_SUCCESS) {
		if (view->new_zone_file != NULL) {
			isc_mem_free(view->mctx, view->new_zone_file);
			view->new_zone_file = NULL;
		}

#ifdef HAVE_LMDB
		if (view->new_zone_db != NULL) {
			isc_mem_free(view->mctx, view->new_zone_db);
			view->new_zone_db = NULL;
		}
		if (env != NULL) {
			mdb_env_close(env);
		}
#endif /* HAVE_LMDB */
		view->new_zone_config = NULL;
		view->cfg_destroy = NULL;
	}

	return (result);
}

void
dns_view_setnewzonedir(dns_view_t *view, const char *dir) {
	REQUIRE(DNS_VIEW_VALID(view));

	if (view->new_zone_dir != NULL) {
		isc_mem_free(view->mctx, view->new_zone_dir);
		view->new_zone_dir = NULL;
	}

	if (dir == NULL) {
		return;
	}

	view->new_zone_dir = isc_mem_strdup(view->mctx, dir);
}

const char *
dns_view_getnewzonedir(dns_view_t *view) {
	REQUIRE(DNS_VIEW_VALID(view));

	return (view->new_zone_dir);
}

isc_result_t
dns_view_searchdlz(dns_view_t *view, const dns_name_t *name,
		   unsigned int minlabels, dns_clientinfomethods_t *methods,
		   dns_clientinfo_t *clientinfo, dns_db_t **dbp) {
	dns_fixedname_t fname;
	dns_name_t *zonename;
	unsigned int namelabels;
	unsigned int i;
	isc_result_t result;
	dns_dlzfindzone_t findzone;
	dns_dlzdb_t *dlzdb;
	dns_db_t *db, *best = NULL;

	/*
	 * Performs checks to make sure data is as we expect it to be.
	 */
	REQUIRE(DNS_VIEW_VALID(view));
	REQUIRE(name != NULL);
	REQUIRE(dbp != NULL && *dbp == NULL);

	/* setup a "fixed" dns name */
	zonename = dns_fixedname_initname(&fname);

	/* count the number of labels in the name */
	namelabels = dns_name_countlabels(name);

	for (dlzdb = ISC_LIST_HEAD(view->dlz_searched); dlzdb != NULL;
	     dlzdb = ISC_LIST_NEXT(dlzdb, link))
	{
		REQUIRE(DNS_DLZ_VALID(dlzdb));

		/*
		 * loop through starting with the longest domain name and
		 * trying shorter names portions of the name until we find a
		 * match, have an error, or are below the 'minlabels'
		 * threshold.  minlabels is 0, if neither the standard
		 * database nor any previous DLZ database had a zone name
		 * match. Otherwise minlabels is the number of labels
		 * in that name.  We need to beat that for a "better"
		 * match for this DLZ database to be authoritative.
		 */
		for (i = namelabels; i > minlabels && i > 1; i--) {
			if (i == namelabels) {
				dns_name_copynf(name, zonename);
			} else {
				dns_name_split(name, i, NULL, zonename);
			}

			/* ask SDLZ driver if the zone is supported */
			db = NULL;
			findzone = dlzdb->implementation->methods->findzone;
			result = (*findzone)(dlzdb->implementation->driverarg,
					     dlzdb->dbdata, dlzdb->mctx,
					     view->rdclass, zonename, methods,
					     clientinfo, &db);

			if (result != ISC_R_NOTFOUND) {
				if (best != NULL) {
					dns_db_detach(&best);
				}
				if (result == ISC_R_SUCCESS) {
					INSIST(db != NULL);
					dns_db_attach(db, &best);
					dns_db_detach(&db);
					minlabels = i;
				} else {
					if (db != NULL) {
						dns_db_detach(&db);
					}
					break;
				}
			} else if (db != NULL) {
				dns_db_detach(&db);
			}
		}
	}

	if (best != NULL) {
		dns_db_attach(best, dbp);
		dns_db_detach(&best);
		return (ISC_R_SUCCESS);
	}

	return (ISC_R_NOTFOUND);
}

uint32_t
dns_view_getfailttl(dns_view_t *view) {
	REQUIRE(DNS_VIEW_VALID(view));
	return (view->fail_ttl);
}

void
dns_view_setfailttl(dns_view_t *view, uint32_t fail_ttl) {
	REQUIRE(DNS_VIEW_VALID(view));
	view->fail_ttl = fail_ttl;
}

isc_result_t
dns_view_saventa(dns_view_t *view) {
	isc_result_t result;
	bool removefile = false;
	dns_ntatable_t *ntatable = NULL;
	FILE *fp = NULL;

	REQUIRE(DNS_VIEW_VALID(view));

	if (view->nta_lifetime == 0) {
		return (ISC_R_SUCCESS);
	}

	/* Open NTA save file for overwrite. */
	CHECK(isc_stdio_open(view->nta_file, "w", &fp));

	result = dns_view_getntatable(view, &ntatable);
	if (result == ISC_R_NOTFOUND) {
		removefile = true;
		result = ISC_R_SUCCESS;
		goto cleanup;
	} else {
		CHECK(result);
	}

	result = dns_ntatable_save(ntatable, fp);
	if (result == ISC_R_NOTFOUND) {
		removefile = true;
		result = ISC_R_SUCCESS;
	} else if (result == ISC_R_SUCCESS) {
		result = isc_stdio_close(fp);
		fp = NULL;
	}

cleanup:
	if (ntatable != NULL) {
		dns_ntatable_detach(&ntatable);
	}

	if (fp != NULL) {
		(void)isc_stdio_close(fp);
	}

	/* Don't leave half-baked NTA save files lying around. */
	if (result != ISC_R_SUCCESS || removefile) {
		(void)isc_file_remove(view->nta_file);
	}

	return (result);
}

#define TSTR(t) ((t).value.as_textregion.base)
#define TLEN(t) ((t).value.as_textregion.length)

isc_result_t
dns_view_loadnta(dns_view_t *view) {
	isc_result_t result;
	dns_ntatable_t *ntatable = NULL;
	isc_lex_t *lex = NULL;
	isc_token_t token;
	isc_stdtime_t now;

	REQUIRE(DNS_VIEW_VALID(view));

	if (view->nta_lifetime == 0) {
		return (ISC_R_SUCCESS);
	}

	CHECK(isc_lex_create(view->mctx, 1025, &lex));
	CHECK(isc_lex_openfile(lex, view->nta_file));
	CHECK(dns_view_getntatable(view, &ntatable));
	isc_stdtime_get(&now);

	for (;;) {
		int options = (ISC_LEXOPT_EOL | ISC_LEXOPT_EOF);
		char *name, *type, *timestamp;
		size_t len;
		dns_fixedname_t fn;
		const dns_name_t *ntaname;
		isc_buffer_t b;
		isc_stdtime_t t;
		bool forced;

		CHECK(isc_lex_gettoken(lex, options, &token));
		if (token.type == isc_tokentype_eof) {
			break;
		} else if (token.type != isc_tokentype_string) {
			CHECK(ISC_R_UNEXPECTEDTOKEN);
		}
		name = TSTR(token);
		len = TLEN(token);

		if (strcmp(name, ".") == 0) {
			ntaname = dns_rootname;
		} else {
			dns_name_t *fname;
			fname = dns_fixedname_initname(&fn);

			isc_buffer_init(&b, name, (unsigned int)len);
			isc_buffer_add(&b, (unsigned int)len);
			CHECK(dns_name_fromtext(fname, &b, dns_rootname, 0,
						NULL));
			ntaname = fname;
		}

		CHECK(isc_lex_gettoken(lex, options, &token));
		if (token.type != isc_tokentype_string) {
			CHECK(ISC_R_UNEXPECTEDTOKEN);
		}
		type = TSTR(token);

		if (strcmp(type, "regular") == 0) {
			forced = false;
		} else if (strcmp(type, "forced") == 0) {
			forced = true;
		} else {
			CHECK(ISC_R_UNEXPECTEDTOKEN);
		}

		CHECK(isc_lex_gettoken(lex, options, &token));
		if (token.type != isc_tokentype_string) {
			CHECK(ISC_R_UNEXPECTEDTOKEN);
		}
		timestamp = TSTR(token);
		CHECK(dns_time32_fromtext(timestamp, &t));

		CHECK(isc_lex_gettoken(lex, options, &token));
		if (token.type != isc_tokentype_eol &&
		    token.type != isc_tokentype_eof)
		{
			CHECK(ISC_R_UNEXPECTEDTOKEN);
		}

		if (now <= t) {
			if (t > (now + 604800)) {
				t = now + 604800;
			}

			(void)dns_ntatable_add(ntatable, ntaname, forced, 0, t);
		} else {
			char nb[DNS_NAME_FORMATSIZE];
			dns_name_format(ntaname, nb, sizeof(nb));
			isc_log_write(dns_lctx, DNS_LOGCATEGORY_DNSSEC,
				      DNS_LOGMODULE_NTA, ISC_LOG_INFO,
				      "ignoring expired NTA at %s", nb);
		}
	}

cleanup:
	if (ntatable != NULL) {
		dns_ntatable_detach(&ntatable);
	}

	if (lex != NULL) {
		isc_lex_close(lex);
		isc_lex_destroy(&lex);
	}

	return (result);
}

void
dns_view_setviewcommit(dns_view_t *view) {
	dns_zone_t *redirect = NULL, *managed_keys = NULL;

	REQUIRE(DNS_VIEW_VALID(view));

	LOCK(&view->lock);

	if (view->redirect != NULL) {
		dns_zone_attach(view->redirect, &redirect);
	}
	if (view->managed_keys != NULL) {
		dns_zone_attach(view->managed_keys, &managed_keys);
	}
	if (view->zonetable != NULL) {
		dns_zt_setviewcommit(view->zonetable);
	}

	UNLOCK(&view->lock);

	if (redirect != NULL) {
		dns_zone_setviewcommit(redirect);
		dns_zone_detach(&redirect);
	}
	if (managed_keys != NULL) {
		dns_zone_setviewcommit(managed_keys);
		dns_zone_detach(&managed_keys);
	}
}

void
dns_view_setviewrevert(dns_view_t *view) {
	dns_zone_t *redirect = NULL, *managed_keys = NULL;
	dns_zt_t *zonetable;

	REQUIRE(DNS_VIEW_VALID(view));

	/*
	 * dns_zt_setviewrevert() attempts to lock this view, so we must
	 * release the lock.
	 */
	LOCK(&view->lock);
	if (view->redirect != NULL) {
		dns_zone_attach(view->redirect, &redirect);
	}
	if (view->managed_keys != NULL) {
		dns_zone_attach(view->managed_keys, &managed_keys);
	}
	zonetable = view->zonetable;
	UNLOCK(&view->lock);

	if (redirect != NULL) {
		dns_zone_setviewrevert(redirect);
		dns_zone_detach(&redirect);
	}
	if (managed_keys != NULL) {
		dns_zone_setviewrevert(managed_keys);
		dns_zone_detach(&managed_keys);
	}
	if (zonetable != NULL) {
		dns_zt_setviewrevert(zonetable);
	}
}

bool
dns_view_staleanswerenabled(dns_view_t *view) {
	uint32_t stale_ttl = 0;
	bool result = false;

	REQUIRE(DNS_VIEW_VALID(view));

	if (dns_db_getservestalettl(view->cachedb, &stale_ttl) != ISC_R_SUCCESS)
	{
		return (false);
	}
	if (stale_ttl > 0) {
		if (view->staleanswersok == dns_stale_answer_yes) {
			result = true;
		} else if (view->staleanswersok == dns_stale_answer_conf) {
			result = view->staleanswersenable;
		}
	}

	return (result);
}
