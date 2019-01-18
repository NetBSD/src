/*	$NetBSD: meminfo.c,v 1.2.2.3 2019/01/18 08:50:01 pgoyette Exp $	*/

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

#include <inttypes.h>
#include <windows.h>

#include <isc/meminfo.h>

uint64_t
isc_meminfo_totalphys(void) {
	MEMORYSTATUSEX statex;

	statex.dwLength = sizeof(statex);
	GlobalMemoryStatusEx(&statex);
	return ((uint64_t)statex.ullTotalPhys);
}
