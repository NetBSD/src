/*	$NetBSD: newsyslog.c,v 1.32 2000/07/18 15:59:25 ad Exp $	*/

/*
 * Copyright (c) 1999, 2000 Andrew Doran <ad@NetBSD.org>
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
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */

/*
 * This file contains changes from the Open Software Foundation.
 */

/*
 * Copyright 1988, 1989 by the Massachusetts Institute of Technology
 * 
 * Permission to use, copy, modify, and distribute this software
 * and its documentation for any purpose and without fee is
 * hereby granted, provided that the above copyright notice
 * appear in all copies and that both that copyright notice and
 * this permission notice appear in supporting documentation,
 * and that the names of M.I.T. and the M.I.T. S.I.P.B. not be
 * used in advertising or publicity pertaining to distribution
 * of the software without specific, written prior permission.
 * M.I.T. and the M.I.T. S.I.P.B. make no representations about
 * the suitability of this software for any purpose.  It is
 * provided "as is" without express or implied warranty.
 * 
 */

/*
 * newsyslog(8) - a program to roll over log files provided that specified
 * critera are met, optionally preserving a number of historical log files.
 */

#include <sys/cdefs.h>
#ifndef lint
__RCSID("$NetBSD: newsyslog.c,v 1.32 2000/07/18 15:59:25 ad Exp $");
#endif /* not lint */

#include <sys/types.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/param.h>

#include <ctype.h>
#include <fcntl.h>
#include <grp.h>
#include <pwd.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <errno.h>
#include <err.h>
#include <util.h>
#include <paths.h>

#include "pathnames.h"

#define	PRHDRINFO(x)	((void)(verbose ? printf x : 0))
#define	PRINFO(x)	((void)(verbose ? printf("  ") + printf x : 0))

#define	CE_COMPRESS	0x01	/* Compress the achived log files */
#define	CE_BINARY	0x02	/* Logfile is a binary file/non-syslog */
#define	CE_NOSIGNAL	0x04	/* Don't send a signal when trimmed */
#define	CE_CREATE	0x08	/* Create log file if none exists */
#define	CE_PLAIN0	0x10	/* Do not compress zero'th history file */

struct conf_entry {
	uid_t	uid;			/* Owner of log */
	gid_t	gid;			/* Group of log */
	mode_t	mode;			/* File permissions */
	int	numhist;		/* Number of historical logs to keep */
	size_t	maxsize;		/* Maximum log size */
	int	maxage;			/* Hours between log trimming */
	time_t	trimat;			/* Specific trim time */
	int	flags;			/* Flags (CE_*) */
	int	signum;			/* Signal to send */
	char	pidfile[MAXPATHLEN];	/* File containing PID to signal */
	char	logfile[MAXPATHLEN];	/* Path to log file */
};

int     verbose = 0;			/* Be verbose */
int	noaction = 0;			/* Take no action */
char    hostname[MAXHOSTNAMELEN + 1];	/* Hostname, stripped of domain */
uid_t	myeuid;				/* EUID we are running with */

int	getsig(const char *);
int	isnumber(const char *);
int	main(int, char **);
int	parse_cfgline(struct conf_entry *, FILE *, size_t *);
time_t	parse_iso8601(char *);
time_t	parse_dwm(char *);
int	parse_userspec(const char *, struct passwd **, struct group **);
pid_t	readpidfile(const char *);
void	usage(void);

void	log_compress(struct conf_entry *, const char *);
void	log_create(struct conf_entry *);
void	log_examine(struct conf_entry *, int);
void	log_trim(struct conf_entry *);
void	log_trimmed(struct conf_entry *);

/*
 * Program entry point.
 */
int
main(int argc, char **argv)
{
	struct conf_entry log;
	FILE *fd;
	char *p, *cfile;
	int c, needroot, i, force;
	size_t lineno;
	
	force = 0;
	needroot = 1;
	cfile = _PATH_NEWSYSLOGCONF;

	gethostname(hostname, sizeof (hostname));
	hostname[sizeof (hostname) - 1] = '\0';

	/* Truncate domain. */
	if ((p = strchr(hostname, '.')) != NULL)
		*p = '\0';

	/* Parse command line options. */
	while ((c = getopt(argc, argv, "f:nrvF")) != -1) {
		switch (c) {
		case 'f':
			cfile = optarg;
			break;
		case 'n':
			noaction = 1;
			verbose = 1;
			break;
		case 'r':
			needroot = 0;
			break;
		case 'v':
			verbose = 1;
			break;
		case 'F':
			force = 1;
			break;
		default:
			usage();
			/* NOTREACHED */
		}
	}

	myeuid = geteuid();
	if (needroot && myeuid != 0)
		errx(EXIT_FAILURE, "must be run as root");

	argc -= optind;
	argv += optind;
	
	if (strcmp(cfile, "-") == 0)
		fd = stdin;
	else if ((fd = fopen(cfile, "rt")) == NULL)
		err(EXIT_FAILURE, "%s", cfile);
	
	for (lineno = 0; !parse_cfgline(&log, fd, &lineno);) {
		/* 
		 * If specific log files were specified, touch only 
		 * those. 
		 */
		if (argc != 0) {
			for (i = 0; i < argc; i++)
				if (strcmp(log.logfile, argv[i]) == 0)
					break;
			if (i == argc)
				continue;
		}
		log_examine(&log, force);
	}
	
	if (fd != stdin)
		fclose(fd);

	exit(EXIT_SUCCESS);
	/* NOTREACHED */
}

/*
 * Parse a single line from the configuration file.
 */
int
parse_cfgline(struct conf_entry *log, FILE *fd, size_t *_lineno)
{
	char *line, *q, **ap, *argv[10];
	struct passwd *pw;
	struct group *gr;
	int nf, lineno, i;

	memset(log, 0, sizeof (*log));
	
	/* Place the white-space separated fields into an array. */
	if ((line = fparseln(fd, NULL, _lineno, NULL, 0)) == NULL)
		return (-1);
	lineno = (int)*_lineno;

	for (ap = argv, nf = 0; (*ap = strsep(&line, " \t")) != NULL;)
		if (**ap != '\0') {
			if (++nf == sizeof (argv) / sizeof (argv[0])) {
				warnx("config line %d: too many fields", 
				    lineno);
				return (-1);
			}
			ap++;
		}
		
	if (nf == 0)
		return (0);

	if (nf < 6)
		errx(EXIT_FAILURE, "config line %d: too few fields", lineno);
	
	/* logfile_name */
	ap = argv;
	strlcpy(log->logfile, *ap++, sizeof (log->logfile));
	if (log->logfile[0] != '/')
		errx(EXIT_FAILURE, 
		    "config line %d: logfile must have a full path", lineno);

	/* owner:group */
	if (strchr(*ap, ':') != NULL || strchr(*ap, '.') != NULL) {
		if (parse_userspec(*ap++, &pw, &gr)) {
			warnx("config line %d: unknown user/group", lineno);
			return (-1);
		}
		
		/*
		 * We may only change the file's group as non-root.
		 */
		if (myeuid != 0) {
			if (pw->pw_uid != myeuid)
				errx(EXIT_FAILURE, "config line %d: user:group "
				    "as non-root must match current user", 
				    lineno);
			log->uid = (uid_t)-1;
		} else
			log->uid = pw->pw_uid;
		log->gid = gr->gr_gid;
		if (nf < 7)
			errx(EXIT_FAILURE, "config line %d: too few fields",
			    lineno);
	} else if (myeuid != 0) {
		log->uid = (uid_t)-1;
		log->gid = getegid();
	}

	/* mode */
	if (sscanf(*ap++, "%o", &i) != 1) {
		warnx("config line %d: bad permissions", lineno);
		return (-1);
	}
	log->mode = (mode_t)i;

	/* count */
	if (sscanf(*ap++, "%d", &log->numhist) != 1) {
		warnx("config line %d: bad log count", lineno);
		return (-1);
	}

	/* size */
	if (isdigit(**ap)) {
		log->maxsize = (int)strtol(*ap, &q, 0);
		if (*q != '\0') {
			warnx("config line %d: bad log size", lineno);
			return (-1);
		}
	} else if (**ap == '*')
		log->maxsize = (size_t)-1;
	else {
		warnx("config line %d: bad log size", lineno);
		return (-1);
	}
	ap++;

	/* when */
	log->maxage = -1;
	log->trimat = (time_t)-1;
	q = *ap++;
	
	if (strcmp(q, "*") != 0) {
		if (isdigit(*q))
			log->maxage = (int)strtol(q, &q, 10);
	
		/* 
		 * One class of periodic interval specification can follow a
		 * maximum age specification.  Handle it.
		 */
		if (*q == '@') {
			log->trimat = parse_iso8601(q + 1);
			if (log->trimat == (time_t)-1) {
				warnx("config line %d: bad trim time", lineno);
				return (-1);
			}
		} else if (*q == '$') {
			if ((log->trimat = parse_dwm(q + 1)) == (time_t)-1) {
				warnx("config line %d: bad trim time", lineno);
				return (-1);
			}
		} else if (log->maxage == -1) {
			warnx("config line %d: bad log age", lineno);
			return (-1);
		}
	}
	
	/* flags */
	log->flags = 0;
	for (q = *ap++; q != NULL && *q != '\0'; q++) {
		switch (tolower(*q)) {
		case 'b':
			log->flags |= CE_BINARY;
			break;
		case 'c':
			log->flags |= CE_CREATE;
			break;
		case 'n':
			log->flags |= CE_NOSIGNAL;
			break;
		case 'p':
			log->flags |= CE_PLAIN0;
			break;
		case 'z':
			log->flags |= CE_COMPRESS;
			break;
		case '-':
			break;
		default:
			warnx("config line %d: bad flags", lineno);
			return (-1);
		}
	}

	/* path_to_pidfile */
	if (*ap != NULL && **ap == '/')
		strlcpy(log->pidfile, *ap++, sizeof (log->pidfile));
	else
		log->pidfile[0] = '\0';

	/* sigtype */
	if (*ap != NULL && (log->signum = getsig(*ap++)) < 0) {
		warnx("config line %d: bad signal type", lineno);
		return (-1);
	} else
		log->signum = SIGHUP;
	
	return (0);
}

/* 
 * Examine a log file.  If the trim conditions are met, call log_trim() to
 * trim the log file.
 */
void
log_examine(struct conf_entry *log, int force)
{
	struct stat sb;
	size_t size;
	int age, trim;
	char tmp[MAXPATHLEN], *reason;
	time_t now;

	/*
	 * XXX We get here for comment/blank lines.  I'm assuming this is
	 * intentional behaviour on the part of fparseln(). [ad]
	 */
	if (log->logfile[0] == '\0')
		return;

	now = time(NULL);

	PRHDRINFO(("\n%s <%d%s>: ", log->logfile, log->numhist, 
	    (log->flags & CE_COMPRESS) != 0 ? "Z" : ""));

	/*
	 * stat() the logfile.  If it doesn't exist and the `c' flag has
	 * been specified, create it.  If it doesn't exist and the `c' flag
	 * hasn't been specified, give up.
	 */
	if (stat(log->logfile, &sb) < 0) {
		if (errno == ENOENT && (log->flags & CE_CREATE) != 0) {
			PRHDRINFO(("creating; "));
			if (!noaction)
				log_create(log);
			else {
				PRHDRINFO(("can't proceed with `-n'\n"));
				return;
			}
			if (stat(log->logfile, &sb))
				err(EXIT_FAILURE, "%s", log->logfile);
		} else if (errno == ENOENT) {
			PRHDRINFO(("does not exist --> skip log\n"));
			return;
		} else if (errno != 0)
			err(EXIT_FAILURE, "%s", log->logfile);
	}

	/* Size of the log file in kB. */
	size = ((size_t)sb.st_blocks * S_BLKSIZE) >> 10;

	/* 
	 * Get the age (expressed in hours) of the current log file with
	 * respect to the newest historical log file.
	 */
	strlcpy(tmp, log->logfile, sizeof (tmp));
	strlcat(tmp, ".0", sizeof (tmp));
	if (stat(tmp, &sb) < 0) {
		strlcat(tmp, ".gz", sizeof (tmp));
		if (stat(tmp, &sb) < 0)
			age = -1;
		else
			age = (int)(now - sb.st_mtime + 1800) / 3600;
	} else
		age = (int)(now - sb.st_mtime + 1800) / 3600;

	/*
	 * Examine the set of given trim conditions and if any one is met,
	 * trim the log.
	 *
	 * Note: if `maxage' or `trimat' is used as a trim condition, we
	 * need at least one historical log file to determine the `age' of
	 * the active log file.  WRT `trimat', we will trim up to one hour
	 * after the specific trim time has passed - we need to know if
	 * we've trimmed to meet that condition with a previous invocation
	 * of newsyslog(8).
	 */
	if (log->maxage >= 0 && (age >= log->maxage || age < 0)) {
		trim = 1;
		reason = "log age > interval";
	} else if (size >= log->maxsize) {
		trim = 1;
		reason = "log size > size";
	} else if (log->trimat != (time_t)-1 && now >= log->trimat &&
		   age > 1 && difftime(now, log->trimat) < 60 * 60) {
		trim = 1;
		reason = "specific trim time";
	} else {
		trim = force;
		reason = "trim forced";
	}

	if (trim) {
		PRHDRINFO(("--> trim log (%s)\n", reason));
		log_trim(log);
	} else
		PRHDRINFO(("--> skip log (trim conditions not met)\n"));
}

/*
 * Trim the specified log file.
 */
void
log_trim(struct conf_entry *log)
{
	char file1[MAXPATHLEN], file2[MAXPATHLEN];
	int i;
	struct stat st;
	pid_t pid;

	/* Remove oldest historical log. */
	snprintf(file1, sizeof (file1), "%s.%d", log->logfile, log->numhist - 1);

	PRINFO(("rm -f %s\n", file1));
	if (!noaction)
		unlink(file1);
	strlcat(file1, ".gz", sizeof (file1));
	PRINFO(("rm -f %s\n", file1));
	if (!noaction)
		unlink(file1);

	/* Move down log files. */
	for (i = log->numhist - 1; i != 0; i--) {
		snprintf(file1, sizeof (file1), "%s.%d", log->logfile, i - 1);
		snprintf(file2, sizeof (file2), "%s.%d", log->logfile, i);

		if (lstat(file1, &st) != 0) {
			strlcat(file1, ".gz", sizeof (file1));
			strlcat(file2, ".gz", sizeof (file2));
			if (lstat(file1, &st) != 0)
				continue;
		}

		PRINFO(("mv %s %s\n", file1, file2));
		if (!noaction)
			if (rename(file1, file2))
				err(EXIT_FAILURE, "%s", file1);
		PRINFO(("chmod %o %s\n", log->mode, file2));
		if (!noaction)
			if (chmod(file2, log->mode))
				err(EXIT_FAILURE, "%s", file2); 
		PRINFO(("chown %d:%d %s\n", log->uid, log->gid,
		    file2));
		if (!noaction)
			if (chown(file2, log->uid, log->gid))
				err(EXIT_FAILURE, "%s", file2);
	}

	/*
	 * If a historical log file isn't compressed, and 'z' has been
	 * specified, compress it.  (This is convenient, but is also needed
	 * if 'p' has been specified.)  It should be noted that gzip(1)
	 * preserves file ownership and file mode.
	 */
	for (i = (log->flags & CE_PLAIN0) != 0; i < log->numhist; i++) {
		snprintf(file1, sizeof (file1), "%s.%d", log->logfile, i);
		if (lstat(file1, &st) != 0)
			continue;
		snprintf(file2, sizeof (file2), "%s.gz", file1);
		if (lstat(file2, &st) == 0)
			continue;
		log_compress(log, file1);
	}

	log_trimmed(log);

	/* Create the historical log file if we're maintaining history. */
	if (log->numhist == 0) {
		PRINFO(("rm -f %s\n", log->logfile));
		if (!noaction)
			if (unlink(log->logfile))
				err(EXIT_FAILURE, "%s", log->logfile);
	} else {
		snprintf(file1, sizeof (file1), "%s.0", log->logfile);
		PRINFO(("mv %s %s\n", log->logfile, file1));
		if (!noaction)
			if (rename(log->logfile, file1))
				err(EXIT_FAILURE, "%s", log->logfile);
	}

	PRINFO(("(create new log)\n"));
	log_create(log);
	log_trimmed(log);

	/* Set the correct permissions on the log. */
	PRINFO(("chmod %o %s\n", log->mode, log->logfile));
	if (!noaction)
		if (chmod(log->logfile, log->mode))
			err(EXIT_FAILURE, "%s", log->logfile);

	/* Do we need to signal a daemon? */
	if ((log->flags & CE_NOSIGNAL) == 0) {
		if (log->pidfile[0] != '\0')
			pid = readpidfile(log->pidfile);
		else
			pid = readpidfile(_PATH_SYSLOGDPID);
		
		if (pid != (pid_t)-1) {
			PRINFO(("kill -%s %lu\n", 
			    sys_signame[log->signum], (u_long)pid));
			if (!noaction)
				if (kill(pid, log->signum))
					warn("kill");
		}
	}

	/* If the newest historical log is to be compressed, do it here. */
	if ((log->flags & (CE_PLAIN0 | CE_COMPRESS)) == CE_COMPRESS) {
		snprintf(file1, sizeof (file1), "%s.0", log->logfile);
		log_compress(log, file1);
	}
}

/* 
 * Write an entry to the log file recording the fact that it was trimmed.
 */
void
log_trimmed(struct conf_entry *log)
{
	FILE *fd;
	time_t now;
	char *daytime;

	if ((log->flags & CE_BINARY) != 0)
		return;
	PRINFO(("(append rotation notice to %s)\n", log->logfile));
	if (noaction)
		return;

	if ((fd = fopen(log->logfile, "at")) == NULL)
		err(EXIT_FAILURE, "%s", log->logfile);

	now = time(NULL);
	daytime = ctime(&now) + 4;
	daytime[15] = '\0';
		
	fprintf(fd, "%s %s newsyslog[%lu]: log file turned over\n", daytime, 
	    hostname, (u_long)getpid());
	fclose(fd);
}

/*
 * Create a new log file.
 */
void
log_create(struct conf_entry *log)
{
	int fd;

	if (noaction)
		return;

	if ((fd = creat(log->logfile, log->mode)) < 0)
		err(EXIT_FAILURE, "%s", log->logfile);
	if (fchown(fd, log->uid, log->gid) < 0)
		err(EXIT_FAILURE, "%s", log->logfile);
	close(fd);
}

/*
 * Fork off gzip(1) to compress a log file.  This routine takes an
 * additional string argument (the name of the file to compress): it is also
 * used to compress historical log files other than the newest.
 */
void
log_compress(struct conf_entry *log, const char *fn)
{
	char tmp[MAXPATHLEN];
	pid_t pid;

	PRINFO(("gzip %s\n", fn));
	if (!noaction) {
		if ((pid = fork()) < 0)
			err(EXIT_FAILURE, "fork");
		else if (pid == 0) {
			execl(_PATH_GZIP, "gzip", "-f", fn, NULL);
			err(EXIT_FAILURE, _PATH_GZIP);
		}
	}

	snprintf(tmp, sizeof (tmp), "%s.gz", fn);
	PRINFO(("chown %d:%d %s\n", log->uid, log->gid, tmp));
	if (!noaction)
		if (!chown(tmp, log->uid, log->gid))
			err(EXIT_FAILURE, "%s", tmp);
}

/*
 * Display program usage information.
 */
void
usage(void)
{

	fprintf(stderr, 
	    "usage: newsyslog [-nrvF] [-f config-file] [file ...]\n");
	exit(EXIT_FAILURE);
}

/*
 * Return non-zero if a string represents a decimal value.
 */
int
isnumber(const char *string)
{

	while (isdigit(*string))
		string++;

	return (*string == '\0');
}

/* 
 * Given a signal name, attempt to find the corresponding signal number.
 */
int
getsig(const char *sig)
{
	char *p;
	int n;
	
	if (isnumber(sig)) {
		n = (int)strtol(sig, &p, 0);
		if (p != '\0' || n < 0 || n >= NSIG)
			return (-1);
		return (n);
	}

	if (strncasecmp(sig, "SIG", 3) == 0)
		sig += 3;
	for (n = 1; n < NSIG; n++)
		if (strcasecmp(sys_signame[n], sig) == 0)
			return (n);
	return (-1);
}

/*
 * Given a path to a PID file, return the PID contained within.
 */
pid_t
readpidfile(const char *file)
{
	FILE *fd;
	char line[BUFSIZ];
	pid_t pid;

#ifdef notyet
	if (file[0] != '/')
		snprintf(tmp, sizeof (tmp), "%s%s", _PATH_VARRUN, file);
	else
		strlcpy(tmp, file, sizeof (tmp));
#endif

	if ((fd = fopen(file, "rt")) == NULL) {
		warn("%s", file);
		return (-1);
	}
	
	if (fgets(line, sizeof (line) - 1, fd) != NULL) {
		line[sizeof (line) - 1] = '\0';
		pid = (pid_t)strtol(line, NULL, 0);
	} else {
		warnx("unable to read %s", file);
		pid = (pid_t)-1;
	}

	fclose(fd);
	return (pid);
}

/*
 * Parse a user:group specification.
 *
 * XXX This is over the top for newsyslog(8).  It should be moved to libutil.
 */
int
parse_userspec(const char *name, struct passwd **pw, struct group **gr)
{
	char buf[MAXLOGNAME * 2 + 2], *group;

	strlcpy(buf, name, sizeof (buf));
	*gr = NULL;

	/* 
	 * Before attempting to use '.' as a separator, see if the whole 
	 * string resolves as a user name.
	 */
	if ((*pw = getpwnam(buf)) != NULL) {
		*gr = getgrgid((*pw)->pw_gid);
		return (0);
	}
	
	/* Split the user and group name. */
	if ((group = strchr(buf, ':')) != NULL || 
	    (group = strchr(buf, '.')) != NULL)
		*group++ = '\0';

	if (isnumber(buf))
		*pw = getpwuid((uid_t)atoi(buf));
	else
		*pw = getpwnam(buf);
		
	/* 
	 * Find the group.  If a group wasn't specified, use the user's
	 * `natural' group.  We get to this point even if no user was found.
	 * This is to allow the caller to get a better idea of what went
	 * wrong, if anything.
	 */
	if (group == NULL || *group == '\0') {
		if (*pw == NULL)
			return (-1);
		*gr = getgrgid((*pw)->pw_gid);
	} else if (isnumber(group))
		*gr = getgrgid((gid_t)atoi(group));
	else
		*gr = getgrnam(group);
		
	return (*pw != NULL && *gr != NULL ? 0 : -1);
}

/*
 * Parse a cyclic time specification, the format is as follows:
 *
 *	[Dhh] or [Wd[Dhh]] or [Mdd[Dhh]]
 *
 * to rotate a log file cyclic at
 *
 *	- every day (D) within a specific hour (hh)	(hh = 0...23)
 *	- once a week (W) at a specific day (d)     OR	(d = 0..6, 0 = Sunday)
 *	- once a month (M) at a specific day (d)	(d = 1..31,l|L)
 *
 * We don't accept a timezone specification; missing fields are defaulted to
 * the current date but time zero.
 */
time_t
parse_dwm(char *s)
{
	char *t;
	struct tm tm, *tmp;
	u_long ul;
	time_t now;
	static int mtab[] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
	int wmseen, dseen, nd, save;
	
	wmseen = 0;
	dseen = 0;

	now = time(NULL);
	tmp = localtime(&now);
	tm = *tmp;

	/* Set no. of days per month */
	nd = mtab[tm.tm_mon];

	if (tm.tm_mon == 1 &&
	    ((tm.tm_year + 1900) % 4 == 0) &&
	    ((tm.tm_year + 1900) % 100 != 0) &&
	    ((tm.tm_year + 1900) % 400 == 0))
		nd++;	/* leap year, 29 days in february */
	tm.tm_hour = tm.tm_min = tm.tm_sec = 0;

	for (;;) {
		switch (*s) {
		case 'D':
			if (dseen)
				return ((time_t)-1);
			dseen++;
			s++;
			ul = strtoul(s, &t, 10);
			if (ul < 0 || ul > 23)
				return ((time_t)-1);
			tm.tm_hour = ul;
			break;

		case 'W':
			if (wmseen)
				return (-1);
			wmseen++;
			s++;
			ul = strtoul(s, &t, 10);
			if (ul < 0 || ul > 6)
				return (-1);
			if (ul != tm.tm_wday) {
				if (ul < tm.tm_wday) {
					save = 6 - tm.tm_wday;
					save += (ul + 1);
				} else
					save = ul - tm.tm_wday;
				tm.tm_mday += save;

				if (tm.tm_mday > nd) {
					tm.tm_mon++;
					tm.tm_mday = tm.tm_mday - nd;
				}
			}
			break;

		case 'M':
			if (wmseen)
				return (-1);
			wmseen++;
			s++;
			if (tolower(*s) == 'l') {
				tm.tm_mday = nd;
				s++;
				t = s;
			} else {
				ul = strtoul(s, &t, 10);
				if (ul < 1 || ul > 31)
					return (-1);

				if (ul > nd)
					return (-1);
				tm.tm_mday = ul;
			}
			break;

		default:
			return (-1);
			break;
		}

		if (*t == '\0' || isspace(*t))
			break;
		else
			s = t;
	}
	
	return (mktime(&tm));
}

/*
 * Parse a limited subset of ISO 8601.  The specific format is as follows:
 *
 * [CC[YY[MM[DD]]]][THH[MM[SS]]]	(where `T' is the literal letter)
 *
 * We don't accept a timezone specification; missing fields (including
 * timezone) are defaulted to the current date but time zero.
 */
time_t
parse_iso8601(char *s)
{
	char *t;
	struct tm tm, *tmp;
	u_long ul;
	time_t now;

	now = time(NULL);
	tmp = localtime(&now);
	tm = *tmp;

	tm.tm_hour = tm.tm_min = tm.tm_sec = 0;

	ul = strtoul(s, &t, 10);
	if (*t != '\0' && *t != 'T')
		return ((time_t)-1);

	/*
	 * Now t points either to the end of the string (if no time was
	 * provided) or to the letter `T' which separates date and time in
	 * ISO 8601.  The pointer arithmetic is the same for either case.
	 */
	switch (t - s) {
	case 8:
		tm.tm_year = ((ul / 1000000) - 19) * 100;
		ul = ul % 1000000;
		/* FALLTHROUGH */
	case 6:
		tm.tm_year = tm.tm_year - (tm.tm_year % 100);
		tm.tm_year += ul / 10000;
		ul = ul % 10000;
		/* FALLTHROUGH */
	case 4:
		tm.tm_mon = (ul / 100) - 1;
		ul = ul % 100;
		/* FALLTHROUGH */
	case 2:
		tm.tm_mday = ul;
		/* FALLTHROUGH */
	case 0:
		break;
	default:
		return ((time_t)-1);
	}

	/* Sanity check */
	if (tm.tm_year < 70 || tm.tm_mon < 0 || tm.tm_mon > 12 ||
	    tm.tm_mday < 1 || tm.tm_mday > 31)
		return ((time_t)-1);

	if (*t != '\0') {
		s = ++t;
		ul = strtoul(s, &t, 10);
		if (*t != '\0' && !isspace(*t))
			return ((time_t)-1);

		switch (t - s) {
		case 6:
			tm.tm_sec = ul % 100;
			ul /= 100;
			/* FALLTHROUGH */
		case 4:
			tm.tm_min = ul % 100;
			ul /= 100;
			/* FALLTHROUGH */
		case 2:
			tm.tm_hour = ul;
			/* FALLTHROUGH */
		case 0:
			break;
		default:
			return ((time_t)-1);
		}

		/* Sanity check */
		if (tm.tm_sec < 0 || tm.tm_sec > 60 || tm.tm_min < 0 ||
		    tm.tm_min > 59 || tm.tm_hour < 0 || tm.tm_hour > 23)
			return ((time_t)-1);
	}
	
	return (mktime(&tm));
}
