/*	$NetBSD: perform.c,v 1.20 1999/11/29 19:48:45 hubertf Exp $	*/

#include <sys/cdefs.h>
#ifndef lint
#if 0
static const char *rcsid = "from FreeBSD Id: perform.c,v 1.38 1997/10/13 15:03:51 jkh Exp";
#else
__RCSID("$NetBSD: perform.c,v 1.20 1999/11/29 19:48:45 hubertf Exp $");
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
 * This is the main body of the create module.
 *
 */

#include "lib.h"
#include "create.h"

#include <err.h>
#include <signal.h>
#include <sys/syslimits.h>
#include <sys/wait.h>
#include <unistd.h>

static char *home;

static void
make_dist(char *home, char *pkg, char *suffix, package_t *plist)
{
	char    tball[FILENAME_MAX];
	plist_t *p;
	int     ret;
	char   *args[50];	/* Much more than enough. */
	int     nargs = 0;
	int     pipefds[2];
	FILE   *totar;
	pid_t   pid;

	args[nargs++] = TAR_CMD;/* argv[0] */

	if (*pkg == '/')
		(void) snprintf(tball, sizeof(tball), "%s.%s", pkg, suffix);
	else
		(void) snprintf(tball, sizeof(tball), "%s/%s.%s", home, pkg, suffix);

	args[nargs++] = "-c";
	args[nargs++] = "-f";
	args[nargs++] = tball;
	if (strchr(suffix, 'z'))/* Compress/gzip? */
		args[nargs++] = "-z";
	if (Dereference)
		args[nargs++] = "-h";
	if (ExcludeFrom) {
		args[nargs++] = "-X";
		args[nargs++] = ExcludeFrom;
	}
	args[nargs++] = "-T";	/* Take filenames from file instead of args. */
	args[nargs++] = "-";	/* Use stdin for the file. */
	args[nargs] = NULL;

	if (Verbose)
		printf("Creating gzip'd %s ball in '%s'\n", TAR_CMD, tball);

	/* Set up a pipe for passing the filenames, and fork off a tar process. */
	if (pipe(pipefds) == -1) {
		cleanup(0);
		errx(2, "cannot create pipe");
	}
	if ((pid = fork()) == -1) {
		cleanup(0);
		errx(2, "cannot fork process for %s", TAR_CMD);
	}
	if (pid == 0) {		/* The child */
		dup2(pipefds[0], 0);
		close(pipefds[0]);
		close(pipefds[1]);
		execv(TAR_FULLPATHNAME, args);
		cleanup(0);
		errx(2, "failed to execute %s command", TAR_CMD);
	}

	/* Meanwhile, back in the parent process ... */
	close(pipefds[0]);
	if ((totar = fdopen(pipefds[1], "w")) == NULL) {
		cleanup(0);
		errx(2, "fdopen failed");
	}

	fprintf(totar, "%s\n", CONTENTS_FNAME);
	fprintf(totar, "%s\n", COMMENT_FNAME);
	fprintf(totar, "%s\n", DESC_FNAME);

	if (Install) {
		fprintf(totar, "%s\n", INSTALL_FNAME);
	}
	if (DeInstall) {
		fprintf(totar, "%s\n", DEINSTALL_FNAME);
	}
	if (Require) {
		fprintf(totar, "%s\n", REQUIRE_FNAME);
	}
	if (Display) {
		fprintf(totar, "%s\n", DISPLAY_FNAME);
	}
	if (Mtree) {
		fprintf(totar, "%s\n", MTREE_FNAME);
	}
	if (BuildVersion) {
		(void) fprintf(totar, "%s\n", BUILD_VERSION_FNAME);
	}
	if (BuildInfo) {
		(void) fprintf(totar, "%s\n", BUILD_INFO_FNAME);
	}
	if (SizePkg) {
		(void) fprintf(totar, "%s\n", SIZE_PKG_FNAME);
	}
	if (SizeAll) {
		(void) fprintf(totar, "%s\n", SIZE_ALL_FNAME);
	}

	for (p = plist->head; p; p = p->next) {
		if (p->type == PLIST_FILE)
			fprintf(totar, "%s\n", p->name);
		else if (p->type == PLIST_CWD)
			fprintf(totar, "-C\n%s\n", p->name);
		else if (p->type == PLIST_IGNORE)
			p = p->next;
	}

	fclose(totar);
	wait(&ret);
	/* assume either signal or bad exit is enough for us */
	if (ret) {
		cleanup(0);
		errx(2, "%s command failed with code %d", TAR_CMD, ret);
	}
}

static void
sanity_check(void)
{
	if (!Comment) {
		cleanup(0);
		errx(2, "required package comment string is missing (-c comment)");
	}
	if (!Desc) {
		cleanup(0);
		errx(2, "required package description string is missing (-d desc)");
	}
	if (!Contents) {
		cleanup(0);
		errx(2, "required package contents list is missing (-f [-]file)");
	}
}


/*
 * Clean up those things that would otherwise hang around
 */
void
cleanup(int sig)
{
	static int alreadyCleaning;
	void    (*oldint) (int);
	void    (*oldhup) (int);
	oldint = signal(SIGINT, SIG_IGN);
	oldhup = signal(SIGHUP, SIG_IGN);

	if (!alreadyCleaning) {
		alreadyCleaning = 1;
		if (sig)
			printf("Signal %d received, cleaning up.\n", sig);
		leave_playpen(home);
		if (sig)
			exit(1);
	}
	signal(SIGINT, oldint);
	signal(SIGHUP, oldhup);
}

int
pkg_perform(lpkg_head_t *pkgs)
{
	char   *pkg;
	char   *cp;
	FILE   *pkg_in, *fp;
	package_t plist;
	char   *suffix;		/* What we tack on to the end of the finished package */
	lpkg_t *lpp;

	lpp = TAILQ_FIRST(pkgs);
	pkg = lpp->lp_name;	/* Only one arg to create */

	/* Preliminary setup */
	sanity_check();
	if (Verbose && !PlistOnly)
		printf("Creating package %s\n", pkg);
	get_dash_string(&Comment);
	get_dash_string(&Desc);
	if (!strcmp(Contents, "-"))
		pkg_in = stdin;
	else {
		pkg_in = fopen(Contents, "r");
		if (!pkg_in) {
			cleanup(0);
			errx(2, "unable to open contents file '%s' for input", Contents);
		}
	}
	plist.head = plist.tail = NULL;

	/* Break the package name into base and desired suffix (if any) */
	if ((cp = strrchr(pkg, '.')) != NULL) {
		suffix = cp + 1;
		*cp = '\0';
	} else
		suffix = "tgz";

	/* Stick the dependencies, if any, at the top */
	if (Pkgdeps) {
		if (Verbose && !PlistOnly)
			printf("Registering depends:");
		while (Pkgdeps) {
			cp = strsep(&Pkgdeps, " \t\n");
			if (*cp) {
				add_plist(&plist, PLIST_PKGDEP, cp);
				if (Verbose && !PlistOnly)
					printf(" %s", cp);
			}
		}
		if (Verbose && !PlistOnly)
			printf(".\n");
	}

	/* Put the conflicts directly after the dependencies, if any */
	if (Pkgcfl) {
		if (Verbose && !PlistOnly)
			printf("Registering conflicts:");
		while (Pkgcfl) {
			cp = strsep(&Pkgcfl, " \t\n");
			if (*cp) {
				add_plist(&plist, PLIST_PKGCFL, cp);
				if (Verbose && !PlistOnly)
					printf(" %s", cp);
			}
		}
		if (Verbose && !PlistOnly)
			printf(".\n");
	}

	/* Slurp in the packing list */
	read_plist(&plist, pkg_in);

	/* Prefix should override the packing list */
	if (Prefix) {
		delete_plist(&plist, FALSE, PLIST_CWD, NULL);
		add_plist_top(&plist, PLIST_CWD, Prefix);
	}
	/*
         * Run down the list and see if we've named it, if not stick in a name
         * at the top.
         */
	if (find_plist(&plist, PLIST_NAME) == NULL) {
		add_plist_top(&plist, PLIST_NAME, basename_of(pkg));
	}

	/*
         * We're just here for to dump out a revised plist for the FreeBSD ports
         * hack.  It's not a real create in progress.
         */
	if (PlistOnly) {
		check_list(home, &plist, basename_of(pkg));
		write_plist(&plist, stdout);
		exit(0);
	}

	/* Make a directory to stomp around in */
	home = make_playpen(PlayPen, PlayPenSize, 0);
	signal(SIGINT, cleanup);
	signal(SIGHUP, cleanup);

	/* Make first "real contents" pass over it */
	check_list(home, &plist, basename_of(pkg));
	(void) umask(022);	/* make sure gen'ed directories, files
				 * don't have group or other write bits. */

	/* Now put the release specific items in */
	add_plist(&plist, PLIST_CWD, ".");
	write_file(COMMENT_FNAME, Comment);
	add_plist(&plist, PLIST_IGNORE, NULL);
	add_plist(&plist, PLIST_FILE, COMMENT_FNAME);
	write_file(DESC_FNAME, Desc);
	add_plist(&plist, PLIST_IGNORE, NULL);
	add_plist(&plist, PLIST_FILE, DESC_FNAME);

	if (Install) {
		copy_file(home, Install, INSTALL_FNAME);
		add_plist(&plist, PLIST_IGNORE, NULL);
		add_plist(&plist, PLIST_FILE, INSTALL_FNAME);
	}
	if (DeInstall) {
		copy_file(home, DeInstall, DEINSTALL_FNAME);
		add_plist(&plist, PLIST_IGNORE, NULL);
		add_plist(&plist, PLIST_FILE, DEINSTALL_FNAME);
	}
	if (Require) {
		copy_file(home, Require, REQUIRE_FNAME);
		add_plist(&plist, PLIST_IGNORE, NULL);
		add_plist(&plist, PLIST_FILE, REQUIRE_FNAME);
	}
	if (Display) {
		copy_file(home, Display, DISPLAY_FNAME);
		add_plist(&plist, PLIST_IGNORE, NULL);
		add_plist(&plist, PLIST_FILE, DISPLAY_FNAME);
		add_plist(&plist, PLIST_DISPLAY, DISPLAY_FNAME);
	}
	if (Mtree) {
		copy_file(home, Mtree, MTREE_FNAME);
		add_plist(&plist, PLIST_IGNORE, NULL);
		add_plist(&plist, PLIST_FILE, MTREE_FNAME);
		add_plist(&plist, PLIST_MTREE, MTREE_FNAME);
	}
	if (BuildVersion) {
		copy_file(home, BuildVersion, BUILD_VERSION_FNAME);
		add_plist(&plist, PLIST_IGNORE, NULL);
		add_plist(&plist, PLIST_FILE, BUILD_VERSION_FNAME);
	}
	if (BuildInfo) {
		copy_file(home, BuildInfo, BUILD_INFO_FNAME);
		add_plist(&plist, PLIST_IGNORE, NULL);
		add_plist(&plist, PLIST_FILE, BUILD_INFO_FNAME);
	}
	if (SizePkg) {
		copy_file(home, SizePkg, SIZE_PKG_FNAME);
		add_plist(&plist, PLIST_IGNORE, NULL);
		add_plist(&plist, PLIST_FILE, SIZE_PKG_FNAME);
	}
	if (SizeAll) {
		copy_file(home, SizeAll, SIZE_ALL_FNAME);
		add_plist(&plist, PLIST_IGNORE, NULL);
		add_plist(&plist, PLIST_FILE, SIZE_ALL_FNAME);
	}

	/* Finally, write out the packing list */
	fp = fopen(CONTENTS_FNAME, "w");
	if (!fp) {
		cleanup(0);
		errx(2, "can't open file %s for writing", CONTENTS_FNAME);
	}
	write_plist(&plist, fp);
	if (fclose(fp)) {
		cleanup(0);
		errx(2, "error while closing %s", CONTENTS_FNAME);
	}

	/* And stick it into a tar ball */
	make_dist(home, pkg, suffix, &plist);

	/* Cleanup */
	free(Comment);
	free(Desc);
	free_plist(&plist);
	leave_playpen(home);
	
	return TRUE;		/* Success */
}
