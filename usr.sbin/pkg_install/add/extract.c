/*	$NetBSD: extract.c,v 1.17 1999/05/29 20:44:20 christos Exp $	*/

#include <sys/cdefs.h>
#ifndef lint
#if 0
static const char *rcsid = "FreeBSD - Id: extract.c,v 1.17 1997/10/08 07:45:35 charnier Exp";
#else
__RCSID("$NetBSD: extract.c,v 1.17 1999/05/29 20:44:20 christos Exp $");
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

#include <err.h>
#include "lib.h"
#include "add.h"

#define TAR_ARGS	" cf - "

#define TOOBIG(str) ((strlen(str) + 22 + strlen(home) + where_count > maxargs) \
		|| (strlen(str) + 6 + strlen(home) + perm_count > maxargs))

#define PUSHOUT(todir) /* push out string */				\
        if (where_count > sizeof(TAR_CMD) + sizeof(TAR_ARGS)-1) {	\
		    strcat(where_args, "|");				\
		    strcat(where_args, TAR_CMD);			\
		    strcat(where_args, " xf - -C ");			\
		    strcat(where_args, todir);				\
		    if (system(where_args)) {				\
			cleanup(0);					\
			errx(2, "can not invoke %lu byte %s pipeline: %s", \
				(u_long)strlen(where_args), TAR_CMD,	\
				where_args);				\
		    }							\
		    strcpy(where_args, TAR_CMD);			\
		    strcat(where_args, TAR_ARGS);			\
		    where_count = sizeof(TAR_CMD) + sizeof(TAR_ARGS)-2;	\
	}								\
	if (perm_count) {						\
		    apply_perms(todir, perm_args);			\
		    perm_args[0] = 0;					\
		    perm_count = 0;					\
	}

static void
rollback(char *name, char *home, plist_t *start, plist_t *stop)
{
    plist_t *q;
    char try[FILENAME_MAX], bup[FILENAME_MAX], *dir;

    dir = home;
    for (q = start; q != stop; q = q->next) {
	if (q->type == PLIST_FILE) {
	    if (q->name[0] == '/')
		snprintf(try, FILENAME_MAX, "%s", q->name);
	    else
		snprintf(try, FILENAME_MAX, "%s/%s", dir, q->name);
	    if (make_preserve_name(bup, FILENAME_MAX, name, try) && fexists(bup)) {
		(void)chflags(try, 0);
		(void)unlink(try);
		if (rename(bup, try))
		    warnx("rollback: unable to rename %s back to %s", bup, try);
	    }
	}
	else if (q->type == PLIST_CWD) {
	    if (strcmp(q->name, "."))
		dir = q->name;
	    else
		dir = home;
	}
    }
}

void
extract_plist(char *home, package_t *pkg)
{
    plist_t *p = pkg->head;
    char *last_file;
    char *where_args, *perm_args, *last_chdir;
    int maxargs, where_count = 0, perm_count = 0, add_count;
    Boolean preserve;

    maxargs = sysconf(_SC_ARG_MAX) / 2;	/* Just use half the argument space */
    where_args = alloca(maxargs);
    if (!where_args) {
	cleanup(0);
	errx(2, "can't get argument list space");
    }
    perm_args = alloca(maxargs);
    if (!perm_args) {
	cleanup(0);
	errx(2, "can't get argument list space");
    }
    strcpy(where_args, TAR_CMD);
    strcat(where_args, TAR_ARGS);
    where_count = sizeof(TAR_CMD) + sizeof(TAR_ARGS) - 2;
    perm_args[0] = 0;

    last_chdir = 0;
    preserve = find_plist_option(pkg, "preserve") ? TRUE : FALSE;

    /* Reset the world */
    Owner = NULL;
    Group = NULL;
    Mode = NULL;
    last_file = NULL;
    Directory = home;

    /* Open Package Database for writing */
    if (pkgdb_open(0) == -1) {
	cleanup(0);
	err(1, "can't open pkgdb");
    }
    
    /* Do it */
    while (p) {
	char cmd[FILENAME_MAX];

	switch(p->type) {
	case PLIST_NAME:
	    PkgName = p->name;
	    if (Verbose)
		printf("extract: Package name is %s\n", p->name);
	    break;

	case PLIST_FILE:
	    last_file = p->name;
	    if (Verbose) {
		if (p->name[0] == '/')
		    printf("extract: %s\n", p->name);
		else
		    printf("extract: %s/%s\n", Directory, p->name);
	    }
	    if (!Fake) {
		char try[FILENAME_MAX];

		if (strrchr(p->name,'\'')) {
		  cleanup(0);
		  errx(2, "Bogus filename \"%s\"", p->name);
		}
		
		/* first try to rename it into place */
		if (p->name[0] == '/')
		    snprintf(try, FILENAME_MAX, "%s", p->name);
		else
		    snprintf(try, FILENAME_MAX, "%s/%s", Directory, p->name);
		if (fexists(try)) {
		    (void)chflags(try, 0);	/* XXX hack - if truly immutable, rename fails */
		    if (preserve && PkgName) {
			char pf[FILENAME_MAX];

			if (make_preserve_name(pf, FILENAME_MAX, PkgName, try)) {
			    if (rename(try, pf)) {
				warnx(
				"unable to back up %s to %s, aborting pkg_add",
				try, pf);
				rollback(PkgName, home, pkg->head, p);
				return;
			    }
			}
		    }
		}
		if (rename(p->name, try) == 0) {
		    /* note in pkgdb */
		    {
			char *s, t[FILENAME_MAX], *u;
			int rc;
			
			if (p->name[0] == '/')
			    u=p->name;
			else {
			    snprintf(t, FILENAME_MAX, "%s/%s", Directory, p->name);
			    u=t;
			}
			
			s=pkgdb_retrieve(u);
#ifdef PKGDB_DEBUG
 printf("pkgdb_retrieve(\"%s\")=\"%s\"\n", t, s); /* pkgdb-debug - HF */
#endif
			if (s)
			    warnx("Overwriting %s - pkg %s bogus/conflicting?", t, s);
			else {
			    rc=pkgdb_store(t, PkgName);
#ifdef PKGDB_DEBUG
 printf("pkgdb_store(\"%s\", \"%s\") = %d\n", t, PkgName, rc); /* pkgdb-debug - HF */
#endif
			    
			}
		    }
		    
		    /* try to add to list of perms to be changed and run in bulk. */
		    if (p->name[0] == '/' || TOOBIG(p->name)) {
			PUSHOUT(Directory);
		    }
		    add_count = snprintf(&perm_args[perm_count], maxargs - perm_count, "'%s' ", p->name);
		    if (add_count > maxargs - perm_count) {
			cleanup(0);
			errx(2, "oops, miscounted strings!");
		    }
		    perm_count += add_count;
		} else {
		    /* rename failed, try copying with a big tar command */
		    if (last_chdir != Directory) {
			PUSHOUT(last_chdir);
			last_chdir = Directory;
		    }
		    else if (p->name[0] == '/' || TOOBIG(p->name)) {
			PUSHOUT(Directory);
		    }
		    add_count = snprintf(&where_args[where_count], maxargs - where_count, " '%s'", p->name);
		    if (add_count > maxargs - where_count) {
			cleanup(0);
			errx(2, "oops, miscounted strings!");
		    }
		    where_count += add_count;
		    add_count = snprintf(&perm_args[perm_count],
					 maxargs - perm_count,
					 "'%s' ", p->name);
		    if (add_count > maxargs - perm_count) {
			cleanup(0);
			errx(2, "oops, miscounted strings!");
		    }
		    perm_count += add_count;

		    /* note in pkgdb */
		    /* XXX would be better to store in PUSHOUT, but
		       that would probably affect too much code I prefer
		       not to touch - HF */
		    {
			char *s, t[FILENAME_MAX], *u;
			int rc;

			if (p->name[0] == '/')
			    u=p->name;
			else {
			    snprintf(t, FILENAME_MAX, "%s/%s", Directory, p->name);
			    u=t;
			}
			
			s=pkgdb_retrieve(t);
#ifdef PKGDB_DEBUG
 printf("pkgdb_retrieve(\"%s\")=\"%s\"\n", t, s); /* pkgdb-debug - HF */
#endif
			if (s)
			    warnx("Overwriting %s - pkg %s bogus/conflicting?", t, s);
			else {
			    rc = pkgdb_store(t, PkgName);
#ifdef PKGDB_DEBUG
 printf("pkgdb_store(\"%s\", \"%s\") = %d\n", t, PkgName, rc); /* pkgdb-debug - HF */
#endif
                        }
		    }
		}
	    }
	    break;

	case PLIST_CWD:
	    if (Verbose)
		printf("extract: CWD to %s\n", p->name);
	    PUSHOUT(Directory);
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
	    if (last_file == NULL && strchr(p->name, '%') != NULL) {
		cleanup(0);
		errx(2, "no last file specified for '%s' command", p->name);
	    }
	    format_cmd(cmd, sizeof(cmd), p->name, Directory, last_file);
	    PUSHOUT(Directory);
	    if (Verbose)
		printf("extract: execute '%s'\n", cmd);
	    if (!Fake && system(cmd))
		warnx("command '%s' failed", cmd);
	    break;

	case PLIST_CHMOD:
	    PUSHOUT(Directory);
	    Mode = p->name;
	    break;

	case PLIST_CHOWN:
	    PUSHOUT(Directory);
	    Owner = p->name;
	    break;

	case PLIST_CHGRP:
	    PUSHOUT(Directory);
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
    PUSHOUT(Directory);
    pkgdb_close();
}
