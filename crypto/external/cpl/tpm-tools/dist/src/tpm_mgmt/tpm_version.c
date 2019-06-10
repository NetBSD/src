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
#include <sys/stat.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <stdlib.h>		//for def. exit
#include <unistd.h>
#include "tpm_tspi.h"


TSS_HCONTEXT hContext = 0;

// Redirect all stderr output to a temporary file.
// Returns a cloned version of the original stderr, for use with end_capture_stderr().
int capture_stderr()
{
  // Save a copy of stderr.
  int olderr = dup(STDERR_FILENO);
  if (olderr < 0) {
    perror("dup(STDERR_FILENO)");
    return -1;
  }

  // Open file that will become new stderr.
  FILE* f = tmpfile();
  if (f == NULL) {
    close(olderr);
    perror("tmpfile()");
    return -1;
  }

  // Override stderr with temp file.
  if (0 > dup2(fileno(f), STDERR_FILENO)) {
    fclose(f);
    close(olderr);
    perror("dup2(, STDERR_FILENO)");
    return -1;
  }

  // Old handle for temp file no longer needed.
  fclose(f);
  return olderr;
}

// Restore stderr and return a newly allocated buffer with the captured stderr output.
// Caller frees the returned buffer.
char* end_capture_stderr(int olderr)
{
  char* buf = NULL;
  struct stat st;
  int memsize;

  if (olderr == -1) {
    // capture_stderr() must have failed. Nothing to restore.
    return strdup("");
  }

  // Find out how much to read.
  if (0 > fstat(STDERR_FILENO, &st)) {
    perror("fstat()");
    goto errout;
  }
  memsize = st.st_size + 1;
  if (memsize <= st.st_size) {
    // Something fishy is going on. Fall back to getting 100 bytes (arbitrary).
    perror("int overflow");
    memsize = 100;
    st.st_size = memsize - 1;
  }
  if (!(buf = malloc(memsize))) {
    perror("malloc()");
    goto errout;
  }

  // Read file content.
  if (0 > lseek(STDERR_FILENO, 0, SEEK_SET)) {
    perror("lseek()");
    goto errout;
  }
  if (st.st_size != read(STDERR_FILENO, buf, st.st_size)) {
    perror("read()");
  }

  // Restore stderr.
 errout:
  if (0 > dup2(olderr, STDERR_FILENO)) {
    perror("dup2()");
    return buf;
  }
  if (0 > close(olderr)) {
    perror("close()");
  }
  return buf;
}

int cmdVersion(const char *a_szCmd)
{
	TSS_HTPM hTpm;
	UINT32 uiSubCap;
	BYTE *pSubCap;
	UINT32 uiResultLen;
	BYTE *pResult;
	int iRc = -1;

	if (contextCreate(&hContext) != TSS_SUCCESS)
		goto out;

	if (contextConnect(hContext) != TSS_SUCCESS)
		goto out_close;

	if (contextGetTpm(hContext, &hTpm) != TSS_SUCCESS)
		goto out_close;
#ifdef TSS_LIB_IS_12
	{
		UINT64 offset;
		TSS_RESULT uiResult;
		TPM_CAP_VERSION_INFO versionInfo;
		char *errbuf = NULL; // Buffer containing what was sent to stderr during getCapability.

		/* Disable logging to of "Bad Mode" during this call. 
		 * If we're on a 1.1 TPM, it'd throw an error.
		 * Save data sent to stderr in case it was any other type of error.
		 */
	        int olderr;   // Saved copy of stderr.

		// Redirect stderr to tempfile.
		olderr = capture_stderr();

		// Get capabilities. Stderr output is captured.
		uiResult = getCapability(hTpm, TSS_TPMCAP_VERSION_VAL, 0, NULL, &uiResultLen,
					 &pResult);

		// Restore output to stderr.
		errbuf = end_capture_stderr(olderr);

		// Don't print errors due to "Bad Mode", that just means we're on a 1.1 TPM.
		if (uiResult == TPM_E_BAD_MODE) {
		        free(errbuf);
			goto print_cap_version;
		}

		fprintf(stderr, "%s", errbuf);
		free(errbuf);

		if (uiResult != TSS_SUCCESS) {
			goto out_close;
		}

		offset = 0;
		if ((uiResult = unloadVersionInfo(&offset, pResult, &versionInfo))) {
			goto out_close;
		}

		logMsg(_("  TPM 1.2 Version Info:\n"));
		logMsg(_("  Chip Version:        %hhu.%hhu.%hhu.%hhu\n"),
		       versionInfo.version.major, versionInfo.version.minor,
		       versionInfo.version.revMajor, versionInfo.version.revMinor);
		logMsg(_("  Spec Level:          %hu\n"), versionInfo.specLevel);
		logMsg(_("  Errata Revision:     %hhu\n"), versionInfo.errataRev);
		logMsg(_("  TPM Vendor ID:       %c%c%c%c\n"),
		       versionInfo.tpmVendorID[0], versionInfo.tpmVendorID[1],
		       versionInfo.tpmVendorID[2], versionInfo.tpmVendorID[3]);

		if (versionInfo.vendorSpecificSize) {
			logMsg(_("  Vendor Specific data: "));
			logHex(versionInfo.vendorSpecificSize, versionInfo.vendorSpecific);

			free(versionInfo.vendorSpecific);
		}
	}

print_cap_version:
#endif
	if (getCapability(hTpm, TSS_TPMCAP_VERSION, 0, NULL, &uiResultLen,
			  &pResult) != TSS_SUCCESS)
		goto out_close;
	logMsg(_("  TPM Version:         "));
	logHex(uiResultLen, pResult);

	uiSubCap = TSS_TPMCAP_PROP_MANUFACTURER;
	pSubCap = (BYTE *) & uiSubCap;
	if (getCapability(hTpm, TSS_TPMCAP_PROPERTY, sizeof(uiSubCap),
			  pSubCap, &uiResultLen, &pResult) != TSS_SUCCESS)
		goto out_close;
	logMsg(_("  Manufacturer Info:   "));
	logHex(uiResultLen, pResult);

	iRc = 0;
	logSuccess(a_szCmd);

      out_close:
	contextClose(hContext);

      out:
	return iRc;
}


int main(int argc, char *argv[])
{
	int rc;

        initIntlSys();

	rc = genericOptHandler(argc, argv, "", NULL, 0, NULL, NULL);
	if (rc)
		exit(0);

	rc = cmdVersion(argv[0]);

	return rc;
}
