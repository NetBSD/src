/*	$NetBSD: cd.c,v 1.53 2022/01/31 16:54:28 kre Exp $	*/

/*-
 * Copyright (c) 1991, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Kenneth Almquist.
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
static char sccsid[] = "@(#)cd.c	8.2 (Berkeley) 5/4/95";
#else
__RCSID("$NetBSD: cd.c,v 1.53 2022/01/31 16:54:28 kre Exp $");
#endif
#endif /* not lint */

#include <sys/types.h>
#include <sys/stat.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

/*
 * The cd and pwd commands.
 */

#include "shell.h"
#include "var.h"
#include "nodes.h"	/* for jobs.h */
#include "jobs.h"
#include "options.h"
#include "builtins.h"
#include "output.h"
#include "memalloc.h"
#include "error.h"
#include "exec.h"
#include "redir.h"
#include "mystring.h"
#include "show.h"
#include "cd.h"

STATIC int docd(const char *, bool, bool);
STATIC char *getcomponent(void);
STATIC bool updatepwd(const char *);
STATIC void find_curdir(int noerror);
STATIC bool is_curdir(const char *);

char *curdir = NULL;		/* current working directory */
char *prevdir;			/* previous working directory */
STATIC char *cdcomppath;

int
cdcmd(int argc, char **argv)
{
	const char *dest;
	const char *path, *cp;
	char *p;
	char *d;
	struct stat statb;
	char opt;
	bool eopt = false;
	int print = cdprint;	/* set -o cdprint to enable */

	while ((opt = nextopt("Pe")) != '\0')
		if (opt == 'e')
			eopt = true;

	/*
	 * Try (quite hard) to have 'curdir' defined, nothing has set
	 * it on entry to the shell, but we want 'cd fred; cd -' to work.
	 */
	getpwd(1);
	dest = *argptr;
	if (dest == NULL) {
		dest = bltinlookup("HOME", 1);
		if (dest == NULL)
			error("HOME not set");
	} else if (argptr[1]) {
		/* Do 'ksh' style substitution */
		if (!curdir)
			error("PWD not set");
		p = strstr(curdir, dest);
		if (!p)
			error("bad substitution");
		d = stalloc(strlen(curdir) + strlen(argptr[1]) + 1);
		memcpy(d, curdir, p - curdir);
		strcpy(d + (p - curdir), argptr[1]);
		strcat(d, p + strlen(dest));
		dest = d;
		print = 1;
	} else if (dest[0] == '-' && dest[1] == '\0') {
		dest = prevdir ? prevdir : curdir;
		print = 1;
	}
	if (*dest == '\0')
	        dest = ".";

	cp = dest;
	if (*cp == '.' && *++cp == '.')
	    cp++;
	if (*cp == 0 || *cp == '/' || (path = bltinlookup("CDPATH", 1)) == NULL)
		path = nullstr;
	while ((p = padvance(&path, dest, 0)) != NULL) {
		stunalloc(p);
		if (stat(p, &statb) >= 0 && S_ISDIR(statb.st_mode)) {
			int dopr = print;
			int x;

			if (!print)
				dopr = strcmp(p, dest);

			if ((x = docd(p, dopr != 0, eopt)) >= 0)
				return x;
		}
	}
	error("can't cd to %s", dest);
	/* NOTREACHED */
}


/*
 * Actually do the chdir.  In an interactive shell, print the
 * directory name if "print" is nonzero.
 */

STATIC int
docd(const char *dest, bool print, bool eopt)
{
	bool gotpwd;

#if 0		/* no "cd -L" (ever) so all this is just a waste of time ... */
	char *p;
	char *q;
	char *component;
	struct stat statb;
	int first;
	int badstat;

	/*
	 *  Check each component of the path. If we find a symlink or
	 *  something we can't stat, clear curdir to force a getcwd()
	 *  next time we get the value of the current directory.
	 */
	badstat = 0;
	cdcomppath = stalloc(strlen(dest) + 1);
	scopy(dest, cdcomppath);
	STARTSTACKSTR(p);
	if (*dest == '/') {
		STPUTC('/', p);
		cdcomppath++;
	}
	first = 1;
	while ((q = getcomponent()) != NULL) {
		if (q[0] == '\0' || (q[0] == '.' && q[1] == '\0'))
			continue;
		if (! first)
			STPUTC('/', p);
		first = 0;
		component = q;
		while (*q)
			STPUTC(*q++, p);
		if (equal(component, ".."))
			continue;
		STACKSTRNUL(p);
		if (lstat(stackblock(), &statb) < 0) {
			badstat = 1;
			break;
		}
	}
#endif

	CTRACE(DBG_CMDS, ("docd(\"%s\", %s, %s) called\n", dest,
	    print ? "true" : "false", eopt ? "true" : "false"));

	INTOFF;
	if (chdir(dest) < 0) {
		INTON;
		return -1;
	}
	gotpwd = updatepwd(NULL);   /* only do cd -P, no "pretend" -L mode */
	INTON;
	if (print && (iflag || posix))
		out1fmt("%s\n", gotpwd ? curdir : dest);
	return gotpwd || !eopt ? 0 : 1;
}


/*
 * Get the next component of the path name pointed to by cdcomppath.
 * This routine overwrites the string pointed to by cdcomppath.
 */

STATIC char *
getcomponent(void)
{
	char *p;
	char *start;

	if ((p = cdcomppath) == NULL)
		return NULL;
	start = cdcomppath;
	while (*p != '/' && *p != '\0')
		p++;
	if (*p == '\0') {
		cdcomppath = NULL;
	} else {
		*p++ = '\0';
		cdcomppath = p;
	}
	return start;
}



/*
 * Update curdir (the name of the current directory) in response to a
 * cd command.  We also call hashcd to let the routines in exec.c know
 * that the current directory has changed.
 */

STATIC bool
updatepwd(const char *dir)
{
	char *new;
	char *p;

	hashcd();				/* update command hash table */

	/*
	 * If our argument is NULL, we don't know the current directory
	 * any more because we traversed a symbolic link or something
	 * we couldn't stat().   Or we simply don't trust what we had.
	 */
	if (dir == NULL || curdir == NULL)  {
		if (prevdir)
			ckfree(prevdir);
		INTOFF;
		prevdir = curdir;
		curdir = NULL;
		getpwd(1);
		INTON;
		if (curdir) {
			setvar("OLDPWD", prevdir, VEXPORT);
			setvar("PWD", curdir, VEXPORT);
			return true;
		} else
			unsetvar("PWD", 0);
		return false;
	}

	/* XXX none of the following code is ever executed any more */

	cdcomppath = stalloc(strlen(dir) + 1);
	scopy(dir, cdcomppath);
	STARTSTACKSTR(new);
	if (*dir != '/') {
		p = curdir;
		while (*p)
			STPUTC(*p++, new);
		if (p[-1] == '/')
			STUNPUTC(new);
	}
	while ((p = getcomponent()) != NULL) {
		if (equal(p, "..")) {
			while (new > stackblock() && (STUNPUTC(new), *new) != '/');
		} else if (*p != '\0' && ! equal(p, ".")) {
			STPUTC('/', new);
			while (*p)
				STPUTC(*p++, new);
		}
	}
	if (new == stackblock())
		STPUTC('/', new);
	STACKSTRNUL(new);
	INTOFF;
	if (prevdir)
		ckfree(prevdir);
	prevdir = curdir;
	curdir = savestr(stackblock());
	setvar("OLDPWD", prevdir, VEXPORT);
	setvar("PWD", curdir, VEXPORT);
	INTON;
	return true;
}

/*
 * Test whether we are currently in the direcory given
 * (provided it is given, and is absolute)
 * ie: determine if path is fully qualified pathname of "."
 */
STATIC bool
is_curdir(const char *path)
{
	struct stat stdot, stpath;

	return	path != NULL &&
		*path == '/' &&
		stat(".", &stdot) != -1 &&
		stat(path, &stpath) != -1 &&
		stdot.st_dev == stpath.st_dev &&
		stdot.st_ino == stpath.st_ino;
}

/*
 * Posix says the default should be 'pwd -L' (as below), however
 * the 'cd' command (above) does something much nearer to the
 * posix 'cd -P' (not the posix default of 'cd -L').
 * If 'cd' is changed to support -P/L then the default here
 * needs to be revisited if the historic behaviour is to be kept.
 */

int
pwdcmd(int argc, char **argv)
{
	int i;
	char opt = 'L';

	while ((i = nextopt("LP")) != '\0')
		opt = i;
	if (*argptr)
		error("unexpected argument");

	if (opt == 'L')
		getpwd(0);
	else
		find_curdir(0);

#if 0	/* posix has been changed to forbid this */
	setvar("OLDPWD", prevdir, VEXPORT);
	setvar("PWD", curdir, VEXPORT);
#endif

	if (!is_curdir(curdir)) {
		find_curdir(1);
		if (curdir == NULL)
			error("Unable to find current directory");
	}

	flushout(out1);		/* make sure buffer is empty */
	clr_err(out1);		/* and forget any earlier errors */
	out1str(curdir);
	out1c('\n');
	flushout(out1);
	if (io_err(out1))
		error("stdout: %s", strerror(errno));

	return 0;
}



void
initpwd(void)
{
	getpwd(1);
	if (curdir)
		setvar("PWD", curdir, VEXPORT);
	else
		sh_warnx("Cannot determine current working directory");
}

#define MAXPWD 256

/*
 * Find out what the current directory is. If we already know the current
 * directory, this routine returns immediately.
 */
void
getpwd(int noerror)
{
	char *pwd;
	static int first = 1;

	if (curdir)
		return;

	if (first) {
		/*
		 * Note that this happens via the call from initpwd()
		 * just above, which is called early from main() during
		 * sh startup, so fetching PWD from the entry environment
		 * (which is what getenv() does) is acceptable.   Here we
		 * could use normal sh var lookup functions instead, as
		 * the arriving environment has already been imported before
		 * we get here, but it makes little difference.
		 *
		 * XXX What would be better perhaps would be to move all of
		 * this into initpwd() instead of here, so we could get rid of
		 * this "first" static - that function is only ever called once.
		 * XXX Some other day.
		 */
		first = 0;
		pwd = getenv("PWD");
		if (is_curdir(pwd)) {
			curdir = savestr(pwd);
			return;
		}
	}

	find_curdir(noerror);

	return;
}

STATIC void
find_curdir(int noerror)
{
	int i;
	char *pwd;

	/*
	 * Things are a bit complicated here; we could have just used
	 * getcwd, but traditionally getcwd is implemented using popen
	 * to /bin/pwd. This creates a problem for us, since we cannot
	 * keep track of the job if it is being ran behind our backs.
	 * XXX That's not actually the problem, a process created and
	 * XXX destroyed that we know nothing about is harmless.  The
	 * XXX problem is that old popen() implementations would use
	 * XXX wait(2) to await completion of the command, and that might
	 * XXX collect (and ignore) our children.   As long as we are
	 * XXX confident that popen() uses waitpid() (or the equv) there
	 * XXX would not be a problem.   But how do we know that?
	 * So we re-implement getcwd(), and we suppress interrupts
	 * throughout the process. This is not completely safe, since
	 * the user can still break out of it by killing the pwd program.
	 * We still try to use getcwd for systems that we know have a
	 * c implementation of getcwd, that does not open a pipe to
	 * /bin/pwd.
	 */
#if defined(__NetBSD__) || defined(__SVR4)

	for (i = MAXPWD;; i *= 2) {
		pwd = stalloc(i);
		if (getcwd(pwd, i) != NULL) {
			curdir = savestr(pwd);
			stunalloc(pwd);
			return;
		}
		stunalloc(pwd);
		if (errno == ERANGE)
			continue;
		if (!noerror)
			error("getcwd() failed: %s", strerror(errno));
		return;
	}
#else
	{
		char *p;
		int status;
		struct job *jp;
		int pip[2];

		pwd = stalloc(MAXPWD);
		INTOFF;
		if (pipe(pip) < 0)
			error("Pipe call failed");
		jp = makejob(NULL, 1);
		if (forkshell(jp, NULL, FORK_NOJOB) == 0) {
			(void) close(pip[0]);
			movefd(pip[1], 1);
			(void) execl("/bin/pwd", "pwd", (char *)0);
			error("Cannot exec /bin/pwd");
		}
		(void) close(pip[1]);
		pip[1] = -1;
		p = pwd;
		while ((i = read(pip[0], p, pwd + MAXPWD - p)) > 0
		     || (i == -1 && errno == EINTR)) {
			if (i > 0)
				p += i;
		}
		(void) close(pip[0]);
		pip[0] = -1;
		status = waitforjob(jp);
		if (status != 0)
			error((char *)0);
		if (i < 0 || p == pwd || p[-1] != '\n') {
			if (noerror) {
				INTON;
				return;
			}
			error("pwd command failed");
		}
		p[-1] = '\0';
		INTON;
		curdir = savestr(pwd);
		stunalloc(pwd);
		return;
	}
#endif
}
