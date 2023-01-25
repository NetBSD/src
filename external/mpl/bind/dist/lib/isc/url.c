/*	$NetBSD: url.c,v 1.4 2023/01/25 21:43:31 christos Exp $	*/

/*
 * Copyright (C) Internet Systems Consortium, Inc. ("ISC")
 *
 * SPDX-License-Identifier: MPL-2.0 and MIT
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, you can obtain one at https://mozilla.org/MPL/2.0/.
 *
 * See the COPYRIGHT file distributed with this work for additional
 * information regarding copyright ownership.
 */

/*
 * Copyright Joyent, Inc. and other Node contributors. All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

#include <ctype.h>
#include <limits.h>
#include <stddef.h>
#include <string.h>

#include <isc/url.h>
#include <isc/util.h>

#ifndef BIT_AT
#define BIT_AT(a, i)                                    \
	(!!((unsigned int)(a)[(unsigned int)(i) >> 3] & \
	    (1 << ((unsigned int)(i)&7))))
#endif

#if HTTP_PARSER_STRICT
#define T(v) 0
#else
#define T(v) v
#endif

static const uint8_t normal_url_char[32] = {
	/*   0 nul  1 soh  2 stx  3 etx  4 eot  5 enq  6 ack  7 bel  */
	0 | 0 | 0 | 0 | 0 | 0 | 0 | 0,
	/*   8 bs   9 ht  10 nl  11 vt  12 np  13 cr  14 so  15 si */
	0 | T(2) | 0 | 0 | T(16) | 0 | 0 | 0,
	/*  16 dle 17 dc1 18 dc2 19 dc3 20 dc4 21 nak 22 syn 23 etb */
	0 | 0 | 0 | 0 | 0 | 0 | 0 | 0,
	/*  24 can 25 em  26 sub 27 esc 28 fs  29 gs  30 rs  31 us */
	0 | 0 | 0 | 0 | 0 | 0 | 0 | 0,
	/*  32 sp  33  !  34  "  35  #  36  $  37  %  38  &  39  ' */
	0 | 2 | 4 | 0 | 16 | 32 | 64 | 128,
	/*  40  (  41  )  42  *  43  +  44  ,  45  -  46  .  47  / */
	1 | 2 | 4 | 8 | 16 | 32 | 64 | 128,
	/*  48  0  49  1  50  2  51  3  52  4  53  5  54  6  55  7 */
	1 | 2 | 4 | 8 | 16 | 32 | 64 | 128,
	/*  56  8  57  9  58  :  59  ;  60  <  61  =  62  >  63  ?  */
	1 | 2 | 4 | 8 | 16 | 32 | 64 | 0,
	/*  64  @  65  A  66  B  67  C  68  D  69  E  70  F  71  G */
	1 | 2 | 4 | 8 | 16 | 32 | 64 | 128,
	/*  72  H  73  I  74  J  75  K  76  L  77  M  78  N  79  O */
	1 | 2 | 4 | 8 | 16 | 32 | 64 | 128,
	/*  80  P  81  Q  82  R  83  S  84  T  85  U  86  V  87  W */
	1 | 2 | 4 | 8 | 16 | 32 | 64 | 128,
	/*  88  X  89  Y  90  Z  91  [  92  \  93  ]  94  ^  95  _ */
	1 | 2 | 4 | 8 | 16 | 32 | 64 | 128,
	/*  96  `  97  a  98  b  99  c 100  d 101  e 102  f 103  g */
	1 | 2 | 4 | 8 | 16 | 32 | 64 | 128,
	/* 104  h 105  i 106  j 107  k 108  l 109  m 110  n 111  o */
	1 | 2 | 4 | 8 | 16 | 32 | 64 | 128,
	/* 112  p 113  q 114  r 115  s 116  t 117  u 118  v 119  w */
	1 | 2 | 4 | 8 | 16 | 32 | 64 | 128,
	/* 120  x 121  y 122  z 123  { 124  | 125  } 126  ~ 127 del */
	1 | 2 | 4 | 8 | 16 | 32 | 64 | 0,
};

#undef T

typedef enum {
	s_dead = 1, /* important that this is > 0 */

	s_start_req_or_res,
	s_res_or_resp_H,
	s_start_res,
	s_res_H,
	s_res_HT,
	s_res_HTT,
	s_res_HTTP,
	s_res_http_major,
	s_res_http_dot,
	s_res_http_minor,
	s_res_http_end,
	s_res_first_status_code,
	s_res_status_code,
	s_res_status_start,
	s_res_status,
	s_res_line_almost_done,

	s_start_req,

	s_req_method,
	s_req_spaces_before_url,
	s_req_schema,
	s_req_schema_slash,
	s_req_schema_slash_slash,
	s_req_server_start,
	s_req_server,
	s_req_server_with_at,
	s_req_path,
	s_req_query_string_start,
	s_req_query_string,
	s_req_fragment_start,
	s_req_fragment,
	s_req_http_start,
	s_req_http_H,
	s_req_http_HT,
	s_req_http_HTT,
	s_req_http_HTTP,
	s_req_http_I,
	s_req_http_IC,
	s_req_http_major,
	s_req_http_dot,
	s_req_http_minor,
	s_req_http_end,
	s_req_line_almost_done,

	s_header_field_start,
	s_header_field,
	s_header_value_discard_ws,
	s_header_value_discard_ws_almost_done,
	s_header_value_discard_lws,
	s_header_value_start,
	s_header_value,
	s_header_value_lws,

	s_header_almost_done,

	s_chunk_size_start,
	s_chunk_size,
	s_chunk_parameters,
	s_chunk_size_almost_done,

	s_headers_almost_done,
	s_headers_done,

	/*
	 * Important: 's_headers_done' must be the last 'header' state. All
	 * states beyond this must be 'body' states. It is used for overflow
	 * checking. See the PARSING_HEADER() macro.
	 */

	s_chunk_data,
	s_chunk_data_almost_done,
	s_chunk_data_done,

	s_body_identity,
	s_body_identity_eof,

	s_message_done
} state_t;

typedef enum {
	s_http_host_dead = 1,
	s_http_userinfo_start,
	s_http_userinfo,
	s_http_host_start,
	s_http_host_v6_start,
	s_http_host,
	s_http_host_v6,
	s_http_host_v6_end,
	s_http_host_v6_zone_start,
	s_http_host_v6_zone,
	s_http_host_port_start,
	s_http_host_port
} host_state_t;

/* Macros for character classes; depends on strict-mode  */
#define IS_MARK(c)                                                             \
	((c) == '-' || (c) == '_' || (c) == '.' || (c) == '!' || (c) == '~' || \
	 (c) == '*' || (c) == '\'' || (c) == '(' || (c) == ')')
#define IS_USERINFO_CHAR(c)                                                    \
	(isalnum((unsigned char)c) || IS_MARK(c) || (c) == '%' ||              \
	 (c) == ';' || (c) == ':' || (c) == '&' || (c) == '=' || (c) == '+' || \
	 (c) == '$' || (c) == ',')

#if HTTP_PARSER_STRICT
#define IS_URL_CHAR(c)	(BIT_AT(normal_url_char, (unsigned char)c))
#define IS_HOST_CHAR(c) (isalnum((unsigned char)c) || (c) == '.' || (c) == '-')
#else
#define IS_URL_CHAR(c) (BIT_AT(normal_url_char, (unsigned char)c) || ((c)&0x80))
#define IS_HOST_CHAR(c) \
	(isalnum((unsigned char)c) || (c) == '.' || (c) == '-' || (c) == '_')
#endif

/*
 * Our URL parser.
 *
 * This is designed to be shared by http_parser_execute() for URL validation,
 * hence it has a state transition + byte-for-byte interface. In addition, it
 * is meant to be embedded in http_parser_parse_url(), which does the dirty
 * work of turning state transitions URL components for its API.
 *
 * This function should only be invoked with non-space characters. It is
 * assumed that the caller cares about (and can detect) the transition between
 * URL and non-URL states by looking for these.
 */
static state_t
parse_url_char(state_t s, const char ch) {
	if (ch == ' ' || ch == '\r' || ch == '\n') {
		return (s_dead);
	}

#if HTTP_PARSER_STRICT
	if (ch == '\t' || ch == '\f') {
		return (s_dead);
	}
#endif

	switch (s) {
	case s_req_spaces_before_url:
		/* Proxied requests are followed by scheme of an absolute URI
		 * (alpha). All methods except CONNECT are followed by '/' or
		 * '*'.
		 */

		if (ch == '/' || ch == '*') {
			return (s_req_path);
		}

		if (isalpha((unsigned char)ch)) {
			return (s_req_schema);
		}

		break;

	case s_req_schema:
		if (isalpha((unsigned char)ch)) {
			return (s);
		}

		if (ch == ':') {
			return (s_req_schema_slash);
		}

		break;

	case s_req_schema_slash:
		if (ch == '/') {
			return (s_req_schema_slash_slash);
		}

		break;

	case s_req_schema_slash_slash:
		if (ch == '/') {
			return (s_req_server_start);
		}

		break;

	case s_req_server_with_at:
		if (ch == '@') {
			return (s_dead);
		}

		FALLTHROUGH;
	case s_req_server_start:
	case s_req_server:
		if (ch == '/') {
			return (s_req_path);
		}

		if (ch == '?') {
			return (s_req_query_string_start);
		}

		if (ch == '@') {
			return (s_req_server_with_at);
		}

		if (IS_USERINFO_CHAR(ch) || ch == '[' || ch == ']') {
			return (s_req_server);
		}

		break;

	case s_req_path:
		if (IS_URL_CHAR(ch)) {
			return (s);
		}

		switch (ch) {
		case '?':
			return (s_req_query_string_start);

		case '#':
			return (s_req_fragment_start);
		}

		break;

	case s_req_query_string_start:
	case s_req_query_string:
		if (IS_URL_CHAR(ch)) {
			return (s_req_query_string);
		}

		switch (ch) {
		case '?':
			/* allow extra '?' in query string */
			return (s_req_query_string);

		case '#':
			return (s_req_fragment_start);
		}

		break;

	case s_req_fragment_start:
		if (IS_URL_CHAR(ch)) {
			return (s_req_fragment);
		}

		switch (ch) {
		case '?':
			return (s_req_fragment);

		case '#':
			return (s);
		}

		break;

	case s_req_fragment:
		if (IS_URL_CHAR(ch)) {
			return (s);
		}

		switch (ch) {
		case '?':
		case '#':
			return (s);
		}

		break;

	default:
		break;
	}

	/*
	 * We should never fall out of the switch above unless there's an
	 * error.
	 */
	return (s_dead);
}

static host_state_t
http_parse_host_char(host_state_t s, const char ch) {
	switch (s) {
	case s_http_userinfo:
	case s_http_userinfo_start:
		if (ch == '@') {
			return (s_http_host_start);
		}

		if (IS_USERINFO_CHAR(ch)) {
			return (s_http_userinfo);
		}
		break;

	case s_http_host_start:
		if (ch == '[') {
			return (s_http_host_v6_start);
		}

		if (IS_HOST_CHAR(ch)) {
			return (s_http_host);
		}

		break;

	case s_http_host:
		if (IS_HOST_CHAR(ch)) {
			return (s_http_host);
		}

		FALLTHROUGH;
	case s_http_host_v6_end:
		if (ch == ':') {
			return (s_http_host_port_start);
		}

		break;

	case s_http_host_v6:
		if (ch == ']') {
			return (s_http_host_v6_end);
		}

		FALLTHROUGH;
	case s_http_host_v6_start:
		if (isxdigit((unsigned char)ch) || ch == ':' || ch == '.') {
			return (s_http_host_v6);
		}

		if (s == s_http_host_v6 && ch == '%') {
			return (s_http_host_v6_zone_start);
		}
		break;

	case s_http_host_v6_zone:
		if (ch == ']') {
			return (s_http_host_v6_end);
		}

		FALLTHROUGH;
	case s_http_host_v6_zone_start:
		/* RFC 6874 Zone ID consists of 1*( unreserved / pct-encoded) */
		if (isalnum((unsigned char)ch) || ch == '%' || ch == '.' ||
		    ch == '-' || ch == '_' || ch == '~')
		{
			return (s_http_host_v6_zone);
		}
		break;

	case s_http_host_port:
	case s_http_host_port_start:
		if (isdigit((unsigned char)ch)) {
			return (s_http_host_port);
		}

		break;

	default:
		break;
	}

	return (s_http_host_dead);
}

static isc_result_t
http_parse_host(const char *buf, isc_url_parser_t *up, int found_at) {
	host_state_t s;
	const char *p = NULL;
	size_t buflen = up->field_data[ISC_UF_HOST].off +
			up->field_data[ISC_UF_HOST].len;

	REQUIRE((up->field_set & (1 << ISC_UF_HOST)) != 0);

	up->field_data[ISC_UF_HOST].len = 0;

	s = found_at ? s_http_userinfo_start : s_http_host_start;

	for (p = buf + up->field_data[ISC_UF_HOST].off; p < buf + buflen; p++) {
		host_state_t new_s = http_parse_host_char(s, *p);

		if (new_s == s_http_host_dead) {
			return (ISC_R_FAILURE);
		}

		switch (new_s) {
		case s_http_host:
			if (s != s_http_host) {
				up->field_data[ISC_UF_HOST].off =
					(uint16_t)(p - buf);
			}
			up->field_data[ISC_UF_HOST].len++;
			break;

		case s_http_host_v6:
			if (s != s_http_host_v6) {
				up->field_data[ISC_UF_HOST].off =
					(uint16_t)(p - buf);
			}
			up->field_data[ISC_UF_HOST].len++;
			break;

		case s_http_host_v6_zone_start:
		case s_http_host_v6_zone:
			up->field_data[ISC_UF_HOST].len++;
			break;

		case s_http_host_port:
			if (s != s_http_host_port) {
				up->field_data[ISC_UF_PORT].off =
					(uint16_t)(p - buf);
				up->field_data[ISC_UF_PORT].len = 0;
				up->field_set |= (1 << ISC_UF_PORT);
			}
			up->field_data[ISC_UF_PORT].len++;
			break;

		case s_http_userinfo:
			if (s != s_http_userinfo) {
				up->field_data[ISC_UF_USERINFO].off =
					(uint16_t)(p - buf);
				up->field_data[ISC_UF_USERINFO].len = 0;
				up->field_set |= (1 << ISC_UF_USERINFO);
			}
			up->field_data[ISC_UF_USERINFO].len++;
			break;

		default:
			break;
		}

		s = new_s;
	}

	/* Make sure we don't end somewhere unexpected */
	switch (s) {
	case s_http_host_start:
	case s_http_host_v6_start:
	case s_http_host_v6:
	case s_http_host_v6_zone_start:
	case s_http_host_v6_zone:
	case s_http_host_port_start:
	case s_http_userinfo:
	case s_http_userinfo_start:
		return (ISC_R_FAILURE);
	default:
		break;
	}

	return (ISC_R_SUCCESS);
}

isc_result_t
isc_url_parse(const char *buf, size_t buflen, bool is_connect,
	      isc_url_parser_t *up) {
	state_t s;
	isc_url_field_t uf, old_uf;
	int found_at = 0;
	const char *p = NULL;

	if (buflen == 0) {
		return (ISC_R_FAILURE);
	}

	up->port = up->field_set = 0;
	s = is_connect ? s_req_server_start : s_req_spaces_before_url;
	old_uf = ISC_UF_MAX;

	for (p = buf; p < buf + buflen; p++) {
		s = parse_url_char(s, *p);

		/* Figure out the next field that we're operating on */
		switch (s) {
		case s_dead:
			return (ISC_R_FAILURE);

		/* Skip delimiters */
		case s_req_schema_slash:
		case s_req_schema_slash_slash:
		case s_req_server_start:
		case s_req_query_string_start:
		case s_req_fragment_start:
			continue;

		case s_req_schema:
			uf = ISC_UF_SCHEMA;
			break;

		case s_req_server_with_at:
			found_at = 1;
			FALLTHROUGH;
		case s_req_server:
			uf = ISC_UF_HOST;
			break;

		case s_req_path:
			uf = ISC_UF_PATH;
			break;

		case s_req_query_string:
			uf = ISC_UF_QUERY;
			break;

		case s_req_fragment:
			uf = ISC_UF_FRAGMENT;
			break;

		default:
			UNREACHABLE();
		}

		/* Nothing's changed; soldier on */
		if (uf == old_uf) {
			up->field_data[uf].len++;
			continue;
		}

		up->field_data[uf].off = (uint16_t)(p - buf);
		up->field_data[uf].len = 1;

		up->field_set |= (1 << uf);
		old_uf = uf;
	}

	/* host must be present if there is a schema */
	/* parsing http:///toto will fail */
	if ((up->field_set & (1 << ISC_UF_SCHEMA)) &&
	    (up->field_set & (1 << ISC_UF_HOST)) == 0)
	{
		return (ISC_R_FAILURE);
	}

	if (up->field_set & (1 << ISC_UF_HOST)) {
		isc_result_t result;

		result = http_parse_host(buf, up, found_at);
		if (result != ISC_R_SUCCESS) {
			return (result);
		}
	}

	/* CONNECT requests can only contain "hostname:port" */
	if (is_connect &&
	    up->field_set != ((1 << ISC_UF_HOST) | (1 << ISC_UF_PORT)))
	{
		return (ISC_R_FAILURE);
	}

	if (up->field_set & (1 << ISC_UF_PORT)) {
		uint16_t off;
		uint16_t len;
		const char *pp = NULL;
		const char *end = NULL;
		unsigned long v;

		off = up->field_data[ISC_UF_PORT].off;
		len = up->field_data[ISC_UF_PORT].len;
		end = buf + off + len;

		/*
		 * NOTE: The characters are already validated and are in the
		 * [0-9] range
		 */
		INSIST(off + len <= buflen);

		v = 0;
		for (pp = buf + off; pp < end; pp++) {
			v *= 10;
			v += *pp - '0';

			/* Ports have a max value of 2^16 */
			if (v > 0xffff) {
				return (ISC_R_RANGE);
			}
		}

		up->port = (uint16_t)v;
	}

	return (ISC_R_SUCCESS);
}
