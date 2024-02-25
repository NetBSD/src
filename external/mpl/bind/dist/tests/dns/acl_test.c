/*	$NetBSD: acl_test.c,v 1.2.2.2 2024/02/25 15:47:39 martin Exp $	*/

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

#include <isc/print.h>
#include <isc/string.h>
#include <isc/util.h>

#include <dns/acl.h>

#include <tests/dns.h>

#define BUFLEN	    255
#define BIGBUFLEN   (70 * 1024)
#define TEST_ORIGIN "test"

/* test that dns_acl_isinsecure works */
ISC_RUN_TEST_IMPL(dns_acl_isinsecure) {
	isc_result_t result;
	dns_acl_t *any = NULL;
	dns_acl_t *none = NULL;
	dns_acl_t *notnone = NULL;
	dns_acl_t *notany = NULL;
#if defined(HAVE_GEOIP2)
	dns_acl_t *geoip = NULL;
	dns_acl_t *notgeoip = NULL;
	dns_aclelement_t *de;
#endif /* HAVE_GEOIP2 */

	UNUSED(state);

	result = dns_acl_any(mctx, &any);
	assert_int_equal(result, ISC_R_SUCCESS);

	result = dns_acl_none(mctx, &none);
	assert_int_equal(result, ISC_R_SUCCESS);

	result = dns_acl_create(mctx, 1, &notnone);
	assert_int_equal(result, ISC_R_SUCCESS);

	result = dns_acl_create(mctx, 1, &notany);
	assert_int_equal(result, ISC_R_SUCCESS);

	result = dns_acl_merge(notnone, none, false);
	assert_int_equal(result, ISC_R_SUCCESS);

	result = dns_acl_merge(notany, any, false);
	assert_int_equal(result, ISC_R_SUCCESS);

#if defined(HAVE_GEOIP2)
	result = dns_acl_create(mctx, 1, &geoip);
	assert_int_equal(result, ISC_R_SUCCESS);

	de = geoip->elements;
	assert_non_null(de);
	strlcpy(de->geoip_elem.as_string, "AU",
		sizeof(de->geoip_elem.as_string));
	de->geoip_elem.subtype = dns_geoip_country_code;
	de->type = dns_aclelementtype_geoip;
	de->negative = false;
	assert_true(geoip->length < geoip->alloc);
	dns_acl_node_count(geoip)++;
	de->node_num = dns_acl_node_count(geoip);
	geoip->length++;

	result = dns_acl_create(mctx, 1, &notgeoip);
	assert_int_equal(result, ISC_R_SUCCESS);

	result = dns_acl_merge(notgeoip, geoip, false);
	assert_int_equal(result, ISC_R_SUCCESS);
#endif /* HAVE_GEOIP2 */

	assert_true(dns_acl_isinsecure(any));	   /* any; */
	assert_false(dns_acl_isinsecure(none));	   /* none; */
	assert_false(dns_acl_isinsecure(notany));  /* !any; */
	assert_false(dns_acl_isinsecure(notnone)); /* !none; */

#if defined(HAVE_GEOIP2)
	assert_true(dns_acl_isinsecure(geoip));	    /* geoip; */
	assert_false(dns_acl_isinsecure(notgeoip)); /* !geoip; */
#endif						    /* HAVE_GEOIP2 */

	dns_acl_detach(&any);
	dns_acl_detach(&none);
	dns_acl_detach(&notany);
	dns_acl_detach(&notnone);
#if defined(HAVE_GEOIP2)
	dns_acl_detach(&geoip);
	dns_acl_detach(&notgeoip);
#endif /* HAVE_GEOIP2 */
}

ISC_TEST_LIST_START
ISC_TEST_ENTRY(dns_acl_isinsecure)
ISC_TEST_LIST_END

ISC_TEST_MAIN
