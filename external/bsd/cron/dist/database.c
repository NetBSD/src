/*	$NetBSD: database.c,v 1.7.8.1 2013/02/25 00:26:02 tls Exp $	*/

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
static char rcsid[] = "Id: database.c,v 1.7 2004/01/23 18:56:42 vixie Exp";
#else
__RCSID("$NetBSD: database.c,v 1.7.8.1 2013/02/25 00:26:02 tls Exp $");
#endif
#endif

/* vix 26jan87 [RCS has the log]
 */

#include "cron.h"

#define TMAX(a,b) ((a)>(b)?(a):(b))

static	void		process_crontab(const char *, const char *,
					const char *, struct stat *,
					cron_db *, cron_db *);

static void
process_dir(const char *dname, struct stat *st, int sys, cron_db *new_db,
    cron_db *old_db)
{
	DIR *dir;
	DIR_T *dp;

	/* we used to keep this dir open all the time, for the sake of
	 * efficiency.  however, we need to close it in every fork, and
	 * we fork a lot more often than the mtime of the dir changes.
	 */
	if (!(dir = opendir(dname))) {
		log_it("CRON", getpid(), "OPENDIR FAILED", dname);
		(void) exit(ERROR_EXIT);
	}

	while (NULL != (dp = readdir(dir))) {
		char	fname[MAXNAMLEN+1], tabname[MAXNAMLEN+1];
		size_t i, len;
		/*
		 * Homage to...
		 */
		static const char *junk[] = {
			"rpmsave", "rpmorig", "rpmnew",
		};

		/* avoid file names beginning with ".".  this is good
		 * because we would otherwise waste two guaranteed calls
		 * to getpwnam() for . and .., and there shouldn't be 
		 * hidden files in here anyway (in the non system case).
		 */
		if (dp->d_name[0] == '.')
			continue;

		/* ignore files starting with # ... */
		if (dp->d_name[0] == '#')
			continue;
		
		len = strlen(dp->d_name);

		/* ... or too big or to small ... */
		if (len == 0 || len >= sizeof(fname)) {
			log_it(dp->d_name, getpid(), "ORPHAN",
			    "name too short or long");
			continue;
		}

		/* ... or ending with ~ ... */
		if (dp->d_name[len - 1] == '~')
			continue;

		(void)strlcpy(fname, dp->d_name, sizeof(fname));
		
		/* ... or look for blacklisted extensions */
		for (i = 0; i < __arraycount(junk); i++) {
			char *p;
			if ((p = strrchr(fname, '.')) != NULL &&
			    strcmp(p + 1, junk[i]) == 0)
				break;
		}
		if (i != __arraycount(junk))
			continue;

		if (!glue_strings(tabname, sizeof tabname, dname, fname, '/')) {
			log_it(fname, getpid(), "ORPHAN",
			    "could not glue strings");
			continue;
		}

		process_crontab(sys ? "root" : fname, sys ? "*system*" :
				fname, tabname, st, new_db, old_db);
	}
	(void)closedir(dir);
}

void
load_database(cron_db *old_db) {
	struct stat spool_stat, syscron_stat, crond_stat;
	cron_db new_db;
	user *u, *nu;
	time_t new_mtime;

	Debug(DLOAD, ("[%ld] load_database()\n", (long)getpid()));

	/* before we start loading any data, do a stat on SPOOL_DIR
	 * so that if anything changes as of this moment (i.e., before we've
	 * cached any of the database), we'll see the changes next time.
	 */
	if (stat(SPOOL_DIR, &spool_stat) < OK) {
		log_it("CRON", getpid(), "STAT FAILED", SPOOL_DIR);
		(void) exit(ERROR_EXIT);
	}

	/* track system crontab directory
	 */
	if (stat(CROND_DIR, &crond_stat) < OK)
		crond_stat.st_mtime = 0;

	/* track system crontab file
	 */
	if (stat(SYSCRONTAB, &syscron_stat) < OK)
		syscron_stat.st_mtime = 0;

	/* if spooldir's mtime has not changed, we don't need to fiddle with
	 * the database.
	 *
	 * Note that old_db->mtime is initialized to 0 in main(), and
	 * so is guaranteed to be different than the stat() mtime the first
	 * time this function is called.
	 */
	new_mtime = TMAX(crond_stat.st_mtime, TMAX(spool_stat.st_mtime,
	    syscron_stat.st_mtime));
	if (old_db->mtime == new_mtime) {
		Debug(DLOAD, ("[%ld] spool dir mtime unch, no load needed.\n",
			      (long)getpid()));
		return;
	}

	/* something's different.  make a new database, moving unchanged
	 * elements from the old database, reloading elements that have
	 * actually changed.  Whatever is left in the old database when
	 * we're done is chaff -- crontabs that disappeared.
	 */
	new_db.mtime = new_mtime;
	new_db.head = new_db.tail = NULL;

	if (syscron_stat.st_mtime)
		process_crontab("root", NULL, SYSCRONTAB, &syscron_stat,
				&new_db, old_db);

	if (crond_stat.st_mtime)
		process_dir(CROND_DIR, &crond_stat, 1, &new_db, old_db);

	process_dir(SPOOL_DIR, &spool_stat, 0, &new_db, old_db);

	/* if we don't do this, then when our children eventually call
	 * getpwnam() in do_command.c's child_process to verify MAILTO=,
	 * they will screw us up (and v-v).
	 */
	endpwent();

	/* whatever's left in the old database is now junk.
	 */
	Debug(DLOAD, ("unlinking old database:\n"));
	for (u = old_db->head;  u != NULL;  u = nu) {
		Debug(DLOAD, ("\t%s\n", u->name));
		nu = u->next;
		unlink_user(old_db, u);
		free_user(u);
	}

	/* overwrite the database control block with the new one.
	 */
	*old_db = new_db;
	Debug(DLOAD, ("load_database is done\n"));
}

void
link_user(cron_db *db, user *u) {
	if (db->head == NULL)
		db->head = u;
	if (db->tail)
		db->tail->next = u;
	u->prev = db->tail;
	u->next = NULL;
	db->tail = u;
}

void
unlink_user(cron_db *db, user *u) {
	if (u->prev == NULL)
		db->head = u->next;
	else
		u->prev->next = u->next;

	if (u->next == NULL)
		db->tail = u->prev;
	else
		u->next->prev = u->prev;
}

user *
find_user(cron_db *db, const char *name) {
	user *u;

	for (u = db->head;  u != NULL;  u = u->next)
		if (strcmp(u->name, name) == 0)
			break;
	return (u);
}

static void
process_crontab(const char *uname, const char *fname, const char *tabname,
		struct stat *statbuf, cron_db *new_db, cron_db *old_db)
{
	struct passwd *pw = NULL;
	int crontab_fd = OK - 1;
	mode_t eqmode = 0400, badmode = 0;
	user *u;

	if (fname == NULL) {
		/*
		 * SYSCRONTAB:
		 * set fname to something for logging purposes.
		 * Allow it to become readable by group and others, but
		 * not writable.
		 */
		fname = "*system*";
		eqmode = 0;
		badmode = 022;
	} else if ((pw = getpwnam(uname)) == NULL) {
		/* file doesn't have a user in passwd file.
		 */
		log_it(fname, getpid(), "ORPHAN", "no passwd entry");
		goto next_crontab;
	}

	if ((crontab_fd = open(tabname, O_RDONLY|O_NONBLOCK|O_NOFOLLOW, 0)) < OK) {
		/* crontab not accessible?
		 */
		log_it(fname, getpid(), "CAN'T OPEN", tabname);
		goto next_crontab;
	}

	if (fstat(crontab_fd, statbuf) < OK) {
		log_it(fname, getpid(), "FSTAT FAILED", tabname);
		goto next_crontab;
	}
	if (!S_ISREG(statbuf->st_mode)) {
		log_it(fname, getpid(), "NOT REGULAR", tabname);
		goto next_crontab;
	}
	if ((eqmode && (statbuf->st_mode & 07577) != eqmode) ||
	    (badmode && (statbuf->st_mode & badmode) != 0)) {
		log_it(fname, getpid(), "BAD FILE MODE", tabname);
		goto next_crontab;
	}
	if (statbuf->st_uid != ROOT_UID && (pw == NULL ||
	    statbuf->st_uid != pw->pw_uid || strcmp(uname, pw->pw_name) != 0)) {
		log_it(fname, getpid(), "WRONG FILE OWNER", tabname);
		goto next_crontab;
	}
	if (statbuf->st_nlink != 1) {
		log_it(fname, getpid(), "BAD LINK COUNT", tabname);
		goto next_crontab;
	}

	Debug(DLOAD, ("\t%s:", fname));
	u = find_user(old_db, fname);
	if (u != NULL) {
		/* if crontab has not changed since we last read it
		 * in, then we can just use our existing entry.
		 */
		if (u->mtime == statbuf->st_mtime) {
			Debug(DLOAD, (" [no change, using old data]"));
			unlink_user(old_db, u);
			link_user(new_db, u);
			goto next_crontab;
		}

		/* before we fall through to the code that will reload
		 * the user, let's deallocate and unlink the user in
		 * the old database.  This is more a point of memory
		 * efficiency than anything else, since all leftover
		 * users will be deleted from the old database when
		 * we finish with the crontab...
		 */
		Debug(DLOAD, (" [delete old data]"));
		unlink_user(old_db, u);
		free_user(u);
		log_it(fname, getpid(), "RELOAD", tabname);
	}
	u = load_user(crontab_fd, pw, fname);
	if (u != NULL) {
		u->mtime = statbuf->st_mtime;
		link_user(new_db, u);
	}

 next_crontab:
	if (crontab_fd >= OK) {
		Debug(DLOAD, (" [done]\n"));
		(void)close(crontab_fd);
	}
}
