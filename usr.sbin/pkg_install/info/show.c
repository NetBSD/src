/*	$NetBSD: show.c,v 1.35 2007/04/20 14:03:15 tnn Exp $	*/

#if HAVE_CONFIG_H
#include "config.h"
#endif
#include <nbcompat.h>
#if HAVE_SYS_CDEFS_H
#include <sys/cdefs.h>
#endif
#ifndef lint
#if 0
static const char *rcsid = "from FreeBSD Id: show.c,v 1.11 1997/10/08 07:47:38 charnier Exp";
#else
__RCSID("$NetBSD: show.c,v 1.35 2007/04/20 14:03:15 tnn Exp $");
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
 * 23 Aug 1993
 *
 * Various display routines for the info module.
 *
 */
/*
 * Copyright (c) 1999 Hubert Feyrer.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed by Hubert Feyrer for
 *      the NetBSD Project.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
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
 */

#if HAVE_ERR_H
#include <err.h>
#endif

#include "defs.h"
#include "lib.h"
#include "info.h"

/* Structure to define entries for the "show table" */
typedef struct show_t {
	pl_ent_t sh_type;	/* type of entry */
	char   *sh_quiet;	/* message when quiet */
	char   *sh_verbose;	/* message when verbose */
}       show_t;

/*
 * The entries in this table must be ordered the same as
 * pl_ent_t constants
 */
static const show_t showv[] = {
	{PLIST_FILE, "%s", "\tFile: %s"},
	{PLIST_CWD, "@cwd %s", "\tCWD to: %s"},
	{PLIST_CMD, "@exec %s", "\tEXEC '%s'"},
	{PLIST_CHMOD, "@chmod %s", "\tCHMOD to %s"},
	{PLIST_CHOWN, "@chown %s", "\tCHOWN to %s"},
	{PLIST_CHGRP, "@chgrp %s", "\tCHGRP to %s"},
	{PLIST_COMMENT, "@comment %s", "\tComment: %s"},
	{PLIST_IGNORE, "@ignore", "Ignore next file:"},
	{PLIST_NAME, "@name %s", "\tPackage name: %s"},
	{PLIST_UNEXEC, "@unexec %s", "\tUNEXEC '%s'"},
	{PLIST_SRC, "@src: %s", "\tSRC to: %s"},
	{PLIST_DISPLAY, "@display %s", "\tInstall message file: %s"},
	{PLIST_PKGDEP, "@pkgdep %s", "\tPackage depends on: %s"},
	{PLIST_MTREE, "@mtree %s", "\tPackage mtree file: %s"},
	{PLIST_DIR_RM, "@dirrm %s", "\tDeinstall directory remove: %s"},
	{PLIST_IGNORE_INST, "@ignore_inst ??? doesn't belong here",
	"\tIgnore next file installation directive (doesn't belong)"},
	{PLIST_OPTION, "@option %s", "\tPackage has option: %s"},
	{PLIST_PKGCFL, "@pkgcfl %s", "\tPackage conflicts with: %s"},
	{PLIST_BLDDEP, "@blddep %s", "\tPackage depends exactly on: %s"},
	{-1, NULL, NULL}
};

static int print_file_as_var(const char *, const char *);

void
show_file(char *pkg, char *title, char *fname, Boolean separator)
{
	FILE   *fp;
	char    line[1024];
	int     n;

	if (!Quiet) {
		printf("%s%s", InfoPrefix, title);
	}
	if ((fp = fopen(fname, "r")) == (FILE *) NULL) {
		printf("ERROR: show_file: package \"%s\": can't open '%s' for reading\n", pkg, fname);
	} else {
		int append_nl = 0;
		while ((n = fread(line, 1, sizeof(line), fp)) != 0) {
			fwrite(line, 1, n, stdout);
			append_nl = (line[n - 1] != '\n');
		}
		(void) fclose(fp);
		if (append_nl)
			printf("\n");
	}
	if (!Quiet || separator) {
		printf("\n");		/* just in case */
	}
}

void
show_var(const char *fname, const char *variable)
{
	char   *value;

	if ((value=var_get(fname, variable)) != NULL) {
	    (void) printf("%s\n", value);
	    free(value);
	}
}

void
show_index(char *pkg, char *title, char *fname)
{
	FILE   *fp;
	char   *line;
	size_t  linelen;
	size_t  maxline = termwidth;

	if (!Quiet) {
		printf("%s%s", InfoPrefix, title);
		maxline -= MAX(MAXNAMESIZE, strlen(title));
	}
	if ((fp = fopen(fname, "r")) == (FILE *) NULL) {
		warnx("show_index: package \"%s\": can't open '%s' for reading", pkg, fname);
		return;
	}
	if ((line = fgetln(fp, &linelen))) {
		line[linelen - 1] = '\0';	/* tromp newline & terminate string */
		if (termwidth && (linelen > maxline)) {
			/* XXX -1 if term does NOT have xn (or xenl) quirk */
			line[maxline] = '\0';
		}
		(void) printf("%s\n", line);
	}
	(void) fclose(fp);
}

/*
 * Show a packing list item type.  If type is PLIST_SHOW_ALL, show all
 */
void
show_plist(char *title, package_t *plist, pl_ent_t type)
{
	plist_t *p;
	Boolean ign;

	if (!Quiet) {
		printf("%s%s", InfoPrefix, title);
	}
	for (ign = FALSE, p = plist->head; p; p = p->next) {
		if (p->type == type || type == PLIST_SHOW_ALL) {
			switch (p->type) {
			case PLIST_FILE:
				printf(Quiet ? showv[p->type].sh_quiet : showv[p->type].sh_verbose, p->name);
				if (ign) {
					if (!Quiet) {
						printf(" (ignored)");
					}
					ign = FALSE;
				}
				break;
			case PLIST_CHMOD:
			case PLIST_CHOWN:
			case PLIST_CHGRP:
				printf(Quiet ? showv[p->type].sh_quiet : showv[p->type].sh_verbose,
				    p->name ? p->name : "(clear default)");
				break;
			case PLIST_IGNORE:
				printf(Quiet ? showv[p->type].sh_quiet : showv[p->type].sh_verbose);
				ign = TRUE;
				break;
			case PLIST_IGNORE_INST:
				printf(Quiet ? showv[p->type].sh_quiet : showv[p->type].sh_verbose, p->name);
				ign = TRUE;
				break;
			case PLIST_CWD:
			case PLIST_CMD:
			case PLIST_SRC:
			case PLIST_UNEXEC:
			case PLIST_COMMENT:
			case PLIST_NAME:
			case PLIST_DISPLAY:
			case PLIST_PKGDEP:
			case PLIST_MTREE:
			case PLIST_DIR_RM:
			case PLIST_OPTION:
			case PLIST_PKGCFL:
			case PLIST_BLDDEP:
				printf(Quiet ? showv[p->type].sh_quiet : showv[p->type].sh_verbose, p->name);
				break;
			default:
				warnx("unknown command type %d (%s)", p->type, p->name);
			}
			(void) fputc('\n', stdout);
		}
	}
}

/*
 * Show all files in the packing list (except ignored ones)
 */
void
show_files(char *title, package_t *plist)
{
	plist_t *p;
	Boolean ign;
	char   *dir = ".";

	if (!Quiet) {
		printf("%s%s", InfoPrefix, title);
	}
	for (ign = FALSE, p = plist->head; p; p = p->next) {
		switch (p->type) {
		case PLIST_FILE:
			if (!ign) {
				printf("%s%s%s\n", dir,
					(strcmp(dir, "/") == 0) ? "" : "/", p->name);
			}
			ign = FALSE;
			break;
		case PLIST_CWD:
			dir = p->name;
			break;
		case PLIST_IGNORE:
			ign = TRUE;
			break;
		default:
			break;
		}
	}
}

/*
 * Show dependencies (packages this pkg requires)
 */
void
show_depends(char *title, package_t *plist)
{
	plist_t *p;
	int     nodepends;

	nodepends = 1;
	for (p = plist->head; p && nodepends; p = p->next) {
		switch (p->type) {
		case PLIST_PKGDEP:
			nodepends = 0;
			break;
		default:
			break;
		}
	}
	if (nodepends)
		return;

	if (!Quiet) {
		printf("%s%s", InfoPrefix, title);
	}
	for (p = plist->head; p; p = p->next) {
		switch (p->type) {
		case PLIST_PKGDEP:
			printf("%s\n", p->name);
			break;
		default:
			break;
		}
	}

	printf("\n");
}

/*
 * Show exact dependencies (packages this pkg was built with)
 */
void
show_bld_depends(char *title, package_t *plist)
{
	plist_t *p;
	int     nodepends;

	nodepends = 1;
	for (p = plist->head; p && nodepends; p = p->next) {
		switch (p->type) {
		case PLIST_BLDDEP:
			nodepends = 0;
			break;
		default:
			break;
		}
	}
	if (nodepends)
		return;

	if (!Quiet) {
		printf("%s%s", InfoPrefix, title);
	}
	for (p = plist->head; p; p = p->next) {
		switch (p->type) {
		case PLIST_BLDDEP:
			printf("%s\n", p->name);
			break;
		default:
			break;
		}
	}

	printf("\n");
}


/*
 * Show entry for pkg_summary.txt file.
 */
void
show_summary(package_t *plist, const char *binpkgfile)
{
	static const char *bi_vars[] = {
		"PKGPATH",
		"CATEGORIES",
		"PROVIDES",
		"REQUIRES",
		"PKG_OPTIONS",
		"OPSYS",
		"OS_VERSION",
		"MACHINE_ARCH",
		"LICENSE",
		"HOMEPAGE",
		"PKGTOOLS_VERSION",
		"BUILD_DATE",
		NULL
	};
	
	plist_t *p;
	struct stat st;

	for (p = plist->head; p; p = p->next) {
		switch (p->type) {
		case PLIST_NAME:
			printf("PKGNAME=%s\n", p->name);
			break;
		case PLIST_PKGDEP:
			printf("DEPENDS=%s\n", p->name);
			break;
		case PLIST_PKGCFL:
			printf("CONFLICTS=%s\n", p->name);
			break;

		default:
			break;
		}
	}

	print_file_as_var("COMMENT", COMMENT_FNAME);
	print_file_as_var("SIZE_PKG", SIZE_PKG_FNAME);

	var_copy_list(BUILD_INFO_FNAME, bi_vars);

	if (binpkgfile != NULL && stat(binpkgfile, &st) == 0) {
	    printf("FILE_SIZE=%" PRIu64 "\n", (uint64_t)st.st_size);
	    /* XXX: DIGETS */
	}

	print_file_as_var("DESCRIPTION", DESC_FNAME);
	putc('\n', stdout);
}

/*
 * Print the contents of file fname as value of variable var to stdout.
 */
static int
print_file_as_var(const char *var, const char *fname)
{
	FILE *fp;
	char *line;
	size_t len;

	fp = fopen(fname, "r");
	if (!fp) {
		warn("unable to open %s file", fname);
		return -1;
	}

	while ((line = fgetln(fp, &len)) != (char *) NULL) {
		if (line[len - 1] == '\n')
			--len;
		printf("%s=%.*s\n", var, (int)len, line);
	}
	
	fclose(fp);

	return 0;
}
