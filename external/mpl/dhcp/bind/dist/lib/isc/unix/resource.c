/*	$NetBSD: resource.c,v 1.1.2.2 2024/02/24 13:07:30 martin Exp $	*/

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
#include <stdbool.h>
#include <sys/resource.h>
#include <sys/time.h> /* Required on some systems for <sys/resource.h>. */
#include <sys/types.h>

#include <isc/platform.h>
#include <isc/resource.h>
#include <isc/result.h>
#include <isc/util.h>

#ifdef __linux__
#include <linux/fs.h> /* To get the large NR_OPEN. */
#endif		      /* ifdef __linux__ */

#include "errno2result.h"

static isc_result_t
resource2rlim(isc_resource_t resource, int *rlim_resource) {
	isc_result_t result = ISC_R_SUCCESS;

	switch (resource) {
	case isc_resource_coresize:
		*rlim_resource = RLIMIT_CORE;
		break;
	case isc_resource_cputime:
		*rlim_resource = RLIMIT_CPU;
		break;
	case isc_resource_datasize:
		*rlim_resource = RLIMIT_DATA;
		break;
	case isc_resource_filesize:
		*rlim_resource = RLIMIT_FSIZE;
		break;
	case isc_resource_lockedmemory:
#ifdef RLIMIT_MEMLOCK
		*rlim_resource = RLIMIT_MEMLOCK;
#else  /* ifdef RLIMIT_MEMLOCK */
		result = ISC_R_NOTIMPLEMENTED;
#endif /* ifdef RLIMIT_MEMLOCK */
		break;
	case isc_resource_openfiles:
#ifdef RLIMIT_NOFILE
		*rlim_resource = RLIMIT_NOFILE;
#else  /* ifdef RLIMIT_NOFILE */
		result = ISC_R_NOTIMPLEMENTED;
#endif /* ifdef RLIMIT_NOFILE */
		break;
	case isc_resource_processes:
#ifdef RLIMIT_NPROC
		*rlim_resource = RLIMIT_NPROC;
#else  /* ifdef RLIMIT_NPROC */
		result = ISC_R_NOTIMPLEMENTED;
#endif /* ifdef RLIMIT_NPROC */
		break;
	case isc_resource_residentsize:
#ifdef RLIMIT_RSS
		*rlim_resource = RLIMIT_RSS;
#else  /* ifdef RLIMIT_RSS */
		result = ISC_R_NOTIMPLEMENTED;
#endif /* ifdef RLIMIT_RSS */
		break;
	case isc_resource_stacksize:
		*rlim_resource = RLIMIT_STACK;
		break;
	default:
		/*
		 * This test is not very robust if isc_resource_t
		 * changes, but generates a clear assertion message.
		 */
		REQUIRE(resource >= isc_resource_coresize &&
			resource <= isc_resource_stacksize);

		result = ISC_R_RANGE;
		break;
	}

	return (result);
}

isc_result_t
isc_resource_setlimit(isc_resource_t resource, isc_resourcevalue_t value) {
	struct rlimit rl;
	rlim_t rlim_value;
	int unixresult;
	int unixresource;
	isc_result_t result;

	result = resource2rlim(resource, &unixresource);
	if (result != ISC_R_SUCCESS) {
		return (result);
	}

	if (value == ISC_RESOURCE_UNLIMITED) {
		rlim_value = RLIM_INFINITY;
	} else {
		/*
		 * isc_resourcevalue_t was chosen as an unsigned 64 bit
		 * integer so that it could contain the maximum range of
		 * reasonable values.  Unfortunately, this exceeds the typical
		 * range on Unix systems.  Ensure the range of
		 * rlim_t is not overflowed.
		 */
		isc_resourcevalue_t rlim_max;
		bool rlim_t_is_signed = (((double)(rlim_t)-1) < 0);

		if (rlim_t_is_signed) {
			rlim_max = ~((rlim_t)1 << (sizeof(rlim_t) * 8 - 1));
		} else {
			rlim_max = (rlim_t)-1;
		}

		if (value > rlim_max) {
			value = rlim_max;
		}

		rlim_value = value;
	}

	rl.rlim_cur = rl.rlim_max = rlim_value;
	unixresult = setrlimit(unixresource, &rl);

	if (unixresult == 0) {
		return (ISC_R_SUCCESS);
	}

#if defined(OPEN_MAX) && defined(__APPLE__)
	/*
	 * The Darwin kernel doesn't accept RLIM_INFINITY for rlim_cur; the
	 * maximum possible value is OPEN_MAX.  BIND8 used to use
	 * sysconf(_SC_OPEN_MAX) for such a case, but this value is much
	 * smaller than OPEN_MAX and is not really effective.
	 */
	if (resource == isc_resource_openfiles && rlim_value == RLIM_INFINITY) {
		rl.rlim_cur = OPEN_MAX;
		unixresult = setrlimit(unixresource, &rl);
		if (unixresult == 0) {
			return (ISC_R_SUCCESS);
		}
	}
#elif defined(__linux__)
#ifndef NR_OPEN
#define NR_OPEN (1024 * 1024)
#endif /* ifndef NR_OPEN */

	/*
	 * Some Linux kernels don't accept RLIM_INFINIT; the maximum
	 * possible value is the NR_OPEN defined in linux/fs.h.
	 */
	if (resource == isc_resource_openfiles && rlim_value == RLIM_INFINITY) {
		rl.rlim_cur = rl.rlim_max = NR_OPEN;
		unixresult = setrlimit(unixresource, &rl);
		if (unixresult == 0) {
			return (ISC_R_SUCCESS);
		}
	}
#endif /* if defined(OPEN_MAX) && defined(__APPLE__) */
	if (resource == isc_resource_openfiles && rlim_value == RLIM_INFINITY) {
		if (getrlimit(unixresource, &rl) == 0) {
			rl.rlim_cur = rl.rlim_max;
			unixresult = setrlimit(unixresource, &rl);
			if (unixresult == 0) {
				return (ISC_R_SUCCESS);
			}
		}
	}
	return (isc__errno2result(errno));
}

isc_result_t
isc_resource_getlimit(isc_resource_t resource, isc_resourcevalue_t *value) {
	int unixresource;
	struct rlimit rl;
	isc_result_t result;

	result = resource2rlim(resource, &unixresource);
	if (result != ISC_R_SUCCESS) {
		return (result);
	}

	if (getrlimit(unixresource, &rl) != 0) {
		return (isc__errno2result(errno));
	}

	*value = rl.rlim_max;
	return (ISC_R_SUCCESS);
}

isc_result_t
isc_resource_getcurlimit(isc_resource_t resource, isc_resourcevalue_t *value) {
	int unixresource;
	struct rlimit rl;
	isc_result_t result;

	result = resource2rlim(resource, &unixresource);
	if (result != ISC_R_SUCCESS) {
		return (result);
	}

	if (getrlimit(unixresource, &rl) != 0) {
		return (isc__errno2result(errno));
	}

	*value = rl.rlim_cur;
	return (ISC_R_SUCCESS);
}
