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

TSS_HCONTEXT hContext = 0;

#ifdef TSS_LIB_IS_12
#include <limits.h>

//Controled by input options
static char in_filename[PATH_MAX] = "", out_filename[PATH_MAX] = "";
static BOOL isRevocable = FALSE;
static BOOL needGenerateSecret = FALSE;
static BOOL inFileSet = FALSE;
static BOOL outFileSet = FALSE;

static void help(const char *aCmd)
{
	logCmdHelp(aCmd);
	logCmdOption("-r, --revocable",
		     _("Creates a revocable EK instead of the default non-revocable one. Requires [-g -o] or [-i]"));
	logCmdOption("-i, --infile FILE",
		     _("Filename containing the secret data used to revoke the EK."));
	logCmdOption("-g, --generate-secret",
		     _("Generates a 20 Bytes secret that is used to revoke the EK. Requires [-o]"));
	logCmdOption("-o, --outfile FILE",
		     _("Filename to write the secret data generated to revoke the EK."));

}

static int parse(const int aOpt, const char *aArg)
{
	switch (aOpt){
	case 'r':
		isRevocable = TRUE;
		break;
	case 'g':
		needGenerateSecret = TRUE;
		break;
	case 'i':
		inFileSet = TRUE;
		if (aArg){
			strncpy(in_filename, aArg, PATH_MAX);
		}
		break;
	case 'o':
		outFileSet = TRUE;
		if (aArg){
			strncpy(out_filename, aArg, PATH_MAX);
		}
		break;
	default:
		return -1;
	}
	return 0;

}

static TSS_RESULT
tpmCreateRevEk(TSS_HTPM a_hTpm, TSS_HKEY a_hKey,
	    TSS_VALIDATION * a_pValData, UINT32 *revDataSz, BYTE **revData)
{
	TSS_RESULT result = Tspi_TPM_CreateRevocableEndorsementKey(a_hTpm, a_hKey,
	a_pValData, revDataSz, revData);
	tspiResult("Tspi_TPM_CreateRevocableEndorsementKey", result);
	return result;
}

static int readData(UINT32 bytesToRead, BYTE **buffer)
{
	FILE *infile = NULL;
	size_t iBytes;
	int rc = 0;
	BYTE eofile;

	memset(*buffer, 0x00, bytesToRead);
	infile = fopen(in_filename, "r");
	if ( !infile ){
		logError(_("Unable to open input file: %s\n"),
				in_filename);
		return -1;
	}

	//Read the data
	iBytes = fread( *buffer, 1, bytesToRead, infile );
	if ( iBytes < bytesToRead ) {
		logError(_("Error: the secret data file %s contains less than %d bytes. Aborting ...\n"),
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

static int writeData(UINT32 bytesToWrite, BYTE *buffer)
{
	FILE *outfile = NULL;
	size_t iBytes;
	int rc = 0;

	logDebug(_("bytesToWrite: %d\n"), bytesToWrite);
	outfile = fopen(out_filename, "w");
	if ( !outfile ) {
		logError(_("Unable to open output file: %s\n"), out_filename);
		return -1;
	}

	//Write data in buffer
	iBytes = fwrite( buffer, 1, bytesToWrite, outfile);
	if ( iBytes != bytesToWrite ) {
		logError(_("Error: Unable to write %d bytes on the file %s.\n"),
				 bytesToWrite, out_filename);
		rc = -1;
	}

	logDebug(_("%zd bytes written on file %s.\n"), iBytes, out_filename);
	fclose( outfile );
	return rc;

}
#endif

static TSS_RESULT
tpmCreateEk(TSS_HTPM a_hTpm, TSS_HKEY a_hKey,
	    TSS_VALIDATION * a_pValData)
{

	TSS_RESULT result = Tspi_TPM_CreateEndorsementKey(a_hTpm, a_hKey,
			 a_pValData);
	tspiResult("Tspi_TPM_CreateEndorsementKey", result);
	return result;
}

int main(int argc, char **argv)
{
	TSS_RESULT tResult;
	TSS_HTPM hTpm;
	TSS_HKEY hEk;
	TSS_FLAG fEkAttrs;
	int iRc = -1;

#ifdef TSS_LIB_IS_12
	struct option opts[] = {{"revocable", no_argument, NULL, 'r'},
	{"generate-secret", no_argument, NULL, 'g'},
	{"infile", required_argument, NULL, 'i'},
	{"outfile", required_argument, NULL, 'o'},
	};
	UINT32 revDataSz;
	BYTE revokeData[TPM_SHA1BASED_NONCE_LEN];
	BYTE *pRevData;
#endif

	initIntlSys();

#ifdef TSS_LIB_IS_12
	if (genericOptHandler(argc, argv, "rgi:o:", opts, sizeof(opts) / sizeof(struct option),
			      parse, help) != 0)
		goto out;

	//Check commands for command hierarchy
	if (isRevocable) {
		if (needGenerateSecret) {
			if (!outFileSet) {
				logError(_("Please specify an output file\n"));
				goto out;
			}
			if (inFileSet) {
				logError(_("The option -i, --infile is not valid with -g\n"));
				goto out;
			}
		} else if (!inFileSet) {
			logError(_("Please specify -i, --infile or -g, --generate-secret\n"));
			goto out;
		} else if (outFileSet) {
			logError(_("The option -o, --outfile is not valid with -i, --infile"));
			goto out;
		}
	}
	logDebug("Input file name: %s\n", in_filename);
	logDebug("Output file name: %s\n", out_filename);

	if (inFileSet) {
		pRevData = revokeData;
		revDataSz = sizeof(revokeData);
		if (readData(revDataSz, &pRevData))
			goto out;
	} else if (outFileSet) {
		FILE *outfile = fopen(out_filename, "w");
		if (!outfile) {
			iRc = -1;
			logError(_("Unable to open output file: %s\n"), out_filename);
			goto out;
		}
		fclose(outfile);

		//TPM should generate the revoke data
		revDataSz = 0;
		pRevData = NULL;
	}
#else
	if (genericOptHandler(argc, argv, NULL, NULL, 0, NULL, NULL) != 0){
		logError(_("See man pages for details.\n"));
		goto out;
	}
#endif

	if (contextCreate(&hContext) != TSS_SUCCESS)
		goto out;

	if (contextConnect(hContext) != TSS_SUCCESS)
		goto out_close;

	if (contextGetTpm(hContext, &hTpm) != TSS_SUCCESS)
		goto out_close;

	//Initialize EK attributes here
	fEkAttrs = TSS_KEY_SIZE_2048 | TSS_KEY_TYPE_LEGACY;
	if (contextCreateObject(hContext, TSS_OBJECT_TYPE_RSAKEY, fEkAttrs, &hEk) != TSS_SUCCESS)
		goto out_close;

#ifdef TSS_LIB_IS_12
	if (isRevocable){
		tResult = tpmCreateRevEk(hTpm, hEk, NULL, &revDataSz, &pRevData);
		if (tResult != TSS_SUCCESS)
			goto out_close;
		//Writes the generated secret into the output file
		if (outFileSet) {
			if (writeData(revDataSz, pRevData)) {
				logError(_("Creating revocable EK succeeded, but writing the EK "
					   "revoke authorization to disk failed.\nPrinting the "
					   "revoke authorization instead:\n"));
				logHex(revDataSz, pRevData);
				logError(_("You should record this data, as its the authorization "
					   "you'll need to revoke your EK!\n"));
				goto out_close;
			}
		}
	} else
#endif
		tResult = tpmCreateEk(hTpm, hEk, NULL);
	if (tResult != TSS_SUCCESS)
		goto out_close;

	iRc = 0;
	logSuccess(argv[0]);

      out_close:
	contextClose(hContext);

      out:
	return iRc;
}
