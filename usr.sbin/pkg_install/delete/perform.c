/*	$NetBSD: perform.c,v 1.48 2003/09/23 09:36:05 wiz Exp $	*/

#include <sys/cdefs.h>
#ifndef lint
#if 0
static const char *rcsid = "from FreeBSD Id: perform.c,v 1.15 1997/10/13 15:03:52 jkh Exp";
#else
__RCSID("$NetBSD: perform.c,v 1.48 2003/09/23 09:36:05 wiz Exp $");
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
static int undepend(const char *, void *);

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
static int
undepend(const char *deppkgname, void *vp)
{
	char   *pkg2delname = vp;
	char    fname[FILENAME_MAX], ftmp[FILENAME_MAX];
	char    fbuf[FILENAME_MAX];
	FILE   *fp, *fpwr;
	int     s;

	(void) snprintf(fname, sizeof(fname), "%s/%s/%s",
	    _pkgdb_getPKGDB_DIR(), deppkgname, REQUIRED_BY_FNAME);
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
 * Remove the current view's package dbdir from the +VIEWS file of the
 * depoted package named by pkgname.
 */
static int
unview(const char *pkgname)
{
	char  fname[FILENAME_MAX], ftmp[FILENAME_MAX];
	char  fbuf[FILENAME_MAX];
	char  dbdir[FILENAME_MAX];
	FILE *fp, *fpwr;
	int  s;
	int  cc;

	(void) snprintf(dbdir, sizeof(dbdir), "%s", _pkgdb_getPKGDB_DIR());

	/* Get the depot directory. */
	(void) snprintf(fname, sizeof(fname), "%s/%s/%s",
	    dbdir, pkgname, DEPOT_FNAME);
	if ((fp = fopen(fname, "r")) == NULL) {
		warnx("unable to open `%s' file", fname);
		return -1;
	}
	if (fgets(fbuf, sizeof(fbuf), fp) == NULL) {
		(void) fclose(fp);
		warnx("empty depot file `%s'", fname);
		return -1;
	}
	if (fbuf[cc = strlen(fbuf) - 1] == '\n') {
		fbuf[cc] = 0;
	}
	fclose(fp);

	/*
	 * Copy the contents of the +VIEWS file into a temp file, but
	 * skip copying the name of the current view's package dbdir.
	 */
	(void) snprintf(fname, sizeof(fname), "%s/%s", fbuf, VIEWS_FNAME);
	if ((fp = fopen(fname, "r")) == NULL) {
		warnx("unable to open `%s' file", fname);
		return -1;
	}
	(void) snprintf(ftmp, sizeof(ftmp), "%s.XXXXXX", fname);
	if ((s = mkstemp(ftmp)) == -1) {
		(void) fclose(fp);
		warnx("unable to open `%s' temp file", ftmp);
		return -1;
	}
	if ((fpwr = fdopen(s, "w")) == NULL) {
		(void) close(s);
		(void) remove(ftmp);
		(void) fclose(fp);
		warnx("unable to fdopen `%s' temp file", ftmp);
		return -1;
	}
	while (fgets(fbuf, sizeof(fbuf), fp) != NULL) {
		if (fbuf[cc = strlen(fbuf) - 1] == '\n') {
			fbuf[cc] = 0;
		}
		if (strcmp(fbuf, dbdir) != 0) {
			(void) fputs(fbuf, fpwr);
			(void) putc('\n', fpwr);
		}
	}
	(void) fclose(fp);
	if (fchmod(s, 0644) == FAIL) {
		(void) fclose(fpwr);
		(void) remove(ftmp);
		warnx("unable to change permissions of `%s' temp file", ftmp);
		return -1;
	}
	if (fclose(fpwr) == EOF) {
		(void) remove(ftmp);
		warnx("unable to close `%s' temp file", ftmp);
		return -1;
	}

	/* Rename the temp file to the +VIEWS file */
	if (rename(ftmp, fname) == -1) {
		(void) remove(ftmp);
		warnx("unable to rename `%s' to `%s'", ftmp, fname);
		return -1;
	}
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
	int     oldcwd;

	/* save cwd */
	oldcwd = open(".", O_RDONLY, 0);
	if (oldcwd == -1)
		err(EXIT_FAILURE, "cannot open \".\"");

	(void) snprintf(pkgdir, sizeof(pkgdir), "%s", _pkgdb_getPKGDB_DIR());

	/* walk list of things to delete */
	fail = 0;
	lpp = TAILQ_FIRST(&lpdelq);
	for (; lpp; lpp = TAILQ_NEXT(lpp, lp_link)) {
		int rm_installed;                /* delete expanded pkg, not @pkgdep value */
		char installed[FILENAME_MAX];
		
		/* go to the db dir */
		if (chdir(pkgdir) == FAIL) {
			warnx("unable to change directory to %s, deinstall failed (1)",
			    pkgdir);
			fail = 1;
			break;
		}

		/* look to see if package was already deleted */
		rm_installed = 0;
		if (ispkgpattern(lpp->lp_name)) {
			if (findmatchingname(".", lpp->lp_name, note_whats_installed, installed) != 1) {
				warnx("%s appears to have been deleted", lpp->lp_name);
				continue;
			}
			rm_installed = 1;
		} else {
			if (!fexists(lpp->lp_name)) {
				warnx("%s appears to have been deleted", lpp->lp_name);
				continue;
			}
		}

		/* return home for execution of command */
		if (chdir(home) == FAIL) {
			warnx("unable to change directory to %s, deinstall failed (2)", home);
			fail = 1;
			break;
		}

		if (Verbose)
			printf("deinstalling %s\n", rm_installed?installed:lpp->lp_name);

		/* delete the package */
		if (Fake)
			rv = 0;
		else
			rv = fexec_skipempty(ProgramPath, "-K",
					     _pkgdb_getPKGDB_DIR(),
					     Prefix ? "-p" : "", Prefix ? Prefix : "",
					     Verbose ? "-v" : "",
					     (Force > 1) ? "-f -f" : (Force == 1) ? "-f" : "",
					     NoDeInstall ? "-D" : "",
					     CleanDirs ? "-d" : "",
					     Fake ? "-n" : "",
					     rm_installed ? installed : lpp->lp_name, NULL);

		/* check for delete failure */
		if (rv && !tryall) {
			fail = 1;
			warnx("had problem removing %s%s", rm_installed?installed:lpp->lp_name,
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
	close(oldcwd);

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
	char   *nl;

	/* see if we are on the find queue -- circular dependency */
	if ((lpp = find_on_queue(&lpfindq, thislpp->lp_name))) {
		warnx("circular dependency found for pkg %s", lpp->lp_name);
		return (1);
	}

	TAILQ_INIT(&reqq);

	(void) snprintf(pkgdir, sizeof(pkgdir), "%s/%s",
	    _pkgdb_getPKGDB_DIR(), thislpp->lp_name);

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
			if (lpp2) {
				TAILQ_REMOVE(&lpdelq, lpp2, lp_link);
				free_lpkg(lpp2);
			}
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

		/* remove a direct req from our queue */
		TAILQ_REMOVE(&reqq, lpp, lp_link);

		/* Reset some state */
		rPlist.head = NULL;
		rPlist.tail = NULL;

		/* prepare for recursion */
		chdir(_pkgdb_getPKGDB_DIR());
		if (ispkgpattern(lpp->lp_name)) {
			char installed[FILENAME_MAX];
			if (findmatchingname(".", lpp->lp_name, note_whats_installed, installed) != 1) {
				warnx("cannot remove dependency for pkg-pattern %s", lpp->lp_name);
				fail = 1;
				goto fail; 
			}
			if (chdir(installed) == -1) {
				warnx("can't chdir to %s", installed);
				fail = 1;
				goto fail;
			}
			sanity_check(installed);
		} else {
			if (chdir(lpp->lp_name) == -1) {
				warnx("cannot remove dependency from %s", lpp->lp_name);
				fail = 1;
				goto fail; 
			}
			sanity_check(lpp->lp_name);
		}

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
			free_plist(&rPlist);
			fail = 1;
			goto fail;
		}

		/* put ourselves on the top of the find queue */
		TAILQ_INSERT_HEAD(&lpfindq, thislpp, lp_link);

		rc = require_find_recursive_down(lpp, &rPlist);
		free_plist(&rPlist);
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
	plist_t	       *p;
	FILE	       *cfile;
	FILE	       *fp;
	char    	home[FILENAME_MAX];
	char    	view[FILENAME_MAX];
	int		cc;
	Boolean		is_depoted_pkg = FALSE;

	/* Reset some state */
	if (Plist.head)
		free_plist(&Plist);

	(void) snprintf(LogDir, sizeof(LogDir), "%s/%s",
	    _pkgdb_getPKGDB_DIR(), pkg);
	if (!fexists(LogDir) || !(isdir(LogDir) || islinktodir(LogDir))) {
		/* Check if the given package name matches something
		 * with 'pkg-[0-9]*' */
		char	        try[FILENAME_MAX];
		lpkg_head_t     trypkgs;
		lpkg_t	       *lpp;
		int		qlen = 0;

		TAILQ_INIT(&trypkgs);
		snprintf(try, FILENAME_MAX, "%s-[0-9]*", pkg);
		if (findmatchingname(_pkgdb_getPKGDB_DIR(), try,
			add_to_list_fn, &trypkgs) == NULL) {
			warnx("package '%s' not installed", pkg);
			return 1;
		}

		TAILQ_FOREACH(lpp, &trypkgs, lp_link)
			qlen++;

		if (qlen > 1) {
			warnx("'%s' matches more than one package:", pkg);
			while ((lpp = TAILQ_FIRST(&trypkgs))) {
				TAILQ_REMOVE(&trypkgs, lpp, lp_link);
				fprintf(stderr, "\t%s\n", lpp->lp_name);
				free_lpkg(lpp);
			}
			return 1;
		}

		/*
		 * Append the package names we've discovered to the
		 * pkgs list after this one, and return 0 so that we
		 * continue processing the pkgs list.
		 */
		TAILQ_FOREACH(lpp, &trypkgs, lp_link)
			TAILQ_INSERT_TAIL(&pkgs, lpp, lp_link);

		return 0;
	}
	if (!getcwd(home, FILENAME_MAX)) {
		cleanup(0);
		errx(2, "unable to get current working directory!");
	}
	if (chdir(LogDir) == FAIL) {
		warnx("unable to change directory to %s! deinstall failed", LogDir);
		return 1;
	}
	if (!fexists(CONTENTS_FNAME)) {
		warnx("package '%s' is not installed, %s missing", pkg, CONTENTS_FNAME);
		if (!Force)
			return 1;
	}
	if (fexists(PRESERVE_FNAME)) {
		printf("Package `%s' is marked as not for deletion\n", pkg);
		if (Force <= 1) {
			return 1;
		}
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
	if (!isemptyfile(VIEWS_FNAME)) {
		/* This package has instances in other views */
		/* Delete them from the views */
		if ((fp = fopen(VIEWS_FNAME, "r")) == NULL) {
			warnx("unable to open '%s' file", VIEWS_FNAME);
			return 1;
		}
		while (fgets(view, sizeof(view), fp) != NULL) {
			if (view[cc = strlen(view) - 1] == '\n') {
				view[cc] = 0;
			}
			if (Verbose) {
				printf("Deleting package %s instance from `%s' view\n", pkg, view);
			}
			if (fexec_skipempty(ProgramPath, "-K", view,
					    (Force > 1) ? "-f -f " : (Force == 1) ? "-f " : "",
					    pkg, NULL) != 0) {
				warnx("unable to delete package %s from view %s", pkg, view);
				(void) fclose(fp);
				return 1;
			}
		}
		(void) fclose(fp);
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
		(void) fexec(CHMOD_CMD, "+x", REQUIRE_FNAME, NULL);	/* be sure */
		if (fexec("./" REQUIRE_FNAME, pkg, "DEINSTALL", NULL)) {
			warnx("package %s fails requirements %s", pkg,
			    Force ? "" : "- not deleted");
			if (!Force)
				return 1;
		}
	}
	/*
	 * Ensure that we don't do VIEW-DEINSTALL action for old packages
	 * or for the package in its depot directory.
	 */
	if (!NoDeInstall && fexists(DEINSTALL_FNAME) && fexists(DEPOT_FNAME)) {
		if (Fake) {
			printf("Would execute view de-install script at this point (arg: VIEW-DEINSTALL).\n");
		} else {
			(void) fexec(CHMOD_CMD, "+x", DEINSTALL_FNAME, NULL);	/* make sure */
			if (fexec("./" DEINSTALL_FNAME, pkg, "VIEW-DEINSTALL", NULL)) {
				warnx("view deinstall script returned error status");
				if (!Force) {
					return 1;
				}
			}
		}
	}
	if (!NoDeInstall && fexists(DEINSTALL_FNAME) && !fexists(DEPOT_FNAME)) {
		if (Fake)
			printf("Would execute de-install script at this point (arg: DEINSTALL).\n");
		else {
			(void) fexec(CHMOD_CMD, "+x", DEINSTALL_FNAME, NULL);	/* make sure */
			if (fexec("./" DEINSTALL_FNAME, pkg, "DEINSTALL", NULL)) {
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
		"couldn't entirely delete package `%s'\n"
		"(perhaps the packing list is incorrectly specified?)", pkg);
	}
	if (!isemptyfile(DEPOT_FNAME)) {
		if (Verbose)
			printf("Attempting to remove the %s registration on package `%s'\n", VIEWS_FNAME, pkg);
		if (!Fake)
			(void) unview(pkg);
	}
	/*
	 * If this isn't a package in a view, then remove this package
	 * from the +REQUIRED_BY list of the packages this depends on.
	 */
	if (!fexists(DEPOT_FNAME)) {
		for (p = Plist.head; p; p = p->next) {
			if (p->type != PLIST_PKGDEP)
				continue;
			if (Verbose)
				printf("Attempting to remove dependency on package `%s'\n", p->name);
			if (!Fake)
				findmatchingname(_pkgdb_getPKGDB_DIR(),
				    p->name, undepend, pkg);
		}
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
	if (!NoDeInstall && fexists(DEINSTALL_FNAME) && !fexists(DEPOT_FNAME)) {
		if (Fake)
			printf("Would execute post-de-install script at this point (arg: POST-DEINSTALL).\n");
		else {
			(void) fexec(CHMOD_CMD, "+x", DEINSTALL_FNAME, NULL);	/* make sure */
			if (fexec("./" DEINSTALL_FNAME, pkg, "POST-DEINSTALL", NULL)) {
				warnx("post-deinstall script returned error status");
				if (!Force)
					return 1;
			}
		}
	}
	if (fexists(VIEWS_FNAME))
		is_depoted_pkg = TRUE;

	/* Change out of LogDir before we remove it.
	 * Do not fail here, as the package is not yet completely deleted! */
	if (chdir(home) == FAIL)
		warnx("Oops - removed current working directory.  Oh, well.");
	if (!Fake) {
		/* Finally nuke the +-files and the pkgdb-dir (/var/db/pkg/foo) */
		if (is_depoted_pkg) {
			(void) vsystem("%s %s/+*", REMOVE_CMD, LogDir);
			if (isemptydir(LogDir))
				(void) fexec(RMDIR_CMD, LogDir, NULL);
			else
				warnx("%s is not empty", LogDir);
			return 0;
		} else {
			if (fexec(REMOVE_CMD, "-r", LogDir, NULL)) {
				warnx("couldn't remove log entry in %s, deinstall failed", LogDir);
				if (!Force)
					return 1;
			}
		}
	}
	return 0;
}

int
pkg_perform(lpkg_head_t *pkghead)
{
	int     err_cnt = 0;
	int     oldcwd;
	lpkg_t *lpp;

	/* save cwd */
	oldcwd = open(".", O_RDONLY, 0);
	if (oldcwd == -1)
		err(EXIT_FAILURE, "cannot open \".\"");

	while ((lpp = TAILQ_FIRST(pkghead))) {
		err_cnt += pkg_do(lpp->lp_name);
		TAILQ_REMOVE(pkghead, lpp, lp_link);
		free_lpkg(lpp);
		if (fchdir(oldcwd) == FAIL)
			err(EXIT_FAILURE, "unable to change to previous directory");
	}
	close(oldcwd);
	return err_cnt;
}
