/*	$NetBSD: dlz_dlopen_driver.c,v 1.2.4.2 2024/02/29 12:28:18 martin Exp $	*/

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

#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <uv.h>

#include <isc/mem.h>
#include <isc/print.h>
#include <isc/result.h>
#include <isc/util.h>

#include <dns/dlz_dlopen.h>
#include <dns/log.h>

#include <dlz/dlz_dlopen_driver.h>
#include <named/globals.h>

static dns_sdlzimplementation_t *dlz_dlopen = NULL;

typedef struct dlopen_data {
	isc_mem_t *mctx;
	char *dl_path;
	char *dlzname;
	uv_lib_t dl_handle;
	void *dbdata;
	unsigned int flags;
	isc_mutex_t lock;
	int version;
	bool in_configure;

	dlz_dlopen_version_t *dlz_version;
	dlz_dlopen_create_t *dlz_create;
	dlz_dlopen_findzonedb_t *dlz_findzonedb;
	dlz_dlopen_lookup_t *dlz_lookup;
	dlz_dlopen_authority_t *dlz_authority;
	dlz_dlopen_allnodes_t *dlz_allnodes;
	dlz_dlopen_allowzonexfr_t *dlz_allowzonexfr;
	dlz_dlopen_newversion_t *dlz_newversion;
	dlz_dlopen_closeversion_t *dlz_closeversion;
	dlz_dlopen_configure_t *dlz_configure;
	dlz_dlopen_ssumatch_t *dlz_ssumatch;
	dlz_dlopen_addrdataset_t *dlz_addrdataset;
	dlz_dlopen_subrdataset_t *dlz_subrdataset;
	dlz_dlopen_delrdataset_t *dlz_delrdataset;
	dlz_dlopen_destroy_t *dlz_destroy;
} dlopen_data_t;

/* Modules can choose whether they are lock-safe or not. */
#define MAYBE_LOCK(cd)                                            \
	do {                                                      \
		if ((cd->flags & DNS_SDLZFLAG_THREADSAFE) == 0 && \
		    !cd->in_configure)                            \
			LOCK(&cd->lock);                          \
	} while (0)

#define MAYBE_UNLOCK(cd)                                          \
	do {                                                      \
		if ((cd->flags & DNS_SDLZFLAG_THREADSAFE) == 0 && \
		    !cd->in_configure)                            \
			UNLOCK(&cd->lock);                        \
	} while (0)

/*
 * Log a message at the given level.
 */
static void
dlopen_log(int level, const char *fmt, ...) {
	va_list ap;
	va_start(ap, fmt);
	isc_log_vwrite(dns_lctx, DNS_LOGCATEGORY_DATABASE, DNS_LOGMODULE_DLZ,
		       ISC_LOG_DEBUG(level), fmt, ap);
	va_end(ap);
}

/*
 * SDLZ methods
 */

static isc_result_t
dlopen_dlz_allnodes(const char *zone, void *driverarg, void *dbdata,
		    dns_sdlzallnodes_t *allnodes) {
	dlopen_data_t *cd = (dlopen_data_t *)dbdata;
	isc_result_t result;

	UNUSED(driverarg);

	if (cd->dlz_allnodes == NULL) {
		return (ISC_R_NOPERM);
	}

	MAYBE_LOCK(cd);
	result = cd->dlz_allnodes(zone, cd->dbdata, allnodes);
	MAYBE_UNLOCK(cd);
	return (result);
}

static isc_result_t
dlopen_dlz_allowzonexfr(void *driverarg, void *dbdata, const char *name,
			const char *client) {
	dlopen_data_t *cd = (dlopen_data_t *)dbdata;
	isc_result_t result;

	UNUSED(driverarg);

	if (cd->dlz_allowzonexfr == NULL) {
		return (ISC_R_NOPERM);
	}

	MAYBE_LOCK(cd);
	result = cd->dlz_allowzonexfr(cd->dbdata, name, client);
	MAYBE_UNLOCK(cd);
	return (result);
}

static isc_result_t
dlopen_dlz_authority(const char *zone, void *driverarg, void *dbdata,
		     dns_sdlzlookup_t *lookup) {
	dlopen_data_t *cd = (dlopen_data_t *)dbdata;
	isc_result_t result;

	UNUSED(driverarg);

	if (cd->dlz_authority == NULL) {
		return (ISC_R_NOTIMPLEMENTED);
	}

	MAYBE_LOCK(cd);
	result = cd->dlz_authority(zone, cd->dbdata, lookup);
	MAYBE_UNLOCK(cd);
	return (result);
}

static isc_result_t
dlopen_dlz_findzonedb(void *driverarg, void *dbdata, const char *name,
		      dns_clientinfomethods_t *methods,
		      dns_clientinfo_t *clientinfo) {
	dlopen_data_t *cd = (dlopen_data_t *)dbdata;
	isc_result_t result;

	UNUSED(driverarg);

	MAYBE_LOCK(cd);
	result = cd->dlz_findzonedb(cd->dbdata, name, methods, clientinfo);
	MAYBE_UNLOCK(cd);
	return (result);
}

static isc_result_t
dlopen_dlz_lookup(const char *zone, const char *name, void *driverarg,
		  void *dbdata, dns_sdlzlookup_t *lookup,
		  dns_clientinfomethods_t *methods,
		  dns_clientinfo_t *clientinfo) {
	dlopen_data_t *cd = (dlopen_data_t *)dbdata;
	isc_result_t result;

	UNUSED(driverarg);

	MAYBE_LOCK(cd);
	result = cd->dlz_lookup(zone, name, cd->dbdata, lookup, methods,
				clientinfo);
	MAYBE_UNLOCK(cd);
	return (result);
}

/*
 * Load a symbol from the library
 */
static void *
dl_load_symbol(dlopen_data_t *cd, const char *symbol, bool mandatory) {
	void *ptr = NULL;
	int r = uv_dlsym(&cd->dl_handle, symbol, &ptr);
	if (r != 0) {
		const char *errmsg = uv_dlerror(&cd->dl_handle);
		if (errmsg == NULL) {
			errmsg = "returned function pointer is NULL";
		}
		if (mandatory) {
			dlopen_log(ISC_LOG_ERROR,
				   "dlz_dlopen: library '%s' is missing "
				   "required symbol '%s': %s",
				   cd->dl_path, symbol, errmsg);
		}
	}

	return (ptr);
}

static void
dlopen_dlz_destroy(void *driverarg, void *dbdata);

/*
 * Called at startup for each dlopen zone in named.conf
 */
static isc_result_t
dlopen_dlz_create(const char *dlzname, unsigned int argc, char *argv[],
		  void *driverarg, void **dbdata) {
	dlopen_data_t *cd;
	isc_mem_t *mctx = NULL;
	isc_result_t result = ISC_R_FAILURE;
	int r;

	UNUSED(driverarg);

	if (argc < 2) {
		dlopen_log(ISC_LOG_ERROR,
			   "dlz_dlopen driver for '%s' needs a path to "
			   "the shared library",
			   dlzname);
		return (ISC_R_FAILURE);
	}

	isc_mem_create(&mctx);
	cd = isc_mem_get(mctx, sizeof(*cd));
	memset(cd, 0, sizeof(*cd));

	cd->mctx = mctx;

	cd->dl_path = isc_mem_strdup(cd->mctx, argv[1]);
	cd->dlzname = isc_mem_strdup(cd->mctx, dlzname);

	/* Initialize the lock */
	isc_mutex_init(&cd->lock);

	r = uv_dlopen(cd->dl_path, &cd->dl_handle);
	if (r != 0) {
		const char *errmsg = uv_dlerror(&cd->dl_handle);
		if (errmsg == NULL) {
			errmsg = "unknown error";
		}
		dlopen_log(ISC_LOG_ERROR,
			   "dlz_dlopen failed to open library '%s': %s",
			   cd->dl_path, errmsg);
		result = ISC_R_FAILURE;
		goto failed;
	}

	/* Find the symbols */
	cd->dlz_version =
		(dlz_dlopen_version_t *)dl_load_symbol(cd, "dlz_version", true);
	cd->dlz_create = (dlz_dlopen_create_t *)dl_load_symbol(cd, "dlz_create",
							       true);
	cd->dlz_lookup = (dlz_dlopen_lookup_t *)dl_load_symbol(cd, "dlz_lookup",
							       true);
	cd->dlz_findzonedb = (dlz_dlopen_findzonedb_t *)dl_load_symbol(
		cd, "dlz_findzonedb", true);

	if (cd->dlz_create == NULL || cd->dlz_version == NULL ||
	    cd->dlz_lookup == NULL || cd->dlz_findzonedb == NULL)
	{
		/* We're missing a required symbol */
		result = ISC_R_FAILURE;
		goto failed;
	}

	cd->dlz_allowzonexfr = (dlz_dlopen_allowzonexfr_t *)dl_load_symbol(
		cd, "dlz_allowzonexfr", false);
	cd->dlz_allnodes = (dlz_dlopen_allnodes_t *)dl_load_symbol(
		cd, "dlz_allnodes", (cd->dlz_allowzonexfr != NULL));
	cd->dlz_authority = (dlz_dlopen_authority_t *)dl_load_symbol(
		cd, "dlz_authority", false);
	cd->dlz_newversion = (dlz_dlopen_newversion_t *)dl_load_symbol(
		cd, "dlz_newversion", false);
	cd->dlz_closeversion = (dlz_dlopen_closeversion_t *)dl_load_symbol(
		cd, "dlz_closeversion", (cd->dlz_newversion != NULL));
	cd->dlz_configure = (dlz_dlopen_configure_t *)dl_load_symbol(
		cd, "dlz_configure", false);
	cd->dlz_ssumatch = (dlz_dlopen_ssumatch_t *)dl_load_symbol(
		cd, "dlz_ssumatch", false);
	cd->dlz_addrdataset = (dlz_dlopen_addrdataset_t *)dl_load_symbol(
		cd, "dlz_addrdataset", false);
	cd->dlz_subrdataset = (dlz_dlopen_subrdataset_t *)dl_load_symbol(
		cd, "dlz_subrdataset", false);
	cd->dlz_delrdataset = (dlz_dlopen_delrdataset_t *)dl_load_symbol(
		cd, "dlz_delrdataset", false);
	cd->dlz_destroy = (dlz_dlopen_destroy_t *)dl_load_symbol(
		cd, "dlz_destroy", false);

	/* Check the version of the API is the same */
	cd->version = cd->dlz_version(&cd->flags);
	if (cd->version < (DLZ_DLOPEN_VERSION - DLZ_DLOPEN_AGE) ||
	    cd->version > DLZ_DLOPEN_VERSION)
	{
		dlopen_log(ISC_LOG_ERROR,
			   "dlz_dlopen: %s: incorrect driver API version %d, "
			   "requires %d",
			   cd->dl_path, cd->version, DLZ_DLOPEN_VERSION);
		result = ISC_R_FAILURE;
		goto failed;
	}

	/*
	 * Call the library's create function. Note that this is an
	 * extended version of dlz create, with the addition of
	 * named function pointers for helper functions that the
	 * driver will need. This avoids the need for the backend to
	 * link the BIND9 libraries
	 */
	MAYBE_LOCK(cd);
	result = cd->dlz_create(dlzname, argc - 1, argv + 1, &cd->dbdata, "log",
				dlopen_log, "putrr", dns_sdlz_putrr,
				"putnamedrr", dns_sdlz_putnamedrr,
				"writeable_zone", dns_dlz_writeablezone, NULL);
	MAYBE_UNLOCK(cd);
	if (result != ISC_R_SUCCESS) {
		goto failed;
	}

	*dbdata = cd;

	return (ISC_R_SUCCESS);

failed:
	dlopen_log(ISC_LOG_ERROR, "dlz_dlopen of '%s' failed", dlzname);

	dlopen_dlz_destroy(NULL, cd);

	return (result);
}

/*
 * Called when bind is shutting down
 */
static void
dlopen_dlz_destroy(void *driverarg, void *dbdata) {
	dlopen_data_t *cd = (dlopen_data_t *)dbdata;

	UNUSED(driverarg);

	if (cd->dlz_destroy && cd->dbdata) {
		MAYBE_LOCK(cd);
		cd->dlz_destroy(cd->dbdata);
		MAYBE_UNLOCK(cd);
	}

	uv_dlclose(&cd->dl_handle);
	isc_mutex_destroy(&cd->lock);
	isc_mem_free(cd->mctx, cd->dl_path);
	isc_mem_free(cd->mctx, cd->dlzname);
	isc_mem_putanddetach(&cd->mctx, cd, sizeof(*cd));
}

/*
 * Called to start a transaction
 */
static isc_result_t
dlopen_dlz_newversion(const char *zone, void *driverarg, void *dbdata,
		      void **versionp) {
	dlopen_data_t *cd = (dlopen_data_t *)dbdata;
	isc_result_t result;

	UNUSED(driverarg);

	if (cd->dlz_newversion == NULL) {
		return (ISC_R_NOTIMPLEMENTED);
	}

	MAYBE_LOCK(cd);
	result = cd->dlz_newversion(zone, cd->dbdata, versionp);
	MAYBE_UNLOCK(cd);
	return (result);
}

/*
 * Called to end a transaction
 */
static void
dlopen_dlz_closeversion(const char *zone, bool commit, void *driverarg,
			void *dbdata, void **versionp) {
	dlopen_data_t *cd = (dlopen_data_t *)dbdata;

	UNUSED(driverarg);

	if (cd->dlz_newversion == NULL) {
		*versionp = NULL;
		return;
	}

	MAYBE_LOCK(cd);
	cd->dlz_closeversion(zone, commit, cd->dbdata, versionp);
	MAYBE_UNLOCK(cd);
}

/*
 * Called on startup to configure any writeable zones
 */
static isc_result_t
dlopen_dlz_configure(dns_view_t *view, dns_dlzdb_t *dlzdb, void *driverarg,
		     void *dbdata) {
	dlopen_data_t *cd = (dlopen_data_t *)dbdata;
	isc_result_t result;

	UNUSED(driverarg);

	if (cd->dlz_configure == NULL) {
		return (ISC_R_SUCCESS);
	}

	MAYBE_LOCK(cd);
	cd->in_configure = true;
	result = cd->dlz_configure(view, dlzdb, cd->dbdata);
	cd->in_configure = false;
	MAYBE_UNLOCK(cd);

	return (result);
}

/*
 * Check for authority to change a name.
 */
static bool
dlopen_dlz_ssumatch(const char *signer, const char *name, const char *tcpaddr,
		    const char *type, const char *key, uint32_t keydatalen,
		    unsigned char *keydata, void *driverarg, void *dbdata) {
	dlopen_data_t *cd = (dlopen_data_t *)dbdata;
	bool ret;

	UNUSED(driverarg);

	if (cd->dlz_ssumatch == NULL) {
		return (false);
	}

	MAYBE_LOCK(cd);
	ret = cd->dlz_ssumatch(signer, name, tcpaddr, type, key, keydatalen,
			       keydata, cd->dbdata);
	MAYBE_UNLOCK(cd);

	return (ret);
}

/*
 * Add an rdataset.
 */
static isc_result_t
dlopen_dlz_addrdataset(const char *name, const char *rdatastr, void *driverarg,
		       void *dbdata, void *version) {
	dlopen_data_t *cd = (dlopen_data_t *)dbdata;
	isc_result_t result;

	UNUSED(driverarg);

	if (cd->dlz_addrdataset == NULL) {
		return (ISC_R_NOTIMPLEMENTED);
	}

	MAYBE_LOCK(cd);
	result = cd->dlz_addrdataset(name, rdatastr, cd->dbdata, version);
	MAYBE_UNLOCK(cd);

	return (result);
}

/*
 * Subtract an rdataset.
 */
static isc_result_t
dlopen_dlz_subrdataset(const char *name, const char *rdatastr, void *driverarg,
		       void *dbdata, void *version) {
	dlopen_data_t *cd = (dlopen_data_t *)dbdata;
	isc_result_t result;

	UNUSED(driverarg);

	if (cd->dlz_subrdataset == NULL) {
		return (ISC_R_NOTIMPLEMENTED);
	}

	MAYBE_LOCK(cd);
	result = cd->dlz_subrdataset(name, rdatastr, cd->dbdata, version);
	MAYBE_UNLOCK(cd);

	return (result);
}

/*
 * Delete a rdataset.
 */
static isc_result_t
dlopen_dlz_delrdataset(const char *name, const char *type, void *driverarg,
		       void *dbdata, void *version) {
	dlopen_data_t *cd = (dlopen_data_t *)dbdata;
	isc_result_t result;

	UNUSED(driverarg);

	if (cd->dlz_delrdataset == NULL) {
		return (ISC_R_NOTIMPLEMENTED);
	}

	MAYBE_LOCK(cd);
	result = cd->dlz_delrdataset(name, type, cd->dbdata, version);
	MAYBE_UNLOCK(cd);

	return (result);
}

static dns_sdlzmethods_t dlz_dlopen_methods = {
	dlopen_dlz_create,	 dlopen_dlz_destroy,	dlopen_dlz_findzonedb,
	dlopen_dlz_lookup,	 dlopen_dlz_authority,	dlopen_dlz_allnodes,
	dlopen_dlz_allowzonexfr, dlopen_dlz_newversion, dlopen_dlz_closeversion,
	dlopen_dlz_configure,	 dlopen_dlz_ssumatch,	dlopen_dlz_addrdataset,
	dlopen_dlz_subrdataset,	 dlopen_dlz_delrdataset
};

/*
 * Register driver with BIND
 */
isc_result_t
dlz_dlopen_init(isc_mem_t *mctx) {
	isc_result_t result;

	dlopen_log(2, "Registering DLZ_dlopen driver");

	result = dns_sdlzregister("dlopen", &dlz_dlopen_methods, NULL,
				  DNS_SDLZFLAG_RELATIVEOWNER |
					  DNS_SDLZFLAG_RELATIVERDATA |
					  DNS_SDLZFLAG_THREADSAFE,
				  mctx, &dlz_dlopen);

	if (result != ISC_R_SUCCESS) {
		UNEXPECTED_ERROR("dns_sdlzregister() failed: %s",
				 isc_result_totext(result));
		result = ISC_R_UNEXPECTED;
	}

	return (result);
}

/*
 * Unregister the driver
 */
void
dlz_dlopen_clear(void) {
	dlopen_log(2, "Unregistering DLZ_dlopen driver");
	if (dlz_dlopen != NULL) {
		dns_sdlzunregister(&dlz_dlopen);
	}
}
