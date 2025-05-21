/*	$NetBSD: grammar_test.c,v 1.1.1.1 2025/05/21 14:41:02 christos Exp $	*/

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
#include <sched.h> /* IWYU pragma: keep */
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define UNIT_TESTING
#include <cmocka.h>

#include <isc/buffer.h>
#include <isc/lex.h>
#include <isc/log.h>
#include <isc/mem.h>
#include <isc/string.h>
#include <isc/types.h>
#include <isc/util.h>

#include <isccfg/cfg.h>
#include <isccfg/grammar.h>
#include <isccfg/namedconf.h>

#include <tests/isc.h>

static void
write_to_buffer(void *closure, const char *text, int textlen);

static isc_buffer_t gbuffer;
static char gtext[512];
static cfg_printer_t gprinter = {
	.f = write_to_buffer, .closure = &gbuffer, .indent = 0, .flags = 0
};

ISC_SETUP_TEST_IMPL(group) {
	isc_buffer_init(&gbuffer, gtext, sizeof(gtext));
	return 0;
}

static void
write_to_buffer(void *closure, const char *text, int textlen) {
	isc_buffer_putmem((isc_buffer_t *)closure, (const unsigned char *)text,
			  textlen);
}

static void
assert_text(const char *text) {
	assert_int_equal(strcasecmp(text, isc_buffer_base(&gbuffer)), 0);
	isc_buffer_clear(&gbuffer);
	memset(gtext, 0, sizeof(gtext));
}

static const cfg_clausedef_t *
find_clause(const cfg_type_t *map, const char *name) {
	const char *found_name = NULL;
	const void *clauses = NULL;
	unsigned int idx;

	found_name = cfg_map_firstclause(map, &clauses, &idx);
	while (name != NULL && strcasecmp(name, found_name)) {
		found_name = cfg_map_nextclause(map, &clauses, &idx);
	}

	return ((cfg_clausedef_t *)clauses) + idx;
}

static void
test__querysource(const char *clause_name, const char *name,
		  const char *expected) {
	const cfg_clausedef_t *options_clause = NULL;
	options_clause = find_clause(&cfg_type_namedconf, clause_name);
	assert_non_null(options_clause);

	const cfg_clausedef_t *querysource_clause = NULL;
	querysource_clause = find_clause(options_clause->type, name);
	assert_non_null(querysource_clause);
	querysource_clause->type->doc(&gprinter, querysource_clause->type);
	assert_text(expected);
}

ISC_RUN_TEST_IMPL(query_source) {
	test__querysource("options", "query-source",
			  "[ address ] ( <ipv4_address> | * | none )");
}

ISC_RUN_TEST_IMPL(query_source_v6) {
	test__querysource("options", "query-source-v6",
			  "[ address ] ( <ipv6_address> | * | none )");
}

ISC_RUN_TEST_IMPL(server_query_source) {
	test__querysource("server", "query-source",
			  "[ address ] ( <ipv4_address> | * )");
}

ISC_RUN_TEST_IMPL(server_query_source_v6) {
	test__querysource("server", "query-source-v6",
			  "[ address ] ( <ipv6_address> | * )");
}

static void
test__query_source_print(const char *config, const char *expected) {
	isc_result_t result;
	isc_buffer_t buffer;
	cfg_parser_t *parser = NULL;
	cfg_obj_t *output_conf = NULL;

	result = cfg_parser_create(mctx, lctx, &parser);
	assert_int_equal(result, ISC_R_SUCCESS);

	isc_buffer_constinit(&buffer, config, strlen(config));
	isc_buffer_add(&buffer, strlen(config));

	result = cfg_parse_buffer(parser, &buffer, "text1", 0,
				  &cfg_type_namedconf, 0, &output_conf);
	assert_int_equal(result, ISC_R_SUCCESS);
	assert_non_null(output_conf);

	cfg_printer_t pctx = gprinter;
	pctx.flags = CFG_PRINTER_ONELINE;

	output_conf->type->print(&pctx, output_conf);
	assert_text(expected);

	cfg_obj_destroy(parser, &output_conf);
	cfg_parser_reset(parser);
	cfg_parser_destroy(&parser);
}

ISC_RUN_TEST_IMPL(query_source_print_none) {
	test__query_source_print(" options     {    query-source none;     };",
				 "options { query-source none; }; ");
	test__query_source_print(
		" options     { query-source address none;  };",
		"options { query-source none; }; ");
	test__query_source_print(" options     {query-source-v6 none;     };",
				 "options { query-source-v6 none; }; ");
	test__query_source_print(" options {query-source-v6 address none;};",
				 "options { query-source-v6 none; }; ");
}

ISC_RUN_TEST_IMPL(query_source_print_addr) {
	test__query_source_print(
		" options{query-source address 127.0.0.1;};",
		"options { query-source 127.0.0.1 port 0; }; ");
	test__query_source_print(" options{query-source-v6 address ::1;     };",
				 "options { query-source-v6 ::1 port 0; }; ");
	test__query_source_print(
		" options{query-source 127.0.0.1;};",
		"options { query-source 127.0.0.1 port 0; }; ");
	test__query_source_print(" options{query-source-v6 ::1;     };",
				 "options { query-source-v6 ::1 port 0; }; ");
	test__query_source_print(
		"options { query-source 127.0.0.1 port 6666; };",
		"options { query-source 127.0.0.1 port 6666; }; ");
}

ISC_TEST_LIST_START

ISC_TEST_ENTRY(query_source)
ISC_TEST_ENTRY(query_source_v6)
ISC_TEST_ENTRY(server_query_source)
ISC_TEST_ENTRY(server_query_source_v6)
ISC_TEST_ENTRY(query_source_print_none)
ISC_TEST_ENTRY(query_source_print_addr)

ISC_TEST_LIST_END

ISC_TEST_MAIN_CUSTOM(setup_test_group, NULL)
