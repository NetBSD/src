/*
 * The Initial Developer of the Original Code is International
 * Business Machines Corporation. Portions created by IBM
 * Corporation are Copyright (C) 2009 International Business
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
#include "tpm_unseal.h"

static void help(const char *aCmd)
{
	logCmdHelp(aCmd);
	logCmdOption("-i, --infile FILE",
		     _
		     ("Filename containing data to unseal."));
	logCmdOption("-o, --outfile FILE",
		     _
		     ("Filename to write unsealed data to.  Default is STDOUT."));
	logCmdOption("-z, --srk-well-known",
		     _
		     ("Use 20 bytes of zeros (TSS_WELL_KNOWN_SECRET) as the SRK secret."));
}

static char in_filename[PATH_MAX] = "", out_filename[PATH_MAX] = "";
static BOOL srkWellKnown = FALSE;

static int parse(const int aOpt, const char *aArg)
{
	int rc = -1;

	switch (aOpt) {
	case 'i':
		if (aArg) {
			strncpy(in_filename, aArg, PATH_MAX);
			rc = 0;
		}
		break;
	case 'o':
		if (aArg) {
			strncpy(out_filename, aArg, PATH_MAX);
			rc = 0;
		}
		break;
	case 'z':
		srkWellKnown = TRUE;
		rc = 0;
		break;
	default:
		break;
	}
	return rc;

}

int main(int argc, char **argv)
{

	struct option opts[] =
	    { {"infile", required_argument, NULL, 'i'},
	      {"outfile", required_argument, NULL, 'o'},
	      {"srk-well-known", no_argument, NULL, 'z'},
	};
	FILE *fp;
	int rc=0, tss_size=0, i;
	unsigned char* tss_data = NULL;

	if (genericOptHandler(argc, argv, "i:o:z", opts,
			      sizeof(opts) / sizeof(struct option), parse,
			      help) != 0)
		return rc;
	
	rc = tpmUnsealFile(in_filename, &tss_data, &tss_size, srkWellKnown);

	if (strlen(out_filename) == 0) {
		for (i=0; i < tss_size; i++)
			printf("%c", tss_data[i]);
		goto out;
	} else if ((fp = fopen(out_filename, "w")) == NULL) {
			logError(_("Unable to open output file\n"));
			goto out;
	}

	if (fwrite(tss_data, tss_size, 1, fp) != 1) {
		logError(_("Unable to write output file\n"));
		goto out;
	}
	fclose(fp);
out:
	free(tss_data);
	return rc;
}
