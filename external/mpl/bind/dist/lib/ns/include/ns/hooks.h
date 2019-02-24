/*	$NetBSD: hooks.h,v 1.3 2019/02/24 20:01:33 christos Exp $	*/

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

#ifndef NS_HOOKS_H
#define NS_HOOKS_H 1

/*! \file */

#include <stdbool.h>

#include <isc/list.h>
#include <isc/magic.h>
#include <isc/result.h>

#include <dns/rdatatype.h>

#include <ns/client.h>
#include <ns/query.h>
/*
 * "Hooks" are a mechanism to call a defined function or set of functions once
 * a certain place in code is reached.  Hook actions can inspect and alter the
 * state of an ongoing process, allowing processing to continue afterward or
 * triggering an early return.
 *
 * Currently hooks are used in two ways: in plugins, which use them to
 * add functionality to query processing, and in the unit tests for libns,
 * where they are used to inspect state before and after certain functions have
 * run.
 *
 * Both of these uses are limited to libns, so hooks are currently defined in
 * the ns/hooks.h header file, and hook-related macro and function names are
 * prefixed with `NS_` and `ns_`.  However, the design is fairly generic and
 * could be repurposed for general use, e.g. as part of libisc, after some
 * further customization.
 *
 * Hooks are created by defining a hook point identifier in the ns_hookpoint_t
 * enum below, and placing a special call at a corresponding location in the
 * code which invokes the action(s) for that hook; there are two such special
 * calls currently implemented, namely the CALL_HOOK() and CALL_HOOK_NORETURN()
 * macros in query.c.  The former macro contains a "goto cleanup" statement
 * which is inlined into the function into which the hook has been inserted;
 * this enables the hook action to cause the calling function to return from
 * the hook insertion point.  For functions returning isc_result_t, if a hook
 * action intends to cause a return at hook insertion point, it also has to set
 * the value to be returned by the calling function.
 *
 * A hook table is an array (indexed by the value of the hook point identifier)
 * in which each cell contains a linked list of structures, each of which
 * contains a function pointer to a hook action and a pointer to data which is
 * to be passed to the action function when it is called.
 *
 * Each view has its own separate hook table, populated by loading plugin
 * modules specified in the "plugin" statements in named.conf.  There is also a
 * special, global hook table (ns__hook_table) that is only used by libns unit
 * tests and whose existence can be safely ignored by plugin modules.
 *
 * Hook actions are functions which:
 *
 *   - return an ns_hookresult_t value:
 *       - if NS_HOOK_RETURN is returned by the hook action, the function
 *         into which the hook is inserted will return and no further hook
 *         actions at the same hook point will be invoked,
 *       - if NS_HOOK_CONTINUE is returned by the hook action and there are
 *         further hook actions set up at the same hook point, they will be
 *         processed; if NS_HOOK_CONTINUE is returned and there are no
 *         further hook actions set up at the same hook point, execution of
 *         the function into which the hook has been inserted will be
 *         resumed.
 *
 *   - accept three pointers as arguments:
 *       - a pointer specified by the special call at the hook insertion point,
 *       - a pointer specified upon inserting the action into the hook table,
 *       - a pointer to an isc_result_t value which will be returned by the
 *         function into which the hook is inserted if the action returns
 *         NS_HOOK_RETURN.
 *
 * In order for a hook action to be called for a given hook, a pointer to that
 * action function (along with an optional pointer to action-specific data) has
 * to be inserted into the relevant hook table entry for that hook using an
 * ns_hook_add() call.  If multiple actions are set up at a single hook point
 * (e.g. by multiple plugin modules), they are processed in FIFO order, that is
 * they are performed in the same order in which their relevant ns_hook_add()
 * calls were issued.  Since the configuration is loaded from a single thread,
 * this means that multiple actions at a single hook point are determined by
 * the order in which the relevant plugin modules were declared in the
 * configuration file(s).  The hook API currently does not support changing
 * this order.
 *
 * As an example, consider the following hypothetical function in query.c:
 *
 * ----------------------------------------------------------------------------
 * static isc_result_t
 * query_foo(query_ctx_t *qctx) {
 *     isc_result_t result;
 *
 *     CALL_HOOK(NS_QUERY_FOO_BEGIN, qctx);
 *
 *     ns_client_log(qctx->client, NS_LOGCATEGORY_CLIENT, NS_LOGMODULE_QUERY,
 *                   ISC_LOG_DEBUG(99), "Lorem ipsum dolor sit amet...");
 *
 *     result = ISC_R_COMPLETE;
 *
 *  cleanup:
 *     return (result);
 * }
 * ----------------------------------------------------------------------------
 *
 * and the following hook action:
 *
 * ----------------------------------------------------------------------------
 * static ns_hookresult_t
 * cause_failure(void *hook_data, void *action_data, isc_result_t *resultp) {
 *     UNUSED(hook_data);
 *     UNUSED(action_data);
 *
 *     *resultp = ISC_R_FAILURE;
 *
 *     return (NS_HOOK_RETURN);
 * }
 * ----------------------------------------------------------------------------
 *
 * If this hook action was installed in the hook table using:
 *
 * ----------------------------------------------------------------------------
 * const ns_hook_t foo_fail = {
 *     .action = cause_failure,
 * };
 *
 * ns_hook_add(..., NS_QUERY_FOO_BEGIN, &foo_fail);
 * ----------------------------------------------------------------------------
 *
 * then query_foo() would return ISC_R_FAILURE every time it is called due
 * to the cause_failure() hook action returning NS_HOOK_RETURN and setting
 * '*resultp' to ISC_R_FAILURE.  query_foo() would also never log the
 * "Lorem ipsum dolor sit amet..." message.
 *
 * Consider a different hook action:
 *
 * ----------------------------------------------------------------------------
 * static ns_hookresult_t
 * log_qtype(void *hook_data, void *action_data, isc_result_t *resultp) {
 *     query_ctx_t *qctx = (query_ctx_t *)hook_data;
 *     FILE *stream = (FILE *)action_data;
 *
 *     UNUSED(resultp);
 *
 *     fprintf(stream, "QTYPE=%u\n", qctx->qtype);
 *
 *     return (NS_HOOK_CONTINUE);
 * }
 * ----------------------------------------------------------------------------
 *
 * If this hook action was installed in the hook table instead of
 * cause_failure(), using:
 *
 * ----------------------------------------------------------------------------
 * const ns_hook_t foo_log_qtype = {
 *     .action = log_qtype,
 *     .action_data = stderr,
 * };
 *
 * ns_hook_add(..., NS_QUERY_FOO_BEGIN, &foo_log_qtype);
 * ----------------------------------------------------------------------------
 *
 * then the QTYPE stored in the query context passed to query_foo() would be
 * logged to stderr upon each call to that function; 'qctx' would be passed to
 * the hook action in 'hook_data' since it is specified in the CALL_HOOK() call
 * inside query_foo() while stderr would be passed to the hook action in
 * 'action_data' since it is specified in the ns_hook_t structure passed to
 * ns_hook_add().  As the hook action returns NS_HOOK_CONTINUE,
 * query_foo() would also be logging the "Lorem ipsum dolor sit amet..."
 * message before returning ISC_R_COMPLETE.
 */

/*!
 * Currently-defined hook points. So long as these are unique,
 * the order in which they are declared is unimportant, but
 * currently matches the order in which they are referenced in
 * query.c.
 */
typedef enum {
	/* hookpoints from query.c */
	NS_QUERY_QCTX_INITIALIZED,
	NS_QUERY_QCTX_DESTROYED,
	NS_QUERY_SETUP,
	NS_QUERY_START_BEGIN,
	NS_QUERY_LOOKUP_BEGIN,
	NS_QUERY_RESUME_BEGIN,
	NS_QUERY_RESUME_RESTORED,
	NS_QUERY_GOT_ANSWER_BEGIN,
	NS_QUERY_RESPOND_ANY_BEGIN,
	NS_QUERY_RESPOND_ANY_FOUND,
	NS_QUERY_ADDANSWER_BEGIN,
	NS_QUERY_RESPOND_BEGIN,
	NS_QUERY_NOTFOUND_BEGIN,
	NS_QUERY_NOTFOUND_RECURSE,
	NS_QUERY_PREP_DELEGATION_BEGIN,
	NS_QUERY_ZONE_DELEGATION_BEGIN,
	NS_QUERY_DELEGATION_BEGIN,
	NS_QUERY_DELEGATION_RECURSE_BEGIN,
	NS_QUERY_NODATA_BEGIN,
	NS_QUERY_NXDOMAIN_BEGIN,
	NS_QUERY_NCACHE_BEGIN,
	NS_QUERY_ZEROTTL_RECURSE,
	NS_QUERY_CNAME_BEGIN,
	NS_QUERY_DNAME_BEGIN,
	NS_QUERY_PREP_RESPONSE_BEGIN,
	NS_QUERY_DONE_BEGIN,
	NS_QUERY_DONE_SEND,

	/* XXX other files could be added later */

	NS_HOOKPOINTS_COUNT	/* MUST BE LAST */
} ns_hookpoint_t;

/*
 * Returned by a hook action to indicate how to proceed after it has
 * been called: continue processing, or return immediately.
 */
typedef enum {
	NS_HOOK_CONTINUE,
	NS_HOOK_RETURN,
} ns_hookresult_t;

typedef ns_hookresult_t
(*ns_hook_action_t)(void *arg, void *data, isc_result_t *resultp);

typedef struct ns_hook {
	isc_mem_t *mctx;
	ns_hook_action_t action;
	void *action_data;
	ISC_LINK(struct ns_hook) link;
} ns_hook_t;

typedef ISC_LIST(ns_hook_t) ns_hooklist_t;
typedef ns_hooklist_t ns_hooktable_t[NS_HOOKPOINTS_COUNT];

/*%
 * ns__hook_table is a global hook table, which is used if view->hooktable
 * is NULL.  It's intended only for use by unit tests.
 */
LIBNS_EXTERNAL_DATA extern ns_hooktable_t *ns__hook_table;

/*
 * Plugin API version
 *
 * When the API changes, increment NS_PLUGIN_VERSION. If the
 * change is backward-compatible (e.g., adding a new function call
 * but not changing or removing an old one), increment NS_PLUGIN_AGE
 * as well; if not, set NS_PLUGIN_AGE to 0.
 */
#ifndef NS_PLUGIN_VERSION
#define NS_PLUGIN_VERSION 1
#define NS_PLUGIN_AGE 0
#endif

typedef isc_result_t
ns_plugin_register_t(const char *parameters,
		     const void *cfg, const char *file, unsigned long line,
		     isc_mem_t *mctx, isc_log_t *lctx, void *actx,
		     ns_hooktable_t *hooktable, void **instp);
/*%<
 * Called when registering a new plugin.
 *
 * 'parameters' contains the plugin configuration text.
 *
 * '*instp' will be set to the module instance handle if the function
 * is successful.
 *
 * Returns:
 *\li	#ISC_R_SUCCESS
 *\li	#ISC_R_NOMEMORY
 *\li	Other errors are possible
 */

typedef void
ns_plugin_destroy_t(void **instp);
/*%<
 * Destroy a plugin instance.
 *
 * '*instp' must be set to NULL by the function before it returns.
 */

typedef isc_result_t
ns_plugin_check_t(const char *parameters,
		  const void *cfg, const char *file, unsigned long line,
		  isc_mem_t *mctx, isc_log_t *lctx, void *actx);
/*%<
 * Check the validity of 'parameters'.
 */

typedef int
ns_plugin_version_t(void);
/*%<
 * Return the API version number a plugin was compiled with.
 *
 * If the returned version number is no greater than
 * NS_PLUGIN_VERSION, and no less than NS_PLUGIN_VERSION - NS_PLUGIN_AGE,
 * then the module is API-compatible with named.
 */

/*%
 * Prototypes for API functions to be defined in each module.
 */
ns_plugin_check_t plugin_check;
ns_plugin_destroy_t plugin_destroy;
ns_plugin_register_t plugin_register;
ns_plugin_version_t plugin_version;

isc_result_t
ns_plugin_register(const char *modpath, const char *parameters,
		   const void *cfg, const char *cfg_file,
		   unsigned long cfg_line,
		   isc_mem_t *mctx, isc_log_t *lctx, void *actx,
		   dns_view_t *view);
/*%<
 * Load the plugin module specified from the file 'modpath', and
 * register an instance using 'parameters'.
 *
 * 'cfg_file' and 'cfg_line' specify the location of the plugin
 * declaration in the configuration file.
 *
 * 'cfg' and 'actx' are the configuration context and ACL configuration
 * context, respectively; they are passed as void * here in order to
 * prevent this library from having a dependency on libisccfg).
 *
 * 'instp' will be left pointing to the instance of the plugin
 * created by the module's plugin_register function.
 */

isc_result_t
ns_plugin_check(const char *modpath, const char *parameters,
		const void *cfg, const char *cfg_file, unsigned long cfg_line,
		isc_mem_t *mctx, isc_log_t *lctx, void *actx);
/*%<
 * Open the plugin module at 'modpath' and check the validity of
 * 'parameters', logging any errors or warnings found, then
 * close it without configuring it.
 */

void
ns_plugins_create(isc_mem_t *mctx, ns_plugins_t **listp);
/*%<
 * Create and initialize a plugin list.
 */

void
ns_plugins_free(isc_mem_t *mctx, void **listp);
/*%<
 * Close each plugin module in a plugin list, then free the list object.
 */

void
ns_hooktable_free(isc_mem_t *mctx, void **tablep);
/*%<
 * Free a hook table.
 */

void
ns_hook_add(ns_hooktable_t *hooktable, isc_mem_t *mctx,
	    ns_hookpoint_t hookpoint, const ns_hook_t *hook);
/*%<
 * Allocate (using memory context 'mctx') a copy of the 'hook' structure
 * describing a hook action and append it to the list of hooks at 'hookpoint'
 * in 'hooktable'.
 *
 * Requires:
 *\li 'hooktable' is not NULL
 *
 *\li 'mctx' is not NULL
 *
 *\li 'hookpoint' is less than NS_QUERY_HOOKS_COUNT
 *
 *\li 'hook' is not NULL
 */

void
ns_hooktable_init(ns_hooktable_t *hooktable);
/*%<
 * Initialize a hook table.
 */

isc_result_t
ns_hooktable_create(isc_mem_t *mctx, ns_hooktable_t **tablep);
/*%<
 * Allocate and initialize a hook table.
 */
#endif /* NS_HOOKS_H */
