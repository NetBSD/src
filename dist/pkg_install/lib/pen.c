/*	$NetBSD: pen.c,v 1.1.1.1 2007/07/16 13:01:47 joerg Exp $	*/

#if HAVE_CONFIG_H
#include "config.h"
#endif
#include <nbcompat.h>
#if HAVE_SYS_CDEFS_H
#include <sys/cdefs.h>
#endif
#ifndef lint
#if 0
static const char *rcsid = "from FreeBSD Id: pen.c,v 1.25 1997/10/08 07:48:12 charnier Exp";
#else
__RCSID("$NetBSD: pen.c,v 1.1.1.1 2007/07/16 13:01:47 joerg Exp $");
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
 * Routines for managing the "play pen".
 *
 */

#if HAVE_ERR_H
#include <err.h>
#endif
#include "lib.h"
#if HAVE_SYS_SIGNAL_H
#include <sys/signal.h>
#endif
#if HAVE_SYS_PARAM_H
#include <sys/param.h>
#endif
#if HAVE_SYS_MOUNT_H
#include <sys/mount.h>
#endif

/* For keeping track of where we are */
static char Current[MaxPathSize];
static char Previous[MaxPathSize];
static int CurrentSet;		/* rm -fr Current only if it's really set! */
                                /* CurrentSet is set to 0 before strcpy()s
				 * to prevent rm'ing of a partial string
				 * when interrupted by ^C */

#if 0
/*
 * Backup Current and Previous into temp. strings that are later
 * restored & freed by restore_dirs
 * This is to make nested calls to make_playpen/leave_playpen work
 */
void
save_dirs(char **c, char **p)
{
	*c = strdup(Current);	/* XXX */
	*p = strdup(Previous);
}

/*
 * Restore Current and Previous from temp strings that were created
 * by safe_dirs.
 * This is to make nested calls to make_playpen/leave_playpen work
 */
void
restore_dirs(char *c, char *p)
{
	CurrentSet = 0;		/* prevent from deleting */
	strlcpy(Current, c, sizeof(Current));
	CurrentSet = 1;		/* rm -fr Current is safe now */
	free(c);
	
	strlcpy(Previous, p, sizeof(Previous));
	free(p);
}
#endif

char   *
where_playpen(void)
{
	return Current;
}

/*
 * Find a good place to play.
 */
static char *
find_play_pen(char *pen, size_t pensize, size_t sz)
{
	const char **cp;
	struct stat sb;
	char *r;
	const char *tmpdir[] = {
		"PKG_TMPDIR",
		"TMPDIR",
		"/var/tmp",
		"/tmp",
		"/usr/tmp",
		NULL
	};

	if (pen == NULL) {
		cleanup(0);
		errx(2, "find_play_pen(): 'pen' variable is NULL\n"
		     "(this should not happen, please report!)");
		return NULL;
	}
	
	if (pen[0] && (r = strrchr(pen, '/')) != NULL) {
		*r = '\0';
		if (stat(pen, &sb) != FAIL && (min_free(pen) >= sz)) {
			*r = '/';
			return pen;
		}
	}

	for (cp = tmpdir; *cp; cp++) {
		const char *d = (**cp == '/') ? *cp : getenv(*cp);

		if (d == NULL || stat(d, &sb) == FAIL || min_free(d) < sz)
			continue;

		(void)snprintf(pen, pensize, "%s/instmp.XXXXXX", d);
		return pen;
	}

	cleanup(0);
	errx(2, "Can't find enough temporary space to extract the files.\n"
	    "Please set your PKG_TMPDIR environment variable to a location "
	    "with at least %zu bytes free", sz);
	return NULL;
}

/*
 * Make a temporary directory to play in and chdir() to it, returning
 * pathname of previous working directory.
 */
char   *
make_playpen(char *pen, size_t pensize, size_t sz)
{
	if (!find_play_pen(pen, pensize, sz))
		return NULL;

	if (!mkdtemp(pen)) {
		cleanup(0);
		errx(2, "can't mkdtemp '%s'", pen);
	}
	if (Verbose) {
		if (sz)
			fprintf(stderr,
		"Requested space: %lu bytes, free space: %lld bytes in %s\n",
			    (u_long) sz, (long long) min_free(pen), pen);
	}
	if (min_free(pen) < sz) {
		rmdir(pen);
		cleanup(0);
		errx(2, "not enough free space to create '%s'.\n"
		    "Please set your PKG_TMPDIR environment variable to a location\n"
		    "with more space and\ntry the command again", pen);
	}
	if (Current[0])
		strlcpy(Previous, Current, sizeof(Previous));
	else if (!getcwd(Previous, MaxPathSize)) {
		cleanup(0);
		err(EXIT_FAILURE, "fatal error during execution: getcwd");
	}
	if (chdir(pen) == FAIL) {
		cleanup(0);
		errx(2, "can't chdir to '%s'", pen);
	}
	CurrentSet = 0; strlcpy(Current, pen, sizeof(Current)); CurrentSet = 1;
	
	return Previous;
}

/*
 * Convenience routine for getting out of playpen
 */
void
leave_playpen(char *save)
{
	void    (*oldsig) (int);

	/* Make us interruptable while we're cleaning up - just in case... */
	oldsig = signal(SIGINT, SIG_DFL);
	if (Previous[0] && chdir(Previous) == FAIL) {
		cleanup(0);
		errx(2, "can't chdir back to '%s'", Previous);
	} else if (CurrentSet && Current[0] && strcmp(Current, Previous)) {
		if (strcmp(Current, "/") == 0) {
			fprintf(stderr, "PANIC: About to rm -fr / (not doing so, aborting)\n");
			abort();
		}
		if (fexec("rm", "-fr", Current, NULL))
			warnx("couldn't remove temporary dir '%s'", Current);
		strlcpy(Current, Previous, sizeof(Current));
	}
	if (save)
		strlcpy(Previous, save, sizeof(Previous));
	else
		Previous[0] = '\0';
	signal(SIGINT, oldsig);
}

/*
 * Return free disk space (in bytes) on given file system.
 * Returns size in a uint64_t since off_t isn't 64 bits on all
 * operating systems.
 */
uint64_t
min_free(const char *tmpdir)
{
	struct statvfs buf;

	if (statvfs(tmpdir, &buf) != 0) {
		warn("statvfs");
		return 0;
	}
	return (uint64_t)buf.f_bavail * buf.f_bsize;
}
