/*	$NetBSD: pam_nologin.c,v 1.7 2008/01/27 01:23:20 christos Exp $	*/

/*-
 * Copyright 2001 Mark R V Murray
 * All rights reserved.
 * Copyright (c) 2001 Networks Associates Technology, Inc.
 * All rights reserved.
 *
 * Portions of this software were developed for the FreeBSD Project by
 * ThinkSec AS and NAI Labs, the Security Research Division of Network
 * Associates, Inc.  under DARPA/SPAWAR contract N66001-01-C-8035
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
 */

#include <sys/cdefs.h>
#ifdef __FreeBSD__
__FBSDID("$FreeBSD: src/lib/libpam/modules/pam_nologin/pam_nologin.c,v 1.10 2002/04/12 22:27:21 des Exp $");
#else
__RCSID("$NetBSD: pam_nologin.c,v 1.7 2008/01/27 01:23:20 christos Exp $");
#endif


#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <login_cap.h>
#include <pwd.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define PAM_SM_AUTH

#include <security/pam_appl.h>
#include <security/pam_modules.h>
#include <security/pam_mod_misc.h>

#define	NOLOGIN	"/etc/nologin"

static char nologin_def[] = NOLOGIN;

PAM_EXTERN int
pam_sm_authenticate(pam_handle_t *pamh, int flags __unused,
    int argc __unused, const char *argv[] __unused)
{
	login_cap_t *lc;
	struct passwd *pwd, pwres;
	struct stat st;
	int retval, fd;
	int ignorenologin = 0;
	int rootlogin = 0;
	const char *user, *nologin;
	char *mtmp;
	char pwbuf[1024];

	if ((retval = pam_get_user(pamh, &user, NULL)) != PAM_SUCCESS)
		return retval;

	PAM_LOG("Got user: %s", user);

	/*
	 * For root, the default is to ignore nologin, but the 
	 * ignorenologin capability can override this, so we
	 * set the default appropriately.
	 * 
	 * Do not allow login of unexisting users, so that a directory
	 * failure will not cause the nologin capability to be ignored.
	 */
	if (getpwnam_r(user, &pwres, pwbuf, sizeof(pwbuf), &pwd) != 0 ||
	    pwd == NULL) {
		return PAM_USER_UNKNOWN;
	} else {
		if (pwd->pw_uid == 0)
			rootlogin = 1;
	}

	lc = login_getclass(pwd->pw_class);
	ignorenologin = login_getcapbool(lc, "ignorenologin", rootlogin);
	nologin = login_getcapstr(lc, "nologin", nologin_def, nologin_def);
	login_close(lc);
	lc = NULL;

	if (ignorenologin)
		return PAM_SUCCESS;

	if ((fd = open(nologin, O_RDONLY, 0)) == -1) {
		/*
		 * The file does not exist, login is granted
		 */
		if (errno == ENOENT)
			return PAM_SUCCESS;

		/* 
		 * open failed, but the file exists. This could be
		 * a temporary problem (system resources exausted): 
		 * Refuse the login.
		 */
		PAM_LOG("Cannot open %s file: %s", nologin, strerror(errno));
		return PAM_AUTH_ERR;
	}

	PAM_LOG("Opened %s file", nologin);

	if (fstat(fd, &st) < 0)
		return PAM_AUTH_ERR;

	mtmp = malloc(st.st_size + 1);
	if (mtmp != NULL) {
		read(fd, mtmp, st.st_size);
		mtmp[st.st_size] = '\0';
		pam_error(pamh, "%s", mtmp);
		free(mtmp);
	}

	PAM_VERBOSE_ERROR("Administrator refusing you: %s", nologin);

	return PAM_AUTH_ERR;
}

PAM_EXTERN int
pam_sm_setcred(pam_handle_t *pamh __unused, int flags __unused,
    int argc __unused, const char *argv[] __unused)
{

	return (PAM_SUCCESS);
}

PAM_MODULE_ENTRY("pam_nologin");
