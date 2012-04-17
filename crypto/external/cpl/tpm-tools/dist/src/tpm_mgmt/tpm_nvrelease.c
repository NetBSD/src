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

#include <limits.h>

#include "tpm_nvcommon.h"
#include "tpm_tspi.h"
#include "tpm_utils.h"

static unsigned int nvindex;
static const char *ownerpass;
static BOOL ownerWellKnown;
static BOOL askOwnerPass;
TSS_HCONTEXT hContext = 0;


static int parse(const int aOpt, const char *aArg)
{

	switch (aOpt) {
	case 'i':
		if (parseHexOrDecimal(aArg, &nvindex, 0, UINT_MAX,
				      "NVRAM index") != 0)
			return -1;
		break;

	case 'o':
		ownerpass = aArg;
		if (!ownerpass)
			askOwnerPass = TRUE;
		else
			askOwnerPass = FALSE;
		ownerWellKnown = FALSE;
		break;

	case 'y':
		ownerWellKnown = TRUE;
		askOwnerPass = FALSE;
		ownerpass = NULL;
		break;

	case 'u':
		useUnicode = TRUE;
		break;

	default:
		return -1;
	}
	return 0;
}

static void help(const char* aCmd)
{
	logCmdHelp(aCmd);
	logUnicodeCmdOption();
	logCmdOption("-y, --owner-well-known",
		     _("Use 20 bytes of zeros (TSS_WELL_KNOWN_SECRET) as the TPM owner password"));
	logOwnerPassCmdOption();
	logNVIndexCmdOption();
}

int main(int argc, char **argv)
{
	TSS_RESULT res;
	TSS_HTPM hTpm;
	TSS_HNVSTORE nvObject;
	TSS_FLAG fNvAttrs;
	TSS_HPOLICY hTpmPolicy;
	int iRc = -1;
	int pswd_len = -1;
	BYTE well_known_secret[] = TSS_WELL_KNOWN_SECRET;
	struct option hOpts[] = {
		{"index"           , required_argument, NULL, 'i'},
		{"pwdo"            , optional_argument, NULL, 'o'},
		{"owner-well-known",       no_argument, NULL, 'y'},
		{NULL              ,       no_argument, NULL, 0},
	};

	initIntlSys();

	if (genericOptHandler
		    (argc, argv, "i:o::y", hOpts,
		     sizeof(hOpts) / sizeof(struct option), parse, help) != 0)
		goto out;

	if (nvindex == 0) {
		logError(_("You must provide an index (!= 0) for the "
		           "NVRAM area.\n"));
		goto out;
	}

	if (contextCreate(&hContext) != TSS_SUCCESS)
		goto out;

	if (contextConnect(hContext) != TSS_SUCCESS)
		goto out_close;

	if (contextGetTpm(hContext, &hTpm) != TSS_SUCCESS)
		goto out_close;

	fNvAttrs = 0;

	if (askOwnerPass) {
		ownerpass = _GETPASSWD(_("Enter owner password: "), &pswd_len,
			FALSE, useUnicode );
		if (!ownerpass) {
			logError(_("Failed to get owner password\n"));
			goto out_close;
		}
	}

	if (ownerpass || ownerWellKnown) {
		if (policyGet(hTpm, &hTpmPolicy) != TSS_SUCCESS)
			goto out_close;
		if (ownerpass) {
			if (pswd_len < 0)
				pswd_len = strlen(ownerpass);

			if (policySetSecret(hTpmPolicy, pswd_len,
					    (BYTE *)ownerpass) != TSS_SUCCESS)
				goto out_close;
		} else {
			if (policySetSecret(hTpmPolicy, TCPA_SHA1_160_HASH_LEN,
					    (BYTE *)well_known_secret) != TSS_SUCCESS)
				goto out_close;
		}
	}

	if (contextCreateObject(hContext,
				TSS_OBJECT_TYPE_NV,
				fNvAttrs,
				&nvObject) != TSS_SUCCESS)
		goto out_close;

	if (Tspi_SetAttribUint32(nvObject,
				 TSS_TSPATTRIB_NV_INDEX,
				 0,
				 nvindex) != TSS_SUCCESS)
		goto out_close_obj;

	if ((res = NVReleaseSpace(nvObject)) != TSS_SUCCESS) {
		goto out_close;
	}

	logMsg(_("Successfully released NVRAM area at index 0x%x (%d).\n"),
	       nvindex, nvindex);

	iRc = 0;

	goto out_close;

      out_close_obj:
	contextCloseObject(hContext, nvObject);

      out_close:
	contextClose(hContext);

      out:
	return iRc;
}
