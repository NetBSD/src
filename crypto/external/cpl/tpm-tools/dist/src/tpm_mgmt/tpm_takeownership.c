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

static void help(const char* aCmd)
{
	logCmdHelp(aCmd);
	logUnicodeCmdOption();
	logCmdOption("-y, --owner-well-known", _("Set the owner secret to all zeros (20 bytes of zeros)."));
	logCmdOption("-z, --srk-well-known", _("Set the SRK secret to all zeros (20 bytes of zeros)."));
}

static BOOL ownerWellKnown = FALSE;
static BOOL srkWellKnown = FALSE;
TSS_HCONTEXT hContext = 0;

static int parse(const int aOpt, const char *aArg)
{

	switch (aOpt) {
	case 'y':
		ownerWellKnown = TRUE;
		break;
	case 'z':
		srkWellKnown = TRUE;
		break;
	default:
		return -1;
	}
	return 0;
}

static inline TSS_RESULT tpmTakeOwnership(TSS_HTPM a_hTpm, TSS_HKEY a_hSrk)
{

	TSS_RESULT result =
	    Tspi_TPM_TakeOwnership(a_hTpm, a_hSrk, NULL_HKEY);
	tspiResult("Tspi_TPM_TakeOwnership", result);

	return result;
}

int main(int argc, char **argv)
{

	char *szTpmPasswd = NULL;
	char *szSrkPasswd = NULL;
	int tpm_len, srk_len;
	TSS_HTPM hTpm;
	TSS_HKEY hSrk;
	TSS_FLAG fSrkAttrs;
	TSS_HPOLICY hTpmPolicy, hSrkPolicy;
	int iRc = -1;
	BYTE well_known_secret[] = TSS_WELL_KNOWN_SECRET;
	struct option opts[] = {
	{"owner-well-known", no_argument, NULL, 'y'},
	{"srk-well-known", no_argument, NULL, 'z'},
	};

	initIntlSys();

	if (genericOptHandler
	    (argc, argv, "yz", opts, sizeof(opts) / sizeof(struct option),
	     parse, help) != 0)
		goto out;

	if (contextCreate(&hContext) != TSS_SUCCESS)
		goto out;

	if (!ownerWellKnown) {
		// Prompt for owner password
		szTpmPasswd = GETPASSWD(_("Enter owner password: "), &tpm_len, TRUE);
		if (!szTpmPasswd)
			goto out;
	}

	if (!srkWellKnown) {
		// Prompt for srk password
		szSrkPasswd = GETPASSWD(_("Enter SRK password: "), &srk_len, TRUE);
		if (!szSrkPasswd)
			goto out;
	}

	if (contextConnect(hContext) != TSS_SUCCESS)
		goto out_close;

	if (contextGetTpm(hContext, &hTpm) != TSS_SUCCESS)
		goto out_close;

	if (policyGet(hTpm, &hTpmPolicy) != TSS_SUCCESS)
		goto out_close;

	if (ownerWellKnown) {
		tpm_len = TCPA_SHA1_160_HASH_LEN;
		if (policySetSecret(hTpmPolicy, tpm_len, well_known_secret) != TSS_SUCCESS)
			goto out_obj_close;
	} else {
		if (policySetSecret(hTpmPolicy, tpm_len, (BYTE *)szTpmPasswd) != TSS_SUCCESS)
			goto out_close;
	}

	fSrkAttrs = TSS_KEY_TSP_SRK | TSS_KEY_AUTHORIZATION;

	if (contextCreateObject
	    (hContext, TSS_OBJECT_TYPE_RSAKEY, fSrkAttrs,
	     &hSrk) != TSS_SUCCESS)
		goto out_close;

	if (policyGet(hSrk, &hSrkPolicy) != TSS_SUCCESS)
		goto out_obj_close;

	if (srkWellKnown) {
		srk_len = TCPA_SHA1_160_HASH_LEN;
		if (policySetSecret(hSrkPolicy, srk_len, well_known_secret) != TSS_SUCCESS)
			goto out_obj_close;
	} else {
		if (policySetSecret(hSrkPolicy, srk_len, (BYTE *)szSrkPasswd) != TSS_SUCCESS)
			goto out_obj_close;
	}

	if (tpmTakeOwnership(hTpm, hSrk) != TSS_SUCCESS)
		goto out_obj_close;

	iRc = 0;
	logSuccess(argv[0]);

	out_obj_close:
		contextCloseObject(hContext, hSrk);

	out_close:
		contextClose(hContext);

	out:
		if (szTpmPasswd)
			shredPasswd(szTpmPasswd);

		if (szSrkPasswd)
			shredPasswd(szSrkPasswd);

	return iRc;
}
