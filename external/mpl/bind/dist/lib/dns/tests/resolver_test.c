/*	$NetBSD: resolver_test.c,v 1.1.1.1 2018/08/12 12:08:20 christos Exp $	*/

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

#include <unistd.h>

#include <isc/app.h>
#include <isc/buffer.h>
#include <isc/socket.h>
#include <isc/task.h>
#include <isc/timer.h>

#include <dns/dispatch.h>
#include <dns/name.h>
#include <dns/resolver.h>
#include <dns/view.h>

#include "dnstest.h"

static dns_dispatchmgr_t *dispatchmgr = NULL;
static dns_dispatch_t *dispatch = NULL;
static dns_view_t *view = NULL;


static void
setup(void) {
	isc_result_t result;
	isc_sockaddr_t local;

	result = dns_test_begin(NULL, ISC_TRUE);
	ATF_REQUIRE_EQ(result, ISC_R_SUCCESS);

	result = dns_dispatchmgr_create(mctx, NULL, &dispatchmgr);
	ATF_REQUIRE_EQ(result, ISC_R_SUCCESS);

	result = dns_test_makeview("view", &view);
	ATF_REQUIRE_EQ(result, ISC_R_SUCCESS);

	isc_sockaddr_any(&local);
	result = dns_dispatch_getudp(dispatchmgr, socketmgr, taskmgr, &local,
				     4096, 100, 100, 100, 500, 0, 0,
				     &dispatch);
	ATF_REQUIRE_EQ(result, ISC_R_SUCCESS);
}

static void
teardown(void) {
	dns_dispatch_detach(&dispatch);
	dns_view_detach(&view);
	dns_dispatchmgr_destroy(&dispatchmgr);
	dns_test_end();
}


static void
mkres(dns_resolver_t **resolverp) {
	isc_result_t result;

	result = dns_resolver_create(view, taskmgr, 1, 1,
			    socketmgr, timermgr, 0,
			    dispatchmgr, dispatch, NULL, resolverp);
	ATF_REQUIRE_EQ(result, ISC_R_SUCCESS);
}

static void
destroy_resolver(dns_resolver_t **resolverp) {
	dns_resolver_shutdown(*resolverp);
	dns_resolver_detach(resolverp);
}

ATF_TC(create);
ATF_TC_HEAD(create, tc) {
	atf_tc_set_md_var(tc, "descr", "dns_resolver_create");
}
ATF_TC_BODY(create, tc) {
	dns_resolver_t *resolver = NULL;

	UNUSED(tc);

	setup();
	mkres(&resolver);
	destroy_resolver(&resolver);
	teardown();
}

ATF_TC(gettimeout);
ATF_TC_HEAD(gettimeout, tc) {
	atf_tc_set_md_var(tc, "descr", "dns_resolver_gettimeout");
}
ATF_TC_BODY(gettimeout, tc) {
	dns_resolver_t *resolver = NULL;
	unsigned int timeout;

	UNUSED(tc);

	setup();
	mkres(&resolver);

	timeout = dns_resolver_gettimeout(resolver);
	ATF_CHECK(timeout > 0);

	destroy_resolver(&resolver);
	teardown();
}

ATF_TC(settimeout);
ATF_TC_HEAD(settimeout, tc) {
	atf_tc_set_md_var(tc, "descr", "dns_resolver_settimeout");
}
ATF_TC_BODY(settimeout, tc) {
	dns_resolver_t *resolver = NULL;
	unsigned int default_timeout, timeout;

	UNUSED(tc);

	setup();

	mkres(&resolver);

	default_timeout = dns_resolver_gettimeout(resolver);
	dns_resolver_settimeout(resolver, default_timeout + 1);
	timeout = dns_resolver_gettimeout(resolver);
	ATF_CHECK(timeout == default_timeout + 1);

	destroy_resolver(&resolver);
	teardown();
}

ATF_TC(settimeout_default);
ATF_TC_HEAD(settimeout_default, tc) {
	atf_tc_set_md_var(tc, "descr", "dns_resolver_settimeout to default");
}
ATF_TC_BODY(settimeout_default, tc) {
	dns_resolver_t *resolver = NULL;
	unsigned int default_timeout, timeout;

	UNUSED(tc);

	setup();

	mkres(&resolver);

	default_timeout = dns_resolver_gettimeout(resolver);
	dns_resolver_settimeout(resolver, default_timeout + 100);

	timeout = dns_resolver_gettimeout(resolver);
	ATF_CHECK_EQ(timeout, default_timeout + 100);

	dns_resolver_settimeout(resolver, 0);
	timeout = dns_resolver_gettimeout(resolver);
	ATF_CHECK_EQ(timeout, default_timeout);

	destroy_resolver(&resolver);
	teardown();
}

ATF_TC(settimeout_belowmin);
ATF_TC_HEAD(settimeout_belowmin, tc) {
	atf_tc_set_md_var(tc, "descr",
			  "dns_resolver_settimeout below minimum");
}
ATF_TC_BODY(settimeout_belowmin, tc) {
	dns_resolver_t *resolver = NULL;
	unsigned int default_timeout, timeout;

	UNUSED(tc);

	setup();

	mkres(&resolver);

	default_timeout = dns_resolver_gettimeout(resolver);
	dns_resolver_settimeout(resolver, 9000);

	timeout = dns_resolver_gettimeout(resolver);
	ATF_CHECK_EQ(timeout, default_timeout);

	destroy_resolver(&resolver);
	teardown();
}

ATF_TC(settimeout_overmax);
ATF_TC_HEAD(settimeout_overmax, tc) {
	atf_tc_set_md_var(tc, "descr", "dns_resolver_settimeout over maximum");
}
ATF_TC_BODY(settimeout_overmax, tc) {
	dns_resolver_t *resolver = NULL;
	unsigned int timeout;

	UNUSED(tc);

	setup();

	mkres(&resolver);

	dns_resolver_settimeout(resolver, 4000000);
	timeout = dns_resolver_gettimeout(resolver);
	ATF_CHECK(timeout < 4000000 && timeout > 0);

	destroy_resolver(&resolver);
	teardown();
}

/*
 * Main
 */
ATF_TP_ADD_TCS(tp) {
	ATF_TP_ADD_TC(tp, create);
	ATF_TP_ADD_TC(tp, gettimeout);
	ATF_TP_ADD_TC(tp, settimeout);
	ATF_TP_ADD_TC(tp, settimeout_default);
	ATF_TP_ADD_TC(tp, settimeout_belowmin);
	ATF_TP_ADD_TC(tp, settimeout_overmax);
	return (atf_no_error());
}
