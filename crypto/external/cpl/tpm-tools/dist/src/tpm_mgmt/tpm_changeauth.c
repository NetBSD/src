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
#include "tpm_utils.h"
#include "tpm_tspi.h"

struct changeAuth {
	char *name;
	char *prompt;
	BOOL change;
};

static BOOL changeRequested = FALSE;
static BOOL origUnicode = FALSE;
static BOOL newUnicode = FALSE;
static BOOL wellKnown = FALSE;
static BOOL setWellKnown = FALSE;
TSS_HCONTEXT hContext = 0;

//Order important so you authenticate once even if both changed with one command
enum {
	srk =  0,
	owner
};

static struct changeAuth auths[] = {
		{N_("SRK"), N_("Enter new SRK password: "), FALSE},
		{N_("owner"), N_("Enter new owner password: "), FALSE},
		{NULL, NULL, FALSE },
	};

static void help(const char *aCmd)
{
	logCmdHelp(aCmd);
	logUnicodeCmdOption();
	logCmdOption("-o, --owner", _("Change the owner password."));
	logCmdOption("-s, --srk", _("Change the SRK password."));
	logCmdOption("-g, --original_password_unicode", _("Use TSS UNICODE encoding for original password to comply with applications using TSS popup boxes"));
	logCmdOption("-n, --new_password_unicode", _("Use TSS UNICODE encoding for new password to comply with applications using TSS popup boxes"));
	logCmdOption("-z, --well-known", _("Change password to a new one when current owner password is a secret of all zeros (20 bytes of zeros). It must be specified which password (owner, SRK or both) to change"));
	logCmdOption("-r, --set-well-known", _("Change password to a secret of all zeros (20 bytes of zeros). It must be specified which password (owner, SRK or both) to change"));
}

static int parse(const int aOpt, const char *aArg)
{

	switch (aOpt) {

	case 'o':
		auths[owner].change = TRUE;
		changeRequested = TRUE;
		break;
	case 's':
		auths[srk].change = TRUE;
		changeRequested = TRUE;
		break;
	case 'g':
		origUnicode = TRUE;
		break;
	case 'n':
		newUnicode = TRUE;
		break;
	case 'z':
		wellKnown = TRUE;
		break;
	case 'r':
		setWellKnown = TRUE;
		break;
	default:
		return -1;
	}
	return 0;
}

static TSS_RESULT
tpmChangeAuth(TSS_HCONTEXT aObjToChange,
	      TSS_HOBJECT aParent, TSS_HPOLICY aNewPolicy)
{
	TSS_RESULT result =
	    Tspi_ChangeAuth(aObjToChange, aParent, aNewPolicy);
	tspiResult("Tspi_ChangeAuth", result);

	return result;
}

/*
 * Affect: Change owner or srk password
 * Default: No action
 * Required: Owner authentication
 */
int main(int argc, char **argv)
{

	int i = 0, iRc = -1;
	char *passwd = NULL;
	int pswd_len;
	TSS_HPOLICY hTpmPolicy, hNewPolicy;
	TSS_HTPM hTpm;
	TSS_HTPM hSrk;
	BYTE well_known_secret[] = TSS_WELL_KNOWN_SECRET;
	struct option opts[] = { {"owner", no_argument, NULL, 'o'},
	{"srk", no_argument, NULL, 's'},
	{"original_password_unicode", no_argument, NULL, 'g'},
	{"new_password_unicode", no_argument, NULL, 'n'},
	{"well-known", no_argument, NULL, 'z'},
	{"set-well-known", no_argument, NULL, 'r'},
	};

	initIntlSys();

	if (genericOptHandler
	    (argc, argv, "zrsogn", opts, sizeof(opts) / sizeof(struct option),
	     parse, help) != 0)
		goto out;

	//nothing selected
	if ((!changeRequested && wellKnown) || (!changeRequested)) {
		help(argv[0]);
		goto out;
	}

	//Connect to TSS and TPM
	if (contextCreate(&hContext) != TSS_SUCCESS)
		goto out;

	if (contextConnect(hContext) != TSS_SUCCESS)
		goto out_close;

	if (contextGetTpm(hContext, &hTpm) != TSS_SUCCESS)
		goto out_close;

	if (wellKnown) {
		passwd = (char *)well_known_secret;
		pswd_len = TCPA_SHA1_160_HASH_LEN;
	} else {
		passwd = _GETPASSWD(_("Enter owner password: "), &pswd_len,
			FALSE, origUnicode || useUnicode );
		if (!passwd) {
			logError(_("Failed to get owner password\n"));
			goto out_close;
		}
	}

	if (policyGet(hTpm, &hTpmPolicy) != TSS_SUCCESS)
		goto out_close;

	if (policySetSecret(hTpmPolicy, pswd_len, (BYTE *)passwd) != TSS_SUCCESS)
		goto out_close;

	if (!wellKnown && !setWellKnown) {
		shredPasswd(passwd);
		passwd = NULL;
	}

	do {
		if (auths[i].change) {
			logInfo(_("Changing password for: %s.\n"), _(auths[i].name));
			if (setWellKnown) {
				passwd = (char *)well_known_secret;
				pswd_len = TCPA_SHA1_160_HASH_LEN;
			} else {
				passwd = _GETPASSWD(_(auths[i].prompt), &pswd_len,
					TRUE, newUnicode || useUnicode );
				if (!passwd) {
					logError(_("Failed to get new password.\n"));
					goto out_close;
				}
			}

			if (contextCreateObject
			    (hContext, TSS_OBJECT_TYPE_POLICY, TSS_POLICY_USAGE,
			     &hNewPolicy) != TSS_SUCCESS)
				goto out_close;

			if (policySetSecret
			    (hNewPolicy, pswd_len,
			     (BYTE *)passwd) != TSS_SUCCESS)
				goto out_close;

			if (i == owner) {
				if (tpmChangeAuth(hTpm, NULL_HOBJECT, hNewPolicy) != TSS_SUCCESS)
					goto out_close;
			} else if (i == srk) {
				if (keyLoadKeyByUUID
				    (hContext, TSS_PS_TYPE_SYSTEM,
				     SRK_UUID, &hSrk) != TSS_SUCCESS)
					goto out_close;
				if (tpmChangeAuth(hSrk, hTpm, hNewPolicy) != TSS_SUCCESS)
					goto out_close;
			}
			logInfo(_("Change of %s password successful.\n"), _(auths[i].name));
			if (!wellKnown && !setWellKnown) {
				shredPasswd(passwd);
				passwd = NULL;
			}
		}
	} while (auths[++i].name);

	iRc = 0;

	out_close:
		contextClose(hContext);

	out:
		if (passwd && !wellKnown && !setWellKnown)
			shredPasswd(passwd);

	return iRc;
}
