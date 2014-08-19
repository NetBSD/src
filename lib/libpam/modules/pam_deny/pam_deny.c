/*	$NetBSD: pam_deny.c,v 1.2.54.1 2014/08/20 00:02:19 tls Exp $	*/

/*-
 * Copyright 2001 Mark R V Murray
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
#ifdef __FreeBSD__
__FBSDID("$FreeBSD: src/lib/libpam/modules/pam_deny/pam_deny.c,v 1.9 2002/04/12 22:27:19 des Exp $");
#else
__RCSID("$NetBSD: pam_deny.c,v 1.2.54.1 2014/08/20 00:02:19 tls Exp $");
#endif

#include <stddef.h>
#include <string.h>
#include <syslog.h>

#define PAM_SM_AUTH
#define PAM_SM_ACCOUNT
#define PAM_SM_SESSION
#define PAM_SM_PASSWORD

#include <security/pam_appl.h>
#include <security/pam_modules.h>

PAM_EXTERN int
pam_sm_authenticate(pam_handle_t *pamh, int flags __unused,
    int argc __unused, const char *argv[] __unused)
{
	const char *user;
	int r;

	if ((r = pam_get_user(pamh, &user, NULL)) != PAM_SUCCESS)
		return (r);

	return (PAM_AUTH_ERR);
}

PAM_EXTERN int
pam_sm_setcred(pam_handle_t *pamh __unused, int flags __unused,
    int argc __unused, const char *argv[] __unused)
{

	return (PAM_CRED_ERR);
}

PAM_EXTERN int
pam_sm_acct_mgmt(pam_handle_t *pamh __unused, int flags __unused,
    int argc __unused, const char *argv[] __unused)
{

	return (PAM_AUTH_ERR);
}

PAM_EXTERN int
pam_sm_chauthtok(pam_handle_t *pamh __unused, int flags,
    int argc, const char *argv[])
{
	int prelim_ignore = 0;
	int i;

	for (i = 0; i < argc; i++) {
		if (strcmp(argv[i], "prelim_ignore") == 0)
			prelim_ignore = 1;
		else if (strcmp(argv[i], "debug") == 0)
			/* nothing */;
		else
			syslog(LOG_ERR, "illegal option %s", argv[i]);
	}

	if (flags & PAM_PRELIM_CHECK && prelim_ignore)
		return (PAM_IGNORE);
	else
		return (PAM_AUTHTOK_ERR);
}

PAM_EXTERN int
pam_sm_open_session(pam_handle_t *pamh __unused, int flags __unused,
    int argc __unused, const char *argv[] __unused)
{

	return (PAM_SESSION_ERR);
}

PAM_EXTERN int
pam_sm_close_session(pam_handle_t *pamh __unused, int flags __unused,
    int argc __unused, const char *argv[] __unused)
{

	return (PAM_SESSION_ERR);
}

PAM_MODULE_ENTRY("pam_deny");
