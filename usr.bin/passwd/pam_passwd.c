/*	$NetBSD: pam_passwd.c,v 1.2 2005/02/24 05:11:34 thorpej Exp $	*/

/*-
 * Copyright (c) 2002 Networks Associates Technologies, Inc.
 * All rights reserved.
 *
 * This software was developed for the FreeBSD Project by ThinkSec AS and
 * NAI Labs, the Security Research Division of Network Associates, Inc.
 * under DARPA/SPAWAR contract N66001-01-C-8035 ("CBOSS"), as part of the
 * DARPA CHATS research program.
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
__FBSDID("$FreeBSD: src/usr.bin/passwd/passwd.c,v 1.23 2003/04/18 21:27:09 nectar Exp $");
#else
__RCSID("$NetBSD: pam_passwd.c,v 1.2 2005/02/24 05:11:34 thorpej Exp $");
#endif

#include <sys/param.h>

#include <err.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <syslog.h>
#include <unistd.h>

#include "extern.h"

#include <security/pam_appl.h>
#include <security/openpam.h>

static pam_handle_t *pamh;
static struct pam_conv pamc = {
	openpam_ttyconv,
	NULL
};

static const char	*progname;
static const char	*yp_domain;
static const char	*yp_server;
static int	 usage = PW_DONT_USE;

#define pam_check(func) do { \
	if (pam_err != PAM_SUCCESS) { \
		if (pam_err == PAM_AUTH_ERR || pam_err == PAM_PERM_DENIED || \
		    pam_err == PAM_AUTHTOK_RECOVERY_ERR) \
			warnx("sorry"); \
		else \
			warnx("%s(): %s", func, pam_strerror(pamh, pam_err)); \
		goto end; \
	} \
} while (0)


int
pwpam_init(const char *pname)
{
	progname = pname;
	return 0;
}

int
pwpam_arg(char arg, const char *optarg)
{
	switch (arg) {
	case 'p':
		usage = PW_USE_FORCE;
		break;
	case 'd':
		yp_domain = optarg;
		break;
	case 's':
		yp_server = optarg;
		break;
	default:
		return 0;
	}
	return 1;
}

int
pwpam_arg_end(void)
{
	return usage;
}

void
pwpam_end(void)
{
	/* NOOP */
}

int
pwpam_chpw(const char *uname)
{
	int pam_err;
	char hostname[MAXHOSTNAMELEN + 1];

	/* initialize PAM */
	pam_err = pam_start(progname, uname, &pamc, &pamh);
	pam_check("pam_start");

	pam_err = pam_set_item(pamh, PAM_TTY, ttyname(STDERR_FILENO));
	pam_check("pam_set_item");
	(void)gethostname(hostname, sizeof hostname);
	pam_err = pam_set_item(pamh, PAM_RHOST, hostname);
	pam_check("pam_set_item");
	pam_err = pam_set_item(pamh, PAM_RUSER, getlogin());
	pam_check("pam_set_item");

	/* set YP domain and host */
	pam_err = pam_set_data(pamh, "yp_domain", __UNCONST(yp_domain), NULL);
	pam_check("pam_set_data");
	pam_err = pam_set_data(pamh, "yp_server", __UNCONST(yp_server), NULL);
	pam_check("pam_set_data");

	/* set new password */
	pam_err = pam_chauthtok(pamh, 0);
	pam_check("pam_chauthtok");

 end:
	pam_end(pamh, pam_err);
	return pam_err == PAM_SUCCESS ? 0 : 1;
}
