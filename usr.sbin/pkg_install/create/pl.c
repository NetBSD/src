/*	$NetBSD: pl.c,v 1.4.2.1 1998/11/06 20:40:52 cgd Exp $	*/

#include <sys/cdefs.h>
#ifndef lint
#if 0
static const char *rcsid = "from FreeBSD Id: pl.c,v 1.11 1997/10/08 07:46:35 charnier Exp";
#else
__RCSID("$NetBSD: pl.c,v 1.4.2.1 1998/11/06 20:40:52 cgd Exp $");
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
 * Routines for dealing with the packing list.
 *
 */

#include "lib.h"
#include "create.h"
#include <errno.h>
#include <err.h>
#include <md5.h>

/* check that any symbolic link is relative to the prefix */
static void
CheckSymlink(char *name, char *prefix, size_t prefixcc)
{
	char	newtgt[MAXPATHLEN];
	char	oldtgt[MAXPATHLEN];
	char	*slash;
	int	slashc;
	int	cc;
	int	i;

	if ((cc = readlink(name, oldtgt, sizeof(oldtgt))) > 0) {
		oldtgt[cc] = 0;
		if (strncmp(oldtgt, prefix, prefixcc) == 0 && oldtgt[prefixcc] == '/') {
			for (slashc = 0, slash = &name[prefixcc + 1] ; (slash = strchr(slash, '/')) != (char *) NULL ; slash++, slashc++) {
			}
			for (cc = i = 0 ; i < slashc ; i++) {
				strnncpy(&newtgt[cc], sizeof(newtgt) - cc, "../", 3);
				cc += 3;
			}
			strnncpy(&newtgt[cc], sizeof(newtgt) - cc, &oldtgt[prefixcc + 1], strlen(&oldtgt[prefixcc + 1]));
			(void) fprintf(stderr, "Full pathname symlink `%s' is target of `%s' - adjusting to `%s'\n", oldtgt, name, newtgt);
			if (unlink(name) != 0) {
				warn("can't unlink `%s'", name);
			} else if (symlink(newtgt, name) != 0) {
				warn("can't symlink `%s' called `%s'", newtgt, name);
			}
		}
	}
}

/* (reversed) comparison routine for directory name sorting */
static int
dircmp(const void *vp1, const void *vp2)
{
	return strcmp(vp2, vp1);
}

/* re-order the PLIST_DIR_RM entries into reverse alphabetic order */
static void
reorder(package_t *pkg, int dirc)
{
	plist_t	*p;
	char	**dirv;
	int	i;

	if ((dirv = (char **) calloc(dirc, sizeof(char *))) == (char **) NULL) {
		warn("No directory re-ordering will be done");
	} else {
		for (p = pkg->head, i = 0 ; p ; p = p->next) {
			if (p->type == PLIST_DIR_RM) {
				dirv[i++] = p->name;
			}
		}
		qsort(dirv, dirc, sizeof(char *), dircmp);
		for (p = pkg->head, i = 0 ; p ; p = p->next) {
			if (p->type == PLIST_DIR_RM) {
				p->name = dirv[i++];
			}
		}
		(void) free(dirv);
	}
}

/* Check a list for files that require preconversion */
void
check_list(char *home, package_t *pkg)
{
	struct stat	st;
	plist_t		*tmp;
	plist_t		*p;
	char		name[FILENAME_MAX];
	char		buf[ChecksumHeaderLen + LegibleChecksumLen];
	char		*cwd = home;
	char		*srcdir = NULL;
	int		dirc;

	for (dirc = 0, p = pkg->head ; p ; p = p->next) {
		switch (p->type) {
		case PLIST_CWD:
			cwd = p->name;
			break;
		case PLIST_IGNORE:
			p = p->next;
			break;
		case PLIST_SRC:
			srcdir = p->name;
			break;
		case PLIST_DIR_RM:
			dirc++;
			break;
		case PLIST_FILE:
			(void) snprintf(name, sizeof(name), "%s/%s", srcdir ? srcdir : cwd, p->name);
			if (lstat(name, &st) < 0) {
				warnx("can't stat `%s'", name);
				continue;
			}
			switch(st.st_mode & S_IFMT) {
			case S_IFDIR:
				p->type = PLIST_DIR_RM;
				dirc++;
				continue;
			case S_IFLNK:
				if (RelativeLinks) {
					CheckSymlink(name, cwd, strlen(cwd));
				}
				break;
			case S_IFCHR:
				warnx("Warning - char special device `%s' in PLIST", name);
				break;
			case S_IFBLK:
				warnx("Warning - block special device `%s' in PLIST", name);
				break;
			default:
				break;
			}
			(void) strcpy(buf, CHECKSUM_HEADER);
			if (MD5File(name, &buf[ChecksumHeaderLen]) != (char *) NULL) {
				tmp = new_plist_entry();
				tmp->name = strdup(buf);
				tmp->type = PLIST_COMMENT;
				tmp->next = p->next;
				tmp->prev = p;
				p->next = tmp;
				p = tmp;
			}
			break;
		default:
			break;
		}
	}
	if (ReorderDirs && dirc > 0) {
		reorder(pkg, dirc);
	}
}

static int
trylink(const char *from, const char *to)
{
	char	*cp;

	if (link(from, to) == 0) {
		return 0;
	}
	if (errno == ENOENT) {
		/* try making the container directory */
		if ((cp = strrchr(to, '/')) != (char *) NULL) {
			vsystem("mkdir -p %.*s", (size_t)(cp - to), to);
		}
		return link(from, to);
	}
	return -1;
}

#define STARTSTRING "tar cf -"
#define TOOBIG(str) strlen(str) + 6 + strlen(home) + where_count > maxargs
#define PUSHOUT() /* push out string */					\
	if (where_count > sizeof(STARTSTRING)-1) {			\
		    strcat(where_args, "|tar xpf -");			\
		    if (system(where_args)) {				\
			cleanup(0);					\
			errx(2, "can't invoke tar pipeline");		\
		    }							\
		    memset(where_args, 0, maxargs);			\
 		    last_chdir = NULL;					\
		    strcpy(where_args, STARTSTRING);			\
		    where_count = sizeof(STARTSTRING)-1;		\
	}

/*
 * Copy unmarked files in packing list to playpen - marked files
 * have already been copied in an earlier pass through the list.
 */
void
copy_plist(char *home, package_t *plist)
{
    plist_t *p = plist->head;
    char *where = home;
    char *there = NULL, *mythere;
    char *where_args, *last_chdir, *root = "/";
    int maxargs, where_count = 0, add_count;
    struct stat stb;
    dev_t curdir;

    maxargs = sysconf(_SC_ARG_MAX);
    maxargs -= 64;			/* some slop for the tar cmd text,
					   and sh -c */
    where_args = malloc(maxargs);
    if (!where_args) {
	cleanup(0);
	errx(2, "can't get argument list space");
    }

    memset(where_args, 0, maxargs);
    strcpy(where_args, STARTSTRING);
    where_count = sizeof(STARTSTRING)-1;
    last_chdir = 0;

    if (stat(".", &stb) == 0)
	curdir = stb.st_dev;
    else
	curdir = (dev_t) -1;		/* It's ok if this is a valid dev_t;
					   this is just a hint for an
					   optimization. */

    while (p) {
	if (p->type == PLIST_CWD)
	    where = p->name;
	else if (p->type == PLIST_SRC)
	    there = p->name;
	else if (p->type == PLIST_IGNORE)
	    p = p->next;
	else if (p->type == PLIST_FILE && !p->marked) {
	    char fn[FILENAME_MAX];


	    /* First, look for it in the "home" dir */
	    (void) snprintf(fn, sizeof(fn), "%s/%s", home, p->name);
	    if (fexists(fn)) {
		if (lstat(fn, &stb) == 0 && stb.st_dev == curdir &&
		    S_ISREG(stb.st_mode)) {
		    /* if we can link it to the playpen, that avoids a copy
		       and saves time. */
		    if (p->name[0] != '/') {
			/* don't link abspn stuff--it doesn't come from
			   local dir! */
			if (trylink(fn, p->name) == 0) {
			    p = p->next;
			    continue;
			}
		    }
		}
		if (TOOBIG(fn)) {
		    PUSHOUT();
		}
		if (p->name[0] == '/') {
		    add_count = snprintf(&where_args[where_count],
					 maxargs - where_count,
					 " %s %s",
					 last_chdir == root ? "" : "-C /",
					 p->name);
		    last_chdir = root;
		} else {
		    add_count = snprintf(&where_args[where_count],
					 maxargs - where_count,
					 " %s%s %s",
					 last_chdir == home ? "" : "-C ",
					 last_chdir == home ? "" : home,
					 p->name);
		    last_chdir = home;
		}
		if (add_count > maxargs - where_count) {
		    cleanup(0);
		    errx(2, "oops, miscounted strings!");
		}
		where_count += add_count;
	    }
	    /*
	     * Otherwise, try along the actual extraction path..
	     */
	    else {
		if (p->name[0] == '/')
		    mythere = root;
		else mythere = there;
		(void) snprintf(fn, sizeof(fn), "%s/%s", mythere ? mythere : where, p->name);
		if (lstat(fn, &stb) == 0 && stb.st_dev == curdir &&
		    S_ISREG(stb.st_mode)) {
		    /* if we can link it to the playpen, that avoids a copy
		       and saves time. */
		    if (trylink(fn, p->name) == 0) {
			p = p->next;
			continue;
		    }
		}
		if (TOOBIG(p->name)) {
		    PUSHOUT();
		}
		if (last_chdir == (mythere ? mythere : where))
		    add_count = snprintf(&where_args[where_count],
					 maxargs - where_count,
					 " %s", p->name);
		else
		    add_count = snprintf(&where_args[where_count],
					 maxargs - where_count,
					 " -C %s %s",
					 mythere ? mythere : where,
					 p->name);
		if (add_count > maxargs - where_count) {
		    cleanup(0);
		    errx(2, "oops, miscounted strings!");
		}
		where_count += add_count;
		last_chdir = (mythere ? mythere : where);
	    }
	}
	p = p->next;
    }
    PUSHOUT();
    free(where_args);
}
