/*	$NetBSD: plist.c,v 1.31 2001/06/19 13:42:23 wiz Exp $	*/

#include <sys/cdefs.h>
#ifndef lint
#if 0
static const char *rcsid = "from FreeBSD Id: plist.c,v 1.24 1997/10/08 07:48:15 charnier Exp";
#else
__RCSID("$NetBSD: plist.c,v 1.31 2001/06/19 13:42:23 wiz Exp $");
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
 * General packing list routines.
 *
 */

#include "lib.h"
#include <errno.h>
#include <err.h>
#include <md5.h>

/* This struct defines a plist command type */
typedef struct cmd_t {
	char   *c_s;		/* string to recognise */
	pl_ent_t c_type;	/* type of command */
	int     c_argc;		/* # of arguments */
	int     c_subst;	/* can substitute real prefix */
}       cmd_t;

/* Commands to recognise */
static cmd_t cmdv[] = {
	{"cwd", PLIST_CWD, 1, 1},
	{"src", PLIST_SRC, 1, 1},
	{"cd", PLIST_CWD, 1, 1},
	{"exec", PLIST_CMD, 1, 0},
	{"unexec", PLIST_UNEXEC, 1, 0},
	{"mode", PLIST_CHMOD, 1, 0},
	{"owner", PLIST_CHOWN, 1, 0},
	{"group", PLIST_CHGRP, 1, 0},
	{"comment", PLIST_COMMENT, 1, 0},
	{"ignore", PLIST_IGNORE, 0, 0},
	{"ignore_inst", PLIST_IGNORE_INST, 0, 0},
	{"name", PLIST_NAME, 1, 0},
	{"display", PLIST_DISPLAY, 1, 0},
	{"pkgdep", PLIST_PKGDEP, 1, 0},
	{"pkgcfl", PLIST_PKGCFL, 1, 0},
	{"mtree", PLIST_MTREE, 1, 0},
	{"dirrm", PLIST_DIR_RM, 1, 0},
	{"option", PLIST_OPTION, 1, 0},
	{NULL, FAIL, 0, 0}
};

/*
 * Add an item to the end of a packing list
 */
void
add_plist(package_t *p, pl_ent_t type, char *arg)
{
	plist_t *tmp;

	tmp = new_plist_entry();
	tmp->name = (arg == (char *) NULL) ? (char *) NULL : strdup(arg);
	tmp->type = type;
	if (!p->head) {
		p->head = p->tail = tmp;
	} else {
		tmp->prev = p->tail;
		p->tail->next = tmp;
		p->tail = tmp;
	}
}

/*
 * Add an item to the start of a packing list
 */
void
add_plist_top(package_t *p, pl_ent_t type, char *arg)
{
	plist_t *tmp;

	tmp = new_plist_entry();
	tmp->name = (arg == (char *) NULL) ? (char *) NULL : strdup(arg);
	tmp->type = type;
	if (!p->head) {
		p->head = p->tail = tmp;
	} else {
		tmp->next = p->head;
		p->head->prev = tmp;
		p->head = tmp;
	}
}

/*
 * Return the last (most recent) entry in a packing list
 */
plist_t *
last_plist(package_t *p)
{
	return p->tail;
}

/*
 * Mark all items in a packing list to prevent iteration over them
 */
void
mark_plist(package_t *pkg)
{
	plist_t *pp;

	for (pp = pkg->head; pp; pp = pp->next) {
		pp->marked = TRUE;
	}
}

/*
 * Find a given item in a packing list and, if so, return it (else NULL)
 */
plist_t *
find_plist(package_t *pkg, pl_ent_t type)
{
	plist_t *pp;

	for (pp = pkg->head; pp && pp->type != type; pp = pp->next) {
	}
	return pp;
}

/*
 * Look for a specific boolean option argument in the list
 */
char   *
find_plist_option(package_t *pkg, char *name)
{
	plist_t *p;

	for (p = pkg->head; p; p = p->next) {
		if (p->type == PLIST_OPTION
		    && strcmp(p->name, name) == 0) {
			return p->name;
		}
	}
	
	return (char *) NULL;
}

/*
 * Delete plist item 'type' in the list (if 'name' is non-null, match it
 * too.)  If 'all' is set, delete all items, not just the first occurance.
 */
void
delete_plist(package_t *pkg, Boolean all, pl_ent_t type, char *name)
{
	plist_t *p = pkg->head;

	while (p) {
		plist_t *pnext = p->next;

		if (p->type == type && (!name || !strcmp(name, p->name))) {
			free(p->name);
			if (p->prev)
				p->prev->next = pnext;
			else
				pkg->head = pnext;
			if (pnext)
				pnext->prev = p->prev;
			else
				pkg->tail = p->prev;
			free(p);
			if (!all)
				return;
			p = pnext;
		} else
			p = p->next;
	}
}

/*
 * Allocate a new packing list entry, and return a pointer to it. 
 */
plist_t *
new_plist_entry(void)
{
	plist_t *ret;

	if ((ret = (plist_t *) malloc(sizeof(plist_t))) == (plist_t *) NULL) {
		err(EXIT_FAILURE, "can't allocate %ld bytes", (long) sizeof(plist_t));
	}
	memset(ret, 0, sizeof(plist_t));
	return ret;
}

/*
 * Free an entire packing list
 */
void
free_plist(package_t *pkg)
{
	plist_t *p = pkg->head;

	while (p) {
		plist_t *p1 = p->next;

		free(p->name);
		free(p);
		p = p1;
	}
	pkg->head = pkg->tail = NULL;
}

/*
 * For an ASCII string denoting a plist command, return its code and
 * optionally its argument(s)
 */
int
plist_cmd(char *s, char **arg)
{
	cmd_t  *cmdp;
	char    cmd[FILENAME_MAX + 20];	/* 20 == fudge for max cmd len */
	char   *cp;
	char   *sp;

	(void) strcpy(cmd, s);
	str_lowercase(cmd);
	for (cp = cmd, sp = s; *cp; cp++, sp++) {
		if (isspace((unsigned char) *cp)) {
			for (*cp = '\0'; isspace((unsigned char) *sp); sp++) {
			}
			break;
		}
	}
	if (arg) {
		*arg = sp;
	}
	for (cmdp = cmdv; cmdp->c_s && strcmp(cmdp->c_s, cmd) != 0; cmdp++) {
	}
	return cmdp->c_type;
}

/*
 * Read a packing list from a file
 */
void
read_plist(package_t *pkg, FILE * fp)
{
	char    pline[FILENAME_MAX];
	char   *cp;
	int     cmd;
	int     len;

	while (fgets(pline, FILENAME_MAX, fp) != (char *) NULL) {
		for (len = strlen(pline); len &&
		    isspace((unsigned char) pline[len - 1]);) {
			pline[--len] = '\0';
		}
		if (len == 0) {
			continue;
		}
		if (*(cp = pline) == CMD_CHAR) {
			if ((cmd = plist_cmd(pline + 1, &cp)) == FAIL) {
				warnx("Unrecognised PLIST command `%s'", pline);
				continue;
			}
			if (*cp == '\0') {
				cp = NULL;
			}
		} else {
			cmd = PLIST_FILE;
		}
		add_plist(pkg, cmd, cp);
	}
}

/*
 * Write a packing list to a file, converting commands to ASCII equivs
 */
void
write_plist(package_t *pkg, FILE * fp, char *realprefix)
{
	plist_t *p;
	cmd_t  *cmdp;

	for (p = pkg->head; p; p = p->next) {
		if (p->type == PLIST_FILE) {
			/* Fast-track files - these are the most common */
			(void) fprintf(fp, "%s\n", p->name);
			continue;
		}
		for (cmdp = cmdv; cmdp->c_type != FAIL && cmdp->c_type != p->type; cmdp++) {
		}
		if (cmdp->c_type == FAIL) {
			warnx("Unknown PLIST command type %d (%s)", p->type, p->name);
		} else if (cmdp->c_argc == 0) {
			(void) fprintf(fp, "%c%s\n", CMD_CHAR, cmdp->c_s);
		} else if (cmdp->c_subst && realprefix) {
			(void) fprintf(fp, "%c%s %s\n", CMD_CHAR, cmdp->c_s, realprefix);
		} else {
			(void) fprintf(fp, "%c%s %s\n", CMD_CHAR, cmdp->c_s,
			    (p->name) ? p->name : "");
		}
	}
}

/*
 * Delete the results of a package installation.
 *
 * This is here rather than in the pkg_delete code because pkg_add needs to
 * run it too in cases of failure.
 */
int
delete_package(Boolean ign_err, Boolean nukedirs, package_t *pkg)
{
	plist_t *p;
	char   *Where = ".", *last_file = "";
	int     fail = SUCCESS;
	Boolean preserve;
	char    tmp[FILENAME_MAX], *name = NULL;

	if (pkgdb_open(0) == -1) {
		err(1, "cannot open pkgdb");
	}

	preserve = find_plist_option(pkg, "preserve") ? TRUE : FALSE;
	for (p = pkg->head; p; p = p->next) {
		switch (p->type) {
		case PLIST_NAME:
			name = p->name;
			break;

		case PLIST_IGNORE:
			p = p->next;
			break;

		case PLIST_CWD:
			Where = p->name;
			if (Verbose)
				printf("Change working directory to %s\n", Where);
			break;

		case PLIST_UNEXEC:
			format_cmd(tmp, sizeof(tmp), p->name, Where, last_file);
			if (Verbose)
				printf("Execute `%s'\n", tmp);
			if (!Fake && system(tmp)) {
				warnx("unexec command for `%s' failed", tmp);
				fail = FAIL;
			}
			break;

		case PLIST_FILE:
			last_file = p->name;
			(void) snprintf(tmp, sizeof(tmp), "%s/%s", Where, p->name);
			if (isdir(tmp)) {
				warnx("attempting to delete directory `%s' as a file\n"
				    "this packing list is incorrect - ignoring delete request", tmp);
			} else {
				if (p->next &&
				    p->next->type == PLIST_COMMENT && /* || PLIST_MD5 - HF */
				    strncmp(p->next->name, CHECKSUM_HEADER, ChecksumHeaderLen) == 0) {
					char   *cp, buf[LegibleChecksumLen];

					if ((cp = MD5File(tmp, buf)) != NULL) {
						/* Mismatch? */
						if (strcmp(cp, p->next->name + ChecksumHeaderLen) != 0) {
							if (Verbose) {
								printf("%s fails original MD5 checksum - %s\n",
								    tmp, Force ? "deleted anyway." : "not deleted.");
							}
							if (!Force) {
								fail = FAIL;
								continue;
							}
						}
					}
				}
				if (Verbose)
					printf("Delete file %s\n", tmp);
				if (!Fake) {
					int     restored = 0;	/* restored from preserve? */

					if (delete_hierarchy(tmp, ign_err, nukedirs))
						fail = FAIL;
					if (preserve && name) {
						char    tmp2[FILENAME_MAX];

						if (make_preserve_name(tmp2, FILENAME_MAX, name, tmp)) {
							if (fexists(tmp2)) {
								if (rename(tmp2, tmp))
									warn("preserve: unable to restore %s as %s",
									    tmp2, tmp);
								else
									restored = 1;
							}
						}
					}

					if (!restored) {
#ifdef PKGDB_DEBUG
						printf("pkgdb_remove(\"%s\")\n", tmp);	/* HF */
#endif
						errno = 0;
						if (pkgdb_remove(tmp)) {
							if (errno) {
								perror("pkgdb_remove");
							}
						} else {
#ifdef PKGDB_DEBUG
							printf("pkgdb_remove: ok\n");
#endif
						}
					}
				}
			}
			break;

		case PLIST_DIR_RM:
			(void) snprintf(tmp, sizeof(tmp), "%s/%s", Where, p->name);
			if (fexists(tmp)) {
			    if (!isdir(tmp)) {
				warnx("cannot remove `%s' as a directory\n"
				    "this packing list is incorrect - ignoring delete request", tmp);
			    } else {
				if (Verbose)
					printf("Delete directory %s\n", tmp);
				if (!Fake && delete_hierarchy(tmp, ign_err, FALSE)) {
					warnx("unable to completely remove directory '%s'", tmp);
					fail = FAIL;
				}
			    }
			} else {
			    warnx("cannot remove non-existent directory `%s'\n"
			            "this packing list is incorrect - ignoring delete request", tmp);
			}
			last_file = p->name;
			break;
		default:
			break;
		}
	}
	pkgdb_close();
	return fail;
}

#ifdef DEBUG
#define RMDIR(dir) vsystem("%s %s", RMDIR_CMD, dir)
#define REMOVE(dir,ie) vsystem("%s %s%s", REMOVE_CMD, (ie ? "-f " : ""), dir)
#else
#define RMDIR rmdir
#define	REMOVE(file,ie) (remove(file) && !(ie))
#endif

/*
 * Selectively delete a hierarchy
 * Returns 1 on error, 0 else.
 */
int
delete_hierarchy(char *dir, Boolean ign_err, Boolean nukedirs)
{
	char   *cp1, *cp2;

	cp1 = cp2 = dir;
	if (!fexists(dir)) {
		if (!ign_err)
			warnx("%s `%s' doesn't really exist",
			    isdir(dir) ? "directory" : "file", dir);
		return !ign_err;
	} else if (nukedirs) {
		if (vsystem("%s -r%s %s", REMOVE_CMD, (ign_err ? "f" : ""), dir))
			return 1;
	} else if (isdir(dir)) {
		if (RMDIR(dir) && !ign_err)
			return 1;
	} else {
		if (REMOVE(dir, ign_err))
			return 1;
	}

	if (!nukedirs)
		return 0;
	while (cp2) {
		if ((cp2 = strrchr(cp1, '/')) != NULL)
			*cp2 = '\0';
		if (!isemptydir(dir))
			return 0;
		if (RMDIR(dir) && !ign_err) {
			if (!fexists(dir))
				warnx("directory `%s' doesn't really exist", dir);
			else
				return 1;
		}
		/* back up the pathname one component */
		if (cp2) {
			cp1 = dir;
		}
	}
	return 0;
}
