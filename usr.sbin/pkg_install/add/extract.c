/*	$NetBSD: extract.c,v 1.38 2006/05/11 23:50:15 mrg Exp $	*/

#if HAVE_CONFIG_H
#include "config.h"
#endif
#include <nbcompat.h>
#if HAVE_SYS_CDEFS_H
#include <sys/cdefs.h>
#endif
#if HAVE_SYS_QUEUE_H
#include <sys/queue.h>
#endif
#ifndef lint
#if 0
static const char *rcsid = "FreeBSD - Id: extract.c,v 1.17 1997/10/08 07:45:35 charnier Exp";
#else
__RCSID("$NetBSD: extract.c,v 1.38 2006/05/11 23:50:15 mrg Exp $");
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
 * This is the package extraction code for the add module.
 *
 */

#if HAVE_ERR_H
#include <err.h>
#endif
#include "lib.h"
#include "add.h"

lfile_head_t files;
lfile_head_t perms;

/* 
 * Copy files from staging area to todir.
 * This is only used when the files cannot be directory rename()ed.
 */
static void
pushout(char *todir)
{
	pipe_to_system_t	*pipe_to;
	char			*file_args[4];
	char			**perm_argv;
	int			perm_argc = 1;
	lfile_t			*lfp;
	int			count;

	/* set up arguments to run "pax -r -w -p e" */
	file_args[0] = (char *)strrchr(PAX_CMD, '/');
	if (file_args[0] == NULL)
		file_args[0] = PAX_CMD;
	else
		file_args[0]++;
	file_args[1] = "-rwpe";
	file_args[2] = todir;
	file_args[3] = NULL;

	/* count entries for files */
	count = 0;
	TAILQ_FOREACH(lfp, &files, lf_link)
		count++;

	if (count > 0)  {
		/* open pipe, feed it files, close pipe */
		pipe_to = pipe_to_system_begin(PAX_CMD, file_args, NULL);
		while ((lfp = TAILQ_FIRST(&files)) != NULL) {
			fprintf(pipe_to->fp, "%s\n", lfp->lf_name);
			TAILQ_REMOVE(&files, lfp, lf_link);
			free(lfp);
		}
		pipe_to_system_end(pipe_to);
        }
 
	/* count entries for permissions */
	count = 0;
	TAILQ_FOREACH(lfp, &perms, lf_link)
		count++;

	if (count > 0) {
		perm_argv = malloc((count + 1) * sizeof(char *));
		perm_argc = 0;
		TAILQ_FOREACH(lfp, &perms, lf_link)
			perm_argv[perm_argc++] = lfp->lf_name;
		perm_argv[perm_argc] = NULL;
		apply_perms(todir, perm_argv, perm_argc);

		/* empty the perm list */
		while ((lfp = TAILQ_FIRST(&perms)) != NULL) {
			TAILQ_REMOVE(&perms, lfp, lf_link);
			free(lfp);
		}
		free(perm_argv);
	}
}

static void
rollback(char *name, char *home, plist_t *start, plist_t *stop)
{
	plist_t *q;
	char    try[MaxPathSize], bup[MaxPathSize], *dir;

	dir = home;
	for (q = start; q != stop; q = q->next) {
		if (q->type == PLIST_FILE) {
			(void) snprintf(try, sizeof(try), "%s/%s", dir, q->name);
			if (make_preserve_name(bup, sizeof(bup), name, try) && fexists(bup)) {
#if HAVE_CHFLAGS
				(void) chflags(try, 0);
#endif
				(void) unlink(try);
				if (rename(bup, try))
					warnx("rollback: unable to rename %s back to %s", bup, try);
			}
		} else if (q->type == PLIST_CWD) {
			if (strcmp(q->name, "."))
				dir = q->name;
			else
				dir = home;
		}
	}
}


/*
 * Return 0 on error, 1 for success.
 */
int
extract_plist(char *home, package_t *pkg)
{
	plist_t *p = pkg->head;
	char   *last_file;
	char	*last_chdir;
	Boolean preserve;
	lfile_t	*lfp;

	TAILQ_INIT(&files);
	TAILQ_INIT(&perms);

	last_chdir = 0;
	preserve = find_plist_option(pkg, "preserve") ? TRUE : FALSE;

	/* Reset the world */
	Owner = NULL;
	Group = NULL;
	Mode = NULL;
	last_file = NULL;
	Directory = home;

	if (!NoRecord) {
		/* Open Package Database for writing */
		if (!pkgdb_open(ReadWrite)) {
			cleanup(0);
			err(EXIT_FAILURE, "can't open pkgdb");
		}
	}
	/* Do it */
	while (p) {
		char    cmd[MaxPathSize];

		switch (p->type) {
		case PLIST_NAME:
			PkgName = p->name;
			if (Verbose)
				printf("extract: Package name is %s\n", p->name);
			break;

		case PLIST_FILE:
			last_file = p->name;
			if (Verbose)
				printf("extract: %s/%s\n", Directory, p->name);
			if (!Fake) {
				char    try[MaxPathSize];

				if (strrchr(p->name, '\'')) {
					cleanup(0);
					errx(2, "Bogus filename \"%s\"", p->name);
				}

				/* first try to rename it into place */
				(void) snprintf(try, sizeof(try), "%s/%s", Directory, p->name);
				if (fexists(try)) {
#if HAVE_CHFLAGS
					(void) chflags(try, 0);	/* XXX hack - if truly immutable, rename fails */
#endif
					if (preserve && PkgName) {
						char    pf[MaxPathSize];

						if (make_preserve_name(pf, sizeof(pf), PkgName, try)) {
							if (rename(try, pf)) {
								warnx(
								    "unable to back up %s to %s, aborting pkg_add",
								    try, pf);
								rollback(PkgName, home, pkg->head, p);
								return 0;
							}
						}
					}
				}
				if (rename(p->name, try) == 0) {
					if (!NoRecord) {
						/* note in pkgdb */
						char   *s, t[MaxPathSize];
						int     rc;

						(void) snprintf(t, sizeof(t), "%s/%s", Directory, p->name);

						s = pkgdb_retrieve(t);
#ifdef PKGDB_DEBUG
						printf("pkgdb_retrieve(\"%s\")=\"%s\"\n", t, s);	/* pkgdb-debug - HF */
#endif
						if (s)
							warnx("Overwriting %s - pkg %s bogus/conflicting?", t, s);
						else {
							rc = pkgdb_store(t, PkgName);
#ifdef PKGDB_DEBUG
							printf("pkgdb_store(\"%s\", \"%s\") = %d\n", t, PkgName, rc);	/* pkgdb-debug - HF */
#endif

						}
					}

					/* try to add to list of perms to be changed and run in bulk. */
					if (p->name[0] == '/')
						pushout(Directory);

					LFILE_ADD(&perms, lfp, p->name);
				} else {
					/* rename failed, try copying with a big tar command */
					if (last_chdir != Directory) {
						pushout(last_chdir);
						last_chdir = Directory;
					} else if (p->name[0] == '/') {
						pushout(Directory);
					}

					if (!NoRecord) {
						/* note in pkgdb */
						/* XXX would be better to store in PUSHOUT, but
						 * that would probably affect too much code I prefer
						 * not to touch - HF */
						
						char   *s, t[MaxPathSize], *u;
						int     rc;

						LFILE_ADD(&files, lfp, p->name);
						LFILE_ADD(&perms, lfp, p->name);
						if (p->name[0] == '/')
							u = p->name;
						else {
							(void) snprintf(t, sizeof(t), "%s/%s", Directory, p->name);
							u = t;
						}

						s = pkgdb_retrieve(t);
#ifdef PKGDB_DEBUG
						printf("pkgdb_retrieve(\"%s\")=\"%s\"\n", t, s);	/* pkgdb-debug - HF */
#endif
						if (s)
							warnx("Overwriting %s - pkg %s bogus/conflicting?", t, s);
						else {
							rc = pkgdb_store(t, PkgName);
#ifdef PKGDB_DEBUG
							printf("pkgdb_store(\"%s\", \"%s\") = %d\n", t, PkgName, rc);	/* pkgdb-debug - HF */
#endif
						}
					}
				}
			}
			break;

		case PLIST_CWD:
			if (Verbose)
				printf("extract: CWD to %s\n", p->name);
			pushout(Directory);
			if (strcmp(p->name, ".")) {
				if (!Fake && make_hierarchy(p->name) == FAIL) {
					cleanup(0);
					errx(2, "unable to make directory '%s'", p->name);
				}
				Directory = p->name;
			} else
				Directory = home;
			break;

		case PLIST_CMD:
			format_cmd(cmd, sizeof(cmd), p->name, Directory, last_file);
			pushout(Directory);
			printf("Executing '%s'\n", cmd);
			if (!Fake && system(cmd))
				warnx("command '%s' failed", cmd);
			break;

		case PLIST_CHMOD:
			pushout(Directory);
			Mode = p->name;
			break;

		case PLIST_CHOWN:
			pushout(Directory);
			Owner = p->name;
			break;

		case PLIST_CHGRP:
			pushout(Directory);
			Group = p->name;
			break;

		case PLIST_COMMENT:
			break;

		case PLIST_IGNORE:
			p = p->next;
			break;

		default:
			break;
		}
		p = p->next;
	}
	pushout(Directory);
	if (!NoRecord)
		pkgdb_close();
	return 1;
}
