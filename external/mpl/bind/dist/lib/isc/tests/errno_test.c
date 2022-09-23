/*	$NetBSD: errno_test.c,v 1.8 2022/09/23 12:15:34 christos Exp $	*/

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

#if HAVE_CMOCKA

#include <errno.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
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

testpair_t testpair[] = { { EPERM, ISC_R_NOPERM },
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
#endif /* ifdef EOVERFLOW */
#ifdef EAFNOSUPPORT
			  { EAFNOSUPPORT, ISC_R_FAMILYNOSUPPORT },
#endif /* ifdef EAFNOSUPPORT */
#ifdef EADDRINUSE
			  { EADDRINUSE, ISC_R_ADDRINUSE },
#endif /* ifdef EADDRINUSE */
			  { EADDRNOTAVAIL, ISC_R_ADDRNOTAVAIL },
#ifdef ENETDOWN
			  { ENETDOWN, ISC_R_NETDOWN },
#endif /* ifdef ENETDOWN */
#ifdef ENETUNREACH
			  { ENETUNREACH, ISC_R_NETUNREACH },
#endif /* ifdef ENETUNREACH */
#ifdef ECONNABORTED
			  { ECONNABORTED, ISC_R_CONNECTIONRESET },
#endif /* ifdef ECONNABORTED */
#ifdef ECONNRESET
			  { ECONNRESET, ISC_R_CONNECTIONRESET },
#endif /* ifdef ECONNRESET */
#ifdef ENOBUFS
			  { ENOBUFS, ISC_R_NORESOURCES },
#endif /* ifdef ENOBUFS */
#ifdef ENOTCONN
			  { ENOTCONN, ISC_R_NOTCONNECTED },
#endif /* ifdef ENOTCONN */
#ifdef ETIMEDOUT
			  { ETIMEDOUT, ISC_R_TIMEDOUT },
#endif /* ifdef ETIMEDOUT */
			  { ECONNREFUSED, ISC_R_CONNREFUSED },
#ifdef EHOSTDOWN
			  { EHOSTDOWN, ISC_R_HOSTDOWN },
#endif /* ifdef EHOSTDOWN */
#ifdef EHOSTUNREACH
			  { EHOSTUNREACH, ISC_R_HOSTUNREACH },
#endif /* ifdef EHOSTUNREACH */
			  { 0, ISC_R_UNEXPECTED } };

/* convert errno to ISC result */
static void
isc_errno_toresult_test(void **state) {
	isc_result_t result, expect;
	size_t i;

	UNUSED(state);

	for (i = 0; i < sizeof(testpair) / sizeof(testpair[0]); i++) {
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
	return (SKIPPED_TEST_EXIT_CODE);
}

#endif /* if HAVE_CMOCKA */
