/*	$NetBSD: misc.c,v 1.5 2017/08/17 08:53:00 christos Exp $	*/

/* Copyright 1988,1990,1993,1994 by Paul Vixie
 * All rights reserved
 */

/*
 * Copyright (c) 2004 by Internet Systems Consortium, Inc. ("ISC")
 * Copyright (c) 1997,2000 by Internet Software Consortium, Inc.
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND ISC DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS.  IN NO EVENT SHALL ISC BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT
 * OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */
#include <sys/cdefs.h>
#if !defined(lint) && !defined(LINT)
#if 0
static char rcsid[] = "Id: misc.c,v 1.16 2004/01/23 18:56:43 vixie Exp";
#else
__RCSID("$NetBSD: misc.c,v 1.5 2017/08/17 08:53:00 christos Exp $");
#endif
#endif

/* vix 26jan87 [RCS has the rest of the log]
 * vix 30dec86 [written]
 */

#include "cron.h"
#include <limits.h>
#include <vis.h>

#if defined(SYSLOG) && defined(LOG_FILE)
# undef LOG_FILE
#endif

#if defined(LOG_DAEMON) && !defined(LOG_CRON)
# define LOG_CRON LOG_DAEMON
#endif

#ifndef FACILITY
#define FACILITY LOG_CRON
#endif

static int LogFD = ERR;

#if defined(SYSLOG)
static int syslog_open = FALSE;
#endif

static void mkprint(char *, char *, size_t);

/*
 * glue_strings is the overflow-safe equivalent of
 *		sprintf(buffer, "%s%c%s", a, separator, b);
 *
 * returns 1 on success, 0 on failure.  'buffer' MUST NOT be used if
 * glue_strings fails.
 */
int
glue_strings(char *buffer, size_t buffer_size, const char *a, const char *b,
	     char separator)
{
	char *buf;
	char *buf_end;

	if (buffer_size == 0)
		return (0);
	buf_end = buffer + buffer_size;
	buf = buffer;

	for ( /* nothing */; buf < buf_end && *a != '\0'; buf++, a++ )
		*buf = *a;
	if (buf == buf_end)
		return (0);
	if (separator != '/' || buf == buffer || buf[-1] != '/')
		*buf++ = separator;
	if (buf == buf_end)
		return (0);
	for ( /* nothing */; buf < buf_end && *b != '\0'; buf++, b++ )
		*buf = *b;
	if (buf == buf_end)
		return (0);
	*buf = '\0';
	return (1);
}

int
strcmp_until(const char *left, const char *right, char until) {
	while (*left && *left != until && *left == *right) {
		left++;
		right++;
	}

	if ((*left=='\0' || *left == until) &&
	    (*right=='\0' || *right == until)) {
		return (0);
	}
	return (*left - *right);
}

#ifdef notdef
/* strdtb(s) - delete trailing blanks in string 's' and return new length
 */
int
strdtb(char *s) {
	char	*x = s;

	/* scan forward to the null
	 */
	while (*x)
		x++;

	/* scan backward to either the first character before the string,
	 * or the last non-blank in the string, whichever comes first.
	 */
	do	{x--;}
	while (x >= s && isspace((unsigned char)*x));

	/* one character beyond where we stopped above is where the null
	 * goes.
	 */
	*++x = '\0';

	/* the difference between the position of the null character and
	 * the position of the first character of the string is the length.
	 */
	return (int)(x - s);
}
#endif

int
set_debug_flags(const char *flags) {
	/* debug flags are of the form    flag[,flag ...]
	 *
	 * if an error occurs, print a message to stdout and return FALSE.
	 * otherwise return TRUE after setting ERROR_FLAGS.
	 */

#if !DEBUGGING

	printf("this program was compiled without debugging enabled\n");
	return (FALSE);

#else /* DEBUGGING */

	const char *pc = flags;

	DebugFlags = 0;

	while (*pc) {
		const char	* const *test;
		int		mask;

		/* try to find debug flag name in our list.
		 */
		for (test = DebugFlagNames, mask = 1;
		     *test != NULL && strcmp_until(*test, pc, ',');
		     test++, mask <<= 1)
			continue;

		if (!*test) {
			warnx("unrecognized debug flag <%s> <%s>\n", flags, pc);
			return (FALSE);
		}

		DebugFlags |= mask;

		/* skip to the next flag
		 */
		while (*pc && *pc != ',')
			pc++;
		if (*pc == ',')
			pc++;
	}

	if (DebugFlags) {
		int flag;

		(void)fprintf(stderr, "debug flags enabled:");

		for (flag = 0;  DebugFlagNames[flag];  flag++)
			if (DebugFlags & (1 << flag))
				(void)fprintf(stderr, " %s", DebugFlagNames[flag]);
		(void)fprintf(stderr, "\n");
	}

	return (TRUE);

#endif /* DEBUGGING */
}

void
set_cron_uid(void) {
#if defined(BSD) || defined(POSIX)
	if (seteuid(ROOT_UID) < OK) {
		err(ERROR_EXIT, "cannot seteuid");
	}
#else
	if (setuid(ROOT_UID) < OK) {
		err(ERROR_EXIT, "cannot setuid");
	}
#endif
}

void
set_cron_cwd(void) {
	struct stat sb;
	struct group *grp = NULL;

#ifdef CRON_GROUP
	grp = getgrnam(CRON_GROUP);
#endif
	/* first check for CRONDIR ("/var/cron" or some such)
	 */
#ifdef ENABLE_FIX_DIRECTORIES
	if (stat(CRONDIR, &sb) < OK && errno == ENOENT) {
		warn("Cannot stat `%s'", CRONDIR);
		if (OK == mkdir(CRONDIR, 0710)) {
			(void)fprintf(stderr, "%s: created\n", CRONDIR);
			if (stat(CRONDIR, &sb) == -1)
				err(ERROR_EXIT, "cannot stat `%s'", CRONDIR);
		} else {
			err(ERROR_EXIT, "cannot create `%s'", CRONDIR);
		}
	}
	if (!S_ISDIR(sb.st_mode)) {
		errx(ERROR_EXIT, "`%s' is not a directory, bailing out.",
			CRONDIR);
	}
#endif /* ENABLE_FIX_DIRECTORIES */
	if (chdir(CRONDIR) < OK) {
		err(ERROR_EXIT, "cannot chdir `%s', bailing out.\n", CRONDIR);
	}

	/* CRONDIR okay (now==CWD), now look at SPOOL_DIR ("tabs" or some such)
	 */
#ifdef ENABLE_FIX_DIRECTORIES
	if (stat(SPOOL_DIR, &sb) < OK && errno == ENOENT) {
		warn("cannot stat `%s'", SPOOL_DIR);
		if (OK == mkdir(SPOOL_DIR, 0700)) {
			(void)fprintf(stderr, "%s: created\n", SPOOL_DIR);
			if (stat(SPOOL_DIR, &sb) == -1)
				err(ERROR_EXIT, "cannot stat `%s'", CRONDIR);
		} else {
			err(ERROR_EXIT, "cannot create `%s'", SPOOL_DIR);
		}
	}
#else
	if (stat(SPOOL_DIR, &sb)) {
		err(ERROR_EXIT, "cannot stat `%s'", SPOOL_DIR);
	}
#endif /* ENABLE_FIX_DIRECTORIES */
	if (!S_ISDIR(sb.st_mode)) {
		errx(ERROR_EXIT, "`%s' is not a directory, bailing out.",
			SPOOL_DIR);
	}
	if (grp != NULL) {
		if (sb.st_gid != grp->gr_gid) {
#ifdef ENABLE_FIX_DIRECTORIES
			errx(ERROR_EXIT, "Bad group %d != %d for `%s'",
			    (int)sb.st_gid, (int)grp->gr_gid, SPOOL_DIR);
#else				
			if (chown(SPOOL_DIR, (uid_t)-1, grp->gr_gid) == -1)
			    err(ERROR_EXIT, "cannot chown `%s'", SPOOL_DIR);
#endif
		}
		if (sb.st_mode != 01730)
#ifdef ENABLE_FIX_DIRECTORIES
			errx(ERROR_EXIT, "Bad mode %#o != %#o for `%s'",
			    (int)sb.st_mode, 01730, SPOOL_DIR);
#else				
			if (chmod(SPOOL_DIR, 01730) == -1)
			    err(ERROR_EXIT, "cannot chmod `%s'", SPOOL_DIR);
#endif
	}
}

/* acquire_daemonlock() - write our PID into /etc/cron.pid, unless
 *	another daemon is already running, which we detect here.
 *
 * note: main() calls us twice; once before forking, once after.
 *	we maintain static storage of the file pointer so that we
 *	can rewrite our PID into _PATH_CRON_PID after the fork.
 */
void
acquire_daemonlock(int closeflag) {
	static int fd = -1;
	char buf[3*MAX_FNAME];
	const char *pidfile;
	char *ep;
	long otherpid;
	ssize_t num;

	if (closeflag) {
		/* close stashed fd for child so we don't leak it. */
		if (fd != -1) {
			(void)close(fd);
			fd = -1;
		}
		return;
	}

	if (fd == -1) {
		pidfile = _PATH_CRON_PID;
		/* Initial mode is 0600 to prevent flock() race/DoS. */
		if ((fd = open(pidfile, O_RDWR|O_CREAT, 0600)) == -1) {
			log_itx("CRON", getpid(), "DEATH",
			    "can't open or create %s: %s",
			    pidfile, strerror(errno));
			exit(ERROR_EXIT);
		}
		/* fd must be > STDERR since we dup fd 0-2 to /dev/null */
		if (fd <= STDERR) {
			if (dup2(fd, STDERR + 1) < 0) {
				log_itx("CRON", getpid(), "DEATH",
				    "can't dup pid fd: %s", strerror(errno));
 				exit(ERROR_EXIT);
 			}
			close(fd);
			fd = STDERR + 1;
		}

		if (flock(fd, LOCK_EX|LOCK_NB) < OK) {
			int save_errno = errno;

			memset(buf, 0, sizeof(buf));
			if ((num = read(fd, buf, sizeof(buf) - 1)) > 0 &&
			    (otherpid = strtol(buf, &ep, 10)) > 0 &&
			    ep != buf && *ep == '\n' && otherpid != LONG_MAX) {
				log_itx("CRON", getpid(), "DEATH",
				    "can't lock %s, otherpid may be %ld: %s",
				    pidfile, otherpid, strerror(save_errno));
			} else {
				log_itx("CRON", getpid(), "DEATH",
				    "can't lock %s, otherpid unknown: %s",
				    pidfile, strerror(save_errno));
			}
			exit(ERROR_EXIT);
		}
		(void) fchmod(fd, 0644);
		(void) fcntl(fd, F_SETFD, 1);
	}

	(void)snprintf(buf, sizeof(buf), "%ld\n", (long)getpid());
	(void) lseek(fd, (off_t)0, SEEK_SET);
	num = write(fd, buf, strlen(buf));
	(void) ftruncate(fd, num);

	/* abandon fd even though the file is open. we need to keep
	 * it open and locked, but we don't need the handles elsewhere.
	 */
}

/* get_char(file) : like getc() but increment LineNumber on newlines
 */
int
get_char(FILE *file) {
	int ch;

	ch = getc(file);
	if (ch == '\n')
		Set_LineNum(LineNumber + 1);
	return (ch);
}

/* unget_char(ch, file) : like ungetc but do LineNumber processing
 */
void
unget_char(int ch, FILE *file) {
	(void)ungetc(ch, file);
	if (ch == '\n')
		Set_LineNum(LineNumber - 1);
}

/* get_string(str, max, file, termstr) : like fgets() but
 *		(1) has terminator string which should include \n
 *		(2) will always leave room for the null
 *		(3) uses get_char() so LineNumber will be accurate
 *		(4) returns EOF or terminating character, whichever
 */
int
get_string(char *string, int size, FILE *file, const char *terms) {
	int ch;

	while (EOF != (ch = get_char(file)) && !strchr(terms, ch)) {
		if (size > 1) {
			*string++ = (char) ch;
			size--;
		}
	}

	if (size > 0)
		*string = '\0';

	return (ch);
}

/* skip_comments(file) : read past comment (if any)
 */
void
skip_comments(FILE *file) {
	int ch;

	while (EOF != (ch = get_char(file))) {
		/* ch is now the first character of a line.
		 */

		while (ch == ' ' || ch == '\t')
			ch = get_char(file);

		if (ch == EOF)
			break;

		/* ch is now the first non-blank character of a line.
		 */

		if (ch != '\n' && ch != '#')
			break;

		/* ch must be a newline or comment as first non-blank
		 * character on a line.
		 */

		while (ch != '\n' && ch != EOF)
			ch = get_char(file);

		/* ch is now the newline of a line which we're going to
		 * ignore.
		 */
	}
	if (ch != EOF)
		unget_char(ch, file);
}

void
log_itx(const char *username, PID_T xpid, const char *event, const char *fmt,
    ...)
{
	char *detail;
	va_list ap;
	va_start(ap, fmt);
	if (vasprintf(&detail, fmt, ap) == -1) {
		va_end(ap);
		return;
	}
	log_it(username, xpid, event, detail);
	free(detail);
}

void
log_it(const char *username, PID_T xpid, const char *event, const char *detail) {
#if defined(LOG_FILE) || DEBUGGING
	PID_T pid = xpid;
#endif
#if defined(LOG_FILE)
	char *msg;
	int msglen;
	TIME_T now = time((TIME_T) 0);
	struct tm *t = localtime(&now);

	if (LogFD < OK) {
		LogFD = open(LOG_FILE, O_WRONLY|O_APPEND|O_CREAT, 0600);
		if (LogFD < OK) {
			warn("can't open log file `%s'", LOG_FILE);
		} else {
			(void) fcntl(LogFD, F_SETFD, FD_CLOEXEC);
		}
	}

	/* we have to sprintf() it because fprintf() doesn't always write
	 * everything out in one chunk and this has to be atomically appended
	 * to the log file.
	 */
	msglen = asprintf(&msg,
	    "%s (%02d/%02d-%02d:%02d:%02d-%d) %s (%s)\n", username,
	    t->tm_mon + 1, t->tm_mday, t->tm_hour, t->tm_min, t->tm_sec, pid,
	    event, detail);
	if (msglen == -1)
		return;

	if (LogFD < OK || write(LogFD, msg, (size_t)msglen) < OK) {
		warn("can't write to log file");
		write(STDERR, msg, (size_t)msglen);
	}

	free(msg);
#endif /*LOG_FILE*/

#if defined(SYSLOG)
	if (!syslog_open) {
# ifdef LOG_DAEMON
		openlog(getprogname(), LOG_PID, FACILITY);
# else
		openlog(getprogname(), LOG_PID);
# endif
		syslog_open = TRUE;		/* assume openlog success */
	}

	syslog(LOG_INFO, "(%s) %s (%s)", username, event, detail);

#endif /*SYSLOG*/

#if DEBUGGING
	if (DebugFlags) {
		(void)fprintf(stderr, "log_it: (%s %ld) %s (%s)\n",
			username, (long)pid, event, detail);
	}
#endif
}

void
log_close(void) {
	if (LogFD != ERR) {
		(void)close(LogFD);
		LogFD = ERR;
	}
#if defined(SYSLOG)
	closelog();
	syslog_open = FALSE;
#endif /*SYSLOG*/
}

/* warning:
 *	heavily ascii-dependent.
 */
static void
mkprint(char *dst, char *src, size_t len)
{
	while(len > 0 && isblank((unsigned char) *src))
		len--, src++;

	(void)strvisx(dst, src, len, VIS_TAB|VIS_NL);
}

/* warning:
 *	returns a pointer to malloc'd storage, you must call free yourself.
 */
char *
mkprints(char *src, size_t len)
{
	char *dst = malloc(len*4 + 1);

	if (dst)
		mkprint(dst, src, len);

	return (dst);
}

#ifdef MAIL_DATE
/* Sat, 27 Feb 1993 11:44:51 -0800 (CST)
 * 1234567890123456789012345678901234567
 */
char *
arpadate(time_t *clock)
{
	time_t t = clock ? *clock : time((TIME_T) 0);
	struct tm tm = *localtime(&t);
	long gmtoff = get_gmtoff(&t, &tm);
	int hours = gmtoff / SECONDS_PER_HOUR;
	int minutes = (gmtoff - (hours * SECONDS_PER_HOUR)) / SECONDS_PER_MINUTE;
	static char ret[64];	/* zone name might be >3 chars */

	if (minutes < 0)
		minutes = -minutes;
	
	(void)strftime(ret, sizeof(ret), "%a, %e %b %Y %T ????? (%Z)", &tm);
	(void)snprintf(strchr(ret, '?'), "% .2d%.2d", hours, minutes);
	ret[sizeof(ret) - 1] = '\0';
	return ret;
}
#endif /*MAIL_DATE*/

size_t
strlens(const char *last, ...) {
	va_list ap;
	size_t ret = 0;
	const char *str;

	va_start(ap, last);
	for (str = last; str != NULL; str = va_arg(ap, const char *))
		ret += strlen(str);
	va_end(ap);
	return (ret);
}

/* Return the offset from GMT in seconds (algorithm taken from sendmail).
 *
 * warning:
 *	clobbers the static storage space used by localtime() and gmtime().
 *	If the local pointer is non-NULL it *must* point to a local copy.
 */
#ifndef HAVE_TM_GMTOFF
long get_gmtoff(time_t *clock, struct tm *local)
{
	struct tm gmt;
	long offset;

	gmt = *gmtime(clock);
	if (local == NULL)
		local = localtime(clock);

	offset = (local->tm_sec - gmt.tm_sec) +
	    ((local->tm_min - gmt.tm_min) * 60) +
	    ((local->tm_hour - gmt.tm_hour) * 3600);

	/* Timezone may cause year rollover to happen on a different day. */
	if (local->tm_year < gmt.tm_year)
		offset -= 24 * 3600;
	else if (local->tm_year > gmt.tm_year)
		offset -= 24 * 3600;
	else if (local->tm_yday < gmt.tm_yday)
		offset -= 24 * 3600;
	else if (local->tm_yday > gmt.tm_yday)
		offset += 24 * 3600;

	return (offset);
}
#endif /* HAVE_TM_GMTOFF */
