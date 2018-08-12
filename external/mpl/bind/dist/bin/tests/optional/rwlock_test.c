/*	$NetBSD: rwlock_test.c,v 1.2 2018/08/12 13:02:29 christos Exp $	*/

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

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <isc/print.h>
#include <isc/thread.h>
#include <isc/rwlock.h>
#include <isc/string.h>
#include <isc/util.h>

#ifdef WIN32
#define sleep(x)	Sleep(1000 * x)
#endif

#ifdef ISC_PLATFORM_USETHREADS

isc_rwlock_t lock;

static isc_threadresult_t
#ifdef WIN32
WINAPI
#endif
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
#endif
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

	if (argc > 1)
		nworkers = atoi(argv[1]);
	else
		nworkers = 2;
	if (nworkers > 100)
		nworkers = 100;
	printf("%u workers\n", nworkers);

	RUNTIME_CHECK(isc_rwlock_init(&lock, 5, 10) == ISC_R_SUCCESS);

	for (i = 0; i < nworkers; i++) {
		snprintf(name, sizeof(name), "%02u", i);
		dupname = strdup(name);
		RUNTIME_CHECK(dupname != NULL);
		if (i != 0 && i % 3 == 0)
			RUNTIME_CHECK(isc_thread_create(run1, dupname,
							&workers[i]) ==
			       ISC_R_SUCCESS);
		else
			RUNTIME_CHECK(isc_thread_create(run2, dupname,
							&workers[i]) ==
			       ISC_R_SUCCESS);
	}

	for (i = 0; i < nworkers; i++)
		(void)isc_thread_join(workers[i], NULL);

	isc_rwlock_destroy(&lock);

	return (0);
}

#else

int
main(int argc, char *argv[]) {
	UNUSED(argc);
	UNUSED(argv);
	fprintf(stderr, "This test requires threads.\n");
	return(1);
}

#endif
