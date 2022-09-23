/*	$NetBSD: os.c,v 1.6 2022/09/23 12:15:35 christos Exp $	*/

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

#include <windows.h>

#include <isc/os.h>

static BOOL bInit = FALSE;
static SYSTEM_INFO SystemInfo;

static void
initialize_action(void) {
	if (bInit) {
		return;
	}

	GetSystemInfo(&SystemInfo);
	bInit = TRUE;
}

unsigned int
isc_os_ncpus(void) {
	long ncpus;
	initialize_action();
	ncpus = SystemInfo.dwNumberOfProcessors;
	if (ncpus <= 0) {
		ncpus = 1;
	}

	return ((unsigned int)ncpus);
}
