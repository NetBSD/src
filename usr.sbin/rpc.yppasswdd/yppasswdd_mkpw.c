/*	$NetBSD: yppasswdd_mkpw.c,v 1.8 2000/12/08 22:23:14 tron Exp $	*/

/*
 * Copyright (c) 1996 Jason R. Thorpe <thorpej@NetBSD.ORG>
 * All rights reserved.
 *
 * Copyright (c) 1994 Mats O Jansson <moj@stacken.kth.se>
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by Mats O Jansson
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
#ifndef lint
__RCSID("$NetBSD: yppasswdd_mkpw.c,v 1.8 2000/12/08 22:23:14 tron Exp $");
#endif /* not lint */

#include <sys/types.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/wait.h>

#include <err.h>
#include <fcntl.h>
#include <stdio.h>
#include <pwd.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <util.h>
#include <limits.h>

#include <rpc/rpc.h>
#include <rpc/xdr.h>
#include <rpcsvc/yppasswd.h>

#include "extern.h"

int	handling_request;		/* simple mutex */

void
make_passwd(yppasswd *argp, struct svc_req *rqstp, SVCXPRT *transp)
{
	struct passwd *pw;
	int pfd, tfd;
	char mpwd[MAXPATHLEN];

#define REPLY(val)	do { \
		int res = (val); \
		if (!svc_sendreply(transp, xdr_int, (caddr_t)&res)) \
			svcerr_systemerr(transp); \
	} while (0)

#define RETURN(val)	do { \
		REPLY((val)); \
		handling_request = 0; \
		return; \
	} while (0)

	if (handling_request) {
		warnx("already handling request; try again later");
		REPLY(1);
		return;
	}
	handling_request = 1;

	pw = getpwnam(argp->newpw.pw_name);
	if (!pw)
		RETURN(1);

	if (*pw->pw_passwd &&
	    strcmp(crypt(argp->oldpass, pw->pw_passwd), pw->pw_passwd) != 0)
		RETURN(1);

	pw_init();
	tfd = pw_lock(0);
	if (tfd < 0) {
		warnx("the passwd file is busy.");
		RETURN(1);
	}
	(void)strlcpy(mpwd, pw_getprefix(), sizeof(mpwd));
	(void)strlcat(mpwd, _PATH_MASTERPASSWD, sizeof(mpwd));
	pfd = open(mpwd, O_RDONLY, 0);
	if (pfd < 0) {
		pw_abort();
		warnx("%s", mpwd);
		RETURN(1);
	}

	/*
	 * Get the new password.  Reset passwd change time to zero; when
	 * classes are implemented, go and get the "offset" value for this
	 * class and reset the timer.
	 */
	if (!nopw) {
		pw->pw_passwd = argp->newpw.pw_passwd;
		pw->pw_change = 0;
	}
	if (!nogecos)
		pw->pw_gecos = argp->newpw.pw_gecos;
	if (!noshell)
		pw->pw_shell = argp->newpw.pw_shell;

	pw_copy(pfd, tfd, pw, NULL);

	if (pw_mkdb() < 0) {
		warnx("pw_mkdb failed");
		pw_abort();
		RETURN(1);
	}

	/* XXX RESTORE SIGNAL STATE? XXX */

	/* Notify caller we succeeded. */
	REPLY(0);

	/* Update the YP maps. */
	if (chdir("/var/yp"))
		err(EXIT_FAILURE, "/var/yp");
	(void) umask(022);
	(void) system(make_arg);

	handling_request = 0;
}
