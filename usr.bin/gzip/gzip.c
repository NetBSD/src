/*	$NetBSD: gzip.c,v 1.29.2.4 2004/04/07 21:41:00 jmc Exp $	*/

/*
 * Copyright (c) 1997, 1998, 2003 Matthew R. Green
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
#ifndef lint
__COPYRIGHT("@(#) Copyright (c) 1997, 1998, 2003 Matthew R. Green\n\
     All rights reserved.\n");
__RCSID("$NetBSD: gzip.c,v 1.29.2.4 2004/04/07 21:41:00 jmc Exp $");
#endif /* not lint */

/*
 * gzip.c -- GPL free gzip using zlib.
 *
 * very minor portions of this code are (very loosely) derived from
 * the minigzip.c in the zlib distribution.
 *
 * TODO:
 *	- handle .taz/.tgz files?
 *	- use mmap where possible
 *	- handle some signals better (remove outfile?)
 */

#include <sys/param.h>
#include <sys/stat.h>
#include <sys/time.h>

#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <zlib.h>
#include <fts.h>
#include <libgen.h>
#include <stdarg.h>
#include <getopt.h>

/* what type of file are we dealing with */
enum filetype {
	FT_GZIP,
#ifndef NO_BZIP2_SUPPORT
	FT_BZIP2,
#endif
#ifndef NO_COMPRESS_SUPPORT
	FT_Z,
#endif
	FT_LAST,
	FT_UNKNOWN
};

#ifndef NO_BZIP2_SUPPORT
#include <bzlib.h>

#define BZ2_SUFFIX	".bz2"
#define BZIP2_MAGIC	"\102\132\150"
#endif

#ifndef NO_COMPRESS_SUPPORT
#define Z_SUFFIX	".Z"
#define Z_MAGIC		"\037\235"
#endif

#define GZ_SUFFIX	".gz"

#define BUFLEN		(32 * 1024)

#define GZIP_MAGIC0	0x1F
#define GZIP_MAGIC1	0x8B
#define GZIP_OMAGIC1	0x9E

#define ORIG_NAME 	0x08

/* Define this if you have the NetBSD gzopenfull(3) extension to zlib(3) */
#ifndef HAVE_ZLIB_GZOPENFULL
#define HAVE_ZLIB_GZOPENFULL 1
#endif

static	const char	gzip_version[] = "NetBSD gzip 2.2";

static	char	gzipflags[3];		/* `w' or `r', possible with [1-9] */
static	int	cflag;			/* stdout mode */
static	int	dflag;			/* decompress mode */
static	int	lflag;			/* list mode */

#ifndef SMALL
static	int	fflag;			/* force mode */
static	int	nflag;			/* don't save name/timestamp */
static	int	Nflag;			/* don't restore name/timestamp */
static	int	qflag;			/* quiet mode */
static	int	rflag;			/* recursive mode */
static	int	tflag;			/* test */
static	char	*Sflag;
static	int	vflag;			/* verbose mode */
#else
#define		qflag	0
#endif

static	char	*suffix;
#define suffix_len	(strlen(suffix) + 1)	/* len + nul */
static	char	*newfile;		/* name of newly created file */
static	char	*infile;		/* name of file coming in */

static	void	maybe_err(int rv, const char *fmt, ...);
static	void	maybe_errx(int rv, const char *fmt, ...);
static	void	maybe_warn(const char *fmt, ...);
static	void	maybe_warnx(const char *fmt, ...);
static	enum filetype file_gettype(u_char *);
static	void	gz_compress(FILE *, gzFile);
static	off_t	gz_uncompress(gzFile, FILE *);
static	off_t	file_compress(char *);
static	off_t	file_uncompress(char *);
static	void	handle_pathname(char *);
static	void	handle_file(char *, struct stat *);
static	void	handle_stdin(void);
static	void	handle_stdout(void);
static	void	print_ratio(off_t, off_t, FILE *);
static	void	print_list(int fd, off_t, const char *, time_t);
static	void	usage(void);
static	void	display_version(void);

#ifndef SMALL
static	void	prepend_gzip(char *, int *, char ***);
static	void	handle_dir(char *, struct stat *);
static	void	print_verbage(char *, char *, off_t, off_t);
static	void	print_test(char *, int);
static	void	copymodes(const char *, struct stat *);
#endif

#ifndef NO_BZIP2_SUPPORT
static	off_t	unbzip2(int, int);
#endif

#ifndef NO_COMPRESS_SUPPORT
static	FILE 	*zopen(const char *);
static	off_t	zuncompress(FILE *, FILE *);
#endif

int main(int, char *p[]);

#ifdef SMALL
#define getopt_long(a,b,c,d,e) getopt(a,b,c)
#else
static const struct option longopts[] = {
	{ "stdout",		no_argument,		0,	'c' },
	{ "to-stdout",		no_argument,		0,	'c' },
	{ "decompress",		no_argument,		0,	'd' },
	{ "uncompress",		no_argument,		0,	'd' },
	{ "force",		no_argument,		0,	'f' },
	{ "help",		no_argument,		0,	'h' },
	{ "list",		no_argument,		0,	'l' },
	{ "no-name",		no_argument,		0,	'n' },
	{ "name",		no_argument,		0,	'N' },
	{ "quiet",		no_argument,		0,	'q' },
	{ "recursive",		no_argument,		0,	'r' },
	{ "suffix",		required_argument,	0,	'S' },
	{ "test",		no_argument,		0,	't' },
	{ "verbose",		no_argument,		0,	'v' },
	{ "version",		no_argument,		0,	'V' },
	{ "fast",		no_argument,		0,	'1' },
	{ "best",		no_argument,		0,	'9' },
#if 0
	/*
	 * This is what else GNU gzip implements.  --ascii isn't useful
	 * on NetBSD, and I don't care to have a --license.
	 */
	{ "ascii",		no_argument,		0,	'a' },
	{ "license",		no_argument,		0,	'L' },
#endif
	{ NULL,			no_argument,		0,	0 },
};
#endif

int
main(int argc, char **argv)
{
	const char *progname = getprogname();
#ifndef SMALL
	char *gzip;
#endif
	int ch;

	/* XXX set up signals */

	gzipflags[0] = 'w';
	gzipflags[1] = '\0';

	suffix = GZ_SUFFIX;;

#ifndef SMALL
	if ((gzip = getenv("GZIP")) != NULL)
		prepend_gzip(gzip, &argc, &argv);
#endif

	/*
	 * XXX
	 * handle being called `gunzip', `zcat' and `gzcat'
	 */
	if (strcmp(progname, "gunzip") == 0)
		dflag = 1;
	else if (strcmp(progname, "zcat") == 0 ||
		 strcmp(progname, "gzcat") == 0)
		dflag = cflag = 1;

#ifdef SMALL
#define OPT_LIST "cdhHltV123456789"
#else
#define OPT_LIST "cdfhHlnNqrS:tvV123456789"
#endif

	while ((ch = getopt_long(argc, argv, OPT_LIST, longopts, NULL)) != -1)
		switch (ch) {
		case 'c':
			cflag = 1;
			break;
		case 'd':
			dflag = 1;
			break;
		case 'l':
			lflag = 1;
			dflag = 1;
			break;
		case 'V':
			display_version();
			/* NOTREACHED */
		case '1': case '2': case '3':
		case '4': case '5': case '6':
		case '7': case '8': case '9':
			gzipflags[1] = (char)ch;
			gzipflags[2] = '\0';
			break;
#ifndef SMALL
		case 'f':
			fflag = 1;
			break;
		case 'n':
			nflag = 1;
			Nflag = 0;
			break;
		case 'N':
			nflag = 0;
			Nflag = 1;
			break;
		case 'q':
			qflag = 1;
			break;
		case 'r':
			rflag = 1;
			break;
		case 'S':
			Sflag = optarg;
			break;
		case 't':
			cflag = 1;
			tflag = 1;
			dflag = 1;
			break;
		case 'v':
			vflag = 1;
			break;
#endif
		default:
			usage();
			/* NOTREACHED */
		}
	argv += optind;
	argc -= optind;
	if (dflag)
		gzipflags[0] = 'r';

	if (argc == 0) {
		if (dflag)	/* stdin mode */
			handle_stdin();
		else		/* stdout mode */
			handle_stdout();
	} else {
		do {
			handle_pathname(argv[0]);
		} while (*++argv);
	}
#ifndef SMALL
	if (qflag == 0 && lflag && argc > 1)
		print_list(-1, 0, "(totals)", 0);
#endif
	exit(0);
}

/* maybe print a warning */
void
maybe_warn(const char *fmt, ...)
{
	va_list ap;

	if (qflag == 0) {
		va_start(ap, fmt);
		vwarn(fmt, ap);
		va_end(ap);
	}
}

void
maybe_warnx(const char *fmt, ...)
{
	va_list ap;

	if (qflag == 0) {
		va_start(ap, fmt);
		vwarnx(fmt, ap);
		va_end(ap);
	}
}

/* maybe print a warning */
void
maybe_err(int rv, const char *fmt, ...)
{
	va_list ap;

	if (qflag == 0) {
		va_start(ap, fmt);
		vwarn(fmt, ap);
		va_end(ap);
	}
	exit(rv);
}

/* maybe print a warning */
void
maybe_errx(int rv, const char *fmt, ...)
{
	va_list ap;

	if (qflag == 0) {
		va_start(ap, fmt);
		vwarnx(fmt, ap);
		va_end(ap);
	}
	exit(rv);
}

#ifndef SMALL
/* split up $GZIP and prepend it to the argument list */
static void
prepend_gzip(char *gzip, int *argc, char ***argv)
{
	char *s, **nargv, **ac;
	int nenvarg = 0, i;

	/* scan how many arguments there are */
	for (s = gzip; *s; s++) {
		if (*s == ' ' || *s == '\t')
			continue;
		nenvarg++;
		for (; *s; s++)
			if (*s == ' ' || *s == '\t')
				break;
		if (*s == 0)
			break;
	}
	/* punt early */
	if (nenvarg == 0)
		return;

	*argc += nenvarg;
	ac = *argv;

	nargv = (char **)malloc((*argc + 1) * sizeof(char *));
	if (nargv == NULL)
		maybe_err(1, "malloc");

	/* stash this away */
	*argv = nargv;

	/* copy the program name first */
	i = 0;
	nargv[i++] = *(ac++);

	/* take a copy of $GZIP and add it to the array */
	s = strdup(gzip);
	if (s == NULL)
		maybe_err(1, "strdup");
	for (; *s; s++) {
		if (*s == ' ' || *s == '\t')
			continue;
		nargv[i++] = s;
		for (; *s; s++)
			if (*s == ' ' || *s == '\t') {
				*s = 0;
				break;
			}
	}

	/* copy the original arguments and a NULL */
	while (*ac)
		nargv[i++] = *(ac++);
	nargv[i] = NULL;
}
#endif

/* compress input to output then close both files */
static void
gz_compress(FILE *in, gzFile out)
{
	char buf[BUFLEN];
	ssize_t len;
	int i;

	for (;;) {
		len = fread(buf, 1, sizeof(buf), in);
		if (ferror(in))
			maybe_err(1, "fread");
		if (len == 0)
			break;

		if ((ssize_t)gzwrite(out, buf, len) != len)
			maybe_err(1, gzerror(out, &i));
	}
	if (fclose(in) < 0)
		maybe_err(1, "failed fclose");
	if (gzclose(out) != Z_OK)
		maybe_err(1, "failed gzclose");
}

/* uncompress input to output then close the input */
static off_t
gz_uncompress(gzFile in, FILE *out)
{
	char buf[BUFLEN];
	off_t size;
	ssize_t len;
	int i;

	for (size = 0;;) {
		len = gzread(in, buf, sizeof(buf));

		if (len < 0) {
#ifndef SMALL
			if (tflag) {
				print_test(infile, 0);
				return (0);
			} else
#endif
				maybe_errx(1, gzerror(in, &i));
		} else if (len == 0) {
#ifndef SMALL
			if (tflag)
				print_test(infile, 1);
#endif
			break;
		}

		size += len;

#ifndef SMALL
		/* don't write anything with -t */
		if (tflag)
			continue;
#endif

		if (fwrite(buf, 1, (unsigned)len, out) != (ssize_t)len)
			maybe_err(1, "failed fwrite");
	}
	if (gzclose(in) != Z_OK)
		maybe_errx(1, "failed gzclose");

	return (size);
}

#ifndef SMALL
/*
 * set the owner, mode, flags & utimes for a file
 */
static void
copymodes(const char *file, struct stat *sbp)
{
	struct timeval times[2];

	/*
	 * If we have no info on the input, give this file some
	 * default values and return..
	 */
	if (sbp == NULL) {
		mode_t mask = umask(022);

		(void)chmod(file, DEFFILEMODE & ~mask);
		(void)umask(mask);
		return; 
	}

	/* if the chown fails, remove set-id bits as-per compress(1) */
	if (chown(file, sbp->st_uid, sbp->st_gid) < 0) {
		if (errno != EPERM)
			maybe_warn("couldn't chown: %s", file);
		sbp->st_mode &= ~(S_ISUID|S_ISGID);
	}

	/* we only allow set-id and the 9 normal permission bits */
	sbp->st_mode &= S_ISUID|S_ISGID|S_IRWXU|S_IRWXG|S_IRWXO;
	if (chmod(file, sbp->st_mode) < 0)
		maybe_warn("couldn't chmod: %s", file);

	/* only try flags if they exist already */
        if (sbp->st_flags != 0 && chflags(file, sbp->st_flags) < 0)
		maybe_warn("couldn't chflags: %s", file);

	TIMESPEC_TO_TIMEVAL(&times[0], &sbp->st_atimespec);
	TIMESPEC_TO_TIMEVAL(&times[1], &sbp->st_mtimespec);
	if (utimes(file, times) < 0)
		maybe_warn("couldn't utimes: %s", file);
}
#endif

/* what sort of file is this? */
static enum filetype
file_gettype(u_char *buf)
{

	if (buf[0] == GZIP_MAGIC0 &&
	    (buf[1] == GZIP_MAGIC1 || buf[1] == GZIP_OMAGIC1))
		return FT_GZIP;
	else
#ifndef NO_BZIP2_SUPPORT
	if (memcmp(buf, BZIP2_MAGIC, 3) == 0 &&
	    buf[3] >= '0' && buf[3] <= '9')
		return FT_BZIP2;
	else
#endif
#ifndef NO_COMPRESS_SUPPORT
	if (memcmp(buf, Z_MAGIC, 2) == 0)
		return FT_Z;
	else
#endif
		return FT_UNKNOWN;
}

/*
 * compress the given file: create a corresponding .gz file and remove the
 * original.
 */
static off_t
file_compress(char *file)
{
	FILE *in;
	gzFile out;
	struct stat isb, osb;
	char outfile[MAXPATHLEN];
	off_t size;
#ifndef SMALL
	u_int32_t mtime = 0;
#endif

	if (cflag == 0) {
		(void)strncpy(outfile, file, MAXPATHLEN - suffix_len);
		outfile[MAXPATHLEN - suffix_len] = '\0';
		(void)strlcat(outfile, suffix, sizeof(outfile));

#ifndef SMALL
		if (fflag == 0) {
			if (stat(outfile, &osb) == 0) {
				maybe_warnx("%s already exists -- skipping",
					      outfile);
				goto lose;
			}
		}
		if (stat(file, &isb) == 0) {
			if (isb.st_nlink > 1 && fflag == 0) {
				maybe_warnx("%s has %d other link%s -- "
					    "skipping", file, isb.st_nlink - 1,
					    isb.st_nlink == 1 ? "" : "s");
				goto lose;
			}
			if (nflag == 0)
				mtime = (u_int32_t)isb.st_mtime;
		}
#endif
	}
	in = fopen(file, "r");
	if (in == 0)
		maybe_err(1, "can't fopen %s", file);

	if (cflag == 0) {
#if HAVE_ZLIB_GZOPENFULL && !defined(SMALL)
		char *savename;

		if (nflag == 0)
			savename = basename(file);
		else
			savename = NULL;
		out = gzopenfull(outfile, gzipflags, savename, mtime);
#else
		out = gzopen(outfile, gzipflags);
#endif
	} else
		out = gzdopen(STDOUT_FILENO, gzipflags);

	if (out == 0)
		maybe_err(1, "can't gz%sopen %s",
		    cflag ? "d"         : "",
		    cflag ? "stdout"    : outfile);

	gz_compress(in, out);

	/*
	 * if we compressed to stdout, we don't know the size and
	 * we don't know the new file name, punt.  if we can't stat
	 * the file, whine, otherwise set the size from the stat
	 * buffer.  we only blow away the file if we can stat the
	 * output, just in case.
	 */
	if (cflag == 0) {
		if (stat(outfile, &osb) < 0) {
			maybe_warn("couldn't stat: %s", outfile);
			maybe_warnx("leaving original %s", file);
			size = 0;
		} else {
			unlink(file);
			size = osb.st_size;
		}
		newfile = outfile;
#ifndef SMALL
		copymodes(outfile, &isb);
#endif
	} else {
lose:
		size = 0;
		newfile = 0;
	}

	return (size);
}

/* uncompress the given file and remove the original */
static off_t
file_uncompress(char *file)
{
	struct stat isb, osb;
	char buf[PATH_MAX];
	char *outfile = buf, *s;
	FILE *out;
	gzFile in;
	off_t size;
	ssize_t len = strlen(file);
	int fd;
	unsigned char header1[10], name[PATH_MAX + 1];
	enum filetype method;

	/* gather the old name info */

	fd = open(file, O_RDONLY);
	if (fd < 0)
		maybe_err(1, "can't open %s", file);
	if (read(fd, header1, 10) != 10) {
		/* we don't want to fail here. */
#ifndef SMALL
		if (fflag)
			goto close_it;
#endif
		maybe_err(1, "can't read %s", file);
	}

	method = file_gettype(header1);

#ifndef SMALL
	if (Sflag == NULL) {
# ifndef NO_BZIP2_SUPPORT
		if (method == FT_BZIP2)
			suffix = BZ2_SUFFIX;
		else
# endif
# ifndef NO_COMPRESS_SUPPORT
		if (method == FT_Z)
			suffix = Z_SUFFIX;
# endif
	}

	if (fflag == 0 && method == FT_UNKNOWN)
		maybe_errx(1, "%s: not in gzip format", file);
#endif

	if (cflag == 0 || lflag) {
		s = &file[len - suffix_len + 1];
		if (strncmp(s, suffix, suffix_len) == 0) {
			(void)strncpy(outfile, file, len - suffix_len + 1);
			outfile[len - suffix_len + 1] = '\0';
		} else if (lflag == 0)
			maybe_errx(1, "unknown suffix %s", s);
	}

#ifdef SMALL
	if (method == FT_GZIP && lflag)
#else
	if (method == FT_GZIP && (Nflag || lflag))
#endif
	{
		if (header1[3] & ORIG_NAME) {
			size_t rbytes;
			int i;

			rbytes = read(fd, name, PATH_MAX + 1);
			if (rbytes < 0)
				maybe_err(1, "can't read %s", file);
			for (i = 0; i < rbytes && name[i]; i++)
				;
			if (i < rbytes) {
				name[i] = 0;
				/* now maybe merge old dirname */
				if (strchr(outfile, '/') == 0)
					outfile = name;
				else {
					char *dir = dirname(outfile);
					if (asprintf(&outfile, "%s/%s", dir,
					    name) == -1)
						maybe_err(1, "malloc");
				}
			}
		}
	}
#ifndef SMALL
close_it:
#endif
	close(fd);

	if (cflag == 0 || lflag) {
#ifndef SMALL
		if (fflag == 0 && lflag == 0 && stat(outfile, &osb) == 0) {
			maybe_warnx("%s already exists -- skipping", outfile);
			goto lose;
		}
#endif
		if (stat(file, &isb) == 0) {
#ifndef SMALL
			if (isb.st_nlink > 1 && lflag == 0 && fflag == 0) {
				maybe_warnx("%s has %d other links -- skipping",
				    file, isb.st_nlink - 1);
				goto lose;
			}
#endif
		} else
			goto lose;
	}

#ifndef NO_BZIP2_SUPPORT
	if (method == FT_BZIP2) {
		int in, out;

		/* XXX */
		if (lflag)
			maybe_errx(1, "no -l with bzip2 files");

		if ((in = open(file, O_RDONLY)) == -1)
			maybe_err(1, "open for read: %s", file);
		if (cflag == 1)
			out = STDOUT_FILENO;
		else 
			out = open(outfile, O_WRONLY|O_CREAT|O_EXCL, 0600);
		if (out == -1)
			maybe_err(1, "open for write: %s", outfile);

		if ((size = unbzip2(in, out)) == 0) {
			unlink(outfile);
			goto lose;
		}
	} else
#endif

#ifndef NO_COMPRESS_SUPPORT
	if (method == FT_Z) {
		FILE *in, *out;
		int fd;

		/* XXX */
		if (lflag)
			maybe_errx(1, "no -l with Lempel-Ziv files");

		if ((in = zopen(file)) == NULL)
			maybe_err(1, "open for read: %s", file);

		if (cflag == 1)
			fd = STDOUT_FILENO;
		else {
			fd = open(outfile, O_WRONLY|O_CREAT|O_EXCL, 0600);
			if (fd == -1)
				maybe_err(1, "open for write: %s", outfile);
		}
		out = fdopen(fd, "w");
		if (out == NULL)
			maybe_err(1, "open for write: %s", outfile);

		if ((size = zuncompress(in, out)) == 0) {
			unlink(outfile);
			goto lose;
		}
		if (ferror(in) || fclose(in)) {
			unlink(outfile);
			maybe_err(1, "failed infile fclose");
		}
		if (fclose(out)) {
			unlink(outfile);
			maybe_err(1, "failed outfile close");
		}
	} else
#endif
	{
		if (lflag) {
			int fd;

			if ((fd = open(file, O_RDONLY)) == -1)
				maybe_err(1, "open");
			print_list(fd, isb.st_size, outfile, isb.st_mtime);
			return 0;	/* XXX */
		}

		in = gzopen(file, gzipflags);
		if (in == NULL)
			maybe_err(1, "can't gzopen %s", file);

		if (cflag == 0) {
			int fd;

			/* Use open(2) directly to get a safe file.  */
			fd = open(outfile, O_WRONLY|O_CREAT|O_EXCL, 0600);
			if (fd < 0)
				maybe_err(1, "can't open %s", outfile);
			out = fdopen(fd, "w");
			if (out == NULL)
				maybe_err(1, "can't fdopen %s", outfile);
		} else
			out = stdout;

		if ((size = gz_uncompress(in, out)) == 0) {
			unlink(outfile);
			goto lose;
		}
		if (fclose(out))
			maybe_err(1, "failed fclose");
	}

	/* if testing, or we uncompressed to stdout, this is all we need */
#ifndef SMALL
	if (tflag)
		return (size);
#endif
	if (cflag)
		return (size);

	/*
	 * if we create a file...
	 */
	if (cflag == 0) {
		/*
		 * if we can't stat the file, or we are uncompressing to
		 * stdin, don't remove the file.
		 */
		if (stat(outfile, &osb) < 0) {
			maybe_warn("couldn't stat (leaving original): %s",
				   outfile);
			goto lose;
		}
		if (osb.st_size != size) {
			maybe_warn("stat gave different size: %llu != %llu "
			    "(leaving original)",
			    (unsigned long long)size,
			    (unsigned long long)osb.st_size);
			goto lose;
		}
		newfile = outfile;
		unlink(file);
		size = osb.st_size;
#ifndef SMALL
		copymodes(outfile, &isb);
#endif
	}
	return (size);

lose:
	newfile = 0;
	return 0;
}

static void
handle_stdin(void)
{
	gzFile *file;

#ifndef SMALL
	if (fflag == 0 && lflag == 0 && isatty(STDIN_FILENO)) {
		maybe_warnx("standard input is a terminal -- ignoring");
		return;
	}
#endif

	if (lflag) {
		struct stat isb;

		if (fstat(STDIN_FILENO, &isb) < 0)
			maybe_err(1, "fstat");
		print_list(STDIN_FILENO, isb.st_size, "stdout", isb.st_mtime);
		return;
	}

	file = gzdopen(STDIN_FILENO, gzipflags);
	if (file == NULL)
		maybe_err(1, "can't gzdopen stdin");
	gz_uncompress(file, stdout);
}

static void
handle_stdout(void)
{
	gzFile *file;

#ifndef SMALL
	if (fflag == 0 && isatty(STDOUT_FILENO)) {
		maybe_warnx("standard output is a terminal -- ignoring");
		return;
	}
#endif
	file = gzdopen(STDOUT_FILENO, gzipflags);
	if (file == NULL)
		maybe_err(1, "can't gzdopen stdout");
	gz_compress(stdin, file);
}

/* do what is asked for, for the path name */
static void
handle_pathname(char *path)
{
	char *opath = path, *s = 0;
	ssize_t len;
	struct stat sb;

	/* check for stdout/stdin */
	if (path[0] == '-' && path[1] == '\0') {
		if (dflag)
			handle_stdin();
		else
			handle_stdout();
	}

retry:
	if (stat(path, &sb) < 0) {
		/* lets try <path>.gz if we're decompressing */
		if (dflag && s == 0 && errno == ENOENT) {
			len = strlen(path);
			s = malloc(len + suffix_len);
			if (s == 0)
				maybe_err(1, "malloc");
			memmove(s, path, len);
			memmove(&s[len], suffix, suffix_len);
			path = s;
			goto retry;
		}
		maybe_warn("can't stat: %s", opath);
		goto out;
	}

	if (S_ISDIR(sb.st_mode)) {
#ifndef SMALL
		if (rflag)
			handle_dir(path, &sb);
		else
#endif
			maybe_warn("%s is a directory", path);
		goto out;
	}

	if (S_ISREG(sb.st_mode))
		handle_file(path, &sb);

out:
	if (s)
		free(s);
}

/* compress/decompress a file */
static void
handle_file(char *file, struct stat *sbp)
{
	off_t usize, gsize;

	infile = file;
	if (dflag) {
		usize = file_uncompress(file);
		if (usize == 0)
			return;
		gsize = sbp->st_size;
	} else {
		gsize = file_compress(file);
		if (gsize == 0)
			return;
		usize = sbp->st_size;
	}


#ifndef SMALL
	if (vflag && !tflag)
		print_verbage(file, cflag == 0 ? newfile : 0, usize, gsize);
#endif
}

#ifndef SMALL
/* this is used with -r to recursively decend directories */
static void
handle_dir(char *dir, struct stat *sbp)
{
	char *path_argv[2];
	FTS *fts;
	FTSENT *entry;

	path_argv[0] = dir;
	path_argv[1] = 0;
	fts = fts_open(path_argv, FTS_PHYSICAL, NULL);
	if (fts == NULL) {
		warn("couldn't fts_open %s", dir);
		return;
	}

	while ((entry = fts_read(fts))) {
		switch(entry->fts_info) {
		case FTS_D:
		case FTS_DP:
			continue;

		case FTS_DNR:
		case FTS_ERR:
		case FTS_NS:
			maybe_warn("%s", entry->fts_path);
			continue;
		case FTS_F:
			handle_file(entry->fts_name, entry->fts_statp);
		}
	}
	(void)fts_close(fts);
}
#endif

/* print a ratio */
static void
print_ratio(off_t in, off_t out, FILE *where)
{
	u_int64_t percent;

	if (out == 0)
		percent = 0;
	else
		percent = 1000 - ((in * 1000ULL) / out);
	fprintf(where, "%3lu.%1lu%%", (unsigned long)percent / 10UL,
	    (unsigned long)percent % 10);
}

#ifndef SMALL
/* print compression statistics, and the new name (if there is one!) */
static void
print_verbage(char *file, char *nfile, off_t usize, off_t gsize)
{
	fprintf(stderr, "%s:%s  ", file,
	    strlen(file) < 7 ? "\t\t" : "\t");
	print_ratio((off_t)usize, (off_t)gsize, stderr);
	if (nfile)
		fprintf(stderr, " -- replaced with %s", nfile);
	fprintf(stderr, "\n");
	fflush(stderr);
}

/* print test results */
static void
print_test(char *file, int ok)
{

	fprintf(stderr, "%s:%s  %s\n", file,
	    strlen(file) < 7 ? "\t\t" : "\t", ok ? "OK" : "NOT OK");
	fflush(stderr);
}
#endif

/* print a file's info ala --list */
/* eg:
  compressed uncompressed  ratio uncompressed_name
      354841      1679360  78.8% /usr/pkgsrc/distfiles/libglade-2.0.1.tar
*/
static void
print_list(int fd, off_t in, const char *outfile, time_t ts)
{
	static int first = 1;
#ifndef SMALL
	static off_t in_tot, out_tot;
	u_int32_t crc;
#endif
	off_t out;
	int rv;

	if (first) {
#ifndef SMALL
		if (vflag)
			printf("method  crc     date  time  ");
#endif
		if (qflag == 0)
			printf("  compressed uncompressed  "
			       "ratio uncompressed_name\n");
	}
	first = 0;

	/* print totals? */
#ifndef SMALL
	if (fd == -1) {
		in = in_tot;
		out = out_tot;
	} else
#endif
	{
		/* read the last 4 bytes - this is the uncompressed size */
		rv = lseek(fd, (off_t)(-8), SEEK_END);
		if (rv != -1) {
			unsigned char buf[8];
			u_int32_t usize;

			if (read(fd, (char *)buf, sizeof(buf)) != sizeof(buf))
				maybe_err(1, "read of uncompressed size");
			usize = buf[4] | buf[5] << 8 | buf[6] << 16 | buf[7] << 24;
			out = (off_t)usize;
#ifndef SMALL
			crc = buf[0] | buf[1] << 8 | buf[2] << 16 | buf[3] << 24;
#endif
		}
	}

#ifndef SMALL
	if (vflag && fd == -1)
		printf("                            ");
	else if (vflag) {
		char *date = ctime(&ts);

		/* skip the day, 1/100th second, and year */
		date += 4;
		date[12] = 0;
		printf("%5s %08x %11s ", "defla"/*XXX*/, crc, date);
	}
	in_tot += in;
	out_tot += out;
#endif
	printf("%12llu %12llu ", (unsigned long long)in, (unsigned long long)out);
	print_ratio(in, out, stdout);
	printf(" %s\n", outfile);
}

/* display the usage of NetBSD gzip */
static void
usage(void)
{

	fprintf(stderr, "%s\n", gzip_version);
	fprintf(stderr,
    "usage: %s [-" OPT_LIST "] [<file> [<file> ...]]\n"
#ifndef SMALL
    " -c --stdout          write to stdout, keep original files\n"
    "    --to-stdout\n"
    " -d --decompress      uncompress files\n"
    "    --uncompress\n"
    " -f --force           force overwriting & compress links\n"
    " -h --help            display this help\n"
    " -n --no-name         don't save original file name or time stamp\n"
    " -N --name            save or restore original file name and time stamp\n"
    " -q --quiet           output no warnings\n"
    " -r --recursive       recursively compress files in directories\n"
    " -S .suf              use suffix .suf instead of .gz\n"
    "    --suffix .suf\n"
    " -t --test            test compressed file\n"
    " -v --verbose         print extra statistics\n"
    " -V --version         display program version\n"
    " -1 --fast            fastest (worst) compression\n"
    " -2 .. -8             set compression level\n"
    " -9 --best            best (slowest) compression\n",
#else
    ,
#endif
	    getprogname());
	exit(0);
}

/* display the version of NetBSD gzip */
static void
display_version(void)
{

	fprintf(stderr, "%s\n", gzip_version);
	exit(0);
}

#ifndef NO_BZIP2_SUPPORT
#include "unbzip2.c"
#endif
#ifndef NO_COMPRESS_SUPPORT
#include "zuncompress.c"
#endif
