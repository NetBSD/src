/*	$NetBSD: pw_yp.c,v 1.10 1997/05/21 02:20:15 lukem Exp $	*/

/*
 * Copyright (c) 1988 The Regents of the University of California.
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
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */
#ifndef lint
#if 0
static char sccsid[] = "@(#)pw_yp.c	1.0 2/2/93";
#else
static char rcsid[] = "$NetBSD: pw_yp.c,v 1.10 1997/05/21 02:20:15 lukem Exp $";
#endif
#endif /* not lint */

#ifdef	YP

#include <stdio.h>
#include <string.h>
#include <netdb.h>
#include <time.h>
#include <pwd.h>
#include <errno.h>
#include <rpc/rpc.h>
#include <rpcsvc/yp_prot.h>
#include <rpcsvc/ypclnt.h>
#define passwd yp_passwd_rec
#include <rpcsvc/yppasswd.h>
#undef passwd

static char *domain;

/*
 * Check if rpc.yppasswdd is running on the master YP server.
 * XXX this duplicates some code, but is much less complex
 * than the alternative.
 */
int
check_yppasswdd()
{
	char *master;
	int rpcport;

	/*
	 * Get local domain
	 */
	if (!domain && yp_get_default_domain(&domain) != 0)
		return (1);

	/*
	 * Find the host for the passwd map; it should be running
	 * the daemon.
	 */
	master = NULL;
	if (yp_master(domain, "passwd.byname", &master) != 0) {
		if (master != NULL)
			free (master);
		return (1);
	}

	/*
	 * Ask the portmapper for the port of the daemon.
	 */
	if ((rpcport = getrpcport(master, YPPASSWDPROG, YPPASSWDPROC_UPDATE,
	    IPPROTO_UDP)) == 0)
		return (1);

	/*
	 * Successful contact with rpc.yppasswdd.
	 */
	return (0);
}

pw_yp(pw, uid)
	struct passwd *pw;
	uid_t uid;
{
	char *master;
	char *pp;
	int r, rpcport, status;
	struct yppasswd yppasswd;
	struct timeval tv;
	CLIENT *client;
	extern char *getpass();
	
	/*
	 * Get local domain
	 */
	if (!domain && (r = yp_get_default_domain(&domain)))
		errx(1, "can't get local YP domain.  Reason: %s",
		    yperr_string(r));

	/*
	 * Find the host for the passwd map; it should be running
	 * the daemon.
	 */
	master = NULL;
	if ((r = yp_master(domain, "passwd.byname", &master)) != 0) {
		if (master)
			free (master);
		warnx("can't find the master YP server.  Reason: %s",
		    yperr_string(r));
		return (1);
	}

	/*
	 * Ask the portmapper for the port of the daemon.
	 */
	if ((rpcport = getrpcport(master, YPPASSWDPROG, YPPASSWDPROC_UPDATE,
	    IPPROTO_UDP)) == 0) {
		warnx("master YP server not running yppasswd daemon.\n\t%s\n",
		    "Can't change password.");
		return (1);
	}

	/*
	 * Be sure the port is priviledged
	 */
	if (rpcport >= IPPORT_RESERVED) {
		warnx("yppasswd daemon is on an invalid port.");
		return (1);
	}

	/* prompt for old password */
	bzero(&yppasswd, sizeof yppasswd);
	yppasswd.oldpass = "none";
	yppasswd.oldpass = getpass("Old password:");
	if (!yppasswd.oldpass) {
		warnx("Cancelled.");
		return (1);
	}

	/* tell rpc.yppasswdd */
	yppasswd.newpw.pw_name	= pw->pw_name;
	yppasswd.newpw.pw_passwd= pw->pw_passwd;
	yppasswd.newpw.pw_uid 	= pw->pw_uid;
	yppasswd.newpw.pw_gid	= pw->pw_gid;
	yppasswd.newpw.pw_gecos = pw->pw_gecos;
	yppasswd.newpw.pw_dir	= pw->pw_dir;
	yppasswd.newpw.pw_shell	= pw->pw_shell;
	
	client = clnt_create(master, YPPASSWDPROG, YPPASSWDVERS, "udp");
	if (client==NULL) {
		warnx("cannot contact yppasswdd on %s:  Reason: %s",
		    master, yperr_string(YPERR_YPBIND));
		return (1);
	}
	client->cl_auth = authunix_create_default();
	tv.tv_sec = 5;
	tv.tv_usec = 0;
	r = clnt_call(client, YPPASSWDPROC_UPDATE,
	    xdr_yppasswd, &yppasswd, xdr_int, &status, tv);
	if (r) {
		warnx("rpc to yppasswdd failed.");
		return (1);
	} else if (status)
		printf("Couldn't change YP password.\n");
	else
		printf("%s %s, %s\n",
		    "The YP password information has been changed on",
		    master, "the master YP passwd server.");
	return (0);
}

static char *
pwskip(p)
	char *p;
{
	while (*p && *p != ':' && *p != '\n')
		++p;
	if (*p)
		*p++ = 0;
	return (p);
}

static struct passwd *
interpret(pwent, line)
	struct passwd *pwent;
	char *line;
{
	char	*p = line;
	int	c;

	pwent->pw_passwd = "*";
	pwent->pw_uid = 0;
	pwent->pw_gid = 0;
	pwent->pw_gecos = "";
	pwent->pw_dir = "";
	pwent->pw_shell = "";
	pwent->pw_change = 0;
	pwent->pw_expire = 0;
	pwent->pw_class = "";
	
	/* line without colon separators is no good, so ignore it */
	if(!strchr(p,':'))
		return(NULL);

	pwent->pw_name = p;
	p = pwskip(p);
	pwent->pw_passwd = p;
	p = pwskip(p);
	pwent->pw_uid = (uid_t)strtoul(p, NULL, 10);
	p = pwskip(p);
	pwent->pw_gid = (gid_t)strtoul(p, NULL, 10);
	p = pwskip(p);
	pwent->pw_gecos = p;
	p = pwskip(p);
	pwent->pw_dir = p;
	p = pwskip(p);
	pwent->pw_shell = p;
	while (*p && *p != '\n')
		p++;
	*p = '\0';
	return (pwent);
}

struct passwd *
ypgetpwnam(nam)
	char *nam;
{
	static struct passwd pwent;
	static char line[1024];
	char *val;
	int reason, vallen;
	
	/*
	 * Get local domain
	 */
	if (!domain && (reason = yp_get_default_domain(&domain)))
		errx(1, "can't get local YP domain. Reason: %s",
		    yperr_string(reason));

	val = NULL;
	reason = yp_match(domain, "passwd.byname", nam, strlen(nam),
	    &val, &vallen);
	if (reason != 0) {
		if (val)
			free (val);
		return (NULL);
	}
	val[vallen] = '\0';
	(void)strncpy(line, val, sizeof(line) - 1);
	free(val);

	return(interpret(&pwent, line));
}

struct passwd *
ypgetpwuid(uid)
	uid_t uid;
{
	static struct passwd pwent;
	static char line[1024];
	char *val;
	int reason, vallen;
	char namebuf[16];
	
	if (!domain && (reason = yp_get_default_domain(&domain)))
		errx(1, "can't get local YP domain. Reason: %s\n",
		    yperr_string(reason));

	(void)snprintf(namebuf, sizeof namebuf, "%d", uid);
	val = NULL;
	reason = yp_match(domain, "passwd.byuid", namebuf, strlen(namebuf),
	    &val, &vallen);
	if (reason != 0) {
		if (val)
			free (val);
		return (NULL);
	}
	val[vallen] = '\0';
	(void)strncpy(line, val, sizeof(line) - 1);
	free(val);

	return(interpret(&pwent, line));
}

void
yppw_error(name, err, eval)
	const char *name;
	int err, eval;
{

	if (err)
		warn(name);

	errx(eval, "YP passwd information unchanged");
}

void
yppw_prompt()
{
	int c;

	(void)printf("re-edit the password file? [y]: ");
	(void)fflush(stdout);
	c = getchar();
	if (c != EOF && c != '\n')
		while (getchar() != '\n');
	if (c == 'n')
		yppw_error(NULL, 0, 0);
}
#endif	/* YP */
