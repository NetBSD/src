/*	$NetBSD: strerror.c,v 1.2 2018/08/12 13:02:39 christos Exp $	*/

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

#include <stdio.h>
#include <string.h>

#include <isc/mutex.h>
#include <isc/once.h>
#include <isc/print.h>
#include <isc/strerror.h>
#include <isc/util.h>

#ifdef HAVE_STRERROR
/*%
 * We need to do this this way for profiled locks.
 */
static isc_mutex_t isc_strerror_lock;
static void init_lock(void) {
	RUNTIME_CHECK(isc_mutex_init(&isc_strerror_lock) == ISC_R_SUCCESS);
}
#else
extern const char * const sys_errlist[];
extern const int sys_nerr;
#endif

void
isc__strerror(int num, char *buf, size_t size) {
#ifdef HAVE_STRERROR
	char *msg;
	unsigned int unum = (unsigned int)num;
	static isc_once_t once = ISC_ONCE_INIT;

	REQUIRE(buf != NULL);

	RUNTIME_CHECK(isc_once_do(&once, init_lock) == ISC_R_SUCCESS);

	LOCK(&isc_strerror_lock);
	msg = strerror(num);
	if (msg != NULL)
		snprintf(buf, size, "%s", msg);
	else
		snprintf(buf, size, "Unknown error: %u", unum);
	UNLOCK(&isc_strerror_lock);
#else
	unsigned int unum = (unsigned int)num;

	REQUIRE(buf != NULL);

	if (num >= 0 && num < sys_nerr)
		snprintf(buf, size, "%s", sys_errlist[num]);
	else
		snprintf(buf, size, "Unknown error: %u", unum);
#endif
}
