/*	$NetBSD: conf.c,v 1.17 1999/02/05 21:40:49 lukem Exp $	*/

/*-
 * Copyright (c) 1997 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Simon Burge and Luke Mewburn.
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
__RCSID("$NetBSD: conf.c,v 1.17 1999/02/05 21:40:49 lukem Exp $");
#endif /* not lint */

#include <sys/types.h>
#include <sys/param.h>
#include <sys/stat.h>

#include <errno.h>
#include <glob.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <string.h>
#include <stringlist.h>
#include <syslog.h>

#include "extern.h"
#include "pathnames.h"

static char *strend __P((const char *, char *));
static int filetypematch __P((char *, int));

struct ftpclass curclass;


/*
 * Parse the configuration file, looking for the named class, and
 * define curclass to contain the appropriate settings.
 */
void
parse_conf(findclass)
	char *findclass;
{
	FILE		*f;
	char		*buf, *p;
	size_t		 len;
	int		 none, match;
	char		*endp;
	char		*class, *word, *arg;
	const char	*infile;
	int		 line;
	unsigned int	 timeout;
	struct ftpconv	*conv, *cnext;

#define REASSIGN(X,Y)	if (X) free(X); (X)=(Y)
#define NEXTWORD(W)	while ((W = strsep(&buf, " \t")) != NULL && *W == '\0')
#define EMPTYSTR(W)	(W == NULL || *W == '\0')

	REASSIGN(curclass.classname, findclass);
	for (conv = curclass.conversions; conv != NULL; conv=cnext) {
		REASSIGN(conv->suffix, NULL);
		REASSIGN(conv->types, NULL);
		REASSIGN(conv->disable, NULL);
		REASSIGN(conv->command, NULL);
		cnext = conv->next;
		free(conv);
	}
	curclass.checkportcmd = 0;
	curclass.conversions =	NULL;
	REASSIGN(curclass.display, NULL);
	curclass.maxtimeout =	7200;		/* 2 hours */
	curclass.modify =	1;
	REASSIGN(curclass.notify, NULL);
	curclass.passive =	1;
	curclass.timeout =	900;		/* 15 minutes */
	curclass.umask =	027;

	if (strcasecmp(findclass, "guest") == 0) {
		curclass.modify = 0;
		curclass.umask = 0707;
	}

	infile = conffilename(_PATH_FTPDCONF);
	if ((f = fopen(infile, "r")) == NULL)
		return;

	line = 0;
	while ((buf = fgetln(f, &len)) != NULL) {
		none = match = 0;
		line++;
		if (len < 1)
			continue;
		if (buf[len - 1] != '\n') {
			syslog(LOG_WARNING,
			    "%s line %d is partially truncated?", infile, line);
			continue;
		}
		buf[--len] = '\0';
		if ((p = strchr(buf, '#')) != NULL)
			*p = '\0';
		if (EMPTYSTR(buf))
			continue;
		
		NEXTWORD(word);
		NEXTWORD(class);
		NEXTWORD(arg);
		if (EMPTYSTR(word) || EMPTYSTR(class))
			continue;
		if (strcasecmp(class, "none") == 0)
			none = 1;
		if (strcasecmp(class, findclass) != 0 &&
		    !none && strcasecmp(class, "all") != 0)
			continue;

		if (strcasecmp(word, "checkportcmd") == 0) {
			if (none ||
			    (!EMPTYSTR(arg) && strcasecmp(arg, "off") == 0))
				curclass.checkportcmd = 0;
			else
				curclass.checkportcmd = 1;
		} else if (strcasecmp(word, "conversion") == 0) {
			char *suffix, *types, *disable, *convcmd;

			if (EMPTYSTR(arg)) {
				syslog(LOG_WARNING,
				    "%s line %d: %s requires a suffix",
				    infile, line, word);
				continue;	/* need a suffix */
			}
			NEXTWORD(types);
			NEXTWORD(disable);
			convcmd = buf;
			if (convcmd)
				convcmd += strspn(convcmd, " \t");
			suffix = strdup(arg);
			if (suffix == NULL) {
				syslog(LOG_WARNING, "can't strdup");
				continue;
			}
			if (none || EMPTYSTR(types) ||
			    EMPTYSTR(disable) || EMPTYSTR(convcmd)) {
				types = NULL;
				disable = NULL;
				convcmd = NULL;
			} else {
				types = strdup(types);
				disable = strdup(disable);
				convcmd = strdup(convcmd);
				if (types == NULL || disable == NULL ||
				    convcmd == NULL) {
					syslog(LOG_WARNING, "can't strdup");
					if (types)
						free(types);
					if (disable)
						free(disable);
					if (convcmd)
						free(convcmd);
					continue;
				}
			}
			for (conv = curclass.conversions; conv != NULL;
			    conv = conv->next) {
				if (strcmp(conv->suffix, suffix) == 0)
					break;
			}
			if (conv == NULL) {
				conv = (struct ftpconv *)
				    calloc(1, sizeof(struct ftpconv));
				if (conv == NULL) {
					syslog(LOG_WARNING, "can't malloc");
					continue;
				}
				conv->next = curclass.conversions;
				curclass.conversions = conv;
			}
			REASSIGN(conv->suffix, suffix);
			REASSIGN(conv->types, types);
			REASSIGN(conv->disable, disable);
			REASSIGN(conv->command, convcmd);
		} else if (strcasecmp(word, "display") == 0) {
			if (none || EMPTYSTR(arg))
				arg = NULL;
			else
				arg = strdup(arg);
			REASSIGN(curclass.display, arg);
		} else if (strcasecmp(word, "maxtimeout") == 0) {
			if (none || EMPTYSTR(arg))
				continue;
			timeout = (unsigned int)strtoul(arg, &endp, 10);
			if (*endp != 0) {
				syslog(LOG_WARNING,
				    "%s line %d: invalid maxtimeout %s",
				    infile, line, arg);
				continue;
			}
			if (timeout < 30) {
				syslog(LOG_WARNING,
				    "%s line %d: maxtimeout %d < 30 seconds",
				    infile, line, timeout);
				continue;
			}
			if (timeout < curclass.timeout) {
				syslog(LOG_WARNING,
				    "%s line %d: maxtimeout %d < timeout (%d)",
				    infile, line, timeout, curclass.timeout);
				continue;
			}
			curclass.maxtimeout = timeout;
		} else if (strcasecmp(word, "modify") == 0) {
			if (none ||
			    (!EMPTYSTR(arg) && strcasecmp(arg, "off") == 0))
				curclass.modify = 0;
			else
				curclass.modify = 1;
		} else if (strcasecmp(word, "notify") == 0) {
			if (none || EMPTYSTR(arg))
				arg = NULL;
			else
				arg = strdup(arg);
			REASSIGN(curclass.notify, arg);
		} else if (strcasecmp(word, "passive") == 0) {
			if (none ||
			    (!EMPTYSTR(arg) && strcasecmp(arg, "off") == 0))
				curclass.passive = 0;
			else
				curclass.passive = 1;
		} else if (strcasecmp(word, "timeout") == 0) {
			if (none || EMPTYSTR(arg))
				continue;
			timeout = (unsigned int)strtoul(arg, &endp, 10);
			if (*endp != 0) {
				syslog(LOG_WARNING,
				    "%s line %d: invalid timeout %s",
				    infile, line, arg);
				continue;
			}
			if (timeout < 30) {
				syslog(LOG_WARNING,
				    "%s line %d: timeout %d < 30 seconds",
				    infile, line, timeout);
				continue;
			}
			if (timeout > curclass.maxtimeout) {
				syslog(LOG_WARNING,
				    "%s line %d: timeout %d > maxtimeout (%d)",
				    infile, line, timeout, curclass.maxtimeout);
				continue;
			}
			curclass.timeout = timeout;
		} else if (strcasecmp(word, "umask") == 0) {
			mode_t umask;

			if (none || EMPTYSTR(arg))
				continue;
			umask = (mode_t)strtoul(arg, &endp, 8);
			if (*endp != 0 || umask > 0777) {
				syslog(LOG_WARNING,
				    "%s line %d: invalid umask %s",
				    infile, line, arg);
				continue;
			}
			curclass.umask = umask;
		} else {
			syslog(LOG_WARNING,
			    "%s line %d: unknown directive '%s'",
			    infile, line, word);
			continue;
		}
	}
#undef REASSIGN
#undef NEXTWORD
#undef EMPTYSTR
	fclose(f);
}

/*
 * Show file listed in curclass.display first time in, and list all the
 * files named in curclass.notify in the current directory.  Send back
 * responses with the prefix `code' + "-".
 */
void
show_chdir_messages(code)
	int	code;
{
	static StringList *slist = NULL;

	struct stat st;
	struct tm *t;
	glob_t	 gl;
	time_t	 now, then;
	int	 age;
	char	 cwd[MAXPATHLEN + 1];
	char	 line[BUFSIZ];
	char	*cp, **rlist;
	FILE	*f;

		/* Setup list for directory cache */
	if (slist == NULL)
		slist = sl_init();

		/* Check if this directory has already been visited */
	if (getcwd(cwd, sizeof(cwd) - 1) == NULL) {
		syslog(LOG_WARNING, "can't getcwd: %s", strerror(errno));
		return;
	}
	if (sl_find(slist, cwd) != NULL)
		return;	

	cp = strdup(cwd);
	if (cp == NULL) {
		syslog(LOG_WARNING, "can't strdup");
		return;
	}
	sl_add(slist, cp);

		/* First check for a display file */
	if (curclass.display != NULL && curclass.display[0] &&
	    (f = fopen(curclass.display, "r")) != NULL) {
		while (fgets(line, BUFSIZ, f)) {
			if ((cp = strchr(line, '\n')) != NULL)
				*cp = '\0';
			lreply(code, "%s", line);
		}
		fclose(f);
		lreply(code, "");
	}

		/* Now see if there are any notify files */
	if (curclass.notify == NULL || curclass.notify[0] == '\0')
		return;

	if (glob(curclass.notify, 0, NULL, &gl) != 0 || gl.gl_matchc == 0)
		return;
	time(&now);
	for (rlist = gl.gl_pathv; *rlist != NULL; rlist++) {
		if (stat(*rlist, &st) != 0)
			continue;
		if (!S_ISREG(st.st_mode))
			continue;
		then = st.st_mtime;
		lreply(code, "Please read the file %s", *rlist);
		t = localtime(&now);
		age = 365 * t->tm_year + t->tm_yday;
		t = localtime(&then);
		age -= 365 * t->tm_year + t->tm_yday;
		lreply(code, "  it was last modified on %.24s - %d day%s ago",
		    ctime(&then), age, age == 1 ? "" : "s");
	}
	globfree(&gl);
}

/*
 * Find s2 at the end of s1.  If found, return a string up and up (but
 * not including) s2, otherwise returns NULL.
 */
static char *
strend(s1, s2)
	const char *s1;
	char *s2;
{
	static	char buf[MAXPATHLEN + 1];

	char	*start;
	size_t	l1, l2;

	l1 = strlen(s1);
	l2 = strlen(s2);

	if (l2 >= l1)
		return(NULL);
	
	strncpy(buf, s1, MAXPATHLEN);
	start = buf + (l1 - l2);

	if (strcmp(start, s2) == 0) {
		*start = '\0';
		return(buf);
	} else
		return(NULL);
}

static int
filetypematch(types, mode)
	char	*types;
	int	mode;
{
	for ( ; types[0] != '\0'; types++)
		switch (*types) {
		  case 'd':
			if (S_ISDIR(mode))
				return(1);
			break;
		  case 'f':
			if (S_ISREG(mode))
				return(1);
			break;
		}
	return(0);
}

/*
 * Look for a conversion.  If we succeed, return a pointer to the
 * command to execute for the conversion.
 *
 * The command is stored in a static array so there's no memory
 * leak problems, and not too much to change in ftpd.c.  This
 * routine doesn't need to be re-entrant unless we start using a
 * multi-threaded ftpd, and that's not likely for a while...
 */
char *
do_conversion(fname)
	const char *fname;
{
	static char	 cmd[LINE_MAX];

	struct ftpconv	*cp;
	struct stat	 st;
	int		 o_errno;
	char		*base = NULL;

	o_errno = errno;
	for (cp = curclass.conversions; cp != NULL; cp = cp->next) {
		if (cp->suffix == NULL) {
			syslog(LOG_WARNING,
			    "cp->suffix==NULL in conv list; SHOULDN'T HAPPEN!");
			continue;
		}
		if ((base = strend(fname, cp->suffix)) == NULL)
			continue;
		if (cp->types == NULL || cp->disable == NULL ||
		    cp->command == NULL)
			continue;
					/* Is it enabled? */
		if (strcmp(cp->disable, ".") != 0 &&
		    stat(cp->disable, &st) == 0)
				continue;
					/* Does the base exist? */
		if (stat(base, &st) < 0)
			continue;
					/* Is the file type ok */
		if (!filetypematch(cp->types, st.st_mode))
			continue;
		break;			/* "We have a winner!" */
	}

	/* If we got through the list, no conversion */
	if (cp == NULL) {
		errno = o_errno;
		return(NULL);
	}

	snprintf(cmd, LINE_MAX, cp->command, base);
	syslog(LOG_DEBUG, "get command: %s", cmd);
	return(cmd);
}
