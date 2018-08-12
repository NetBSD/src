/*	$NetBSD: catz.c,v 1.1.1.1 2018/08/12 12:08:14 christos Exp $	*/

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

#include <isc/hex.h>
#include <isc/mem.h>
#include <isc/parseint.h>
#include <isc/print.h>
#include <isc/result.h>
#include <isc/sha2.h>
#include <isc/task.h>
#include <isc/util.h>

#include <dns/catz.h>
#include <dns/dbiterator.h>
#include <dns/events.h>
#include <dns/rdatasetiter.h>
#include <dns/view.h>
#include <dns/zone.h>


/*%
 * Single member zone in a catalog
 */
struct dns_catz_entry {
	dns_name_t		name;
	dns_catz_options_t	opts;
	isc_refcount_t		refs;
};

/*%
 * Catalog zone
 */
struct dns_catz_zone {
	dns_name_t		name;
	dns_catz_zones_t	*catzs;
	dns_rdata_t		soa;
	/* key in entries is 'mhash', not domain name! */
	isc_ht_t		*entries;
	/*
	 * defoptions are taken from named.conf
	 * zoneoptions are global options from zone
	 */
	dns_catz_options_t	defoptions;
	dns_catz_options_t	zoneoptions;
	isc_time_t		lastupdated;
	isc_boolean_t		updatepending;
	isc_uint32_t		version;

	dns_db_t		*db;
	dns_dbversion_t		*dbversion;

	isc_timer_t		*updatetimer;
	isc_event_t		updateevent;

	isc_boolean_t		active;
	isc_boolean_t		db_registered;

	isc_refcount_t		refs;
};

static isc_result_t
catz_process_zones_entry(dns_catz_zone_t *zone, dns_rdataset_t *value,
			 dns_label_t *mhash);
static isc_result_t
catz_process_zones_suboption(dns_catz_zone_t *zone, dns_rdataset_t *value,
			     dns_label_t *mhash, dns_name_t *name);

/*%
 * Collection of catalog zones for a view
 */
struct dns_catz_zones {
	isc_ht_t			*zones;
	isc_mem_t			*mctx;
	isc_refcount_t			refs;
	isc_mutex_t			lock;
	dns_catz_zonemodmethods_t	*zmm;
	isc_taskmgr_t			*taskmgr;
	isc_timermgr_t			*timermgr;
	dns_view_t			*view;
	isc_task_t			*updater;
};

void
dns_catz_options_init(dns_catz_options_t *options) {
	dns_ipkeylist_init(&options->masters);

	options->allow_query = NULL;
	options->allow_transfer = NULL;

	options->allow_query = NULL;
	options->allow_transfer = NULL;

	options->in_memory = ISC_FALSE;
	options->min_update_interval = 5;
	options->zonedir = NULL;
}

void
dns_catz_options_free(dns_catz_options_t *options, isc_mem_t *mctx) {
	if (options->masters.count != 0)
		dns_ipkeylist_clear(mctx, &options->masters);
	if (options->zonedir != NULL) {
		isc_mem_free(mctx, options->zonedir);
		options->zonedir = NULL;
	}
	if (options->allow_query != NULL)
		isc_buffer_free(&options->allow_query);
	if (options->allow_transfer != NULL)
		isc_buffer_free(&options->allow_transfer);
}

isc_result_t
dns_catz_options_copy(isc_mem_t *mctx, const dns_catz_options_t *src,
		      dns_catz_options_t *dst)
{
	REQUIRE(src != NULL);
	REQUIRE(dst != NULL);
	REQUIRE(dst->masters.count == 0);
	REQUIRE(dst->allow_query == NULL);
	REQUIRE(dst->allow_transfer == NULL);

	if (src->masters.count != 0)
		dns_ipkeylist_copy(mctx, &src->masters, &dst->masters);

	if (dst->zonedir != NULL) {
		isc_mem_free(mctx, dst->zonedir);
		dst->zonedir = NULL;
	}

	if (src->zonedir != NULL)
		dst->zonedir = isc_mem_strdup(mctx, src->zonedir);

	if (src->allow_query != NULL)
		isc_buffer_dup(mctx, &dst->allow_query, src->allow_query);

	if (src->allow_transfer != NULL)
		isc_buffer_dup(mctx, &dst->allow_transfer, src->allow_transfer);

	return (ISC_R_SUCCESS);
}

isc_result_t
dns_catz_options_setdefault(isc_mem_t *mctx, const dns_catz_options_t *defaults,
			    dns_catz_options_t *opts)
{
	if (opts->masters.count == 0 && defaults->masters.count != 0)
		dns_ipkeylist_copy(mctx, &defaults->masters, &opts->masters);

	if (defaults->zonedir != NULL)
		opts->zonedir = isc_mem_strdup(mctx, defaults->zonedir);

	if (opts->allow_query == NULL && defaults->allow_query != NULL)
		isc_buffer_dup(mctx, &opts->allow_query, defaults->allow_query);
	if (opts->allow_transfer == NULL && defaults->allow_transfer != NULL)
		isc_buffer_dup(mctx, &opts->allow_transfer,
			       defaults->allow_transfer);

	/* This option is always taken from config, so it's always 'default' */
	opts->in_memory = defaults->in_memory;
	return (ISC_R_SUCCESS);
}

isc_result_t
dns_catz_entry_new(isc_mem_t *mctx, const dns_name_t *domain,
		   dns_catz_entry_t **nentryp)
{
	dns_catz_entry_t *nentry;
	isc_result_t result;

	REQUIRE(nentryp != NULL && *nentryp == NULL);

	nentry = isc_mem_get(mctx, sizeof(dns_catz_entry_t));
	if (nentry == NULL)
		return (ISC_R_NOMEMORY);

	dns_name_init(&nentry->name, NULL);
	if (domain != NULL) {
		result = dns_name_dup(domain, mctx, &nentry->name);
		if (result != ISC_R_SUCCESS)
			goto cleanup;
	}

	dns_catz_options_init(&nentry->opts);
	isc_refcount_init(&nentry->refs, 1);
	*nentryp = nentry;
	return (ISC_R_SUCCESS);

cleanup:
	isc_mem_put(mctx, nentry, sizeof(dns_catz_entry_t));
	return (result);
}

dns_name_t *
dns_catz_entry_getname(dns_catz_entry_t *entry) {
	return (&entry->name);
}

isc_result_t
dns_catz_entry_copy(dns_catz_zone_t *zone, const dns_catz_entry_t *entry,
		    dns_catz_entry_t **nentryp)
{
	isc_result_t result;
	dns_catz_entry_t *nentry = NULL;

	result = dns_catz_entry_new(zone->catzs->mctx, &entry->name, &nentry);
	if (result != ISC_R_SUCCESS)
		return (result);

	result = dns_catz_options_copy(zone->catzs->mctx, &entry->opts,
				       &nentry->opts);
	if (result != ISC_R_SUCCESS)
		dns_catz_entry_detach(zone, &nentry);

	*nentryp = nentry;
	return (result);
}

void
dns_catz_entry_attach(dns_catz_entry_t *entry, dns_catz_entry_t **entryp) {
	REQUIRE(entryp != NULL && *entryp == NULL);
	isc_refcount_increment(&entry->refs, NULL);
	*entryp = entry;
}

void
dns_catz_entry_detach(dns_catz_zone_t *zone, dns_catz_entry_t **entryp) {
	dns_catz_entry_t *entry;
	isc_mem_t *mctx;
	unsigned int refs;

	REQUIRE(entryp != NULL && *entryp != NULL);

	entry = *entryp;
	*entryp = NULL;

	mctx = zone->catzs->mctx;

	isc_refcount_decrement(&entry->refs, &refs);
	if (refs == 0) {
		dns_catz_options_free(&entry->opts, mctx);
		if (dns_name_dynamic(&entry->name))
			dns_name_free(&entry->name, mctx);
		isc_refcount_destroy(&entry->refs);
		isc_mem_put(mctx, entry, sizeof(dns_catz_entry_t));
	}
}

isc_boolean_t
dns_catz_entry_validate(const dns_catz_entry_t *entry) {
	UNUSED(entry);

	return (ISC_TRUE);
}

isc_boolean_t
dns_catz_entry_cmp(const dns_catz_entry_t *ea, const dns_catz_entry_t *eb) {
	isc_region_t ra, rb;

	if (ea == eb)
		return (ISC_TRUE);

	if (ea->opts.masters.count != eb->opts.masters.count)
		return (ISC_FALSE);

	if (memcmp(ea->opts.masters.addrs, eb->opts.masters.addrs,
		   ea->opts.masters.count * sizeof(isc_sockaddr_t)))
		return (ISC_FALSE);

	/* If one is NULL and the other isn't, the entries don't match */
	if ((ea->opts.allow_query == NULL) != (eb->opts.allow_query == NULL))
		return (ISC_FALSE);

	/* If one is non-NULL, then they both are */
	if (ea->opts.allow_query != NULL) {
		isc_buffer_usedregion(ea->opts.allow_query, &ra);
		isc_buffer_usedregion(eb->opts.allow_query, &rb);
		if (isc_region_compare(&ra, &rb))
			return (ISC_FALSE);
	}

	/* Repeat the above checks with allow_transfer */
	if ((ea->opts.allow_transfer == NULL) !=
	    (eb->opts.allow_transfer == NULL))
		return (ISC_FALSE);

	if (ea->opts.allow_transfer != NULL) {
		isc_buffer_usedregion(ea->opts.allow_transfer, &ra);
		isc_buffer_usedregion(eb->opts.allow_transfer, &rb);
		if (isc_region_compare(&ra, &rb))
			return (ISC_FALSE);
	}

	/* xxxwpk TODO compare dscps/keys! */
	return (ISC_TRUE);
}

dns_name_t *
dns_catz_zone_getname(dns_catz_zone_t *zone) {
	REQUIRE(zone != NULL);

	return (&zone->name);
}

dns_catz_options_t *
dns_catz_zone_getdefoptions(dns_catz_zone_t *zone) {
	REQUIRE(zone != NULL);

	return (&zone->defoptions);
}

void
dns_catz_zone_resetdefoptions(dns_catz_zone_t *zone) {
	REQUIRE(zone != NULL);

	dns_catz_options_free(&zone->defoptions, zone->catzs->mctx);
	dns_catz_options_init(&zone->defoptions);
}

isc_result_t
dns_catz_zones_merge(dns_catz_zone_t *target, dns_catz_zone_t *newzone) {
	isc_result_t result;
	isc_ht_iter_t *iter1 = NULL, *iter2 = NULL;
	isc_ht_iter_t *iteradd = NULL, *itermod = NULL;
	isc_ht_t *toadd = NULL, *tomod = NULL;
	isc_boolean_t delcur = ISC_FALSE;
	char czname[DNS_NAME_FORMATSIZE];
	char zname[DNS_NAME_FORMATSIZE];
	dns_catz_zoneop_fn_t addzone, modzone, delzone;

	REQUIRE(target != NULL);
	REQUIRE(newzone != NULL);

	/* TODO verify the new zone first! */

	addzone = target->catzs->zmm->addzone;
	modzone = target->catzs->zmm->modzone;
	delzone = target->catzs->zmm->delzone;

	/* Copy zoneoptions from newzone into target. */

	dns_catz_options_free(&target->zoneoptions, target->catzs->mctx);
	dns_catz_options_copy(target->catzs->mctx, &newzone->zoneoptions,
			      &target->zoneoptions);
	dns_catz_options_setdefault(target->catzs->mctx, &target->defoptions,
				    &target->zoneoptions);

	dns_name_format(&target->name, czname, DNS_NAME_FORMATSIZE);

	result = isc_ht_init(&toadd, target->catzs->mctx, 16);
	if (result != ISC_R_SUCCESS)
		goto cleanup;

	result = isc_ht_init(&tomod, target->catzs->mctx, 16);
	if (result != ISC_R_SUCCESS)
		goto cleanup;

	result = isc_ht_iter_create(newzone->entries, &iter1);
	if (result != ISC_R_SUCCESS)
		goto cleanup;

	result = isc_ht_iter_create(target->entries, &iter2);
	if (result != ISC_R_SUCCESS)
		goto cleanup;

	/*
	 * We can create those iterators now, even though toadd and tomod are
	 * empty
	 */
	result = isc_ht_iter_create(toadd, &iteradd);
	if (result != ISC_R_SUCCESS)
		goto cleanup;

	result = isc_ht_iter_create(tomod, &itermod);
	if (result != ISC_R_SUCCESS)
		goto cleanup;

	/*
	 * First - walk the new zone and find all nodes that are not in the
	 * old zone, or are in both zones and are modified.
	 */
	for (result = isc_ht_iter_first(iter1);
	     result == ISC_R_SUCCESS;
	     result = delcur ? isc_ht_iter_delcurrent_next(iter1) :
			     isc_ht_iter_next(iter1))
	{
		dns_catz_entry_t *nentry;
		dns_catz_entry_t *oentry;
		unsigned char * key;
		size_t keysize;
		delcur = ISC_FALSE;

		isc_ht_iter_current(iter1, (void **) &nentry);
		isc_ht_iter_currentkey(iter1, &key, &keysize);

		/*
		 * Spurious record that came from suboption without main
		 * record, removed.
		 * xxxwpk: make it a separate verification phase?
		 */
		if (dns_name_countlabels(&nentry->name) == 0) {
			dns_catz_entry_detach(newzone, &nentry);
			delcur = ISC_TRUE;
			continue;
		}

		dns_name_format(&nentry->name, zname, DNS_NAME_FORMATSIZE);

		isc_log_write(dns_lctx, DNS_LOGCATEGORY_GENERAL,
			      DNS_LOGMODULE_MASTER, ISC_LOG_DEBUG(3),
			      "catz: iterating over '%s' from catalog '%s'",
			      zname, czname);
		dns_catz_options_setdefault(target->catzs->mctx,
					    &target->zoneoptions,
					    &nentry->opts);

		result = isc_ht_find(target->entries, key,
				     (isc_uint32_t)keysize, (void **) &oentry);
		if (result != ISC_R_SUCCESS) {
			result = isc_ht_add(toadd, key, (isc_uint32_t)keysize,
					    nentry);
			if (result != ISC_R_SUCCESS)
				isc_log_write(dns_lctx, DNS_LOGCATEGORY_GENERAL,
					      DNS_LOGMODULE_MASTER,
					      ISC_LOG_ERROR,
					      "catz: error adding zone '%s' "
					      "from catalog '%s' - %s",
					      zname, czname,
					      isc_result_totext(result));
			continue;
		}

		if (dns_catz_entry_cmp(oentry, nentry) != ISC_TRUE) {
			result = isc_ht_add(tomod, key, (isc_uint32_t)keysize,
					    nentry);
			if (result != ISC_R_SUCCESS)
				isc_log_write(dns_lctx, DNS_LOGCATEGORY_GENERAL,
					      DNS_LOGMODULE_MASTER,
					      ISC_LOG_ERROR,
					      "catz: error modifying zone '%s' "
					      "from catalog '%s' - %s",
					      zname, czname,
					      isc_result_totext(result));
		}
		dns_catz_entry_detach(target, &oentry);
		result = isc_ht_delete(target->entries, key,
				       (isc_uint32_t)keysize);
		RUNTIME_CHECK(result == ISC_R_SUCCESS);
	}
	RUNTIME_CHECK(result == ISC_R_NOMORE);
	isc_ht_iter_destroy(&iter1);

	/*
	 * Then - walk the old zone; only deleted entries should remain.
	 */
	for (result = isc_ht_iter_first(iter2);
	     result == ISC_R_SUCCESS;
	     result = isc_ht_iter_delcurrent_next(iter2))
	{
		dns_catz_entry_t *entry;
		isc_ht_iter_current(iter2, (void **) &entry);

		dns_name_format(&entry->name, zname, DNS_NAME_FORMATSIZE);
		result = delzone(entry, target, target->catzs->view,
			      target->catzs->taskmgr,
			      target->catzs->zmm->udata);
		isc_log_write(dns_lctx, DNS_LOGCATEGORY_GENERAL,
			      DNS_LOGMODULE_MASTER, ISC_LOG_INFO,
			      "catz: deleting zone '%s' from catalog '%s' - %s",
			      zname, czname, isc_result_totext(result));
		dns_catz_entry_detach(target, &entry);
	}
	RUNTIME_CHECK(result == ISC_R_NOMORE);
	isc_ht_iter_destroy(&iter2);
	/* At this moment target->entries has to be be empty. */
	INSIST(isc_ht_count(target->entries) == 0);
	isc_ht_destroy(&target->entries);

	for (result = isc_ht_iter_first(iteradd);
	     result == ISC_R_SUCCESS;
	     result = isc_ht_iter_delcurrent_next(iteradd))
	{
		dns_catz_entry_t *entry;
		isc_ht_iter_current(iteradd, (void **) &entry);

		dns_name_format(&entry->name, zname, DNS_NAME_FORMATSIZE);
		result = addzone(entry, target, target->catzs->view,
			      target->catzs->taskmgr,
			      target->catzs->zmm->udata);
		isc_log_write(dns_lctx, DNS_LOGCATEGORY_GENERAL,
			      DNS_LOGMODULE_MASTER, ISC_LOG_INFO,
			      "catz: adding zone '%s' from catalog "
			      "'%s' - %s",
			      zname, czname,
			      isc_result_totext(result));
	}

	for (result = isc_ht_iter_first(itermod);
	     result == ISC_R_SUCCESS;
	     result = isc_ht_iter_delcurrent_next(itermod))
	{
		dns_catz_entry_t *entry;
		isc_ht_iter_current(itermod, (void **) &entry);
		result = modzone(entry, target, target->catzs->view,
				 target->catzs->taskmgr,
				 target->catzs->zmm->udata);
		isc_log_write(dns_lctx, DNS_LOGCATEGORY_GENERAL,
			      DNS_LOGMODULE_MASTER, ISC_LOG_INFO,
			      "catz: modifying zone '%s' from catalog "
			      "'%s' - %s",
			      zname, czname,
			      isc_result_totext(result));
	}

	target->entries = newzone->entries;
	newzone->entries = NULL;

	result = ISC_R_SUCCESS;

cleanup:
	if (iter1 != NULL)
		isc_ht_iter_destroy(&iter1);
	if (iter2 != NULL)
		isc_ht_iter_destroy(&iter2);
	if (iteradd != NULL)
		isc_ht_iter_destroy(&iteradd);
	if (itermod != NULL)
		isc_ht_iter_destroy(&itermod);
	if (toadd != NULL)
		isc_ht_destroy(&toadd);
	if (tomod != NULL)
		isc_ht_destroy(&tomod);
	return (result);

}

isc_result_t
dns_catz_new_zones(dns_catz_zones_t **catzsp, dns_catz_zonemodmethods_t *zmm,
		   isc_mem_t *mctx, isc_taskmgr_t *taskmgr,
		   isc_timermgr_t *timermgr)
{
	dns_catz_zones_t *new_zones;
	isc_result_t result;

	REQUIRE(catzsp != NULL && *catzsp == NULL);
	REQUIRE(zmm != NULL);

	new_zones = isc_mem_get(mctx, sizeof(*new_zones));
	if (new_zones == NULL)
		return (ISC_R_NOMEMORY);
	memset(new_zones, 0, sizeof(*new_zones));

	result = isc_mutex_init(&new_zones->lock);
	if (result != ISC_R_SUCCESS)
		goto cleanup_newzones;

	result = isc_refcount_init(&new_zones->refs, 1);
	if (result != ISC_R_SUCCESS)
		goto cleanup_mutex;

	result = isc_ht_init(&new_zones->zones, mctx, 4);
	if (result != ISC_R_SUCCESS)
		goto cleanup_refcount;

	isc_mem_attach(mctx, &new_zones->mctx);
	new_zones->zmm = zmm;
	new_zones->timermgr = timermgr;
	new_zones->taskmgr = taskmgr;

	result = isc_task_create(taskmgr, 0, &new_zones->updater);
	if (result != ISC_R_SUCCESS)
		goto cleanup_ht;

	*catzsp = new_zones;
	return (ISC_R_SUCCESS);

  cleanup_ht:
	isc_ht_destroy(&new_zones->zones);
  cleanup_refcount:
	isc_refcount_destroy(&new_zones->refs);
  cleanup_mutex:
	isc_mutex_destroy(&new_zones->lock);
  cleanup_newzones:
	isc_mem_put(mctx, new_zones, sizeof(*new_zones));

	return (result);
}

void
dns_catz_catzs_set_view(dns_catz_zones_t *catzs, dns_view_t *view) {
	REQUIRE(catzs != NULL);
	REQUIRE(view != NULL);
	/* Either it's a new one or it's being reconfigured. */
	REQUIRE(catzs->view == NULL || !strcmp(catzs->view->name, view->name));

	catzs->view = view;
}

isc_result_t
dns_catz_new_zone(dns_catz_zones_t *catzs, dns_catz_zone_t **zonep,
		  const dns_name_t *name)
{
	isc_result_t result;
	dns_catz_zone_t *new_zone;

	REQUIRE(zonep != NULL && *zonep == NULL);

	new_zone = isc_mem_get(catzs->mctx, sizeof(*new_zone));
	if (new_zone == NULL)
		return (ISC_R_NOMEMORY);

	memset(new_zone, 0, sizeof(*new_zone));

	dns_name_init(&new_zone->name, NULL);

	result = dns_name_dup(name, catzs->mctx, &new_zone->name);
	if (result != ISC_R_SUCCESS)
		goto cleanup_newzone;

	result = isc_ht_init(&new_zone->entries, catzs->mctx, 4);
	if (result != ISC_R_SUCCESS)
		goto cleanup_name;

	new_zone->updatetimer = NULL;
	result = isc_timer_create(catzs->timermgr, isc_timertype_inactive,
				  NULL, NULL, catzs->updater,
				  dns_catz_update_taskaction,
				  new_zone, &new_zone->updatetimer);
	if (result != ISC_R_SUCCESS)
		goto cleanup_ht;

	isc_time_settoepoch(&new_zone->lastupdated);
	new_zone->updatepending = ISC_FALSE;
	new_zone->db = NULL;
	new_zone->dbversion = NULL;
	new_zone->catzs = catzs;
	dns_catz_options_init(&new_zone->defoptions);
	dns_catz_options_init(&new_zone->zoneoptions);
	new_zone->active = ISC_TRUE;
	new_zone->db_registered = ISC_FALSE;
	new_zone->version = (isc_uint32_t)(-1);
	isc_refcount_init(&new_zone->refs, 1);

	*zonep = new_zone;

	return (ISC_R_SUCCESS);

  cleanup_ht:
	isc_ht_destroy(&new_zone->entries);
  cleanup_name:
	dns_name_free(&new_zone->name, catzs->mctx);
  cleanup_newzone:
	isc_mem_put(catzs->mctx, new_zone, sizeof(*new_zone));

	return (result);
}

isc_result_t
dns_catz_add_zone(dns_catz_zones_t *catzs, const dns_name_t *name,
		  dns_catz_zone_t **zonep)
{
	dns_catz_zone_t *new_zone = NULL;
	isc_result_t result, tresult;
	char zname[DNS_NAME_FORMATSIZE];

	REQUIRE(catzs != NULL);
	REQUIRE(name != NULL);
	REQUIRE(zonep != NULL && *zonep == NULL);
	dns_name_format(name, zname, DNS_NAME_FORMATSIZE);
	isc_log_write(dns_lctx, DNS_LOGCATEGORY_GENERAL,
		      DNS_LOGMODULE_MASTER, ISC_LOG_DEBUG(3),
		      "catz: dns_catz_add_zone %s", zname);

	LOCK(&catzs->lock);

	result = dns_catz_new_zone(catzs, &new_zone, name);
	if (result != ISC_R_SUCCESS)
		goto cleanup;

	result = isc_ht_add(catzs->zones, new_zone->name.ndata,
			    new_zone->name.length, new_zone);
	if (result != ISC_R_SUCCESS) {
		dns_catz_zone_detach(&new_zone);
		if (result != ISC_R_EXISTS)
			goto cleanup;
	}

	if (result == ISC_R_EXISTS) {
		tresult = isc_ht_find(catzs->zones, name->ndata,
				     name->length, (void **) &new_zone);
		INSIST(tresult == ISC_R_SUCCESS && !new_zone->active);
		new_zone->active = ISC_TRUE;
	}

	*zonep = new_zone;

 cleanup:
	UNLOCK(&catzs->lock);

	return (result);
}

dns_catz_zone_t *
dns_catz_get_zone(dns_catz_zones_t *catzs, const dns_name_t *name) {
	isc_result_t result;
	dns_catz_zone_t *found;

	result = isc_ht_find(catzs->zones, name->ndata, name->length,
			     (void **) &found);
	if (result != ISC_R_SUCCESS)
		return (NULL);

	return (found);
}

void
dns_catz_catzs_attach(dns_catz_zones_t *catzs, dns_catz_zones_t **catzsp) {
	REQUIRE(catzsp != NULL && *catzsp == NULL);

	isc_refcount_increment(&catzs->refs, NULL);
	*catzsp = catzs;
}

void
dns_catz_zone_attach(dns_catz_zone_t *zone, dns_catz_zone_t **zonep) {
	REQUIRE(zonep != NULL && *zonep == NULL);

	isc_refcount_increment(&zone->refs, NULL);
	*zonep = zone;
}

void
dns_catz_zone_detach(dns_catz_zone_t **zonep) {
	isc_result_t result;
	dns_catz_zone_t *zone;
	isc_ht_iter_t *iter = NULL;
	isc_mem_t *mctx;
	unsigned int refs;

	REQUIRE(zonep != NULL && *zonep != NULL);

	zone = *zonep;
	*zonep = NULL;
	isc_refcount_decrement(&zone->refs, &refs);
	if (refs == 0) {
		if (zone->entries != NULL) {
			result = isc_ht_iter_create(zone->entries, &iter);
			INSIST(result == ISC_R_SUCCESS);
			for (result = isc_ht_iter_first(iter);
			     result == ISC_R_SUCCESS;
			     result = isc_ht_iter_delcurrent_next(iter))
			{
				dns_catz_entry_t *entry;

				isc_ht_iter_current(iter, (void **) &entry);
				dns_catz_entry_detach(zone, &entry);
			}
			INSIST(result == ISC_R_NOMORE);
			isc_ht_iter_destroy(&iter);

			/* The hashtable has to be empty now. */
			INSIST(isc_ht_count(zone->entries) == 0);
			isc_ht_destroy(&zone->entries);
		}
		mctx = zone->catzs->mctx;
		isc_timer_detach(&zone->updatetimer);
		isc_refcount_destroy(&zone->refs);
		if (zone->db_registered == ISC_TRUE) {
			result = dns_db_updatenotify_unregister(zone->db,
						    dns_catz_dbupdate_callback,
						    zone->catzs);
			INSIST(result == ISC_R_SUCCESS);
		}
		if (zone->dbversion)
			dns_db_closeversion(zone->db, &zone->dbversion,
					    ISC_FALSE);
		if (zone->db != NULL)
			dns_db_detach(&zone->db);

		dns_name_free(&zone->name, mctx);
		dns_catz_options_free(&zone->defoptions, mctx);
		dns_catz_options_free(&zone->zoneoptions, mctx);

		zone->catzs = NULL;
		isc_mem_put(mctx, zone, sizeof(dns_catz_zone_t));
	}
}

void
dns_catz_catzs_detach(dns_catz_zones_t ** catzsp) {
	dns_catz_zones_t *catzs;
	isc_ht_iter_t *iter = NULL;
	isc_result_t result;
	unsigned int refs;
	dns_catz_zone_t *zone;


	REQUIRE(catzsp != NULL);
	catzs = *catzsp;
	REQUIRE(catzs != NULL);

	*catzsp = NULL;
	isc_refcount_decrement(&catzs->refs, &refs);

	if (refs == 0) {
		DESTROYLOCK(&catzs->lock);
		if (catzs->zones != NULL) {
			result = isc_ht_iter_create(catzs->zones, &iter);
			INSIST(result == ISC_R_SUCCESS);
			for (result = isc_ht_iter_first(iter);
			     result == ISC_R_SUCCESS;)
			{
				isc_ht_iter_current(iter, (void **) &zone);
				result = isc_ht_iter_delcurrent_next(iter);
				dns_catz_zone_detach(&zone);
			}
			INSIST(result == ISC_R_NOMORE);
			isc_ht_iter_destroy(&iter);
			INSIST(isc_ht_count(catzs->zones) == 0);
			isc_ht_destroy(&catzs->zones);
		}
		isc_refcount_destroy(&catzs->refs);
		isc_task_destroy(&catzs->updater);
		isc_mem_putanddetach(&catzs->mctx, catzs, sizeof(*catzs));
	}
}

typedef enum {
	CATZ_OPT_NONE,
	CATZ_OPT_ZONES,
	CATZ_OPT_MASTERS,
	CATZ_OPT_ALLOW_QUERY,
	CATZ_OPT_ALLOW_TRANSFER,
	CATZ_OPT_VERSION,
} catz_opt_t;

static isc_boolean_t
catz_opt_cmp(const dns_label_t *option, const char *opt) {
	unsigned int l = strlen(opt);
	if (option->length - 1 == l &&
	    memcmp(opt, option->base + 1, l - 1) == 0)
		return (ISC_TRUE);
	else
		return (ISC_FALSE);
}

static catz_opt_t
catz_get_option(const dns_label_t *option) {
	if (catz_opt_cmp(option, "zones"))
		return (CATZ_OPT_ZONES);
	else if (catz_opt_cmp(option, "masters"))
		return (CATZ_OPT_MASTERS);
	else if (catz_opt_cmp(option, "allow-query"))
		return (CATZ_OPT_ALLOW_QUERY);
	else if (catz_opt_cmp(option, "allow-transfer"))
		return (CATZ_OPT_ALLOW_TRANSFER);
	else if (catz_opt_cmp(option, "version"))
		return (CATZ_OPT_VERSION);
	else
		return (CATZ_OPT_NONE);
}

static isc_result_t
catz_process_zones(dns_catz_zone_t *zone, dns_rdataset_t *value,
		   dns_name_t *name)
{
	dns_label_t mhash;
	dns_name_t opt;

	REQUIRE(zone != NULL);
	REQUIRE(DNS_RDATASET_VALID(value));
	REQUIRE(name != NULL);

	if (value->rdclass != dns_rdataclass_in)
		return (ISC_R_FAILURE);

	if (name->labels == 0)
		return (ISC_R_FAILURE);

	dns_name_getlabel(name, name->labels-1, &mhash);

	if (name->labels == 1)
		return (catz_process_zones_entry(zone, value, &mhash));
	else {
		dns_name_init(&opt, NULL);
		dns_name_split(name, 1, &opt, NULL);
		return (catz_process_zones_suboption(zone, value, &mhash, &opt));
	}
}

static isc_result_t
catz_process_zones_entry(dns_catz_zone_t *zone, dns_rdataset_t *value,
			 dns_label_t *mhash)
{
	isc_result_t result;
	dns_rdata_t rdata;
	dns_rdata_ptr_t ptr;
	dns_catz_entry_t *entry = NULL;

	/*
	 * We only take -first- value, as mhash must be
	 * different.
	 */
	if (value->type != dns_rdatatype_ptr)
		return (ISC_R_FAILURE);

	result = dns_rdataset_first(value);
	if (result != ISC_R_SUCCESS)
		return (ISC_R_FAILURE);

	dns_rdata_init(&rdata);
	dns_rdataset_current(value, &rdata);

	result = dns_rdata_tostruct(&rdata, &ptr, NULL);
	RUNTIME_CHECK(result == ISC_R_SUCCESS);

	result = isc_ht_find(zone->entries, mhash->base,
			     mhash->length, (void **) &entry);
	if (result == ISC_R_SUCCESS) {
		if (dns_name_countlabels(&entry->name) != 0) {
			/* We have a duplicate. */
			dns_rdata_freestruct(&ptr);
			return (ISC_R_FAILURE);
		} else {
			result = dns_name_dup(&ptr.ptr, zone->catzs->mctx,
					      &entry->name);
			if (result != ISC_R_SUCCESS) {
				dns_rdata_freestruct(&ptr);
				return (result);
			}
		}
	} else {
		result = dns_catz_entry_new(zone->catzs->mctx, &ptr.ptr,
					    &entry);
		if (result != ISC_R_SUCCESS) {
			dns_rdata_freestruct(&ptr);
			return (result);
		}

		result = isc_ht_add(zone->entries, mhash->base,
				    mhash->length, entry);
		if (result != ISC_R_SUCCESS) {
			dns_rdata_freestruct(&ptr);
			dns_catz_entry_detach(zone, &entry);
			return (result);
		}
	}

	dns_rdata_freestruct(&ptr);

	return (ISC_R_SUCCESS);
}

static isc_result_t
catz_process_version(dns_catz_zone_t *zone, dns_rdataset_t *value) {
	isc_result_t result;
	dns_rdata_t rdata;
	dns_rdata_txt_t rdatatxt;
	dns_rdata_txt_string_t rdatastr;
	isc_uint32_t tversion;
	char t[16];

	REQUIRE(zone != NULL);
	REQUIRE(DNS_RDATASET_VALID(value));

	if (value->rdclass != dns_rdataclass_in ||
	    value->type != dns_rdatatype_txt)
		return (ISC_R_FAILURE);

	result = dns_rdataset_first(value);
	if (result != ISC_R_SUCCESS)
		return (result);

	dns_rdata_init(&rdata);
	dns_rdataset_current(value, &rdata);

	result = dns_rdata_tostruct(&rdata, &rdatatxt, NULL);
	RUNTIME_CHECK(result == ISC_R_SUCCESS);

	result = dns_rdata_txt_first(&rdatatxt);
	if (result != ISC_R_SUCCESS)
		goto cleanup;

	result = dns_rdata_txt_current(&rdatatxt, &rdatastr);
	if (result != ISC_R_SUCCESS)
		goto cleanup;

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
	zone->version = tversion;
	result = ISC_R_SUCCESS;

cleanup:
	dns_rdata_freestruct(&rdatatxt);
	return (result);
}

static isc_result_t
catz_process_masters(dns_catz_zone_t *zone, dns_ipkeylist_t *ipkl,
		     dns_rdataset_t *value, dns_name_t *name)
{
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
	unsigned int i;

	REQUIRE(zone != NULL);
	REQUIRE(ipkl != NULL);
	REQUIRE(DNS_RDATASET_VALID(value));
	REQUIRE(dns_rdataset_isassociated(value));
	REQUIRE(name != NULL);

	mctx = zone->catzs->mctx;
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
	if (value->rdclass != dns_rdataclass_in)
		return (ISC_R_FAILURE);

	if (name->labels > 0) {
		isc_sockaddr_t sockaddr;

		/*
		 * We're pre-preparing the data once, we'll put it into
		 * the right spot in the masters array once we find it.
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
			break;
		case dns_rdatatype_aaaa:
			result = dns_rdata_tostruct(&rdata, &rdata_aaaa, NULL);
			RUNTIME_CHECK(result == ISC_R_SUCCESS);
			isc_sockaddr_fromin6(&sockaddr, &rdata_aaaa.in6_addr, 0);
			break;
		case dns_rdatatype_txt:
			result = dns_rdata_tostruct(&rdata, &rdata_txt, NULL);
			RUNTIME_CHECK(result == ISC_R_SUCCESS);

			result = dns_rdata_txt_first(&rdata_txt);
			if (result != ISC_R_SUCCESS)
				return (result);

			result = dns_rdata_txt_current(&rdata_txt, &rdatastr);
			if (result != ISC_R_SUCCESS)
				return (result);

			result = dns_rdata_txt_next(&rdata_txt);
			if (result != ISC_R_NOMORE)
				return (ISC_R_FAILURE);

			/* rdatastr.length < DNS_NAME_MAXTEXT */
			keyname = isc_mem_get(mctx, sizeof(dns_name_t));
			if (keyname == NULL)
				return (ISC_R_NOMEMORY);
			dns_name_init(keyname, 0);
			memmove(keycbuf, rdatastr.data, rdatastr.length);
			keycbuf[rdatastr.length] = 0;
			result = dns_name_fromstring(keyname, keycbuf, 0, mctx);
			if (result != ISC_R_SUCCESS) {
				dns_name_free(keyname, mctx);
				isc_mem_put(mctx, keyname, sizeof(dns_name_t));
				return (result);
			}
			break;
		default:
			return (ISC_R_FAILURE);
		}

		/*
		 * We have to find the appropriate labeled record in masters
		 * if it exists.
		 * In common case we'll have no more than 3-4 records here so
		 * no optimization.
		 */
		for (i = 0; i < ipkl->count; i++) {
			if (ipkl->labels[i] != NULL &&
			    !dns_name_compare(name, ipkl->labels[i]))
				break;
		}

		if (i < ipkl->count) { /* we have this record already */
			if (value->type == dns_rdatatype_txt)
				ipkl->keys[i] = keyname;
			else /* A/AAAA */
				memmove(&ipkl->addrs[i], &sockaddr,
					sizeof(isc_sockaddr_t));
		} else {
			result = dns_ipkeylist_resize(mctx, ipkl,
						      i+1);
			if (result != ISC_R_SUCCESS) {
				return (result);
			}

			ipkl->labels[i] = isc_mem_get(mctx, sizeof(dns_name_t));
			if (ipkl->labels[i] == NULL)  {
				if (keyname != NULL) {
					dns_name_free(keyname, mctx);
					isc_mem_put(mctx, keyname,
						    sizeof(dns_name_t));
				}
				return (ISC_R_NOMEMORY);
			}
			dns_name_init(ipkl->labels[i], NULL);
			result = dns_name_dup(name, mctx, ipkl->labels[i]);
			if (result != ISC_R_SUCCESS) {
				if (keyname != NULL) {
					dns_name_free(keyname, mctx);
					isc_mem_put(mctx, keyname,
						    sizeof(dns_name_t));
				}
				return (result);
			}

			if (value->type == dns_rdatatype_txt)
				ipkl->keys[i] = keyname;
			else /* A/AAAA */
				memmove(&ipkl->addrs[i], &sockaddr,
					sizeof(isc_sockaddr_t));
			ipkl->count++;
		}
		return (ISC_R_SUCCESS);
	}
	/* else - 'simple' case - without labels */

	if (value->type != dns_rdatatype_a &&
	    value->type != dns_rdatatype_aaaa)
		return (ISC_R_FAILURE);

	rcount = dns_rdataset_count(value) + ipkl->count;

	result = dns_ipkeylist_resize(mctx, ipkl, rcount);
	if (result != ISC_R_SUCCESS) {
		return (result);
	}

	for (result = dns_rdataset_first(value);
	     result == ISC_R_SUCCESS;
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
		} else {
			result = dns_rdata_tostruct(&rdata, &rdata_aaaa, NULL);
			RUNTIME_CHECK(result == ISC_R_SUCCESS);
			isc_sockaddr_fromin6(&ipkl->addrs[ipkl->count],
					     &rdata_aaaa.in6_addr, 0);
		}
		ipkl->keys[ipkl->count] = NULL;
		ipkl->labels[ipkl->count] = NULL;
		ipkl->count++;
		dns_rdata_freestruct(&rdata_a);
	}
	return (ISC_R_SUCCESS);
}

static isc_result_t
catz_process_apl(dns_catz_zone_t *zone, isc_buffer_t **aclbp,
		 dns_rdataset_t *value)
{
	isc_result_t result = ISC_R_SUCCESS;
	dns_rdata_t rdata;
	dns_rdata_in_apl_t rdata_apl;
	dns_rdata_apl_ent_t apl_ent;
	isc_netaddr_t addr;
	isc_buffer_t *aclb = NULL;
	unsigned char buf[256]; /* larger than INET6_ADDRSTRLEN */

	REQUIRE(zone != NULL);
	REQUIRE(aclbp != NULL);
	REQUIRE(*aclbp == NULL);
	REQUIRE(DNS_RDATASET_VALID(value));
	REQUIRE(dns_rdataset_isassociated(value));

	if (value->rdclass != dns_rdataclass_in ||
	    value->type != dns_rdatatype_apl)
		return (ISC_R_FAILURE);


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
	result = dns_rdata_tostruct(&rdata, &rdata_apl, zone->catzs->mctx);
	if (result != ISC_R_SUCCESS)
		return (result);
	result = isc_buffer_allocate(zone->catzs->mctx, &aclb, 16);
	isc_buffer_setautorealloc(aclb, ISC_TRUE);
	if (result != ISC_R_SUCCESS)
		goto cleanup;
	for (result = dns_rdata_apl_first(&rdata_apl);
	     result == ISC_R_SUCCESS;
	     result = dns_rdata_apl_next(&rdata_apl)) {
		result = dns_rdata_apl_current(&rdata_apl, &apl_ent);
		RUNTIME_CHECK(result == ISC_R_SUCCESS);
		memset(buf, 0, sizeof(buf));
		if (apl_ent.data != NULL && apl_ent.length > 0)
			memmove(buf, apl_ent.data, apl_ent.length);
		if (apl_ent.family == 1)
			isc_netaddr_fromin(&addr, (struct in_addr*) buf);
		else if (apl_ent.family == 2)
			isc_netaddr_fromin6(&addr, (struct in6_addr*) buf);
		else
			continue; /* xxxwpk log it or simply ignore? */
		if (apl_ent.negative)
			isc_buffer_putuint8(aclb, '!');
		isc_buffer_reserve(&aclb, INET6_ADDRSTRLEN);
		result = isc_netaddr_totext(&addr, aclb);
		RUNTIME_CHECK(result == ISC_R_SUCCESS);
		if ((apl_ent.family == 1 && apl_ent.prefix < 32) ||
		    (apl_ent.family == 2 && apl_ent.prefix < 128)) {
			isc_buffer_putuint8(aclb, '/');
			isc_buffer_putdecint(aclb, apl_ent.prefix);
		}
		isc_buffer_putstr(aclb, "; ");
	}
	if (result == ISC_R_NOMORE)
		result = ISC_R_SUCCESS;
	else
		goto cleanup;
	*aclbp = aclb;
	aclb = NULL;
cleanup:
	if (aclb != NULL)
		isc_buffer_free(&aclb);
	dns_rdata_freestruct(&rdata_apl);
	return (result);
}

static isc_result_t
catz_process_zones_suboption(dns_catz_zone_t *zone, dns_rdataset_t *value,
			     dns_label_t *mhash, dns_name_t *name)
{
	isc_result_t result;
	dns_catz_entry_t *entry = NULL;
	dns_label_t option;
	dns_name_t prefix;
	catz_opt_t opt;

	REQUIRE(zone != NULL);
	REQUIRE(mhash != NULL);
	REQUIRE(DNS_RDATASET_VALID(value));

	if (name->labels == 0)
		return (ISC_R_FAILURE);
	dns_name_getlabel(name, name->labels - 1, &option);
	opt = catz_get_option(&option);

	/*
	 * We're adding this entry now, in case the option is invalid we'll get
	 * rid of it in verification phase.
	 */
	result = isc_ht_find(zone->entries, mhash->base, mhash->length,
			     (void **) &entry);
	if (result != ISC_R_SUCCESS) {
		result = dns_catz_entry_new(zone->catzs->mctx, NULL, &entry);
		if (result != ISC_R_SUCCESS)
			return (result);
		result = isc_ht_add(zone->entries, mhash->base, mhash->length,
				    entry);
		if (result != ISC_R_SUCCESS) {
			dns_catz_entry_detach(zone, &entry);
			return (result);
		}
	}

	dns_name_init(&prefix, NULL);
	dns_name_split(name, 1, &prefix, NULL);
	switch (opt) {
	case CATZ_OPT_MASTERS:
		return (catz_process_masters(zone, &entry->opts.masters, value,
					     &prefix));
	case CATZ_OPT_ALLOW_QUERY:
		if (prefix.labels != 0)
			return (ISC_R_FAILURE);
		return (catz_process_apl(zone, &entry->opts.allow_query,
					 value));
	case CATZ_OPT_ALLOW_TRANSFER:
		if (prefix.labels != 0)
			return (ISC_R_FAILURE);
		return (catz_process_apl(zone, &entry->opts.allow_transfer,
					 value));
	default:
		return (ISC_R_FAILURE);
	}

	return (ISC_R_FAILURE);
}

static isc_result_t
catz_process_value(dns_catz_zone_t *zone, dns_name_t *name,
		   dns_rdataset_t *rdataset)
{
	dns_label_t option;
	dns_name_t prefix;
	catz_opt_t opt;

	REQUIRE(zone != NULL);
	REQUIRE(name != NULL);
	REQUIRE(DNS_RDATASET_VALID(rdataset));

	dns_name_getlabel(name, name->labels - 1, &option);
	opt = catz_get_option(&option);
	dns_name_init(&prefix, NULL);
	dns_name_split(name, 1, &prefix, NULL);
	switch (opt) {
	case CATZ_OPT_ZONES:
		return (catz_process_zones(zone, rdataset, &prefix));
	case CATZ_OPT_MASTERS:
		return (catz_process_masters(zone, &zone->zoneoptions.masters,
					     rdataset, &prefix));
	case CATZ_OPT_ALLOW_QUERY:
		if (prefix.labels != 0)
			return (ISC_R_FAILURE);
		return (catz_process_apl(zone, &zone->zoneoptions.allow_query,
					 rdataset));
	case CATZ_OPT_ALLOW_TRANSFER:
		if (prefix.labels != 0)
			return (ISC_R_FAILURE);
		return (catz_process_apl(zone,
					 &zone->zoneoptions.allow_transfer,
					 rdataset));
	case CATZ_OPT_VERSION:
		if (prefix.labels != 0)
			return (ISC_R_FAILURE);
		return (catz_process_version(zone, rdataset));
	default:
		return (ISC_R_FAILURE);
	}
}

isc_result_t
dns_catz_update_process(dns_catz_zones_t *catzs, dns_catz_zone_t *zone,
			const dns_name_t *src_name, dns_rdataset_t *rdataset)
{
	isc_result_t result;
	int order;
	unsigned int nlabels;
	dns_namereln_t nrres;
	dns_rdata_t rdata = DNS_RDATA_INIT;
	dns_rdata_soa_t soa;
	dns_name_t prefix;

	REQUIRE(catzs != NULL);
	REQUIRE(zone != NULL);

	nrres = dns_name_fullcompare(src_name, &zone->name, &order, &nlabels);
	if (nrres == dns_namereln_equal) {
		if (rdataset->type == dns_rdatatype_soa) {
			result = dns_rdataset_first(rdataset);
			if (result != ISC_R_SUCCESS)
				return (result);

			dns_rdataset_current(rdataset, &rdata);
			result = dns_rdata_tostruct(&rdata, &soa, NULL);
			RUNTIME_CHECK(result == ISC_R_SUCCESS);

			/*
			 * xxxwpk TODO do we want to save something from SOA?
			 */
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
	dns_name_split(src_name, zone->name.labels, &prefix, NULL);
	result = catz_process_value(zone, &prefix, rdataset);

	return (result);
}

isc_result_t
dns_catz_generate_masterfilename(dns_catz_zone_t *zone, dns_catz_entry_t *entry,
				 isc_buffer_t **buffer)
{
	isc_buffer_t *tbuf = NULL;
	isc_sha256_t sha256;
	isc_region_t r;
	isc_result_t result;
	size_t rlen;

	REQUIRE(zone != NULL);
	REQUIRE(entry != NULL);
	REQUIRE(buffer != NULL && *buffer != NULL);

	result = isc_buffer_allocate(zone->catzs->mctx, &tbuf,
				     strlen(zone->catzs->view->name) +
				     2 * DNS_NAME_FORMATSIZE + 2);
	if (result != ISC_R_SUCCESS)
		return (result);
	INSIST(tbuf != NULL);

	isc_buffer_putstr(tbuf, zone->catzs->view->name);
	isc_buffer_putstr(tbuf, "_");
	result = dns_name_totext(&zone->name, ISC_TRUE, tbuf);
	if (result != ISC_R_SUCCESS)
		goto cleanup;

	isc_buffer_putstr(tbuf, "_");
	result = dns_name_totext(&entry->name, ISC_TRUE, tbuf);
	if (result != ISC_R_SUCCESS)
		goto cleanup;

	/* __catz__<digest>.db */
	rlen = ISC_SHA256_DIGESTSTRINGLENGTH + 12;

	/* optionally prepend with <zonedir>/ */
	if (entry->opts.zonedir != NULL)
		rlen += strlen(entry->opts.zonedir) + 1;

	result = isc_buffer_reserve(buffer, (unsigned int)rlen);
	if (result != ISC_R_SUCCESS)
		goto cleanup;

	if (entry->opts.zonedir != NULL) {
		isc_buffer_putstr(*buffer, entry->opts.zonedir);
		isc_buffer_putstr(*buffer, "/");
	}

	isc_buffer_usedregion(tbuf, &r);
	isc_buffer_putstr(*buffer, "__catz__");
	if (tbuf->used > ISC_SHA256_DIGESTSTRINGLENGTH) {
		isc_sha256_init(&sha256);
		isc_sha256_update(&sha256, r.base, r.length);
		/* we can do that because digest string < 2 * DNS_NAME */
		isc_sha256_end(&sha256, (char *) r.base);
		isc_buffer_putstr(*buffer, (char *) r.base);
	} else {
		isc_buffer_copyregion(*buffer, &r);
	}

	isc_buffer_putstr(*buffer, ".db");
	result = ISC_R_SUCCESS;

cleanup:
	isc_buffer_free(&tbuf);
	return (result);
}

isc_result_t
dns_catz_generate_zonecfg(dns_catz_zone_t *zone, dns_catz_entry_t *entry,
			  isc_buffer_t **buf)
{
	/*
	 * We have to generate a text buffer with regular zone config:
	 * zone foo.bar {
	 * 	type slave;
	 * 	masters [ dscp X ] { ip1 port port1; ip2 port port2; };
	 * }
	 */
	isc_buffer_t *buffer = NULL;
	isc_region_t region;
	isc_result_t result;
	isc_uint32_t i;
	isc_netaddr_t netaddr;
	char pbuf[sizeof("65535")]; /* used both for port number and DSCP */
	char zname[DNS_NAME_FORMATSIZE];

	REQUIRE(zone != NULL);
	REQUIRE(entry != NULL);
	REQUIRE(buf != NULL && *buf == NULL);

	/*
	 * The buffer will be reallocated if something won't fit,
	 * ISC_BUFFER_INCR seems like a good start.
	 */
	result = isc_buffer_allocate(zone->catzs->mctx, &buffer,
				     ISC_BUFFER_INCR);
	if (result != ISC_R_SUCCESS) {
		goto cleanup;
	}

	isc_buffer_setautorealloc(buffer, ISC_TRUE);
	isc_buffer_putstr(buffer, "zone ");
	dns_name_totext(&entry->name, ISC_TRUE, buffer);
	isc_buffer_putstr(buffer, " { type slave; masters");

	/*
	 * DSCP value has no default, but when it is specified, it is identical
	 * for all masters and cannot be overriden for a specific master IP, so
	 * use the DSCP value set for the first master
	 */
	if (entry->opts.masters.count > 0 &&
	    entry->opts.masters.dscps[0] >= 0) {
		isc_buffer_putstr(buffer, " dscp ");
		snprintf(pbuf, sizeof(pbuf), "%hd",
			 entry->opts.masters.dscps[0]);
		isc_buffer_putstr(buffer, pbuf);
	}

	isc_buffer_putstr(buffer, " { ");
	for (i = 0; i < entry->opts.masters.count; i++) {
		/*
		 * Every master must have an IP address assigned.
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
				      "catz: zone '%s' uses an invalid master "
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
						 ISC_TRUE, buffer);
			if (result != ISC_R_SUCCESS)
				goto cleanup;
		}
		isc_buffer_putstr(buffer, "; ");
	}
	isc_buffer_putstr(buffer, "}; ");
	if (entry->opts.in_memory == ISC_FALSE) {
		isc_buffer_putstr(buffer, "file \"");
		result = dns_catz_generate_masterfilename(zone, entry, &buffer);
		if (result != ISC_R_SUCCESS)
			goto cleanup;
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
	if (buffer != NULL)
		isc_buffer_free(&buffer);
	return (result);
}

void
dns_catz_update_taskaction(isc_task_t *task, isc_event_t *event) {
	isc_result_t result;
	dns_catz_zone_t * zone;
	(void) task;

	REQUIRE(event != NULL);
	zone = event->ev_arg;
	REQUIRE(zone != NULL);

	LOCK(&zone->catzs->lock);
	zone->updatepending = ISC_FALSE;
	dns_catz_update_from_db(zone->db, zone->catzs);
	result = isc_timer_reset(zone->updatetimer, isc_timertype_inactive,
				 NULL, NULL, ISC_TRUE);
	RUNTIME_CHECK(result == ISC_R_SUCCESS);
	isc_event_free(&event);
	result = isc_time_now(&zone->lastupdated);
	RUNTIME_CHECK(result == ISC_R_SUCCESS);
	UNLOCK(&zone->catzs->lock);
}

isc_result_t
dns_catz_dbupdate_callback(dns_db_t *db, void *fn_arg) {
	dns_catz_zones_t *catzs;
	dns_catz_zone_t *zone = NULL;
	isc_time_t now;
	isc_uint64_t tdiff;
	isc_result_t result = ISC_R_SUCCESS;
	isc_region_t r;

	REQUIRE(DNS_DB_VALID(db));
	REQUIRE(fn_arg != NULL);
	catzs = (dns_catz_zones_t *) fn_arg;

	dns_name_toregion(&db->origin, &r);

	LOCK(&catzs->lock);
	result = isc_ht_find(catzs->zones, r.base, r.length, (void **) &zone);
	if (result != ISC_R_SUCCESS)
		goto cleanup;

	/* New zone came as AXFR */
	if (zone->db != NULL && zone->db != db) {
		if (zone->dbversion != NULL)
			dns_db_closeversion(zone->db, &zone->dbversion,
						    ISC_FALSE);
		dns_db_detach(&zone->db);
		/*
		 * We're not registering db update callback, it will be
		 * registered at the end of update_from_db
		 */
		zone->db_registered = ISC_FALSE;
	}
	if (zone->db == NULL)
		dns_db_attach(db, &zone->db);

	if (zone->updatepending == ISC_FALSE) {
		zone->updatepending = ISC_TRUE;
		isc_time_now(&now);
		tdiff = isc_time_microdiff(&now, &zone->lastupdated)/1000000;
		if (tdiff < zone->defoptions.min_update_interval) {
			isc_interval_t interval;
			isc_log_write(dns_lctx, DNS_LOGCATEGORY_GENERAL,
				      DNS_LOGMODULE_MASTER, ISC_LOG_INFO,
				      "catz: new zone version came too soon, "
				      "deferring update");
			isc_interval_set(&interval,
					 zone->defoptions.min_update_interval -
					 (unsigned int)tdiff, 0);
			dns_db_currentversion(db, &zone->dbversion);
			result = isc_timer_reset(zone->updatetimer,
						 isc_timertype_once,
						 NULL, &interval, ISC_TRUE);
			if (result != ISC_R_SUCCESS)
				goto cleanup;
		} else {
			isc_event_t *event;

			dns_db_currentversion(db, &zone->dbversion);
			ISC_EVENT_INIT(&zone->updateevent,
				       sizeof(zone->updateevent), 0, NULL,
				       DNS_EVENT_CATZUPDATED,
				       dns_catz_update_taskaction,
				       zone, zone, NULL, NULL);
			event = &zone->updateevent;
			isc_task_send(catzs->updater, &event);
		}
	} else {
		isc_log_write(dns_lctx, DNS_LOGCATEGORY_GENERAL,
			      DNS_LOGMODULE_MASTER, ISC_LOG_DEBUG(3),
			      "catz: update already queued");
		if (zone->dbversion != NULL)
			dns_db_closeversion(zone->db, &zone->dbversion,
					    ISC_FALSE);
		dns_db_currentversion(zone->db, &zone->dbversion);
	}

  cleanup:
	UNLOCK(&catzs->lock);

	return (result);
}

void
dns_catz_update_from_db(dns_db_t *db, dns_catz_zones_t *catzs) {
	dns_catz_zone_t *oldzone = NULL, *newzone = NULL;
	isc_result_t result;
	isc_region_t r;
	dns_dbnode_t *node = NULL;
	dns_dbiterator_t *it = NULL;
	dns_fixedname_t fixname;
	dns_name_t *name;
	dns_rdatasetiter_t *rdsiter = NULL;
	dns_rdataset_t rdataset;
	char bname[DNS_NAME_FORMATSIZE];
	isc_buffer_t ibname;
	isc_uint32_t vers;

	REQUIRE(DNS_DB_VALID(db));
	REQUIRE(catzs != NULL);

	/*
	 * Create a new catz in the same context as current catz.
	 */
	dns_name_toregion(&db->origin, &r);
	result = isc_ht_find(catzs->zones, r.base, r.length, (void **)&oldzone);
	if (result != ISC_R_SUCCESS) {
		/* This can happen if we remove the zone in the meantime. */
		isc_log_write(dns_lctx, DNS_LOGCATEGORY_GENERAL,
			      DNS_LOGMODULE_MASTER, ISC_LOG_ERROR,
			      "catz: zone '%s' not in config",
			      bname);
		return;
	}

	isc_buffer_init(&ibname, bname, DNS_NAME_FORMATSIZE);
	result = dns_name_totext(&db->origin, ISC_TRUE, &ibname);
	INSIST(result == ISC_R_SUCCESS);

	result = dns_db_getsoaserial(db, oldzone->dbversion, &vers);
	if (result != ISC_R_SUCCESS) {
		/* A zone without SOA record?!? */
		isc_log_write(dns_lctx, DNS_LOGCATEGORY_GENERAL,
			      DNS_LOGMODULE_MASTER, ISC_LOG_ERROR,
			      "catz: zone '%s' has no SOA record (%s)",
			      bname, isc_result_totext(result));
		return;
	}

	isc_log_write(dns_lctx, DNS_LOGCATEGORY_GENERAL,
		      DNS_LOGMODULE_MASTER, ISC_LOG_INFO,
		      "catz: updating catalog zone '%s' with serial %d",
		      bname, vers);

	result = dns_catz_new_zone(catzs, &newzone, &db->origin);
	if (result != ISC_R_SUCCESS) {
		dns_db_closeversion(db, &oldzone->dbversion, ISC_FALSE);
		isc_log_write(dns_lctx, DNS_LOGCATEGORY_GENERAL,
			      DNS_LOGMODULE_MASTER, ISC_LOG_ERROR,
			      "catz: failed to create new zone - %s",
			      isc_result_totext(result));
		return;
	}

	result = dns_db_createiterator(db, DNS_DB_NONSEC3, &it);
	if (result != ISC_R_SUCCESS) {
		dns_catz_zone_detach(&newzone);
		dns_db_closeversion(db, &oldzone->dbversion, ISC_FALSE);
		isc_log_write(dns_lctx, DNS_LOGCATEGORY_GENERAL,
			      DNS_LOGMODULE_MASTER, ISC_LOG_ERROR,
			      "catz: failed to create DB iterator - %s",
			      isc_result_totext(result));
		return;
	}

	name = dns_fixedname_initname(&fixname);

	/*
	 * Iterate over database to fill the new zone.
	 */
	result = dns_dbiterator_first(it);
	if (result != ISC_R_SUCCESS) {
		isc_log_write(dns_lctx, DNS_LOGCATEGORY_GENERAL,
			      DNS_LOGMODULE_MASTER, ISC_LOG_ERROR,
			      "catz: failed to get db iterator - %s",
			      isc_result_totext(result));
	}

	while (result == ISC_R_SUCCESS) {
		result = dns_dbiterator_current(it, &node, name);
		if (result != ISC_R_SUCCESS) {
			isc_log_write(dns_lctx, DNS_LOGCATEGORY_GENERAL,
				      DNS_LOGMODULE_MASTER, ISC_LOG_ERROR,
				      "catz: failed to get db iterator - %s",
				      isc_result_totext(result));
			break;
		}

		result = dns_db_allrdatasets(db, node, oldzone->dbversion, 0,
					  &rdsiter);
		if (result != ISC_R_SUCCESS) {
			isc_log_write(dns_lctx, DNS_LOGCATEGORY_GENERAL,
				      DNS_LOGMODULE_MASTER, ISC_LOG_ERROR,
				      "catz: failed to fetch rrdatasets - %s",
				      isc_result_totext(result));
			dns_db_detachnode(db, &node);
			break;
		}

		dns_rdataset_init(&rdataset);
		result = dns_rdatasetiter_first(rdsiter);
		while (result == ISC_R_SUCCESS) {
			dns_rdatasetiter_current(rdsiter, &rdataset);
			result = dns_catz_update_process(catzs, newzone, name,
							 &rdataset);
			if (result != ISC_R_SUCCESS) {
				char cname[DNS_NAME_FORMATSIZE];
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
					      "catz: unknown record in catalog "
					      "zone - %s %s %s(%s) - ignoring",
					      cname, classbuf, typebuf,
					      isc_result_totext(result));
			}
			dns_rdataset_disassociate(&rdataset);
			if (result != ISC_R_SUCCESS) {
				break;
			}
			result = dns_rdatasetiter_next(rdsiter);
		}

		dns_rdatasetiter_destroy(&rdsiter);

		dns_db_detachnode(db, &node);
		result = dns_dbiterator_next(it);
	}

	dns_dbiterator_destroy(&it);
	dns_db_closeversion(db, &oldzone->dbversion, ISC_FALSE);
	isc_log_write(dns_lctx, DNS_LOGCATEGORY_GENERAL,
		      DNS_LOGMODULE_MASTER, ISC_LOG_DEBUG(3),
		      "catz: update_from_db: iteration finished");

	/*
	 * Finally merge new zone into old zone.
	 */
	result = dns_catz_zones_merge(oldzone, newzone);
	dns_catz_zone_detach(&newzone);
	if (result != ISC_R_SUCCESS) {
		isc_log_write(dns_lctx, DNS_LOGCATEGORY_GENERAL,
			      DNS_LOGMODULE_MASTER,
			      ISC_LOG_ERROR,
			      "catz: failed merging zones: %s",
			      isc_result_totext(result));

		return;
	}

	isc_log_write(dns_lctx, DNS_LOGCATEGORY_GENERAL,
		      DNS_LOGMODULE_MASTER, ISC_LOG_DEBUG(3),
		      "catz: update_from_db: new zone merged");

	/*
	 * When we're doing reconfig and setting a new catalog zone
	 * from an existing zone we won't have a chance to set up
	 * update callback in zone_startload or axfr_makedb, but we will
	 * call onupdate() artificially so we can register the callback here.
	 */
	if (oldzone->db_registered == ISC_FALSE) {
		result = dns_db_updatenotify_register(db,
						    dns_catz_dbupdate_callback,
						    oldzone->catzs);
		if (result == ISC_R_SUCCESS)
			oldzone->db_registered = ISC_TRUE;
	}
}

void
dns_catz_prereconfig(dns_catz_zones_t *catzs) {
	isc_result_t result;
	isc_ht_iter_t *iter = NULL;
	dns_catz_zone_t *zone;

	REQUIRE(catzs != NULL);

	result = isc_ht_iter_create(catzs->zones, &iter);
	INSIST(result == ISC_R_SUCCESS);
	for (result = isc_ht_iter_first(iter);
	     result == ISC_R_SUCCESS;
	     result = isc_ht_iter_next(iter))
	{
		isc_ht_iter_current(iter, (void **) &zone);
		zone->active = ISC_FALSE;
	}
	INSIST(result == ISC_R_NOMORE);
	isc_ht_iter_destroy(&iter);
}

void
dns_catz_postreconfig(dns_catz_zones_t *catzs) {
	isc_result_t result;
	dns_catz_zone_t *newzone = NULL;
	isc_ht_iter_t *iter = NULL;
	dns_catz_zone_t *zone;

	LOCK(&catzs->lock);
	result = isc_ht_iter_create(catzs->zones, &iter);
	INSIST(result == ISC_R_SUCCESS);
	for (result = isc_ht_iter_first(iter);
	     result == ISC_R_SUCCESS;)
	{
		isc_ht_iter_current(iter, (void **) &zone);
		if (zone->active == ISC_FALSE) {
			char cname[DNS_NAME_FORMATSIZE];
			dns_name_format(&zone->name, cname,
					DNS_NAME_FORMATSIZE);
			isc_log_write(dns_lctx, DNS_LOGCATEGORY_GENERAL,
				      DNS_LOGMODULE_MASTER,
				      ISC_LOG_WARNING,
				      "catz: removing catalog zone %s", cname);

			/*
			 * Merge the old zone with an empty one to remove
			 * all members.
			 */
			result = dns_catz_new_zone(catzs, &newzone,
						   &zone->name);
			INSIST(result == ISC_R_SUCCESS);
			dns_catz_zones_merge(zone, newzone);
			dns_catz_zone_detach(&newzone);

			/* Make sure that we have an empty catalog zone. */
			INSIST(isc_ht_count(zone->entries) == 0);
			result = isc_ht_iter_delcurrent_next(iter);
			dns_catz_zone_detach(&zone);
		} else {
			result = isc_ht_iter_next(iter);
		}
	}
	UNLOCK(&catzs->lock);
	RUNTIME_CHECK(result == ISC_R_NOMORE);
	isc_ht_iter_destroy(&iter);
}

isc_result_t
dns_catz_get_iterator(dns_catz_zone_t *catz, isc_ht_iter_t **itp) {
	return (isc_ht_iter_create(catz->entries, itp));
}
