/* $NetBSD: rm.c,v 1.35 2003/08/04 22:31:25 jschauma Exp $ */

/*-
 * Copyright (c) 1990, 1993, 1994, 2003
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
 */

#include <sys/cdefs.h>
#ifndef lint
__COPYRIGHT("@(#) Copyright (c) 1990, 1993, 1994\n\
	The Regents of the University of California.  All rights reserved.\n");
#endif /* not lint */

#ifndef lint
#if 0
static char sccsid[] = "@(#)rm.c	8.8 (Berkeley) 4/27/95";
#else
__RCSID("$NetBSD: rm.c,v 1.35 2003/08/04 22:31:25 jschauma Exp $");
#endif
#endif /* not lint */

#include <sys/param.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <fts.h>
#include <grp.h>
#include <locale.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <vis.h>

int dflag, eval, fflag, iflag, Pflag, stdin_ok, stdout_ok, vflag, Wflag;

int	check(char *, char *, struct stat *);
void	checkdot(char **);
char 	*printescaped(const char *);
void	rm_file(char **);
void	rm_overwrite(char *, struct stat *);
void	rm_tree(char **);
void	usage(void);
int	main(int, char *[]);

/*
 * For the sake of the `-f' flag, check whether an error number indicates the
 * failure of an operation due to an non-existent file, either per se (ENOENT)
 * or because its filename argument was illegal (ENAMETOOLONG, ENOTDIR).
 */
#define NONEXISTENT(x) \
    ((x) == ENOENT || (x) == ENAMETOOLONG || (x) == ENOTDIR)

/*
 * rm --
 *	This rm is different from historic rm's, but is expected to match
 *	POSIX 1003.2 behavior.  The most visible difference is that -f
 *	has two specific effects now, ignore non-existent files and force
 * 	file removal.
 */
int
main(int argc, char *argv[])
{
	int ch, rflag;

	setprogname(argv[0]);
	(void)setlocale(LC_ALL, "");

	Pflag = rflag = 0;
	while ((ch = getopt(argc, argv, "dfiPRrvW")) != -1)
		switch (ch) {
		case 'd':
			dflag = 1;
			break;
		case 'f':
			fflag = 1;
			iflag = 0;
			break;
		case 'i':
			fflag = 0;
			iflag = 1;
			break;
		case 'P':
			Pflag = 1;
			break;
		case 'R':
		case 'r':			/* Compatibility. */
			rflag = 1;
			break;
		case 'v':
			vflag = 1;
			break;
		case 'W':
			Wflag = 1;
			break;
		case '?':
		default:
			usage();
		}
	argc -= optind;
	argv += optind;

	if (argc < 1)
		usage();

	checkdot(argv);

	if (*argv) {
		stdin_ok = isatty(STDIN_FILENO);
		stdout_ok = isatty(STDOUT_FILENO);

		if (rflag)
			rm_tree(argv);
		else
			rm_file(argv);
	}

	exit(eval);
	/* NOTREACHED */
}

void
rm_tree(char **argv)
{
	FTS *fts;
	FTSENT *p;
	int flags, needstat, rval;
	char *fn;
			
	/*
	 * Remove a file hierarchy.  If forcing removal (-f), or interactive
	 * (-i) or can't ask anyway (stdin_ok), don't stat the file.
	 */
	needstat = !fflag && !iflag && stdin_ok;

	/*
	 * If the -i option is specified, the user can skip on the pre-order
	 * visit.  The fts_number field flags skipped directories.
	 */
#define	SKIPPED	1

	flags = FTS_PHYSICAL;
	if (!needstat)
		flags |= FTS_NOSTAT;
	if (Wflag)
		flags |= FTS_WHITEOUT;
	if (!(fts = fts_open(argv, flags,
	    (int (*)(const FTSENT **, const FTSENT **))NULL)))
		err(1, NULL);
	while ((p = fts_read(fts)) != NULL) {
	
		switch (p->fts_info) {
		case FTS_DNR:
			if (!fflag || p->fts_errno != ENOENT) {
				fn = printescaped(p->fts_path);
				warnx("%s: %s", fn, strerror(p->fts_errno));
				free(fn);
				eval = 1;
			}
			continue;
		case FTS_ERR:
			errx(EXIT_FAILURE, "%s: %s", printescaped(p->fts_path),
					strerror(p->fts_errno));
			/* NOTREACHED */
		case FTS_NS:
			/*
			 * FTS_NS: assume that if can't stat the file, it
			 * can't be unlinked.
			 */
			if (fflag && NONEXISTENT(p->fts_errno))
				continue;
			if (needstat) {
				fn = printescaped(p->fts_path);
				warnx("%s: %s", fn, strerror(p->fts_errno));
				free(fn);
				eval = 1;
				continue;
			}
			break;
		case FTS_D:
			/* Pre-order: give user chance to skip. */
			if (!fflag && !check(p->fts_path, p->fts_accpath,
			    p->fts_statp)) {
				(void)fts_set(fts, p, FTS_SKIP);
				p->fts_number = SKIPPED;
			}
			continue;
		case FTS_DP:
			/* Post-order: see if user skipped. */
			if (p->fts_number == SKIPPED)
				continue;
			break;
		default:
			if (!fflag &&
			    !check(p->fts_path, p->fts_accpath, p->fts_statp))
				continue;
		}

		rval = 0;
		/*
		 * If we can't read or search the directory, may still be
		 * able to remove it.  Don't print out the un{read,search}able
		 * message unless the remove fails.
		 */
		switch (p->fts_info) {
		case FTS_DP:
		case FTS_DNR:
			rval = rmdir(p->fts_accpath);
			if (rval != 0 && fflag && errno == ENOENT)
				continue;
			break;

		case FTS_W:
			rval = undelete(p->fts_accpath);
			if (rval != 0 && fflag && errno == ENOENT)
				continue;
			break;

		default:
			if (Pflag)
				rm_overwrite(p->fts_accpath, NULL);
			rval = unlink(p->fts_accpath);
			if (rval != 0 && fflag && NONEXISTENT(errno))
				continue;
			break;
		}
		if (rval != 0) {
			fn = printescaped(p->fts_path);
			warn("%s", fn);
			free(fn);
			eval = 1;
		} else if (vflag) {
			fn = printescaped(p->fts_path);
			(void)printf("%s\n", fn);
			free(fn);
		}
	}
	if (errno)
		err(1, "fts_read");
}

void
rm_file(char **argv)
{
	struct stat sb;
	int rval;
	char *f, *fn;

	/*
	 * Remove a file.  POSIX 1003.2 states that, by default, attempting
	 * to remove a directory is an error, so must always stat the file.
	 */
	while ((f = *argv++) != NULL) {
		/* Assume if can't stat the file, can't unlink it. */
		if (lstat(f, &sb)) {
			if (Wflag) {
				sb.st_mode = S_IFWHT|S_IWUSR|S_IRUSR;
			} else {
				if (!fflag || !NONEXISTENT(errno)) {
					fn = printescaped(f);
					warn("%s", fn);
					free(fn);
					eval = 1;
				}
				continue;
			}
		} else if (Wflag) {
			fn = printescaped(f);
			warnx("%s: %s", fn, strerror(EEXIST));
			free(fn);
			eval = 1;
			continue;
		}

		if (S_ISDIR(sb.st_mode) && !dflag) {
			fn = printescaped(f);
			warnx("%s: is a directory", fn);
			free(fn);
			eval = 1;
			continue;
		}
		if (!fflag && !S_ISWHT(sb.st_mode) && !check(f, f, &sb))
			continue;
		if (S_ISWHT(sb.st_mode))
			rval = undelete(f);
		else if (S_ISDIR(sb.st_mode))
			rval = rmdir(f);
		else {
			if (Pflag)
				rm_overwrite(f, &sb);
			rval = unlink(f);
		}
		if (rval && (!fflag || !NONEXISTENT(errno))) {
			fn = printescaped(f);
			warn("%s", fn);
			free(fn);
			eval = 1;
		}
		if (vflag && rval == 0) {
			fn = printescaped(f);
			(void)printf("%s\n", fn);
			free(fn);
		}
	}
}

/*
 * rm_overwrite --
 *	Overwrite the file 3 times with varying bit patterns.
 *
 * XXX
 * This is a cheap way to *really* delete files.  Note that only regular
 * files are deleted, directories (and therefore names) will remain.
 * Also, this assumes a fixed-block file system (like FFS, or a V7 or a
 * System V file system).  In a logging file system, you'll have to have
 * kernel support.
 */
void
rm_overwrite(char *file, struct stat *sbp)
{
	struct stat sb;
	off_t len;
	int fd, wlen;
	char buf[8 * 1024];
	char *fn;

	fd = -1;
	if (sbp == NULL) {
		if (lstat(file, &sb))
			goto err;
		sbp = &sb;
	}
	if (!S_ISREG(sbp->st_mode))
		return;
	if ((fd = open(file, O_WRONLY, 0)) == -1)
		goto err;

#define	PASS(byte) do {							\
	memset(buf, byte, sizeof(buf));					\
	for (len = sbp->st_size; len > 0; len -= wlen) {		\
		wlen = len < sizeof(buf) ? len : sizeof(buf);		\
		if (write(fd, buf, wlen) != wlen)			\
			goto err;					\
	}								\
} while (/* CONSTCOND */ 0)
	PASS(0xff);
	if (fsync(fd) || lseek(fd, (off_t)0, SEEK_SET))
		goto err;
	PASS(0x00);
	if (fsync(fd) || lseek(fd, (off_t)0, SEEK_SET))
		goto err;
	PASS(0xff);
	if (!fsync(fd) && !close(fd))
		return;

err:	eval = 1;
	fn = printescaped(file);
	warn("%s", fn);
	free(fn);
}

int
check(char *path, char *name, struct stat *sp)
{
	int ch, first;
	char modep[15];
	char *fn;

	/* Check -i first. */
	if (iflag) {
		fn = printescaped(path);
		(void)fprintf(stderr, "remove '%s'? ", fn);
		free(fn);
	} else {
		/*
		 * If it's not a symbolic link and it's unwritable and we're
		 * talking to a terminal, ask.  Symbolic links are excluded
		 * because their permissions are meaningless.  Check stdin_ok
		 * first because we may not have stat'ed the file.
		 */
		if (!stdin_ok || S_ISLNK(sp->st_mode) ||
		    !(access(name, W_OK) && (errno != ETXTBSY)))
			return (1);
		strmode(sp->st_mode, modep);
		fn =  printescaped(path);
		(void)fprintf(stderr, "override %s%s%s/%s for '%s'? ",
		    modep + 1, modep[9] == ' ' ? "" : " ",
		    user_from_uid(sp->st_uid, 0),
		    group_from_gid(sp->st_gid, 0), fn);
		free(fn);
	}
	(void)fflush(stderr);

	first = ch = getchar();
	while (ch != '\n' && ch != EOF)
		ch = getchar();
	return (first == 'y' || first == 'Y');
}

/*
 * POSIX.2 requires that if "." or ".." are specified as the basename
 * portion of an operand, a diagnostic message be written to standard
 * error and nothing more be done with such operands.
 *
 * Since POSIX.2 defines basename as the final portion of a path after
 * trailing slashes have been removed, we'll remove them here.
 */
#define ISDOT(a) ((a)[0] == '.' && (!(a)[1] || ((a)[1] == '.' && !(a)[2])))
void
checkdot(char **argv)
{
	char *p, **save, **t;
	int complained;

	complained = 0;
	for (t = argv; *t;) {
		/* strip trailing slashes */
		p = strrchr(*t, '\0');
		while (--p > *t && *p == '/')
			*p = '\0';

		/* extract basename */
		if ((p = strrchr(*t, '/')) != NULL)
			++p;
		else
			p = *t;

		if (ISDOT(p)) {
			if (!complained++)
				warnx("\".\" and \"..\" may not be removed");
			eval = 1;
			for (save = t; (t[0] = t[1]) != NULL; ++t)
				continue;
			t = save;
		} else
			++t;
	}
}

char *
printescaped(const char *src)
{
	size_t len;
	char *retval;

	len = strlen(src);
	if (len != 0 && SIZE_T_MAX/len <= 4) {
		errx(EXIT_FAILURE, "%s: name too long", src);
		/* NOTREACHED */
	}

	retval = (char *)malloc(4*len+1);
	if (retval != NULL) {
		if (stdout_ok)
			(void)strvis(retval, src, VIS_NL | VIS_CSTYLE);
		else
			(void)strcpy(retval, src);
		return retval;
	} else
		errx(EXIT_FAILURE, "out of memory!");
		/* NOTREACHED */
}

void
usage(void)
{

	(void)fprintf(stderr, "usage: %s [-f|-i] [-dPRrvW] file ...\n",
	    getprogname());
	exit(1);
	/* NOTREACHED */
}
