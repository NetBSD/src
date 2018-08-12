/*	$NetBSD: hooks.h,v 1.1.1.1 2018/08/12 12:08:07 christos Exp $	*/

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

#ifdef NS_HOOKS_ENABLE

/*! \file */

#include <isc/result.h>

/*
 * Hooks provide a way of running a callback function once a certain place in
 * code is reached.  Current use is limited to libns unit tests and thus:
 *
 *   - hook-related types and macros are not placed in libns header files,
 *   - hook-related code is compiled away unless --with-atf is used,
 *   - hook-related macro names are prefixed with "NS_".
 *
 * However, the implementation is pretty generic and could be repurposed for
 * general use, e.g. as part of libisc, after some further customization.
 *
 * Hooks are created by inserting a macro into any function returning
 * isc_result_t (NS_PROCESS_HOOK()) or void (NS_PROCESS_HOOK_VOID()).  As both
 * of these macros contain a return statement which is inlined into the
 * function into which the hook is inserted, a hook callback is able to cause
 * that function to return at hook insertion point.  For functions returning
 * isc_result_t, if a hook callback intends to cause a return at hook insertion
 * point, it also has to set the value to be returned by the function.
 *
 * Hook callbacks are functions which:
 *
 *   - return a boolean value; if ISC_TRUE is returned by the callback, the
 *     function into which the hook is inserted will return at hook insertion
 *     point; if ISC_FALSE is returned by the callback, execution of the
 *     function into which the hook is inserted continues normally,
 *
 *   - accept three pointers as arguments:
 *
 *       - a pointer specified by the hook itself,
 *       - a pointer specified upon inserting the callback into the hook table,
 *       - a pointer to isc_result_t which will be returned by the function
 *         into which the hook is inserted if the callback returns ISC_TRUE.
 *
 * Hook tables are arrays which consist of a number of tuples (one tuple per
 * hook identifier), each of which determines the callback to be invoked when a
 * given hook is processed and the data to be passed to that callback.  In an
 * attempt to keep things as simple as possible, current implementation uses
 * hook tables which are statically-sized arrays only allowing a single
 * callback to be invoked for each hook identifier.
 *
 * In order for a hook callback to be called for a given hook, a pointer to
 * that callback (along with an optional pointer to callback-specific data) has
 * to be inserted into the relevant hook table entry for that hook.  Replacing
 * whole hook tables is also possible.
 *
 * Consider the following sample code:
 *
 * ----------------------------------------------------------------------------
 * ns_hook_t *foo_hook_table = NULL;
 *
 * isc_result_t
 * foo_bar(void) {
 *     int val = 42;
 *
 *     ...
 *
 *     NS_PROCESS_HOOK(foo_hook_table, FOO_EXTRACT_VAL, &val);
 *
 *     ...
 *
 *     printf("This message may not be printed due to use of hooks.");
 *
 *     return (ISC_R_SUCCESS);
 * }
 *
 * isc_boolean_t
 * cause_failure(void *hook_data, void *callback_data, isc_result_t *resultp) {
 *     int *valp = (int *)hook_data;
 *     isc_boolean_t *calledp = (isc_boolean_t *)callback_data;
 *
 *     ...
 *
 *     *resultp = ISC_R_FAILURE;
 *
 *     return (ISC_TRUE);
 * }
 *
 * isc_boolean_t
 * examine_val(void *hook_data, void *callback_data, isc_result_t *resultp) {
 *     int *valp = (int *)hook_data;
 *     int *valcopyp = (int *)callback_data;
 *
 *     UNUSED(resultp);
 *
 *     ...
 *
 *     return (ISC_FALSE);
 * }
 *
 * void
 * test_foo_bar(void) {
 *     isc_boolean_t called = ISC_FALSE;
 *     int valcopy;
 *
 *     ns_hook_t my_hooks[FOO_HOOKS_COUNT] = {
 *         [FOO_EXTRACT_VAL] = {
 *             .callback = cause_failure,
 *             .callback_data = &called,
 *         },
 *     };
 *
 *     foo_hook_table = my_hooks;
 *     foo_bar();
 *
 *     {
 *         const ns_hook_t examine_hook = {
 *             .callback = examine_val,
 *             .callback_data = &valcopy,
 *         };
 *
 *         my_hooks[FOO_EXTRACT_VAL] = examine_hook;
 *     }
 *     foo_bar();
 *
 * }
 * ----------------------------------------------------------------------------
 *
 * When test_foo_bar() is called, "foo_hook_table" is set to "my_hooks".  Then
 * foo_bar() gets invoked.  Once execution reaches the insertion point for hook
 * FOO_EXTRACT_VAL, cause_failure() will be called with &val as "hook_data" and
 * &called as "callback_data".  It can do whatever it pleases with these two
 * values.  Eventually, cause_failure() sets *resultp to ISC_R_FAILURE and
 * returns ISC_TRUE, which causes foo_bar() to return ISC_R_FAILURE and never
 * execute the printf() call below hook insertion point.
 *
 * Execution then returns to test_foo_bar().  Unlike before the first call to
 * foo_bar(), this time only a single hook ("examine_hook") is defined instead
 * of a complete hook table.  This hook is then subsequently inserted at index
 * FOO_EXTRACT_VAL into the "my_hook" hook table.  This causes the hook
 * previously set at that index (the one calling cause_failure()) to be
 * replaced with "examine_hook".  Thus, when the second call to foo_bar() is
 * subsequently made, examine_val() will be called with &val as "hook_data" and
 * &valcopy as "callback_data".  Contrary to cause_failure(), extract_val()
 * returns ISC_FALSE, which means it does not access "resultp" and does not
 * cause foo_bar() to return at hook insertion point.  Thus, printf() will be
 * called this time and foo_bar() will return ISC_R_SUCCESS.
 */

enum {
	NS_QUERY_SETUP_QCTX_INITIALIZED,
	NS_QUERY_LOOKUP_BEGIN,
	NS_QUERY_DONE_BEGIN,
	NS_QUERY_HOOKS_COUNT	/* MUST BE LAST */
};

typedef isc_boolean_t
(*ns_hook_cb_t)(void *hook_data, void *callback_data, isc_result_t *resultp);

typedef struct ns_hook {
	ns_hook_cb_t callback;
	void *callback_data;
} ns_hook_t;

#define _NS_PROCESS_HOOK(table, id, data, ...)				\
	if (table != NULL) {						\
		ns_hook_cb_t _callback = table[id].callback;		\
		void *_callback_data = table[id].callback_data;		\
		isc_result_t _result;					\
									\
		if (_callback != NULL &&				\
		    _callback(data, _callback_data, &_result)) {	\
			return __VA_ARGS__;				\
		}							\
	}

#define NS_PROCESS_HOOK(table, id, data) \
	_NS_PROCESS_HOOK(table, id, data, _result)

#define NS_PROCESS_HOOK_VOID(table, id, data) \
	_NS_PROCESS_HOOK(table, id, data)

LIBNS_EXTERNAL_DATA extern ns_hook_t *ns__hook_table;

#endif /* NS_HOOKS_ENABLE */
#endif /* NS_HOOKS_H */
