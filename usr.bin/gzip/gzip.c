/*	$NetBSD: gzip.c,v 1.4 2003/12/23 15:02:40 mrg Exp $	*/

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
__RCSID("$NetBSD: gzip.c,v 1.4 2003/12/23 15:02:40 mrg Exp $");
#endif /* not lint */

/*
 * gzip.c -- GPL free gzip using zlib.
 *
 * Very minor portions of this code are (very loosely) derived from
 * the minigzip.c in the zlib distribution.
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

#ifndef GZ_SUFFIX
# define GZ_SUFFIX ".gz"
#endif

#define BUFLEN 4096

#define ORIG_NAME 0x08

/* Define this if you have the NetBSD gzopenfull(3) extension to zlib(3) */
#define HAVE_ZLIB_GZOPENFULL 0

static	const char	gzip_version[] = "NetBSD gzip 2.0";

static	char	gzipflags[3];		/* `w' or `r', possible with [1-9] */
static	int	cflag;			/* stdout mode */
static	int	dflag;			/* decompress mode */
static	int	fflag;			/* force mode */
static	int	nflag;			/* don't save name/timestamp */
static	int	Nflag;			/* don't restore name/timestamp */
static	int	qflag;			/* quiet mode */
static	int	rflag;			/* recursive mode */
static	int	tflag;			/* test */
static	int	vflag;			/* verbose mode */
static	const char *Sflag = GZ_SUFFIX;	/* suffix (.gz) */

static	int	suffix_len;		/* length of suffix; includes nul */
static	char	*newfile;		/* name of newly created file */
static	char	*infile;		/* name of file coming in */

static	void	maybe_err(int rv, const char *fmt, ...);
static	void	maybe_warn(const char *fmt, ...);
static	void	maybe_warnx(const char *fmt, ...);
static	void	usage(void);
static	void	display_version(void);
static	void	gz_compress(FILE *, gzFile);
static	off_t	gz_uncompress(gzFile, FILE *);
static	void	copymodes(const char *, struct stat *);
static	ssize_t	file_compress(char *);
static	ssize_t	file_uncompress(char *);
static	void	handle_pathname(char *);
static	void	handle_file(char *, struct stat *);
static	void	handle_dir(char *, struct stat *);
static	void	handle_stdin(void);
static	void	handle_stdout(void);
static	void	print_verbage(char *, char *, ssize_t, ssize_t);
static	void	print_test(char *, int);

int main(int, char *p[]);

static const struct option longopts[] = {
	{ "stdout",		no_argument,		0,	'c' },
	{ "to-stdout",		no_argument,		0,	'c' },
	{ "decompress",		no_argument,		0,	'd' },
	{ "uncompress",		no_argument,		0,	'd' },
	{ "force",		no_argument,		0,	'f' },
	{ "help",		no_argument,		0,	'h' },
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
	 * This is what else GNU gzip implements.  Maybe --list is
	 * useful, but --ascii isn't useful on NetBSD, and I don't
	 * care to have a --license.
	 */
	{ "ascii",		no_argument,		0,	'a' },
	{ "list",		no_argument,		0,	'l' },
	{ "license",		no_argument,		0,	'L' },
#endif
};

int
main(int argc, char **argv)
{
	const char *progname = getprogname();
	int ch;

	gzipflags[0] = 'w';
	gzipflags[1] = '\0';

	/*
	 * XXX
	 * handle being called `gunzip', `zcat' and `gzcat'
	 */
	if (strcmp(progname, "gunzip") == 0)
		dflag = 1;
	else if (strcmp(progname, "zcat") == 0 ||
		 strcmp(progname, "gzcat") == 0)
		dflag = cflag = 1;

	while ((ch = getopt_long(argc, argv, "cdfhHlnNqrS:tvV123456789",
				 longopts, NULL)) != -1)
		switch (ch) {
		case 'c':
			cflag = 1;
			break;
		case 'd':
			dflag = 1;
			break;
		case 'f':
			fflag = 1;
			break;
		case 'h':
		case 'H':
			usage();
			/* NOTREACHED */
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
		case 'V':
			display_version();
			/* NOTREACHED */
		case '1': case '2': case '3':
		case '4': case '5': case '6':
		case '7': case '8': case '9':
			gzipflags[1] = (char)ch;
			gzipflags[2] = '\0';
			break;
		}
	argv += optind;
	argc -= optind;
	if (dflag)
		gzipflags[0] = 'r';

	suffix_len = strlen(Sflag) + 1;

	if (argc == 0) {
		if (dflag)	/* stdin mode */
			handle_stdin();
		else		/* stdout mode */
			handle_stdout();
	} else {
		do {
			handle_pathname(argv[0]);
		} while (argv++, --argc);
	}
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
			if (tflag) {
				print_test(infile, 0);
				return (0);
			} else
				maybe_err(1, gzerror(in, &i));
		} else if (len == 0) {
			if (tflag)
				print_test(infile, 1);
			break;
		}

		size += len;

		/* don't write anything with -t */
		if (tflag)
			continue;

		if (fwrite(buf, 1, (unsigned)len, out) != (ssize_t)len)
			maybe_err(1, "failed fwrite");
	}
	if (gzclose(in) != Z_OK)
		maybe_err(1, "failed gzclose");

	return (size);
}

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

/*
 * compress the given file: create a corresponding .gz file and remove the
 * original.
 */
static ssize_t
file_compress(char *file)
{
	char outfile[MAXPATHLEN];
	FILE *in;
	gzFile out;
	struct stat isb, osb;
	ssize_t size;
	u_int32_t mtime = 0;

	if (cflag == 0) {
		(void)strncpy(outfile, file, MAXPATHLEN - suffix_len);
		outfile[MAXPATHLEN - suffix_len] = '\0';
		(void)strlcat(outfile, Sflag, sizeof(outfile));

		if (fflag == 0) {
			if (stat(outfile, &osb) == 0) {
				maybe_warnx("%s already exists -- skipping",
					      outfile);
				goto lose;
			}
		}
		if (stat(file, &isb) == 0) {
			if (isb.st_nlink > 1) {
				maybe_warnx("%s has %d other link%s -- "
					    "skipping", file, isb.st_nlink-1,
					    isb.st_nlink == 1 ? "" : "s");
				goto lose;
			}
			if (nflag == 0)
				mtime = (u_int32_t)isb.st_mtime;
		}
	}
	in = fopen(file, "r");
	if (in == 0)
		maybe_err(1, "can't fopen %s", file);

	if (cflag == 0) {
#if HAVE_ZLIB_GZOPENFULL
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
		copymodes(outfile, &isb);
	} else {
lose:
		size = 0;
		newfile = 0;
	}

	return (size);
}

/* uncompress the given file and remove the original */
static ssize_t
file_uncompress(char *file)
{
	struct stat isb, osb;
	char buf[MAXPATHLEN];
	char *outfile = buf, *s;
	FILE *out;
	gzFile in;
	off_t size;
	ssize_t len = strlen(file);

	if (cflag == 0) {
		s = &file[len - suffix_len + 1];
		if (strncmp(s, Sflag, suffix_len) == 0) {
			(void)strncpy(outfile, file, len - suffix_len + 1);
			outfile[len - suffix_len + 1] = '\0';
		} else
			maybe_err(1, "unknown suffix %s", s);

		/* gather the old name info */
		if (Nflag) {
			int fd;
			char header1[10], name[PATH_MAX + 1];

			fd = open(file, O_RDONLY);
			if (fd < 0)
				maybe_err(1, "can't open %s", file);
			if (read(fd, header1, 10) != 10)
				maybe_err(1, "can't read %s", file);

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
						if (asprintf(&outfile, "%s/%s",
						    dir, name) == -1)
							maybe_err(1, "malloc");
					}
				}
			}
			close(fd);
		}

		if (fflag == 0) {
			if (stat(outfile, &isb) == 0) {
				maybe_warnx("%s already exists -- skipping",
					    outfile);
				goto lose;
			}
			if (stat(file, &isb) == 0 && isb.st_nlink > 1) {
				maybe_warnx("%s has %d other link%s -- "
					    "skipping", file, isb.st_nlink-1,
					    isb.st_nlink == 1 ? "" : "s");
				goto lose;
			}
		}
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

	if ((size = gz_uncompress(in, out)) == 0)
		goto lose;

	/* if testing, or we uncompressed to stdout, this is all we need */
	if (tflag || cflag)
		return (size);

	/*
	 * if we create a file...
	 */
	if (cflag == 0) {
		/* close the file */
		if (fclose(out))
			maybe_err(1, "failed fclose");

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
		copymodes(outfile, &isb);
	}
	return (size);

lose:
	newfile = 0;
	return (0);
}

static void
handle_stdin(void)
{
	gzFile *file;

	if (fflag == 0 && isatty(STDIN_FILENO)) {
		maybe_warnx("standard input is a terminal -- ignoring");
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

	if (fflag == 0 && isatty(STDOUT_FILENO)) {
		maybe_warnx("standard output is a terminal -- ignoring");
		return;
	}
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
			memmove(&s[len], Sflag, suffix_len);
			path = s;
			goto retry;
		}
		maybe_warn("can't stat: %s", opath);
		goto out;
	}

	if (S_ISDIR(sb.st_mode)) {
		if (rflag)
			handle_dir(path, &sb);
		else
			maybe_warn("%s is a directory", path);
		goto out;
	}

	if (S_ISREG(sb.st_mode))
		handle_file(path, &sb);

out:
	if (s)
		free(s);
	return;
}

/* compress/decompress a file */
static void
handle_file(char *file, struct stat *sbp)
{
	ssize_t usize, gsize;

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

	if (vflag && !tflag)
		print_verbage(file, cflag == 0 ? newfile : 0, usize, gsize);
}

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

/* print compression statistics, and the new name (if there is one!) */
static void
print_verbage(char *file, char *nfile, ssize_t usize, ssize_t gsize)
{
	float percent = 100.0 - (100.0 * gsize / usize);

	fprintf(stderr, "%s:%s  %4.1f%%", file,
	    strlen(file) < 7 ? "\t\t" : "\t", percent);
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

/* display the usage of NetBSD gzip */
static void
usage(void)
{

	fprintf(stderr, "%s\n", gzip_version);
	fprintf(stderr,
    "Usage: %s [-cdfhnNqrStvV123456789] [<file> [<file> ...]]\n"
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
	    getprogname());
	fflush(stderr);
	exit(0);
}

/* display the version of NetBSD gzip */
static void
display_version(void)
{

	fprintf(stderr, "%s\n", gzip_version);
	fflush(stderr);
	exit(0);
}
