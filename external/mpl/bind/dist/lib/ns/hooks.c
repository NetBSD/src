/*	$NetBSD: hooks.c,v 1.9 2023/01/25 21:43:32 christos Exp $	*/

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

#include <errno.h>
#include <stdio.h>
#include <string.h>

#if HAVE_DLFCN_H
#include <dlfcn.h>
#elif _WIN32
#include <windows.h>
#endif /* if HAVE_DLFCN_H */

#include <isc/errno.h>
#include <isc/list.h>
#include <isc/log.h>
#include <isc/mem.h>
#include <isc/mutex.h>
#include <isc/platform.h>
#include <isc/print.h>
#include <isc/result.h>
#include <isc/types.h>
#include <isc/util.h>

#include <dns/view.h>

#include <ns/hooks.h>
#include <ns/log.h>
#include <ns/query.h>

#define CHECK(op)                              \
	do {                                   \
		result = (op);                 \
		if (result != ISC_R_SUCCESS) { \
			goto cleanup;          \
		}                              \
	} while (0)

struct ns_plugin {
	isc_mem_t *mctx;
	void *handle;
	void *inst;
	char *modpath;
	ns_plugin_check_t *check_func;
	ns_plugin_register_t *register_func;
	ns_plugin_destroy_t *destroy_func;
	LINK(ns_plugin_t) link;
};

static ns_hooklist_t default_hooktable[NS_HOOKPOINTS_COUNT];
LIBNS_EXTERNAL_DATA ns_hooktable_t *ns__hook_table = &default_hooktable;

isc_result_t
ns_plugin_expandpath(const char *src, char *dst, size_t dstsize) {
	int result;

#ifndef WIN32
	/*
	 * On Unix systems, differentiate between paths and filenames.
	 */
	if (strchr(src, '/') != NULL) {
		/*
		 * 'src' is an absolute or relative path.  Copy it verbatim.
		 */
		result = snprintf(dst, dstsize, "%s", src);
	} else {
		/*
		 * 'src' is a filename.  Prepend default plugin directory path.
		 */
		result = snprintf(dst, dstsize, "%s/%s", NAMED_PLUGINDIR, src);
	}
#else  /* ifndef WIN32 */
	/*
	 * On Windows, always copy 'src' do 'dst'.
	 */
	result = snprintf(dst, dstsize, "%s", src);
#endif /* ifndef WIN32 */

	if (result < 0) {
		return (isc_errno_toresult(errno));
	} else if ((size_t)result >= dstsize) {
		return (ISC_R_NOSPACE);
	} else {
		return (ISC_R_SUCCESS);
	}
}

#if HAVE_DLFCN_H && HAVE_DLOPEN
static isc_result_t
load_symbol(void *handle, const char *modpath, const char *symbol_name,
	    void **symbolp) {
	void *symbol = NULL;

	REQUIRE(handle != NULL);
	REQUIRE(symbolp != NULL && *symbolp == NULL);

	/*
	 * Clear any pre-existing error conditions before running dlsym().
	 * (In this case, we expect dlsym() to return non-NULL values
	 * and will always return an error if it returns NULL, but
	 * this ensures that we'll report the correct error condition
	 * if there is one.)
	 */
	dlerror();
	symbol = dlsym(handle, symbol_name);
	if (symbol == NULL) {
		const char *errmsg = dlerror();
		if (errmsg == NULL) {
			errmsg = "returned function pointer is NULL";
		}
		isc_log_write(ns_lctx, NS_LOGCATEGORY_GENERAL,
			      NS_LOGMODULE_HOOKS, ISC_LOG_ERROR,
			      "failed to look up symbol %s in "
			      "plugin '%s': %s",
			      symbol_name, modpath, errmsg);
		return (ISC_R_FAILURE);
	}

	*symbolp = symbol;

	return (ISC_R_SUCCESS);
}

static isc_result_t
load_plugin(isc_mem_t *mctx, const char *modpath, ns_plugin_t **pluginp) {
	isc_result_t result;
	void *handle = NULL;
	ns_plugin_t *plugin = NULL;
	ns_plugin_check_t *check_func = NULL;
	ns_plugin_register_t *register_func = NULL;
	ns_plugin_destroy_t *destroy_func = NULL;
	ns_plugin_version_t *version_func = NULL;
	int version, flags;

	REQUIRE(pluginp != NULL && *pluginp == NULL);

	flags = RTLD_LAZY | RTLD_LOCAL;
#if defined(RTLD_DEEPBIND) && !__SANITIZE_ADDRESS__ && !__SANITIZE_THREAD__
	flags |= RTLD_DEEPBIND;
#endif /* if defined(RTLD_DEEPBIND) && !__SANITIZE_ADDRESS__ && \
	  !__SANITIZE_THREAD__ */

	handle = dlopen(modpath, flags);
	if (handle == NULL) {
		const char *errmsg = dlerror();
		if (errmsg == NULL) {
			errmsg = "unknown error";
		}
		isc_log_write(ns_lctx, NS_LOGCATEGORY_GENERAL,
			      NS_LOGMODULE_HOOKS, ISC_LOG_ERROR,
			      "failed to dlopen() plugin '%s': %s", modpath,
			      errmsg);
		return (ISC_R_FAILURE);
	}

	CHECK(load_symbol(handle, modpath, "plugin_version",
			  (void **)&version_func));

	version = version_func();
	if (version < (NS_PLUGIN_VERSION - NS_PLUGIN_AGE) ||
	    version > NS_PLUGIN_VERSION)
	{
		isc_log_write(ns_lctx, NS_LOGCATEGORY_GENERAL,
			      NS_LOGMODULE_HOOKS, ISC_LOG_ERROR,
			      "plugin API version mismatch: %d/%d", version,
			      NS_PLUGIN_VERSION);
		CHECK(ISC_R_FAILURE);
	}

	CHECK(load_symbol(handle, modpath, "plugin_check",
			  (void **)&check_func));
	CHECK(load_symbol(handle, modpath, "plugin_register",
			  (void **)&register_func));
	CHECK(load_symbol(handle, modpath, "plugin_destroy",
			  (void **)&destroy_func));

	plugin = isc_mem_get(mctx, sizeof(*plugin));
	memset(plugin, 0, sizeof(*plugin));
	isc_mem_attach(mctx, &plugin->mctx);
	plugin->handle = handle;
	plugin->modpath = isc_mem_strdup(plugin->mctx, modpath);
	plugin->check_func = check_func;
	plugin->register_func = register_func;
	plugin->destroy_func = destroy_func;

	ISC_LINK_INIT(plugin, link);

	*pluginp = plugin;
	plugin = NULL;

cleanup:
	if (result != ISC_R_SUCCESS) {
		isc_log_write(ns_lctx, NS_LOGCATEGORY_GENERAL,
			      NS_LOGMODULE_HOOKS, ISC_LOG_ERROR,
			      "failed to dynamically load "
			      "plugin '%s': %s",
			      modpath, isc_result_totext(result));

		if (plugin != NULL) {
			isc_mem_putanddetach(&plugin->mctx, plugin,
					     sizeof(*plugin));
		}

		(void)dlclose(handle);
	}

	return (result);
}

static void
unload_plugin(ns_plugin_t **pluginp) {
	ns_plugin_t *plugin = NULL;

	REQUIRE(pluginp != NULL && *pluginp != NULL);

	plugin = *pluginp;
	*pluginp = NULL;

	isc_log_write(ns_lctx, NS_LOGCATEGORY_GENERAL, NS_LOGMODULE_HOOKS,
		      ISC_LOG_DEBUG(1), "unloading plugin '%s'",
		      plugin->modpath);

	if (plugin->inst != NULL) {
		plugin->destroy_func(&plugin->inst);
	}
	if (plugin->handle != NULL) {
		(void)dlclose(plugin->handle);
	}
	if (plugin->modpath != NULL) {
		isc_mem_free(plugin->mctx, plugin->modpath);
	}

	isc_mem_putanddetach(&plugin->mctx, plugin, sizeof(*plugin));
}
#elif _WIN32
static isc_result_t
load_symbol(HMODULE handle, const char *modpath, const char *symbol_name,
	    void **symbolp) {
	void *symbol = NULL;

	REQUIRE(handle != NULL);
	REQUIRE(symbolp != NULL && *symbolp == NULL);

	symbol = GetProcAddress(handle, symbol_name);
	if (symbol == NULL) {
		int errstatus = GetLastError();
		isc_log_write(ns_lctx, NS_LOGCATEGORY_GENERAL,
			      NS_LOGMODULE_HOOKS, ISC_LOG_ERROR,
			      "failed to look up symbol %s in "
			      "plugin '%s': %d",
			      symbol_name, modpath, errstatus);
		return (ISC_R_FAILURE);
	}

	*symbolp = symbol;

	return (ISC_R_SUCCESS);
}

static isc_result_t
load_plugin(isc_mem_t *mctx, const char *modpath, ns_plugin_t **pluginp) {
	isc_result_t result;
	HMODULE handle;
	ns_plugin_t *plugin = NULL;
	ns_plugin_register_t *register_func = NULL;
	ns_plugin_destroy_t *destroy_func = NULL;
	ns_plugin_version_t *version_func = NULL;
	int version;

	REQUIRE(pluginp != NULL && *pluginp == NULL);

	handle = LoadLibraryA(modpath);
	if (handle == NULL) {
		CHECK(ISC_R_FAILURE);
	}

	CHECK(load_symbol(handle, modpath, "plugin_version",
			  (void **)&version_func));

	version = version_func();
	if (version < (NS_PLUGIN_VERSION - NS_PLUGIN_AGE) ||
	    version > NS_PLUGIN_VERSION)
	{
		isc_log_write(ns_lctx, NS_LOGCATEGORY_GENERAL,
			      NS_LOGMODULE_HOOKS, ISC_LOG_ERROR,
			      "plugin API version mismatch: %d/%d", version,
			      NS_PLUGIN_VERSION);
		CHECK(ISC_R_FAILURE);
	}

	CHECK(load_symbol(handle, modpath, "plugin_register",
			  (void **)&register_func));
	CHECK(load_symbol(handle, modpath, "plugin_destroy",
			  (void **)&destroy_func));

	plugin = isc_mem_get(mctx, sizeof(*plugin));
	memset(plugin, 0, sizeof(*plugin));
	isc_mem_attach(mctx, &plugin->mctx);
	plugin->handle = handle;
	plugin->modpath = isc_mem_strdup(plugin->mctx, modpath);
	plugin->register_func = register_func;
	plugin->destroy_func = destroy_func;

	ISC_LINK_INIT(plugin, link);

	*pluginp = plugin;
	plugin = NULL;

cleanup:
	if (result != ISC_R_SUCCESS) {
		isc_log_write(ns_lctx, NS_LOGCATEGORY_GENERAL,
			      NS_LOGMODULE_HOOKS, ISC_LOG_ERROR,
			      "failed to dynamically load "
			      "plugin '%s': %d (%s)",
			      modpath, GetLastError(),
			      isc_result_totext(result));

		if (plugin != NULL) {
			isc_mem_putanddetach(&plugin->mctx, plugin,
					     sizeof(*plugin));
		}

		if (handle != NULL) {
			FreeLibrary(handle);
		}
	}

	return (result);
}

static void
unload_plugin(ns_plugin_t **pluginp) {
	ns_plugin_t *plugin = NULL;

	REQUIRE(pluginp != NULL && *pluginp != NULL);

	plugin = *pluginp;
	*pluginp = NULL;

	isc_log_write(ns_lctx, NS_LOGCATEGORY_GENERAL, NS_LOGMODULE_HOOKS,
		      ISC_LOG_DEBUG(1), "unloading plugin '%s'",
		      plugin->modpath);

	if (plugin->inst != NULL) {
		plugin->destroy_func(&plugin->inst);
	}
	if (plugin->handle != NULL) {
		FreeLibrary(plugin->handle);
	}

	if (plugin->modpath != NULL) {
		isc_mem_free(plugin->mctx, plugin->modpath);
	}

	isc_mem_putanddetach(&plugin->mctx, plugin, sizeof(*plugin));
}
#else  /* HAVE_DLFCN_H || _WIN32 */
static isc_result_t
load_plugin(isc_mem_t *mctx, const char *modpath, ns_plugin_t **pluginp) {
	UNUSED(mctx);
	UNUSED(modpath);
	UNUSED(pluginp);

	isc_log_write(ns_lctx, NS_LOGCATEGORY_GENERAL, NS_LOGMODULE_HOOKS,
		      ISC_LOG_ERROR, "plugin support is not implemented");

	return (ISC_R_NOTIMPLEMENTED);
}

static void
unload_plugin(ns_plugin_t **pluginp) {
	UNUSED(pluginp);
}
#endif /* HAVE_DLFCN_H */

isc_result_t
ns_plugin_register(const char *modpath, const char *parameters, const void *cfg,
		   const char *cfg_file, unsigned long cfg_line,
		   isc_mem_t *mctx, isc_log_t *lctx, void *actx,
		   dns_view_t *view) {
	isc_result_t result;
	ns_plugin_t *plugin = NULL;

	REQUIRE(mctx != NULL);
	REQUIRE(lctx != NULL);
	REQUIRE(view != NULL);

	isc_log_write(ns_lctx, NS_LOGCATEGORY_GENERAL, NS_LOGMODULE_HOOKS,
		      ISC_LOG_INFO, "loading plugin '%s'", modpath);

	CHECK(load_plugin(mctx, modpath, &plugin));

	isc_log_write(ns_lctx, NS_LOGCATEGORY_GENERAL, NS_LOGMODULE_HOOKS,
		      ISC_LOG_INFO, "registering plugin '%s'", modpath);

	CHECK(plugin->register_func(parameters, cfg, cfg_file, cfg_line, mctx,
				    lctx, actx, view->hooktable,
				    &plugin->inst));

	ISC_LIST_APPEND(*(ns_plugins_t *)view->plugins, plugin, link);

cleanup:
	if (result != ISC_R_SUCCESS && plugin != NULL) {
		unload_plugin(&plugin);
	}

	return (result);
}

isc_result_t
ns_plugin_check(const char *modpath, const char *parameters, const void *cfg,
		const char *cfg_file, unsigned long cfg_line, isc_mem_t *mctx,
		isc_log_t *lctx, void *actx) {
	isc_result_t result;
	ns_plugin_t *plugin = NULL;

	CHECK(load_plugin(mctx, modpath, &plugin));

	result = plugin->check_func(parameters, cfg, cfg_file, cfg_line, mctx,
				    lctx, actx);

cleanup:
	if (plugin != NULL) {
		unload_plugin(&plugin);
	}

	return (result);
}

void
ns_hooktable_init(ns_hooktable_t *hooktable) {
	int i;

	for (i = 0; i < NS_HOOKPOINTS_COUNT; i++) {
		ISC_LIST_INIT((*hooktable)[i]);
	}
}

isc_result_t
ns_hooktable_create(isc_mem_t *mctx, ns_hooktable_t **tablep) {
	ns_hooktable_t *hooktable = NULL;

	REQUIRE(tablep != NULL && *tablep == NULL);

	hooktable = isc_mem_get(mctx, sizeof(*hooktable));

	ns_hooktable_init(hooktable);

	*tablep = hooktable;

	return (ISC_R_SUCCESS);
}

void
ns_hooktable_free(isc_mem_t *mctx, void **tablep) {
	ns_hooktable_t *table = NULL;
	ns_hook_t *hook = NULL, *next = NULL;
	int i = 0;

	REQUIRE(tablep != NULL && *tablep != NULL);

	table = *tablep;
	*tablep = NULL;

	for (i = 0; i < NS_HOOKPOINTS_COUNT; i++) {
		for (hook = ISC_LIST_HEAD((*table)[i]); hook != NULL;
		     hook = next)
		{
			next = ISC_LIST_NEXT(hook, link);
			ISC_LIST_UNLINK((*table)[i], hook, link);
			if (hook->mctx != NULL) {
				isc_mem_putanddetach(&hook->mctx, hook,
						     sizeof(*hook));
			}
		}
	}

	isc_mem_put(mctx, table, sizeof(*table));
}

void
ns_hook_add(ns_hooktable_t *hooktable, isc_mem_t *mctx,
	    ns_hookpoint_t hookpoint, const ns_hook_t *hook) {
	ns_hook_t *copy = NULL;

	REQUIRE(hooktable != NULL);
	REQUIRE(mctx != NULL);
	REQUIRE(hookpoint < NS_HOOKPOINTS_COUNT);
	REQUIRE(hook != NULL);

	copy = isc_mem_get(mctx, sizeof(*copy));
	memset(copy, 0, sizeof(*copy));

	copy->action = hook->action;
	copy->action_data = hook->action_data;
	isc_mem_attach(mctx, &copy->mctx);

	ISC_LINK_INIT(copy, link);
	ISC_LIST_APPEND((*hooktable)[hookpoint], copy, link);
}

void
ns_plugins_create(isc_mem_t *mctx, ns_plugins_t **listp) {
	ns_plugins_t *plugins = NULL;

	REQUIRE(listp != NULL && *listp == NULL);

	plugins = isc_mem_get(mctx, sizeof(*plugins));
	memset(plugins, 0, sizeof(*plugins));
	ISC_LIST_INIT(*plugins);

	*listp = plugins;
}

void
ns_plugins_free(isc_mem_t *mctx, void **listp) {
	ns_plugins_t *list = NULL;
	ns_plugin_t *plugin = NULL, *next = NULL;

	REQUIRE(listp != NULL && *listp != NULL);

	list = *listp;
	*listp = NULL;

	for (plugin = ISC_LIST_HEAD(*list); plugin != NULL; plugin = next) {
		next = ISC_LIST_NEXT(plugin, link);
		ISC_LIST_UNLINK(*list, plugin, link);
		unload_plugin(&plugin);
	}

	isc_mem_put(mctx, list, sizeof(*list));
}
