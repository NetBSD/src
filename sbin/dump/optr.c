/*	$NetBSD: optr.c,v 1.38.2.3 2014/08/20 00:02:24 tls Exp $	*/

/*-
 * Copyright (c) 1980, 1988, 1993
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
static char sccsid[] = "@(#)optr.c	8.2 (Berkeley) 1/6/94";
#else
__RCSID("$NetBSD: optr.c,v 1.38.2.3 2014/08/20 00:02:24 tls Exp $");
#endif
#endif /* not lint */

#include <sys/param.h>
#include <sys/queue.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <sys/ucred.h>
#include <sys/mount.h>

#include <errno.h>
#include <fstab.h>
#include <grp.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <time.h>
#include <tzfile.h>
#include <unistd.h>
#include <util.h>

#include "dump.h"
#include "pathnames.h"

extern  char *time_string;
extern  char default_time_string[];

static void alarmcatch(int);
static struct fstab *allocfsent(const struct fstab *);
static int datesort(const void *, const void *);
static void do_timestamp(time_t, const char *);

/*
 *	Query the operator; This previously-fascist piece of code
 *	no longer requires an exact response.
 *	It is intended to protect dump aborting by inquisitive
 *	people banging on the console terminal to see what is
 *	happening which might cause dump to croak, destroying
 *	a large number of hours of work.
 *
 *	Every 2 minutes we reprint the message, alerting others
 *	that dump needs attention.
 */
static	int timeout;
static	const char *attnmessage;		/* attention message */

int
query(const char *question)
{
	char	replybuffer[64];
	int	back, errcount;
	FILE	*mytty;
	time_t	firstprompt, when_answered;

	firstprompt = time((time_t *)0);

	if ((mytty = fopen(_PATH_TTY, "r")) == NULL)
		quit("fopen on %s fails: %s\n", _PATH_TTY, strerror(errno));
	attnmessage = question;
	timeout = 0;
	alarmcatch(0);
	back = -1;
	errcount = 0;
	do {
		if (fgets(replybuffer, 63, mytty) == NULL) {
			clearerr(mytty);
			if (++errcount > 30)	/* XXX	ugly */
				quit("excessive operator query failures\n");
		} else if (replybuffer[0] == 'y' || replybuffer[0] == 'Y') {
			back = 1;
		} else if (replybuffer[0] == 'n' || replybuffer[0] == 'N') {
			back = 0;
		} else {
			(void) fprintf(stderr,
			    "  DUMP: \"Yes\" or \"No\"?\n");
			(void) fprintf(stderr,
			    "  DUMP: %s: (\"yes\" or \"no\") ", question);
		}
	} while (back < 0);

	/*
	 *	Turn off the alarm, and reset the signal to trap out..
	 */
	(void) alarm(0);
	if (signal(SIGALRM, sig) == SIG_IGN)
		signal(SIGALRM, SIG_IGN);
	(void) fclose(mytty);
	when_answered = time((time_t *)0);
	/*
	 * Adjust the base for time estimates to ignore time we spent waiting
	 * for operator input.
	 */
	if (tstart_writing != 0)
		tstart_writing += (when_answered - firstprompt);
	if (tstart_volume != 0)
		tstart_volume += (when_answered - firstprompt);
	return(back);
}

char lastmsg[200];

/*
 *	Alert the console operator, and enable the alarm clock to
 *	sleep for 2 minutes in case nobody comes to satisfy dump
 */
static void
alarmcatch(int dummy __unused)
{

	if (notify == 0) {
		if (timeout == 0)
			(void) fprintf(stderr,
			    "  DUMP: %s: (\"yes\" or \"no\") ",
			    attnmessage);
		else
			msgtail("\a\a");
	} else {
		if (timeout) {
			msgtail("\n");
			broadcast("");		/* just print last msg */
		}
		(void) fprintf(stderr,"  DUMP: %s: (\"yes\" or \"no\") ",
		    attnmessage);
	}
	signal(SIGALRM, alarmcatch);
	(void) alarm(120);
	timeout = 1;
}

/*
 *	Here if an inquisitive operator interrupts the dump program
 */
void
interrupt(int signo __unused)
{
	int errno_save;

	errno_save = errno;
	msg("Interrupt received.\n");
	if (query("Do you want to abort dump?"))
		dumpabort(0);
	errno = errno_save;
}

/*
 *	Use wall(1) "-g operator" to do the actual broadcasting.
 */
void
broadcast(const char *message)
{
	FILE	*fp;
	char	buf[sizeof(_PATH_WALL) + sizeof(OPGRENT) + 3];

	if (!notify)
		return;

	(void)snprintf(buf, sizeof(buf), "%s -g %s", _PATH_WALL, OPGRENT);
	if ((fp = popen(buf, "w")) == NULL)
		return;

	(void) fputs("\a\a\aMessage from the dump program to all operators\n\nDUMP: NEEDS ATTENTION: ", fp);
	if (lastmsg[0])
		(void) fputs(lastmsg, fp);
	if (message[0])
		(void) fputs(message, fp);

	(void) pclose(fp);
}

/*
 *	print out the timestamp string to stderr.
 */
#define STAMP_LENGTH 80

static void
do_timestamp(time_t thistime, const char *message)
{
	struct tm tm_time;
	char then[STAMP_LENGTH + 1];

	(void) localtime_r(&thistime, &tm_time);
	if (strftime(then, STAMP_LENGTH, time_string, &tm_time) == 0) {
		time_string = default_time_string;
		strftime(then, STAMP_LENGTH, time_string, &tm_time);
		fprintf(stderr,
		   "DUMP: ERROR: TIMEFORMAT too long, reverting to default\n");
	}

	fprintf(stderr, message, then);
}


/*
 *	print out an estimate of the amount of time left to do the dump
 */

time_t	tschedule = 0;

void
timeest(void)
{
	time_t	tnow, deltat;

	(void) time((time_t *) &tnow);
	if (tnow >= tschedule) {
		tschedule = tnow + 300;
		if (blockswritten < 500)
			return;
		deltat = tstart_writing - tnow +
			(1.0 * (tnow - tstart_writing))
			/ blockswritten * tapesize;

		msg("%3.2f%% done, finished in %ld:%02ld",
		    (blockswritten * 100.0) / tapesize,
		    (long)(deltat / 3600), (long)((deltat % 3600) / 60));

		if (timestamp == 1)
			do_timestamp(tnow + deltat, " (at %s)");

		fprintf(stderr, "\n");
	}
}

void
msg(const char *fmt, ...)
{
	time_t  tnow;
	va_list ap;

	fprintf(stderr, "  ");
	if (timestamp == 1) {
		(void) time((time_t *) &tnow);
		do_timestamp(tnow, "[%s] ");
	}

	(void) fprintf(stderr,"DUMP: ");
#ifdef TDEBUG
	(void) fprintf(stderr, "pid=%d ", getpid());
#endif
	va_start(ap, fmt);
	(void) vsnprintf(lastmsg, sizeof lastmsg, fmt, ap);
	fputs(lastmsg, stderr);
	(void) fflush(stdout);
	(void) fflush(stderr);
	va_end(ap);
}

void
msgtail(const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	(void) vfprintf(stderr, fmt, ap);
	va_end(ap);
}

void
quit(const char *fmt, ...)
{
	va_list ap;

	(void) fprintf(stderr,"  DUMP: ");
#ifdef TDEBUG
	(void) fprintf(stderr, "pid=%d ", getpid());
#endif
	va_start(ap, fmt);
	(void) vfprintf(stderr, fmt, ap);
	va_end(ap);
	(void) fflush(stdout);
	(void) fflush(stderr);
	dumpabort(0);
}

/*
 *	Tell the operator what has to be done;
 *	we don't actually do it
 */

static struct fstab *
allocfsent(const struct fstab *fs)
{
	struct fstab *new;

	new = xmalloc(sizeof (*fs));
	new->fs_file = xstrdup(fs->fs_file);
	new->fs_type = xstrdup(fs->fs_type);
	new->fs_spec = xstrdup(fs->fs_spec);
	new->fs_passno = fs->fs_passno;
	new->fs_freq = fs->fs_freq;
	return (new);
}

struct	pfstab {
	SLIST_ENTRY(pfstab) pf_list;
	struct	fstab *pf_fstab;
};

static	SLIST_HEAD(, pfstab) table;

void
getfstab(void)
{
	struct fstab *fs;
	struct pfstab *pf;

	if (setfsent() == 0) {
		msg("Can't open %s for dump table information: %s\n",
		    _PATH_FSTAB, strerror(errno));
		return;
	}
	while ((fs = getfsent()) != NULL) {
		if (strcmp(fs->fs_type, FSTAB_RW) &&
		    strcmp(fs->fs_type, FSTAB_RO) &&
		    strcmp(fs->fs_type, FSTAB_RQ))
			continue;
#ifdef DUMP_LFS
		if (strcmp(fs->fs_vfstype, "lfs"))
			continue;
#else
		if (strcmp(fs->fs_vfstype, "ufs") &&
		    strcmp(fs->fs_vfstype, "ffs"))
			continue;
#endif
		fs = allocfsent(fs);
		pf = (struct pfstab *)xmalloc(sizeof (*pf));
		pf->pf_fstab = fs;
		SLIST_INSERT_HEAD(&table, pf, pf_list);
	}
	(void) endfsent();
}

/*
 * Search in the fstab for a file name.
 * This file name can be either the special or the path file name.
 *
 * The entries in the fstab are the BLOCK special names, not the
 * character special names.
 * The caller of fstabsearch assures that the character device
 * is dumped (that is much faster)
 *
 * The file name can omit the leading '/'.
 */
struct fstab *
fstabsearch(const char *key)
{
	struct pfstab *pf;
	struct fstab *fs;
	const char *rn;
	char buf[MAXPATHLEN];

	SLIST_FOREACH(pf, &table, pf_list) {
		fs = pf->pf_fstab;
		if (strcmp(fs->fs_file, key) == 0 ||
		    strcmp(fs->fs_spec, key) == 0)
			return (fs);
		rn = getdiskrawname(buf, sizeof(buf), fs->fs_spec);
		if (rn != NULL && strcmp(rn, key) == 0)
			return (fs);
		if (key[0] != '/') {
			if (*fs->fs_spec == '/' &&
			    strcmp(fs->fs_spec + 1, key) == 0)
				return (fs);
			if (*fs->fs_file == '/' &&
			    strcmp(fs->fs_file + 1, key) == 0)
				return (fs);
		}
	}
	return (NULL);
}

/*
 * Search in the mounted file list for a file name.
 * This file name can be either the special or the path file name.
 *
 * The entries in the list are the BLOCK special names, not the
 * character special names.
 * The caller of mntinfosearch assures that the character device
 * is dumped (that is much faster)
 */
struct statvfs *
mntinfosearch(const char *key)
{
	int i, mntbufc;
	struct statvfs *mntbuf, *fs;
	const char *rn;
	char buf[MAXPATHLEN];

	if ((mntbufc = getmntinfo(&mntbuf, MNT_NOWAIT)) == 0)
		quit("Can't get mount list: %s", strerror(errno));
	for (fs = mntbuf, i = 0; i < mntbufc; i++, fs++) {
#ifdef DUMP_LFS
		if (strcmp(fs->f_fstypename, "lfs") != 0)
			continue;
#else /* ! DUMP_LFS */
		if (strcmp(fs->f_fstypename, "ufs") != 0 &&
		    strcmp(fs->f_fstypename, "ffs") != 0)
			continue;
#endif /* ! DUMP_LFS */
		if (strcmp(fs->f_mntonname, key) == 0 ||
		    strcmp(fs->f_mntfromname, key) == 0)
			return (fs);
		rn = getdiskrawname(buf, sizeof(buf), fs->f_mntfromname);
		if (rn != NULL && strcmp(rn, key) == 0)
			return (fs);
	}
	return (NULL);
}


/*
 *	Tell the operator what to do.
 *	arg:	w ==> just what to do; W ==> most recent dumps
 */
void
lastdump(char arg)
{
	int i;
	struct fstab *dt;
	struct dumpdates *dtwalk;
	char *date;
	const char *lastname;
	int dumpme;
	time_t tnow;

	(void) time(&tnow);
	getfstab();		/* /etc/fstab input */
	initdumptimes();	/* /etc/dumpdates input */
	qsort((char *) ddatev, nddates, sizeof(struct dumpdates *), datesort);

	if (arg == 'w')
		(void) printf("Dump these file systems:\n");
	else
		(void) printf("Last dump(s) done (Dump '>' file systems):\n");
	lastname = "??";
	ITITERATE(i, dtwalk) {
		if (strncmp(lastname, dtwalk->dd_name,
		    sizeof(dtwalk->dd_name)) == 0)
			continue;
		date = (char *)ctime(&dtwalk->dd_ddate);
		date[24] = '\0';
		strcpy(date + 16, date + 19);	/* blast away seconds */
		lastname = dtwalk->dd_name;
		dt = fstabsearch(dtwalk->dd_name);
		dumpme = (dt != NULL &&
		    dt->fs_freq != 0 &&
		    dtwalk->dd_ddate < tnow - (dt->fs_freq * SECSPERDAY));
		if (arg != 'w' || dumpme)
			(void) printf(
			    "%c %8s\t(%6s) Last dump: Level %c, Date %s\n",
			    dumpme && (arg != 'w') ? '>' : ' ',
			    dtwalk->dd_name,
			    dt ? dt->fs_file : "",
			    dtwalk->dd_level,
			    date);
	}
}

static int
datesort(const void *a1, const void *a2)
{
	const struct dumpdates *d1 = *(const struct dumpdates *const *)a1;
	const struct dumpdates *d2 = *(const struct dumpdates *const *)a2;
	int diff;

	diff = strncmp(d1->dd_name, d2->dd_name, sizeof(d1->dd_name));
	if (diff == 0)
		return (d2->dd_ddate - d1->dd_ddate);
	return (diff);
}
