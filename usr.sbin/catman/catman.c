/*      $NetBSD: catman.c,v 1.15 2000/05/29 21:05:34 jdolecek Exp $       */

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

#include "config.h"
#include "pathnames.h"

int f_nowhatis = 0;
int f_noaction = 0;
int f_noformat = 0;
int f_ignerr = 0;
int f_noprint = 0;
int dowhatis = 0;

TAG *defp;	/* pointer to _default list */

int		main __P((int, char * const *));
static void	setdefentries __P((char *, char *, const char *));
static void	uniquepath __P((void));
static void	catman __P((void));
static void	scanmandir __P((const char *, const char *));
static int	splitentry __P((char *, char *, char *));
static void	setcatsuffix __P((char *, const char *, const char *));
static void	makecat __P((const char *, const char *, const char *,
							const char *));
static void	makewhatis __P((void));
static void	dosystem __P((const char *));
static void	usage __P((void));


int
main(argc, argv)
	int argc;
	char * const *argv;
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

		case '?':
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
setdefentries(m_path, m_add, sections)
	char *m_path;
	char *m_add;
	const char *sections;
{
	TAG *defnewp, *sectnewp, *subp;
	ENTRY *e_defp, *e_subp;
	const char *p;
	char *slashp, *machine;
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
	if ((defp = getlist("_default")) == NULL)
		defp = addlist("_default");

	subp = getlist("_subdir");

	/*
	 * 0: If one or more sections was specified, rewrite _subdir list.
	 */
	if (sections != NULL) {
		sectnewp = addlist("_section_new");
		for(p=sections; *p;) {
			i = snprintf(buf, sizeof(buf), "man%c", *p++);
			for(; *p && !isdigit(*p) && i<sizeof(buf)-1; i++)
				buf[i] = *p++;
			buf[i] = '\0';
			addentry(sectnewp, buf, 0);
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
		while ((e_defp = defp->list.tqh_first) != NULL) {
			free(e_defp->s);
			TAILQ_REMOVE(&defp->list, e_defp, q);
		}
		for (p = strtok(m_path, ":");
		    p != NULL; p = strtok(NULL, ":")) {
			slashp = p[strlen(p) - 1] == '/' ? "" : "/";
			e_subp = subp == NULL ?  NULL : subp->list.tqh_first;
			for (; e_subp != NULL; e_subp = e_subp->q.tqe_next) {
				if(!strncmp(e_subp->s, "cat", 3))
					continue;
				(void)snprintf(buf, sizeof(buf), "%s%s%s{/%s,}",
				    p, slashp, e_subp->s, machine);
				addentry(defp, buf, 0);
			}
		}
	}

	/*
	 * 2: If the user did not specify MANPATH, -M or a section, rewrite
	 *    the _default list to include the _subdir list and the machine.
	 */
	if (m_path == NULL) {
		defp = getlist("_default");
		defnewp = addlist("_default_new1");
		e_defp =
		    defp->list.tqh_first == NULL ? NULL : defp->list.tqh_first;
		for (; e_defp; e_defp = e_defp->q.tqe_next) {
			slashp =
			    e_defp->s[strlen(e_defp->s) - 1] == '/' ? "" : "/";
			e_subp = subp == NULL ? NULL : subp->list.tqh_first;
			for (; e_subp; e_subp = e_subp->q.tqe_next) {
				if(!strncmp(e_subp->s, "cat", 3))
					continue;
				(void)snprintf(buf, sizeof(buf), "%s%s%s{/%s,}",
					e_defp->s, slashp, e_subp->s, machine);
				addentry(defnewp, buf, 0);
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
			e_subp = subp == NULL ? NULL : subp->list.tqh_first;
			for (; e_subp != NULL; e_subp = e_subp->q.tqe_next) {
				if(!strncmp(e_subp->s, "cat", 3))
					continue;
				(void)snprintf(buf, sizeof(buf), "%s%s%s{/%s,}",
				    p, slashp, e_subp->s, machine);
				addentry(defp, buf, 1);
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
	int i,j,len,lnk;
	char path[PATH_MAX], *p;


	e_defp = defp->list.tqh_first;
	glob(e_defp->s, GLOB_BRACE | GLOB_NOSORT, NULL, &manpaths);
	for(e_defp = e_defp->q.tqe_next; e_defp; e_defp = e_defp->q.tqe_next) {
		glob(e_defp->s, GLOB_BRACE | GLOB_NOSORT | GLOB_APPEND, NULL,
				&manpaths);
	}

	defnewp = addlist("_default_new2");

	for(i=0; i<manpaths.gl_pathc; i++) {
		lnk = 0;
		lstat(manpaths.gl_pathv[i], &st1);
		for(j=0; j<manpaths.gl_pathc; j++) {
			if (i != j) {
				lstat(manpaths.gl_pathv[j], &st2);
				if (st1.st_ino == st2.st_ino) {
					strcpy(path, manpaths.gl_pathv[i]);
					for(p = path; *(p+1) != '\0';) {
						p = dirname(p);
						lstat(p, &st3);
						if (S_ISLNK(st3.st_mode)) {
							lnk = 1;
							break;
						}
					}
				} else {
					len = readlink(manpaths.gl_pathv[i],
							path, PATH_MAX);
					if (len == -1)
						continue;
					if (!strcmp(path, manpaths.gl_pathv[j]))
						lnk = 1;
				}
				if (lnk)
					break;
			}
		}

		if (!lnk)
			addentry(defnewp, manpaths.gl_pathv[i], 0);
	}

	globfree(&manpaths);

	defp = defnewp;
}

static void
catman(void)
{
	TAG *pathp;
	ENTRY *e_path;
	const char *mandir;
	char catdir[PATH_MAX], *cp;


	pathp = getlist("_default");
	for(e_path = pathp->list.tqh_first; e_path;
				e_path = e_path->q.tqe_next) {
		mandir = e_path->s;
		strcpy(catdir, mandir);
		if(!(cp = strstr(catdir, "man/man")))
			continue;
		cp+=4; *cp++ = 'c'; *cp++ = 'a'; *cp = 't';
		scanmandir(catdir, mandir);
	}
}

static void
scanmandir(catdir, mandir)
	const char *catdir;
	const char *mandir;
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
		if (f_noaction == 0 && mkdir(catdir,0755) < 0) {
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
		buildp = getlist("_build");
		if(buildp) {
			for(e_build = buildp->list.tqh_first; e_build;
					e_build = e_build->q.tqe_next) {
				splitentry(e_build->s, buildsuff, buildcmd);
				snprintf(match, sizeof(match), "*%s",
							buildsuff);
				if(!fnmatch(match, manpage, 0))
					break;
			}
		}

		if(e_build == NULL)
			continue;

		e_crunch = NULL;
		crunchp = getlist("_crunch");
		if (crunchp) {
			for(e_crunch = crunchp->list.tqh_first; e_crunch;
					e_crunch=e_crunch->q.tqe_next) {
				splitentry(e_crunch->s, crunchsuff, crunchcmd);
				snprintf(match, sizeof(match), "*%s",
							crunchsuff);
				if(!fnmatch(match, manpage, 0))
					break;
			}
		}

		if (lstat(manpage, &manstat) <0) {
			warn("can't stat %s", manpage);
			continue;
		} else {
			if(S_ISLNK(manstat.st_mode)) {
				strcpy(buffer, catpage);
				strcpy(linkname, basename(buffer));
				len = readlink(manpage, buffer, PATH_MAX);
				if(len == -1) {
					warn("can't stat read symbolic link %s",
							manpage);
					continue;
				}
				buffer[len] = '\0';
				bp = basename(buffer);
				strcpy(tmp, manpage);
				snprintf(manpage, sizeof(manpage), "%s/%s",
						dirname(tmp), bp);
				strcpy(tmp, catpage);
				snprintf(catpage, sizeof(catpage), "%s/%s",
						dirname(tmp), buffer);
			}
			else
				*linkname='\0';
		}

		if(!e_crunch) {
			*crunchsuff = *crunchcmd = '\0';
		}
		setcatsuffix(catpage, buildsuff, crunchsuff);
		if(*linkname != '\0')
			setcatsuffix(linkname, buildsuff, crunchsuff);

		if (stat(manpage, &manstat) < 0) {
			warn("can't stat %s", manpage);
			continue;
		}

		if (!S_ISREG(manstat.st_mode)) {
			warnx("not a regular file %s",manpage);
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

		if(*linkname != '\0') {
			strcpy(tmp, catpage);
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
						strcpy(tmp, catpage);
						if(chdir(dirname(tmp)) == -1) {
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
splitentry(s, first, second)
	char *s;
	char *first;
	char *second;
{
	char *c;

	for(c = s; *c != '\0' && !isspace(*c); ++c);
	if(*c == '\0')
		return(0);
	strncpy(first, s, c-s);
	first[c-s] = '\0';
	for(; *c != '\0' && isspace(*c); ++c);
	strcpy(second, c);
		return(1);
}

static void
setcatsuffix(catpage, suffix, crunchsuff)
	char		*catpage;
	const char	*suffix;
	const char	*crunchsuff;
{
	TAG *tp;
	ENTRY *ep;
	char *p;

	for(p = catpage + strlen(catpage); p!=catpage; p--)
		if(!fnmatch(suffix, p, 0)) {
			if((tp = getlist("_suffix"))) {
				ep = tp->list.tqh_first;
				sprintf(p, "%s%s", ep->s, crunchsuff);
			} else {
				sprintf(p, ".0%s", crunchsuff);
			}
			break;
		}
}

static void
makecat(manpage, catpage, buildcmd, crunchcmd)
	const char	*manpage;
	const char	*catpage;
	const char	*buildcmd;
	const char	*crunchcmd;
{
	char crunchbuf[1024];
	char sysbuf[2048];

	snprintf(sysbuf, sizeof(sysbuf), buildcmd, manpage);

	if(*crunchcmd != '\0') {
		snprintf(crunchbuf, sizeof(crunchbuf), crunchcmd, catpage);
		snprintf(sysbuf, sizeof(sysbuf), "%s | %s", sysbuf, crunchbuf);
	} else {
		snprintf(sysbuf, sizeof(sysbuf), "%s > %s", sysbuf, catpage);
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

	whatdbp = getlist("_whatdb");
	for(e_whatdb = whatdbp->list.tqh_first; e_whatdb;
			e_whatdb = e_whatdb->q.tqe_next) {
		snprintf(sysbuf, sizeof(sysbuf), "%s %s",
				_PATH_WHATIS, dirname(e_whatdb->s));
		if (f_noprint == 0)
			printf("%s\n", sysbuf);
		if (f_noaction == 0)
			dosystem(sysbuf);
	}
}

static void
dosystem(cmd)
	const char *cmd;
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
