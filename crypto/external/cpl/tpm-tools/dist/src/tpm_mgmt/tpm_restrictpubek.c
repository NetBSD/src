/*
 * The Initial Developer of the Original Code is International
 * Business Machines Corporation. Portions created by IBM
 * Corporation are Copyright (C) 2005 International Business
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
#include <getopt.h>

//controlled by input options
static BOOL bCheck = TRUE;
static BOOL isWellKnown = FALSE;
TSS_HCONTEXT hContext = 0;

static int parse(const int aOpt, const char *aArg)
{

	switch (aOpt) {
	case 's':
		logDebug(_("Changing mode to check status.\n"));
		bCheck = TRUE;
		break;
	case 'r':
		logDebug(_("Changing mode to restrist PubEK access\n"));
		bCheck = FALSE;
		break;
	case 'z':
		logDebug(_("Using TSS_WELL_KNOWN_SECRET to authorize the TPM command\n"));
		isWellKnown = TRUE;
		break;
	default:
		return -1;
	}
	return 0;
}

static void help(const char *aCmd)
{

	logCmdHelp(aCmd);
	logUnicodeCmdOption();
	logCmdOption("-s, --status", _("Display current status"));
	logCmdOption("-r, --restrict",
		     _("Restrict PubEK read to owner only"));
	logCmdOption("-z, --well-known",
		     _("Use 20 bytes of zeros (TSS_WELL_KNOWN_SECRET) as the TPM secret authorization data"));
}

int main(int argc, char **argv)
{

	char *szTpmPasswd = NULL;
	int pswd_len;
	TSS_HPOLICY hTpmPolicy;
	TSS_HTPM hTpm;
	int iRc = -1;
	struct option opts[] = { {"status", no_argument, NULL, 's'},
	{"restrict", no_argument, NULL, 'r'},
	{"well-known", no_argument, NULL, 'z'},
	};
	BYTE well_known[TCPA_SHA1_160_HASH_LEN] = TSS_WELL_KNOWN_SECRET;

        initIntlSys();

	if (genericOptHandler
	    (argc, argv, "srz", opts, sizeof(opts) / sizeof(struct option),
	     parse, help) != 0)
		goto out;

	//Connect to TSS and TPM
	if (contextCreate(&hContext) != TSS_SUCCESS)
		goto out;

	if (contextConnect(hContext) != TSS_SUCCESS)
		goto out_close;

	if (contextGetTpm(hContext, &hTpm) != TSS_SUCCESS)
		goto out_close;

	if (isWellKnown) {
		szTpmPasswd = (char *)well_known;
		pswd_len = sizeof(well_known);
	} else {
		// Prompt for owner password
		szTpmPasswd = GETPASSWD(_("Enter owner password: "), &pswd_len, FALSE);
		if (!szTpmPasswd) {
			logMsg(_("Failed to get password\n"));
			goto out_close;
		}
	}
	if (policyGet(hTpm, &hTpmPolicy) != TSS_SUCCESS)
		goto out_close;
	if (policySetSecret
	    (hTpmPolicy, pswd_len, (BYTE *)szTpmPasswd) != TSS_SUCCESS)
		goto out_close;

	if (bCheck) {
		TSS_BOOL bValue;
		if (tpmGetStatus
		    (hTpm, TSS_TPMSTATUS_DISABLEPUBEKREAD,
		     &bValue) != TSS_SUCCESS)
			goto out;
		logMsg(_("Public Endorsement Key readable by: %s\n"),
		       bValue ? _("owner") : _("everyone"));

	} else {
		if (tpmSetStatus(hTpm, TSS_TPMSTATUS_DISABLEPUBEKREAD, 0)
		    != TSS_SUCCESS)
			goto out_close;
	}

	iRc = 0;
	logSuccess(argv[0]);

      out_close:
	contextClose(hContext);

      out:
	if (szTpmPasswd && !isWellKnown)
		shredPasswd(szTpmPasswd);

	return iRc;

}
