/*	$NetBSD: pam_get_user.c,v 1.2.4.2 2012/04/17 00:03:59 yamt Exp $	*/

/*-
 * Copyright (c) 2002-2003 Networks Associates Technology, Inc.
 * Copyright (c) 2004-2011 Dag-Erling Smørgrav
 * All rights reserved.
 *
 * This software was developed for the FreeBSD Project by ThinkSec AS and
 * Network Associates Laboratories, the Security Research Division of
 * Network Associates, Inc.  under DARPA/SPAWAR contract N66001-01-C-8035
 * ("CBOSS"), as part of the DARPA CHATS research program.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote
 *    products derived from this software without specific prior written
 *    permission.
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
 *
 * Id: pam_get_user.c 455 2011-10-29 18:31:11Z des
 */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <sys/param.h>

#include <stdlib.h>

#include <security/pam_appl.h>
#include <security/openpam.h>

#include "openpam_impl.h"

static const char user_prompt[] = "Login:";

/*
 * XSSO 4.2.1
 * XSSO 6 page 52
 *
 * Retrieve user name
 */

int
pam_get_user(pam_handle_t *pamh,
	const char **user,
	const char *prompt)
{
	char prompt_buf[1024];
	size_t prompt_size;
	const void *promptp;
	char *resp;
	int r;

	ENTER();
	if (pamh == NULL || user == NULL)
		RETURNC(PAM_SYSTEM_ERR);
	r = pam_get_item(pamh, PAM_USER, (const void **)user);
	if (r == PAM_SUCCESS && *user != NULL)
		RETURNC(PAM_SUCCESS);
	/* pam policy overrides the module's choice */
	if ((promptp = openpam_get_option(pamh, "user_prompt")) != NULL)
		prompt = promptp;
	/* no prompt provided, see if there is one tucked away somewhere */
	if (prompt == NULL)
		if (pam_get_item(pamh, PAM_USER_PROMPT, &promptp) &&
		    promptp != NULL)
			prompt = promptp;
	/* fall back to hardcoded default */
	if (prompt == NULL)
		prompt = user_prompt;
	/* expand */
	prompt_size = sizeof prompt_buf;
	r = openpam_subst(pamh, prompt_buf, &prompt_size, prompt);
	if (r == PAM_SUCCESS && prompt_size <= sizeof prompt_buf)
		prompt = prompt_buf;
	r = pam_prompt(pamh, PAM_PROMPT_ECHO_ON, &resp, "%s", prompt);
	if (r != PAM_SUCCESS)
		RETURNC(r);
	r = pam_set_item(pamh, PAM_USER, resp);
	FREE(resp);
	if (r != PAM_SUCCESS)
		RETURNC(r);
	r = pam_get_item(pamh, PAM_USER, (const void **)user);
	RETURNC(r);
	/*NOTREACHED*/
}

/*
 * Error codes:
 *
 *	=pam_get_item
 *	=pam_prompt
 *	=pam_set_item
 *	!PAM_SYMBOL_ERR
 */

/**
 * The =pam_get_user function returns the name of the target user, as
 * specified to =pam_start.
 * If no user was specified, nor set using =pam_set_item, =pam_get_user
 * will prompt for a user name.
 * Either way, a pointer to the user name is stored in the location
 * pointed to by the =user argument.
 *
 * The =prompt argument specifies a prompt to use if no user name is
 * cached.
 * If it is =NULL, the =PAM_USER_PROMPT item will be used.
 * If that item is also =NULL, a hardcoded default prompt will be used.
 * Either way, the prompt is expanded using =openpam_subst before it is
 * passed to the conversation function.
 *
 * If =pam_get_user is called from a module and the ;user_prompt option is
 * set in the policy file, the value of that option takes precedence over
 * both the =prompt argument and the =PAM_USER_PROMPT item.
 *
 * >pam_get_item
 * >pam_get_authtok
 * >openpam_subst
 */
