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

#include <stdio.h>
#include <errno.h>
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
static UINT32 selectedPcrsRead[24];
static UINT32 selectedPcrsWrite[24];
static UINT32 selectedPcrsReadLen = 0;
static UINT32 selectedPcrsWriteLen = 0;
static const char *filename;

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

	case 'r':
		if (aArg && atoi(aArg) >= 0 && atoi(aArg) < 24) {
			selectedPcrsRead[selectedPcrsReadLen++] = atoi(aArg);
		} else
			return -1;
		break;

	case 'w':
		if (aArg && atoi(aArg) >= 0 && atoi(aArg) < 24) {
			selectedPcrsWrite[selectedPcrsWriteLen++] = atoi(aArg);
		} else
			return -1;
		break;

	case 'f':
		filename = aArg;
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
	logCmdOption("-r, --rpcrs",
		     _("PCRs to seal the NVRAM area to for reading (use multiple times)"));
	logCmdOption("-w, --wpcrs",
		     _("PCRs to seal the NVRAM area to for writing (use multiple times)"));
	logCmdOption("-f, --filename",
		     _("File containing PCR info for the NVRAM area"));

	logCmdOption("-p, --permissions",
		     _("Permissions of the NVRAM area"));
        displayStringsAndValues(permvalues, "                ");
}

void logInvalidPcrInfoFile()
{
	logError(_("Invalid PCR info file. Format is:\n"
		 "[r/w] [PCR IDX] [SHA-1 ascii]\n\nExample:\n"
		 "r 9 00112233445566778899AABBCCDDEEFF00112233"));
}

int
parseNVPermsFile(FILE *f, TSS_HCONTEXT *hContext, TSS_HNVSTORE *nvObject,
		 TSS_HPCRS *hPcrsRead, TSS_HPCRS *hPcrsWrite)
{
	UINT32 pcrSize;
	char rw;
	unsigned int pcr, n;
	char hash_ascii[65], hash_bin[32], save;
	int rc = -1;

	while (!feof(f)) {
		errno = 0;
		n = fscanf(f, "%c %u %s\n", &rw, &pcr, hash_ascii);
		if (n != 3) {
			logInvalidPcrInfoFile();
			goto out;
		} else if (errno != 0) {
			perror("fscanf");
			goto out;
		}

		if (rw != 'r' && rw != 'w') {
			logInvalidPcrInfoFile();
			goto out;
		}

		if (pcr > 15) {
			logError(_("Cannot seal NVRAM area to PCR > 15\n"));
			goto out;
		}

		for (n = 0; n < strlen(hash_ascii); n += 2) {
			save = hash_ascii[n + 2];
			hash_ascii[n + 2] = '\0';
			hash_bin[n/2] = strtoul(&hash_ascii[n], NULL, 16);
			hash_ascii[n + 2] = save;
		}
		pcrSize = n/2;

		if (rw == 'r') {
			if (*hPcrsRead == NULL_HPCRS)
				if (contextCreateObject(*hContext, TSS_OBJECT_TYPE_PCRS,
							TSS_PCRS_STRUCT_INFO_SHORT,
							hPcrsRead) != TSS_SUCCESS)
					goto out;

			if (pcrcompositeSetPcrValue(*hPcrsRead, pcr, pcrSize, (BYTE *)hash_bin)
					!= TSS_SUCCESS)
				goto out;
		} else {
			if (*hPcrsWrite == NULL_HPCRS)
				if (contextCreateObject(*hContext, TSS_OBJECT_TYPE_PCRS,
							TSS_PCRS_STRUCT_INFO_SHORT,
							hPcrsWrite) != TSS_SUCCESS)
					goto out;

			if (pcrcompositeSetPcrValue(*hPcrsWrite, pcr, pcrSize, (BYTE *)hash_bin)
					!= TSS_SUCCESS)
				goto out;
		}
	}

	rc = 0;
out:
	return rc;
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
	TSS_HPCRS hPcrsRead = 0, hPcrsWrite = 0;
	struct option hOpts[] = {
		{"index"           , required_argument, NULL, 'i'},
		{"size"            , required_argument, NULL, 's'},
		{"permissions"     , required_argument, NULL, 'p'},
		{"rpcrs"           , required_argument, NULL, 'r'},
		{"wpcrs"           , required_argument, NULL, 'w'},
		{"filename"        , required_argument, NULL, 'f'},
		{"pwdo"            , optional_argument, NULL, 'o'},
		{"pwda"            , optional_argument, NULL, 'a'},
		{"use-unicode"     ,       no_argument, NULL, 'u'},
		{"data-well-known" ,       no_argument, NULL, 'z'},
		{"owner-well-known",       no_argument, NULL, 'y'},
		{NULL              ,       no_argument, NULL, 0},
	};
	TSS_FLAG initFlag = TSS_PCRS_STRUCT_INFO_SHORT;
	UINT32 localityValue = TPM_LOC_ZERO | TPM_LOC_ONE | TPM_LOC_TWO |
			       TPM_LOC_THREE | TPM_LOC_FOUR;

	initIntlSys();

	if (genericOptHandler
		    (argc, argv, "i:s:p:o:a:r:w:f:yzu", hOpts,
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

	if (selectedPcrsReadLen) {
		UINT32 pcrSize;
		BYTE *pcrValue;
		UINT32 i;

		for (i = 0; i < selectedPcrsReadLen; i++) {
			if (selectedPcrsRead[i] > 15) {
				logError(_("Cannot seal NVRAM area to PCR > 15\n"));
				goto out_close;
			}
		}

		if (contextCreateObject(hContext, TSS_OBJECT_TYPE_PCRS, initFlag,
					&hPcrsRead) != TSS_SUCCESS)
			goto out_close;

		for (i = 0; i < selectedPcrsReadLen; i++) {
			if (tpmPcrRead(hTpm, selectedPcrsRead[i], &pcrSize, &pcrValue) !=
			    TSS_SUCCESS)
				goto out_close;

			if (pcrcompositeSetPcrValue(hPcrsRead, selectedPcrsRead[i],
						    pcrSize, pcrValue)
					!= TSS_SUCCESS)
				goto out_close;
		}
	}

	if (selectedPcrsWriteLen) {
		UINT32 pcrSize;
		BYTE *pcrValue;
		UINT32 i;

		for (i = 0; i < selectedPcrsWriteLen; i++) {
			if (selectedPcrsWrite[i] > 15) {
				logError(_("Cannot seal NVRAM area to PCR > 15\n"));
				goto out_close;
			}
		}

		if (contextCreateObject(hContext, TSS_OBJECT_TYPE_PCRS, initFlag,
					&hPcrsWrite) != TSS_SUCCESS)
			goto out_close;

		for (i = 0; i < selectedPcrsWriteLen; i++) {
			if (tpmPcrRead(hTpm, selectedPcrsWrite[i], &pcrSize, &pcrValue) !=
			    TSS_SUCCESS)
				goto out_close;

			if (pcrcompositeSetPcrValue(hPcrsWrite, selectedPcrsWrite[i],
						    pcrSize, pcrValue)
					!= TSS_SUCCESS)
				goto out_close;
		}
	}

	if (filename) {
		FILE *f;

		f = fopen(filename, "r");
		if (!f) {
			logError(_("Could not access file '%s'\n"), filename);
			goto out_close_obj;
		}

		if (parseNVPermsFile(f, &hContext, &nvObject, &hPcrsRead, &hPcrsWrite)
		    != TSS_SUCCESS)
			goto out_close_obj;
	}

	if (hPcrsRead)
		if (pcrcompositeSetPcrLocality(hPcrsRead, localityValue) != TSS_SUCCESS)
			goto out_close;

	if (hPcrsWrite)
		if (pcrcompositeSetPcrLocality(hPcrsWrite, localityValue) != TSS_SUCCESS)
			goto out_close;

	if (NVDefineSpace(nvObject, hPcrsRead, hPcrsWrite) != TSS_SUCCESS)
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
