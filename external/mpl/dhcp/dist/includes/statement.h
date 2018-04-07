/*	$NetBSD: statement.h,v 1.2 2018/04/07 22:37:29 christos Exp $	*/

/* statement.h

   Definitions for executable statements... */

/*
 * Copyright (c) 2004-2017 by Internet Systems Consortium, Inc. ("ISC")
 * Copyright (c) 1996-2003 by Internet Software Consortium
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND ISC DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS.  IN NO EVENT SHALL ISC BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT
 * OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 *   Internet Systems Consortium, Inc.
 *   950 Charter Street
 *   Redwood City, CA 94063
 *   <info@isc.org>
 *   https://www.isc.org/
 *
 */

struct executable_statement {
	int refcnt;
	struct executable_statement *next;
	enum statement_op {
		null_statement,
		if_statement,
		add_statement,
		eval_statement,
		break_statement,
		default_option_statement,
		supersede_option_statement,
		append_option_statement,
		prepend_option_statement,
		send_option_statement,
		statements_statement,
		on_statement,
		switch_statement,
		case_statement,
		default_statement,
		set_statement,
		unset_statement,
		let_statement,
		define_statement,
		log_statement,
		return_statement,
		execute_statement,
		vendor_opt_statement
	} op;
	union {
		struct {
			struct executable_statement *tc, *fc;
			struct expression *expr;
		} ie;
		struct expression *eval;
		struct expression *retval;
		struct class *add;
		struct option_cache *option;
		struct option_cache *supersede;
		struct option_cache *prepend;
		struct option_cache *append;
		struct executable_statement *statements;
		struct {
			int evtypes;
#			define ON_COMMIT  1
#			define ON_EXPIRY  2
#			define ON_RELEASE 4
#			define ON_TRANSMISSION 8
			struct executable_statement *statements;
		} on;
		struct {
			struct expression *expr;
			struct executable_statement *statements;
		} s_switch;
		struct expression *c_case;
		struct {
			char *name;
			struct expression *expr;
			struct executable_statement *statements;
		} set, let;
		char *unset;
		struct {
			enum {
				log_priority_fatal,
				log_priority_error,
				log_priority_debug,
				log_priority_info
			} priority;
			struct expression *expr;
		} log;
		struct {
			char *command;
			struct expression *arglist;
			int argc;
		} execute;
	} data;
};

