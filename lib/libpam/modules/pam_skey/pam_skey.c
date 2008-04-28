/*	$NetBSD: pam_skey.c,v 1.3 2008/04/28 20:23:01 martin Exp $	*/

/*-
 * Copyright (c) 2005 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe.
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
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__RCSID("$NetBSD: pam_skey.c,v 1.3 2008/04/28 20:23:01 martin Exp $");

#include <sys/types.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <skey.h>

#define	PAM_SM_AUTH

#include <security/pam_appl.h>
#include <security/pam_modules.h>
#include <security/pam_mod_misc.h>

/*
 * authentication management
 */
PAM_EXTERN int
/*ARGSUSED*/
pam_sm_authenticate(pam_handle_t *pamh, int flags __unused,
    int argc __unused, const char *argv[] __unused)
{
	const char *user, *skinfo, *pass;
	char *response;
	int retval;
	char skprompt[80];

	if (openpam_get_option(pamh, PAM_OPT_AUTH_AS_SELF)) {
		user = getlogin();
	} else {
		retval = pam_get_user(pamh, &user, NULL);
		if (retval != PAM_SUCCESS)
			return (retval);
		PAM_LOG("Got user: %s", user);
	}

	if (skey_haskey(user) != 0)
		return (PAM_SERVICE_ERR);	/* XXX PAM_AUTHINFO_UNAVAIL? */

	skinfo = skey_keyinfo(user);
	if (skinfo == NULL) {
		PAM_VERBOSE_ERROR("Error getting S/Key challenge");
		return (PAM_SERVICE_ERR);
	}

	(void) snprintf(skprompt, sizeof(skprompt),
	    "Password [ %s ]:", skinfo);

	retval = pam_get_authtok(pamh, PAM_AUTHTOK, &pass, skprompt);
	if (retval != PAM_SUCCESS)
		return (retval);

	response = strdup(pass);
	if (response == NULL) {
		pam_error(pamh, "Unable to copy S/Key response");
		return (PAM_SERVICE_ERR);
	}

	retval = skey_passcheck(user, response) == -1 ?
	    PAM_AUTH_ERR : PAM_SUCCESS;

	free(response);

	return (retval);
}

PAM_EXTERN int
/*ARGSUSED*/
pam_sm_setcred(pam_handle_t *pamh __unused, int flags __unused,
    int argc __unused, const char *argv[] __unused)
{

	return (PAM_SUCCESS);
}

PAM_MODULE_ENTRY("pam_skey");
