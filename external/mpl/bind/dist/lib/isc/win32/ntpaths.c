/*	$NetBSD: ntpaths.c,v 1.2 2018/08/12 13:02:40 christos Exp $	*/

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


/*
 * This module fetches the required path information that is specific
 * to NT systems which can have its configuration and system files
 * almost anywhere. It can be used to override whatever the application
 * had previously assigned to the pointer. Basic information about the
 * file locations are stored in the registry.
 */

#include <config.h>

#include <isc/bind_registry.h>
#include <isc/ntpaths.h>
#include <isc/string.h>

/*
 * Module Variables
 */

static char systemDir[MAX_PATH];
static char namedBase[MAX_PATH];
static char ns_confFile[MAX_PATH];
static char rndc_confFile[MAX_PATH];
static char ns_defaultpidfile[MAX_PATH];
static char ns_lockfile[MAX_PATH];
static char local_state_dir[MAX_PATH];
static char sys_conf_dir[MAX_PATH];
static char rndc_keyFile[MAX_PATH];
static char session_keyFile[MAX_PATH];
static char resolv_confFile[MAX_PATH];

static DWORD baseLen = MAX_PATH;
static BOOL Initialized = FALSE;

void
isc_ntpaths_init(void) {
	HKEY hKey;
	BOOL keyFound = TRUE;

	memset(namedBase, 0, sizeof(namedBase));
	if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, BIND_SUBKEY, 0, KEY_READ, &hKey)
		!= ERROR_SUCCESS)
		keyFound = FALSE;

	if (keyFound == TRUE) {
		/* Get the named directory */
		if (RegQueryValueEx(hKey, "InstallDir", NULL, NULL,
			(LPBYTE)namedBase, &baseLen) != ERROR_SUCCESS)
			keyFound = FALSE;
		RegCloseKey(hKey);
	}

	GetSystemDirectory(systemDir, MAX_PATH);

	if (keyFound == FALSE) {
		/* Use the System Directory as a default */
		strlcpy(namedBase, systemDir, sizeof(namedBase));
	}

	strlcpy(ns_confFile, namedBase, sizeof(ns_confFile));
	strlcat(ns_confFile, "\\etc\\named.conf", sizeof(ns_confFile));

	strlcpy(rndc_keyFile, namedBase, sizeof(rndc_keyFile));
	strlcat(rndc_keyFile, "\\etc\\rndc.key", sizeof(rndc_keyFile));

	strlcpy(session_keyFile, namedBase, sizeof(session_keyFile));
	strlcat(session_keyFile, "\\etc\\session.key", sizeof(session_keyFile));

	strlcpy(rndc_confFile, namedBase, sizeof(rndc_confFile));
	strlcat(rndc_confFile, "\\etc\\rndc.conf", sizeof(rndc_confFile));

	strlcpy(ns_defaultpidfile, namedBase, sizeof(ns_defaultpidfile));
	strlcat(ns_defaultpidfile, "\\etc\\named.pid",
		sizeof(ns_defaultpidfile));

	strlcpy(ns_lockfile, namedBase, sizeof(ns_lockfile));
	strlcat(ns_lockfile, "\\etc\\named.lock", sizeof(ns_lockfile));

	strlcpy(local_state_dir, namedBase, sizeof(local_state_dir));
	strlcat(local_state_dir, "\\bin", sizeof(local_state_dir));

	strlcpy(sys_conf_dir, namedBase, sizeof(sys_conf_dir));
	strlcat(sys_conf_dir, "\\etc", sizeof(sys_conf_dir));

	/* Added to avoid an assert on NULL value */
	strlcpy(resolv_confFile, namedBase, sizeof(resolv_confFile));
	strlcat(resolv_confFile, "\\etc\\resolv.conf",
		sizeof(resolv_confFile));

	Initialized = TRUE;
}

char *
isc_ntpaths_get(int ind) {
	if (!Initialized)
		isc_ntpaths_init();

	switch (ind) {
	case NAMED_CONF_PATH:
		return (ns_confFile);
		break;
	case RESOLV_CONF_PATH:
		return (resolv_confFile);
		break;
	case RNDC_CONF_PATH:
		return (rndc_confFile);
		break;
	case NAMED_PID_PATH:
		return (ns_defaultpidfile);
		break;
	case NAMED_LOCK_PATH:
		return (ns_lockfile);
		break;
	case LOCAL_STATE_DIR:
		return (local_state_dir);
		break;
	case SYS_CONF_DIR:
		return (sys_conf_dir);
		break;
	case RNDC_KEY_PATH:
		return (rndc_keyFile);
		break;
	case SESSION_KEY_PATH:
		return (session_keyFile);
		break;
	default:
		return (NULL);
	}
}
