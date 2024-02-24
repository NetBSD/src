/*	$NetBSD: os.c,v 1.1.2.2 2024/02/24 13:07:30 martin Exp $	*/

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

#include <isc/os.h>

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

unsigned int
isc_os_ncpus(void) {
	long ncpus = 0;

#if defined(HAVE_SYSCONF)
	ncpus = sysconf_ncpus();
#endif /* if defined(HAVE_SYSCONF) */
#if defined(HAVE_SYS_SYSCTL_H) && defined(HAVE_SYSCTLBYNAME)
	if (ncpus <= 0) {
		ncpus = sysctl_ncpus();
	}
#endif /* if defined(HAVE_SYS_SYSCTL_H) && defined(HAVE_SYSCTLBYNAME) */
	if (ncpus <= 0) {
		ncpus = 1;
	}

	return ((unsigned int)ncpus);
}
