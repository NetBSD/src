/*	$NetBSD: badcache_test.c,v 1.2 2025/01/26 16:25:47 christos Exp $	*/

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
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define UNIT_TESTING
#include <cmocka.h>

#include <isc/buffer.h>
#include <isc/commandline.h>
#include <isc/md.h>
#include <isc/mem.h>
#include <isc/os.h>
#include <isc/thread.h>
#include <isc/urcu.h>
#include <isc/util.h>
#include <isc/uv.h>

#include <dns/badcache.h>
#include <dns/compress.h>
#include <dns/fixedname.h>
#include <dns/name.h>
#include <dns/rdatatype.h>

#include <tests/dns.h>

#define BADCACHE_TEST_FLAG 1 << 3

ISC_LOOP_TEST_IMPL(basic) {
	dns_badcache_t *bc = NULL;
	dns_fixedname_t fname = { 0 };
	dns_name_t *name = dns_fixedname_initname(&fname);
	isc_stdtime_t now = isc_stdtime_now();
	isc_result_t result;
	uint32_t flags = BADCACHE_TEST_FLAG;

	dns_name_fromstring(name, "example.com.", NULL, 0, NULL);

	bc = dns_badcache_new(mctx, loopmgr);
	dns_badcache_add(bc, name, dns_rdatatype_aaaa, flags, now + 60);

	flags = 0;
	result = dns_badcache_find(bc, name, dns_rdatatype_aaaa, &flags, now);
	assert_int_equal(result, ISC_R_SUCCESS);
	assert_int_equal(flags, BADCACHE_TEST_FLAG);

	flags = 0;
	result = dns_badcache_find(bc, name, dns_rdatatype_a, &flags, now);
	assert_int_equal(result, ISC_R_NOTFOUND);
	assert_int_equal(flags, 0);

	dns_badcache_destroy(&bc);

	isc_loopmgr_shutdown(loopmgr);
}

ISC_LOOP_TEST_IMPL(expire) {
	dns_badcache_t *bc = NULL;
	dns_fixedname_t fname = { 0 };
	dns_name_t *name = dns_fixedname_initname(&fname);
	isc_stdtime_t now = isc_stdtime_now();
	isc_result_t result;
	uint32_t flags = BADCACHE_TEST_FLAG;

	dns_name_fromstring(name, "example.com.", NULL, 0, NULL);

	bc = dns_badcache_new(mctx, loopmgr);
	dns_badcache_add(bc, name, dns_rdatatype_aaaa, flags, now + 60);
	dns_badcache_add(bc, name, dns_rdatatype_a, flags, now + 60);

	result = dns_badcache_find(bc, name, dns_rdatatype_aaaa, &flags, now);
	assert_int_equal(result, ISC_R_SUCCESS);
	assert_int_equal(flags, BADCACHE_TEST_FLAG);

	result = dns_badcache_find(bc, name, dns_rdatatype_aaaa, &flags,
				   now + 61);
	assert_int_equal(result, ISC_R_NOTFOUND);

	result = dns_badcache_find(bc, name, dns_rdatatype_aaaa, &flags, now);
	assert_int_equal(result, ISC_R_NOTFOUND);

	result = dns_badcache_find(bc, name, dns_rdatatype_a, &flags, now);
	assert_int_equal(result, ISC_R_NOTFOUND);

	dns_badcache_add(bc, name, dns_rdatatype_a, flags, now + 120);

	result = dns_badcache_find(bc, name, dns_rdatatype_a, &flags, now + 61);
	assert_int_equal(result, ISC_R_SUCCESS);
	assert_int_equal(flags, BADCACHE_TEST_FLAG);

	dns_badcache_destroy(&bc);

	isc_loopmgr_shutdown(loopmgr);
}

ISC_LOOP_TEST_IMPL(print) {
	dns_badcache_t *bc = NULL;
	dns_fixedname_t fname = { 0 };
	dns_name_t *name = dns_fixedname_initname(&fname);
	isc_stdtime_t now = isc_stdtime_now();
	isc_stdtime_t expire = now + 60;
	uint32_t flags = BADCACHE_TEST_FLAG;
	FILE *file = NULL;
	char buf[4096];
	size_t len;
	char *pos;
	char *endptr;
	const char *header_part = ";\n; badcache\n;\n";
	const char *bol_part = "; ";
	const char *name_part = "example.com/";
	const char *ttl_part = " [ttl ";
	const char *eol_part = "]\n";
	size_t num_a = 0;
	bool seen_a = false, seen_aaaa = false;
	long ttl;

	dns_name_fromstring(name, "example.com.", NULL, 0, NULL);

	bc = dns_badcache_new(mctx, loopmgr);
	dns_badcache_add(bc, name, dns_rdatatype_a, flags, expire);
	dns_badcache_add(bc, name, dns_rdatatype_aaaa, flags, expire);

	file = fopen("./badcache.out", "w");
	dns_badcache_print(bc, "badcache", file);
	fclose(file);

	file = fopen("./badcache.out", "r");
	len = fread(buf, sizeof(buf[0]), ARRAY_SIZE(buf), file);
	assert_int_equal(len, 68);
	fclose(file);

	pos = buf;
	assert_memory_equal(pos, header_part, strlen(header_part));
	pos += strlen(header_part);

line:
	/* There's no fixed order for A and AAAA types in the hash table */
	assert_memory_equal(pos, bol_part, strlen(bol_part));
	pos += strlen(bol_part);

	assert_memory_equal(pos, name_part, strlen(name_part));
	pos += strlen(name_part);

	num_a = 0;
	while (*pos == 'A') {
		num_a++;
		pos++;
	}
	switch (num_a) {
	case 1:
		seen_a = true;
		break;
	case 4:
		seen_aaaa = true;
		break;
	default:
		assert_true(num_a == 1 || num_a == 4);
	}

	assert_memory_equal(pos, ttl_part, strlen(ttl_part));
	pos += strlen(ttl_part);

	ttl = strtol(pos, &endptr, 0);
	assert_ptr_not_equal(pos, endptr);
	assert_true(ttl >= 0 && ttl <= 60);
	pos = endptr;

	assert_memory_equal(pos, eol_part, strlen(eol_part));
	pos += strlen(eol_part);

	if (!seen_a || !seen_aaaa) {
		goto line;
	}

	assert_int_equal(pos - buf, len);

	dns_badcache_destroy(&bc);

	isc_loopmgr_shutdown(loopmgr);
}

ISC_LOOP_TEST_IMPL(flush) {
	dns_badcache_t *bc = NULL;
	dns_fixedname_t fname = { 0 };
	dns_name_t *name = dns_fixedname_initname(&fname);
	isc_stdtime_t now = isc_stdtime_now();
	isc_result_t result;
	uint32_t flags = BADCACHE_TEST_FLAG;

	dns_name_fromstring(name, "example.com.", NULL, 0, NULL);

	bc = dns_badcache_new(mctx, loopmgr);
	dns_badcache_add(bc, name, dns_rdatatype_aaaa, flags, now + 60);

	result = dns_badcache_find(bc, name, dns_rdatatype_aaaa, &flags, now);
	assert_int_equal(result, ISC_R_SUCCESS);

	dns_badcache_flush(bc);

	result = dns_badcache_find(bc, name, dns_rdatatype_aaaa, &flags, now);
	assert_int_equal(result, ISC_R_NOTFOUND);

	dns_badcache_destroy(&bc);

	isc_loopmgr_shutdown(loopmgr);
}

ISC_LOOP_TEST_IMPL(flushname) {
	dns_badcache_t *bc = NULL;
	dns_fixedname_t fname = { 0 };
	dns_name_t *name = dns_fixedname_initname(&fname);
	isc_stdtime_t now = isc_stdtime_now();
	isc_result_t result;
	uint32_t flags = BADCACHE_TEST_FLAG;

	bc = dns_badcache_new(mctx, loopmgr);

	dns_name_fromstring(name, "example.com.", NULL, 0, NULL);
	dns_badcache_add(bc, name, dns_rdatatype_aaaa, flags, now + 60);
	result = dns_badcache_find(bc, name, dns_rdatatype_aaaa, &flags, now);
	assert_int_equal(result, ISC_R_SUCCESS);

	dns_name_fromstring(name, "sub.example.com.", NULL, 0, NULL);
	dns_badcache_add(bc, name, dns_rdatatype_aaaa, flags, now + 60);
	result = dns_badcache_find(bc, name, dns_rdatatype_aaaa, &flags, now);
	assert_int_equal(result, ISC_R_SUCCESS);

	dns_name_fromstring(name, "sub.sub.example.com.", NULL, 0, NULL);
	dns_badcache_add(bc, name, dns_rdatatype_aaaa, flags, now + 60);
	result = dns_badcache_find(bc, name, dns_rdatatype_aaaa, &flags, now);
	assert_int_equal(result, ISC_R_SUCCESS);

	dns_name_fromstring(name, "sub.example.com.", NULL, 0, NULL);
	dns_badcache_flushname(bc, name);

	result = dns_badcache_find(bc, name, dns_rdatatype_aaaa, &flags, now);
	assert_int_equal(result, ISC_R_NOTFOUND);
	result = dns_badcache_find(bc, name, dns_rdatatype_a, &flags, now);
	assert_int_equal(result, ISC_R_NOTFOUND);

	dns_name_fromstring(name, "sub.sub.example.com.", NULL, 0, NULL);
	result = dns_badcache_find(bc, name, dns_rdatatype_aaaa, &flags, now);
	assert_int_equal(result, ISC_R_SUCCESS);

	dns_name_fromstring(name, "example.com.", NULL, 0, NULL);
	result = dns_badcache_find(bc, name, dns_rdatatype_aaaa, &flags, now);
	assert_int_equal(result, ISC_R_SUCCESS);

	dns_badcache_destroy(&bc);

	isc_loopmgr_shutdown(loopmgr);
}

ISC_LOOP_TEST_IMPL(flushtree) {
	dns_badcache_t *bc = NULL;
	dns_fixedname_t fname = { 0 };
	dns_name_t *name = dns_fixedname_initname(&fname);
	isc_stdtime_t now = isc_stdtime_now();
	isc_result_t result;
	uint32_t flags = BADCACHE_TEST_FLAG;

	bc = dns_badcache_new(mctx, loopmgr);

	dns_name_fromstring(name, "example.com.", NULL, 0, NULL);
	dns_badcache_add(bc, name, dns_rdatatype_aaaa, flags, now + 60);
	result = dns_badcache_find(bc, name, dns_rdatatype_aaaa, &flags, now);
	assert_int_equal(result, ISC_R_SUCCESS);
	assert_int_equal(flags, BADCACHE_TEST_FLAG);

	dns_name_fromstring(name, "sub.example.com.", NULL, 0, NULL);
	dns_badcache_add(bc, name, dns_rdatatype_aaaa, flags, now + 60);
	result = dns_badcache_find(bc, name, dns_rdatatype_aaaa, &flags, now);
	assert_int_equal(result, ISC_R_SUCCESS);
	assert_int_equal(flags, BADCACHE_TEST_FLAG);

	dns_name_fromstring(name, "sub.sub.example.com.", NULL, 0, NULL);
	dns_badcache_add(bc, name, dns_rdatatype_aaaa, flags, now + 60);
	result = dns_badcache_find(bc, name, dns_rdatatype_aaaa, &flags, now);
	assert_int_equal(result, ISC_R_SUCCESS);
	assert_int_equal(flags, BADCACHE_TEST_FLAG);

	dns_name_fromstring(name, "sub.example.com.", NULL, 0, NULL);
	dns_badcache_flushtree(bc, name);

	dns_name_fromstring(name, "sub.sub.example.com.", NULL, 0, NULL);
	result = dns_badcache_find(bc, name, dns_rdatatype_aaaa, &flags, now);
	assert_int_equal(result, ISC_R_NOTFOUND);

	dns_name_fromstring(name, "sub.example.com.", NULL, 0, NULL);
	result = dns_badcache_find(bc, name, dns_rdatatype_aaaa, &flags, now);
	assert_int_equal(result, ISC_R_NOTFOUND);

	dns_name_fromstring(name, "example.com.", NULL, 0, NULL);
	result = dns_badcache_find(bc, name, dns_rdatatype_aaaa, &flags, now);
	assert_int_equal(result, ISC_R_SUCCESS);
	assert_int_equal(flags, BADCACHE_TEST_FLAG);

	dns_badcache_destroy(&bc);

	isc_loopmgr_shutdown(loopmgr);
}

ISC_LOOP_TEST_IMPL(purge) {
	dns_badcache_t *bc = NULL;
	dns_fixedname_t fname = { 0 };
	dns_name_t *name = dns_fixedname_initname(&fname);
	isc_stdtime_t now = isc_stdtime_now();
	isc_result_t result;
	uint32_t flags = BADCACHE_TEST_FLAG;

	bc = dns_badcache_new(mctx, loopmgr);

	dns_name_fromstring(name, "example.com.", NULL, 0, NULL);
	dns_badcache_add(bc, name, dns_rdatatype_aaaa, flags, now);
	result = dns_badcache_find(bc, name, dns_rdatatype_aaaa, &flags,
				   now - 60);
	assert_int_equal(result, ISC_R_SUCCESS);

	dns_name_fromstring(name, "sub.example.com.", NULL, 0, NULL);
	dns_badcache_add(bc, name, dns_rdatatype_aaaa, flags, now);
	result = dns_badcache_find(bc, name, dns_rdatatype_aaaa, &flags,
				   now - 60);
	assert_int_equal(result, ISC_R_SUCCESS);

	dns_name_fromstring(name, "sub.sub.example.com.", NULL, 0, NULL);
	dns_badcache_add(bc, name, dns_rdatatype_aaaa, flags, now);
	result = dns_badcache_find(bc, name, dns_rdatatype_aaaa, &flags,
				   now - 60);
	assert_int_equal(result, ISC_R_SUCCESS);

	result = dns_badcache_find(bc, name, dns_rdatatype_aaaa, &flags,
				   now + 30);
	assert_int_equal(result, ISC_R_NOTFOUND);

	dns_name_fromstring(name, "sub.sub.example.com.", NULL, 0, NULL);
	result = dns_badcache_find(bc, name, dns_rdatatype_aaaa, &flags,
				   now + 30);
	assert_int_equal(result, ISC_R_NOTFOUND);

	dns_name_fromstring(name, "sub.example.com.", NULL, 0, NULL);
	result = dns_badcache_find(bc, name, dns_rdatatype_aaaa, &flags,
				   now + 30);
	assert_int_equal(result, ISC_R_NOTFOUND);

	dns_name_fromstring(name, "example.com.", NULL, 0, NULL);
	result = dns_badcache_find(bc, name, dns_rdatatype_aaaa, &flags,
				   now + 30);
	assert_int_equal(result, ISC_R_NOTFOUND);

	dns_badcache_destroy(&bc);

	isc_loopmgr_shutdown(loopmgr);
}

ISC_TEST_LIST_START
ISC_TEST_ENTRY_CUSTOM(basic, setup_managers, teardown_managers)
ISC_TEST_ENTRY_CUSTOM(expire, setup_managers, teardown_managers)
ISC_TEST_ENTRY_CUSTOM(print, setup_managers, teardown_managers)
ISC_TEST_ENTRY_CUSTOM(flush, setup_managers, teardown_managers)
ISC_TEST_ENTRY_CUSTOM(flushname, setup_managers, teardown_managers)
ISC_TEST_ENTRY_CUSTOM(flushtree, setup_managers, teardown_managers)
ISC_TEST_ENTRY_CUSTOM(purge, setup_managers, teardown_managers)
ISC_TEST_LIST_END

ISC_TEST_MAIN
