/*	$NetBSD: resconf.c,v 1.7 2023/01/25 21:43:31 christos Exp $	*/

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

/*
 * Note that on Win32 there is normally no resolv.conf since all information
 * is stored in the registry. Therefore there is no ordering like the
 * contents of resolv.conf. Since the "search" or "domain" keyword, on
 * Win32 if a search list is found it is used, otherwise the domain name
 * is used since they are mutually exclusive. The search list can be entered
 * in the DNS tab of the "Advanced TCP/IP settings" window under the same place
 * that you add your nameserver list.
 */

#define HAVE_GET_WIN32_NAMESERVERS 1

#include "../resconf.c"
#include <iphlpapi.h>

#define TCPIP_SUBKEY "SYSTEM\\CurrentControlSet\\Services\\Tcpip\\Parameters"

isc_result_t
get_win32_searchlist(irs_resconf_t *conf) {
	isc_result_t result = ISC_R_SUCCESS;
	HKEY hKey;
	char searchlist[MAX_PATH];
	DWORD searchlen = MAX_PATH;
	LSTATUS status;
	char *cp;

	REQUIRE(conf != NULL);

	memset(searchlist, 0, MAX_PATH);
	status = RegOpenKeyEx(HKEY_LOCAL_MACHINE, TCPIP_SUBKEY, 0, KEY_READ,
			      &hKey);
	if (status != ERROR_SUCCESS) {
		return (ISC_R_SUCCESS);
	}

	status = RegQueryValueEx(hKey, "SearchList", NULL, NULL,
				 (LPBYTE)searchlist, &searchlen);
	RegCloseKey(hKey);
	if (status != ERROR_SUCCESS) {
		return (ISC_R_SUCCESS);
	}

	cp = strtok((char *)searchlist, ", \0");
	while (cp != NULL) {
		result = add_search(conf, cp);
		if (result != ISC_R_SUCCESS) {
			break;
		}
		cp = strtok(NULL, ", \0");
	}
	return (result);
}

isc_result_t
get_win32_nameservers(irs_resconf_t *conf) {
	isc_result_t result;
	FIXED_INFO *FixedInfo;
	ULONG BufLen = sizeof(FIXED_INFO);
	DWORD dwRetVal;
	IP_ADDR_STRING *pIPAddr;

	REQUIRE(conf != NULL);

	FixedInfo = (FIXED_INFO *)GlobalAlloc(GPTR, BufLen);
	if (FixedInfo == NULL) {
		return (ISC_R_NOMEMORY);
	}
	dwRetVal = GetNetworkParams(FixedInfo, &BufLen);
	if (dwRetVal == ERROR_BUFFER_OVERFLOW) {
		GlobalFree(FixedInfo);
		FixedInfo = GlobalAlloc(GPTR, BufLen);
		if (FixedInfo == NULL) {
			return (ISC_R_NOMEMORY);
		}
		dwRetVal = GetNetworkParams(FixedInfo, &BufLen);
	}
	if (dwRetVal != ERROR_SUCCESS) {
		GlobalFree(FixedInfo);
		return (ISC_R_FAILURE);
	}

	result = get_win32_searchlist(conf);
	if (result != ISC_R_SUCCESS) {
		goto cleanup;
	}

	if (ISC_LIST_EMPTY(conf->searchlist) &&
	    strlen(FixedInfo->DomainName) > 0)
	{
		result = add_search(conf, FixedInfo->DomainName);
		if (result != ISC_R_SUCCESS) {
			goto cleanup;
		}
	}

	/* Get the list of nameservers */
	pIPAddr = &FixedInfo->DnsServerList;
	while (pIPAddr) {
		if (conf->numns >= RESCONFMAXNAMESERVERS) {
			break;
		}

		result = add_server(conf->mctx, pIPAddr->IpAddress.String,
				    &conf->nameservers);
		if (result != ISC_R_SUCCESS) {
			break;
		}
		conf->numns++;
		pIPAddr = pIPAddr->Next;
	}

cleanup:
	if (FixedInfo != NULL) {
		GlobalFree(FixedInfo);
	}
	return (result);
}
