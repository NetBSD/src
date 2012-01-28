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

//Controled by input options
static char in_filename[PATH_MAX] = "";
static BOOL isInputSet = FALSE;
TSS_HCONTEXT hContext = 0;

static void help(const char *aCmd)
{
	logCmdHelp(aCmd);
	logCmdOption("-i, --infile FILE",
		     _("Filename containing the secret data used to revoke the EK."));

}

static int parse(const int aOpt, const char *aArg)
{
	switch (aOpt) {
	case 'i':
		isInputSet = TRUE;
		if (aArg){
			strncpy(in_filename, aArg, PATH_MAX);
		}
		break;
	default:
		return -1;
	}

	return 0;
}

static TSS_RESULT
tpmRevokeEk(TSS_HTPM a_hTpm, UINT32 revDataSz, BYTE *revData)
{
	TSS_RESULT result = Tspi_TPM_RevokeEndorsementKey( a_hTpm, revDataSz, revData);
	tspiResult("Tspi_TPM_RevokeEndorsementKey", result);
	return result;
}

static int readData(UINT32 bytesToRead, BYTE **buffer)
{
	FILE *infile = NULL;
	size_t iBytes;
	int rc = 0;
	BYTE eofile;

	infile = fopen(in_filename, "r");
	if ( !infile ){
		logError(_("Unable to open input file: %s\n"),
				in_filename);
		return -1;
	}

	//Read the data
	iBytes = fread( *buffer, 1, bytesToRead, infile );
	if ( iBytes < bytesToRead ){
		logError(_("Error: the secret data file %s contains less than %d bytes. Aborting with %s...\n"),
				in_filename, bytesToRead);
		rc = -1;
	} else if ( (iBytes = fread( &eofile, 1, 1, infile )) ) {
		//Test if there's more than 20 bytes
		if ( !feof( infile))
			logMsg(_("WARNING: Using only the first %d bytes of file %s for secret data\n"),
					bytesToRead, in_filename);
	} else {
		logDebug(_("Read %d bytes of secret data from file %s.\n"),
				bytesToRead, in_filename);
	}

	fclose( infile);
	return rc;
}

int main(int argc, char **argv)
{
	TSS_RESULT tResult;
	TSS_HTPM hTpm;
	int iRc = -1;
	struct option opts[] = {
	{"infile", required_argument, NULL, 'i'},
	};
	BYTE revokeData[TPM_SHA1BASED_NONCE_LEN];
	BYTE *revData = revokeData;

        initIntlSys();

	if (genericOptHandler(argc, argv, "i:", opts, sizeof(opts) / sizeof(struct option), parse,
			      help) != 0)
		goto out;

	if (isInputSet) {
		if (readData(sizeof(revokeData), &revData))
			goto out;
	} else {
		logError(_("Please specify which file contains the secret to revoke the Ek (use option -i, --infile).\n"));
		goto out;
	}

	logDebug("Input file name: %s\n", in_filename);

	if (contextCreate(&hContext) != TSS_SUCCESS)
		goto out;

	if (contextConnect(hContext) != TSS_SUCCESS)
		goto out_close;

	if (contextGetTpm(hContext, &hTpm) != TSS_SUCCESS)
		goto out_close;

	tResult = tpmRevokeEk(hTpm, sizeof(revokeData), revData);
	if (tResult != TSS_SUCCESS)
		goto out_close;

	iRc = 0;
	logSuccess(argv[0]);

      out_close:
	contextClose(hContext);

      out:
	return iRc;
}
