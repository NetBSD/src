/*	$NetBSD: perform.c,v 1.1.1.2 2009/02/02 20:44:04 joerg Exp $	*/

#if HAVE_CONFIG_H
#include "config.h"
#endif
#include <nbcompat.h>
#if HAVE_SYS_CDEFS_H
#include <sys/cdefs.h>
#endif
__RCSID("$NetBSD: perform.c,v 1.1.1.2 2009/02/02 20:44:04 joerg Exp $");

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

#if HAVE_ERR_H
#include <err.h>
#endif
#if HAVE_FCNTL_H
#include <fcntl.h>
#endif
#include "lib.h"
#include "delete.h"


/* In which direction to search in require_find() */
typedef enum {
	FIND_UP, FIND_DOWN
}       rec_find_t;

static int require_find_recursive_up(lpkg_t *);
static int require_find_recursive_down(lpkg_t *, package_t *);
static int require_find(char *, rec_find_t);
static int require_delete(int);
static void require_print(void);
static int undepend(const char *, void *);

static char LogDir[MaxPathSize];
static char linebuf[MaxPathSize];

static package_t Plist;

static lpkg_head_t lpfindq;
static lpkg_head_t lpdelq;

static void
sanity_check(const char *pkg)
{
	char *fname;

	fname = pkgdb_pkg_file(pkg, CONTENTS_FNAME);

	if (!fexists(fname)) {
		cleanup(0);
		errx(2, "installed package %s has no %s file!",
		     pkg, CONTENTS_FNAME);
	}

	free(fname);
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
 * match_installed_pkgs(), deppkgname is expanded from a (possible) pattern.
 */
static int
undepend(const char *deppkgname, void *vp)
{
	char *fname, *fname_tmp;
	char fbuf[MaxPathSize];
	const char *pkg2delname = vp;
	FILE *fp, *fpwr;

	fname = pkgdb_pkg_file(deppkgname, REQUIRED_BY_FNAME);
	fname_tmp = pkgdb_pkg_file(deppkgname, REQUIRED_BY_FNAME_TMP);

	if ((fp = fopen(fname, "r")) == NULL) {
		warnx("couldn't open dependency file `%s'", fname);
		free(fname);
		free(fname_tmp);
		return 0;
	}
	if ((fpwr = fopen(fname_tmp, "w")) == NULL) {
		fclose(fp);
		warnx("couldn't open temporary file `%s'", fname_tmp);
		free(fname);
		free(fname_tmp);
		return 0;
	}
	while (fgets(fbuf, sizeof(fbuf), fp) != NULL) {
		if (fbuf[strlen(fbuf) - 1] == '\n')
			fbuf[strlen(fbuf) - 1] = '\0';
		if (strcmp(fbuf, pkg2delname))	/* no match */
			fputs(fbuf, fpwr), putc('\n', fpwr);
	}
	(void) fclose(fp);
	if (fclose(fpwr) == EOF) {
		warnx("error closing temporary file `%s'", fname_tmp);
		remove(fname_tmp);
		free(fname);
		free(fname_tmp);
		return 0;
	}
	if (rename(fname_tmp, fname) == -1)
		warn("error renaming `%s' to `%s'", fname_tmp, fname);
	remove(fname_tmp);	/* just in case */

	free(fname);
	free(fname_tmp);

	return 0;
}

/*
 * Remove the current view's package dbdir from the +VIEWS file of the
 * depoted package named by pkgname.
 */
static int
unview(const char *pkgname)
{
	const char *dbdir;
	char *fname, *fname_tmp;
	char  fbuf[MaxPathSize];
	FILE *fp, *fpwr;
	int  rv;
	int  cc;

	dbdir = _pkgdb_getPKGDB_DIR();

	fname = pkgdb_pkg_file(pkgname, DEPOT_FNAME);
	if ((fp = fopen(fname, "r")) == NULL) {
		warnx("unable to open `%s' file", fname);
		return -1;
	}
	if (fgets(fbuf, sizeof(fbuf), fp) == NULL) {
		(void) fclose(fp);
		warnx("empty depot file `%s'", fname);
		free(fname);
		return -1;
	}
	if (fbuf[cc = strlen(fbuf) - 1] == '\n') {
		fbuf[cc] = 0;
	}
	fclose(fp);
	free(fname);

	/*
	 * Copy the contents of the +VIEWS file into a temp file, but
	 * skip copying the name of the current view's package dbdir.
	 */
	fname = pkgdb_pkg_file(pkgname, VIEWS_FNAME);
	fname_tmp = pkgdb_pkg_file(pkgname, VIEWS_FNAME_TMP);
	if ((fp = fopen(fname, "r")) == NULL) {
		warnx("unable to open `%s' file", fname);
		free(fname);
		free(fname_tmp);
		return -1;
	}
	if ((fpwr = fopen(fname_tmp, "w")) == NULL) {
		(void) fclose(fp);
		warnx("unable to fopen `%s' temporary file", fname_tmp);
		free(fname);
		free(fname_tmp);
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

	if (fclose(fpwr) == EOF) {
		remove(fname_tmp);
		warnx("unable to close `%s' temp file", fname_tmp);
		free(fname);
		free(fname_tmp);
		return -1;
	}

	/* Rename the temp file to the +VIEWS file */
	if ((rv = rename(fname_tmp, fname)) == -1)
		warnx("unable to rename `%s' to `%s'", fname_tmp, fname);

	remove(fname_tmp);
	free(fname);
	free(fname_tmp);

	return rv;
}

/*
 * Delete from directory 'home' all packages on lpkg_list. 
 * If tryall is set, ignore errors from pkg_delete(1).
 */
static int
require_delete(int tryall)
{
	char *best_installed;
	lpkg_t *lpp;
	int     rv, fail;

	best_installed = NULL;

	/* walk list of things to delete */
	fail = 0;
	lpp = TAILQ_FIRST(&lpdelq);
	for (; lpp; lpp = TAILQ_NEXT(lpp, lp_link)) {
		free(best_installed);
		best_installed = NULL;
		
		/* look to see if package was already deleted */
		best_installed = find_best_matching_installed_pkg(lpp->lp_name);
		if (best_installed == NULL) {
			warnx("%s appears to have been deleted", lpp->lp_name);
			continue;
		}

		if (Verbose)
			printf("deinstalling %s\n", best_installed);

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
					     NoDeleteFiles ? "-N" : "",
					     CleanDirs ? "-d" : "",
					     Fake ? "-n" : "",
					     best_installed, NULL);

		/* check for delete failure */
		if (rv && !tryall) {
			fail = 1;
			warnx("had problem removing %s%s", best_installed,
			    Force ? ", continuing" : "");
			if (!Force)
				break;
		}
	}

	free(best_installed);

	/* cleanup list */
	while ((lpp = TAILQ_FIRST(&lpdelq))) {
		TAILQ_REMOVE(&lpdelq, lpp, lp_link);
		free_lpkg(lpp);
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
	char *fname;
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

	fname = pkgdb_pkg_file(thislpp->lp_name, REQUIRED_BY_FNAME);

	/* terminate recursion if no required by's */
	if (isemptyfile(fname)) {
		free(fname);
		return (0);
	}

	/* get packages that directly require us */
	cfile = fopen(fname, "r");
	if (!cfile) {
		warnx("cannot open requirements file `%s'", fname);
		free(fname);
		return (1);
	}
	free(fname);
	
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
		char *best_installed, *fname;

		/* remove a direct req from our queue */
		TAILQ_REMOVE(&reqq, lpp, lp_link);

		/* prepare for recursion */
		best_installed = find_best_matching_installed_pkg(lpp->lp_name);

		if (best_installed == NULL) {
			warnx("cannot remove dependency for pkg-pattern %s",
			    lpp->lp_name);
			fail = 1;
			goto fail; 
		}
		sanity_check(best_installed);

		fname = pkgdb_pkg_file(best_installed, CONTENTS_FNAME);
		free(best_installed);
		cfile = fopen(fname, "r");
		if (!cfile) {
			warn("unable to open '%s' file", fname);
			free(fname);
			fail = 1;
			goto fail;
		}
		free(fname);
		read_plist(&rPlist, cfile);
		fclose(cfile);
		/* If we have a prefix, replace the first @cwd. */
		if (Prefix) {
			delete_plist(&Plist, FALSE, PLIST_CWD, NULL);
			add_plist_top(&Plist, PLIST_CWD, Prefix);
		}
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
	char	       *pkgdir, *fname;
	plist_t	       *p;
	FILE	       *cfile;
	FILE	       *fp;
	int		cc;
	Boolean		is_depoted_pkg = FALSE;

	/* Reset some state */
	if (Plist.head)
		free_plist(&Plist);

	pkgdir = xasprintf("%s/%s", _pkgdb_getPKGDB_DIR(), pkg);
	if (!fexists(pkgdir) || !(isdir(pkgdir) || islinktodir(pkgdir))) {
		/* Check if the given package name matches something
		 * with 'pkg-[0-9]*' */
		lpkg_head_t     trypkgs;
		lpkg_t	       *lpp;
		int		qlen = 0;

		free(pkgdir);

		TAILQ_INIT(&trypkgs);

		switch (add_installed_pkgs_by_basename(pkg, &trypkgs)) {
		case 0:
			warnx("package '%s' not installed", pkg);
			return 1;
		case -1:
			errx(EXIT_FAILURE, "Error during search in pkgdb for %s", pkg);
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
	free(pkgdir);
	setenv(PKG_REFCOUNT_DBDIR_VNAME, pkgdb_refcount_dir(), 1);
	fname = pkgdb_pkg_file(pkg, CONTENTS_FNAME);
	if (!fexists(fname)) {
		warnx("package '%s' is not installed, %s missing", pkg, fname);
		if (!Force) {
			free(fname);
			return 1;
		}
	}
	free(fname);

	fname = pkgdb_pkg_file(pkg, PRESERVE_FNAME);
	if (fexists(fname)) {
		printf("Package `%s' is marked as not for deletion\n", pkg);
		if (Force <= (NoDeleteFiles ? 0 : 1)) {
			free(fname);
			return 1;
		}
		printf("Deleting anyway\n");
	}
	free(fname);

	fname = pkgdb_pkg_file(pkg, REQUIRED_BY_FNAME);
	if (!isemptyfile(fname)) {
		/* This package is required by others. Either nuke
		 * them (-r), or stop. */
		if (!Recurse_up)
			warnx("package `%s' is required by other packages:", pkg);
		else if (Verbose)
			printf("Building list of packages that require `%s'"
			    " to deinstall\n", pkg);
		if (require_find(pkg, FIND_UP)) {
			if (!Force || Recurse_up) {
				free(fname);
				return (1);
			}
		}
		if (!Recurse_up) {
			require_print();
			if (!Force) {
				free(fname);
				return 1;
			}
		} else
			require_delete(0);
	}
	free(fname);

	fname = pkgdb_pkg_file(pkg, VIEWS_FNAME);
	if (!isemptyfile(fname)) {
		char view[MaxPathSize];

		/* This package has instances in other views */
		/* Delete them from the views */
		if ((fp = fopen(fname, "r")) == NULL) {
			warnx("unable to open '%s' file", fname);
			free(fname);
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
				free(fname);
				return 1;
			}
		}
		(void) fclose(fp);
	}
	free(fname);

	sanity_check(pkg);
	fname = pkgdb_pkg_file(pkg, CONTENTS_FNAME);
	if ((cfile = fopen(fname, "r")) == NULL) {
		warnx("unable to open '%s' file", fname);
		free(fname);
		return 1;
	}
	free(fname);

	read_plist(&Plist, cfile);
	fclose(cfile);
	p = find_plist(&Plist, PLIST_CWD);
	/* If we have a prefix, replace the first @cwd. */
	if (Prefix) {
		delete_plist(&Plist, FALSE, PLIST_CWD, NULL);
		add_plist_top(&Plist, PLIST_CWD, Prefix);
	}
	if (!p) {
		warnx("package '%s' doesn't have a prefix", pkg);
		return 1;
	}
	if (Destdir != NULL)
		setenv(PKG_DESTDIR_VNAME, Destdir, 1);
	setenv(PKG_PREFIX_VNAME, p->name, 1);
	setenv(PKG_METADATA_DIR_VNAME, LogDir, 1);
	/*
	 * Ensure that we don't do VIEW-DEINSTALL action for old packages
	 * or for the package in its depot directory.
	 */
	fname = pkgdb_pkg_file(pkg, DEINSTALL_FNAME);
	if (!NoDeInstall && fexists(fname)) {
		const char *target, *text;
		char *fname2;

		fname2 = pkgdb_pkg_file(pkg, DEPOT_FNAME);
		if (fexists(fname2)) {
			target = "VIEW-DEINSTALL";
			text = "view deinstall";
		} else {
			target = "DEINSTALL";
			text = "deinstall";
		}
		free(fname2);

		if (Fake) {
			printf("Would execute %s script at this point "
			    "(arg: %s).\n", text, target);
		} else {
			pkgdir = xasprintf("%s/%s", _pkgdb_getPKGDB_DIR(), pkg);
			if (chmod(fname, 0555))
				warn("chmod of %s failed", fname);
			if (fcexec(pkgdir, fname, pkg, target, NULL)) {
				warnx("%s script returned error status", text);
				if (!Force) {
					free(pkgdir);
					return 1;
				}
			}
			free(pkgdir);
		}
	}
	free(fname);

	if (!Fake) {
		/* Some packages aren't packed right, so we need to just ignore delete_package()'s status.  Ugh! :-( */
		if (delete_package(FALSE, CleanDirs, &Plist, NoDeleteFiles, Destdir) == FAIL)
			warnx("couldn't entirely delete package `%s'\n", pkg);
	} else {   /* Fake means Verbose */
		printf("Attempting to delete package `%s'\n", pkg);
	}
	fname = pkgdb_pkg_file(pkg, DEPOT_FNAME);
	if (!isemptyfile(fname)) {
		if (Verbose)
			printf("Attempting to remove the %s registration"
			    " on package `%s'\n", fname, pkg);
		if (!Fake)
			(void) unview(pkg);
	}
	/*
	 * If this isn't a package in a view, then remove this package
	 * from the +REQUIRED_BY list of the packages this depends on.
	 */
	if (!fexists(fname)) {
		for (p = Plist.head; p; p = p->next) {
			if (p->type != PLIST_PKGDEP)
				continue;
			if (Verbose)
				printf("Attempting to remove dependency on package `%s'\n", p->name);
			if (!Fake)
				match_installed_pkgs(p->name, undepend, pkg);
		}
	}
	free(fname);
	if (Recurse_down) {
		/* Also remove the packages further down, now that there's
		 * (most likely) nothing left which requires them. */
		if (Verbose)
			printf("Building list of packages that `%s' required\n", pkg);
		if (require_find(pkg, FIND_DOWN))
			return (1);

		require_delete(1);
	}

	fname = pkgdb_pkg_file(pkg, DEINSTALL_FNAME);
	if (!NoDeInstall && fexists(fname)) {
		char *fname2;

		fname2 = pkgdb_pkg_file(pkg, DEPOT_FNAME);
		if (!fexists(fname2)) {
			if (Fake)
				printf("Would execute post-de-install script at this point (arg: POST-DEINSTALL).\n");
			else {
				pkgdir = xasprintf("%s/%s", _pkgdb_getPKGDB_DIR(), pkg);
				if (chmod(fname, 0555))
					warn("chmod of %s failed", fname);
				if (fcexec(pkgdir, fname, pkg, "POST-DEINSTALL", NULL)) {
					warnx("post-deinstall script returned error status");
					if (!Force) {
						free(pkgdir);
						free(fname);
						free(fname2);
						return 1;
					}
				}
				free(pkgdir);
			}
		}
		free(fname2);
	}
	free(fname);

	fname = pkgdb_pkg_file(pkg, VIEWS_FNAME);
	if (fexists(fname))
		is_depoted_pkg = TRUE;
	free(fname);

	/* Change out of LogDir before we remove it.
	 * Do not fail here, as the package is not yet completely deleted! */
	if (!Fake) {
		/* Finally nuke the +-files and the pkgdb-dir (/var/db/pkg/foo) */
		pkgdir = xasprintf("%s/%s", _pkgdb_getPKGDB_DIR(), pkg);
		(void) remove_files(pkgdir, "+*");
		if (isemptydir(pkgdir))
			(void)rmdir(pkgdir);
		else if (is_depoted_pkg)
			warnx("%s is not empty", pkgdir);
		else if (Force) {
			if (recursive_remove(pkgdir, 1)) {
				warn("Couldn't remove log entry in %s", pkgdir);
				free(pkgdir);
				return 1;
			} else {
				warnx("log entry forcefully removed in %s", pkgdir);
				free(pkgdir);
				return 0;
			}
		} else {
			warnx("couldn't remove log entry in %s", pkgdir);
			free(pkgdir);
			return 1;
		}
		free(pkgdir);
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
