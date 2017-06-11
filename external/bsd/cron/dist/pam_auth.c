#include "cron.h"

#ifdef USE_PAM

#include <security/pam_appl.h>

static pam_handle_t *pamh = NULL;
static const struct pam_conv cron_conv;

int
cron_pam_start (const char *username)
{
	int     retval;

	if (pamh)
		return 0;

	retval = pam_start ("cron", username, &cron_conv, &pamh);
	log_close ();
	if (retval != PAM_SUCCESS)
	{
		pamh = NULL;
		log_it ("CRON", getpid (), "pam_start failed",
			pam_strerror (pamh, retval));
		return 0;
	}
	retval = pam_authenticate (pamh, PAM_SILENT);
	log_close ();
	if (retval != PAM_SUCCESS)
	{
		log_it ("CRON", getpid (), "pam_authenticate failed",
			pam_strerror (pamh, retval));
		pam_end (pamh, retval);
		pamh = NULL;
		return 0;
	}
	retval = pam_acct_mgmt (pamh, PAM_SILENT);
	log_close ();
	if (retval != PAM_SUCCESS)
	{
		log_it ("CRON", getpid (), "pam_acct_mgmt failed",
			pam_strerror (pamh, retval));
		pam_end (pamh, retval);
		pamh = NULL;
		return 0;
	}
	retval = pam_open_session (pamh, PAM_SILENT);
	log_close ();
	if (retval != PAM_SUCCESS)
	{
		log_it ("CRON", getpid (), "pam_open_session failed",
			pam_strerror (pamh, retval));
		pam_end (pamh, retval);
		pamh = NULL;
		return 0;
	}

	return 1;
}

int
cron_pam_setcred (void)
{
	int     retval;

	if (!pamh)
		return 0;

	retval = pam_setcred (pamh, PAM_ESTABLISH_CRED | PAM_SILENT);
	log_close ();
	if (retval != PAM_SUCCESS)
	{
		log_it ("CRON", getpid (), "pam_setcred failed",
			pam_strerror (pamh, retval));
		pam_end (pamh, retval);
		pamh = NULL;
		log_close ();
		return 0;
	}

	return 1;
}

void
cron_pam_finish (void)
{
	if (!pamh)
		return;

	pam_close_session (pamh, 0);
	pam_end (pamh, 0);
	pamh = NULL;
	log_close ();
}

#ifndef PAM_DATA_SILENT
#define PAM_DATA_SILENT 0
#endif

void
cron_pam_child_close (void)
{
	pam_end (pamh, PAM_DATA_SILENT);
	pamh = NULL;
	log_close ();
}

char  **
cron_pam_getenvlist (char **envp)
{
	if (!pamh || !envp)
		return 0;

	for (; *envp; ++envp)
		if (pam_putenv (pamh, *envp) != PAM_SUCCESS)
			return 0;

	return pam_getenvlist (pamh);
}

#endif /* USE_PAM */
