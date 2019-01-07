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

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <limits.h>

#include "tpm_nvcommon.h"
#include "tpm_tspi.h"
#include "tpm_utils.h"

static BOOL nvindex_set;
static unsigned int nvindex;
static unsigned int offset;
static unsigned int length;
static int fillvalue = -1;
static const char *filename;
static BOOL passWellKnown;
static BOOL askPassword;
static const char *password;
static char *data;

TSS_HCONTEXT hContext = 0;


static int parse(const int aOpt, const char *aArg)
{

	switch (aOpt) {
	case 'i':
		if (parseHexOrDecimal(aArg, &nvindex, 0, UINT_MAX,
				      "NVRAM index") != 0)
			return -1;

		nvindex_set = 1;

		break;

	case 's':
		if (parseHexOrDecimal(aArg, &length, 0, UINT_MAX,
				      "length of data") != 0)
			return -1;
		break;

	case 'n':
		if (parseHexOrDecimal(aArg, &offset, 0, UINT_MAX,
				      "write offset") != 0)
			return -1;
		break;

	case 'd':
		data = strdup(aArg);
		if (data == NULL) {
			logError(_("Out of memory\n"));
			return -1;
		}
		break;

	case 'f':
		filename = aArg;
		break;

	case 'm':
		if (parseHexOrDecimal(aArg, (unsigned int *)&fillvalue,
		                      0, UCHAR_MAX,
				      "fill value") != 0)
			return -1;
		break;

	case 'p':
		password = aArg;
		if (!password)
			askPassword = TRUE;
		else
			askPassword = FALSE;
		passWellKnown =  FALSE;
		break;

	case 'z':
		password = NULL;
		passWellKnown =  TRUE;
		askPassword = FALSE;
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
	logCmdOption("-z, --well-known",
		     _("Use 20 bytes of zeros (TSS_WELL_KNOWN_SECRET) as the TPM secret authorization data"));
	logCmdOption("-p, --password",
		     _("Owner or NVRAM area password depending on permissions"));
	logNVIndexCmdOption();
	logCmdOption("-s, --size",
		     _("Number of bytes to write to the NVRAM area"));
	logCmdOption("-n, --offset",
		     _("Offset at which to start writing into the NVRAM area"));
	logCmdOption("-f, --filename",
		     _("File whose contents to write into the NVRAM area"));
	logCmdOption("-d, --data",
		     _("Data to write into the NVRAM area"));
	logCmdOption("-m, --fill-value",
		     _("The byte to fill the NVRAM area with"));
}

int main(int argc, char **argv)
{

	TSS_HTPM hTpm;
	TSS_HNVSTORE nvObject;
	TSS_FLAG fNvAttrs;
	UINT32 ulDataLength, bytesToWrite, off;
	BYTE *rgbDataToWrite = NULL;
	TSS_HPOLICY hTpmPolicy, hDataPolicy;
	TPM_NV_DATA_PUBLIC *nvpub = NULL;
	int iRc = -1;
	BYTE well_known_secret[] = TSS_WELL_KNOWN_SECRET;
	int pswd_len = -1;
	struct option hOpts[] = {
		{"index"      , required_argument, NULL, 'i'},
		{"size"       , required_argument, NULL, 's'},
		{"offset"     , required_argument, NULL, 'n'},
		{"data"       , required_argument, NULL, 'd'},
		{"filename"   , required_argument, NULL, 'f'},
		{"fillvalue"  , required_argument, NULL, 'm'},
		{"password"   , optional_argument, NULL, 'p'},
		{"use-unicode",       no_argument, NULL, 'u'},
		{"well-known" ,       no_argument, NULL, 'z'},
		{NULL	 ,       no_argument, NULL, 0},
	};
	struct stat statbuf;
	int fd = -1;
	ssize_t read_bytes;

	initIntlSys();

	if (genericOptHandler
		    (argc, argv, "i:s:n:d:f:m:p::zu", hOpts,
		     sizeof(hOpts) / sizeof(struct option), parse, help) != 0)
		goto out;

	if (nvindex_set == 0) {
		logError(_("You must provide an index for the NVRAM area.\n"));
		goto out;
	}

	if (length > 0 && data == NULL &&
	    filename == NULL &&
	    fillvalue == -1) {
		logError(_("Either data, name of file or fill value must be "
			   "provided.\n"));
		goto out;
	}

	if (data) {
		ulDataLength = strlen(data);

		if (length > 0 && (UINT32)length < ulDataLength)
			ulDataLength = length;

		rgbDataToWrite = (BYTE *)data;
		data = NULL;
	} else if (filename) {
		if (stat(filename, &statbuf) != 0) {
			logError(_("Could not access file '%s'\n"),
				 filename);
			goto out;
		}
		ulDataLength = statbuf.st_size;

		if (length > 0 && (UINT32)length < ulDataLength)
			ulDataLength = length;

		rgbDataToWrite = malloc(ulDataLength);
		if (rgbDataToWrite == NULL) {
			logError(_("Out of memory.\n"));
			return -1;
		}
		fd = open(filename, O_RDONLY);
		if (fd < 0) {
			logError(_("Could not open file %s for reading.\n"));
			return -1;
		}
		read_bytes = read(fd, rgbDataToWrite, ulDataLength);

		if (read_bytes < 0 || ulDataLength != (UINT32)read_bytes) {
			logError(_("Error while reading data.\n"));
			return -1;
		}
		close(fd);
		fd = -1;
	} else if (fillvalue >= 0) {
		if (length < 0) {
			logError(_("Requiring size parameter.\n"));
			return -1;
		}
		ulDataLength = length;
		rgbDataToWrite = malloc(ulDataLength);
		if (rgbDataToWrite == NULL) {
			logError(_("Out of memory.\n"));
			return -1;
		}
		__memset(rgbDataToWrite, fillvalue, ulDataLength);
	} else {
		ulDataLength = 0;
	}

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


	if (askPassword) {
		password = _GETPASSWD(_("Enter NVRAM access password: "), &pswd_len,
			FALSE, useUnicode );
		if (!password) {
			logError(_("Failed to get NVRAM access password\n"));
			goto out_close;
		}
	}
	if (password || passWellKnown) {
		if (policyGet(hTpm, &hTpmPolicy) != TSS_SUCCESS)
			goto out_close;

		if (password) {
			if (pswd_len < 0)
				pswd_len = strlen(password);
			if (policySetSecret(hTpmPolicy, strlen(password),
					    (BYTE *)password) != TSS_SUCCESS)
				goto out_close;
		} else {
			if (policySetSecret(hTpmPolicy, TCPA_SHA1_160_HASH_LEN,
					    (BYTE *)well_known_secret) != TSS_SUCCESS)
				goto out_close;
		}

		if (contextCreateObject
		    (hContext, TSS_OBJECT_TYPE_POLICY, TSS_POLICY_USAGE,
		     &hDataPolicy) != TSS_SUCCESS)
			goto out_close;

		if (password) {
			if (policySetSecret(hDataPolicy, strlen(password),
					    (BYTE *)password) != TSS_SUCCESS)
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

	if (nvindex != TPM_NV_INDEX0) {
		if (getNVDataPublic(hTpm, nvindex, &nvpub)  != TSS_SUCCESS) {
			logError(_("Could not get NVRAM area public information.\n"));
			goto out_close_obj;
		}

		if ((UINT32)offset > nvpub->dataSize) {
			logError(_("The offset is outside the NVRAM area's size of "
			           "%u bytes.\n"),
				 nvpub->dataSize);
			goto out_close_obj;
		}

		if ((UINT32)offset + ulDataLength > nvpub->dataSize) {
			logError(_("Writing of data would go beyond the NVRAM area's size "
			           "of %u bytes.\n"),
				 nvpub->dataSize);
			goto out_close_obj;
		}
	}

	if (Tspi_SetAttribUint32(nvObject,
				 TSS_TSPATTRIB_NV_INDEX,
				 0,
				 nvindex) != TSS_SUCCESS)
		goto out_close_obj;


	bytesToWrite = ulDataLength;
	off = offset;

	if (bytesToWrite == 0 &&
	    NVWriteValue(nvObject, 0, 0, NULL) != TSS_SUCCESS)
		goto out_close_obj;

#define WRITE_CHUNK_SIZE   1024
	while (bytesToWrite > 0) {
		UINT32 chunk = (bytesToWrite > WRITE_CHUNK_SIZE)
			       ? WRITE_CHUNK_SIZE
			       : bytesToWrite;
		if (NVWriteValue(nvObject, off, chunk, &rgbDataToWrite[off-offset])
		    != TSS_SUCCESS)
			goto out_close_obj;

		bytesToWrite -= chunk;
		off += chunk;
	}

	logMsg(_("Successfully wrote %d bytes at offset %d to NVRAM index "
	         "0x%x (%u).\n"),
	       ulDataLength, offset, nvindex, nvindex);

	iRc = 0;

	goto out_close;

      out_close_obj:
	contextCloseObject(hContext, nvObject);

      out_close:
	contextClose(hContext);

      out:
	free(rgbDataToWrite);
	freeNVDataPublic(nvpub);

	return iRc;
}
