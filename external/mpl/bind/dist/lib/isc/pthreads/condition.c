/*	$NetBSD: condition.c,v 1.1.1.1 2018/08/12 12:08:28 christos Exp $	*/

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

#include <errno.h>

#include <isc/condition.h>
#include <isc/msgs.h>
#include <isc/strerror.h>
#include <isc/string.h>
#include <isc/time.h>
#include <isc/util.h>

isc_result_t
isc_condition_waituntil(isc_condition_t *c, isc_mutex_t *m, isc_time_t *t) {
	int presult;
	isc_result_t result;
	struct timespec ts;
	char strbuf[ISC_STRERRORSIZE];

	REQUIRE(c != NULL && m != NULL && t != NULL);

	/*
	 * POSIX defines a timespec's tv_sec as time_t.
	 */
	result = isc_time_secondsastimet(t, &ts.tv_sec);

	/*
	 * If we have a range error ts.tv_sec is most probably a signed
	 * 32 bit value.  Set ts.tv_sec to INT_MAX.  This is a kludge.
	 */
	if (result == ISC_R_RANGE)
		ts.tv_sec = INT_MAX;
	else if (result != ISC_R_SUCCESS)
		return (result);

	/*!
	 * POSIX defines a timespec's tv_nsec as long.  isc_time_nanoseconds
	 * ensures its return value is < 1 billion, which will fit in a long.
	 */
	ts.tv_nsec = (long)isc_time_nanoseconds(t);

	do {
#if ISC_MUTEX_PROFILE
		presult = pthread_cond_timedwait(c, &m->mutex, &ts);
#else
		presult = pthread_cond_timedwait(c, m, &ts);
#endif
		if (presult == 0)
			return (ISC_R_SUCCESS);
		if (presult == ETIMEDOUT)
			return (ISC_R_TIMEDOUT);
	} while (presult == EINTR);

	isc__strerror(presult, strbuf, sizeof(strbuf));
	UNEXPECTED_ERROR(__FILE__, __LINE__,
			 "pthread_cond_timedwait() %s %s",
			 isc_msgcat_get(isc_msgcat, ISC_MSGSET_GENERAL,
					ISC_MSG_RETURNED, "returned"),
			 strbuf);
	return (ISC_R_UNEXPECTED);
}
