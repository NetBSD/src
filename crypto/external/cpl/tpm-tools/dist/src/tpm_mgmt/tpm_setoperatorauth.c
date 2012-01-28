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

#include "tpm_utils.h"
#include "tpm_tspi.h"

static BOOL passUnicode = FALSE;
static BOOL isWellKnown = FALSE;
TSS_HCONTEXT hContext = 0;

static void help(const char *aCmd)
{
	logCmdHelp(aCmd);
	logUnicodeCmdOption();
	logCmdOption("-z, --well-known", _("Use TSS_WELL_KNOWN_SECRET as the operator's default secret."));
	logCmdOption("-p, --op_password_unicode", _("Use TSS UNICODE encoding for operator password to comply with applications using TSS popup boxes"));
}

static int parse(const int aOpt, const char *aArg)
{

	switch (aOpt) {
	case 'p':
		passUnicode = TRUE;
		break;
	case 'z':
		isWellKnown = TRUE;
		break;
	default:
		return -1;
	}
	return 0;
}

static TSS_RESULT
tpmSetOpAuth(TSS_HTPM a_hTpm, TSS_HPOLICY aOpPolicy)
{
	TSS_RESULT result = Tspi_TPM_SetOperatorAuth(a_hTpm, aOpPolicy);
	tspiResult("Tspi_TPM_SetOperatorAuth", result);
	return result;
}

int main(int argc, char **argv)
{

	int iRc = -1;
	char *passwd = NULL;
	int pswd_len;
	TSS_HPOLICY hNewPolicy;
	TSS_HTPM hTpm;
	struct option opts[] = {
	{"well-known", no_argument, NULL, 'z'},
	{"op_password_unicode", no_argument, NULL, 'p'},
	};
	BYTE wellKnown[TCPA_SHA1_160_HASH_LEN] = TSS_WELL_KNOWN_SECRET;

	initIntlSys();
	if (genericOptHandler
	    (argc, argv, "zp", opts, sizeof(opts) / sizeof(struct option),
	     parse, help) != 0)
		goto out;

	//Connect to TSS and TPM
	if (contextCreate(&hContext) != TSS_SUCCESS)
		goto out;

	if (contextConnect(hContext) != TSS_SUCCESS)
		goto out_close;

	if (contextGetTpm(hContext, &hTpm) != TSS_SUCCESS)
		goto out_close;

	//Prompt for operator password
	if (!isWellKnown) {
		passwd = _GETPASSWD(_("Enter operator password: "), (int *)&pswd_len, TRUE,
				    passUnicode || useUnicode );
		if (!passwd) {
			logError(_("Failed to get operator password\n"));
			goto out_close;
		}
	} else {
		passwd = (char *)wellKnown;
		pswd_len = sizeof(wellKnown);
	}

	if (contextCreateObject(hContext, TSS_OBJECT_TYPE_POLICY, TSS_POLICY_OPERATOR,
			&hNewPolicy) != TSS_SUCCESS)
		goto out_close;

	if (policySetSecret(hNewPolicy, (UINT32)pswd_len, (BYTE *)passwd) != TSS_SUCCESS)
		goto out_close;

	if (!isWellKnown)
		shredPasswd(passwd);
	passwd = NULL;

	if (tpmSetOpAuth(hTpm, hNewPolicy) != TSS_SUCCESS)
		goto out_close;

	iRc = 0;
	out_close:
	contextClose(hContext);
	out:
	return iRc;
}
