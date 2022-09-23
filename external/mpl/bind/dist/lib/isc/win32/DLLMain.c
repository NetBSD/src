/*	$NetBSD: DLLMain.c,v 1.8 2022/09/23 12:15:34 christos Exp $	*/

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

#include <stdio.h>
#include <windows.h>

#include <isc/mem.h>
#include <isc/util.h>

#include "lib_p.h"

/*
 * Called when we enter the DLL
 */
__declspec(dllexport) BOOL WINAPI
	DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved) {
	switch (fdwReason) {
	/*
	 * The DLL is loading due to process initialization or a call to
	 * LoadLibrary.
	 */
	case DLL_PROCESS_ATTACH:
		isc__initialize();
		break;

	/*
	 * The DLL is unloading from a process due to process
	 * termination or a call to FreeLibrary.
	 */
	case DLL_PROCESS_DETACH:
		isc__shutdown();
		break;

	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
		break;

	default:
		break;
	}
	return (TRUE);
}
