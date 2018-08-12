/*	$NetBSD: dh_test.c,v 1.1.1.1 2018/08/12 12:08:21 christos Exp $	*/

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


/* ! \file */

#include <config.h>

#include <atf-c.h>

#include <unistd.h>

#include <isc/util.h>
#include <isc/string.h>

#include <pk11/site.h>

#include <dns/name.h>
#include <dst/result.h>

#include "../dst_internal.h"

#include "dnstest.h"

#if defined(OPENSSL) && !defined(PK11_DH_DISABLE)

ATF_TC(isc_dh_computesecret);
ATF_TC_HEAD(isc_dh_computesecret, tc) {
	atf_tc_set_md_var(tc, "descr", "OpenSSL DH_compute_key() failure");
}
ATF_TC_BODY(isc_dh_computesecret, tc) {
	dst_key_t *key = NULL;
	isc_buffer_t buf;
	unsigned char array[1024];
	isc_result_t ret;
	dns_fixedname_t fname;
	dns_name_t *name;

	UNUSED(tc);

	ret = dns_test_begin(NULL, ISC_FALSE);
	ATF_REQUIRE_EQ(ret, ISC_R_SUCCESS);

	name = dns_fixedname_initname(&fname);
	isc_buffer_constinit(&buf, "dh.", 3);
	isc_buffer_add(&buf, 3);
	ret = dns_name_fromtext(name, &buf, NULL, 0, NULL);
	ATF_REQUIRE_EQ(ret, ISC_R_SUCCESS);

	ret = dst_key_fromfile(name, 18602, DST_ALG_DH,
			       DST_TYPE_PUBLIC | DST_TYPE_KEY,
			       "./", mctx, &key);
	ATF_REQUIRE_EQ(ret, ISC_R_SUCCESS);

	isc_buffer_init(&buf, array, sizeof(array));
	ret = dst_key_computesecret(key, key, &buf);
	ATF_REQUIRE_EQ(ret, DST_R_NOTPRIVATEKEY);
	ret = key->func->computesecret(key, key, &buf);
	ATF_REQUIRE_EQ(ret, DST_R_COMPUTESECRETFAILURE);

	dst_key_free(&key);
	dns_test_end();
}
#else
ATF_TC(untested);
ATF_TC_HEAD(untested, tc) {
	atf_tc_set_md_var(tc, "descr", "skipping OpenSSL DH test");
}
ATF_TC_BODY(untested, tc) {
	UNUSED(tc);
	atf_tc_skip("OpenSSL DH not compiled in");
}
#endif
/*
 * Main
 */
ATF_TP_ADD_TCS(tp) {
#if defined(OPENSSL) && !defined(PK11_DH_DISABLE)
	ATF_TP_ADD_TC(tp, isc_dh_computesecret);
#else
	ATF_TP_ADD_TC(tp, untested);
#endif
	return (atf_no_error());
}
