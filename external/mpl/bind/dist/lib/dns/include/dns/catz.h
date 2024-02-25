/*	$NetBSD: catz.h,v 1.6.2.1 2024/02/25 15:46:55 martin Exp $	*/

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

#pragma once

/* Add -DDNS_CATZ_TRACE=1 to CFLAGS for detailed reference tracing */

#include <inttypes.h>
#include <stdbool.h>

#include <isc/ht.h>
#include <isc/lang.h>
#include <isc/refcount.h>
#include <isc/rwlock.h>
#include <isc/time.h>
#include <isc/timer.h>

#include <dns/db.h>
#include <dns/fixedname.h>
#include <dns/ipkeylist.h>
#include <dns/rdata.h>
#include <dns/types.h>

ISC_LANG_BEGINDECLS

#define DNS_CATZ_ERROR_LEVEL  ISC_LOG_WARNING
#define DNS_CATZ_INFO_LEVEL   ISC_LOG_INFO
#define DNS_CATZ_DEBUG_LEVEL1 ISC_LOG_DEBUG(1)
#define DNS_CATZ_DEBUG_LEVEL2 ISC_LOG_DEBUG(2)
#define DNS_CATZ_DEBUG_LEVEL3 ISC_LOG_DEBUG(3)
#define DNS_CATZ_DEBUG_QUIET  (DNS_CATZ_DEBUG_LEVEL3 + 1)

/*
 * Catalog Zones functions and structures.
 */

/*
 * Options for a member zone in a catalog
 */
struct dns_catz_entry_options {
	/*
	 * Options that can be overridden in catalog zone
	 */
	/* default-masters/default-primaries definition */
	dns_ipkeylist_t masters;

	/* both as text in config format, NULL if none */
	isc_buffer_t *allow_query;
	isc_buffer_t *allow_transfer;

	/*
	 * Options that are only set in named.conf
	 */
	/* zone-directory definition */
	char *zonedir;

	/* zone should not be stored on disk (no 'file' statement in def */
	bool in_memory;
	/*
	 * Minimal interval between catalog zone updates, if a new version
	 * of catalog zone is received before this time the update will be
	 * postponed. This is a global option for the whole catalog zone.
	 */
	uint32_t min_update_interval;
};

void
dns_catz_options_init(dns_catz_options_t *options);
/*%<
 * Initialize 'options' to NULL values.
 *
 * Requires:
 * \li	'options' to be non NULL.
 */

void
dns_catz_options_free(dns_catz_options_t *options, isc_mem_t *mctx);
/*%<
 * Free 'options' contents into 'mctx'. ('options' itself is not freed.)
 *
 * Requires:
 * \li	'options' to be non NULL.
 * \li	'mctx' to be a valid memory context.
 */

void
dns_catz_options_copy(isc_mem_t *mctx, const dns_catz_options_t *opts,
		      dns_catz_options_t *nopts);
/*%<
 * Duplicate 'opts' into 'nopts', allocating space from 'mctx'.
 *
 * Requires:
 * \li	'mctx' to be a valid memory context.
 * \li	'options' to be non NULL and valid options.
 * \li	'nopts' to be non NULL.
 */

void
dns_catz_options_setdefault(isc_mem_t *mctx, const dns_catz_options_t *defaults,
			    dns_catz_options_t *opts);
/*%<
 * Replace empty values in 'opts' with values from 'defaults'
 *
 * Requires:
 * \li	'mctx' to be a valid memory context.
 * \li	'defaults' to be non NULL and valid options.
 * \li	'opts' to be non NULL.
 */

dns_name_t *
dns_catz_entry_getname(dns_catz_entry_t *entry);
/*%<
 * Get domain name for 'entry'
 *
 * Requires:
 * \li	'entry' to be non NULL.
 *
 * Returns:
 * \li	domain name for entry.
 */

void
dns_catz_entry_new(isc_mem_t *mctx, const dns_name_t *domain,
		   dns_catz_entry_t **nentryp);
/*%<
 * Allocate a new catz_entry on 'mctx', with the name 'domain'
 *
 * Requires:
 * \li	'mctx' to be a valid memory context.
 * \li	'domain' to be valid dns_name or NULL.
 * \li	'nentryp' to be non NULL, *nentryp to be NULL.
 *
 * Returns:
 * \li	ISC_R_SUCCESS on success
 * \li	ISC_R_NOMEMORY on allocation failure
 */

void
dns_catz_entry_copy(dns_catz_zone_t *catz, const dns_catz_entry_t *entry,
		    dns_catz_entry_t **nentryp);
/*%<
 * Allocate a new catz_entry and deep copy 'entry' into 'nentryp'.
 *
 * Requires:
 * \li	'mctx' to be a valid memory context.
 * \li	'entry' to be non NULL.
 * \li	'nentryp' to be non NULL, *nentryp to be NULL.
 *
 * Returns:
 * \li	ISC_R_SUCCESS on success
 * \li	ISC_R_NOMEMORY on allocation failure
 */

void
dns_catz_entry_attach(dns_catz_entry_t *entry, dns_catz_entry_t **entryp);
/*%<
 * Attach an entry
 *
 * Requires:
 * \li	'entry' is a valid dns_catz_entry_t.
 * \li	'entryp' is not NULL and '*entryp' is NULL.
 */

void
dns_catz_entry_detach(dns_catz_zone_t *catz, dns_catz_entry_t **entryp);
/*%<
 * Detach an entry, free if no further references
 *
 * Requires:
 * \li	'catz' is a valid dns_catz_zone_t.
 * \li	'entryp' is not NULL and '*entryp' is not NULL.
 */

bool
dns_catz_entry_validate(const dns_catz_entry_t *entry);
/*%<
 * Validate whether entry is correct.
 * (NOT YET IMPLEMENTED: always returns true)
 *
 * Requires:
 *\li	'entry' is a valid dns_catz_entry_t.
 */

bool
dns_catz_entry_cmp(const dns_catz_entry_t *ea, const dns_catz_entry_t *eb);
/*%<
 * Deep compare two entries
 *
 * Requires:
 * \li	'ea' is a valid dns_catz_entry_t.
 * \li	'eb' is a valid dns_catz_entry_t.
 *
 * Returns:
 * \li 'true' if entries are the same.
 * \li 'false' if the entries differ.
 */

isc_result_t
dns_catz_new_zone(dns_catz_zones_t *catzs, dns_catz_zone_t **catzp,
		  const dns_name_t *name);
/*%<
 * Allocate a new catz zone on catzs mctx
 *
 * Requires:
 * \li	'catzs' is a valid dns_catz_zones_t.
 * \li	'catzp' is not NULL and '*zonep' is NULL.
 * \li	'name' is a valid dns_name_t.
 *
 */

dns_name_t *
dns_catz_zone_getname(dns_catz_zone_t *catz);
/*%<
 * Get catalog zone name
 *
 * Requires:
 * \li	'catz' is a valid dns_catz_zone_t.
 */

dns_catz_options_t *
dns_catz_zone_getdefoptions(dns_catz_zone_t *catz);
/*%<
 * Get default member zone options for catalog zone 'catz'
 *
 * Requires:
 * \li	'catz' is a valid dns_catz_zone_t.
 */

void
dns_catz_zone_resetdefoptions(dns_catz_zone_t *catz);
/*%<
 * Reset the default member zone options for catalog zone 'catz' to
 * the default values.
 *
 * Requires:
 * \li	'catz' is a valid dns_catz_zone_t.
 */

isc_result_t
dns_catz_generate_masterfilename(dns_catz_zone_t *catz, dns_catz_entry_t *entry,
				 isc_buffer_t **buffer);
/*%<
 * Generate master file name and put it into *buffer (might be reallocated).
 * The general format of the file name is:
 * __catz__catalog.zone.name__member_zone_name.db
 * But if it's too long it's shortened to:
 * __catz__unique_hash_generated_from_the_above.db
 *
 * Requires:
 * \li	'catz' is a valid dns_catz_zone_t.
 * \li	'entry' is a valid dns_catz_entry_t.
 * \li	'buffer' is not NULL and '*buffer' is not NULL.
 */

isc_result_t
dns_catz_generate_zonecfg(dns_catz_zone_t *catz, dns_catz_entry_t *entry,
			  isc_buffer_t **buf);
/*%<
 * Generate a zone config entry (in text form) from dns_catz_entry and puts
 * it into *buf. buf might be reallocated.
 *
 * Requires:
 * \li	'catz' is a valid dns_catz_zone_t.
 * \li	'entry' is a valid dns_catz_entry_t.
 * \li	'buf' is not NULL and '*buf' is NULL.
 *
 */

/* Methods provided by named to dynamically modify the member zones */
/* xxxwpk TODO config! */
typedef isc_result_t (*dns_catz_zoneop_fn_t)(dns_catz_entry_t *entry,
					     dns_catz_zone_t  *origin,
					     dns_view_t	      *view,
					     isc_taskmgr_t    *taskmgr,
					     void	      *udata);
struct dns_catz_zonemodmethods {
	dns_catz_zoneop_fn_t addzone;
	dns_catz_zoneop_fn_t modzone;
	dns_catz_zoneop_fn_t delzone;
	void		    *udata;
};

isc_result_t
dns_catz_new_zones(isc_mem_t *mctx, isc_taskmgr_t *taskmgr,
		   isc_timermgr_t *timermgr, dns_catz_zones_t **catzsp,
		   dns_catz_zonemodmethods_t *zmm);
/*%<
 * Allocate a new catz_zones object, a collection storing all catalog zones
 * for a view.
 *
 * Requires:
 * \li 'mctx' is not NULL.
 * \li 'taskmgr' is not NULL.
 * \li 'timermgr' is not NULL.
 * \li 'catzsp' is not NULL and '*catzsp' is NULL.
 * \li 'zmm' is not NULL.
 *
 */

isc_result_t
dns_catz_add_zone(dns_catz_zones_t *catzs, const dns_name_t *name,
		  dns_catz_zone_t **catzp);
/*%<
 * Allocate a new catz named 'name' and put it in 'catzs' collection. This
 * function is safe to call only during a (re)configuration.
 *
 * Requires:
 * \li	'catzs' is a valid dns_catz_zones_t.
 * \li	'name' is a valid dns_name_t.
 * \li	'catzp' is not NULL and *catzp is NULL.
 *
 */

dns_catz_zone_t *
dns_catz_get_zone(dns_catz_zones_t *catzs, const dns_name_t *name);
/*%<
 * Returns a zone named 'name' from collection 'catzs'
 *
 * Requires:
 * \li	'catzs' is a valid dns_catz_zones_t.
 * \li	'name' is a valid dns_name_t.
 */

void
dns_catz_catzs_set_view(dns_catz_zones_t *catzs, dns_view_t *view);
/*%<
 * Set a view for 'catzs'.
 *
 * Requires:
 * \li	'catzs' is a valid dns_catz_zones_t.
 * \li	'catzs->view' is NULL or 'catzs->view' == 'view'.
 */

isc_result_t
dns_catz_dbupdate_callback(dns_db_t *db, void *fn_arg);
/*%<
 * Callback for update of catalog zone database.
 * If there was no catalog zone update recently it launches an
 * update_taskaction immediately.
 * If there was an update recently it schedules update_taskaction for some time
 * in the future.
 * If there is an update scheduled it replaces old db version with a new one.
 *
 * Requires:
 * \li	'db' is a valid database.
 * \li	'fn_arg' is not NULL (casted to dns_catz_zones_t*).
 */

void
dns_catz_dbupdate_unregister(dns_db_t *db, dns_catz_zones_t *catzs);
/*%<
 * Register the catalog zone database update notify callback.
 *
 * Requires:
 * \li	'db' is a valid database.
 * \li	'catzs' is valid.
 */

void
dns_catz_dbupdate_register(dns_db_t *db, dns_catz_zones_t *catzs);
/*%<
 * Unregister the catalog zone database update notify callback.
 *
 * Requires:
 * \li	'db' is a valid database.
 * \li	'catzs' is valid.
 */

void
dns_catz_prereconfig(dns_catz_zones_t *catzs);
/*%<
 * Called before reconfig, clears 'active' flag on all the zones in set
 *
 * Requires:
 * \li	'catzs' is a valid dns_catz_zones_t.
 *
 */

void
dns_catz_postreconfig(dns_catz_zones_t *catzs);
/*%<
 * Called after reconfig, walks through all zones in set, removes those
 * inactive and force reload of those with changed configuration.
 *
 * Requires:
 * \li	'catzs' is a valid dns_catz_zones_t.
 */

void
dns_catz_get_iterator(dns_catz_zone_t *catz, isc_ht_iter_t **itp);
/*%<
 * Get the hashtable iterator on catalog zone members, point '*itp' to it.
 *
 * Requires:
 * \li	'catzs' is a valid dns_catz_zones_t.
 * \li	'itp' is not NULL and '*itp' is NULL.
 *
 */

void
dns_catz_shutdown_catzs(dns_catz_zones_t *catzs);
/*%<
 * Shut down the catalog zones.
 *
 * Requires:
 * \li	'catzs' is a valid dns_catz_zones_t.
 *
 */

#ifdef DNS_CATZ_TRACE
/* Compatibility macros */
#define dns_catz_attach_catz(catz, catzp) \
	dns_catz_zone__attach(catz, catzp, __func__, __FILE__, __LINE__)
#define dns_catz_detach_catz(catzp) \
	dns_catz_zone__detach(catzp, __func__, __FILE__, __LINE__)
#define dns_catz_ref_catz(ptr) \
	dns_catz_zone__ref(ptr, __func__, __FILE__, __LINE__)
#define dns_catz_unref_catz(ptr) \
	dns_catz_zone__unref(ptr, __func__, __FILE__, __LINE__)

#define dns_catz_attach_catzs(catzs, catzsp) \
	dns_catz_zones__attach(catzs, catzsp, __func__, __FILE__, __LINE__)
#define dns_catz_detach_catzs(catzsp) \
	dns_catz_zones__detach(catzsp, __func__, __FILE__, __LINE__)
#define dns_catz_ref_catzs(ptr) \
	dns_catz_zones__ref(ptr, __func__, __FILE__, __LINE__)
#define dns_catz_unref_catzs(ptr) \
	dns_catz_zones__unref(ptr, __func__, __FILE__, __LINE__)

ISC_REFCOUNT_TRACE_DECL(dns_catz_zone);
ISC_REFCOUNT_TRACE_DECL(dns_catz_zones);
#else
/* Compatibility macros */
#define dns_catz_attach_catz(catz, catzp) dns_catz_zone_attach(catz, catzp)
#define dns_catz_detach_catz(catzp)	  dns_catz_zone_detach(catzp)
#define dns_catz_ref_catz(ptr)		  dns_catz_zone_ref(ptr)
#define dns_catz_unref_catz(ptr)	  dns_catz_zone_unref(ptr)

#define dns_catz_attach_catzs(catzs, catzsp) \
	dns_catz_zones_attach(catzs, catzsp)
#define dns_catz_detach_catzs(catzsp) dns_catz_zones_detach(catzsp)
#define dns_catz_ref_catzs(ptr)	      dns_catz_zones_ref(ptr)
#define dns_catz_unref_catzs(ptr)     dns_catz_zones_unref(ptr)

ISC_REFCOUNT_DECL(dns_catz_zone);
ISC_REFCOUNT_DECL(dns_catz_zones);
#endif /* DNS_CATZ_TRACE */

ISC_LANG_ENDDECLS
