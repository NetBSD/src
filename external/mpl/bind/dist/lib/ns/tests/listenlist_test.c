/*	$NetBSD: listenlist_test.c,v 1.1.1.1 2018/08/12 12:08:07 christos Exp $	*/

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

/*! \file */

#include <config.h>

#include <atf-c.h>

#include <stdio.h>
#include <unistd.h>

#include <isc/list.h>
#include <isc/print.h>

#include <dns/acl.h>

#include <ns/listenlist.h>

#include "nstest.h"

/*
 * Helper functions
 */

ATF_TC(ns_listenlist_default);
ATF_TC_HEAD(ns_listenlist_default, tc) {
	atf_tc_set_md_var(tc, "descr", "test that ns_listenlist_default works");
}
ATF_TC_BODY(ns_listenlist_default, tc) {
	isc_result_t result;
	ns_listenlist_t *list = NULL;
	ns_listenelt_t *elt;
	int count;

	UNUSED(tc);

	result = ns_test_begin(NULL, ISC_FALSE);
	ATF_REQUIRE_EQ(result, ISC_R_SUCCESS);

	result = ns_listenlist_default(mctx, 5300, -1, ISC_FALSE, &list);
	ATF_REQUIRE_EQ(result, ISC_R_SUCCESS);
	ATF_REQUIRE(list != NULL);

	ATF_CHECK(!ISC_LIST_EMPTY(list->elts));

	count = 0;
	elt = ISC_LIST_HEAD(list->elts);
	while (elt != NULL) {
		ns_listenelt_t *next = ISC_LIST_NEXT(elt, link);
		dns_acl_t *acl = NULL;

		dns_acl_attach(elt->acl, &acl);
		ISC_LIST_UNLINK(list->elts, elt, link);
		ns_listenelt_destroy(elt);
		elt = next;

		ATF_CHECK(dns_acl_isnone(acl));
		dns_acl_detach(&acl);
		count++;
	}

	ATF_CHECK(ISC_LIST_EMPTY(list->elts));
	ATF_CHECK_EQ(count, 1);

	ns_listenlist_detach(&list);

	result = ns_listenlist_default(mctx, 5300, -1, ISC_TRUE, &list);
	ATF_CHECK_EQ(result, ISC_R_SUCCESS);

	ATF_CHECK(!ISC_LIST_EMPTY(list->elts));

	/* This time just use ns_listenlist_detach() to destroy elements */
	count = 0;
	elt = ISC_LIST_HEAD(list->elts);
	while (elt != NULL) {
		ns_listenelt_t *next = ISC_LIST_NEXT(elt, link);
		ATF_CHECK(dns_acl_isany(elt->acl));
		elt = next;
		count++;
	}

	ATF_CHECK_EQ(count, 1);

	ns_listenlist_detach(&list);

	ns_test_end();
}

/*
 * Main
 */
ATF_TP_ADD_TCS(tp) {
	ATF_TP_ADD_TC(tp, ns_listenlist_default);
	return (atf_no_error());
}
