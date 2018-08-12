/*	$NetBSD: resource.c,v 1.2 2018/08/12 13:02:40 christos Exp $	*/

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

#include <isc/platform.h>
#include <isc/resource.h>
#include <isc/result.h>
#include <isc/util.h>

#include "errno2result.h"

/*
 * Windows limits the maximum number of open files to 2048
 */

#define WIN32_MAX_OPEN_FILES	2048

isc_result_t
isc_resource_setlimit(isc_resource_t resource, isc_resourcevalue_t value) {
	isc_resourcevalue_t rlim_value;
	int wresult;

	if (resource != isc_resource_openfiles)
		return (ISC_R_NOTIMPLEMENTED);


	if (value == ISC_RESOURCE_UNLIMITED)
		rlim_value = WIN32_MAX_OPEN_FILES;
	else
		rlim_value = min(value, WIN32_MAX_OPEN_FILES);

	wresult = _setmaxstdio((int) rlim_value);

	if (wresult > 0)
		return (ISC_R_SUCCESS);
	else
		return (isc__errno2result(errno));
}

isc_result_t
isc_resource_getlimit(isc_resource_t resource, isc_resourcevalue_t *value) {

	if (resource != isc_resource_openfiles)
		return (ISC_R_NOTIMPLEMENTED);

	*value = WIN32_MAX_OPEN_FILES;
	return (ISC_R_SUCCESS);
}

isc_result_t
isc_resource_getcurlimit(isc_resource_t resource, isc_resourcevalue_t *value) {
	return (isc_resource_getlimit(resource, value));
}
