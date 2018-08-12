/*	$NetBSD: os.c,v 1.2 2018/08/12 13:02:39 christos Exp $	*/

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

#include <isc/os.h>


#ifdef HAVE_SYSCONF

#include <unistd.h>

#ifndef __hpux
static inline long
sysconf_ncpus(void) {
#if defined(_SC_NPROCESSORS_ONLN)
	return sysconf((_SC_NPROCESSORS_ONLN));
#elif defined(_SC_NPROC_ONLN)
	return sysconf((_SC_NPROC_ONLN));
#else
	return (0);
#endif
}
#endif
#endif /* HAVE_SYSCONF */


#ifdef __hpux

#include <sys/pstat.h>

static inline int
hpux_ncpus(void) {
	struct pst_dynamic psd;
	if (pstat_getdynamic(&psd, sizeof(psd), 1, 0) != -1)
		return (psd.psd_proc_cnt);
	else
		return (0);
}

#endif /* __hpux */

#if defined(HAVE_SYS_SYSCTL_H) && defined(HAVE_SYSCTLBYNAME)
#include <sys/types.h>  /* for FreeBSD */
#include <sys/param.h>  /* for NetBSD */
#include <sys/sysctl.h>

static int
sysctl_ncpus(void) {
	int ncpu, result;
	size_t len;

	len = sizeof(ncpu);
	result = sysctlbyname("hw.ncpu", &ncpu, &len , 0, 0);
	if (result != -1)
		return (ncpu);
	return (0);
}
#endif

unsigned int
isc_os_ncpus(void) {
	long ncpus = 0;

#ifdef __hpux
	ncpus = hpux_ncpus();
#elif defined(HAVE_SYSCONF)
	ncpus = sysconf_ncpus();
#endif
#if defined(HAVE_SYS_SYSCTL_H) && defined(HAVE_SYSCTLBYNAME)
	if (ncpus <= 0)
		ncpus = sysctl_ncpus();
#endif
	if (ncpus <= 0)
		ncpus = 1;

	return ((unsigned int)ncpus);
}
