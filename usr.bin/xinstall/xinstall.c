/*	$NetBSD: xinstall.c,v 1.44 2001/03/21 23:16:33 cgd Exp $	*/

/*
 * Copyright (c) 1987, 1993
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
__COPYRIGHT("@(#) Copyright (c) 1987, 1993\n\
	The Regents of the University of California.  All rights reserved.\n");
#endif /* not lint */

#ifndef lint
#if 0
static char sccsid[] = "@(#)xinstall.c	8.1 (Berkeley) 7/21/93";
#else
__RCSID("$NetBSD: xinstall.c,v 1.44 2001/03/21 23:16:33 cgd Exp $");
#endif
#endif /* not lint */

#include <sys/param.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include <sys/stat.h>

#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <grp.h>
#include <paths.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <err.h>

#include "pathnames.h"
#include "stat_flags.h"

#define STRIP_ARGS_MAX 32
#define BACKUP_SUFFIX ".old"

struct passwd *pp;
struct group *gp;
int dobackup=0, docopy=0, dodir=0, dostrip=0, dolink=0, dopreserve=0,
    dorename = 0, unpriv = 0;
static int numberedbackup = 0;
int mode = S_IRWXU|S_IRGRP|S_IXGRP|S_IROTH|S_IXOTH;
char pathbuf[MAXPATHLEN];
uid_t uid;
gid_t gid;
char *stripArgs=NULL;
char *suffix=BACKUP_SUFFIX;

#define LN_ABSOLUTE	0x01
#define LN_RELATIVE	0x02
#define LN_HARD		0x04
#define LN_SYMBOLIC	0x08
#define LN_MIXED	0x10

#define	DIRECTORY	0x01		/* Tell install it's a directory. */
#define	SETFLAGS	0x02		/* Tell install to set flags. */

void	copy __P((int, char *, int, char *, off_t));
void	makelink __P((char *, char *));
void	install __P((char *, char *, u_long, u_int));
void	install_dir __P((char *));
void	strip __P((char *));
void	backup __P((const char *));
void	usage __P((void));
int	main __P((int, char *[]));

int
main(argc, argv)
	int argc;
	char *argv[];
{
	struct stat from_sb, to_sb;
	void *set;
	u_long fset;
	u_int iflags;
	int ch, no_target;
	char *p;
	char *flags = NULL, *to_name, *group = NULL, *owner = NULL;

	setprogname(argv[0]);

	iflags = 0;
	while ((ch = getopt(argc, argv, "cbB:df:g:l:m:o:prsS:U")) != -1)
		switch((char)ch) {
		case 'B':
			suffix = optarg;
			numberedbackup = 0;
			{
				/* Check if given suffix really generates
				   different suffixes - catch e.g. ".%" */
				char suffix_expanded0[FILENAME_MAX],
				     suffix_expanded1[FILENAME_MAX];
				(void)snprintf(suffix_expanded0, FILENAME_MAX,
					       suffix, 0);
				(void)snprintf(suffix_expanded1, FILENAME_MAX,
					       suffix, 1);
				if (strcmp(suffix_expanded0, suffix_expanded1)
				    != 0)
					numberedbackup = 1;
			}
			/* fall through; -B implies -b */
		case 'b':
			dobackup = 1;
			break;
		case 'c':
			docopy = 1;
			break;
		case 'd':
			dodir = 1;
			break;
		case 'f':
			flags = optarg;
			if (string_to_flags(&flags, &fset, NULL))
				errx(1, "%s: invalid flag", flags);
			iflags |= SETFLAGS;
			break;
		case 'g':
			group = optarg;
			break;
		case 'l':
			for (p = optarg; *p; p++)
				switch (*p) {
				case 's':
					dolink &= ~(LN_HARD|LN_MIXED);
					dolink |= LN_SYMBOLIC;
					break;
				case 'h':
					dolink &= ~(LN_SYMBOLIC|LN_MIXED);
					dolink |= LN_HARD;
					break;
				case 'm':
					dolink &= ~(LN_SYMBOLIC|LN_HARD);
					dolink |= LN_MIXED;
					break;
				case 'a':
					dolink &= ~LN_RELATIVE;
					dolink |= LN_ABSOLUTE;
					break;
				case 'r':
					dolink &= ~LN_ABSOLUTE;
					dolink |= LN_RELATIVE;
					break;
				default:
					errx(1, "%c: invalid link type", *p);
					break;
				}
			break;
		case 'm':
			if (!(set = setmode(optarg)))
				errx(1, "%s: invalid file mode", optarg);
			mode = getmode(set, 0);
			free(set);
			break;
		case 'o':
			owner = optarg;
			break;
		case 'p':
			dopreserve = 1;
			break;
		case 'r':
			dorename = 1;
			break;
		case 'S':
			stripArgs = (char*)malloc(sizeof(char)*(strlen(optarg)+1));
			strcpy(stripArgs,optarg);
			/* fall through; -S implies -s */
		case 's':
			dostrip = 1;
			break;
		case 'U':
			unpriv = 1;
			break;
		case '?':
		default:
			usage();
		}
	argc -= optind;
	argv += optind;

	/* strip and link options make no sense when creating directories */
	if ((dostrip || dolink) && dodir)
		usage();

	/* strip and flags make no sense with links */
	if ((dostrip || flags) && dolink)
		usage();

	/* must have at least two arguments, except when creating directories */
	if (argc < 2 && !dodir)
		usage();

	/* get group and owner id's */
	if (unpriv)
		uid = gid = -1;
	else {
		if (group && !(gp = getgrnam(group)) &&
		    !isdigit((unsigned char)*group))
			errx(1, "unknown group %s", group);
		gid = (group) ? ((gp) ? gp->gr_gid : atoi(group)) : -1;
		if (owner && !(pp = getpwnam(owner)) &&
		    !isdigit((unsigned char)*owner))
			errx(1, "unknown user %s", owner);
		uid = (owner) ? ((pp) ? pp->pw_uid : atoi(owner)) : -1;
	}

	if (dodir) {
		for (; *argv != NULL; ++argv)
			install_dir(*argv);
		exit (0);
	}

	no_target = stat(to_name = argv[argc - 1], &to_sb);
	if (!no_target && S_ISDIR(to_sb.st_mode)) {
		for (; *argv != to_name; ++argv)
			install(*argv, to_name, fset, iflags | DIRECTORY);
		exit(0);
	}

	/* can't do file1 file2 directory/file */
	if (argc != 2)
		usage();

	if (!no_target) {
		if (stat(*argv, &from_sb))
			err(1, "%s", *argv);
		if (!S_ISREG(to_sb.st_mode))
			errx(1, "%s: not a regular file", to_name);
		if (!dolink && to_sb.st_dev == from_sb.st_dev &&
		    to_sb.st_ino == from_sb.st_ino)
			errx(1, "%s and %s are the same file", *argv, to_name);
		/*
		 * Unlink now... avoid ETXTBSY errors later.  Try and turn
		 * off the append/immutable bits -- if we fail, go ahead,
		 * it might work.
		 */
#define	NOCHANGEBITS	(UF_IMMUTABLE | UF_APPEND | SF_IMMUTABLE | SF_APPEND)
		if (to_sb.st_flags & NOCHANGEBITS)
			(void)chflags(to_name,
			    to_sb.st_flags & ~(NOCHANGEBITS));
		if (dobackup)
			backup(to_name);
		else if (!dorename)
			(void)unlink(to_name);
	}
	install(*argv, to_name, fset, iflags);
	exit(0);
}

/*
 * makelink --
 *	make a link from source to destination
 */
void
makelink(from_name, to_name)
	char *from_name;
	char *to_name;
{
	char src[MAXPATHLEN], dst[MAXPATHLEN], lnk[MAXPATHLEN];

	/* Try hard links first */
	if (dolink & (LN_HARD|LN_MIXED)) {
		if (link(from_name, to_name) == -1) {
			if ((dolink & LN_HARD) || errno != EXDEV)
				err(1, "link %s -> %s", from_name, to_name);
		}
		else
			return;
	}

	/* Symbolic links */
	if (dolink & LN_ABSOLUTE) {
		/* Convert source path to absolute */
		if (realpath(from_name, src) == NULL)
			err(1, "%s", src);
		if (symlink(src, to_name) == -1)
			err(1, "symlink %s -> %s", src, to_name);
		return;
	}

	if (dolink & LN_RELATIVE) {
		char *s, *d;

		/* Resolve pathnames */
		if (realpath(from_name, src) == NULL)
			err(1, "%s", src);
		if (realpath(to_name, dst) == NULL)
			err(1, "%s", dst);

		/* trim common path components */
		for (s = src, d = dst; *s == *d; s++, d++)
			continue;
		while (*s != '/')
			s--, d--;

		/* count the number of directories we need to backtrack */
		for (++d, lnk[0] = '\0'; *d; d++)
			if (*d == '/')
				(void) strcat(lnk, "../");

		(void) strcat(lnk, ++s);

		if (symlink(lnk, dst) == -1)
			err(1, "symlink %s -> %s", lnk, dst);
		return;
	}

	/*
	 * If absolute or relative was not specified, 
	 * try the names the user provided
	 */
	if (symlink(from_name, to_name) == -1)
		err(1, "symlink %s -> %s", from_name, to_name);

}

/*
 * install --
 *	build a path name and install the file
 */
void
install(from_name, to_name, fset, flags)
	char *from_name, *to_name;
	u_long fset;
	u_int flags;
{
	struct stat from_sb, to_sb;
	int devnull, from_fd, to_fd, serrno;
	char *p, tmpl[MAXPATHLEN], *oto_name;

	/* If try to install NULL file to a directory, fails. */
	if (flags & DIRECTORY || strcmp(from_name, _PATH_DEVNULL)) {
		if (stat(from_name, &from_sb))
			err(1, "%s", from_name);
		if (!S_ISREG(from_sb.st_mode))
			errx(1, "%s: not a regular file", to_name);
		/* Build the target path. */
		if (flags & DIRECTORY) {
			(void)snprintf(pathbuf, sizeof(pathbuf), "%s/%s",
			    to_name,
			    (p = strrchr(from_name, '/')) ? ++p : from_name);
			to_name = pathbuf;
		}
		devnull = 0;
	} else {
		from_sb.st_flags = 0;	/* XXX */
		devnull = 1;
	}

	/*
	 * Unlink now... avoid ETXTBSY errors later.  Try and turn
	 * off the append/immutable bits -- if we fail, go ahead,
	 * it might work.
	 */
	if (stat(to_name, &to_sb) == 0 &&
	    to_sb.st_flags & (NOCHANGEBITS))
		(void)chflags(to_name, to_sb.st_flags & ~(NOCHANGEBITS));
	if (dorename) {
		char *ptr, c, *dir;

		if ((ptr = strrchr(to_name, '/')) != NULL) {
			c = *++ptr;
			*ptr = '\0';
			dir = to_name;
		} else {
			c = '\0';	/* pacify gcc */
			dir = tmpl;
			*dir = '\0';
		}
		(void)snprintf(tmpl, sizeof(tmpl), "%sinst.XXXXXX", dir);
		if (ptr)
			*ptr = c;
		oto_name = to_name;
		to_name = tmpl;

	} else {
		oto_name = NULL;	/* pacify gcc */
		if (dobackup)
			backup(to_name);
		else
			(void)unlink(to_name);
	}

	if (dolink) {
		makelink(from_name, to_name);
		return;
	}

	/* Create target. */
	if (dorename) {
		if ((to_fd = mkstemp(to_name)) == -1)
			err(1, "%s", to_name);
	} else {
		if ((to_fd = open(to_name,
		    O_CREAT | O_WRONLY | O_TRUNC, S_IRUSR | S_IWUSR)) < 0)
			err(1, "%s", to_name);
	}
	if (!devnull) {
		if ((from_fd = open(from_name, O_RDONLY, 0)) < 0) {
			(void)unlink(to_name);
			err(1, "%s", from_name);
		}
		copy(from_fd, from_name, to_fd, to_name, from_sb.st_size);
		(void)close(from_fd);
	}

	if (dostrip) {
		strip(to_name);

		/*
		 * Re-open our fd on the target, in case we used a strip
		 *  that does not work in-place -- like gnu binutils strip.
		 */
		close(to_fd);
		if ((to_fd = open(to_name, O_RDONLY, S_IRUSR | S_IWUSR)) < 0)
		  err(1, "stripping %s", to_name);
	}

	/*
	 * Set owner, group, mode for target; do the chown first,
	 * chown may lose the setuid bits.
	 */
	if ((gid != -1 || uid != -1) && fchown(to_fd, uid, gid)) {
		serrno = errno;
		(void)unlink(to_name);
		errx(1, "%s: chown/chgrp: %s", to_name, strerror(serrno));
	}
	if (fchmod(to_fd, mode)) {
		serrno = errno;
		(void)unlink(to_name);
		errx(1, "%s: chmod: %s", to_name, strerror(serrno));
	}

	/*
	 * If provided a set of flags, set them, otherwise, preserve the
	 * flags, except for the dump flag.
	 */
	if (fchflags(to_fd,
	    flags & SETFLAGS ? fset : from_sb.st_flags & ~UF_NODUMP)) {
		if (errno != EOPNOTSUPP || (from_sb.st_flags & ~UF_NODUMP) != 0)
			warn("%s: chflags", to_name);
	}

	/*
	 * Preserve the date of the source file.
	 */
	if (dopreserve) {
		struct timeval tv[2];

#ifdef BSD4_4
		TIMESPEC_TO_TIMEVAL(&tv[0], &from_sb.st_atimespec);
		TIMESPEC_TO_TIMEVAL(&tv[1], &from_sb.st_mtimespec);
#else
		tv[0].tv_sec = from_sb.st_atime;
		tv[0].tv_usec = 0;
		tv[1].tv_sec = from_sb.st_mtime;
		tv[1].tv_usec = 0;
#endif
		if (futimes(to_fd, tv) == -1)
			warn("%s: futimes", to_name);
	}

	(void)close(to_fd);

	if (dorename)
		if (rename(to_name, oto_name) == -1)
			err(1, "%s: rename", to_name);

	if (!docopy && !devnull && unlink(from_name))
		err(1, "%s", from_name);
}

/*
 * copy --
 *	copy from one file to another
 */
void
copy(from_fd, from_name, to_fd, to_name, size)
	int from_fd, to_fd;
	char *from_name, *to_name;
	off_t size;
{
	ssize_t nr, nw;
	int serrno;
	char *p;
	char buf[MAXBSIZE];

	/*
	 * There's no reason to do anything other than close the file
	 * now if it's empty, so let's not bother.
	 */
	if (size > 0) {
		/*
		 * Mmap and write if less than 8M (the limit is so we don't totally
		 * trash memory on big files).  This is really a minor hack, but it
		 * wins some CPU back.
		 */
		if (size <= 8 * 1048576) {
			if ((p = mmap(NULL, (size_t)size, PROT_READ,
			    MAP_FILE|MAP_SHARED, from_fd, (off_t)0)) == MAP_FAILED) {
				serrno = errno;
				(void)unlink(to_name);
				errx(1, "%s: %s", from_name, strerror(serrno));
			}
#ifdef MADV_SEQUENTIAL
			if (madvise(p, (size_t)size, MADV_SEQUENTIAL) == -1
			    && errno != EOPNOTSUPP)
				warnx("madvise: %s", strerror(errno));
#endif

			if (write(to_fd, p, size) != size) {
				serrno = errno;
				(void)unlink(to_name);
				errx(1, "%s: %s",
				    to_name, strerror(serrno));
			}
		} else {
			while ((nr = read(from_fd, buf, sizeof(buf))) > 0) {
				if ((nw = write(to_fd, buf, nr)) != nr) {
					serrno = errno;
					(void)unlink(to_name);
					errx(1, "%s: %s",
					    to_name, strerror(nw > 0 ? EIO : serrno));
				}
			}
			if (nr != 0) {
				serrno = errno;
				(void)unlink(to_name);
				errx(1, "%s: %s", from_name, strerror(serrno));
			}
		}
	}
}

/*
 * strip --
 *	use strip(1) to strip the target file
 */
void
strip(to_name)
	char *to_name;
{
	int serrno, status;
	char *stripprog;

	switch (vfork()) {
	case -1:
		serrno = errno;
		(void)unlink(to_name);
		errx(1, "vfork: %s", strerror(serrno));
	case 0:
		stripprog = getenv("STRIP");
		if (stripprog == NULL)
			stripprog = _PATH_STRIP;

		if (stripArgs) {
			/* build up a command line and let /bin/sh parse the arguments */
			char* cmd = (char*)malloc(sizeof(char)*
						  (3+strlen(stripprog)+
						     strlen(stripArgs)+
						     strlen(to_name)));

			sprintf(cmd, "%s %s %s", stripprog, stripArgs, to_name);

			execl(_PATH_BSHELL, "sh", "-c", cmd, NULL);
		} else
			execlp(stripprog, "strip", to_name, NULL);

		warn("%s", stripprog);
		_exit(1);
	default:
		if (wait(&status) == -1 || status)
			(void)unlink(to_name);
	}
}

/*
 * backup file "to_name" to to_name.suffix
 * if suffix contains a "%", it's taken as a printf(3) pattern
 * used for a numbered backup.
 */
void
backup(to_name)
	const char *to_name;
{
	char backup[FILENAME_MAX];
	
	if (numberedbackup) {
		/* Do numbered backup */
		int cnt;
		char suffix_expanded[FILENAME_MAX];
		
		cnt=0;
		do {
			(void)snprintf(suffix_expanded, FILENAME_MAX, suffix, cnt);
			(void)snprintf(backup, FILENAME_MAX, "%s%s",
				       to_name, suffix_expanded);
			cnt++;
		} while (access(backup, F_OK)==0); 
	} else {
		/* Do simple backup */
		(void)snprintf(backup, FILENAME_MAX, "%s%s", to_name, suffix);
	}
	
	(void)rename(to_name, backup);
}

/*
 * install_dir --
 *	build directory heirarchy
 */
void
install_dir(path)
        char *path;
{
        char *p;
        struct stat sb;
        int ch;

        for (p = path;; ++p)
                if (!*p || (p != path && *p  == '/')) {
                        ch = *p;
                        *p = '\0';
                        if (stat(path, &sb)) {
                                if (errno != ENOENT || mkdir(path, 0777) < 0) {
					err(1, "%s", path);
					/* NOTREACHED */
                                }
                        }
                        if (!(*p = ch))
				break;
                }

	if (((gid != -1 || uid != -1) && chown(path, uid, gid)) ||
            chmod(path, mode)) {
                warn("%s", path);
	}
}

/*
 * usage --
 *	print a usage message and die
 */
void
usage()
{
	(void)fprintf(stderr, "\
usage: install [-Ubcps] [-B suffix] [-f flags] [-m mode] [-o owner] [-g group] [-l linkflags] [-S stripflags] file1 file2\n\
       install [-Ubcps] [-B suffix] [-f flags] [-m mode] [-o owner] [-g group] [-l linkflags] [-S stripflags] file1 ... fileN directory\n\
       install [-Up] -d [-m mode] [-o owner] [-g group] directory ...\n");
	exit(1);
}
