/*      $NetBSD: catman.c,v 1.34.6.1 2014/08/20 00:05:07 tls Exp $       */

/*
 * Copyright (c) 1998 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * Author: Baldassare Dante Profeta <dante@mclink.it>
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

#include <sys/types.h>
#include <sys/queue.h>
#include <sys/cdefs.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/utsname.h>
#include <ctype.h>
#include <dirent.h>
#include <err.h>
#include <errno.h>
#include <fnmatch.h>
#include <limits.h>
#include <libgen.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <glob.h>

#include "manconf.h"
#include "pathnames.h"

int f_nowhatis = 0;
int f_noaction = 0;
int f_noformat = 0;
int f_ignerr = 0;
int f_noprint = 0;
int dowhatis = 0;

TAG *defp;	/* pointer to _default list */

static void	setdefentries(char *, char *, const char *);
static void	uniquepath(void);
static void	catman(void);
static void	scanmandir(const char *, const char *);
static int	splitentry(char *, char *, size_t, char *, size_t);
static void	setcatsuffix(char *, const char *, const char *);
static void	makecat(const char *, const char *, const char *, const char *);
static void	makewhatis(void);
static void	dosystem(const char *);
__dead static void	usage(void);


int
main(int argc, char * const *argv)
{
	char *m_path = NULL;
	char *m_add = NULL;
	int c;

	while ((c = getopt(argc, argv, "km:M:npsw")) != -1) {
		switch (c) {
		case 'k':
			f_ignerr = 1;
			break;
		case 'n':
			f_nowhatis = 1;
			break;
		case 'p':
			f_noaction = 1;
			break;
		case 's':
			f_noprint = 1;
			break;
		case 'w':
			f_noformat = 1;
			break;
		case 'm':
			m_add = optarg;
			break;
		case 'M':
			m_path = optarg;
			break;
		default:
			usage();
		}
	}

	argc -= optind;
	argv += optind;

	if (f_noprint && f_noaction)
		f_noprint = 0;

	if (argc > 1)
		usage();

	config(_PATH_MANCONF);
	setdefentries(m_path, m_add, (argc == 0) ? NULL : argv[argc-1]);
	uniquepath();

	if (f_noformat == 0 || f_nowhatis == 0)
		catman();
	if (f_nowhatis == 0 && dowhatis)
		makewhatis();

	return(0);
}

static void
setdefentries(char *m_path, char *m_add, const char *sections)
{
	TAG *defnewp, *sectnewp, *subp;
	ENTRY *e_defp, *e_subp;
	const char *p, *slashp;
	char *machine;
	char buf[MAXPATHLEN * 2];
	int i;

	/* Get the machine type. */
	if ((machine = getenv("MACHINE")) == NULL) {
		struct utsname utsname;

		if (uname(&utsname) == -1) {
			perror("uname");
			exit(1);
		}
		machine = utsname.machine;
	}

	/* If there's no _default list, create an empty one. */
	defp = gettag("_default", 1);
	subp = gettag("_subdir", 1);
	if (defp == NULL || subp == NULL)
		err(1, "malloc");

	/*
	 * 0: If one or more sections was specified, rewrite _subdir list.
	 */
	if (sections != NULL) {
		if ((sectnewp = gettag("_section_new", 1)) == NULL)
			err(1, "malloc");
		for (p = sections; *p;) {
			i = snprintf(buf, sizeof(buf), "man%c", *p++);
			for (; *p && !isdigit((unsigned char)*p) && i < (int)sizeof(buf) - 1; i++)
				buf[i] = *p++;
			buf[i] = '\0';
			if (addentry(sectnewp, buf, 0) < 0)
				err(1, "malloc");
		}
		subp = sectnewp;
	}

	/*
	 * 1: If the user specified a MANPATH variable, or set the -M
	 *    option, we replace the _default list with the user's list,
	 *    appending the entries in the _subdir list and the machine.
	 */
	if (m_path == NULL)
		m_path = getenv("MANPATH");
	if (m_path != NULL) {
		while ((e_defp = TAILQ_FIRST(&defp->entrylist)) != NULL) {
			free(e_defp->s);
			TAILQ_REMOVE(&defp->entrylist, e_defp, q);
		}
		for (p = strtok(m_path, ":");
		    p != NULL; p = strtok(NULL, ":")) {
			slashp = p[strlen(p) - 1] == '/' ? "" : "/";
			TAILQ_FOREACH(e_subp, &subp->entrylist, q) {
				if (!strncmp(e_subp->s, "cat", 3))
					continue;
				(void)snprintf(buf, sizeof(buf), "%s%s%s{/%s,}",
				    p, slashp, e_subp->s, machine);
				if (addentry(defp, buf, 0) < 0)
					err(1, "malloc");
			}
		}
	}

	/*
	 * 2: If the user did not specify MANPATH, -M or a section, rewrite
	 *    the _default list to include the _subdir list and the machine.
	 */
	if (m_path == NULL) {
		defp = gettag("_default", 1);
		defnewp = gettag("_default_new1", 1);
		if (defp == NULL || defnewp == NULL)
			err(1, "malloc");

		TAILQ_FOREACH(e_defp, &defp->entrylist, q) {
			slashp =
			    e_defp->s[strlen(e_defp->s) - 1] == '/' ? "" : "/";
			TAILQ_FOREACH(e_subp, &subp->entrylist, q) {
				if (!strncmp(e_subp->s, "cat", 3))
					continue;
				(void)snprintf(buf, sizeof(buf), "%s%s%s{/%s,}",
					e_defp->s, slashp, e_subp->s, machine);
				if (addentry(defnewp, buf, 0) < 0)
					err(1, "malloc");
			}
		}
		defp = defnewp;
	}

	/*
	 * 3: If the user set the -m option, insert the user's list before
	 *    whatever list we have, again appending the _subdir list and
	 *    the machine.
	 */
	if (m_add != NULL)
		for (p = strtok(m_add, ":"); p != NULL; p = strtok(NULL, ":")) {
			slashp = p[strlen(p) - 1] == '/' ? "" : "/";
			TAILQ_FOREACH(e_subp, &subp->entrylist, q) {
				if (!strncmp(e_subp->s, "cat", 3))
					continue;
				(void)snprintf(buf, sizeof(buf), "%s%s%s{/%s,}",
				    p, slashp, e_subp->s, machine);
				if (addentry(defp, buf, 1) < 0)
					err(1, "malloc");
			}
		}
}

/*
 * Remove entries (directory) which are symbolic links to other entries.
 * Some examples are showed below:
 * 1) if /usr/X11 -> /usr/X11R6 then remove all /usr/X11/man entries.
 * 2) if /usr/local/man -> /usr/share/man then remove all /usr/local/man
 *    entries
 */
static void
uniquepath(void)
{
	TAG *defnewp;
	ENTRY *e_defp;
	glob_t manpaths;
	struct stat st1;
	struct stat st2;
	struct stat st3;
	int len, lnk, gflags;
	size_t i, j;
	char path[PATH_MAX], *p;

	gflags = 0;
	TAILQ_FOREACH(e_defp, &defp->entrylist, q) {
		glob(e_defp->s, GLOB_BRACE | GLOB_NOSORT | gflags, NULL,
		    &manpaths);
		gflags = GLOB_APPEND;
	}

	if ((defnewp = gettag("_default_new2", 1)) == NULL)
		err(1, "malloc");

	for (i = 0; i < manpaths.gl_pathc; i++) {
		lnk = 0;
		lstat(manpaths.gl_pathv[i], &st1);
		for (j = 0; j < manpaths.gl_pathc; j++) {
			if (i != j) {
				lstat(manpaths.gl_pathv[j], &st2);
				if (st1.st_ino == st2.st_ino) {
					strlcpy(path, manpaths.gl_pathv[i],
					    sizeof(path));
					for (p = path; *(p+1) != '\0';) {
						p = dirname(p);
						lstat(p, &st3);
						if (S_ISLNK(st3.st_mode)) {
							lnk = 1;
							break;
						}
					}
				} else {
					len = readlink(manpaths.gl_pathv[i],
					    path, sizeof(path) - 1);
					if (len == -1)
						continue;
					path[len] = '\0';
					if (!strcmp(path, manpaths.gl_pathv[j]))
						lnk = 1;
				}
				if (lnk)
					break;
			}
		}

		if (!lnk) {
			if (addentry(defnewp, manpaths.gl_pathv[i], 0) < 0)
				err(1, "malloc");
		}
	}

	globfree(&manpaths);

	defp = defnewp;
}

static void
catman(void)
{
	ENTRY *e_path;
	const char *mandir;
	char catdir[PATH_MAX], *cp;

	TAILQ_FOREACH(e_path, &defp->entrylist, q) {
		mandir = e_path->s;
		strlcpy(catdir, mandir, sizeof(catdir));
		if (!(cp = strstr(catdir, "man/man")))
			continue;
		cp += 4; *cp++ = 'c'; *cp++ = 'a'; *cp = 't';
		scanmandir(catdir, mandir);
	}
}

static void
scanmandir(const char *catdir, const char *mandir)
{
	TAG *buildp, *crunchp;
	ENTRY *e_build, *e_crunch;
	char manpage[PATH_MAX];
	char catpage[PATH_MAX];
	char linkname[PATH_MAX];
	char buffer[PATH_MAX], *bp;
	char tmp[PATH_MAX];
	char buildsuff[256], buildcmd[256];
	char crunchsuff[256], crunchcmd[256];
	char match[256];
	struct stat manstat;
	struct stat catstat;
	struct stat lnkstat;
	struct dirent *dp;
	DIR *dirp;
	int len, error;

	if ((dirp = opendir(mandir)) == 0) {
		warn("can't open %s", mandir);
		return;
	}

	if (stat(catdir, &catstat) < 0) {
		if (errno != ENOENT) {
			warn("can't stat %s", catdir);
			closedir(dirp);
			return;
		}
		if (f_noprint == 0)
			printf("mkdir %s\n", catdir);
		if (f_noaction == 0 && mkdir(catdir, 0755) < 0) {
			warn("can't create %s", catdir);
			closedir(dirp);
			return;
		}
	}

	while ((dp = readdir(dirp)) != NULL) {
		if (strcmp(dp->d_name, ".") == 0 ||
		    strcmp(dp->d_name, "..") == 0)
			continue;

		snprintf(manpage, sizeof(manpage), "%s/%s", mandir, dp->d_name);
		snprintf(catpage, sizeof(catpage), "%s/%s", catdir, dp->d_name);

		e_build = NULL;
		if ((buildp = gettag("_build", 1)) == NULL)
			err(1, "malloc");
		TAILQ_FOREACH(e_build, &buildp->entrylist, q) {
			splitentry(e_build->s, buildsuff, sizeof(buildsuff),
			    buildcmd, sizeof(buildcmd));
			snprintf(match, sizeof(match), "*%s",
						buildsuff);
			if (!fnmatch(match, manpage, 0))
				break;
		}

		if (e_build == NULL)
			continue;

		e_crunch = NULL;
		if ((crunchp = gettag("_crunch", 1)) == NULL)
			err(1, "malloc");
		TAILQ_FOREACH(e_crunch, &crunchp->entrylist, q) {
			splitentry(e_crunch->s, crunchsuff, sizeof(crunchsuff),
			    crunchcmd, sizeof(crunchcmd));
			snprintf(match, sizeof(match), "*%s", crunchsuff);
			if (!fnmatch(match, manpage, 0))
				break;
		}

		if (lstat(manpage, &manstat) <0) {
			warn("can't stat %s", manpage);
			continue;
		} else {
			if (S_ISLNK(manstat.st_mode)) {
				strlcpy(buffer, catpage, sizeof(buffer));
				strlcpy(linkname, basename(buffer),
				    sizeof(linkname));
				len = readlink(manpage, buffer,
				    sizeof(buffer) - 1);
				if (len == -1) {
					warn("can't stat read symbolic link %s",
					    manpage);
					continue;
				}
				buffer[len] = '\0';

				if (strcmp(buffer, basename(buffer)) == 0) {
					bp = basename(buffer);
					strlcpy(tmp, manpage, sizeof(tmp));
					snprintf(manpage, sizeof(manpage),
					    "%s/%s", dirname(tmp), bp);
					strlcpy(tmp, catpage, sizeof(tmp));
					snprintf(catpage, sizeof(catpage),
					    "%s/%s", dirname(tmp), buffer);
				} else {
					*linkname = '\0';
				}

			}
			else
				*linkname = '\0';
		}

		if (!e_crunch) {
			*crunchsuff = *crunchcmd = '\0';
		}
		setcatsuffix(catpage, buildsuff, crunchsuff);
		if (*linkname != '\0')
			setcatsuffix(linkname, buildsuff, crunchsuff);

		if (stat(manpage, &manstat) < 0) {
			warn("can't stat %s", manpage);
			continue;
		}

		if (!S_ISREG(manstat.st_mode)) {
			warnx("not a regular file %s", manpage);
			continue;
		}

		if ((error = stat(catpage, &catstat)) &&
		    errno != ENOENT) {
			warn("can't stat %s", catpage);
			continue;
		}

		if ((error && errno == ENOENT) ||
		    manstat.st_mtime > catstat.st_mtime) {
			if (f_noformat) {
				dowhatis = 1;
			} else {
				/*
				 * reformat out of date manpage
				 */
				makecat(manpage, catpage, buildcmd, crunchcmd);
				dowhatis = 1;
			}
		}

		if (*linkname != '\0') {
			strlcpy(tmp, catpage, sizeof(tmp));
			snprintf(tmp, sizeof(tmp), "%s/%s", dirname(tmp),
					linkname);
			if ((error = lstat(tmp, &lnkstat)) &&
			    errno != ENOENT) {
				warn("can't stat %s", tmp);
				continue;
			}

			if (error && errno == ENOENT)  {
				if (f_noformat) {
					dowhatis = 1;
				} else {
					/*
					 * create symbolic link
					 */
					if (f_noprint == 0)
						printf("ln -s %s %s\n", catpage,
						    linkname);
					if (f_noaction == 0) {
						strlcpy(tmp, catpage,
						    sizeof(tmp));
						if (chdir(dirname(tmp)) == -1) {
							warn("can't chdir");
							continue;
						}

						if (symlink(catpage, linkname)
								== -1) {
							warn("can't create"
								" symbolic"
								" link %s",
								linkname);
							continue;
						}
					}
					dowhatis = 1;
				}
			}
		}
	}
	closedir(dirp);
}

static int
splitentry(char *s, char *first, size_t firstlen, char *second, 
	   size_t secondlen)
{
	char *c;

	for (c = s; *c != '\0' && !isspace((unsigned char)*c); ++c)
		;
	if (*c == '\0')
		return(0);
	if ((size_t)(c - s + 1) > firstlen)
		return(0);
	strncpy(first, s, c-s);
	first[c-s] = '\0';
	for (; *c != '\0' && isspace((unsigned char)*c); ++c)
		;
	if (strlcpy(second, c, secondlen) >= secondlen)
		return(0);
	return(1);
}

static void
setcatsuffix(char *catpage, const char *suffix, const char *crunchsuff)
{
	TAG *tp;
	char *p;

	for (p = catpage + strlen(catpage); p != catpage; p--)
		if (!fnmatch(suffix, p, 0)) {
			if ((tp = gettag("_suffix", 1)) == NULL)
				err(1, "malloc");
			if (! TAILQ_EMPTY(&tp->entrylist)) {
				sprintf(p, "%s%s",
				    TAILQ_FIRST(&tp->entrylist)->s, crunchsuff);
			} else {
				sprintf(p, ".0%s", crunchsuff);
			}
			break;
		}
}

static void
makecat(const char *manpage, const char *catpage, const char *buildcmd, 
    const char *crunchcmd)
{
	char crunchbuf[1024];
	char sysbuf[2048];
	size_t len;

	len = snprintf(sysbuf, sizeof(sysbuf), buildcmd, manpage);
	if (len > sizeof(sysbuf))
		errx(1, "snprintf");

	if (*crunchcmd != '\0') {
		snprintf(crunchbuf, sizeof(crunchbuf), crunchcmd, catpage);
		snprintf(sysbuf + len, sizeof(sysbuf) - len, " | %s", 
		    crunchbuf);
	} else {
		snprintf(sysbuf + len, sizeof(sysbuf) - len, " > %s", catpage);
	}

	if (f_noprint == 0)
		printf("%s\n", sysbuf);
	if (f_noaction == 0)
		dosystem(sysbuf);
}

static void
makewhatis(void)
{
	TAG *whatdbp;
	ENTRY *e_whatdb;
	char sysbuf[1024];

	if ((whatdbp = gettag("_whatdb", 1)) == NULL)
		err(1, "malloc");
	TAILQ_FOREACH(e_whatdb, &whatdbp->entrylist, q) {
		snprintf(sysbuf, sizeof(sysbuf), "%s %s",
		    _PATH_WHATIS, dirname(e_whatdb->s));
		if (f_noprint == 0)
			printf("%s\n", sysbuf);
		if (f_noaction == 0)
			dosystem(sysbuf);
	}
}

static void
dosystem(const char *cmd)
{
	int status;

	if ((status = system(cmd)) == 0)
		return;

	if (status == -1)
		err(1, "cannot execute action");
	if (WIFSIGNALED(status))
		errx(1, "child was signaled to quit. aborting");
	if (WIFSTOPPED(status))
		errx(1, "child was stopped. aborting");
	if (f_ignerr == 0)
		errx(1, "*** Exited %d", status);
	warnx("*** Exited %d (continuing)", status);
}

static void
usage(void)
{
	(void)fprintf(stderr,
	    "usage: catman [-knpsw] [-m manpath] [sections]\n");
	(void)fprintf(stderr,
	    "       catman [-knpsw] [-M manpath] [sections]\n");
	exit(1);
}
