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
#include <arpa/inet.h>

#include "tpm_tspi.h"
#include "tpm_utils.h"
#include "tpm_nvcommon.h"


static BOOL nvindex_set;
static unsigned int nvindex;
static BOOL list_only;
TSS_HCONTEXT hContext = 0;

static int parse(const int aOpt, const char *aArg)
{

	switch (aOpt) {
	case 'i':
		if (parseHexOrDecimal(aArg, &nvindex, 0, UINT_MAX,
				      "NVRAM index") != 0)
			return -1;

		nvindex_set = TRUE;
		list_only = FALSE;

		break;

	case 'n':
		list_only = TRUE;
		nvindex_set = FALSE;
		break;

	default:
		return -1;
	}
	return 0;
}


static void help(const char* aCmd)
{
	logCmdHelp(aCmd);
	logNVIndexCmdOption();
	logCmdOption("-n, --list-only",
		     _("Only list the defined NVRAM areas' indices."));
}


static void pcrInfoShortDisplay(TPM_PCR_INFO_SHORT *tpis, const char *type)
{
	UINT16 i, c;

	c = 0;

	logMsg("PCR %sselection:\n", type);

	for (i = 0; i < tpis->pcrSelection.sizeOfSelect * 8; i++) {
		if (tpis->pcrSelection.pcrSelect[(i / 8)] & (1 << (i & 0x7))) {
			if (!c)
				logMsg(" PCRs	 : ");
			if (c)
				logMsg(", ");
			printf("%d", i);
			c++;
		}
	}

	if (c)
		logMsg("\n");

	if (tpis->localityAtRelease) {
		if (tpis->localityAtRelease == 0x1f) {
			logMsg(" Localities   : ALL\n");
		} else {
			logMsg(" Localities   : 0x%01x\n", tpis->localityAtRelease);
		}
	}

	if (c) {
		logMsg(" Hash	 : ");
		for (i = 0; i < 20; i++)
			logMsg("%02x", tpis->digestAtRelease.digest[i]);
		logMsg("\n");
	}
}


static void nvindexDisplay(TSS_HTPM hTpm, UINT32 nvindex)
{
	TSS_RESULT res;
	char *buffer;
	TPM_NV_DATA_PUBLIC *nvpub = NULL;

	logMsg("NVRAM index   : 0x%08x (%u)\n", nvindex, nvindex);

	res = getNVDataPublic(hTpm, nvindex, &nvpub);

	if (res != TSS_SUCCESS)
		goto out;

	pcrInfoShortDisplay(&nvpub->pcrInfoRead , "read  ");
	pcrInfoShortDisplay(&nvpub->pcrInfoWrite, "write ");

	buffer = printValueAsStrings((unsigned int)nvpub->permission.attributes,
	                             permvalues);

	logMsg("Permissions   : 0x%08x (%s)\n", nvpub->permission.attributes, buffer);
	free(buffer);
	buffer = NULL;

	logMsg("bReadSTClear  : %s\n", nvpub->bReadSTClear ? "TRUE" : "FALSE");
	logMsg("bWriteSTClear : %s\n", nvpub->bWriteSTClear ? "TRUE" : "FALSE");
	logMsg("bWriteDefine  : %s\n", nvpub->bWriteDefine ? "TRUE" : "FALSE");

	logMsg("Size          : %d (0x%x)\n", nvpub->dataSize, nvpub->dataSize);


     out:
	freeNVDataPublic(nvpub);

	return;
}


int main(int argc, char **argv)
{
	TSS_HTPM hTpm;
	UINT32 ulResultLen;
	BYTE *pResult = NULL;
	int iRc = -1;
	unsigned int i;
	struct option hOpts[] = {
		{"index"    , required_argument, NULL, 'i'},
		{"list-only",       no_argument, NULL, 'n'},
		{NULL       ,       no_argument, NULL, 0},
	};

	initIntlSys();

	if (genericOptHandler
		    (argc, argv, "i:o:n", hOpts,
		     sizeof(hOpts) / sizeof(struct option), parse, help) != 0)
		goto out;

	if (contextCreate(&hContext) != TSS_SUCCESS)
		goto out;

	if (contextConnect(hContext) != TSS_SUCCESS)
		goto out_close;

	if (contextGetTpm(hContext, &hTpm) != TSS_SUCCESS)
		goto out_close;


	if (getCapability(hTpm, TSS_TPMCAP_NV_LIST, 0, NULL,
			  &ulResultLen, &pResult) != TSS_SUCCESS) {
		goto out_close;
	}

	if (list_only) {
		logMsg(_("The following NVRAM areas have been defined:\n"));
	}

	for (i = 0; i < ulResultLen/sizeof(UINT32); i++) {
		UINT32 nvi;
		nvi = Decode_UINT32(pResult + i * sizeof(UINT32));

		if (list_only) {
			logMsg("0x%08x (%d)\n", nvi, nvi);
		} else {
			if ((nvindex_set && nvi == (UINT32)nvindex) ||
			     !nvindex_set) {
				nvindexDisplay(hTpm, nvi);
				logMsg("\n");
			}
		}
	}

	iRc = 0;

      out_close:
	contextClose(hContext);

      out:

	return iRc;
}
