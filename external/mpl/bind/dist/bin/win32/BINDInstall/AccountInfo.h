/*	$NetBSD: AccountInfo.h,v 1.1.1.1 2018/08/12 12:07:40 christos Exp $	*/

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



#define RTN_OK		0
#define RTN_NOACCOUNT	1
#define RTN_NOMEMORY	2
#define RTN_ERROR	10

#define SE_SERVICE_LOGON_PRIV	L"SeServiceLogonRight"

/*
 * This routine retrieves the list of all Privileges associated with
 * a given account as well as the groups to which it beongs
 */
int
GetAccountPrivileges(
	char *name,			/* Name of Account */
	wchar_t **PrivList,		/* List of Privileges returned */
	unsigned int *PrivCount,	/* Count of Privileges returned */
	char **Groups,		/* List of Groups to which account belongs */
	unsigned int *totalGroups,	/* Count of Groups returned */
	int maxGroups		/* Maximum number of Groups to return */
	);

/*
 * This routine creates an account with the given name which has just
 * the logon service privilege and no membership of any groups,
 * i.e. it's part of the None group.
 */
BOOL
CreateServiceAccount(char *name, char *password);
