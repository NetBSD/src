/*	$NetBSD: perform.c,v 1.16 1999/03/03 20:12:06 hubertf Exp $	*/

#include <sys/cdefs.h>
#ifndef lint
#if 0
static const char *rcsid = "from FreeBSD Id: perform.c,v 1.15 1997/10/13 15:03:52 jkh Exp";
#else
__RCSID("$NetBSD: perform.c,v 1.16 1999/03/03 20:12:06 hubertf Exp $");
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

#include <sys/queue.h>
#include <err.h>
#include "lib.h"
#include "delete.h"

typedef struct _rec_del_t {
    TAILQ_ENTRY(_rec_del_t)	rd_link;
    char	*rd_name;
} rec_del_t;

TAILQ_HEAD(_rec_del_head_t, _rec_del_t);
typedef struct _rec_del_head_t rec_del_head_t;

rec_del_t *alloc_rec_del(char *);
rec_del_t *find_on_queue(rec_del_head_t *, char *);
void free_rec_del(rec_del_t *);
int recurse_require_find(rec_del_t *);
int require_find(char *, char *);
int require_delete(char *);
void require_print(int);
int undepend(const char *deppkgname, char *pkg2delname);

static char LogDir[FILENAME_MAX];
static char linebuf[FILENAME_MAX];
static char pkgdir[FILENAME_MAX];

static package_t Plist;

rec_del_head_t rdfindq;
rec_del_head_t rddelq;

static void
sanity_check(char *pkg)
{
    if (!fexists(CONTENTS_FNAME)) {
	cleanup(0);
	errx(2, "installed package %s has no %s file!", pkg, CONTENTS_FNAME);
    }
}

void
cleanup(int sig)
{
    /* Nothing to do */
    if(sig)	/* in case this is ever used as a signal handler */
	exit(1);
}

/* deppkgname is the pkg from which's +REQUIRED_BY file we are
 * about to remove pkg2delname. This function is called from
 * findmatchingname(), deppkgname is expanded from a (possible) pattern.
 */
int
undepend(const char *deppkgname, char *pkg2delname)
{
     char fname[FILENAME_MAX], ftmp[FILENAME_MAX];
     char fbuf[FILENAME_MAX];
     FILE *fp, *fpwr;
     char *tmp;
     int s;

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
	 if (fbuf[strlen(fbuf)-1] == '\n')
	     fbuf[strlen(fbuf)-1] = '\0';
	 if (strcmp(fbuf, pkg2delname))		/* no match */
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
     remove(ftmp);			/* just in case */
     
     return 0;
}

/* add a package to the recursive delete list */
rec_del_t *
alloc_rec_del(char *pkgname)
{
    rec_del_t *rdp;

    if ((rdp = malloc(sizeof(*rdp))) == 0)
	err(1, "cannot allocate recursion data");
    if ((rdp->rd_name = strdup(pkgname)) == 0)
	err(1, "cannot allocate recursion data");
    return (rdp);
}

void
free_rec_del(rec_del_t *rdp)
{
    free(rdp->rd_name);
    free(rdp);
}

rec_del_t *
find_on_queue(rec_del_head_t *qp, char *name)
{
    rec_del_t *rdp;

    for (rdp = TAILQ_FIRST(qp); rdp; rdp = TAILQ_NEXT(rdp, rd_link))
	if (!strcmp(name, rdp->rd_name))
	    return (rdp);
    return (0);
}


/* delete from 'home' all packages on rec_del_list */
int
require_delete(char *home)
{
    rec_del_t *rdp;
    int rv, fail;
    char *tmp;

    (void)snprintf(pkgdir, sizeof(pkgdir), "%s",
	(tmp = getenv(PKG_DBDIR)) ? tmp : DEF_LOG_DIR);

    /* walk list of things to delete */
    fail = 0;
    rdp = TAILQ_FIRST(&rddelq);
    for (; rdp; rdp = TAILQ_NEXT(rdp, rd_link)) {
	/* go to the db dir */
	if (chdir(pkgdir) == FAIL) {
	    warnx("unable to change directory to %s, deinstall failed",
		pkgdir);
	    fail = 1;
	    break;
	}

	/* look to see if package was already deleted */
	if (!fexists(rdp->rd_name)) {
	    warnx("%s appears to have been deleted", rdp->rd_name);
	    continue;
	}

	/* return home for execution of command */
	if (chdir(home) == FAIL) {
	    warnx("unable to change directory to %s! deinstall failed", home);
	    fail = 1;
	    break;
	}

	if (Verbose)
		printf("deinstalling %s\n", rdp->rd_name);

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
		rdp->rd_name);

	/* check for delete failure */
	if (rv) {
	    fail = 1;
	    warnx("had problem removing %s%s", rdp->rd_name,
		Force ? ", continuing" : "");
	    if (!Force)
		break;
	}
    }

    /* cleanup list */
    while ((rdp = TAILQ_FIRST(&rddelq))) {
	TAILQ_REMOVE(&rddelq, rdp, rd_link);
	free_rec_del(rdp);
    }

    /* return to the log dir */
    if (chdir(LogDir) == FAIL) {
	warnx("unable to change directory to %s! deinstall failed", LogDir);
	fail = 1;
    }

    return (fail);
}

/* recursively find all packages, return 1 on errors */
int
recurse_require_find(rec_del_t *thisrdp)
{
    rec_del_head_t reqq;
    rec_del_t *rdp = NULL;
    FILE *cfile;
    char *nl, *tmp;

    /* see if we are on the find queue -- circular dependency */
    if ((rdp = find_on_queue(&rdfindq, thisrdp->rd_name))) {
	warnx("circular dependency found for pkg %s", rdp->rd_name);
	return (1);
    }

    TAILQ_INIT(&reqq);

    (void)snprintf(pkgdir, sizeof(pkgdir), "%s/%s",
	(tmp = getenv(PKG_DBDIR)) ? tmp : DEF_LOG_DIR, thisrdp->rd_name);

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
	rdp = alloc_rec_del(linebuf);
	TAILQ_INSERT_TAIL(&reqq, rdp, rd_link);
    }
    fclose(cfile);

    /* put ourselves on the top of the find queue */
    TAILQ_INSERT_HEAD(&rdfindq, thisrdp, rd_link);

    while ((rdp = TAILQ_FIRST(&reqq))) {
	/* remove a direct req from our queue */
	TAILQ_REMOVE(&reqq, rdp, rd_link);

	/* find direct required requires */
	if (recurse_require_find(rdp))
	    goto fail;

	/*
	 * all requires taken care of, add to tail of delete queue
	 * if not already there
	 */
	if (find_on_queue(&rddelq, rdp->rd_name))
	    free_rec_del(rdp);
	else
	    TAILQ_INSERT_TAIL(&rddelq, rdp, rd_link);
    }

    /* take ourselves off the find queue */
    TAILQ_REMOVE(&rdfindq, thisrdp, rd_link);

    return (0);

fail:
    while ((rdp = TAILQ_FIRST(&reqq))) {
	TAILQ_REMOVE(&reqq, rdp, rd_link);
	free_rec_del(rdp);
    }
    return (1);
}

int
require_find(char *home, char *pkg)
{
    rec_del_t *rdp;
    int rv;

    TAILQ_INIT(&rdfindq);
    TAILQ_INIT(&rddelq);

    rdp = alloc_rec_del(pkg);
    rv = recurse_require_find(rdp);
    free_rec_del(rdp);

    return (rv);
}

void
require_print(int del)
{
    rec_del_t *rdp;

    /* print all but last -- deleting if requested */
    if (del) {
	while ((rdp = TAILQ_FIRST(&rddelq))) {
	    TAILQ_REMOVE(&rddelq, rdp, rd_link);
	    fprintf(stderr, "\t%s\n", rdp->rd_name);
	    free_rec_del(rdp);
	}
    } else {
	for (rdp = TAILQ_FIRST(&rddelq); rdp; rdp = TAILQ_NEXT(rdp, rd_link))
	    fprintf(stderr, "\t%s\n", rdp->rd_name);
    }
}

/* This is seriously ugly code following.  Written very fast! */
static int
pkg_do(char *pkg)
{
    FILE *cfile;
    char home[FILENAME_MAX];
    plist_t *p;
    char *tmp;

    /* Reset some state */
    if (Plist.head)
	free_plist(&Plist);

    (void) snprintf(LogDir, sizeof(LogDir), "%s/%s", (tmp = getenv(PKG_DBDIR)) ? tmp : DEF_LOG_DIR,
    	    pkg);
    if (!fexists(LogDir)) {
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
	if (!Recurse)
		warnx("package `%s' is required by other packages:", pkg);
	else if (Verbose)
		printf("building list of packages that require `%s'"
		    " to deinstall\n", pkg);
	if (require_find(home, pkg)) {
		if (!Force || Recurse)
			return (1);
	}
	if (!Recurse) {
	    require_print(1);
	    return 1;
	} else
	    require_delete(home);
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
	vsystem("chmod +x %s", REQUIRE_FNAME);	/* be sure */
	if (vsystem("./%s %s DEINSTALL", REQUIRE_FNAME, pkg)) {
	    warnx("package %s fails requirements %s", pkg,
		   Force ? "" : "- not deleted");
	    if (!Force)
		return 1;
	}
    }
    if (!NoDeInstall && fexists(DEINSTALL_FNAME)) {
	if (Fake)
	    printf("Would execute de-install script at this point.\n");
	else {
	    vsystem("chmod +x %s", DEINSTALL_FNAME);	/* make sure */
	    if (vsystem("./%s %s DEINSTALL", DEINSTALL_FNAME, pkg)) {
		warnx("deinstall script returned error status");
		if (!Force)
		    return 1;
	    }
	}
    }
    if (chdir(home) == FAIL) {
	cleanup(0);
	errx(2, "Toto! This doesn't look like Kansas anymore!");
    }
    if (!Fake) {
	/* Some packages aren't packed right, so we need to just ignore delete_package()'s status.  Ugh! :-( */
	if (delete_package(FALSE, CleanDirs, &Plist) == FAIL)
	    warnx(
	"couldn't entirely delete package (perhaps the packing list is\n"
	"incorrectly specified?)");
	if (vsystem("%s -r %s", REMOVE_CMD, LogDir)) {
	    warnx("couldn't remove log entry in %s, deinstall failed", LogDir);
	    if (!Force)
		return 1;
	}
    }
    for (p = Plist.head; p ; p = p->next) {
	if (p->type != PLIST_PKGDEP)
	    continue;
	if (Verbose)
	    printf("Attempting to remove dependency on package `%s'\n", p->name);
	if (!Fake)
	    findmatchingname((tmp = getenv(PKG_DBDIR)) ? tmp : DEF_LOG_DIR,
			     p->name, undepend, pkg);
    }
    return 0;
}

int
pkg_perform(char **pkgs)
{
    int i, err_cnt = 0;

    for (i = 0; pkgs[i]; i++)
	err_cnt += pkg_do(pkgs[i]);
    return err_cnt;
}
