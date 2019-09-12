/*	$NetBSD: errno_test.c,v 1.3.4.1 2019/09/12 19:18:16 martin Exp $	*/

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

#include <config.h>

#if HAVE_CMOCKA

#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>

#include <errno.h>
#include <stdlib.h>
#include <string.h>

#define UNIT_TESTING
#include <cmocka.h>

#include <isc/errno.h>
#include <isc/result.h>
#include <isc/util.h>

typedef struct {
	int err;
	isc_result_t result;
} testpair_t;

testpair_t testpair[] = {
	{ EPERM, ISC_R_NOPERM },
	{ ENOENT, ISC_R_FILENOTFOUND },
	{ EIO, ISC_R_IOERROR },
	{ EBADF, ISC_R_INVALIDFILE },
	{ ENOMEM, ISC_R_NOMEMORY },
	{ EACCES, ISC_R_NOPERM },
	{ EEXIST, ISC_R_FILEEXISTS },
	{ ENOTDIR, ISC_R_INVALIDFILE },
	{ EINVAL, ISC_R_INVALIDFILE },
	{ ENFILE, ISC_R_TOOMANYOPENFILES },
	{ EMFILE, ISC_R_TOOMANYOPENFILES },
	{ EPIPE, ISC_R_CONNECTIONRESET },
	{ ENAMETOOLONG, ISC_R_INVALIDFILE },
	{ ELOOP, ISC_R_INVALIDFILE },
#ifdef EOVERFLOW
	{ EOVERFLOW, ISC_R_RANGE },
#endif
#ifdef EAFNOSUPPORT
	{ EAFNOSUPPORT, ISC_R_FAMILYNOSUPPORT },
#endif
#ifdef EADDRINUSE
	{ EADDRINUSE, ISC_R_ADDRINUSE },
#endif
	{ EADDRNOTAVAIL, ISC_R_ADDRNOTAVAIL },
#ifdef ENETDOWN
	{ ENETDOWN, ISC_R_NETDOWN },
#endif
#ifdef ENETUNREACH
	{ ENETUNREACH, ISC_R_NETUNREACH },
#endif
#ifdef ECONNABORTED
	{ ECONNABORTED, ISC_R_CONNECTIONRESET },
#endif
#ifdef ECONNRESET
	{ ECONNRESET, ISC_R_CONNECTIONRESET },
#endif
#ifdef ENOBUFS
	{ ENOBUFS, ISC_R_NORESOURCES },
#endif
#ifdef ENOTCONN
	{ ENOTCONN, ISC_R_NOTCONNECTED },
#endif
#ifdef ETIMEDOUT
	{ ETIMEDOUT, ISC_R_TIMEDOUT },
#endif
	{ ECONNREFUSED, ISC_R_CONNREFUSED },
#ifdef EHOSTDOWN
	{ EHOSTDOWN, ISC_R_HOSTDOWN },
#endif
#ifdef EHOSTUNREACH
	{ EHOSTUNREACH, ISC_R_HOSTUNREACH },
#endif
	{ 0, ISC_R_UNEXPECTED }
};

/* convert errno to ISC result */
static void
isc_errno_toresult_test(void **state) {
	isc_result_t result, expect;
	size_t i;

	UNUSED(state);

	for (i = 0; i < sizeof(testpair)/sizeof(testpair[0]); i++) {
		result = isc_errno_toresult(testpair[i].err);
		expect = testpair[i].result;
		assert_int_equal(result, expect);
	}
}

int
main(void) {
	const struct CMUnitTest tests[] = {
		cmocka_unit_test(isc_errno_toresult_test),
	};

	return (cmocka_run_group_tests(tests, NULL, NULL));
}

#else /* HAVE_CMOCKA */

#include <stdio.h>

int
main(void) {
	printf("1..0 # Skipped: cmocka not available\n");
	return (0);
}

#endif
