/*
 * The Initial Developer of the Original Code is International
 * Business Machines Corporation. Portions created by IBM
 * Corporation are Copyright (C) 2005, 2007 International Business
 * Machines Corporation. All Rights Reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the Common Public License as published by
 * IBM Corporation; either version 1 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * Common Public License for more details.
 *
 * You should have received a copy of the Common Public License
 * along with this program; if not, a copy can be viewed at
 * http://www.opensource.org/licenses/cpl1.0.php.
 */

#include "tpm_tspi.h"
#include "tpm_utils.h"


static BOOL isWellKnown = FALSE;
TSS_HCONTEXT hContext = 0;


static void help(const char* aCmd)
{
	logCmdHelp(aCmd);
	logCmdOption("-z, --well-known", _("Use TSS_WELL_KNOWN_SECRET (20 zero bytes) as the owner secret."));
}

static int parse(const int aOpt, const char *aArg)
{

	switch (aOpt) {
	case 'z':
		isWellKnown = TRUE;
		break;
	default:
		return -1;
	}

	return 0;
}

int
main( int argc, char **argv )
{
	char *szTpmPasswd = NULL;
	int tpm_len;
	TSS_HTPM hTpm;
	TSS_HPOLICY hTpmPolicy;
	TSS_BOOL bValue = TRUE;
	int iRc = -1;
	struct option opts[] = {
		{"well-known", no_argument, NULL, 'z'},
	};
	BYTE wellKnown[TCPA_SHA1_160_HASH_LEN] = TSS_WELL_KNOWN_SECRET;

	initIntlSys();

	if (genericOptHandler(argc, argv, "z", opts, sizeof(opts) / sizeof(struct option), parse,
			      help) != 0)
		goto out;

	if (contextCreate(&hContext) != TSS_SUCCESS)
		goto out;

	if (!isWellKnown) {
		// Prompt for owner password
		szTpmPasswd = GETPASSWD(_("Enter owner password: "), &tpm_len, FALSE);
		if (!szTpmPasswd) {
			logError(_("Failed to get Owner password\n"));
			goto out;
		}
	} else {
		szTpmPasswd = (char *)wellKnown;
		tpm_len = sizeof(wellKnown);
	}

	if (contextConnect(hContext) != TSS_SUCCESS)
		goto out_close;

	if (contextGetTpm(hContext, &hTpm) != TSS_SUCCESS)
		goto out_close;

	if (policyGet(hTpm, &hTpmPolicy) != TSS_SUCCESS)
		goto out_close;

	if (policySetSecret(hTpmPolicy, tpm_len, (BYTE *)szTpmPasswd) != TSS_SUCCESS)
		goto out_close;

	if (tpmSetStatus(hTpm, TSS_TPMSTATUS_RESETLOCK, bValue) != TSS_SUCCESS)
		goto out_close;

	iRc = 0;
	logSuccess(argv[0]);

	out_close:
		contextClose(hContext);

	out:
	if (!isWellKnown && szTpmPasswd)
		shredPasswd(szTpmPasswd);

	return iRc;
}
