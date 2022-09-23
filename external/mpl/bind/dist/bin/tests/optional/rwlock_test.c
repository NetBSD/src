/*	$NetBSD: rwlock_test.c,v 1.7 2022/09/23 12:15:23 christos Exp $	*/

/*
 * Copyright (C) Internet Systems Consortium, Inc. ("ISC")
 *
 * SPDX-License-Identifier: MPL-2.0
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0.  If a copy of the MPL was not distributed with this
 * file, you can obtain one at https://mozilla.org/MPL/2.0/.
 *
 * See the COPYRIGHT file distributed with this work for additional
 * information regarding copyright ownership.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <isc/print.h>
#include <isc/rwlock.h>
#include <isc/string.h>
#include <isc/thread.h>
#include <isc/util.h>

#ifdef WIN32
#define sleep(x) Sleep(1000 * x)
#endif /* ifdef WIN32 */

isc_rwlock_t lock;

static isc_threadresult_t
#ifdef WIN32
	WINAPI
#endif /* ifdef WIN32 */
	run1(void *arg) {
	char *message = arg;

	RUNTIME_CHECK(isc_rwlock_lock(&lock, isc_rwlocktype_read) ==
		      ISC_R_SUCCESS);
	printf("%s got READ lock\n", message);
	sleep(1);
	printf("%s giving up READ lock\n", message);
	RUNTIME_CHECK(isc_rwlock_unlock(&lock, isc_rwlocktype_read) ==
		      ISC_R_SUCCESS);
	RUNTIME_CHECK(isc_rwlock_lock(&lock, isc_rwlocktype_read) ==
		      ISC_R_SUCCESS);
	printf("%s got READ lock\n", message);
	sleep(1);
	printf("%s giving up READ lock\n", message);
	RUNTIME_CHECK(isc_rwlock_unlock(&lock, isc_rwlocktype_read) ==
		      ISC_R_SUCCESS);
	RUNTIME_CHECK(isc_rwlock_lock(&lock, isc_rwlocktype_write) ==
		      ISC_R_SUCCESS);
	printf("%s got WRITE lock\n", message);
	sleep(1);
	printf("%s giving up WRITE lock\n", message);
	RUNTIME_CHECK(isc_rwlock_unlock(&lock, isc_rwlocktype_write) ==
		      ISC_R_SUCCESS);
	return ((isc_threadresult_t)0);
}

static isc_threadresult_t
#ifdef WIN32
	WINAPI
#endif /* ifdef WIN32 */
	run2(void *arg) {
	char *message = arg;

	RUNTIME_CHECK(isc_rwlock_lock(&lock, isc_rwlocktype_write) ==
		      ISC_R_SUCCESS);
	printf("%s got WRITE lock\n", message);
	sleep(1);
	printf("%s giving up WRITE lock\n", message);
	RUNTIME_CHECK(isc_rwlock_unlock(&lock, isc_rwlocktype_write) ==
		      ISC_R_SUCCESS);
	RUNTIME_CHECK(isc_rwlock_lock(&lock, isc_rwlocktype_write) ==
		      ISC_R_SUCCESS);
	printf("%s got WRITE lock\n", message);
	sleep(1);
	printf("%s giving up WRITE lock\n", message);
	RUNTIME_CHECK(isc_rwlock_unlock(&lock, isc_rwlocktype_write) ==
		      ISC_R_SUCCESS);
	RUNTIME_CHECK(isc_rwlock_lock(&lock, isc_rwlocktype_read) ==
		      ISC_R_SUCCESS);
	printf("%s got READ lock\n", message);
	sleep(1);
	printf("%s giving up READ lock\n", message);
	RUNTIME_CHECK(isc_rwlock_unlock(&lock, isc_rwlocktype_read) ==
		      ISC_R_SUCCESS);
	return ((isc_threadresult_t)0);
}

int
main(int argc, char *argv[]) {
	unsigned int nworkers;
	unsigned int i;
	isc_thread_t workers[100];
	char name[100];
	void *dupname;

	if (argc > 1) {
		nworkers = atoi(argv[1]);
	} else {
		nworkers = 2;
	}
	if (nworkers > 100) {
		nworkers = 100;
	}
	printf("%u workers\n", nworkers);

	isc_rwlock_init(&lock, 5, 10);

	for (i = 0; i < nworkers; i++) {
		snprintf(name, sizeof(name), "%02u", i);
		dupname = strdup(name);
		RUNTIME_CHECK(dupname != NULL);
		if (i != 0 && i % 3 == 0) {
			isc_thread_create(run1, dupname, &workers[i]);
		} else {
			isc_thread_create(run2, dupname, &workers[i]);
		}
	}

	for (i = 0; i < nworkers; i++) {
		isc_thread_join(workers[i], NULL);
	}

	isc_rwlock_destroy(&lock);

	return (0);
}
