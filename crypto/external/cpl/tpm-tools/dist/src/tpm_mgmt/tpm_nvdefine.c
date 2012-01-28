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

#include "tpm_tspi.h"
#include "tpm_utils.h"
#include "tpm_nvcommon.h"

static unsigned int nvindex;
static BOOL nvindex_set;
static unsigned int nvperm;
static unsigned int nvsize;
static const char *ownerpass;
static BOOL ownerWellKnown;
static BOOL askOwnerPass;
static const char *datapass;
static BOOL dataWellKnown;
static BOOL askDataPass;
static int end;

TSS_HCONTEXT hContext = 0;


static int parse(const int aOpt, const char *aArg)
{
	switch (aOpt) {
	case 'i':
		if (parseHexOrDecimal(aArg, &nvindex, 0, UINT_MAX,
				      "NVRAM index") != 0)
			return -1;
		nvindex_set = TRUE;
		break;

	case 'p':
		if (!strcmp(aArg, "?")) {
			displayStringsAndValues(permvalues, "");
			end = 1;
			return 0;
		}
		if (parseStringWithValues(aArg, permvalues, &nvperm, UINT_MAX,
				          "NVRAM permission") != 0)
			return -1;
		break;

	case 's':
		if (parseHexOrDecimal(aArg, &nvsize, 0, UINT_MAX,
				      "NVRAM index size") != 0)
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
		ownerpass = NULL;
		askOwnerPass = FALSE;
		break;

	case 'a':
		datapass = aArg;
		if (!datapass)
			askDataPass = TRUE;
		else
			askDataPass = FALSE;
		dataWellKnown = FALSE;
		break;

	case 'z':
		dataWellKnown = TRUE;
		datapass = NULL;
		askDataPass = FALSE;
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
		     _("Use 20 bytes of zeros (TSS_WELL_KNOWN_SECRET) as the "
		       "TPM owner secret"));
	logCmdOption("-z, --data-well-known",
		     _("Use 20 bytes of zeros (TSS_WELL_KNOWN_SECRET) as the "
		       "NVRAM area's secret"));
	logOwnerPassCmdOption();
	logCmdOption("-a, --pwda",
		     _("NVRAM area password"));
	logNVIndexCmdOption();
	logCmdOption("-s, --size",
		     _("Size of the NVRAM area"));
	logCmdOption("-p, --permissions",
		     _("Permissions of the NVRAM area"));

        displayStringsAndValues(permvalues, "                ");
}

int main(int argc, char **argv)
{
	TSS_HTPM hTpm;
	TSS_HNVSTORE nvObject;
	TSS_FLAG fNvAttrs;
	TSS_HPOLICY hTpmPolicy, hDataPolicy;
	int iRc = -1;
	BYTE well_known_secret[] = TSS_WELL_KNOWN_SECRET;
	int opswd_len = -1;
	int dpswd_len = -1;
	struct option hOpts[] = {
		{"index"           , required_argument, NULL, 'i'},
		{"size"            , required_argument, NULL, 's'},
		{"permissions"     , required_argument, NULL, 'p'},
		{"pwdo"            , optional_argument, NULL, 'o'},
		{"pwda"            , optional_argument, NULL, 'a'},
		{"use-unicode"     ,       no_argument, NULL, 'u'},
		{"data-well-known" ,       no_argument, NULL, 'z'},
		{"owner-well-known",       no_argument, NULL, 'y'},
		{NULL              ,       no_argument, NULL, 0},
	};

	initIntlSys();

	if (genericOptHandler
		    (argc, argv, "i:s:p:o:a:yzu", hOpts,
		     sizeof(hOpts) / sizeof(struct option), parse, help) != 0)
		goto out;

	if (end) {
		iRc = 0;
		goto out;
	}

	if (!nvindex_set) {
		logError(_("You must provide an index for the NVRAM area.\n"));
		goto out;
	}

	if (nvperm == 0 &&
	    (UINT32)nvindex != TPM_NV_INDEX_LOCK &&
	    (UINT32)nvindex != TPM_NV_INDEX0) {
		logError(_("You must provide permission bits for the NVRAM area.\n"));
		goto out;
	}

	logDebug("permissions = 0x%08x\n", nvperm);

	if (contextCreate(&hContext) != TSS_SUCCESS)
		goto out;

	if (contextConnect(hContext) != TSS_SUCCESS)
		goto out_close;

	if (contextGetTpm(hContext, &hTpm) != TSS_SUCCESS)
		goto out_close;

	fNvAttrs = 0;

	if (contextCreateObject(hContext,
				TSS_OBJECT_TYPE_NV,
				fNvAttrs,
				&nvObject) != TSS_SUCCESS)
		goto out_close;

	if (askOwnerPass) {
		ownerpass = _GETPASSWD(_("Enter owner password: "), &opswd_len,
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
			if (opswd_len < 0)
				opswd_len = strlen(ownerpass);
			if (policySetSecret(hTpmPolicy, opswd_len,
					    (BYTE *)ownerpass) != TSS_SUCCESS)
				goto out_close;
		} else {
			if (policySetSecret(hTpmPolicy, TCPA_SHA1_160_HASH_LEN,
					    (BYTE *)well_known_secret) != TSS_SUCCESS)
				goto out_close;
		}
	}

	if (askDataPass) {
		datapass = _GETPASSWD(_("Enter NVRAM data password: "), &dpswd_len,
			TRUE, useUnicode );
		if (!datapass) {
			logError(_("Failed to get NVRAM data password\n"));
			goto out_close;
		}
	}

	if (datapass || dataWellKnown) {
		if (contextCreateObject
		    (hContext, TSS_OBJECT_TYPE_POLICY, TSS_POLICY_USAGE,
		     &hDataPolicy) != TSS_SUCCESS)
			goto out_close;

		if (datapass) {
			if (dpswd_len < 0)
				dpswd_len = strlen(datapass);
			if (policySetSecret(hDataPolicy, dpswd_len,
				    (BYTE *)datapass) != TSS_SUCCESS)
				goto out_close;
		} else {
			if (policySetSecret(hDataPolicy, TCPA_SHA1_160_HASH_LEN,
				    (BYTE *)well_known_secret) != TSS_SUCCESS)
				goto out_close;
		}

		if (Tspi_Policy_AssignToObject(hDataPolicy, nvObject) !=
		    TSS_SUCCESS)
			goto out_close;
	}

	if (Tspi_SetAttribUint32(nvObject,
				 TSS_TSPATTRIB_NV_INDEX,
				 0,
				 nvindex) != TSS_SUCCESS)
		goto out_close_obj;

	if (Tspi_SetAttribUint32(nvObject,
				 TSS_TSPATTRIB_NV_PERMISSIONS,
				 0,
				 nvperm) != TSS_SUCCESS)
		goto out_close_obj;

	if (Tspi_SetAttribUint32(nvObject,
				 TSS_TSPATTRIB_NV_DATASIZE,
				 0,
				 nvsize) != TSS_SUCCESS)
		goto out_close_obj;

	if (NVDefineSpace(nvObject, (TSS_HPCRS)0, (TSS_HPCRS)0) !=
	    TSS_SUCCESS)
		goto out_close;

	logMsg(_("Successfully created NVRAM area at index 0x%x (%u).\n"),
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
