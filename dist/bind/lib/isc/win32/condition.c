/*	$NetBSD: condition.c,v 1.1.1.3.4.1 2007/05/17 00:42:54 jdc Exp $	*/

/*
 * Copyright (C) 2004, 2006  Internet Systems Consortium, Inc. ("ISC")
 * Copyright (C) 1998-2001  Internet Software Consortium.
 *
 * Permission to use, copy, modify, and distribute this software for any
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

/* Id: condition.c,v 1.18.18.2 2006/02/13 23:50:53 marka Exp */

#include <config.h>

#include <isc/condition.h>
#include <isc/assertions.h>
#include <isc/util.h>
#include <isc/time.h>

#define LSIGNAL		0
#define LBROADCAST	1

isc_result_t
isc_condition_init(isc_condition_t *cond) {
	HANDLE h;

	REQUIRE(cond != NULL);

	cond->waiters = 0;
	h = CreateEvent(NULL, FALSE, FALSE, NULL);
	if (h == NULL) {
		/* XXX */
		return (ISC_R_UNEXPECTED);
	}
	cond->events[LSIGNAL] = h;
	h = CreateEvent(NULL, TRUE, FALSE, NULL);
	if (h == NULL) {
		(void)CloseHandle(cond->events[LSIGNAL]);
		/* XXX */
		return (ISC_R_UNEXPECTED);
	}
	cond->events[LBROADCAST] = h;

	return (ISC_R_SUCCESS);
}

isc_result_t
isc_condition_signal(isc_condition_t *cond) {

	/*
	 * Unlike pthreads, the caller MUST hold the lock associated with
	 * the condition variable when calling us.
	 */
	REQUIRE(cond != NULL);

	if (cond->waiters > 0 &&
	    !SetEvent(cond->events[LSIGNAL])) {
		/* XXX */
		return (ISC_R_UNEXPECTED);
	}

	return (ISC_R_SUCCESS);
}

isc_result_t
isc_condition_broadcast(isc_condition_t *cond) {

	/*
	 * Unlike pthreads, the caller MUST hold the lock associated with
	 * the condition variable when calling us.
	 */
	REQUIRE(cond != NULL);

	if (cond->waiters > 0 &&
	    !SetEvent(cond->events[LBROADCAST])) {
		/* XXX */
		return (ISC_R_UNEXPECTED);
	}

	return (ISC_R_SUCCESS);
}

isc_result_t
isc_condition_destroy(isc_condition_t *cond) {

	REQUIRE(cond != NULL);
	REQUIRE(cond->waiters == 0);

	(void)CloseHandle(cond->events[LSIGNAL]);
	(void)CloseHandle(cond->events[LBROADCAST]);

	return (ISC_R_SUCCESS);
}

/*
 * This is always called when the mutex (lock) is held, but because
 * we are waiting we need to release it and reacquire it as soon as the wait
 * is over. This allows other threads to make use of the object guarded
 * by the mutex but it should never try to delete it as long as the
 * number of waiters > 0. Always reacquire the mutex regardless of the
 * result of the wait. Note that EnterCriticalSection will wait to acquire
 * the mutex.
 */
static isc_result_t
wait(isc_condition_t *cond, isc_mutex_t *mutex, DWORD milliseconds) {
	DWORD result;

	cond->waiters++;
	LeaveCriticalSection(mutex);
	result = WaitForMultipleObjects(2, cond->events, FALSE, milliseconds);
	EnterCriticalSection(mutex);
	cond->waiters--;
	if (result == WAIT_FAILED) {
		/* XXX */
		return (ISC_R_UNEXPECTED);
	}
	if (cond->waiters == 0 &&
	    !ResetEvent(cond->events[LBROADCAST])) {
		/* XXX */
		return (ISC_R_UNEXPECTED);
	}

	if (result == WAIT_TIMEOUT)
		return (ISC_R_TIMEDOUT);

	return (ISC_R_SUCCESS);
}

isc_result_t
isc_condition_wait(isc_condition_t *cond, isc_mutex_t *mutex) {
	return (wait(cond, mutex, INFINITE));
}

isc_result_t
isc_condition_waituntil(isc_condition_t *cond, isc_mutex_t *mutex,
			isc_time_t *t) {
	DWORD milliseconds;
	isc_uint64_t microseconds;
	isc_time_t now;

	if (isc_time_now(&now) != ISC_R_SUCCESS) {
		/* XXX */
		return (ISC_R_UNEXPECTED);
	}

	microseconds = isc_time_microdiff(t, &now);
	if (microseconds > 0xFFFFFFFFi64 * 1000)
		milliseconds = 0xFFFFFFFF;
	else
		milliseconds = (DWORD)(microseconds / 1000);

	return (wait(cond, mutex, milliseconds));
}
