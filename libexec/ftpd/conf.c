/*	$NetBSD: conf.c,v 1.25 2000/01/08 11:09:56 lukem Exp $	*/

/*-
 * Copyright (c) 1997-2000 The NetBSD Foundation, Inc.
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
__RCSID("$NetBSD: conf.c,v 1.25 2000/01/08 11:09:56 lukem Exp $");
#endif /* not lint */

#include <sys/types.h>
#include <sys/param.h>
#include <sys/stat.h>

#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <glob.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stringlist.h>
#include <syslog.h>
#include <time.h>
#include <unistd.h>
#include <util.h>

#ifdef KERBEROS5
#include <krb5/krb5.h>
#endif

#include "extern.h"
#include "pathnames.h"

static char *strend __P((const char *, char *));
static int filetypematch __P((char *, int));

struct ftpclass curclass;


/*
 * Initialise curclass to an `empty' state
 */
void
init_curclass()
{
	struct ftpconv	*conv, *cnext;

	for (conv = curclass.conversions; conv != NULL; conv = cnext) {
		REASSIGN(conv->suffix, NULL);
		REASSIGN(conv->types, NULL);
		REASSIGN(conv->disable, NULL);
		REASSIGN(conv->command, NULL);
		cnext = conv->next;
		free(conv);
	}
	curclass.checkportcmd = 0;
	REASSIGN(curclass.classname, NULL);
	curclass.conversions =	NULL;
	REASSIGN(curclass.display, NULL);
	curclass.limit =	-1;		/* unlimited connections */
	REASSIGN(curclass.limitfile, NULL);
	curclass.maxrateget =	0;
	curclass.maxrateput =	0;
	curclass.maxtimeout =	7200;		/* 2 hours */
	curclass.modify =	1;
	REASSIGN(curclass.motd, xstrdup(_PATH_FTPLOGINMESG));
	REASSIGN(curclass.notify, NULL);
	curclass.passive =	1;
	curclass.maxrateget =	0;
	curclass.maxrateput =	0;
	curclass.rateget =	0;
	curclass.rateput =	0;
	curclass.timeout =	900;		/* 15 minutes */
	curclass.umask =	027;
	curclass.upload =	1;

}

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
	int		 none, match, rate;
	char		*endp;
	char		*class, *word, *arg;
	const char	*infile;
	size_t		 line;
	unsigned int	 timeout;
	struct ftpconv	*conv, *cnext;

	init_curclass();
	REASSIGN(curclass.classname, xstrdup(findclass));
	if (strcasecmp(findclass, "guest") == 0) {
		curclass.modify = 0;
		curclass.umask = 0707;
	}

	infile = conffilename(_PATH_FTPDCONF);
	if ((f = fopen(infile, "r")) == NULL)
		return;

	line = 0;
	for (;
	    (buf = fparseln(f, &len, &line, NULL, FPARSELN_UNESCCOMM |
	    		FPARSELN_UNESCCONT | FPARSELN_UNESCESC)) != NULL;
	    free(buf)) {
		none = match = 0;
		p = buf;
		if (len < 1)
			continue;
		if (p[len - 1] == '\n')
			p[--len] = '\0';
		if (EMPTYSTR(p))
			continue;
		
		NEXTWORD(p, word);
		NEXTWORD(p, class);
		NEXTWORD(p, arg);
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

		} else if (strcasecmp(word, "classtype") == 0) {
			if (!none && !EMPTYSTR(arg)) {
				if (strcasecmp(arg, "GUEST") == 0)
					curclass.type = CLASS_GUEST;
				else if (strcasecmp(arg, "CHROOT") == 0)
					curclass.type = CLASS_CHROOT;
				else if (strcasecmp(arg, "REAL") == 0)
					curclass.type = CLASS_REAL;
				else {
					syslog(LOG_WARNING,
				    "%s line %d: unknown class type `%s'",
					    infile, (int)line, arg);
					continue;
				}
			}

		} else if (strcasecmp(word, "conversion") == 0) {
			char *suffix, *types, *disable, *convcmd;

			if (EMPTYSTR(arg)) {
				syslog(LOG_WARNING,
				    "%s line %d: %s requires a suffix",
				    infile, (int)line, word);
				continue;	/* need a suffix */
			}
			NEXTWORD(p, types);
			NEXTWORD(p, disable);
			convcmd = p;
			if (convcmd)
				convcmd += strspn(convcmd, " \t");
			suffix = xstrdup(arg);
			if (none || EMPTYSTR(types) ||
			    EMPTYSTR(disable) || EMPTYSTR(convcmd)) {
				types = NULL;
				disable = NULL;
				convcmd = NULL;
			} else {
				types = xstrdup(types);
				disable = xstrdup(disable);
				convcmd = xstrdup(convcmd);
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
				conv->next = NULL;
				for (cnext = curclass.conversions;
				    cnext != NULL; cnext = cnext->next)
					if (cnext->next == NULL)
						break;
				if (cnext != NULL)
					cnext->next = conv;
				else
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
				arg = xstrdup(arg);
			REASSIGN(curclass.display, arg);

		} else if (strcasecmp(word, "limit") == 0) {
			int limit;

			if (none || EMPTYSTR(arg))
				continue;
			limit = (int)strtol(arg, &endp, 10);
			if (*endp != 0) {
				syslog(LOG_WARNING,
				    "%s line %d: invalid limit %s",
				    infile, (int)line, arg);
				continue;
			}
			curclass.limit = limit;
			REASSIGN(curclass.limitfile, xstrdup(p));

		} else if (strcasecmp(word, "maxtimeout") == 0) {
			if (none || EMPTYSTR(arg))
				continue;
			timeout = (unsigned int)strtoul(arg, &endp, 10);
			if (*endp != 0) {
				syslog(LOG_WARNING,
				    "%s line %d: invalid maxtimeout %s",
				    infile, (int)line, arg);
				continue;
			}
			if (timeout < 30) {
				syslog(LOG_WARNING,
				    "%s line %d: maxtimeout %d < 30 seconds",
				    infile, (int)line, timeout);
				continue;
			}
			if (timeout < curclass.timeout) {
				syslog(LOG_WARNING,
				    "%s line %d: maxtimeout %d < timeout (%d)",
				    infile, (int)line, timeout,
				    curclass.timeout);
				continue;
			}
			curclass.maxtimeout = timeout;

		} else if (strcasecmp(word, "modify") == 0) {
			if (none ||
			    (!EMPTYSTR(arg) && strcasecmp(arg, "off") == 0))
				curclass.modify = 0;
			else
				curclass.modify = 1;

		} else if (strcasecmp(word, "motd") == 0) {
			if (none || EMPTYSTR(arg))
				arg = NULL;
			else
				arg = xstrdup(arg);
			REASSIGN(curclass.motd, arg);


		} else if (strcasecmp(word, "notify") == 0) {
			if (none || EMPTYSTR(arg))
				arg = NULL;
			else
				arg = xstrdup(arg);
			REASSIGN(curclass.notify, arg);

		} else if (strcasecmp(word, "passive") == 0) {
			if (none ||
			    (!EMPTYSTR(arg) && strcasecmp(arg, "off") == 0))
				curclass.passive = 0;
			else
				curclass.passive = 1;

		} else if (strcasecmp(word, "rateget") == 0) {
			if (none || EMPTYSTR(arg))
				continue;
			rate = strsuftoi(arg);
			if (rate == -1) {
				syslog(LOG_WARNING,
				    "%s line %d: invalid rateget %s",
				    infile, (int)line, arg);
				continue;
			}
			curclass.maxrateget = rate;
			curclass.rateget = rate;

		} else if (strcasecmp(word, "rateput") == 0) {
			if (none || EMPTYSTR(arg))
				continue;
			rate = strsuftoi(arg);
			if (rate == -1) {
				syslog(LOG_WARNING,
				    "%s line %d: invalid rateput %s",
				    infile, (int)line, arg);
				continue;
			}
			curclass.maxrateput = rate;
			curclass.rateput = rate;

		} else if (strcasecmp(word, "timeout") == 0) {
			if (none || EMPTYSTR(arg))
				continue;
			timeout = (unsigned int)strtoul(arg, &endp, 10);
			if (*endp != 0) {
				syslog(LOG_WARNING,
				    "%s line %d: invalid timeout %s",
				    infile, (int)line, arg);
				continue;
			}
			if (timeout < 30) {
				syslog(LOG_WARNING,
				    "%s line %d: timeout %d < 30 seconds",
				    infile, (int)line, timeout);
				continue;
			}
			if (timeout > curclass.maxtimeout) {
				syslog(LOG_WARNING,
				    "%s line %d: timeout %d > maxtimeout (%d)",
				    infile, (int)line, timeout,
				    curclass.maxtimeout);
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
				    infile, (int)line, arg);
				continue;
			}
			curclass.umask = umask;

		} else if (strcasecmp(word, "upload") == 0) {
			if (none ||
			    (!EMPTYSTR(arg) && strcasecmp(arg, "off") == 0)) {
				curclass.modify = 0;
				curclass.upload = 0;
			} else
				curclass.upload = 1;

		} else {
			syslog(LOG_WARNING,
			    "%s line %d: unknown directive '%s'",
			    infile, (int)line, word);
			continue;
		}
	}
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
	char	 cwd[MAXPATHLEN];
	char	*cp, **rlist;

		/* Setup list for directory cache */
	if (slist == NULL)
		slist = sl_init();
	if (slist == NULL) {
		syslog(LOG_WARNING, "can't allocate memory for stringlist");
		return;
	}

		/* Check if this directory has already been visited */
	if (getcwd(cwd, sizeof(cwd) - 1) == NULL) {
		syslog(LOG_WARNING, "can't getcwd: %s", strerror(errno));
		return;
	}
	if (sl_find(slist, cwd) != NULL)
		return;	

	cp = xstrdup(cwd);
	if (sl_add(slist, cp) == -1)
		syslog(LOG_WARNING, "can't add `%s' to stringlist", cp);

		/* First check for a display file */
	(void)format_file(curclass.display, code);

		/* Now see if there are any notify files */
	if (EMPTYSTR(curclass.notify))
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
		if (code != 0) {
			lreply(code, "");
			code = 0;
		}
		lreply(code, "Please read the file %s", *rlist);
		t = localtime(&now);
		age = 365 * t->tm_year + t->tm_yday;
		t = localtime(&then);
		age -= 365 * t->tm_year + t->tm_yday;
		lreply(code, "  it was last modified on %.24s - %d day%s ago",
		    ctime(&then), age, PLURAL(age));
	}
	globfree(&gl);
}

int
format_file(file, code)
	const char *file;
	int code;
{
	FILE   *f;
	char   *buf, *p, *cwd;
	size_t	len;
	off_t	b;
	time_t	now;

#define PUTC(x)	putchar(x), b++

	if (EMPTYSTR(file))
		return(0);
	if ((f = fopen(file, "r")) == NULL)
		return (0);
	lreply(code, "");

	b = 0;
	for (;
	    (buf = fparseln(f, &len, NULL, "\0\0\0", 0)) != NULL; free(buf)) {
		if (len > 0)
			if (buf[len - 1] == '\n')
				buf[--len] = '\0';
		b += printf("    ");

		for (p = buf; *p; p++) {
			if (*p == '%') {
				p++;
				switch (*p) {

				case 'c':
					b += printf("%s",
					    curclass.classname ?
					    curclass.classname : "<unknown>");
					break;

				case 'C':
					if (getcwd(cwd, sizeof(cwd)-1) == NULL){
						syslog(LOG_WARNING,
						    "can't getcwd: %s",
						    strerror(errno));
						continue;
					}
					b += printf("%s", cwd);
					break;

				case 'E':
						/* XXXX email address */
					break;

				case 'L':
					b += printf("%s", hostname);
					break;

				case 'M':
					if (curclass.limit == -1)
						b += printf("unlimited");
					else
						b += printf("%d",
						    curclass.limit);
					break;

				case 'N':
					if (connections > 0)
						b += printf("%d", connections);
					break;

				case 'R':
					b += printf("%s", remotehost);
					break;

				case 'T':
					now = time(NULL);
					b += printf("%.24s", ctime(&now));
					break;

				case 'U':
					b += printf("%s",
					    pw ? pw->pw_name : "<unknown>");
					break;

				case '%':
					PUTC('%');
					break;

				}
			} else {
				PUTC(*p);
			}
		}
		PUTC('\r');
		PUTC('\n');
	}

	total_bytes += b;
	total_bytes_out += b;
	(void)fflush(stdout);
	(void)fclose(f);
	return (1);
}

/*
 * Find s2 at the end of s1.  If found, return a string up to (but
 * not including) s2, otherwise returns NULL.
 */
static char *
strend(s1, s2)
	const char *s1;
	char *s2;
{
	static	char buf[MAXPATHLEN];

	char	*start;
	size_t	l1, l2;

	l1 = strlen(s1);
	l2 = strlen(s2);

	if (l2 >= l1)
		return(NULL);
	
	strlcpy(buf, s1, sizeof(buf));
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
char **
do_conversion(fname)
	const char *fname;
{
	struct ftpconv	*cp;
	struct stat	 st;
	int		 o_errno;
	char		*base = NULL;
	char		*cmd, *p, *lp, **argv;
	StringList	*sl;

	o_errno = errno;
	sl = NULL;
	cmd = NULL;
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
	if (cp == NULL)
		goto cleanup_do_conv;

	/* Split up command into an argv */
	if ((sl = sl_init()) == NULL)
		goto cleanup_do_conv;
	cmd = xstrdup(cp->command);
	p = cmd;
	while (p) {
		NEXTWORD(p, lp);
		if (strcmp(lp, "%s") == 0)
			lp = base;
		if (sl_add(sl, xstrdup(lp)) == -1)
			goto cleanup_do_conv;
	}

	if (sl_add(sl, NULL) == -1)
		goto cleanup_do_conv;
	argv = sl->sl_str;
	free(cmd);
	free(sl);
	return(argv);

 cleanup_do_conv:
	if (sl)
		sl_free(sl, 1);
	free(cmd);
	errno = o_errno;
	return(NULL);
}

/*
 * Convert the string `arg' to an int, which may have an optional SI suffix
 * (`b', `k', `m', `g'). Returns the number for success, -1 otherwise.
 */
int
strsuftoi(arg)
	const char *arg;
{
	char *cp;
	long val;

	if (!isdigit((unsigned char)arg[0]))
		return (-1);

	val = strtol(arg, &cp, 10);
	if (cp != NULL) {
		if (cp[0] != '\0' && cp[1] != '\0')
			 return (-1);
		switch (tolower((unsigned char)cp[0])) {
		case '\0':
		case 'b':
			break;
		case 'k':
			val <<= 10;
			break;
		case 'm':
			val <<= 20;
			break;
		case 'g':
			val <<= 30;
			break;
		default:
			return (-1);
		}
	}
	if (val < 0 || val > INT_MAX)
		return (-1);

	return (val);
}

void
count_users()
{
	char	fn[MAXPATHLEN];
	int	fd, i, last;
	size_t	count;
	pid_t  *pids, mypid;
	struct stat sb;

	(void)strlcpy(fn, _PATH_CLASSPIDS, sizeof(fn));
	(void)strlcat(fn, curclass.classname, sizeof(fn));
	pids = NULL;
	connections = 1;

	if ((fd = open(fn, O_RDWR | O_CREAT | O_EXLOCK, 0600)) == -1)
		return;
	if (fstat(fd, &sb) == -1)
		goto cleanup_count;
	if ((pids = malloc(sb.st_size + sizeof(pid_t))) == NULL)
		goto cleanup_count;
	count = read(fd, pids, sb.st_size);
	if (count < 0 || count != sb.st_size)
		goto cleanup_count;
	count /= sizeof(pid_t);
	mypid = getpid();
	last = 0;
	for (i = 0; i < count; i++) {
		if (pids[i] == 0)
			continue;
		if (kill(pids[i], 0) == -1 && errno != EPERM) {
			if (mypid != 0) {
				pids[i] = mypid;
				mypid = 0;
				last = i;
			}
		} else {
			connections++;
			last = i;
		}
	}
	if (mypid != 0) {
		if (pids[last] != 0)
			last++;
		pids[last] = mypid;
	}
	count = (last + 1) * sizeof(pid_t);
	if (lseek(fd, 0, SEEK_SET) == -1)
		goto cleanup_count;
	if (write(fd, pids, count) == -1)
		goto cleanup_count;
	(void)ftruncate(fd, count);

 cleanup_count:
	(void)flock(fd, LOCK_UN);
	close(fd);
	REASSIGN(pids, NULL);
}
