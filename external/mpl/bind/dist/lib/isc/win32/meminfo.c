/*	$NetBSD: meminfo.c,v 1.2 2018/08/12 13:02:40 christos Exp $	*/

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

#include <windows.h>

#include <isc/meminfo.h>

isc_uint64_t
isc_meminfo_totalphys(void) {
	MEMORYSTATUSEX statex;

	statex.dwLength = sizeof(statex);
	GlobalMemoryStatusEx(&statex);
	return ((isc_uint64_t)statex.ullTotalPhys);
}
