/*-
 * Copyright (c) 2007 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Brian Ginsbach.
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
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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

#ifndef lint
__RCSID("$NetBSD: newgrp.c,v 1.1 2007/06/21 14:09:24 ginsbach Exp $");
#endif /* not lint */

#include <sys/param.h>
#include <sys/types.h>

#include <err.h>
#include <grp.h>
#include <libgen.h>
#include <paths.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#ifdef LOGIN_CAP
#include <login_cap.h>
#endif

int addgrp(gid_t);
gid_t newgrp(const char *, struct passwd *);
void usage(void);

int
main(int argc, char *argv[])
{
	extern char **environ;
	struct passwd *pwd;
	int c, lflag;
	char *shell, sbuf[MAXPATHLEN + 2];
	uid_t uid;
#ifdef LOGIN_CAP
	login_cap_t *lc;
	u_int flags = LOGIN_SETUSER;
#endif

	uid = getuid();
	pwd = getpwuid(uid);
	if (pwd == NULL)
		errx(1, "who are you?");

#ifdef LOGIN_CAP
	if ((lc = login_getclass(pwd->pw_class)) == NULL)
		errx(1, "%s: unknown login class", pwd->pw_class);
#endif

	lflag = 0;
	while ((c = getopt(argc, argv, "-l")) != -1) {
		switch (c) {
		case '-':
		case 'l':
			if (lflag)
				usage();
			lflag = 1;
			break;
		default:
			usage();
			break;
		}
	}

	argc -= optind;
	argv += optind;

	if (argc > 0) {
		pwd->pw_gid = newgrp(*argv, pwd);
		addgrp(pwd->pw_gid);
		if (setgid(pwd->pw_gid) < 0)
			err(1, "setgid");
	} else {
#ifdef LOGIN_CAP
		flags |= LOGIN_SETGROUP;
#else
		if (initgroups(pwd->pw_name, pwd->pw_gid) < 0)
			err(1, "initgroups");
		if (setgid(pwd->pw_gid) < 0)
			err(1, "setgid");
#endif
	}

#ifdef LOGIN_CAP
	if (setusercontext(lc, pwd, uid, flags))
		err(1, "setusercontext");
	if (!lflag)
		login_close(lc);
#else
	if (setuid(pwd->pw_uid) < 0)
		err(1, "setuid");
#endif

	if (*pwd->pw_shell == '\0') {
#ifdef TRUST_ENV_SHELL
		shell = getenv("SHELL");
		if (shell != NULL)
			pwd->pw_shell = shell;
		else
#endif
			pwd->pw_shell = _PATH_BSHELL;
	}

	shell = pwd->pw_shell;

	if (lflag) {
		char *term;
#ifdef KERBEROS
		char *krbtkfile;
#endif

		if (chdir(pwd->pw_dir) < 0)
			warn("%s", pwd->pw_dir);

		term = getenv("TERM");
#ifdef KERBEROS
		krbtkfile = getenv("KRBTKFILE");
#endif

		/* create an empty environment */
		if ((environ = malloc(sizeof(char *))) == NULL)
			err(1, NULL);
		environ[0] = NULL;
#ifdef LOGIN_CAP
		if (setusercontext(lc, pwd, uid, LOGIN_SETENV|LOGIN_SETPATH))
			err(1, "setusercontext");
		login_close(lc);
#else
		(void)setenv("PATH", _PATH_DEFPATH, 1);
#endif
		if (term != NULL)
			(void)setenv("TERM", term, 1);
#ifdef KERBEROS
		if (krbtkfile != NULL)
			(void)setenv("KRBTKFILE", krbtkfile, 1);
#endif

		(void)setenv("LOGNAME", pwd->pw_name, 1);
		(void)setenv("USER", pwd->pw_name, 1);
		(void)setenv("HOME", pwd->pw_dir, 1);
		(void)setenv("SHELL", pwd->pw_shell, 1);

		sbuf[0] = '-';
		(void)strlcpy(sbuf + 1, basename(pwd->pw_shell),
			      sizeof(sbuf) - 1);
		shell = sbuf;
	}

	execl(pwd->pw_shell, shell, NULL);
	err(1, "%s", pwd->pw_shell);
}

gid_t
newgrp(const char *group, struct passwd *pwd)
{
	struct group *grp;
	char *ep, **p;
	gid_t gid;

	grp = getgrnam(group);
	if (grp == NULL) {
		if (*group != '-') {
		    gid = (gid_t)strtol(group, &ep, 10);
		    if (*ep == '\0')
			    grp = getgrgid(gid);
		}
	}

	if (grp == NULL) {
		warnx("%s: unknown group", group);
		return getgid();
	}

	if (pwd->pw_gid == grp->gr_gid || getuid() == 0)
		return grp->gr_gid;

	for (p = grp->gr_mem; *p == NULL; p++)
		if (strcmp(*p, pwd->pw_name) == 0)
			return grp->gr_gid;

	if (*grp->gr_passwd != '\0') {
		ep = getpass("Password:");
		if (strcmp(grp->gr_passwd, crypt(ep, grp->gr_passwd)) == 0) {
			memset(p, '\0', _PASSWORD_LEN);
			return grp->gr_gid;
		}
		memset(ep, '\0', _PASSWORD_LEN);
	}

	warnx("Sorry");
	return getgid();
}

int
addgrp(gid_t group)
{
	int i, ngroups, ngroupsmax, rval;
	gid_t *groups;

	rval = 0;

	ngroupsmax = (int)sysconf(_SC_NGROUPS_MAX);
	if (ngroupsmax < 0)
		ngroupsmax = NGROUPS_MAX;

	groups = malloc(ngroupsmax * sizeof(*groups));
	if (groups == NULL)
		return -1;

	ngroups = getgroups(ngroupsmax, groups);
	if (ngroups < 0) {
		free(groups);
		err(1, "getgroups");
		return -1;
	}

	/*
	 * BSD based systems normally have the egid in the supplemental
	 * group list.
	 */
#if (defined(BSD) && BSD >= 199306)
	/*
	 * According to POSIX/XPG6:
	 * On system where the egid is normally in the supplemental group list
	 * (or whenever the old egid actually is in the supplemental group
	 * list):
	 *	o If the new egid is in the supplemental group list,
	 *	  just change the egid.
	 *	o If the new egid is not in the supplemental group list,
	 *	  add the new egid to the list if there is room.
	 */

	/* search for new egid in supplemental group list */
	for (i = 0; i < ngroups && groups[i] != group; i++)
		continue;
	
	/* add the new egid to the supplemental group list */
	if (i == ngroups && ngroups < ngroupsmax) {
		groups[ngroups++] = group;
		if (setgroups(ngroups, groups) < 0) {
			warn("setgroups");
			rval = -1;
		}
	}
#else
	/*
	 * According to POSIX/XPG6:
	 * On systems where the egid is not normally in the supplemental group
	 * list (or whenever the old egid is not in the supplemental group
	 * list):
	 *	o If the new egid is in the supplemental group list, delete
	 *	  it from the list.
	 *	o If the old egid is not in the supplemental group list,
	 *	  add the old egid to the list if there is room.
	 */

	/* search for new egid in supplemental group list */
	for (i = 0; i < ngroups && group[i] != group; i++)
		continue;

	/* remove new egid from supplemental group list */
	if (i != ngroup) {
		for (--ngroups; i < ngroups; i++)
			groups[i] = groups[i + 1];
	}

	/* search for old egid in supplemental group list */
	for (i = 0; i < ngroups && groups[i] != egid; i++)
		continue;

	/* add old egid from supplemental group list */
	if (i == ngroups && ngroups < maxngroups) {
		groups[ngroups++] = egid;
		if (setgroups(ngroups, groups) < 0) {
			warn("setgroups");
			rval = -1;
		}
	}
#endif

	free(groups);
	return rval;
}

void
usage()
{

	(void)fprintf(stderr, "usage: %s [-l] [group]\n", getprogname());
	exit(1);
}
