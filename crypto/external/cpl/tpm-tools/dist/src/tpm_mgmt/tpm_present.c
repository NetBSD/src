/*
 * The Initial Developer of the Original Code is International
 * Business Machines Corporation. Portions created by IBM
 * Corporation are Copyright (C) 2005, 2006 International Business
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

/*
 * Never set change to false.
 * Value doesn't matter for the locks.
 */
struct physFlag {
	char *name;
	TSS_FLAG property;
	BOOL change;
	BOOL value;
};

/*
 * Order is important.
 * First must set cmd and hw enable correctly followed by the lock.
 * Then setting presence can be attempted followed by the lock
 */
enum {
	cmdEnable = 0,
	hwdEnable,
	lifeLock,
	present,
	lock,
	num_flags
};

static struct physFlag flags[] = {
	{N_("Command Enable"), TSS_TPMSTATUS_PHYSPRES_CMDENABLE},
	{N_("Hardware Enable"), TSS_TPMSTATUS_PHYSPRES_HWENABLE},
	{N_("Lifetime Lock"), TSS_TPMSTATUS_PHYSPRES_LIFETIMELOCK},
	{N_("Physical Presence"), TSS_TPMSTATUS_PHYSPRESENCE},
	{N_("Lock"), TSS_TPMSTATUS_PHYSPRES_LOCK},
	{0, 0, 0, 0}
};
static BOOL bCheck = FALSE;
static BOOL bChangeRequested = FALSE;
static BOOL bYes = FALSE;
static TSS_BOOL bValue;
static BOOL isWellKnown = FALSE;
static BYTE well_known[] = TSS_WELL_KNOWN_SECRET;
TSS_HCONTEXT hContext = 0;


static void help(const char *aCmd)
{
	logCmdHelp(aCmd);
	logUnicodeCmdOption();
	logCmdOption("-s, --status",
		     _("Report current physical presence states."));
	logCmdOption("-a, --assert", _("Assert that admin is present."));
	logCmdOption("-c, --clear", _("Clear assertion of admin presence."));
	logCmdOption("--lock",
		     _("Lock TPM presence assertion into specified state."));
	logCmdOption("--enable-cmd",
		     _("Allow TPM to accept Physical Presence Command."));
	logCmdOption("--disable-cmd",
		     _("Disallow TPM to accept Physical Presence Command."));
	logCmdOption("--enable-hw",
		     _("Allow TPM to accept Hardware Physical Presence."));
	logCmdOption("--disable-hw",
		     _("Disallow TPM to accept Hardware Physical Presence."));
	logCmdOption("--set-lifetime-lock",
		     _("Prevent further modification of TPM Physical Presence\n\t\tCommand and Hardware Enablement states.\n\t\tTHIS ACTION IS PERMANENT AND CAN NEVER BE UNDONE."));
	logCmdOption("-y, --yes",
		     _("Automatically respond yes to all prompts.  Only use\n\t\tthis if you are sure of the current state and don't want\n\t\tany textra checking done before setting the lifetime lock"));
	logCmdOption("-z, --well-known",
		     _("Use 20 bytes of zeros (TSS_WELL_KNOWN_SECRET) as the TPM secret authorization data."));
}

static int parse(const int aOpt, const char *aArg)
{
	switch (aOpt) {
	case 's':
		logDebug(_("Changing mode to check status.\n"));
		bCheck = TRUE;
		break;
	case 'a':
		logDebug(_("Changing mode to assert presence.\n"));
		flags[present].change = TRUE;
		flags[present].value = TRUE;
		bChangeRequested = TRUE;
		break;
	case 'c':
		logDebug(_("Changing mode to clear presence.\n"));
		flags[present].change = TRUE;
		flags[present].value = FALSE;
		bChangeRequested = TRUE;
		break;
	case 'k':
		logDebug(_("Changing mode to lock presence.\n"));
		flags[lock].change = TRUE;
		flags[lock].value = TRUE;
		bChangeRequested = TRUE;
		break;
	case 'm':
		logDebug(_("Changing mode to enable command presence.\n"));
		flags[cmdEnable].change = TRUE;
		flags[cmdEnable].value = TRUE;
		bChangeRequested = TRUE;
		break;
	case 'd':
		logDebug(_("Changing mode to disable command presence.\n"));
		flags[cmdEnable].change = TRUE;
		flags[cmdEnable].value = FALSE;
		bChangeRequested = TRUE;
		break;
	case 'e':
		logDebug(_("Changing mode to enable hardware presence.\n"));
		flags[hwdEnable].change = TRUE;
		flags[hwdEnable].value = TRUE;
		bChangeRequested = TRUE;
		break;
	case 'h':
		logDebug(_("Changing mode to disable hardware presence.\n"));
		flags[hwdEnable].change = TRUE;
		flags[hwdEnable].value = FALSE;
		bChangeRequested = TRUE;
		break;
	case 't':
		logDebug(_("Changing mode to set lifetime presence lock.\n"));
		flags[lifeLock].change = TRUE;
		flags[lifeLock].value = TRUE;
		bChangeRequested = TRUE;
		break;
	case 'y':
		logDebug(_("Changing mode to automatically answer yes.\n"));
		bYes = TRUE;
		break;
	case 'z':
		logDebug(_("Using TSS_WELL_KNOWN_SECRET to authorize the TPM command\n"));
		isWellKnown = TRUE;
		break;
	default:
		return -1;
	}
	return 0;

}

static BOOL confirmLifeLock(TSS_HCONTEXT hContext, TSS_HTPM hTpm)
{

	TSS_BOOL bCmd, bHwd;
	BOOL bRc;
	TSS_HPOLICY hTpmPolicy;
	char *pwd = NULL;
	int pswd_len;
	char rsp[5];
	int scanCount;

	//get status w/o owner auth (FAILS 1.1, should PASS 1.2)
	if (tpmGetStatus(hTpm, flags[cmdEnable].property, &bCmd) !=
	    TSS_SUCCESS
	    || tpmGetStatus(hTpm, flags[hwdEnable].property,
			    &bHwd) != TSS_SUCCESS) {
		logDebug
		    (_("Unable to determine current state without authorization\n"));
		if (isTpmOwned(hContext)) {
			logDebug(_("TPM is owned\n"));
			if (isWellKnown) {
				pwd = (char *)well_known;
				pswd_len = sizeof(well_known);
			} else {
				// Prompt for owner password
				pwd = GETPASSWD(_("Enter owner password: "), &pswd_len, FALSE);
				if (!pwd) {
					logMsg(_("Failed to get password\n"));
					goto warn;
				}
			}

			if (policyGet(hTpm, &hTpmPolicy) != TSS_SUCCESS)
				goto warn;

			if (policySetSecret(hTpmPolicy, pswd_len, (BYTE *)pwd)
			    != TSS_SUCCESS)
				goto warn;
			//get status w/ owner auth
			if (tpmGetStatus
			    (hTpm, flags[cmdEnable].property,
			     &bCmd) != TSS_SUCCESS
			    || tpmGetStatus(hTpm,
					    flags[hwdEnable].property,
					    &bHwd) != TSS_SUCCESS) {
				logDebug
				    (_("Unable to determine current state with the entered password.\n"));
				goto warn;
			}
			goto give_vals;
		} else {	//can't determine values
		      warn:
			logMsg
			    (_("Unable to programatically determine the current setting of TPM Physcial Presence Command Enable and Hardware Enable states.  Make sure you are aware of and comfortable with the current states.\n"));
		}
	} else {
	      give_vals:
		logMsg(_("Current State:\n"));
		logMsg("\t%s: %s\n", _(flags[cmdEnable].name), logBool(mapTssBool(bCmd)));
		logMsg("\t%s: %s\n", _(flags[hwdEnable].name), logBool(mapTssBool(bHwd)));
		logMsg
		    (_("These will be the permanent values if you choose to proceed.\n"));
	}
	logMsg
	    (_("This command cannot be undone.  Are you sure you want to continue?[y/N]\n"));
	scanCount = scanf("%5s", rsp);

	 /* TRANSLATORS: this should be the affirmative letter that was  prompted for in the message corresponding to: "Are you sure you want to continue?[y/N]" */ 
	if (scanCount >= 1 && strcmp(rsp, _("y")) == 0) {
		logMsg
		    (_("Setting the lifetime lock was confirmed.\nContinuing.\n"));
		bRc = TRUE;

	} else {
		logMsg
		    (_("Continuing to set the lifetime lock was declined.\nAction canceled.\n"));
		bRc = FALSE;
	}

	if (hTpmPolicy)
		policyFlushSecret(hTpmPolicy);

	if (pwd && !isWellKnown)
		shredPasswd(pwd);

	return bRc;
}



/*
 * Affect: Toggle TPM presence states
 * Default: Display current states 
 * Requires: Display requires owner auth.  
 * 	Lifetime lock will attempt owner auth to warn about current states before confirming
 */
int main(int argc, char **argv)
{

	char *szTpmPasswd = NULL;
	int pswd_len;
	TSS_HTPM hTpm;
	TSS_HPOLICY hTpmPolicy;
	int iRc = -1;
	int i = 0;
	struct option opts[] = { {"status", no_argument, NULL, 's'},
	{"assert", no_argument, NULL, 'a'},
	{"clear", no_argument, NULL, 'c'},
	{"lock", no_argument, NULL, 'k'},
	{"enable-cmd", no_argument, NULL, 'm'},
	{"disable-cmd", no_argument, NULL, 'd'},
	{"enable-hw", no_argument, NULL, 'e'},
	{"disable-hw", no_argument, NULL, 'w'},
	{"set-lifetime-lock", no_argument, NULL, 't'},
	{"yes", no_argument, NULL, 'y'},
	{"well-known", no_argument, NULL, 'z'},
	};

	initIntlSys();

	if (genericOptHandler
	    (argc, argv, "acsyz", opts,
	     sizeof(opts) / sizeof(struct option), parse, help) != 0)
		goto out;

	//Connect to TSS and TPM
	if (contextCreate(&hContext) != TSS_SUCCESS)
		goto out;

	if (contextConnect(hContext) != TSS_SUCCESS)
		goto out_close;

	if (contextGetTpm(hContext, &hTpm) != TSS_SUCCESS)
		goto out_close;

	if (bCheck || !bChangeRequested) {
		logInfo(_("Checking current status: \n"));
		if (isWellKnown) {
			szTpmPasswd = (char *)well_known;
			pswd_len = sizeof(well_known);
		} else {
			// Prompt for owner password
			szTpmPasswd = GETPASSWD(_("Enter owner password: "), &pswd_len, FALSE);
			if (!szTpmPasswd) {
				logMsg(_("Failed to get password\n"));
				goto out_close;
			}
		}

		if (policyGet(hTpm, &hTpmPolicy) != TSS_SUCCESS)
			goto out_close;

		if (policySetSecret(hTpmPolicy, pswd_len,
				    (BYTE *)szTpmPasswd) != TSS_SUCCESS)
			goto out_close;

		logMsg(_("Physical Presence Status:\n"));

		do {
			if (tpmGetStatus(hTpm, flags[i].property,
					 &bValue) != TSS_SUCCESS)
				goto out_close;
			logMsg("\t%s: %s\n", _(flags[i].name),
			       logBool(mapTssBool(bValue)));
		} while (flags[++i].name);

		goto out_success;
	}

	do {
		if (flags[i].change) {
			logInfo(_("Requested to Change %s to %s\n"),
				_(flags[i].name), logBool(flags[i].value));

			if (i == lifeLock && !
			    (bYes || confirmLifeLock(hContext, hTpm)))
				continue;

			if (tpmSetStatus(hTpm, flags[i].property,
					 flags[i].value) != TSS_SUCCESS) {
				logError(_("Change to %s Failed\n"),
					 _(flags[i].name));
				goto out;
			}
			logInfo(_("Change to %s Successful\n"),
				_(flags[i].name));
		}
	} while (flags[++i].name);

out_success:
	logSuccess(argv[0]);
	iRc = 0;
out_close:
	contextClose(hContext);
out:
    if (szTpmPasswd && !isWellKnown)
	shredPasswd( szTpmPasswd );
	return iRc;
}
