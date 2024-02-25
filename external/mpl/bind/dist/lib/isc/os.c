/*	$NetBSD: os.c,v 1.2.2.2 2024/02/25 15:47:17 martin Exp $	*/

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

#include <inttypes.h>
#include <sys/stat.h>

#include <isc/os.h>
#include <isc/types.h>
#include <isc/util.h>

#include "os_p.h"

static unsigned int isc__os_ncpus = 0;
static unsigned long isc__os_cacheline = ISC_OS_CACHELINE_SIZE;
static mode_t isc__os_umask = 0;

#ifdef HAVE_SYSCONF

#include <unistd.h>

static long
sysconf_ncpus(void) {
#if defined(_SC_NPROCESSORS_ONLN)
	return (sysconf((_SC_NPROCESSORS_ONLN)));
#elif defined(_SC_NPROC_ONLN)
	return (sysconf((_SC_NPROC_ONLN)));
#else  /* if defined(_SC_NPROCESSORS_ONLN) */
	return (0);
#endif /* if defined(_SC_NPROCESSORS_ONLN) */
}
#endif /* HAVE_SYSCONF */

#if defined(HAVE_SYS_SYSCTL_H) && defined(HAVE_SYSCTLBYNAME)
#include <sys/param.h> /* for NetBSD */
#include <sys/sysctl.h>
#include <sys/types.h> /* for FreeBSD */

static int
sysctl_ncpus(void) {
	int ncpu, result;
	size_t len;

	len = sizeof(ncpu);
	result = sysctlbyname("hw.ncpu", &ncpu, &len, 0, 0);
	if (result != -1) {
		return (ncpu);
	}
	return (0);
}
#endif /* if defined(HAVE_SYS_SYSCTL_H) && defined(HAVE_SYSCTLBYNAME) */

static void
ncpus_initialize(void) {
#if defined(HAVE_SYSCONF)
	isc__os_ncpus = sysconf_ncpus();
#endif /* if defined(HAVE_SYSCONF) */
#if defined(HAVE_SYS_SYSCTL_H) && defined(HAVE_SYSCTLBYNAME)
	if (isc__os_ncpus <= 0) {
		isc__os_ncpus = sysctl_ncpus();
	}
#endif /* if defined(HAVE_SYS_SYSCTL_H) && defined(HAVE_SYSCTLBYNAME) */
	if (isc__os_ncpus == 0) {
		isc__os_ncpus = 1;
	}
}

static void
umask_initialize(void) {
	isc__os_umask = umask(0);
	(void)umask(isc__os_umask);
}

unsigned int
isc_os_ncpus(void) {
	return (isc__os_ncpus);
}

unsigned long
isc_os_cacheline(void) {
	return (isc__os_cacheline);
}

mode_t
isc_os_umask(void) {
	return (isc__os_umask);
}

void
isc__os_initialize(void) {
	umask_initialize();
	ncpus_initialize();
#if defined(HAVE_SYSCONF) && defined(_SC_LEVEL1_DCACHE_LINESIZE)
	long s = sysconf(_SC_LEVEL1_DCACHE_LINESIZE);
	if (s > 0 && (unsigned long)s > isc__os_cacheline) {
		isc__os_cacheline = s;
	}
#endif
}

void
isc__os_shutdown(void) {
	/* empty, but defined for completeness */;
}
