/*	$NetBSD: DLLMain.c,v 1.2 2018/08/12 13:02:31 christos Exp $	*/

/*
 * Copyright (C) 2001, 2004, 2007, 2016  Internet Systems Consortium, Inc. ("ISC")
 * 
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */


#include <windows.h>
#include <signal.h>

/*
 * Called when we enter the DLL
 */
__declspec(dllexport) BOOL WINAPI DllMain(HINSTANCE hinstDLL,
					  DWORD fdwReason, LPVOID lpvReserved)
{
	switch (fdwReason) { 
	/*
	 * The DLL is loading due to process 
	 * initialization or a call to LoadLibrary. 
	 */
	case DLL_PROCESS_ATTACH: 
		break; 
 
	/* The attached process creates a new thread.  */
	case DLL_THREAD_ATTACH: 
		break; 
 
	/* The thread of the attached process terminates. */
	case DLL_THREAD_DETACH: 
		break; 
 
	/*
	 * The DLL is unloading from a process due to 
	 * process termination or a call to FreeLibrary. 
	 */
	case DLL_PROCESS_DETACH: 
		break; 

	default: 
		break; 
	} 
	return (TRUE);
}

