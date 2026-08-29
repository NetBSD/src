/*	$NetBSD: url_test.c,v 1.1.1.1 2026/08/29 14:32:15 christos Exp $	*/

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

#include <inttypes.h>
#include <sched.h> /* IWYU pragma: keep */
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define UNIT_TESTING
#include <cmocka.h>

#include <isc/mem.h>
#include <isc/url.h>
#include <isc/util.h>

#include <tests/isc.h>

typedef struct {
	const char *uri;
	isc_result_t result;
	/* Expected components (checked only when result is ISC_R_SUCCESS). */
	const char *scheme;
	const char *userinfo;
	const char *host;
	const char *port;
	const char *path;
	const char *query;
	const char *fragment;
} url_testcase_t;

/*
 * Extract the substring of component 'uf' from 'buf' and compare it with
 * 'expected'. A field that is not set is treated as the empty string.
 */
static void
check_field(const isc_url_parser_t *up, const char *buf, isc_url_field_t uf,
	    const char *expected) {
	char got[256] = { 0 };

	if ((up->field_set & (1 << uf)) != 0) {
		uint16_t off = up->field_data[uf].off;
		uint16_t len = up->field_data[uf].len;

		INSIST(len < sizeof(got));
		memmove(got, buf + off, len);
	}

	assert_string_equal(got, expected);
}

static void
run_url_testcases(const url_testcase_t *cases, size_t ncases) {
	for (size_t i = 0; i < ncases; i++) {
		const url_testcase_t *tc = &cases[i];
		isc_url_parser_t up = { 0 };
		isc_result_t result = isc_url_parse(tc->uri, strlen(tc->uri),
						    false, &up);

		assert_int_equal(result, tc->result);
		if (result != ISC_R_SUCCESS) {
			continue;
		}

		check_field(&up, tc->uri, ISC_UF_SCHEMA, tc->scheme);
		check_field(&up, tc->uri, ISC_UF_USERINFO, tc->userinfo);
		check_field(&up, tc->uri, ISC_UF_HOST, tc->host);
		check_field(&up, tc->uri, ISC_UF_PORT, tc->port);
		check_field(&up, tc->uri, ISC_UF_PATH, tc->path);
		check_field(&up, tc->uri, ISC_UF_QUERY, tc->query);
		check_field(&up, tc->uri, ISC_UF_FRAGMENT, tc->fragment);
	}
}

ISC_RUN_TEST_IMPL(parse) {
	isc_result_t result;
	isc_url_parser_t up = { 0 };
	const char *url = NULL;

	/* Test an empty buffer. */
	url = "";
	result = isc_url_parse(url, strlen(url), false, &up);
	assert_int_equal(ISC_R_RANGE, result);

	/* Test a buffer with a valid URI. */
	url = "http://user:pass@example.com:8080/test?a=b&c=d";
	result = isc_url_parse(url, strlen(url), false, &up);
	assert_int_equal(ISC_R_SUCCESS, result);
	assert_int_equal(0, up.field_data[ISC_UF_SCHEMA].off);
	assert_int_equal(4, up.field_data[ISC_UF_SCHEMA].len);
	assert_int_equal(17, up.field_data[ISC_UF_HOST].off);
	assert_int_equal(11, up.field_data[ISC_UF_HOST].len);
	assert_int_equal(29, up.field_data[ISC_UF_PORT].off);
	assert_int_equal(4, up.field_data[ISC_UF_PORT].len);
	assert_int_equal(33, up.field_data[ISC_UF_PATH].off);
	assert_int_equal(5, up.field_data[ISC_UF_PATH].len);
	assert_int_equal(39, up.field_data[ISC_UF_QUERY].off);
	assert_int_equal(7, up.field_data[ISC_UF_QUERY].len);
	assert_int_equal(0, up.field_data[ISC_UF_FRAGMENT].off);
	assert_int_equal(0, up.field_data[ISC_UF_FRAGMENT].len);
	assert_int_equal(7, up.field_data[ISC_UF_USERINFO].off);
	assert_int_equal(9, up.field_data[ISC_UF_USERINFO].len);

	/*
	 * Test the maximum accepted buffer length (URL_MAX_LENGTH). A path of
	 * exactly URL_MAX_LENGTH bytes also drives ISC_UF_PATH.len to its
	 * maximum without overflowing.
	 */
	size_t max_len = URL_MAX_LENGTH;
	char *max_buf = isc_mem_get(mctx, max_len);
	memset(max_buf, 'a', max_len);
	max_buf[0] = '/';
	result = isc_url_parse(max_buf, max_len, false, &up);
	isc_mem_put(mctx, max_buf, max_len);
	assert_int_equal(ISC_R_SUCCESS, result);
	assert_int_equal(0, up.field_data[ISC_UF_PATH].off);
	assert_int_equal(URL_MAX_LENGTH, up.field_data[ISC_UF_PATH].len);

	/* Test a too big buffer (URL_MAX_LENGTH + 1). */
	size_t buf_len = URL_MAX_LENGTH + 2;
	char *buf = isc_mem_get(mctx, buf_len);
	memset(buf, 'a', buf_len);
	buf[0] = '/';
	result = isc_url_parse(buf, URL_MAX_LENGTH + 1, false, &up);
	isc_mem_put(mctx, buf, buf_len);
	assert_int_equal(ISC_R_RANGE, result);
}

/*
 * isc_url_parse() splits an absolute URI / request target into components;
 * it does NOT resolve relative references or remove dot-segments
 * (RFC 3986 section 5). The inputs below are the base URI and a
 * representative subset of the resolved targets from RFC 3986 section 5.4,
 * parsed as standalone absolute URIs. Note in particular that '.'/'..' and
 * embedded slashes inside the query and fragment are preserved verbatim
 * rather than normalized.
 */
ISC_RUN_TEST_IMPL(parse_rfc3986) {
	static const url_testcase_t testcases[] = {
		/* The base URI used throughout RFC 3986 section 5.4. */
		{ "http://a/b/c/d;p?q", ISC_R_SUCCESS, "http", "", "a", "",
		  "/b/c/d;p", "q", "" },

		/* Normal examples (section 5.4.1). */
		{ "http://a/b/c/g", ISC_R_SUCCESS, "http", "", "a", "",
		  "/b/c/g", "", "" },
		{ "http://a/g", ISC_R_SUCCESS, "http", "", "a", "", "/g", "",
		  "" },
		{ "http://g", ISC_R_SUCCESS, "http", "", "g", "", "", "", "" },
		{ "http://a/", ISC_R_SUCCESS, "http", "", "a", "", "/", "",
		  "" },
		{ "http://a/b/c/;x", ISC_R_SUCCESS, "http", "", "a", "",
		  "/b/c/;x", "", "" },
		{ "http://a/b/c/g;x?y#s", ISC_R_SUCCESS, "http", "", "a", "",
		  "/b/c/g;x", "y", "s" },
		{ "http://a/b/c/d;p?q#s", ISC_R_SUCCESS, "http", "", "a", "",
		  "/b/c/d;p", "q", "s" },

		/*
		 * Abnormal examples (section 5.4.2): dot-segments and slashes
		 * embedded in the query and fragment are not normalized.
		 */
		{ "http://a/b/c/..g", ISC_R_SUCCESS, "http", "", "a", "",
		  "/b/c/..g", "", "" },
		{ "http://a/b/c/g..", ISC_R_SUCCESS, "http", "", "a", "",
		  "/b/c/g..", "", "" },
		{ "http://a/b/c/g?y/./x", ISC_R_SUCCESS, "http", "", "a", "",
		  "/b/c/g", "y/./x", "" },
		{ "http://a/b/c/g#s/../x", ISC_R_SUCCESS, "http", "", "a", "",
		  "/b/c/g", "", "s/../x" },

		/* The full generic-syntax example from RFC 3986 section 3. */
		{ "foo://example.com:8042/over/there?name=ferret#nose",
		  ISC_R_SUCCESS, "foo", "", "example.com", "8042",
		  "/over/there", "name=ferret", "nose" },

		/*
		 * Relative references cannot be parsed as request targets:
		 * a scheme requires an authority, and a bare reference has no
		 * host.
		 */
		{ "g:h", ISC_R_FAILURE, "", "", "", "", "", "", "" },
		{ "g", ISC_R_FAILURE, "", "", "", "", "", "", "" },
		{ "http:g", ISC_R_FAILURE, "", "", "", "", "", "", "" },
	};

	run_url_testcases(testcases, ARRAY_SIZE(testcases));
}

/*
 * Authority and IPv6 edge cases, drawn from the Addressable URI test suite.
 * isc_url_parse() is stricter than a generic RFC 3986 parser: it requires a
 * scheme followed by "://" and an authority, the host of an IPv6 literal
 * excludes the surrounding brackets, and it neither percent-decodes nor
 * case-normalizes any component.
 */
ISC_RUN_TEST_IMPL(parse_addressable) {
	static const url_testcase_t testcases[] = {
		/* IPv6 literal hosts: the brackets are stripped from the host.
		 */
		{ "http://[::1]/", ISC_R_SUCCESS, "http", "", "[::1]", "", "/",
		  "", "" },
		{ "http://[fe80::200:f8ff:fe21:67cf]/", ISC_R_SUCCESS, "http",
		  "", "[fe80::200:f8ff:fe21:67cf]", "", "/", "", "" },
		{ "http://[2001:db8::7]:8080/path", ISC_R_SUCCESS, "http", "",
		  "[2001:db8::7]", "8080", "/path", "", "" },
		{ "ldap://[2001:db8::7]/c=GB?objectClass?one", ISC_R_SUCCESS,
		  "ldap", "", "[2001:db8::7]", "", "/c=GB", "objectClass?one",
		  "" },
		/* An IPv6 zone identifier is kept verbatim (not decoded). */
		{ "http://[fe80::1%25en0]/", ISC_R_SUCCESS, "http", "",
		  "[fe80::1%25en0]", "", "/", "", "" },

		/* IPv4 host with an explicit port. */
		{ "telnet://192.0.2.16:80/", ISC_R_SUCCESS, "telnet", "",
		  "192.0.2.16", "80", "/", "", "" },

		/* Userinfo, with and without a password; case is preserved. */
		{ "http://user@example.com/", ISC_R_SUCCESS, "http", "user",
		  "example.com", "", "/", "", "" },
		{ "http://:@example.com/", ISC_R_SUCCESS, "http", ":",
		  "example.com", "", "/", "", "" },
		{ "HTTP://EXAMPLE.COM/", ISC_R_SUCCESS, "HTTP", "",
		  "EXAMPLE.COM", "", "/", "", "" },

		/* A trailing dot in the host is part of the host. */
		{ "http://example.com./", ISC_R_SUCCESS, "http", "",
		  "example.com.", "", "/", "", "" },

		/* No path component at all. */
		{ "http://example.com", ISC_R_SUCCESS, "http", "",
		  "example.com", "", "", "", "" },

		/* Path parameters stay in the path. */
		{ "http://example.com/file.txt;x=y", ISC_R_SUCCESS, "http", "",
		  "example.com", "", "/file.txt;x=y", "", "" },

		/*
		 * Rejected where a generic RFC 3986 parser would succeed: a '+'
		 * in the scheme, an IPvFuture literal, and a percent-encoded
		 * port.
		 */
		{ "svn+ssh://developername@rubyforge.org/var/svn/project",
		  ISC_R_FAILURE, "", "", "", "", "", "", "" },
		{ "http://[v9.3ffe:1900:4545:3:200:f8ff:fe21:67cf]/",
		  ISC_R_FAILURE, "", "", "", "", "", "", "" },
		{ "http://example.com:%38%30/", ISC_R_FAILURE, "", "", "", "",
		  "", "", "" },
	};

	run_url_testcases(testcases, ARRAY_SIZE(testcases));
}

ISC_TEST_LIST_START

ISC_TEST_ENTRY(parse)
ISC_TEST_ENTRY(parse_rfc3986)
ISC_TEST_ENTRY(parse_addressable)

ISC_TEST_LIST_END

ISC_TEST_MAIN
