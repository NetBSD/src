/*	$NetBSD: ntgroups.c,v 1.9 2023/01/25 21:43:32 christos Exp $	*/

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
 * The NT Groups have two groups that are not well documented and are
 * not normally seen: None and Everyone.  A user account belongs to
 * any number of groups, but if it is not a member of any group then
 * it is a member of the None Group. The None group is not listed
 * anywhere. You cannot remove an account from the none group except
 * by making it a member of some other group, The second group is the
 * Everyone group.  All accounts, no matter how many groups that they
 * belong to, also belong to the Everyone group. You cannot remove an
 * account from the Everyone group.
 */

#ifndef UNICODE
#define UNICODE
#endif /* UNICODE */

/*
 * Silence warnings.
 */
#define _CRT_SECURE_NO_DEPRECATE 1

/* clang-format off */
#include <assert.h>
#include <windows.h>
#include <lm.h>
/* clang-format on */

#include <isc/ntgroups.h>
#include <isc/result.h>

#define MAX_NAME_LENGTH 256

#define CHECK(op)                              \
	do {                                   \
		result = (op);                 \
		if (result != ISC_R_SUCCESS) { \
			goto cleanup;          \
		}                              \
	} while (0)

isc_result_t
isc_ntsecurity_getaccountgroups(char *username, char **GroupList,
				unsigned int maxgroups,
				unsigned int *totalGroups) {
	LPGROUP_USERS_INFO_0 pTmpBuf;
	LPLOCALGROUP_USERS_INFO_0 pTmpLBuf;
	DWORD i;
	LPLOCALGROUP_USERS_INFO_0 pBuf = NULL;
	LPGROUP_USERS_INFO_0 pgrpBuf = NULL;
	DWORD dwLevel = 0;
	DWORD dwFlags = LG_INCLUDE_INDIRECT;
	DWORD dwPrefMaxLen = MAX_PREFERRED_LENGTH;
	DWORD dwEntriesRead = 0;
	DWORD dwTotalEntries = 0;
	NET_API_STATUS nStatus;
	size_t retlen;
	wchar_t user[MAX_NAME_LENGTH];
	isc_result_t result;

	*totalGroups = 0;

	retlen = mbstowcs(user, username, MAX_NAME_LENGTH);
	if (retlen == (size_t)(-1)) {
		return (ISC_R_FAILURE);
	}

	/*
	 * Call the NetUserGetLocalGroups function
	 * specifying information level 0.
	 *
	 * The LG_INCLUDE_INDIRECT flag specifies that the
	 * function should also return the names of the local
	 * groups in which the user is indirectly a member.
	 */
	nStatus = NetUserGetLocalGroups(NULL, user, dwLevel, dwFlags,
					(LPBYTE *)&pBuf, dwPrefMaxLen,
					&dwEntriesRead, &dwTotalEntries);
	/*
	 * See if the call succeeds,
	 */
	if (nStatus != NERR_Success) {
		if (nStatus == ERROR_ACCESS_DENIED) {
			return (ISC_R_NOPERM);
		}
		if (nStatus == ERROR_MORE_DATA) {
			return (ISC_R_NOSPACE);
		}
		if (nStatus == NERR_UserNotFound) {
			dwEntriesRead = 0;
		}
	}

	if (pBuf != NULL) {
		pTmpLBuf = pBuf;
		/*
		 * Loop through the entries
		 */
		for (i = 0; (i < dwEntriesRead && *totalGroups < maxgroups);
		     i++)
		{
			assert(pTmpLBuf != NULL);
			if (pTmpLBuf == NULL) {
				break;
			}
			retlen = wcslen(pTmpLBuf->lgrui0_name);
			GroupList[*totalGroups] = (char *)malloc(retlen + 1);
			if (GroupList[*totalGroups] == NULL) {
				CHECK(ISC_R_NOMEMORY);
			}

			retlen = wcstombs(GroupList[*totalGroups],
					  pTmpLBuf->lgrui0_name, retlen);
			if (retlen == (size_t)(-1)) {
				free(GroupList[*totalGroups]);
				CHECK(ISC_R_FAILURE);
			}
			GroupList[*totalGroups][retlen] = '\0';
			if (strcmp(GroupList[*totalGroups], "None") == 0) {
				free(GroupList[*totalGroups]);
			} else {
				(*totalGroups)++;
			}
			pTmpLBuf++;
		}
	}
	/* Free the allocated memory. */
	/* cppcheck-suppress duplicateCondition */
	if (pBuf != NULL) {
		NetApiBufferFree(pBuf);
	}

	/*
	 * Call the NetUserGetGroups function, specifying level 0.
	 */
	nStatus = NetUserGetGroups(NULL, user, dwLevel, (LPBYTE *)&pgrpBuf,
				   dwPrefMaxLen, &dwEntriesRead,
				   &dwTotalEntries);
	/*
	 * See if the call succeeds,
	 */
	if (nStatus != NERR_Success) {
		if (nStatus == ERROR_ACCESS_DENIED) {
			return (ISC_R_NOPERM);
		}
		if (nStatus == ERROR_MORE_DATA) {
			return (ISC_R_NOSPACE);
		}
		if (nStatus == NERR_UserNotFound) {
			dwEntriesRead = 0;
		}
	}

	if (pgrpBuf != NULL) {
		pTmpBuf = pgrpBuf;
		/*
		 * Loop through the entries
		 */
		for (i = 0; (i < dwEntriesRead && *totalGroups < maxgroups);
		     i++)
		{
			assert(pTmpBuf != NULL);

			if (pTmpBuf == NULL) {
				break;
			}
			retlen = wcslen(pTmpBuf->grui0_name);
			GroupList[*totalGroups] = (char *)malloc(retlen + 1);
			if (GroupList[*totalGroups] == NULL) {
				CHECK(ISC_R_NOMEMORY);
			}

			retlen = wcstombs(GroupList[*totalGroups],
					  pTmpBuf->grui0_name, retlen);
			if (retlen == (size_t)(-1)) {
				free(GroupList[*totalGroups]);
				CHECK(ISC_R_FAILURE);
			}
			GroupList[*totalGroups][retlen] = '\0';
			if (strcmp(GroupList[*totalGroups], "None") == 0) {
				free(GroupList[*totalGroups]);
			} else {
				(*totalGroups)++;
			}
			pTmpBuf++;
		}
	}
	/*
	 * Free the allocated memory.
	 */
	/* cppcheck-suppress duplicateCondition */
	if (pgrpBuf != NULL) {
		NetApiBufferFree(pgrpBuf);
	}

	return (ISC_R_SUCCESS);

cleanup:
	while (--(*totalGroups) > 0) {
		free(GroupList[*totalGroups]);
	}
	return (result);
}
