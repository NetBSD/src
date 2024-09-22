/*	$NetBSD: os.c,v 1.3 2024/09/22 00:14:08 christos Exp $	*/

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

#if defined(HAVE_SCHED_GETAFFINITY)

#if defined(HAVE_SCHED_H)
#include <sched.h>
#endif

/*
 * Administrators may wish to constrain the set of cores that BIND runs
 * on via the 'taskset' or 'numactl' programs (or equivalent on other
 * O/S), for example to achieve higher (or more stable) performance by
 * more closely associating threads with individual NIC rx queues. If
 * the admin has used taskset, it follows that BIND ought to
 * automatically use the given number of CPUs rather than the system
 * wide count.
 */
static int
sched_affinity_ncpus(void) {
	cpu_set_t cpus;
	int result;

	result = sched_getaffinity(0, sizeof(cpus), &cpus);
	if (result != -1) {
#ifdef CPU_COUNT
		return (CPU_COUNT(&cpus));
#else
		int i, n = 0;

		for (i = 0; i < CPU_SETSIZE; ++i) {
			if (CPU_ISSET(i, &cpus))
				++n;
		}
		return (n);
#endif
	}
	return (0);
}
#endif

/*
 * Affinity detecting variant of sched_affinity_cpus() for FreeBSD
 */

#if defined(HAVE_SYS_CPUSET_H) && defined(HAVE_CPUSET_GETAFFINITY)
#include <sys/cpuset.h>
#include <sys/param.h>

static int
cpuset_affinity_ncpus(void) {
	cpuset_t cpus;
	int result;

	result = cpuset_getaffinity(CPU_LEVEL_WHICH, CPU_WHICH_PID, -1,
				    sizeof(cpus), &cpus);
	if (result != -1) {
		int i, n = 0;
		for (i = 0; i < CPU_SETSIZE; ++i) {
			if (CPU_ISSET(i, &cpus))
				++n;
		}
		return (n);
	}
	return (0);
}
#endif

static void
ncpus_initialize(void) {
#if defined(HAVE_SYS_CPUSET_H) && defined(HAVE_CPUSET_GETAFFINITY)
	if (isc__os_ncpus <= 0) {
		isc__os_ncpus = cpuset_affinity_ncpus();
	}
#endif
#if defined(HAVE_SCHED_GETAFFINITY)
	if (isc__os_ncpus <= 0) {
		isc__os_ncpus = sched_affinity_ncpus();
	}
#endif
#if defined(HAVE_SYSCONF)
	if (isc__os_ncpus <= 0) {
		isc__os_ncpus = sysconf_ncpus();
	}
#endif /* if defined(HAVE_SYSCONF) */
#if defined(HAVE_SYS_SYSCTL_H) && defined(HAVE_SYSCTLBYNAME)
	if (isc__os_ncpus <= 0) {
		isc__os_ncpus = sysctl_ncpus();
	}
#endif /* if defined(HAVE_SYS_SYSCTL_H) && defined(HAVE_SYSCTLBYNAME) */
	if (isc__os_ncpus <= 0) {
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
