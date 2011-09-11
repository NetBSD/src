/*	$NetBSD: zonemgr_test.c,v 1.1.1.1 2011/09/11 17:19:03 christos Exp $	*/

/*
 * Copyright (C) 2011  Internet Systems Consortium, Inc. ("ISC")
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND ISC DISCLAIMS ALL WARRANTIES WITH
 * REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS.  IN NO EVENT SHALL ISC BE LIABLE FOR ANY SPECIAL, DIRECT,
 * INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
 * LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE
 * OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */

/* Id: zonemgr_test.c,v 1.2 2011-07-06 05:05:51 each Exp */

/*! \file */

#include <config.h>

#include <atf-c.h>

#include <unistd.h>

#include <isc/buffer.h>
#include <isc/task.h>
#include <isc/timer.h>

#include <dns/name.h>
#include <dns/view.h>
#include <dns/zone.h>

#include "dnstest.h"

static isc_result_t
make_zone(const char *name, dns_zone_t **zonep) {
	isc_result_t result;
	dns_view_t *view = NULL;
	dns_zone_t *zone = NULL;
	isc_buffer_t buffer;
	dns_fixedname_t fixorigin;
	dns_name_t *origin;

	CHECK(dns_view_create(mctx, dns_rdataclass_in, "view", &view));
	CHECK(dns_zone_create(&zone, mctx));

	isc_buffer_init(&buffer, name, strlen(name));
	isc_buffer_add(&buffer, strlen(name));
	dns_fixedname_init(&fixorigin);
	origin = dns_fixedname_name(&fixorigin);
	CHECK(dns_name_fromtext(origin, &buffer, dns_rootname, 0, NULL));
	CHECK(dns_zone_setorigin(zone, origin));
	dns_zone_setview(zone, view);
	dns_zone_settype(zone, dns_zone_master);
	dns_zone_setclass(zone, view->rdclass);

	dns_view_detach(&view);
	*zonep = zone;

	return (ISC_R_SUCCESS);

  cleanup:
	if (zone != NULL)
		dns_zone_detach(&zone);
	if (view != NULL)
		dns_view_detach(&view);
	return (result);
}

/*
 * Individual unit tests
 */
ATF_TC(zonemgr_create);
ATF_TC_HEAD(zonemgr_create, tc) {
	atf_tc_set_md_var(tc, "descr", "create zone manager");
}
ATF_TC_BODY(zonemgr_create, tc) {
	dns_zonemgr_t *zonemgr = NULL;
	isc_result_t result;

	UNUSED(tc);

	result = dns_test_begin(NULL, ISC_TRUE);
	ATF_REQUIRE_EQ(result, ISC_R_SUCCESS);

	result = dns_zonemgr_create(mctx, taskmgr, timermgr, socketmgr,
				    &zonemgr);
	ATF_REQUIRE_EQ(result, ISC_R_SUCCESS);

	dns_zonemgr_shutdown(zonemgr);
	dns_zonemgr_detach(&zonemgr);
	ATF_REQUIRE_EQ(zonemgr, NULL);

	dns_test_end();
}


ATF_TC(zonemgr_managezone);
ATF_TC_HEAD(zonemgr_managezone, tc) {
	atf_tc_set_md_var(tc, "descr", "manage and release a zone");
}
ATF_TC_BODY(zonemgr_managezone, tc) {
	dns_zonemgr_t *zonemgr = NULL;
	dns_zone_t *zone = NULL;
	isc_result_t result;

	UNUSED(tc);

	result = dns_test_begin(NULL, ISC_TRUE);
	ATF_REQUIRE_EQ(result, ISC_R_SUCCESS);

	result = dns_zonemgr_create(mctx, taskmgr, timermgr, socketmgr,
				    &zonemgr);
	ATF_REQUIRE_EQ(result, ISC_R_SUCCESS);

	result = make_zone("foo", &zone);
	ATF_REQUIRE_EQ(result, ISC_R_SUCCESS);

	/* This should not succeed until the dns_zonemgr_setsize() is run */
	result = dns_zonemgr_managezone(zonemgr, zone);
	ATF_REQUIRE_EQ(result, ISC_R_FAILURE);

	ATF_REQUIRE_EQ(dns_zonemgr_getcount(zonemgr, DNS_ZONESTATE_ANY), 0);

	result = dns_zonemgr_setsize(zonemgr, 1);
	ATF_REQUIRE_EQ(result, ISC_R_SUCCESS);

	/* Now it should succeed */
	result = dns_zonemgr_managezone(zonemgr, zone);
	ATF_REQUIRE_EQ(result, ISC_R_SUCCESS);

	ATF_REQUIRE_EQ(dns_zonemgr_getcount(zonemgr, DNS_ZONESTATE_ANY), 1);

	dns_zonemgr_releasezone(zonemgr, zone);
	dns_zone_detach(&zone);

	ATF_REQUIRE_EQ(dns_zonemgr_getcount(zonemgr, DNS_ZONESTATE_ANY), 0);

	dns_zonemgr_shutdown(zonemgr);
	dns_zonemgr_detach(&zonemgr);
	ATF_REQUIRE_EQ(zonemgr, NULL);

	dns_test_end();
}

/*
 * Main
 */
ATF_TP_ADD_TCS(tp) {
	ATF_TP_ADD_TC(tp, zonemgr_create);
	ATF_TP_ADD_TC(tp, zonemgr_managezone);
	return (atf_no_error());
}

/*
 * XXX:
 * dns_zonemgr API calls that are not yet part of this unit test:
 *
 * 	- dns_zonemgr_attach
 * 	- dns_zonemgr_forcemaint
 * 	- dns_zonemgr_resumexfrs
 * 	- dns_zonemgr_shutdown
 * 	- dns_zonemgr_setsize
 * 	- dns_zonemgr_settransfersin
 * 	- dns_zonemgr_getttransfersin
 * 	- dns_zonemgr_settransfersperns
 * 	- dns_zonemgr_getttransfersperns
 * 	- dns_zonemgr_setiolimit
 * 	- dns_zonemgr_getiolimit
 * 	- dns_zonemgr_dbdestroyed
 * 	- dns_zonemgr_setserialqueryrate
 * 	- dns_zonemgr_getserialqueryrate
 * 	- dns_zonemgr_unreachable
 * 	- dns_zonemgr_unreachableadd
 */
