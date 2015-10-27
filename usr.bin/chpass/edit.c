/*	$NetBSD: edit.c,v 1.22 2015/10/27 14:47:45 shm Exp $	*/

/*-
 * Copyright (c) 1990, 1993, 1994
 *	The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
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

#include <sys/cdefs.h>
#ifndef lint
#if 0
static char sccsid[] = "@(#)edit.c	8.3 (Berkeley) 4/2/94";
#else
__RCSID("$NetBSD: edit.c,v 1.22 2015/10/27 14:47:45 shm Exp $");
#endif
#endif /* not lint */

#include <sys/param.h>
#include <sys/stat.h>

#include <ctype.h>
#include <err.h>
#include <errno.h>
#include <paths.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <util.h>

#include "chpass.h"

void
edit(char *tempname, struct passwd *pw)
{
	struct stat begin, end;

	for (;;) {
		if (stat(tempname, &begin))
			(*Pw_error)(tempname, 1, 1);
		pw_edit(1, tempname);
		if (stat(tempname, &end))
			(*Pw_error)(tempname, 1, 1);
		if (begin.st_mtime == end.st_mtime) {
			warnx("no changes made");
			(*Pw_error)(NULL, 0, 0);
		}
		if (verify(tempname, pw))
			break;
#ifdef YP
		if (use_yp)
			yppw_prompt();
		else
#endif
			pw_prompt();
	}
}

/*
 * display --
 *	print out the file for the user to edit; strange side-effect:
 *	set conditional flag if the user gets to edit the shell.
 */
void
display(char *tempname, int fd, struct passwd *pw)
{
	FILE *fp;
	char *bp, *p;
	char chgstr[256], expstr[256];

	if (!(fp = fdopen(fd, "w")))
		(*Pw_error)(tempname, 1, 1);

	(void)fprintf(fp,
	    "#Changing user %sdatabase information for %s.\n",
	    use_yp ? "YP " : "", pw->pw_name);
	if (!uid) {
		(void)fprintf(fp, "Login: %s\n", pw->pw_name);
		(void)fprintf(fp, "Password: %s\n", pw->pw_passwd);
		(void)fprintf(fp, "Uid [#]: %d\n", pw->pw_uid);
		(void)fprintf(fp, "Gid [# or name]: %d\n", pw->pw_gid);
		(void)fprintf(fp, "Change [month day year]: %s\n",
		    ttoa(chgstr, sizeof chgstr, pw->pw_change));
		(void)fprintf(fp, "Expire [month day year]: %s\n",
		    ttoa(expstr, sizeof expstr, pw->pw_expire));
		(void)fprintf(fp, "Class: %s\n", pw->pw_class);
		(void)fprintf(fp, "Home directory: %s\n", pw->pw_dir);
		(void)fprintf(fp, "Shell: %s\n",
		    *pw->pw_shell ? pw->pw_shell : _PATH_BSHELL);
	}
	/* Only admin can change "restricted" shells. */
	else if (ok_shell(pw->pw_shell))
		/*
		 * Make shell a restricted field.  Ugly with a
		 * necklace, but there's not much else to do.
		 */
		(void)fprintf(fp, "Shell: %s\n",
		    *pw->pw_shell ? pw->pw_shell : _PATH_BSHELL);
	else
		list[E_SHELL].restricted = 1;
	bp = strdup(pw->pw_gecos);
	if (!bp) {
		err(1, "strdup");
		/*NOTREACHED*/
	}
	p = strsep(&bp, ",");
	(void)fprintf(fp, "Full Name: %s\n", p ? p : "");
	p = strsep(&bp, ",");
	(void)fprintf(fp, "Location: %s\n", p ? p : "");
	p = strsep(&bp, ",");
	(void)fprintf(fp, "Office Phone: %s\n", p ? p : "");
	p = strsep(&bp, ",");
	(void)fprintf(fp, "Home Phone: %s\n", p ? p : "");

	(void)fchown(fd, getuid(), getgid());
	(void)fclose(fp);
	free(bp);
}

int
verify(char *tempname, struct passwd *pw)
{
	ENTRY *ep;
	char *p;
	struct stat sb;
	FILE *fp = NULL;
	int len, fd;
	static char buf[LINE_MAX];

	if ((fd = open(tempname, O_RDONLY|O_NOFOLLOW)) == -1 ||
	    (fp = fdopen(fd, "r")) == NULL)
		(*Pw_error)(tempname, 1, 1);
	if (fstat(fd, &sb))
		(*Pw_error)(tempname, 1, 1);
	if (sb.st_size == 0 || sb.st_nlink != 1) {
		warnx("corrupted temporary file");
		goto bad;
	}
	while (fgets(buf, sizeof(buf), fp)) {
		if (!buf[0] || buf[0] == '#')
			continue;
		if (!(p = strchr(buf, '\n'))) {
			warnx("line too long");
			goto bad;
		}
		*p = '\0';
		for (ep = list;; ++ep) {
			if (!ep->prompt) {
				warnx("unrecognized field");
				goto bad;
			}
			if (!strncasecmp(buf, ep->prompt, ep->len)) {
				if (ep->restricted && uid) {
					warnx(
					    "you may not change the %s field",
						ep->prompt);
					goto bad;
				}
				if (!(p = strchr(buf, ':'))) {
					warnx("line corrupted");
					goto bad;
				}
				while (isspace((unsigned char)*++p));
				if (ep->except && strpbrk(p, ep->except)) {
					warnx(
				   "illegal character in the \"%s\" field",
					    ep->prompt);
					goto bad;
				}
				if ((ep->func)(p, pw, ep)) {
bad:					(void)fclose(fp);
					return (0);
				}
				break;
			}
		}
	}
	(void)fclose(fp);

	/* Build the gecos field. */
	len = strlen(list[E_NAME].save) + strlen(list[E_BPHONE].save) +
	    strlen(list[E_HPHONE].save) + strlen(list[E_LOCATE].save) + 4;
	if (!(p = malloc(len)))
		err(1, "malloc");
	(void)snprintf(p, len, "%s,%s,%s,%s", list[E_NAME].save,
	    list[E_LOCATE].save, list[E_BPHONE].save, list[E_HPHONE].save);
	pw->pw_gecos = p;

	if (snprintf(buf, sizeof(buf),
	    "%s:%s:%d:%d:%s:%lu:%lu:%s:%s:%s",
	    pw->pw_name, pw->pw_passwd, pw->pw_uid, pw->pw_gid, pw->pw_class,
	    (u_long)pw->pw_change, (u_long)pw->pw_expire, pw->pw_gecos,
	    pw->pw_dir, pw->pw_shell) >= (int)sizeof(buf)) {
		warnx("entries too long");
		return (0);
	}
	return (pw_scan(buf, pw, NULL));
}
