/*	$NetBSD: xinstall.c,v 1.67 2002/01/31 22:43:59 tv Exp $	*/

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

#if HAVE_CONFIG_H
#include "config.h"
#else
#define HAVE_FUTIMES 1
#define HAVE_STRUCT_STAT_ST_FLAGS 1
#endif

#include <sys/cdefs.h>
#if defined(__COPYRIGHT) && !defined(lint)
__COPYRIGHT("@(#) Copyright (c) 1987, 1993\n\
	The Regents of the University of California.  All rights reserved.\n");
#endif /* not lint */

#if defined(__RCSID) && !defined(lint)
#if 0
static char sccsid[] = "@(#)xinstall.c	8.1 (Berkeley) 7/21/93";
#else
__RCSID("$NetBSD: xinstall.c,v 1.67 2002/01/31 22:43:59 tv Exp $");
#endif
#endif /* not lint */

#include <sys/param.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/wait.h>

#include <ctype.h>
#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <grp.h>
#include <libgen.h>
#include <paths.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <vis.h>

#include "pathnames.h"
#include "stat_flags.h"

#define STRIP_ARGS_MAX 32
#define BACKUP_SUFFIX ".old"

int	dobackup, docopy, dodir, dostrip, dolink, dopreserve, dorename,
	    dounpriv;
int	numberedbackup;
int	mode = S_IRWXU|S_IRGRP|S_IXGRP|S_IROTH|S_IXOTH;
char	pathbuf[MAXPATHLEN];
id_t	uid, gid;
char	*group, *owner, *fflags, *tags;
FILE	*metafp;
char	*metafile;
u_long	fileflags;
char	*stripArgs;
char	*suffix = BACKUP_SUFFIX;

#define LN_ABSOLUTE	0x01
#define LN_RELATIVE	0x02
#define LN_HARD		0x04
#define LN_SYMBOLIC	0x08
#define LN_MIXED	0x10

#define	DIRECTORY	0x01		/* Tell install it's a directory. */
#define	SETFLAGS	0x02		/* Tell install to set flags. */
#define	HASUID		0x04		/* Tell install the uid was given */
#define	HASGID		0x08		/* Tell install the gid was given */

void	backup(const char *);
void	copy(int, char *, int, char *, off_t);
int	do_link(char *, char *);
void	do_symlink(char *, char *);
void	install(char *, char *, u_int);
void	install_dir(char *, u_int);
int	main(int, char *[]);
void	makelink(char *, char *);
void	metadata_log(const char *, const char *, struct timeval *, const char *);
int	parseid(char *, id_t *);
void	strip(char *);
void	usage(void);
char   *xbasename(char *);
char   *xdirname(char *);

int
main(int argc, char *argv[])
{
	struct stat	from_sb, to_sb;
	void		*set;
	u_int		iflags;
	int		ch, no_target;
	char		*p, *to_name;

	setprogname(argv[0]);

	iflags = 0;
	while ((ch = getopt(argc, argv, "cbB:df:g:l:m:M:o:prsS:T:U")) != -1)
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
			/*FALLTHROUGH*/
		case 'b':
			dobackup = 1;
			break;
		case 'c':
			docopy = 1;
			break;
		case 'd':
			dodir = 1;
			break;
#if !HAVE_CONFIG_H
		case 'f':
			fflags = optarg;
			break;
#endif
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
					/* NOTREACHED */
				}
			break;
		case 'm':
			if (!(set = setmode(optarg)))
				errx(1, "%s: invalid file mode", optarg);
			mode = getmode(set, 0);
			free(set);
			break;
		case 'M':
			metafile = optarg;
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
			stripArgs = strdup(optarg);
			if (stripArgs == NULL)
				errx(1, "%s", strerror(ENOMEM));
			/* fall through; -S implies -s */
			/*FALLTHROUGH*/
		case 's':
			dostrip = 1;
			break;
		case 'T':
			tags = optarg;
			break;
		case 'U':
			dounpriv = 1;
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
	if ((dostrip || fflags) && dolink)
		usage();

	/* must have at least two arguments, except when creating directories */
	if (argc < 2 && !dodir)
		usage();

	/* get group and owner id's */
	if (group && !dounpriv) {
		struct group *gp;

		if ((gp = getgrnam(group)) != NULL)
			gid = gp->gr_gid;
		else if (! parseid(group, &gid))
			errx(1, "unknown group %s", group);
		iflags |= HASGID;
	}
	if (owner && !dounpriv) {
		struct passwd *pp;

		if ((pp = getpwnam(owner)) != NULL)
			uid = pp->pw_uid;
		else if (! parseid(owner, &uid))
			errx(1, "unknown user %s", owner);
		iflags |= HASUID;
	}

#if !HAVE_CONFIG_H
	if (fflags && !dounpriv) {
		if (string_to_flags(&fflags, &fileflags, NULL))
			errx(1, "%s: invalid flag", fflags);
		iflags |= SETFLAGS;
	}
#endif

	if (metafile) {
		if ((metafp = fopen(metafile, "a")) == NULL)
			warn("open %s", metafile);
	}

	if (dodir) {
		for (; *argv != NULL; ++argv)
			install_dir(*argv, iflags);
		exit (0);
	}

	no_target = stat(to_name = argv[argc - 1], &to_sb);
	if (!no_target && S_ISDIR(to_sb.st_mode)) {
		for (; *argv != to_name; ++argv)
			install(*argv, to_name, iflags | DIRECTORY);
		exit(0);
	}

	/* can't do file1 file2 directory/file */
	if (argc != 2)
		usage();

	if (!no_target) {
		/* makelink() handles checks for links */
		if (!dolink) {
			if (stat(*argv, &from_sb))
				err(1, "%s: stat", *argv);
			if (!S_ISREG(to_sb.st_mode))
				errx(1, "%s: not a regular file", to_name);
			if (to_sb.st_dev == from_sb.st_dev &&
			    to_sb.st_ino == from_sb.st_ino)
				errx(1, "%s and %s are the same file", *argv,
				    to_name);
		}
		/*
		 * Unlink now... avoid ETXTBSY errors later.  Try and turn
		 * off the append/immutable bits -- if we fail, go ahead,
		 * it might work.
		 */
#if !HAVE_CONFIG_H
#define	NOCHANGEBITS	(UF_IMMUTABLE | UF_APPEND | SF_IMMUTABLE | SF_APPEND)
		if (to_sb.st_flags & NOCHANGEBITS)
			(void)chflags(to_name,
			    to_sb.st_flags & ~(NOCHANGEBITS));
#endif
		if (dobackup)
			backup(to_name);
		else if (!dorename)
			(void)unlink(to_name);
	}
	install(*argv, to_name, iflags);
	exit(0);
}

/*
 * parseid --
 *	parse uid or gid from arg into id, returning non-zero if successful
 */
int
parseid(char *name, id_t *id)
{
	char	*ep;

	errno = 0;
	*id = (id_t)strtoul(name, &ep, 10);
	if (errno || *ep != '\0')
		return (0);
	return (1);
}

/*
 * do_link --
 *	make a hard link, obeying dorename if set
 *	return -1 on failure
 */
int
do_link(char *from_name, char *to_name)
{
	char tmpl[MAXPATHLEN];
	int ret;

	if (dorename) {
		(void)snprintf(tmpl, sizeof(tmpl), "%s/inst.XXXXXX",
		    xdirname(to_name));
		/* This usage is safe. The linker will bitch anyway. */
		if (mktemp(tmpl) == NULL)
			err(1, "%s: mktemp", tmpl);
		ret = link(from_name, tmpl);
		if (ret == 0) {
			ret = rename(tmpl, to_name);
			if (ret < 0)
				/* remove temporary link before exiting */
				(void)unlink(tmpl);
		}
		return (ret);
	} else
		return (link(from_name, to_name));
}

/*
 * do_symlink --
 *	make a symbolic link, obeying dorename if set
 *	exit on failure
 */
void
do_symlink(char *from_name, char *to_name)
{
	char tmpl[MAXPATHLEN];

	if (dorename) {
		(void)snprintf(tmpl, sizeof(tmpl), "%s/inst.XXXXXX",
		    xdirname(to_name));
		/* This usage is safe. The linker will bitch anyway. */
		if (mktemp(tmpl) == NULL)
			err(1, "%s: mktemp", tmpl);

		if (symlink(from_name, tmpl) == -1)
			err(1, "symlink %s -> %s", from_name, tmpl);
		if (rename(tmpl, to_name) == -1) {
			/* remove temporary link before exiting */
			(void)unlink(tmpl);
			err(1, "%s: rename", to_name);
		}
	} else {
		if (symlink(from_name, to_name) == -1)
			err(1, "symlink %s -> %s", from_name, to_name);
	}
}

/*
 * makelink --
 *	make a link from source to destination
 */
void
makelink(char *from_name, char *to_name)
{
	char	src[MAXPATHLEN], dst[MAXPATHLEN], lnk[MAXPATHLEN];

	/* Try hard links first */
	if (dolink & (LN_HARD|LN_MIXED)) {
		if (do_link(from_name, to_name) == -1) {
			if ((dolink & LN_HARD) || errno != EXDEV)
				err(1, "link %s -> %s", from_name, to_name);
		}
		else {
			metadata_log(to_name, "hlink", NULL, from_name);
			return;
		}
	}

	/* Symbolic links */
	if (dolink & LN_ABSOLUTE) {
		/* Convert source path to absolute */
		if (realpath(from_name, src) == NULL)
			err(1, "%s: realpath", from_name);
		do_symlink(src, to_name);
		metadata_log(to_name, "link", NULL, src);
		return;
	}

	if (dolink & LN_RELATIVE) {
		char *cp, *d, *s;

		/* Resolve pathnames */
		if (realpath(from_name, src) == NULL)
			err(1, "%s: realpath", from_name);

		/*
		 * The last component of to_name may be a symlink,
		 * so use realpath to resolve only the directory.
		 */
		cp = xdirname(to_name);
		if (realpath(cp, dst) == NULL)
			err(1, "%s: realpath", cp);
		/* .. and add the last component */
		if (strcmp(dst, "/") != 0) {
			if (strlcat(dst, "/", sizeof(dst)) > sizeof(dst))
				errx(1, "resolved pathname too long");
		}
		cp = xbasename(to_name);
		if (strlcat(dst, cp, sizeof(dst)) > sizeof(dst))
			errx(1, "resolved pathname too long");

		/* trim common path components */
		for (s = src, d = dst; *s == *d; s++, d++)
			continue;
		while (*s != '/')
			s--, d--;

		/* count the number of directories we need to backtrack */
		for (++d, lnk[0] = '\0'; *d; d++)
			if (*d == '/')
				(void)strcat(lnk, "../");

		(void)strcat(lnk, ++s);

		do_symlink(lnk, dst);
		metadata_log(dst, "link", NULL, lnk);
		return;
	}

	/*
	 * If absolute or relative was not specified, 
	 * try the names the user provided
	 */
	do_symlink(from_name, to_name);
	metadata_log(to_name, "link", NULL, from_name);
}

/*
 * install --
 *	build a path name and install the file
 */
void
install(char *from_name, char *to_name, u_int flags)
{
	struct stat	from_sb;
#if !HAVE_CONFIG_H
	struct stat	to_sb;
#endif
	struct timeval	tv[2];
	int		devnull, from_fd, to_fd, serrno, tmpmode;
	char		*p, tmpl[MAXPATHLEN], *oto_name;

	if (flags & DIRECTORY || strcmp(from_name, _PATH_DEVNULL)) {
		if (!dolink) {
			if (stat(from_name, &from_sb))
				err(1, "%s: stat", from_name);
			if (!S_ISREG(from_sb.st_mode))
				errx(1, "%s: not a regular file", from_name);
		}
		/* Build the target path. */
		if (flags & DIRECTORY) {
			(void)snprintf(pathbuf, sizeof(pathbuf), "%s/%s",
			    to_name,
			    (p = strrchr(from_name, '/')) ? ++p : from_name);
			to_name = pathbuf;
		}
		devnull = 0;
	} else {
#if HAVE_STRUCT_STAT_ST_FLAGS
		from_sb.st_flags = 0;	/* XXX */
#endif
		devnull = 1;
	}

	/*
	 * Unlink now... avoid ETXTBSY errors later.  Try and turn
	 * off the append/immutable bits -- if we fail, go ahead,
	 * it might work.
	 */
#if !HAVE_CONFIG_H
	if (stat(to_name, &to_sb) == 0 &&
	    to_sb.st_flags & (NOCHANGEBITS))
		(void)chflags(to_name, to_sb.st_flags & ~(NOCHANGEBITS));
#endif
	if (dorename) {
		(void)snprintf(tmpl, sizeof(tmpl), "%s/inst.XXXXXX",
		    xdirname(to_name));
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
		makelink(from_name, dorename ? oto_name : to_name);
		return;
	}

	/* Create target. */
	if (dorename) {
		if ((to_fd = mkstemp(to_name)) == -1)
			err(1, "%s: mkstemp", to_name);
	} else {
		if ((to_fd = open(to_name,
		    O_CREAT | O_WRONLY | O_TRUNC, S_IRUSR | S_IWUSR)) < 0)
			err(1, "%s: open", to_name);
	}
	if (!devnull) {
		if ((from_fd = open(from_name, O_RDONLY, 0)) < 0) {
			(void)unlink(to_name);
			err(1, "%s: open", from_name);
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
	if (!dounpriv &&
	    (flags & (HASUID | HASGID)) && fchown(to_fd, uid, gid) == -1) {
		serrno = errno;
		(void)unlink(to_name);
		errx(1, "%s: chown/chgrp: %s", to_name, strerror(serrno));
	}
	tmpmode = mode;
	if (dounpriv)
		tmpmode &= S_IRWXU|S_IRWXG|S_IRWXO;
	if (fchmod(to_fd, tmpmode) == -1) {
		serrno = errno;
		(void)unlink(to_name);
		errx(1, "%s: chmod: %s", to_name, strerror(serrno));
	}

	/*
	 * Preserve the date of the source file.
	 */
	if (dopreserve) {
#ifdef BSD4_4
		TIMESPEC_TO_TIMEVAL(&tv[0], &from_sb.st_atimespec);
		TIMESPEC_TO_TIMEVAL(&tv[1], &from_sb.st_mtimespec);
#else
		tv[0].tv_sec = from_sb.st_atime;
		tv[0].tv_usec = 0;
		tv[1].tv_sec = from_sb.st_mtime;
		tv[1].tv_usec = 0;
#endif
#if HAVE_FUTIMES
		if (futimes(to_fd, tv) == -1)
			warn("%s: futimes", to_name);
#else
		if (utimes(to_name, tv) == -1)
			warn("%s: utimes", to_name);
#endif
	}

	(void)close(to_fd);

	if (dorename) {
		if (rename(to_name, oto_name) == -1)
			err(1, "%s: rename", to_name);
		to_name = oto_name;
	}

	if (!docopy && !devnull && unlink(from_name))
		err(1, "%s: unlink", from_name);

	/*
	 * If provided a set of flags, set them, otherwise, preserve the
	 * flags, except for the dump flag.
	 */
#if !HAVE_CONFIG_H
	if (!dounpriv && chflags(to_name,
	    flags & SETFLAGS ? fileflags : from_sb.st_flags & ~UF_NODUMP) == -1)
	{
		if (errno != EOPNOTSUPP || (from_sb.st_flags & ~UF_NODUMP) != 0)
			warn("%s: chflags", to_name);
	}
#endif

	metadata_log(to_name, "file", tv, NULL);
}

/*
 * copy --
 *	copy from one file to another
 */
void
copy(int from_fd, char *from_name, int to_fd, char *to_name, off_t size)
{
	ssize_t	nr, nw;
	int	serrno;
	char	*p;
	char	buf[MAXBSIZE];

	/*
	 * There's no reason to do anything other than close the file
	 * now if it's empty, so let's not bother.
	 */
	if (size > 0) {

		/*
		 * Mmap and write if less than 8M (the limit is so we
		 * don't totally trash memory on big files).  This is
		 * really a minor hack, but it wins some CPU back.
		 */

		if (size <= 8 * 1048576) {
			if ((p = mmap(NULL, (size_t)size, PROT_READ,
			    MAP_FILE|MAP_SHARED, from_fd, (off_t)0))
			    == MAP_FAILED) {
				goto mmap_failed;
			}
#ifdef MADV_SEQUENTIAL
			if (madvise(p, (size_t)size, MADV_SEQUENTIAL) == -1
			    && errno != EOPNOTSUPP)
				warnx("madvise: %s", strerror(errno));
#endif

			if (write(to_fd, p, size) != size) {
				serrno = errno;
				(void)unlink(to_name);
				errx(1, "%s: write: %s",
				    to_name, strerror(serrno));
			}
		} else {
mmap_failed:
			while ((nr = read(from_fd, buf, sizeof(buf))) > 0) {
				if ((nw = write(to_fd, buf, nr)) != nr) {
					serrno = errno;
					(void)unlink(to_name);
					errx(1, "%s: write: %s", to_name,
					    strerror(nw > 0 ? EIO : serrno));
				}
			}
			if (nr != 0) {
				serrno = errno;
				(void)unlink(to_name);
				errx(1, "%s: read: %s", from_name, strerror(serrno));
			}
		}
	}
}

/*
 * strip --
 *	use strip(1) to strip the target file
 */
void
strip(char *to_name)
{
	int	serrno, status;
	char	*stripprog;

	switch (vfork()) {
	case -1:
		serrno = errno;
		(void)unlink(to_name);
		errx(1, "vfork: %s", strerror(serrno));
		/*NOTREACHED*/
	case 0:
		stripprog = getenv("STRIP");
		if (stripprog == NULL)
			stripprog = _PATH_STRIP;

		if (stripArgs) {
			/*
			 * build up a command line and let /bin/sh
			 * parse the arguments
			 */
			char* cmd = (char*)malloc(sizeof(char)*
						  (3+strlen(stripprog)+
						     strlen(stripArgs)+
						     strlen(to_name)));

			if (cmd == NULL)
				errx(1, "%s", strerror(ENOMEM));

			sprintf(cmd, "%s %s %s", stripprog, stripArgs, to_name);

			execl(_PATH_BSHELL, "sh", "-c", cmd, NULL);
		} else
			execlp(stripprog, "strip", to_name, NULL);

		warn("%s: exec of strip", stripprog);
		_exit(1);
		/*NOTREACHED*/
	default:
		if (wait(&status) == -1 || status)
			(void)unlink(to_name);
	}
}

/*
 * backup --
 *	backup file "to_name" to to_name.suffix
 *	if suffix contains a "%", it's taken as a printf(3) pattern
 *	used for a numbered backup.
 */
void
backup(const char *to_name)
{
	char	bname[FILENAME_MAX];
	
	if (numberedbackup) {
		/* Do numbered backup */
		int cnt;
		char suffix_expanded[FILENAME_MAX];
		
		cnt=0;
		do {
			(void)snprintf(suffix_expanded, FILENAME_MAX, suffix,
			    cnt);
			(void)snprintf(bname, FILENAME_MAX, "%s%s", to_name,
			    suffix_expanded);
			cnt++;
		} while (access(bname, F_OK) == 0); 
	} else {
		/* Do simple backup */
		(void)snprintf(bname, FILENAME_MAX, "%s%s", to_name, suffix);
	}
	
	(void)rename(to_name, bname);
}

/*
 * install_dir --
 *	build directory hierarchy
 */
void
install_dir(char *path, u_int flags)
{
        char		*p;
        struct stat	sb;
        int		ch;

        for (p = path;; ++p)
                if (!*p || (p != path && *p  == '/')) {
                        ch = *p;
                        *p = '\0';
                        if (stat(path, &sb)) {
                                if (errno != ENOENT || mkdir(path, 0777) < 0) {
					err(1, "%s: mkdir", path);
                                }
                        }
                        if (!(*p = ch))
				break;
                }

	if (!dounpriv && (
	    ((flags & (HASUID | HASGID)) && chown(path, uid, gid) == -1)
	    || chmod(path, mode) == -1 )) {
                warn("%s: chown/chmod", path);
	}
	metadata_log(path, "dir", NULL, NULL);
}

/*
 * metadata_log --
 *	if metafp is not NULL, output mtree(8) full path name and settings to
 *	metafp, to allow permissions to be set correctly by other tools.
 */
void
metadata_log(const char *path, const char *type, struct timeval *tv,
	const char *link)
{
	const char	extra[] = { ' ', '\t', '\n', '\\', '#', '\0' };
	char		*buf;

	if (!metafp)	
		return;
	buf = (char *)malloc(4 * strlen(path) + 1);	/* buf for strsvis(3) */
	if (buf == NULL) {
		warnx("%s", strerror(ENOMEM));
		return;
	}
	if (flock(fileno(metafp), LOCK_EX) == -1) {	/* lock log file */
		warn("can't lock %s", metafile);
		return;
	}

	strsvis(buf, path, VIS_CSTYLE, extra);		/* encode name */
	fprintf(metafp, ".%s%s type=%s mode=%#o",	/* print details */
	    buf[0] == '/' ? "" : "/", buf, type, mode);
	if (link)
		fprintf(metafp, " link=%s", link);
	if (owner)
		fprintf(metafp, " uname=%s", owner);
	if (group)
		fprintf(metafp, " gname=%s", group);
	if (fflags)
		fprintf(metafp, " flags=%s", fflags);
	if (tags)
		fprintf(metafp, " tags=%s", tags);
	if (tv != NULL && dopreserve)
		fprintf(metafp, " time=%ld.%ld", tv[1].tv_sec, tv[1].tv_usec);
	fputc('\n', metafp);
	fflush(metafp);					/* flush output */
	if (flock(fileno(metafp), LOCK_UN) == -1) {	/* unlock log file */
		warn("can't unlock %s", metafile);
	}
	free(buf);
}

/*
 * xbasename --
 *	libc basename(3) that returns a pointer to a static buffer
 *	instead of overwriting that passed-in string.
 */
char *
xbasename(char *path)
{
	static char tmp[MAXPATHLEN];

	(void)strlcpy(tmp, path, sizeof(tmp));
	return (basename(tmp));
}

/*
 * xdirname --
 *	libc dirname(3) that returns a pointer to a static buffer
 *	instead of overwriting that passed-in string.
 */
char *
xdirname(char *path)
{
	static char tmp[MAXPATHLEN];

	(void)strlcpy(tmp, path, sizeof(tmp));
	return (dirname(tmp));
}

/*
 * usage --
 *	print a usage message and die
 */
void
usage(void)
{

	(void)fprintf(stderr, "\
usage: install [-Ubcprs] [-M log] [-T tags] [-B suffix] [-f flags] [-m mode]\n\
	    [-o owner] [-g group] [-l linkflags] [-S stripflags] file1 file2\n\
       install [-Ubcprs] [-M log] [-T tags] [-B suffix] [-f flags] [-m mode]\n\
	    [-o owner] [-g group] [-l linkflags] [-S stripflags]\n\
	    file1 ... fileN directory\n\
       install [-Up] [-M log] [-T tags] -d [-m mode]\n\
	    [-o owner] [-g group] directory ...\n");
	exit(1);
}
