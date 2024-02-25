/*	$NetBSD: catz.c,v 1.8.2.2 2024/02/25 15:46:48 martin Exp $	*/

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
#include <stdint.h>
#include <stdlib.h>

#include <isc/hex.h>
#include <isc/md.h>
#include <isc/mem.h>
#include <isc/parseint.h>
#include <isc/print.h>
#include <isc/result.h>
#include <isc/task.h>
#include <isc/thread.h>
#include <isc/util.h>

#include <dns/catz.h>
#include <dns/dbiterator.h>
#include <dns/events.h>
#include <dns/rdatasetiter.h>
#include <dns/view.h>
#include <dns/zone.h>

#define DNS_CATZ_ZONE_MAGIC  ISC_MAGIC('c', 'a', 't', 'z')
#define DNS_CATZ_ZONES_MAGIC ISC_MAGIC('c', 'a', 't', 's')
#define DNS_CATZ_ENTRY_MAGIC ISC_MAGIC('c', 'a', 't', 'e')
#define DNS_CATZ_COO_MAGIC   ISC_MAGIC('c', 'a', 't', 'c')

#define DNS_CATZ_ZONE_VALID(catz)   ISC_MAGIC_VALID(catz, DNS_CATZ_ZONE_MAGIC)
#define DNS_CATZ_ZONES_VALID(catzs) ISC_MAGIC_VALID(catzs, DNS_CATZ_ZONES_MAGIC)
#define DNS_CATZ_ENTRY_VALID(entry) ISC_MAGIC_VALID(entry, DNS_CATZ_ENTRY_MAGIC)
#define DNS_CATZ_COO_VALID(coo)	    ISC_MAGIC_VALID(coo, DNS_CATZ_COO_MAGIC)

#define DNS_CATZ_VERSION_UNDEFINED ((uint32_t)(-1))

/*%
 * Change of ownership permissions
 */
struct dns_catz_coo {
	unsigned int magic;
	dns_name_t name;
	isc_refcount_t references;
};

/*%
 * Single member zone in a catalog
 */
struct dns_catz_entry {
	unsigned int magic;
	dns_name_t name;
	dns_catz_options_t opts;
	isc_refcount_t references;
};

/*%
 * Catalog zone
 */
struct dns_catz_zone {
	unsigned int magic;
	dns_name_t name;
	dns_catz_zones_t *catzs;
	dns_rdata_t soa;
	uint32_t version;
	/* key in entries is 'mhash', not domain name! */
	isc_ht_t *entries;
	/* key in coos is domain name */
	isc_ht_t *coos;

	/*
	 * defoptions are taken from named.conf
	 * zoneoptions are global options from zone
	 */
	dns_catz_options_t defoptions;
	dns_catz_options_t zoneoptions;
	isc_time_t lastupdated;

	bool updatepending;	      /* there is an update pending */
	bool updaterunning;	      /* there is an update running */
	isc_result_t updateresult;    /* result from the offloaded work */
	dns_db_t *db;		      /* zones database */
	dns_dbversion_t *dbversion;   /* version we will be updating to */
	dns_db_t *updb;		      /* zones database we're working on */
	dns_dbversion_t *updbversion; /* version we're working on */

	isc_timer_t *updatetimer;
	isc_event_t updateevent;

	bool active;
	bool db_registered;
	bool broken;

	isc_refcount_t references;
	isc_mutex_t lock;
};

static void
dns__catz_timer_cb(isc_task_t *task, isc_event_t *event);

static void
dns__catz_update_cb(void *data);
static void
dns__catz_done_cb(void *data, isc_result_t result);

static isc_result_t
catz_process_zones_entry(dns_catz_zone_t *catz, dns_rdataset_t *value,
			 dns_label_t *mhash);
static isc_result_t
catz_process_zones_suboption(dns_catz_zone_t *catz, dns_rdataset_t *value,
			     dns_label_t *mhash, dns_name_t *name);
static void
catz_entry_add_or_mod(dns_catz_zone_t *catz, isc_ht_t *ht, unsigned char *key,
		      size_t keysize, dns_catz_entry_t *nentry,
		      dns_catz_entry_t *oentry, const char *msg,
		      const char *zname, const char *czname);

/*%
 * Collection of catalog zones for a view
 */
struct dns_catz_zones {
	unsigned int magic;
	isc_ht_t *zones;
	isc_mem_t *mctx;
	isc_refcount_t references;
	isc_mutex_t lock;
	dns_catz_zonemodmethods_t *zmm;
	isc_taskmgr_t *taskmgr;
	isc_timermgr_t *timermgr;
	dns_view_t *view;
	isc_task_t *updater;
	atomic_bool shuttingdown;
};

void
dns_catz_options_init(dns_catz_options_t *options) {
	REQUIRE(options != NULL);

	dns_ipkeylist_init(&options->masters);

	options->allow_query = NULL;
	options->allow_transfer = NULL;

	options->allow_query = NULL;
	options->allow_transfer = NULL;

	options->in_memory = false;
	options->min_update_interval = 5;
	options->zonedir = NULL;
}

void
dns_catz_options_free(dns_catz_options_t *options, isc_mem_t *mctx) {
	REQUIRE(options != NULL);
	REQUIRE(mctx != NULL);

	if (options->masters.count != 0) {
		dns_ipkeylist_clear(mctx, &options->masters);
	}
	if (options->zonedir != NULL) {
		isc_mem_free(mctx, options->zonedir);
		options->zonedir = NULL;
	}
	if (options->allow_query != NULL) {
		isc_buffer_free(&options->allow_query);
	}
	if (options->allow_transfer != NULL) {
		isc_buffer_free(&options->allow_transfer);
	}
}

void
dns_catz_options_copy(isc_mem_t *mctx, const dns_catz_options_t *src,
		      dns_catz_options_t *dst) {
	REQUIRE(mctx != NULL);
	REQUIRE(src != NULL);
	REQUIRE(dst != NULL);
	REQUIRE(dst->masters.count == 0);
	REQUIRE(dst->allow_query == NULL);
	REQUIRE(dst->allow_transfer == NULL);

	if (src->masters.count != 0) {
		dns_ipkeylist_copy(mctx, &src->masters, &dst->masters);
	}

	if (dst->zonedir != NULL) {
		isc_mem_free(mctx, dst->zonedir);
		dst->zonedir = NULL;
	}

	if (src->zonedir != NULL) {
		dst->zonedir = isc_mem_strdup(mctx, src->zonedir);
	}

	if (src->allow_query != NULL) {
		isc_buffer_dup(mctx, &dst->allow_query, src->allow_query);
	}

	if (src->allow_transfer != NULL) {
		isc_buffer_dup(mctx, &dst->allow_transfer, src->allow_transfer);
	}
}

void
dns_catz_options_setdefault(isc_mem_t *mctx, const dns_catz_options_t *defaults,
			    dns_catz_options_t *opts) {
	REQUIRE(mctx != NULL);
	REQUIRE(defaults != NULL);
	REQUIRE(opts != NULL);

	if (opts->masters.count == 0 && defaults->masters.count != 0) {
		dns_ipkeylist_copy(mctx, &defaults->masters, &opts->masters);
	}

	if (defaults->zonedir != NULL) {
		opts->zonedir = isc_mem_strdup(mctx, defaults->zonedir);
	}

	if (opts->allow_query == NULL && defaults->allow_query != NULL) {
		isc_buffer_dup(mctx, &opts->allow_query, defaults->allow_query);
	}
	if (opts->allow_transfer == NULL && defaults->allow_transfer != NULL) {
		isc_buffer_dup(mctx, &opts->allow_transfer,
			       defaults->allow_transfer);
	}

	/* This option is always taken from config, so it's always 'default' */
	opts->in_memory = defaults->in_memory;
}

static void
catz_coo_new(isc_mem_t *mctx, const dns_name_t *domain,
	     dns_catz_coo_t **ncoop) {
	dns_catz_coo_t *ncoo;

	REQUIRE(mctx != NULL);
	REQUIRE(domain != NULL);
	REQUIRE(ncoop != NULL && *ncoop == NULL);

	ncoo = isc_mem_get(mctx, sizeof(*ncoo));
	dns_name_init(&ncoo->name, NULL);
	dns_name_dup(domain, mctx, &ncoo->name);
	isc_refcount_init(&ncoo->references, 1);
	ncoo->magic = DNS_CATZ_COO_MAGIC;
	*ncoop = ncoo;
}

static void
catz_coo_detach(dns_catz_zone_t *catz, dns_catz_coo_t **coop) {
	dns_catz_coo_t *coo;

	REQUIRE(DNS_CATZ_ZONE_VALID(catz));
	REQUIRE(coop != NULL && DNS_CATZ_COO_VALID(*coop));
	coo = *coop;
	*coop = NULL;

	if (isc_refcount_decrement(&coo->references) == 1) {
		isc_mem_t *mctx = catz->catzs->mctx;
		coo->magic = 0;
		isc_refcount_destroy(&coo->references);
		if (dns_name_dynamic(&coo->name)) {
			dns_name_free(&coo->name, mctx);
		}
		isc_mem_put(mctx, coo, sizeof(*coo));
	}
}

void
dns_catz_entry_new(isc_mem_t *mctx, const dns_name_t *domain,
		   dns_catz_entry_t **nentryp) {
	dns_catz_entry_t *nentry;

	REQUIRE(mctx != NULL);
	REQUIRE(nentryp != NULL && *nentryp == NULL);

	nentry = isc_mem_get(mctx, sizeof(*nentry));

	dns_name_init(&nentry->name, NULL);
	if (domain != NULL) {
		dns_name_dup(domain, mctx, &nentry->name);
	}

	dns_catz_options_init(&nentry->opts);
	isc_refcount_init(&nentry->references, 1);
	nentry->magic = DNS_CATZ_ENTRY_MAGIC;
	*nentryp = nentry;
}

dns_name_t *
dns_catz_entry_getname(dns_catz_entry_t *entry) {
	REQUIRE(DNS_CATZ_ENTRY_VALID(entry));
	return (&entry->name);
}

void
dns_catz_entry_copy(dns_catz_zone_t *catz, const dns_catz_entry_t *entry,
		    dns_catz_entry_t **nentryp) {
	dns_catz_entry_t *nentry = NULL;

	REQUIRE(DNS_CATZ_ZONE_VALID(catz));
	REQUIRE(DNS_CATZ_ENTRY_VALID(entry));
	REQUIRE(nentryp != NULL && *nentryp == NULL);

	dns_catz_entry_new(catz->catzs->mctx, &entry->name, &nentry);

	dns_catz_options_copy(catz->catzs->mctx, &entry->opts, &nentry->opts);
	*nentryp = nentry;
}

void
dns_catz_entry_attach(dns_catz_entry_t *entry, dns_catz_entry_t **entryp) {
	REQUIRE(DNS_CATZ_ENTRY_VALID(entry));
	REQUIRE(entryp != NULL && *entryp == NULL);

	isc_refcount_increment(&entry->references);
	*entryp = entry;
}

void
dns_catz_entry_detach(dns_catz_zone_t *catz, dns_catz_entry_t **entryp) {
	dns_catz_entry_t *entry;

	REQUIRE(DNS_CATZ_ZONE_VALID(catz));
	REQUIRE(entryp != NULL && DNS_CATZ_ENTRY_VALID(*entryp));
	entry = *entryp;
	*entryp = NULL;

	if (isc_refcount_decrement(&entry->references) == 1) {
		isc_mem_t *mctx = catz->catzs->mctx;
		entry->magic = 0;
		isc_refcount_destroy(&entry->references);
		dns_catz_options_free(&entry->opts, mctx);
		if (dns_name_dynamic(&entry->name)) {
			dns_name_free(&entry->name, mctx);
		}
		isc_mem_put(mctx, entry, sizeof(*entry));
	}
}

bool
dns_catz_entry_validate(const dns_catz_entry_t *entry) {
	REQUIRE(DNS_CATZ_ENTRY_VALID(entry));
	UNUSED(entry);

	return (true);
}

bool
dns_catz_entry_cmp(const dns_catz_entry_t *ea, const dns_catz_entry_t *eb) {
	isc_region_t ra, rb;

	REQUIRE(DNS_CATZ_ENTRY_VALID(ea));
	REQUIRE(DNS_CATZ_ENTRY_VALID(eb));

	if (ea == eb) {
		return (true);
	}

	if (ea->opts.masters.count != eb->opts.masters.count) {
		return (false);
	}

	if (memcmp(ea->opts.masters.addrs, eb->opts.masters.addrs,
		   ea->opts.masters.count * sizeof(isc_sockaddr_t)))
	{
		return (false);
	}

	for (size_t i = 0; i < eb->opts.masters.count; i++) {
		if ((ea->opts.masters.keys[i] == NULL) !=
		    (eb->opts.masters.keys[i] == NULL))
		{
			return (false);
		}
		if (ea->opts.masters.keys[i] == NULL) {
			continue;
		}
		if (!dns_name_equal(ea->opts.masters.keys[i],
				    eb->opts.masters.keys[i]))
		{
			return (false);
		}
	}

	for (size_t i = 0; i < eb->opts.masters.count; i++) {
		if ((ea->opts.masters.tlss[i] == NULL) !=
		    (eb->opts.masters.tlss[i] == NULL))
		{
			return (false);
		}
		if (ea->opts.masters.tlss[i] == NULL) {
			continue;
		}
		if (!dns_name_equal(ea->opts.masters.tlss[i],
				    eb->opts.masters.tlss[i]))
		{
			return (false);
		}
	}

	/* If one is NULL and the other isn't, the entries don't match */
	if ((ea->opts.allow_query == NULL) != (eb->opts.allow_query == NULL)) {
		return (false);
	}

	/* If one is non-NULL, then they both are */
	if (ea->opts.allow_query != NULL) {
		isc_buffer_usedregion(ea->opts.allow_query, &ra);
		isc_buffer_usedregion(eb->opts.allow_query, &rb);
		if (isc_region_compare(&ra, &rb)) {
			return (false);
		}
	}

	/* Repeat the above checks with allow_transfer */
	if ((ea->opts.allow_transfer == NULL) !=
	    (eb->opts.allow_transfer == NULL))
	{
		return (false);
	}

	if (ea->opts.allow_transfer != NULL) {
		isc_buffer_usedregion(ea->opts.allow_transfer, &ra);
		isc_buffer_usedregion(eb->opts.allow_transfer, &rb);
		if (isc_region_compare(&ra, &rb)) {
			return (false);
		}
	}

	return (true);
}

dns_name_t *
dns_catz_zone_getname(dns_catz_zone_t *catz) {
	REQUIRE(DNS_CATZ_ZONE_VALID(catz));

	return (&catz->name);
}

dns_catz_options_t *
dns_catz_zone_getdefoptions(dns_catz_zone_t *catz) {
	REQUIRE(DNS_CATZ_ZONE_VALID(catz));

	return (&catz->defoptions);
}

void
dns_catz_zone_resetdefoptions(dns_catz_zone_t *catz) {
	REQUIRE(DNS_CATZ_ZONE_VALID(catz));

	dns_catz_options_free(&catz->defoptions, catz->catzs->mctx);
	dns_catz_options_init(&catz->defoptions);
}

/*%<
 * Merge 'newcatz' into 'catz', calling addzone/delzone/modzone
 * (from catz->catzs->zmm) for appropriate member zones.
 *
 * Requires:
 * \li	'catz' is a valid dns_catz_zone_t.
 * \li	'newcatz' is a valid dns_catz_zone_t.
 *
 */
static isc_result_t
dns__catz_zones_merge(dns_catz_zone_t *catz, dns_catz_zone_t *newcatz) {
	isc_result_t result;
	isc_ht_iter_t *iter1 = NULL, *iter2 = NULL;
	isc_ht_iter_t *iteradd = NULL, *itermod = NULL;
	isc_ht_t *toadd = NULL, *tomod = NULL;
	bool delcur = false;
	char czname[DNS_NAME_FORMATSIZE];
	char zname[DNS_NAME_FORMATSIZE];
	dns_catz_zoneop_fn_t addzone, modzone, delzone;

	REQUIRE(DNS_CATZ_ZONE_VALID(catz));
	REQUIRE(DNS_CATZ_ZONE_VALID(newcatz));

	LOCK(&catz->lock);

	/* TODO verify the new zone first! */

	addzone = catz->catzs->zmm->addzone;
	modzone = catz->catzs->zmm->modzone;
	delzone = catz->catzs->zmm->delzone;

	/* Copy zoneoptions from newcatz into catz. */

	dns_catz_options_free(&catz->zoneoptions, catz->catzs->mctx);
	dns_catz_options_copy(catz->catzs->mctx, &newcatz->zoneoptions,
			      &catz->zoneoptions);
	dns_catz_options_setdefault(catz->catzs->mctx, &catz->defoptions,
				    &catz->zoneoptions);

	dns_name_format(&catz->name, czname, DNS_NAME_FORMATSIZE);

	isc_ht_init(&toadd, catz->catzs->mctx, 16, ISC_HT_CASE_SENSITIVE);
	isc_ht_init(&tomod, catz->catzs->mctx, 16, ISC_HT_CASE_SENSITIVE);
	isc_ht_iter_create(newcatz->entries, &iter1);
	isc_ht_iter_create(catz->entries, &iter2);

	/*
	 * We can create those iterators now, even though toadd and tomod are
	 * empty
	 */
	isc_ht_iter_create(toadd, &iteradd);
	isc_ht_iter_create(tomod, &itermod);

	/*
	 * First - walk the new zone and find all nodes that are not in the
	 * old zone, or are in both zones and are modified.
	 */
	for (result = isc_ht_iter_first(iter1); result == ISC_R_SUCCESS;
	     result = delcur ? isc_ht_iter_delcurrent_next(iter1)
			     : isc_ht_iter_next(iter1))
	{
		isc_result_t zt_find_result;
		dns_catz_zone_t *parentcatz = NULL;
		dns_catz_entry_t *nentry = NULL;
		dns_catz_entry_t *oentry = NULL;
		dns_zone_t *zone = NULL;
		unsigned char *key = NULL;
		size_t keysize;
		delcur = false;

		isc_ht_iter_current(iter1, (void **)&nentry);
		isc_ht_iter_currentkey(iter1, &key, &keysize);

		/*
		 * Spurious record that came from suboption without main
		 * record, removed.
		 * xxxwpk: make it a separate verification phase?
		 */
		if (dns_name_countlabels(&nentry->name) == 0) {
			dns_catz_entry_detach(newcatz, &nentry);
			delcur = true;
			continue;
		}

		dns_name_format(&nentry->name, zname, DNS_NAME_FORMATSIZE);

		isc_log_write(dns_lctx, DNS_LOGCATEGORY_GENERAL,
			      DNS_LOGMODULE_MASTER, ISC_LOG_DEBUG(3),
			      "catz: iterating over '%s' from catalog '%s'",
			      zname, czname);
		dns_catz_options_setdefault(catz->catzs->mctx,
					    &catz->zoneoptions, &nentry->opts);

		/* Try to find the zone in the view */
		zt_find_result = dns_zt_find(catz->catzs->view->zonetable,
					     dns_catz_entry_getname(nentry), 0,
					     NULL, &zone);
		if (zt_find_result == ISC_R_SUCCESS) {
			dns_catz_coo_t *coo = NULL;
			char pczname[DNS_NAME_FORMATSIZE];
			bool parentcatz_locked = false;

			/*
			 * Change of ownership (coo) processing, if required
			 */
			parentcatz = dns_zone_get_parentcatz(zone);
			if (parentcatz != NULL && parentcatz != catz) {
				UNLOCK(&catz->lock);
				LOCK(&parentcatz->lock);
				parentcatz_locked = true;
			}
			if (parentcatz_locked &&
			    isc_ht_find(parentcatz->coos, nentry->name.ndata,
					nentry->name.length,
					(void **)&coo) == ISC_R_SUCCESS &&
			    dns_name_equal(&coo->name, &catz->name))
			{
				dns_name_format(&parentcatz->name, pczname,
						DNS_NAME_FORMATSIZE);
				isc_log_write(dns_lctx, DNS_LOGCATEGORY_GENERAL,
					      DNS_LOGMODULE_MASTER,
					      ISC_LOG_DEBUG(3),
					      "catz: zone '%s' "
					      "change of ownership from "
					      "'%s' to '%s'",
					      zname, pczname, czname);
				result = delzone(nentry, parentcatz,
						 parentcatz->catzs->view,
						 parentcatz->catzs->taskmgr,
						 parentcatz->catzs->zmm->udata);
				isc_log_write(dns_lctx, DNS_LOGCATEGORY_GENERAL,
					      DNS_LOGMODULE_MASTER,
					      ISC_LOG_INFO,
					      "catz: deleting zone '%s' "
					      "from catalog '%s' - %s",
					      zname, pczname,
					      isc_result_totext(result));
			}
			if (parentcatz_locked) {
				UNLOCK(&parentcatz->lock);
				LOCK(&catz->lock);
			}
		}
		if (zt_find_result == ISC_R_SUCCESS ||
		    zt_find_result == DNS_R_PARTIALMATCH)
		{
			dns_zone_detach(&zone);
		}

		/* Try to find the zone in the old catalog zone */
		result = isc_ht_find(catz->entries, key, (uint32_t)keysize,
				     (void **)&oentry);
		if (result != ISC_R_SUCCESS) {
			if (zt_find_result == ISC_R_SUCCESS &&
			    parentcatz == catz)
			{
				/*
				 * This means that the zone's unique label
				 * has been changed, in that case we must
				 * reset the zone's internal state by removing
				 * and re-adding it.
				 *
				 * Scheduling the addition now, the removal will
				 * be scheduled below, when walking the old
				 * zone for remaining entries, and then we will
				 * perform deletions earlier than additions and
				 * modifications.
				 */
				isc_log_write(dns_lctx, DNS_LOGCATEGORY_GENERAL,
					      DNS_LOGMODULE_MASTER,
					      ISC_LOG_INFO,
					      "catz: zone '%s' unique label "
					      "has changed, reset state",
					      zname);
			}

			catz_entry_add_or_mod(catz, toadd, key, keysize, nentry,
					      NULL, "adding", zname, czname);
			continue;
		}

		if (zt_find_result != ISC_R_SUCCESS) {
			isc_log_write(dns_lctx, DNS_LOGCATEGORY_GENERAL,
				      DNS_LOGMODULE_MASTER, ISC_LOG_DEBUG(3),
				      "catz: zone '%s' was expected to exist "
				      "but can not be found, will be restored",
				      zname);
			catz_entry_add_or_mod(catz, toadd, key, keysize, nentry,
					      oentry, "adding", zname, czname);
			continue;
		}

		if (dns_catz_entry_cmp(oentry, nentry) != true) {
			catz_entry_add_or_mod(catz, tomod, key, keysize, nentry,
					      oentry, "modifying", zname,
					      czname);
			continue;
		}

		/*
		 * Delete the old entry so that it won't accidentally be
		 * removed as a non-existing entry below.
		 */
		dns_catz_entry_detach(catz, &oentry);
		result = isc_ht_delete(catz->entries, key, (uint32_t)keysize);
		RUNTIME_CHECK(result == ISC_R_SUCCESS);
	}
	RUNTIME_CHECK(result == ISC_R_NOMORE);
	isc_ht_iter_destroy(&iter1);

	/*
	 * Then - walk the old zone; only deleted entries should remain.
	 */
	for (result = isc_ht_iter_first(iter2); result == ISC_R_SUCCESS;
	     result = isc_ht_iter_delcurrent_next(iter2))
	{
		dns_catz_entry_t *entry = NULL;
		isc_ht_iter_current(iter2, (void **)&entry);

		dns_name_format(&entry->name, zname, DNS_NAME_FORMATSIZE);
		result = delzone(entry, catz, catz->catzs->view,
				 catz->catzs->taskmgr, catz->catzs->zmm->udata);
		isc_log_write(dns_lctx, DNS_LOGCATEGORY_GENERAL,
			      DNS_LOGMODULE_MASTER, ISC_LOG_INFO,
			      "catz: deleting zone '%s' from catalog '%s' - %s",
			      zname, czname, isc_result_totext(result));
		dns_catz_entry_detach(catz, &entry);
	}
	RUNTIME_CHECK(result == ISC_R_NOMORE);
	isc_ht_iter_destroy(&iter2);
	/* At this moment catz->entries has to be be empty. */
	INSIST(isc_ht_count(catz->entries) == 0);
	isc_ht_destroy(&catz->entries);

	for (result = isc_ht_iter_first(iteradd); result == ISC_R_SUCCESS;
	     result = isc_ht_iter_delcurrent_next(iteradd))
	{
		dns_catz_entry_t *entry = NULL;
		isc_ht_iter_current(iteradd, (void **)&entry);

		dns_name_format(&entry->name, zname, DNS_NAME_FORMATSIZE);
		result = addzone(entry, catz, catz->catzs->view,
				 catz->catzs->taskmgr, catz->catzs->zmm->udata);
		isc_log_write(dns_lctx, DNS_LOGCATEGORY_GENERAL,
			      DNS_LOGMODULE_MASTER, ISC_LOG_INFO,
			      "catz: adding zone '%s' from catalog "
			      "'%s' - %s",
			      zname, czname, isc_result_totext(result));
	}

	for (result = isc_ht_iter_first(itermod); result == ISC_R_SUCCESS;
	     result = isc_ht_iter_delcurrent_next(itermod))
	{
		dns_catz_entry_t *entry = NULL;
		isc_ht_iter_current(itermod, (void **)&entry);

		dns_name_format(&entry->name, zname, DNS_NAME_FORMATSIZE);
		result = modzone(entry, catz, catz->catzs->view,
				 catz->catzs->taskmgr, catz->catzs->zmm->udata);
		isc_log_write(dns_lctx, DNS_LOGCATEGORY_GENERAL,
			      DNS_LOGMODULE_MASTER, ISC_LOG_INFO,
			      "catz: modifying zone '%s' from catalog "
			      "'%s' - %s",
			      zname, czname, isc_result_totext(result));
	}

	catz->entries = newcatz->entries;
	newcatz->entries = NULL;

	/*
	 * We do not need to merge old coo (change of ownership) permission
	 * records with the new ones, just replace them.
	 */
	if (catz->coos != NULL && newcatz->coos != NULL) {
		isc_ht_iter_t *iter = NULL;

		isc_ht_iter_create(catz->coos, &iter);
		for (result = isc_ht_iter_first(iter); result == ISC_R_SUCCESS;
		     result = isc_ht_iter_delcurrent_next(iter))
		{
			dns_catz_coo_t *coo = NULL;

			isc_ht_iter_current(iter, (void **)&coo);
			catz_coo_detach(catz, &coo);
		}
		INSIST(result == ISC_R_NOMORE);
		isc_ht_iter_destroy(&iter);

		/* The hashtable has to be empty now. */
		INSIST(isc_ht_count(catz->coos) == 0);
		isc_ht_destroy(&catz->coos);

		catz->coos = newcatz->coos;
		newcatz->coos = NULL;
	}

	result = ISC_R_SUCCESS;

	isc_ht_iter_destroy(&iteradd);
	isc_ht_iter_destroy(&itermod);
	isc_ht_destroy(&toadd);
	isc_ht_destroy(&tomod);

	UNLOCK(&catz->lock);

	return (result);
}

isc_result_t
dns_catz_new_zones(isc_mem_t *mctx, isc_taskmgr_t *taskmgr,
		   isc_timermgr_t *timermgr, dns_catz_zones_t **catzsp,
		   dns_catz_zonemodmethods_t *zmm) {
	isc_result_t result;
	dns_catz_zones_t *catzs = NULL;

	REQUIRE(mctx != NULL);
	REQUIRE(taskmgr != NULL);
	REQUIRE(timermgr != NULL);
	REQUIRE(catzsp != NULL && *catzsp == NULL);
	REQUIRE(zmm != NULL);

	catzs = isc_mem_get(mctx, sizeof(*catzs));
	*catzs = (dns_catz_zones_t){ .taskmgr = taskmgr,
				     .timermgr = timermgr,
				     .zmm = zmm,
				     .magic = DNS_CATZ_ZONES_MAGIC };

	result = isc_taskmgr_excltask(taskmgr, &catzs->updater);
	if (result != ISC_R_SUCCESS) {
		goto cleanup_task;
	}

	isc_mutex_init(&catzs->lock);
	isc_refcount_init(&catzs->references, 1);
	isc_ht_init(&catzs->zones, mctx, 4, ISC_HT_CASE_SENSITIVE);
	isc_mem_attach(mctx, &catzs->mctx);

	*catzsp = catzs;

	return (ISC_R_SUCCESS);

cleanup_task:
	isc_mem_put(mctx, catzs, sizeof(*catzs));

	return (result);
}

void
dns_catz_catzs_set_view(dns_catz_zones_t *catzs, dns_view_t *view) {
	REQUIRE(DNS_CATZ_ZONES_VALID(catzs));
	REQUIRE(DNS_VIEW_VALID(view));
	/* Either it's a new one or it's being reconfigured. */
	REQUIRE(catzs->view == NULL || !strcmp(catzs->view->name, view->name));

	catzs->view = view;
}

isc_result_t
dns_catz_new_zone(dns_catz_zones_t *catzs, dns_catz_zone_t **catzp,
		  const dns_name_t *name) {
	isc_result_t result;
	dns_catz_zone_t *catz = NULL;

	REQUIRE(DNS_CATZ_ZONES_VALID(catzs));
	REQUIRE(catzp != NULL && *catzp == NULL);
	REQUIRE(ISC_MAGIC_VALID(name, DNS_NAME_MAGIC));

	catz = isc_mem_get(catzs->mctx, sizeof(*catz));
	*catz = (dns_catz_zone_t){ .active = true,
				   .version = DNS_CATZ_VERSION_UNDEFINED,
				   .magic = DNS_CATZ_ZONE_MAGIC };

	result = isc_timer_create(catzs->timermgr, isc_timertype_inactive, NULL,
				  NULL, catzs->updater, dns__catz_timer_cb,
				  catz, &catz->updatetimer);
	if (result != ISC_R_SUCCESS) {
		goto cleanup_timer;
	}

	dns_catz_zones_attach(catzs, &catz->catzs);
	isc_mutex_init(&catz->lock);
	isc_refcount_init(&catz->references, 1);
	isc_ht_init(&catz->entries, catzs->mctx, 4, ISC_HT_CASE_SENSITIVE);
	isc_ht_init(&catz->coos, catzs->mctx, 4, ISC_HT_CASE_INSENSITIVE);
	isc_time_settoepoch(&catz->lastupdated);
	dns_catz_options_init(&catz->defoptions);
	dns_catz_options_init(&catz->zoneoptions);
	dns_name_init(&catz->name, NULL);
	dns_name_dup(name, catzs->mctx, &catz->name);

	*catzp = catz;

	return (ISC_R_SUCCESS);

cleanup_timer:
	isc_mem_put(catzs->mctx, catz, sizeof(*catz));

	return (result);
}

isc_result_t
dns_catz_add_zone(dns_catz_zones_t *catzs, const dns_name_t *name,
		  dns_catz_zone_t **catzp) {
	dns_catz_zone_t *catz = NULL;
	isc_result_t result, tresult;
	char zname[DNS_NAME_FORMATSIZE];

	REQUIRE(DNS_CATZ_ZONES_VALID(catzs));
	REQUIRE(ISC_MAGIC_VALID(name, DNS_NAME_MAGIC));
	REQUIRE(catzp != NULL && *catzp == NULL);

	dns_name_format(name, zname, DNS_NAME_FORMATSIZE);
	isc_log_write(dns_lctx, DNS_LOGCATEGORY_GENERAL, DNS_LOGMODULE_MASTER,
		      ISC_LOG_DEBUG(3), "catz: dns_catz_add_zone %s", zname);

	LOCK(&catzs->lock);

	/*
	 * This function is called only during a (re)configuration, while
	 * 'catzs->zones' can become NULL only during shutdown.
	 */
	INSIST(catzs->zones != NULL);
	INSIST(!atomic_load(&catzs->shuttingdown));

	result = dns_catz_new_zone(catzs, &catz, name);
	if (result != ISC_R_SUCCESS) {
		goto cleanup;
	}

	result = isc_ht_add(catzs->zones, catz->name.ndata, catz->name.length,
			    catz);
	if (result != ISC_R_SUCCESS) {
		dns_catz_detach_catz(&catz);
		if (result != ISC_R_EXISTS) {
			goto cleanup;
		}
	}

	if (result == ISC_R_EXISTS) {
		tresult = isc_ht_find(catzs->zones, name->ndata, name->length,
				      (void **)&catz);
		INSIST(tresult == ISC_R_SUCCESS && !catz->active);
		catz->active = true;
	}

	*catzp = catz;

cleanup:
	UNLOCK(&catzs->lock);

	return (result);
}

dns_catz_zone_t *
dns_catz_get_zone(dns_catz_zones_t *catzs, const dns_name_t *name) {
	isc_result_t result;
	dns_catz_zone_t *found = NULL;

	REQUIRE(DNS_CATZ_ZONES_VALID(catzs));
	REQUIRE(ISC_MAGIC_VALID(name, DNS_NAME_MAGIC));

	LOCK(&catzs->lock);
	if (catzs->zones == NULL) {
		UNLOCK(&catzs->lock);
		return (NULL);
	}
	result = isc_ht_find(catzs->zones, name->ndata, name->length,
			     (void **)&found);
	UNLOCK(&catzs->lock);
	if (result != ISC_R_SUCCESS) {
		return (NULL);
	}

	return (found);
}

static void
dns__catz_shutdown(dns_catz_zone_t *catz) {
	/* lock must be locked */
	if (catz->updatetimer != NULL) {
		isc_result_t result;

		/* Don't wait for timer to trigger for shutdown */
		result = isc_timer_reset(catz->updatetimer,
					 isc_timertype_inactive, NULL, NULL,
					 true);
		RUNTIME_CHECK(result == ISC_R_SUCCESS);
	}

	dns_catz_detach_catz(&catz);
}

static void
dns__catz_zone_destroy(dns_catz_zone_t *catz) {
	isc_mem_t *mctx = catz->catzs->mctx;

	if (catz->entries != NULL) {
		isc_ht_iter_t *iter = NULL;
		isc_result_t result;
		isc_ht_iter_create(catz->entries, &iter);
		for (result = isc_ht_iter_first(iter); result == ISC_R_SUCCESS;
		     result = isc_ht_iter_delcurrent_next(iter))
		{
			dns_catz_entry_t *entry = NULL;

			isc_ht_iter_current(iter, (void **)&entry);
			dns_catz_entry_detach(catz, &entry);
		}
		INSIST(result == ISC_R_NOMORE);
		isc_ht_iter_destroy(&iter);

		/* The hashtable has to be empty now. */
		INSIST(isc_ht_count(catz->entries) == 0);
		isc_ht_destroy(&catz->entries);
	}
	if (catz->coos != NULL) {
		isc_ht_iter_t *iter = NULL;
		isc_result_t result;
		isc_ht_iter_create(catz->coos, &iter);
		for (result = isc_ht_iter_first(iter); result == ISC_R_SUCCESS;
		     result = isc_ht_iter_delcurrent_next(iter))
		{
			dns_catz_coo_t *coo = NULL;

			isc_ht_iter_current(iter, (void **)&coo);
			catz_coo_detach(catz, &coo);
		}
		INSIST(result == ISC_R_NOMORE);
		isc_ht_iter_destroy(&iter);

		/* The hashtable has to be empty now. */
		INSIST(isc_ht_count(catz->coos) == 0);
		isc_ht_destroy(&catz->coos);
	}
	catz->magic = 0;

	isc_mutex_destroy(&catz->lock);
	isc_timer_destroy(&catz->updatetimer);
	if (catz->db_registered) {
		dns_db_updatenotify_unregister(
			catz->db, dns_catz_dbupdate_callback, catz->catzs);
	}
	if (catz->dbversion != NULL) {
		dns_db_closeversion(catz->db, &catz->dbversion, false);
	}
	if (catz->db != NULL) {
		dns_db_detach(&catz->db);
	}

	INSIST(!catz->updaterunning);

	dns_name_free(&catz->name, mctx);
	dns_catz_options_free(&catz->defoptions, mctx);
	dns_catz_options_free(&catz->zoneoptions, mctx);

	dns_catz_zones_detach(&catz->catzs);
	isc_refcount_destroy(&catz->references);

	isc_mem_put(mctx, catz, sizeof(*catz));
}

static void
dns__catz_zones_destroy(dns_catz_zones_t *catzs) {
	REQUIRE(atomic_load(&catzs->shuttingdown));
	REQUIRE(catzs->zones == NULL);

	catzs->magic = 0;
	isc_task_detach(&catzs->updater);
	isc_mutex_destroy(&catzs->lock);
	isc_refcount_destroy(&catzs->references);

	isc_mem_putanddetach(&catzs->mctx, catzs, sizeof(*catzs));
}

void
dns_catz_shutdown_catzs(dns_catz_zones_t *catzs) {
	REQUIRE(DNS_CATZ_ZONES_VALID(catzs));

	if (!atomic_compare_exchange_strong(&catzs->shuttingdown,
					    &(bool){ false }, true))
	{
		return;
	}

	LOCK(&catzs->lock);
	if (catzs->zones != NULL) {
		isc_ht_iter_t *iter = NULL;
		isc_result_t result;
		isc_ht_iter_create(catzs->zones, &iter);
		for (result = isc_ht_iter_first(iter); result == ISC_R_SUCCESS;)
		{
			dns_catz_zone_t *catz = NULL;
			isc_ht_iter_current(iter, (void **)&catz);
			result = isc_ht_iter_delcurrent_next(iter);
			dns__catz_shutdown(catz);
		}
		INSIST(result == ISC_R_NOMORE);
		isc_ht_iter_destroy(&iter);
		INSIST(isc_ht_count(catzs->zones) == 0);
		isc_ht_destroy(&catzs->zones);
	}
	UNLOCK(&catzs->lock);
}

#ifdef DNS_CATZ_TRACE
ISC_REFCOUNT_TRACE_IMPL(dns_catz_zone, dns__catz_zone_destroy);
ISC_REFCOUNT_TRACE_IMPL(dns_catz_zones, dns__catz_zones_destroy);
#else
ISC_REFCOUNT_IMPL(dns_catz_zone, dns__catz_zone_destroy);
ISC_REFCOUNT_IMPL(dns_catz_zones, dns__catz_zones_destroy);
#endif

typedef enum {
	CATZ_OPT_NONE,
	CATZ_OPT_ZONES,
	CATZ_OPT_COO,
	CATZ_OPT_VERSION,
	CATZ_OPT_CUSTOM_START, /* CATZ custom properties must go below this */
	CATZ_OPT_EXT,
	CATZ_OPT_PRIMARIES,
	CATZ_OPT_ALLOW_QUERY,
	CATZ_OPT_ALLOW_TRANSFER,
} catz_opt_t;

static bool
catz_opt_cmp(const dns_label_t *option, const char *opt) {
	size_t len = strlen(opt);

	if (option->length - 1 == len &&
	    memcmp(opt, option->base + 1, len) == 0)
	{
		return (true);
	} else {
		return (false);
	}
}

static catz_opt_t
catz_get_option(const dns_label_t *option) {
	if (catz_opt_cmp(option, "ext")) {
		return (CATZ_OPT_EXT);
	} else if (catz_opt_cmp(option, "zones")) {
		return (CATZ_OPT_ZONES);
	} else if (catz_opt_cmp(option, "masters") ||
		   catz_opt_cmp(option, "primaries"))
	{
		return (CATZ_OPT_PRIMARIES);
	} else if (catz_opt_cmp(option, "allow-query")) {
		return (CATZ_OPT_ALLOW_QUERY);
	} else if (catz_opt_cmp(option, "allow-transfer")) {
		return (CATZ_OPT_ALLOW_TRANSFER);
	} else if (catz_opt_cmp(option, "coo")) {
		return (CATZ_OPT_COO);
	} else if (catz_opt_cmp(option, "version")) {
		return (CATZ_OPT_VERSION);
	} else {
		return (CATZ_OPT_NONE);
	}
}

static isc_result_t
catz_process_zones(dns_catz_zone_t *catz, dns_rdataset_t *value,
		   dns_name_t *name) {
	dns_label_t mhash;
	dns_name_t opt;

	REQUIRE(DNS_CATZ_ZONE_VALID(catz));
	REQUIRE(DNS_RDATASET_VALID(value));
	REQUIRE(ISC_MAGIC_VALID(name, DNS_NAME_MAGIC));

	if (name->labels == 0) {
		return (ISC_R_FAILURE);
	}

	dns_name_getlabel(name, name->labels - 1, &mhash);

	if (name->labels == 1) {
		return (catz_process_zones_entry(catz, value, &mhash));
	} else {
		dns_name_init(&opt, NULL);
		dns_name_split(name, 1, &opt, NULL);
		return (catz_process_zones_suboption(catz, value, &mhash,
						     &opt));
	}
}

static isc_result_t
catz_process_coo(dns_catz_zone_t *catz, dns_label_t *mhash,
		 dns_rdataset_t *value) {
	isc_result_t result;
	dns_rdata_t rdata;
	dns_rdata_ptr_t ptr;
	dns_catz_entry_t *entry = NULL;
	dns_catz_coo_t *ncoo = NULL;
	dns_catz_coo_t *ocoo = NULL;

	REQUIRE(DNS_CATZ_ZONE_VALID(catz));
	REQUIRE(mhash != NULL);
	REQUIRE(DNS_RDATASET_VALID(value));

	/* Change of Ownership was introduced in version "2" of the schema. */
	if (catz->version < 2) {
		return (ISC_R_FAILURE);
	}

	if (value->type != dns_rdatatype_ptr) {
		return (ISC_R_FAILURE);
	}

	if (dns_rdataset_count(value) != 1) {
		isc_log_write(dns_lctx, DNS_LOGCATEGORY_GENERAL,
			      DNS_LOGMODULE_MASTER, ISC_LOG_WARNING,
			      "catz: 'coo' property PTR RRset contains "
			      "more than one record, which is invalid");
		catz->broken = true;
		return (ISC_R_FAILURE);
	}

	result = dns_rdataset_first(value);
	if (result != ISC_R_SUCCESS) {
		return (result);
	}

	dns_rdata_init(&rdata);
	dns_rdataset_current(value, &rdata);

	result = dns_rdata_tostruct(&rdata, &ptr, NULL);
	if (result != ISC_R_SUCCESS) {
		return (result);
	}

	if (dns_name_countlabels(&ptr.ptr) == 0) {
		result = ISC_R_FAILURE;
		goto cleanup;
	}

	result = isc_ht_find(catz->entries, mhash->base, mhash->length,
			     (void **)&entry);
	if (result != ISC_R_SUCCESS) {
		/* The entry was not found .*/
		goto cleanup;
	}

	if (dns_name_countlabels(&entry->name) == 0) {
		result = ISC_R_FAILURE;
		goto cleanup;
	}

	result = isc_ht_find(catz->coos, entry->name.ndata, entry->name.length,
			     (void **)&ocoo);
	if (result == ISC_R_SUCCESS) {
		/* The change of ownership permission was already registered. */
		goto cleanup;
	}

	catz_coo_new(catz->catzs->mctx, &ptr.ptr, &ncoo);
	result = isc_ht_add(catz->coos, entry->name.ndata, entry->name.length,
			    ncoo);
	if (result != ISC_R_SUCCESS) {
		catz_coo_detach(catz, &ncoo);
	}

cleanup:
	dns_rdata_freestruct(&ptr);

	return (result);
}

static isc_result_t
catz_process_zones_entry(dns_catz_zone_t *catz, dns_rdataset_t *value,
			 dns_label_t *mhash) {
	isc_result_t result;
	dns_rdata_t rdata;
	dns_rdata_ptr_t ptr;
	dns_catz_entry_t *entry = NULL;

	if (value->type != dns_rdatatype_ptr) {
		return (ISC_R_FAILURE);
	}

	if (dns_rdataset_count(value) != 1) {
		isc_log_write(dns_lctx, DNS_LOGCATEGORY_GENERAL,
			      DNS_LOGMODULE_MASTER, ISC_LOG_WARNING,
			      "catz: member zone PTR RRset contains "
			      "more than one record, which is invalid");
		catz->broken = true;
		return (ISC_R_FAILURE);
	}

	result = dns_rdataset_first(value);
	if (result != ISC_R_SUCCESS) {
		return (result);
	}

	dns_rdata_init(&rdata);
	dns_rdataset_current(value, &rdata);

	result = dns_rdata_tostruct(&rdata, &ptr, NULL);
	if (result != ISC_R_SUCCESS) {
		return (result);
	}

	result = isc_ht_find(catz->entries, mhash->base, mhash->length,
			     (void **)&entry);
	if (result == ISC_R_SUCCESS) {
		if (dns_name_countlabels(&entry->name) != 0) {
			/* We have a duplicate. */
			dns_rdata_freestruct(&ptr);
			return (ISC_R_FAILURE);
		} else {
			dns_name_dup(&ptr.ptr, catz->catzs->mctx, &entry->name);
		}
	} else {
		dns_catz_entry_new(catz->catzs->mctx, &ptr.ptr, &entry);

		result = isc_ht_add(catz->entries, mhash->base, mhash->length,
				    entry);
		if (result != ISC_R_SUCCESS) {
			dns_rdata_freestruct(&ptr);
			dns_catz_entry_detach(catz, &entry);
			return (result);
		}
	}

	dns_rdata_freestruct(&ptr);

	return (ISC_R_SUCCESS);
}

static isc_result_t
catz_process_version(dns_catz_zone_t *catz, dns_rdataset_t *value) {
	isc_result_t result;
	dns_rdata_t rdata;
	dns_rdata_txt_t rdatatxt;
	dns_rdata_txt_string_t rdatastr;
	uint32_t tversion;
	char t[16];

	REQUIRE(DNS_CATZ_ZONE_VALID(catz));
	REQUIRE(DNS_RDATASET_VALID(value));

	if (value->type != dns_rdatatype_txt) {
		return (ISC_R_FAILURE);
	}

	if (dns_rdataset_count(value) != 1) {
		isc_log_write(dns_lctx, DNS_LOGCATEGORY_GENERAL,
			      DNS_LOGMODULE_MASTER, ISC_LOG_WARNING,
			      "catz: 'version' property TXT RRset contains "
			      "more than one record, which is invalid");
		catz->broken = true;
		return (ISC_R_FAILURE);
	}

	result = dns_rdataset_first(value);
	if (result != ISC_R_SUCCESS) {
		return (result);
	}

	dns_rdata_init(&rdata);
	dns_rdataset_current(value, &rdata);

	result = dns_rdata_tostruct(&rdata, &rdatatxt, NULL);
	if (result != ISC_R_SUCCESS) {
		return (result);
	}

	result = dns_rdata_txt_first(&rdatatxt);
	if (result != ISC_R_SUCCESS) {
		goto cleanup;
	}

	result = dns_rdata_txt_current(&rdatatxt, &rdatastr);
	if (result != ISC_R_SUCCESS) {
		goto cleanup;
	}

	result = dns_rdata_txt_next(&rdatatxt);
	if (result != ISC_R_NOMORE) {
		result = ISC_R_FAILURE;
		goto cleanup;
	}
	if (rdatastr.length > 15) {
		result = ISC_R_BADNUMBER;
		goto cleanup;
	}
	memmove(t, rdatastr.data, rdatastr.length);
	t[rdatastr.length] = 0;
	result = isc_parse_uint32(&tversion, t, 10);
	if (result != ISC_R_SUCCESS) {
		goto cleanup;
	}
	catz->version = tversion;
	result = ISC_R_SUCCESS;

cleanup:
	dns_rdata_freestruct(&rdatatxt);
	if (result != ISC_R_SUCCESS) {
		isc_log_write(dns_lctx, DNS_LOGCATEGORY_GENERAL,
			      DNS_LOGMODULE_MASTER, ISC_LOG_WARNING,
			      "catz: invalid record for the catalog "
			      "zone version property");
		catz->broken = true;
	}
	return (result);
}

static isc_result_t
catz_process_primaries(dns_catz_zone_t *catz, dns_ipkeylist_t *ipkl,
		       dns_rdataset_t *value, dns_name_t *name) {
	isc_result_t result;
	dns_rdata_t rdata;
	dns_rdata_in_a_t rdata_a;
	dns_rdata_in_aaaa_t rdata_aaaa;
	dns_rdata_txt_t rdata_txt;
	dns_rdata_txt_string_t rdatastr;
	dns_name_t *keyname = NULL;
	isc_mem_t *mctx;
	char keycbuf[DNS_NAME_FORMATSIZE];
	isc_buffer_t keybuf;
	unsigned int rcount;

	REQUIRE(DNS_CATZ_ZONE_VALID(catz));
	REQUIRE(ipkl != NULL);
	REQUIRE(DNS_RDATASET_VALID(value));
	REQUIRE(dns_rdataset_isassociated(value));
	REQUIRE(ISC_MAGIC_VALID(name, DNS_NAME_MAGIC));

	mctx = catz->catzs->mctx;
	memset(&rdata_a, 0, sizeof(rdata_a));
	memset(&rdata_aaaa, 0, sizeof(rdata_aaaa));
	memset(&rdata_txt, 0, sizeof(rdata_txt));
	isc_buffer_init(&keybuf, keycbuf, sizeof(keycbuf));

	/*
	 * We have three possibilities here:
	 * - either empty name and IN A/IN AAAA record
	 * - label and IN A/IN AAAA
	 * - label and IN TXT - TSIG key name
	 */
	if (name->labels > 0) {
		isc_sockaddr_t sockaddr;
		size_t i;

		/*
		 * We're pre-preparing the data once, we'll put it into
		 * the right spot in the primaries array once we find it.
		 */
		result = dns_rdataset_first(value);
		RUNTIME_CHECK(result == ISC_R_SUCCESS);
		dns_rdata_init(&rdata);
		dns_rdataset_current(value, &rdata);
		switch (value->type) {
		case dns_rdatatype_a:
			result = dns_rdata_tostruct(&rdata, &rdata_a, NULL);
			RUNTIME_CHECK(result == ISC_R_SUCCESS);
			isc_sockaddr_fromin(&sockaddr, &rdata_a.in_addr, 0);
			dns_rdata_freestruct(&rdata_a);
			break;
		case dns_rdatatype_aaaa:
			result = dns_rdata_tostruct(&rdata, &rdata_aaaa, NULL);
			RUNTIME_CHECK(result == ISC_R_SUCCESS);
			isc_sockaddr_fromin6(&sockaddr, &rdata_aaaa.in6_addr,
					     0);
			dns_rdata_freestruct(&rdata_aaaa);
			break;
		case dns_rdatatype_txt:
			result = dns_rdata_tostruct(&rdata, &rdata_txt, NULL);
			RUNTIME_CHECK(result == ISC_R_SUCCESS);

			result = dns_rdata_txt_first(&rdata_txt);
			if (result != ISC_R_SUCCESS) {
				dns_rdata_freestruct(&rdata_txt);
				return (result);
			}

			result = dns_rdata_txt_current(&rdata_txt, &rdatastr);
			if (result != ISC_R_SUCCESS) {
				dns_rdata_freestruct(&rdata_txt);
				return (result);
			}

			result = dns_rdata_txt_next(&rdata_txt);
			if (result != ISC_R_NOMORE) {
				dns_rdata_freestruct(&rdata_txt);
				return (ISC_R_FAILURE);
			}

			/* rdatastr.length < DNS_NAME_MAXTEXT */
			keyname = isc_mem_get(mctx, sizeof(*keyname));
			dns_name_init(keyname, 0);
			memmove(keycbuf, rdatastr.data, rdatastr.length);
			keycbuf[rdatastr.length] = 0;
			dns_rdata_freestruct(&rdata_txt);
			result = dns_name_fromstring(keyname, keycbuf, 0, mctx);
			if (result != ISC_R_SUCCESS) {
				dns_name_free(keyname, mctx);
				isc_mem_put(mctx, keyname, sizeof(*keyname));
				return (result);
			}
			break;
		default:
			return (ISC_R_FAILURE);
		}

		/*
		 * We have to find the appropriate labeled record in
		 * primaries if it exists.  In the common case we'll
		 * have no more than 3-4 records here, so no optimization.
		 */
		for (i = 0; i < ipkl->count; i++) {
			if (ipkl->labels[i] != NULL &&
			    !dns_name_compare(name, ipkl->labels[i]))
			{
				break;
			}
		}

		if (i < ipkl->count) { /* we have this record already */
			if (value->type == dns_rdatatype_txt) {
				ipkl->keys[i] = keyname;
			} else { /* A/AAAA */
				memmove(&ipkl->addrs[i], &sockaddr,
					sizeof(sockaddr));
			}
		} else {
			result = dns_ipkeylist_resize(mctx, ipkl, i + 1);
			if (result != ISC_R_SUCCESS) {
				return (result);
			}

			ipkl->labels[i] = isc_mem_get(mctx,
						      sizeof(*ipkl->labels[0]));
			dns_name_init(ipkl->labels[i], NULL);
			dns_name_dup(name, mctx, ipkl->labels[i]);

			if (value->type == dns_rdatatype_txt) {
				ipkl->keys[i] = keyname;
			} else { /* A/AAAA */
				memmove(&ipkl->addrs[i], &sockaddr,
					sizeof(sockaddr));
			}
			ipkl->count++;
		}
		return (ISC_R_SUCCESS);
	}
	/* else - 'simple' case - without labels */

	if (value->type != dns_rdatatype_a && value->type != dns_rdatatype_aaaa)
	{
		return (ISC_R_FAILURE);
	}

	rcount = dns_rdataset_count(value) + ipkl->count;

	result = dns_ipkeylist_resize(mctx, ipkl, rcount);
	if (result != ISC_R_SUCCESS) {
		return (result);
	}

	for (result = dns_rdataset_first(value); result == ISC_R_SUCCESS;
	     result = dns_rdataset_next(value))
	{
		dns_rdata_init(&rdata);
		dns_rdataset_current(value, &rdata);
		/*
		 * port 0 == take the default
		 */
		if (value->type == dns_rdatatype_a) {
			result = dns_rdata_tostruct(&rdata, &rdata_a, NULL);
			RUNTIME_CHECK(result == ISC_R_SUCCESS);
			isc_sockaddr_fromin(&ipkl->addrs[ipkl->count],
					    &rdata_a.in_addr, 0);
			dns_rdata_freestruct(&rdata_a);
		} else {
			result = dns_rdata_tostruct(&rdata, &rdata_aaaa, NULL);
			RUNTIME_CHECK(result == ISC_R_SUCCESS);
			isc_sockaddr_fromin6(&ipkl->addrs[ipkl->count],
					     &rdata_aaaa.in6_addr, 0);
			dns_rdata_freestruct(&rdata_aaaa);
		}
		ipkl->keys[ipkl->count] = NULL;
		ipkl->labels[ipkl->count] = NULL;
		ipkl->count++;
	}
	return (ISC_R_SUCCESS);
}

static isc_result_t
catz_process_apl(dns_catz_zone_t *catz, isc_buffer_t **aclbp,
		 dns_rdataset_t *value) {
	isc_result_t result = ISC_R_SUCCESS;
	dns_rdata_t rdata;
	dns_rdata_in_apl_t rdata_apl;
	dns_rdata_apl_ent_t apl_ent;
	isc_netaddr_t addr;
	isc_buffer_t *aclb = NULL;
	unsigned char buf[256]; /* larger than INET6_ADDRSTRLEN */

	REQUIRE(DNS_CATZ_ZONE_VALID(catz));
	REQUIRE(aclbp != NULL);
	REQUIRE(*aclbp == NULL);
	REQUIRE(DNS_RDATASET_VALID(value));
	REQUIRE(dns_rdataset_isassociated(value));

	if (value->type != dns_rdatatype_apl) {
		return (ISC_R_FAILURE);
	}

	if (dns_rdataset_count(value) > 1) {
		isc_log_write(dns_lctx, DNS_LOGCATEGORY_GENERAL,
			      DNS_LOGMODULE_MASTER, ISC_LOG_WARNING,
			      "catz: more than one APL entry for member zone, "
			      "result is undefined");
	}
	result = dns_rdataset_first(value);
	RUNTIME_CHECK(result == ISC_R_SUCCESS);
	dns_rdata_init(&rdata);
	dns_rdataset_current(value, &rdata);
	result = dns_rdata_tostruct(&rdata, &rdata_apl, catz->catzs->mctx);
	if (result != ISC_R_SUCCESS) {
		return (result);
	}
	isc_buffer_allocate(catz->catzs->mctx, &aclb, 16);
	isc_buffer_setautorealloc(aclb, true);
	for (result = dns_rdata_apl_first(&rdata_apl); result == ISC_R_SUCCESS;
	     result = dns_rdata_apl_next(&rdata_apl))
	{
		result = dns_rdata_apl_current(&rdata_apl, &apl_ent);
		RUNTIME_CHECK(result == ISC_R_SUCCESS);
		memset(buf, 0, sizeof(buf));
		if (apl_ent.data != NULL && apl_ent.length > 0) {
			memmove(buf, apl_ent.data, apl_ent.length);
		}
		if (apl_ent.family == 1) {
			isc_netaddr_fromin(&addr, (struct in_addr *)buf);
		} else if (apl_ent.family == 2) {
			isc_netaddr_fromin6(&addr, (struct in6_addr *)buf);
		} else {
			continue; /* xxxwpk log it or simply ignore? */
		}
		if (apl_ent.negative) {
			isc_buffer_putuint8(aclb, '!');
		}
		isc_buffer_reserve(&aclb, INET6_ADDRSTRLEN);
		result = isc_netaddr_totext(&addr, aclb);
		RUNTIME_CHECK(result == ISC_R_SUCCESS);
		if ((apl_ent.family == 1 && apl_ent.prefix < 32) ||
		    (apl_ent.family == 2 && apl_ent.prefix < 128))
		{
			isc_buffer_putuint8(aclb, '/');
			isc_buffer_putdecint(aclb, apl_ent.prefix);
		}
		isc_buffer_putstr(aclb, "; ");
	}
	if (result == ISC_R_NOMORE) {
		result = ISC_R_SUCCESS;
	} else {
		goto cleanup;
	}
	*aclbp = aclb;
	aclb = NULL;
cleanup:
	if (aclb != NULL) {
		isc_buffer_free(&aclb);
	}
	dns_rdata_freestruct(&rdata_apl);
	return (result);
}

static isc_result_t
catz_process_zones_suboption(dns_catz_zone_t *catz, dns_rdataset_t *value,
			     dns_label_t *mhash, dns_name_t *name) {
	isc_result_t result;
	dns_catz_entry_t *entry = NULL;
	dns_label_t option;
	dns_name_t prefix;
	catz_opt_t opt;
	unsigned int suffix_labels = 1;

	REQUIRE(DNS_CATZ_ZONE_VALID(catz));
	REQUIRE(mhash != NULL);
	REQUIRE(DNS_RDATASET_VALID(value));
	REQUIRE(ISC_MAGIC_VALID(name, DNS_NAME_MAGIC));

	if (name->labels < 1) {
		return (ISC_R_FAILURE);
	}
	dns_name_getlabel(name, name->labels - 1, &option);
	opt = catz_get_option(&option);

	/*
	 * The custom properties in version 2 schema must be placed under the
	 * "ext" label.
	 */
	if (catz->version >= 2 && opt >= CATZ_OPT_CUSTOM_START) {
		if (opt != CATZ_OPT_EXT || name->labels < 2) {
			return (ISC_R_FAILURE);
		}
		suffix_labels++;
		dns_name_getlabel(name, name->labels - 2, &option);
		opt = catz_get_option(&option);
	}

	/*
	 * We're adding this entry now, in case the option is invalid we'll get
	 * rid of it in verification phase.
	 */
	result = isc_ht_find(catz->entries, mhash->base, mhash->length,
			     (void **)&entry);
	if (result != ISC_R_SUCCESS) {
		dns_catz_entry_new(catz->catzs->mctx, NULL, &entry);
		result = isc_ht_add(catz->entries, mhash->base, mhash->length,
				    entry);
		if (result != ISC_R_SUCCESS) {
			dns_catz_entry_detach(catz, &entry);
			return (result);
		}
	}

	dns_name_init(&prefix, NULL);
	dns_name_split(name, suffix_labels, &prefix, NULL);
	switch (opt) {
	case CATZ_OPT_COO:
		return (catz_process_coo(catz, mhash, value));
	case CATZ_OPT_PRIMARIES:
		return (catz_process_primaries(catz, &entry->opts.masters,
					       value, &prefix));
	case CATZ_OPT_ALLOW_QUERY:
		if (prefix.labels != 0) {
			return (ISC_R_FAILURE);
		}
		return (catz_process_apl(catz, &entry->opts.allow_query,
					 value));
	case CATZ_OPT_ALLOW_TRANSFER:
		if (prefix.labels != 0) {
			return (ISC_R_FAILURE);
		}
		return (catz_process_apl(catz, &entry->opts.allow_transfer,
					 value));
	default:
		return (ISC_R_FAILURE);
	}

	return (ISC_R_FAILURE);
}

static void
catz_entry_add_or_mod(dns_catz_zone_t *catz, isc_ht_t *ht, unsigned char *key,
		      size_t keysize, dns_catz_entry_t *nentry,
		      dns_catz_entry_t *oentry, const char *msg,
		      const char *zname, const char *czname) {
	isc_result_t result = isc_ht_add(ht, key, (uint32_t)keysize, nentry);

	if (result != ISC_R_SUCCESS) {
		isc_log_write(dns_lctx, DNS_LOGCATEGORY_GENERAL,
			      DNS_LOGMODULE_MASTER, ISC_LOG_ERROR,
			      "catz: error %s zone '%s' from catalog '%s' - %s",
			      msg, zname, czname, isc_result_totext(result));
	}
	if (oentry != NULL) {
		dns_catz_entry_detach(catz, &oentry);
		result = isc_ht_delete(catz->entries, key, (uint32_t)keysize);
		RUNTIME_CHECK(result == ISC_R_SUCCESS);
	}
}

static isc_result_t
catz_process_value(dns_catz_zone_t *catz, dns_name_t *name,
		   dns_rdataset_t *rdataset) {
	dns_label_t option;
	dns_name_t prefix;
	catz_opt_t opt;
	unsigned int suffix_labels = 1;

	REQUIRE(DNS_CATZ_ZONE_VALID(catz));
	REQUIRE(ISC_MAGIC_VALID(name, DNS_NAME_MAGIC));
	REQUIRE(DNS_RDATASET_VALID(rdataset));

	if (name->labels < 1) {
		return (ISC_R_FAILURE);
	}
	dns_name_getlabel(name, name->labels - 1, &option);
	opt = catz_get_option(&option);

	/*
	 * The custom properties in version 2 schema must be placed under the
	 * "ext" label.
	 */
	if (catz->version >= 2 && opt >= CATZ_OPT_CUSTOM_START) {
		if (opt != CATZ_OPT_EXT || name->labels < 2) {
			return (ISC_R_FAILURE);
		}
		suffix_labels++;
		dns_name_getlabel(name, name->labels - 2, &option);
		opt = catz_get_option(&option);
	}

	dns_name_init(&prefix, NULL);
	dns_name_split(name, suffix_labels, &prefix, NULL);

	switch (opt) {
	case CATZ_OPT_ZONES:
		return (catz_process_zones(catz, rdataset, &prefix));
	case CATZ_OPT_PRIMARIES:
		return (catz_process_primaries(catz, &catz->zoneoptions.masters,
					       rdataset, &prefix));
	case CATZ_OPT_ALLOW_QUERY:
		if (prefix.labels != 0) {
			return (ISC_R_FAILURE);
		}
		return (catz_process_apl(catz, &catz->zoneoptions.allow_query,
					 rdataset));
	case CATZ_OPT_ALLOW_TRANSFER:
		if (prefix.labels != 0) {
			return (ISC_R_FAILURE);
		}
		return (catz_process_apl(
			catz, &catz->zoneoptions.allow_transfer, rdataset));
	case CATZ_OPT_VERSION:
		if (prefix.labels != 0) {
			return (ISC_R_FAILURE);
		}
		return (catz_process_version(catz, rdataset));
	default:
		return (ISC_R_FAILURE);
	}
}

/*%<
 * Process a single rdataset from a catalog zone 'catz' update, src_name is the
 * record name.
 *
 * Requires:
 * \li	'catz' is a valid dns_catz_zone_t.
 * \li	'src_name' is a valid dns_name_t.
 * \li	'rdataset' is valid rdataset.
 */
static isc_result_t
dns__catz_update_process(dns_catz_zone_t *catz, const dns_name_t *src_name,
			 dns_rdataset_t *rdataset) {
	isc_result_t result;
	int order;
	unsigned int nlabels;
	dns_namereln_t nrres;
	dns_rdata_t rdata = DNS_RDATA_INIT;
	dns_rdata_soa_t soa;
	dns_name_t prefix;

	REQUIRE(DNS_CATZ_ZONE_VALID(catz));
	REQUIRE(ISC_MAGIC_VALID(src_name, DNS_NAME_MAGIC));

	if (rdataset->rdclass != dns_rdataclass_in) {
		isc_log_write(dns_lctx, DNS_LOGCATEGORY_GENERAL,
			      DNS_LOGMODULE_MASTER, ISC_LOG_ERROR,
			      "catz: RR found which has a non-IN class");
		catz->broken = true;
		return (ISC_R_FAILURE);
	}

	nrres = dns_name_fullcompare(src_name, &catz->name, &order, &nlabels);
	if (nrres == dns_namereln_equal) {
		if (rdataset->type == dns_rdatatype_soa) {
			result = dns_rdataset_first(rdataset);
			if (result != ISC_R_SUCCESS) {
				return (result);
			}

			dns_rdataset_current(rdataset, &rdata);
			result = dns_rdata_tostruct(&rdata, &soa, NULL);
			RUNTIME_CHECK(result == ISC_R_SUCCESS);

			/*
			 * xxxwpk TODO do we want to save something from SOA?
			 */
			dns_rdata_freestruct(&soa);
			return (result);
		} else if (rdataset->type == dns_rdatatype_ns) {
			return (ISC_R_SUCCESS);
		} else {
			return (ISC_R_UNEXPECTED);
		}
	} else if (nrres != dns_namereln_subdomain) {
		return (ISC_R_UNEXPECTED);
	}

	dns_name_init(&prefix, NULL);
	dns_name_split(src_name, catz->name.labels, &prefix, NULL);
	result = catz_process_value(catz, &prefix, rdataset);

	return (result);
}

static isc_result_t
digest2hex(unsigned char *digest, unsigned int digestlen, char *hash,
	   size_t hashlen) {
	unsigned int i;
	for (i = 0; i < digestlen; i++) {
		size_t left = hashlen - i * 2;
		int ret = snprintf(hash + i * 2, left, "%02x", digest[i]);
		if (ret < 0 || (size_t)ret >= left) {
			return (ISC_R_NOSPACE);
		}
	}
	return (ISC_R_SUCCESS);
}

isc_result_t
dns_catz_generate_masterfilename(dns_catz_zone_t *catz, dns_catz_entry_t *entry,
				 isc_buffer_t **buffer) {
	isc_buffer_t *tbuf = NULL;
	isc_region_t r;
	isc_result_t result;
	size_t rlen;
	bool special = false;

	REQUIRE(DNS_CATZ_ZONE_VALID(catz));
	REQUIRE(DNS_CATZ_ENTRY_VALID(entry));
	REQUIRE(buffer != NULL && *buffer != NULL);

	isc_buffer_allocate(catz->catzs->mctx, &tbuf,
			    strlen(catz->catzs->view->name) +
				    2 * DNS_NAME_FORMATSIZE + 2);

	isc_buffer_putstr(tbuf, catz->catzs->view->name);
	isc_buffer_putstr(tbuf, "_");
	result = dns_name_totext(&catz->name, true, tbuf);
	if (result != ISC_R_SUCCESS) {
		goto cleanup;
	}

	isc_buffer_putstr(tbuf, "_");
	result = dns_name_totext(&entry->name, true, tbuf);
	if (result != ISC_R_SUCCESS) {
		goto cleanup;
	}

	/*
	 * Search for slash and other special characters in the view and
	 * zone names.  Add a null terminator so we can use strpbrk(), then
	 * remove it.
	 */
	isc_buffer_putuint8(tbuf, 0);
	if (strpbrk(isc_buffer_base(tbuf), "\\/:") != NULL) {
		special = true;
	}
	isc_buffer_subtract(tbuf, 1);

	/* __catz__<digest>.db */
	rlen = (isc_md_type_get_size(ISC_MD_SHA256) * 2 + 1) + 12;

	/* optionally prepend with <zonedir>/ */
	if (entry->opts.zonedir != NULL) {
		rlen += strlen(entry->opts.zonedir) + 1;
	}

	result = isc_buffer_reserve(buffer, (unsigned int)rlen);
	if (result != ISC_R_SUCCESS) {
		goto cleanup;
	}

	if (entry->opts.zonedir != NULL) {
		isc_buffer_putstr(*buffer, entry->opts.zonedir);
		isc_buffer_putstr(*buffer, "/");
	}

	isc_buffer_usedregion(tbuf, &r);
	isc_buffer_putstr(*buffer, "__catz__");
	if (special || tbuf->used > ISC_SHA256_DIGESTLENGTH * 2 + 1) {
		unsigned char digest[ISC_MAX_MD_SIZE];
		unsigned int digestlen;

		/* we can do that because digest string < 2 * DNS_NAME */
		result = isc_md(ISC_MD_SHA256, r.base, r.length, digest,
				&digestlen);
		if (result != ISC_R_SUCCESS) {
			goto cleanup;
		}
		result = digest2hex(digest, digestlen, (char *)r.base,
				    ISC_SHA256_DIGESTLENGTH * 2 + 1);
		if (result != ISC_R_SUCCESS) {
			goto cleanup;
		}
		isc_buffer_putstr(*buffer, (char *)r.base);
	} else {
		isc_buffer_copyregion(*buffer, &r);
	}

	isc_buffer_putstr(*buffer, ".db");
	result = ISC_R_SUCCESS;

cleanup:
	isc_buffer_free(&tbuf);
	return (result);
}

/*
 * We have to generate a text buffer with regular zone config:
 * zone "foo.bar" {
 * 	type secondary;
 * 	primaries { ip1 port port1; ip2 port port2; };
 * }
 */
isc_result_t
dns_catz_generate_zonecfg(dns_catz_zone_t *catz, dns_catz_entry_t *entry,
			  isc_buffer_t **buf) {
	isc_buffer_t *buffer = NULL;
	isc_region_t region;
	isc_result_t result;
	uint32_t i;
	isc_netaddr_t netaddr;
	char pbuf[sizeof("65535")]; /* used for port number */
	char zname[DNS_NAME_FORMATSIZE];

	REQUIRE(DNS_CATZ_ZONE_VALID(catz));
	REQUIRE(DNS_CATZ_ENTRY_VALID(entry));
	REQUIRE(buf != NULL && *buf == NULL);

	/*
	 * The buffer will be reallocated if something won't fit,
	 * ISC_BUFFER_INCR seems like a good start.
	 */
	isc_buffer_allocate(catz->catzs->mctx, &buffer, ISC_BUFFER_INCR);

	isc_buffer_setautorealloc(buffer, true);
	isc_buffer_putstr(buffer, "zone \"");
	dns_name_totext(&entry->name, true, buffer);
	isc_buffer_putstr(buffer, "\" { type secondary; primaries");

	isc_buffer_putstr(buffer, " { ");
	for (i = 0; i < entry->opts.masters.count; i++) {
		/*
		 * Every primary must have an IP address assigned.
		 */
		switch (entry->opts.masters.addrs[i].type.sa.sa_family) {
		case AF_INET:
		case AF_INET6:
			break;
		default:
			dns_name_format(&entry->name, zname,
					DNS_NAME_FORMATSIZE);
			isc_log_write(dns_lctx, DNS_LOGCATEGORY_GENERAL,
				      DNS_LOGMODULE_MASTER, ISC_LOG_ERROR,
				      "catz: zone '%s' uses an invalid primary "
				      "(no IP address assigned)",
				      zname);
			result = ISC_R_FAILURE;
			goto cleanup;
		}
		isc_netaddr_fromsockaddr(&netaddr,
					 &entry->opts.masters.addrs[i]);
		isc_buffer_reserve(&buffer, INET6_ADDRSTRLEN);
		result = isc_netaddr_totext(&netaddr, buffer);
		RUNTIME_CHECK(result == ISC_R_SUCCESS);

		isc_buffer_putstr(buffer, " port ");
		snprintf(pbuf, sizeof(pbuf), "%u",
			 isc_sockaddr_getport(&entry->opts.masters.addrs[i]));
		isc_buffer_putstr(buffer, pbuf);

		if (entry->opts.masters.keys[i] != NULL) {
			isc_buffer_putstr(buffer, " key ");
			result = dns_name_totext(entry->opts.masters.keys[i],
						 true, buffer);
			if (result != ISC_R_SUCCESS) {
				goto cleanup;
			}
		}

		if (entry->opts.masters.tlss[i] != NULL) {
			isc_buffer_putstr(buffer, " tls ");
			result = dns_name_totext(entry->opts.masters.tlss[i],
						 true, buffer);
			if (result != ISC_R_SUCCESS) {
				goto cleanup;
			}
		}
		isc_buffer_putstr(buffer, "; ");
	}
	isc_buffer_putstr(buffer, "}; ");
	if (!entry->opts.in_memory) {
		isc_buffer_putstr(buffer, "file \"");
		result = dns_catz_generate_masterfilename(catz, entry, &buffer);
		if (result != ISC_R_SUCCESS) {
			goto cleanup;
		}
		isc_buffer_putstr(buffer, "\"; ");
	}
	if (entry->opts.allow_query != NULL) {
		isc_buffer_putstr(buffer, "allow-query { ");
		isc_buffer_usedregion(entry->opts.allow_query, &region);
		isc_buffer_copyregion(buffer, &region);
		isc_buffer_putstr(buffer, "}; ");
	}
	if (entry->opts.allow_transfer != NULL) {
		isc_buffer_putstr(buffer, "allow-transfer { ");
		isc_buffer_usedregion(entry->opts.allow_transfer, &region);
		isc_buffer_copyregion(buffer, &region);
		isc_buffer_putstr(buffer, "}; ");
	}

	isc_buffer_putstr(buffer, "};");
	*buf = buffer;

	return (ISC_R_SUCCESS);

cleanup:
	isc_buffer_free(&buffer);
	return (result);
}

static void
dns__catz_timer_cb(isc_task_t *task, isc_event_t *event) {
	char domain[DNS_NAME_FORMATSIZE];
	isc_result_t result;
	dns_catz_zone_t *catz = NULL;

	UNUSED(task);
	REQUIRE(event != NULL);
	REQUIRE(event->ev_arg != NULL);

	catz = (dns_catz_zone_t *)event->ev_arg;
	isc_event_free(&event);

	REQUIRE(isc_nm_tid() >= 0);
	REQUIRE(DNS_CATZ_ZONE_VALID(catz));

	if (atomic_load(&catz->catzs->shuttingdown)) {
		return;
	}

	LOCK(&catz->catzs->lock);

	INSIST(DNS_DB_VALID(catz->db));
	INSIST(catz->dbversion != NULL);
	INSIST(catz->updb == NULL);
	INSIST(catz->updbversion == NULL);

	catz->updatepending = false;
	catz->updaterunning = true;
	catz->updateresult = ISC_R_UNSET;

	dns_name_format(&catz->name, domain, DNS_NAME_FORMATSIZE);

	if (!catz->active) {
		isc_log_write(dns_lctx, DNS_LOGCATEGORY_GENERAL,
			      DNS_LOGMODULE_MASTER, ISC_LOG_INFO,
			      "catz: %s: no longer active, reload is canceled",
			      domain);
		catz->updaterunning = false;
		catz->updateresult = ISC_R_CANCELED;
		goto exit;
	}

	dns_db_attach(catz->db, &catz->updb);
	catz->updbversion = catz->dbversion;
	catz->dbversion = NULL;

	isc_log_write(dns_lctx, DNS_LOGCATEGORY_GENERAL, DNS_LOGMODULE_MASTER,
		      ISC_LOG_INFO, "catz: %s: reload start", domain);

	dns_catz_ref_catz(catz);
	isc_nm_work_offload(isc_task_getnetmgr(catz->catzs->updater),
			    dns__catz_update_cb, dns__catz_done_cb, catz);

exit:
	result = isc_time_now(&catz->lastupdated);
	RUNTIME_CHECK(result == ISC_R_SUCCESS);

	UNLOCK(&catz->catzs->lock);
}

isc_result_t
dns_catz_dbupdate_callback(dns_db_t *db, void *fn_arg) {
	dns_catz_zones_t *catzs = (dns_catz_zones_t *)fn_arg;
	dns_catz_zone_t *catz = NULL;
	isc_time_t now;
	isc_result_t result = ISC_R_SUCCESS;
	isc_region_t r;
	char dname[DNS_NAME_FORMATSIZE];

	REQUIRE(DNS_DB_VALID(db));
	REQUIRE(DNS_CATZ_ZONES_VALID(catzs));

	if (atomic_load(&catzs->shuttingdown)) {
		return (ISC_R_SHUTTINGDOWN);
	}

	dns_name_toregion(&db->origin, &r);

	LOCK(&catzs->lock);
	if (catzs->zones == NULL) {
		result = ISC_R_SHUTTINGDOWN;
		goto cleanup;
	}
	result = isc_ht_find(catzs->zones, r.base, r.length, (void **)&catz);
	if (result != ISC_R_SUCCESS) {
		goto cleanup;
	}

	/* New zone came as AXFR */
	if (catz->db != NULL && catz->db != db) {
		/* Old db cleanup. */
		if (catz->dbversion != NULL) {
			dns_db_closeversion(catz->db, &catz->dbversion, false);
		}
		dns_db_updatenotify_unregister(
			catz->db, dns_catz_dbupdate_callback, catz->catzs);
		dns_db_detach(&catz->db);
		catz->db_registered = false;
	}
	if (catz->db == NULL) {
		/* New db registration. */
		dns_db_attach(db, &catz->db);
		result = dns_db_updatenotify_register(
			db, dns_catz_dbupdate_callback, catz->catzs);
		if (result == ISC_R_SUCCESS) {
			catz->db_registered = true;
		}
	}

	dns_name_format(&catz->name, dname, DNS_NAME_FORMATSIZE);

	if (!catz->updatepending && !catz->updaterunning) {
		uint64_t tdiff;

		catz->updatepending = true;

		isc_time_now(&now);
		tdiff = isc_time_microdiff(&now, &catz->lastupdated) / 1000000;
		if (tdiff < catz->defoptions.min_update_interval) {
			uint64_t defer = catz->defoptions.min_update_interval -
					 tdiff;
			isc_interval_t interval;

			isc_log_write(dns_lctx, DNS_LOGCATEGORY_GENERAL,
				      DNS_LOGMODULE_MASTER, ISC_LOG_INFO,
				      "catz: %s: new zone version came "
				      "too soon, deferring update for "
				      "%" PRIu64 " seconds",
				      dname, defer);
			isc_interval_set(&interval, (unsigned int)defer, 0);
			dns_db_currentversion(db, &catz->dbversion);
			result = isc_timer_reset(catz->updatetimer,
						 isc_timertype_once, NULL,
						 &interval, true);
			if (result != ISC_R_SUCCESS) {
				goto cleanup;
			}
		} else {
			isc_event_t *event;

			dns_db_currentversion(db, &catz->dbversion);
			ISC_EVENT_INIT(
				&catz->updateevent, sizeof(catz->updateevent),
				0, NULL, DNS_EVENT_CATZUPDATED,
				dns__catz_timer_cb, catz, catz, NULL, NULL);
			event = &catz->updateevent;
			isc_task_send(catzs->updater, &event);
		}
	} else {
		catz->updatepending = true;
		isc_log_write(dns_lctx, DNS_LOGCATEGORY_GENERAL,
			      DNS_LOGMODULE_MASTER, ISC_LOG_DEBUG(3),
			      "catz: %s: update already queued or running",
			      dname);
		if (catz->dbversion != NULL) {
			dns_db_closeversion(catz->db, &catz->dbversion, false);
		}
		dns_db_currentversion(catz->db, &catz->dbversion);
	}

cleanup:
	UNLOCK(&catzs->lock);

	return (result);
}

void
dns_catz_dbupdate_unregister(dns_db_t *db, dns_catz_zones_t *catzs) {
	REQUIRE(DNS_DB_VALID(db));
	REQUIRE(DNS_CATZ_ZONES_VALID(catzs));

	dns_db_updatenotify_unregister(db, dns_catz_dbupdate_callback, catzs);
}

void
dns_catz_dbupdate_register(dns_db_t *db, dns_catz_zones_t *catzs) {
	REQUIRE(DNS_DB_VALID(db));
	REQUIRE(DNS_CATZ_ZONES_VALID(catzs));

	dns_db_updatenotify_register(db, dns_catz_dbupdate_callback, catzs);
}

static bool
catz_rdatatype_is_processable(const dns_rdatatype_t type) {
	return (!dns_rdatatype_isdnssec(type) && type != dns_rdatatype_cds &&
		type != dns_rdatatype_cdnskey && type != dns_rdatatype_zonemd);
}

/*
 * Process an updated database for a catalog zone.
 * It creates a new catz, iterates over database to fill it with content, and
 * then merges new catz into old catz.
 */
static void
dns__catz_update_cb(void *data) {
	dns_catz_zone_t *catz = (dns_catz_zone_t *)data;
	dns_db_t *updb = NULL;
	dns_catz_zones_t *catzs = NULL;
	dns_catz_zone_t *oldcatz = NULL, *newcatz = NULL;
	isc_result_t result;
	isc_region_t r;
	dns_dbnode_t *node = NULL;
	const dns_dbnode_t *vers_node = NULL;
	dns_dbiterator_t *updbit = NULL;
	dns_fixedname_t fixname;
	dns_name_t *name;
	dns_rdatasetiter_t *rdsiter = NULL;
	dns_rdataset_t rdataset;
	char bname[DNS_NAME_FORMATSIZE];
	char cname[DNS_NAME_FORMATSIZE];
	bool is_vers_processed = false;
	bool is_active;
	uint32_t vers;
	uint32_t catz_vers;

	REQUIRE(DNS_CATZ_ZONE_VALID(catz));
	REQUIRE(DNS_DB_VALID(catz->updb));
	REQUIRE(DNS_CATZ_ZONES_VALID(catz->catzs));

	updb = catz->updb;
	catzs = catz->catzs;

	if (atomic_load(&catzs->shuttingdown)) {
		result = ISC_R_SHUTTINGDOWN;
		goto exit;
	}

	dns_name_format(&updb->origin, bname, DNS_NAME_FORMATSIZE);

	/*
	 * Create a new catz in the same context as current catz.
	 */
	dns_name_toregion(&updb->origin, &r);
	LOCK(&catzs->lock);
	if (catzs->zones == NULL) {
		UNLOCK(&catzs->lock);
		result = ISC_R_SHUTTINGDOWN;
		goto exit;
	}
	result = isc_ht_find(catzs->zones, r.base, r.length, (void **)&oldcatz);
	is_active = (result == ISC_R_SUCCESS && oldcatz->active);
	UNLOCK(&catzs->lock);
	if (result != ISC_R_SUCCESS) {
		/* This can happen if we remove the zone in the meantime. */
		isc_log_write(dns_lctx, DNS_LOGCATEGORY_GENERAL,
			      DNS_LOGMODULE_MASTER, ISC_LOG_ERROR,
			      "catz: zone '%s' not in config", bname);
		goto exit;
	}

	INSIST(catz == oldcatz);

	if (!is_active) {
		/* This can happen during a reconfiguration. */
		isc_log_write(dns_lctx, DNS_LOGCATEGORY_GENERAL,
			      DNS_LOGMODULE_MASTER, ISC_LOG_INFO,
			      "catz: zone '%s' is no longer active", bname);
		result = ISC_R_CANCELED;
		goto exit;
	}

	result = dns_db_getsoaserial(updb, oldcatz->updbversion, &vers);
	if (result != ISC_R_SUCCESS) {
		/* A zone without SOA record?!? */
		isc_log_write(dns_lctx, DNS_LOGCATEGORY_GENERAL,
			      DNS_LOGMODULE_MASTER, ISC_LOG_ERROR,
			      "catz: zone '%s' has no SOA record (%s)", bname,
			      isc_result_totext(result));
		goto exit;
	}

	isc_log_write(dns_lctx, DNS_LOGCATEGORY_GENERAL, DNS_LOGMODULE_MASTER,
		      ISC_LOG_INFO,
		      "catz: updating catalog zone '%s' with serial %" PRIu32,
		      bname, vers);

	result = dns_catz_new_zone(catzs, &newcatz, &updb->origin);
	if (result != ISC_R_SUCCESS) {
		isc_log_write(dns_lctx, DNS_LOGCATEGORY_GENERAL,
			      DNS_LOGMODULE_MASTER, ISC_LOG_ERROR,
			      "catz: failed to create new zone - %s",
			      isc_result_totext(result));
		goto exit;
	}

	result = dns_db_createiterator(updb, DNS_DB_NONSEC3, &updbit);
	if (result != ISC_R_SUCCESS) {
		dns_catz_detach_catz(&newcatz);
		isc_log_write(dns_lctx, DNS_LOGCATEGORY_GENERAL,
			      DNS_LOGMODULE_MASTER, ISC_LOG_ERROR,
			      "catz: failed to create DB iterator - %s",
			      isc_result_totext(result));
		goto exit;
	}

	name = dns_fixedname_initname(&fixname);

	/*
	 * Take the version record to process first, because the other
	 * records might be processed differently depending on the version of
	 * the catalog zone's schema.
	 */
	result = dns_name_fromstring2(name, "version", &updb->origin, 0, NULL);
	if (result != ISC_R_SUCCESS) {
		dns_dbiterator_destroy(&updbit);
		dns_catz_detach_catz(&newcatz);
		isc_log_write(dns_lctx, DNS_LOGCATEGORY_GENERAL,
			      DNS_LOGMODULE_MASTER, ISC_LOG_ERROR,
			      "catz: failed to create name from string - %s",
			      isc_result_totext(result));
		goto exit;
	}
	result = dns_dbiterator_seek(updbit, name);
	if (result != ISC_R_SUCCESS) {
		dns_dbiterator_destroy(&updbit);
		isc_log_write(dns_lctx, DNS_LOGCATEGORY_GENERAL,
			      DNS_LOGMODULE_MASTER, ISC_LOG_ERROR,
			      "catz: zone '%s' has no 'version' record (%s)",
			      bname, isc_result_totext(result));
		newcatz->broken = true;
		goto final;
	}

	name = dns_fixedname_initname(&fixname);

	/*
	 * Iterate over database to fill the new zone.
	 */
	while (result == ISC_R_SUCCESS) {
		if (atomic_load(&catzs->shuttingdown)) {
			result = ISC_R_SHUTTINGDOWN;
			break;
		}

		result = dns_dbiterator_current(updbit, &node, name);
		if (result != ISC_R_SUCCESS) {
			isc_log_write(dns_lctx, DNS_LOGCATEGORY_GENERAL,
				      DNS_LOGMODULE_MASTER, ISC_LOG_ERROR,
				      "catz: failed to get db iterator - %s",
				      isc_result_totext(result));
			break;
		}

		result = dns_dbiterator_pause(updbit);
		RUNTIME_CHECK(result == ISC_R_SUCCESS);

		if (!is_vers_processed) {
			/* Keep the version node to skip it later in the loop */
			vers_node = node;
		} else if (node == vers_node) {
			/* Skip the already processed version node */
			dns_db_detachnode(updb, &node);
			result = dns_dbiterator_next(updbit);
			continue;
		}

		result = dns_db_allrdatasets(updb, node, oldcatz->updbversion,
					     0, 0, &rdsiter);
		if (result != ISC_R_SUCCESS) {
			isc_log_write(dns_lctx, DNS_LOGCATEGORY_GENERAL,
				      DNS_LOGMODULE_MASTER, ISC_LOG_ERROR,
				      "catz: failed to fetch rrdatasets - %s",
				      isc_result_totext(result));
			dns_db_detachnode(updb, &node);
			break;
		}

		dns_rdataset_init(&rdataset);
		result = dns_rdatasetiter_first(rdsiter);
		while (result == ISC_R_SUCCESS) {
			dns_rdatasetiter_current(rdsiter, &rdataset);

			/*
			 * Skip processing DNSSEC-related and ZONEMD types,
			 * because we are not interested in them in the context
			 * of a catalog zone, and processing them will fail
			 * and produce an unnecessary warning message.
			 */
			if (!catz_rdatatype_is_processable(rdataset.type)) {
				goto next;
			}

			/*
			 * Although newcatz->coos is accessed in
			 * catz_process_coo() in the call-chain below, we don't
			 * need to hold the newcatz->lock, because the newcatz
			 * is still local to this thread and function and
			 * newcatz->coos can't be accessed from the outside
			 * until dns__catz_zones_merge() has been called.
			 */
			result = dns__catz_update_process(newcatz, name,
							  &rdataset);
			if (result != ISC_R_SUCCESS) {
				char typebuf[DNS_RDATATYPE_FORMATSIZE];
				char classbuf[DNS_RDATACLASS_FORMATSIZE];

				dns_name_format(name, cname,
						DNS_NAME_FORMATSIZE);
				dns_rdataclass_format(rdataset.rdclass,
						      classbuf,
						      sizeof(classbuf));
				dns_rdatatype_format(rdataset.type, typebuf,
						     sizeof(typebuf));
				isc_log_write(dns_lctx, DNS_LOGCATEGORY_GENERAL,
					      DNS_LOGMODULE_MASTER,
					      ISC_LOG_WARNING,
					      "catz: invalid record in catalog "
					      "zone - %s %s %s (%s) - ignoring",
					      cname, classbuf, typebuf,
					      isc_result_totext(result));
			}
		next:
			dns_rdataset_disassociate(&rdataset);
			result = dns_rdatasetiter_next(rdsiter);
		}

		dns_rdatasetiter_destroy(&rdsiter);

		dns_db_detachnode(updb, &node);

		if (!is_vers_processed) {
			is_vers_processed = true;
			result = dns_dbiterator_first(updbit);
		} else {
			result = dns_dbiterator_next(updbit);
		}
	}

	dns_dbiterator_destroy(&updbit);
	isc_log_write(dns_lctx, DNS_LOGCATEGORY_GENERAL, DNS_LOGMODULE_MASTER,
		      ISC_LOG_DEBUG(3),
		      "catz: update_from_db: iteration finished: %s",
		      isc_result_totext(result));

	/*
	 * Check catalog zone version compatibilites.
	 */
	catz_vers = (newcatz->version == DNS_CATZ_VERSION_UNDEFINED)
			    ? oldcatz->version
			    : newcatz->version;
	if (catz_vers == DNS_CATZ_VERSION_UNDEFINED) {
		isc_log_write(dns_lctx, DNS_LOGCATEGORY_GENERAL,
			      DNS_LOGMODULE_MASTER, ISC_LOG_WARNING,
			      "catz: zone '%s' version is not set", bname);
		newcatz->broken = true;
	} else if (catz_vers != 1 && catz_vers != 2) {
		isc_log_write(dns_lctx, DNS_LOGCATEGORY_GENERAL,
			      DNS_LOGMODULE_MASTER, ISC_LOG_WARNING,
			      "catz: zone '%s' unsupported version "
			      "'%" PRIu32 "'",
			      bname, catz_vers);
		newcatz->broken = true;
	} else {
		oldcatz->version = catz_vers;
	}

final:
	if (newcatz->broken) {
		dns_name_format(name, cname, DNS_NAME_FORMATSIZE);
		isc_log_write(dns_lctx, DNS_LOGCATEGORY_GENERAL,
			      DNS_LOGMODULE_MASTER, ISC_LOG_ERROR,
			      "catz: new catalog zone '%s' is broken and "
			      "will not be processed",
			      bname);
		dns_catz_detach_catz(&newcatz);
		result = ISC_R_FAILURE;
		goto exit;
	}

	/*
	 * Finally merge new zone into old zone.
	 */
	result = dns__catz_zones_merge(oldcatz, newcatz);
	dns_catz_detach_catz(&newcatz);
	if (result != ISC_R_SUCCESS) {
		isc_log_write(dns_lctx, DNS_LOGCATEGORY_GENERAL,
			      DNS_LOGMODULE_MASTER, ISC_LOG_ERROR,
			      "catz: failed merging zones: %s",
			      isc_result_totext(result));

		goto exit;
	}

	isc_log_write(dns_lctx, DNS_LOGCATEGORY_GENERAL, DNS_LOGMODULE_MASTER,
		      ISC_LOG_DEBUG(3),
		      "catz: update_from_db: new zone merged");

exit:
	catz->updateresult = result;
}

static void
dns__catz_done_cb(void *data, isc_result_t result) {
	dns_catz_zone_t *catz = (dns_catz_zone_t *)data;
	char dname[DNS_NAME_FORMATSIZE];

	REQUIRE(DNS_CATZ_ZONE_VALID(catz));

	if (result == ISC_R_SUCCESS && catz->updateresult != ISC_R_SUCCESS) {
		result = catz->updateresult;
	}

	LOCK(&catz->catzs->lock);
	catz->updaterunning = false;

	dns_name_format(&catz->name, dname, DNS_NAME_FORMATSIZE);

	/*
	 * When we're doing reconfig and setting a new catalog zone
	 * from an existing zone we won't have a chance to set up
	 * update callback in zone_startload or axfr_makedb, but we will
	 * call onupdate() artificially so we can register the callback
	 * here.
	 */
	if (result == ISC_R_SUCCESS && !catz->db_registered) {
		result = dns_db_updatenotify_register(
			catz->db, dns_catz_dbupdate_callback, catz->catzs);
		if (result == ISC_R_SUCCESS) {
			catz->db_registered = true;
		}
	}

	/* If there's no update pending, or if shutting down, finish. */
	if (!catz->updatepending || atomic_load(&catz->catzs->shuttingdown)) {
		goto done;
	}

	/* If there's an update pending, schedule it */
	if (catz->defoptions.min_update_interval > 0) {
		uint64_t defer = catz->defoptions.min_update_interval;
		isc_interval_t interval;

		isc_log_write(dns_lctx, DNS_LOGCATEGORY_GENERAL,
			      DNS_LOGMODULE_MASTER, ISC_LOG_INFO,
			      "catz: %s: new zone version came "
			      "too soon, deferring update for "
			      "%" PRIu64 " seconds",
			      dname, defer);
		isc_interval_set(&interval, (unsigned int)defer, 0);
		(void)isc_timer_reset(catz->updatetimer, isc_timertype_once,
				      NULL, &interval, true);
	} else {
		isc_event_t *event = NULL;
		INSIST(!ISC_LINK_LINKED(&catz->updateevent, ev_link));
		ISC_EVENT_INIT(&catz->updateevent, sizeof(catz->updateevent), 0,
			       NULL, DNS_EVENT_CATZUPDATED, dns__catz_timer_cb,
			       catz, catz, NULL, NULL);
		event = &catz->updateevent;
		isc_task_send(catz->catzs->updater, &event);
	}

done:
	dns_db_closeversion(catz->updb, &catz->updbversion, false);
	dns_db_detach(&catz->updb);

	UNLOCK(&catz->catzs->lock);

	isc_log_write(dns_lctx, DNS_LOGCATEGORY_GENERAL, DNS_LOGMODULE_MASTER,
		      ISC_LOG_INFO, "catz: %s: reload done: %s", dname,
		      isc_result_totext(result));

	dns_catz_unref_catz(catz);
}

void
dns_catz_prereconfig(dns_catz_zones_t *catzs) {
	isc_result_t result;
	isc_ht_iter_t *iter = NULL;

	REQUIRE(DNS_CATZ_ZONES_VALID(catzs));

	LOCK(&catzs->lock);
	isc_ht_iter_create(catzs->zones, &iter);
	for (result = isc_ht_iter_first(iter); result == ISC_R_SUCCESS;
	     result = isc_ht_iter_next(iter))
	{
		dns_catz_zone_t *catz = NULL;
		isc_ht_iter_current(iter, (void **)&catz);
		catz->active = false;
	}
	UNLOCK(&catzs->lock);
	INSIST(result == ISC_R_NOMORE);
	isc_ht_iter_destroy(&iter);
}

void
dns_catz_postreconfig(dns_catz_zones_t *catzs) {
	isc_result_t result;
	dns_catz_zone_t *newcatz = NULL;
	isc_ht_iter_t *iter = NULL;

	REQUIRE(DNS_CATZ_ZONES_VALID(catzs));

	LOCK(&catzs->lock);
	isc_ht_iter_create(catzs->zones, &iter);
	for (result = isc_ht_iter_first(iter); result == ISC_R_SUCCESS;) {
		dns_catz_zone_t *catz = NULL;

		isc_ht_iter_current(iter, (void **)&catz);
		if (!catz->active) {
			char cname[DNS_NAME_FORMATSIZE];
			dns_name_format(&catz->name, cname,
					DNS_NAME_FORMATSIZE);
			isc_log_write(dns_lctx, DNS_LOGCATEGORY_GENERAL,
				      DNS_LOGMODULE_MASTER, ISC_LOG_WARNING,
				      "catz: removing catalog zone %s", cname);

			/*
			 * Merge the old zone with an empty one to remove
			 * all members.
			 */
			result = dns_catz_new_zone(catzs, &newcatz,
						   &catz->name);
			INSIST(result == ISC_R_SUCCESS);
			dns__catz_zones_merge(catz, newcatz);
			dns_catz_detach_catz(&newcatz);

			/* Make sure that we have an empty catalog zone. */
			INSIST(isc_ht_count(catz->entries) == 0);
			result = isc_ht_iter_delcurrent_next(iter);
			dns_catz_detach_catz(&catz);
		} else {
			result = isc_ht_iter_next(iter);
		}
	}
	UNLOCK(&catzs->lock);
	RUNTIME_CHECK(result == ISC_R_NOMORE);
	isc_ht_iter_destroy(&iter);
}

void
dns_catz_get_iterator(dns_catz_zone_t *catz, isc_ht_iter_t **itp) {
	REQUIRE(DNS_CATZ_ZONE_VALID(catz));

	isc_ht_iter_create(catz->entries, itp);
}
