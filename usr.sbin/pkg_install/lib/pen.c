/*	$NetBSD: pen.c,v 1.24 2003/03/15 20:54:25 hubertf Exp $	*/

#include <sys/cdefs.h>
#ifndef lint
#if 0
static const char *rcsid = "from FreeBSD Id: pen.c,v 1.25 1997/10/08 07:48:12 charnier Exp";
#else
__RCSID("$NetBSD: pen.c,v 1.24 2003/03/15 20:54:25 hubertf Exp $");
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

#include <err.h>
#include "lib.h"
#include <sys/signal.h>
#include <sys/param.h>
#include <sys/mount.h>

/* For keeping track of where we are */
static char Current[FILENAME_MAX];
static char Previous[FILENAME_MAX];
static int CurrentSet;		/* rm -rf Current only if it's really set! */
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
	strcpy(Current, c);
	CurrentSet = 1;		/* rm -fr Current is safe now */
	free(c);
	
	strcpy(Previous, p);
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
	char   *cp;
	struct stat sb;

	if (pen && pen[0] && stat(pen, &sb) != FAIL && (min_free(pen) >= sz))
		return pen;
	else if ((cp = getenv("PKG_TMPDIR")) != NULL && stat(cp, &sb) != FAIL && (min_free(cp) >= sz))
		(void) snprintf(pen, pensize, "%s/instmp.XXXXXX", cp);
	else if ((cp = getenv("TMPDIR")) != NULL && stat(cp, &sb) != FAIL && (min_free(cp) >= sz))
		(void) snprintf(pen, pensize, "%s/instmp.XXXXXX", cp);
	else if (stat("/var/tmp", &sb) != FAIL && min_free("/var/tmp") >= sz)
		strcpy(pen, "/var/tmp/instmp.XXXXXX");
	else if (stat("/tmp", &sb) != FAIL && min_free("/tmp") >= sz)
		strcpy(pen, "/tmp/instmp.XXXXXX");
	else if (stat("/usr/tmp", &sb) != FAIL && min_free("/usr/tmp") >= sz)
		strcpy(pen, "/usr/tmp/instmp.XXXXXX");
	else {
		cleanup(0);
		errx(2,
		    "can't find enough temporary space to extract the files, please set your\n"
		    "PKG_TMPDIR environment variable to a location with at least %lu bytes\n"
		    "free", (u_long) sz);
		return NULL;
	}
	return pen;
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

#if (defined(NetBSD1_3) || (NetBSD <= 199713)) && (NetBSD1_3 <9)
	/* values from 1.3.2/1.3I */
	/* mkdtemp(3) is not present on 1.3.3 and below */
	if (!mktemp(pen)) {
		cleanup(0);
		errx(2, "can't mktemp '%s'", pen);
	}
	if (mkdir(pen, 0755) == FAIL) {
		cleanup(0);
		errx(2, "can't mkdir '%s'", pen);
	}
#else
	if (!mkdtemp(pen)) {
		cleanup(0);
		errx(2, "can't mkdtemp '%s'", pen);
	}
#endif
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
		strcpy(Previous, Current);
	else if (!getcwd(Previous, FILENAME_MAX)) {
		cleanup(0);
		err(EXIT_FAILURE, "fatal error during execution: getcwd");
	}
	if (chdir(pen) == FAIL) {
		cleanup(0);
		errx(2, "can't chdir to '%s'", pen);
	}
	CurrentSet = 0; strcpy(Current, pen); CurrentSet = 1;
	
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
			fprintf(stderr, "PANIC: About to rm -rf / (not doing so, aborting)\n");
			abort();
		}
		if (vsystem("rm -rf %s", Current))
			warnx("couldn't remove temporary dir '%s'", Current);
		strcpy(Current, Previous);
	}
	if (save)
		strcpy(Previous, save);
	else
		Previous[0] = '\0';
	signal(SIGINT, oldsig);
}

/*
 * Return free disk space (in bytes) on given file system
 */
off_t
min_free(char *tmpdir)
{
	struct statfs buf;

	if (statfs(tmpdir, &buf) != 0) {
		warn("statfs");
		return -1;
	}
	return (off_t) buf.f_bavail * (off_t) buf.f_bsize;
}
