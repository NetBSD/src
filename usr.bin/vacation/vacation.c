/*	$NetBSD: vacation.c,v 1.32.2.1 2007/07/16 09:58:48 liamjfoy Exp $	*/

/*
 * Copyright (c) 1983, 1987, 1993
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
__COPYRIGHT("@(#) Copyright (c) 1983, 1987, 1993\n\
	The Regents of the University of California.  All rights reserved.\n");
#endif /* not lint */

#ifndef lint
#if 0
static char sccsid[] = "@(#)vacation.c	8.2 (Berkeley) 1/26/94";
#endif
__RCSID("$NetBSD: vacation.c,v 1.32.2.1 2007/07/16 09:58:48 liamjfoy Exp $");
#endif /* not lint */

/*
**  Vacation
**  Copyright (c) 1983  Eric P. Allman
**  Berkeley, California
*/

#include <sys/param.h>
#include <sys/stat.h>

#include <ctype.h>
#include <db.h>
#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <paths.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <time.h>
#include <tzfile.h>
#include <unistd.h>

/*
 *  VACATION -- return a message to the sender when on vacation.
 *
 *	This program is invoked as a message receiver.  It returns a
 *	message specified by the user to whomever sent the mail, taking
 *	care not to return a message too often to prevent "I am on
 *	vacation" loops.
 */

#define	MAXLINE	1024			/* max line from mail header */

static const char *dbprefix = ".vacation";	/* dbm's database sans .db */
static const char *msgfile = ".vacation.msg";	/* vacation message */

typedef struct alias {
	struct alias *next;
	const char *name;
} alias_t;
static alias_t *names;

static DB *db;
static char from[MAXLINE];
static char subject[MAXLINE];

static int iflag = 0;		/* Initialize the database */

static int tflag = 0;
#define	APPARENTLY_TO		1
#define	DELIVERED_TO		2

static int fflag = 0;
#define	FROM_FROM		1
#define	RETURN_PATH_FROM	2
#define	SENDER_FROM		4

static int toanybody = 0;	/* Don't check if we appear in the to or cc */

static int debug = 0;

int main(int, char **);
static void opendb(void);
static int junkmail(const char *);
static int nsearch(const char *, const char *);
static int readheaders(void);
static int recent(void);
static void getfrom(char *);
static void sendmessage(const char *);
static void setinterval(time_t);
static void setreply(void);
static void usage(void);

int
main(int argc, char **argv)
{
	struct passwd *pw;
	alias_t *cur;
	long interval;
	int ch, rv;
	char *p;

	setprogname(argv[0]);
	opterr = 0;
	interval = -1;
	openlog(getprogname(), 0, LOG_USER);
	while ((ch = getopt(argc, argv, "a:df:F:Iijr:s:t:T:")) != -1)
		switch((char)ch) {
		case 'a':			/* alias */
			if (!(cur = (alias_t *)malloc((size_t)sizeof(alias_t))))
				break;
			cur->name = optarg;
			cur->next = names;
			names = cur;
			break;
		case 'd':
			debug++;
			break;
		case 'F':
			for (p = optarg; *p; p++)
				switch (*p) {
				case 'F':
					fflag |= FROM_FROM;
					break;
				case 'R':
					fflag |= RETURN_PATH_FROM;
					break;
				case 'S':
					fflag |= SENDER_FROM;
					break;
				default:
					errx(1, "Unknown -f option `%c'", *p);
				}
			break;
		case 'f':
			dbprefix = optarg;
			break;
		case 'I':			/* backward compatible */
		case 'i':			/* init the database */
			iflag = 1;
			break;
		case 'j':
			toanybody = 1;
			break;
		case 'm':
			msgfile = optarg;
			break;
		case 'r':
		case 't':	/* Solaris compatibility */
			if (!isdigit((unsigned char)*optarg)) {
				interval = LONG_MAX;
				break;
			}
			if (*optarg == '\0')
				goto bad;
			interval = strtol(optarg, &p, 0);
			if (errno == ERANGE &&
			    (interval == LONG_MAX || interval == LONG_MIN))
				err(1, "Bad interval `%s'", optarg);
			switch (*p) {
			case 's':
				break;
			case 'm':
				interval *= SECSPERMIN;
				break;
			case 'h':
				interval *= SECSPERHOUR;
				break;
			case 'd':
			case '\0':
				interval *= SECSPERDAY;
				break;
			case 'w':
				interval *= DAYSPERWEEK * SECSPERDAY;
				break;
			default:
			bad:
				errx(1, "Invalid interval `%s'", optarg);
			}
			if (interval < 0 || (*p && p[1]))
				goto bad;
			break;
		case 's':
			(void)strlcpy(from, optarg, sizeof(from));
			break;
		case 'T':
			for (p = optarg; *p; p++)
				switch (*p) {
				case 'A':
					tflag |= APPARENTLY_TO;
					break;
				case 'D':
					tflag |= DELIVERED_TO;
					break;
				default:
					errx(1, "Unknown -t option `%c'", *p);
				}
			break;
		case '?':
		default:
			usage();
		}
	argc -= optind;
	argv += optind;

	if (argc != 1) {
		if (!iflag)
			usage();
		if (!(pw = getpwuid(getuid()))) {
			syslog(LOG_ERR, "%s: no such user uid %u.",
			    getprogname(), getuid());
			exit(1);
		}
	}
	else if (!(pw = getpwnam(*argv))) {
		syslog(LOG_ERR, "%s: no such user %s.",
		    getprogname(), *argv);
		exit(1);
	}
	if (chdir(pw->pw_dir) == -1 && 
	    (dbprefix[0] != '/' || msgfile[0] != '/')) {
		syslog(LOG_ERR, "%s: no such directory %s.",
		    getprogname(), pw->pw_dir);
		exit(1);
	}

	opendb();

	if (interval != -1)
		setinterval((time_t)interval);

	if (iflag) {
		(void)(db->close)(db);
		exit(0);
	}

	if (!(cur = malloc((size_t)sizeof(alias_t)))) {
		syslog(LOG_ERR, "%s: %m", getprogname());
		(void)(db->close)(db);
		exit(1);
	}
	cur->name = pw->pw_name;
	cur->next = names;
	names = cur;

	if ((rv = readheaders()) != -1) {
		(void)(db->close)(db);
		exit(rv);
	}
		
	if (!recent()) {
		setreply();
		(void)(db->close)(db);
		sendmessage(pw->pw_name);
	}
	else
		(void)(db->close)(db);
	exit(0);
	/* NOTREACHED */
}

static void
opendb(void)
{
	char path[MAXPATHLEN];

	(void)snprintf(path, sizeof(path), "%s.db", dbprefix);
	db = dbopen(path, O_CREAT|O_RDWR | (iflag ? O_TRUNC : 0),
	    S_IRUSR|S_IWUSR, DB_HASH, NULL);

	if (!db) {
		syslog(LOG_ERR, "%s: %s: %m", getprogname(), path);
		exit(1);
	}
}

/*
 * readheaders --
 *	read mail headers
 */
static int
readheaders(void)
{
	alias_t *cur;
	char *p;
	int tome, cont;
	char buf[MAXLINE];

	cont = tome = 0;
#define COMPARE(a, b)		strncmp(a, b, sizeof(b) - 1)
#define CASECOMPARE(a, b)	strncasecmp(a, b, sizeof(b) - 1)
	while (fgets(buf, sizeof(buf), stdin) && *buf != '\n')
		switch(*buf) {
		case 'F':		/* "From " or "From:" */
			cont = 0;
			if (COMPARE(buf, "From ") == 0)
				getfrom(buf + sizeof("From ") - 1);
			if ((fflag & FROM_FROM) != 0 &&
			    COMPARE(buf, "From:") == 0)
				getfrom(buf + sizeof("From:") - 1);
			break;
		case 'P':		/* "Precedence:" */
			cont = 0;
			if (CASECOMPARE(buf, "Precedence") != 0 ||
			    (buf[10] != ':' && buf[10] != ' ' &&
			    buf[10] != '\t'))
				break;
			if ((p = strchr(buf, ':')) == NULL)
				break;
			while (*++p && isspace((unsigned char)*p))
				continue;
			if (!*p)
				break;
			if (CASECOMPARE(p, "junk") == 0 ||
			    CASECOMPARE(p, "bulk") == 0||
			    CASECOMPARE(p, "list") == 0)
				exit(0);
			break;
		case 'C':		/* "Cc:" */
			if (COMPARE(buf, "Cc:"))
				break;
			cont = 1;
			goto findme;
		case 'T':		/* "To:" */
			if (COMPARE(buf, "To:"))
				break;
			cont = 1;
			goto findme;
		case 'A':		/* "Apparently-To:" */
			if ((tflag & APPARENTLY_TO) == 0 ||
			    COMPARE(buf, "Apparently-To:") != 0)
				break;
			cont = 1;
			goto findme;
		case 'D':		/* "Delivered-To:" */
			if ((tflag & DELIVERED_TO) == 0 ||
			    COMPARE(buf, "Delivered-To:") != 0)
				break;
			cont = 1;
			goto findme;
		case 'R':		/* "Return-Path:" */
			cont = 0;
			if ((fflag & RETURN_PATH_FROM) != 0 &&
			    COMPARE(buf, "Return-Path:") == 0)
				getfrom(buf + sizeof("Return-Path:") - 1);
			break;
		case 'S':		/* "Sender:" */
			cont = 0;
			if (COMPARE(buf, "Subject:") == 0) {
				/* trim leading blanks */
				char *s = NULL;
				for (p = buf + sizeof("Subject:") - 1; *p; p++)
					if (s == NULL &&
					    !isspace((unsigned char)*p))
						s = p;
				/* trim trailing blanks */
				if (s) {
					for (--p; p != s; p--)
						if (!isspace((unsigned char)*p))
							break;
					*++p = '\0';
				}
				if (s) {
					(void)strlcpy(subject, s, sizeof(subject));
				} else {
					subject[0] = '\0';
				}
			}
			if ((fflag & SENDER_FROM) != 0 &&
			    COMPARE(buf, "Sender:") == 0)
				getfrom(buf + sizeof("Sender:") - 1);
			break;
		default:
			if (!isspace((unsigned char)*buf) || !cont || tome) {
				cont = 0;
				break;
			}
findme:			for (cur = names; !tome && cur; cur = cur->next)
				tome += nsearch(cur->name, buf);
		}
	if (!toanybody && !tome)
		return 0;
	if (!*from) {
		syslog(LOG_ERR, "%s: no initial \"From\" line.",
			getprogname());
		return 1;
	}
	return -1;
}

/*
 * nsearch --
 *	do a nice, slow, search of a string for a substring.
 */
static int
nsearch(const char *name, const char *str)
{
	size_t len;

	for (len = strlen(name); *str; ++str)
		if (!strncasecmp(name, str, len))
			return(1);
	return(0);
}

/*
 * getfrom --
 *	return the first string in the buffer, stripping leading and trailing
 *	blanks and <>.
 */
void
getfrom(char *buf)
{
	char *s, *p;

	if ((s = strchr(buf, '<')) != NULL)
		s++;
	else
		s = buf;

	for (; *s && isspace((unsigned char)*s); s++)
		continue;
	for (p = s; *p && !isspace((unsigned char)*p); p++)
		continue;

	if (*--p == '>')
		*p = '\0';
	else
		*++p = '\0';

	if (junkmail(s))
		exit(0);

	if (!*from)
		(void)strlcpy(from, s, sizeof(from));
}

/*
 * junkmail --
 *	read the header and return if automagic/junk/bulk/list mail
 */
static int
junkmail(const char *addr)
{
	static struct ignore {
		const char *name;
		size_t	len;
	} ignore[] = {
#define INIT(a) { a, sizeof(a) - 1 }
		INIT("-request"),
		INIT("postmaster"),
		INIT("uucp"),
		INIT("mailer-daemon"),
		INIT("mailer"),
		INIT("-relay"),
		{NULL, 0 }
	};
	struct ignore *cur;
	size_t len;
	const char *p;

	/*
	 * This is mildly amusing, and I'm not positive it's right; trying
	 * to find the "real" name of the sender, assuming that addresses
	 * will be some variant of:
	 *
	 * From site!site!SENDER%site.domain%site.domain@site.domain
	 */
	if (!(p = strchr(addr, '%')))
		if (!(p = strchr(addr, '@'))) {
			if ((p = strrchr(addr, '!')) != NULL)
				++p;
			else
				p = addr;
			for (; *p; ++p)
				continue;
		}
	len = p - addr;
	for (cur = ignore; cur->name; ++cur)
		if (len >= cur->len &&
		    !strncasecmp(cur->name, p - cur->len, cur->len))
			return(1);
	return(0);
}

#define	VIT	"__VACATION__INTERVAL__TIMER__"

/*
 * recent --
 *	find out if user has gotten a vacation message recently.
 *	use memmove for machines with alignment restrictions
 */
static int
recent(void)
{
	DBT key, data;
	time_t then, next;

	/* get interval time */
	key.data = (void *)(intptr_t)VIT;
	key.size = sizeof(VIT);
	if ((db->get)(db, &key, &data, 0))
		next = SECSPERDAY * DAYSPERWEEK;
	else
		(void)memmove(&next, data.data, sizeof(next));

	/* get record for this address */
	key.data = from;
	key.size = strlen(from);
	if (!(db->get)(db, &key, &data, 0)) {
		(void)memmove(&then, data.data, sizeof(then));
		if (next == (time_t)LONG_MAX ||			/* XXX */
		    then + next > time(NULL))
			return(1);
	}
	return(0);
}

/*
 * setinterval --
 *	store the reply interval
 */
static void
setinterval(time_t interval)
{
	DBT key, data;

	key.data = (void *)(intptr_t)VIT;
	key.size = sizeof(VIT);
	data.data = &interval;
	data.size = sizeof(interval);
	(void)(db->put)(db, &key, &data, 0);
}

/*
 * setreply --
 *	store that this user knows about the vacation.
 */
static void
setreply(void)
{
	DBT key, data;
	time_t now;

	key.data = from;
	key.size = strlen(from);
	(void)time(&now);
	data.data = &now;
	data.size = sizeof(now);
	(void)(db->put)(db, &key, &data, 0);
}

/*
 * sendmessage --
 *	exec sendmail to send the vacation file to sender
 */
static void
sendmessage(const char *myname)
{
	FILE *mfp, *sfp;
	int i;
	int pvect[2];
	char buf[MAXLINE];

	mfp = fopen(msgfile, "r");
	if (mfp == NULL) {
		syslog(LOG_ERR, "%s: no `%s' file for `%s'.", getprogname(),
		    myname, msgfile);
		exit(1);
	}

	if (debug) {
		sfp = stdout;
	} else {
		if (pipe(pvect) < 0) {
			syslog(LOG_ERR, "%s: pipe: %m", getprogname());
			exit(1);
		}
		i = vfork();
		if (i < 0) {
			syslog(LOG_ERR, "%s: fork: %m", getprogname());
			exit(1);
		}
		if (i == 0) {
			(void)dup2(pvect[0], 0);
			(void)close(pvect[0]);
			(void)close(pvect[1]);
			(void)close(fileno(mfp));
			(void)execl(_PATH_SENDMAIL, "sendmail", "-f", myname,
			    "--", from, NULL);
			syslog(LOG_ERR, "%s: can't exec %s: %m",
			    getprogname(), _PATH_SENDMAIL);
			_exit(1);
		}
		(void)close(pvect[0]);
		sfp = fdopen(pvect[1], "w");
		if (sfp == NULL) {
			syslog(LOG_ERR, "%s: can't fdopen %d: %m",
			    getprogname(), pvect[1]);
			_exit(1);
		}
	} 
	(void)fprintf(sfp, "To: %s\n", from);
	(void)fputs("Auto-Submitted: auto-replied\n", sfp);
	while (fgets(buf, sizeof buf, mfp) != NULL) {
		char *p;
		if ((p = strstr(buf, "$SUBJECT")) != NULL) {
			*p = '\0';
			(void)fputs(buf, sfp);
			(void)fputs(subject, sfp);
			p += sizeof("$SUBJECT") - 1;
			(void)fputs(p, sfp);
		} else
		    (void)fputs(buf, sfp);
	}
	(void)fclose(mfp);
	if (sfp != stdout)
		(void)fclose(sfp);
}

static void
usage(void)
{

	syslog(LOG_ERR, "uid %u: Usage: %s [-dIij] [-a alias] [-f database_file] [-F F|R|S] [-m message_file] [-s sender] [-t interval] [-T A|D]"
	    " login", getuid(), getprogname());
	exit(1);
}
