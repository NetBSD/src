/*	$NetBSD: ntpaths.c,v 1.5.14.1 2018/04/16 01:58:00 pgoyette Exp $	*/

/*
 * Copyright (C) 2004, 2007, 2009, 2014, 2017  Internet Systems Consortium, Inc. ("ISC")
 * Copyright (C) 2001  Internet Software Consortium.
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND ISC DISCLAIMS ALL WARRANTIES WITH
 * REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS.  IN NO EVENT SHALL ISC BE LIABLE FOR ANY SPECIAL, DIRECT,
 * INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
 * LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE
 * OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */

/* Id: ntpaths.c,v 1.15 2009/07/14 22:54:57 each Exp  */

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
static char lwresd_confFile[MAX_PATH];
static char lwresd_resolvconfFile[MAX_PATH];
static char rndc_confFile[MAX_PATH];
static char ns_defaultpidfile[MAX_PATH];
static char lwresd_defaultpidfile[MAX_PATH];
static char local_state_dir[MAX_PATH];
static char sys_conf_dir[MAX_PATH];
static char rndc_keyFile[MAX_PATH];
static char session_keyFile[MAX_PATH];

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

	strlcpy(lwresd_confFile, namedBase, sizeof(lwresd_confFile)) ;
	strlcat(lwresd_confFile, "\\etc\\lwresd.conf",
		sizeof(lwresd_confFile));

	strlcpy(lwresd_defaultpidfile, namedBase,
		sizeof(lwresd_defaultpidfile));
	strlcat(lwresd_defaultpidfile, "\\etc\\lwresd.pid",
		sizeof(lwresd_defaultpidfile));

	strlcpy(lwresd_resolvconfFile, systemDir,
		sizeof(lwresd_resolvconfFile));
	strlcat(lwresd_resolvconfFile, "\\Drivers\\etc\\resolv.conf",
		sizeof(lwresd_resolvconfFile));

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

	strlcpy(local_state_dir, namedBase, sizeof(local_state_dir));
	strlcat(local_state_dir, "\\bin", sizeof(local_state_dir));

	strlcpy(sys_conf_dir, namedBase, sizeof(sys_conf_dir));
	strlcat(sys_conf_dir, "\\etc", sizeof(sys_conf_dir));

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
	case LWRES_CONF_PATH:
		return (lwresd_confFile);
		break;
	case RESOLV_CONF_PATH:
		return (lwresd_resolvconfFile);
		break;
	case RNDC_CONF_PATH:
		return (rndc_confFile);
		break;
	case NAMED_PID_PATH:
		return (ns_defaultpidfile);
		break;
	case LWRESD_PID_PATH:
		return (lwresd_defaultpidfile);
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
