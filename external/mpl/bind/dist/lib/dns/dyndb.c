/*	$NetBSD: dyndb.c,v 1.1.1.1 2018/08/12 12:08:14 christos Exp $	*/

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


#include <config.h>

#if HAVE_DLFCN_H
#include <dlfcn.h>
#elif _WIN32
#include <windows.h>
#endif

#include <isc/buffer.h>
#include <isc/mem.h>
#include <isc/mutex.h>
#include <isc/once.h>
#include <isc/result.h>
#include <isc/region.h>
#include <isc/task.h>
#include <isc/types.h>
#include <isc/util.h>

#include <dns/dyndb.h>
#include <dns/log.h>
#include <dns/types.h>
#include <dns/view.h>
#include <dns/zone.h>

#include <string.h>

#define CHECK(op)						\
	do { result = (op);					\
		if (result != ISC_R_SUCCESS) goto cleanup;	\
	} while (0)


typedef struct dyndb_implementation dyndb_implementation_t;
struct dyndb_implementation {
	isc_mem_t			*mctx;
	void				*handle;
	dns_dyndb_register_t		*register_func;
	dns_dyndb_destroy_t		*destroy_func;
	char				*name;
	void				*inst;
	LINK(dyndb_implementation_t)	link;
};

/*
 * List of dyndb implementations. Locked by dyndb_lock.
 *
 * These are stored here so they can be cleaned up on shutdown.
 * (The order in which they are stored is not important.)
 */
static LIST(dyndb_implementation_t) dyndb_implementations;

/* Locks dyndb_implementations. */
static isc_mutex_t dyndb_lock;
static isc_once_t once = ISC_ONCE_INIT;

static void
dyndb_initialize(void) {
	RUNTIME_CHECK(isc_mutex_init(&dyndb_lock) == ISC_R_SUCCESS);
	INIT_LIST(dyndb_implementations);
}

static dyndb_implementation_t *
impfind(const char *name) {
	dyndb_implementation_t *imp;

	for (imp = ISC_LIST_HEAD(dyndb_implementations);
	     imp != NULL;
	     imp = ISC_LIST_NEXT(imp, link))
		if (strcasecmp(name, imp->name) == 0)
			return (imp);
	return (NULL);
}

#if HAVE_DLFCN_H && HAVE_DLOPEN
static isc_result_t
load_symbol(void *handle, const char *filename,
	    const char *symbol_name, void **symbolp)
{
	const char *errmsg;
	void *symbol;

	REQUIRE(handle != NULL);
	REQUIRE(symbolp != NULL && *symbolp == NULL);

	symbol = dlsym(handle, symbol_name);
	if (symbol == NULL) {
		errmsg = dlerror();
		if (errmsg == NULL)
			errmsg = "returned function pointer is NULL";
		isc_log_write(dns_lctx, DNS_LOGCATEGORY_DATABASE,
			      DNS_LOGMODULE_DYNDB, ISC_LOG_ERROR,
			      "failed to lookup symbol %s in "
			      "dyndb module '%s': %s",
			      symbol_name, filename, errmsg);
		return (ISC_R_FAILURE);
	}
	dlerror();

	*symbolp = symbol;

	return (ISC_R_SUCCESS);
}

static isc_result_t
load_library(isc_mem_t *mctx, const char *filename, const char *instname,
	     dyndb_implementation_t **impp)
{
	isc_result_t result;
	void *handle = NULL;
	dyndb_implementation_t *imp = NULL;
	dns_dyndb_register_t *register_func = NULL;
	dns_dyndb_destroy_t *destroy_func = NULL;
	dns_dyndb_version_t *version_func = NULL;
	int version, flags;

	REQUIRE(impp != NULL && *impp == NULL);

	isc_log_write(dns_lctx, DNS_LOGCATEGORY_DATABASE,
		      DNS_LOGMODULE_DYNDB, ISC_LOG_INFO,
		      "loading DynDB instance '%s' driver '%s'",
		      instname, filename);

	flags = RTLD_NOW|RTLD_LOCAL;
#ifdef RTLD_DEEPBIND
	flags |= RTLD_DEEPBIND;
#endif

	handle = dlopen(filename, flags);
	if (handle == NULL)
		CHECK(ISC_R_FAILURE);

	/* Clear dlerror */
	dlerror();

	CHECK(load_symbol(handle, filename, "dyndb_version",
			  (void **)&version_func));

	version = version_func(NULL);
	if (version < (DNS_DYNDB_VERSION - DNS_DYNDB_AGE) ||
	    version > DNS_DYNDB_VERSION)
	{
		isc_log_write(dns_lctx, DNS_LOGCATEGORY_DATABASE,
			      DNS_LOGMODULE_DYNDB, ISC_LOG_ERROR,
			      "driver API version mismatch: %d/%d",
			      version, DNS_DYNDB_VERSION);
		CHECK(ISC_R_FAILURE);
	}

	CHECK(load_symbol(handle, filename, "dyndb_init",
			  (void **)&register_func));
	CHECK(load_symbol(handle, filename, "dyndb_destroy",
			  (void **)&destroy_func));

	imp = isc_mem_get(mctx, sizeof(dyndb_implementation_t));
	if (imp == NULL)
		CHECK(ISC_R_NOMEMORY);

	imp->mctx = NULL;
	isc_mem_attach(mctx, &imp->mctx);
	imp->handle = handle;
	imp->register_func = register_func;
	imp->destroy_func = destroy_func;
	imp->name = isc_mem_strdup(mctx, instname);
	if (imp->name == NULL)
		CHECK(ISC_R_NOMEMORY);

	imp->inst = NULL;
	INIT_LINK(imp, link);

	*impp = imp;
	imp = NULL;

cleanup:
	if (result != ISC_R_SUCCESS)
		isc_log_write(dns_lctx, DNS_LOGCATEGORY_DATABASE,
			      DNS_LOGMODULE_DYNDB, ISC_LOG_ERROR,
			      "failed to dynamically load instance '%s' "
			      "driver '%s': %s (%s)", instname, filename,
			      dlerror(), isc_result_totext(result));
	if (imp != NULL)
		isc_mem_putanddetach(&imp->mctx, imp, sizeof(dyndb_implementation_t));
	if (result != ISC_R_SUCCESS && handle != NULL)
		dlclose(handle);

	return (result);
}

static void
unload_library(dyndb_implementation_t **impp) {
	dyndb_implementation_t *imp;

	REQUIRE(impp != NULL && *impp != NULL);

	imp = *impp;

	isc_mem_free(imp->mctx, imp->name);
	isc_mem_putanddetach(&imp->mctx, imp, sizeof(dyndb_implementation_t));

	*impp = NULL;
}
#elif _WIN32
static isc_result_t
load_symbol(HMODULE handle, const char *filename,
	    const char *symbol_name, void **symbolp)
{
	void *symbol;

	REQUIRE(handle != NULL);
	REQUIRE(symbolp != NULL && *symbolp == NULL);

	symbol = GetProcAddress(handle, symbol_name);
	if (symbol == NULL) {
		int errstatus = GetLastError();
		isc_log_write(dns_lctx, DNS_LOGCATEGORY_DATABASE,
			      DNS_LOGMODULE_DYNDB, ISC_LOG_ERROR,
			      "failed to lookup symbol %s in "
			      "dyndb module '%s': %d",
			      symbol_name, filename, errstatus);
		return (ISC_R_FAILURE);
	}

	*symbolp = symbol;

	return (ISC_R_SUCCESS);
}

static isc_result_t
load_library(isc_mem_t *mctx, const char *filename, const char *instname,
	     dyndb_implementation_t **impp)
{
	isc_result_t result;
	HMODULE handle;
	dyndb_implementation_t *imp = NULL;
	dns_dyndb_register_t *register_func = NULL;
	dns_dyndb_destroy_t *destroy_func = NULL;
	dns_dyndb_version_t *version_func = NULL;
	int version;

	REQUIRE(impp != NULL && *impp == NULL);

	isc_log_write(dns_lctx, DNS_LOGCATEGORY_DATABASE,
		      DNS_LOGMODULE_DYNDB, ISC_LOG_INFO,
		      "loading DynDB instance '%s' driver '%s'",
		      instname, filename);

	handle = LoadLibraryA(filename);
	if (handle == NULL)
		CHECK(ISC_R_FAILURE);

	CHECK(load_symbol(handle, filename, "dyndb_version",
			  (void **)&version_func));

	version = version_func(NULL);
	if (version < (DNS_DYNDB_VERSION - DNS_DYNDB_AGE) ||
	    version > DNS_DYNDB_VERSION)
	{
		isc_log_write(dns_lctx, DNS_LOGCATEGORY_DATABASE,
			      DNS_LOGMODULE_DYNDB, ISC_LOG_ERROR,
			      "driver API version mismatch: %d/%d",
			      version, DNS_DYNDB_VERSION);
		CHECK(ISC_R_FAILURE);
	}

	CHECK(load_symbol(handle, filename, "dyndb_init",
			  (void **)&register_func));
	CHECK(load_symbol(handle, filename, "dyndb_destroy",
			  (void **)&destroy_func));

	imp = isc_mem_get(mctx, sizeof(dyndb_implementation_t));
	if (imp == NULL)
		CHECK(ISC_R_NOMEMORY);

	imp->mctx = NULL;
	isc_mem_attach(mctx, &imp->mctx);
	imp->handle = handle;
	imp->register_func = register_func;
	imp->destroy_func = destroy_func;
	imp->name = isc_mem_strdup(mctx, instname);
	if (imp->name == NULL)
		CHECK(ISC_R_NOMEMORY);

	imp->inst = NULL;
	INIT_LINK(imp, link);

	*impp = imp;
	imp = NULL;

cleanup:
	if (result != ISC_R_SUCCESS)
		isc_log_write(dns_lctx, DNS_LOGCATEGORY_DATABASE,
			      DNS_LOGMODULE_DYNDB, ISC_LOG_ERROR,
			      "failed to dynamically load instance '%s' "
			      "driver '%s': %d (%s)", instname, filename,
			      GetLastError(), isc_result_totext(result));
	if (imp != NULL)
		isc_mem_putanddetach(&imp->mctx, imp, sizeof(dyndb_implementation_t));
	if (result != ISC_R_SUCCESS && handle != NULL)
		FreeLibrary(handle);

	return (result);
}

static void
unload_library(dyndb_implementation_t **impp) {
	dyndb_implementation_t *imp;

	REQUIRE(impp != NULL && *impp != NULL);

	imp = *impp;

	isc_mem_free(imp->mctx, imp->name);
	isc_mem_putanddetach(&imp->mctx, imp, sizeof(dyndb_implementation_t));

	*impp = NULL;
}
#else	/* HAVE_DLFCN_H || _WIN32 */
static isc_result_t
load_library(isc_mem_t *mctx, const char *filename, const char *instname,
	     dyndb_implementation_t **impp)
{
	UNUSED(mctx);
	UNUSED(filename);
	UNUSED(instname);
	UNUSED(impp);

	isc_log_write(dns_lctx, DNS_LOGCATEGORY_DATABASE, DNS_LOGMODULE_DYNDB,
		      ISC_LOG_ERROR,
		      "dynamic database support is not implemented");

	return (ISC_R_NOTIMPLEMENTED);
}

static void
unload_library(dyndb_implementation_t **impp)
{
	UNUSED(impp);
}
#endif	/* HAVE_DLFCN_H */

isc_result_t
dns_dyndb_load(const char *libname, const char *name, const char *parameters,
	       const char *file, unsigned long line, isc_mem_t *mctx,
	       const dns_dyndbctx_t *dctx)
{
	isc_result_t result;
	dyndb_implementation_t *implementation = NULL;

	REQUIRE(DNS_DYNDBCTX_VALID(dctx));
	REQUIRE(name != NULL);

	RUNTIME_CHECK(isc_once_do(&once, dyndb_initialize) == ISC_R_SUCCESS);

	LOCK(&dyndb_lock);

	/* duplicate instance names are not allowed */
	if (impfind(name) != NULL)
		CHECK(ISC_R_EXISTS);

	CHECK(load_library(mctx, libname, name, &implementation));
	CHECK(implementation->register_func(mctx, name, parameters, file, line,
					    dctx, &implementation->inst));

	APPEND(dyndb_implementations, implementation, link);
	result = ISC_R_SUCCESS;

cleanup:
	if (result != ISC_R_SUCCESS)
		if (implementation != NULL)
			unload_library(&implementation);

	UNLOCK(&dyndb_lock);
	return (result);
}

void
dns_dyndb_cleanup(isc_boolean_t exiting) {
	dyndb_implementation_t *elem;
	dyndb_implementation_t *prev;

	RUNTIME_CHECK(isc_once_do(&once, dyndb_initialize) == ISC_R_SUCCESS);

	LOCK(&dyndb_lock);
	elem = TAIL(dyndb_implementations);
	while (elem != NULL) {
		prev = PREV(elem, link);
		UNLINK(dyndb_implementations, elem, link);
		isc_log_write(dns_lctx, DNS_LOGCATEGORY_DATABASE,
			      DNS_LOGMODULE_DYNDB, ISC_LOG_INFO,
			      "unloading DynDB instance '%s'", elem->name);
		elem->destroy_func(&elem->inst);
		ENSURE(elem->inst == NULL);
		unload_library(&elem);
		elem = prev;
	}
	UNLOCK(&dyndb_lock);

	if (exiting == ISC_TRUE)
		isc_mutex_destroy(&dyndb_lock);
}

isc_result_t
dns_dyndb_createctx(isc_mem_t *mctx, const void *hashinit, isc_log_t *lctx,
		    dns_view_t *view, dns_zonemgr_t *zmgr, isc_task_t *task,
		    isc_timermgr_t *tmgr, dns_dyndbctx_t **dctxp)
{
	dns_dyndbctx_t *dctx;

	REQUIRE(dctxp != NULL && *dctxp == NULL);

	dctx = isc_mem_get(mctx, sizeof(*dctx));
	if (dctx == NULL)
		return (ISC_R_NOMEMORY);

	memset(dctx, 0, sizeof(*dctx));
	if (view != NULL)
		dns_view_attach(view, &dctx->view);
	if (zmgr != NULL)
		dns_zonemgr_attach(zmgr, &dctx->zmgr);
	if (task != NULL)
		isc_task_attach(task, &dctx->task);
	dctx->timermgr = tmgr;
	dctx->hashinit = hashinit;
	dctx->lctx = lctx;
	dctx->refvar = &isc_bind9;

	isc_mem_attach(mctx, &dctx->mctx);
	dctx->magic = DNS_DYNDBCTX_MAGIC;

	*dctxp = dctx;

	return (ISC_R_SUCCESS);
}

void
dns_dyndb_destroyctx(dns_dyndbctx_t **dctxp) {
	dns_dyndbctx_t *dctx;

	REQUIRE(dctxp != NULL && DNS_DYNDBCTX_VALID(*dctxp));

	dctx = *dctxp;
	*dctxp = NULL;

	dctx->magic = 0;

	if (dctx->view != NULL)
		dns_view_detach(&dctx->view);
	if (dctx->zmgr != NULL)
		dns_zonemgr_detach(&dctx->zmgr);
	if (dctx->task != NULL)
		isc_task_detach(&dctx->task);
	dctx->timermgr = NULL;
	dctx->lctx = NULL;

	isc_mem_putanddetach(&dctx->mctx, dctx, sizeof(*dctx));
}
