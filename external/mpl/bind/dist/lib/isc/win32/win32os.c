/*	$NetBSD: win32os.c,v 1.2 2018/08/12 13:02:40 christos Exp $	*/

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

#include <windows.h>

#ifndef TESTVERSION
#include <isc/win32os.h>
#else
#include <stdio.h>
#include <isc/util.h>
#endif
#include <isc/print.h>

int
isc_win32os_versioncheck(unsigned int major, unsigned int minor,
			 unsigned int spmajor, unsigned int spminor)
{
	OSVERSIONINFOEX osVer;
	DWORD typeMask;
	ULONGLONG conditionMask;

	memset(&osVer, 0, sizeof(OSVERSIONINFOEX));
	osVer.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
	typeMask = 0;
	conditionMask = 0;

	/* Optimistic: likely greater */
	osVer.dwMajorVersion = major;
	typeMask |= VER_MAJORVERSION;
	conditionMask = VerSetConditionMask(conditionMask,
					    VER_MAJORVERSION,
					    VER_GREATER);
	osVer.dwMinorVersion = minor;
	typeMask |= VER_MINORVERSION;
	conditionMask = VerSetConditionMask(conditionMask,
					    VER_MINORVERSION,
					    VER_GREATER);
	osVer.wServicePackMajor = spmajor;
	typeMask |= VER_SERVICEPACKMAJOR;
	conditionMask = VerSetConditionMask(conditionMask,
					    VER_SERVICEPACKMAJOR,
					    VER_GREATER);
	osVer.wServicePackMinor = spminor;
	typeMask |= VER_SERVICEPACKMINOR;
	conditionMask = VerSetConditionMask(conditionMask,
					    VER_SERVICEPACKMINOR,
					    VER_GREATER);
	if (VerifyVersionInfo(&osVer, typeMask, conditionMask))
		return (1);

	/* Failed: retry with equal */
	conditionMask = 0;
	conditionMask = VerSetConditionMask(conditionMask,
					    VER_MAJORVERSION,
					    VER_EQUAL);
	conditionMask = VerSetConditionMask(conditionMask,
					    VER_MINORVERSION,
					    VER_EQUAL);
	conditionMask = VerSetConditionMask(conditionMask,
					    VER_SERVICEPACKMAJOR,
					    VER_EQUAL);
	conditionMask = VerSetConditionMask(conditionMask,
					    VER_SERVICEPACKMINOR,
					    VER_EQUAL);
	if (VerifyVersionInfo(&osVer, typeMask, conditionMask))
		return (0);
	else
		return (-1);
}

#ifdef TESTVERSION
int
main(int argc, char **argv) {
	unsigned int major = 0;
	unsigned int minor = 0;
	unsigned int spmajor = 0;
	unsigned int spminor = 0;
	int ret;

	if (argc > 1) {
		--argc;
		++argv;
		major = (unsigned int) atoi(argv[0]);
	}
	if (argc > 1) {
		--argc;
		++argv;
		minor = (unsigned int) atoi(argv[0]);
	}
	if (argc > 1) {
		--argc;
		++argv;
		spmajor = (unsigned int) atoi(argv[0]);
	}
	if (argc > 1) {
		--argc;
		POST(argc);
		++argv;
		spminor = (unsigned int) atoi(argv[0]);
	}

	ret = isc_win32os_versioncheck(major, minor, spmajor, spminor);

	printf("%s major %u minor %u SP major %u SP minor %u\n",
	       ret > 0 ? "greater" : (ret == 0 ? "equal" : "less"),
	       major, minor, spmajor, spminor);
	return (ret);
}
#endif
