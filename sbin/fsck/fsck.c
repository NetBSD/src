/*	$NetBSD: fsck.c,v 1.1 1996/09/11 20:27:14 christos Exp $	*/

/*
 * Copyright (c) 1996 Christos Zoulas. All rights reserved.
 * Copyright (c) 1980, 1989, 1993, 1994
 *	The Regents of the University of California.  All rights reserved.
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
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * From: @(#)mount.c	8.19 (Berkeley) 4/19/94
 * From: NetBSD: mount.c,v 1.24 1995/11/18 03:34:29 cgd Exp 
 *
 */

static char rcsid[] = "$NetBSD: fsck.c,v 1.1 1996/09/11 20:27:14 christos Exp $";

#include <sys/param.h>
#include <sys/mount.h>
#include <sys/queue.h>
#include <sys/wait.h>

#include <err.h>
#include <errno.h>
#include <fstab.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "pathnames.h"

static enum { IN_LIST, NOT_IN_LIST } which;

TAILQ_HEAD(fstypelist, entry) head;

struct entry {
	char *type;
	TAILQ_ENTRY(entry) entries;
};

static int	preen = 0, verbose = 0, debug = 0;
static struct fstypelist *typelist;

static int checkfs __P((const char *, const char *, const char *));
static int selected __P((const char *));
static void addentry __P((const char *));
static void rementry __P((const char *));
static void unselect __P((const char *));
static void maketypelist __P((char *));
static char *catopt __P((char *, const char *));
static void mangle __P((char *, int *, const char **));
static void *emalloc __P((size_t));
static char *estrdup __P((const char *));
static void usage __P((void));


int
main(argc, argv)
	int argc;
	char * const argv[];
{
	struct fstab *fs;
	int i, rval = 0;
	char *options = NULL;
	char *vfstype = NULL;

	while ((i = getopt(argc, argv, "dvpnyt:o:")) != -1)
		switch (i) {
		case 'd':
			debug++;
			break;

		case 'v':
			verbose++;
			break;

		case 'y':
			options = catopt(options, "-y");
			break;

		case 'n':
			options = catopt(options, "-n");
			break;

		case 'o':
			if (*optarg)
				options = catopt(options, optarg);
			break;

		case 't':
			if (typelist != NULL)
				errx(1, "only one -t option may be specified.");
			maketypelist(optarg);
			vfstype = optarg;
			break;

		case 'p':
			options = catopt(options, "-p");
			preen++;
			break;

		case '?':
		default:
			usage();
			/* NOTREACHED */
		}

	argc -= optind;
	argv += optind;

#define	BADTYPE(type)							\
	(strcmp(type, FSTAB_RO) &&					\
	    strcmp(type, FSTAB_RW) && strcmp(type, FSTAB_RQ))

	if (argc == 0) {
		while ((fs = getfsent()) != NULL) {

			if (fs->fs_passno == 0)
				continue;

			if (BADTYPE(fs->fs_type))
				continue;

			if (!selected(fs->fs_vfstype))
				continue;

			if (preen) {
				rval |= checkfs(fs->fs_vfstype, NULL, options);
				unselect(fs->fs_vfstype);
			}
			else
				rval |= checkfs(fs->fs_vfstype, fs->fs_spec,
				    options);
		}
		return rval;
	}

	/*
	 * When we select specific filesystems, we cannot preen 
	 */
	if (preen)
		usage();

	for (; argc--; argv++) {
		char *spec, *type;

		if ((fs = getfsfile(*argv)) == NULL &&
		    (fs = getfsspec(*argv)) == NULL) {
			if (vfstype == NULL)
				errx(1,
				    "%s: unknown special file or file system.",
				    *argv);
			spec = *argv;
			type = vfstype;
		}
		else {
			spec = fs->fs_spec;
			type = fs->fs_vfstype;
			if (BADTYPE(fs->fs_type))
				errx(1, "%s has unknown file system type.",
				    *argv);
		}

		rval |= checkfs(type, spec, options);
	}

	return rval;
}

static int
checkfs(vfstype, spec, options)
	const char *vfstype, *spec, *options;
{
	/* List of directories containing fsck_xxx subcommands. */
	static const char *edirs[] = {
		_PATH_SBIN,
		_PATH_USRSBIN,
		NULL
	};
	const char *argv[100], **edir;
	pid_t pid;
	int argc, i, status;
	char *optbuf = NULL, execname[MAXPATHLEN + 1];

#ifdef __GNUC__
	/* Avoid vfork clobbering */
	(void) &optbuf;
#endif

	argc = 0;
	argv[argc++] = vfstype;
	if (options)
		mangle(optbuf = estrdup(options), &argc, argv);
	if (spec)
		argv[argc++] = spec;
	argv[argc] = NULL;

	if (debug || verbose) {
		(void)printf("fsck_%s", vfstype);
		for (i = 1; i < argc; i++)
			(void)printf(" %s", argv[i]);
		(void)printf("\n");
		if (debug)
			return (0);
	}

	switch (pid = vfork()) {
	case -1:				/* Error. */
		warn("vfork");
		if (optbuf)
			free(optbuf);
		return (1);

	case 0:					/* Child. */
		/* Go find an executable. */
		edir = edirs;
		do {
			(void)snprintf(execname,
			    sizeof(execname), "%s/fsck_%s", *edir, vfstype);
			execv(execname, (char * const *)argv);
			if (errno != ENOENT)
				if (spec)
					warn("exec %s for %s", execname, spec);
				else
					warn("exec %s", execname);
		} while (*++edir != NULL);

		if (errno == ENOENT)
			if (spec)
				warn("exec %s for %s", execname, spec);
			else
				warn("exec %s", execname);
		exit(1);
		/* NOTREACHED */

	default:				/* Parent. */
		if (optbuf)
			free(optbuf);

		if (waitpid(pid, &status, 0) < 0) {
			warn("waitpid");
			return (1);
		}

		if (WIFEXITED(status)) {
			if (WEXITSTATUS(status) != 0)
				return (WEXITSTATUS(status));
		}
		else if (WIFSIGNALED(status)) {
			warnx("%s: %s", spec, strsignal(WTERMSIG(status)));
			return (1);
		}
		break;
	}

	return (0);
}


static int
selected(type)
	const char *type;
{
	struct entry *e;

	/* If no type specified, it's always selected. */
	if (typelist == NULL)
		return (1);

	for (e = typelist->tqh_first; e != NULL; e = e->entries.tqe_next)
		if (!strncmp(e->type, type, MFSNAMELEN))
			return which == IN_LIST ? 1 : 0;

	return which == IN_LIST ? 0 : 1;
}


static void
addentry(type)
	const char *type;
{
	struct entry *e;

	e = emalloc(sizeof(struct entry));
	e->type = estrdup(type);

	TAILQ_INSERT_TAIL(typelist, e, entries);
}


static void
rementry(type)
	const char *type;
{
	struct entry *e;

	for (e = typelist->tqh_first; e != NULL; e = e->entries.tqe_next)
		if (!strncmp(e->type, type, MFSNAMELEN)) {
			TAILQ_REMOVE(typelist, e, entries);
			free(e->type);
			free(e);
			return;
		}
}


static void
unselect(type)
	const char *type;
{
	if (typelist == NULL) {
		TAILQ_INIT(&head);
		typelist = &head;
		which = NOT_IN_LIST;
		addentry(type);
		return;
	}
	if (which == IN_LIST)
		rementry(type);
	else
		addentry(type);
}


static void
maketypelist(fslist)
	char *fslist;
{
	char *ptr;

	if (typelist == NULL) {
		TAILQ_INIT(&head);
		typelist = &head;
	}

	if ((fslist == NULL) || (fslist[0] == '\0'))
		errx(1, "empty type list");

	if (fslist[0] == 'n' && fslist[1] == 'o') {
		fslist += 2;
		which = NOT_IN_LIST;
	}
	else
		which = IN_LIST;

	while ((ptr = strsep(&fslist, ",")) != NULL)
		addentry(ptr);

}


static char *
catopt(s0, s1)
	char *s0;
	const char *s1;
{
	size_t i;
	char *cp;

	if (s0 && *s0) {
		i = strlen(s0) + strlen(s1) + 1 + 1;
		cp = emalloc(i);
		(void)snprintf(cp, i, "%s,%s", s0, s1);
	}
	else
		cp = estrdup(s1);

	if (s0)
		free(s0);
	return (cp);
}


static void
mangle(options, argcp, argv)
	char *options;
	int *argcp;
	const char **argv;
{
	char *p, *s;
	int argc;

	argc = *argcp;
	for (s = options; (p = strsep(&s, ",")) != NULL;)
		if (*p != '\0')
			if (*p == '-') {
				argv[argc++] = p;
				p = strchr(p, '=');
				if (p) {
					*p = '\0';
					argv[argc++] = p+1;
				}
			}
			else {
				argv[argc++] = "-o";
				argv[argc++] = p;
			}

	*argcp = argc;
}


static void *
emalloc(s)
	size_t s;
{
	void *p = malloc(s);
	if (p == NULL)
		err(1, "malloc failed");
	return p;
}


static char *
estrdup(s)
	const char *s;
{
	char *p = strdup(s);
	if (p == NULL)
		err(1, "strdup failed");
	return p;
}


static void
usage()
{
	extern char *__progname;
	static const char common[] = "dvYyNn] [-o fsoptions] [-t fstype]";

	(void)fprintf(stderr, "Usage: %s [-p%s\n\t%s [-%s [special|node]...\n",
		__progname, common, __progname, common);
	exit(1);
}
