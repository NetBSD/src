/*	$NetBSD: perform.c,v 1.28 1999/09/09 01:36:30 hubertf Exp $	*/

#include <sys/cdefs.h>
#ifndef lint
#if 0
static const char *rcsid = "from FreeBSD Id: perform.c,v 1.15 1997/10/13 15:03:52 jkh Exp";
#else
__RCSID("$NetBSD: perform.c,v 1.28 1999/09/09 01:36:30 hubertf Exp $");
#endif
#endif

/*
 * FreeBSD install - a package for the installation and maintainance
 * of non-core utilities.
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
 * Jordan K. Hubbard
 * 18 July 1993
 *
 * This is the main body of the delete module.
 *
 */
/*
 * Copyright (c) 1999 Christian E. Hopps
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
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * Added the require find and require delete code
 */

#include <err.h>
#include <fcntl.h>
#include "lib.h"
#include "delete.h"


/* In which direction to search in require_find() */
typedef enum {
	FIND_UP, FIND_DOWN
}       rec_find_t;

static int require_find_recursive_up(lpkg_t *);
static int require_find_recursive_down(lpkg_t *, package_t *);
static int require_find(char *, rec_find_t);
static int require_delete(char *, int);
static void require_print(void);
static int undepend(const char *deppkgname, char *pkg2delname);

static char LogDir[FILENAME_MAX];
static char linebuf[FILENAME_MAX];
static char pkgdir[FILENAME_MAX];

static package_t Plist;

static lpkg_head_t lpfindq;
static lpkg_head_t lpdelq;

static void
sanity_check(char *pkg)
{
	if (!fexists(CONTENTS_FNAME)) {
		cleanup(0);
		errx(2, "installed package %s has no %s file!",
		     pkg, CONTENTS_FNAME);
	}
}

void
cleanup(int sig)
{
	/* Nothing to do */
	if (sig)		/* in case this is ever used as a signal handler */
		exit(1);
}

/*
 * deppkgname is the pkg from which's +REQUIRED_BY file we are
 * about to remove pkg2delname. This function is called from
 * findmatchingname(), deppkgname is expanded from a (possible) pattern.
 */
int
undepend(const char *deppkgname, char *pkg2delname)
{
	char    fname[FILENAME_MAX], ftmp[FILENAME_MAX];
	char    fbuf[FILENAME_MAX];
	FILE   *fp, *fpwr;
	char   *tmp;
	int     s;

	(void) snprintf(fname, sizeof(fname), "%s/%s/%s",
	    (tmp = getenv(PKG_DBDIR)) ? tmp : DEF_LOG_DIR,
	    deppkgname, REQUIRED_BY_FNAME);
	fp = fopen(fname, "r");
	if (fp == NULL) {
		warnx("couldn't open dependency file `%s'", fname);
		return 0;
	}
	(void) snprintf(ftmp, sizeof(ftmp), "%s.XXXXXX", fname);
	s = mkstemp(ftmp);
	if (s == -1) {
		fclose(fp);
		warnx("couldn't open temp file `%s'", ftmp);
		return 0;
	}
	fpwr = fdopen(s, "w");
	if (fpwr == NULL) {
		close(s);
		fclose(fp);
		warnx("couldn't fdopen temp file `%s'", ftmp);
		remove(ftmp);
		return 0;
	}
	while (fgets(fbuf, sizeof(fbuf), fp) != NULL) {
		if (fbuf[strlen(fbuf) - 1] == '\n')
			fbuf[strlen(fbuf) - 1] = '\0';
		if (strcmp(fbuf, pkg2delname))	/* no match */
			fputs(fbuf, fpwr), putc('\n', fpwr);
	}
	(void) fclose(fp);
	if (fchmod(s, 0644) == FAIL) {
		warnx("error changing permission of temp file `%s'", ftmp);
		fclose(fpwr);
		remove(ftmp);
		return 0;
	}
	if (fclose(fpwr) == EOF) {
		warnx("error closing temp file `%s'", ftmp);
		remove(ftmp);
		return 0;
	}
	if (rename(ftmp, fname) == -1)
		warnx("error renaming `%s' to `%s'", ftmp, fname);
	remove(ftmp);		/* just in case */

	return 0;
}

/*
 * Delete from directory 'home' all packages on lpkg_list. 
 * If tryall is set, ignore errors from pkg_delete(1).
 */
int
require_delete(char *home, int tryall)
{
	lpkg_t *lpp;
	int     rv, fail;
	char   *tmp;
	int     oldcwd;

	/* save cwd */
	oldcwd = open(".", O_RDONLY, 0);
	if (oldcwd == -1)
		err(1, "cannot open \".\"");

	(void) snprintf(pkgdir, sizeof(pkgdir), "%s",
	    (tmp = getenv(PKG_DBDIR)) ? tmp : DEF_LOG_DIR);

	/* walk list of things to delete */
	fail = 0;
	lpp = TAILQ_FIRST(&lpdelq);
	for (; lpp; lpp = TAILQ_NEXT(lpp, lp_link)) {
		/* go to the db dir */
		if (chdir(pkgdir) == FAIL) {
			warnx("unable to change directory to %s, deinstall failed (1)",
			    pkgdir);
			fail = 1;
			break;
		}

		/* look to see if package was already deleted */
		if (!fexists(lpp->lp_name)) {
			warnx("%s appears to have been deleted", lpp->lp_name);
			continue;
		}

		/* return home for execution of command */
		if (chdir(home) == FAIL) {
			warnx("unable to change directory to %s, deinstall failed (2)", home);
			fail = 1;
			break;
		}

		if (Verbose)
			printf("deinstalling %s\n", lpp->lp_name);

		/* delete the package */
		if (Fake)
			rv = 0;
		else
			rv = vsystem("%s %s %s %s %s %s %s %s %s", ProgramPath,
			    Prefix ? "-p" : "",
			    Prefix ? Prefix : "",
			    Verbose ? "-v" : "",
			    Force ? "-f" : "",
			    NoDeInstall ? "-D" : "",
			    CleanDirs ? "-d" : "",
			    Fake ? "-n" : "",
			    lpp->lp_name);

		/* check for delete failure */
		if (rv && !tryall) {
			fail = 1;
			warnx("had problem removing %s%s", lpp->lp_name,
			    Force ? ", continuing" : "");
			if (!Force)
				break;
		}
	}

	/* cleanup list */
	while ((lpp = TAILQ_FIRST(&lpdelq))) {
		TAILQ_REMOVE(&lpdelq, lpp, lp_link);
		free_lpkg(lpp);
	}

	/* return to the log dir */
	if (fchdir(oldcwd) == FAIL) {
		warnx("unable to change to previous directory, deinstall failed");
		fail = 1;
	}

	return (fail);
}

/*
 * Recursively find all packages "up" the tree (follow +REQUIRED_BY). 
 * Return 1 on errors
 */
int
require_find_recursive_up(lpkg_t *thislpp)
{
	lpkg_head_t reqq;
	lpkg_t *lpp = NULL;
	FILE   *cfile;
	char   *nl, *tmp;

	/* see if we are on the find queue -- circular dependency */
	if ((lpp = find_on_queue(&lpfindq, thislpp->lp_name))) {
		warnx("circular dependency found for pkg %s", lpp->lp_name);
		return (1);
	}

	TAILQ_INIT(&reqq);

	(void) snprintf(pkgdir, sizeof(pkgdir), "%s/%s",
	    (tmp = getenv(PKG_DBDIR)) ? tmp : DEF_LOG_DIR, thislpp->lp_name);

	/* change to package's dir */
	if (chdir(pkgdir) == FAIL) {
		warnx("unable to change directory to %s! deinstall failed", pkgdir);
		return (1);
	}

	/* terminate recursion if no required by's */
	if (isemptyfile(REQUIRED_BY_FNAME))
		return (0);

	/* get packages that directly require us */
	cfile = fopen(REQUIRED_BY_FNAME, "r");
	if (!cfile) {
		warnx("cannot open requirements file `%s'", REQUIRED_BY_FNAME);
		return (1);
	}
	while (fgets(linebuf, sizeof(linebuf), cfile)) {
		if ((nl = strrchr(linebuf, '\n')))
			*nl = 0;
		lpp = alloc_lpkg(linebuf);
		TAILQ_INSERT_TAIL(&reqq, lpp, lp_link);
	}
	fclose(cfile);

	/* put ourselves on the top of the find queue */
	TAILQ_INSERT_HEAD(&lpfindq, thislpp, lp_link);

	while ((lpp = TAILQ_FIRST(&reqq))) {
		/* remove a direct req from our queue */
		TAILQ_REMOVE(&reqq, lpp, lp_link);

		/* find direct required requires */
		if (require_find_recursive_up(lpp))
			goto fail;

		/* all requires taken care of, add to tail of delete queue
		 * if not already there */
		if (find_on_queue(&lpdelq, lpp->lp_name))
			free_lpkg(lpp);
		else
			TAILQ_INSERT_TAIL(&lpdelq, lpp, lp_link);
	}

	/* take ourselves off the find queue */
	TAILQ_REMOVE(&lpfindq, thislpp, lp_link);

	return (0);

fail:
	while ((lpp = TAILQ_FIRST(&reqq))) {
		TAILQ_REMOVE(&reqq, lpp, lp_link);
		free_lpkg(lpp);
	}
	return (1);
}

/*
 * Recursively find all packages "down" the tree (follow @pkgdep). 
 * Return 1 on errors
 */
int
require_find_recursive_down(lpkg_t *thislpp, package_t *plist)
{
	plist_t *p;
	lpkg_t *lpp, *lpp2;
	lpkg_head_t reqq;
	int     rc, fail = 0;

	/* see if we are on the find queue -- circular dependency */
	if ((lpp = find_on_queue(&lpfindq, thislpp->lp_name))) {
		warnx("circular dependency found for pkg %s", lpp->lp_name);
		return (1);
	}

	TAILQ_INIT(&reqq);

	/* width-first scan */
	/* first enqueue all @pkgdep's to lpdelq, then (further below)
	 * go in recursively */
	for (p = plist->head; p; p = p->next) {
		switch (p->type) {
		case PLIST_PKGDEP:
			lpp = alloc_lpkg(p->name);
			TAILQ_INSERT_TAIL(&reqq, lpp, lp_link);

			lpp2 = find_on_queue(&lpdelq, p->name);
			if (lpp2)
				TAILQ_REMOVE(&lpdelq, lpp2, lp_link);
			lpp = alloc_lpkg(p->name);
			TAILQ_INSERT_TAIL(&lpdelq, lpp, lp_link);

			break;
		default:
			break;
		}
	}

	while ((lpp = TAILQ_FIRST(&reqq))) {
		FILE   *cfile;
		package_t rPlist;
		char   *tmp;
		plist_t *p;

		/* remove a direct req from our queue */
		TAILQ_REMOVE(&reqq, lpp, lp_link);

		/* Reset some state */
		rPlist.head = NULL;
		rPlist.tail = NULL;

		/* prepare for recursion */
		chdir((tmp = getenv(PKG_DBDIR)) ? tmp : DEF_LOG_DIR);
		chdir(lpp->lp_name);
		sanity_check(lpp->lp_name);

		cfile = fopen(CONTENTS_FNAME, "r");
		if (!cfile) {
			warn("unable to open '%s' file", CONTENTS_FNAME);
			fail = 1;
			goto fail;
		}
		/* If we have a prefix, add it now */
		if (Prefix)
			add_plist(&rPlist, PLIST_CWD, Prefix);
		read_plist(&rPlist, cfile);
		fclose(cfile);
		p = find_plist(&rPlist, PLIST_CWD);
		if (!p) {
			warnx("package '%s' doesn't have a prefix", lpp->lp_name);
			fail = 1;
			goto fail;
		}

		/* put ourselves on the top of the find queue */
		TAILQ_INSERT_HEAD(&lpfindq, thislpp, lp_link);

		rc = require_find_recursive_down(lpp, &rPlist);
		if (rc) {
			fail = 1;
			goto fail;
		}

		/* take ourselves off the find queue */
		TAILQ_REMOVE(&lpfindq, thislpp, lp_link);
		free_lpkg(lpp);
	}

fail:
	/* Clean out reqq */
	while ((lpp = TAILQ_FIRST(&reqq))) {
		TAILQ_REMOVE(&reqq, lpp, lp_link);
		free_lpkg(lpp);
	}

	return fail;
}

/*
 * Start recursion in the one or other direction. 
 */
int
require_find(char *pkg, rec_find_t updown)
{
	lpkg_t *lpp;
	int     rv = 0;

	TAILQ_INIT(&lpfindq);
	TAILQ_INIT(&lpdelq);

	lpp = alloc_lpkg(pkg);
	switch (updown) {
	case FIND_UP:
		rv = require_find_recursive_up(lpp);
		break;
	case FIND_DOWN:
		rv = require_find_recursive_down(lpp, &Plist);
		break;
	}
	free_lpkg(lpp);

	return (rv);
}

void
require_print(void)
{
	lpkg_t *lpp;

	/* print all but last -- deleting if requested */
	while ((lpp = TAILQ_FIRST(&lpdelq))) {
		TAILQ_REMOVE(&lpdelq, lpp, lp_link);
		fprintf(stderr, "\t%s\n", lpp->lp_name);
		free_lpkg(lpp);
	}
}

/*
 * This is seriously ugly code following.  Written very fast!
 */
static int
pkg_do(char *pkg)
{
	FILE   *cfile;
	char    home[FILENAME_MAX];
	plist_t *p;
	char   *tmp;

	/* Reset some state */
	if (Plist.head)
		free_plist(&Plist);

	(void) snprintf(LogDir, sizeof(LogDir), "%s/%s", (tmp = getenv(PKG_DBDIR)) ? tmp : DEF_LOG_DIR,
	    pkg);
	if (!fexists(LogDir) || !isdir(LogDir)) {
		{
			/* Check if the given package name matches something
			 * with 'pkg-[0-9]*' */
			char    try[FILENAME_MAX];
			snprintf(try, FILENAME_MAX, "%s-[0-9]*", pkg);
			if (findmatchingname(_pkgdb_getPKGDB_DIR(), try,
				find_fn, NULL) != 0) {
				return 0;	/* we've just appended some names to the pkgs list,
						 * they will be processed after this package. */
			}
		}

		/* No match */
		warnx("no such package '%s' installed", pkg);
		return 1;
	}
	if (!getcwd(home, FILENAME_MAX)) {
		cleanup(0);
		errx(2, "unable to get current working directory!");
	}
	if (chdir(LogDir) == FAIL) {
		warnx("unable to change directory to %s! deinstall failed", LogDir);
		return 1;
	}
	if (!isemptyfile(REQUIRED_BY_FNAME)) {
		/* This package is required by others. Either nuke
		 * them (-r), or stop. */
		if (!Recurse_up)
			warnx("package `%s' is required by other packages:", pkg);
		else if (Verbose)
			printf("Building list of packages that require `%s'"
			    " to deinstall\n", pkg);
		if (require_find(pkg, FIND_UP)) {
			if (!Force || Recurse_up)
				return (1);
		}
		chdir(LogDir);	/* CWD was changed by require_find() */
		if (!Recurse_up) {
			require_print();
			if (!Force)
				return 1;
		} else
			require_delete(home, 0);
	}
	sanity_check(LogDir);
	cfile = fopen(CONTENTS_FNAME, "r");
	if (!cfile) {
		warnx("unable to open '%s' file", CONTENTS_FNAME);
		return 1;
	}
	/* If we have a prefix, add it now */
	if (Prefix)
		add_plist(&Plist, PLIST_CWD, Prefix);
	read_plist(&Plist, cfile);
	fclose(cfile);
	p = find_plist(&Plist, PLIST_CWD);
	if (!p) {
		warnx("package '%s' doesn't have a prefix", pkg);
		return 1;
	}
	setenv(PKG_PREFIX_VNAME, p->name, 1);
	if (fexists(REQUIRE_FNAME)) {
		if (Verbose)
			printf("Executing 'require' script.\n");
		vsystem("%s +x %s", CHMOD, REQUIRE_FNAME);	/* be sure */
		if (vsystem("./%s %s DEINSTALL", REQUIRE_FNAME, pkg)) {
			warnx("package %s fails requirements %s", pkg,
			    Force ? "" : "- not deleted");
			if (!Force)
				return 1;
		}
	}
	if (!NoDeInstall && fexists(DEINSTALL_FNAME)) {
		if (Fake)
			printf("Would execute de-install script at this point (arg: DEINSTALL).\n");
		else {
			vsystem("%s +x %s", CHMOD, DEINSTALL_FNAME);	/* make sure */
			if (vsystem("./%s %s DEINSTALL", DEINSTALL_FNAME, pkg)) {
				warnx("deinstall script returned error status");
				if (!Force)
					return 1;
			}
		}
	}
	if (!Fake) {
		/* Some packages aren't packed right, so we need to just ignore delete_package()'s status.  Ugh! :-( */
		if (delete_package(FALSE, CleanDirs, &Plist) == FAIL)
			warnx(
			    "couldn't entirely delete package (perhaps the packing list is\n"
			    "incorrectly specified?)");
	}
	/* Remove this package from the +REQUIRED_BY list of the packages this depends on */
	for (p = Plist.head; p; p = p->next) {
		if (p->type != PLIST_PKGDEP)
			continue;
		if (Verbose)
			printf("Attempting to remove dependency on package `%s'\n", p->name);
		if (!Fake)
			findmatchingname((tmp = getenv(PKG_DBDIR)) ? tmp : DEF_LOG_DIR,
			    p->name, undepend, pkg);
	}
	if (Recurse_down) {
		/* Also remove the packages further down, now that there's
		 * (most likely) nothing left which requires them. */
		if (Verbose)
			printf("Building list of packages that `%s' required\n", pkg);
		if (require_find(pkg, FIND_DOWN))
			return (1);

		require_delete(home, 1);
	}
	if (!NoDeInstall && fexists(DEINSTALL_FNAME)) {
		if (Fake)
			printf("Would execute post-de-install script at this point (arg: POST-DEINSTALL).\n");
		else {
			vsystem("chmod +x %s", DEINSTALL_FNAME);	/* make sure */
			if (vsystem("./%s %s POST-DEINSTALL", DEINSTALL_FNAME, pkg)) {
				warnx("post-deinstall script returned error status");
				if (!Force)
					return 1;
			}
		}
	}
	/* Change out of LogDir before we remove it */
	if (chdir(home) == FAIL) {
		cleanup(0);
		errx(2, "Toto! This doesn't look like Kansas anymore!");
	}
	if (!Fake) {
		/* Finally nuke the +-files and the pkgdb-dir (/var/db/pkg/foo) */
		if (vsystem("%s -r %s", REMOVE_CMD, LogDir)) {
			warnx("couldn't remove log entry in %s, deinstall failed", LogDir);
			if (!Force)
				return 1;
		}
	}
	return 0;
}

int
pkg_perform(lpkg_head_t *pkgs)
{
	int     err_cnt = 0;
	int     oldcwd;
	lpkg_t *lpp;

	/* save cwd */
	oldcwd = open(".", O_RDONLY, 0);
	if (oldcwd == -1)
		err(1, "cannot open \".\"");

	while ((lpp = TAILQ_FIRST(pkgs))) {
		err_cnt += pkg_do(lpp->lp_name);
		TAILQ_REMOVE(pkgs, lpp, lp_link);
		free_lpkg(lpp);
		if (fchdir(oldcwd) == FAIL)
			err(1, "unable to change to previous directory");
	}
	return err_cnt;
}
