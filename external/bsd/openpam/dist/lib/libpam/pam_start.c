/*	$NetBSD: pam_start.c,v 1.3 2017/05/06 19:50:10 christos Exp $	*/

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
 * $OpenPAM: pam_start.c 938 2017-04-30 21:34:42Z des $
 */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <sys/cdefs.h>
__RCSID("$NetBSD: pam_start.c,v 1.3 2017/05/06 19:50:10 christos Exp $");

#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <security/pam_appl.h>

#include "openpam_impl.h"
#include "openpam_strlcpy.h"


/*
 * XSSO 4.2.1
 * XSSO 6 page 89
 *
 * Initiate a PAM transaction
 */

int
pam_start(const char *service,
	const char *user,
	const struct pam_conv *pam_conv,
	pam_handle_t **pamh)
{
	char *hostname = NULL;
	struct pam_handle *ph;
	int r;
	size_t hostname_size;
	long h;

#ifdef _SC_HOST_NAME_MAX
	h = sysconf(_SC_HOST_NAME_MAX);
#else
	h = -1;
#endif
	hostname_size = (size_t)(h < 10 ? 1024 : h) + 1;

	ENTER();
	if ((ph = calloc((size_t)1, sizeof *ph)) == NULL)
		RETURNC(PAM_BUF_ERR);
	if ((r = pam_set_item(ph, PAM_SERVICE, service)) != PAM_SUCCESS)
		goto fail;
	if ((hostname = malloc(hostname_size)) == NULL)
		goto fail;
	if (gethostname(hostname, hostname_size) != 0)
		strlcpy(hostname, "localhost", hostname_size);
	if ((r = pam_set_item(ph, PAM_HOST, hostname)) != PAM_SUCCESS)
		goto fail;
	if ((r = pam_set_item(ph, PAM_USER, user)) != PAM_SUCCESS)
		goto fail;
	if ((r = pam_set_item(ph, PAM_CONV, pam_conv)) != PAM_SUCCESS)
		goto fail;
	if ((r = openpam_configure(ph, service)) != PAM_SUCCESS)
		goto fail;
	free(hostname);
	*pamh = ph;
	openpam_log(PAM_LOG_DEBUG, "pam_start(\"%s\") succeeded", service);
	RETURNC(PAM_SUCCESS);
fail:
	free(hostname);
	pam_end(ph, r);
	RETURNC(r);
}

/*
 * Error codes:
 *
 *	=openpam_configure
 *	=pam_set_item
 *	!PAM_SYMBOL_ERR
 *	PAM_BUF_ERR
 */

/**
 * The =pam_start function creates and initializes a PAM context.
 *
 * The =service argument specifies the name of the policy to apply, and is
 * stored in the =PAM_SERVICE item in the created context.
 *
 * The =user argument specifies the name of the target user - the user the
 * created context will serve to authenticate.
 * It is stored in the =PAM_USER item in the created context.
 *
 * The =pam_conv argument points to a =struct pam_conv describing the
 * conversation function to use; see =pam_conv for details.
 *
 * >pam_get_item
 * >pam_set_item
 * >pam_end
 */
