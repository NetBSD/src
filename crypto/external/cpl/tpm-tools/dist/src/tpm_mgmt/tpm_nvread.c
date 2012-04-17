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
#include <ctype.h>
#include <fcntl.h>
#include <unistd.h>

#include "tpm_nvcommon.h"
#include "tpm_tspi.h"
#include "tpm_utils.h"

static unsigned int nvindex;
static unsigned int offset;
static unsigned int length;
static BOOL length_set;
static const char *filename;
static BOOL passWellKnown;
static const char *password;
static BOOL askPassword;

TSS_HCONTEXT hContext = 0;


static int parse(const int aOpt, const char *aArg)
{

	switch (aOpt) {
	case 'i':
		if (parseHexOrDecimal(aArg, &nvindex, 0, UINT_MAX,
				      "NVRAM index") != 0)
			return -1;
		break;

	case 'n':
		if (parseHexOrDecimal(aArg, &offset, 0, UINT_MAX,
				      "read offset") != 0)
			return -1;
		break;

	case 's':
		if (parseHexOrDecimal(aArg, &length, 0, UINT_MAX,
				      "length of data") != 0)
			return -1;
		length_set = TRUE;
		break;

	case 'f':
		filename = aArg;
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

static void displayData(FILE *stream, UINT32 offset, BYTE *data, UINT32 length)
{
	unsigned int len = (length + 0xf) & ~0xf;
	unsigned int c;
	unsigned char buf[17] = { 0, };

	for (c = 0; c < len; c++) {
		if ((c & 0xf) == 0)
			printf("%08x  ", c + offset);

		if (c < length) {
			printf("%02x ", data[c]);
			if (isgraph(data[c]))
				buf[c & 0xf] = data[c];
			else
				buf[c & 0xf] = ' ';
		} else {
			printf("   ");
			buf[c & 0xf] = 0;
		}

		if ((c & 0xf) == 0xf) {
			printf(" %s\n", buf);
		}
	}
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
		     _("Number of bytes to read from the NVRAM area"));
	logCmdOption("-n, --offset",
		     _("Offset at which to start reading from the NVRAM area"));
	logCmdOption("-f, --filename",
		     _("File to write data to."));
}

int main(int argc, char **argv)
{

	TSS_HTPM hTpm;
	TSS_HNVSTORE nvObject;
	TSS_FLAG fNvAttrs;
	TSS_HPOLICY hTpmPolicy, hDataPolicy;
	int iRc = -1;
	int pswd_len = -1;
	BYTE well_known_secret[] = TSS_WELL_KNOWN_SECRET;
	UINT32 ulDataLength;
	BYTE *rgbDataRead = NULL;
	TPM_NV_DATA_PUBLIC *nvpub = NULL;
	struct option hOpts[] = {
		{"index"      , required_argument, NULL, 'i'},
		{"size"       , required_argument, NULL, 's'},
		{"offset"     , required_argument, NULL, 'n'},
		{"filename"   , required_argument, NULL, 'f'},
		{"password"   , optional_argument, NULL, 'p'},
		{"use-unicode",       no_argument, NULL, 'u'},
		{"well-known" ,       no_argument, NULL, 'z'},
		{NULL         ,       no_argument, NULL, 0},
	};
	int fd = -1;
	ssize_t written;

	initIntlSys();

	if (genericOptHandler
		    (argc, argv, "i:s:n:f:p::zu", hOpts,
		     sizeof(hOpts) / sizeof(struct option), parse, help) != 0)
		goto out;

	if (nvindex == 0) {
		logError(_("You must provide an index (!= 0) for the "
		           "NVRAM area.\n"));
		goto out;
	}

	ulDataLength = length;

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
			if (policySetSecret(hTpmPolicy, pswd_len,
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
			if (policySetSecret(hDataPolicy, pswd_len,
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

	if (getNVDataPublic(hTpm, nvindex, &nvpub)  != TSS_SUCCESS) {
		logError(_("Could not get the NVRAM area's public information.\n"));
		goto out_close_obj;
	}

	if (!length_set)
		ulDataLength = nvpub->dataSize;

	if ((UINT32)offset > nvpub->dataSize) {
		logError(_("The offset is outside the NVRAM area's size of "
		           "%u bytes.\n"),
			 nvpub->dataSize);
		goto out_close_obj;
	}

	if ((UINT32)offset + ulDataLength > nvpub->dataSize) {
		ulDataLength = nvpub->dataSize - (UINT32)offset;
	}

	if (Tspi_SetAttribUint32(nvObject,
				 TSS_TSPATTRIB_NV_INDEX,
				 0,
				 nvindex) != TSS_SUCCESS)
		goto out_close_obj;


	if (NVReadValue(nvObject, offset, &ulDataLength, &rgbDataRead) !=
	    TSS_SUCCESS)
		goto out_close_obj;

	if (filename) {
		fd = open(filename, O_WRONLY|O_TRUNC|O_CREAT, S_IRUSR|S_IWUSR);
		if (fd < 0) {
			logError(_("Could not open file %s for writing."),
				 filename);
			goto out_close_obj;
		}

		written = write(fd, rgbDataRead, ulDataLength);

		if (written < 0 || ulDataLength != (UINT32)written) {
			logError(_("Error while writing to file.\n"));
			close(fd);
			fd = -1;
			goto out_close_obj;
		}
		close(fd);
		fd = -1;
		logMsg(_("Successfully wrote data from NVRAM area 0x%x (%u) "
		       "to file.\n"), nvindex, nvindex);
	} else {
		displayData(stdout, offset, rgbDataRead, ulDataLength);
	}

	iRc = 0;

	goto out_close;

      out_close_obj:
	contextCloseObject(hContext, nvObject);

      out_close:
	contextClose(hContext);

      out:
	free(rgbDataRead);
	freeNVDataPublic(nvpub);

	return iRc;
}
